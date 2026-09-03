/* Enhetstester för den rena logiken (ingen webbläsare inblandad).
 * Kör med:  node --test tests/
 * DOM-parsning testas separat i tests/dom.test.mjs (Playwright + Chromium). */
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const path = require('node:path');

globalThis.self = globalThis;

/** Minimal chrome.storage.local i minnet, för lagringstesterna. */
const memory = {};
globalThis.chrome = {
  storage: {
    local: {
      async get(keys) {
        if (keys === null || keys === undefined) return Object.assign({}, memory);
        const list = Array.isArray(keys) ? keys : [keys];
        const out = {};
        for (const key of list) if (key in memory) out[key] = memory[key];
        return out;
      },
      async set(obj) {
        Object.assign(memory, obj);
      },
      async clear() {
        for (const key of Object.keys(memory)) delete memory[key];
      },
      async getBytesInUse() {
        return JSON.stringify(memory).length;
      },
    },
  },
};

const LIB = path.join(__dirname, '..', 'lib');
for (const file of [
  'constants',
  'parser-utils',
  'status-model',
  'dates',
  'storage',
  'anomaly-detection',
  'image-hash',
  'fx',
  'zip',
  'price-match',
]) {
  require(path.join(LIB, `${file}.js`));
}
const AOM = globalThis.AOM;
const P = AOM.parse;

test('parseMoney tolkar svenska och europeiska prisformat', () => {
  assert.deepEqual(P.parseMoney('1 249,00 kr'), { amount: 1249, currency: 'SEK', raw: '1 249,00 kr' });
  assert.equal(P.parseMoney('SEK 1.234,56').amount, 1234.56);
  assert.equal(P.parseMoney('€12,99').currency, 'EUR');
  assert.equal(P.parseMoney('£9.99').amount, 9.99);
  assert.equal(P.parseMoney('1,234.56').amount, 1234.56);
  assert.equal(P.parseMoney('1,234.56').currency, null, 'gissar aldrig valuta');
  assert.equal(P.parseMoney('inget pris här'), null);
});

test('parseSwedishDate gissar aldrig årtal', () => {
  assert.equal(P.parseSwedishDate('3 september 2026'), '2026-09-03');
  assert.equal(P.parseSwedishDate('3 sep. 2026'), '2026-09-03');
  assert.equal(P.parseSwedishDate('2026-09-03'), '2026-09-03');
  assert.equal(P.parseSwedishDate('3 september'), null);
  assert.equal(P.parseSwedishDate('Levererad 24 augusti 2026'), '2026-08-24');
});

test('identifierare plockas ur länkar och text', () => {
  assert.equal(P.extractAsin('/Samsung-990/dp/B0CX23V2ZK/ref=x'), 'B0CX23V2ZK');
  assert.equal(P.extractAsin('/gp/product/B08KFQ9HK5'), 'B08KFQ9HK5');
  assert.equal(P.extractAsin('/inget/här'), null);
  assert.equal(P.extractOrderId('Ordernummer 404-1234567-1234567'), '404-1234567-1234567');
  assert.equal(P.extractSellerId('/sp?seller=A2XYZ12345&ref=odr'), 'A2XYZ12345');
  assert.equal(P.parseEan('EAN: 8806094905366'), '8806094905366');
  assert.equal(P.parseEan('123'), null);
});

test('kapacitetsord normaliseras för variantverifiering', () => {
  assert.deepEqual(P.capacityTokens('Samsung 990 PRO 2 TB SSD'), ['2TB']);
  assert.deepEqual(P.capacityTokens('T-shirt XL 100 % bomull'), ['XL']);
});

test('statusmodellen normaliserar och svarar okänt när den inte vet', () => {
  const S = AOM.STATUS;
  assert.equal(AOM.status.normalizeStatus('Levererad 5 augusti').status, S.DELIVERED);
  assert.equal(AOM.status.normalizeStatus('På väg – beräknad leverans 3 sep').status, S.ON_WAY);
  assert.equal(AOM.status.normalizeStatus('Beräknad leverans imorgon').status, S.NOT_SHIPPED);
  assert.equal(AOM.status.normalizeStatus('Avbruten').status, S.CANCELLED);
  assert.equal(AOM.status.normalizeStatus('Hejsan hoppsan').status, null);
});

