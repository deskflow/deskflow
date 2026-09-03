/* Installationstester: tillägget laddas olåst i en riktig Chromium via
 * Playwright. Detta täcker grindarna som går att verifiera utan ett inloggat
 * Amazon-konto (steg 1, 4, 5, 8, 9 och hash-/lagringsdelen av steg 11).
 *
 * Kör:  PLAYWRIGHT_BROWSERS_PATH=/opt/pw-browsers node --test tests/extension.test.mjs
 */
import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
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
const extensionPath = join(here, '..');

async function withExtension(fn) {
  const userDataDir = mkdtempSync(join(tmpdir(), 'orderkoll-'));
  const context = await chromium.launchPersistentContext(userDataDir, {
    // channel: 'chromium' krävs – headless shell laddar inte tillägg.
    channel: 'chromium',
    headless: true,
    args: [
      `--disable-extensions-except=${extensionPath}`,
      `--load-extension=${extensionPath}`,
      '--no-first-run',
    ],
  });
  try {
    let [worker] = context.serviceWorkers();
    if (!worker) worker = await context.waitForEvent('serviceworker', { timeout: 15000 });
    const extensionId = new URL(worker.url()).host;
    await fn({ context, worker, extensionId });
  } finally {
    await context.close();
    rmSync(userDataDir, { recursive: true, force: true });
  }
}

test('steg 1: tillägget laddas, manifestet valideras och service workern startar', async () => {
  await withExtension(async ({ worker, extensionId }) => {
    assert.match(extensionId, /^[a-p]{32}$/, 'ett giltigt tilläggs-ID tilldelas');
    const manifest = await worker.evaluate(() => chrome.runtime.getManifest());
    assert.equal(manifest.manifest_version, 3);
    assert.deepEqual(manifest.permissions, ['storage', 'unlimitedStorage', 'alarms', 'notifications']);
    assert.ok(manifest.host_permissions.includes('*://*.amazon.se/*'));
    assert.ok(manifest.action.default_popup.endsWith('popup.html'));
    const iconOk = await worker.evaluate(async () => {
      const res = await fetch(chrome.runtime.getURL('icons/icon-128.png'));
      return res.ok;
    });
    assert.ok(iconOk, 'ikonen finns och kan läsas');
  });
});

test('steg 1: alarmet skapas med inställt intervall', async () => {
  await withExtension(async ({ worker }) => {
    const alarm = await worker.evaluate(async () => {
      await self.AOM.sw.ensureAlarm();
      return chrome.alarms.get('aom-order-sync');
    });
    assert.ok(alarm, 'ett alarm finns');
    assert.equal(alarm.periodInMinutes, 30);
  });
});

test('steg 4: lagrad data överlever att kontexten byggs om', async () => {
  await withExtension(async ({ worker }) => {
    await worker.evaluate(async () => {
      await self.AOM.storage.upsertOrders([
        {
          orderId: '404-5550000-5550000',
          orderDate: '2026-08-15',
          totalAmount: 299,
          currency: 'SEK',
          lineItems: [
            { asin: 'BTEST00001', lineKey: 'BTEST00001:0', title: 'Testvara', status: 'levererad' },
          ],
        },
      ]);
    });
    const readBack = await worker.evaluate(async () => {
      const raw = await chrome.storage.local.get('orders');
      return raw.orders['404-5550000-5550000'];
    });
    assert.equal(readBack.totalAmount, 299);
    assert.equal(readBack.lineItems[0].title, 'Testvara');
  });
});

test('steg 5: popupen visar sparad data utanför amazon.se och gråar ut synk-knappen', async () => {
  await withExtension(async ({ context, worker, extensionId }) => {
    await worker.evaluate(async () => {
      await self.AOM.storage.upsertOrders([
        {
          orderId: '404-5550000-5550000',
          orderDate: '2026-08-15',
          totalAmount: 299,
          currency: 'SEK',
          sellerNameSnapshot: 'Testsäljare',
          lineItems: [
            { asin: 'BTEST00001', lineKey: 'BTEST00001:0', title: 'Testvara', status: 'levererad' },
          ],
        },
      ]);
    });

    const page = await context.newPage();
    await page.route('https://example.com/**', (route) =>
      route.fulfill({ contentType: 'text/html', body: '<html><body>inte amazon</body></html>' })
    );
    await page.goto('https://example.com/');

    const popup = await context.newPage();
    await popup.goto(`chrome-extension://${extensionId}/popup/popup.html`);
    await popup.waitForFunction(() => document.getElementById('count-orders').textContent !== '0');

    assert.equal(await popup.locator('#count-orders').textContent(), '1');
    assert.equal(await popup.locator('#count-pending').textContent(), '1', 'levererad + obekräftad hamnar i kön');
    assert.ok((await popup.locator('.pop-title').first().textContent()).includes('Testvara'));
    assert.equal(await popup.locator('#sync').isDisabled(), true, 'synk är gråad utanför orderhistoriken');
    assert.equal(
      await popup.locator('#sync-hint').textContent(),
      'Gå till dina ordrar på amazon.se, öppna tillägget och tryck Synka för att uppdatera.'
    );
    assert.ok((await popup.locator('#disclaimer').textContent()).includes('oberoende'));
  });
});

