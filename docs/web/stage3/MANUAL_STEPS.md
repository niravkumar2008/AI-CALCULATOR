# ai_calc_web_3: your steps

The full walk-through, with the why, is `docs/web/GUIDE.md`. This is the short checklist.

## Now (free, no accounts)
1. In this worktree: `cd web/api` then `npm install` (if not done), back to the repo root, `node web/dev/run-local.mjs`.
2. Open http://127.0.0.1:8787 and follow GUIDE section 2 to try every flow.
3. Run the tests if you like: `cd web/api && npx vitest run`, and `cd web/tests && npm run test:full`
   (stop run-local first: they share ports 8787–8789).
4. Review and commit **from this worktree** (`ai-calculator-launch-site-034250`): it has stage 1 + stage 3 +
   the website. The old copy of stage 1 in `ai-calc-sell-subscribe-4bd464` is superseded; don't commit both.
   Suggested message: `Stage 3: Stripe subscriptions and store, multi-page site, account pages, local dev stack`.
   `node_modules`, `.wrangler` and `.dev.vars` are git-ignored.

## Later (when you're ready to take money)
GUIDE sections 3–6, in order: Stripe account → Stripe sandbox setup
(products, Terms URL, failed-payment and e-mail settings, customer portal, restricted key, webhook) →
Cloudflare deploy → test with card 4242 → lawyer and accountant → domain → live mode → `STORE_OPEN=1`.
