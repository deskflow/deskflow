/* Orderkoll – inline-SVG-diagram för utgifter per månad.
 * En serie, en färg: ingen legend behövs (rubriken namnger serien), värdet
 * skrivs bara ut på den högsta stapeln, axlarna är återhållsamma och varje
 * stapel har en hover-ruta. Fungerar likadant i Shadow DOM som på en vanlig
 * sida eftersom all färg kommer från CSS-variabler. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});
  const NS = 'http://www.w3.org/2000/svg';

  function svgEl(name, attrs) {
    const node = document.createElementNS(NS, name);
    for (const [key, value] of Object.entries(attrs || {})) {
      if (value === null || value === undefined) continue;
      node.setAttribute(key, String(value));
    }
    return node;
  }

  /** Stapel med rundade toppar (4px) och rak fot mot baslinjen. */
  function barPath(x, y, w, h, r) {
    const radius = Math.max(0, Math.min(r, w / 2, h));
    return [
      `M ${x} ${y + h}`,
      `L ${x} ${y + radius}`,
      `A ${radius} ${radius} 0 0 1 ${x + radius} ${y}`,
      `L ${x + w - radius} ${y}`,
      `A ${radius} ${radius} 0 0 1 ${x + w} ${y + radius}`,
      `L ${x + w} ${y + h}`,
      'Z',
    ].join(' ');
  }

  function niceMax(value) {
    if (!Number.isFinite(value) || value <= 0) return 100;
    const magnitude = Math.pow(10, Math.floor(Math.log10(value)));
    const scaled = value / magnitude;
    const step = scaled <= 1 ? 1 : scaled <= 2 ? 2 : scaled <= 5 ? 5 : 10;
    return step * magnitude;
  }

  function formatSek(value) {
    return `${Math.round(value).toLocaleString('sv-SE')} kr`;
  }

  /**
   * data: [{ key: '2026-04', value: 1234, count: 3 }]
   * Returnerar en figur (figure > svg + tooltip) som kan monteras var som helst.
   */
  function monthlySpend(data, options = {}) {
    const rows = (data || []).slice(-12);
    const width = options.width || 560;
    const height = options.height || 180;
    const padding = { top: 18, right: 12, bottom: 26, left: 46 };
    const plotW = width - padding.left - padding.right;
    const plotH = height - padding.top - padding.bottom;

    const figure = document.createElement('figure');
    figure.className = 'aom-chart';

    if (!rows.length) {
      const empty = document.createElement('p');
      empty.className = 'aom-chart-empty';
      empty.textContent = 'Ingen orderhistorik att visa ännu.';
      figure.appendChild(empty);
      return figure;
    }

    const maxValue = niceMax(Math.max(...rows.map((r) => r.value || 0)));
    const gap = 2; // 2px yta mellan staplar
    const bandW = plotW / rows.length;
    const barW = Math.max(4, bandW - gap * 2);
    const maxIndex = rows.reduce((best, r, i) => (r.value > (rows[best] ? rows[best].value : -1) ? i : best), 0);

    const svg = svgEl('svg', {
      viewBox: `0 0 ${width} ${height}`,
      width: '100%',
      height,
      role: 'img',
      'aria-label': options.ariaLabel || 'Utgifter per månad',
      preserveAspectRatio: 'xMidYMid meet',
    });

    // Återhållsamma stödlinjer med värdeetiketter.
    const ticks = [0, 0.5, 1];
    for (const t of ticks) {
      const y = padding.top + plotH - plotH * t;
      svg.appendChild(
        svgEl('line', {
          x1: padding.left,
          x2: width - padding.right,
          y1: y,
          y2: y,
          class: 'aom-chart-grid',
        })
      );
      const label = svgEl('text', {
        x: padding.left - 8,
        y: y + 4,
        'text-anchor': 'end',
        class: 'aom-chart-tick',
      });
      label.textContent = t === 0 ? '0' : formatSek(maxValue * t).replace(' kr', '');
      svg.appendChild(label);
    }

    rows.forEach((row, index) => {
      const value = Math.max(0, row.value || 0);
      const h = maxValue ? (value / maxValue) * plotH : 0;
      const x = padding.left + index * bandW + gap;
      const y = padding.top + plotH - h;

      const path = svgEl('path', {
        d: barPath(x, y, barW, Math.max(h, 1), 4),
        class: 'aom-chart-bar',
        'data-key': row.key,
      });
      svg.appendChild(path);

      // Osynlig träffyta som är större än stapeln.
      const hit = svgEl('rect', {
        x: padding.left + index * bandW,
        y: padding.top,
        width: bandW,
        height: plotH,
        class: 'aom-chart-hit',
        tabindex: '0',
        role: 'button',
        'aria-label': `${AOM.dates.formatMonthSv(row.key)}: ${formatSek(value)}${row.count ? `, ${row.count} ordrar` : ''}`,
      });
      hit.dataset.key = row.key;
      hit.dataset.value = String(value);
      hit.dataset.count = String(row.count || 0);
      svg.appendChild(hit);

      const monthLabel = svgEl('text', {
        x: padding.left + index * bandW + bandW / 2,
        y: height - 8,
        'text-anchor': 'middle',
        class: 'aom-chart-tick',
      });
      monthLabel.textContent = AOM.dates.formatMonthSv(row.key);
      svg.appendChild(monthLabel);

      // Direktetikett bara på den högsta stapeln – aldrig ett tal per stapel.
      if (index === maxIndex && value > 0) {
        const valueLabel = svgEl('text', {
          x: x + barW / 2,
          y: Math.max(padding.top - 4, y - 6),
          'text-anchor': 'middle',
          class: 'aom-chart-value',
        });
        valueLabel.textContent = formatSek(value);
        svg.appendChild(valueLabel);
      }
    });

    const tooltip = document.createElement('div');
    tooltip.className = 'aom-chart-tooltip';
    tooltip.hidden = true;

    const showTooltip = (target) => {
      const key = target.dataset.key;
      const value = Number(target.dataset.value);
      const count = Number(target.dataset.count);
      tooltip.textContent = `${AOM.dates.formatMonthSv(key)} · ${formatSek(value)}${count ? ` · ${count} ordrar` : ''}`;
      tooltip.hidden = false;
      const box = target.getBoundingClientRect();
      const parentBox = figure.getBoundingClientRect();
      tooltip.style.left = `${Math.max(0, box.left - parentBox.left + box.width / 2)}px`;
      tooltip.style.top = `${Math.max(0, box.top - parentBox.top - 6)}px`;
      const bar = svg.querySelector(`path[data-key="${key}"]`);
      svg.querySelectorAll('.aom-chart-bar').forEach((b) => b.classList.remove('is-active'));
      if (bar) bar.classList.add('is-active');
    };
    const hideTooltip = () => {
      tooltip.hidden = true;
      svg.querySelectorAll('.aom-chart-bar').forEach((b) => b.classList.remove('is-active'));
    };

    svg.addEventListener('mouseover', (event) => {
      const target = event.target.closest ? event.target.closest('.aom-chart-hit') : null;
      if (target) showTooltip(target);
    });
    svg.addEventListener('focusin', (event) => {
      if (event.target.classList.contains('aom-chart-hit')) showTooltip(event.target);
    });
    svg.addEventListener('mouseleave', hideTooltip);
    svg.addEventListener('focusout', hideTooltip);

    figure.appendChild(svg);
    figure.appendChild(tooltip);

    // Tabellvy som alternativ till grafen (tillgänglighet).
    const details = document.createElement('details');
    details.className = 'aom-chart-table';
    const summary = document.createElement('summary');
    summary.textContent = 'Visa som tabell';
    details.appendChild(summary);
    const table = document.createElement('table');
    const thead = document.createElement('thead');
    thead.innerHTML = '<tr><th>Månad</th><th>Utgift</th><th>Ordrar</th></tr>';
    table.appendChild(thead);
    const tbody = document.createElement('tbody');
    for (const row of rows) {
      const tr = document.createElement('tr');
      const month = document.createElement('td');
      month.textContent = AOM.dates.formatMonthSv(row.key);
      const value = document.createElement('td');
      value.textContent = formatSek(row.value || 0);
      const count = document.createElement('td');
      count.textContent = String(row.count || 0);
      tr.append(month, value, count);
      tbody.appendChild(tr);
    }
    table.appendChild(tbody);
    details.appendChild(table);
    figure.appendChild(details);
    return figure;
  }

  /** Aggregerar ordrar till månadsvärden i stigande ordning. */
  function monthlySeries(orders, months) {
    const map = new Map();
    for (const order of orders || []) {
      const key = AOM.dates.monthKey(order.orderDate);
      if (!key) continue;
      const prev = map.get(key) || { key, value: 0, count: 0 };
      prev.value += Number(order.totalAmount) || 0;
      prev.count += 1;
      map.set(key, prev);
    }
    const rows = Array.from(map.values()).sort((a, b) => a.key.localeCompare(b.key));
    return months ? rows.slice(-months) : rows;
  }

  AOM.chart = { monthlySpend, monthlySeries, barPath, niceMax, formatSek };
})(typeof self !== 'undefined' ? self : globalThis);