test('steg 6: mottagningsbekräftelse tar bort posten ur kön men behåller ordern', async () => {
  await withExtension(async ({ worker }) => {
    const result = await worker.evaluate(async () => {
      await self.AOM.storage.upsertOrders([
        {
          orderId: '404-6660000-6660000',
          orderDate: '2026-08-20',
          lineItems: [{ asin: 'BTEST00002', lineKey: 'BTEST00002:0', title: 'Kvitterad vara', status: 'levererad' }],
        },
      ]);
      const before = await self.AOM.storage.getState();
      const pendingBefore = self.AOM.status.pendingConfirmations(self.AOM.storage.orderList(before)).length;

      await self.AOM.storage.setLineFlag('404-6660000-6660000', 'BTEST00002:0', {
        userReceivedConfirmed: true,
        userReceivedConfirmedAt: new Date().toISOString(),
      });

      const after = await self.AOM.storage.getState();
      const orders = self.AOM.storage.orderList(after);
      return {
        pendingBefore,
        pendingAfter: self.AOM.status.pendingConfirmations(orders).length,
        stillListed: orders.some((o) => o.orderId === '404-6660000-6660000'),
        confirmed: after.orders['404-6660000-6660000'].lineItems[0].userReceivedConfirmed,
      };
    });
    assert.equal(result.pendingBefore, 1);
    assert.equal(result.pendingAfter, 0);
    assert.equal(result.stillListed, true, 'ordern finns kvar i huvudlistan');
    assert.equal(result.confirmed, true);
  });
});

test('steg 7: kryssrutan tar bort avvikelseflaggan, utan kryss flaggas den', async () => {
  await withExtension(async ({ worker }) => {
    const result = await worker.evaluate(async () => {
      await self.AOM.storage.upsertOrders([
        {
          orderId: '404-7770000-7770000',
          orderDate: '2026-08-25',
          lineItems: [{ asin: 'BTEST00003', lineKey: 'BTEST00003:0', title: 'Ej levererad vara', status: 'pa_vag' }],
        },
      ]);
      await self.AOM.storage.upsertRefundRecords([
        { orderId: '404-7770000-7770000', refundDetected: true, evidenceText: 'Återbetalning utfärdad' },
      ]);
      const stateA = await self.AOM.storage.getState();
      const flaggedWithout = self.AOM.anomaly.evaluateAll(stateA.orders, stateA.refundRecords).anomalies.filter(
        (a) => a.type === 'refund_without_return_request'
      ).length;

      await self.AOM.storage.setOrderFlag('404-7770000-7770000', {
        userConfirmedReturnOrRefundRequested: true,
      });
      const stateB = await self.AOM.storage.getState();
      const flaggedWith = self.AOM.anomaly.evaluateAll(stateB.orders, stateB.refundRecords).anomalies.filter(
        (a) => a.type === 'refund_without_return_request'
      ).length;
      return { flaggedWithout, flaggedWith };
    });
    assert.equal(result.flaggedWithout, 1, 'återbetalning utan kryss ger flagga');
    assert.equal(result.flaggedWith, 0, 'kryssrutan vinner över skrapningen');
  });
});

