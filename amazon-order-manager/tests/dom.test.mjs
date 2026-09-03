/* DOM-tester: content scripts körs i riktig Chromium via Playwright, mot de
 * syntetiska fixturerna i tests/fixtures. Detta bevisar att parsningslogiken
 * fungerar mot de antagna strukturmönstren – INTE att mönstren stämmer med
 * dagens amazon.se (det kräver sparad HTML från ett riktigt konto).
 *
 * Kör:  node --test tests/dom.test.mjs
 */
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);

/** Hittar Playwright oavsett om det är installerat lokalt eller globalt. */
function loadPlaywright() {
  const candidates = ['playwright', 'playwright-core', '/opt/node22/lib/node_modules/playwright'];
  for (const id of candidates) {
    try {
      return require(id);
    } catch (_err) {
      /* provar nästa */
    }
  }
  throw new Error('Playwright saknas. Installera med: npm install -D playwright');
}
const { chromium } = loadPlaywright();

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, '..');
const fixture = (name) => readFileSync(join(here, 'fixtures', name), 'utf8');

const CHROME_STUB = () => {
  window.__aomMessages = [];
  window.chrome = {
    runtime: {
      id: 'test-extension',
      lastError: null,
      getURL: (p) => `chrome-extension://test/${p}`,
      sendMessage: (message, callback) => {
        window.__aomMessages.push(message);
        if (typeof callback === 'function') callback({ ok: true });
      },
      onMessage: { addListener: () => {} },
      openOptionsPage: () => {},
    },
    storage: { local: { get: async () => ({}), set: async () => {} } },
    tabs: { query: async () => [], create: async () => {} },
  };
};

async function withPage(fixtureName, url, scripts, fn) {
  const browser = await chromium.launch();
  const context = await browser.newContext();
  const page = await context.newPage();
  const html = fixture(fixtureName);
  await page.route('**/*', (route) => route.fulfill({ contentType: 'text/html; charset=utf-8', body: html }));
  await page.addInitScript(CHROME_STUB);
  const errors = [];
  page.on('pageerror', (err) => errors.push(String(err)));
  await page.goto(url);
  for (const script of scripts) await page.addScriptTag({ path: join(root, script) });
  try {
    await fn(page, errors);
  } finally {
    await browser.close();
  }
}

const HISTORY_SCRIPTS = [
  'lib/constants.js',
  'lib/parser-utils.js',
  'lib/status-model.js',
  'content-scripts/order-history.js',
];

test('orderhistoriken parsas till datamodellen', async () => {
  await withPage(
    'order-history.sample.html',
    'https://www.amazon.se/gp/css/order-history',
    HISTORY_SCRIPTS,
    async (page, errors) => {
      const result = await page.evaluate(() => window.AOM.orderHistory.parsePage());
      assert.equal(errors.length, 0, `inga sidfel: ${errors.join('; ')}`);
      assert.equal(result.orders.length, 3, 'tre ordrar hittas');

      const first = result.orders.find((o) => o.orderId === '404-1234567-1234567');
      assert.ok(first, 'ordernummer läses av');
      assert.equal(first.orderDate, '2026-08-03');
      assert.equal(first.totalAmount, 1249);
      assert.equal(first.currency, 'SEK');
      assert.equal(first.sellerId, 'A2XYZ12345');
      assert.equal(first.sellerNameSnapshot, 'Nordic Gadgets AB');
      assert.equal(first.sellerIsThirdParty, true);
      assert.equal(first.lineItems.length, 1);
      assert.equal(first.lineItems[0].asin, 'B0CX23V2ZK');
      assert.equal(first.lineItems[0].status, 'levererad');
      assert.equal(first.lineItems[0].deliveredDate, '2026-08-05');
      assert.equal(first.lineItems[0].quantity, null, 'antal hämtas inte från historikvyn');

      const mixed = result.orders.find((o) => o.orderId === '404-7654321-7654321');
      assert.equal(mixed.lineItems.length, 2);
      assert.deepEqual(
        mixed.lineItems.map((i) => i.status),
        ['levererad', 'pa_vag']
      );
      assert.equal(mixed.lineItems[1].estimatedDeliveryDate, '2026-09-03');
      assert.equal(mixed.sellerIsThirdParty, false, 'Amazon EU räknas inte som tredjepart');

      const cancelled = result.orders.find((o) => o.orderId === '404-1112223-3334445');
      assert.equal(cancelled.lineItems[0].status, 'avbruten');
      assert.equal(cancelled.orderDate, '2026-08-28');
      assert.equal(cancelled.totalAmount, 199);

      assert.ok(result.nextPageUrl.includes('startIndex=10'), 'nästa sida hittas');
    }
  );
});

