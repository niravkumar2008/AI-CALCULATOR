# Sell-and-subscribe layer: plan

Status: **APPROVED 2026-10-08** with the notes below. The API contract is `docs/web/openapi.yaml`; every build agent codes against it.

## 0. Approved decisions (Nirav, 2026-10-08)

**Updated 2026-10-11 (Nirav):** D3's price is now **$10/month or $100/year** (2 months free); software updates
come **only with the plan**; buyers are adults (18+); the brand is **Proofline** (company: Proofline
Corporation, not registered yet; placeholder domain `proofline.example`, contact `hello@proofline.example`).
The per-account AI spend cap is now **$5** a month (`MONTHLY_SPEND_USD`; 300 solves unchanged; see
`docs/web/GUIDE.md` section 1). The cost and revenue numbers in section 6 below were worked out at $15 and are
out of date. Also changed since this plan was written:
- **Hardware:** the v15 colour-LCD board is ordered (5 samples, then bulk); v14 was never ordered; preorders are
  planned for 2027. Where this plan says "e-paper", read "screen".
- **Calculator menus:** SETUP 9 "Account" (sections 4 and 5) was replaced. MODE 4 now opens **Apps** (AI Solve,
  Type it, History, Notes, Geography, Periodic Table, Settings). The link code is shown under **MODE 4 →
  Settings → Link calculator**. Exam mode turns all apps off. The Worker's revoked message already says
  "Link it again in Apps > Settings."
- **Marketing** never names the AI or payment vendors (only the privacy policy lists processors), and there is
  no parent-targeted content; buyers and account holders are 18+, users 13+.
- **Status:** stages 1, 3 and 4 are built and tested locally (Worker API with Stripe test-mode monthly and
  yearly subscriptions, store, waitlist, 29-page site); nothing is deployed. Stage 2 (connecting the firmware
  to this Worker API; it still talks to the older Python server/proxy) is the open item.

| # | Decision | Nirav's notes, and what they change |
|---|---|---|
| D1 | Cloudflare Workers + D1 | **Spend nothing until it's essential.** Everything is built and tested locally (`wrangler dev`, Miniflare). Firmware images go in **Workers KV**, not R2, because R2 needs a card on file. Rate limits use D1 counters, not a paid binding. Devices use the free `*.workers.dev` address until a domain is bought. The $5 Workers Paid plan (free plan: 10 ms CPU per request) and a domain are the first things to buy, at launch. |
| D2 | No-card free month that starts on first pairing | When the month is over, the calculator must say so plainly. `subscription_inactive` / `DeviceStatus.reason` = "Your free month is over. Subscribe at <domain>/account to keep using AI." (110 characters or fewer). |
| D3 | $15/month, 300 solves and $8 of API spend per account per month, no free tier, $50/day global cap | Keep the price; review it after buyer feedback. |
| D4 | Sonnet 5.5, `max_tokens` 16,000 | STEM and basic finance. |
| D5 | Self-registration on first Wi-Fi connect | Must be simple for non-technical buyers: no step should ask for a token or URL. **Added: automatic firmware updates over Wi-Fi** (stage 2, firmware-agent). Today OTA runs only when the calculator is off and on USB. New rule: check after connecting, at most once a day. Install only when on cable or battery ≥ 3.8 V, never during a solve, and never in exam mode. Keep the two-slot rollback. |

The cost table in section 6 is **per month**.

Based on commit 48d422a (`hardware-stage11-roadmap`). Recon: firmware, repo, hosting stack, provisioning, compliance (summaries in section 10).

## 1. What we are replacing, and what we keep

Today:
- `server/proxy/` is a FastAPI + SQLite prototype (review: "usable as a prototype, not ready to sell").
- Tokens are minted by hand with `admin.py`.
- `/link` is an unauthenticated form keyed only by e-mail.
- The billing webhook is a shared-secret stub, not Stripe.
- There is no rate limit.
- It isn't deployed anywhere.

