// Dev-only WCAG 2.1 AA spot check for web/site. Paste into the console on the running site.
// Not a replacement for axe or a real screen reader; it checks what's easy to get wrong here:
// text contrast against the effective background, touch target size, alt text, heading order,
// form labels, and accessible names on buttons and links.
(() => {
  const parse = (c) => {
    const m = c.match(/rgba?\(([^)]+)\)/);
    if (!m) return null;
    const [r, g, b, a = 1] = m[1].split(/[ ,/]+/).filter(Boolean).map(Number);
    return { r, g, b, a };
  };
  const lum = ({ r, g, b }) => {
    const f = (v) => ((v /= 255) <= 0.03928 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4);
    return 0.2126 * f(r) + 0.7152 * f(g) + 0.0722 * f(b);
  };
  const blend = (top, under) => ({
    r: top.r * top.a + under.r * (1 - top.a),
    g: top.g * top.a + under.g * (1 - top.a),
    b: top.b * top.a + under.b * (1 - top.a),
    a: 1,
  });
  const bgOf = (el) => {
    const layers = [];
    for (let n = el; n; n = n.parentElement) {
      const c = parse(getComputedStyle(n).backgroundColor);
      if (c && c.a > 0) {
        layers.push(c);
        if (c.a >= 1) break;
      }
    }
    let col = { r: 255, g: 255, b: 255, a: 1 };
    for (let i = layers.length - 1; i >= 0; i--) col = blend(layers[i], col);
    return col;
  };
  const ratio = (a, b) => {
    const [x, y] = [lum(a), lum(b)].sort((p, q) => q - p);
    return (x + 0.05) / (y + 0.05);
  };

  const contrast = [];
  const seen = new Set();
  for (const el of document.querySelectorAll("body *")) {
    if (el.closest("svg, iframe, .hp, .sr-only, script, style, template, .img-missing")) continue;
    const own = [...el.childNodes].some((n) => n.nodeType === 3 && n.textContent.trim());
    if (!own) continue;
    const cs = getComputedStyle(el);
    if (cs.visibility === "hidden" || cs.display === "none" || +cs.opacity === 0) continue;
    const r = el.getBoundingClientRect();
    if (!r.width || !r.height) continue;
    const fg = parse(cs.color);
    const bg = bgOf(el);
    const cr = ratio(blend(fg, bg), bg);
    const size = parseFloat(cs.fontSize);
    const bold = +cs.fontWeight >= 700;
    const large = size >= 24 || (bold && size >= 18.66);
    const need = large ? 3 : 4.5;
    if (cr < need) {
      const key = el.className + cs.color + cs.backgroundColor;
      if (seen.has(key)) continue;
      seen.add(key);
      contrast.push({ el: (el.tagName + "." + el.className).slice(0, 60), text: el.textContent.trim().slice(0, 40), ratio: +cr.toFixed(2), need });
    }
  }

  const targets = [...document.querySelectorAll("a, button, summary, input, [role=button]")]
    .filter((e) => !e.closest(".hp") && e.offsetParent)
    .map((e) => ({ e, r: e.getBoundingClientRect() }))
    // inline links inside running text are exempt (WCAG 2.5.5 inline exception)
    .filter(({ e, r }) => (r.height < 44 || r.width < 44) && !(e.tagName === "A" && getComputedStyle(e).display === "inline"))
    .map(({ e, r }) => ({ el: (e.tagName + "." + e.className).slice(0, 50), text: (e.getAttribute("aria-label") || e.textContent).trim().slice(0, 30), w: Math.round(r.width), h: Math.round(r.height) }));

  const imgs = [...document.querySelectorAll("img")].filter((i) => !i.hasAttribute("alt")).map((i) => i.src);
  const heads = [...document.querySelectorAll("h1,h2,h3,h4,h5,h6")].map((h) => +h.tagName[1]);
  const headingJumps = heads.filter((h, i) => i && h - heads[i - 1] > 1).length;
  const unlabeled = [...document.querySelectorAll("input:not([type=hidden]), select, textarea")]
    .filter((i) => !i.closest(".hp"))
    .filter((i) => !(i.labels && i.labels.length) && !i.getAttribute("aria-label"))
    .map((i) => i.id || i.name);
  const nameless = [...document.querySelectorAll("button, a")]
    .filter((e) => e.offsetParent && !(e.getAttribute("aria-label") || e.textContent.trim()))
    .map((e) => e.outerHTML.slice(0, 80));
  const landmarks = ["header", "nav", "main", "footer"].filter((t) => !document.querySelector(t));

  return {
    scheme: matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light",
    contrastFails: contrast,
    smallTargets: targets,
    imagesWithoutAlt: imgs,
    headingJumps,
    h1Count: heads.filter((h) => h === 1).length,
    unlabeledInputs: unlabeled,
    namelessControls: nameless,
    missingLandmarks: landmarks,
  };
})();
