/* Orderkoll – IndexedDB för bevisarkivet (arbetsorder 5.6).
 * Full HTML och bilder hamnar här (fungerar i MV3 service workers, till
 * skillnad från localStorage). Index/metadata ligger i chrome.storage.local. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});

  const DB_NAME = 'aom-proof';
  const DB_VERSION = 1;
  const STORE_BUNDLES = 'bundles';
  const STORE_BLOBS = 'blobs';
  const STORE_PROVISIONAL = 'provisional';

  let dbPromise = null;

  function openDb() {
    if (dbPromise) return dbPromise;
    dbPromise = new Promise((resolve, reject) => {
      const req = indexedDB.open(DB_NAME, DB_VERSION);
      req.onupgradeneeded = () => {
        const db = req.result;
        if (!db.objectStoreNames.contains(STORE_BUNDLES)) {
          const s = db.createObjectStore(STORE_BUNDLES, { keyPath: 'id' });
          s.createIndex('orderId', 'orderId', { unique: false });
          s.createIndex('createdAt', 'createdAt', { unique: false });
          s.createIndex('asin', 'asin', { unique: false });
        }
        if (!db.objectStoreNames.contains(STORE_BLOBS)) {
          const s = db.createObjectStore(STORE_BLOBS, { keyPath: 'id' });
          s.createIndex('bundleId', 'bundleId', { unique: false });
        }
        if (!db.objectStoreNames.contains(STORE_PROVISIONAL)) {
          const s = db.createObjectStore(STORE_PROVISIONAL, { keyPath: 'id' });
          s.createIndex('createdAt', 'createdAt', { unique: false });
          s.createIndex('asin', 'asin', { unique: false });
        }
      };
      req.onsuccess = () => resolve(req.result);
      req.onerror = () => reject(req.error);
    });
    return dbPromise;
  }

  function reqToPromise(request) {
    return new Promise((resolve, reject) => {
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
  }

  async function put(store, value) {
    const db = await openDb();
    return new Promise((resolve, reject) => {
      const t = db.transaction(store, 'readwrite');
      const req = t.objectStore(store).put(value);
      req.onerror = () => reject(req.error);
      t.oncomplete = () => resolve(value);
      t.onerror = () => reject(t.error);
    });
  }

  async function getOne(store, key) {
    const db = await openDb();
    return reqToPromise(db.transaction(store, 'readonly').objectStore(store).get(key));
  }

  async function getAll(store, indexName, query) {
    const db = await openDb();
    const s = db.transaction(store, 'readonly').objectStore(store);
    const source = indexName ? s.index(indexName) : s;
    return reqToPromise(source.getAll(query));
  }

  async function del(store, key) {
    const db = await openDb();
    return new Promise((resolve, reject) => {
      const t = db.transaction(store, 'readwrite');
      t.objectStore(store).delete(key);
      t.oncomplete = () => resolve(true);
      t.onerror = () => reject(t.error);
    });
  }

  async function putBundle(bundle) {
    return put(STORE_BUNDLES, bundle);
  }
  async function getBundle(id) {
    return getOne(STORE_BUNDLES, id);
  }
  async function listBundles() {
    return getAll(STORE_BUNDLES);
  }
  async function bundlesForOrder(orderId) {
    return getAll(STORE_BUNDLES, 'orderId', orderId);
  }
  async function putBlob(blobRecord) {
    return put(STORE_BLOBS, blobRecord);
  }
  async function blobsForBundle(bundleId) {
    return getAll(STORE_BLOBS, 'bundleId', bundleId);
  }
  async function deleteBundle(id) {
    const blobs = await blobsForBundle(id);
    for (const b of blobs) await del(STORE_BLOBS, b.id);
    return del(STORE_BUNDLES, id);
  }
  async function putProvisional(record) {
    return put(STORE_PROVISIONAL, record);
  }
  async function listProvisional() {
    return getAll(STORE_PROVISIONAL);
  }
  async function getProvisional(id) {
    return getOne(STORE_PROVISIONAL, id);
  }
  async function deleteProvisional(id) {
    return del(STORE_PROVISIONAL, id);
  }

  /** Städar tillfälliga ögonblicksbilder som aldrig blev ett köp. */
  async function pruneProvisional(ttlDays) {
    const cutoff = Date.now() - (ttlDays || 4) * 86400000;
    const all = await listProvisional();
    let removed = 0;
    for (const rec of all) {
      if (new Date(rec.createdAt).getTime() < cutoff) {
        await deleteProvisional(rec.id);
        removed += 1;
      }
    }
    return removed;
  }

  async function estimateUsage() {
    if (typeof navigator !== 'undefined' && navigator.storage && navigator.storage.estimate) {
      try {
        return await navigator.storage.estimate();
      } catch (_err) {
        /* faller igenom */
      }
    }
    return null;
  }

  AOM.db = {
    openDb,
    putBundle,
    getBundle,
    listBundles,
    bundlesForOrder,
    putBlob,
    blobsForBundle,
    deleteBundle,
    putProvisional,
    listProvisional,
    getProvisional,
    deleteProvisional,
    pruneProvisional,
    estimateUsage,
    STORE_BUNDLES,
    STORE_BLOBS,
    STORE_PROVISIONAL,
  };
})(typeof self !== 'undefined' ? self : globalThis);
