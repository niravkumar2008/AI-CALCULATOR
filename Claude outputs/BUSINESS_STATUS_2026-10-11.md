# Proofline: business status, 2026-10-11

One page. Facts from Nirav on 2026-10-11 override older docs. Numbers marked *(est.)* are model estimates, not invoices. Full working: `hardware/production/UNIT_ECONOMICS.md` §8 and `Claude outputs/LAUNCH_ROADMAP.md` §8.

## Where everything stands

| Area | State on 2026-10-11 |
|---|---|
| Brand / company | **Proofline**; company name **Proofline Corporation**, **not registered yet**. Domain and email are placeholders (`proofline.example`). Trademark search not done. |
| Price | Device **$225**. AI tutor plan **$10/month or $100/year** (2 months free). **First month free, no card.** Firmware updates only with the plan. Per-account AI spend cap **$5/month and 300 solves/month**. Buyers **18+**. (Replaces $225 + $15/month, 500-solve cap.) |
| PCB v14 (e-paper) | **Never ordered.** It was a refinement step only. Older docs that say "v14 ordered 10/8" are wrong; dated corrections were added. |
| PCB v15 (colour LCD) | **Ordered.** Vendor, quantity, total paid and delivery date are **not recorded — ask Nirav.** Planning figure in the model: JLCPCB, 5 PCBs, 5 assembled, ≈ $270 delivered *(est.)*. Screen decision is final: colour LCD (BuyDisplay ER-TFT019-1). |
| Hardware plan | **Build 5 samples first, then bulk-focused production.** The old 5 e-paper testers and the 10-unit pilot are dropped. |
| Software (built, tested, **not deployed**) | Website: 28 pages, store + subscriptions (**Stripe test mode only**), waitlist. Cloudflare Worker API (stages 1 + 3). v15 firmware with MODE 4 Apps (AI Solve, Type it, History, Notes, Geography, Periodic Table, Settings). Casio-exact calculator screens. Desktop + browser simulators. |
| Software still to do | Firmware → new Cloudflare API (stage 2; it still talks to the old Python proxy). Turnstile. Real email sending. Deploy. |
| Sales | **Preorders planned for 2027.** Nothing is sold; no live payments. |

## Key numbers (recomputed 2026-10-11)

**AI cost:** ≈ **$0.026 per solve** (Sonnet 5.5; `LAUNCH_ROADMAP.md` §3.3 table, tokens are estimates) × 150 solves typical = **$3.90 per user per month**. The $5 cap is reached at **≈ 192 solves** on Sonnet, so on Sonnet the 300-solve cap never binds. Worst case per account = **$5.00/month**.

**Subscription, per paying user per month** (Stripe 2.9 % + 30¢ + 0.7 % Billing; typical use):

| Share of payers on the yearly plan | 30 % | 50 % | 70 % |
|---|---|---|---|
| Gross revenue | $9.50 | $9.17 | $8.83 |
| After Stripe + Billing | $8.94 | $8.67 | $8.41 |
| Minus AI at $3.90 | **$5.04** | **$4.77** | **$4.51** |
| Same, user at the $5 cap | $3.94 | $3.67 | $3.41 |

Monthly plan: $10 → $9.34 net. Yearly: $100 → $96.10 net ($8.01/month). Every paying user is profitable even at the cap. Cloudflare Workers $5/month is covered by about 1 subscriber.

**12-month subscription value per device sold** (model guesses: 70 % convert after the free month, 8 %/month churn on monthly; yearly payers stay the year): **$30.35 / $31.54 / $32.73** at 30 / 50 / 70 % yearly.

**Hardware profit per $225 unit** (LCD v15, 150 mAh; unit costs from `COST_MODEL.xlsx` → Versions; minus card fee $6.83, shipping $6, 3 % warranty $6.75, free-month AI $3.90):

| Units | 10 | 100 | 1,000 |
|---|---|---|---|
| Unit cost (incl. selling-legally from 100) | $91.92 | $77.69 | $35.23 |
| **Profit per unit** | **$109.60** | **$123.83** | **$166.29** |

(Old figures $106 / $120 / $163 assumed the free month on Opus at $7.52.)

