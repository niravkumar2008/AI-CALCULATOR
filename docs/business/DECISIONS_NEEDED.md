# Proofline: decisions needed

Written 2026-10-12. This is one list of every open choice, taken from the docs in both worktrees plus the
items raised last night. Each row gives the options, our recommendation with a one-line reason, what the
decision holds up, and the doc where the detail lives. Mark a row **DECIDED (date)** when you choose, then
update the source doc too.

Not legal, tax or financial advice. Wherever the law matters, the row says "lawyer".

**Where the source docs are.** **LS** = this worktree (`.claude/worktrees/ai-calculator-launch-site-034250`).
**V15** = `.claude/worktrees/v15-mode4-menu`. Short names used below:
`BS` = V15 `Claude outputs/BUSINESS_STATUS_2026-10-11.md`, `SU` = V15 `hardware/production/SCALE_UP_PLAN.md` §9,
`UE` = V15 `hardware/production/UNIT_ECONOMICS.md` §8, `SB` = V15 `hardware/SAMPLES_BRINGUP_PLAN.md`,
`FW` = V15 `firmware-v15-lcd/README.md`, `PN` = V15 `firmware-v15-lcd/PORT_NOTES.md`,
`GUIDE` = LS `docs/web/GUIDE.md` §9, `KS` = LS `docs/business/KICKSTARTER_DRAFT.md`,
`MG` = LS `docs/business/MARKETING_GUIDE.md`, `TK` = LS `docs/business/TEACHER_KIT.md`.

