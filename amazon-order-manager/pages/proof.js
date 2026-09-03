/* Orderkoll – bevisvyn.
 * Sidan är en extension-sida och läser IndexedDB direkt (samma origin som
 * service workern), eftersom blobbar inte kan skickas via meddelanden.
 * Ingenting skickas härifrån – exporten är en lokal nedladdning. */
(function () {
  'use strict';
  const AOM = self.AOM;
  const $ = (id) => document.getElementById(id);
  const bundleId = decodeURIComponent((location.hash || '').replace('#', ''));
  let bundle = null;
  let blobs = [];

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

  async function sha256Hex(bytes) {
    const digest = await crypto.subtle.digest('SHA-256', bytes);
    return Array.from(new Uint8Array(digest))
      .map((b) => b.toString(16).padStart(2, '0'))
      .join('');
  }

  function row(label, value) {
    const dt = document.createElement('div');
    dt.className = 'aom-muted';
    dt.textContent = label;
    const dd = document.createElement('div');
    dd.textContent = value === null || value === undefined || value === '' ? 'okänt' : String(value);
    $('facts').append(dt, dd);
  }

  async function verify() {
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
      if ((await sha256Hex(bytes)) !== file.sha256) problems.push(`${file.name}: hash stämmer inte`);
    }
    const lines = bundle.files
      .filter((f) => f.sha256)
      .map((f) => `${f.name}:${f.sha256}`)
      .sort()
      .join('\n');
    const recomputed = await sha256Hex(new TextEncoder().encode(lines));
    if (recomputed !== bundle.sha256) problems.push('samlingshashen stämmer inte');

    const el = $('verify');
    if (problems.length) {
      el.className = 'proof-bad';
      el.textContent = `Integritetskontroll misslyckades: ${problems.join('; ')}`;
    } else {
      el.className = 'proof-ok';
      el.textContent = 'Integritetskontroll OK – innehållet är oförändrat sedan det sparades.';
    }
  }

  function renderFiles() {
    const table = $('files');
    table.textContent = '';
    const thead = document.createElement('thead');
    thead.innerHTML = '<tr><th>Fil</th><th>Typ</th><th>Storlek</th><th>SHA-256</th></tr>';
    table.appendChild(thead);
    const tbody = document.createElement('tbody');
    for (const file of bundle.files) {
      const tr = document.createElement('tr');
      const name = document.createElement('td');
      name.textContent = file.name;
      const type = document.createElement('td');
      type.textContent = file.type || '–';
      const size = document.createElement('td');
      size.textContent = `${Math.round((file.size || 0) / 1024)} kB`;
      const hash = document.createElement('td');
      hash.className = 'aom-mono';
      hash.textContent = file.sha256 || '–';
      tr.append(name, type, size, hash);
      tbody.appendChild(tr);
    }
    table.appendChild(tbody);
  }

  function renderImages() {
    const box = $('images');
    box.textContent = '';
    const images = blobs.filter((b) => (b.type || '').startsWith('image/'));
    if (!images.length) {
      box.appendChild(Object.assign(document.createElement('p'), { className: 'aom-muted', textContent: 'Inga bilder sparade i paketet.' }));
      return;
    }
    for (const blob of images) {
      const img = document.createElement('img');
      img.src = URL.createObjectURL(blob.blob);
      img.alt = blob.name;
      img.title = `${blob.name} – högerklicka för att spara`;
      box.appendChild(img);
    }
  }

  async function downloadZip() {
    const files = [];
    for (const blob of blobs) {
      files.push({ name: blob.name, data: new Uint8Array(await blob.blob.arrayBuffer()) });
    }
    const manifest = {
      bundleId: bundle.id,
      kind: bundle.kind,
      orderId: bundle.orderId,
      asin: bundle.asin,
      pageUrl: bundle.pageUrl,
      createdAt: bundle.createdAt,
      bundleSha256: bundle.sha256,
      files: bundle.files,
      note:
        'Hasharna beräknades när beviset sparades. Samlingshashen är SHA-256 över raderna "filnamn:filhash" i namnordning.',
    };
    files.push({ name: 'manifest.json', data: new TextEncoder().encode(JSON.stringify(manifest, null, 2)) });
    const zipBlob = AOM.zip.createZip(files);
    const a = document.createElement('a');
    a.href = URL.createObjectURL(zipBlob);
    a.download = `orderkoll-bevis-${(bundle.orderId || bundle.asin || bundle.id).replace(/[^\w-]/g, '')}.zip`;
    a.click();
    setTimeout(() => URL.revokeObjectURL(a.href), 4000);
  }

  async function openSavedHtml() {
    const html = blobs.find((b) => b.name === 'sida.html');
    if (!html) return;
    const url = URL.createObjectURL(html.blob);
    window.open(url, '_blank', 'noopener');
  }

  async function runDiff() {
    const saved = blobs.find((b) => b.name === 'sida.html');
    if (!saved || !bundle.pageUrl) {
      $('diffOut').textContent = 'Ingen sparad HTML eller ingen käll-URL – kan inte jämföra.';
      $('diffCard').hidden = false;
      return;
    }
    $('diffCard').hidden = false;
    $('diffOut').textContent = 'Hämtar dagens version…';
    const res = await send({ type: AOM.MSG.FETCH_URL, url: bundle.pageUrl });
    if (!res || res.error) {
      $('diffOut').textContent = `Kunde inte hämta dagens version: ${(res && res.error) || 'okänt fel'}`;
      return;
    }
    const savedHtml = await saved.blob.text();
    const { changes } = AOM.proofDiff.diffHtml(savedHtml, res.text, bundle.pageUrl);
    if (!changes.length) {
      $('diffOut').textContent = 'Inga skillnader hittades i titel, pris, bild, lagerstatus eller specifikationer.';
      return;
    }
    const table = document.createElement('table');
    table.className = 'aom-table proof-diff';
    const thead = document.createElement('thead');
    thead.innerHTML = '<tr><th>Fält</th><th>Sparat</th><th>Idag</th></tr>';
    table.appendChild(thead);
    const tbody = document.createElement('tbody');
    for (const change of changes) {
      const tr = document.createElement('tr');
      const f = document.createElement('td');
      f.textContent = change.label;
      const a = document.createElement('td');
      a.textContent = change.saved === null ? '(saknas)' : change.saved;
      const b = document.createElement('td');
      b.textContent = change.live === null ? '(saknas)' : change.live;
      tr.append(f, a, b);
      tbody.appendChild(tr);
    }
    table.appendChild(tbody);
    $('diffOut').textContent = '';
    $('diffOut').appendChild(table);
  }

  async function init() {
    if (!bundleId) {
      $('subtitle').textContent = 'Inget bevispaket angivet.';
      return;
    }
    bundle = await AOM.db.getBundle(bundleId);
    if (!bundle) {
      $('subtitle').textContent = 'Bevispaketet hittades inte.';
      return;
    }
    blobs = await AOM.db.blobsForBundle(bundleId);

    $('title').textContent = bundle.title || bundle.orderId || bundle.asin || 'Bevispaket';
    $('subtitle').textContent = `${bundle.kind} · sparat ${new Date(bundle.createdAt).toLocaleString('sv-SE')}`;

    row('Ordernummer', bundle.orderId);
    row('ASIN', bundle.asin);
    row('Käll-URL', bundle.pageUrl);
    row('Pris vid tillfället', bundle.meta && bundle.meta.price !== undefined && bundle.meta.price !== null ? `${bundle.meta.price} ${bundle.meta.currency || ''}` : null);
    row('Lagerstatus', bundle.meta && bundle.meta.availability);
    row('Varumärke', bundle.meta && bundle.meta.brand);
    row('EAN', bundle.meta && bundle.meta.ean);
    row('Antal filer', bundle.files.length);
    row('Samlingshash (SHA-256)', bundle.sha256);

    renderFiles();
    renderImages();
    await verify();

    $('zip').addEventListener('click', downloadZip);
    $('print').addEventListener('click', () => window.print());
    $('diff').addEventListener('click', runDiff);
    $('open').addEventListener('click', openSavedHtml);
  }

  init();
})();