**Break-even:** selling-legally costs ≈ $3,547 one-time ÷ $159.30 hardware contribution at the 100-unit variable cost ($42.22) = **23 units**. Also recovering the $2,000 budget and a year of Cloudflare: **36 units** on hardware alone, **30 units** with 12 months of subscription (50 % yearly).

**Cash vs the $2,000** *(est.)*: v15 order $270 (real total not recorded) + parts for 5 samples $369.60 + knockoff shell samples $60 + bulk samples $90 + Cloudflare 3 months $15 + free-month AI for 5 users at the cap $25 + lawyer $150 + entity filing $97 (repo has only the Indiana LLC fee; a corporation's fee isn't in the repo) + domain/email $20 = $1,096.60, + 10 % contingency = **≈ $1,206 → ≈ $794 left**. Adding 5 more pilot units (≈ $506 with contingency) would leave ≈ $288.

**Not inside the $2,000:** first 100 public units + selling-legally ≈ **$7,770** (repo figure) → **≈ 32 preorders at $225** after card fees, using the ≈ $794 left.

## Timeline to the 2027 preorders (proposed; dates depend on the unrecorded v15 delivery)

| When *(est.)* | What |
|---|---|
| Oct – Nov 2026 | v15 boards arrive (JLCPCB is typically 10–14 days door to door). Bench checks (`hardware/ARRIVAL_CHECKLIST.md` §4). Build sample #1 end to end, then samples 2–5. |
| Nov – Dec 2026 | Software stage 2 (firmware → Cloudflare API), Turnstile, real email, deploy site + Worker (Stripe stays in test mode). 5 samples in use by adult testers; collect solves/day and battery life. |
| Dec 2026 – Feb 2027 | Knockoff shell + bulk supplier samples, LCD panel quote. Lawyer/IP clinic. Register Proofline Corporation, trademark search, real domain. |
| Q1 – Q2 2027 | FCC Part 15B SDoC plan, product insurance, Stripe live. **Open preorders** (target ≈ 32+ to fund 100 units). Then the 100-unit batch. |

## Next 10 actions for Nirav

1. **Record the v15 order** (vendor, PCB and assembled quantity, total paid incl. shipping and duty, order date, expected delivery) in `hardware/ORDER_WALKTHROUGH_v15_lcd.md` §8 and `COST_MODEL.xlsx` Inputs → "v15-LCD JLCPCB order".
2. **Check whether the ER-TFT019-1 panels were bought;** you need 5 for 5 samples (+1 spare).
3. **Buy the other sample parts** (camera, #1317 battery, #5358 magnet, #5412 cable, shells) and decide the sample shell: genuine Casio (fine for samples, not for sale) or knockoff.
4. **Run the doc update package** (dry-run first), then `graphify update .`, and rebuild `COST_MODEL.xlsx` with the new Inputs.
5. **Software stage 2:** point the v15 firmware at the Cloudflare Worker API; retire the Python proxy.
6. **Turnstile + real email sending, then deploy** the site and Worker (Stripe test mode until the company and bank account exist).
7. **Trademark knockout search for "Proofline"** (USPTO, classes 9 and 42) and a domain check before buying either.
8. **Decide the company form** (corporation vs LLC, state, who signs if you're under 18) and get its real fee.
9. **Lawyer / IP clinic:** trade dress, subscription terms, "firmware updates only with the plan", 18+ terms.
10. **On arrival:** bench checks, sample #1 end to end, then log real solves/day to replace the 150-solve guess.

## Open decisions

- **Which model serves solves** (Sonnet 5.5 assumed). On Opus 5.5 ($0.052/solve) the $5 cap hits at ≈ 96 solves; the advertised 300 is only reachable with Haiku-first routing (≈ $0.010/solve).
- **Whether to advertise "300 solves"** when the $5 cap binds first on Sonnet (≈ 192).
- **Firmware updates only with the plan:** do security fixes go to everyone?
- **Free month with no card:** the 70 % conversion guess is shakier without a card on file.
- **Pilot beyond the 5 samples** before preorders, or straight to preorders?
- **Preorders:** own Stripe store vs Crowd Supply/Kickstarter; deposit vs full price; month in 2027; whether money can be taken before the FCC SDoC (ask the lawyer).
- **Company:** Proofline Corporation vs LLC, and when to register.
- **Real v15 order numbers** — not recorded; every cash figure above uses the $270 estimate.
