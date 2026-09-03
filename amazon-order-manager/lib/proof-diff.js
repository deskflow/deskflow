/* Orderkoll – diff mellan sparat bevis och dagens liveversion (arbetsorder 5.6).
 * Körs i en extension-sida där DOMParser finns. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});
  const P = AOM.parse;

  function specRows(doc) {
    const rows = new Map();
    const nodes = doc.querySelectorAll(
      '#productDetails_techSpec_section_1 tr, #productDetails_detailBullets_sections1 tr, table.a-keyvalue tr, #detailBullets_feature_div li'
    );
    for (const node of nodes) {
      const cells = node.querySelectorAll('th,td,span.a-text-bold');
      if (cells.length >= 2) {
        const key = P.text(cells[0]).replace(/[:\s]+$/, '');
        const value = P.text(cells[1]);
        if (key) rows.set(key, value);
        continue;
      }
      const t = P.text(node);
      const m = t.match(/^([^:]{2,60}):\s*(.+)$/);
      if (m) rows.set(m[1].trim(), m[2].trim());
    }
    return rows;
  }

  function summarize(doc, url) {
    const identity = AOM.priceMatch.extractIdentity(doc, url);
    return {
      title: identity.title,
      price: identity.price,
      currency: identity.currency,
      mainImageUrl: identity.mainImageUrl,
      availability: identity.availability,
      specs: specRows(doc),
    };
  }

  /** Jämför två HTML-strängar (sparad vs live) och returnerar skillnader. */
  function diffHtml(savedHtml, liveHtml, url) {
    const parser = new DOMParser();
    const saved = summarize(parser.parseFromString(savedHtml, 'text/html'), url);
    const live = summarize(parser.parseFromString(liveHtml, 'text/html'), url);
    const changes = [];

    const compare = (field, label, a, b) => {
      const av = a === null || a === undefined ? null : String(a);
      const bv = b === null || b === undefined ? null : String(b);
      if (av !== bv) changes.push({ field, label, saved: av, live: bv });
    };
    compare('title', 'Titel', saved.title, live.title);
    compare('price', 'Pris', saved.price, live.price);
    compare('mainImageUrl', 'Huvudbild', saved.mainImageUrl, live.mainImageUrl);
    compare('availability', 'Lagerstatus', saved.availability, live.availability);

    const keys = new Set([...saved.specs.keys(), ...live.specs.keys()]);
    for (const key of keys) {
      const a = saved.specs.get(key) ?? null;
      const b = live.specs.get(key) ?? null;
      if (a !== b) changes.push({ field: `spec:${key}`, label: `Specifikation: ${key}`, saved: a, live: b });
    }
    return { changes, saved, live };
  }

  AOM.proofDiff = { diffHtml, summarize, specRows };
})(typeof self !== 'undefined' ? self : globalThis);
