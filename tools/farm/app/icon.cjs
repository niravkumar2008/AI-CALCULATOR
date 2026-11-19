// Draws the Proofline Farm icon (a pixel sprout in a field) in code and encodes it as PNG / ICO, so the app
// needs no image files. 16 x 16 pixels, scaled up without smoothing.
'use strict';
const zlib = require('zlib');

const PALETTE = { '.': null, g: [76, 175, 80], G: [46, 125, 50], b: [121, 85, 58], B: [93, 64, 44], y: [255, 213, 79], s: [135, 206, 235] };
const ART = [
  'ssssssssssssssss',
  'ssssssssssssssss',
  'sssssyysssssssss',
  'ssssyyyyssssssss',
  'sssssyysssgGssss',
  'ssssssssssgggsss',
  'sssssssgGsgGssss',
  'ssssssgggsgsssss',
  'sssssssgGsgsssss',
  'sssssssssggsssss',
  'ssssssssssgsssss',
  'bbbbbbbbbbgbbbbb',
  'bBbbBbbBbbbbBbbB',
  'bbbbbbbbbbbbbbbb',
  'BbbBbbBbbBbbBbbb',
  'bbbbbbbbbbbbbbbb',
];
const STATUS = { ok: [76, 175, 80], wait: [255, 179, 0], off: [158, 158, 158], alert: [229, 57, 53] };

function crc32(buf) {
  let c, crc = 0xffffffff;
  for (let n = 0; n < buf.length; n++) {
    c = (crc ^ buf[n]) & 0xff;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    crc = (crc >>> 8) ^ c;
  }
  return (crc ^ 0xffffffff) >>> 0;
}

function chunk(type, data) {
  const len = Buffer.alloc(4); len.writeUInt32BE(data.length);
  const td = Buffer.concat([Buffer.from(type), data]);
  const crc = Buffer.alloc(4); crc.writeUInt32BE(crc32(td));
  return Buffer.concat([len, td, crc]);
}

// status: optional dot in the corner (ok / wait / off / alert)
function png(size = 32, status = null) {
  const scale = size / 16;
  const rows = [];
  for (let y = 0; y < size; y++) {
    const row = Buffer.alloc(1 + size * 4);
    for (let x = 0; x < size; x++) {
      const px = Math.floor(x / scale), py = Math.floor(y / scale);
      let c = PALETTE[ART[py][px]];
      if (status && px >= 11 && py <= 4 && (px - 13.5) ** 2 + (py - 2) ** 2 <= 5.5) c = STATUS[status];
      const o = 1 + x * 4;
      if (c) { row[o] = c[0]; row[o + 1] = c[1]; row[o + 2] = c[2]; row[o + 3] = 255; }
    }
    rows.push(row);
  }
  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(size, 0); ihdr.writeUInt32BE(size, 4);
  ihdr[8] = 8; ihdr[9] = 6; // 8-bit RGBA
  return Buffer.concat([
    Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
    chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(Buffer.concat(rows))), chunk('IEND', Buffer.alloc(0)),
  ]);
}

// Windows .ico holding PNG images (fine since Windows Vista).
function ico(sizes = [16, 32, 48, 256]) {
  const imgs = sizes.map(s => png(s));
  const head = Buffer.alloc(6 + 16 * imgs.length);
  head.writeUInt16LE(0, 0); head.writeUInt16LE(1, 2); head.writeUInt16LE(imgs.length, 4);
  let offset = head.length;
  imgs.forEach((img, i) => {
    const e = 6 + i * 16, s = sizes[i];
    head[e] = s >= 256 ? 0 : s; head[e + 1] = s >= 256 ? 0 : s;
    head.writeUInt16LE(1, e + 4); head.writeUInt16LE(32, e + 6);
    head.writeUInt32LE(img.length, e + 8); head.writeUInt32LE(offset, e + 12);
    offset += img.length;
  });
  return Buffer.concat([head, ...imgs]);
}

module.exports = { png, ico };