The board firmware (`firmware-prototype/`) already holds **no Anthropic key**. It talks only to the proxy with a Bearer token. These paths still use a direct key:
- `firmware/`: the breadboard tester. It keeps its key, is never flashed to the board, and is out of scope.
- `sim/win32_main.cpp`, `sim/web/page.html` and `AI_Calc_Simulator.html`: these move to the proxy (sim-agent).

What we keep, so boards flashed today keep working:
- the device paths `/v1/solve` (SSE), `/v1/pair/start`, `/v1/device/status`, `/v1/firmware`, `/v1/firmware/image`
- the headers `Authorization: Bearer`, `x-device-id`, `x-firmware`
- the error shape `{"type":"error","error":{"type","message"}}`, with messages of 110 characters or fewer

## 2. Chosen stack (decision D1)

| Piece | Choice | Why |
|---|---|---|
| API, portal backend, webhooks | **Cloudflare Workers** (TypeScript), one Worker in `web/api` | A 60 s wait on Anthropic costs no CPU. 100 MB body limit. No egress fees. `wrangler dev` runs on Windows without Docker. |
| Database | **Cloudflare D1** (SQLite) | One writer, so the spend counters are consistent. 5 GB is included. SQL migrations live in `web/api/migrations/`. |
| Firmware images | **Workers KV** (free: 1 GB, values up to 25 MiB, no card needed) | Replaces the `/data/firmware` volume. |
| Site and portal | **Cloudflare Pages**, static HTML/CSS plus a little vanilla JS | No framework and no build step. Same domain as the API, so cookies are first-party. |
| Magic-link e-mail | **Resend** (free up to 3,000/month, 100/day) | Simple HTTP API. Cloudflare's own e-mail sending is still beta. |
| Payments | **Stripe** Checkout (hardware and subscription), Billing, Customer Portal, optionally Stripe Tax | Card data never touches our code. |

Domain layout (example `aicalc.example`; update 2026-10-11: the placeholder is now `proofline.example`):
- `aicalc.example`: Pages (site and `/account` portal). The Worker is routed on `aicalc.example/api/*`.
- `api.aicalc.example`: the same Worker, for devices (`/v1/*`).

Libraries:
- `stripe` (official, with the SubtleCrypto webhook verifier)
- `vitest` + `@cloudflare/vitest-pool-workers` for tests
- nothing else unless an agent justifies it

## 3. Data model (D1)

All money is stored as integer **micro-USD**, and all times as integer Unix seconds.

```sql
users(id TEXT PK 'usr_', email TEXT UNIQUE NOT NULL COLLATE NOCASE, created_at, trial_ends_at NULL,
      stripe_customer_id NULL UNIQUE, deleted_at NULL)
login_tokens(token_hash TEXT PK, email, expires_at, used_at NULL)        -- 15 min, single use
sessions(id_hash TEXT PK, user_id, created_at, expires_at)                -- 30 days, HttpOnly cookie
devices(id TEXT PK 'dev_', token_hash TEXT UNIQUE NOT NULL, hw_id, fw_version, user_id NULL,
        label, created_at, last_seen_at, revoked_at NULL)
pair_codes(code TEXT PK, device_id, expires_at, claimed_at NULL)          -- 15 min
subscriptions(id TEXT PK (Stripe sub id), user_id, status, price_id, current_period_end,
              cancel_at_period_end, updated_at)
orders(id TEXT PK (Checkout session id), email, user_id NULL, amount_total, currency,
       status, shipping_json, created_at)
usage(id TEXT PK, device_id, user_id, started_at, state 'reserved'|'settled'|'failed',
      reserved_micro, cost_micro, model, input_tokens, output_tokens)
spend_daily(day TEXT PK 'YYYY-MM-DD', reserved_micro, settled_micro)      -- global cap
stripe_events(id TEXT PK, type, received_at)                              -- webhook idempotency
```

