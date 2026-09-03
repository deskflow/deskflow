/* Orderkoll – valutaomräkning via ECB:s dagliga referenskurser.
 * Källa: https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml
 * Kurserna uttrycks mot EUR och uppdateras en gång per bankdag. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});
  const URL_DAILY = 'https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml';

  /** Plockar ut kurserna ur ECB:s XML utan DOM-beroende (fungerar i SW). */
  function parseEcbXml(xml) {
    const rates = { EUR: 1 };
    const re = /currency=['"]([A-Z]{3})['"]\s+rate=['"]([\d.]+)['"]/g;
    let m;
    while ((m = re.exec(xml))) rates[m[1]] = Number(m[2]);
    const timeMatch = xml.match(/time=['"](\d{4}-\d{2}-\d{2})['"]/);
    return { rates, date: timeMatch ? timeMatch[1] : null };
  }

  async function fetchRates() {
    const res = await fetch(URL_DAILY);
    if (!res.ok) throw new Error(`ECB svarade HTTP ${res.status}`);
    const xml = await res.text();
    const parsed = parseEcbXml(xml);
    if (!parsed.date || Object.keys(parsed.rates).length < 5) {
      throw new Error('Kunde inte tolka ECB-svaret');
    }
    return parsed;
  }

  /** Cachar dagens kurser i chrome.storage; hämtar bara om datumet är nytt. */
  async function getRates() {
    const cached = await AOM.storage.get(AOM.STORAGE_KEYS.FX, null);
    const today = AOM.dates.todayIso();
    if (cached && cached.date && cached.fetchedFor === today) return cached;
    try {
      const fresh = await fetchRates();
      const record = Object.assign({}, fresh, { fetchedFor: today, fetchedAt: new Date().toISOString() });
      await AOM.storage.set(AOM.STORAGE_KEYS.FX, record);
      return record;
    } catch (err) {
      if (cached) return Object.assign({}, cached, { stale: true, error: String(err.message || err) });
      throw err;
    }
  }

  /** Returnerar null när någon av valutorna saknas – räknar aldrig på gissningar. */
  function convert(amount, from, to, rates) {
    if (!Number.isFinite(amount) || !from || !to || !rates) return null;
    if (from === to) return amount;
    const rFrom = rates[from];
    const rTo = rates[to];
    if (!Number.isFinite(rFrom) || !Number.isFinite(rTo)) return null;
    const inEur = amount / rFrom;
    return inEur * rTo;
  }

  AOM.fx = { URL_DAILY, parseEcbXml, fetchRates, getRates, convert };
})(typeof self !== 'undefined' ? self : globalThis);
