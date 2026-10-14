# Launch site (stage 4, `ai_calc_web_4`, multi-page since stage 3)

The public website: shop, product pages, support, account pages and legal templates. Static HTML,
CSS and a little vanilla JS: no framework, no build step (plan decision D1). Everything in
`web/site/` is what the Worker serves as static assets (`[assets]` in `web/api/wrangler.toml`);
this file and the notes beside it live outside it on purpose.

## Run it locally

Whole site plus the server, with a fake payment page and a fake AI (recommended):

```bash
node web/dev/run-local.mjs
```

Then open http://127.0.0.1:8787/. Sign-in e-mails go to a local outbox; `node web/dev/calculator.mjs new`
pretends to be a calculator. `--fresh` wipes the local data. The full walk-through is
`docs/web/GUIDE.md` section 2.

Static pages only (forms show their friendly "can't reach" message):

```bash
python -m http.server 8765 --bind 127.0.0.1 --directory web/site
```

Then open http://localhost:8765/. `.claude/launch.json` has both ("local-all" and "launch-site") for the
browser pane. Nothing is deployed and nothing costs money.

## Pages (29, plus the `/try/` simulator frame)

| Section | Paths |
|---|---|
| Home | `/` (hero, shop row, explore grid, short FAQ, waitlist) |
| Shop | `/shop/`, `/shop/calculator/`, `/shop/ai-tutor-plan/`, `/cart/`, `/order/complete/` |
| Product | `/how-it-works/`, `/features/` (+ `ai-solve/`, `apps/`, `exam-mode/`, `calculator/`), `/try-it/`, `/specs/`, `/pricing/` |
| Support | `/faq/` (38 answers), `/support/` (+ `setup/`, `wi-fi/`) |
| Audiences | `/teachers/`, `/honest/` (the "honest by design" points moved off the homepage) |
| Account | `/login/`, `/login/confirm/`, `/link/`, `/account/` |
| Legal | `/privacy/`, `/terms/`, `/refunds/` (NEEDS LAWYER REVIEW, `noindex`) |
| Other | `404.html`, `/try/` (the simulator iframe) |

## What else is in `web/site/`

| Path | What |
|---|---|
| `assets/css/site.css` | Tokens (colour, type, space, motion), shared chrome, building blocks (page-hero, breadcrumbs, prose, cards, split, callout, product grid, qty, cta-band) |
| `assets/js/site.js` | Header menus, `window.SiteCart` (localStorage `cart.v1`, max 3), waitlist forms, lazy video, simulator, exam demo, FAQ deep links, reveals |
| `assets/js/shop.js` | Store status, quantity steppers, gallery, add to cart / preorder, cart page, checkout |
| `assets/js/portal.js`, `assets/css/portal.css` | Sign in, link a calculator, account (subscribe, manage billing, delete) |
| `assets/img/`, `assets/video/`, `assets/og-image.jpg` | Fusion render cutouts, the AI Solve video, share image |
| `assets/img/apps/`, `assets/css/apps.css` | v15 firmware screens (from `hardware/renders_ui_v15/` on the firmware branch), nearest-neighbour 2x/3x: lossless WebP + PNG (lossless WebP is smaller than AVIF for pixel art, so no AVIF). Shown with `image-rendering: pixelated` |
| `assets/img/calc/` | Calculator-mode screens (STAT editor, TABLE, SOLVE, ∫dx, STAT in exam mode from `M*.png`; EQN quadratic `E12`, CMPLX `C04`, BASE-N xor `B11`, MATRIX inverse `X12`, all in `hardware/renders_ui_v15/`; the Apps page's `apps/mode-menu` is `E01`, the nine-mode menu), same format as `assets/img/apps/`; shown on `/features/calculator/` (`.calc-modes` in `apps.css`) and `/features/exam-mode/` |
| `_headers`, `robots.txt`, `sitemap.xml` (19 URLs), `site.webmanifest`, favicons | Housekeeping. CSS/JS `max-age=0, must-revalidate`; personal pages `no-store` |

## Shared header and footer

Edit `web/site-src/partials/{head,header,footer}.html`, then stamp them into every page:

```bash
node web/tools/chrome.mjs
```

Pages carry `<!-- @head -->`, `<!-- @header -->`, `<!-- @footer -->` marker regions; links are root-absolute.
`node web/tools/chrome.mjs --check` fails if any page is out of date. `web/site-src/PAGE_BRIEF.md` holds
the facts and copy rules every page follows.

## Checks

```bash
node web/tools/check-site.mjs
```

Links, ids, icons, images, headings, chrome up to date, and the copy rule: marketing pages never name the
AI or payment providers (only the privacy policy lists processors). The console helpers in `web/tests/site/`
(`break-ui.js`, `a11y-audit.js`) still work on any page.

## Design system

- Colour: navy `#10141c` and DEL-key orange `#e07a4f`. Light mode is warm paper `#fbfaf6` with navy text;
  orange text in light mode is `#a84c25` (5.4:1). Dark bands use `.band-dark` in both modes.
- Type: Barlow Semi Condensed (headings), Barlow (body), IBM Plex Mono (screen, specs, labels).
- Space: 4px grid; sections `clamp(72px, …, 128px)`.
- Motion: `--ease-out: cubic-bezier(0.23, 1, 0.32, 1)`; reveals staggered per batch; everything respects
  `prefers-reduced-motion`.

## Placeholders still to replace

- The name `Proofline` is chosen (2026-10-11) but not trademark-checked, and Proofline Corporation isn't registered yet (see `NAMES.md`).
  `proofline.example` and `hello@proofline.example` are placeholders (canonical, OG, sitemap, robots, contact e-mail).
- Product images are CAD renders until real photos exist (`website/PHOTO_SHOT_LIST.md`).
- Screen: colour (v15 boards ordered). The site says "a sharp colour display", no size or panel type until the samples are tested.
- Legal pages: highlighted TODOs (entity, retention, return window, warranty length).
## Rebuild the assets

- Simulator (`/try/`, embedded on `/try-it/`): the **v15 firmware's own code** compiled to WebAssembly. It is built
  on the firmware branch (`claude/v15-mode4-menu`): `tools\web_sim_v15\build.bat` (needs emsdk in
  `%USERPROFILE%\emsdk`), then copy `tools/web_sim_v15/dist/index.html` to `web/site/try/index.html`. One
  self-contained file (~510 KB, ~224 KB gzipped), no network calls, demo answers only; its build refuses any
  vendor name. The old e-paper pipeline (`sim/web/make_embed.py`) is retired for the site.
- Video: see `website/README.md`, then copy `ai-solve.mp4` into `web/site/assets/video/`.
- Images: cutouts from `hardware/enclosure/final_assembly/renders/` (originals untouched).