Entitlement is "can this device solve?". All of these must hold:
- the device exists and is not revoked;
- the device is linked to a user;
- the user is in trial (`trial_ends_at > now`), OR a subscription is `trialing`/`active`, OR it is `past_due` and less than 3 days past `current_period_end`;
- the user's monthly solves and spend are under the plan limits;
- the global daily spend is under the cap.

## 4. Device-to-backend protocol

Full contract: `openapi.yaml`. Summary:

1. **First boot or after "Forget account"**: `POST /v1/device/register` with `{hw_id, fw}`.
   - No auth. IP rate limit of 5 per hour.
   - Returns `{device_id, token, pair_code, link_url, expires_at}`.
   - The token is `dt_` + 32 random bytes in base64url. The server stores only its SHA-256. The device stores it in NVS `token` (decision D5).
2. **Pairing**: the device shows the 8-character code and a QR of `link_url?code=XXXX-XXXX`.
   - The owner, logged in to the portal, enters or scans the code.
   - `POST /api/devices/claim` links the device to their account in one atomic `UPDATE … WHERE claimed_at IS NULL AND expires_at > now`. This fixes review item M2.
   - The device polls `GET /v1/device/status` every 5 s while the pairing screen is up, and stops after 15 min.
   - A fresh code comes from `POST /v1/pair/start`.
3. **Solve**: `POST /v1/solve`, with the reply streamed as SSE.
   - **The server owns the system prompt, the model, `max_tokens` and the output schema** (review M3).
   - From the request body it reads only the images (1–2 JPEG/PNG, 6 MB or less decoded) and `tutor`.
   - Both the new short body and the current firmware's Messages-shaped body are accepted; for the old shape every other field is ignored. So boards flashed before stage 2 keep working.
4. **Revocation**: `POST /api/devices/{id}/revoke` sets `revoked_at`.
   - The next call gets `401 device_revoked`, and the device shows "This calculator was removed from its account. SETUP 9 to link again."
   - Re-registering issues a new, unlinked token. A stolen calculator therefore can't solve unless the owner pairs it again.
5. **Limits and the spend cap (fail closed)**. Before calling Anthropic, one D1 batch:
   1. requires no other `reserved` row for this device (at most 1 solve in flight per device);
   2. checks the per-device burst limit (D1 counter, 6 per minute);
   3. adds the **worst-case** cost of the request (input estimate + `max_tokens` × output price) to `spend_daily`. This only succeeds if it stays under `GLOBAL_DAILY_CAP_USD`, and is checked again against the user's monthly limits.
   4. inserts a `reserved` usage row.

   Any D1 error or a failed condition means **no upstream call** (503 `spend_cap` / 429 `quota_exceeded`).

   After the stream ends, the Worker reads `usage` from the SSE `message_delta` and settles the row to the real cost in `ctx.waitUntil`. If the Worker dies first, the reservation stays at worst case, which errs on the safe side.

   Backstop: a monthly spend limit set by hand in the Anthropic Console.
6. **Firmware OTA**: unchanged paths. The image is stored in Workers KV. Publishing is admin-only (`POST /v1/admin/firmware` with an `ADMIN_TOKEN` header, used from a small `web/api/scripts/publish-firmware.mjs`).
   - *Added 2026-10-11:* per-family channels. A device that sends `?family=v15lcd` gets only images published with `--family v15lcd` (KV `meta:v15lcd` / `image:v15lcd`); without `family` it gets the legacy channel as before. A publish with `--rollback` sets `rollback: true` in the manifest so devices accept an older build. `DeviceStatus` also gained optional `pair_code`, `link_url` and `plan_reason`, so the firmware no longer has to parse `reason`.

TLS: the device trusts a fixed 7-root bundle (GTS, ISRG X1/X2, DigiCert, Amazon). Cloudflare Universal SSL may issue from Let's Encrypt, Google or SSL.com. So:
- the firmware-agent adds the SSL.com roots to `root_ca.h` in stage 2 (and fixes the stale "api.anthropic.com" comment);
- stage 1's manual steps include checking which CA signed the live certificate.

