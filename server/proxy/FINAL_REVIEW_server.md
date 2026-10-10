# Server-side review: server/proxy (stopped early, 2026-10-06 ~01:35 USEDT)

Review of the FastAPI proxy and of how the firmware talks to it. Work was cut short by the
coordinator to save usage: **no files in `server/` were modified**. Everything below is findings;
the "Changes made" list is empty on purpose.

## Verdict

**Usable as a prototype, not ready to sell.** The proxy design is right (API key only on the
server, hashed per-device tokens, request whitelisting, monthly cap, streamed pass-through) and the
stage-13 firmware (`firmware-prototype/`) matches it. The blockers are on the firmware side
(`firmware/` still holds the Claude API key) and in the model/fallback defaults and the link page.

## Findings

### FATAL
- **F1. `firmware/src/claude_client.cpp` bypasses the proxy.** It connects to `api.anthropic.com`
  with an `x-api-key` read from NVS (`firmware/src/settings.cpp`, key `"key"`; `firmware/src/main.cpp:85`).
  That is exactly what the business model forbids (device must never hold the key). The
  proxy-aware client exists only in `firmware-prototype/src/claude_client.cpp`. See "Firmware
  changes requested".

### SERIOUS
- **S1. Default model.** `config.py` defaults `MAIN_MODEL=claude-sonnet-5-5`. Requested default is
  `claude-opus-5-5` with `claude-sonnet-5-5` as the fallback. Not changed. Notes for whoever does it:
  Opus 5.5's default `effort` is `medium` (the proxy already forwards the device's high/xhigh/max, so
  that is fine); `thinking: {type: "adaptive"}` is accepted; the refusal fallback must stay the
  `"default"` scalar form with beta `server-side-fallback-2026-07-01` (already what `solve.py` sends;
  the array form needs the `-2026-06-01` header and the two must not be mixed). A plain
  "Sonnet if Opus is down" fallback (429/529/5xx) is **not** implemented: `app.py` returns 503 and
  the device shows "busy"; a second attempt on `FALLBACK_MODEL` before giving up is the missing piece.
- **S2. No per-device burst/rate limit.** Only the monthly cap (500) exists. A stolen token can burn
  the whole month in minutes, and nothing limits concurrent streams per device. Add a per-device
  token bucket (e.g. 6/min, 1 in flight) in `policy.py`.
- **S3. `/link` is unauthenticated.** Anyone with a pairing code can link a calculator to any e-mail
  (README already says so). Also hand-rolled form parsing and no CSRF. Must be behind a sign-in
  (magic link) before launch.
- **S4. Billing is a shared-secret stub.** `/v1/billing/webhook` takes `{email, paid_until}` with
  `x-billing-secret`; it is not a Stripe signature check. Fine as the documented stub; do not expose
  it as "Stripe" yet.
- **S5. Haiku-first path counts a solve before any byte is sent** and the main path counts only on
  `message_stop`. Consistent enough, but the Haiku path also has no fallback when `try_haiku`
  returns `None` because of a key/connection error (it silently proceeds to the main model, which
  then fails the same way: fine, but doubles the latency on an outage).

### MINOR
- M1. `GET /healthz` reveals the model and routing mode; make it `{"ok": true}`.
- M2. `db.note_hw_id` does an UPDATE on every request; cheap but needless. `claim_pair_code` uses
  two connections (small race: a code could be claimed twice).
