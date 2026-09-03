/* Orderkoll – den gemensamma instrumentpanelen.
 * Samma modul driver både den injicerade overlayen (Shadow DOM på amazon.se)
 * och den fristående sidan pages/dashboard.html. Ingen skrapning sker här –
 * allt läses ur lagrat tillstånd via service workern. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});

  const VIEWS = [
    { id: 'oversikt', label: 'Översikt' },
    { id: 'ordrar', label: 'Alla ordrar' },
    { id: 'bekrafta', label: 'Att bekräfta' },
    { id: 'avvikelser', label: 'Avvikelser' },
    { id: 'bevakningar', label: 'Bevakningar' },
    { id: 'bevis', label: 'Bevisarkiv' },
    { id: 'saljare', label: 'Säljare' },
    { id: 'installningar', label: 'Inställningar' },
  ];

  function h(tag, props, children) {
    const el = document.createElement(tag);
    for (const [key, value] of Object.entries(props || {})) {
      if (value === null || value === undefined) continue;
      if (key === 'class') el.className = value;
      else if (key === 'text') el.textContent = value;
      else if (key === 'html') el.innerHTML = value;
      else if (key.startsWith('on') && typeof value === 'function') {
        el.addEventListener(key.slice(2).toLowerCase(), value);
      } else if (key === 'dataset') {
        Object.assign(el.dataset, value);
      } else if (value === true) el.setAttribute(key, '');
      else if (value !== false) el.setAttribute(key, String(value));
    }
    for (const child of [].concat(children || [])) {
      if (child === null || child === undefined || child === false) continue;
      el.appendChild(typeof child === 'string' ? document.createTextNode(child) : child);
    }
    return el;
  }

  const money = (amount, currency) => AOM.format.money(amount, currency);

  function statusChip(status, textOverride) {
    const chip = h('span', { class: `aom-chip aom-chip-${status || 'okand'}` });
    chip.textContent = textOverride || AOM.status.label(status);
    return chip;
  }

  const timeLabel = (iso) => AOM.format.timeLabel(iso);

  function create(options) {
    const mount = options.root;
    const send = options.send;
    const openUrl = options.openUrl || ((url) => window.open(url, '_blank', 'noopener'));
    const state = { view: options.initialView || 'oversikt', data: null, filter: { text: '', status: 'alla' } };

    const header = h('header', { class: 'aom-header' });
    const nav = h('nav', { class: 'aom-sidebar' });
    const main = h('main', { class: 'aom-main' });
    const shell = h('div', { class: 'aom-shell' }, [nav, h('div', { class: 'aom-content' }, [header, main])]);
    mount.appendChild(shell);

    function setView(view) {
      state.view = view;
      render();
    }

    async function refresh() {
      state.data = await send({ type: AOM.MSG.GET_STATE });
      render();
    }

    async function act(message) {
      await send(message);
      await refresh();
    }

    /* ------------------------------------------------------------- delar */

    function renderSidebar() {
      nav.textContent = '';
      nav.appendChild(
        h('div', { class: 'aom-brand' }, [
          h('span', { class: 'aom-brand-mark', 'aria-hidden': 'true' }),
          h('span', { class: 'aom-brand-name', text: 'Orderkoll' }),
        ])
      );
      const counts = (state.data && state.data.counts) || {};
      for (const view of VIEWS) {
        const badgeCount =
          view.id === 'bekrafta' ? counts.pending : view.id === 'avvikelser' ? counts.flagged : 0;
        nav.appendChild(
          h(
            'button',
            {
              class: `aom-nav-item${state.view === view.id ? ' is-active' : ''}`,
              type: 'button',
              onClick: () => setView(view.id),
            },
            [
              h('span', { text: view.label }),
              badgeCount ? h('span', { class: 'aom-nav-badge', text: String(badgeCount) }) : null,
            ]
          )
        );
      }
      nav.appendChild(h('p', { class: 'aom-sidebar-note', text: AOM.DISCLAIMER }));
    }

    function renderHeader() {
      header.textContent = '';
      const sync = (state.data && state.data.syncState) || {};
      const title = VIEWS.find((v) => v.id === state.view);
      header.appendChild(h('h1', { class: 'aom-title', text: title ? title.label : 'Orderkoll' }));

      const right = h('div', { class: 'aom-header-right' });
      right.appendChild(
        h('span', { class: 'aom-freshness', text: `Senast synkad ${timeLabel(sync.lastSyncAt)}` })
      );
      right.appendChild(
        h('button', {
          class: 'aom-btn',
          type: 'button',
          text: sync.running ? 'Synkar…' : 'Synka nu',
          disabled: !!sync.running,
          onClick: async () => {
            await act({ type: AOM.MSG.SYNC_NOW });
          },
        })
      );
      if (options.onClose) {
        right.appendChild(
          h('button', { class: 'aom-btn aom-btn-icon', type: 'button', text: '✕', title: 'Stäng', onClick: options.onClose })
        );
      }
      header.appendChild(right);

      if (sync.blockedReason || sync.lastError) {
        header.appendChild(
          h('div', { class: 'aom-banner aom-banner-warn' }, [
            h('strong', {
              text:
                sync.blockedReason === 'captcha'
                  ? 'Amazon visade en CAPTCHA.'
                  : sync.blockedReason === 'signin'
                  ? 'Din Amazon-session har gått ut.'
                  : 'Senaste synken misslyckades.',
            }),
            h('span', {
              text:
                sync.blockedReason === 'signin'
                  ? ' Logga in på amazon.se och synka igen.'
                  : ` ${sync.lastError || 'Vänta en stund och försök igen.'}`,
            }),
          ])
        );
      }
    }

    /* ------------------------------------------------------------- vyer */

    function viewOversikt() {
      const data = state.data;
      const orders = data.orderList || [];
      const counts = data.counts || {};
      const wrap = h('div', { class: 'aom-view' });

      const series = AOM.chart.monthlySeries(orders, 12);
      const spentTotal = series.reduce((sum, r) => sum + r.value, 0);
      const hero = h('section', { class: 'aom-card aom-hero' }, [
        h('div', { class: 'aom-hero-head' }, [
          h('div', {}, [
            h('h2', { class: 'aom-card-title', text: 'Utgifter per månad' }),
            h('p', {
              class: 'aom-card-sub',
              text: `${money(Math.round(spentTotal), 'SEK')} totalt de senaste ${series.length} månaderna`,
            }),
          ]),
        ]),
        AOM.chart.monthlySpend(series, { ariaLabel: 'Utgifter per månad på Amazon.se' }),
      ]);
      wrap.appendChild(hero);

      const actionItems = [];
      for (const entry of AOM.status.pendingConfirmations(orders).slice(0, 5)) {
        actionItems.push(
          h('li', { class: 'aom-action-row' }, [
            h('div', {}, [
              h('strong', { text: entry.item.title || entry.order.orderId }),
              h('span', { class: 'aom-muted', text: ` · levererad enligt Amazon` }),
            ]),
            h('button', {
              class: 'aom-btn aom-btn-accent',
              type: 'button',
              text: 'Mottagen ✓',
              onClick: () =>
                act({
                  type: AOM.MSG.SET_RECEIVED,
                  orderId: entry.order.orderId,
                  lineKey: entry.lineKey,
                  value: true,
                }),
            }),
          ])
        );
      }
      for (const anomaly of (data.anomalies || []).filter((a) => a.type === AOM.anomaly.TYPE.REFUND_WITHOUT_REQUEST).slice(0, 5)) {
        actionItems.push(
          h('li', { class: 'aom-action-row' }, [
            h('div', {}, [
              h('strong', { text: anomaly.title || anomaly.orderId }),
              h('span', { class: 'aom-muted', text: ` · ${anomaly.reason}` }),
            ]),
            h('button', {
              class: 'aom-btn',
              type: 'button',
              text: 'Visa avvikelse',
              onClick: () => setView('avvikelser'),
            }),
          ])
        );
      }
      wrap.appendChild(
        h('section', { class: 'aom-card' }, [
          h('h2', { class: 'aom-card-title', text: 'Kräver åtgärd' }),
          actionItems.length
            ? h('ul', { class: 'aom-action-list' }, actionItems)
            : h('p', { class: 'aom-muted', text: 'Inget väntar på dig just nu.' }),
        ])
      );

      const stats = [
        { label: 'Ordrar', value: orders.length },
        { label: 'Att bekräfta', value: counts.pending || 0 },
        { label: 'Avvikelser', value: counts.flagged || 0 },
        { label: 'Bevakningar', value: Object.keys(data.watches || {}).length },
        { label: 'Bevispaket', value: Object.keys(data.proofIndex || {}).length },
      ];
      wrap.appendChild(
        h(
          'section',
          { class: 'aom-stat-grid' },
          stats.map((s) =>
            h('div', { class: 'aom-card aom-stat' }, [
              h('span', { class: 'aom-stat-value', text: String(s.value) }),
              h('span', { class: 'aom-stat-label', text: s.label }),
            ])
          )
        )
      );
      return wrap;
    }

    function orderRow(order) {
      const items = order.lineItems || [];
      const composite = AOM.status.compositeLabel(items);
      const first = items[0] || {};
      const row = h('tr', { class: 'aom-order-row' }, [
        h('td', {}, [
          first.thumbnailUrl
            ? h('img', { class: 'aom-thumb', src: first.thumbnailUrl, alt: '', loading: 'lazy' })
            : h('div', { class: 'aom-thumb aom-thumb-empty' }),
        ]),
        h('td', {}, [
          h('div', { class: 'aom-order-title', text: first.title || '(okänd artikel)' }),
          items.length > 1 ? h('div', { class: 'aom-muted', text: `+ ${items.length - 1} till i ordern` }) : null,
        ]),
        h('td', {}, [
          h('div', { text: order.orderId }),
          h('div', { class: 'aom-muted', text: order.orderDate ? AOM.dates.formatSv(order.orderDate) : 'okänt datum' }),
        ]),
        h('td', { text: order.sellerNameSnapshot || (order.sellerId ? order.sellerId : 'okänd säljare') }),
        h('td', { text: money(order.totalAmount, order.currency) }),
        h('td', {}, [statusChip(composite.status === 'blandad' ? null : composite.status, composite.text)]),
      ]);
      row.addEventListener('click', () => {
        const next = row.nextElementSibling;
        if (next && next.classList.contains('aom-detail-row')) next.hidden = !next.hidden;
      });
      return row;
    }

    function detailRow(order) {
      const cell = h('td', { colspan: '6' });
      const box = h('div', { class: 'aom-detail' });

      box.appendChild(
        h('div', { class: 'aom-detail-actions' }, [
          h('label', { class: 'aom-check' }, [
            h('input', {
              type: 'checkbox',
              checked: !!order.userConfirmedReturnOrRefundRequested,
              onChange: (event) =>
                act({
                  type: AOM.MSG.SET_RETURN_REQUESTED,
                  orderId: order.orderId,
                  value: event.target.checked,
                }),
            }),
            h('span', { text: 'Jag har begärt retur/återbetalning för den här ordern' }),
          ]),
          h('label', { class: 'aom-check' }, [
            h('input', {
              type: 'checkbox',
              checked: !!order.userHasDefect,
              onChange: (event) =>
                act({ type: AOM.MSG.SET_HAS_DEFECT, orderId: order.orderId, value: event.target.checked }),
            }),
            h('span', { text: 'Varan har fel (visar reklamationsfristen)' }),
          ]),
          order.detailUrl
            ? h('button', {
                class: 'aom-btn',
                type: 'button',
                text: 'Öppna ordern på Amazon',
                onClick: () => openUrl(order.detailUrl),
              })
            : null,
          h('button', {
            class: 'aom-btn',
            type: 'button',
            text: 'Kontakta Amazon',
            onClick: () => openUrl('https://www.amazon.se/gp/help/customer/contact-us'),
          }),
          h('button', {
            class: 'aom-btn',
            type: 'button',
            text: 'Spara bevis i efterhand',
            onClick: () => act({ type: AOM.MSG.PROOF_CAPTURE_FOR_ORDER, orderId: order.orderId }),
          }),
        ])
      );

      const settings = (state.data && state.data.settings) || {};
      for (const [index, item] of (order.lineItems || []).entries()) {
        const key = item.lineKey || AOM.status.lineKey(item, index);
        const windows = AOM.dates.returnWindows(item, order, settings);
        const lines = [
          h('div', { class: 'aom-item-head' }, [
            statusChip(item.status),
            h('strong', { text: item.title || '(okänd artikel)' }),
          ]),
          h('div', { class: 'aom-muted', text: `Antal: ${item.quantity ?? 'okänt'} · Styckpris: ${item.unitPrice !== null && item.unitPrice !== undefined ? money(item.unitPrice, item.currency) : 'okänt'}` }),
          h('div', { class: 'aom-windows' }, [
            h('span', {
              text: `Amazons returpolicy: ${
                windows.amazonPolicyDeadline
                  ? AOM.dates.formatSv(windows.amazonPolicyDeadline)
                  : windows.amazonPolicyDays
                  ? `${windows.amazonPolicyDays} dagar`
                  : 'okänt'
              }`,
              title: windows.amazonPolicySourceText || 'Läses av från Amazons sida – aldrig antagen.',
            }),
            h('span', {
              text: `Ångerrätt (14 dagar): ${
                windows.angerrattDeadline ? AOM.dates.formatSv(windows.angerrattDeadline) : 'kräver mottagandedatum'
              }`,
            }),
            order.userHasDefect
              ? h('span', {
                  class: 'aom-warn-text',
                  text: `Reklamationsrätt (3 år): ${
                    windows.reklamationDeadline ? AOM.dates.formatSv(windows.reklamationDeadline) : 'kräver mottagandedatum'
                  }`,
                })
              : null,
            windows.bankDisputeDeadline
              ? h('span', { text: `Bankreklamation: ${AOM.dates.formatSv(windows.bankDisputeDeadline)}` })
              : null,
          ]),
        ];
        if (AOM.status.needsReceiptConfirmation(item)) {
          lines.push(
            h('button', {
              class: 'aom-btn aom-btn-accent',
              type: 'button',
              text: 'Mottagen ✓',
              onClick: () => act({ type: AOM.MSG.SET_RECEIVED, orderId: order.orderId, lineKey: key, value: true }),
            })
          );
        } else if (item.userReceivedConfirmed) {
          lines.push(
            h('span', {
              class: 'aom-confirmed',
              text: `Bekräftad mottagen ${item.userReceivedConfirmedAt ? timeLabel(item.userReceivedConfirmedAt) : ''}`,
            })
          );
        }
        box.appendChild(h('div', { class: 'aom-item' }, lines));
      }

      cell.appendChild(box);
      return h('tr', { class: 'aom-detail-row', hidden: true }, [cell]);
    }

    function viewOrdrar() {
      const data = state.data;
      const wrap = h('div', { class: 'aom-view' });
      const filters = h('div', { class: 'aom-filters' }, [
        h('input', {
          class: 'aom-input',
          type: 'search',
          placeholder: 'Sök titel, ordernummer eller säljare',
          value: state.filter.text,
          onInput: (event) => {
            state.filter.text = event.target.value;
            state.filter.restoreFocus = true;
            render();
          },
        }),
        h(
          'select',
          {
            class: 'aom-input',
            onChange: (event) => {
              state.filter.status = event.target.value;
              render();
            },
          },
          [
            h('option', { value: 'alla', text: 'Alla statusar', selected: state.filter.status === 'alla' }),
            ...Object.entries(AOM.STATUS_LABEL).map(([value, label]) =>
              h('option', { value, text: label, selected: state.filter.status === value })
            ),
          ]
        ),
      ]);
      wrap.appendChild(filters);

      const needle = state.filter.text.trim().toLowerCase();
      const rows = (data.orderList || []).filter((order) => {
        if (state.filter.status !== 'alla') {
          if (!(order.lineItems || []).some((i) => i.status === state.filter.status)) return false;
        }
        if (!needle) return true;
        const hay = [
          order.orderId,
          order.sellerNameSnapshot,
          ...(order.lineItems || []).map((i) => i.title),
        ]
          .filter(Boolean)
          .join(' ')
          .toLowerCase();
        return hay.includes(needle);
      });

      const table = h('table', { class: 'aom-table' }, [
        h('thead', {}, [
          h('tr', {}, ['', 'Artikel', 'Order', 'Säljare', 'Belopp', 'Status'].map((label) => h('th', { text: label }))),
        ]),
      ]);
      const tbody = h('tbody');
      for (const order of rows) {
        tbody.appendChild(orderRow(order));
        tbody.appendChild(detailRow(order));
      }
      table.appendChild(tbody);
      wrap.appendChild(
        rows.length ? table : h('p', { class: 'aom-muted', text: 'Inga ordrar matchar filtret.' })
      );
      return wrap;
    }

    function viewBekrafta() {
      const wrap = h('div', { class: 'aom-view' });
      const pending = AOM.status.pendingConfirmations(state.data.orderList || []);
      if (!pending.length) {
        wrap.appendChild(h('p', { class: 'aom-muted', text: 'Inga leveranser väntar på bekräftelse.' }));
        return wrap;
      }
      wrap.appendChild(
        h(
          'ul',
          { class: 'aom-action-list' },
          pending.map((entry) =>
            h('li', { class: 'aom-action-row' }, [
              h('div', {}, [
                h('strong', { text: entry.item.title || entry.order.orderId }),
                h('div', {
                  class: 'aom-muted',
                  text: `${entry.order.orderId} · ${entry.item.amazonStatusRaw || 'levererad'}`,
                }),
              ]),
              h('button', {
                class: 'aom-btn aom-btn-accent',
                type: 'button',
                text: 'Mottagen ✓',
                onClick: () =>
                  act({
                    type: AOM.MSG.SET_RECEIVED,
                    orderId: entry.order.orderId,
                    lineKey: entry.lineKey,
                    value: true,
                  }),
              }),
            ])
          )
        )
      );
      return wrap;
    }

    function viewAvvikelser() {
      const wrap = h('div', { class: 'aom-view' });
      const anomalies = state.data.anomalies || [];
      if (!anomalies.length) {
        wrap.appendChild(h('p', { class: 'aom-muted', text: 'Inga avvikelser hittade.' }));
        return wrap;
      }
      for (const anomaly of anomalies) {
        const order = (state.data.orders || {})[anomaly.orderId];
        wrap.appendChild(
          h('section', { class: `aom-card aom-anomaly aom-anomaly-${anomaly.type}` }, [
            h('h3', { class: 'aom-card-title', text: anomaly.title || anomaly.orderId }),
            h('p', { text: anomaly.reason }),
            anomaly.evidenceText ? h('p', { class: 'aom-muted', text: `Underlag: ${anomaly.evidenceText}` }) : null,
            h('div', { class: 'aom-detail-actions' }, [
              order
                ? h('label', { class: 'aom-check' }, [
                    h('input', {
                      type: 'checkbox',
                      checked: !!order.userConfirmedReturnOrRefundRequested,
                      onChange: (event) =>
                        act({
                          type: AOM.MSG.SET_RETURN_REQUESTED,
                          orderId: anomaly.orderId,
                          value: event.target.checked,
                        }),
                    }),
                    h('span', { text: 'Jag har begärt retur/återbetalning – ta bort flaggan' }),
                  ])
                : null,
              order && order.detailUrl
                ? h('button', {
                    class: 'aom-btn',
                    type: 'button',
                    text: 'Öppna ordern',
                    onClick: () => openUrl(order.detailUrl),
                  })
                : null,
            ]),
          ])
        );
      }
      return wrap;
    }

    function viewBevakningar() {
      const wrap = h('div', { class: 'aom-view' });
      const watches = Object.values(state.data.watches || {});
      if (!watches.length) {
        wrap.appendChild(
          h('p', { class: 'aom-muted', text: 'Inga bevakningar. Lägg till en från en produktsida på amazon.se.' })
        );
        return wrap;
      }
      wrap.appendChild(
        h(
          'ul',
          { class: 'aom-action-list' },
          watches.map((watch) =>
            h('li', { class: 'aom-action-row' }, [
              h('div', {}, [
                h('strong', { text: watch.title || watch.asin }),
                h('div', {
                  class: 'aom-muted',
                  text: `${watch.asin} · ${
                    watch.lastInStock === null || watch.lastInStock === undefined
                      ? 'inte kontrollerad ännu'
                      : watch.lastInStock
                      ? 'i lager'
                      : 'slut'
                  } · kontrollerad ${timeLabel(watch.lastCheckedAt)}`,
                }),
              ]),
              h('div', {}, [
                h('button', {
                  class: 'aom-btn',
                  type: 'button',
                  text: 'Öppna',
                  onClick: () => openUrl(watch.url || `https://www.amazon.se/dp/${watch.asin}`),
                }),
                h('button', {
                  class: 'aom-btn',
                  type: 'button',
                  text: 'Ta bort',
                  onClick: () => act({ type: AOM.MSG.WATCH_REMOVE, asin: watch.asin }),
                }),
              ]),
            ])
          )
        )
      );
      return wrap;
    }

    function viewBevis() {
      const wrap = h('div', { class: 'aom-view' });
      const bundles = Object.values(state.data.proofIndex || {}).sort((a, b) =>
        String(b.createdAt).localeCompare(String(a.createdAt))
      );
      if (!bundles.length) {
        wrap.appendChild(
          h('p', { class: 'aom-muted', text: 'Bevisarkivet är tomt. Bevis sparas automatiskt vid köp.' })
        );
        return wrap;
      }
      const table = h('table', { class: 'aom-table' }, [
        h('thead', {}, [
          h('tr', {}, ['Sparat', 'Typ', 'Order', 'Titel', 'Storlek', 'SHA-256', ''].map((l) => h('th', { text: l }))),
        ]),
      ]);
      const tbody = h('tbody');
      for (const bundle of bundles) {
        tbody.appendChild(
          h('tr', {}, [
            h('td', { text: timeLabel(bundle.createdAt) }),
            h('td', { text: bundle.kind }),
            h('td', { text: bundle.orderId || '–' }),
            h('td', { text: bundle.title || bundle.asin || '–' }),
            h('td', { text: `${Math.round((bundle.sizeBytes || 0) / 1024)} kB` }),
            h('td', { class: 'aom-mono', text: (bundle.sha256 || '').slice(0, 12) + '…' }),
            h('td', {}, [
              h('button', {
                class: 'aom-btn',
                type: 'button',
                text: 'Öppna',
                onClick: () => openUrl(chrome.runtime.getURL(`pages/proof.html#${bundle.id}`)),
              }),
            ]),
          ])
        );
      }
      table.appendChild(tbody);
      wrap.appendChild(table);
      return wrap;
    }

    function viewSaljare() {
      const wrap = h('div', { class: 'aom-view' });
      const sellers = Object.values(state.data.sellers || {}).sort(
        (a, b) => (b.flaggedIssueCount || 0) - (a.flaggedIssueCount || 0) || (b.purchaseCount || 0) - (a.purchaseCount || 0)
      );
      if (!sellers.length) {
        wrap.appendChild(h('p', { class: 'aom-muted', text: 'Inga säljare registrerade ännu.' }));
        return wrap;
      }
      const table = h('table', { class: 'aom-table' }, [
        h('thead', {}, [
          h('tr', {}, ['Säljare', 'Säljar-ID', 'Typ', 'Köp', 'Flaggade avvikelser'].map((l) => h('th', { text: l }))),
        ]),
      ]);
      const tbody = h('tbody');
      for (const seller of sellers) {
        tbody.appendChild(
          h('tr', {}, [
            h('td', { text: seller.displayName || 'okänt namn' }),
            h('td', { class: 'aom-mono', text: seller.sellerId }),
            h('td', { text: seller.isThirdParty === null || seller.isThirdParty === undefined ? 'okänt' : seller.isThirdParty ? 'Tredjepart' : 'Amazon' }),
            h('td', { text: String(seller.purchaseCount || 0) }),
            h('td', {}, [
              seller.flaggedIssueCount
                ? h('span', { class: 'aom-chip aom-chip-avbruten', text: String(seller.flaggedIssueCount) })
                : h('span', { text: '0' }),
            ]),
          ])
        );
      }
      table.appendChild(tbody);
      wrap.appendChild(table);
      return wrap;
    }

    function viewInstallningar() {
      const wrap = h('div', { class: 'aom-view' });
      const settings = state.data.settings || {};
      wrap.appendChild(
        h('section', { class: 'aom-card' }, [
          h('h2', { class: 'aom-card-title', text: 'Inställningar' }),
          h('p', {
            text: `Bakgrundssynk: ${settings.backgroundSyncEnabled ? `var ${settings.syncIntervalMinutes}:e minut` : 'avstängd'}`,
          }),
          h('p', { text: `Marknader i prisjämförelsen: ${(settings.compareMarketplaces || []).join(', ') || 'inga'}` }),
          h('p', {
            text: `Bankreklamationsfrister: ${
              (settings.bankDisputeDeadlines || []).length
                ? settings.bankDisputeDeadlines.map((b) => `${b.label} ${b.days} dagar`).join('; ')
                : 'inga angivna'
            }`,
          }),
          h('button', {
            class: 'aom-btn aom-btn-primary',
            type: 'button',
            text: 'Öppna alla inställningar',
            onClick: () => {
              if (chrome.runtime.openOptionsPage) chrome.runtime.openOptionsPage();
              else openUrl(chrome.runtime.getURL('options/options.html'));
            },
          }),
          h('p', { class: 'aom-muted', text: AOM.RISK_NOTICE }),
          h('p', { class: 'aom-muted', text: AOM.DISCLAIMER }),
        ])
      );
      return wrap;
    }

    const RENDERERS = {
      oversikt: viewOversikt,
      ordrar: viewOrdrar,
      bekrafta: viewBekrafta,
      avvikelser: viewAvvikelser,
      bevakningar: viewBevakningar,
      bevis: viewBevis,
      saljare: viewSaljare,
      installningar: viewInstallningar,
    };

    function render() {
      renderSidebar();
      renderHeader();
      main.textContent = '';
      if (!state.data) {
        main.appendChild(h('p', { class: 'aom-muted', text: 'Läser in…' }));
        return;
      }
      const renderer = RENDERERS[state.view] || viewOversikt;
      main.appendChild(renderer());
      if (state.filter.restoreFocus) {
        state.filter.restoreFocus = false;
        const search = main.querySelector('input[type="search"]');
        if (search) {
          search.focus();
          const end = search.value.length;
          search.setSelectionRange(end, end);
        }
      }
    }

    render();
    refresh();
    return { refresh, setView, element: shell, state };
  }

  AOM.dashboard = { create, VIEWS, h, money, timeLabel };
})(typeof self !== 'undefined' ? self : globalThis);