Token theft: the token sits in NVS without encryption. Flash encryption would burn security eFuses, which `FINAL_STATUS` forbids. Mitigations:
- revoke from the portal;
- 1 solve in flight per device and 6 per minute;
- monthly per-user limits;
- `last_seen_at` and `x-device-id` shown in the portal, so the owner can spot a clone.

## 4b. Shared build conventions (backend ↔ tests)

`web/api` (backend-agent):
- TypeScript, `wrangler`, D1 binding `DB`, KV binding `FIRMWARE`.
- `npm test` runs vitest with `@cloudflare/vitest-pool-workers`.
- `npm run dev` runs `wrangler dev --local` on port 8787.
- Migrations live in `web/api/migrations/NNNN_*.sql`, applied with `npm run db:migrate:local`.

Settings: secrets go in `.dev.vars`, which is git-ignored. `.dev.vars.example` is committed with blank values.

| Name | Default | Purpose |
|---|---|---|
| `ANTHROPIC_API_KEY` | (secret) | Upstream key, server-side only |
| `ANTHROPIC_BASE_URL` | `https://api.anthropic.com` | Tests point this at a mock |
| `MODEL` | `claude-sonnet-5-5` | |
| `MAX_TOKENS` | `16000` | |
| `PUBLIC_URL` | `http://localhost:8787` | Used to build `link_url` |
| `TRIAL_DAYS` | `30` | |
| `MONTHLY_SOLVES` | `300` | |
| `MONTHLY_SPEND_USD` | `8` | |
| `GLOBAL_DAILY_CAP_USD` | `50` | |
| `PRICE_IN_PER_MTOK` / `PRICE_OUT_PER_MTOK` | `2` / `10` | USD, used for reservations and settlement |
| `RESEND_API_KEY` | (secret, optional) | Unset means e-mails go to the dev outbox only |
| `MAIL_FROM` | | |
| `ADMIN_TOKEN` | (secret, optional) | |
| `DEV_MODE` | unset | |

`DEV_MODE=1` enables `/__dev/*`. It is honoured **only** when the request host is `localhost` or `127.0.0.1`, and the production config never sets it. Two endpoints:
- `GET /__dev/outbox?email=` returns `{link, token}` for the last sign-in e-mail.
- `POST /__dev/user {email, trial_ends_at?, plan_override?}` sets trial or plan state for tests.

Mock Anthropic:
- `web/tests` runs a small Node HTTP server that answers `POST /v1/messages` with a canned SSE stream, including `message_start.usage` and `message_delta.usage`.
- It can fail on request: 500 before the stream, or a mid-stream error. It is triggered by a marker in the image data.

Contract tests: `web/tests` (test-agent). Node built-in `node:test`, black-box over HTTP against `BASE_URL`. Responses are validated against `openapi.yaml` with `ajv`. `npm test` starts the mock and `wrangler dev`, then runs the tests.

Fixed device-facing messages (110 characters or fewer):

| Error type | Message |
|---|---|
| `subscription_inactive` (trial over) | "Your free month is over. Subscribe at <PUBLIC_URL host>/account to keep using AI." |
| `subscription_inactive` (subscription ended) | "Your subscription has ended. Renew at <host>/account to keep using AI." |
| `device_not_linked` | "Link this calculator: go to <host>/link and enter code XXXX-XXXX" |
| `device_revoked` | "This calculator was removed from its account. SETUP 9 to link again." (Update 2026-10-11: now "Link it again in Apps > Settings.") |
| `quota_exceeded` | "Monthly AI limit reached. It resets on the 1st." |
| `spend_cap` | "AI is busy right now. Please try again later." |

## 4c. Security review stage 1: decisions (2026-10-08)

**S1 (HIGH), fake devices farming free months.** Fixed by **board enrolment**:
- During the first USB flash at QA (already a required step), `scripts/enroll-board.mjs` creates a random `hw_secret`, writes it to the calculator's NVS namespace `factory` over serial, and registers its hash with `POST /v1/admin/hardware`.
- Customers still do nothing; self-registration (D5) is unchanged.
- Only enrolled boards can start a free month, once per board ever, once per e-mail ever (kept as a hash after account deletion; this closes S2), and at most 20 new trials a day.
- Unenrolled boards (dev boards, fakes) can link but must subscribe.