test('sammansatt statusetikett för blandade ordrar', () => {
  const label = AOM.status.compositeLabel([
    { status: 'levererad' },
    { status: 'levererad' },
    { status: 'pa_vag' },
  ]);
  assert.equal(label.text, '2 av 3 levererade');
});

test('de tre returfönstren hålls isär', () => {
  assert.equal(AOM.dates.angerrattDeadline('2026-09-03'), '2026-09-18');
  assert.equal(AOM.dates.reklamationDeadline('2026-09-03'), '2029-09-03');
  assert.equal(AOM.dates.presumtionDeadline('2026-09-03'), '2027-03-03');
  assert.equal(AOM.dates.angerrattDeadline(null), null, 'utan mottagandedatum: inget påhittat datum');
  assert.equal(AOM.dates.bankDisputeDeadline('2026-09-03', undefined), null, 'ingen hårdkodad bankfrist');
  assert.equal(AOM.dates.bankDisputeDeadline('2026-09-03', 120), '2027-01-01');
});

test('returnWindows läser Amazons policy men beräknar den aldrig', () => {
  const item = {
    userReceivedConfirmedAt: '2026-08-24T10:00:00.000Z',
    returnWindow: { amazonPolicyDays: null, amazonPolicyDeadline: '2026-09-20', sourceText: 'Returnerbar t.o.m. 20 september 2026' },
  };
  const w = AOM.dates.returnWindows(item, { orderDate: '2026-08-21' }, AOM.DEFAULT_SETTINGS);
  assert.equal(w.amazonPolicyDeadline, '2026-09-20');
  assert.equal(w.angerrattDeadline, '2026-09-08');
  assert.equal(w.bankDisputeDeadline, null);
});

test('avvikelse: återbetalning utan begärd retur på ej levererad artikel', () => {
  const order = {
    orderId: '404-1',
    userConfirmedReturnOrRefundRequested: false,
    lineItems: [{ lineKey: 'A:0', status: AOM.STATUS.ON_WAY, title: 'Skärmskydd' }],
  };
  const hits = AOM.anomaly.evaluateOrder(order, { refundDetected: true }, { todayIso: '2026-09-03' });
  assert.equal(hits.length, 1);
  assert.equal(hits[0].type, AOM.anomaly.TYPE.REFUND_WITHOUT_REQUEST);
});

test('avvikelse: kryssrutan slår alltid skrapningen', () => {
  const order = {
    orderId: '404-1',
    userConfirmedReturnOrRefundRequested: true,
    lineItems: [{ lineKey: 'A:0', status: AOM.STATUS.ON_WAY }],
  };
  const hits = AOM.anomaly.evaluateOrder(
    order,
    { refundDetected: true, returnRequestedDetected: false },
    { todayIso: '2026-09-03' }
  );
  assert.equal(hits.filter((h) => h.type === AOM.anomaly.TYPE.REFUND_WITHOUT_REQUEST).length, 0);
});

test('avvikelse: levererad artikel med återbetalning flaggas inte', () => {
  const order = {
    orderId: '404-2',
    userConfirmedReturnOrRefundRequested: false,
    lineItems: [{ lineKey: 'A:0', status: AOM.STATUS.DELIVERED }],
  };
  const hits = AOM.anomaly.evaluateOrder(order, { refundDetected: true }, { todayIso: '2026-09-03' });
  assert.equal(hits.length, 0);
});

test('försenad leverans flaggas separat', () => {
  const order = {
    orderId: '404-3',
    lineItems: [{ lineKey: 'A:0', status: AOM.STATUS.ON_WAY, estimatedDeliveryDate: '2026-08-01' }],
  };
  const hits = AOM.anomaly.evaluateOrder(order, undefined, { todayIso: '2026-09-03' });
  assert.equal(hits[0].type, AOM.anomaly.TYPE.DELAYED_DELIVERY);
});

test('säljarräknare summerar avvikelser per säljar-ID', () => {
  const orders = [
    {
      orderId: '404-a',
      sellerId: 'A1',
      lineItems: [{ lineKey: 'x:0', status: AOM.STATUS.ON_WAY }],
    },
    {
      orderId: '404-b',
      sellerId: 'A1',
      lineItems: [{ lineKey: 'y:0', status: AOM.STATUS.NOT_SHIPPED }],
    },
  ];
  const refunds = { '404-a': { refundDetected: true }, '404-b': { refundDetected: true } };
  const { sellerCounts } = AOM.anomaly.evaluateAll(orders, refunds, { todayIso: '2026-09-03' });
  assert.equal(sellerCounts.A1, 2);
});

