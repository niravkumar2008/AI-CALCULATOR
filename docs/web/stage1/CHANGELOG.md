# ai_calc_web_1: backend + device sign-in (2026-10-08)

> Update 2026-10-11: superseded by `docs/web/stage3/`. The $8 monthly limit below is now $5 (plan $10/month or
> $100/year), the unit tests number 159, and the exam-mode plan's SETUP 9 is now MODE 4 → Settings → Link calculator.

Stage 1 of the sell-and-subscribe layer (`docs/web/PLAN.md`). New code only; the existing `server/proxy/` and all firmware are untouched.

## Added
- `docs/web/PLAN.md`: the approved plan.
  - Section 0: your decisions.
  - Section 4b: settings and test conventions.
  - Section 4c: security-review decisions.
- `docs/web/openapi.yaml`: the API contract. It passes the Redocly lint.
- `web/api/`: the new AI server, a Cloudflare Worker in TypeScript with a D1 database and KV storage.
  - **Calculator sign-up:** each calculator registers itself and gets its own token. The server stores only a hash of the token.
  - **Pairing:** an 8-character code, valid for 15 minutes, claimed atomically.
  - **Status:** the screen shows the exact message from the server, including "Your free month is over. Subscribe at …/account to keep using AI."
  - **Solve:** the server owns the prompt, the model (Sonnet 5.5) and the 16,000-token cap.
    - Boards flashed today keep working, because their old request format is accepted. Only the photos and the tutor flag are read from it.
    - Answers stream through unchanged.
  - **Spend cap that fails closed:**
    - Limits: 1 solve in flight and 6 per minute per calculator; 300 solves and $8 per account per month; $50 per day for everyone.
    - The worst-case cost is reserved before every AI call and corrected to the real cost afterwards.
    - Any database error means no AI call.
  - **Sign-in for the portal:** a magic link by e-mail.
    - The link opens a confirm button, so e-mail scanners can't use it up.
    - It never reveals whether an account exists.
    - Cookie: HttpOnly, Secure, SameSite=Lax. Origin checks against cross-site requests.
  - **Account:** your devices, link, rename, revoke, delete account.
  - **Firmware updates:** the image is stored in KV. Same calculator endpoints as before.
  - **Board enrolment (security fix S1):** only boards you enrolled at QA get a free month.
    - One free month per board ever and one per e-mail ever, at most 20 new trials a day.
    - Script: `scripts/enroll-board.mjs`.
  - **Abuse limits:** rate limits stored in D1 (no paid add-on), 80 sign-in e-mails a day, optional Turnstile, a 290 s stream cap, and an hourly job that clears stuck charges.
  - **Security round 2:** IPv6 /64 rate-limit keys, a global sign-up cap, an HMAC'd e-mail hash (`TRIAL_PEPPER`), a race-proof free-month claim, and config sanity checks.
  - **Tests:** 91 unit tests (vitest).
- `web/tests/`: a black-box test kit.
  - It checks every response against `openapi.yaml`.
  - A fake Anthropic server means no real AI calls and no cost.
  - Three passes: normal, tiny monthly limit, and tiny daily cap.
  - `EXAM_MODE_REGRESSION_PLAN.md`: the exam-mode tests for stage 2.

## Changed
- Nothing outside `docs/web/` and `web/`.

## Not in this stage
- Stripe checkout, subscriptions and webhooks (stage 3; these routes return 404 for now).
- Customer portal pages, calculator firmware changes, Wi-Fi typing on the keys, auto-update, firmware signing (stage 2).
- The public website (stage 4).
