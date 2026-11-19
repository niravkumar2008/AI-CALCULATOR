# QR linking: server side (2026-10-16)

Spec: `docs/LINK_QR_CONTRACT.md` (in the firmware worktree). Decisions: 1 calculator per plan,
a camera link goes live only after one Approve tap, Wi-Fi password only ever in the browser,
payment grace 7 days.

## API (web/api, documented in docs/web/openapi.yaml)

- Migration `0007_link_tokens.sql`: table `link_tokens` (SHA-256 of the token only), plus
  `dev_outbox.kind` (`login` | `linked`). The hourly cron deletes link tokens older than a day.
- `POST /api/link-tokens` -> `{id, token: "L1.<22>", qr_url: "<site>/q/L1.<22>", expires_at}`,
  10 minutes, cancels the user's older unused tokens, 10 per user per hour.
- `GET /api/link-tokens/{id}` -> `{state, device, replaces, approve_by}` (owner only).
- `POST /api/link-tokens/{id}/approve` `{approve: true|false}`. True links through the same
  claim engine as `/api/devices/claim` (`linkToUser` in `src/account.ts`: identical free-month
  rules and atomic batch), revokes the replaced calculator, sends "A calculator was linked to
  your account" (dev: `GET /__dev/outbox?email=..&kind=linked`).
- `POST /v1/pair/redeem` (calculator, Bearer dt_): 404 `link_token_invalid`, 410
  `link_token_expired`, 409 `link_token_used`, 409 `already_linked`, 200 `pending_approval`
  (approve within 300 s) or `linked` (same account). 10/min per calculator, 30/min per address.
- `GET /v1/device/status` gains `link_state`: `pending_approval` | `declined` (10 min) | `""`.
- `/api/devices/claim`: 1-device slot. 409 `needs_replace` with `replaces {id, suffix, label}`
  unless `replace: true` (old calculator revoked in the same batch, only if the link succeeds).
  `device_limit` (the old 3-device cap) is no longer returned.
- `PAST_DUE_GRACE_SEC` 3 -> 7 days.
- `/q/*` now runs the Worker first: `/q/L1.<token>` 302s to `/q/` (token out of the address bar),
  `/q/` is served from the static site via the new `ASSETS` binding.

## Site (web/site)

- `/link/?code=` pre-fills (as before) and handles `needs_replace` ("Replace calculator ending
  7K2Q with this one?").
- New `/link/camera/`: optional Wi-Fi name/password/hidden, builds the QR in the browser
  (`assets/js/qrcode.js`, qrcode-generator 1.4.4, MIT, vendored), `PL1;...;;` payload escaped per
  the contract, 10-minute countdown, New code, polls every 2 s, Approve / No, replace warning.
- New `/q/` explanation page. Account page links to the camera page; plan copy across the site now
  says one calculator per plan; "keeps working for 7 days".

## Local

`node web/dev/calculator.mjs scan "<QR text>"` pretends the calculator scanned the phone;
`status` shows `link_state` and the Calculator ID suffix.

## Tests

- `web/api`: `test/link.test.ts` (new, 26 tests) plus updated claim / grace tests: 234 pass
  (206 before).
- `web/tests` black-box: `link.test.mjs` (new, 4 tests) plus updated claim tests; all four
  passes green (main 116 tests).
