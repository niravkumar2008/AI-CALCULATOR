# Unit economics: what each calculator costs and earns

Written 2026-10-06 for Nirav, **updated 2026-10-08 for the new plan** (5 v14 e-paper testers, then the v15-LCD board, a bigger battery later) and **again on the evening of 10/8**: the v15-LCD board is verified "ORDER" (`../verification/14_v15_final_preorder.md`), the panel is the **BuyDisplay ER-TFT019-1**, and the v15 JLCPCB order is now in the cash plan. Every number here comes from `COST_MODEL.xlsx` in this folder (rebuilt by `build_cost_model.py`). If you change a blue cell there, these numbers change with it. Companion files: `KNOCKOFF_SHELL_PLAN.md` (the clone shell), `QA_TEST_PLAN.md` (testing), `../stage15_lcd.md` (the LCD board), `Claude outputs/AI_Calculator_Shopping_List.xlsx` (what to buy now) and `Claude outputs/Alibaba_Bulk_Sourcing.xlsx` (bulk prices; the new sheet "v15-LCD cost by batch" has the LCD bill of materials).

> **2026-10-11 (from Nirav):** v14 was **never ordered** and v15-LCD **has been ordered**. The $275 "already paid on 10/8" v14 order and the 5 v14 e-paper testers below didn't happen; the plan is **5 v15-LCD samples first, then bulk**. Prices are now $225 + AI tutor plan $10/month or $100/year (2 months free), first month free with no card, firmware updates only with the plan, per-account AI cap $5/month and 300 solves/month, buyers 18+. **Recomputed numbers: section 8** (the real v15 order total isn't recorded, so the $270 estimate is still used).

**The short answer:**
- **The 5 testers cost about $664 in cash** with option C. That's $275 for the JLCPCB order (already paid on 10/8) plus $389 for the rest.
- **The LCD version costs the same as the e-paper one at 10 units and $2–3 less from 100 on:** $92 vs $92 at 10 units, $42 vs $46 at 100, $32 vs $34 at 1,000. At 10 the panel is the datasheet-verified BuyDisplay ER-TFT019-1 ($6.22 @10); from 100 on it is an Alibaba panel at $2.30–2.50 (after a sample check), and the simpler board pays for the extra socket and FETs.
- **The LCD drains the battery much faster**, though: about 1.5 h of screen-on time on the 150 mAh cell. The 1,200–1,500 mAh cell adds about $3 a unit at 100 ($2 at 1,000) and gives 13–17 h.
- **At $225 the calculator makes about $101–163 a unit** from 10 units on, whichever version you build.
- *(Superseded 2026-10-11: the plan is $10/month or $100/year with a $5 / 300-solve cap; every paying user covers the API, see §8.)* **The $15 subscription covers the Claude API for a typical student.** A heavy user at the 500-solve cap costs more than $15 on Opus 5.5, but not on Sonnet 5.5.
- **Ordering v15 now (≈ $270 delivered, estimate) plus 3 panels (≈ $32) means the $2,000 no longer covers a 10-unit LCD pilot.** The full plan with a 10-unit pilot comes to about **$2,475, which is $475 over**. If the 5 assembled v15 boards become the first pilot units, a **5-unit pilot** is about $11 over, a **4-unit pilot** leaves about $62, and a 5-unit pilot with **Sonnet 5.5** for the beta leaves about $30 (section 5).

Most inputs are still estimates: the clone shell price, tariffs, the LCD panel quote, how many buyers keep paying, and how much students actually use it. Treat the numbers as "about right", not exact.

---

## 1. The 5 testers (v14 e-paper, genuine Casio shells)

The v14 board was ordered at JLCPCB on 2026-10-08 (5 PCBs, git tag `v14-order`; it stays frozen). The model assumes **all 5 boards assembled**, which adds about $55 over 2 assembled. If you picked 2 at checkout, change the PCBA input in the Shopping List.

