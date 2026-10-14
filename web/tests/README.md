# web/tests: contract tests for the AI Calculator Worker

Black-box tests over HTTP against the Worker in `web/api`, validated against
`docs/web/openapi.yaml` (the contract). Node built-ins only (`node:test`, `node:assert`,
`fetch`); dev dependencies are `ajv`, `ajv-formats` and `yaml`.

## Run

```sh
cd web/tests
npm install
npm run test:full     # mocks + wrangler dev (4 passes) + all tests + clean shutdown
# port 8787 busy (e.g. another wrangler)?  PORT=8797 MOCK_PORT=8798 STRIPE_MOCK_PORT=8799 npm run test:full
```

`test:full` (`run-all.mjs`):

1. starts the mock Anthropic server on `127.0.0.1:8788` and the Stripe mock (`mock-stripe.mjs`) on
   `127.0.0.1:8789`;
2. creates a throw-away state directory and applies the D1 migrations to it
   (`npx wrangler d1 migrations apply DB --local --persist-to <tmp>`), so every run starts
   with an empty DB and KV;
3. runs `npm run dev -- --persist-to <tmp> --var KEY:VALUE ...` in `../api`. Settings go in
   as `--var` flags (DEV_MODE=1, ANTHROPIC_BASE_URL=mock, a fake ANTHROPIC_API_KEY,
   PUBLIC_URL, a random ADMIN_TOKEN, an empty RESEND_API_KEY so no real e-mail is ever
   sent, the Stripe mock's key, webhook secret and price ids, `STORE_OPEN=1`). It never creates or edits `../api/.dev.vars`; in wrangler 4.x `--var` overrides
   both `wrangler.toml` vars and `.dev.vars`;
4. waits for `/healthz`, runs the tests one file at a time, then kills the wrangler
   process tree (`taskkill /T /F` on Windows) and deletes the temp state.

Everything binds and talks to `127.0.0.1` (`--ip 127.0.0.1`, BASE_URL and PUBLIC_URL
`http://127.0.0.1:8787`): on Windows Node can resolve `localhost` to `::1` while workerd
listens on IPv4 only. Messages that embed the host are checked against PUBLIC_URL's host.

Four passes, each on fresh state: `main` (all files, MAX_NEW_TRIALS_PER_DAY=100000 so the
suite's many trials fit), `quota` (limits.test.mjs with MONTHLY_SOLVES=2,
MAX_NEW_TRIALS_PER_DAY=1), `cap` (limits.test.mjs with GLOBAL_DAILY_CAP_USD=0.01) and
`closed` (stage3.test.mjs with STORE_OPEN=0).

Knobs: `PASSES="main"` (default `main quota cap closed`), `KEEP_STATE=1` (use `../api/.wrangler/state` and
`npm run db:migrate:local`), `TEST_FILES="solve.test.mjs"` (main pass), `PORT` (any port
other than 8787 calls `npx wrangler dev --local --port PORT` instead of `npm run dev`),
`MOCK_PORT`, `STRIPE_MOCK_PORT`, `API_DIR`, `CONTRACT_PATH`.

Against a server you started yourself:

```sh
npm run mock          # terminal 1 (only needed for solve tests)
# terminal 2: wrangler dev with DEV_MODE=1, ANTHROPIC_BASE_URL=http://127.0.0.1:8788
BASE_URL=http://127.0.0.1:8787 ADMIN_TOKEN=... npm test
```

`npm run selfcheck` checks the kit with no Worker: every file parses, the contract loads
and all its schemas compile (97 on 2026-10-11), and the mock streams correctly in all failure modes.

## Files

| File | Covers |
|---|---|
| `contract.mjs` | Loads `openapi.yaml`; `assertMatches(method, path, res)` fails on any undocumented status, wrong content type, schema mismatch ($refs, oneOf), missing documented header (Retry-After, Set-Cookie) or a body on a bodiless response. Every `api()` call in the tests goes through it. |
| `mock-anthropic.mjs` | `POST /v1/messages` with a real-format SSE stream (usage in `message_start` and `message_delta`, the calculator's JSON answer in text deltas). Checks `x-api-key` and `anthropic-version`. Markers in the image bytes: `FAIL500`, `MIDFAIL`, `SLOW`. `GET /__last`, `GET /__count`, `POST /__reset`. |
| `lib.mjs` | HTTP helper, register / login (dev outbox) / claim flows, test images. |
| `device.test.mjs` | register (shape, formats, hw_id 400, 5/IP/hour 429), pair/start, status before/after claim, 401 unauthorized vs device_revoked, x-device-id optional. |
| `solve.test.mjs` | SSE passthrough, new + legacy bodies, legacy model/system/max_tokens/text/tools not forwarded, 402 not linked / trial over (exact D2 message) / plan inactive, 400s, 6 MB decoded, 8 MB raw 413, upstream 500 -> 502, mid-stream `event: error`, one-in-flight 429, burst 7th 429. Every rejection asserts the mock call count did not move. |
| `auth.test.mjs` | login always 204, age_confirmed, Origin and Content-Type 403, IP limit 429, cookie flags, single-use token, logout, /api/me 401. |
| `account.test.mjs` | claim (dash / no dash / lower case / spaces), one invalid_code message, claim race (exactly one 201), 3-device limit 409, rename, label validation, cross-account 404, revoke -> device_revoked, trial starts on first claim only, DELETE /api/me confirm_email. |
| `enrolment.test.mjs` | /v1/admin/hardware (401/400/201), register hw_secret, free month only for enrolled boards, once per board (revoke + re-register, re-enrol keeps trial_used), once per e-mail even after DELETE /api/me, unenrolled -> plan none + "No free month" reason, solve 402. |
| `limits.test.mjs` | Runs only in the extra passes: quota (MONTHLY_SOLVES=2 -> 429 quota_exceeded; MAX_NEW_TRIALS_PER_DAY=1 -> claim 429 with the code not consumed) and cap (GLOBAL_DAILY_CAP_USD=0.01 -> 503 spend_cap). |
| `firmware.test.mjs` | nothing published (fresh state only), admin 401s, bad image 400s, publish -> update true/false, image sha256. |
| `dev.test.mjs` | `/__dev/*` is 404 for foreign Host headers (uses `node:http`, since fetch cannot set Host). |
| `mock-stripe.mjs` | Local Stripe stand-in (customers, Checkout sessions, subscriptions, portal, `__fail` injection). Never a real key. |
| `stage3.test.mjs` | Stage 3 (replaced `stage3.todo.test.mjs`): subscription checkout (monthly and yearly), billing portal, store status and checkout (open and `closed` passes), webhook signature / idempotency / subscription states, waitlist. |
| `site/` | Console helpers for the website (`break-ui.js`, `a11y-audit.js`); see `web/SITE_README.md`. |
| `EXAM_MODE_REGRESSION_PLAN.md` | Exam-mode behaviours stage 2 must pin in `tests/tests.cpp`, with proposed code (written for the e-paper menus; see its 2026-10-11 note). |

## Skipped, and why

- Stage 3 tests: skipped when `STRIPE_WEBHOOK_SECRET` is not set (`test:full` sets it); the waitlist block runs in the `main` pass only.
- Admin firmware tests: skipped when `ADMIN_TOKEN` is not given (`test:full` sets one).
- "Nothing published": runs only with a fresh state (`FRESH_STATE=1`, set by `test:full`).

## Conventions the Worker must honour (beyond openapi.yaml)

1. **`x-dev-client-ip`**: when DEV_MODE=1 and the host is localhost, IP rate limits are keyed
   on this header if present. Without it all tests share one IP and the 5-per-hour register
   limit stops the suite after 5 devices. Never honoured outside DEV_MODE.
2. `/__dev/outbox?email=` matches e-mails case-insensitively and returns `{link, token}`
   (status 200) for the last sign-in e-mail.
3. `/__dev/user` `plan_override` takes a `PlanState` (`active`, `past_due`, `inactive`, ...).
   `inactive` with an expired trial means "subscription ended".
4. State-changing calls without a body (logout, revoke) are sent with
   `Content-Type: application/json` and body `{}`.
5. Images are forwarded to Anthropic unchanged (the mock reads its markers from the bytes).
6. Device firmware endpoints work for unlinked devices (an unpaired calculator must still update).
7. A pair code rejected with `device_limit`, or with a bad label, stays claimable.
8. On a 429 the `Retry-After` header is always sent (the contract documents it).

## Contract change proposals (not applied; for the lead)

1. (done in the 4c revision) PATCH 400 and the claim `bad_request` wording.
2. Say which error `/v1/solve` returns for a linked board with plan `none` (no free month).
   The tests assume 402 `subscription_inactive` with the device-status reason as message.
3. Say whether a registration with a well-formed but wrong `hw_secret` is 201 (unenrolled)
   or 400. The tests accept either.
3b. `/v1/firmware`, `/v1/pair/start`, `/v1/device/status`: no `429` is documented. If the
   Worker rate-limits polling, add it.
4. `/api/auth/logout` without a session: document `401` or say it is 204 either way.
5. (obsolete since stage 3: the Stripe routes exist and `stage3.test.mjs` validates them.)
6. `ImagePart.data` "Max 6 MB decoded" vs the 8 MB raw limit: say MB = 10^6 or MiB, since
   6 MiB decoded is already over 8 MiB of base64 (one of the two limits is unreachable).
