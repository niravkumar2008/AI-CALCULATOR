# ai_calc_web_3: payments, subscriptions and the store (2026-10-11)

Stage 3 of the sell-and-subscribe layer (`docs/web/PLAN.md`), plus the multi-page website (stage 4) and the
account pages (the portal part of stage 2). Built in worktree `ai-calculator-launch-site-034250`, on top of a
copy of stage 1 (which is still uncommitted in `ai-calc-sell-subscribe-4bd464`; commit from this worktree only).
Nothing is deployed and nothing costs money.

> Update 2026-10-11 (later the same day): the site now has **29 pages** (added `/features/apps/`), the FAQ has
> 37 answers, and `/try/` runs the v15 firmware's own simulator instead of `sim/web/make_embed.py`'s output.

## Added: server (`web/api`)
- `src/stripe.ts`: a small Stripe client (form-encoded calls, idempotency keys, 10 s timeout) and webhook
  signature checking (HMAC-SHA256 of `t.body`, constant-time, 5-minute window). Reads both the older and
  the 2025+ ("basil") Stripe shapes. No Stripe SDK: faster on the free plan's 10 ms CPU budget and easy to
  test offline.
- `src/billing.ts`:
  - `POST /api/billing/checkout`: subscription checkout for the AI tutor plan ($10/month or $100/year). Creates the
    Stripe customer once, no Stripe trial (our own free month, decision D2), 18+ and Terms consent,
    optional Stripe Tax, 31-minute expiry, refuses a second subscription (checked locally and at Stripe).
  - `POST /api/billing/portal`: Stripe's "Manage billing" page.
  - `POST /api/store/checkout` and `GET /api/store/status`: buying 1–3 calculators (US shipping), behind
    `STORE_OPEN`, with per-IP and global hourly limits.
  - `POST /api/stripe/webhook`: verified, idempotent (event recorded in the same transaction as its effects),
    re-reads every subscription from Stripe so delivery order can't matter, maps accounts only by our own
    Stripe customer id, ignores other prices and other payments, refuses test/live mismatches.
  - Account deletion now deletes the Stripe customer first (stops all billing, kills open checkouts).
- `src/waitlist.ts`: `POST /api/waitlist` for the website (JSON or a plain form post; same answer for new
  and repeated addresses; honeypot; 5 per IP per hour; 500 new addresses a day).
- `src/plan.ts`: entitlement counts only the AI tutor plan's price; the 3-day grace for a failed payment
  counts from the failure (`past_due_since`); a live subscription wins over an old cancelled one.
- `GET /api/me` adds `has_billing`.
- Migrations `0003_waitlist.sql`, `0004_past_due_since.sql`.
- `wrangler.toml`: the Worker now also serves the website (`[assets]` from `web/site`, the Worker runs only
  for `/api/*`, `/v1/*`, `/__dev/*`, `/healthz`; unknown pages get `404.html`), plus the stage 3 settings.

## Added: website (`web/site`)
- 28 pages: home; shop (all products, the calculator, the AI tutor plan, cart, order complete); product
  (how it works, features, AI Solve, exam mode, the calculator, try it, specs, pricing); support (FAQ with 34
  answers, support hub, setup guide, Wi-Fi help); for teachers; honest by design; account
  (sign in, confirm, link a calculator, your account); legal templates; 404.
- One shared header (menus, account, cart with a count, mobile menu) and footer, stamped into every page by
  `web/tools/chrome.mjs` from `web/site-src/partials/`. `web/tools/check-site.mjs` checks every page
  (links, ids, icons, images, headings, the no-vendor copy rule, chrome up to date).
- Marketing copy never names the AI or payment providers (Nirav's decision, 2026-10-11); only the privacy
  policy lists data processors, as the law requires.
- The homepage's three "honest" points moved to `/honest/`.
- `web/site-src/PAGE_BRIEF.md`: the facts and rules every page follows.

## Added: local development (`web/dev`)
- `run-local.mjs`: the whole site and server on http://127.0.0.1:8787 with a fake Stripe (pretend checkout
  and billing pages that send real signed webhooks) and a fake AI. Data kept between runs.
- `calculator.mjs`: pretend to be a calculator (new, status, code, solve, forget).
- `.claude/launch.json`: "local-all" for the Claude desktop app's browser pane.

## Changed
- `docs/web/openapi.yaml`: the stage 3 routes in full, `/api/waitlist`, `/api/store/status`, `has_billing`,
  the webhook's 200 body, and the deletion behaviour.
- `web/tests`: `stage3.todo.test.mjs` replaced by `stage3.test.mjs` against a Stripe mock; a fourth pass
  ("closed" store).
- `sim/web/make_embed.py`: `BRAND` setting; also writes `web/site/try/`.

## Security review
Round 1 of stage 3 found 2 HIGH, 2 MEDIUM, 5 LOW, 3 INFO. All HIGH and MEDIUM are fixed, most LOW/INFO too;
details in `docs/web/GUIDE.md` section 10.
