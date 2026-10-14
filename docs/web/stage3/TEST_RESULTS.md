# ai_calc_web_3: test results (2026-10-11, re-run after the Proofline / $10 / yearly changes; Windows 11, Node 24.21)

All local. Stripe and the AI are fakes, so nothing costs money.

| Suite | Command | Result |
|---|---|---|
| TypeScript | `cd web/api && npx tsc --noEmit` | pass |
| Worker unit tests | `cd web/api && npx vitest run` | **159 / 159 pass** (8 files; includes the yearly plan and security round 2) |
| Contract tests, main pass | `cd web/tests && npm run test:full` | **pass** (now with yearly-plan checkout and `plan_interval`) |
| Contract tests, tiny monthly limit | (same run) | 4 pass, 1 skipped by design |
| Contract tests, tiny daily cap | (same run) | 1 pass, 4 skipped by design |
| Contract tests, store closed | (same run) | 2 pass |
| Site checks (28 pages; re-run with 29 pages on 2026-10-11) | `node web/tools/check-site.mjs` | all good |
| Shared header/footer up to date | `node web/tools/chrome.mjs --check` | up to date |

## Checked by mutation
Two safety rules were broken on purpose to prove the tests catch them: the 5-minute webhook window and the
"older event can't overwrite newer state" guard. Each was caught by exactly its own test.

## End to end, in the browser, on the local stack (`node web/dev/run-local.mjs`)
1. `calculator.mjs new` showed a link code; opening its link while signed out went to sign-in and back.
2. Sign-in via the local outbox; link; the calculator's status showed the free month; `solve` printed the
   answer and steps.
3. Account → Subscribe → fake checkout (price, renewal, 18+ consent) → pay → "Your subscription is active",
   renews in 30 days (webhook verified and applied).
4. Manage billing → simulated failed payment → calculator shows `past_due`, still solves (inside the 3 days).
5. Shop → calculator → quantity 2 → Preorder → cart ($450) → checkout → fake pay → "Thanks for your order";
   the order is stored as `paid` with the shipping address; the cart empties.
6. Mobile (375px) menus, FAQ and pricing; no horizontal scroll.

## Not tested
- Real Stripe (needs your sandbox), real e-mail, Turnstile, the free plan's CPU limit after deployment.
- Screen readers and real phones (the code follows WCAG 2.1 AA; the automated checks pass).
- Print styles on the teachers page.
- The C++ core and firmware (nothing changed there).
