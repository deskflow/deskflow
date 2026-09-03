/* Orderkoll – minimal ZIP-skrivare (metod "stored", ingen komprimering).
 * Används för bevisexport: råfiler + manifest med SHA-256-hashar.
 * Egen implementation för att slippa tredjepartsberoenden i tillägget. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});

  const CRC_TABLE = (() => {
    const table = new Uint32Array(256);
    for (let n = 0; n < 256; n += 1) {
      let c = n;
      for (let k = 0; k < 8; k += 1) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
      table[n] = c >>> 0;
    }
    return table;
  })();

  function crc32(bytes) {
    let c = 0xffffffff;
    for (let i = 0; i < bytes.length; i += 1) {
      c = CRC_TABLE[(c ^ bytes[i]) & 0xff] ^ (c >>> 8);
    }
    return (c ^ 0xffffffff) >>> 0;
  }

  function dosDateTime(date) {
    const d = date || new Date();
    const time = ((d.getHours() << 11) | (d.getMinutes() << 5) | (d.getSeconds() / 2)) & 0xffff;
    const day = (((d.getFullYear() - 1980) << 9) | ((d.getMonth() + 1) << 5) | d.getDate()) & 0xffff;
    return { time, day };
  }

  function writeU16(view, offset, value) {
    view.setUint16(offset, value, true);
  }
  function writeU32(view, offset, value) {
    view.setUint32(offset, value >>> 0, true);
  }

  /**
   * files: [{ name: 'sokvag/fil.html', data: Uint8Array }]
   * Returnerar en Blob (application/zip).
   */
  function createZip(files, options = {}) {
    const encoder = new TextEncoder();
    const entries = files.map((file) => {
      const nameBytes = encoder.encode(file.name);
      const data = file.data instanceof Uint8Array ? file.data : new Uint8Array(file.data);
      return { nameBytes, data, crc: crc32(data) };
    });
    const { time, day } = dosDateTime(options.date);

    let localSize = 0;
    let centralSize = 0;
    for (const e of entries) {
      localSize += 30 + e.nameBytes.length + e.data.length;
      centralSize += 46 + e.nameBytes.length;
    }
    const total = localSize + centralSize + 22;
    const buf = new ArrayBuffer(total);
    const bytes = new Uint8Array(buf);
    const view = new DataView(buf);

    let offset = 0;
    const offsets = [];
    for (const e of entries) {
      offsets.push(offset);
      writeU32(view, offset, 0x04034b50);
      writeU16(view, offset + 4, 20); // version needed
      writeU16(view, offset + 6, 0x0800); // UTF-8 filnamn
      writeU16(view, offset + 8, 0); // stored
      writeU16(view, offset + 10, time);
      writeU16(view, offset + 12, day);
      writeU32(view, offset + 14, e.crc);
      writeU32(view, offset + 18, e.data.length);
      writeU32(view, offset + 22, e.data.length);
      writeU16(view, offset + 26, e.nameBytes.length);
      writeU16(view, offset + 28, 0);
      bytes.set(e.nameBytes, offset + 30);
      bytes.set(e.data, offset + 30 + e.nameBytes.length);
      offset += 30 + e.nameBytes.length + e.data.length;
    }

    const centralStart = offset;
    entries.forEach((e, i) => {
      writeU32(view, offset, 0x02014b50);
      writeU16(view, offset + 4, 20); // version made by
      writeU16(view, offset + 6, 20); // version needed
      writeU16(view, offset + 8, 0x0800);
      writeU16(view, offset + 10, 0);
      writeU16(view, offset + 12, time);
      writeU16(view, offset + 14, day);
      writeU32(view, offset + 16, e.crc);
      writeU32(view, offset + 20, e.data.length);
      writeU32(view, offset + 24, e.data.length);
      writeU16(view, offset + 28, e.nameBytes.length);
      writeU16(view, offset + 30, 0); // extra
      writeU16(view, offset + 32, 0); // comment
      writeU16(view, offset + 34, 0); // disk
      writeU16(view, offset + 36, 0); // internal attrs
      writeU32(view, offset + 38, 0); // external attrs
      writeU32(view, offset + 42, offsets[i]);
      bytes.set(e.nameBytes, offset + 46);
      offset += 46 + e.nameBytes.length;
    });

    writeU32(view, offset, 0x06054b50);
    writeU16(view, offset + 4, 0);
    writeU16(view, offset + 6, 0);
    writeU16(view, offset + 8, entries.length);
    writeU16(view, offset + 10, entries.length);
    writeU32(view, offset + 12, offset - centralStart);
    writeU32(view, offset + 16, centralStart);
    writeU16(view, offset + 20, 0);

    return typeof Blob === 'function' ? new Blob([bytes], { type: 'application/zip' }) : bytes;
  }

  AOM.zip = { createZip, crc32 };
})(typeof self !== 'undefined' ? self : globalThis);
