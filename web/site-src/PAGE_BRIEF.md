# Brief for every page of the Proofline site

Read this before building or editing any page in `web/site/`.

## How pages are built
- Plain static HTML, one folder per page with `index.html` (e.g. `web/site/specs/index.html` is `/specs/`). No framework, no build step at deploy time.
- Links and assets use root paths: `/shop/`, `/assets/img/...`.
- Every page uses this skeleton. The shared parts are stamped by `node web/tools/chrome.mjs` (run it from the worktree root after creating or editing pages; it fills the marked regions and highlights the current page in the menus):
```html
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
  <title>Page name · Proofline</title>
  <meta name="description" content="One honest sentence, under 160 characters.">
  <link rel="canonical" href="https://proofline.example/page/">   <!-- placeholder domain -->
  <!-- @head -->
  <!-- /@head -->
  <link rel="stylesheet" href="/assets/css/<your-area>.css">   <!-- only if you need one -->
</head>
<body>
  <!-- @header -->
  <!-- /@header -->
  <main id="main">
    <section class="page-hero band-dark">
      <div class="container">
        <nav class="breadcrumbs" aria-label="Breadcrumb"><ol><li><a href="/">Home</a></li><li><a href="/shop/">Shop</a></li><li><span aria-current="page">The calculator</span></li></ol></nav>
        <h1>…</h1>
        <p class="lead">…</p>
      </div>
    </section>
    <section class="section">…</section>
    …
  </main>
  <!-- @footer -->
  <!-- /@footer -->
</body>
</html>
```
- The shared head loads fonts, `/assets/css/site.css` and `/assets/js/site.js` (deferred). Do not edit `site.css`, `site.js`, the partials in `web/site-src/partials/`, or other people's pages. Put area-specific CSS in your own `/assets/css/<area>.css` and JS in `/assets/js/<area>.js` (small, deferred, no dependencies, the page must work without it).
- The shared header defines these SVG icons you can use with `<svg aria-hidden="true"><use href="#i-NAME"/></svg>`: mark, chev, user, cart, menu, close, arrow, check, plus, minus, info, alert, lock, wifi. Add any others in your own inline `<svg width="0" height="0" style="position:absolute">` sprite in `<main>` (ids must be unique on the page; don't reuse the names above).
- Building blocks already in site.css (use them; don't restyle them): `.section`, `.section--tint`, `.band-dark` (always-dark band), `.container`, `.section-head` (+ `.eyebrow`, `h2`, `p`), `.page-hero`, `.page-hero__actions`, `.breadcrumbs`, `.lead`, `.prose` (long text), `.card-grid` + `a.card` (with `.card__icon`, `h3`, `p`, `.card__more`), `.split` (2 columns), `.callout` / `.callout--warn`, `.badge`, `.product-grid` + `.product-card` (`__media`, `__body`, `__price`), `.qty` (quantity stepper), `.cta-band` (+ `.cta-band__inner`), `.btn` `.btn--primary` `.btn--ghost` `.btn--sm`, `.keycap` (`--accent`, `--shift`), `.steps`/`.step` (numbered steps), `.spec-table`, `.faq` (details/summary accordion with `.chev` and `.faq__answer`), `.features`/`.feature`, `.waitlist` form (copy the markup from the footer partial; site.js handles it; give each form unique ids and `data-source="site"`), `[data-reveal]` (scroll fade-in, add to a few blocks only).
- One primary (orange) button per screen. No cards inside cards. One idea per section. Sentence case. Light and dark mode come free from the tokens; check both.
- Images: `/assets/img/hero-front-{560,960,1060}.{avif,webp,png}` (1060×761), `/assets/img/hero-back-{560,960,1021}.*` (1021×800), `/assets/img/exploded-front-{560,905}.*` (905×968), `/assets/video/ai-solve.mp4` (16:9, 12.8 s, silent) with `/assets/video/ai-solve-poster-{960,1600}.{jpg,webp}`. Use `<picture>` with AVIF then WebP then PNG, `width`/`height` set, `loading="lazy"` below the fold, real alt text. They are CAD renders: say "CAD render" where it matters. No stock photos, no AI-generated product images, no faces of minors.
- Accessibility: WCAG 2.1 AA. Labels on inputs, 44px targets, visible focus (site.css does it), alt text, headings in order (one h1), `aria-live` for async messages.

## Voice
Short, plain, confident, honest. No hype ("revolutionary", "unlock", "seamless", "game-changing"), no exclamation marks. The video's lines are the voice: "Snap a problem." "Press = to shoot." "See every step." "Learn every step."

## Hard rules
- **Keep the business side private.** Present AI Solve as part of a simple subscription, the **AI tutor plan ($10/month or $100/year)**. Never mention APIs, servers, keys, tokens, cloud providers, Stripe, Claude, Anthropic or any AI vendor on marketing or help pages. (Only the privacy policy names data processors; leave that page alone.) Say "the calculator works out the answer", "AI Solve", "the AI tutor plan".
- **The AI tutor plan is never shown as a separate product** (owner decision, 2026-10-14). The calculator is the only product: no plan product card, no plan page, no "add the plan" or "buy the plan" button, no plan in menus or as its own Product/Offer in structured data. The plan lives in the calculator's description (`/shop/calculator/#ai-tutor`), on `/pricing/` (a quiet line under the calculator price) and on the account page (where it is started after linking). Keep it calm and factual. Link to `/shop/calculator/#ai-tutor` or `/pricing/`; the old `/shop/ai-tutor-plan/` redirects there (`web/site/_redirects`).
- **Invent nothing.** Use only the facts below. No made-up reviews, ratings, testimonials, user counts, press logos, awards, accessories, colors, bundles, discounts, shipping dates or "in stock". No "what's in the box" list (not decided). No battery-life number.
- Brand name is the placeholder `Proofline`. Never use "Casio" or a Casio model name as our brand. Footer already says "Not affiliated with Casio Computer Co., Ltd."
- Never say or imply: exam-approved, undetectable, stealth, hidden mode, panic/hide button, cheat.
- Audience: high school and college students (13+) and the adults who buy it. **Never mention parents, children or families** (no parents page, no "for parents" copy; decided 2026-10-11). Nothing aimed at kids 12 and under.
- Domain placeholder `proofline.example`, contact `hello@proofline.example`.

## Facts (the only facts)
- What it is: an AI study tutor that looks and feels like a real scientific calculator. Our own circuit board in a familiar scientific-calculator shell with the standard US key layout and a real rubber keymat.
- A full calculator that works offline: fractions, trig, logs, memory, replay and more.
- Calculator modes (built and tested in the prototype software, 2026-10-11 and 2026-10-12): the MODE menu is 1 COMP, 2 STAT, 3 TABLE, 4 Apps, 5 EQN, 6 CMPLX, 7 BASE-N, 8 MATRIX, 9 VECTOR. **STAT (MODE 2)**: 1-variable and 2-variable statistics, 7 regression types (linear, quadratic, logarithmic, e-exponential, ab-exponential, power, inverse), a data editor of up to 80 rows (fewer for paired data or with the frequency column), optional frequency column (SETUP), sums, means, standard deviations, min/max, regression coefficients and estimates. **TABLE (MODE 3)**: f(x) table from a start, end and step, up to 30 rows. **CALC** substitutes values into the variables of an expression; **SHIFT CALC (SOLVE)** solves an equation numerically from a guess ("Can't Solve" when it finds no root). **∫dx**: definite integrals; **SHIFT ∫dx (d/dx)**: the derivative at a point. **EQN (MODE 5)**: linear systems with 2 or 3 unknowns ("No Solution" / "Infinite Solution" when there isn't a single answer), quadratic and cubic equations including complex roots (shown as a+bi), and a quadratic's minimum or maximum point. **CMPLX (MODE 6)**: complex arithmetic (+ − × ÷, powers) in a+bi or polar r∠θ form, answers in either form, arg, conjugate, absolute value. **BASE-N (MODE 7)**: integers in DEC, HEX, BIN and OCT (one key converts the answer), logic and, or, xor, xnor, Not, Neg. **MATRIX (MODE 8)**: up to 3×3, + − ×, determinant, transpose, inverse, kept as exact fractions. **VECTOR (MODE 9)**: 2D or 3D, + −, dot product, cross product, magnitude. All work offline and stay on in exam mode (they are normal calculator features; only the AI, camera, Wi-Fi and apps turn off), so a teacher who doesn't want one used in a test says so in their rules. Nothing beyond these (no graphing, no symbolic algebra/CAS, no matrices bigger than 3×3): don't claim them. The calculator screens use a classic LCD look (no color); color is only in the Apps. Real screens: `/assets/img/calc/` (stat-editor, table, solve, integral, exam-stat, eqn-quadratic, cmplx, basen-xor, matrix-inverse), same format as `/assets/img/apps/`.
- Apps: MODE 4 opens Apps: AI Solve, Type it, History, Notes, Geography, Periodic Table, Settings (Settings includes Link calculator). The calculator itself is a normal scientific calculator; the apps are extra.
- AI Solve (MODE 4 → AI Solve): the screen becomes a live camera viewfinder; press = to take a photo of a problem; the answer and every step appear on the calculator. Works for STEM (math and science) and basic finance. If the handwriting is unclear, it says so instead of guessing. Needs Wi-Fi.
- Exam mode: SHIFT MODE 8 = turns the AI, camera, Wi-Fi and all the apps off (turning it on closes any open app; the calculator, STAT, TABLE, EQN, CMPLX, BASE-N, MATRIX, VECTOR, CALC, SOLVE, ∫dx and d/dx keep working) and turns the status bar black (padlock + "EXAM" and a timer), so a teacher can see it at a glance. It ends by itself after 12 hours, or a teacher unlocks it with a USB cable.
- Not allowed on the SAT, ACT or AP exams (it has Wi-Fi and a camera), even in exam mode. Use a standard calculator there. For school tests: ask the teacher; use exam mode or leave it at home.
- Setup: it registers itself the first time it joins Wi-Fi; no codes or web addresses to type for Wi-Fi. To use AI Solve, the account holder links it to their account once: the calculator shows an 8-character code (and a QR code) under MODE 4 → Settings → Link calculator, and you enter it at `/link/` on any phone or computer. Wi-Fi: 2.4 GHz home Wi-Fi or a phone hotspot. School Wi-Fi with a login page isn't supported. Wi-Fi can be set up by typing on the keys, or with a phone. Automatic updates over Wi-Fi are planned (say "planned").
- Hardware: ESP32-S3 processor, 5 MP autofocus camera on the back, Wi-Fi (2.4 GHz), magnetic charging connector, 150 mAh rechargeable battery (battery life is being measured on prototypes), no LED. Screen: "a sharp color display" (never "sunlight-readable"; don't state size, resolution or panel type).
- Price: the calculator is $225. The AI tutor plan is $10/month, or $100/year (2 months free, about $8.33 a month; recommend yearly, honestly, no fake urgency). One plan per account: up to 3 calculators sharing up to 300 solves a month (fair use). The plan covers AI Solve and software updates: **updates come only with the plan** (decided 2026-10-11). First month free, starting when you first link the calculator, before either plan starts; no card needed; the calculator says plainly when the free month is over. No free tier after that. Cancel anytime from your account page. First year: $225 + 11 × $10 = $335 monthly, or $225 + $100 = $325 yearly. Without the plan it is still a full scientific calculator (offline), on the software it has. AI can make mistakes; that's why it shows every step.
- Who buys: buyers and account holders are adults (18+). People using it should be 13+. Don't say who the adult is (no parent/guardian wording).
- Privacy: photos are used only to solve that problem and are not stored; we keep only usage counts. Up to 3 calculators per account. A calculator can be removed from the account any time (it then needs linking again).
- Status: our first prototype boards are ordered (October 2026). We're building 5 samples first, then moving to production; preorders are planned for 2027. We are not taking payment yet; the main call to action is "Join the waitlist". The shop shows prices and a "Preorder" button that only works when the store opens (GET `/api/store/status` -> `{open, max_quantity}`; until then show "Join the waitlist").
- Shipping: US addresses only (when orders open). Refund/return/warranty terms are not decided yet (link to `/refunds/`).
- Real photos come later: leave a clear spot ("Prototype photos coming soon"), never fake them.
