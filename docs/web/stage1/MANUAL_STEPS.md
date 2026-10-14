# ai_calc_web_1: your steps

> Update 2026-10-11: superseded by `docs/web/stage3/MANUAL_STEPS.md` and `docs/web/GUIDE.md`. Commit from the
> `ai-calculator-launch-site-034250` worktree; today `npm test` in `web/api` expects 159 passed, not 91.

Part A costs nothing and needs no accounts. Part B is for when you want the server online. It also costs nothing (free Cloudflare plan, `workers.dev` address), but it needs a free Cloudflare account. **You** type every password and key; never paste them into a chat or a file that goes into git.

## Part A: put it in your repo and run the tests (about 10 minutes)

1. Extract `ai_calc_web_1.zip` over the repo root. It adds `docs/web/` and `web/`. Nothing else is touched.
2. Open a terminal in `web/api` and run `npm install`.
   - Ignore the warning that says "install-scripts … workerd": it still works.
3. In `web/api`, run `npm test`. Expect **91 passed**.
4. Open a terminal in `web/tests` and run `npm install`, then `npm run test:full`.
   - Expect `summary: main=pass quota=pass cap=pass`.
   - Takes about a minute.
   - If it says "something is already answering on port 8787", close other terminals running `npm run dev`. If that doesn't help, run this in PowerShell:
     `Get-NetTCPConnection -LocalPort 8787 -State Listen | % { Stop-Process -Id $_.OwningProcess }`.
5. (Optional) Try it by hand: in `web/api`, run `npm run db:migrate:local`, then `npm run dev`, then open http://localhost:8787/healthz. It should show `{"ok":true}`. Press Ctrl+C to stop.
6. Commit: `git add docs/web web` then `git commit`. Suggested message:
   `Stage 1 sell-and-subscribe: Cloudflare Worker API, device sign-in, spend cap, contract tests`.
   - `node_modules`, `.wrangler` and `.dev.vars` are already git-ignored.

## Part B: put the server online for free (when you're ready; about 30 minutes)

The calculator firmware can't use this server until stage 2, which adds sign-up, enrolment and the new menus. So there's no rush. Boards flashed today still point at whatever `proxy` URL you set.

7. Make a free account at https://dash.cloudflare.com/sign-up. The Free plan needs no card. Then in `web/api`, run `npx wrangler login`. It opens your browser; click Allow.
8. Create the database and storage, and paste the two ids the commands print into `web/api/wrangler.toml`. They replace the all-zero placeholders.
   ```
   npx wrangler d1 create ai-calc
   npx wrangler kv namespace create FIRMWARE
   ```
9. Create the tables:
   ```
   npx wrangler d1 migrations apply DB --remote
   ```
10. Add the secrets. Each command asks you to type the value, so it never goes in a file.
    ```
    npx wrangler secret put ANTHROPIC_API_KEY
    npx wrangler secret put ADMIN_TOKEN
    npx wrangler secret put TRIAL_PEPPER
    ```
    - `ANTHROPIC_API_KEY`: the same key as in your git-ignored `api_key.txt`.
    - `ADMIN_TOKEN`: at least 32 random characters, saved in your password manager. Make one with `node -e "console.log(require('crypto').randomBytes(32).toString('base64url'))"`.
    - `TRIAL_PEPPER`: another 32 random characters (same command), saved in your password manager. **Never change it** once customers exist: changing it makes every e-mail eligible for a free month again. Without it the online server refuses to run.
    - `RESEND_API_KEY` is only needed when real customers sign in. Until then, sign-in e-mails aren't sent.
11. Deploy once with `npm run deploy`. Wrangler prints your address, e.g. `https://ai-calc-api.<you>.workers.dev`.
    - Put that address in `PUBLIC_URL` in `wrangler.toml`. It must be **https**; the server refuses to run with an http address.
    - Run `npm run deploy` again.
12. Check that `https://ai-calc-api.<you>.workers.dev/healthz` shows `{"ok":true}`.
13. **Check who signed the certificate.** The calculator only trusts certain authorities. In Git Bash, run:
    ```
    echo | openssl s_client -connect ai-calc-api.<you>.workers.dev:443 -servername ai-calc-api.<you>.workers.dev 2>/dev/null | grep -i "issuer"
    ```
    - If the issuer mentions **Google Trust Services** or **Let's Encrypt**, you're fine.
    - Anything else (e.g. SSL.com): tell Claude. The stage-2 firmware will add that root.
14. **Set a spending limit at Anthropic** as a backstop: https://console.anthropic.com → Settings → Limits. For example, $50/month while testing.
15. (Recommended, free) Add a Cloudflare rate-limiting rule (security finding S4):
    - In the dashboard: your Worker's zone → Security → WAF → Rate limiting rules.
    - Paths `/v1/device/register` and `/api/auth/login`, 20 requests per 10 seconds per IP, then block.
    - If your account only has the `workers.dev` address and no zone, skip this until you buy a domain.
16. Leave `server/proxy/` alone for now. It is retired after stage 3.

## Later (not now)
- **Enrolling boards at QA** (`node scripts/enroll-board.mjs calc-<mac> https://…workers.dev`): this needs the stage-2 firmware command `enroll`, so do it when stage 2 arrives.
- **What costs money, and when** (see `web/api/README.md` section 6):
  - Workers Paid at $5/month, once real customers' photos hit the free plan's CPU limit.
  - A domain, about $10–15/year.
  - Resend Pro, $20/month, but only at around 5,000 subscribers.
- **Turnstile** (free): turn it on when the stage-2 portal has the widget.
