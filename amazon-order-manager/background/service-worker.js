/* Orderkoll – service worker (MV3).
 * Ansvar: alarm, den gemensamma kön, hämtningar mot Amazon, notiser, badge,
 * bevisarkivets skrivningar och all lagring. Content scripts skrapar; här
 * fattas besluten. */
importScripts(
  '/lib/constants.js',
  '/lib/parser-utils.js',
  '/lib/status-model.js',
  '/lib/dates.js',
  '/lib/storage.js',
  '/lib/db.js',
  '/lib/anomaly-detection.js',
  '/lib/sync-queue.js',
  '/lib/image-hash.js',
  '/lib/fx.js',
  '/lib/zip.js',
  '/lib/proof-capture.js'
);

(function () {
  'use strict';
  const AOM = self.AOM;
  const MSG = AOM.MSG;
  const { sleep, fetchText } = AOM.queueUtils;

  let queue = null;
  const waiters = [];

  async function getQueue() {
    if (queue) return queue;
    const settings = await AOM.storage.getSettings();
    queue = new AOM.SyncQueue({
      minDelayMs: settings.requestDelayMinMs,
      maxDelayMs: settings.requestDelayMaxMs,
      onEvent: async (event) => {
        if (event.type === 'paused') {
          await AOM.storage.setSyncState({ blockedReason: event.reason, running: false });
          await refreshBadge();
        }
      },
    });
    return queue;
  }

  /* ---------------------------------------------------------------- alarm */

  async function ensureAlarm() {
    const settings = await AOM.storage.getSettings();
    const existing = await chrome.alarms.get(AOM.ALARM_SYNC);
    if (!settings.backgroundSyncEnabled) {
      if (existing) await chrome.alarms.clear(AOM.ALARM_SYNC);
      return;
    }
    const minutes = Math.max(1, Number(settings.syncIntervalMinutes) || 30);
    if (existing && existing.periodInMinutes === minutes) return;
    await chrome.alarms.clear(AOM.ALARM_SYNC);
    // chrome.alarms tillåter minst 30 sekunder; vi ligger långt över.
    chrome.alarms.create(AOM.ALARM_SYNC, { periodInMinutes: minutes, delayInMinutes: 1 });
    console.info('[Orderkoll] alarm satt till var', minutes, 'minut(er)');
  }

  chrome.runtime.onInstalled.addListener(async () => {
    await ensureAlarm();
    await refreshBadge();
  });

  chrome.runtime.onStartup.addListener(async () => {
    // Alarm överlever inte garanterat en omstart – återskapa alltid.
    await ensureAlarm();
    await refreshBadge();
  });

  chrome.alarms.onAlarm.addListener(async (alarm) => {
    if (alarm.name !== AOM.ALARM_SYNC) return;
    await runBackgroundSync('alarm');
    await maintenance();
  });

  /* ------------------------------------------------------------- bakgrund */

  function waitForTabMessage(tabId, type, timeoutMs) {
    return new Promise((resolve, reject) => {
      const waiter = { tabId, type, resolve, reject };
      waiters.push(waiter);
      waiter.timer = setTimeout(() => {
        const idx = waiters.indexOf(waiter);
        if (idx >= 0) waiters.splice(idx, 1);
        reject(new Error(`Tidsgräns: inget "${type}" från flik ${tabId}`));
      }, timeoutMs || 30000);
    });
  }

  function resolveWaiters(sender, message) {
    const tabId = sender && sender.tab ? sender.tab.id : null;
    if (tabId === null) return;
    for (let i = waiters.length - 1; i >= 0; i -= 1) {
      const w = waiters[i];
      if (w.tabId === tabId && w.type === message.type) {
        clearTimeout(w.timer);
        waiters.splice(i, 1);
        w.resolve(message);
      }
    }
  }

  async function openBackgroundTab(url, mode) {
    if (mode === 'minimized-window') {
      const win = await chrome.windows.create({ url, state: 'minimized' });
      return { tabId: win.tabs[0].id, windowId: win.id };
    }
    const tab = await chrome.tabs.create({ url, active: false });
    return { tabId: tab.id, windowId: null };
  }

  async function closeBackgroundTab(handle) {
    try {
      if (handle.windowId) await chrome.windows.remove(handle.windowId);
      else await chrome.tabs.remove(handle.tabId);
    } catch (_err) {
      /* fliken kan redan vara stängd */
    }
  }

  /**
   * En synkomgång: orderhistoriken, ett fåtal orderdetaljsidor och
   * returer-sidan – sekventiellt, i samma flik, med paus emellan.
   */
  async function runBackgroundSync(source) {
    const settings = await AOM.storage.getSettings();
    const syncState = await AOM.storage.getSyncState();
    if (syncState.running) {
      console.info('[Orderkoll] synk pågår redan, hoppar över');
      return { skipped: true };
    }
    await AOM.storage.setSyncState({ running: true, lastError: null });
    const changes = [];
    let handle = null;
    try {
      handle = await openBackgroundTab(AOM.ORDER_HISTORY_URL, settings.backgroundTabMode);
      const historyMsg = await waitForTabMessage(handle.tabId, MSG.ORDER_HISTORY_PARSED, 45000);
      if (historyMsg.error) throw Object.assign(new Error(historyMsg.error), { wall: historyMsg.error });
      const res = await AOM.storage.upsertOrders(historyMsg.orders || []);
      changes.push(...res.changes);

      // Orderdetaljer för de ordrar som saknar antal/styckpris.
      const state = await AOM.storage.getState();
      const needsDetail = AOM.storage
        .orderList(state)
        .filter((o) => (o.lineItems || []).some((i) => i.quantity === null))
        .slice(0, Math.max(0, settings.maxDetailVisitsPerSync));

      for (const order of needsDetail) {
        const url =
          order.detailUrl ||
          `https://www.amazon.se/gp/your-account/order-details?orderID=${order.orderId}`;
        await sleep(AOM.queueUtils.randomBetween(settings.requestDelayMinMs, settings.requestDelayMaxMs));
        await chrome.tabs.update(handle.tabId, { url });
        try {
          const detailMsg = await waitForTabMessage(handle.tabId, MSG.ORDER_DETAIL_PARSED, 45000);
          if (detailMsg.order) {
            const merged = await AOM.storage.upsertOrders([detailMsg.order]);
            changes.push(...merged.changes);
          }
          if (detailMsg.refund) {
            const refundRes = await AOM.storage.upsertRefundRecords([detailMsg.refund]);
            changes.push(...refundRes.changes);
          }
        } catch (err) {
          console.warn('[Orderkoll] orderdetalj hoppades över:', err.message);
        }
      }

      // Returer & beställningar.
      await sleep(AOM.queueUtils.randomBetween(settings.requestDelayMinMs, settings.requestDelayMaxMs));
      await chrome.tabs.update(handle.tabId, { url: AOM.RETURNS_URL });
      try {
        const returnsMsg = await waitForTabMessage(handle.tabId, MSG.RETURNS_PARSED, 45000);
        if (returnsMsg.records && returnsMsg.records.length) {
          const refundRes = await AOM.storage.upsertRefundRecords(returnsMsg.records);
          changes.push(...refundRes.changes);
        }
      } catch (err) {
        console.warn('[Orderkoll] returer-sidan hoppades över:', err.message);
      }

      await AOM.storage.setSyncState({
        running: false,
        lastSyncAt: new Date().toISOString(),
        lastSyncSource: source,
        blockedReason: null,
        lastError: null,
      });
    } catch (err) {
      const wall = err && err.wall ? err.wall : null;
      await AOM.storage.setSyncState({
        running: false,
        lastError: String((err && err.message) || err),
        blockedReason: wall,
        lastSyncSource: source,
      });
      console.warn('[Orderkoll] synk misslyckades:', err);
    } finally {
      if (handle) await closeBackgroundTab(handle);
    }

    await afterChanges(changes, source);
    return { changes: changes.length };
  }

  /** Kör avvikelselogiken, uppdaterar säljarräknare, badge och EN notis. */
  async function afterChanges(changes, source) {
    const state = await AOM.storage.getState();
    const { anomalies, sellerCounts } = AOM.anomaly.evaluateAll(state.orders, state.refundRecords);

    await AOM.storage.mutate(
      AOM.STORAGE_KEYS.SELLERS,
      (current) => {
        const sellers = Object.assign({}, current || {});
        for (const [sellerId, count] of Object.entries(sellerCounts)) {
          if (!sellers[sellerId]) sellers[sellerId] = { sellerId, purchaseCount: 0, purchaseOrderIds: [] };
          sellers[sellerId] = Object.assign({}, sellers[sellerId], { flaggedIssueCount: count });
        }
        for (const id of Object.keys(sellers)) {
          if (!sellerCounts[id]) sellers[id] = Object.assign({}, sellers[id], { flaggedIssueCount: 0 });
        }
        return sellers;
      },
      {}
    );

    await refreshBadge();

    const settings = await AOM.storage.getSettings();
    const notable = changes.filter((c) => c.type !== 'noop');
    if (!settings.notificationsEnabled || !notable.length) return;

    // En sammanfattande notis per synkomgång (arbetsorder 5.8).
    const hasAnomaly = notable.some((c) => c.type === 'refund_detected');
    const view = hasAnomaly ? 'avvikelser' : 'ordrar';
    const lines = notable.slice(0, 3).map(describeChange);
    await chrome.notifications.create(`aom-${Date.now()}`, {
      type: 'basic',
      iconUrl: chrome.runtime.getURL('icons/icon-128.png'),
      title: `Orderkoll: ${notable.length} ${notable.length === 1 ? 'uppdatering' : 'uppdateringar'}`,
      message: lines.join('\n') + (notable.length > 3 ? `\n…och ${notable.length - 3} till` : ''),
      contextMessage: `Synk: ${source}`,
    });
    await AOM.storage.set('lastNotificationView', view);
  }

  function describeChange(change) {
    switch (change.type) {
      case 'new_order':
        return `Ny order ${change.orderId}`;
      case 'status_change':
        return `${(change.title || change.orderId || '').slice(0, 40)}: ${AOM.status.label(change.to)}`;
      case 'refund_detected':
        return `Återbetalning registrerad på ${change.orderId}`;
      case 'back_in_stock':
        return `Åter i lager: ${(change.title || change.asin || '').slice(0, 40)}`;
      default:
        return change.type;
    }
  }

  async function actionCounts(state) {
    const orders = AOM.storage.orderList(state);
    const pending = AOM.status.pendingConfirmations(orders).length;
    const { anomalies } = AOM.anomaly.evaluateAll(state.orders, state.refundRecords);
    const flagged = anomalies.filter((a) => a.type === AOM.anomaly.TYPE.REFUND_WITHOUT_REQUEST).length;
    return { pending, anomalies: anomalies.length, flagged, total: pending + flagged };
  }

  async function refreshBadge() {
    const state = await AOM.storage.getState();
    const counts = await actionCounts(state);
    const blocked = state.syncState.blockedReason;
    await chrome.action.setBadgeBackgroundColor({ color: blocked ? '#B12704' : '#FF9900' });
    await chrome.action.setBadgeText({ text: blocked ? '!' : counts.total ? String(counts.total) : '' });
  }

  chrome.notifications.onClicked.addListener(async (id) => {
    if (!id.startsWith('aom-')) return;
    const view = await AOM.storage.get('lastNotificationView', 'oversikt');
    await chrome.tabs.create({ url: chrome.runtime.getURL(`pages/dashboard.html#${view}`) });
  });

  /* --------------------------------------------------------- underhåll */

  async function maintenance() {
    const settings = await AOM.storage.getSettings();
    try {
      const removed = await AOM.db.pruneProvisional(settings.provisionalTtlDays);
      if (removed) console.info('[Orderkoll] städade', removed, 'tillfälliga ögonblicksbilder');
    } catch (err) {
      console.warn('[Orderkoll] kunde inte städa tillfälliga bilder:', err);
    }
    await runWatchChecks();
    await runFollowUps();
  }

  /** Lagerbevakning – samma kö, minst en timme mellan kontroller per produkt. */
  async function runWatchChecks() {
    const settings = await AOM.storage.getSettings();
    const watches = await AOM.storage.get(AOM.STORAGE_KEYS.WATCHES, {});
    const q = await getQueue();
    const changes = [];
    for (const watch of Object.values(watches)) {
      q.enqueue({
        type: 'stock-check',
        key: `stock:${watch.asin}`,
        minIntervalMs: Math.max(1, settings.stockCheckMinIntervalMinutes) * 60000,
        run: async () => {
          const res = await fetchText(`https://www.amazon.se/dp/${watch.asin}`);
          const inStock = !/Tillfälligt slut|Ej i lager|Currently unavailable|slutsåld/i.test(res.text);
          const wasOut = watch.lastInStock === false;
          await AOM.storage.mutate(
            AOM.STORAGE_KEYS.WATCHES,
            (current) => {
              const map = Object.assign({}, current || {});
              if (map[watch.asin]) {
                map[watch.asin] = Object.assign({}, map[watch.asin], {
                  lastCheckedAt: new Date().toISOString(),
                  lastInStock: inStock,
                });
              }
              return map;
            },
            {}
          );
          if (inStock && wasOut) {
            changes.push({ type: 'back_in_stock', asin: watch.asin, title: watch.title });
            await afterChanges(changes.splice(0), 'lagerbevakning');
          }
        },
      });
    }
  }

  /** Förnyad kopia av orderdetaljsidan ca ett dygn efter köpet (5.6). */
  async function runFollowUps() {
    const pending = await AOM.storage.get('followUps', []);
    if (!pending.length) return;
    const due = pending.filter((f) => new Date(f.dueAt).getTime() <= Date.now());
    if (!due.length) return;
    const q = await getQueue();
    for (const item of due) {
      q.enqueue({
        type: 'proof-followup',
        key: `followup:${item.orderId}`,
        run: async () => {
          const res = await fetchText(item.url);
          await AOM.proof.captureBundle(
            {
              kind: AOM.proof.KIND.ORDER_DETAIL_FOLLOWUP,
              orderId: item.orderId,
              pageUrl: item.url,
              html: res.text,
              imageUrls: [],
              meta: { reason: 'förnyad kopia ca ett dygn efter köp', capturedAt: new Date().toISOString() },
            },
            { fetchBinary }
          );
        },
      });
    }
    const remaining = pending.filter((f) => new Date(f.dueAt).getTime() > Date.now());
    await AOM.storage.set('followUps', remaining);
  }

  async function scheduleFollowUp(orderId, url) {
    await AOM.storage.mutate(
      'followUps',
      (current) => {
        const list = Array.isArray(current) ? current.slice() : [];
        if (list.some((f) => f.orderId === orderId)) return list;
        list.push({ orderId, url, dueAt: new Date(Date.now() + 86400000).toISOString() });
        return list;
      },
      []
    );
  }

  async function fetchBinary(url) {
    const res = await fetch(url, { credentials: 'omit' });
    if (!res.ok) throw new Error(`HTTP ${res.status} för bild`);
    const buffer = await res.arrayBuffer();
    return { bytes: new Uint8Array(buffer), type: res.headers.get('content-type') || 'image/jpeg' };
  }

  /* --------------------------------------------------- meddelanderouter */

  async function handleMessage(message, sender) {
    switch (message.type) {
      case MSG.ORDER_HISTORY_PARSED: {
        if (message.error) {
          await AOM.storage.setSyncState({ blockedReason: message.error });
          await refreshBadge();
          return { ok: false, error: message.error };
        }
        const res = await AOM.storage.upsertOrders(message.orders || []);
        await AOM.storage.setSyncState({
          lastSyncAt: new Date().toISOString(),
          lastSyncSource: sender && sender.tab ? 'sidbesök' : 'okänd',
          blockedReason: null,
        });
        await afterChanges(res.changes, 'sidbesök');
        // Reservflöde 5.6: nya ordrar utan bevis får en efterhandskopia.
        for (const change of res.changes.filter((c) => c.type === 'new_order')) {
          const state = await AOM.storage.getState();
          const order = state.orders[change.orderId];
          if (!order) continue;
          const existing = await AOM.proof.bundlesForOrder(order.orderId);
          if (existing.length) continue;
          const provisional = await AOM.proof.findProvisionalForOrder(order);
          if (provisional) {
            await AOM.proof.promoteProvisional(provisional.id, order.orderId, { fetchBinary });
          } else {
            await enqueueAfterwardsCapture(order);
          }
          if (order.detailUrl) await scheduleFollowUp(order.orderId, order.detailUrl);
        }
        return { ok: true, count: res.count };
      }

      case MSG.ORDER_DETAIL_PARSED: {
        if (message.error) return { ok: false, error: message.error };
        const res = await AOM.storage.upsertOrders([message.order]);
        if (message.refund) await AOM.storage.upsertRefundRecords([message.refund]);
        await afterChanges(res.changes, 'sidbesök');
        return { ok: true };
      }

      case MSG.RETURNS_PARSED: {
        if (message.error) return { ok: false, error: message.error };
        const res = await AOM.storage.upsertRefundRecords(message.records || []);
        await afterChanges(res.changes, 'sidbesök');
        return { ok: true, count: (message.records || []).length };
      }

      case MSG.PROVISIONAL_SNAPSHOT: {
        const record = await AOM.proof.saveProvisional(message.snapshot || {});
        return { ok: true, id: record.id };
      }

      case MSG.PRODUCT_SNAPSHOT: {
        const bundle = await AOM.proof.captureBundle(
          {
            kind: message.snapshot.orderId
              ? AOM.proof.KIND.PRODUCT_AT_PURCHASE
              : AOM.proof.KIND.PRODUCT_AFTERWARDS,
            orderId: message.snapshot.orderId || null,
            asin: message.snapshot.asin,
            pageUrl: message.snapshot.pageUrl,
            html: message.snapshot.html,
            imageUrls: message.snapshot.imageUrls,
            meta: message.snapshot.meta,
          },
          { fetchBinary }
        );
        return { ok: true, bundleId: bundle.id, sha256: bundle.sha256 };
      }

      case MSG.GET_STATE: {
        const state = await AOM.storage.getState();
        const orders = AOM.storage.orderList(state);
        const { anomalies } = AOM.anomaly.evaluateAll(state.orders, state.refundRecords);
        return Object.assign({}, state, {
          orderList: orders,
          anomalies,
          counts: await actionCounts(state),
          queueSize: queue ? queue.size() : 0,
        });
      }

      case MSG.SYNC_NOW: {
        if (message.tabId) {
          try {
            await chrome.tabs.sendMessage(message.tabId, { type: MSG.RESCAN });
            return { ok: true, mode: 'aktiv flik' };
          } catch (err) {
            return { ok: false, error: `Kunde inte nå fliken: ${err.message}` };
          }
        }
        const res = await runBackgroundSync('manuell');
        return Object.assign({ ok: true, mode: 'bakgrundsflik' }, res);
      }

      case MSG.SET_RECEIVED: {
        await AOM.storage.setLineFlag(message.orderId, message.lineKey, {
          userReceivedConfirmed: !!message.value,
          userReceivedConfirmedAt: message.value ? new Date().toISOString() : null,
        });
        await refreshBadge();
        return { ok: true };
      }

      case MSG.SET_RETURN_REQUESTED: {
        await AOM.storage.setOrderFlag(message.orderId, {
          userConfirmedReturnOrRefundRequested: !!message.value,
        });
        await afterChanges([], 'kryssruta');
        return { ok: true };
      }

      case MSG.SET_HAS_DEFECT: {
        await AOM.storage.setOrderFlag(message.orderId, { userHasDefect: !!message.value });
        return { ok: true };
      }

      case MSG.SETTINGS_UPDATE: {
        const settings = await AOM.storage.setSettings(message.patch || {});
        queue = null; // byggs om med nya fördröjningar
        await ensureAlarm();
        return { ok: true, settings };
      }

      case MSG.WATCH_ADD: {
        await AOM.storage.mutate(
          AOM.STORAGE_KEYS.WATCHES,
          (current) => {
            const map = Object.assign({}, current || {});
            const w = message.watch;
            map[w.asin] = Object.assign({ addedAt: new Date().toISOString(), lastInStock: null }, map[w.asin], w);
            return map;
          },
          {}
        );
        return { ok: true };
      }

      case MSG.WATCH_REMOVE: {
        await AOM.storage.mutate(
          AOM.STORAGE_KEYS.WATCHES,
          (current) => {
            const map = Object.assign({}, current || {});
            delete map[message.asin];
            return map;
          },
          {}
        );
        return { ok: true };
      }

      case MSG.PROOF_LIST: {
        const bundles = await AOM.db.listBundles();
        return {
          ok: true,
          bundles: bundles
            .map((b) => ({
              id: b.id,
              kind: b.kind,
              orderId: b.orderId,
              asin: b.asin,
              title: b.title,
              createdAt: b.createdAt,
              sha256: b.sha256,
              sizeBytes: b.sizeBytes,
              fileCount: b.files.length,
              pageUrl: b.pageUrl,
            }))
            .sort((a, b) => String(b.createdAt).localeCompare(String(a.createdAt))),
        };
      }

      case MSG.PROOF_GET: {
        const bundle = await AOM.db.getBundle(message.id);
        if (!bundle) return { ok: false, error: 'okänt bevispaket' };
        const blobs = await AOM.db.blobsForBundle(message.id);
        const verification = await AOM.proof.verifyBundle(message.id);
        return { ok: true, bundle, blobs, verification };
      }

      case MSG.PROOF_DELETE: {
        await AOM.proof.deleteBundle(message.id);
        return { ok: true };
      }

      case MSG.PROOF_CAPTURE_FOR_ORDER: {
        const state = await AOM.storage.getState();
        const order = state.orders[message.orderId];
        if (!order) return { ok: false, error: 'okänd order' };
        await enqueueAfterwardsCapture(order);
        return { ok: true, queued: true };
      }

      case MSG.FETCH_URL: {
        const q = await getQueue();
        if (q.paused) {
          return { error: `Kön är pausad (${q.pauseReason}) – ingen hämtning görs.`, wall: q.pauseReason };
        }
        return new Promise((resolve) => {
          q.enqueue({
            type: 'fetch',
            key: `fetch:${message.url}:${Date.now()}`,
            run: async () => {
              try {
                const res = await fetchText(message.url);
                resolve({ ok: true, url: res.url, text: res.text, status: res.status });
              } catch (err) {
                resolve({ error: String(err.message || err), wall: err.wall || null });
              }
            },
          });
          if (q.paused) {
            resolve({ error: `Kön pausades (${q.pauseReason}).`, wall: q.pauseReason });
          }
        });
      }

      case MSG.IMAGE_HASH: {
        try {
          const res = await fetch(message.url, { credentials: 'omit' });
          const blob = await res.blob();
          const hash = await AOM.imageHash.dHashFromBlob(blob);
          return { ok: true, hash };
        } catch (err) {
          return { ok: false, error: String(err.message || err) };
        }
      }

      case MSG.FX_RATES: {
        try {
          const rates = await AOM.fx.getRates();
          return { ok: true, rates: rates.rates, date: rates.date, stale: !!rates.stale };
        } catch (err) {
          return { ok: false, error: String(err.message || err) };
        }
      }

      case MSG.STORAGE_USAGE: {
        const estimate = await AOM.db.estimateUsage();
        const bytes = await chrome.storage.local.getBytesInUse(null);
        return { ok: true, indexedDb: estimate, storageBytes: bytes };
      }

      case MSG.EXPORT_ALL: {
        const state = await AOM.storage.getState();
        return { ok: true, data: state };
      }

      case MSG.CLEAR_ALL: {
        await chrome.storage.local.clear();
        const bundles = await AOM.db.listBundles();
        for (const b of bundles) await AOM.db.deleteBundle(b.id);
        await refreshBadge();
        return { ok: true };
      }

      case MSG.OPEN_URL: {
        await chrome.tabs.create({ url: message.url });
        return { ok: true };
      }

      case MSG.TOGGLE_OVERLAY: {
        if (message.tabId) {
          await chrome.tabs.sendMessage(message.tabId, { type: MSG.TOGGLE_OVERLAY });
          return { ok: true };
        }
        return { ok: false, error: 'ingen flik angiven' };
      }

      default:
        return { ok: false, error: `okänt meddelande: ${message.type}` };
    }
  }

  async function enqueueAfterwardsCapture(order) {
    const item = (order.lineItems || []).find((i) => i.productUrl || i.asin);
    if (!item) return;
    const url = item.productUrl || `https://www.amazon.se/dp/${item.asin}`;
    const q = await getQueue();
    q.enqueue({
      type: 'proof-afterwards',
      key: `proof:${order.orderId}`,
      run: async () => {
        const res = await fetchText(url);
        await AOM.proof.captureBundle(
          {
            kind: AOM.proof.KIND.PRODUCT_AFTERWARDS,
            orderId: order.orderId,
            asin: item.asin,
            pageUrl: url,
            html: res.text,
            imageUrls: item.thumbnailUrl ? [item.thumbnailUrl] : [],
            meta: {
              title: item.title,
              reason: 'efterhandskopia – ingen ögonblicksbild fanns vid köpet',
              capturedAt: new Date().toISOString(),
            },
          },
          { fetchBinary }
        );
      },
    });
  }

  chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
    if (!message || !message.type) return undefined;
    resolveWaiters(sender, message);
    handleMessage(message, sender)
      .then((result) => sendResponse(result))
      .catch((err) => sendResponse({ ok: false, error: String((err && err.message) || err) }));
    return true; // svaret kommer asynkront
  });

  self.AOM.sw = { runBackgroundSync, refreshBadge, ensureAlarm, maintenance, actionCounts, afterChanges };
  self.__aomAfterChanges = afterChanges; // används av installationstesterna
  ensureAlarm();
  refreshBadge();
})();
