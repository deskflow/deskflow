/* Orderkoll – avvikelselogik (arbetsorder 5.3).
 *
 * En artikelrad flaggas när ALLA tre gäller:
 *   1. raden är inte levererad (ej_skickad / pa_vag) ELLER leveransen är försenad,
 *   2. en återbetalning är registrerad på ordern,
 *   3. användaren har INTE kryssat "jag har begärt retur/återbetalning".
 *
 * Punkt 3 är definitiv: användarens kryss slår alltid skrapningen av
 * returer-sidan, oavsett vad den visar. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});
  const S = AOM.STATUS;

  const TYPE = {
    REFUND_WITHOUT_REQUEST: 'refund_without_return_request',
    DELAYED_DELIVERY: 'delayed_delivery',
  };

  function isDelayed(item, todayIso) {
    if (!item || !item.estimatedDeliveryDate) return false;
    if (item.status === S.DELIVERED || item.status === S.CANCELLED) return false;
    const diff = AOM.dates.daysBetween(item.estimatedDeliveryDate, todayIso);
    return Number.isFinite(diff) && diff > 0;
  }

  /**
   * Utvärderar en order mot dess återbetalningspost.
   * refundRecord: { refundDetected, returnRequestedDetected, evidenceText } | undefined
   */
  function evaluateOrder(order, refundRecord, opts = {}) {
    const today = opts.todayIso || AOM.dates.todayIso(opts.now);
    const out = [];
    if (!order) return out;

    const refundDetected = !!(refundRecord && refundRecord.refundDetected);
    const userSaysRequested = !!order.userConfirmedReturnOrRefundRequested;

    (order.lineItems || []).forEach((item, index) => {
      const lineKey = item.lineKey || AOM.status.lineKey(item, index);
      const notDelivered = item.status === S.NOT_SHIPPED || item.status === S.ON_WAY;
      const delayed = isDelayed(item, today);

      if (refundDetected && !userSaysRequested && (notDelivered || delayed)) {
        out.push({
          type: TYPE.REFUND_WITHOUT_REQUEST,
          orderId: order.orderId,
          lineKey,
          sellerId: order.sellerId || null,
          title: item.title || null,
          status: item.status || null,
          delayed,
          detectedAt: new Date().toISOString(),
          reason: delayed
            ? 'Återbetalning registrerad på en försenad leverans utan att du begärt retur.'
            : 'Återbetalning registrerad på en artikel som inte är levererad, utan att du begärt retur.',
          evidenceText: (refundRecord && refundRecord.evidenceText) || null,
        });
      } else if (delayed && !refundDetected) {
        out.push({
          type: TYPE.DELAYED_DELIVERY,
          orderId: order.orderId,
          lineKey,
          sellerId: order.sellerId || null,
          title: item.title || null,
          status: item.status || null,
          delayed: true,
          detectedAt: new Date().toISOString(),
          reason: `Beräknad leverans ${AOM.dates.formatSv(item.estimatedDeliveryDate)} har passerat.`,
        });
      }
    });
    return out;
  }

  /** Kör över hela beståndet. Returnerar { anomalies, sellerCounts }. */
  function evaluateAll(orders, refundRecords, opts = {}) {
    const list = Array.isArray(orders) ? orders : Object.values(orders || {});
    const refunds = refundRecords || {};
    const anomalies = [];
    for (const order of list) {
      anomalies.push(...evaluateOrder(order, refunds[order.orderId], opts));
    }
    const sellerCounts = {};
    for (const a of anomalies) {
      if (a.type !== TYPE.REFUND_WITHOUT_REQUEST || !a.sellerId) continue;
      sellerCounts[a.sellerId] = (sellerCounts[a.sellerId] || 0) + 1;
    }
    return { anomalies, sellerCounts };
  }

  AOM.anomaly = { TYPE, evaluateOrder, evaluateAll, isDelayed };
})(typeof self !== 'undefined' ? self : globalThis);
