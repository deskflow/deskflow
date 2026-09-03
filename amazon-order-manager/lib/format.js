/* Orderkoll – delad presentationsformatering (svensk lokalisering). */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});

  /** Belopp: heltal utan decimaler, annars alltid två. "okänt" när värdet saknas. */
  function money(amount, currency) {
    if (!Number.isFinite(amount)) return 'okänt';
    const decimals = Number.isInteger(amount) ? 0 : 2;
    const suffix = !currency || currency === 'SEK' ? 'kr' : currency;
    return `${amount.toLocaleString('sv-SE', {
      minimumFractionDigits: decimals,
      maximumFractionDigits: decimals,
    })} ${suffix}`;
  }

  /** Tidsstämpel: "kl. 14:32" i dag, annars "3 sep 2026 kl. 14:32". */
  function timeLabel(iso) {
    if (!iso) return 'aldrig';
    const d = new Date(iso);
    if (Number.isNaN(d.getTime())) return 'okänt';
    const time = d.toLocaleTimeString('sv-SE', { hour: '2-digit', minute: '2-digit' });
    const sameDay = d.toDateString() === new Date().toDateString();
    return sameDay ? `kl. ${time}` : `${AOM.dates.formatSv(iso.slice(0, 10))} kl. ${time}`;
  }

  AOM.format = { money, timeLabel };
})(typeof self !== 'undefined' ? self : globalThis);