test('orderhistoriken skickar resultatet till service workern', async () => {
  await withPage(
    'order-history.sample.html',
    'https://www.amazon.se/gp/css/order-history',
    HISTORY_SCRIPTS,
    async (page) => {
      const messages = await page.evaluate(() => window.__aomMessages);
      const parsed = messages.find((m) => m.type === 'orderHistoryParsed');
      assert.ok(parsed, 'ett meddelande skickas vid sidladdning');
      assert.equal(parsed.orders.length, 3);
    }
  );
});

test('orderdetaljsidan ger antal, styckpris, betalsätt och returfrist', async () => {
  await withPage(
    'order-detail.sample.html',
    'https://www.amazon.se/gp/your-account/order-details?orderID=404-7654321-7654321',
    [
      'lib/constants.js',
      'lib/parser-utils.js',
      'lib/status-model.js',
      'content-scripts/order-detail.js',
    ],
    async (page, errors) => {
      const parsed = await page.evaluate(() => window.AOM.orderDetail.parsePage());
      assert.equal(errors.length, 0, `inga sidfel: ${errors.join('; ')}`);
      const order = parsed.order;
      assert.equal(order.orderId, '404-7654321-7654321');
      assert.equal(order.paymentMethodLabel, 'Visa ••4417');
      assert.equal(order.totalAmount, 648.5);
      assert.equal(order.orderDate, '2026-08-21');
      assert.equal(order.lineItems.length, 2);

      const kabel = order.lineItems[0];
      assert.equal(kabel.quantity, 2);
      assert.equal(kabel.unitPrice, 149.25);
      assert.equal(kabel.lineTotal, 298.5);
      assert.equal(kabel.sellerId, 'A9LMN67890');
      assert.equal(kabel.status, 'levererad');
      assert.equal(kabel.returnWindow.amazonPolicyDeadline, '2026-09-20');
      assert.equal(kabel.returnWindow.amazonPolicyDays, null);

      const skydd = order.lineItems[1];
      assert.equal(skydd.quantity, 1);
      assert.equal(skydd.returnWindow.amazonPolicyDays, 30);
      assert.equal(skydd.status, 'pa_vag');

      // Kontrollsumma enligt grinden i steg 3: 2 x 149,25 + 1 x 350 = 648,50.
      assert.equal(order.checksum.lineSum, 648.5);
      assert.equal(order.checksum.ok, true);
      assert.equal(parsed.refund.refundDetected, false);
    }
  );
});

test('returer-sidan upptäcker registrerad återbetalning', async () => {
  await withPage(
    'returns.sample.html',
    'https://www.amazon.se/spr/returns/list',
    ['lib/constants.js', 'lib/parser-utils.js', 'content-scripts/returns-page.js'],
    async (page, errors) => {
      const records = await page.evaluate(() => window.AOM.returnsPage.parsePage());
      assert.equal(errors.length, 0);
      const refunded = records.find((r) => r.orderId === '404-7654321-7654321');
      assert.equal(refunded.refundDetected, true);
      assert.equal(refunded.refundCompleted, true);
      const other = records.find((r) => r.orderId === '404-1234567-1234567');
      assert.equal(other.refundDetected, false);
    }
  );
});

test('produktsidan ger identitet och panelen injiceras under köpblocket', async () => {
  await withPage(
    'product.sample.html',
    'https://www.amazon.se/dp/B0CX23V2ZK',
    [
      'lib/constants.js',
      'lib/parser-utils.js',
      'lib/price-match.js',
      'content-scripts/product-page.js',
    ],
    async (page, errors) => {
      const identity = await page.evaluate(() => window.AOM.productPage.identity());
      assert.equal(errors.length, 0, `inga sidfel: ${errors.join('; ')}`);
      assert.equal(identity.asin, 'B0CX23V2ZK');
      assert.equal(identity.title, 'Samsung 990 PRO 2 TB NVMe M.2 SSD');
      assert.equal(identity.price, 2199);
      assert.equal(identity.currency, 'SEK');
      assert.equal(identity.brand, 'samsung');
      assert.equal(identity.ean, '8806094905366');
      assert.equal(identity.mpn, 'MZ-V9P2T0BW');
      assert.deepEqual(identity.capacityTokens, ['2TB']);
      assert.equal(identity.mainImageUrl, 'https://m.media-amazon.com/images/I/main-hires.jpg');

      const panel = await page.evaluate(() => {
        const el = document.getElementById('aom-product-panel');
        return el
          ? {
              buttons: Array.from(el.querySelectorAll('button')).map((b) => b.textContent),
              afterBuybox: el.previousElementSibling && el.previousElementSibling.id,
            }
          : null;
      });
      assert.ok(panel, 'panelen injiceras');
      assert.equal(panel.afterBuybox, 'desktop_buybox', 'panelen ligger direkt under köpblocket');
      assert.ok(panel.buttons.some((b) => b.includes('Jämför pris')));
    }
  );
});

