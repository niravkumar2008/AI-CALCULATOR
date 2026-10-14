# AI Calculator API (`web/api`)

One Cloudflare Worker that does everything the calculators and the account website need:

- **Calculators** (`/v1/*`): register, pairing codes, status, photo solving (streams Claude's
  answer), firmware updates.
- **Website / portal** (`/api/*`): e-mail sign-in links, linking a calculator with its code,
  renaming and removing calculators, deleting the account, the waitlist, and (stage 3) the
  AI tutor plan checkout ($10/month or $100/year), "Manage billing", the store and the Stripe webhook.
- **The website itself**: the static pages in `web/site/` are served by this same Worker
  (`[assets]` in `wrangler.toml`; see `web/SITE_README.md`).

The exact rules for every endpoint are in `docs/web/openapi.yaml` (the contract) and the
design is in `docs/web/PLAN.md`. Data lives in **D1** (Cloudflare's SQLite) and firmware
images in **Workers KV**. Your Anthropic API key stays on the server; calculators never see it.

Payments (Stripe, stage 3) are built and tested in Stripe test mode only: `/api/billing/*`,
`/api/store/*` and `/api/stripe/webhook`. The Stripe setup, step by step, is in
`docs/web/GUIDE.md` sections 3 to 5. Nothing is deployed yet.

The v15 firmware can talk to this Worker (stage 2, built and tested end to end against the local
stack on 2026-10-12, branch `claude/v15-mode4-menu`: register, status with `pair_code`/`plan_reason`,
link, solve incl. typed PNGs, firmware check by family). It still defaults to the older Python
server/proxy until switched with the serial command `api worker <url>`; real-board testing is pending.

**Free month rules** (security review S1): the free month starts when a calculator is linked,
but only if the board was **enrolled at QA** (see section 5b), that board never had a free
month, the account never had one, the e-mail address never had one (remembered as a hash even
after the account is deleted), and fewer than 20 new free months started today. Dev boards and
fakes can still link, but must subscribe.

## 1. Install (once)

You need Node.js 20 or newer (`node --version`). In a terminal:

```
cd web/api
npm install
```

## 2. Run it on your computer

```
copy .dev.vars.example .dev.vars        (macOS/Linux: cp .dev.vars.example .dev.vars)
npm run db:migrate:local                 creates the local database tables
npm run dev                              starts the API at http://localhost:8787 (with DEV_MODE=1)
```

Open http://localhost:8787/healthz in a browser: it should show `{"ok":true}`.

- To make real solves, put your Anthropic key in `.dev.vars` (`ANTHROPIC_API_KEY=...`).
  Without it, `/v1/solve` answers 503 "unavailable" and nothing is charged.
- `npm run dev` sets `DEV_MODE=1`, which turns on the two test helpers `/__dev/outbox` (read
  the last sign-in link) and `/__dev/user` (set trial/plan), and allows the http:// address.
  The helpers only work on `localhost`, and production never sets `DEV_MODE` (without it,
  the server refuses to run with a non-https `PUBLIC_URL`).
- Any setting can be changed for one run with `--var`, e.g. tiny limits for testing:
  `npx wrangler dev --local --port 8787 --var DEV_MODE:1 --var MONTHLY_SOLVES:2`.

Everything runs locally (no Cloudflare account needed, nothing is uploaded).

## 3. Test

```
npm run typecheck     TypeScript checks
npm test              all tests, inside the real Workers runtime (Miniflare)
```

Tests never call Anthropic: they replace it with a fake that streams a canned answer.

## 4. Settings

In `wrangler.toml` under `[vars]` (not secret):

| Name | Default | What it does |
|---|---|---|
| `PUBLIC_URL` | `http://localhost:8787` | The site address. Used in links, codes shown on the calculator, cookie security and the Origin check. Set it to your real address before going live. |
| `MODEL` / `MAX_TOKENS` / `EFFORT` | `claude-sonnet-5-5` / `16000` / `high` | What the server asks Claude for. |
| `REFUSAL_FALLBACK` | `0` | `1` lets Anthropic retry a refused photo on another model. Off until the limits know that model's price. |
| `TRIAL_DAYS` | `30` | Free month length. |
| `MONTHLY_SOLVES` / `MONTHLY_SPEND_USD` | `300` / `5` | Per-account monthly limits (whichever comes first). |
| `GLOBAL_DAILY_CAP_USD` | `50` | Hard stop for all accounts together, per UTC day. |
| `PRICE_IN_PER_MTOK` / `PRICE_OUT_PER_MTOK` | `2` / `10` | Model price in USD per million tokens, for the limits. Update if the model changes. |
| `UPSTREAM_TIMEOUT_MS` | `60000` | How long to wait for Claude to start answering. |
| `MAX_NEW_TRIALS_PER_DAY` | `20` | At most this many new free months per UTC day. |
| `MAIL_DAILY_CAP` | `80` | At most this many sign-in e-mails per UTC day (Resend free plan: 100). |
| `REGISTER_GLOBAL_PER_HOUR` | `200` | At most this many calculator registrations per hour in total (plus 5 per IP; IPv6 counted per /64). |
| `MAIL_FROM` | (empty) | Sender for sign-in e-mails, e.g. `Proofline <login@proofline.example>`. |
| `STRIPE_PRICE_SUB` / `STRIPE_PRICE_SUB_YEARLY` / `STRIPE_PRICE_DEVICE` | (empty) | Stripe price ids: AI tutor plan monthly ($10) and yearly ($100; empty = monthly only), and the $225 calculator. |
| `STRIPE_TAX` | `0` | `1` turns on Stripe Tax at checkout. |
| `STORE_OPEN` | `0` | `1` when preorders open; the site switches from "Join the waitlist" to "Preorder" by itself. |
| `WAITLIST_DAILY_CAP` / `STORE_GLOBAL_PER_HOUR` | `20000` / `300` | New waitlist addresses per day (an abuse backstop sized for a viral day; once hit, nothing is stored and the form says "busy, try tomorrow"); store checkouts per hour for everyone. |

Secrets (never in git; locally in `.dev.vars`, in production with `npx wrangler secret put NAME`):
`ANTHROPIC_API_KEY`, `RESEND_API_KEY` (sign-in e-mails; without it, e-mails are not sent),
`TRIAL_PEPPER` (**required in production**: the server refuses to run without it; it keys the
hashes of e-mail addresses that already had a free month), `ADMIN_TOKEN` (enables board
enrolment and firmware publishing; must be at least 32 characters, otherwise both stay off), `TURNSTILE_SECRET` (when set, sign-in needs a Cloudflare
Turnstile token; the sign-in page doesn't have the widget yet, and it is required before
launch, see `docs/web/GUIDE.md` section 6), `STRIPE_SECRET_KEY` (a restricted key) and
`STRIPE_WEBHOOK_SECRET` (stage 3).

## 5. Deploy on the FREE plan (workers.dev address)

1. Make a free Cloudflare account and log in: `npx wrangler login`.
2. Create the database and the firmware store, and paste the printed ids into `wrangler.toml`:
   ```
   npx wrangler d1 create ai-calc                  -> database_id
   npx wrangler kv namespace create FIRMWARE       -> id
   ```
3. Create the tables in the real database: `npx wrangler d1 migrations apply DB --remote`
4. Add the secrets. `TRIAL_PEPPER` and `ADMIN_TOKEN` must each be 32 random bytes made with
   this command (run it once for each; paste the printed value when asked):
   ```
   node -e "console.log(require('crypto').randomBytes(32).toString('base64url'))"
   ```
   ```
   npx wrangler secret put ANTHROPIC_API_KEY
   npx wrangler secret put RESEND_API_KEY     (optional until you want real e-mails)
   npx wrangler secret put TRIAL_PEPPER       (required; never change it once live)
   npx wrangler secret put ADMIN_TOKEN        (for enrolment and firmware publishing)
   ```
5. Set `PUBLIC_URL` in `wrangler.toml` to the address the site will use, then `npm run deploy`.
   Wrangler prints the address, like `https://ai-calc-api.<you>.workers.dev`.
6. Check it: open `<address>/healthz`.
7. Also set a **monthly spend limit in the Anthropic Console** as a backstop.
8. In the Cloudflare dashboard add a free **WAF rate-limiting rule** for `/v1/device/register`
   and `/api/auth/login` (extra protection in front of the Worker's own limits).

## 5b. Enrol each board at QA (once, during the first USB flash)

```
set ADMIN_TOKEN=<the same token>
node scripts/enroll-board.mjs calc-0123456789ab https://ai-calc-api.<you>.workers.dev
```

It prints a line `enroll hs_...`: type it on the board's serial console. (The `enroll`
serial command comes with the stage-2 firmware.) Without enrolment a board still works with a
subscription, but never gets the free month. Both scripts refuse plain http except to
localhost, so the admin token is never sent unencrypted.

Publishing firmware for over-the-air updates:

```
set ADMIN_TOKEN=<the same token>                     (PowerShell: $env:ADMIN_TOKEN="...")
node scripts/publish-firmware.mjs firmware.bin stage14-2026.10.08 https://ai-calc-api.<you>.workers.dev
```

Boards that send their family (`GET /v1/firmware?family=v15lcd`) only see images published
for that family, so publish those with `--family` (the version must start with `v15lcd-`).
To undo a bad release, publish the older good image again with `--rollback`; the calculators
then accept the downgrade. The next normal publish clears the rollback mark.

```
node scripts/publish-firmware.mjs v15lcd.bin v15lcd-2026.10.11 <api-url> --family v15lcd
node scripts/publish-firmware.mjs v15lcd-old.bin v15lcd-2026.10.09 <api-url> --family v15lcd --rollback
```

## 6. What costs money, and when

| Item | When you pay |
|---|---|
| Cloudflare Workers, D1, KV | **Free** to build and test. The free plan allows 100,000 requests/day but only 10 ms of CPU per request; big photos can go over that, so buy **Workers Paid ($5/month)** at launch. |
| Domain name | About $10-15/year, when you want your own domain (placeholder `proofline.example`) instead of `workers.dev`. |
| Anthropic API | Every real solve (about $0.01-0.03). Capped by the limits above. Tests cost nothing. |
| Resend (e-mail) | Free up to 3,000 e-mails/month (100/day). |
| Stripe | Only takes a fee when someone pays (about $0.66 per $10 monthly payment; see `docs/web/GUIDE.md` section 7). |

## 7. How the spend cap works (short version)

Before calling Claude, one database transaction checks the device has no other solve running,
the account's monthly solves and spend, and the global daily cap, and reserves the
**worst-case** cost of this solve. If any check fails, or the database has any problem, no call
is made (fail closed). When the answer has streamed, the reservation is replaced with the real
cost read from Claude's usage numbers. If something dies half-way, the worst case stays
counted, which errs on the safe side.

## Files

```
src/index.ts     routing, error handling, daily clean-up (cron)
src/solve.ts     /v1/solve: checks, reservation, Claude call, streaming, settlement
src/prompt.ts    the system prompt and answer format sent to Claude (ported from core/)
src/device.ts    register, pair/start, status, firmware download
src/account.ts   /api/me, claim, rename, revoke
src/billing.ts   subscription checkout, billing portal, store, Stripe webhook
src/stripe.ts    small Stripe client and webhook signature check
src/waitlist.ts  /api/waitlist
src/auth.ts      sign-in links, sessions, Origin/Content-Type guard
src/plan.ts      plan state, monthly usage, cost model
src/admin.ts     firmware publishing
src/dev.ts       /__dev helpers (DEV_MODE on localhost only)
migrations/      database tables
test/            vitest tests
scripts/         publish-firmware.mjs, enroll-board.mjs
```