test('steg 8: kön kör sekventiellt med paus mellan hämtningar, aldrig parallellt', async () => {
  await withExtension(async ({ worker }) => {
    const trace = await worker.evaluate(async () => {
      const queue = new self.AOM.SyncQueue({ minDelayMs: 300, maxDelayMs: 500 });
      const events = [];
      let concurrent = 0;
      let maxConcurrent = 0;
      const job = (name) => ({
        type: 'test',
        key: name,
        network: true,
        run: async () => {
          concurrent += 1;
          maxConcurrent = Math.max(maxConcurrent, concurrent);
          events.push({ name, start: Date.now() });
          await new Promise((r) => setTimeout(r, 40));
          concurrent -= 1;
        },
      });
      queue.enqueue(job('a'));
      queue.enqueue(job('b'));
      queue.enqueue(job('c'));
      await new Promise((r) => setTimeout(r, 2500));
      return { events, maxConcurrent, log: queue.log.map((l) => l.event) };
    });

    assert.equal(trace.events.length, 3, 'alla tre jobben kördes');
    assert.equal(trace.maxConcurrent, 1, 'aldrig två samtidiga hämtningar');
    const gapAB = trace.events[1].start - trace.events[0].start;
    const gapBC = trace.events[2].start - trace.events[1].start;
    assert.ok(gapAB >= 300, `paus mellan jobb 1 och 2: ${gapAB} ms`);
    assert.ok(gapBC >= 300, `paus mellan jobb 2 och 3: ${gapBC} ms`);
    assert.ok(trace.log.some((e) => e.startsWith('waiting_')), 'pausen syns i service worker-loggen');
  });
});

test('steg 8: CAPTCHA-svar pausar kön i stället för att hamra vidare', async () => {
  await withExtension(async ({ worker }) => {
    const result = await worker.evaluate(async () => {
      const queue = new self.AOM.SyncQueue({ minDelayMs: 10, maxDelayMs: 20 });
      let ran = 0;
      queue.enqueue({
        type: 'test',
        key: 'wall',
        run: async () => {
          const err = new Error('CAPTCHA');
          err.wall = 'captcha';
          throw err;
        },
      });
      queue.enqueue({ type: 'test', key: 'efter', run: async () => { ran += 1; } });
      await new Promise((r) => setTimeout(r, 400));
      return { paused: queue.paused, reason: queue.pauseReason, ran, kvar: queue.size() };
    });
    assert.equal(result.paused, true);
    assert.equal(result.reason, 'captcha');
    assert.equal(result.ran, 0, 'inga fler anrop görs efter en vägg');
  });
});

test('steg 9: overlayen isolerar sin CSS i Shadow DOM', async () => {
  await withExtension(async ({ context, worker }) => {
    const page = await context.newPage();
    await page.route('https://www.amazon.se/**', (route) =>
      route.fulfill({
        contentType: 'text/html; charset=utf-8',
        body: `<!doctype html><html lang="sv"><head><style>
          * { color: rgb(255, 0, 0) !important; font-size: 33px; }
          .aom-card { border: 9px dashed rgb(0, 255, 0); }
        </style></head><body><h1 id="amazon-rubrik">Amazons egen sida</h1></body></html>`,
      })
    );
    await page.goto('https://www.amazon.se/gp/css/order-history');

    // Samma väg som popupen använder: service workern ber content scriptet
    // att fälla ut panelen. (Content scripts lever i en isolerad värld och
    // kan inte nås direkt med page.evaluate.)
    const sent = await worker.evaluate(async () => {
      const tabs = await chrome.tabs.query({ url: '*://*.amazon.se/*' });
      if (!tabs.length) return 'ingen flik';
      await chrome.tabs.sendMessage(tabs[0].id, { type: 'toggleOverlay', view: 'oversikt' });
      return 'skickat';
    });
    assert.equal(sent, 'skickat');

    await page.waitForFunction(
      () => {
        const host = document.getElementById('aom-overlay-host');
        return !!(host && host.shadowRoot && host.shadowRoot.querySelector('.aom-nav-item'));
      },
      null,
      { timeout: 10000 }
    );

    const leak = await page.evaluate(() => {
      const host = document.getElementById('aom-overlay-host');
      const sidebar = host.shadowRoot.querySelector('.aom-sidebar');
      const navItem = host.shadowRoot.querySelector('.aom-nav-item');
      const pageHeading = document.getElementById('amazon-rubrik');
      return {
        sidebarBg: getComputedStyle(sidebar).backgroundColor,
        navColor: getComputedStyle(navItem).color,
        navFontSize: getComputedStyle(navItem).fontSize,
        pageColor: getComputedStyle(pageHeading).color,
        pageFontSize: getComputedStyle(pageHeading).fontSize,
        overlayCount: document.querySelectorAll('#aom-overlay-host').length,
        views: Array.from(host.shadowRoot.querySelectorAll('.aom-nav-item')).map((b) =>
          b.textContent.trim()
        ),
      };
    });

    assert.equal(leak.sidebarBg, 'rgb(35, 47, 62)', 'sidokolumnens egen färg står emot sidans !important-regel');
    assert.notEqual(leak.navColor, 'rgb(255, 0, 0)', 'Amazons färgregel läcker inte in');
    assert.notEqual(leak.navFontSize, '33px', 'Amazons typsnittsstorlek läcker inte in');
    assert.equal(leak.pageColor, 'rgb(255, 0, 0)', 'sidans egen stil är orörd');
    assert.equal(leak.pageFontSize, '33px', 'tilläggets CSS läcker inte ut till sidan');
    assert.equal(leak.overlayCount, 1);
    assert.deepEqual(leak.views, [
      'Översikt',
      'Alla ordrar',
      'Att bekräfta',
      'Avvikelser',
      'Bevakningar',
      'Bevisarkiv',
      'Säljare',
      'Inställningar',
    ]);
  });
});