test('lagring: användarens bekräftelser överlever en ny skrapning', async () => {
  await chrome.storage.local.clear();
  await AOM.storage.upsertOrders([
    {
      orderId: '404-9',
      orderDate: '2026-08-01',
      lineItems: [{ asin: 'B001', lineKey: 'B001:0', status: AOM.STATUS.DELIVERED, title: 'Vara' }],
    },
  ]);
  await AOM.storage.setLineFlag('404-9', 'B001:0', {
    userReceivedConfirmed: true,
    userReceivedConfirmedAt: '2026-08-05T12:00:00.000Z',
  });
  await AOM.storage.setOrderFlag('404-9', { userConfirmedReturnOrRefundRequested: true });

  await AOM.storage.upsertOrders([
    {
      orderId: '404-9',
      orderDate: '2026-08-01',
      lineItems: [{ asin: 'B001', lineKey: 'B001:0', status: AOM.STATUS.DELIVERED, title: 'Vara', quantity: 2 }],
    },
  ]);
  const state = await AOM.storage.getState();
  const order = state.orders['404-9'];
  assert.equal(order.userConfirmedReturnOrRefundRequested, true);
  assert.equal(order.lineItems[0].userReceivedConfirmed, true);
  assert.equal(order.lineItems[0].quantity, 2, 'nya Amazon-fält skrivs in');
});

test('lagring: statusändringar rapporteras som förändringar', async () => {
  await chrome.storage.local.clear();
  await AOM.storage.upsertOrders([
    { orderId: '404-8', lineItems: [{ asin: 'B002', lineKey: 'B002:0', status: AOM.STATUS.ON_WAY }] },
  ]);
  const res = await AOM.storage.upsertOrders([
    { orderId: '404-8', lineItems: [{ asin: 'B002', lineKey: 'B002:0', status: AOM.STATUS.DELIVERED }] },
  ]);
  const change = res.changes.find((c) => c.type === 'status_change');
  assert.ok(change, 'statusändring ska rapporteras');
  assert.equal(change.to, AOM.STATUS.DELIVERED);
});

test('prisjämförelse: variantverifiering fäller fel kapacitet', () => {
  const source = { brand: 'samsung', brandRaw: 'Samsung', capacityTokens: ['2TB'] };
  const same = { brand: 'samsung', brandRaw: 'Samsung', capacityTokens: ['2TB'] };
  const other = { brand: 'samsung', brandRaw: 'Samsung', capacityTokens: ['1TB'] };
  assert.equal(AOM.priceMatch.verifyCandidate(source, same).ok, true);
  assert.equal(AOM.priceMatch.verifyCandidate(source, other).ok, false);
});

test('ECB-kurser tolkas och omräkning kräver båda valutorna', () => {
  const xml = `<Cube time='2026-09-02'><Cube currency='SEK' rate='11.2345'/><Cube currency='GBP' rate='0.8543'/><Cube currency='USD' rate='1.0821'/><Cube currency='DKK' rate='7.46'/><Cube currency='NOK' rate='11.7'/></Cube>`;
  const parsed = AOM.fx.parseEcbXml(xml);
  assert.equal(parsed.date, '2026-09-02');
  assert.equal(parsed.rates.SEK, 11.2345);
  const sek = AOM.fx.convert(100, 'EUR', 'SEK', parsed.rates);
  assert.ok(Math.abs(sek - 1123.45) < 0.01);
  assert.equal(AOM.fx.convert(100, 'JPY', 'SEK', parsed.rates), null, 'okänd valuta ger null');
});

test('bildhash: likhet mellan identiska och olika hashar', () => {
  const a = 'ffffffffffffffff';
  const b = 'fffffffffffffffe';
  assert.equal(AOM.imageHash.similarity(a, a), 1);
  assert.ok(AOM.imageHash.similarity(a, b) < 1);
  assert.equal(AOM.imageHash.similarity(a, null), null);
});

test('ZIP-skrivaren producerar giltiga poster', () => {
  const enc = new TextEncoder();
  const out = AOM.zip.createZip([{ name: 'a.txt', data: enc.encode('hej') }]);
  const bytes = out instanceof Uint8Array ? out : null;
  assert.ok(AOM.zip.crc32(enc.encode('hej')) > 0);
  if (bytes) assert.equal(bytes[0], 0x50);
});