**Other findings:**

| Finding | Fix |
|---|---|
| S3 | `REFUSAL_FALLBACK` defaults to `0` until fallback pricing is added |
| S4 | Rate-limit counters stop writing once over the limit. A free Cloudflare WAF rule goes on register and login (manual step). |
| S5 | Optional Turnstile check on login (`TURNSTILE_SECRET`; the widget arrives with the stage 2 portal) and a global daily mail cap of 80 |
| S6 | The linked/entitled and burst checks run before the body is read |
| S8 | A non-https `PUBLIC_URL` is refused unless `DEV_MODE` is on |
| S9 | The publish/enrol scripts refuse http except to localhost |
| S10 | `ADMIN_TOKEN` must be ≥ 32 chars (else admin stays off) |
| S12 | Total stream time capped at 290 s, plus a cron job that settles stale reservations |
| S13 | Portal (stage 2): `Referrer-Policy: no-referrer` on `/login/confirm` |
| S11 | Firmware signing with Ed25519 (the public key compiled into firmware; no eFuses) moves to stage 2 with the auto-update work |

## 5. Provisioning flow (keypad + e-paper only; phone optional)

Two ways to get Wi-Fi, both from the existing menus:

**SETUP (SHIFT MODE) → 6 "Wi-Fi"** opens a sub-menu:
1. **Type on keys** (new; the zero-phone, zero-computer path)
   - Scan, then pick a network with ▲▼, or choose "Other…" and type the name.
   - Password entry: multi-tap text entry, a new `TextEntry` view in core. Digit keys cycle letters as on a phone keypad; ALPHA cycles lower / UPPER / symbols; DEL deletes; ◀▶ move the cursor; = saves.
   - Each character shows for 1 s, then is masked. SHIFT+▶ toggles "show password".
2. **Set up with phone** (existing SoftAP captive portal)
   - Adds a **Wi-Fi QR** (`WIFI:T:WPA;S:AI-Calc-XXXX;P:…;;`) drawn at native resolution, so the phone joins without typing.
   - The proxy URL and token fields are removed from the phone form, because the token now comes from registration.
3. **Forget Wi-Fi**.

**MODE → 4:AI**, then =, walks through whatever is missing, in order:
1. No Wi-Fi: "1:Type on keys 2:Use phone" (jumps to the sub-menu above).
2. Wi-Fi fails: a plain reason ("Wrong password", "Network not found, 2.4 GHz only", "School login pages not supported, use a phone hotspot").
3. Not registered: registers silently.
4. Not linked: shows the code in large text, the QR of the link URL, and "Go to aicalc.example/link on any phone or computer". It polls until linked.
5. No plan: "Free month used up. Subscribe at aicalc.example/account".

Linking an account needs *some* browser, because it binds the calculator to a paying person. It can be a friend's phone, a school computer or a library computer. Wi-Fi itself needs only the keys.

*(Update 2026-10-11: SETUP 9 became MODE 4 → Settings → Link calculator; the domain is `proofline.example`. See section 0.)*

**SETUP 9 "Account"** (new): shows the linked e-mail (masked, e.g. `n***@gmail.com`), plan and solves left. Option 1 is "Unlink and get new code".

**Exam mode stays exactly as it is.**
- SHIFT+7+ON, or SETUP 8. Wi-Fi, camera and AI are off.
- It ends after 12 h, or with the serial `exam off` over the cable.
- Every new provisioning screen checks `radiosAllowed()`, and the menu entries are blocked in exam mode just as MODE 4 is today.
- Recon note: the "unlock by cable" is the serial `exam off` command, not VBUS detection. We do **not** change that; the regression test pins the current behaviour.

## 6. Cost table (per month)

