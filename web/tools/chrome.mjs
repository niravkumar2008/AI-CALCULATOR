// Stamps the shared head, header and footer into every page of web/site.
//
//   node web/tools/chrome.mjs           update all pages in place
//   node web/tools/chrome.mjs --check   exit 1 if any page is out of date (for CI / before deploy)
//
// The site stays plain static HTML with no build step at deploy time: this only rewrites the
// regions between these markers, and the result is committed as is.
//   <!-- @head -->   ... <!-- /@head -->     (inside <head>, after the page's own title/meta)
//   <!-- @header --> ... <!-- /@header -->   (first thing in <body>)
//   <!-- @footer --> ... <!-- /@footer -->   (last thing in <body>)
// Partials: web/site-src/partials/{head,header,footer}.html
// It also marks the current page in the menus: aria-current="page" on links to this page and
// `is-current` on the menu that contains it.
//
// Pages without markers (e.g. try/index.html, the iframe calculator) are left alone.

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const SITE = path.resolve(here, '../site');
const PARTIALS = path.resolve(here, '../site-src/partials');
const check = process.argv.includes('--check');

const partial = (name) => fs.readFileSync(path.join(PARTIALS, `${name}.html`), 'utf8').replace(/\s+$/, '');

function pagesIn(dir) {
  const out = [];
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const p = path.join(dir, e.name);
    if (e.isDirectory()) {
      if (e.name === 'assets') continue;
      out.push(...pagesIn(p));
    } else if (e.name.endsWith('.html')) out.push(p);
  }
  return out;
}

/** /shop/calculator/ for site/shop/calculator/index.html; /404.html for site/404.html. */
function urlPathOf(file) {
  const rel = path.relative(SITE, file).split(path.sep).join('/');
  if (rel === 'index.html') return '/';
  if (rel.endsWith('/index.html')) return '/' + rel.slice(0, -'index.html'.length);
  return '/' + rel;
}

function markCurrent(html, here) {
  // Links to this page get aria-current; the <details> menu containing one gets is-current.
  let out = html.replace(/<a ([^>]*?)href="([^"]+)"([^>]*)>/g, (m, a, href, b) => {
    const clean = `<a ${a}href="${href}"${b}>`.replace(/ aria-current="page"/g, '');
    return href === here && !/class="brand"/.test(m) ? clean.replace('<a ', '<a aria-current="page" ') : clean;
  });
  out = out.replace(/<details class="nav__menu( is-current)?" data-menu>([\s\S]*?)<\/details>/g, (m, _cur, inner) =>
    `<details class="nav__menu${inner.includes('aria-current="page"') ? ' is-current' : ''}" data-menu>${inner}</details>`,
  );
  return out;
}

function stamp(html, name, body) {
  const re = new RegExp(`(<!-- @${name} -->)[\\s\\S]*?(<!-- /@${name} -->)`);
  if (!re.test(html)) return { html, found: false };
  return { html: html.replace(re, (_m, open, close) => `${open}\n${body}\n  ${close}`), found: true };
}

const parts = { head: partial('head'), header: partial('header'), footer: partial('footer') };
let changed = 0;
let stamped = 0;
for (const file of pagesIn(SITE)) {
  const before = fs.readFileSync(file, 'utf8');
  let html = before;
  let any = false;
  for (const name of ['head', 'header', 'footer']) {
    const body = name === 'head' ? parts.head : markCurrent(parts[name], urlPathOf(file));
    const r = stamp(html, name, body);
    html = r.html;
    any ||= r.found;
  }
  if (!any) continue;
  stamped++;
  if (html !== before) {
    changed++;
    if (check) console.log(`out of date: ${path.relative(SITE, file)}`);
    else fs.writeFileSync(file, html, 'utf8');
  }
}
console.log(`${stamped} pages with shared chrome, ${changed} ${check ? 'out of date' : 'updated'}`);
if (check && changed) process.exit(1);