test('prisjämförelsekedjan: ASIN-träff, variantvarning och ej i katalogen', async () => {
  await withPage(
    'product.sample.html',
    'https://www.amazon.se/dp/B0CX23V2ZK',
    ['lib/constants.js', 'lib/parser-utils.js', 'lib/image-hash.js', 'lib/fx.js', 'lib/price-match.js'],
    async (page) => {
      const result = await page.evaluate(async () => {
        const AOM = window.AOM;
        const source = AOM.priceMatch.extractIdentity(document, location.href);
        const german = document.documentElement.outerHTML
          .replace('2 199,00 kr', '€179,99')
          .replace('Samsung 990 PRO 2 TB', 'Samsung 990 PRO 2 TB');
        const variant = document.documentElement.outerHTML
          .replace('2 199,00 kr', '€99,99')
          .replace(/2 TB/g, '1 TB');
        const deps = {
          parseHtml: (html) => new DOMParser().parseFromString(html, 'text/html'),
          async fetchText(url) {
            if (url.includes('amazon.de')) return { url, text: german };
            if (url.includes('amazon.fr')) return { url, text: '<html><head><title>Sidan hittades inte</title></head><body></body></html>' };
            if (url.includes('amazon.it')) {
              if (url.includes('/s?k=')) {
                return {
                  url,
                  text: '<html><body><div data-asin="B0VARIANT1" data-component-type="s-search-result"><h2>Samsung 990 PRO 1 TB</h2></div></body></html>',
                };
              }
              return { url, text: variant };
            }
            throw new Error('okänd domän i testet');
          },
        };
        const de = await AOM.priceMatch.matchOnDomain(source, 'de', deps, {});
        const fr = await AOM.priceMatch.matchOnDomain(source, 'fr', deps, {});
        const itSource = Object.assign({}, source, { asin: null }); // tvingar EAN-steget
        const it = await AOM.priceMatch.matchOnDomain(itSource, 'it', deps, {});
        return { de, fr, it };
      });

      assert.equal(result.de.status, 'match');
      assert.equal(result.de.method, 'ASIN');
      assert.equal(result.de.price, 179.99);
      assert.equal(result.de.currency, 'EUR');

      assert.equal(result.fr.status, 'not_found', 'saknad produkt ger "ej i katalogen", inte en gissning');

      assert.equal(result.it.method, 'EAN');
      assert.equal(result.it.status, 'variant_uncertain', 'fel kapacitet ska inte visas som träff');
      assert.ok(result.it.verifyReasons.some((r) => r.includes('kapacitet')));
    }
  );
});

test('utgiftsdiagrammet renderar staplar, etikett och tabellvy', async () => {
  await withPage(
    'product.sample.html',
    'https://www.amazon.se/dp/B0CX23V2ZK',
    ['lib/constants.js', 'lib/dates.js', 'lib/chart.js'],
    async (page) => {
      const info = await page.evaluate(() => {
        const orders = [
          { orderDate: '2026-06-04', totalAmount: 400 },
          { orderDate: '2026-07-02', totalAmount: 1249 },
          { orderDate: '2026-08-21', totalAmount: 648.5 },
        ];
        const series = window.AOM.chart.monthlySeries(orders, 12);
        const figure = window.AOM.chart.monthlySpend(series, {});
        document.body.appendChild(figure);
        return {
          bars: figure.querySelectorAll('.aom-chart-bar').length,
          hits: figure.querySelectorAll('.aom-chart-hit').length,
          valueLabels: figure.querySelectorAll('.aom-chart-value').length,
          hasTable: !!figure.querySelector('.aom-chart-table table'),
          ariaLabel: figure.querySelector('svg').getAttribute('aria-label'),
        };
      });
      assert.equal(info.bars, 3);
      assert.equal(info.hits, 3, 'varje stapel har en större träffyta');
      assert.equal(info.valueLabels, 1, 'bara den högsta stapeln får en direktetikett');
      assert.ok(info.hasTable, 'tabellvy finns som alternativ');
      assert.equal(info.ariaLabel, 'Utgifter per månad');
    }
  );
});
