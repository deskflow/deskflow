/* Orderkoll – produktsidan: prisjämförelse, bevissäkring, lagerbevakning.
 * Knappen placeras direkt under köpblocket, aldrig som en overlay ovanpå
 * sidan (arbetsorder 5.5), och prisjämförelsen körs bara på klick. */
(function () {
  'use strict';
  const AOM = self.AOM;
  const P = AOM.parse;
  const MSG = AOM.MSG;

  const BUYBOX_SELECTORS = [
    '#desktop_buybox',
    '#buybox',
    '#rightCol',
    '#addToCart_feature_div',
    '#submit\\.add-to-cart',
  ];

  function identity() {
    return AOM.priceMatch.extractIdentity(document, location.href);
  }

  function collectImageUrls() {
    const urls = new Set();
    const main = document.querySelector('#landingImage, #imgBlkFront');
    if (main) {
      const hires = main.getAttribute('data-old-hires');
      if (hires) urls.add(hires);
      const dyn = main.getAttribute('data-a-dynamic-image');
      if (dyn) {
        try {
          Object.keys(JSON.parse(dyn)).forEach((u) => urls.add(u));
        } catch (_err) {
          /* ignoreras */
        }
      }
      if (main.src) urls.add(main.src);
    }
    document.querySelectorAll('#altImages img, #imageBlock img').forEach((img) => {
      if (img.src) urls.add(img.src);
    });
    return Array.from(urls).slice(0, 12);
  }

  function send(message) {
    return new Promise((resolve) => {
      chrome.runtime.sendMessage(message, (response) => {
        if (chrome.runtime.lastError) {
          resolve({ error: chrome.runtime.lastError.message });
          return;
        }
        resolve(response || {});
      });
    });
  }

  function parseHtml(html) {
    return new DOMParser().parseFromString(html, 'text/html');
  }

  const deps = {
    async fetchText(url) {
      const res = await send({ type: MSG.FETCH_URL, url });
      if (res.error) {
        const err = new Error(res.error);
        err.wall = res.wall || null;
        throw err;
      }
      return res;
    },
    parseHtml,
    async imageHash(url) {
      const res = await send({ type: MSG.IMAGE_HASH, url });
      return res && res.hash ? res.hash : null;
    },
  };

  function el(tag, className, textContent) {
    const node = document.createElement(tag);
    if (className) node.className = className;
    if (textContent !== undefined) node.textContent = textContent;
    return node;
  }

  function methodBadge(result) {
    const badge = el('span', 'aom-badge');
    if (result.method === AOM.priceMatch.METHOD.IMAGE) {
      badge.textContent = `Bild · ${Math.round((result.confidence || 0) * 100)}%`;
    } else {
      badge.textContent = result.method || '–';
    }
    badge.dataset.method = result.method || 'none';
    return badge;
  }

  function renderResults(container, results, sourcePrice) {
    container.textContent = '';
    const table = el('table', 'aom-table');
    const thead = el('thead');
    const headRow = el('tr');
    ['Marknad', 'Träffades via', 'Pris', 'Ca i SEK', 'Status'].forEach((label) => {
      headRow.appendChild(el('th', null, label));
    });
    thead.appendChild(headRow);
    table.appendChild(thead);

    const tbody = el('tbody');
    for (const r of results) {
      const tr = el('tr');
      const market = el('td');
      if (r.url) {
        const a = el('a', null, r.label);
        a.href = r.url;
        a.target = '_blank';
        a.rel = 'noopener noreferrer';
        market.appendChild(a);
      } else {
        market.textContent = r.label;
      }
      tr.appendChild(market);

      const via = el('td');
      if (r.status === AOM.priceMatch.RESULT.NOT_FOUND) {
        via.textContent = '—';
      } else if (r.status === AOM.priceMatch.RESULT.ERROR) {
        via.textContent = '—';
      } else {
        via.appendChild(methodBadge(r));
      }
      tr.appendChild(via);

      tr.appendChild(
        el('td', null, r.price !== null && r.price !== undefined ? `${r.price} ${r.currency || ''}`.trim() : '–')
      );
      tr.appendChild(
        el(
          'td',
          null,
          r.convertedPrice !== null && r.convertedPrice !== undefined
            ? `${r.convertedPrice.toLocaleString('sv-SE')} kr`
            : 'okänt'
        )
      );

      const status = el('td');
      if (r.status === AOM.priceMatch.RESULT.MATCH) {
        const diff =
          Number.isFinite(sourcePrice) && Number.isFinite(r.convertedPrice)
            ? Math.round(r.convertedPrice - sourcePrice)
            : null;
        status.textContent =
          diff === null ? 'Träff' : diff < 0 ? `${Math.abs(diff)} kr billigare` : `${diff} kr dyrare`;
        if (diff !== null && diff < 0) status.className = 'aom-good';
      } else if (r.status === AOM.priceMatch.RESULT.VARIANT_UNCERTAIN) {
        status.textContent = 'Variant osäker';
        status.title = (r.verifyReasons || []).join('; ');
        status.className = 'aom-warn';
      } else if (r.status === AOM.priceMatch.RESULT.NOT_FOUND) {
        status.textContent = 'Ej i katalogen';
      } else {
        status.textContent = 'Fel vid hämtning';
        status.title = r.error || '';
        status.className = 'aom-warn';
      }
      tr.appendChild(status);
      tbody.appendChild(tr);
    }
    table.appendChild(tbody);
    container.appendChild(table);
    container.appendChild(
      el('p', 'aom-note', 'Valutakurser: ECB:s dagliga referenskurser. Priser kan skilja sig åt i frakt, moms och tillgänglighet.')
    );
  }

  async function runComparison(button, container) {
    const source = identity();
    if (!source.asin && !source.ean && !source.mpn) {
      container.textContent = 'Kunde inte läsa av produktens identitet på den här sidan.';
      return;
    }
    button.disabled = true;
    const original = button.textContent;
    button.textContent = 'Jämför…';
    container.textContent = 'Hämtar en marknad i taget, med paus emellan…';
    try {
      const state = await send({ type: MSG.GET_STATE });
      const settings = (state && state.settings) || AOM.DEFAULT_SETTINGS;
      const results = await AOM.priceMatch.compare(source, settings.compareMarketplaces, deps, {
        imageMatchThreshold: settings.imageMatchThreshold,
      });
      const fx = await send({ type: MSG.FX_RATES });
      const withSek = AOM.priceMatch.withConverted(results, fx && fx.rates, 'SEK');
      renderResults(container, withSek, source.price);
    } catch (err) {
      container.textContent = `Prisjämförelsen avbröts: ${err.message || err}`;
    } finally {
      button.disabled = false;
      button.textContent = original;
    }
  }

  async function saveProof(button, container) {
    const source = identity();
    button.disabled = true;
    const original = button.textContent;
    button.textContent = 'Sparar…';
    const res = await send({
      type: MSG.PRODUCT_SNAPSHOT,
      snapshot: {
        kind: 'manual',
        asin: source.asin,
        pageUrl: location.href,
        html: document.documentElement.outerHTML,
        imageUrls: collectImageUrls(),
        meta: {
          title: source.title,
          price: source.price,
          currency: source.currency,
          availability: source.availability,
          brand: source.brandRaw,
          ean: source.ean,
          mpn: source.mpn,
          capturedAt: new Date().toISOString(),
        },
      },
    });
    button.disabled = false;
    button.textContent = original;
    container.textContent = res && res.ok
      ? `Bevis sparat lokalt (${res.bundleId ? res.bundleId.slice(0, 14) : 'ok'}…). Hittas under Bevisarkiv.`
      : `Kunde inte spara beviset: ${(res && res.error) || 'okänt fel'}`;
  }

  async function toggleWatch(button, container) {
    const source = identity();
    if (!source.asin) {
      container.textContent = 'Ingen ASIN kunde läsas av – kan inte bevaka den här sidan.';
      return;
    }
    const res = await send({
      type: MSG.WATCH_ADD,
      watch: {
        asin: source.asin,
        title: source.title,
        url: location.href.split('?')[0],
        lastAvailability: source.availability || null,
      },
    });
    container.textContent = res && res.ok ? 'Lagerbevakning påslagen för den här produkten.' : 'Kunde inte lägga till bevakningen.';
  }

  function insertPanel() {
    if (document.getElementById('aom-product-panel')) return;
    let anchor = null;
    for (const sel of BUYBOX_SELECTORS) {
      anchor = document.querySelector(sel);
      if (anchor) break;
    }
    if (!anchor) return;

    const panel = el('div', 'aom-panel');
    panel.id = 'aom-product-panel';

    const row = el('div', 'aom-actions');
    const compareBtn = el('button', 'aom-btn aom-btn-primary', 'Jämför pris på andra Amazon-marknader');
    const proofBtn = el('button', 'aom-btn', 'Spara bevis');
    const watchBtn = el('button', 'aom-btn', 'Bevaka lager');
    [compareBtn, proofBtn, watchBtn].forEach((b) => {
      b.type = 'button';
      row.appendChild(b);
    });

    const output = el('div', 'aom-output');
    panel.appendChild(row);
    panel.appendChild(output);
    panel.appendChild(el('p', 'aom-disclaimer', AOM.DISCLAIMER));

    compareBtn.addEventListener('click', () => runComparison(compareBtn, output));
    proofBtn.addEventListener('click', () => saveProof(proofBtn, output));
    watchBtn.addEventListener('click', () => toggleWatch(watchBtn, output));

    const parent = anchor.parentElement || document.body;
    if (anchor.nextSibling) parent.insertBefore(panel, anchor.nextSibling);
    else parent.appendChild(panel);
  }

  /** Primärflödet i 5.6: ögonblicksbild när produkten läggs i varukorgen. */
  function hookAddToCart() {
    const targets = [
      '#add-to-cart-button',
      'input[name="submit.add-to-cart"]',
      '#buy-now-button',
      'input[name="submit.buy-now"]',
    ];
    for (const sel of targets) {
      const btn = document.querySelector(sel);
      if (!btn || btn.dataset.aomHooked) continue;
      btn.dataset.aomHooked = '1';
      btn.addEventListener(
        'click',
        () => {
          const source = identity();
          send({
            type: MSG.PROVISIONAL_SNAPSHOT,
            snapshot: {
              asin: source.asin,
              pageUrl: location.href,
              html: document.documentElement.outerHTML,
              imageUrls: collectImageUrls(),
              meta: {
                title: source.title,
                price: source.price,
                currency: source.currency,
                availability: source.availability,
                brand: source.brandRaw,
                ean: source.ean,
                mpn: source.mpn,
                capturedAt: new Date().toISOString(),
                trigger: sel,
              },
            },
          });
        },
        { capture: true }
      );
    }
  }

  insertPanel();
  hookAddToCart();
  self.AOM.productPage = { identity, collectImageUrls, insertPanel, deps };
})();
