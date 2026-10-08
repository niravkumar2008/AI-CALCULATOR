# AI Calculator API (`web/api`)

One Cloudflare Worker that does everything the calculators and the account website need:

- **Calculators** (`/v1/*`): register, pairing codes, status, photo solving (streams Claude's
  answer), firmware updates.
- **Website / portal** (`/api/*`): e-mail sign-in links, linking a calculator with its code,
  renaming and removing calculators, deleting the account.

The exact rules for every endpoint are in `docs/web/openapi.yaml` (the contract) and the
design is in `docs/web/PLAN.md`. Data lives in **D1** (Cloudflare's SQLite) and firmware
images in **Workers KV**. Your Anthropic API key stays on the server; calculators never see it.

Payments (Stripe) come in stage 3: `/api/billing/*`, `/api/store/checkout` and
`/api/stripe/webhook` answer 404 for now. Until then the only plans are the free month
(started when the first calculator is linked) and test overrides.

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
npm run dev                              starts the API at http://localhost:8787
```

Open http://localhost:8787/healthz in a browser: it should show `{"ok":true}`.

- To make real solves, put your Anthropic key in `.dev.vars` (`ANTHROPIC_API_KEY=...`).
  Without it, `/v1/solve` answers 503 "unavailable" and nothing is charged.
- `npm run dev:test` is the same server with `DEV_MODE=1`, which turns on the two test
  helpers `/__dev/outbox` (read the last sign-in link) and `/__dev/user` (set trial/plan).
  They only work on `localhost`, and production never sets `DEV_MODE`.
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
| `REFUSAL_FALLBACK` | `1` | Lets Anthropic retry a refused photo on another model (`0` = off). |
| `TRIAL_DAYS` | `30` | Free month length. |
| `MONTHLY_SOLVES` / `MONTHLY_SPEND_USD` | `300` / `8` | Per-account monthly limits (whichever comes first). |
| `GLOBAL_DAILY_CAP_USD` | `50` | Hard stop for all accounts together, per UTC day. |
| `PRICE_IN_PER_MTOK` / `PRICE_OUT_PER_MTOK` | `2` / `10` | Model price in USD per million tokens, for the limits. Update if the model changes. |
| `UPSTREAM_TIMEOUT_MS` | `60000` | How long to wait for Claude to start answering. |
| `MAIL_FROM` | (empty) | Sender for sign-in e-mails, e.g. `AI Calculator <login@yourdomain>`. |

Secrets (never in git; locally in `.dev.vars`, in production with `npx wrangler secret put NAME`):
`ANTHROPIC_API_KEY`, `RESEND_API_KEY` (sign-in e-mails; without it, e-mails are not sent),
`ADMIN_TOKEN` (enables firmware publishing; without it that endpoint is off).

## 5. Deploy on the FREE plan (workers.dev address)

1. Make a free Cloudflare account and log in: `npx wrangler login`.
2. Create the database and the firmware store, and paste the printed ids into `wrangler.toml`:
   ```
   npx wrangler d1 create ai-calc                  -> database_id
   npx wrangler kv namespace create FIRMWARE       -> id
   ```
3. Create the tables in the real database: `npx wrangler d1 migrations apply DB --remote`
4. Add the secrets:
   ```
   npx wrangler secret put ANTHROPIC_API_KEY
   npx wrangler secret put RESEND_API_KEY     (optional until you want real e-mails)
   npx wrangler secret put ADMIN_TOKEN        (a long random string, for firmware publishing)
   ```
5. Set `PUBLIC_URL` in `wrangler.toml` to the address the site will use, then `npm run deploy`.
   Wrangler prints the address, like `https://ai-calc-api.<you>.workers.dev`.
6. Check it: open `<address>/healthz`.
7. Also set a **monthly spend limit in the Anthropic Console** as a backstop.

Publishing firmware for over-the-air updates:

```
set ADMIN_TOKEN=<the same token>                     (PowerShell: $env:ADMIN_TOKEN="...")
node scripts/publish-firmware.mjs firmware.bin stage14-2026.10.08 https://ai-calc-api.<you>.workers.dev
```

## 6. What costs money, and when

| Item | When you pay |
|---|---|
| Cloudflare Workers, D1, KV | **Free** to build and test. The free plan allows 100,000 requests/day but only 10 ms of CPU per request; big photos can go over that, so buy **Workers Paid ($5/month)** at launch. |
| Domain name | About $10-15/year, when you want `aicalc.yourname` instead of `workers.dev`. |
| Anthropic API | Every real solve (about $0.01-0.03). Capped by the limits above. Tests cost nothing. |
| Resend (e-mail) | Free up to 3,000 e-mails/month (100/day). |
| Stripe | Stage 3; only takes a fee when someone pays. |

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
src/auth.ts      sign-in links, sessions, Origin/Content-Type guard
src/plan.ts      plan state, monthly usage, cost model
src/admin.ts     firmware publishing
src/dev.ts       /__dev helpers (DEV_MODE on localhost only)
migrations/      database tables
test/            vitest tests
scripts/         publish-firmware.mjs
```
