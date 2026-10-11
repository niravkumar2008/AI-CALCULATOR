# AI Calculator proxy

A small web server that sits between the calculators and Claude. **It holds the Claude API key; the
calculators never do.** Each calculator has its own device token instead.

```
calculator --(HTTPS, device token)--> proxy --(HTTPS, API key)--> Claude API
```

What it does for every AI solve:
1. Checks the device token (stored only as a hash; revocable).
2. Checks the calculator is linked to an account, and the account's subscription: **trial** (first month
   free from linking, one free month per calculator), **active**, **past_due** (card failed: a few days'
   grace), **canceled**. Optional free tier: `FREE_DAILY`.
3. Enforces fair use: a monthly cap per tier (`MONTHLY_CAP` 500 base, `PRO_MONTHLY_CAP` 280 pro) and
   per-calculator burst limits (1 solve at a time, 4 a minute, 40 an hour). Only finished answers count.
4. Keeps only the photos and the short instruction from the calculator's request. **The system prompt and
   the JSON schema are the server's** (`prompts/solver_v1.txt`, `prompts/solver_v1.schema.json`), so a
   prompt fix needs no firmware flash and a stolen token can't be used for anything but photo solves.
   The v15 firmware no longer uploads `system` at all (`CALC_SEND_SYSTEM_PROMPT=0`, about 5 KB less per
   solve); the v14 firmware still does, so it also works with an older proxy. `PROMPT_SOURCE=device`
   (bench testing the prompt in `core/claude_api.cpp`) needs a firmware built with `-DCALC_SEND_SYSTEM_PROMPT=1`.
5. Picks the model by tier and streams Claude's reply back event by event, with a `: ping` comment every
   15 s while Claude thinks, so the calculator's 120 s idle timeout never fires.

**Models** (prices from the Claude API price list; per-user costs from
`hardware/production/UNIT_ECONOMICS.md`):

| Tier | Model | $ per M tokens in / out | Typical user / month | At the cap |
| --- | --- | --- | --- | --- |
| base (the $15 plan) | `claude-sonnet-5-5` | $2 / $10 | about $3.77 | $12.40 at 500 |
| pro (optional) | `claude-opus-5-5` | $4 / $20 | about $7.52 | about $13.84 at 280 ($24.72 at 500) |

- **Fallback:** if the tier's model answers 429 / 5xx / 529 overloaded (or can't be reached), the proxy tries
  the other model once before the calculator shows "busy" (`MODEL_FALLBACK=1`).
- **Prompt caching:** the fixed system prompt (about 1,500 tokens) carries a `cache_control` breakpoint, so
  repeat solves within 5 minutes read it from the cache at a tenth of the input price. The log line of
  every solve shows `cache_read=` / `cache_write=` so you can see it working.
- **Refusal fallback** (`REFUSAL_FALLBACK=1`): the Claude API's server-side fallback (beta
  `server-side-fallback-2026-07-01`, `fallbacks: "default"`), so a harmless homework photo that a safety
  filter declines is answered by another model instead of showing an error. Set 0 if your account rejects it.
- Effort is always sent explicitly (Opus 5.5 would otherwise default to `medium`).
- Optional **Haiku-first routing** (`ROUTING=haiku-first`): tries `claude-haiku-4-5` first and keeps its
  answer only if it is confident and clean.

## Files

| File | What |
| --- | --- |
| `app.py` | the web server (FastAPI): every endpoint, request ids, the SSE relay with keep-alive |
| `solve.py` | checks the calculator's request, the server's prompt, model routing + fallback, prompt caching |
| `policy.py` | who may solve: link, subscription state, tier cap, free tier (messages shown on the calculator) |
| `limits.py` | per-calculator burst limits (in memory: run one worker) |
| `linking.py` + `mailer.py` | the e-mail confirmed link flow (signed, short-lived tokens; SMTP) |
| `billing.py` | Stripe-signed webhook check + subscription events -> accounts |
| `db.py` | SQLite: devices, accounts, pairing codes, usage, billing events (adds new columns itself) |
| `logs.py` | request ids and redaction (no tokens, keys, photos or links in the log) |
| `firmware.py` | over-the-air updates: the published image + manifest, board-family check |
| `admin.py` | command line: devices, tiers, accounts, paid, prompt version, firmware publish |
| `prompts/` | the versioned system prompt and JSON schema |
| `config.py` + `.env.example` | every setting |
| `test_proxy.py`, `test_hardening.py` | tests: `python -m pytest -q` (offline: a fake Anthropic client, no key) |