test('steg 11: bevispaket hashas, lagras i IndexedDB och kan verifieras', async () => {
  await withExtension(async ({ worker }) => {
    const result = await worker.evaluate(async () => {
      const bytes = new Uint8Array([137, 80, 78, 71, 1, 2, 3, 4]);
      const bundle = await self.AOM.proof.captureBundle(
        {
          kind: self.AOM.proof.KIND.PRODUCT_AT_PURCHASE,
          orderId: '404-8880000-8880000',
          asin: 'BTEST00004',
          pageUrl: 'https://www.amazon.se/dp/BTEST00004',
          html: '<html><body><h1>Testprodukt</h1></body></html>',
          imageUrls: ['https://m.media-amazon.com/images/I/test.jpg'],
          meta: { title: 'Testprodukt', price: 499, currency: 'SEK' },
        },
        { fetchBinary: async () => ({ bytes, type: 'image/jpeg' }) }
      );
      const verification = await self.AOM.proof.verifyBundle(bundle.id);
      const index = await chrome.storage.local.get('proofIndex');
      const stored = await self.AOM.db.getBundle(bundle.id);
      const blobs = await self.AOM.db.blobsForBundle(bundle.id);

      // Manipulera en fil och kontrollera att verifieringen upptäcker det.
      const html = blobs.find((b) => b.name === 'sida.html');
      await self.AOM.db.putBlob(
        Object.assign({}, html, { blob: new Blob(['<html>manipulerad</html>'], { type: 'text/html' }) })
      );
      const afterTamper = await self.AOM.proof.verifyBundle(bundle.id);

      return {
        sha256: bundle.sha256,
        fileNames: bundle.files.map((f) => f.name),
        verification,
        indexed: !!index.proofIndex[bundle.id],
        storedOrderId: stored.orderId,
        blobCount: blobs.length,
        afterTamper,
      };
    });

    assert.match(result.sha256, /^[0-9a-f]{64}$/, 'SHA-256 beräknas över paketet');
    assert.deepEqual(result.fileNames.sort(), ['bilder/01.jpeg', 'metadata.json', 'sida.html']);
    assert.equal(result.verification.ok, true);
    assert.equal(result.indexed, true, 'paketet indexeras i chrome.storage för snabb sökning');
    assert.equal(result.storedOrderId, '404-8880000-8880000');
    assert.equal(result.blobCount, 3);
    assert.equal(result.afterTamper.ok, false, 'manipulerat innehåll upptäcks');
    assert.ok(result.afterTamper.problems.some((p) => p.includes('sida.html')));
  });
});

test('notiser batchas till en per synkomgång', async () => {
  await withExtension(async ({ worker }) => {
    const created = await worker.evaluate(async () => {
      const calls = [];
      const original = chrome.notifications.create;
      chrome.notifications.create = async (id, opts) => {
        calls.push(opts);
        return id;
      };
      await self.AOM.storage.upsertOrders([
        { orderId: '404-9990000-9990000', orderDate: '2026-08-30', lineItems: [{ asin: 'B1', lineKey: 'B1:0', status: 'pa_vag' }] },
        { orderId: '404-9990001-9990001', orderDate: '2026-08-30', lineItems: [{ asin: 'B2', lineKey: 'B2:0', status: 'pa_vag' }] },
      ]);
      // Tre förändringar i samma omgång ska ge EN notis.
      await self.AOM.sw.refreshBadge();
      const changes = [
        { type: 'new_order', orderId: '404-9990000-9990000' },
        { type: 'new_order', orderId: '404-9990001-9990001' },
        { type: 'status_change', orderId: '404-9990000-9990000', to: 'levererad', title: 'Vara' },
      ];
      await self.__aomAfterChanges(changes, 'test');
      chrome.notifications.create = original;
      return calls;
    });
    assert.equal(created.length, 1, 'en sammanfattande notis, inte en per händelse');
    assert.match(created[0].title, /3 uppdateringar/);
  });
});