Assumptions:
- 150 solves per subscriber per month on average (from `LAUNCH_ROADMAP`).
- About $0.026 per solve on Sonnet 5.5 with thinking (repo estimate; the bare vision call is about $0.009).
- $15 plan; Stripe fees 2.9% + $0.30 + 0.7% Billing ≈ $0.84, +$0.075 with Stripe Tax ≈ $0.92.
- Prices checked 2026-10-08.

| | 50 subs | 500 subs | 5,000 subs |
|---|---|---|---|
| Cloudflare Workers Paid (incl. D1, KV) | $5 | $5 | $5–10 |
| Resend | $0 | $0 | $20 |
| Domain | ~$1 | ~$1 | ~$1 |
| **Infrastructure** | **≈ $6** | **≈ $6** | **≈ $26–31** |
| Anthropic API (150 × $0.026) | $195 | $1,950 | $19,500 |
| Stripe + Stripe Tax | $46 | $460 | $4,600 |
| **Total cost** | **≈ $247** | **≈ $2,416** | **≈ $24,130** |
| Revenue at $15 | $750 | $7,500 | $75,000 |

Worst case per subscriber: at the 500-solve cap the API cost is $13 of the $15. So D3 adds a **dollar** cap per account as well as a solve count.

Not monthly, and outside this layer:
- $225 hardware sale: Stripe fee ≈ $6.83
- compliance one-offs (section 9): FCC Part 15B ≈ $1.5k–3.5k, lawyer review $0.5k–2.5k

## 7. Decisions for Nirav (recommended default first)

**D1. Backend platform.**
- **Default: rewrite the proxy on Cloudflare Workers + D1** (about $6/month, nothing to keep running, scales to 5,000 without changes).
- Alternative: keep the FastAPI code and deploy it on Render or Fly with a SQLite volume (about $7–25/month). It is one instance and you have to do the backups.
- The device protocol is identical either way.

**D2. Free month: no card vs card up front.**
- **Default: no card.** The 30-day trial starts automatically on first pairing (as the prototype does now). After it, the customer subscribes in Stripe Checkout.
- No automatic free-to-paid conversion means much less exposure under California's auto-renewal law.
- Alternative: Checkout with `trial_period_days=30` and a card required. Conversion is higher, but you need the reminder e-mail and consent records.

**D3. Plan limits.**
- **Default: $15/month, 300 solves and $8 of API spend per account per month (whichever comes first), no free tier, global cap $50/day.**
- All of these are settings and Stripe Prices, so changing them later needs no code change.
- The competitor research still suggests a free daily tier plus about $5/month; this layer supports that by changing settings.

**D4. Model.**
- **Default: `claude-sonnet-5-5`, adaptive thinking at effort high, `max_tokens` 16,000** (was 32,000). That halves the worst-case reservation.
- Alternatives: Opus 5.5 (review S1; about 2.5× the cost), or `haiku-first` routing.

**D5. How devices get tokens.**
- **Default: self-registration on first connect.** No USB step at the factory. A reset or revoke just re-registers, and an unlinked token can't spend money.
- Alternative: mint tokens at end-of-line QA over USB. This is tighter against fake devices but adds a factory step, and a factory reset then needs a computer.

## 8. Build stages, ownership, delivery

| Stage | Zip | Agents (own directories) | Contents |
|---|---|---|---|
| 1 | `ai_calc_web_1.zip` | backend-agent (`web/api`), test-agent (`web/tests`), security-reviewer | Worker, migrations, device register/pair/status/solve, spend cap, rate limit, firmware OTA on KV, magic-link auth, device claim/revoke API, contract tests (mock Anthropic) |
| 2 | `ai_calc_web_2.zip` | firmware-agent (`firmware-prototype`, `core`), portal-agent (`web/portal`), sim-agent (`sim`), test-agent | Keypad text entry, Wi-Fi sub-menu, QR codes, register/pair flow, SETUP 9, roots update, removal of the "API key" wording; portal login, pair and revoke pages; sims on the proxy; exam-mode regression test |
| 3 | `ai_calc_web_3.zip` | backend-agent, portal-agent, test-agent | Stripe Checkout (subscription and hardware), webhooks with signature check and idempotency, Customer Portal, plan/usage view, e2e in Stripe test mode |
| 4 | `ai_calc_web_4.zip` | site-agent (`web/site`), test-agent | Landing, product, pricing, FAQ, legal templates marked "NEEDS LAWYER REVIEW", accessibility check |

