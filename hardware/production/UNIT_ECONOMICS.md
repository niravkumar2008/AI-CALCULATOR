# Unit economics: what each calculator costs and earns

Written 2026-10-06 for Nirav, **updated 2026-10-08 for the new plan** (5 v14 e-paper testers, then the v15-LCD board, a bigger battery later) and **again on the evening of 10/8**: the v15-LCD board is verified "ORDER" (`../verification/14_v15_final_preorder.md`), the panel is the **BuyDisplay ER-TFT019-1**, and the v15 JLCPCB order is now in the cash plan. Every number here comes from `COST_MODEL.xlsx` in this folder (rebuilt by `build_cost_model.py`). If you change a blue cell there, these numbers change with it. Companion files: `KNOCKOFF_SHELL_PLAN.md` (the clone shell), `QA_TEST_PLAN.md` (testing), `../stage15_lcd.md` (the LCD board), `Claude outputs/AI_Calculator_Shopping_List.xlsx` (what to buy now) and `Claude outputs/Alibaba_Bulk_Sourcing.xlsx` (bulk prices; the new sheet "v15-LCD cost by batch" has the LCD bill of materials).

**The short answer:**
- **The 5 testers cost about $664 in cash** with option C. That's $275 for the JLCPCB order (already paid on 10/8) plus $389 for the rest.
- **The LCD version costs the same as the e-paper one at 10 units and $2–3 less from 100 on:** $92 vs $92 at 10 units, $42 vs $46 at 100, $32 vs $34 at 1,000. At 10 the panel is the datasheet-verified BuyDisplay ER-TFT019-1 ($6.22 @10); from 100 on it is an Alibaba panel at $2.30–2.50 (after a sample check), and the simpler board pays for the extra socket and FETs.
- **The LCD drains the battery much faster**, though: about 1.5 h of screen-on time on the 150 mAh cell. The 1,200–1,500 mAh cell adds about $3 a unit at 100 ($2 at 1,000) and gives 13–17 h.
- **At $225 the calculator makes about $101–163 a unit** from 10 units on, whichever version you build.
- **The $15 subscription covers the Claude API for a typical student.** A heavy user at the 500-solve cap costs more than $15 on Opus 5.5, but not on Sonnet 5.5.
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

"Current" parts are the Seeed OV5640 AF camera (114993115, $12.99), the Waveshare 2.13" V4 screen ($6.99) and the Adafruit #1317 battery ($5.95). "Generic" parts are an AliExpress OV5640 AF 24-pin camera (~$7), the GDEY0213B74 screen (~$4.50) and a 302030 150 mAh JST-PH cell (~$3).

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

## 4. Does $15 a month cover the Claude API?

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

## 5. Where the $2,000 goes (new order of steps)

| # | What | When | $ |
|---|---|---|---|
| 1 | v14 JLCPCB order: 5 PCBs, 5 assembled, delivered | paid 10/8 | 275 |
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