## Run it on your PC (to try it)

```
cd server/proxy
python -m venv .venv
.venv\Scripts\activate            (Windows)      or   source .venv/bin/activate
pip install -r requirements.txt -r requirements-dev.txt
python -m pytest -q                (all tests, no network)
copy .env.example .env             then edit .env: ANTHROPIC_API_KEY, LINK_SECRET; MAIL_BACKEND=console
python admin.py new-device "board 1"
uvicorn app:app --port 8000
```
On your PC, `MAIL_BACKEND=console` plus `MAIL_LOG_LINKS=1` prints the confirmation link in the terminal
instead of sending an e-mail (never on the real server). The calculator needs **HTTPS**, so for a real
test use a hosted option below (or `cloudflared tunnel --url http://localhost:8000`).

## Deploy (Render, Railway or Fly.io)

The `Dockerfile` is ready (Python 3.12, non-root user, **one** uvicorn worker: the burst limiter is in
memory and SQLite has one writer; one small instance serves thousands of calculators because each solve
mostly waits for Claude).

1. **Create the service from this folder** (`server/proxy`):
   - **Render:** New -> Web Service -> your repo, Root Directory `server/proxy`, Runtime Docker. Add a
     **Disk** mounted at `/data` (1 GB is plenty). Instance: Starter. Health check path `/healthz`.
   - **Railway:** New Project -> Deploy from repo, set the service's Root Directory to `server/proxy`.
     Add a **Volume** mounted at `/data`.
   - **Fly.io:** `cd server/proxy`, `fly launch --no-deploy` (internal port 8000), then
     `fly volumes create data --size 1` and in `fly.toml` add `[mounts] source="data" destination="/data"`.
     Keep exactly one machine (`fly scale count 1`). `fly deploy`.
2. **Set the environment variables** in the host's dashboard (Render: Environment; Railway: Variables;
   Fly: `fly secrets set NAME=value`). Never in git. The required ones are in the table below.
3. Open `https://<your address>/healthz`: `{"ok": true, "version": ...}`. That address is your `PUBLIC_URL`
   (set it, redeploy).
4. **Stripe:** Developers -> Webhooks -> add endpoint `https://<your address>/v1/billing/webhook`, events
   `checkout.session.completed`, `customer.subscription.created`, `customer.subscription.updated`,
   `customer.subscription.deleted`, `invoice.paid`, `invoice.payment_failed`. Copy its signing secret
   (`whsec_...`) into `STRIPE_WEBHOOK_SECRET`. In Checkout, pass the customer's e-mail (or
   `metadata.email`) and start the subscription with `subscription_data.trial_end` = the account's `trial_ends_at` (the
   first month is already free on the proxy). Give the pro price the lookup key `calc_pro_monthly` (or
   list yours in `PRO_PRICE_KEYS`).
5. Set a **monthly spend limit** on the Claude Console (last line of defence).

The calculator trusts Let's Encrypt, Google Trust Services, DigiCert and Amazon certificates, which covers
all three hosts.

## Environment variables

Required in production:

| Variable | What |
| --- | --- |
| `ANTHROPIC_API_KEY` | the Claude API key (only here) |
| `PUBLIC_URL` | the proxy's https:// address (shown with the pairing code, used in e-mails) |
| `LINK_SECRET` | 32+ random characters (`python -c "import secrets;print(secrets.token_urlsafe(32))"`): signs the e-mail links |
| `MAIL_BACKEND=smtp`, `SMTP_HOST`, `SMTP_PORT`, `SMTP_USER`, `SMTP_PASSWORD`, `MAIL_FROM` | where the confirmation e-mails go out (any SMTP provider: Postmark, Resend, SES, Mailgun...) |
| `STRIPE_WEBHOOK_SECRET` | the webhook's signing secret (`whsec_...`) |