Rules for every agent:
- Each works in its own git worktree.
- Code to `openapi.yaml`. Propose changes to the contract to the lead, never edit it yourself.
- Commit small. Report what was tested and what wasn't.

The security-reviewer reads each agent's output (token theft, replay, spoofing, webhook forgery, cost attacks, secrets). A HIGH finding blocks the zip.

The lead merges the work and runs the whole suite before each zip:
- `web/api` vitest
- the contract tests
- the C++ core tests: they need a compiler (none is on PATH here), so stage 1's manual steps include installing one, or we use `winget install -e --id MSYS2.MSYS2`
- `pio run` for the firmware

Each zip contains `CHANGELOG.md`, `TEST_RESULTS.md` and `MANUAL_STEPS.md`, and goes in `Claude outputs/`. Nirav commits and pushes.

Retired after stage 3: `server/proxy/`. It is kept until the Worker is live, then marked superseded in its README rather than deleted.

## 9. Compliance items that affect this layer (not legal advice)

Built into the plan:
- Privacy policy, ToS, refund and warranty templates are marked **NEEDS LAWYER REVIEW**.
- Photos are not stored, only usage numbers; this is disclosed, with Anthropic named as a subprocessor.
- Account deletion is in the portal (`DELETE /api/me`).
- The Stripe Customer Portal gives "click to cancel".
- An "account holder must be 18+ (parent buys)" checkbox at checkout and in the ToS. (Update 2026-10-11: the copy says "18 or older" and never mentions parents.)
- The site and FAQ say plainly that the device is **not allowed on the SAT/ACT/AP** (it has Wi-Fi and a camera), and never claim it is exam-approved.
- No Casio name or logo in the branding.

For Nirav, outside the code:

| Item | Who |
|---|---|
| Stripe account holder (a guardian if under 18) | lawyer / guardian |
| LLC | lawyer / accountant |
| Sales-tax registration | accountant |
| COPPA and the Anthropic minors policy | lawyer |
| FCC Part 15B SDoC and module labelling | test lab |
| UN38.3 summary from the cell supplier | supplier |
| Trademark search | lawyer |

## 10. Recon summary (2026-10-08)

**Firmware**
- Arduino-ESP32 2.0.17 / IDF 4.4.7 on PlatformIO `espressif32@6.9.0`.
- OTA slots of about 1.9 MB, with about 1.3 MB used.
- Wi-Fi, proxy and token are stored as plaintext in NVS (`settings.cpp`).
- Captive portal in `setup_portal.cpp`; menus hard-coded in `core/device.cpp`.
- **No text entry exists**.
- Exam mode is all in core: `kExamMaxMs`, unlocked by serial `exam off`.
- Tests: `tests/tests.cpp` + 42 goldens. No compiler on PATH on this PC.

**Repo**
- No CI, no JS tooling.
- The proxy is FastAPI and SQLite, with tokens stored as SHA-256. It has a stub billing webhook, no rate limit, and is not deployed.
- No secrets are tracked; `api_key.txt` exists only, git-ignored, in the main checkout.

**Stack**
- Workers $5 flat up to 5,000 subs; Supabase $25+; Vercel Pro + Neon $20–110.
- Anthropic network waits don't count as Workers CPU time.

**Provisioning**
- The SoftAP portal is the only no-app option that works on both iOS and Android.
- BLE needs an app on iOS and adds about 150 KB.
- SmartConfig can't use the phone's own hotspot.
- Keypad entry is the only option with no phone at all.

**Compliance**: see section 9. The top risks are exam-policy marketing, a minor as the Stripe owner, and COPPA/minors.
