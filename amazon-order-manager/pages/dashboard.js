/* Orderkoll – fristående instrumentpanel (samma modul som overlayen). */
(function () {
  'use strict';
  const AOM = self.AOM;

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

  const view = (location.hash || '').replace('#', '') || 'oversikt';
  const controller = AOM.dashboard.create({
    root: document.getElementById('mount'),
    send,
    initialView: AOM.dashboard.VIEWS.some((v) => v.id === view) ? view : 'oversikt',
    openUrl: (url) => chrome.tabs.create({ url }),
  });

  window.addEventListener('hashchange', () => {
    const next = (location.hash || '').replace('#', '');
    if (AOM.dashboard.VIEWS.some((v) => v.id === next)) controller.setView(next);
  });
})();