- M3. `MAX_SYSTEM_CHARS=20000` but the proxy still forwards the *device's* system prompt. Safer: the
  proxy owns the prompt (ignore the device's `system`), so prompt fixes do not need a firmware flash.
- M4. Device idle timeout is 120 s (`kIdleMs`); SDK timeout is 10 min. With streaming and adaptive
  thinking the stream keeps sending `thinking` events, so this normally holds; a keep-alive SSE
  comment every 30 s from the proxy would make it robust.
- M5. Logs: only device ids are logged (good). uvicorn access logs include the path only (tokens are
  in headers). CORS: no middleware = off (good). Docs/redoc off (good).

### Verified OK (device <-> proxy contract)
- Firmware sends the Messages-API body from `core/claude_api.cpp::buildSolveRequest` plus
  `"tutor":true` appended by string; `solve.clean_request` whitelists exactly that (1-2 base64 JPEG/PNG,
  one text block, `output_config.format` json_schema, `effort` in {high,xhigh,max}) and strips `tutor`.
- Error contract: proxy JSON `{"type":"error","error":{"type","message"}}`; firmware
  `classifyFailure` maps `device_unknown / device_not_linked / subscription_inactive /
  fair_use_exceeded / free_tier_exhausted` to `Failure::Account` with the message (<=110 chars, which
  `policy.py` respects). 401 without `device_unknown` -> "token not accepted". 429/5xx/529 -> busy.
- Streaming: SSE passed through byte-for-byte; Haiku answers are re-synthesised as a valid Messages
  event stream (`synth_stream`), ending in `message_stop` as the firmware requires.
- TLS: `firmware/src/root_ca.h` and `firmware-prototype/src/root_ca.h` pin GTS Root R1/R4 (exp 2036),
  ISRG Root X1 (2035) / X2 (2040), DigiCert Global Root G2 (2038), DigiCert Global Root CA (2031),
  Amazon Root CA 1 (2038). Fly.io and Railway issue Let's Encrypt certs (ISRG X1 chain) -> covered.
  Earliest expiry in the bundle: DigiCert Global Root CA, Nov 2031. Plan a firmware CA refresh before then.
- Existing tests: `test_proxy.py` (9 tests) pass under Python 3.14 with fastapi 0.142 /
  anthropic 1.11 / pytest 9.1 (`python -m pytest test_proxy.py`). No HTTP-level test with a mocked
  Anthropic client exists yet (not written: stopped).

## Changes made
None in `server/`. (Only scratch files outside the repo.)

## Firmware changes requested
1. **Replace `firmware/src/claude_client.*` and `firmware/src/settings.*` with the proxy version from
   `firmware-prototype/src/`** (`proxyUrl` + `deviceToken` in NVS, `POST /v1/solve` with
   `authorization: Bearer <token>`, `x-device-id`, `x-firmware`; `/v1/pair/start`, `/v1/device/status`),
   and erase any stored API key on first boot, as the prototype does. `core/claude_api.h::kModel` can
   stay: the proxy overrides the model.
2. Keep `kIdleMs` >= 120 s; if the proxy later adds SSE keep-alive comments (`: ping`), the
   `StreamReader` already ignores non-`data:` lines.
3. Schedule a root-CA bundle refresh before 2031 (DigiCert Global Root CA expiry).

## Fixes applied (2026-10-10, hardening for paying users)

Tests: `python -m pytest -q` in `server/proxy` -> **51 passed** (was 15). New `test_hardening.py` (36 tests)
runs offline against a fake Anthropic client; `conftest.py` gives every run a throw-away database.

| Finding | Fix |
| --- | --- |
| S1 model default / no 429-5xx fallback | Tiers: `base` = `claude-sonnet-5-5` (default, per UNIT_ECONOMICS), `pro` = `claude-opus-5-5` (`accounts.tier`, `admin.py tier`, or the Stripe price). `solve.open_main_stream` tries the tier's model, then the other one on 408/409/429/5xx/529 or a connection error (`MODEL_FALLBACK`, `CLAUDE_MAX_RETRIES=1` per model); 400/401 are not retried. Effort always sent (Opus 5.5 defaults to medium). Refusal fallback unchanged (`"default"` form, beta `server-side-fallback-2026-07-01`). |
| (new) prompt caching | System prompt sent as one text block with `cache_control: ephemeral` (`PROMPT_CACHE`); ~1.4k tokens, above the 512-token minimum on Sonnet/Opus 5.5. Cache read/write tokens logged per solve. |
| M3 device-owned prompt | `prompts/solver_v1.txt` + `prompts/solver_v1.schema.json` (decoded byte-for-byte from `core/claude_api.cpp`, so answers don't change). Device `system` and schema ignored (`PROMPT_SOURCE=server`; `device` for bench tests). Version `solver_v1+<hash>` logged per solve; `admin.py prompt`. Request schema unchanged; content check tightened (one text block <= 1000 chars, non-empty base64). |
| S2 no burst limit | `limits.py`: per calculator 1 solve in flight, 4/minute, 40/hour; 30/minute on pairing/status/firmware; 10/minute per IP on `/link`. Leases expire after 15 min. Monthly cap per tier (`MONTHLY_CAP` 500, `PRO_MONTHLY_CAP` 280 = Opus break-even at $15). Burst refusals are `429 fair_use_exceeded` + `retry-after` with a <= 110-char message, because that is a type the firmware already displays. In memory: run one worker (Dockerfile does). |
| S3 `/link` unauthenticated | `LINK_MODE=email` (default): `/link` validates the code and e-mails a link signed with HMAC-SHA256 (`LINK_SECRET`, 30 min); `GET /link/confirm` only shows a button (mail scanners link nothing); `POST /link/confirm` links. Single use (the code is consumed). `parse_qs` form parsing, e-mail validation, CSP / no-referrer / nosniff headers. One free month per calculator (`devices.first_linked_at`): relinking to a new e-mail doesn't restart the trial. SMTP or console mailer (`mailer.py`). |
| M2 `claim_pair_code` race | One `BEGIN IMMEDIATE` transaction: select, delete code, `INSERT OR IGNORE` account, update device. 8-thread test: exactly one winner. `note_hw_id` only writes when the hw id is still empty. |
| S4 billing stub | `billing.py`: `WebhookVerifier` interface, `StripeVerifier` (`Stripe-Signature` t/v1, HMAC-SHA256 over `t.body`, 300 s tolerance, constant-time compare). Events: checkout.session.completed, customer.subscription.created/updated/deleted, invoice.paid, invoice.payment_failed; replays ignored (`billing_events`). States trial / active / past_due (3-day grace) / canceled; first month free from linking. Old `x-billing-secret` endpoint removed (`admin.py paid` still works by hand). |
| M1 `/healthz` leak | Returns only `{"ok": true, "version": ...}`. Docs, redoc and openapi.json off. |
| M4 idle timeout | Relay re-frames upstream bytes into whole SSE events and sends `: ping` every 15 s (`KEEPALIVE_SECONDS`) only between events; otherwise byte-for-byte Claude's stream (tested with a Python port of `StreamReader`). An upstream drop mid-answer now ends with an SSE `error` event `overloaded_error` (calculator: "busy") and isn't counted. |
| M5 logging | `logs.py`: `x-request-id` per request (accepted from the caller if sane), in every log line and response header; redaction of `sk-ant-`, `dt_`, `whsec_`, bearer values, `?t=` link tokens, long base64. Bodies, photos and answers never logged. Per solve: device, model (and the model that actually answered), tier, prompt version, token usage, pings, duration. |
| (new) OTA board family | `/v1/firmware/image` also refuses an image of another board family when the calculator sends `x-firmware`. |
| (new) `admin.py` didn't run | A literal newline inside an f-string (in HEAD) was a SyntaxError: every admin command failed. Fixed; new commands `tiers`, `tier`, `account`, `prompt`. |
| (new) Docker | Copies `prompts/`, runs as a non-root user, one worker, `--forwarded-allow-ips`. |

Still open (server): only one OTA image at a time, so v14 and v15 calculators can't both be offered
updates at once (needs one manifest per board family); a mid-stream `overloaded` error after the first
byte isn't retried on the other model; the burst limiter is per process (move to Redis/DB before running
more than one instance); the real Stripe Checkout page and the shop's sign-in are outside this folder.

### Firmware changes requested (2026-10-10)
1. `core/claude_api.cpp` `classifyFailure`: add `"rate_limited"` to the account error types shown as a
   message (and optionally read `retry-after`). Then set `policy.RATE_LIMIT_ERROR_TYPE = "rate_limited"`;
   until then the proxy reuses `fair_use_exceeded`, which already displays.
2. `firmware-v15-lcd/src/main.cpp:58` `kSendSolveTimeoutMs = 120000` is a *total* limit on the preview
   Send, not an idle limit: a long `xhigh`/`max` solve that is still streaming (or pinging) is reported as
   "took too long" at 120 s. Raise it (e.g. 300 s) or reset it whenever bytes arrive.
3. Optional, saves ~5 KB upload per solve: stop sending `system` in `buildSolveRequest` (the proxy
   ignores it and accepts its absence). Keep sending `output_config.format` (still required by the
   request check).
4. Still open from above: replace `firmware/src/claude_client.*` with the proxy client; keep `kIdleMs`
   >= 120 s (the proxy's `: ping` comments are already ignored by `StreamReader`); CA refresh before 2031.