| | Option A: all current parts | Option B: all generic | **Option C: units 1–3 current, 4–5 generic (recommended)** |
|---|---|---|---|
| JLCPCB delivered (5 PCBs, 5 assembled, incl. DHL + ~35 % duty) | $275 | $275 | $275 |
| 3 more Casio fx-115ES at ~$20 (you own 2) | $60 | $60 | $60 |
| Cameras, screens, batteries (6 of each kind = 1 spare) | $155.58 | $87.00 | $147.22 |
| 6 magnet pieces (#5358) + 5 cables (#5412) | $63.75 | $63.75 | $63.75 |
| Shipping (Adafruit, Seeed, Waveshare, AliExpress, as needed) | $47 | $25 | $57 |
| Supplies and tools (tape, glue, drill bits, vinyl, IPA, glasses) | $61 | $61 | $61 |
| **Total cash** | **$662.33** | **$571.75** | **$663.97** |

"Current" parts are the Seeed OV5640 AF camera (114993115, $12.99), the Waveshare 2.13" V4 screen ($6.99) and the Adafruit #1317 battery ($5.95). "Generic" parts are an AliExpress OV5640 AF 24-pin camera (~$7), the GDEY0213B74 screen (~$4.50) and a 302030 150 mAh JST-PH cell (~$3). The named bulk candidate is the **Taiwoo TW302030** (140 mAh, 30 × 20 × 3.0 mm, Alibaba, price not yet quoted, so the ~$3 estimate stands): CAD fit-checked 2026-10-10 on v14 and v15, it fits when placed **1 mm left of the #1317 position** (`../enclosure/final_assembly/battery_upgrade.md`, `variant_battery_tw302030/results.json`). Order spec: PCM, JST-PH 2.0 with red (+) on pin 2, 50–80 mm leads, UN38.3 + MSDS; meter every sample.

**Why C costs as much as A:** it pays shipping to both kinds of seller and buys a spare of each kind. It's still the better choice. The first 3 units use parts that are known to work, so any problem they show is a board problem. Units 4 and 5 then test the cheap parts that bulk production will need. Before using a generic camera, do the beep test on it; before plugging in a generic battery, meter its lead (reversed JST leads are common).

Valued per tester (including the 2 shells you already own), that's **$140.79 each**. Testers aren't sold, so they have no margin.

## 2. What one calculator costs at volume (e-paper vs LCD, small vs big battery)

The 10 / 100 / 1,000 columns assume a knockoff 991ES-style shell and the v15 board. The LCD version is the **v15-LCD** board from `../stage15_lcd.md`, verified "ORDER" on 2026-10-08 (review 14) but not ordered yet. It takes the 1.9" 170×320 IPS ST7789 30-pin panel (BuyDisplay ER-TFT019-1 for prototypes) instead of the e-paper. The board change takes out 17 parts (J2, L1, Q3, R11, R12, D3–D5, C20–C30) and adds J5 C2919501 ($0.36), Q4 AO3401A, Q5 AO3400A C20917, R21, R22, C36 and R23 15 Ω (C22810, an extended part since review 14). That's about $0.19 cheaper per board, plus one fewer JLCPCB extended part type (13 instead of 14: L1, Q3 and R12 leave, J5 and C22810 join).

**Cost per unit (parts, freight, scrap, samples, jig), before the selling-legally costs:**

| Version | 10 units | 100 units | 1,000 units |
|---|---|---|---|
| E-paper, 150 mAh | $91.56 | $45.66 | $33.68 |
| **LCD, 150 mAh (v15-LCD)** | **$91.92** | **$42.22** | **$31.69** |
| E-paper, 1,200–1,500 mAh | $96.51 | $48.71 | $35.83 |
| **LCD, 1,200–1,500 mAh (the long-term target)** | **$96.87** | **$45.27** | **$33.84** |

**Same costs plus the selling-legally costs spread over the batch** (FCC test, lawyer, LLC, trademark, insurance: $3,547 one-time; at 10 units the pilot is a free beta, so none of it is added there):

| Version | 10 units | 100 units | 1,000 units |
|---|---|---|---|
| E-paper, 150 mAh | $91.56 | $81.13 | $37.22 |
| LCD, 150 mAh | $91.92 | $77.69 | $35.23 |
| E-paper, 1,200–1,500 mAh | $96.51 | $84.18 | $39.38 |
| LCD, 1,200–1,500 mAh | $96.87 | $80.74 | $37.39 |

How to read it:
- **The LCD panel prices:**
  - **At 10 units:** BuyDisplay **ER-TFT019-1** (no touch), **$6.22 each at 10** ($5.71 at 100, ~$6–7 for 1–9) plus one shipping charge per order (≈ $12, estimate). Its datasheet was checked pin for pin against J5 (`../LCD_PANEL_OPTIONS.md`, `../verification/14_v15_final_preorder.md`). Adafruit #5394 is only an optional fallback.
  - **At 100 and 1,000 units:** the Alibaba search in `lcd_suppliers.json`, Zhengzhou Zhongjingyuan (ZJY) at **$2.50 @100 and $2.30 @1,000** (MOQ 2).
  - **Not confirmed yet.** Those prices come from search snippets, so get a written quote, check the drawing and pin table, and test a sample (diode test, tail length, frame thickness) first.
  - **Samples:** Shenzhen Goldenmorning T190X7-C30-01 is $1.85–2.50. Ask for the ~1.4 mm plastic-frame build, because the 2.11 mm metal frame is too thick.
  - **Short tail:** no listing gave a price for a short 18–20 mm FPC tail.
- **The bigger battery** is an Adafruit #258 (1,200 mAh, $9.95) at 10 units and an Alibaba 504060 (~1,500 mAh, ~$3–5, estimated at $4 / $3) at 100+. A larger dangerous-goods surcharge and a longer JST-PH lead are added on top. It only fits where there's a big pocket: the custom drop-in shell has **70 × 41 × 5.4 mm** under the keypad (`../enclosure/final_assembly/battery_upgrade.md`), and a knockoff only fits it if checks K-D7/K-S5 show the room. R2 should go 20 k → 4.7 k for a charge of about 3 h.
- **Why the big battery matters for the LCD:** with the screen on, the LCD version draws about 80–90 mA. That's ~1.5 h on the 150 mAh cell (charge every 1–2 days) and 13–17 h on 1,200–1,500 mAh. The e-paper drew almost nothing between refreshes.
- **From 10 to 100 units the cost halves** (board $18 → $9, camera $12 → $6, Adafruit retail parts → Alibaba).
- **The biggest unknown is still import duty.** The model assumes ~35 %, the low end of JLCPCB's 35–92.5 %. At the high end it adds about $8 a unit at 100.
- **Your own time isn't counted** at 5–100 units. That's about 37 min a unit, worth about $9 at $15 an hour.

## 3. What you keep on a $225 sale

| Per calculator sold (e-paper, 150 mAh) | 10 units | 100 units | 1,000 units |
|---|---|---|---|
| Price | $225 | $225 | $225 |
| Card fee (2.9 % + 30¢) | −$6.83 | −$6.83 | −$6.83 |
| Shipping it to the customer | −$6.00 | −$6.00 | −$6.00 |
| Returns / warranty allowance (3 %) | −$6.75 | −$6.75 | −$6.75 |
| Claude API for the free first month (Opus 5.5) | −$7.52 | −$7.52 | −$7.52 |
| Unit cost (incl. selling-legally at 100+) | −$91.56 | −$81.13 | −$37.22 |
| **Hardware profit, e-paper** | **$106** (47 %) | **$117** (52 %) | **$161** (71 %) |
| **Hardware profit, LCD** | **$106** | **$120** | **$163** |
| **Hardware profit, LCD + 1,200–1,500 mAh** | **$101** | **$117** | **$161** |

## 4. Does $15 a month cover the Claude API? (history; the 2026-10-11 prices are in §8)

**For a typical user, yes.** "Typical" means about 5 photo solves a day, or 150 a month.

| Claude API cost per user per month | 2 a day (light) | 5 a day (typical) | 10 a day (heavy) | at the 500/month cap |
|---|---|---|---|---|
| **Opus 5.5** ($4 / $20 per M tokens) | $3.01 | **$7.52** | $15.03 | **$24.72** |
| **Sonnet 5.5** ($2 / $10; the proxy's default) | $1.51 | **$3.77** | $7.54 | **$12.40** |
| Haiku 4.5 ($1 / $5) | $0.36 | $0.90 | $1.79 | $2.95 |

What's left from the $15 per paying user per month on Opus 5.5 (after the 74¢ card fee, the API and a share of the $25/month server): **$3.18** at 10 units, **$6.39** at 100 and **$6.71** at 1,000. A user at the 500 cap loses you about **$10.81 a month** at 100 units.

**Which model:**
- **Opus 5.5** costs about $7.5 per typical user a month and **loses money at the 500-solve cap**. To keep it, lower `MONTHLY_CAP` to about **280**, the break-even point (about 9.5 solves a day).
- **Sonnet 5.5** costs about $3.77 typical and $12.40 at the cap, so it **covers the cap with room to spare**. It's already the proxy's default.
- The model input in `COST_MODEL.xlsx` (Inputs → "Model used for solves") is still set to Opus 5.5, so every number in this file is the conservative one. Switch it to Sonnet 5.5 and the subscription profit roughly doubles.

Over a year, one buyer's subscription is worth about **$34 of profit** at 100 units (Opus). This assumes 70 % start paying after the free month and 8 % cancel each month, which is about 5.3 paid months out of 12. Both percentages are guesses until the testers and the pilot give real numbers.

## 5. Where the $2,000 goes (new order of steps) (superseded 2026-10-11 by §8.5)

| # | What | When | $ |
|---|---|---|---|
| 1 | v14 JLCPCB order: 5 PCBs, 5 assembled, delivered **(2026-10-11: never ordered)** | not paid | 275 |
| 2 | Tester parts, 3 shells, shipping, supplies (option C) | now, before boards land (~10/18–22) | 389 |
| 3 | v15-LCD prototype panels: 3 × BuyDisplay ER-TFT019-1 + shipping (estimate) | now, with the v15 order | 32 |
| 4 | **v15-LCD JLCPCB order** (verified ORDER): 5 PCBs, 5 assembled, delivered (estimate) | now | 270 |
| 5 | 4 knockoff shell samples from 2 sellers | during tester feedback | 60 |
| 6 | Bulk samples (camera, bare LCD panels, small + big cells, magnet pair) | after the shells pass | 90 |
| 7 | **v15-LCD pilot, 10 units in knockoff shells** (all parts, freight, scrap; the 5 boards from row 4 are re-used, so only 5 more boards) | after the v15 boards pass the bench checks | 679 |
| 8 | Proxy server, 3 months | beta | 75 |
| 9 | Claude API: free month for 5 testers + 10 pilot users (Opus) | beta | 113 |
| 10 | Law clinic / lawyer check (trade dress) | before units leave the house | 150 |
| 11 | Indiana LLC | before taking money | 97 |
| 12 | Domain and email | beta | 20 |
| | 10 % contingency | | 225 |
| | **Total** | | **2,475** |
| | **Left from $2,000** | | **≈ −475 (over)** |

**Ways back under $2,000** (all of them are inputs in `COST_MODEL.xlsx`):
- **A smaller pilot built from the 5 v15 boards:** 5 units ≈ $11 over, **4 units leaves about $62**. This is the simplest fix (Inputs → "v15-LCD pilot size"; the input "Assembled v15 boards from that order that go into the pilot" is 5).
- **5-unit pilot + Sonnet 5.5 for the beta** leaves about **$30**.
- **Sonnet 5.5 for the beta** saves about $62 (with 10 pilot users).
- **Option B for the testers** saves about $101, but it puts untested generic parts in every unit.
- **Selling some pilot units at $225,** if the lawyer says that's OK, brings in about $110 each.

**Not covered by the $2,000:** the first 100 public units (LCD version, ≈ $4,220 including the samples and the jig) and the selling-legally bill (≈ $3,550) come to **≈ $7,770**, or **about 36 pre-orders** at $225. Take the pre-orders **before** you pay for the 100.

## 6. Next sourcing actions

1. **Order the tester parts now** (Shopping List option C). Do the Adafruit order first, because #5412 cable stock is low.
   - Alternate / bulk source for the magnet parts (added 2026-10-10): Yiwei **MG04254FRA1S1N** = Adafruit #5358 (same part) and Yiwei **MG0425-UB-60-4P** cable = equivalent of #5412 (GND, D+, D−, V+ from its N end). Prices not checked yet; `Alibaba_Bulk_Sourcing.xlsx` 'Supplier options' rows 47–48.
2. **Order v15 and 3 × BuyDisplay ER-TFT019-1 (no touch)** (`../ORDER_WALKTHROUGH_v15_lcd.md`, Shopping List sheet "v15-LCD prototype"). No panel test is needed before ordering any more (review 14). When boards and panels arrive (`../ARRIVAL_CHECKLIST.md`, v15 section):
   - Diode test on the first tail: finger 1 at the J5 silk tick, the LED fingers in the top half of J5.
   - Backlight current = V(R23) / 15: expect 16–37 mA.
   - Panel picture at 3.3 V, `LCD_INVERT` / RGB-order flags, tail 36.6 ± 0.3 mm vs rib B, feel of the SHIFT / ALPHA / ON keys.
3. **Ask for a quote on the bare LCD panel:**
   - Samples from Goldenmorning (T190X7-C30-01, plastic frame).
   - A written quote from ZJY for 100 / 1,000, with the drawing and the pin table.
   - The message template is in `Alibaba_Bulk_Sourcing.xlsx`.
4. **Order the knockoff shell samples during tester feedback** (`KNOCKOFF_SHELL_PLAN.md` §1). Measure the battery spot (K-S5) and the AAA tube (K-D7) to see whether the 1,200–1,500 mAh cell fits.
5. **Send the 1,500 mAh 504060 RFQ** with the UN38.3 report in the seller's name, together with the camera RFQ.

## 7. Numbers to replace with real data first

| Guess today | Where it lives | Replace it with |
|---|---|---|
| JLCPCB v14 delivered $275 (5 assembled) | Shopping List inputs, Cash budget row 1 | The real order total from 10/8 |
| LCD panel $2.50 / $2.30 at 100 / 1,000 | `Versions` sheet; Alibaba sheet inputs | ZJY's or Goldenmorning's written quote after a sample check (and a short-tail FPC price) |
| v15 JLCPCB order ≈ $270 delivered; BuyDisplay shipping ≈ $12 | Inputs "v15-LCD JLCPCB order"; Versions "BuyDisplay shipping"; Shopping List | The real checkout totals |
| v15 board ≈ $0.19 cheaper, 13 instead of 14 extended parts | Inputs "v15-LCD board change" | JLCPCB's BOM page when v15 is quoted |
| 1,500 mAh cell $4 / $3, DG $1.20 / $0.50 | `Versions` sheet | The Alibaba quote with UN38.3 |
| 5 solves a day, 2,100 output tokens a solve | Inputs | The proxy's usage log from the testers |
| 70 % start paying, 8 % cancel a month | Inputs "Subscription behaviour" | Tester / pilot conversions |
| Clone shell $3.99 at 100 | Unit cost row 2 | The seller's written quote for the plain version |
| ~35 % import duty | Unit cost row 13 | The tariff line on the v14 JLCPCB checkout |

---

## 8. Update 2026-10-11: Proofline prices, v15 ordered, 5 samples (supersedes the numbers above where they conflict)

**Facts from Nirav (2026-10-11):** brand **Proofline** (company "Proofline Corporation", not registered yet). Device **$225**. AI tutor plan **$10/month or $100/year** (2 months free). **First month free, no card.** Firmware updates only with the plan. Per-account AI spend cap **$5/month, 300 solves/month** (was $15/month and a 500-solve cap; the "$8 cap" figure in some notes is also superseded). Buyers **18+**. **v14 was never ordered; v15-LCD has been ordered** (vendor, quantity, total and delivery date **not recorded — ask Nirav**). Plan: **5 samples first, then bulk.** Preorders in 2027.

Sections 1, 3, 4 and 5 above are kept as history. What changed: section 1 (v14 testers) didn't happen; the $15 subscription in sections 3–4 is now $10 / $100; the cash plan in section 5 loses the $275 v14 row, the $389 tester parts and the 10-unit pilot. Section 2 (unit cost) still stands: it already describes the v15-LCD board.

### 8.1 Inputs used (and where they come from)

| Input | Value | Source |
|---|---|---|
| AI cost per solve | **$0.026** | `Claude outputs/LAUNCH_ROADMAP.md` §3.3, Sonnet 5.5 row ($2 / $10 per M tokens, 2,500 in + 2,100 out per solve; token counts are estimates). §4 above gives $3.77/month with caching, close to the same. |
| Typical solves per user per month | **150** | Same table, "150/mo (typical)"; `COST_MODEL.xlsx` Inputs "5 solves a day". A guess until the sample log replaces it. |
| Stripe | **2.9 % + 30¢ per charge, + 0.7 % Billing** on subscriptions | 2.9 % + 30¢ from `COST_MODEL.xlsx` Inputs; 0.7 % Billing given by Nirav 2026-10-11. The device sale pays 2.9 % + 30¢ only. |
| Cloudflare Workers | **$5/month when needed** (free tier until then) | Given 2026-10-11. Replaces the $25/month proxy host. |
| Unit cost, LCD v15, 150 mAh | **$91.92 / $42.22 / $31.69** at 10 / 100 / 1,000 (incl. selling-legally: $91.92 / $77.69 / $35.23) | `COST_MODEL.xlsx` → Versions (section 2 above). No part prices were changed. |
| Selling-legally one-time | **$3,547** | `COST_MODEL.xlsx` (FCC $1,750, lawyer $300, LLC $97, trademark $700, insurance $700). |
| Conversion / churn | 70 % pay after the free month; 8 %/month cancel | `COST_MODEL.xlsx` Inputs (guesses). Yearly payers are assumed to stay the full year. |

### 8.2 Subscription money per paying user per month

| | Monthly $10 | Yearly $100 (per month) |
|---|---|---|
| Gross | $10.00 | $8.33 |
| Stripe 2.9 % + 0.7 % Billing + 30¢ | −$0.66 | −$3.90 a year = −$0.33 |
| **Net** | **$9.34** | **$8.01** ($96.10 a year) |
| AI, typical (150 × $0.026) | −$3.90 | −$3.90 |
| **Left, typical user** | **$5.44** | **$4.11** |
| Left, user at the $5 cap | $4.34 | $3.01 |

**Three mixes** (share of paying users on the yearly plan):

| | 30 % yearly | 50 % yearly | 70 % yearly |
|---|---|---|---|
| Gross per paying user-month | $9.50 | $9.17 | $8.83 |
| Net after Stripe | $8.94 | $8.67 | $8.41 |
| **Left after AI (typical)** | **$5.04** | **$4.77** | **$4.51** |
| Left after AI (at the $5 cap) | $3.94 | $3.67 | $3.41 |
| 12-month subscription value per device sold* | $30.35 | $31.54 | $32.73 |

\* Per device sold: 70 % convert; a monthly payer averages 7.50 paid months in months 2–12 at 8 % churn (= 7.50 × $5.44 = $40.82); a yearly payer gives one paid year ($96.10 − 12 × $3.90 = $49.30). Value = 0.7 × (yearly share × $49.30 + monthly share × $40.82). Yearly is less per month but worth more per buyer because nobody churns inside the year.

**What the cap means:** at $0.026 a solve the **$5 cap is reached at ≈ 192 solves**, before the 300-solve cap. So the worst any account can cost is $5.00 a month, and **every paying user is profitable even at the cap**. The 300-solve cap only binds if a solve costs less than $0.0167 (for example Haiku-first routing, ≈ $0.010 in `LAUNCH_ROADMAP.md` §3.3). On Opus 5.5 ($0.052) the $5 cap would hit at ≈ 96 solves.

**Free first month, no card:** every activated device costs up to $5 of AI (typical $3.90) whether or not it converts. It's counted once, in the hardware profit below.

**Cloudflare $5/month** is covered by the first subscriber ($5.04 left at a 30 % yearly mix).

### 8.3 Hardware profit per $225 unit (LCD v15, 150 mAh)

| Per unit | 10 | 100 | 1,000 |
|---|---|---|---|
| Price | $225.00 | $225.00 | $225.00 |
| Card fee 2.9 % + 30¢ | −$6.83 | −$6.83 | −$6.83 |
| Shipping to the customer | −$6.00 | −$6.00 | −$6.00 |
| Returns / warranty 3 % | −$6.75 | −$6.75 | −$6.75 |
| Free-month AI (typical, Sonnet) | −$3.90 | −$3.90 | −$3.90 |
| Unit cost incl. selling-legally from 100 | −$91.92 | −$77.69 | −$35.23 |
| **Hardware profit** | **$109.60** | **$123.83** | **$166.29** |
| Old (section 3, free month on Opus $7.52) | $106 | $120 | $163 |

Without the selling-legally share, the 100-unit contribution is **$159.30** (= $201.52 − $42.22).

### 8.4 Break-even

| Fixed costs to recover | ÷ contribution per unit | Units |
|---|---|---|
| Selling-legally $3,547 | $159.30 (hardware, 100-unit variable cost) | **23** |
| $3,547 + the $2,000 budget + 12 × $5 Cloudflare = $5,607 | $159.30 (hardware only) | **36** |
| Same $5,607 | $190.84 (hardware + 12-month subscription at 50 % yearly) | **30** |

### 8.5 Cash plan vs the $2,000 (replaces section 5)

| # | What | When | $ |
|---|---|---|---|
| 1 | v15-LCD order (ordered; **real total not recorded**, model estimate) | ordered by 2026-10-11 | 270.00 |
| 2 | Parts for the **5 v15 samples** (camera, LCD panel, battery, magnet, cable, shell, freight, scrap) = 5 × ($91.92 − $18.00 board) | on arrival | 369.60 |
| 3 | Knockoff shell samples (4 from 2 sellers) | Nov – Dec | 60.00 |
| 4 | Bulk supplier samples (camera, bare LCD, cells, magnet pair) | after the shells pass | 90.00 |
| 5 | Cloudflare Workers, 3 months | at deploy | 15.00 |
| 6 | AI for the 5 sample users' free month, at the $5 cap | samples | 25.00 |
| 7 | Lawyer / IP clinic | before units leave | 150.00 |
| 8 | Company filing (repo figure = Indiana LLC $97; a corporation's fee isn't in the repo) | before taking money | 97.00 |
| 9 | Domain + email | at deploy | 20.00 |
| | Subtotal | | 1,096.60 |
| | 10 % contingency | | 109.66 |
| | **Total** | | **1,206.26** |
| | **Left from $2,000** | | **≈ 793.74** |

The old v14 row ($275), the tester parts ($389), the 3-panel line ($32, now inside row 2) and the 10-unit pilot ($679) are gone. Optional: 5 more pilot units with new boards (5 × $91.92 + 10 % = $505.56) would leave ≈ $288.

**Not inside the $2,000:** the first 100 public units (≈ $4,220) + selling-legally (≈ $3,550) ≈ **$7,770** (section 5 figure). Using the ≈ $794 left, that needs **≈ 32 preorders** at $225 after card fees ($218.18 each).

### 8.6 Numbers to replace first (new)

| Guess | Replace with |
|---|---|
| v15 order ≈ $270 | **The real v15 order total, vendor, quantity, delivery date (not recorded — ask Nirav)** |
| 150 solves/month, $0.026/solve | Real usage from the 5 samples and the Cloudflare Worker logs |
| 70 % convert after a no-card free month | Sample and preorder data |
| Yearly share 30–70 % | Stripe test-mode → live data |
| Company filing $97 (LLC) | The real fee for Proofline Corporation in its state |
