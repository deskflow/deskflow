/* Orderkoll – parsning av orderhistoriken (arbetsorder steg 2).
 *
 * VIKTIGT OM SELEKTORER: Amazon byter klassnamn med jämna mellanrum, och
 * markup skiljer sig mellan marknader och kontotyper. Därför är varje fält
 * kopplat till en lista strategier – stabila ankare först (data-attribut,
 * länkmönster, etiketttexter), skira klassnamn sist. Vilken strategi som bar
 * vikten loggas i utvecklarkonsolen (ParseReport), aldrig för användaren.
 * Selektorlistorna nedan är utgångsläget och SKA kalibreras mot sparad HTML
 * från det egna kontot innan de anses klara (arbetsorder 3). */
(function () {
  'use strict';
  const AOM = self.AOM;
  const P = AOM.parse;

  const ORDER_CARD_SELECTORS = [
    '[data-order-id]',
    '.order-card',
    '.js-order-card',
    '[class*="order-card"]',
    '.order',
    '.a-box-group',
  ];

  const SHIPMENT_SELECTORS = [
    '[data-component="shipments"] > div',
    '.shipment',
    '[class*="shipment"]',
    '.a-box.delivery-box',
    '.a-box-inner > .a-fixed-right-grid',
  ];

  const ITEM_SELECTORS = [
    '[data-component="purchasedItems"] .a-fixed-left-grid',
    '.yohtmlc-item',
    '[class*="purchased-item"]',
    '.a-fixed-left-grid-inner',
  ];

  const STATUS_TEXT_SELECTORS = [
    '[data-component="shipmentStatus"]',
    '.delivery-box__primary-text',
    '.js-shipment-info-container .a-text-bold',
    '[class*="shipment-status"]',
    '.a-row .a-text-bold',
    'h3',
  ];

  /** Alla ordernummer som förekommer i ett element (attribut, text, länkar). */
  function distinctOrderIds(el) {
    const ids = new Set();
    const attr = el.getAttribute && el.getAttribute('data-order-id');
    if (attr && P.ORDER_ID_RE.test(attr)) ids.add(attr);
    const matches = P.text(el).match(new RegExp(P.ORDER_ID_RE.source, 'g'));
    if (matches) matches.forEach((id) => ids.add(id));
    el.querySelectorAll('a[href*="orderID="], a[href*="orderId="]').forEach((a) => {
      const id = P.extractOrderId(a.getAttribute('href'));
      if (id) ids.add(id);
    });
    return ids;
  }

  /**
   * Ett orderkort är den största behållare som innehåller exakt ETT
   * ordernummer. Alla selektorer provas och slås ihop, i stället för att
   * stanna vid den första som ger träff – annars missas kort som saknar
   * data-order-id när andra kort har det.
   */
  function cardCandidates() {
    const best = new Map(); // orderId -> { el, size, via }
    const consider = (els, via) => {
      for (const el of els) {
        const ids = distinctOrderIds(el);
        if (ids.size !== 1) continue;
        const id = Array.from(ids)[0];
        const size = el.querySelectorAll('*').length;
        const prev = best.get(id);
        if (!prev || size > prev.size) best.set(id, { el, size, via });
      }
    };

    for (const sel of ORDER_CARD_SELECTORS) {
      consider(Array.from(document.querySelectorAll(sel)), sel);
    }
    if (!best.size) {
      // Sista utväg: minsta behållare som innehåller ett ordernummer.
      const hits = Array.from(document.querySelectorAll('div,li,section')).filter((el) => {
        if (!P.ORDER_ID_RE.test(P.text(el))) return false;
        return !Array.from(el.children).some((child) => P.ORDER_ID_RE.test(P.text(child)));
      });
      consider(hits, 'fallback:minsta-behållare-med-ordernummer');
    }

    const entries = Array.from(best.values());
    const vias = Array.from(new Set(entries.map((e) => e.via))).join(' + ') || 'ingen';
    return { cards: entries.map((e) => e.el), via: vias, perCard: entries };
  }

  function orderIdFor(card, report) {
    return report.take(
      'orderId',
      P.resolve(card, [
        { via: 'data-order-id', get: (el) => el.getAttribute && el.getAttribute('data-order-id') },
        {
          via: 'a[href*=orderID]',
          get: (el) => {
            const a = el.querySelector('a[href*="orderID="], a[href*="orderId="]');
            return a ? P.extractOrderId(a.getAttribute('href')) : null;
          },
        },
        {
          via: 'label:Ordernummer',
          get: (el) => {
            const hit = P.findByLabel(el, /^(ordernummer|order[\s#]*nr|order #|order number)/i, {
              scope: 'span,div,dt,bdi',
            });
            return hit ? P.extractOrderId(hit.value) : null;
          },
        },
        { via: 'regex:korttext', get: (el) => P.extractOrderId(P.text(el)) },
      ])
    );
  }

  function orderDateFor(card, report) {
    return report.take(
      'orderDate',
      P.resolve(
        card,
        [
          {
            via: 'label:Orderlagd',
            get: (el) => {
              const hit = P.findByLabel(el, /^(orderlagd|order lagd|beställning gjord|order placed|beställd)/i, {
                scope: 'span,div,dt,bdi',
              });
              return hit ? hit.value : null;
            },
          },
          { via: '.order-date-invoice-item', sel: '.order-date-invoice-item' },
          { via: '.a-color-secondary .value', sel: '.a-color-secondary .value' },
        ],
        (v) => P.parseSwedishDate(v)
      )
    );
  }

  function totalFor(card, report) {
    const raw = report.take(
      'totalAmount',
      P.resolve(card, [
        {
          via: 'label:Summa',
          get: (el) => {
            const hit = P.findByLabel(el, /^(summa|totalt|total|ordersumma|order total)/i, {
              scope: 'span,div,dt,bdi',
            });
            return hit ? hit.value : null;
          },
        },
        { via: '.yohtmlc-order-level-connections .a-color-base', sel: '.yohtmlc-order-level-connections .a-color-base' },
        { via: '.a-price .a-offscreen', sel: '.a-price .a-offscreen' },
      ])
    );
    const money = P.parseMoney(raw);
    return money ? money : null;
  }

  function sellerFor(card, report) {
    const sellerLink = card.querySelector('a[href*="seller="], a[href*="/sp?"]');
    const sellerId = sellerLink ? P.extractSellerId(sellerLink.getAttribute('href')) : null;
    if (sellerId) report.used('sellerId', 'a[href*=seller=]');
    const nameHit = P.findByLabel(card, /^(såld av|sold by|säljare)/i, { scope: 'span,div,dt' });
    if (nameHit) report.used('sellerNameSnapshot', nameHit.via);
    const name = nameHit ? nameHit.value : sellerLink ? P.text(sellerLink) : null;
    return {
      sellerId,
      sellerNameSnapshot: name,
      sellerIsThirdParty: name ? !/^amazon(\.| eu| se|$)/i.test(name.trim()) : null,
    };
  }

  function statusTextIn(container) {
    for (const sel of STATUS_TEXT_SELECTORS) {
      const el = container.querySelector(sel);
      const t = P.text(el);
      if (t && AOM.status.normalizeStatus(t).status) return { text: t, via: sel };
    }
    const own = P.text(container);
    if (AOM.status.normalizeStatus(own).status) return { text: own.slice(0, 200), via: 'containertext' };
    return { text: null, via: null };
  }

  function itemsIn(container) {
    for (const sel of ITEM_SELECTORS) {
      const els = Array.from(container.querySelectorAll(sel)).filter((el) =>
        el.querySelector('a[href*="/dp/"], a[href*="/gp/product/"]')
      );
      if (els.length) return { els, via: sel };
    }
    // Fallback: gruppera på produktlänkarna själva.
    const links = Array.from(container.querySelectorAll('a[href*="/dp/"], a[href*="/gp/product/"]'));
    const blocks = [];
    const seen = new Set();
    for (const link of links) {
      const block = link.closest('div.a-row, div.a-fixed-left-grid, li') || link.parentElement;
      if (!block || seen.has(block)) continue;
      seen.add(block);
      blocks.push(block);
    }
    return { els: blocks, via: 'fallback:produktlänk-block' };
  }

  function parseItem(block, statusText, report, index) {
    const link =
      block.querySelector('a[href*="/dp/"]') || block.querySelector('a[href*="/gp/product/"]');
    const href = link ? link.getAttribute('href') : null;
    const asin = P.extractAsin(href) || null;
    const img = block.querySelector('img[src*="media-amazon"], img[src*="images-amazon"], img');
    const titleEl =
      block.querySelector('.yohtmlc-product-title') ||
      block.querySelector('a[href*="/dp/"] span') ||
      link;
    const title = P.text(titleEl) || (img && img.getAttribute('alt')) || null;

    const normalized = AOM.status.normalizeStatus(statusText || '');
    if (normalized.via) report.used(`item${index}.status`, `status:${normalized.via}`);

    const dateFromStatus = statusText ? P.parseSwedishDate(statusText) : null;
    const priceEl = block.querySelector('.a-price .a-offscreen, .a-color-price');
    const money = P.parseMoney(P.text(priceEl));

    const item = {
      asin,
      title,
      /* Antal och styckpris finns inte tillförlitligt i historikvyn – de hämtas
       * från orderdetaljsidan (arbetsorder 4 och steg 3). null = okänt. */
      quantity: null,
      unitPrice: money ? money.amount : null,
      lineTotal: null,
      currency: money ? money.currency : null,
      thumbnailUrl: img ? img.getAttribute('src') : null,
      productUrl: href ? new URL(href, location.origin).toString() : null,
      amazonStatusRaw: statusText || null,
      status: normalized.status,
      deliveredDate: normalized.status === AOM.STATUS.DELIVERED ? dateFromStatus : null,
      estimatedDeliveryDate: normalized.status !== AOM.STATUS.DELIVERED ? dateFromStatus : null,
      returnWindow: { amazonPolicyDays: null, amazonPolicyDeadline: null, sourceText: null },
      userReceivedConfirmed: false,
      userReceivedConfirmedAt: null,
    };
    item.lineKey = AOM.status.lineKey(item, index);
    return item;
  }

  function detailUrlFor(card) {
    const a = card.querySelector(
      'a[href*="order-details"], a[href*="orderID="], a[href*="order_details"]'
    );
    return a ? new URL(a.getAttribute('href'), location.origin).toString() : null;
  }

  function parseCard(card, cardVia) {
    const report = new P.ParseReport('orderhistorik-kort');
    report.used('card', cardVia);
    const orderId = orderIdFor(card, report);
    if (!orderId) return null;

    const total = totalFor(card, report);
    const seller = sellerFor(card, report);

    const shipments = (() => {
      for (const sel of SHIPMENT_SELECTORS) {
        const els = Array.from(card.querySelectorAll(sel)).filter((el) =>
          el.querySelector('a[href*="/dp/"], a[href*="/gp/product/"]')
        );
        if (els.length) return { els, via: sel };
      }
      return { els: [card], via: 'kortet-som-en-leverans' };
    })();
    report.used('shipments', shipments.via);

    const lineItems = [];
    for (const shipment of shipments.els) {
      const status = statusTextIn(shipment);
      const items = itemsIn(shipment);
      if (items.via) report.used('items', items.via);
      for (const block of items.els) {
        const item = parseItem(block, status.text, report, lineItems.length);
        if (!item.asin && !item.title) continue;
        if (lineItems.some((existing) => existing.productUrl && existing.productUrl === item.productUrl)) {
          continue; // samma produktlänk två gånger = dubblettblock, inte två artiklar
        }
        lineItems.push(item);
      }
    }

    const order = {
      orderId,
      orderDate: orderDateFor(card, report),
      totalAmount: total ? total.amount : null,
      currency: total ? total.currency : null,
      paymentMethodLabel: null, // finns på orderdetaljsidan
      sellerId: seller.sellerId,
      sellerNameSnapshot: seller.sellerNameSnapshot,
      sellerIsThirdParty: seller.sellerIsThirdParty,
      detailUrl: detailUrlFor(card),
      lineItems,
      source: 'order-history',
      scrapedAt: new Date().toISOString(),
      parseReport: report.toJSON(),
    };
    report.log();
    return order;
  }

  function nextPageUrl() {
    const a = document.querySelector('.a-pagination .a-last a, ul.a-pagination li.a-last a');
    return a ? new URL(a.getAttribute('href'), location.origin).toString() : null;
  }

  function parsePage() {
    const { cards, via } = cardCandidates();
    const orders = [];
    for (const card of cards) {
      try {
        const order = parseCard(card, via);
        if (order) orders.push(order);
      } catch (err) {
        console.warn('[Orderkoll] kunde inte tolka ett orderkort:', err);
      }
    }
    // Dubbletter kan uppstå när fallback-selektorn träffar både yttre och inre nod.
    const byId = new Map();
    for (const order of orders) {
      const prev = byId.get(order.orderId);
      if (!prev || (order.lineItems.length > prev.lineItems.length)) byId.set(order.orderId, order);
    }
    return { orders: Array.from(byId.values()), cardVia: via, nextPageUrl: nextPageUrl() };
  }

  function run(reason) {
    const wall = P.detectWall(location.href) || P.detectWall(document.title);
    if (wall) {
      chrome.runtime.sendMessage({
        type: AOM.MSG.ORDER_HISTORY_PARSED,
        error: wall,
        pageUrl: location.href,
        orders: [],
      });
      return;
    }
    const result = parsePage();
    console.info(
      `[Orderkoll] orderhistorik (${reason}): ${result.orders.length} ordrar via "${result.cardVia}"`,
      result.orders
    );
    chrome.runtime.sendMessage({
      type: AOM.MSG.ORDER_HISTORY_PARSED,
      orders: result.orders,
      pageUrl: location.href,
      nextPageUrl: result.nextPageUrl,
      cardVia: result.cardVia,
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

  // Exponeras för test och för manuell felsökning i konsolen.
  self.AOM.orderHistory = { parsePage, parseCard, cardCandidates };
  run('sidladdning');
})();
