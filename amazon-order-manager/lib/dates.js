/* Orderkoll – datum- och fristberäkningar (arbetsorder 5.4).
 * Tre separata fönster, aldrig sammanblandade:
 *   1. Amazons returpolicy  – läses av sidan, beräknas inte här.
 *   2. Ångerrätt            – 14 dagar, distansavtalslagen (2005:59).
 *   3. Reklamationsrätt     – 3 år, konsumentköplagen (2022:260), endast vid fel.
 */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});

  function pad2(n) {
    return String(n).padStart(2, '0');
  }

  function toDate(iso) {
    if (!iso) return null;
    const m = String(iso).match(/^(\d{4})-(\d{2})-(\d{2})/);
    if (!m) return null;
    return new Date(Date.UTC(Number(m[1]), Number(m[2]) - 1, Number(m[3])));
  }

  function toIso(date) {
    if (!date || Number.isNaN(date.getTime())) return null;
    return `${date.getUTCFullYear()}-${pad2(date.getUTCMonth() + 1)}-${pad2(date.getUTCDate())}`;
  }

  function addDays(iso, days) {
    const d = toDate(iso);
    if (!d) return null;
    d.setUTCDate(d.getUTCDate() + days);
    return toIso(d);
  }

  function addYears(iso, years) {
    const d = toDate(iso);
    if (!d) return null;
    d.setUTCFullYear(d.getUTCFullYear() + years);
    return toIso(d);
  }

  function todayIso(now) {
    const d = now ? new Date(now) : new Date();
    return `${d.getFullYear()}-${pad2(d.getMonth() + 1)}-${pad2(d.getDate())}`;
  }

  /** Hela dagar från a till b (b - a). Negativt = a ligger efter b. */
  function daysBetween(aIso, bIso) {
    const a = toDate(aIso);
    const b = toDate(bIso);
    if (!a || !b) return null;
    return Math.round((b.getTime() - a.getTime()) / 86400000);
  }

  function daysUntil(iso, now) {
    return daysBetween(todayIso(now), iso);
  }

  /**
   * Ångerfristen löper i 14 dagar och räknas från dagen efter mottagandet,
   * dvs. sista dagen är mottagandedatum + 1 + 14 - 1 = +14 dagar räknat från
   * dagen efter. Implementerat som mottagande + 15 dagar (sista hela dagen).
   * Utan känt mottagandedatum returneras null – aldrig ett antagande.
   */
  function angerrattDeadline(receivedIso) {
    if (!receivedIso) return null;
    return addDays(receivedIso, 15);
  }

  /** Reklamationsrätt: 3 år från mottagandet. Endast relevant vid fel. */
  function reklamationDeadline(receivedIso) {
    if (!receivedIso) return null;
    return addYears(receivedIso, 3);
  }

  /** Presumtionsregeln: fel som visar sig inom 6 månader antas ha funnits vid leverans. */
  function presumtionDeadline(receivedIso) {
    if (!receivedIso) return null;
    const d = toDate(receivedIso);
    d.setUTCMonth(d.getUTCMonth() + 6);
    return toIso(d);
  }

  /**
   * Bank-/kortreklamation. Ingen lagstadgad frist – dagar måste komma från
   * användarens inställningar (arbetsorder 9.2). Utan konfiguration: null.
   */
  function bankDisputeDeadline(purchaseIso, days) {
    if (!purchaseIso || !Number.isFinite(days)) return null;
    return addDays(purchaseIso, days);
  }

  const SV_MONTHS = ['jan', 'feb', 'mar', 'apr', 'maj', 'jun', 'jul', 'aug', 'sep', 'okt', 'nov', 'dec'];

  function formatSv(iso) {
    const d = toDate(iso);
    if (!d) return 'okänt';
    return `${d.getUTCDate()} ${SV_MONTHS[d.getUTCMonth()]} ${d.getUTCFullYear()}`;
  }

  function monthKey(iso) {
    return iso ? String(iso).slice(0, 7) : null;
  }

  function formatMonthSv(key) {
    if (!key) return '';
    const [y, m] = key.split('-');
    return `${SV_MONTHS[Number(m) - 1]} ${String(y).slice(2)}`;
  }

  /** Alla tre fönstren för en artikelrad, tydligt märkta och separerade. */
  function returnWindows(item, order, settings) {
    const received =
      (item && item.userReceivedConfirmedAt && String(item.userReceivedConfirmedAt).slice(0, 10)) ||
      (item && item.deliveredDate) ||
      null;
    const bankDays = (() => {
      const list = (settings && settings.bankDisputeDeadlines) || [];
      const hit = list.find(
        (entry) => order && order.paymentMethodLabel && entry.label === order.paymentMethodLabel
      );
      return hit && Number.isFinite(hit.days) ? hit.days : null;
    })();

    return {
      /** Amazons frivilliga policy – läst av sidan, aldrig beräknad här. */
      amazonPolicyDays: (item && item.returnWindow && item.returnWindow.amazonPolicyDays) ?? null,
      amazonPolicyDeadline: (item && item.returnWindow && item.returnWindow.amazonPolicyDeadline) ?? null,
      amazonPolicySourceText: (item && item.returnWindow && item.returnWindow.sourceText) ?? null,
      /** Lagstadgad ångerrätt, 14 dagar. */
      angerrattDeadline: angerrattDeadline(received),
      /** Lagstadgad reklamationsrätt, 3 år – visas bara vid fel. */
      reklamationDeadline: reklamationDeadline(received),
      presumtionDeadline: presumtionDeadline(received),
      bankDisputeDeadline: bankDays ? bankDisputeDeadline(order && order.orderDate, bankDays) : null,
      receivedDate: received,
    };
  }

  AOM.dates = {
    toIso,
    toDate,
    addDays,
    addYears,
    todayIso,
    daysBetween,
    daysUntil,
    angerrattDeadline,
    reklamationDeadline,
    presumtionDeadline,
    bankDisputeDeadline,
    formatSv,
    monthKey,
    formatMonthSv,
    returnWindows,
  };
})(typeof self !== 'undefined' ? self : globalThis);