**Already decided (don't reopen without a reason):** brand Proofline; $225 device; $10/month or $100/year;
first month free with no card; buyers 18+; v15 colour LCD; 5 samples first, then bulk; English only;
MODE 4 = Apps; calculator screens look exactly like the Casio; preorders in 2027; firmware updates only with
the plan (but see A4); marketing never names the AI or payment vendors.

---

## A. Before the samples arrive (this week)

| # | Decision | Options | Our recommendation, and why | What it blocks | Source |
|---|---|---|---|---|---|
| A1 | **How exam mode is unlocked.** Today: it ends after 12 h, or by typing `exam off` in a USB serial terminal. Anyone with a laptop and a cable can do that. | (a) Teacher sets a PIN when exam mode starts; (b) a teacher-only unlock tool or web page; (c) keep it like the TI-84 (anyone can end it, but everyone can see it was ended) | **(c), and say so honestly.** The protection is that the dark EXAM bar disappears when it ends, and its clock restarts from 0:00 if a student turns it back on. A teacher sees both from across the room, the same deal teachers already accept from the TI-84. A PIN means a teacher has to touch 30 calculators, and a forgotten PIN locks a student out for 12 h. A tool or page needs teacher accounts we don't have. If teachers ask for more, revisit with (a) after launch. | Exam-mode tests E12–E13 on the samples; the teacher one-pager; the exam-mode FAQ | `SB` §5; `TK` B2; FW / core `startExam` |
| A2 | **How many LCD panels to order** | 3 (what the order walkthrough said) / 5 + 1 spare | **5 + 1 spare (6).** You need one per sample, and the glass is the part most likely to crack during assembly. The extra panels cost very little. | Building samples 4 and 5 | `SB` §0.2; `BS` action 2 |
| A3 | **Casio logo on the sample shells** (genuine fx-115ES shells) | Show it / cover it / use knockoff shells for the samples too | **Genuine shells for the 5 samples, with matte black vinyl over the logo before any camera sees it. Never sell a Casio shell.** It's the fastest way to working samples, and it keeps their trademark and trade dress out of every photo. | Filming milestones M2–M7; the Kickstarter video | `SB` §0.4, §0.12; V15 `hardware/production/KNOCKOFF_SHELL_PLAN.md` §5 |
| A4 | **Security fixes for people without the plan** | Updates only with the plan, no exceptions / security fixes for everyone, features plan-only | **Security fixes for everyone.** It costs almost nothing, it protects your server and your name, and a lawyer may say you have to anyway. New features stay plan-only. | The OTA rules in the Worker; the terms page; the FAQ wording | `GUIDE` item 8; `SU` D13; `BS` open decisions |

## B. Before taking any money

| # | Decision | Options | Our recommendation, and why | What it blocks | Source |
|---|---|---|---|---|---|
| B1 | **Company form** | Corporation ("Proofline Corporation", already on the site) / LLC; which state; who signs if you are under 18 | **An LLC in your home state, unless you plan to raise investor money soon.** It's cheaper and simpler and the profit is taxed once, on your return. Ask the lawyer or accountant first. Until it's registered, don't use "Corporation" anywhere public. | Bank account, Stripe live mode, Kickstarter, the FCC paperwork (it needs a US responsible party), and every legal page | `SU` D8; `BS` action 8; `GUIDE` §6.2; `UE` §8.6 (only the LLC fee, $97, is in the model) |
| B2 | **Trademark search on "Proofline"** | Skip / do your own quick search now / a lawyer's clearance search | **Do a free knockout search on USPTO now (classes 9 and 42), and have the lawyer do a clearance search before buying the domain or printing anything.** Renaming after printing boxes and filming is expensive. | Domain, company name, shell and box printing, the Kickstarter page | `BS` action 7; LS `web/NAMES.md` |
| B3 | **Domain and contact email** | Buy now / buy after the trademark search | **Buy right after a clean knockout search** (about $10–15 a year). Use free Cloudflare Email Routing for `hello@` and the free Resend plan to send sign-in emails. | Deploying on a real address, real sign-in emails, replacing `proofline.example` everywhere | `GUIDE` items 1 and §6.3–6.5; LS `web/NAMES.md` |
| B4 | **Who owns the Stripe account** | You personally / an adult on your behalf / the company | **The company, once it's registered, with an adult (18+) as its representative.** Stripe needs an adult and a bank account in the business's name. Test mode needs neither, so keep building in test mode until then. | Stripe live mode; any real payment | `GUIDE` item 2 and §3; LS `docs/web/PLAN.md` §9 |
| B5 | **What goes to the lawyer** (one review, ≈ $150–300) | — | **Send it all as one package:** Terms, Privacy and Refunds templates; California's auto-renewal law; "updates only with the plan" and A4; the 18+ buyer terms and B7; under-18 users and the AI provider's minors policy (COPPA and student privacy); the shell's trade dress (knockoff or custom); whether preorders can be taken before the FCC SDoC; the Kickstarter prepaid-plan reward (B9); how long to keep order data; warranty wording (B6); company form (B1). | Taking money; publishing the legal pages; the shell choice for production | `GUIDE` §6.1; `SU` §5, §6.3; `KS` launch checklist; `BS` action 9 |
| B6 | **Return window and warranty** | 14- or 30-day returns; 90-day or 1-year warranty | **30-day returns and a 1-year limited warranty.** That's normal for a $225 gadget, and the 3% warranty budget ($6.75 a unit) already covers it. The lawyer writes the wording. | `/refunds/` page (still "not decided"); Kickstarter launch | `SU` §6.3, D12; `MG` §10; `GUIDE` item 6 |
| B7 | **Waitlist age checkbox** (the site says "I'm 13 or older") | Keep 13+ / change to 18+ | **Change it to 18+.** Everyone who can buy (and every Kickstarter backer) has to be 18+, and some states now add rules for marketing to under-18s. You lose few useful sign-ups. Students can still read the site and use the device. Ask the lawyer as part of B5. | Collecting the waitlist; the email sequences | LS `web/site/index.html`, `web/api/src/waitlist.ts`; LS `web/site-src/PAGE_BRIEF.md` |
| B8 | **What the waitlist says when its daily cap is full** (20,000 new addresses a day) | Honest "busy, try tomorrow" (HTTP 429) / quietly accept and drop the address | **Keep the honest 429** ("The waitlist is busy today. Please try again tomorrow."). Pretending to accept an address and then losing it breaks the honesty rule, and the cap only matters if a bot attacks the form. | Nothing urgent (already built this way) | LS `web/api/src/waitlist.ts` |
| B9 | **Kickstarter rewards with a year of the plan** ($285 / $299 bundles) | Offer them / calculator-only rewards | **Offer them only if (1) the server can credit 12 prepaid months to an account (not built yet) and (2) Kickstarter confirms in writing that it's allowed. Otherwise use calculator-only rewards.** The bundles are the best deal for us and for backers, but a reward we can't honour is worse than no reward. | Kickstarter reward table; one small server feature | `KS` launch checklist and §3 |
| B10 | **Preorder platform** | Kickstarter all-or-nothing / own-site preorders / "reserve now, pay when it ships" | **Kickstarter all-or-nothing in spring 2027 (goal ≈ $9,500), then own-site preorders.** If the goal isn't met, nobody is charged and you don't build stock you can't sell. | The campaign plan; store opening (C3) | `MG` §10; `SU` D7; `KS` §3.3 |
| B11 | **When to add Turnstile** (Cloudflare's free bot check on sign-in and the waitlist) | Before the first public deploy / at launch | **Before the first public deploy.** Without it, one address can use up the day's sign-in emails in about 4 hours, and bots can fill the waitlist. It's free and takes about an hour. | Deploying the site and Worker publicly | `GUIDE` §6.6 and §10 (S4, open) |

## C. Before launch

| # | Decision | Options | Our recommendation, and why | What it blocks | Source |
|---|---|---|---|---|---|
| C1 | **AI model for solves** | Sonnet 5.5 (≈ $0.026 a solve) / Opus 5.5 (≈ $0.052) / Haiku first, then a stronger model when needed (≈ $0.010) | **Sonnet 5.5.** The $5 monthly cap means a paying user never loses you money. Re-check with the real solves a day from the samples. | Worker settings; cost model | `BS` open decisions; `SU` D14; LS `docs/web/PLAN.md` D4 |
| C2 | **"300 solves a month" on the site vs the $5 cap.** On Sonnet the $5 cap is reached at about 192 solves, so 300 is never reached. The site says 300 in 8 places. | Keep "300" / advertise what $5 really buys / Haiku first so 300 is reachable / raise the cap | **Advertise what's true: lower the solve cap to about 190 and say "up to 190 solves a month (fair use)", or say "fair use" with no number.** Promising 300 that the cap never lets anyone reach is the kind of claim the FTC and reviewers pick on. Revisit once real costs are measured. | Pricing, FAQ, shop and account pages (site agent); `MONTHLY_SPEND_USD` / solve-cap settings | `BS` open decisions; LS `web/site/pricing/`, `faq/`, `shop/` |
| C3 | **Store opening date** (`STORE_OPEN`) | Open at deploy / open after Kickstarter / when stock exists | **Keep it closed (waitlist only) until the Kickstarter ends, then open own-site preorders.** Selling with no booked production date falls foul of the FTC Mail Order Rule. | Turning the site's buttons from "Join the waitlist" to "Preorder" | `GUIDE` item 6, §6.8; `MG` §10 |
| C4 | **Subscribing during the free month** | Allowed, and billing starts at once (today; the page warns) / hide the button until the free month ends | **Hide it until the last week of the free month.** Today a customer who subscribes early pays for days they already had free, and that leads to complaints. | Account page; small server change | `GUIDE` item 4 |
| C5 | **Failed payments** | Cancel after Stripe's retries / mark unpaid | **Cancel after the retries** (recommended in the guide). It's clearer for the customer and keeps the data clean. | Stripe Billing settings | `GUIDE` item 5 |
| C6 | **Stripe Tax** | Now / later | **Later, when the accountant says you've reached a sales-tax threshold.** It costs a fee per transaction and there's nothing to tax yet. | Stripe setup | `GUIDE` item 7 |
| C7 | **One Worker for site + server** | One Worker / Cloudflare Pages + a Worker | **One Worker** (already built that way). One thing to deploy and one bill. | Deploy | `GUIDE` item 3 |
| C8 | **Privacy page and browser storage.** The site keeps the cart in the browser's storage (`cart.v1`), and the privacy page doesn't mention it. | Add a sentence / leave it | **Add one plain sentence:** the cart is kept only in your own browser, no tracking cookies are used, and here's how to clear it. It's honest and costs nothing. | Privacy page (site agent; lawyer sees it in B5) | LS `web/site/privacy/`, `web/SITE_README.md` |
| C9 | **Free AI after the free month** (competitors give a free tier) | None (today) / a few solves a week / a waitlist perk such as an extra free month | **None for now. Lead with "still a full calculator without the plan" and show yearly as $8.33 a month.** Decide on a small free allowance after you've measured real costs on the samples. | Pricing copy; cost model | LS `docs/business/MARKET_RESEARCH.md` §6.2 |
| C10 | **Pilot units beyond the 5 samples** | Straight to preorders / 5 more pilot units (≈ $506) | **Straight to preorders, unless a sample shows a real problem.** The 5 extra units would leave only about $288 of the $2,000. | Budget | `BS` open decisions; `UE` §8.5 |
| C11 | **Shell for the first 100 units** | Plain clone shell / custom 3D-printed shell / Casio (never) | **Plain clone if it passes the 9 go/no-go checks and the lawyer agrees; otherwise a custom shell.** | Production, FCC test unit | `SU` D1 |
| C12 | **Battery, panel supplier, batch sizes, certification scope, fulfilment** | See `SU` §9 D2–D6, D9–D11 | **Follow `SU` §9:** sample 3 panel suppliers; TW302030 cell if its UN38.3 papers arrive; 1 × 100 after a 5-board prototype; US-only FCC first; ship the first 100 yourself. | Stage B/C production | `SU` §9 |
| C13 | **Root README** (it still describes the old e-paper prototype) | Patch it again / rewrite it when the branches are merged | **Rewrite it at merge time** as a short Proofline overview with links to `web/SITE_README.md`, `docs/web/GUIDE.md`, `firmware-v15-lcd/README.md` and the hardware plans. Today it misleads anyone who opens the repo. | Nothing urgent; do it with the merge | LS `README.md` |

## D. Product polish (calculator behaviour; firmware "choices to confirm")

The firmware copies the Casio fx-115ES exactly unless a row below says otherwise. All of these are already
built one way; "keep" means no work is needed.

| # | Decision | Options | Our recommendation, and why | What it blocks | Source |
|---|---|---|---|---|---|
| D1 | **d/dx at a corner** (for example d/dx(Abs(X), 0)) | Math ERROR (today) / 0 like the Casio | **Keep Math ERROR.** The slope really doesn't exist there, and a tutor product shouldn't print a wrong answer. Add one FAQ line explaining the difference. | Nothing (FAQ line) | `PN` "STAT, TABLE, CALC…" Decisions; `core/numeric.h` |
| D2 | **CALC, SOLVE, ∫dx and d/dx in CMPLX mode** (they do nothing there; the fx-115ES PLUS allows CALC) | Leave them off / add CALC later | **Leave them off for the samples, and add CALC in a later update** if testers ask. It's rarely used, and it needs a complex-number version of the CALC runner. | Nothing now | `FW` MODE 6 "Choices to confirm" |
| D3 | **Leaving CMPLX** keeps only the real parts of Ans, PreAns and the memories, and clears the replay | Keep / keep the imaginary parts too | **Keep** (it's what the Casio does). | — | `FW` MODE 6 |
| D4 | **CMPLX display details:** a real result stays real in r∠θ mode unless ▶r∠θ asks; R⇔I sits left of the result (no room in the top row next to the exam clock) | Keep / change | **Keep.** These are forced by the screen layout and match the Casio's behaviour. | — | `FW` MODE 6 |
| D5 | **BASE-N details:** menu order `1:and 2:or 3:xnor 4:xor` (the guide lists and, or, xor, xnor); no spaces around operators; "−" typed before a value is its sign; a value that doesn't fit after switching to BIN shows Math ERROR; steps may use 32 bits in BIN | Keep / reorder the menu to match the guide | **Keep, but check the menu order against a real fx-115ES before the samples ship.** Students' muscle memory comes from the real calculator. | 5-minute check on a real Casio | `FW` MODE 7 "Choices to confirm" |
| D6 | **MATRIX / VECTOR details:** entering MODE 8/9 clears the variables; no Ref/Rref (only in the 2nd-edition guide); scalar ± matrix and M^(n) are Syntax ERROR; 2D × 2D vectors are Dim ERROR; message reads `Dim ERROR`; dot drawn `·` | Keep / add Ref/Rref | **Keep. Leave Ref/Rref out** (the fx-115ES PLUS doesn't have them, and flash is at 86.0%). | — | `FW` MODE 8/9 "Choices to confirm"; `PN` |
| D7 | **EQN details:** results are not stored in Ans (as on the Casio); quadratics also show the vertex (X/Y-Value Minimum/Maximum) like newer Casio models | Keep / drop the vertex screens | **Keep both.** The vertex is useful and costs nothing. | — | `FW` MODE 5 |
| D8 | **SOLVE and Ans** (SOLVE puts the root in X and leaves Ans unchanged) | Keep (Casio) / also put it in Ans | **Keep.** It's what students expect from the Casio. | — | `PN` "STAT, TABLE, CALC…" Decisions |
| D9 | **STAT:** changing the SETUP Frequency setting clears the STAT data, and entering MODE 2 starts empty (as the Casio does) | Keep / keep the data | **Keep** (Casio-exact). | — | `FW` "Calculator modes" |
| D10 | **Typed problems (Type it)** are sent as a picture of the text through the photo path | Keep (no server change) / send plain text (cheaper per solve, needs a contract change) | **Keep for the samples; consider plain text after measuring costs.** It works today with no server or contract change. | — | V15 `docs/MODE4_APPS_SPEC.md`; `FW` Apps |

---

**Fastest path:** A1–A4 this week (they're quick), B1–B3 next (they gate everything with money), B5 as one
lawyer session, and leave D for a single "Casio check" session with a real fx-115ES on the desk.
