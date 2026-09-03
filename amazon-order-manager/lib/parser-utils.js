/* Orderkoll – delade parsningshjälpare.
 *
 * Bärande princip (arbetsorder 0.1 och 3): ingenting gissas. Varje fält
 * plockas via en lista kandidatstrategier; den som lyckades loggas i en
 * ParseReport så att framtida underhåll ser vilken fallback som bär vikten.
 * Hittas inget returneras null – aldrig ett påhittat värde. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});

  const NBSP = /[     - ]/g;

  /** Normaliserad textinnehåll: nbsp -> mellanslag, kollapsad whitespace. */
  function text(el) {
    if (!el) return '';
    const raw = typeof el === 'string' ? el : el.textContent || '';
    return raw.replace(NBSP, ' ').replace(/\s+/g, ' ').trim();
  }

  /** Samma normalisering men behåller radbrytningar (för snapshot/diff). */
  function blockText(el) {
    if (!el) return '';
    const raw = typeof el === 'string' ? el : el.textContent || '';
    return raw
      .replace(NBSP, ' ')
      .split('\n')
      .map((line) => line.replace(/\s+/g, ' ').trim())
      .filter(Boolean)
      .join('\n');
  }

  /** Första matchande elementet ur en lista selektorer. */
  function queryFirst(rootEl, selectors) {
    if (!rootEl) return null;
    for (const sel of selectors) {
      let el = null;
      try {
        el = rootEl.querySelector(sel);
      } catch (_err) {
        continue; // ogiltig selector ska inte fälla hela parsningen
      }
      if (el) return { el, via: sel };
    }
    return null;
  }

  function queryAll(rootEl, selectors) {
    if (!rootEl) return { els: [], via: null };
    for (const sel of selectors) {
      let els = [];
      try {
        els = Array.from(rootEl.querySelectorAll(sel));
      } catch (_err) {
        continue;
      }
      if (els.length) return { els, via: sel };
    }
    return { els: [], via: null };
  }

  /**
   * Kör kandidatstrategier i ordning.
   * candidates: [{ via, sel }] eller [{ via, get(rootEl) }]
   * Returnerar { value, via } eller { value: null, via: null, tried: [...] }.
   */
  function resolve(rootEl, candidates, transform) {
    const tried = [];
    for (const cand of candidates) {
      tried.push(cand.via);
      let raw = null;
      try {
        if (typeof cand.get === 'function') {
          raw = cand.get(rootEl);
        } else if (cand.sel) {
          const hit = queryFirst(rootEl, [cand.sel]);
          raw = hit ? (cand.attr ? hit.el.getAttribute(cand.attr) : text(hit.el)) : null;
        }
      } catch (_err) {
        raw = null;
      }
      if (raw === null || raw === undefined || raw === '') continue;
      const value = transform ? transform(raw) : raw;
      if (value === null || value === undefined || value === '') continue;
      return { value, via: cand.via };
    }
    return { value: null, via: null, tried };
  }

  /**
   * Hittar värdet som hör till en etikett ("Ordersumma", "Antal:", ...).
   * Strategier: elementets egen text efter etiketten, nästa syskon, förälderns
   * nästa syskon, och tabellrad (th -> td).
   */
  function findByLabel(rootEl, labelRegex, opts = {}) {
    if (!rootEl) return null;
    const scope = opts.scope || '*';
    let nodes;
    try {
      nodes = Array.from(rootEl.querySelectorAll(scope));
    } catch (_err) {
      return null;
    }
    for (const node of nodes) {
      if (node.children.length > 2) continue; // vill ha bladnära noder
      const own = text(node);
      if (!own || own.length > 120) continue;
      if (!labelRegex.test(own)) continue;

      const after = own.replace(labelRegex, '').replace(/^[\s:–-]+/, '').trim();
      if (after) return { value: after, via: 'label-inline' };

      const sib = node.nextElementSibling;
      if (sib && text(sib)) return { value: text(sib), via: 'label-sibling' };

      if (node.tagName === 'TH' && node.parentElement) {
        const td = node.parentElement.querySelector('td');
        if (td && text(td)) return { value: text(td), via: 'label-table-row' };
      }
      const parentSib = node.parentElement && node.parentElement.nextElementSibling;
      if (parentSib && text(parentSib)) {
        return { value: text(parentSib), via: 'label-parent-sibling' };
      }
    }
    return null;
  }

  const CURRENCY_PATTERNS = [
    { re: /\bkr\b|\bSEK\b/i, currency: 'SEK' },
    { re: /€|\bEUR\b/i, currency: 'EUR' },
    { re: /£|\bGBP\b/i, currency: 'GBP' },
    { re: /\$|\bUSD\b/i, currency: 'USD' },
  ];

  /**
   * Tolkar prissträngar från olika Amazon-marknader.
   * "1 234,56 kr", "SEK 1.234,56", "€12,99", "£9.99", "1,234.56" hanteras.
   * Returnerar { amount, currency, raw } eller null. Gissar aldrig valuta:
   * saknas valutatecken blir currency null.
   */
  function parseMoney(input) {
    if (input === null || input === undefined) return null;
    const raw = String(input).replace(NBSP, ' ').trim();
    if (!raw) return null;

    const numMatch = raw.match(/-?\d[\d\s.,]*\d|-?\d/);
    if (!numMatch) return null;
    let numStr = numMatch[0].replace(/\s/g, '');

    const lastComma = numStr.lastIndexOf(',');
    const lastDot = numStr.lastIndexOf('.');
    const lastSep = Math.max(lastComma, lastDot);
    if (lastSep === -1) {
      numStr = numStr;
    } else {
      const decimals = numStr.length - lastSep - 1;
      const sepChar = numStr[lastSep];
      const otherChar = sepChar === ',' ? '.' : ',';
      if (decimals === 1 || decimals === 2) {
        // Sista separatorn är decimaltecken.
        numStr =
          numStr.slice(0, lastSep).split(otherChar).join('').split(sepChar).join('') +
          '.' +
          numStr.slice(lastSep + 1);
      } else {
        // Alla separatorer är tusentalsavgränsare.
        numStr = numStr.replace(/[.,]/g, '');
      }
    }
    const amount = Number(numStr);
    if (!Number.isFinite(amount)) return null;

    let currency = null;
    for (const c of CURRENCY_PATTERNS) {
      if (c.re.test(raw)) {
        currency = c.currency;
        break;
      }
    }
    return { amount, currency, raw };
  }

  const MONTHS_SV = {
    januari: 1, jan: 1,
    februari: 2, feb: 2,
    mars: 3, mar: 3,
    april: 4, apr: 4,
    maj: 5,
    juni: 6, jun: 6,
    juli: 7, jul: 7,
    augusti: 8, aug: 8,
    september: 9, sep: 9, sept: 9,
    oktober: 10, okt: 10,
    november: 11, nov: 11,
    december: 12, dec: 12,
  };

  function pad2(n) {
    return String(n).padStart(2, '0');
  }

  /**
   * Tolkar svenska datumsträngar till 'YYYY-MM-DD'.
   * "3 september 2026", "3 sep. 2026", "2026-09-03", "3 september" (utan år
   * -> null, vi gissar inte årtal). refYear kan skickas in när sidan själv
   * anger året i närliggande kontext.
   */
  function parseSwedishDate(input, opts = {}) {
    if (!input) return null;
    const s = String(input).replace(NBSP, ' ').toLowerCase();

    const iso = s.match(/(\d{4})-(\d{2})-(\d{2})/);
    if (iso) return `${iso[1]}-${iso[2]}-${iso[3]}`;

    const named = s.match(
      /(\d{1,2})\s*\.?\s*([a-zåäö]+)\.?(?:\s+(\d{4}))?/
    );
    if (named) {
      const day = Number(named[1]);
      const month = MONTHS_SV[named[2]];
      const year = named[3] ? Number(named[3]) : opts.refYear || null;
      if (month && day >= 1 && day <= 31 && year) {
        return `${year}-${pad2(month)}-${pad2(day)}`;
      }
      if (month && !year) return null; // årtal gissas aldrig
    }

    const numeric = s.match(/(\d{1,2})\/(\d{1,2})(?:[\/\s](\d{4}))?/);
    if (numeric && numeric[3]) {
      return `${numeric[3]}-${pad2(Number(numeric[2]))}-${pad2(Number(numeric[1])) }`;
    }
    return null;
  }

  /** ASIN ur en produkt-URL eller ett attribut. */
  function extractAsin(input) {
    if (!input) return null;
    const s = String(input);
    const m =
      s.match(/\/(?:dp|gp\/product|gp\/aw\/d|product|dp\/product)\/([A-Z0-9]{10})(?=[/?#]|$)/i) ||
      s.match(/[?&]asin=([A-Z0-9]{10})\b/i) ||
      s.match(/^([A-Z0-9]{10})$/);
    return m ? m[1].toUpperCase() : null;
  }

  /* Ordernummer får sitta direkt efter en etikett utan mellanslag
   * ("Ordernummer404-1234567-1234567"), därför lookaround i stället för \\b. */
  const ORDER_ID_RE = /(?<![\d-])\d{3}-\d{7}-\d{7}(?![\d-])/;

  function extractOrderId(input) {
    if (!input) return null;
    const m = String(input).replace(NBSP, ' ').match(ORDER_ID_RE);
    return m ? m[0] : null;
  }

  /** Säljar-ID ur en säljarlänk (/sp?seller=A1234...). */
  function extractSellerId(input) {
    if (!input) return null;
    const m = String(input).match(/[?&](?:seller|sellerID|merchantId)=([A-Z0-9]{5,20})/i);
    return m ? m[1] : null;
  }

  /** "Antal: 2", "2 st", "2" -> 2. Returnerar null om inget tal finns. */
  function parseQuantity(input) {
    if (input === null || input === undefined) return null;
    const m = String(input).replace(NBSP, ' ').match(/(\d{1,4})/);
    if (!m) return null;
    const n = Number(m[1]);
    return Number.isInteger(n) && n > 0 ? n : null;
  }

  /** EAN/GTIN: 8, 12, 13 eller 14 siffror. */
  function parseEan(input) {
    if (!input) return null;
    const digits = String(input).replace(/[^\d]/g, '');
    if ([8, 12, 13, 14].includes(digits.length)) return digits;
    return null;
  }

  /**
   * Kapacitets- och storleksord ur en titel, för variantverifiering
   * (arbetsorder 5.5). Normaliserade: "2 TB" -> "2TB".
   */
  function capacityTokens(title) {
    if (!title) return [];
    const s = String(title).replace(NBSP, ' ').toUpperCase();
    const tokens = new Set();
    const unitRe = /(\d+(?:[.,]\d+)?)\s?(TB|GB|MB|KB|W|MM|CM|"|INCH|TUM|ML|L|KG|G|MAH|HZ|MHZ|GHZ|PACK|ST)(?![A-Za-z0-9])/g;
    let m;
    while ((m = unitRe.exec(s))) {
      tokens.add(`${m[1].replace(',', '.')}${m[2]}`);
    }
    // Storleksbokstäver bara som fristående ord: "M.2" och "L-format" ska inte räknas.
    const sizeRe = /(?<![\w.])(XXS|XS|S|M|L|XL|XXL|XXXL|3XL)(?![\w.])/g;
    while ((m = sizeRe.exec(s))) tokens.add(m[1]);
    return Array.from(tokens).sort();
  }

  function normalizeBrand(input) {
    if (!input) return null;
    return String(input)
      .replace(NBSP, ' ')
      .replace(/^(varumärke|brand|marke|marque|marca)\s*[:–-]\s*/i, '')
      .replace(/\s+/g, ' ')
      .trim()
      .toLowerCase();
  }

  /** Rapport över vilka strategier som bar vikten – loggas bara i konsolen. */
  class ParseReport {
    constructor(label) {
      this.label = label;
      this.fields = {};
      this.missing = {};
      this.startedAt = new Date().toISOString();
    }
    used(field, via) {
      if (via) this.fields[field] = via;
      return this;
    }
    lost(field, tried) {
      this.missing[field] = tried || [];
      return this;
    }
    /** Tar värdet ur resolve() och bokför samtidigt vilken strategi som gav det. */
    take(field, result) {
      if (!result) {
        this.lost(field, []);
        return null;
      }
      if (result.value === null || result.value === undefined) {
        this.lost(field, result.tried);
        return null;
      }
      this.used(field, result.via);
      return result.value;
    }
    toJSON() {
      return {
        label: this.label,
        startedAt: this.startedAt,
        fields: this.fields,
        missing: this.missing,
      };
    }
    log() {
      const missingKeys = Object.keys(this.missing);
      // Endast utvecklarkonsolen – aldrig användarsynligt (arbetsorder 3.5).
      console.groupCollapsed(
        `[Orderkoll] parsning: ${this.label} (${Object.keys(this.fields).length} fält, ${missingKeys.length} saknade)`
      );
      console.table(
        Object.entries(this.fields).map(([field, via]) => ({ field, via }))
      );
      if (missingKeys.length) console.warn('Saknade fält:', this.missing);
      console.groupEnd();
      return this;
    }
  }

  /** Känner igen CAPTCHA-/inloggningsvägg i en hämtad HTML-sträng. */
  function detectWall(htmlOrUrl) {
    const s = String(htmlOrUrl || '');
    if (/\/errors\/validateCaptcha|captchacharacters|Ange tecknen|Skriv in tecknen/i.test(s)) {
      return 'captcha';
    }
    if (/\/ap\/signin|ap_email|Logga in|Sign-In/i.test(s) && /form/i.test(s)) {
      return 'signin';
    }
    return null;
  }

  AOM.parse = {
    text,
    blockText,
    queryFirst,
    queryAll,
    resolve,
    findByLabel,
    parseMoney,
    parseSwedishDate,
    extractAsin,
    extractOrderId,
    extractSellerId,
    parseQuantity,
    parseEan,
    capacityTokens,
    normalizeBrand,
    detectWall,
    ParseReport,
    ORDER_ID_RE,
    MONTHS_SV,
  };
})(typeof self !== 'undefined' ? self : globalThis);
