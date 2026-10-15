# ai_calc_web_1: test results (2026-10-08, Windows 11, Node 24.21)

All runs are local only. AI calls go to a fake Anthropic server (`web/tests/mock-anthropic.mjs`), so they cost nothing.

| Suite | Command | Result |
|---|---|---|
| TypeScript check | `cd web/api && npm run typecheck` | pass |
| Worker unit tests (vitest) | `cd web/api && npm test` | **91 / 91 pass** (5 files) |
| Contract tests, main pass | `cd web/tests && npm run test:full` | **74 pass, 0 fail, 19 skipped** |
| Contract tests, tiny monthly limit | (same command, second pass) | 4 pass, 0 fail, 1 skipped (it runs in the third pass) |
| Contract tests, tiny daily cap | (same command, third pass) | 1 pass, 0 fail, 4 skipped (they run in the second pass) |
| Contract lint | `npx @redocly/cli lint docs/web/openapi.yaml` | valid |

## The 19 skipped tests in the main pass
- **17 are Stripe tests:** billing, store and webhooks. They are stage 3 placeholders and stay skipped until then.
- **2 are spending-total checks:** that the settled cost is correct and that the monthly $8 limit triggers. The unit tests cover both (`web/api/test/solve.test.ts`, `security.test.ts`); the black-box versions come in stage 3.

## Not tested (needs real accounts, or comes later)
- A real Anthropic call. The fake server copies the real streaming format.
- A real Resend e-mail and a real Turnstile check (both are mocked).
- The free plan's 10 ms CPU limit, which needs a real deploy. Big photos may go over it; if so, the fix is the $5 plan.
- Which certificate authority signed the live `workers.dev` certificate (manual step 9).
- Two claims racing for the same board in the same second. Only the logic was reviewed.
- C++ core tests: nothing in `core/` changed, and this PC has no compiler.

## Fixes made while merging
- Three tests reused one calculator for many bad requests. After security fix S6 they hit the 6-per-minute limit first. They now use a new calculator each.
- The main test pass raises the 80-mails-a-day cap so the suite can sign in more than 80 times. The real cap stays 80, and a dedicated test checks it.
- The test runner now uses `127.0.0.1`, because on Windows `localhost` resolved to IPv6.

## Security review
- Round 1 found 1 HIGH, 4 MEDIUM, 5 LOW and 3 INFO findings. All are fixed or deferred, as recorded in PLAN section 4c.
- Round 2 found no HIGH findings, so nothing blocks the release.
  - Fixed in this release: R2-1 (IPv6 /64 rate-limit keys plus a global register cap of 200/hour, read before any write), R2-3 (HMAC e-mail hash with `TRIAL_PEPPER`), R2-4 and R2-5 (the free-month claim is one chained batch; a cap race returns 429 and nothing is linked), R2-6 (time-limit sanity check), R2-7 (DEV_MODE refused unless the address is local) and R2-9 (cost settling in one batch).
  - Moved to stage 2: R2-2 (Turnstile required in production; needs the sign-in page widget), R2-8 (slow-reader edge case, harmless to the caps), S11 (firmware signing) and S13 (sign-in page referrer policy).
  - Known and accepted: someone with physical access to a board can read its enrolment secret, but that still gets them only that one board's single free month.
