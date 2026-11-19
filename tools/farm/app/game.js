// Proofline Farm world: a top-down pixel farm drawn in code (no image files).
// Pixel art is drawn on a small canvas and scaled up by a whole number; names, bubbles and signs are drawn
// on the big canvas with fillText. Text from the farm (written by AI runs) only ever goes through fillText.
'use strict';
(function () {
  const CW = 480, CH = 270;      // the designed farm area, in world pixels
  const RY = 117;                // middle of the main road
  const SPEED = 34;              // walking speed, world pixels per second
  const GRASS = '#7cb35a';

  let canvas, ctx, lo, lx, bg, bx;
  let k = 2, dpr = 1, LW = CW, LH = CH, ox = 0, oy = 0, bw = 0, bh = 0;
  let insets = { left: 0, top: 0, right: 0, bottom: 0 };
  let model = { agents: [], plots: [], harvest: [], readyCount: 0, mail: 0, usage: {}, asleep: false, off: false };
  let handlers = {};
  let selected = null, hoverHit = null;
  const rm = window.matchMedia('(prefers-reduced-motion: reduce)');
  let rafId = 0, last = 0, clock = 0, bgKey = '', lastBgCheck = 0, hourOverride = null;
  const chars = new Map();
  const slots = new Array(6).fill(null);

  // ---------- helpers ----------
  const rng = seed => () => { seed = seed + 0x6D2B79F5 | 0; let t = Math.imul(seed ^ seed >>> 15, 1 | seed); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; };
  function hash(s) { let h = 2166136261; for (const ch of String(s)) { h ^= ch.charCodeAt(0); h = Math.imul(h, 16777619); } return h >>> 0; }
  const safeColor = c => /^#[0-9a-f]{6}$/i.test(c || '') ? c : (/^#[0-9a-f]{3}$/i.test(c || '') ? '#' + c.slice(1).split('').map(x => x + x).join('') : '#90a4ae');
  function shade(hex, amt) {
    const n = parseInt(safeColor(hex).slice(1), 16);
    let r = n >> 16, g = n >> 8 & 255, b = n & 255;
    const t = amt < 0 ? 0 : 255, p = Math.abs(amt);
    r = Math.round(r + (t - r) * p); g = Math.round(g + (t - g) * p); b = Math.round(b + (t - b) * p);
    return `rgb(${r},${g},${b})`;
  }
  function R(g, x, y, w, h, c) { g.fillStyle = c; g.fillRect(x, y, w, h); }
  function disc(g, cx, cy, r, c) { g.fillStyle = c; for (let dy = -r; dy <= r; dy++) { const w = Math.round(Math.sqrt(r * r - dy * dy)); g.fillRect(cx - w, cy + dy, 2 * w + 1, 1); } }
  function oval(g, cx, cy, rx, ry, c) { g.fillStyle = c; for (let dy = -ry; dy <= ry; dy++) { const w = Math.round(rx * Math.sqrt(Math.max(0, 1 - (dy / ry) ** 2))); g.fillRect(cx - w, cy + dy, 2 * w + 1, 1); } }
  function shadow(g, x, y, w, h) { g.fillStyle = 'rgba(25,45,15,.28)'; g.fillRect(x + 3, y + h, w, 4); g.fillRect(x + w, y + 5, 5, h - 1); }
  // A 3/4-view roof: trapezoid with shingle rows.
  function roof(g, x, y, w, h, base, dark, light) {
    for (let i = 0; i < h; i++) {
      const inset = Math.round(5 * (1 - i / h));
      const row = i === 0 ? light : (i % 4 === 3 || i >= h - 2) ? dark : base;
      R(g, x + inset, y + i, w - 2 * inset, 1, row);
      if (i % 4 === 1) for (let sx = x + inset + ((i >> 2) % 2 ? 3 : 0); sx < x + w - inset; sx += 6) R(g, sx, y + i, 1, 2, dark);
    }
  }
  function windowAt(g, x, y, w, h, lit, shutter) {
    R(g, x - 1, y - 1, w + 2, h + 2, '#5a3d26');
    R(g, x, y, w, h, lit ? '#ffd77a' : '#8fc3e0');
    if (!lit) { R(g, x + 1, y + 1, 2, 2, '#d7f0ff'); }
    R(g, x + (w >> 1), y, 1, h, '#5a3d26'); R(g, x, y + (h >> 1), w, 1, '#5a3d26');
    if (shutter) { R(g, x - 4, y - 1, 3, h + 2, shutter); R(g, x + w + 1, y - 1, 3, h + 2, shutter); }
  }
  function door(g, x, y, w, h, c) {
    R(g, x - 1, y - 1, w + 2, h + 1, '#3d2a1b'); R(g, x, y, w, h, c);
    R(g, x + 2, y + 2, w - 4, (h >> 1) - 3, shade(c, -0.18)); R(g, x + 2, y + (h >> 1) + 1, w - 4, (h >> 1) - 3, shade(c, -0.18));
    R(g, x + w - 3, y + (h >> 1), 1, 2, '#ffd54f');
  }
  function tree(g, x, y, rnd, pine) {
    oval(g, x + 3, y, 9, 3, 'rgba(25,45,15,.3)');
    R(g, x - 2, y - 8, 4, 8, '#6b4a2e'); R(g, x - 2, y - 8, 1, 8, '#8a6440');
    if (pine) {
      for (let i = 0; i < 4; i++) { const w = 3 + i * 2.5 | 0; R(g, x - w, y - 26 + i * 5, w * 2 + 1, 6, i % 2 ? '#2f6b3a' : '#3c7d45'); R(g, x - w + 1, y - 26 + i * 5, 2, 1, '#5da05f'); }
      R(g, x, y - 29, 1, 3, '#3c7d45');
      return;
    }
    disc(g, x, y - 14, 9, '#3a7334'); disc(g, x - 3, y - 16, 7, '#4b9142'); disc(g, x + 3, y - 18, 6, '#58a64e');
    for (let i = 0; i < 6; i++) R(g, x - 6 + (rnd() * 10 | 0), y - 22 + (rnd() * 10 | 0), 1, 1, '#7cc66a');
    if (rnd() < 0.4) for (let i = 0; i < 3; i++) R(g, x - 5 + (rnd() * 10 | 0), y - 18 + (rnd() * 8 | 0), 2, 2, '#e05050');
  }
  function bush(g, x, y) { oval(g, x + 1, y + 1, 6, 2, 'rgba(25,45,15,.25)'); disc(g, x, y - 3, 4, '#4b9142'); disc(g, x + 3, y - 2, 3, '#58a64e'); R(g, x - 1, y - 5, 1, 1, '#7cc66a'); }

  // ---------- the map ----------
  const HOMES = {
    workshop: { x: 20, y: 30, w: 74, h: 70, door: 57, name: 'Workshop' },
    farmhouse: { x: 120, y: 22, w: 84, h: 78, door: 162, name: 'Farmhouse' },
    silo: { x: 212, y: 18, w: 28, h: 82, name: 'Silo' },
    shed: { x: 250, y: 48, w: 54, h: 52, door: 275, name: 'Seed shed' },
    library: { x: 318, y: 26, w: 76, h: 74, door: 355, name: 'Library' },
    tower: { x: 412, y: 6, w: 48, h: 94, door: 436, name: 'Watchtower' },
    market: { x: 26, y: 140, w: 78, h: 50, door: 66, name: 'Market' },
    postoffice: { x: 62, y: 207, w: 42, h: 34, door: 83, name: 'Post office' },
  };
  const MAILBOX = { x: 202, y: 92, w: 16, h: 20 };
  const FIELD = { x: 130, y: 136, w: 200, h: 104 };
  const PLOTS = [[142, 148], [206, 148], [270, 148], [142, 194], [206, 194], [270, 194]];
  const PW = 56, PH = 38;
  const BASKETS = { x: 336, y: 140, w: 46, h: 32 };
  const LOC = {
    'home:workshop': [[57, RY], [57, 106]],
    'home:farmhouse': [[162, RY], [162, 106]],
    'home:library': [[355, RY], [355, 106]],
    'home:tower': [[436, RY], [436, 106]],
    'home:market': [[112, RY], [112, 199], [66, 199]],
    'home:postoffice': [[112, RY], [112, 199], [83, 199], [86, 203]],
    'home:shed': [[275, RY], [275, 106]],
    desk: [[190, RY], [190, 104]],
    bench1: [[106, RY], [106, 104]],
    bench2: [[408, RY], [408, 171]],
    field: [[232, RY], [232, 143]],
  };
  PLOTS.forEach(([px, py], i) => {
    LOC[`plot:${i}`] = [[232, RY], [232, 143], [px + 18, py + 36]];
    LOC[`check:${i}`] = [[232, RY], [232, 143], [px + 42, py + 36]];
  });
  const homeOf = a => HOMES[a.home] && LOC[`home:${a.home}`] ? a.home : a.role === 'foreman' ? 'farmhouse' : a.role === 'inspector' ? 'tower' : 'workshop';
  const benchOf = home => ['library', 'tower'].includes(home) ? 'bench2' : 'bench1';

  // ---------- day and night ----------
  const SKY = [[0, 138, 150, 206], [5, 138, 150, 206], [6.5, 240, 196, 178], [8, 255, 255, 255], [17.5, 255, 255, 255], [19, 250, 192, 150], [20.5, 138, 150, 206], [24, 138, 150, 206]];
  function hourNow() { if (hourOverride != null) return hourOverride; const d = new Date(); return d.getHours() + d.getMinutes() / 60; }
  function sky(h) {
    for (let i = 1; i < SKY.length; i++) if (h <= SKY[i][0]) {
      const a = SKY[i - 1], b = SKY[i], t = (h - a[0]) / (b[0] - a[0] || 1);
      return [0, 1, 2].map(j => Math.round(a[j + 1] + (b[j + 1] - a[j + 1]) * t));
    }
    return [255, 255, 255];
  }
  const darkness = h => { const c = sky(h); return 1 - (c[0] + c[1] + c[2]) / 765; };

  // ---------- static background (rebuilt on resize / when lights turn on) ----------
  function buildBg(lit) {
    bg.width = LW; bg.height = LH;
    const g = bx;
    g.setTransform(1, 0, 0, 1, 0, 0);
    R(g, 0, 0, LW, LH, GRASS);
    const rnd = rng(11);
    for (let i = 0; i < LW * LH / 10; i++) R(g, rnd() * LW | 0, rnd() * LH | 0, 1, 1, rnd() < 0.5 ? '#6fa64f' : '#89c066');
    for (let i = 0; i < LW * LH / 180; i++) { const x = rnd() * LW | 0, y = rnd() * LH | 0; R(g, x, y, 1, 2, '#5f9845'); R(g, x + 2, y, 1, 2, '#5f9845'); R(g, x + 1, y - 1, 1, 2, '#6aa64c'); }
    const flowers = ['#fff3a0', '#ffffff', '#f8a5c2', '#c5a3ff', '#ffcf6b'];
    for (let i = 0; i < LW * LH / 420; i++) { const x = rnd() * LW | 0, y = rnd() * LH | 0, c = flowers[rnd() * flowers.length | 0]; R(g, x, y, 1, 1, c); R(g, x - 1, y + 1, 1, 1, c); R(g, x + 1, y + 1, 1, 1, c); R(g, x, y + 1, 1, 1, '#e0a030'); }
    g.setTransform(1, 0, 0, 1, ox, oy);

    // main road (full width) and paths
    const road = (x, y, w, h) => { R(g, x, y, w, h, '#d2b47a'); for (let i = 0; i < w * h / 18; i++) R(g, x + rnd() * w | 0, y + rnd() * h | 0, 1, 1, rnd() < 0.5 ? '#c2a368' : '#e2c792'); };
    road(-ox, RY - 5, LW, 10); R(g, -ox, RY - 6, LW, 1, '#b8975e'); R(g, -ox, RY + 5, LW, 1, '#b8975e');
    road(52, 100, 11, 12); road(156, 106, 13, 6); road(270, 100, 11, 12); road(350, 100, 11, 12); road(431, 100, 11, 12);
    road(226, RY + 5, 13, 16); road(107, RY + 5, 11, 84); road(40, 194, 78, 10); road(352, RY + 5, 9, 18);

    // pond
    oval(g, 424, 207, 41, 29, '#d9c48f'); oval(g, 424, 207, 38, 26, '#3a7fae'); oval(g, 424, 205, 35, 23, '#4fa3d1');
    oval(g, 414, 199, 12, 5, '#6cbbe2');
    [[400, 214], [440, 196], [446, 220]].forEach(([x, y]) => { oval(g, x, y, 4, 2, '#4f9a45'); R(g, x + 1, y - 1, 2, 1, '#3a7334'); });
    R(g, 437, 218, 2, 2, '#f8a5c2');
    for (let i = 0; i < 9; i++) { const a = i / 9 * Math.PI * 2; R(g, 424 + Math.cos(a) * 40 | 0, 207 + Math.sin(a) * 28 | 0, 1, 3, '#4f8a3d'); }

    // field: fence, darker ground, empty plots
    R(g, FIELD.x, FIELD.y, FIELD.w, FIELD.h, '#73a852');
    for (let i = 0; i < 260; i++) R(g, FIELD.x + rnd() * FIELD.w | 0, FIELD.y + rnd() * FIELD.h | 0, 1, 1, '#669a48');
    PLOTS.forEach(([px, py]) => {
      R(g, px - 1, py - 1, PW + 2, PH + 2, '#5e3d27'); R(g, px, py, PW, PH, '#7a5236');
      for (let fy = py + 4; fy < py + PH; fy += 6) { R(g, px + 1, fy, PW - 2, 1, '#62412b'); R(g, px + 1, fy - 1, PW - 2, 1, '#8a6040'); }
    });
    const post = (x, y) => { R(g, x, y - 6, 2, 7, '#efe3c8'); R(g, x + 1, y - 6, 1, 7, '#cdbb98'); R(g, x, y - 7, 2, 1, '#fff7e6'); };
    for (let x = FIELD.x; x <= FIELD.x + FIELD.w; x += 10) {
      if (x > 220 && x < 242) continue;
      post(x, FIELD.y); post(x, FIELD.y + FIELD.h);
    }
    const rail = (x, y, w) => { R(g, x, y - 5, w, 1, '#e6d8b8'); R(g, x, y - 2, w, 1, '#e6d8b8'); };
    rail(FIELD.x, FIELD.y, 92); rail(242, FIELD.y, FIELD.x + FIELD.w - 242); rail(FIELD.x, FIELD.y + FIELD.h, FIELD.w);
    for (let y = FIELD.y; y <= FIELD.y + FIELD.h; y += 8) { post(FIELD.x, y); post(FIELD.x + FIELD.w, y); }
    R(g, FIELD.x, FIELD.y - 5, 1, FIELD.h, '#e6d8b8'); R(g, FIELD.x + FIELD.w + 1, FIELD.y - 5, 1, FIELD.h, '#e6d8b8');

    // chicken coop
    shadow(g, 30, 214, 26, 20); R(g, 30, 222, 26, 12, '#c0743f'); roof(g, 27, 212, 32, 11, '#8f3b2e', '#6e2c22', '#b5543c');
    R(g, 39, 226, 8, 8, '#3d2a1b'); R(g, 32, 234, 22, 2, '#d9c48f');

    // trees & bushes
    const trees = [[104, 22, 0], [8, 64, 1], [9, 104, 0], [6, 150, 1], [244, 20, 0], [308, 30, 1], [400, 30, 0], [470, 116, 1],
      [12, 256, 0], [70, 258, 1], [124, 262, 0], [350, 252, 0], [300, 262, 1], [474, 246, 0], [376, 176, 1], [470, 168, 0], [20, 196, 1]];
    for (let i = 0; i < 60; i++) {
      const x = -ox + rnd() * LW | 0, y = -oy + rnd() * LH | 0;
      if (x > -6 && x < CW + 6 && y > -10 && y < CH + 22) continue;
      if (y > RY - 14 && y < RY + 30) continue;
      trees.push([x, y, rnd() < 0.4 ? 1 : 0]);
    }
    trees.sort((a, b) => a[1] - b[1]).forEach(([x, y, p]) => tree(g, x, y, rnd, p));
    [[114, 134], [124, 238], [338, 236], [110, 248], [342, 132], [205, 252], [262, 256]].forEach(([x, y]) => bush(g, x, y));

    // buildings
    drawWorkshop(g, lit); drawFarmhouse(g, lit); drawSilo(g); drawShed(g, lit); drawLibrary(g, lit); drawTower(g, lit); drawMarket(g); drawPostOffice(g, lit);

    // benches, lanterns, mailbox post
    const bench = (x, y) => { R(g, x, y - 6, 16, 2, '#8a5d34'); R(g, x, y - 3, 16, 3, '#a0683a'); R(g, x, y - 3, 16, 1, '#bf8550'); R(g, x + 1, y, 2, 3, '#5a3d26'); R(g, x + 13, y, 2, 3, '#5a3d26'); R(g, x + 2, y + 3, 14, 1, 'rgba(25,45,15,.3)'); };
    bench(98, 101); bench(400, 168);
    [112, 244, 312, 404, 6].forEach(x => { R(g, x, 100, 2, 12, '#4a3526'); R(g, x - 1, 96, 4, 5, '#3a2a1e'); R(g, x, 97, 2, 3, lit ? '#ffe9a8' : '#c9b37a'); R(g, x + 2, 111, 3, 1, 'rgba(25,45,15,.3)'); });
    R(g, 208, 102, 2, 10, '#6b4a2e'); R(g, 209, 112, 4, 1, 'rgba(25,45,15,.3)');
    // basket area: little straw mat
    R(g, BASKETS.x, BASKETS.y + 6, BASKETS.w, BASKETS.h - 6, '#cdb177'); R(g, BASKETS.x, BASKETS.y + 6, BASKETS.w, 1, '#e3cd96');
    g.setTransform(1, 0, 0, 1, 0, 0);
  }

  function drawFarmhouse(g, lit) {
    const { x, y } = HOMES.farmhouse;
    shadow(g, x, y, 84, 78);
    R(g, x, 58, 84, 42, '#ead7ae');
    for (let sy = 61; sy < 100; sy += 4) R(g, x, sy, 84, 1, '#d4bd8f');
    R(g, x, 58, 2, 42, '#c9ae7e'); R(g, x + 82, 58, 2, 42, '#c9ae7e'); R(g, x, 96, 84, 4, '#9b7a55');
    R(g, 178, 12, 9, 16, '#9c5b45'); R(g, 177, 11, 11, 3, '#6e3b2c'); R(g, 178, 15, 9, 1, '#b56e55');
    roof(g, x - 4, y, 92, 37, '#c0443a', '#8f2f28', '#e0705f');
    R(g, x - 4, 58, 92, 2, '#6e2420');
    windowAt(g, 130, 68, 12, 11, lit, '#3e7c4a'); windowAt(g, 182, 68, 12, 11, lit, '#3e7c4a');
    R(g, 127, 80, 18, 2, '#7a4e2d'); R(g, 179, 80, 18, 2, '#7a4e2d');
    [129, 133, 137, 141, 181, 185, 189, 193].forEach((fx, i) => R(g, fx, 79, 2, 1, i % 2 ? '#ff8fb1' : '#ffd54f'));
    door(g, 155, 76, 14, 24, '#8a5232');
    R(g, 116, 100, 92, 6, '#a87b4f'); for (let px = 118; px < 208; px += 6) R(g, px, 100, 1, 6, '#8a5d34'); R(g, 116, 106, 92, 1, '#6b4a2e');
  }
  function drawWorkshop(g, lit) {
    const { x, y } = HOMES.workshop;
    shadow(g, x, y, 74, 70);
    R(g, x, 56, 74, 44, '#8b6b4a'); for (let sx = x + 3; sx < x + 74; sx += 4) R(g, sx, 56, 1, 44, '#7a5c3e');
    R(g, x, 96, 74, 4, '#5e4630');
    roof(g, x - 3, y, 80, 28, '#56707f', '#3e5260', '#7d98a6');
    R(g, x - 3, 56, 80, 2, '#2f3e48');
    R(g, 39, 68, 36, 32, '#3d2a1b'); R(g, 41, 70, 32, 30, lit ? '#5a3d22' : '#2e2219');
    R(g, 44, 80, 26, 3, '#a0683a'); R(g, 46, 76, 6, 4, '#2a3a4a'); R(g, 47, 77, 4, 2, lit ? '#9fe0ff' : '#5fb0e0'); R(g, 60, 75, 2, 5, '#9aa7ad'); R(g, 64, 77, 4, 3, '#c0392b');
    R(g, 41, 70, 32, 2, '#6b4a2e');
    // gear sign
    disc(g, 57, 62, 4, '#cfd8dc'); disc(g, 57, 62, 1, '#56707f'); [[57, 57], [57, 67], [52, 62], [62, 62]].forEach(([a, b]) => R(g, a - 1, b - 1, 2, 2, '#cfd8dc'));
    windowAt(g, 25, 66, 10, 10, lit); windowAt(g, 79, 66, 10, 10, lit);
    R(g, 86, 100, 8, 6, '#a0683a'); R(g, 86, 100, 8, 1, '#bf8550'); R(g, 88, 97, 4, 3, '#9aa7ad');
  }
  function drawSilo(g) {
    const { x, y, w } = HOMES.silo;
    shadow(g, x, y + 6, w, 76);
    for (let i = 0; i < w; i++) R(g, x + i, y + 14, 1, 68, i < 5 ? '#e7edf0' : i < w - 7 ? '#cfd8dc' : '#a9b6bc');
    for (let by = y + 22; by < y + 80; by += 12) R(g, x, by, w, 1, '#93a1a8');
    for (let i = 0; i < 14; i++) { const half = Math.round(Math.sqrt(1 - ((13 - i) / 14) ** 2) * w / 2); R(g, x + w / 2 - half, y + i, half * 2, 1, i < 3 ? '#d16557' : i % 4 === 3 ? '#8f2f28' : '#b8453a'); }
    R(g, x + 9, y + 26, 10, 50, '#5a3d26'); R(g, x + 10, y + 27, 8, 48, '#3a2a1e');
    R(g, x + 9, y + 78, 10, 4, '#7a4e2d');
  }
  function drawShed(g, lit) {
    const { x, y } = HOMES.shed;
    shadow(g, x, y, 54, 52);
    R(g, x, 70, 54, 30, '#b07a46'); for (let sx = x + 3; sx < x + 54; sx += 4) R(g, sx, 70, 1, 30, '#9a6a3c');
    R(g, x, 97, 54, 3, '#7a4e2d');
    roof(g, x - 3, y, 60, 23, '#5f8d4e', '#466b39', '#7fae6a');
    R(g, x - 3, 70, 60, 2, '#36552c');
    door(g, 269, 80, 13, 20, '#7a4e2d');
    R(g, 253, 75, 15, 14, '#5a3d26'); R(g, 254, 76, 13, 12, '#c49a6c');
    R(g, 286, 90, 7, 10, '#d9c08f'); R(g, 293, 92, 7, 8, '#cdb07a'); R(g, 287, 89, 5, 2, '#b89a60'); R(g, 294, 91, 5, 2, '#a88a52');
    R(g, 288, 94, 3, 2, '#7bc043'); R(g, 295, 95, 3, 2, '#e9c46a');
    if (lit) R(g, 271, 84, 3, 3, '#ffd77a');
  }
  function drawLibrary(g, lit) {
    const { x, y } = HOMES.library;
    shadow(g, x, y, 76, 74);
    R(g, x, 54, 76, 46, '#d8cbb8');
    for (let sy = 57; sy < 100; sy += 5) { R(g, x, sy, 76, 1, '#c2b29a'); for (let sx = x + ((sy / 5) % 2 ? 4 : 0); sx < x + 76; sx += 8) R(g, sx, sy, 1, 5, '#c2b29a'); }
    roof(g, x - 3, y, 82, 30, '#6d4c7d', '#4f3560', '#8f6aa0');
    R(g, x - 3, 54, 82, 3, '#efe7d8'); R(g, x - 3, 56, 82, 1, '#b8a88e');
    [x + 3, x + 15, x + 57, x + 69].forEach(cx => { R(g, cx, 57, 4, 41, '#f3eee4'); R(g, cx + 3, 57, 1, 41, '#cfc4b0'); R(g, cx - 1, 96, 6, 2, '#efe7d8'); });
    windowAt(g, 340, 64, 9, 16, lit); windowAt(g, 362, 64, 9, 16, lit);
    door(g, 350, 76, 12, 24, '#5b3b6b');
    R(g, 344, 100, 24, 2, '#c9bba4'); R(g, 346, 102, 20, 1, '#b8a88e');
    disc(g, 356, 40, 5, '#efe7d8'); disc(g, 356, 40, 3, lit ? '#ffd77a' : '#8fc3e0'); R(g, 356, 37, 1, 7, '#5a3d26'); R(g, 353, 40, 7, 1, '#5a3d26');
  }
  function drawTower(g, lit) {
    const x = HOMES.tower.x;
    R(g, x + 6, 104, 44, 4, 'rgba(25,45,15,.28)');
    R(g, x + 12, 44, 3, 58, '#5e3d22'); R(g, x + 34, 44, 3, 58, '#5e3d22');
    for (let i = 0; i < 14; i++) { R(g, x + 9 + i * 2, 52 + i * 3, 2, 2, '#6b4a2e'); R(g, x + 37 - i * 2, 52 + i * 3, 2, 2, '#6b4a2e'); }
    R(g, x + 5, 44, 3, 58, '#8a5d34'); R(g, x + 41, 44, 3, 58, '#8a5d34'); R(g, x + 5, 44, 1, 58, '#a87b4f');
    R(g, x + 21, 50, 7, 52, '#8a5d34'); for (let ly = 52; ly < 100; ly += 5) R(g, x + 21, ly, 7, 1, '#5e3d22');
    R(g, x, 40, 48, 6, '#8a5d34'); R(g, x, 40, 48, 1, '#a87b4f'); R(g, x, 45, 48, 1, '#5e3d22');
    R(g, x + 4, 24, 40, 16, '#c58b4f'); for (let sx = x + 6; sx < x + 44; sx += 4) R(g, sx, 24, 1, 16, '#a8743f');
    R(g, x + 9, 27, 30, 9, '#3a2a1e'); R(g, x + 10, 28, 28, 7, lit ? '#ffc964' : '#2e2219');
    R(g, x + 9, 31, 30, 1, '#8a5d34');
    roof(g, x - 2, 10, 52, 15, '#d9822b', '#a85f1a', '#f0a050');
    R(g, x + 23, 0, 1, 11, '#5e3d22'); R(g, x + 24, 0, 8, 5, '#e85d3a'); R(g, x + 24, 4, 6, 1, '#b84528');
  }
  function drawPostOffice(g, lit) {
    const { x, y } = HOMES.postoffice;
    R(g, x + 3, y + 34, 40, 3, 'rgba(25,45,15,.28)');
    R(g, x + 2, y + 12, 38, 22, '#b5543c'); for (let by = y + 14; by < y + 34; by += 4) for (let bx = x + 3 + ((by / 4) % 2) * 3; bx < x + 39; bx += 6) R(g, bx, by, 5, 1, '#9c4532');
    roof(g, x - 1, y, 44, 13, '#3f51b5', '#303f9f', '#5c6bc0');
    R(g, x + 8, y + 14, 26, 5, '#fffdf5'); R(g, x + 10, y + 16, 2, 1, '#3f51b5'); R(g, x + 13, y + 16, 2, 1, '#3f51b5'); R(g, x + 16, y + 16, 2, 1, '#3f51b5'); R(g, x + 19, y + 16, 2, 1, '#3f51b5'); R(g, x + 25, y + 15, 7, 3, '#e8eaf6'); R(g, x + 25, y + 15, 7, 1, '#9fa8da');
    R(g, x + 17, y + 22, 9, 12, '#5a3d26'); R(g, x + 18, y + 23, 7, 11, lit ? '#ffc964' : '#6b4a2e'); R(g, x + 23, y + 28, 1, 1, '#ffd166');
    R(g, x + 5, y + 22, 8, 6, '#2b1d16'); R(g, x + 6, y + 23, 6, 4, lit ? '#ffc964' : '#9fd3ef'); R(g, x + 29, y + 22, 8, 6, '#2b1d16'); R(g, x + 30, y + 23, 6, 4, lit ? '#ffc964' : '#9fd3ef');
    R(g, x + 40, y + 24, 4, 10, '#3f51b5'); R(g, x + 39, y + 23, 6, 2, '#303f9f'); R(g, x + 41, y + 27, 2, 1, '#fffdf5');
  }
  function drawMarket(g) {
    const { x, y } = HOMES.market;
    R(g, x + 4, y + 50, 76, 4, 'rgba(25,45,15,.28)');
    R(g, x + 4, y + 8, 3, 40, '#6b4a2e'); R(g, x + 71, y + 8, 3, 40, '#6b4a2e');
    R(g, x + 2, y + 28, 74, 22, '#a0683a'); for (let sx = x + 4; sx < x + 76; sx += 6) R(g, sx, y + 30, 1, 20, '#8a5d34'); R(g, x + 2, y + 28, 74, 2, '#bf8550');
    // produce crates
    const crate = (cx, c1, c2) => { R(g, cx, y + 22, 16, 7, '#7a4e2d'); for (let i = 0; i < 5; i++) { R(g, cx + 1 + i * 3, y + 20, 3, 3, i % 2 ? c1 : c2); R(g, cx + 1 + i * 3, y + 20, 1, 1, '#ffffff66'); } };
    crate(x + 8, '#e74c3c', '#c0392b'); crate(x + 30, '#f39c12', '#e67e22'); crate(x + 52, '#7bc043', '#5e9a2f');
    for (let i = 0; i < 13; i++) R(g, x + i * 6, y, 6, 15, i % 2 ? '#fff3f6' : '#f06292');
    R(g, x, y, 78, 1, '#ffd1e0');
    for (let i = 0; i < 13; i++) { R(g, x + i * 6 + 1, y + 15, 4, 1, i % 2 ? '#fff3f6' : '#f06292'); R(g, x + i * 6 + 2, y + 16, 2, 1, i % 2 ? '#fff3f6' : '#f06292'); }
    R(g, x, y + 15, 78, 0, '#000');
  }

  // ---------- characters ----------
  const HAIR = ['#4a2f1d', '#2b2b2b', '#c98b3c', '#8a3b2a', '#e0c070', '#5a4636', '#d9d9d9'];
  const SKIN = ['#f2c9a0', '#d9a173', '#a86f4a', '#f5d6b8', '#8d5a3b'];
  const HEAD = ['..oooooo..', '.ohhhhhho.', '.ohhhhhho.', '.ohssssho.', '.osesseso.', '.osssssso.', '..oooooo..'];
  const BODY = ['.occcccco.', 'occcccccco', 'ocdccccdco', 'okccccccko', '.oddddddo.'];
  const LEGS = {
    stand: ['.oppooppo.', '.oppooppo.', '.obboobbo.', '..oo..oo..'],
    walkA: ['.oppooppo.', '.obbooppo.', '..oo.obbo.', '......oo..'],
    walkB: ['.oppooppo.', '.oppoobbo.', '.obbo.oo..', '..oo......'],
    sit: ['.oppppppo.', '.obboobbo.', '..oo..oo..'],
  };
  const look = a => a.role === 'foreman' || a.home === 'farmhouse' ? 'foreman' : a.role === 'inspector' || a.home === 'tower' ? 'inspector' : a.home === 'market' ? 'market' : a.home === 'library' ? 'library' : 'workshop';
  const spriteCache = new Map();
  function sprite(a, pose, facing) {
    const color = safeColor(a.color);
    const key = `${a.id}|${color}|${look(a)}|${pose}|${facing}`;
    let c = spriteCache.get(key);
    if (c) return c;
    c = document.createElement('canvas'); c.width = 16; c.height = 21;
    const g = c.getContext('2d');
    const h = hash(a.id);
    const skin = SKIN[(h >>> 3) % SKIN.length];
    const pal = { o: '#2b1d16', e: '#2b1d16', h: HAIR[h % HAIR.length], s: skin, k: skin, c: color, d: shade(color, -0.3), p: '#3d4f73', b: '#5a3a26' };
    const px = (x, y, col) => { g.fillStyle = col; g.fillRect(3 + (facing < 0 ? 9 - x : x), 3 + y, 1, 1); };
    [...HEAD, ...BODY, ...LEGS[pose]].forEach((row, y) => { for (let x = 0; x < 10; x++) if (row[x] !== '.') px(x, y, pal[row[x]]); });
    const kind = look(a);
    if (kind === 'foreman') {
      for (let x = 2; x <= 7; x++) { px(x, -2, '#e9c46a'); px(x, -1, '#e9c46a'); px(x, 0, '#c0392b'); }
      for (let x = -1; x <= 10; x++) px(x, 1, '#d4a743');
    } else if (kind === 'inspector') {
      for (let x = 2; x <= 7; x++) { px(x, -1, '#7a5230'); px(x, 0, '#7a5230'); }
      for (let x = 0; x <= 9; x++) px(x, 1, '#5e3d22');
      px(3, 8, '#ffd54f');
    } else if (kind === 'workshop') {
      const cap = shade(color, -0.45);
      for (let x = 1; x <= 8; x++) { px(x, 0, cap); px(x, 1, cap); }
      for (let x = 8; x <= 10; x++) px(x, 1, shade(color, -0.6));
      px(3, 0, shade(color, 0.2));
    } else if (kind === 'market') {
      px(7, 0, '#ffd1e0'); px(6, 1, '#ffd1e0'); px(8, 1, '#ffd1e0'); px(7, 2, '#ffd1e0'); px(7, 1, '#ffeb3b');
      for (let y = 9; y <= 10; y++) for (let x = 3; x <= 6; x++) px(x, y, '#fff3e0');
    } else if (kind === 'library') {
      [2, 4, 5, 7].forEach(x => px(x, 4, '#3a3a4a')); px(3, 4, '#cfe8ff'); px(6, 4, '#cfe8ff');
      for (let y = 8; y <= 9; y++) px(1, y, '#8e44ad');
    }
    spriteCache.set(key, c);
    return c;
  }
  function drawAvatar(cv, a) {
    const size = cv.clientWidth || 34;
    const d = window.devicePixelRatio || 1;
    cv.width = Math.round(size * d); cv.height = Math.round(size * d);
    const g = cv.getContext('2d');
    g.imageSmoothingEnabled = false;
    g.fillStyle = shade(a.color, -0.55); g.fillRect(0, 0, cv.width, cv.height);
    g.fillStyle = shade(a.color, -0.35); g.fillRect(0, cv.height * 0.62, cv.width, cv.height);
    const s = sprite(a, 'stand', 1);
    const scale = Math.floor(cv.width / 13);
    g.drawImage(s, 0, 0, 16, 15, Math.round((cv.width - 16 * scale) / 2), Math.round(cv.height * 0.1), 16 * scale, 15 * scale);
  }

  function makeChar(a) {
    const home = homeOf(a);
    const pts = LOC[`home:${home}`];
    const [x, y] = pts[pts.length - 1];
    return { id: a.id, x, y, loc: `home:${home}`, route: [], routeKey: null, facing: 1, moving: false, wait: 1 + Math.random() * 3,
      seed: hash(a.id) % 1000, bubble: null, lastLine: undefined, idleGoal: null, placed: false };
  }
  function desiredFor(a, c) {
    const home = homeOf(a);
    const slot = a.taskId ? slots.indexOf(a.taskId) : -1;
    switch (a.status) {
      case 'working': return { key: slot >= 0 ? `plot:${slot}` : 'field', act: 'work' };
      case 'checking': return { key: slot >= 0 ? `check:${slot}` : 'field', act: 'inspect' };
      case 'planning': return { key: 'desk', act: 'plan' };
      case 'writing': return { key: `home:${home}`, act: 'plan' };
      case 'asleep': return { key: `home:${home}`, act: 'sleep' };
      case 'chatting': return { key: /^(home|bench|desk)/.test(c.loc) ? c.loc : `home:${home}`, act: 'chat' };
      default:
        if (c.idleGoal && c.idleGoal.until > clock) return { key: c.idleGoal.key, act: 'sit' };
        return { key: `home:${home}`, act: 'idle' };
    }
  }
  function routeTo(c, key) {
    const from = LOC[c.loc] || LOC['home:workshop'], to = LOC[key];
    let common = 0;
    while (common < from.length - 1 && common < to.length - 1 && from[common][0] === to[common][0] && from[common][1] === to[common][1]) common++;
    const pts = [];
    for (let i = from.length - 1; i >= Math.max(0, common - 1); i--) pts.push(from[i]);
    for (let i = Math.max(0, common - 1); i < to.length; i++) pts.push(to[i]);
    const out = [];
    for (const p of pts) { const l = out[out.length - 1]; if (!l || l[0] !== p[0] || l[1] !== p[1]) out.push([p[0], p[1]]); }
    if (key.startsWith('home:') && out.length) out[out.length - 1][0] += c.off || 0;
    c.route = out; c.routeKey = key;
  }
  function updateChars(dt) {
    const alive = new Set();
    const perHome = {};
    for (const a of model.agents) (perHome[homeOf(a)] = perHome[homeOf(a)] || []).push(a.id);
    for (const a of model.agents) {
      alive.add(a.id);
      let c = chars.get(a.id);
      if (!c) { c = makeChar(a); chars.set(a.id, c); }
      c.agent = a;
      { const list = perHome[homeOf(a)]; c.off = Math.round((list.indexOf(a.id) - (list.length - 1) / 2) * 26); }
      if (a.lastLine !== c.lastLine) {
        if (a.lastLine && c.lastLine !== undefined || (a.lastLine && c.lastLine === undefined)) c.bubble = { text: String(a.lastLine), until: clock + 7 };
        c.lastLine = a.lastLine;
      }
      const want = desiredFor(a, c);
      c.act = want.act;
      if (!c.placed || rm.matches) {
        // first sight (or reduced motion): stand at the right place right away
        if (c.loc !== want.key || !c.placed) {
          const p = LOC[want.key]; c.loc = want.key; c.route = []; c.x = p[p.length - 1][0] + (want.key.startsWith('home:') ? c.off : 0); c.y = p[p.length - 1][1];
        }
        c.placed = true;
      }
      if (c.route.length) {
        const [tx, ty] = c.route[0];
        const dx = tx - c.x, dy = ty - c.y, d = Math.hypot(dx, dy);
        const step = SPEED * dt * (a.status === 'idle' ? 0.8 : 1);
        if (Math.abs(dx) > 0.5) c.facing = dx > 0 ? 1 : -1;
        if (d <= step) { c.x = tx; c.y = ty; c.route.shift(); if (!c.route.length && c.routeKey) { c.loc = c.routeKey; c.routeKey = null; } }
        else { c.x += dx / d * step; c.y += dy / d * step; }
        c.moving = true;
        continue;
      }
      c.moving = false;
      if (c.loc !== want.key) { routeTo(c, want.key); continue; }
      // idle life at home: wander a little, sometimes sit on the bench
      if (want.act === 'idle') {
        c.wait -= dt;
        if (c.wait <= 0) {
          c.wait = 3 + Math.random() * 5;
          const bench = benchOf(homeOf(a));
          const benchTaken = [...chars.values()].some(o => o !== c && (o.loc === bench || o.routeKey === bench));
          if (!benchTaken && Math.random() < 0.18) { c.idleGoal = { key: bench, until: clock + 12 + Math.random() * 12 }; continue; }
          const p = LOC[c.loc]; const [hx, hy] = p[p.length - 1];
          const nx = hx + c.off + (Math.random() * 16 - 8), ny = hy + (Math.random() * 5 - 2);
          c.route = [[nx, ny]]; c.routeKey = null;
        }
      }
      if (want.act === 'work' || want.act === 'inspect') c.facing = want.act === 'inspect' ? -1 : 1;
    }
    for (const id of [...chars.keys()]) if (!alive.has(id)) chars.delete(id);
  }

  // ---------- ambient life ----------
  const chickens = [0, 1, 2, 3].map(i => ({ x: 60 + i * 12, y: 214 + (i % 2) * 14, tx: 60, ty: 220, wait: i, peck: 0, f: 1 }));
  const flies = [0, 1, 2].map(i => ({ x: [180, 300, 90][i], y: [250, 255, 125][i], p: i * 2.1, c: ['#fff59d', '#f8bbd0', '#b3e5fc'][i] }));
  const puffs = [];
  function updateAmbient(dt) {
    for (const ch of chickens) {
      if (ch.peck > 0) { ch.peck -= dt; continue; }
      const dx = ch.tx - ch.x, dy = ch.ty - ch.y, d = Math.hypot(dx, dy);
      if (d < 0.6) {
        ch.wait -= dt;
        if (ch.wait <= 0) { ch.wait = 1 + Math.random() * 3; if (Math.random() < 0.5) ch.peck = 0.8; else { ch.tx = 62 + Math.random() * 60; ch.ty = 208 + Math.random() * 34; } }
      } else { ch.x += dx / d * 12 * dt; ch.y += dy / d * 12 * dt; ch.f = dx > 0 ? 1 : -1; }
    }
    if (Math.random() < dt * 1.2) puffs.push({ x: 182.5, y: 10, t: 0 });
    for (const p of puffs) { p.t += dt; p.y -= dt * 5; p.x += dt * 2.5; }
    while (puffs.length && puffs[0].t > 3.5) puffs.shift();
  }
  function drawChicken(g, ch) {
    const x = Math.round(ch.x), y = Math.round(ch.y), f = ch.f;
    R(g, x - 2, y, 6, 1, 'rgba(25,45,15,.3)');
    R(g, x - 2, y - 4, 5, 3, '#fafafa'); R(g, x - 2 - (f > 0 ? 1 : -5), y - 4, 1, 2, '#e0e0e0');
    const hx = f > 0 ? x + 2 : x - 3, hy = ch.peck > 0 ? y - 3 : y - 6;
    R(g, hx, hy, 2, 2, '#fafafa'); R(g, hx + (f > 0 ? 0 : 1), hy - 1, 1, 1, '#e53935'); R(g, hx + (f > 0 ? 2 : -1), hy + 1, 1, 1, '#ffb300');
    R(g, x - 1, y - 1, 1, 1, '#ff9800'); R(g, x + 1, y - 1, 1, 1, '#ff9800');
  }

  // ---------- crops & dynamic props ----------
  const FRUIT = { code: '#f4d03f', marketing: '#ff8fb1', research: '#f39c12', 'hardware-check': '#e0c070' };
  const stageOf = n => n >= 20 ? 4 : n >= 12 ? 3 : n >= 6 ? 2 : n >= 2 ? 1 : 0;
  function crop(g, x, y, stage, fruit, wob) {
    if (stage === 0) { R(g, x, y, 1, 1, '#3d2a1b'); R(g, x + 2, y + 1, 1, 1, '#3d2a1b'); return; }
    const leaf = '#7bc043', dark = '#4f8a2c';
    if (stage === 1) { R(g, x, y - 1, 1, 2, dark); R(g, x - 1, y - 2, 1, 1, leaf); R(g, x + 1, y - 2, 1, 1, leaf); return; }
    const h = stage === 2 ? 5 : 8;
    R(g, x, y - h + 1, 1, h, dark);
    for (let i = 1; i < h; i += 2) { R(g, x - 2 + (wob && i % 4 === 1 ? 1 : 0), y - i, 2, 1, leaf); R(g, x + 1, y - i - 1, 2, 1, leaf); }
    if (stage === 4) { R(g, x - 1, y - h - 1, 3, 3, fruit); R(g, x - 1, y - h - 1, 1, 1, '#ffffffaa'); R(g, x + 2, y - 4, 2, 2, fruit); }
  }
  function drawDynamic(g) {
    // silo grain level = weekly usage, red line = where the farm stops
    const s = HOMES.silo, u = model.usage || {};
    const top = s.y + 27, hh = 48;
    if (u.weekly != null) { const fh = Math.round(Math.min(100, u.weekly) / 100 * hh); R(g, s.x + 10, top + hh - fh, 8, fh, '#e9c46a'); R(g, s.x + 10, top + hh - fh, 8, 1, '#f6dc8a'); }
    if (u.line != null) R(g, s.x + 8, top + hh - Math.round(u.line / 100 * hh), 12, 1, '#e53935');
    // notes on the seed shed board = ready tasks
    for (let i = 0; i < Math.min(6, model.readyCount || 0); i++) { const nx = 255 + (i % 3) * 4, ny = 77 + (i / 3 | 0) * 5; R(g, nx, ny, 3, 4, ['#fffde7', '#fff59d', '#c8e6c9'][i % 3]); R(g, nx + 1, ny, 1, 1, '#e53935'); }
    // mailbox: flag up when something waits
    R(g, 204, 96, 9, 7, '#2f5d9e'); R(g, 204, 96, 9, 2, '#4a86d4'); R(g, 205, 99, 1, 3, '#1d3f70');
    if (model.mail > 0) { R(g, 213, 91, 1, 8, '#7a4e2d'); R(g, 214, 91, 4, 3, '#e53935'); }
    else { R(g, 213, 99, 5, 1, '#e53935'); }
    // crops in the plots
    const wob = !rm.matches && Math.floor(clock * 2) % 2 === 0;
    PLOTS.forEach(([px, py], i) => {
      const id = slots[i]; if (!id) return;
      const p = model.plots.find(q => q.id === id); if (!p) return;
      const st = p.phase === 'checking' ? 4 : stageOf(p.toolCalls || 0);
      const fruit = FRUIT[p.kind] || FRUIT.code;
      for (let r = 0; r < 2; r++) for (let j = 0; j < 5; j++) crop(g, px + 8 + j * 10, py + 12 + r * 13, Math.max(0, st - ((j + r) % 3 === 2 ? 1 : 0)), fruit, wob && j % 2);
      R(g, px + 1, py - 5, 1, 7, '#6b4a2e');
    });
    // harvest baskets
    (model.harvest || []).slice(0, 6).forEach((t, i) => {
      const bx0 = BASKETS.x + 3 + (i % 3) * 14, by0 = BASKETS.y + 10 + (i / 3 | 0) * 12;
      const fr = FRUIT[t.kind] || FRUIT.code;
      R(g, bx0 + 1, by0 + 8, 11, 1, 'rgba(25,45,15,.3)');
      for (let j = 0; j < 4; j++) R(g, bx0 + 2 + j * 2 + (j % 2), by0 - 1 + (j % 2), 3, 3, fr);
      R(g, bx0 + 3, by0 - 1, 1, 1, '#ffffffaa');
      R(g, bx0, by0 + 2, 12, 6, '#b07a46'); R(g, bx0, by0 + 2, 12, 1, '#d29a5c');
      for (let j = 1; j < 12; j += 3) R(g, bx0 + j, by0 + 3, 1, 5, '#8a5d34');
    });
    // pond sparkles and a duck
    if (!rm.matches) {
      const t = Math.floor(clock * 2);
      for (let i = 0; i < 4; i++) { const r = rng(t * 7 + i)(); R(g, 400 + (r * 48 | 0), 192 + ((r * 977) % 1 * 26 | 0), 2, 1, '#d7f0ff'); }
    }
    const da = rm.matches ? 1 : clock * 0.15;
    const dxp = Math.round(424 + Math.cos(da) * 22), dyp = Math.round(207 + Math.sin(da) * 12), df = -Math.sin(da) > 0 ? 1 : -1;
    R(g, dxp - 3, dyp, 7, 1, '#3a7fae'); R(g, dxp - 3, dyp - 3, 6, 3, '#fafafa'); R(g, dxp + (df > 0 ? 2 : -3), dyp - 5, 2, 2, '#2e7d32'); R(g, dxp + (df > 0 ? 4 : -4), dyp - 4, 1, 1, '#ffb300');
    // chimney smoke
    for (const p of puffs) { const a = Math.max(0, 0.6 - p.t / 6); g.fillStyle = `rgba(235,235,235,${a})`; const r = 1 + p.t * 0.7 | 0; g.fillRect(Math.round(p.x - r), Math.round(p.y - r), r * 2 + 1, r * 2 + 1); }
  }
  function drawDesk(g) {
    R(g, 182, 109, 17, 1, 'rgba(25,45,15,.3)');
    R(g, 182, 101, 17, 4, '#8a5d34'); R(g, 182, 101, 17, 1, '#a87b4f'); R(g, 182, 105, 17, 3, '#6b4a2e');
    R(g, 183, 108, 2, 1, '#4a3526'); R(g, 196, 108, 2, 1, '#4a3526');
    R(g, 185, 100, 5, 2, '#fafafa'); R(g, 192, 99, 2, 2, '#c0392b');
  }
  function drawCharacter(g, c) {
    const a = c.agent;
    const sit = c.act === 'sit' || c.act === 'sleep';
    const pose = sit && !c.moving ? 'sit' : c.moving ? (Math.floor(clock * 7 + c.seed) % 2 ? 'walkA' : 'walkB') : 'stand';
    const fx = Math.round(c.x), fy = Math.round(c.y);
    const bob = !rm.matches && !c.moving && c.act !== 'sleep' && Math.floor(clock * 1.5 + c.seed) % 2 === 0 ? 0 : 0;
    const dy = pose === 'sit' ? 2 : (c.moving && pose === 'walkA' ? -1 : 0) + bob;
    oval(g, fx, fy + 1, 5, 1, 'rgba(20,35,10,.35)');
    if (selected === a.id) {
      const on = rm.matches || Math.floor(clock * 3) % 2;
      g.fillStyle = on ? '#ffe066' : '#fff6c0';
      for (let i = -7; i <= 7; i++) { g.fillRect(fx + i, fy + 3, 1, 1); }
      g.fillRect(fx - 8, fy + 2, 1, 1); g.fillRect(fx + 8, fy + 2, 1, 1); g.fillRect(fx - 7, fy - 1, 1, 3); g.fillRect(fx + 7, fy - 1, 1, 3);
    }
    const X0 = fx - 5, Y0 = fy - 15 + dy;
    g.drawImage(sprite(a, pose, c.facing), X0 - 3, Y0 - 3);
    const mx = x => c.facing < 0 ? X0 + 9 - x : X0 + x;
    const P = (x, y, col) => R(g, mx(x), Y0 + y, 1, 1, col);
    if (c.moving) return;
    const phase = rm.matches ? 0 : Math.floor(clock * 3 + c.seed) % 2;
    if (c.act === 'work') {
      const coder = look(a) === 'workshop';
      if (coder) {
        if (!phase) { P(9, 9, '#8a5d34'); P(10, 8, '#8a5d34'); P(10, 7, '#8a5d34'); P(9, 6, '#9aa7ad'); P(10, 6, '#9aa7ad'); P(11, 6, '#9aa7ad'); P(11, 5, '#9aa7ad'); }
        else { P(10, 10, '#8a5d34'); P(11, 10, '#8a5d34'); P(12, 10, '#8a5d34'); P(13, 9, '#9aa7ad'); P(13, 10, '#9aa7ad'); P(13, 11, '#9aa7ad'); P(14, 12, '#ffd54f'); P(15, 10, '#ffd54f'); }
      } else {
        if (!phase) { [[9, 9], [10, 8], [11, 7], [12, 6], [13, 5]].forEach(([x, y]) => P(x, y, '#8a5d34')); P(13, 4, '#9aa7ad'); P(14, 4, '#9aa7ad'); P(14, 5, '#9aa7ad'); }
        else { [[9, 10], [10, 11], [11, 12], [12, 13], [13, 14]].forEach(([x, y]) => P(x, y, '#8a5d34')); P(14, 14, '#9aa7ad'); P(14, 15, '#9aa7ad'); P(13, 15, '#9aa7ad'); P(16, 14, '#5e3d27'); P(15, 12, '#5e3d27'); }
      }
    } else if (c.act === 'inspect') {
      for (let y = 8; y <= 11; y++) for (let x = 9; x <= 11; x++) P(x, y, '#8a5d34');
      P(10, 9, '#fafafa'); P(11, 9, '#fafafa'); P(10, 10, '#fafafa'); P(11, 10, '#fafafa');
      if (phase) P(10, 7, '#2b1d16');
    } else if (c.act === 'plan') {
      const by = Y0 - 9;
      R(g, X0 + 3, by, 4, 4, '#ffe066'); R(g, X0 + 2, by + 1, 6, 2, '#ffe066'); R(g, X0 + 3, by, 1, 1, '#fffbe0');
      R(g, X0 + 4, by + 4, 2, 2, '#9aa7ad');
    }
  }

  // ---------- frame ----------
  // A timer loop (not requestAnimationFrame): steady 30 fps, about 4 fps with reduced motion, nothing while hidden.
  function frame() {
    rafId = 0;
    if (document.hidden) return;
    const ts = performance.now();
    const dt = Math.min(0.25, (ts - last) / 1000 || 0);
    last = ts;
    clock += dt;
    if (!rm.matches) updateAmbient(dt);
    updateChars(rm.matches ? 1 : dt);
    if (ts - lastBgCheck > 30000 || !bgKey) { lastBgCheck = ts; checkBg(); }
    render();
    schedule();
  }
  function schedule() { if (!rafId && !document.hidden && canvas) rafId = setTimeout(frame, rm.matches ? 250 : 33); }
  function checkBg() {
    const lit = darkness(hourNow()) > 0.12;
    const key = `${LW}x${LH}|${ox},${oy}|${lit}`;
    if (key !== bgKey) { bgKey = key; buildBg(lit); }
  }

  function render() {
    const g = lx;
    g.setTransform(1, 0, 0, 1, 0, 0);
    g.globalCompositeOperation = 'source-over';
    g.drawImage(bg, 0, 0);
    g.setTransform(1, 0, 0, 1, ox, oy);
    drawDynamic(g);
    const list = [];
    for (const c of chars.values()) list.push({ y: c.y, d: () => drawCharacter(g, c) });
    list.push({ y: 108, d: () => drawDesk(g) });
    for (const ch of chickens) list.push({ y: ch.y, d: () => drawChicken(g, ch) });
    list.sort((a, b) => a.y - b.y).forEach(o => o.d());
    if (!rm.matches) for (const f of flies) {
      const t = clock + f.p, x = Math.round(f.x + Math.sin(t * 0.7) * 26), y = Math.round(f.y + Math.sin(t * 1.9) * 7 - 4);
      if (Math.floor(t * 8) % 2) { R(g, x - 1, y, 1, 1, f.c); R(g, x + 1, y, 1, 1, f.c); } else { R(g, x - 1, y - 1, 1, 2, f.c); R(g, x + 1, y - 1, 1, 2, f.c); }
      R(g, x, y, 1, 1, '#3a2a1e');
    }
    // hover outline
    const hr = hoverRect();
    if (hr) { g.fillStyle = 'rgba(255,240,170,.95)'; g.fillRect(hr.x - 1, hr.y - 1, hr.w + 2, 1); g.fillRect(hr.x - 1, hr.y + hr.h, hr.w + 2, 1); g.fillRect(hr.x - 1, hr.y, 1, hr.h); g.fillRect(hr.x + hr.w, hr.y, 1, hr.h); }
    // light: time of day, then lamps
    g.setTransform(1, 0, 0, 1, 0, 0);
    const h = hourNow(), skyc = sky(h), dk = darkness(h);
    if (dk > 0.01) { g.globalCompositeOperation = 'multiply'; g.fillStyle = `rgb(${skyc.join(',')})`; g.fillRect(0, 0, LW, LH); }
    if (model.asleep) { g.globalCompositeOperation = 'multiply'; g.fillStyle = '#9696a8'; g.fillRect(0, 0, LW, LH); }
    if (dk > 0.12) {
      g.globalCompositeOperation = 'lighter';
      const glow = (x, y, r, a) => { const gr = g.createRadialGradient(ox + x, oy + y, 0, ox + x, oy + y, r); gr.addColorStop(0, `rgba(255,190,90,${a})`); gr.addColorStop(1, 'rgba(255,190,90,0)'); g.fillStyle = gr; g.fillRect(ox + x - r, oy + y - r, r * 2, r * 2); };
      const a = Math.min(0.5, dk * 1.1);
      [112, 244, 312, 404, 6].forEach(x => glow(x + 1, 99, 18, a));
      [[136, 73], [188, 73], [57, 85], [345, 72], [367, 72], [436, 31], [275, 86]].forEach(([x, y]) => glow(x, y, 12, a * 0.7));
    }
    g.globalCompositeOperation = 'source-over';
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(lo, 0, 0, LW * k, LH * k);
    drawText();
  }

  // ---------- crisp text layer ----------
  const D = (x, y) => [(ox + x) * k, (oy + y) * k];
  const font = (px, w = 600) => `${w} ${Math.round(px * dpr)}px "Segoe UI", system-ui, sans-serif`;
  function pill(x, y, text, opts = {}) {
    ctx.font = opts.font || font(11);
    const pad = 5 * dpr, w = ctx.measureText(text).width + pad * 2 + (opts.dot ? 9 * dpr : 0), h = (opts.h || 16) * dpr;
    const left = x - w / 2, top = y - h;
    ctx.fillStyle = opts.bg || 'rgba(28,18,10,.82)';
    ctx.beginPath(); ctx.roundRect(left, top, w, h, 3 * dpr); ctx.fill();
    if (opts.border) { ctx.strokeStyle = opts.border; ctx.lineWidth = dpr; ctx.stroke(); }
    if (opts.dot) { ctx.fillStyle = opts.dot; ctx.fillRect(left + pad, top + h / 2 - 3 * dpr, 6 * dpr, 6 * dpr); }
    ctx.fillStyle = opts.color || '#fff6e0'; ctx.textBaseline = 'middle'; ctx.textAlign = 'left';
    ctx.fillText(text, left + pad + (opts.dot ? 9 * dpr : 0), top + h / 2 + 0.5 * dpr);
    return { left, top, w, h };
  }
  function wrap(text, maxW, lines) {
    const words = String(text).replace(/\s+/g, ' ').trim().split(' ');
    const out = []; let cur = '';
    for (const w of words) {
      const t = cur ? cur + ' ' + w : w;
      if (ctx.measureText(t).width > maxW && cur) { out.push(cur); cur = w; if (out.length === lines) break; } else cur = t;
    }
    if (out.length < lines && cur) out.push(cur);
    const used = out.join(' ').length;
    if (used < String(text).replace(/\s+/g, ' ').trim().length) {
      let l = out[out.length - 1] || '';
      while (l && ctx.measureText(l + '…').width > maxW) l = l.slice(0, -1);
      out[out.length - 1] = l + '…';
    }
    return out;
  }
  let placed = [];   // bubbles drawn this frame, so the next one can move up instead of covering it
  function bubble(x, y, text, opts = {}) {
    ctx.font = font(12, 500);
    const lines = wrap(text, 160 * dpr, 2);
    const lh = 15 * dpr, pad = 6 * dpr;
    const w = Math.max(...lines.map(l => ctx.measureText(l).width)) + pad * 2, h = lines.length * lh + pad * 1.4;
    const left = Math.max(4, Math.min(bw - w - 4, x - w / 2));
    let top = y - h - 6 * dpr;
    for (let tries = 0; tries < 5; tries++) {
      const hit = placed.find(r => left < r.left + r.w + 3 * dpr && left + w + 3 * dpr > r.left && top < r.top + r.h + 3 * dpr && top + h + 3 * dpr > r.top);
      if (!hit) break;
      top = hit.top - h - 5 * dpr;
    }
    placed.push({ left, top, w, h });
    ctx.fillStyle = opts.bg || '#fffdf5'; ctx.strokeStyle = '#3a2a1e'; ctx.lineWidth = 1.5 * dpr;
    const tailTop = top + h, tailEnd = y - 1 * dpr;
    ctx.beginPath(); ctx.moveTo(x - 4 * dpr, tailTop - 1); ctx.lineTo(x, tailEnd); ctx.lineTo(x + 4 * dpr, tailTop - 1); ctx.closePath(); ctx.fill(); ctx.stroke();
    ctx.beginPath(); ctx.roundRect(left, top, w, h, 5 * dpr); ctx.fill(); ctx.stroke();
    ctx.fillStyle = '#2b1d16'; ctx.textBaseline = 'top'; ctx.textAlign = 'left';
    lines.forEach((l, i) => ctx.fillText(l, left + pad, top + pad * 0.8 + i * lh));
  }
  function drawText() {
    placed = [];
    // building plaques
    const plaque = (x, y, t) => { const [X, Y] = D(x, y); pill(X, Y, t, { font: font(10, 700), bg: 'rgba(58,38,22,.78)', color: '#f7e7c4', h: 15 }); };
    plaque(162, 64, 'FARMHOUSE'); plaque(57, 64, 'WORKSHOP'); plaque(356, 64, 'LIBRARY'); plaque(277, 79, 'SEED SHED');
    plaque(436, 56, 'WATCHTOWER'); plaque(65, 160, 'MARKET'); plaque(226, 30, 'SILO');
    if ((model.harvest || []).length) plaque(359, 146, 'HARVEST');
    // plot signs
    PLOTS.forEach(([px, py], i) => {
      const id = slots[i]; if (!id) return;
      const [X, Y] = D(px + 2, py - 3);
      ctx.font = font(10, 700);
      const w = ctx.measureText(id).width + 10 * dpr, h = 14 * dpr;
      ctx.fillStyle = '#d9b27a'; ctx.strokeStyle = '#5a3d26'; ctx.lineWidth = 1.5 * dpr;
      ctx.beginPath(); ctx.roundRect(X - 2 * dpr, Y - h, w, h, 2 * dpr); ctx.fill(); ctx.stroke();
      ctx.fillStyle = '#3a2614'; ctx.textBaseline = 'middle'; ctx.textAlign = 'left'; ctx.fillText(id, X + 3 * dpr, Y - h / 2 + dpr * 0.5);
    });
    // mailbox badge
    if (model.mail > 0) {
      const [X, Y] = D(219, 92);
      ctx.fillStyle = '#e53935'; ctx.beginPath(); ctx.arc(X, Y, 8 * dpr, 0, Math.PI * 2); ctx.fill();
      ctx.strokeStyle = '#fff'; ctx.lineWidth = 1.5 * dpr; ctx.stroke();
      ctx.fillStyle = '#fff'; ctx.font = font(10, 800); ctx.textAlign = 'center'; ctx.textBaseline = 'middle'; ctx.fillText(String(model.mail), X, Y + 0.5 * dpr);
    }
    // characters: name tags, bubbles, Zzz
    const list = [...chars.values()].sort((a, b) => a.y - b.y);
    for (const c of list) {
      const a = c.agent; if (!a) continue;
      const sitting = (c.act === 'sit' || c.act === 'sleep') && !c.moving;
      const [X, Y] = D(c.x, c.y - (sitting ? 15 : 17) - (c.act === 'plan' && !c.moving ? 9 : 0));
      const isSel = selected === a.id;
      const tag = pill(X, Y, String(a.name || a.id), { dot: safeColor(a.color), border: isSel ? '#ffe066' : null, bg: isSel ? 'rgba(60,40,10,.92)' : undefined });
      if (c.act === 'sleep' && !c.moving) {
        const t = rm.matches ? 0 : (clock % 2.4) / 2.4;
        ctx.font = font(12, 800); ctx.fillStyle = `rgba(220,230,255,${1 - t * 0.6})`; ctx.textAlign = 'left'; ctx.textBaseline = 'alphabetic';
        ctx.fillText('z', tag.left + tag.w + 2 * dpr, tag.top + 4 * dpr - t * 8 * dpr);
        ctx.font = font(15, 800); ctx.fillText('Z', tag.left + tag.w + 9 * dpr, tag.top - 4 * dpr - t * 10 * dpr);
      } else if (c.act === 'chat') {
        const n = rm.matches ? 3 : 1 + Math.floor(clock * 2.5) % 3;
        bubble(X, tag.top, '•'.repeat(n).padEnd(3, ' '), {});
      } else if (c.bubble && c.bubble.until > clock) {
        bubble(X, tag.top, c.bubble.text);
      }
    }
  }

  // ---------- hit testing ----------
  function toWorld(clientX, clientY) {
    const r = canvas.getBoundingClientRect();
    return [((clientX - r.left) * dpr) / k - ox, ((clientY - r.top) * dpr) / k - oy];
  }
  const inR = (x, y, r) => x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
  function hitAt(x, y) {
    const cs = [...chars.values()].sort((a, b) => b.y - a.y);
    for (const c of cs) if (x >= c.x - 6 && x <= c.x + 6 && y >= c.y - 19 && y <= c.y + 2) return { type: 'agent', id: c.id, rect: { x: c.x - 6, y: c.y - 19, w: 12, h: 21 } };
    if (inR(x, y, MAILBOX)) return { type: 'mailbox', rect: MAILBOX };
    if (inR(x, y, BASKETS) && (model.harvest || []).length) return { type: 'baskets', rect: BASKETS };
    for (let i = 0; i < PLOTS.length; i++) {
      const [px, py] = PLOTS[i], r = { x: px, y: py, w: PW, h: PH };
      if (inR(x, y, r)) return { type: 'plot', slot: i, taskId: slots[i], rect: r };
    }
    for (const [id, b] of Object.entries(HOMES)) {
      const r = { x: b.x, y: b.y, w: b.w, h: b.h + (id === 'market' ? 0 : 0) };
      if (inR(x, y, r)) return { type: 'building', id, rect: r };
    }
    if (inR(x, y, { x: 386, y: 180, w: 76, h: 56 })) return { type: 'pond', rect: null };
    return null;
  }
  function hoverRect() { return hoverHit && hoverHit.rect && hoverHit.type !== 'agent' ? hoverHit.rect : null; }

  // ---------- size ----------
  function resize() {
    dpr = window.devicePixelRatio || 1;
    const cssW = window.innerWidth, cssH = window.innerHeight;
    bw = Math.round(cssW * dpr); bh = Math.round(cssH * dpr);
    canvas.width = bw; canvas.height = bh;
    const freeW = (cssW - insets.left - insets.right) * dpr, freeH = (cssH - insets.top - insets.bottom) * dpr;
    k = Math.max(1, Math.floor(Math.min(freeW / CW, freeH / (CH - 6))));
    LW = Math.ceil(bw / k); LH = Math.ceil(bh / k);
    lo.width = LW; lo.height = LH;
    const cx = (insets.left + (cssW - insets.left - insets.right) / 2) * dpr / k;
    const cy = (insets.top + (cssH - insets.top - insets.bottom) / 2) * dpr / k;
    ox = Math.round(cx - CW / 2); oy = Math.round(cy - CH / 2 + 2);
    bgKey = '';
    checkBg();
    render();
  }

  // ---------- building interiors (a cross-section card, like rooms in a spaceship) ----------
  function drawRoom(cv, home, agents) {
    const W = 160, H = 60;
    const d = window.devicePixelRatio || 1;
    const cssW = cv.clientWidth || 560;
    const s = Math.max(1, Math.floor(cssW * d / W));
    cv.width = W * s; cv.height = H * s;
    cv.style.height = `${H * s / d}px`;
    const off = document.createElement('canvas'); off.width = W; off.height = H;
    const g = off.getContext('2d');
    const lit = darkness(hourNow()) > 0.12;
    const walls = { farmhouse: ['#ead7ae', '#d4bd8f'], workshop: ['#7a5c3e', '#6b4f35'], library: ['#d8cbb8', '#c2b29a'], market: ['#fff3f6', '#f8d0dc'], tower: ['#c58b4f', '#a8743f'], postoffice: ['#e8eaf6', '#c5cae9'] }[home] || ['#ead7ae', '#d4bd8f'];
    R(g, 0, 0, W, H, '#2b1d16');
    R(g, 2, 2, W - 4, 44, walls[0]);
    for (let x = 2; x < W - 2; x += home === 'market' ? 10 : 6) R(g, x, 2, home === 'market' ? 5 : 1, 44, walls[1]);
    R(g, 2, 2, W - 4, 3, '#5a3d26');
    R(g, 2, 46, W - 4, 12, '#a87b4f'); for (let x = 4; x < W - 2; x += 12) R(g, x, 46, 1, 12, '#8a5d34'); R(g, 2, 46, W - 4, 1, '#6b4a2e');
    const win = (x, y, w, h) => { R(g, x - 1, y - 1, w + 2, h + 2, '#5a3d26'); R(g, x, y, w, h, lit ? '#1f2a52' : '#9fd3ef'); if (lit) { R(g, x + 3, y + 2, 1, 1, '#fff'); R(g, x + w - 4, y + 4, 1, 1, '#fff'); } else { R(g, x, y + h - 3, w, 3, '#7cb35a'); } R(g, x + (w >> 1), y, 1, h, '#5a3d26'); };
    const shelf = (x, y, w) => { R(g, x, y, w, 2, '#6b4a2e'); const r = rng(x * 7 + y); for (let bx2 = x + 1; bx2 < x + w - 2; bx2 += 3) R(g, bx2, y - 7 + (r() * 2 | 0), 2, 7 - (r() * 2 | 0), ['#c0392b', '#2e86c1', '#27ae60', '#8e44ad', '#f39c12', '#16a085'][r() * 6 | 0]); };
    const table = (x, y, w) => { R(g, x, y, w, 3, '#8a5d34'); R(g, x, y, w, 1, '#a87b4f'); R(g, x + 1, y + 3, 2, 46 - y + 6, '#5a3d26'); R(g, x + w - 3, y + 3, 2, 46 - y + 6, '#5a3d26'); };
    const screen = (x, y) => { R(g, x, y, 10, 7, '#2b2b2b'); R(g, x + 1, y + 1, 8, 5, '#4fc3f7'); R(g, x + 2, y + 2, 4, 1, '#e1f5fe'); R(g, x + 2, y + 4, 5, 1, '#b3e5fc'); R(g, x + 4, y + 7, 2, 2, '#2b2b2b'); };
    if (home === 'farmhouse') {
      win(12, 12, 18, 14); shelf(120, 22, 30); shelf(120, 36, 30);
      R(g, 44, 18, 26, 18, '#f5e6c8'); R(g, 45, 19, 24, 16, '#e9d8b0'); R(g, 48, 22, 8, 5, '#7cb35a'); R(g, 58, 25, 6, 6, '#7a5236'); R(g, 50, 29, 4, 3, '#4fa3d1'); R(g, 44, 18, 26, 1, '#c0392b');
      R(g, 84, 26, 22, 20, '#9c5b45'); R(g, 87, 32, 16, 14, '#2b1d16'); R(g, 89, 38, 12, 8, lit ? '#ff9f43' : '#e67e22'); R(g, 92, 36, 6, 4, '#ffd166'); R(g, 82, 24, 26, 3, '#6e3b2c');
      table(38, 40, 40); R(g, 44, 38, 8, 2, '#fafafa'); R(g, 60, 37, 3, 3, '#ffe066');
    } else if (home === 'workshop') {
      win(130, 10, 18, 12);
      R(g, 10, 8, 40, 22, '#a0683a'); for (let i = 0; i < 6; i++) R(g, 14 + i * 6, 12 + (i % 2) * 4, 2, 8, '#9aa7ad'); R(g, 12, 10, 3, 3, '#c0392b');
      table(20, 36, 50); screen(26, 29); screen(48, 29); table(84, 36, 40); screen(98, 29); R(g, 112, 32, 6, 4, '#c0392b'); R(g, 90, 33, 5, 3, '#9aa7ad');
      R(g, 60, 6, 1, 6, '#2b2b2b'); R(g, 57, 12, 7, 3, '#ffd166');
    } else if (home === 'library') {
      for (let y = 14; y <= 40; y += 9) { shelf(6, y, 44); shelf(110, y, 44); }
      win(66, 10, 28, 20); table(62, 38, 36); R(g, 70, 36, 8, 2, '#fafafa'); R(g, 84, 33, 2, 5, '#fffde7'); R(g, 84, 32, 2, 1, '#ffb300');
    } else if (home === 'market') {
      for (let i = 0; i < 16; i++) R(g, 2 + i * 10, 2, 10, 6, i % 2 ? '#fff3f6' : '#f06292');
      R(g, 12, 14, 34, 20, '#2f4f3a'); R(g, 13, 15, 32, 18, '#355e43'); R(g, 16, 18, 20, 1, '#e8f5e9'); R(g, 16, 22, 14, 1, '#e8f5e9'); R(g, 16, 26, 22, 1, '#e8f5e9');
      table(60, 34, 70); [[64, '#e74c3c'], [80, '#f39c12'], [96, '#7bc043'], [112, '#ffd54f']].forEach(([x, c]) => { R(g, x, 28, 14, 6, '#7a4e2d'); for (let i = 0; i < 4; i++) R(g, x + 1 + i * 3, 26, 3, 3, c); });
      win(138, 14, 14, 12);
    } else if (home === 'tower') {
      win(40, 8, 80, 26); R(g, 40, 22, 80, 12, lit ? '#1b2440' : '#7cb35a'); R(g, 60, 26, 30, 6, lit ? '#2a3560' : '#7a5236');
      R(g, 122, 26, 3, 20, '#5a3d26'); R(g, 116, 22, 16, 5, '#9aa7ad'); R(g, 114, 22, 3, 3, '#4fc3f7');
      table(14, 36, 26); R(g, 18, 31, 8, 5, '#8a5d34'); R(g, 19, 32, 6, 3, '#fafafa');
    }
    // residents
    const n = agents.length;
    agents.forEach((a, i) => {
      const fx = Math.round(W * (i + 1) / (n + 1)), fy = 55;
      const sleep = a.status === 'asleep';
      const spr = sprite(a, sleep ? 'sit' : 'stand', i % 2 ? -1 : 1);
      oval(g, fx, fy + 1, 5, 1, 'rgba(0,0,0,.3)');
      g.drawImage(spr, fx - 8, fy - 18 + (sleep ? 2 : 0));
      if (sleep) { R(g, fx + 5, fy - 22, 3, 1, '#dfe7ff'); R(g, fx + 7, fy - 21, 1, 1, '#dfe7ff'); R(g, fx + 5, fy - 20, 3, 1, '#dfe7ff'); }
      if (a.status === 'writing') { R(g, fx - 5, fy - 27, 10, 7, '#fffdf5'); R(g, fx - 5, fy - 27, 10, 1, '#c5cae9'); R(g, fx - 4, fy - 26, 4, 3, '#7986cb'); R(g, fx, fy - 26, 4, 3, '#7986cb'); }
      if (a.status === 'planning') { R(g, fx - 2, fy - 26, 4, 4, '#ffe066'); R(g, fx - 1, fy - 22, 2, 2, '#9aa7ad'); }
      if (a.status === 'chatting') { R(g, fx + 5, fy - 24, 9, 6, '#fffdf5'); R(g, fx + 7, fy - 21, 1, 1, '#2b1d16'); R(g, fx + 9, fy - 21, 1, 1, '#2b1d16'); R(g, fx + 11, fy - 21, 1, 1, '#2b1d16'); R(g, fx + 6, fy - 18, 2, 1, '#fffdf5'); }
      if (a.status === 'working' || a.status === 'checking') { R(g, fx - 12, fy - 26, 24, 5, 'rgba(43,29,22,.8)'); R(g, fx - 11, fy - 25, Math.max(2, 22 * ((hash(a.taskId || '') % 70 + 20) / 100)) | 0, 3, '#7bc043'); }
    });
    const c = cv.getContext('2d');
    c.imageSmoothingEnabled = false;
    c.drawImage(off, 0, 0, W * s, H * s);
  }

  // ---------- public ----------
  window.Game = {
    init(cv, h) {
      canvas = cv; ctx = canvas.getContext('2d'); handlers = h || {};
      lo = document.createElement('canvas'); lx = lo.getContext('2d');
      bg = document.createElement('canvas'); bx = bg.getContext('2d');
      const m = /(?:^|[#&])hour=(\d+(?:\.\d+)?)/.exec(location.hash); if (m) hourOverride = Number(m[1]) % 24;
      window.addEventListener('resize', resize);
      document.addEventListener('visibilitychange', () => { if (!document.hidden) { last = performance.now(); schedule(); } else if (rafId) { clearTimeout(rafId); rafId = 0; } });
      rm.addEventListener?.('change', () => schedule());
      canvas.addEventListener('mousemove', e => {
        const [x, y] = toWorld(e.clientX, e.clientY);
        hoverHit = hitAt(x, y);
        canvas.style.cursor = hoverHit && hoverHit.type !== 'pond' && !(hoverHit.type === 'plot' && !hoverHit.taskId) ? 'pointer' : 'default';
        handlers.onHover && handlers.onHover(hoverHit, e.clientX, e.clientY);
      });
      canvas.addEventListener('mouseleave', () => { hoverHit = null; handlers.onHover && handlers.onHover(null); });
      canvas.addEventListener('click', e => { const [x, y] = toWorld(e.clientX, e.clientY); const hit = hitAt(x, y); if (hit && handlers.onClick) handlers.onClick(hit); });
      resize();
      schedule();
    },
    setInsets(i) { insets = { ...insets, ...i }; if (canvas) resize(); },
    setHour(h) { hourOverride = h; bgKey = ''; },
    setModel(m) {
      model = m;
      const ids = new Set((m.plots || []).map(p => p.id));
      for (let i = 0; i < slots.length; i++) if (slots[i] && !ids.has(slots[i])) slots[i] = null;
      for (const p of m.plots || []) if (!slots.includes(p.id)) { const f = slots.indexOf(null); if (f >= 0) slots[f] = p.id; }
      schedule();
    },
    setSelected(id) { selected = id || null; },
    drawAvatar,
    drawRoom,
    slotOf: id => slots.indexOf(id),
  };
})();
