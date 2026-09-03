/* Orderkoll – parsning av orderdetaljsidan (arbetsorder steg 3).
 * Här finns det som historikvyn inte har: antal per artikel, styckpris,
 * säljar-ID, betalsätt och Amazons egen returfrist. */
(function () {
  'use strict';
  const AOM = self.AOM;
  const P = AOM.parse;

  const ITEM_SELECTORS = [
    '.yohtmlc-item',
    '[data-component="purchasedItems"] .a-fixed-left-grid',
    '#orderDetails .a-fixed-left-grid-inner',
    '.a-fixed-left-grid',
  ];

  const QTY_SELECTORS = ['.item-view-qty', '[class*="item-view-qty"]', 'span.a-size-small.a-color-secondary'];

  function orderIdFromPage(report) {
    return report.take(
      'orderId',
      P.resolve(document, [
        { via: 'url:orderID', get: () => P.extractOrderId(location.search) },
        {
          via: 'label:Ordernummer',
          get: (d) => {
            const hit = P.findByLabel(d, /^(ordernummer|order[\s#]*nr|order #|order number)/i, {
              scope: 'span,div,dt,bdi',
            });
            return hit ? P.extractOrderId(hit.value) : null;
          },
        },
        { via: 'regex:sidtext', get: (d) => P.extractOrderId(P.text(d.body).slice(0, 4000)) },
      ])
    );
  }

  function paymentLabel(report) {
    return report.take(
      'paymentMethodLabel',
      P.resolve(document, [
        {
          via: 'label:Betalningssätt',
          get: (d) => {
            const hit = P.findByLabel(d, /^(betalningssätt|betalningsmetod|payment method)/i, {
              scope: 'span,div,h5,dt',
            });
            return hit ? hit.value : null;
          },
        },
        { via: '.pmts-payment-instrument-detail', sel: '.pmts-payment-instrument-detail' },
        {
          via: 'regex:kortsiffror',
          get: (d) => {
            const m = P.text(d.body).match(/([A-Za-zÅÄÖåäö]+)\s*(?:•|\*|\.){2,}\s*(\d{4})/);
            return m ? `${m[1]} ••${m[2]}` : null;
          },
        },
      ])
    );
  }

  /** Amazons frivilliga returpolicy – läses av, aldrig antagen (5.4 punkt 1). */
  function returnPolicy(scope) {
    const text = P.text(scope);
    const deadline = text.match(
      /(?:returnerbar|retur(?:er)?\s*(?:accepteras|godkänns)?|kan returneras)[^.]{0,40}?(?:t\.?o\.?m\.?|till och med|senast|until)\s*([0-9]{1,2}\s+[a-zåäö]+\.?(?:\s+\d{4})?|\d{4}-\d{2}-\d{2})/i
    );
    if (deadline) {
      const iso = P.parseSwedishDate(deadline[1], { refYear: new Date().getFullYear() });
      return { amazonPolicyDeadline: iso, amazonPolicyDays: null, sourceText: deadline[0] };
    }
    const days = text.match(/inom\s+(\d{1,3})\s+dagar/i);
    if (days) {
      return { amazonPolicyDeadline: null, amazonPolicyDays: Number(days[1]), sourceText: days[0] };
    }
    return { amazonPolicyDeadline: null, amazonPolicyDays: null, sourceText: null };
  }

  function refundInfo() {
    const body = P.text(document.body);
    const hit = body.match(/(återbetal\w*|refund(?:ed)?)[^.]{0,80}/i);
    if (!hit) return { refundDetected: false, evidenceText: null };
    return { refundDetected: true, evidenceText: hit[0].slice(0, 160) };
  }

  function statusForItem(block) {
    const container =
      block.closest('[data-component="shipments"] > div') ||
      block.closest('.shipment') ||
      block.closest('.a-box') ||
      block.parentElement;
    const candidates = [
      container && container.querySelector('[data-component="shipmentStatus"]'),
      container && container.querySelector('.js-shipment-info-container'),
      container && container.querySelector('h3, .a-text-bold'),
    ].filter(Boolean);
    for (const el of candidates) {
      const t = P.text(el);
      if (AOM.status.normalizeStatus(t).status) return t;
    }
    const own = container ? P.text(container) : '';
    return AOM.status.normalizeStatus(own).status ? own.slice(0, 200) : null;
  }

  function parseItems(report) {
    let blocks = [];
    let via = null;
    for (const sel of ITEM_SELECTORS) {
      const els = Array.from(document.querySelectorAll(sel)).filter((el) =>
        el.querySelector('a[href*="/dp/"], a[href*="/gp/product/"]')
      );
      if (els.length) {
        blocks = els;
        via = sel;
        break;
      }
    }
    if (via) report.used('items', via);

    const items = [];
    blocks.forEach((block, index) => {
      const link = block.querySelector('a[href*="/dp/"], a[href*="/gp/product/"]');
      const href = link ? link.getAttribute('href') : null;
      const asin = P.extractAsin(href);
      const img = block.querySelector('img');
      const title = P.text(link) || (img && img.getAttribute('alt')) || null;
      if (!asin && !title) return;

      let quantity = null;
      for (const sel of QTY_SELECTORS) {
        const el = block.querySelector(sel);
        const q = P.parseQuantity(P.text(el));
        if (q) {
          quantity = q;
          report.used(`item${index}.quantity`, sel);
          break;
        }
      }
      if (quantity === null) {
        const hit = P.findByLabel(block, /^(antal|kvantitet|qty|quantity)/i, { scope: 'span,div,td' });
        if (hit) {
          quantity = P.parseQuantity(hit.value);
          if (quantity) report.used(`item${index}.quantity`, `label:${hit.via}`);
        }
      }
      /* Ingen defaultning till 1: okänt antal ska synas som okänt, annars
       * går kontrollsumman i steg 3 inte att lita på. */

      const priceEl = block.querySelector('.a-price .a-offscreen, .a-color-price, .yohtmlc-item-price');
      const money = P.parseMoney(P.text(priceEl));
      if (money) report.used(`item${index}.unitPrice`, '.a-price/.a-color-price');

      const sellerLink = block.querySelector('a[href*="seller="], a[href*="/sp?"]');
      const sellerHit = P.findByLabel(block, /^(såld av|sold by|säljare)/i, { scope: 'span,div' });
      const statusRaw = statusForItem(block);
      const normalized = AOM.status.normalizeStatus(statusRaw || '');
      const dateFromStatus = statusRaw ? P.parseSwedishDate(statusRaw) : null;
      const policy = returnPolicy(block.closest('.a-box') || block);

      const item = {
        asin,
        title,
        quantity,
        unitPrice: money ? money.amount : null,
        currency: money ? money.currency : null,
        lineTotal: money && quantity ? Math.round(money.amount * quantity * 100) / 100 : null,
        thumbnailUrl: img ? img.getAttribute('src') : null,
        productUrl: href ? new URL(href, location.origin).toString() : null,
        amazonStatusRaw: statusRaw,
        status: normalized.status,
        deliveredDate: normalized.status === AOM.STATUS.DELIVERED ? dateFromStatus : null,
        estimatedDeliveryDate: normalized.status !== AOM.STATUS.DELIVERED ? dateFromStatus : null,
        returnWindow: policy,
        sellerId: sellerLink ? P.extractSellerId(sellerLink.getAttribute('href')) : null,
        sellerNameSnapshot: sellerHit ? sellerHit.value : sellerLink ? P.text(sellerLink) : null,
      };
      item.lineKey = AOM.status.lineKey(item, items.length);
      items.push(item);
    });
    return items;
  }

  /** Kontrollsumma för grinden i steg 3: summan av raderna mot ordersumman. */
  function checksum(items, totalAmount) {
    const known = items.filter((i) => Number.isFinite(i.unitPrice) && Number.isFinite(i.quantity));
    if (!known.length || !Number.isFinite(totalAmount)) {
      return { ok: null, reason: 'otillräckliga data för kontrollsumma' };
    }
    const sum = known.reduce((acc, i) => acc + i.unitPrice * i.quantity, 0);
    const diff = Math.abs(sum - totalAmount);
    return {
      ok: diff <= Math.max(1, totalAmount * 0.02),
      lineSum: Math.round(sum * 100) / 100,
      totalAmount,
      diff: Math.round(diff * 100) / 100,
      note: 'skillnad kan bero på frakt, rabatt eller moms – kontrollera manuellt',
    };
  }

  function parsePage() {
    const report = new P.ParseReport('orderdetalj');
    const orderId = orderIdFromPage(report);
    if (!orderId) {
      report.log();
      return null;
    }
    const items = parseItems(report);
    const totalRaw = (() => {
      const hit = P.findByLabel(document, /^(ordersumma|totalsumma|totalt|summa|order total|grand total)/i, {
        scope: 'span,div,td,th',
      });
      return hit ? hit.value : null;
    })();
    const total = P.parseMoney(totalRaw);
    if (total) report.used('totalAmount', 'label:Ordersumma');

    const orderDate = (() => {
      const hit = P.findByLabel(document, /^(orderlagd|beställd|beställning gjord|order placed)/i, {
        scope: 'span,div,dt,bdi',
      });
      const iso = hit ? P.parseSwedishDate(hit.value) : null;
      if (iso) report.used('orderDate', `label:${hit.via}`);
      return iso;
    })();

    const sellerFromItems = items.find((i) => i.sellerId) || {};
    const refund = refundInfo();

    const order = {
      orderId,
      orderDate,
      totalAmount: total ? total.amount : null,
      currency: total ? total.currency : null,
      paymentMethodLabel: paymentLabel(report),
      sellerId: sellerFromItems.sellerId || null,
      sellerNameSnapshot: sellerFromItems.sellerNameSnapshot || null,
      sellerIsThirdParty: sellerFromItems.sellerNameSnapshot
        ? !/^amazon(\.| eu| se|$)/i.test(sellerFromItems.sellerNameSnapshot.trim())
        : null,
      detailUrl: location.href,
      lineItems: items,
      source: 'order-detail',
      scrapedAt: new Date().toISOString(),
      parseReport: report.toJSON(),
    };
    order.checksum = checksum(items, order.totalAmount);
    report.log();
    console.info('[Orderkoll] orderdetalj', order.orderId, 'kontrollsumma:', order.checksum);
    return { order, refund };
  }

  function run(reason) {
    const wall = P.detectWall(location.href);
    if (wall) {
      chrome.runtime.sendMessage({ type: AOM.MSG.ORDER_DETAIL_PARSED, error: wall, pageUrl: location.href });
      return;
    }
    const parsed = parsePage();
    if (!parsed) return;
    chrome.runtime.sendMessage({
      type: AOM.MSG.ORDER_DETAIL_PARSED,
      order: parsed.order,
      refund: Object.assign({ orderId: parsed.order.orderId }, parsed.refund),
      pageUrl: location.href,
      reason,
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

  self.AOM.orderDetail = { parsePage, checksum, returnPolicy };
  run('sidladdning');
})();