Useful knobs (defaults in brackets): `BASE_MODEL` [claude-sonnet-5-5], `PRO_MODEL` [claude-opus-5-5],
`MODEL_FALLBACK` [1], `CLAUDE_MAX_RETRIES` [1 per model], `PROMPT_CACHE` [1], `REFUSAL_FALLBACK` [1],
`ROUTING` [main], `PROMPT_SOURCE` [server], `SYSTEM_PROMPT_FILE` / `SCHEMA_FILE` [prompts/solver_v1.*],
`MONTHLY_CAP` [500], `PRO_MONTHLY_CAP` [280], `TRIAL_DAYS` [30], `PAST_DUE_GRACE_DAYS` [3], `FREE_DAILY` [0],
`SOLVE_CONCURRENCY` [1], `SOLVES_PER_MINUTE` [4], `SOLVES_PER_HOUR` [40], `OTHER_PER_MINUTE` [30],
`LINK_PER_MINUTE_PER_IP` [10], `KEEPALIVE_SECONDS` [15], `LINK_MODE` [email], `LINK_CONFIRM_MINUTES` [30],
`PAIR_CODE_MINUTES` [15], `WEBHOOK_TOLERANCE_S` [300], `PRO_PRICE_KEYS`, `ADMIN_TOKEN` [off],
`DB_PATH` [/data/proxy.db in Docker], `FIRMWARE_DIR` [/data/firmware in Docker], `APP_VERSION`, `LOG_LEVEL`.
`MAIN_MODEL` still works as the old name of `BASE_MODEL`.

## Admin commands

Run them in the host's shell so they use the database on `/data` (Render: the service's **Shell** tab;
Fly.io: `fly ssh console`, then `cd /app`; Railway: the service's shell / `railway ssh`).

```
python admin.py new-device "Nirav board 1"      a device token, shown ONCE (type it into the calculator: token)
python admin.py devices                         every calculator, its account, revoked?
python admin.py revoke dev_1a2b3c               a lost calculator stops working at once
python admin.py tiers                           models (with fallback), caps and limits per tier
python admin.py tier someone@example.com pro    move an account to pro (Opus) or base (Sonnet)
python admin.py account someone@example.com     subscription state, tier, solves this month
python admin.py paid someone@example.com 2026-12-31   mark paid by hand (without Stripe)
python admin.py usage                           solves per account this month
python admin.py prompt                          the prompt version the proxy sends (also in every solve's log line)
python admin.py firmware <firmware.bin> <version>   publish an over-the-air update for that version's board family
python admin.py firmware                        what is published now, per board family
```
With `ADMIN_TOKEN` set, devices can also be created over HTTP:
`curl -X POST "https://<proxy>/v1/admin/devices?label=board%201" -H "x-admin-token: <ADMIN_TOKEN>"`.

**Changing the prompt:** add `prompts/solver_v2.txt` (and a schema file if the reply format changes, which
also needs a firmware change), set `SYSTEM_PROMPT_FILE`, redeploy. The version (`solver_v2+<hash>`) shows
in every solve's log line.

## Set up a calculator

1. `python admin.py new-device "..."` on the server -> copy the token.
2. Calculator on USB, Serial Monitor: `proxy` -> paste `https://your-proxy-address`; `token` -> paste the
   token; `wifi` -> hotspot name and password (or the phone setup page, `firmware-prototype/README.md` §2).
3. Link it: `pair` prints a 6-letter code (an unlinked calculator also shows it when you try an AI solve).
   Open `https://your-proxy-address/link`, type the code and your e-mail. **Open the e-mail and press the
   button**: only then is the calculator linked and the free month starts.
4. `account` on the calculator shows the subscription, tier and solves used this month.

