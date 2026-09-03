/* Orderkoll – perceptuell bildhash (dHash) för bildmatchning (arbetsorder 5.5).
 * Körs i service workern, där createImageBitmap och OffscreenCanvas finns. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});

  const W = 9;
  const H = 8;

  /** 64-bitars differenshash som hexsträng, eller null om bilden inte kan läsas. */
  async function dHashFromBlob(blob) {
    if (typeof createImageBitmap !== 'function' || typeof OffscreenCanvas !== 'function') {
      return null;
    }
    let bitmap;
    try {
      bitmap = await createImageBitmap(blob);
    } catch (_err) {
      return null;
    }
    const canvas = new OffscreenCanvas(W, H);
    const ctx = canvas.getContext('2d', { willReadFrequently: true });
    ctx.drawImage(bitmap, 0, 0, W, H);
    bitmap.close && bitmap.close();
    const { data } = ctx.getImageData(0, 0, W, H);

    const gray = new Array(W * H);
    for (let i = 0; i < W * H; i += 1) {
      const r = data[i * 4];
      const g = data[i * 4 + 1];
      const b = data[i * 4 + 2];
      gray[i] = 0.299 * r + 0.587 * g + 0.114 * b;
    }
    let bits = '';
    for (let y = 0; y < H; y += 1) {
      for (let x = 0; x < W - 1; x += 1) {
        bits += gray[y * W + x] > gray[y * W + x + 1] ? '1' : '0';
      }
    }
    let hex = '';
    for (let i = 0; i < bits.length; i += 4) {
      hex += parseInt(bits.slice(i, i + 4), 2).toString(16);
    }
    return hex;
  }

  function hexToBits(hex) {
    let bits = '';
    for (const ch of String(hex)) {
      bits += parseInt(ch, 16).toString(2).padStart(4, '0');
    }
    return bits;
  }

  /** Hammingavstånd mellan två hexhashar (samma längd), annars null. */
  function hamming(hexA, hexB) {
    if (!hexA || !hexB || hexA.length !== hexB.length) return null;
    const a = hexToBits(hexA);
    const b = hexToBits(hexB);
    let dist = 0;
    for (let i = 0; i < a.length; i += 1) if (a[i] !== b[i]) dist += 1;
    return dist;
  }

  /** 1.0 = identiska. Jämförs mot användarens tröskelvärde. */
  function similarity(hexA, hexB) {
    const dist = hamming(hexA, hexB);
    if (dist === null) return null;
    const bits = String(hexA).length * 4;
    return 1 - dist / bits;
  }

  AOM.imageHash = { dHashFromBlob, hamming, similarity, hexToBits };
})(typeof self !== 'undefined' ? self : globalThis);
