/* Orderkoll – wrapper kring chrome.storage.local.
 * Metadata och index bor här (snabb sökning); tunga binärdata i IndexedDB
 * (lib/db.js). Alla skrivningar serialiseras genom en promise-kedja så att
 * två samtidiga synkar inte skriver över varandra. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});
  const K = AOM.STORAGE_KEYS;

  let writeChain = Promise.resolve();

  function area() {
    if (typeof chrome === 'undefined' || !chrome.storage || !chrome.storage.local) {
      throw new Error('chrome.storage.local saknas i denna kontext');
    }
    return chrome.storage.local;
  }

  async function get(key, fallback) {
    const res = await area().get(key);
    return res && Object.prototype.hasOwnProperty.call(res, key) ? res[key] : fallback;
  }

  async function set(key, value) {
    await area().set({ [key]: value });
    return value;
  }

  /** Läs-modifiera-skriv, serialiserat. fn får nuvarande värde och returnerar nytt. */
  function mutate(key, fn, fallback) {
    const run = async () => {
      const current = await get(key, fallback);
      const next = await fn(current);
      if (next !== undefined) await set(key, next);
      return next;
    };
    writeChain = writeChain.then(run, run);
    return writeChain;
  }

  async function getSettings() {
    const stored = await get(K.SETTINGS, {});
    return Object.assign({}, AOM.DEFAULT_SETTINGS, stored || {});
  }

  async function setSettings(patch) {
    return mutate(
      K.SETTINGS,
      (current) => Object.assign({}, AOM.DEFAULT_SETTINGS, current || {}, patch || {}),
      {}
    );
  }

  async function getSyncState() {
    const stored = await get(K.SYNC_STATE, {});
    return Object.assign({}, AOM.DEFAULT_SYNC_STATE, stored || {});
  }

  async function setSyncState(patch) {
    return mutate(
      K.SYNC_STATE,
      (current) => Object.assign({}, AOM.DEFAULT_SYNC_STATE, current || {}, patch || {}),
      {}
    );
  }

  const USER_ORDER_FIELDS = ['userConfirmedReturnOrRefundRequested', 'userHasDefect', 'userNote'];
  const USER_ITEM_FIELDS = ['userReceivedConfirmed', 'userReceivedConfirmedAt', 'userHasDefect'];

  function isEmpty(v) {
    return v === null || v === undefined || v === '';
  }

  /**
   * Matchar inkommande artikelrader mot befintliga. lineKey först; annars ASIN
   * i förekomstordning (samma ASIN kan finnas flera gånger i en order).
   */
  function mergeLineItems(existing, incoming) {
    const prev = existing || [];
    const next = incoming || [];
    if (!next.length) return prev;

    const byKey = new Map();
    const byAsin = new Map();
    prev.forEach((item, index) => {
      const key = item.lineKey || AOM.status.lineKey(item, index);
      byKey.set(key, item);
      const list = byAsin.get(item.asin) || [];
      list.push(item);
      byAsin.set(item.asin, list);
    });
    const asinCursor = new Map();

    return next.map((item, index) => {
      const key = item.lineKey || AOM.status.lineKey(item, index);
      let old = byKey.get(key);
      if (!old && item.asin) {
        const cursor = asinCursor.get(item.asin) || 0;
        const list = byAsin.get(item.asin) || [];
        old = list[cursor];
        asinCursor.set(item.asin, cursor + 1);
      }
      const merged = Object.assign({}, old || {}, {}, key ? { lineKey: key } : {});
      // Amazon-härledda fält uppdateras bara när det inkommande värdet finns.
      for (const [field, value] of Object.entries(item)) {
        if (USER_ITEM_FIELDS.includes(field)) continue;
        if (isEmpty(value) && !isEmpty(merged[field])) continue;
        if (field === 'returnWindow' && value && merged.returnWindow) {
          merged.returnWindow = Object.assign({}, merged.returnWindow, value);
          continue;
        }
        merged[field] = value;
      }
      // Användarens egna bekräftelser överlever alltid en ny skrapning.
      for (const field of USER_ITEM_FIELDS) {
        if (old && !isEmpty(old[field])) merged[field] = old[field];
      }
      if (merged.status === AOM.STATUS.DELIVERED && !merged.deliveredDate && item.deliveredDate) {
        merged.deliveredDate = item.deliveredDate;
      }
      return merged;
    });
  }

  /** Sammanfogar en order. Returnerar { order, changes: [{type, ...}] }. */
  function mergeOrder(existing, incoming) {
    const changes = [];
    if (!existing) {
      changes.push({ type: 'new_order', orderId: incoming.orderId });
      return {
        order: Object.assign(
          {
            userConfirmedReturnOrRefundRequested: false,
            userHasDefect: false,
            createdAt: new Date().toISOString(),
          },
          incoming,
          { updatedAt: new Date().toISOString() }
        ),
        changes,
      };
    }

    const merged = Object.assign({}, existing);
    for (const [field, value] of Object.entries(incoming)) {
      if (USER_ORDER_FIELDS.includes(field)) continue;
      if (field === 'lineItems') continue;
      if (isEmpty(value) && !isEmpty(merged[field])) continue;
      merged[field] = value;
    }

    const prevStatuses = new Map(
      (existing.lineItems || []).map((it, i) => [it.lineKey || AOM.status.lineKey(it, i), it.status])
    );
    merged.lineItems = mergeLineItems(existing.lineItems, incoming.lineItems);
    merged.lineItems.forEach((item, index) => {
      const key = item.lineKey || AOM.status.lineKey(item, index);
      const before = prevStatuses.get(key);
      if (before !== undefined && before !== item.status && item.status) {
        changes.push({
          type: 'status_change',
          orderId: merged.orderId,
          lineKey: key,
          from: before,
          to: item.status,
          title: item.title,
        });
      }
    });
    merged.updatedAt = new Date().toISOString();
    return { order: merged, changes };
  }

  /** Skriver in nyskrapade ordrar. Returnerar samlade förändringar. */
  async function upsertOrders(list) {
    const incoming = (list || []).filter((o) => o && o.orderId);
    if (!incoming.length) return { changes: [], count: 0 };
    let changes = [];
    await mutate(
      K.ORDERS,
      (current) => {
        const orders = Object.assign({}, current || {});
        for (const order of incoming) {
          const res = mergeOrder(orders[order.orderId], order);
          orders[order.orderId] = res.order;
          changes = changes.concat(res.changes);
        }
        return orders;
      },
      {}
    );
    await syncSellers(incoming);
    return { changes, count: incoming.length };
  }

  async function syncSellers(orders) {
    const relevant = (orders || []).filter((o) => o.sellerId);
    if (!relevant.length) return;
    await mutate(
      K.SELLERS,
      (current) => {
        const sellers = Object.assign({}, current || {});
        for (const order of relevant) {
          const prev = sellers[order.sellerId] || {
            sellerId: order.sellerId,
            purchaseOrderIds: [],
            flaggedIssueCount: 0,
          };
          const ids = new Set(prev.purchaseOrderIds || []);
          ids.add(order.orderId);
          sellers[order.sellerId] = Object.assign({}, prev, {
            displayName: order.sellerNameSnapshot || prev.displayName || null,
            isThirdParty:
              typeof order.sellerIsThirdParty === 'boolean'
                ? order.sellerIsThirdParty
                : prev.isThirdParty ?? null,
            purchaseOrderIds: Array.from(ids),
            purchaseCount: ids.size,
          });
        }
        return sellers;
      },
      {}
    );
  }

  async function setOrderFlag(orderId, patch) {
    return mutate(
      K.ORDERS,
      (current) => {
        const orders = Object.assign({}, current || {});
        if (!orders[orderId]) return orders;
        orders[orderId] = Object.assign({}, orders[orderId], patch, {
          updatedAt: new Date().toISOString(),
        });
        return orders;
      },
      {}
    );
  }

  async function setLineFlag(orderId, lineKey, patch) {
    return mutate(
      K.ORDERS,
      (current) => {
        const orders = Object.assign({}, current || {});
        const order = orders[orderId];
        if (!order) return orders;
        const items = (order.lineItems || []).map((item, index) => {
          const key = item.lineKey || AOM.status.lineKey(item, index);
          return key === lineKey ? Object.assign({}, item, patch) : item;
        });
        orders[orderId] = Object.assign({}, order, {
          lineItems: items,
          updatedAt: new Date().toISOString(),
        });
        return orders;
      },
      {}
    );
  }

  async function upsertRefundRecords(records) {
    const list = (records || []).filter((r) => r && r.orderId);
    if (!list.length) return { changes: [] };
    const changes = [];
    await mutate(
      K.REFUND_RECORDS,
      (current) => {
        const map = Object.assign({}, current || {});
        for (const rec of list) {
          const prev = map[rec.orderId];
          const next = Object.assign({}, prev || {}, rec, { seenAt: new Date().toISOString() });
          if (!prev || prev.refundDetected !== next.refundDetected) {
            if (next.refundDetected) {
              changes.push({ type: 'refund_detected', orderId: rec.orderId });
            }
          }
          map[rec.orderId] = next;
        }
        return map;
      },
      {}
    );
    return { changes };
  }

  async function saveParseReport(report) {
    if (!report) return;
    await mutate(
      K.PARSE_REPORTS,
      (current) => {
        const list = Array.isArray(current) ? current.slice(-19) : [];
        list.push(report);
        return list;
      },
      []
    );
  }

  async function getState() {
    const raw = await area().get([
      K.ORDERS,
      K.SELLERS,
      K.SETTINGS,
      K.SYNC_STATE,
      K.WATCHES,
      K.PROOF_INDEX,
      K.REFUND_RECORDS,
    ]);
    return {
      orders: raw[K.ORDERS] || {},
      sellers: raw[K.SELLERS] || {},
      settings: Object.assign({}, AOM.DEFAULT_SETTINGS, raw[K.SETTINGS] || {}),
      syncState: Object.assign({}, AOM.DEFAULT_SYNC_STATE, raw[K.SYNC_STATE] || {}),
      watches: raw[K.WATCHES] || {},
      proofIndex: raw[K.PROOF_INDEX] || {},
      refundRecords: raw[K.REFUND_RECORDS] || {},
    };
  }

  function orderList(state) {
    return Object.values(state.orders || {}).sort((a, b) =>
      String(b.orderDate || '').localeCompare(String(a.orderDate || ''))
    );
  }

  AOM.storage = {
    get,
    set,
    mutate,
    getSettings,
    setSettings,
    getSyncState,
    setSyncState,
    upsertOrders,
    upsertRefundRecords,
    setOrderFlag,
    setLineFlag,
    saveParseReport,
    getState,
    orderList,
    mergeOrder,
    mergeLineItems,
  };
})(typeof self !== 'undefined' ? self : globalThis);
