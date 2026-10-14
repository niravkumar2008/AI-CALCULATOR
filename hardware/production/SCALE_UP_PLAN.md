# Scale-up plan: from 5 samples to the first 100, then 1,000 Proofline calculators

Written 2026-10-11. Companion to `../SAMPLES_BRINGUP_PLAN.md` (the 5 samples), `UNIT_ECONOMICS.md` §8 (the money), `KNOCKOFF_SHELL_PLAN.md` (the shell), `QA_TEST_PLAN.md` (testing), `../LCD_PANEL_OPTIONS.md` (panels) and `Claude outputs/LAUNCH_ROADMAP.md` §3–§7.

**Ground rules for the numbers.** Every cost below comes from a repo file (named next to it) or from a web source listed at the end with its date. Numbers marked *(est.)* are model estimates, not quotes or invoices. **The v15 order's vendor, quantity, total and delivery date are not recorded** (`Claude outputs/BUSINESS_STATUS_2026-10-11.md`); the model's $270 is a placeholder. Nothing in this file has been bought, booked or quoted.

---

## 1. The plan in one screen

| Stage | Units | When (target, not a promise) | Shell | Who builds | Cash needed | Paid by |
|---|---|---|---|---|---|---|
| **A. Samples** | 5 | Day 0 → ~Day 21 (Oct–Dec 2026) | Genuine fx-115ES (not for sale) | You | ≈ $1,206 total plan incl. the v15 order *(est.)* | The $2,000 budget → ≈ $794 left (`UNIT_ECONOMICS.md` §8.5) |
| **B. Production-intent** | 5–10 | Dec 2026 – Mar 2027 | Knockoff (plain clone) or custom | You | Shell + bulk samples $150 (in stage A's plan) + a knockoff-board prototype order (≈ $270 *(est.)*, **not in the model**) | Budget remainder |
| **C. First 100** | 100 | Order ~May 2027, ship Jun–Jul 2027 | Same as B | You (≈ 37 min/unit) | ≈ **$7,770** (parts ≈ $4,220 + selling-legally ≈ $3,550; `UNIT_ECONOMICS.md` §5/§8.5) | Preorders: ≈ 32 own-site or ≈ 34–35 Kickstarter backers (§8) |
| **D. 1,000** | 1,000 (2 × 500) | Decide Jul 2027; first 500 for back-to-school only if C sells | Same, or custom if the clone fails | EMS box build | ≈ **$31,690** parts at the 1,000 unit cost $31.69 *(est.)* (`COST_MODEL_update_2026-10-11.csv`) | Stage C profit + new preorders (§8) |

The single most important rule (from `LAUNCH_ROADMAP.md` §6): **take the money for each step before you pay for it.**

---

## 2. What changes between the samples and production

| Area | Samples (now) | First 100 | 1,000 | Source |
|---|---|---|---|---|
| **Shell** | Genuine Casio fx-115ES, ground by hand with jigs | **Plain 991ES-style clone** ($3.25–3.99 at MOQ 20, plain = no logo or model text) **if** it passes the 9-point go/no-go; else custom 3D-printed shell + clone key mat | Clone, or custom (3D print / urethane). Injection moulds only pay off above ≈ 3,300 units | `KNOCKOFF_SHELL_PLAN.md` §1, §4; `LAUNCH_ROADMAP.md` §3.2 |
| **Board** | v15-LCD (fx-115ES hole map) | **A new board revision** laid out for the clone's posts and key mat (outline, holes, 50 pad positions). Add pogo test pads (3.3 V, GND, EN, IO0, D+/D−, VBAT) and our own silk logo. Schematic, camera GPIO map, J1/J5 reversed footprints, J3 pin order stay | Same board, bigger batches | `KNOCKOFF_SHELL_PLAN.md` §3 |
| **Jigs** | A/A2/A3/B/C for the Casio shell | **Re-make for the clone** (or none if the clone needs no grinding: "no solar box" version, K-D6) | Same | `../enclosure/jigs/README.md`; `KNOCKOFF_SHELL_PLAN.md` §2 |
| **LCD panel** | BuyDisplay ER-TFT019-1, $6.22 @10 | Alibaba panel ≈ $2.50 @100 after a **sample check** (pin table, diode test, tail 36.6 mm, plastic frame ~1.4 mm) | ≈ $2.30 @1,000 (ZJY tier); optional custom short tail (18–20 mm) | `../LCD_PANEL_OPTIONS.md`; `UNIT_ECONOMICS.md` §2 |
| **Camera** | Seeed OV5640 AF ($12.99) | Generic OV5640 AF 24-pin, beep-tested (≈ $6 @100 *(est.)*) | ≈ $4.50 *(est.)* | `LAUNCH_ROADMAP.md` §3, §6 |
| **Battery** | Adafruit #1317 150 mAh | #1317 or **Taiwoo TW302030** 140 mAh (CAD-fits 1 mm left of the #1317 spot) with the supplier's **UN38.3** summary | Custom LiPo with UN38.3 + IEC 62133 from the supplier (MOQ ≈ 500–1,000) | `UNIT_ECONOMICS.md` §1, §2; `LAUNCH_ROADMAP.md` §3.1, §6 |
| **Magnet connector** | Adafruit #5358/#5412 | Same, or Yiwei MG04254FRA1S1N + MG0425-UB-60-4P (meter-check every lot) | OEM pogo pair (needs a footprint change) at ≈ 500 | `UNIT_ECONOMICS.md` §6; `LAUNCH_ROADMAP.md` §6 |
| **Test** | Serial Monitor by hand | Pogo-pin jig + PC script: flash → self-test JSON → enrol → label (~2 min/unit *(est.)*; jig $50–150 *(est.)*) | The EMS runs your jig; you get the CSV | `LAUNCH_ROADMAP.md` §5 |
| **Server** | Python proxy (Worker built, not deployed) | **Cloudflare Worker only**; every board enrolled at QA | Same | `firmware-v15-lcd/README.md`; `BUSINESS_STATUS` |

### 2.1 LCD panel supplier path (Sungood / Goldenmorning / ZJY)

All three sell the same 1.9" 170×320 ST7789 30-pin glass family as the ER-TFT019-1, but only some are checked:

| Supplier | What we know | Price seen | Checked? | Role |
|---|---|---|---|---|
| **Shenzhen Sungood** (Alibaba, 4 yrs, 4.7/5) | Listing drawing: 25.8 × 49.72 × **1.43 mm**, tail **36.6 ± 0.3**, pin table identical to J5. Take the first (plastic-frame) variant, **not** the "glue-and-metal-frame" one | $2.30–3.20, tiers not visible | **Yes, from Nirav's screenshots** (2026-10-09) | Nirav's preferred listing; sample first |
| **Shenzhen Goldenmorning** T190X7-C30-01 (8 yrs, 4.7/5, maker with a public datasheet) | Datasheet pins 1–30 match; tail 36.6 ± 0.5; published drawing is the **2.11 mm metal frame** (too thick) | $1.85–2.50, MOQ 1 | **Yes (datasheet)**; ask for the plastic-frame build | Sample; volume fallback; custom-tail quote |
| **Zhengzhou Zhongjingyuan (ZJY)** (5 yrs, 5.0/5, 1 review) | Only listing with published tiers | **$2.50 @2–999, $2.30 @1,000–4,999**, MOQ 2 | **No** (drawing couldn't be opened) | The model's 100/1,000 price; needs drawing + pin table + 2-piece check before a PO |

Sources: `../LCD_PANEL_OPTIONS.md` (Alibaba table 2026-10-08, Sungood section 2026-10-09). Tier prices came from search snippets, so they are not quotes.

**Quote request.** A draft request ("5 samples, then bulk") is in the conversation notes, not in the repo. Before sending, make sure it asks the three questions in `../LCD_PANEL_OPTIONS.md` "Ask the supplier before ordering": (1) drawing + 30-pin table for the exact part number and the frame thickness, (2) the real controller (no DJ9853 clone), init code and backlight Vf/current, (3) the price and MOQ for an 18–20 mm short tail plus the NRE, the unit price at 100 / 500 / 1,000, and the sample and production lead times. Ask for tray + ESD-bag packing. Keep everything on-platform, pay by Trade Assurance, and never pay more than a 30 % deposit on a bulk order (`KNOCKOFF_SHELL_PLAN.md` §1).

**Panel acceptance test (every new supplier and every lot):** diode test on fingers 20/22, tail length 36.6 ± 0.3 mm, frame thickness ≤ 1.6 mm, colour bars right with the frozen `LCD_INVERT`/`LCD_RGB_ORDER` flags, backlight current via V(R23)/15 in 16–37 mA (`../ARRIVAL_CHECKLIST.md` §4).

**Custom short tail:** only worth it at 1,000+. No supplier has published an MOQ or tooling charge; expect a one-off FPC tooling charge and around 1k pieces, **not confirmed** (`../LCD_PANEL_OPTIONS.md`). The current tail works with one bow under the board (`../enclosure/final_assembly_v15_lcd/REPORT.md` §4), so the short tail is an assembly-speed and reliability improvement, not a must.

---

## 3. PCBA ordering in batches

| Batch | What | Why this size | Cost basis | Lead time |
|---|---|---|---|---|
| **Prototype of the knockoff board** | 5 PCBs, 5 assembled (same JLCPCB settings as `../ORDER_WALKTHROUGH_v15_lcd.md`: 0.8 mm, ENIG, Standard PCBA, rails right + bottom) | Proves the new outline, holes and pads in the clone shell before 100 boards are bought | ≈ $255–285 delivered for a v15-sized order *(est., `ORDER_WALKTHROUGH_v15_lcd.md` §8)* | JLCPCB quotes about 5–7 days for standard assembly (2026 comparison, web) plus shipping; the repo plans **10–14 days door to door** |
| **First 100** | 100 assembled in **one** order | Setup, stencil and the 13 extended-part fees ($3 each) are paid once per order; board cost falls to ≈ $9 at 100 | PCB + PCBA ≈ $9.00/board @100 *(est., `LAUNCH_ROADMAP.md` §3)* | Same, plus parts-stock risk (below) |
| **1,000** | **2 × 500**, 4–6 weeks apart | Second batch can take fixes from the first; less money tied up; only $0.60/board more than one 1,000 order | ≈ $7.90 @500 vs $7.30 @1,000 *(est., `LAUNCH_ROADMAP.md` §3)* | Same |

**Before every PCBA order:**
- [ ] Check stock of **U1 ESP32-S3-MINI-1-N4R2 (C3013941)**: it was the tightest part (1,218 in stock on 2026-10-08). **Never accept a substitute** (an N8R8 loses IO33–37). If short, order bare PCBs and pre-order the module (`../ORDER_WALKTHROUGH_v15_lcd.md` §4).
- [ ] R23 may only be another 15 Ω 0603.
- [ ] Upload the CPL exactly as generated (rotation offsets already applied; don't rotate in the preview).
- [ ] Ask for X-ray of U1 and U6; photo confirmation; review before payment.
- [ ] Put the real checkout total (with duty) into `COST_MODEL.xlsx`; duty is the biggest unknown (model assumes ≈ 35 %, the low end of 35–92.5 %; at the high end ≈ +$8/unit at 100: `UNIT_ECONOMICS.md` §2).
- [ ] Avoid ordering into Chinese holidays: JLCPCB publishes a holiday schedule each year (the Sept/Oct 2026 notice is on its news page); Chinese New Year 2027 falls in early February, and suppliers typically pause for 1–3 weeks around it. Place the stage-B orders by mid-January.

---

## 4. Test fixtures and enrolment at QA

### 4.1 The QA line for 100 units (one bench, one person)
Per board, in this order (times are `QA_TEST_PLAN.md` / `LAUNCH_ROADMAP.md` §5 estimates):

1. **Incoming (Part A1–A3, ~5 min):** visual, loupe, meter. Every board at 100 and at 1,000 (`QA_TEST_PLAN.md` Part C).
2. **First power + flash (A4):** on the pogo jig, USB only. Flash `firmware-v15-lcd` (or its successor for the knockoff board) over USB (the two-slot OTA table can only be written over USB).
3. **Self-test** with panel + camera attached: save the `SELFTEST_JSON` line.
4. **Enrol** (below).
5. **Assemble**, then **Part B** on the closed unit: B1 look/feel, B2 self-test (SHIFT + ALPHA + ON), B3 charge 40–60 mA, B4 focus card, B6 magnet polarity; replace B5 (e-paper) with the panel checks (colour bars, no pressure marks, no dust).
6. **Label and pack (B7):** serial/device_id/firmware/QA date; battery at 40–80 % for shipping; quick-start card with "Not affiliated with Casio" and the FCC line.

Stop-the-line rule at 100: 3 units in a row fail the same step, or > 5 % fail B2 on the first run. At 1,000: first-pass yield < 95 % (`QA_TEST_PLAN.md` Part C).

### 4.2 Enrolment (security review S1)
Only an enrolled board can start the free month (`firmware-v15-lcd/README.md` "Enrolment").

- [ ] Read the board's hw_id from `status` (`calc-` + 12 hex digits).
- [ ] On the QA PC: `ADMIN_TOKEN` set **only** as an environment variable; run `node web/api/scripts/enroll-board.mjs <hw_id> https://api.<domain>` (https only; plain http is refused except to localhost).
- [ ] Type the printed `enroll hs_…` line into the board's serial console. The secret is stored in NVS `factory`, never printed again, and survives `forget` and unlinking.
- [ ] `api` shows "board enrolled". Log hw_id + serial + date in `qa_log.csv`; **never log the `hs_…` secret.**
- [ ] Run `register` / show the link code once to confirm, but **don't link it**: the buyer links it and starts their own free month (`QA_TEST_PLAN.md` B7).

**To build before 100 units (software tasks, not done yet):** a PC script that reads `status`, calls the enrol endpoint, and writes the `enroll` line over serial in one step, plus prints the label. (`LAUNCH_ROADMAP.md` §5 "Automated flashing"; `esptool` + Python.)

**At 1,000 with an EMS:** don't give the factory your `ADMIN_TOKEN`. The secret has to be written into each board, so a hw_id list alone isn't enough. Options: (a) units come to you for a last enrol + spot-QA step, or (b) build a limited, revocable "enrol-only" credential for the EMS (a software task). Decide before the EMS quote (decision table D11).

---

## 5. Certification path (FCC, ISED, CE)

Not legal advice; ask the lawyer/clinic before taking money (`MARKETING_GUIDE.md` §13).

| Market | What applies | How | Cost | Source |
|---|---|---|---|---|
| **USA (launch market)** | **FCC Part 15B, Class B digital device, Supplier's Declaration of Conformity (SDoC).** The Wi-Fi is inside the **ESP32-S3-MINI-1**, which has Espressif's own modular grant (grantee 2AC7Z), so you test only as an unintentional radiator | 1. Pick an accredited lab; 2. optional pre-scan on a stage-B unit; 3. test a **production-intent** unit (final board, final shell, final firmware) ~1 lab day; 4. sign the SDoC; 5. label "Contains FCC ID: 2AC7Z-…" (look up the exact ID) and add the SDoC compliance statement to the manual. The SDoC needs a **US responsible party** (company name, address); every unit must stay identical to the one tested | **$1,000–2,500** range; model uses **$1,750** | `LAUNCH_ROADMAP.md` §4 [S10][S22]; `COST_MODEL_update_2026-10-11.csv`; 47 CFR §2.1077 (web) |
| USA, battery shipping | **UN38.3** test summary + SDS for the cell | From the battery supplier ($0); ≈ $1,500 per model if you pay | `LAUNCH_ROADMAP.md` §4, §3.1 [S13] |
| USA, children's products | **CPSIA only if marketed to 12 and under** | Not needed: buyers are 18+ and marketing is 13+ (General Certificate of Conformity only) | — | `LAUNCH_ROADMAP.md` §4; `BUSINESS_STATUS` |
| **Canada (optional, at 100+)** | **ISED ICES-003** (Class B) for the host; the module's own ISED certification covers the radio | Ask the FCC lab to test to ICES-003 in the same session; label "Contains IC: …" for the module and add the ICES-003 statement | **Not in the model: get a quote** | Digi / Lantronix module docs (web) |
| **EU / UK (defer)** | **CE under the Radio Equipment Directive** (the product contains a radio): safety EN 62368-1, EMC EN 301 489-1/-17, plus radio and RF-exposure assessment (the module's CE reports help), an EU representative, WEEE and battery rules; UKCA similar | Not before 1,000 units | **Not in the model; get a quote** | Espressif / DEKRA / TI docs (web) |

**Recommendation:** launch **US only** (Kickstarter shipping limited to the US), add Canada if the lab quotes ICES-003 cheaply in the same visit, and leave the EU/UK for after 1,000 units. The selling-legally line in the model ($3,547 = FCC $1,750 + lawyer $300 + company $97 + trademark $700 + insurance $700) covers the US path only.

**Timing rule:** test the unit you will ship. Any change to the board, shell material or radio settings after the test means re-checking with the lab. So the FCC test comes **after** stage B is frozen and **before** shipping stage C. Ask the lawyer whether preorder money can be taken before the SDoC is signed (`BUSINESS_STATUS` open decisions).

---

## 6. Packaging, fulfilment and returns

### 6.1 Packaging (from `QA_TEST_PLAN.md` B7 and `LAUNCH_ROADMAP.md` §3)
In each box: calculator (battery 40–80 %), magnetic USB cable, quick-start card (setup, link URL, "AI can make mistakes", "Not affiliated with Casio Computer Co., Ltd.", FCC statement, warranty summary), serial label inside the battery-lid recess. Budget: packaging + QC **$2.00/unit at 100, $1.20 at 1,000** *(est.)*. Our own box, never the clone's box.

### 6.2 Fulfilment options

| Option | Fits | Costs | Notes |
|---|---|---|---|
| **Ship yourself** (USPS Ground Advantage or similar) | Stage C (100) | Model: **$6.00/unit** to the customer (`UNIT_ECONOMICS.md` §8.3) | USPS allows small lithium-ion cells **installed in equipment** domestically without special markings up to 4 cells / 2 batteries per parcel (USPS Postal Bulletin, web). Keep the UN38.3 summary on file. Batch shipping days (e.g. 20 parcels a session) |
| **3PL: ShipMonk** | 1,000 if orders are steady | $0 setup, **$250/month minimum**, ~$2.50–3.50 per order pick fees (2026 reviews) | Better than ShipBob under ~400 orders/month (web) |
| **3PL: ShipBob** | Later | **$275/month minimum, ~$975 implementation; asks for ~400 orders/month** (2026 reviews) | Too big for year 1 |
| **Amazon FBA** | 1,000+ (repo plan) | ≈ $3.15/unit fulfilment + 3.5 % fuel + 8–15 % referral *(est.)* | Needs UN38.3 + SDS; `LAUNCH_ROADMAP.md` §4 "Sales channels" |

**Recommendation:** ship yourself for stage C. For stage D, only move to a 3PL once you see more than about 200 orders a month for two months in a row; otherwise the monthly minimums eat the margin.

### 6.3 Returns, warranty and RMA
- **Budget:** 3 % of price = **$6.75/unit** (model, `UNIT_ECONOMICS.md` §8.3). Roadmap support estimate at 1k+: 2–5 % of units need help or replacement *(est.)*.
- **Policy (decide with the lawyer; `/refunds/` page is still "not decided": `MARKETING_GUIDE.md` §10):** recommend a **30-day return** and a **1-year limited warranty** on the hardware. Under US federal law a written warranty on a consumer product above $15 has to be available to read before purchase; let the lawyer word it.
- **RMA flow:**
  1. Customer contacts support with serial and device_id (on the label and in Settings → About).
  2. Try remote first: firmware update (OTA), `forget` + re-setup, re-link via the site.
  3. If hardware: send a prepaid label; **ship a replacement from a spare pool** (keep 3 spares per 100 units) and repair the returned unit at the bench using the `SAMPLES_BRINGUP_PLAN.md` §8 table.
  4. Log every RMA (serial, fault, fix) next to the unit's original QA JSON: that is the input for the next board revision.
  5. Revoke a stolen or replaced unit's device on the server (the Worker's `device_revoked` path clears it on the calculator).
- **Firmware updates only with the plan:** decide whether **security fixes go to everyone** (open decision in `BUSINESS_STATUS`). Recommendation: yes, security fixes for all units.

---

## 7. Inventory risk (what can be wasted, and how to limit it)

| Item | Risk | Limit it by |
|---|---|---|
| **Clone shells** | Seller changes the mould; batch-to-batch spread; trade-dress letter | Mould number in writing; ±0.2 mm check on every lot; plain version only; lawyer before sale (`KNOCKOFF_SHELL_PLAN.md` §4–§5). Buy shells for the batch + 10 % |
| **LCD panels** (glass) | Breakage, silent controller change, wrong frame | Acceptance test per lot (§2.1); tray + ESD packing; +5 % spares |
| **LiPo cells** | Shelf ageing, swelling, shipping rules | Buy per batch (don't stock 1,000 cells early); store at ~40–60 % charge, cool, away from metal; UN38.3 before paying |
| **ESP32 module** | Stock-outs (1,218 in stock on 2026-10-08) | Check stock before each PCBA order; pre-order if short; no substitutes |
| **Assembled boards** | A design fault found after 100 are built | 5-board prototype first; 2 × 500 at the 1,000 stage |
| **Finished units** | Unsold stock after back-to-school | Build to preorders + a small buffer; the all-or-nothing campaign is the safety net (`MARKETING_GUIDE.md` §10) |
| **Tariffs / duty** | Unit cost jump | Budget the high end; put each real checkout total in the model |

---

## 8. Cost and cash per stage vs the $2,000 and preorders

| Stage | Cash out | Where it comes from | After the stage |
|---|---|---|---|
| A. Samples (incl. v15 order $270 *(est.)*, sample parts $369.60, shell samples $60, bulk samples $90, Cloudflare 3 months $15, free-month AI $25, lawyer $150, company filing $97 (LLC figure; a corporation's fee isn't in the repo), domain $20, 10 % contingency) | **$1,206.26** *(est.)* | $2,000 budget | **≈ $793.74 left** (`UNIT_ECONOMICS.md` §8.5) |
| B. Knockoff-board prototype (5 assembled) | ≈ $270 *(est.; same size as v15; not in the model)* | Budget remainder | ≈ $524 left |
| B. Pogo jig parts | $50–150 *(est.)* | Budget remainder | ≈ $374–474 left |
| C. First 100 units + selling legally | **≈ $7,770** (`UNIT_ECONOMICS.md` §5/§8.5) | Preorders | — |
| C. Preorders needed (own Stripe store, $218.18 net each) | (7,770 − 793.74) ÷ 218.18 ≈ **32** | `UNIT_ECONOMICS.md` §8.5 | — |
| C. Same on **Kickstarter** (≈ 8–10 % fees: `MARKETING_GUIDE.md` §10) | (7,770 − 793.74) ÷ (225 × 0.90 to 0.92) ≈ **34–35 backers** | Computed here | Set the all-or-nothing goal to the real batch cost once quotes are in |
| C. If all 100 sell at $225 | Profit ≈ 100 × $123.83 = **≈ $12,383** (`UNIT_ECONOMICS.md` §8.3, model) | — | Cash toward stage D |
| D. 1,000 units (parts, freight, scrap at $31.69/unit) | **≈ $31,690** *(est.)* + EMS box build ≈ $2.50–3/unit *(est., `LAUNCH_ROADMAP.md` §6)* ≈ $2,500–3,000 | Stage C profit + new preorders | Gap ≈ $22,000 ÷ $218.18 ≈ **~100 prepaid orders** before ordering |

Notes: stage B's two lines push the leftover below what the model shows; if cash is tight, drop the 5 extra pilot units (≈ $506, `BUSINESS_STATUS`) rather than the knockoff-board prototype. CE, ISED and a custom FPC tail are **not** in any of these numbers.

---

## 9. Decision table (open choices, with a recommendation)

| # | Decision | Options | Recommendation | Decide by |
|---|---|---|---|---|
| D1 | Shell for the first 100 | Plain clone / custom 3D-printed shell + clone key mat / Casio | **Plain clone if all 9 go/no-go rules pass and the lawyer agrees; otherwise custom shell.** Never Casio for sale | Jan 2027 |
| D2 | Panel supplier at 100 | Sungood / Goldenmorning / ZJY / BuyDisplay ($5.71 @100) | **Sample Sungood + Goldenmorning (plastic frame) + ZJY; pick on the acceptance test.** BuyDisplay is the safe fallback (+≈ $3.21/unit ≈ $321 at 100) | Feb 2027 |
| D3 | Custom short FPC tail | Now / at 1,000 / never | **Quote only; decide at 1,000** | Jul 2027 |
| D4 | Battery at 100 | #1317 150 mAh / TW302030 140 mAh / big cell (custom shell only) | **TW302030 if its UN38.3 is supplied; else #1317.** Big cell only with the custom shell. Use the measured screen-on hours from the samples | Feb 2027 |
| D5 | PCBA batch size at 100 | 1 × 100 / 2 × 50 | **1 × 100 after a 5-board prototype** | Apr 2027 |
| D6 | PCBA at 1,000 | 1 × 1,000 / 2 × 500 | **2 × 500** | Jul 2027 |
| D7 | Preorder platform | Kickstarter / own site / reserve-pay-at-ship | **Kickstarter all-or-nothing in spring 2027**, then own-site preorders (`MARKETING_GUIDE.md` §10). Needs an **adult (18+) account holder** and the company registered | Feb 2027 |
| D8 | Company form | Corporation / LLC; state; who signs if under 18 | Ask the lawyer; register before any money is taken | Dec 2026 |
| D9 | Certification scope | US / US + Canada / + EU | **US first; Canada if cheap in the same lab visit; EU after 1,000** | Feb 2027 |
| D10 | Fulfilment | Self / ShipMonk / ShipBob / FBA | **Self for 100; 3PL only above ~200 orders/month** | Jul 2027 |
| D11 | Assembly and enrolment at 1,000 | Self / EMS box build (+ how to enrol without sharing the admin token) | **EMS box build; enrol-only credential or a final enrol step at your bench** | Jun 2027 |
| D12 | Warranty and returns | 90 days / 1 year; 14 / 30-day returns | **1-year limited, 30-day returns** (3 % budget), lawyer wording | Mar 2027 |
| D13 | Security fixes without the plan | Yes / no | **Yes** | Mar 2027 |
| D14 | AI model for solves | Sonnet 5.5 / Opus 5.5 / Haiku-first | **Sonnet 5.5** ($0.026/solve; the $5 cap never loses money) (`BUSINESS_STATUS`) | Before preorders |

---

## 10. 12-month plan (Gantt-style), Oct 2026 → Sep 2027 (back-to-school)

`■` = planned work, `◆` = milestone/gate, `·` = optional/conditional. Dates are targets; the first row moves with the unrecorded v15 delivery date.

| Work | Oct | Nov | Dec | Jan | Feb | Mar | Apr | May | Jun | Jul | Aug | Sep |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| v15 boards arrive, S1 bring-up (M2–M6) | ◆■ | ■ | | | | | | | | | | |
| Samples S2–S5 built, enrolled (M7) | | ■ | ◆ | | | | | | | | | |
| Software stage 2 (firmware → Worker), Turnstile, email, deploy | ■ | ■ | | | | | | | | | | |
| Sample use: solves/day, battery data | | ■ | ■ | ■ | | | | | | | | |
| Knockoff shell samples: order, measure, go/no-go | | ■ | ◆ | | | | | | | | | |
| LCD panel samples (Sungood / Goldenmorning / ZJY) + quote | | ■ | ■ | ◆ | | | | | | | | |
| Battery + camera RFQ (UN38.3) | | | ■ | ■ | | | | | | | | |
| Lawyer / IP clinic; company registration; trademark search | | ■ | ■ | ◆ | | | | | | | | |
| Knockoff board layout + 5-board prototype | | | | ■ | ■ | | | | | | | |
| Chinese New Year slowdown (suppliers) | | | | | ■ | | | | | | | |
| Production-intent units (stage B), pogo jig, QA script | | | | | ■ | ■ | | | | | | |
| FCC Part 15B test + SDoC (· ICES-003) | | | | | | ■ | ◆ | | | | | |
| Insurance, Stripe live, terms (refunds/warranty) | | | | | | ■ | ■ | | | | | |
| Pre-launch (4–6 weeks) + Kickstarter campaign | | | | | | ■ | ■◆ | | | | | |
| Order 100: PCBA, shells, panels, cells, packaging | | | | | | | | ■ | | | | |
| Build + QA + enrol 100; ship backers | | | | | | | | ■ | ■ | ◆ | | |
| Own-site preorders / sales; reviews | | | | | | | | | ■ | ■ | ■ | ■ |
| Decide 1,000 (2 × 500), EMS quote, 3PL check | | | | | | | | | | ◆ | | |
| First 500 (only if stage C sells) | | | | | | | | | | · | · | · |
| Back-to-school content peak (Jul–Aug) | | | | | | | | | | ■ | ■ | ■ |

Critical path: **knockoff go/no-go → new board prototype → frozen production-intent unit → FCC test → Kickstarter → order 100.** A month's slip in any of these moves the 100-unit shipment past July.

---

## Web sources (accessed 2026-10-11)

- FCC SDoC: US responsible party, 47 CFR §2.1077 / §15.101 (example SDoC statements): https://ouraring.com/fcc-compliance-statements/oa14 , https://smseagle.eu/docs/certificates/FCC_SDoC_MHD-8100.pdf
- ISED host labelling ("Contains IC"), ICES-003 for the host: https://docs.digi.com/resources/documentation/Digidocs/90001541/reference/r_certs_cell_labelling.htm , https://cdn.lantronix.com/wp-content/uploads/pdf/CERT-00580-REV-A-ISED-XPICO-600.pdf
- CE / RED with a pre-certified module (EN 62368-1, EN 301 489-1/-17): https://www.espressif.com/sites/default/files/ESP32-C6-MINI-1%20CE%20Certification.pdf , https://hardware.iot.telekom.com/Document/7581064112225629843/DEKRA%20IoT%20-%20CE%20Mark%20Modular%20-%20Product%20Information.pdf , https://blog.promise.legal/fcc-certification-ce-marking-connected-devices-hardware-startups/
- ShipBob / ShipMonk minimums and fees (2026 reviews): https://www.fulfill.com/shipbob-vs-shipmonk , https://fitsmallbusiness.com/best-fulfillment-warehouse/
- USPS lithium cells installed in equipment (domestic): https://about.usps.com/postal-bulletin/2015/pb22408/html/updt_008.htm
- JLCPCB standard assembly ≈ 5–7 days (2026 comparison) and holiday notices: https://www.nextpcb.com/blog/jlcpcb-vs-pcbway-vs-nextpcb-comparison-2026 , https://jlcpcb.com/news/jlcpcb-september-october-2026-holiday-schedule
- Repo sources for FCC cost, UN38.3, moulds, FBA: `Claude outputs/LAUNCH_ROADMAP.md` "Sources (checked 2026-10-03)" [S10], [S13], [S17], [S21], [S22], [S26].
