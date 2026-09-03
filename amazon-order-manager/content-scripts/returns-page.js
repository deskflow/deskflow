/* Orderkoll – parsning av "Returer & beställningar" (arbetsorder 5.3).
 * Kompletterar avvikelselogiken genom att upptäcka registrerade
 * återbetalningar och returer. Användarens egen kryssruta vinner alltid vid
 * konflikt – den här sidan förifyller bara. */
(function () {
  'use strict';
  const AOM = self.AOM;
  const P = AOM.parse;

  const REFUND_RE = /(återbetal\w*|refund(?:ed|ing)?|pengarna tillbaka|kreditering)/i;
  const RETURN_RE = /(retur\w*|return(?:ed|ing)?|skicka tillbaka|returetikett)/i;
  const COMPLETED_RE = /(utfärdad|slutförd|genomförd|klar|mottagen av amazon|refunded|issued)/i;

  function rowContainers() {
    const selectors = [
      '[data-order-id]',
      '.order-card',
      '.a-box-group',
      '.your-orders-content-container .a-box',
      'li.order',
    ];
    for (const sel of selectors) {
      const els = Array.from(document.querySelectorAll(sel)).filter((el) =>
        P.ORDER_ID_RE.test(P.text(el))
      );
      if (els.length) return { els, via: sel };
    }
    const fallback = Array.from(document.querySelectorAll('div,li,section')).filter((el) => {
      if (!P.ORDER_ID_RE.test(P.text(el))) return false;
      return !Array.from(el.children).some((c) => P.ORDER_ID_RE.test(P.text(c)));
    });
    return { els: fallback, via: 'fallback:minsta-behållare' };
  }

  function parsePage() {
    const report = new P.ParseReport('returer');
    const { els, via } = rowContainers();
    report.used('rows', via);
    const records = [];
    for (const el of els) {
      const text = P.text(el);
      const orderId = P.extractOrderId(text) || (el.getAttribute && el.getAttribute('data-order-id'));
      if (!orderId) continue;
      const refundHit = text.match(REFUND_RE);
      const returnHit = text.match(RETURN_RE);
      const evidence = (() => {
        const idx = refundHit ? text.indexOf(refundHit[0]) : returnHit ? text.indexOf(returnHit[0]) : -1;
        return idx >= 0 ? text.slice(Math.max(0, idx - 60), idx + 120) : null;
      })();
      records.push({
        orderId,
        refundDetected: !!refundHit,
        refundCompleted: !!(refundHit && COMPLETED_RE.test(text)),
        returnRequestedDetected: !!returnHit,
        evidenceText: evidence,
        sourceUrl: location.href,
        seenAt: new Date().toISOString(),
      });
    }
    report.log();
    return records;
  }

  function run(reason) {
    const wall = P.detectWall(location.href);
    if (wall) {
      chrome.runtime.sendMessage({ type: AOM.MSG.RETURNS_PARSED, error: wall, records: [] });
      return;
    }
    const records = parsePage();
    console.info(`[Orderkoll] returer (${reason}): ${records.length} poster`, records);
    chrome.runtime.sendMessage({
      type: AOM.MSG.RETURNS_PARSED,
      records,
      pageUrl: location.href,
      parsedAt: new Date().toISOString(),
    });
  }

  chrome.runtime.onMessage.addListener((msg, _sender, respond) => {
    if (msg && msg.type === AOM.MSG.RESCAN) {
      run('manuell synk');
      respond({ ok: true });
      return true;
    }
    return undefined;
  });

  self.AOM.returnsPage = { parsePage };
  run('sidladdning');
})();