## Firmware updates over the air

Switched off on a cable, a calculator asks `GET /v1/firmware` (device token, its version in `x-firmware`);
when the answer is `"update": true` it downloads `/v1/firmware/image`, checks the SHA-256 and installs it
into its second app slot (rollback if the new one crashes). The board family (`stage14-...` = v14 e-paper,
`v15lcd-...` = v15 LCD) must match, on the manifest and on the image download.

```
python admin.py firmware ../../firmware-prototype/.pio/build/prototype/firmware.bin stage14-2026.10.10
python admin.py firmware ../../firmware-v15-lcd/.pio/build/v15lcd/firmware.bin v15lcd-2026.10.10
python admin.py firmware          (what is published, per family)
```
One image **per board family** (`FIRMWARE_DIR/<family>/firmware.bin` + `firmware.json`; the version's part
before the first `-` picks the folder), so v14 and v15 calculators can be offered updates at the same time;
publishing one family's build never touches the other's. A calculator is only ever offered, and only ever
downloads, its own family's image (by `x-firmware`, which every firmware sends); a request without
`x-firmware` gets the image only when exactly one family has one. An image published by an older proxy
(`FIRMWARE_DIR/firmware.bin`) is still served to its own family until that family is published again.

## Errors the calculator understands

`401 device_unknown`, `402 device_not_linked` (message carries the pairing code), `402 subscription_inactive`
(free month over / payment failed / subscription ended), `429 fair_use_exceeded` (monthly cap),
`429 rate_limited` (burst limits, with `retry-after`), `429 free_tier_exhausted`. The calculator shows these
messages (<= 110 characters). `503 overloaded_error` (and an SSE `error` event if Claude drops mid-answer)
shows "busy".

`rate_limited` is only shown by firmware of 2026-10-10 or later (`core/claude_api.cpp` classifyFailure).
The rule (`policy.rate_limit_error_type`): if `x-firmware` reads `<family>-YYYY.MM.DD...` with a known family
(`stage14`, `v15lcd`) and a date on or after `RATE_LIMITED_SINCE` (2026.10.10 for both), the burst limit is
sent as `rate_limited`; anything else (older builds such as `stage14-2026.10.06` / `v15lcd-2026.10.08`, an
unknown board, no or unreadable header) gets `fair_use_exceeded` with the same message, which every
firmware displays. Guessing "old" is always safe; only the type name differs.

**Early failures are retried:** if Claude's stream fails before the first content event (an `error` event
such as `overloaded_error` / `api_error` / `rate_limit_error`, or the connection drops / ends right after
`message_start`), the proxy has only sent `: ping` comments so far, so it quietly opens the request again on
the other model (`MODEL_FALLBACK`). Once content has started, a failure is passed on as before ("busy"): the
calculator has already shown part of the answer and its `StreamReader` appends text, so a second answer
would be glued onto the first (and the first one's tokens are already paid for).

## Security checklist (before selling)

- [ ] `ANTHROPIC_API_KEY`, `LINK_SECRET`, `STRIPE_WEBHOOK_SECRET`, `SMTP_PASSWORD`, `ADMIN_TOKEN` only in the
      host's secret settings; `.env` and `api_key.txt` are git-ignored, never copied anywhere.
- [ ] Spend limit set on the Claude Console; usage alerts on.
- [ ] `LINK_MODE=email` (default) and SMTP working: link a test calculator end to end.
- [ ] Stripe webhook test event returns 200; an edited payload returns 400.
- [ ] `ADMIN_TOKEN` empty unless needed (admin.py in the host shell is enough).
- [ ] One instance, one worker, a volume on `/data`; back up `/data/proxy.db` (Render/Fly snapshots).
- [ ] `/healthz` shows only ok/version; the logs show device ids, models and token counts, never tokens,
      photos, answers or e-mail links (checked by the tests).
- [ ] Revoke lost calculators with `admin.py revoke`.
- [ ] Root CA bundle in the firmware refreshed before 2031 (DigiCert Global Root CA expiry).
