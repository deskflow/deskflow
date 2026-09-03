/* Orderkoll – injicerad instrumentpanel på amazon.se.
 * Allt ligger i en sluten Shadow DOM så att varken Amazons CSS läcker in
 * eller tilläggets CSS läcker ut (grind steg 9). */
(function () {
  'use strict';
  const AOM = self.AOM;
  let host = null;
  let controller = null;

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

  async function mount(initialView) {
    if (host) {
      host.hidden = false;
      if (controller && initialView) controller.setView(initialView);
      if (controller) controller.refresh();
      return;
    }
    host = document.createElement('div');
    host.id = 'aom-overlay-host';
    host.className = 'aom-overlay-host';
    // Ingen ärvd stil från Amazon på värdelementet.
    host.style.all = 'initial';
    host.style.position = 'fixed';
    host.style.top = '0';
    host.style.right = '0';
    host.style.bottom = '0';
    host.style.zIndex = '2147483000';

    const shadow = host.attachShadow({ mode: 'open' });
    const styleUrl = chrome.runtime.getURL('content-scripts/overlay/overlay-styles.css');
    const css = await fetch(styleUrl).then((r) => r.text());
    const style = document.createElement('style');
    style.textContent = `:host{display:block;width:min(940px,96vw);height:100vh}` + css;
    shadow.appendChild(style);

    const container = document.createElement('div');
    container.className = 'aom-scope';
    container.style.height = '100%';
    shadow.appendChild(container);
    document.documentElement.appendChild(host);

    controller = AOM.dashboard.create({
      root: container,
      send,
      initialView: initialView || 'oversikt',
      openUrl: (url) => {
        // Tilläggets egna sidor kan inte öppnas direkt från ett content script.
        if (url.startsWith('chrome-extension://')) send({ type: AOM.MSG.OPEN_URL, url });
        else window.open(url, '_blank', 'noopener');
      },
      onClose: () => {
        host.hidden = true;
      },
    });
  }

  function toggle(view) {
    if (host && !host.hidden) {
      host.hidden = true;
      return;
    }
    mount(view);
  }

  chrome.runtime.onMessage.addListener((msg, _sender, respond) => {
    if (msg && msg.type === AOM.MSG.TOGGLE_OVERLAY) {
      toggle(msg.view);
      respond({ ok: true });
      return true;
    }
    return undefined;
  });

  // Öppnas automatiskt när användaren kommer via en notis: #aom-panel
  if (location.hash.startsWith('#aom-panel')) {
    mount(location.hash.split('=')[1] || 'oversikt');
  }

  self.AOM.overlay = { toggle, mount };
})();
