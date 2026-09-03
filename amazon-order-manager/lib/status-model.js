/* Orderkoll – statusmodell (arbetsorder 5.1).
 * Amazons fritext normaliseras till exakt fyra statusar. Känns texten inte
 * igen returneras status: null – "okänt" är ett giltigt svar, en gissning är
 * det inte. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});
  const S = AOM.STATUS;

  /* Ordningen är signifikant: avbruten och levererad testas före på väg,
   * eftersom "Levererad" ofta står tillsammans med leveransformuleringar. */
  const RULES = [
    {
      status: S.CANCELLED,
      via: 'avbruten',
      re: /\b(avbrut\w*|inställ\w*|makuler\w*|annuller\w*|cancell?ed)\b/i,
    },
    {
      status: S.DELIVERED,
      via: 'levererad',
      re: /\b(levererad\w*|levererades|har levererats|utlämnad\w*|uthämtad\w*|hämtad\w*|delivered)\b/i,
    },
    {
      status: S.ON_WAY,
      via: 'pa_vag',
      re: /(på väg|har skickats|skickad\w*|skickas i dag|ute för leverans|i transit|anländer|beräknad ankomst|kommer i dag|kommer imorgon|shipped|out for delivery)/i,
    },
    {
      status: S.NOT_SHIPPED,
      via: 'ej_skickad',
      re: /(inte skickad|ej skickad|har inte skickats|förbereder|förbereds|behandlas|bearbetas|väntar på|beräknad leverans|inte behandlad|not yet shipped|preparing for shipment)/i,
    },
  ];

  /** Returnerar { status, via } – status är null när texten inte känns igen. */
  function normalizeStatus(raw) {
    const s = (raw || '').toString();
    if (!s.trim()) return { status: null, via: null };
    for (const rule of RULES) {
      if (rule.re.test(s)) return { status: rule.status, via: rule.via };
    }
    return { status: null, via: null };
  }

  function label(status) {
    return AOM.STATUS_LABEL[status] || 'Okänd status';
  }

  /** Stabil nyckel per artikelrad inom en order. */
  function lineKey(item, index) {
    return `${(item && item.asin) || 'na'}:${index}`;
  }

  /** Sammansatt indikator för en order med blandade artikelstatusar. */
  function compositeLabel(items) {
    const list = items || [];
    if (!list.length) return { text: 'Inga artiklar', status: null };
    const counts = list.reduce((acc, it) => {
      const key = it.status || 'okand';
      acc[key] = (acc[key] || 0) + 1;
      return acc;
    }, {});
    const distinct = Object.keys(counts);
    if (distinct.length === 1) {
      return { text: label(distinct[0] === 'okand' ? null : distinct[0]), status: distinct[0] === 'okand' ? null : distinct[0] };
    }
    const delivered = counts[S.DELIVERED] || 0;
    if (delivered) {
      return { text: `${delivered} av ${list.length} levererade`, status: 'blandad' };
    }
    const onWay = counts[S.ON_WAY] || 0;
    if (onWay) return { text: `${onWay} av ${list.length} på väg`, status: 'blandad' };
    return { text: 'Blandad status', status: 'blandad' };
  }

  /** "Att bekräfta"-kön: levererad enligt Amazon men inte kvitterad av dig. */
  function needsReceiptConfirmation(item) {
    return !!item && item.status === S.DELIVERED && !item.userReceivedConfirmed;
  }

  /** Alla artikelrader i alla ordrar som väntar på mottagningsbekräftelse. */
  function pendingConfirmations(orders) {
    const out = [];
    for (const order of orders || []) {
      (order.lineItems || []).forEach((item, index) => {
        if (needsReceiptConfirmation(item)) {
          out.push({ order, item, index, lineKey: item.lineKey || lineKey(item, index) });
        }
      });
    }
    return out;
  }

  AOM.status = {
    normalizeStatus,
    label,
    lineKey,
    compositeLabel,
    needsReceiptConfirmation,
    pendingConfirmations,
    RULES,
  };
})(typeof self !== 'undefined' ? self : globalThis);
