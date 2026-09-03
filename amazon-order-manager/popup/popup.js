/* Orderkoll – popup. Läser bara lagrad data (ingen skrapning här).
 * Synk-knappen är aktiv bara när aktiv flik är orderhistoriken på amazon.se
 * (arbetsorder 5.7 och grind steg 5). */
(function () {
  'use strict';
  const AOM = self.AOM;
  const ORDER_HISTORY_RE = /^https?:\/\/(www\.)?amazon\.se\/(gp\/css\/order-history|your-orders\/orders|gp\/your-account\/order-history)/i;

  const $ = (id) => document.getElementById(id);
  let state = null;
  let filter = 'alla';
  let activeTab = null;

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

  const timeLabel = (iso) => AOM.format.timeLabel(iso);
  const money = (amount, currency) => AOM.format.money(amount, currency);

  function renderBanner() {
    const banner = $('banner');
    const sync = state.syncState || {};
    if (sync.blockedReason === 'captcha') {
      banner.textContent = 'Amazon visade en CAPTCHA vid senaste bakgrundssynken. Öppna amazon.se och lös den, synka sedan igen.';
      banner.hidden = false;
    } else if (sync.blockedReason === 'signin') {
      banner.textContent = 'Din Amazon-session har gått ut. Logga in på amazon.se och synka igen.';
      banner.hidden = false;
    } else if (sync.lastError) {
      banner.textContent = `Senaste synken misslyckades: ${sync.lastError}`;
      banner.hidden = false;
    } else {
      banner.hidden = true;
    }
  }

  function renderList() {
    const list = $('list');
    list.textContent = '';
    const orders = state.orderList || [];
    let entries = [];

    if (filter === 'bekrafta') {
      entries = AOM.status.pendingConfirmations(orders).map((entry) => ({
        order: entry.order,
        item: entry.item,
        lineKey: entry.lineKey,
      }));
    } else if (filter === 'avvikelser') {
      entries = (state.anomalies || []).map((a) => {
        const order = (state.orders || {})[a.orderId] || { orderId: a.orderId, lineItems: [] };
        const item = (order.lineItems || []).find((i) => i.lineKey === a.lineKey) || { title: a.title };
        return { order, item, anomaly: a };
      });
    } else {
      entries = orders.slice(0, 25).map((order) => ({ order, item: (order.lineItems || [])[0] || {} }));
    }

    $('empty').hidden = entries.length > 0;
    for (const entry of entries) {
      const li = document.createElement('li');
      li.className = 'pop-item';

      const img = document.createElement('img');
      img.className = 'pop-thumb';
      img.alt = '';
      if (entry.item && entry.item.thumbnailUrl) img.src = entry.item.thumbnailUrl;
      li.appendChild(img);

      const body = document.createElement('div');
      body.className = 'pop-body';
      const title = document.createElement('div');
      title.className = 'pop-title';
      title.textContent = (entry.item && entry.item.title) || entry.order.orderId;
      title.title = title.textContent;
      body.appendChild(title);

      const meta = document.createElement('div');
      meta.className = 'pop-meta';
      meta.textContent = `${entry.order.orderId} · ${
        entry.order.sellerNameSnapshot || 'okänd säljare'
      } · ${money(entry.order.totalAmount, entry.order.currency)}`;
      body.appendChild(meta);

      if (entry.anomaly) {
        const reason = document.createElement('div');
        reason.className = 'pop-meta';
        reason.textContent = entry.anomaly.reason;
        body.appendChild(reason);
      } else {
        const composite = AOM.status.compositeLabel(entry.order.lineItems || []);
        const chip = document.createElement('span');
        chip.className = `pop-chip pop-chip-${composite.status || 'okand'}`;
        chip.textContent = composite.text;
        body.appendChild(chip);
      }
      li.appendChild(body);

      if (filter === 'bekrafta') {
        const btn = document.createElement('button');
        btn.className = 'pop-confirm';
        btn.type = 'button';
        btn.textContent = 'Mottagen ✓';
        btn.addEventListener('click', async () => {
          await send({
            type: AOM.MSG.SET_RECEIVED,
            orderId: entry.order.orderId,
            lineKey: entry.lineKey,
            value: true,
          });
          await load();
        });
        li.appendChild(btn);
      }
      list.appendChild(li);
    }
  }

  function renderSyncButton() {
    const btn = $('sync');
    const hint = $('sync-hint');
    const onHistory = !!(activeTab && activeTab.url && ORDER_HISTORY_RE.test(activeTab.url));
    const running = !!(state.syncState && state.syncState.running);
    btn.disabled = !onHistory || running;
    if (running) {
      hint.textContent = 'Synk pågår…';
    } else if (onHistory) {
      hint.textContent = 'Läser av orderhistoriken i den öppna fliken.';
    } else {
      hint.textContent = 'Gå till dina ordrar på amazon.se, öppna tillägget och tryck Synka för att uppdatera.';
    }
  }

  function render() {
    $('freshness').textContent = `Senast synkad ${timeLabel(state.syncState && state.syncState.lastSyncAt)}`;
    $('count-orders').textContent = String((state.orderList || []).length);
    $('count-pending').textContent = String((state.counts && state.counts.pending) || 0);
    $('count-flagged').textContent = String((state.counts && state.counts.flagged) || 0);
    document.querySelectorAll('.pop-stat').forEach((el) => {
      el.classList.toggle('is-active', el.dataset.filter === filter);
    });
    renderBanner();
    renderSyncButton();
    renderList();
  }

  async function load() {
    state = await send({ type: AOM.MSG.GET_STATE });
    render();
  }

  async function init() {
    $('disclaimer').textContent = AOM.DISCLAIMER;
    const tabs = await chrome.tabs.query({ active: true, currentWindow: true });
    activeTab = tabs && tabs[0];

    $('sync').addEventListener('click', async () => {
      $('sync').disabled = true;
      await send({ type: AOM.MSG.SYNC_NOW, tabId: activeTab && activeTab.id });
      setTimeout(load, 1200);
    });
    $('panel').addEventListener('click', async () => {
      const isAmazon = activeTab && activeTab.url && /^https?:\/\/(www\.)?amazon\.se\//i.test(activeTab.url);
      if (isAmazon) {
        await send({ type: AOM.MSG.TOGGLE_OVERLAY, tabId: activeTab.id });
        window.close();
      } else {
        await chrome.tabs.create({ url: chrome.runtime.getURL('pages/dashboard.html') });
      }
    });
    $('options').addEventListener('click', () => chrome.runtime.openOptionsPage());
    document.querySelectorAll('.pop-stat').forEach((el) => {
      el.addEventListener('click', () => {
        filter = el.dataset.filter;
        render();
      });
    });
    await load();
  }

  init();
})();
