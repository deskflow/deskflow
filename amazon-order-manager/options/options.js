/* Orderkoll – inställningar. Ändringar sparas direkt och skickas till
 * service workern, som bygger om alarm och kö med de nya värdena. */
(function () {
  'use strict';
  const AOM = self.AOM;
  const $ = (id) => document.getElementById(id);

  const NUMBER_FIELDS = [
    'syncIntervalMinutes',
    'maxDetailVisitsPerSync',
    'requestDelayMinMs',
    'requestDelayMaxMs',
    'imageMatchThreshold',
    'stockCheckMinIntervalMinutes',
    'provisionalTtlDays',
  ];
  const BOOL_FIELDS = ['backgroundSyncEnabled', 'notificationsEnabled', 'riskNoticeAcknowledged'];
  const TEXT_FIELDS = ['backgroundTabMode'];

  let settings = null;

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

  function flash(text) {
    $('status').textContent = text;
    setTimeout(() => {
      if ($('status').textContent === text) $('status').textContent = '';
    }, 2500);
  }

  async function save(patch) {
    const res = await send({ type: AOM.MSG.SETTINGS_UPDATE, patch });
    if (res && res.settings) settings = res.settings;
    flash('Sparat.');
  }

  function renderMarkets() {
    const box = $('markets');
    box.textContent = '';
    for (const market of AOM.MARKETPLACES.filter((m) => m.id !== 'se')) {
      const label = document.createElement('label');
      label.className = 'aom-check';
      const input = document.createElement('input');
      input.type = 'checkbox';
      input.checked = (settings.compareMarketplaces || []).includes(market.id);
      input.addEventListener('change', () => {
        const set = new Set(settings.compareMarketplaces || []);
        if (input.checked) set.add(market.id);
        else set.delete(market.id);
        settings.compareMarketplaces = Array.from(set);
        save({ compareMarketplaces: settings.compareMarketplaces });
      });
      const span = document.createElement('span');
      span.textContent = market.label;
      label.append(input, span);
      box.appendChild(label);
    }
  }

  function renderBankRows() {
    const box = $('bankRows');
    box.textContent = '';
    const rows = settings.bankDisputeDeadlines || [];
    rows.forEach((row, index) => {
      const wrap = document.createElement('div');
      wrap.className = 'opt-bank-row';
      const label = document.createElement('input');
      label.className = 'aom-input';
      label.placeholder = 'Betalsätt, t.ex. Visa ••4417';
      label.value = row.label || '';
      const days = document.createElement('input');
      days.className = 'aom-input';
      days.type = 'number';
      days.min = '1';
      days.placeholder = 'dagar enligt din bank';
      days.value = row.days ?? '';
      const remove = document.createElement('button');
      remove.className = 'aom-btn';
      remove.type = 'button';
      remove.textContent = 'Ta bort';

      const commit = () => {
        const list = (settings.bankDisputeDeadlines || []).slice();
        list[index] = { label: label.value.trim(), days: days.value ? Number(days.value) : null };
        settings.bankDisputeDeadlines = list.filter((r) => r.label);
        save({ bankDisputeDeadlines: settings.bankDisputeDeadlines });
      };
      label.addEventListener('change', commit);
      days.addEventListener('change', commit);
      remove.addEventListener('click', () => {
        const list = (settings.bankDisputeDeadlines || []).slice();
        list.splice(index, 1);
        settings.bankDisputeDeadlines = list;
        save({ bankDisputeDeadlines: list });
        renderBankRows();
      });

      wrap.append(label, days, remove);
      box.appendChild(wrap);
    });
  }

  function bindFields() {
    for (const id of BOOL_FIELDS) {
      const el = $(id);
      el.checked = !!settings[id];
      el.addEventListener('change', () => save({ [id]: el.checked }));
    }
    for (const id of NUMBER_FIELDS) {
      const el = $(id);
      el.value = settings[id] ?? '';
      el.addEventListener('change', () => {
        const value = el.value === '' ? null : Number(el.value);
        save({ [id]: value });
      });
    }
    for (const id of TEXT_FIELDS) {
      const el = $(id);
      el.value = settings[id] || '';
      el.addEventListener('change', () => save({ [id]: el.value }));
    }
    const downscale = $('proofDownscaleAfterDays');
    downscale.value = settings.proofDownscaleAfterDays ?? '';
    downscale.addEventListener('change', () =>
      save({ proofDownscaleAfterDays: downscale.value === '' ? null : Number(downscale.value) })
    );
  }

  async function renderUsage() {
    const res = await send({ type: AOM.MSG.STORAGE_USAGE });
    if (!res || !res.ok) return;
    const size = (bytes) => {
      const value = Number(bytes) || 0;
      if (value >= 1024 ** 3) return `${(value / 1024 ** 3).toFixed(1)} GB`;
      if (value >= 1024 ** 2) return `${(value / 1024 ** 2).toFixed(1)} MB`;
      return `${Math.round(value / 1024)} kB`;
    };
    const idb = res.indexedDb
      ? `${size(res.indexedDb.usage)} av ${size(res.indexedDb.quota)} tillgängliga`
      : 'okänt';
    $('usage').textContent = `Lagring: bevisarkiv ${idb}, metadata ${size(res.storageBytes)}.`;
  }

  async function init() {
    $('disclaimer').textContent = AOM.DISCLAIMER;
    $('risk').textContent = AOM.RISK_NOTICE;
    const state = await send({ type: AOM.MSG.GET_STATE });
    settings = (state && state.settings) || Object.assign({}, AOM.DEFAULT_SETTINGS);

    bindFields();
    renderMarkets();
    renderBankRows();
    renderUsage();

    $('addBank').addEventListener('click', () => {
      settings.bankDisputeDeadlines = (settings.bankDisputeDeadlines || []).concat([{ label: '', days: null }]);
      renderBankRows();
    });

    $('export').addEventListener('click', async () => {
      const res = await send({ type: AOM.MSG.EXPORT_ALL });
      const blob = new Blob([JSON.stringify(res.data, null, 2)], { type: 'application/json' });
      const a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = `orderkoll-export-${AOM.dates.todayIso()}.json`;
      a.click();
      URL.revokeObjectURL(a.href);
    });

    $('clear').addEventListener('click', async () => {
      if (!window.confirm('Radera all lokal data, inklusive bevisarkivet? Detta går inte att ångra.')) return;
      await send({ type: AOM.MSG.CLEAR_ALL });
      flash('All lokal data raderad.');
    });
  }

  init();
})();
