/* Orderkoll – bevissäkring (arbetsorder 5.6).
 *
 * Ett bevispaket = produktsidans HTML + bilderna + orderdata + tidsstämplar,
 * med SHA-256 per fil och en samlingshash över hela paketet. Allt sparas
 * lokalt i IndexedDB. Inget skickas någonsin någonstans automatiskt. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});

  const KIND = {
    PROVISIONAL: 'provisional',       // vid "lägg i varukorg", inte ännu ett köp
    PRODUCT_AT_PURCHASE: 'product_at_purchase',
    ORDER_CONFIRMATION: 'order_confirmation',
    ORDER_DETAIL_FOLLOWUP: 'order_detail_followup',
    PRODUCT_AFTERWARDS: 'product_afterwards', // reservflödet
  };

  function randomId(prefix) {
    const rand =
      typeof crypto !== 'undefined' && crypto.randomUUID
        ? crypto.randomUUID()
        : Math.random().toString(36).slice(2) + Date.now().toString(36);
    return `${prefix}-${rand}`;
  }

  function toBytes(input) {
    if (input instanceof Uint8Array) return input;
    if (input instanceof ArrayBuffer) return new Uint8Array(input);
    return new TextEncoder().encode(String(input));
  }

  async function sha256Hex(input) {
    const bytes = toBytes(input);
    const digest = await crypto.subtle.digest('SHA-256', bytes);
    return Array.from(new Uint8Array(digest))
      .map((b) => b.toString(16).padStart(2, '0'))
      .join('');
  }

  /** Samlingshash: SHA-256 över "namn:filhash"-rader i namnordning. */
  async function bundleHash(files) {
    const lines = files
      .map((f) => `${f.name}:${f.sha256}`)
      .sort()
      .join('\n');
    return sha256Hex(lines);
  }

  /**
   * Skapar och sparar ett bevispaket.
   * input: { kind, orderId, asin, pageUrl, html, imageUrls, meta }
   * deps:  { fetchBinary(url) -> { bytes, type } }
   */
  async function captureBundle(input, deps = {}) {
    const id = input.id || randomId('bundle');
    const createdAt = new Date().toISOString();
    const files = [];
    const blobs = [];

    if (input.html) {
      const bytes = toBytes(input.html);
      files.push({
        name: 'sida.html',
        type: 'text/html',
        size: bytes.byteLength,
        sha256: await sha256Hex(bytes),
      });
      blobs.push({ name: 'sida.html', type: 'text/html', bytes });
    }

    const imageUrls = Array.from(new Set(input.imageUrls || [])).slice(0, 20);
    const imageMeta = [];
    for (const url of imageUrls) {
      if (typeof deps.fetchBinary !== 'function') break;
      try {
        /* eslint-disable no-await-in-loop */
        const res = await deps.fetchBinary(url);
        if (!res || !res.bytes || !res.bytes.byteLength) continue;
        const ext = (res.type && res.type.split('/')[1] ? res.type.split('/')[1] : 'jpg').replace(
          /[^a-z0-9]/gi,
          ''
        );
        const name = `bilder/${String(imageMeta.length + 1).padStart(2, '0')}.${ext}`;
        const sha256 = await sha256Hex(res.bytes);
        files.push({ name, type: res.type || 'image/jpeg', size: res.bytes.byteLength, sha256, sourceUrl: url });
        blobs.push({ name, type: res.type || 'image/jpeg', bytes: res.bytes });
        imageMeta.push({ name, sourceUrl: url });
      } catch (_err) {
        // En bild som inte kan hämtas ska inte fälla hela beviset.
        files.push({ name: `bilder/MISSLYCKAD-${imageMeta.length + 1}`, type: null, size: 0, sha256: null, sourceUrl: url, error: true });
      }
    }

    const metaBytes = toBytes(JSON.stringify(input.meta || {}, null, 2));
    files.push({
      name: 'metadata.json',
      type: 'application/json',
      size: metaBytes.byteLength,
      sha256: await sha256Hex(metaBytes),
    });
    blobs.push({ name: 'metadata.json', type: 'application/json', bytes: metaBytes });

    const bundle = {
      id,
      kind: input.kind || KIND.PRODUCT_AT_PURCHASE,
      orderId: input.orderId || null,
      asin: input.asin || null,
      pageUrl: input.pageUrl || null,
      title: (input.meta && input.meta.title) || null,
      createdAt,
      capturedAtIso: createdAt,
      files,
      images: imageMeta,
      meta: input.meta || {},
      sizeBytes: files.reduce((sum, f) => sum + (f.size || 0), 0),
    };
    bundle.sha256 = await bundleHash(files.filter((f) => f.sha256));

    await AOM.db.putBundle(bundle);
    for (const blob of blobs) {
      await AOM.db.putBlob({
        id: `${id}::${blob.name}`,
        bundleId: id,
        name: blob.name,
        type: blob.type,
        size: blob.bytes.byteLength,
        blob: typeof Blob === 'function' ? new Blob([blob.bytes], { type: blob.type || 'application/octet-stream' }) : blob.bytes,
      });
    }
    await indexBundle(bundle);
    return bundle;
  }

  async function indexBundle(bundle) {
    await AOM.storage.mutate(
      AOM.STORAGE_KEYS.PROOF_INDEX,
      (current) => {
        const index = Object.assign({}, current || {});
        index[bundle.id] = {
          id: bundle.id,
          kind: bundle.kind,
          orderId: bundle.orderId,
          asin: bundle.asin,
          title: bundle.title,
          createdAt: bundle.createdAt,
          sha256: bundle.sha256,
          sizeBytes: bundle.sizeBytes,
          fileCount: bundle.files.length,
          pageUrl: bundle.pageUrl,
        };
        return index;
      },
      {}
    );
  }

  /** Sparar en tillfällig ögonblicksbild vid "lägg i varukorg". */
  async function saveProvisional(input) {
    const record = {
      id: input.id || randomId('prov'),
      asin: input.asin || null,
      pageUrl: input.pageUrl || null,
      title: (input.meta && input.meta.title) || null,
      html: input.html || null,
      imageUrls: input.imageUrls || [],
      meta: input.meta || {},
      createdAt: new Date().toISOString(),
    };
    await AOM.db.putProvisional(record);
    return record;
  }

  /** Kopplar en tillfällig ögonblicksbild till en verklig order. */
  async function promoteProvisional(provisionalId, orderId, deps) {
    const record = await AOM.db.getProvisional(provisionalId);
    if (!record) return null;
    const bundle = await captureBundle(
      {
        kind: KIND.PRODUCT_AT_PURCHASE,
        orderId,
        asin: record.asin,
        pageUrl: record.pageUrl,
        html: record.html,
        imageUrls: record.imageUrls,
        meta: Object.assign({}, record.meta, {
          provisionalId,
          provisionalCreatedAt: record.createdAt,
          promotedAt: new Date().toISOString(),
        }),
      },
      deps
    );
    await AOM.db.deleteProvisional(provisionalId);
    return bundle;
  }

  /** Hittar en tillfällig ögonblicksbild som matchar en orders artiklar. */
  async function findProvisionalForOrder(order) {
    const all = await AOM.db.listProvisional();
    if (!all.length) return null;
    const asins = new Set((order.lineItems || []).map((i) => i.asin).filter(Boolean));
    const orderTime = order.orderDate ? new Date(`${order.orderDate}T23:59:59Z`).getTime() : Date.now();
    const candidates = all
      .filter((rec) => rec.asin && asins.has(rec.asin))
      .filter((rec) => new Date(rec.createdAt).getTime() <= orderTime + 86400000)
      .sort((a, b) => new Date(b.createdAt) - new Date(a.createdAt));
    return candidates[0] || null;
  }

  /** Alla paket för en order, nyast först. */
  async function bundlesForOrder(orderId) {
    const list = await AOM.db.bundlesForOrder(orderId);
    return list.sort((a, b) => String(b.createdAt).localeCompare(String(a.createdAt)));
  }

  async function deleteBundle(id) {
    await AOM.db.deleteBundle(id);
    await AOM.storage.mutate(
      AOM.STORAGE_KEYS.PROOF_INDEX,
      (current) => {
        const index = Object.assign({}, current || {});
        delete index[id];
        return index;
      },
      {}
    );
    return true;
  }

  /** Verifierar att ett sparat paket inte ändrats sedan det skapades. */
  async function verifyBundle(id) {
    const bundle = await AOM.db.getBundle(id);
    if (!bundle) return { ok: false, error: 'paketet finns inte' };
    const blobs = await AOM.db.blobsForBundle(id);
    const byName = new Map(blobs.map((b) => [b.name, b]));
    const problems = [];
    for (const file of bundle.files) {
      if (!file.sha256) continue;
      const blob = byName.get(file.name);
      if (!blob) {
        problems.push(`${file.name}: filen saknas`);
        continue;
      }
      const bytes = new Uint8Array(await blob.blob.arrayBuffer());
      const hash = await sha256Hex(bytes);
      if (hash !== file.sha256) problems.push(`${file.name}: hash stämmer inte`);
    }
    const recomputed = await bundleHash(bundle.files.filter((f) => f.sha256));
    if (recomputed !== bundle.sha256) problems.push('samlingshashen stämmer inte');
    return { ok: problems.length === 0, problems, sha256: bundle.sha256 };
  }

  AOM.proof = {
    KIND,
    captureBundle,
    saveProvisional,
    promoteProvisional,
    findProvisionalForOrder,
    bundlesForOrder,
    deleteBundle,
    verifyBundle,
    sha256Hex,
    bundleHash,
    indexBundle,
    randomId,
  };
})(typeof self !== 'undefined' ? self : globalThis);
