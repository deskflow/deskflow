/* Orderkoll – prisjämförelse mellan Amazon-marknader (arbetsorder 5.5).
 *
 * Matchningskedjan körs i ordning och stannar vid första godkända träffen:
 *   1. ASIN direkt   2. EAN/GTIN   3. Varumärke + modellnummer   4. Bildhash
 * Ingen träff ger "ej i katalogen" – aldrig en gissning.
 *
 * Modulen är ren logik: nätverk (fetchText), HTML-tolkning (parseHtml) och
 * bildhashning (imageHash) skickas in som beroenden, så att den kan testas
 * utan webbläsare och köras från ett content script med service workern som
 * hämtningskanal. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});
  const P = AOM.parse;

  const METHOD = { ASIN: 'ASIN', EAN: 'EAN', MPN: 'Modellnr', IMAGE: 'Bild' };
  const RESULT = {
    MATCH: 'match',
    VARIANT_UNCERTAIN: 'variant_uncertain',
    NOT_FOUND: 'not_found',
    ERROR: 'error',
  };

  const TITLE_SELECTORS = ['#productTitle', '#title span', 'h1 span#productTitle', 'h1'];
  const PRICE_SELECTORS = [
    '#corePrice_feature_div .a-offscreen',
    '#corePriceDisplay_desktop_feature_div .a-offscreen',
    '#price_inside_buybox',
    '#priceblock_ourprice',
    '#priceblock_dealprice',
    '.a-price .a-offscreen',
  ];
  const IMAGE_SELECTORS = ['#landingImage', '#imgBlkFront', '#main-image', '#imgTagWrapperId img'];
  const AVAILABILITY_SELECTORS = ['#availability span', '#availability', '#outOfStock'];

  function absoluteImageUrl(el) {
    if (!el) return null;
    return (
      el.getAttribute('data-old-hires') ||
      (() => {
        const dyn = el.getAttribute('data-a-dynamic-image');
        if (!dyn) return null;
        try {
          const keys = Object.keys(JSON.parse(dyn));
          return keys.length ? keys[0] : null;
        } catch (_err) {
          return null;
        }
      })() ||
      el.getAttribute('src')
    );
  }

  /** Plockar identitetsfälten ur en produktsida (egen eller hämtad). */
  function extractIdentity(doc, pageUrl) {
    const report = new P.ParseReport('produktsida');
    const identity = {};

    identity.asin = report.take(
      'asin',
      P.resolve(doc, [
        { via: 'input#ASIN', get: (d) => (d.querySelector('input#ASIN') || {}).value },
        { via: 'input[name=ASIN]', get: (d) => (d.querySelector('input[name="ASIN"]') || {}).value },
        { via: 'data-asin', get: (d) => (d.querySelector('[data-asin]') || { getAttribute: () => null }).getAttribute('data-asin') },
        { via: 'url', get: () => P.extractAsin(pageUrl) },
        { via: 'canonical', get: (d) => P.extractAsin((d.querySelector('link[rel="canonical"]') || {}).href) },
      ], (v) => P.extractAsin(v) || null)
    );

    identity.title = report.take(
      'title',
      P.resolve(doc, TITLE_SELECTORS.map((sel) => ({ via: sel, sel })))
    );

    const priceRaw = report.take(
      'price',
      P.resolve(doc, PRICE_SELECTORS.map((sel) => ({ via: sel, sel })))
    );
    const money = P.parseMoney(priceRaw);
    identity.price = money ? money.amount : null;
    identity.currency = money ? money.currency : null;
    identity.priceRaw = priceRaw;

    identity.mainImageUrl = report.take(
      'mainImageUrl',
      P.resolve(doc, IMAGE_SELECTORS.map((sel) => ({
        via: sel,
        get: (d) => absoluteImageUrl(d.querySelector(sel)),
      })))
    );

    identity.availability = report.take(
      'availability',
      P.resolve(doc, AVAILABILITY_SELECTORS.map((sel) => ({ via: sel, sel })))
    );

    const brandRaw = report.take(
      'brand',
      P.resolve(doc, [
        { via: '#bylineInfo', sel: '#bylineInfo' },
        { via: 'tr.po-brand td:last-child', sel: 'tr.po-brand td:last-child' },
        { via: 'label:Varumärke', get: (d) => {
          const hit = P.findByLabel(d, /^(varumärke|brand|märke)\b/i, { scope: 'td,th,span,div,li' });
          return hit ? hit.value : null;
        } },
      ])
    );
    identity.brand = P.normalizeBrand(brandRaw);
    identity.brandRaw = brandRaw;

    identity.ean = report.take(
      'ean',
      P.resolve(doc, [
        { via: 'label:EAN', get: (d) => {
          const hit = P.findByLabel(d, /\b(ean|gtin|upc|streckkod)\b/i, { scope: 'td,th,span,div,li' });
          return hit ? hit.value : null;
        } },
        { via: 'detail-bullets', get: (d) => {
          const li = Array.from(d.querySelectorAll('#detailBullets_feature_div li, #productDetails_techSpec_section_1 tr'));
          for (const el of li) {
            const t = P.text(el);
            if (/\b(ean|gtin|upc)\b/i.test(t)) return t;
          }
          return null;
        } },
      ], (v) => P.parseEan(v))
    );

    identity.mpn = report.take(
      'mpn',
      P.resolve(doc, [
        { via: 'label:Modellnummer', get: (d) => {
          const hit = P.findByLabel(d, /\b(modellnummer|artikelmodellnummer|model number|mpn|tillverkarens artikelnummer)\b/i, {
            scope: 'td,th,span,div,li',
          });
          return hit ? hit.value : null;
        } },
        { via: 'tr.po-model_name td:last-child', sel: 'tr.po-model_name td:last-child' },
      ])
    );

    identity.capacityTokens = P.capacityTokens(identity.title);
    identity.sourceUrl = pageUrl || null;
    identity.parseReport = report.toJSON();
    return identity;
  }

  /** Verifieringsregel innan en icke-ASIN-träff får kallas träff. */
  function verifyCandidate(source, candidate) {
    const reasons = [];
    if (source.brand && candidate.brand && source.brand !== candidate.brand) {
      reasons.push(`varumärke skiljer (${source.brandRaw} ≠ ${candidate.brandRaw})`);
    }
    if (!source.brand || !candidate.brand) {
      reasons.push('varumärke kunde inte läsas av på båda sidorna');
    }
    const a = (source.capacityTokens || []).join('|');
    const b = (candidate.capacityTokens || []).join('|');
    if (a !== b) reasons.push(`storlek/kapacitet skiljer (${a || '–'} ≠ ${b || '–'})`);
    return { ok: reasons.length === 0, reasons };
  }

  function marketplaceById(id) {
    return AOM.MARKETPLACES.find((m) => m.id === id) || null;
  }

  function isMissingPage(doc, html) {
    const t = P.text(doc.querySelector('title')) + ' ' + String(html || '').slice(0, 2000);
    return /Sidan hittades inte|Page Not Found|Seite nicht gefunden|dogs of Amazon|Beklager/i.test(t);
  }

  function searchUrl(domain, query) {
    return `https://${domain}/s?k=${encodeURIComponent(query)}`;
  }

  function parseSearchResults(doc, limit) {
    const nodes = Array.from(
      doc.querySelectorAll('div[data-asin][data-component-type="s-search-result"], div.s-result-item[data-asin]')
    );
    const out = [];
    for (const node of nodes) {
      const asin = node.getAttribute('data-asin');
      if (!asin || asin.length !== 10) continue;
      out.push({ asin, title: P.text(node.querySelector('h2')) || null });
      if (out.length >= (limit || 5)) break;
    }
    return out;
  }

  /**
   * Kör hela kedjan mot en marknad.
   * deps: { fetchText(url), parseHtml(html), imageHash(url) }
   */
  async function matchOnDomain(source, marketId, deps, options = {}) {
    const market = marketplaceById(marketId);
    if (!market) return { marketId, status: RESULT.ERROR, error: 'okänd marknad' };
    const domain = market.domain;
    const threshold = options.imageMatchThreshold ?? 0.85;

    const loadProduct = async (asin) => {
      const url = `https://${domain}/dp/${asin}?language=sv_SE`;
      const res = await deps.fetchText(url);
      const doc = deps.parseHtml(res.text);
      if (isMissingPage(doc, res.text)) return null;
      const identity = extractIdentity(doc, res.url || url);
      if (!identity.title) return null;
      return identity;
    };

    const finish = (method, candidate, extra) =>
      Object.assign(
        {
          marketId,
          domain,
          label: market.label,
          method,
          status: RESULT.MATCH,
          url: candidate.sourceUrl,
          title: candidate.title,
          price: candidate.price,
          currency: candidate.currency || market.currency,
          availability: candidate.availability,
        },
        extra || {}
      );

    try {
      // 1. ASIN direkt.
      if (source.asin) {
        const candidate = await loadProduct(source.asin);
        if (candidate) return finish(METHOD.ASIN, candidate, { confidence: 1 });
      }

      // 2. EAN/GTIN.
      if (source.ean) {
        const res = await deps.fetchText(searchUrl(domain, source.ean));
        const hits = parseSearchResults(deps.parseHtml(res.text), 3);
        for (const hit of hits) {
          const candidate = await loadProduct(hit.asin);
          if (!candidate) continue;
          const verdict = verifyCandidate(source, candidate);
          return finish(METHOD.EAN, candidate, {
            confidence: verdict.ok ? 0.95 : 0.6,
            status: verdict.ok ? RESULT.MATCH : RESULT.VARIANT_UNCERTAIN,
            verifyReasons: verdict.reasons,
          });
        }
      }

      // 3. Varumärke + modellnummer.
      if (source.brandRaw && source.mpn) {
        const res = await deps.fetchText(searchUrl(domain, `${source.brandRaw} ${source.mpn}`));
        const hits = parseSearchResults(deps.parseHtml(res.text), 3);
        for (const hit of hits) {
          const candidate = await loadProduct(hit.asin);
          if (!candidate) continue;
          const verdict = verifyCandidate(source, candidate);
          return finish(METHOD.MPN, candidate, {
            confidence: verdict.ok ? 0.9 : 0.55,
            status: verdict.ok ? RESULT.MATCH : RESULT.VARIANT_UNCERTAIN,
            verifyReasons: verdict.reasons,
          });
        }
      }

      // 4. Perceptuell bildhash.
      if (source.mainImageUrl && source.title && typeof deps.imageHash === 'function') {
        const sourceHash = await deps.imageHash(source.mainImageUrl);
        if (sourceHash) {
          const query = [source.brandRaw, source.title].filter(Boolean).join(' ').slice(0, 120);
          const res = await deps.fetchText(searchUrl(domain, query));
          const hits = parseSearchResults(deps.parseHtml(res.text), 4);
          let best = null;
          for (const hit of hits) {
            const candidate = await loadProduct(hit.asin);
            if (!candidate || !candidate.mainImageUrl) continue;
            const hash = await deps.imageHash(candidate.mainImageUrl);
            const similarity = AOM.imageHash.similarity(sourceHash, hash);
            if (similarity === null) continue;
            if (!best || similarity > best.similarity) best = { candidate, similarity };
          }
          if (best && best.similarity >= threshold) {
            const verdict = verifyCandidate(source, best.candidate);
            return finish(METHOD.IMAGE, best.candidate, {
              confidence: best.similarity,
              status: verdict.ok ? RESULT.MATCH : RESULT.VARIANT_UNCERTAIN,
              verifyReasons: verdict.reasons,
            });
          }
        }
      }

      return { marketId, domain, label: market.label, status: RESULT.NOT_FOUND, method: null };
    } catch (err) {
      return {
        marketId,
        domain,
        label: market.label,
        status: RESULT.ERROR,
        error: String((err && err.message) || err),
        wall: err && err.wall ? err.wall : null,
      };
    }
  }

  /** Kör alla valda marknader sekventiellt (aldrig parallella anrop mot Amazon). */
  async function compare(source, marketIds, deps, options = {}) {
    const results = [];
    for (const id of marketIds || []) {
      /* eslint-disable no-await-in-loop */
      results.push(await matchOnDomain(source, id, deps, options));
    }
    return results;
  }

  /** Lägger på omräknat pris i SEK när kurser finns. Aldrig uppskattat. */
  function withConverted(results, rates, targetCurrency) {
    return (results || []).map((r) => {
      if (!r || r.price === null || r.price === undefined || !r.currency) return r;
      const converted = AOM.fx.convert(r.price, r.currency, targetCurrency || 'SEK', rates);
      return Object.assign({}, r, {
        convertedPrice: converted === null ? null : Math.round(converted * 100) / 100,
        convertedCurrency: converted === null ? null : targetCurrency || 'SEK',
      });
    });
  }

  AOM.priceMatch = {
    METHOD,
    RESULT,
    extractIdentity,
    verifyCandidate,
    matchOnDomain,
    compare,
    withConverted,
    parseSearchResults,
    isMissingPage,
    searchUrl,
  };
})(typeof self !== 'undefined' ? self : globalThis);
