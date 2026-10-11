# AI Calculator launch roadmap

Written Saturday 2026-10-03. **Re-planned 2026-10-08, updated the evening of 10/8** (v15-LCD verified "ORDER", BuyDisplay ER-TFT019-1 panel, v15 order in the budget):
- 5 v14 e-paper testers, then the v15-LCD board, then knockoff shells and bulk.
- Budget and cost numbers come from `hardware/production/COST_MODEL.xlsx` (`UNIT_ECONOMICS.md`) and `AI_Calculator_Shopping_List.xlsx`.
- Prices were checked online on 2026-10-03/06/08 unless marked **(estimate)**. Sources are listed at the end as [S1], [S2] and so on.

Chart data (timeline tasks, price-scaling table, API costs) is in `launch_roadmap_data.json` in this folder. The page version is `launch_roadmap.html`.

> **Status 2026-10-08:**
> - **The v14 board is ordered at JLCPCB** (git tag `v14-order`, files in `hardware/kicad/` and `hardware/fab/`, frozen). It's 5 PCBs, assembly qty 5 assumed (Nirav was choosing 2 vs 5 at checkout).
> - **Delivered cost:** about $200–240 for 2 assembled, or about $50–60 more for 5 assembled. Both include DHL and the ~35 % US duty collected at checkout.
> - **Boards arrive about 10/18–22.**
> - **Next board: v15-LCD, verified "ORDER"** (`hardware/verification/14_v15_final_preorder.md`, 2026-10-08 evening). Not ordered yet; no panel test is needed first. Order it with `hardware/ORDER_WALKTHROUGH_v15_lcd.md` (files `hardware/fab_v15_lcd/`, 21:38) and buy 3 × **BuyDisplay ER-TFT019-1** (no touch, ~$6–7 each).
>
> Things decided earlier that still hold:
> - **Battery:** Adafruit **#1317** (150 mAh).
> - **API proxy:** exists (`server/proxy/`). It gives each device its own token, a 30-day free trial and a 500 solves/month cap.
> - **Factory self-test:** exists (SHIFT + ALPHA, ON).
> - **Camera hole:** 7 mm.
> - **Price:** **$225 + $15/month, first month free**.

---

## 0. The short version

| Question | Answer |
|---|---|
| What's ordered | **v14 at JLCPCB, 10/8**: 5 PCBs, 5 assembled (assumed), ≈ **$275 delivered** (estimate: $220 for 2 assembled + $55 for 3 more). |
| What you build next | **5 testers** in genuine Casio fx-115ES shells (you own 2, buy 3 at ≈ $20). **Option C** (recommended): units 1–3 with the current Seeed / Waveshare / Adafruit parts, units 4–5 with generic parts. |
| What it costs | **≈ $664 for the 5 testers with option C**, incl. the JLCPCB order (option A ≈ $662, option B ≈ $572). Plus the **v15-LCD order ≈ $270 delivered (estimate)** and **3 × ER-TFT019-1 ≈ $32** (estimate incl. shipping). That's **≈ $966 spent now**, leaving ≈ $1,034 of the $2,000. |
| When you'll have working units | Boards ≈ **10/18–22** → board #1 working ≈ 2 days later → all 5 testers ≈ **10/25–29** (estimate). |
| The next board | **v15-LCD**: a 1.9" 170×320 colour IPS LCD replaces the e-paper (`hardware/stage15_lcd.md`, reviews 13 and 14). **Verified "ORDER"** on 10/8 (R23 15 Ω, SHIFT/ALPHA/ON pads moved to the Casio positions). Order whenever Nirav is ready; the bench checks (pin-1 diode test, backlight current, 3.3 V, colours, tail length, the 3 moved keys) happen after arrival and can't block the order. |
| Cost per unit (`COST_MODEL.xlsx`) | E-paper **$92 at 10 · $46 at 100 · $34 at 1,000**. LCD **$92 · $42 · $32** (BuyDisplay panel at 10, Alibaba from 100). LCD + 1,200–1,500 mAh battery **$97 · $45 · $34**. At 100 units, add ≈ $35 of one-time selling-legally costs per unit. |
| Does the $2,000 cover it? | The whole plan (testers, **v15 order + 3 panels**, knockoff and bulk samples, **10-unit v15-LCD pilot** re-using the 5 v15 boards, server, API, lawyer, LLC, 10 % contingency) comes to **≈ $2,475, about $475 over**. A **4-unit pilot** fits with ≈ $62 left; 5 units is ≈ $11 over, or ≈ $30 left with Sonnet 5.5 for the beta (`UNIT_ECONOMICS.md` §5). |
| Claude API | Opus 5.5 ≈ **$7.52 per typical user a month**. It **loses money at the 500-solve cap** ($24.72). Sonnet 5.5 is ≈ $3.77 typical and $12.40 at the cap, so **it covers the cap** (`UNIT_ECONOMICS.md` §4). |
| Price (decided 10/4) | **$225** per calculator + **$15/month**, **first month free**, 500 solves/month fair-use cap. Without a subscription it must still work as a normal scientific calculator. |
| Biggest legal facts | 1. A camera + Wi-Fi calculator is **banned on the SAT and ACT** [S16], so market it as a homework and study tool. 2. **Don't sell modified Casio shells at scale** (trademark risk): testers only, then knockoff and own shells. 3. Before selling to the public you need an **FCC Part 15B SDoC** (about $1–2.5k [S10]), and the battery needs a **UN38.3 test summary**. |

---

## 1. What to buy now (`AI_Calculator_Shopping_List.xlsx`)

### 1A. The JLCPCB order (done 10/8)

| Item | Qty | Total | Notes |
|---|---|---|---|
| v14 PCB 2-layer, 0.8 mm, ENIG, 5 pcs, Standard PCBA | 5 PCBs, **5 assembled** (editable input; 2 if that was chosen) | ≈ $220 (2 assembled) + ≈ $55 (3 more) = **≈ $275 delivered** | Includes DHL and the ~35 % US duty collected at checkout (DDP; de-minimis suspended since 2026-06-24) [S8]. Replace with the real total. |

### 1B. Tester parts: options A / B / C (5 testers, 1 spare of each kind)

| Part | Current (known-good) | Generic (cheaper, must be checked) |
|---|---|---|
| Camera | **Seeed OV5640 AF 114993115**, $12.99 (J1 was designed for its ribbon) | OV5640 AF 24-pin DVP, ≈ $7 **(estimate)**: beep test E6 first (fingers 2 and 15 to GND) |
| Screen | **Waveshare 2.13" V4 raw panel**, $6.99 [S4] | **GDEY0213B74**, ≈ $4.50 **(estimate)**: same SSD1680 family and pin order [S20] |
| Battery | **Adafruit #1317** 150 mAh, $5.95 [S1] | Generic **302030** 150 mAh JST-PH with PCM, ≈ $3 **(estimate)**: meter the polarity, leads are often reversed |
| Magnet piece (all options) | Adafruit **#5358**, $6.50 × 6 [S2] | – |
| Charge cable (all options) | Adafruit **#5412**, $4.95 × 5 [S3]: low stock, order first | – |
| Shells | 3 × Casio fx-115ES ≈ $20 (same edition as the measured one) [S6] | – |

| Option | What | Total for 5 testers incl. JLCPCB |
|---|---|---|
| A | All current parts | **≈ $662** |
| B | All generic screens / cameras / batteries | **≈ $572** |
| **C (recommended)** | **Units 1–3 current, 4–5 generic** (spare of each kind) | **≈ $664** |

Each total includes shipping (Adafruit $15, Seeed $12, Waveshare $20 and AliExpress $10, depending on the option) and **≈ $61 of supplies and tools**: Kapton, thin tape, epoxy, drill bits, vinyl, IPA, glasses. C costs about as much as A because it pays two sets of shipping and two kinds of spare. It's still the right choice: the first 3 testers isolate board problems, and the last 2 test the parts bulk production needs.

### 1C. v15-LCD order and panels (verified "ORDER", 2026-10-08)

| Item | Qty | Total | Why |
|---|---|---|---|
| **v15-LCD JLCPCB order**: 5 PCBs, 5 assembled, delivered | 1 | **≈ $270** (estimate, $255–285) | Verified "ORDER" in `hardware/verification/14_v15_final_preorder.md`. Same settings as v14; step by step in `hardware/ORDER_WALKTHROUGH_v15_lcd.md`. One fewer extended part type than v14 (13), but R23 C22810 is new. J5, Q4, Q5, R21–R23 (15 Ω), C36 are placed by JLCPCB. |
| **BuyDisplay ER-TFT019-1**, no touch | 3 | **≈ $32** (≈ $6.75 each + ≈ $12 shipping, estimates) | The panel the board was verified against, pin for pin from its datasheet ($6.22 @10, $5.71 @100). No ER-CON30HT-1 socket needed. Adafruit #5394 ($17.50) is only an optional fallback. |

### 1D. Don't buy yet
- Knockoff shells in bulk, custom batteries, OEM magnet connectors: samples only (§2).
- A soldering station: the boards need no soldering from you.

---

## 2. Timeline

### 2A. From the order to v15 (estimate)

| Date | Task | Who | Waits on |
|---|---|---|---|
| **Thu 10/8** | **v14 ordered at JLCPCB** (5 PCBs, PCBA qty 5). Answer JLCPCB's engineering emails within hours. Order the tester parts (option C); Adafruit first, because #5412 stock is low. **v15-LCD verified "ORDER"** the same evening. | Nirav | – |
| **from 10/9, Nirav's call** | **v15-LCD order** at JLCPCB (`hardware/ORDER_WALKTHROUGH_v15_lcd.md`) and 3 × BuyDisplay ER-TFT019-1. Boards ≈ 10–14 days after the order. | Nirav | – |
| 10/9–10/15 | JLCPCB builds (≈ 5–7 days). Meanwhile: buy the 3 Casio shells, prep and grind the 5 shells, deploy the proxy on Render. | Nirav + Claude | – |
| ≈ 10/12–10/16 | Adafruit, Seeed, Waveshare and AliExpress parts arrive. Meter the battery polarity; beep-test the generic cameras (E6). | outside | vendors |
| **≈ 10/18–10/22** | **Boards arrive.** Board #1 on USB only (`hardware/ARRIVAL_CHECKLIST.md`, First Power-Up Guide), then the e-paper, camera and battery; run the self-test. | Nirav | DHL |
| ≈ 10/20–10/24 | **Board #1 end-to-end:** photo → Claude → answer on the e-paper, in shell #1. | Nirav | bring-up |
| **≈ 10/25–10/29** | **All 5 testers built and tested.** Units 1–3 get the current parts, 4–5 the generic ones. 24-hour battery soak on each. | Nirav | boards + parts |
| ≈ 10/26–11/9 | **Tester feedback**, about 2 weeks with 5 users. Collect the proxy usage log (solves a day, tokens), battery life and bugs, and write the v15 fix list. | testers + Nirav | testers |
| ≈ 10–14 days after the v15 order | **v15 boards + ER-TFT019-1 panels arrive: bench checks** (`hardware/ARRIVAL_CHECKLIST.md` §4): diode test for the pin-1 end, backlight V(R23)/15 (expect 16–37 mA), panel at 3.3 V, `LCD_INVERT`/RGB flags, tail 36.6 ± 0.3 mm vs rib B, feel of SHIFT/ALPHA/ON. Bring-up `hardware/bringup_guide_v15_lcd.html`, assembly `hardware/enclosure/final_assembly_v15_lcd/assembly_guide_v15_lcd.html`. | Nirav + Claude | JLCPCB, BuyDisplay |
| in parallel, late Oct | **Knockoff shell samples** (4 clones from 2 sellers, `hardware/production/KNOCKOFF_SHELL_PLAN.md` §1). Ask for quotes on the bare LCD panel (ZJY, Goldenmorning samples; `hardware/production/lcd_suppliers.json`). | Nirav | – |
| ≈ mid-Nov | **Pilot board order** (only the boards the pilot still needs), with the tester fixes and the v15 bench-check answers. If the knockoff outline differs, that becomes a v16 board: regenerate, review, dry fit. | Nirav + Claude | bench checks + feedback |
| Dec 2026 – Jan 2027 | **v15-LCD pilot** in knockoff shells: 4–10 units depending on the budget (the 5 v15 boards count). Bulk samples (bare LCD panels, 1,200–1,500 mAh cell, OEM camera, magnet pair). | Nirav | knockoff go/no-go |
| 2027 | **Knockoff / bulk**: pre-orders → 100 units (FCC SDoC, LLC, lawyer first) → own 3D-printed drop-in shell with the 1,200–1,500 mAh battery. | Nirav | pre-orders |

**Critical path:** v14 order (10/8) → boards ≈ 10/18–22 → 5 testers built and tested (≈ 10/29) → feedback (≈ 2 weeks) → pilot → knockoff/bulk. The **v15-LCD order** (verified, any day from 10/9) and its bench checks run in parallel with the testers. Only the JLCPCB and shipping legs are outside your control.

### 2B. Phases

| Phase | When (estimate) | Units | What happens | What Nirav does |
|---|---|---|---|---|
| **Testers (v14 e-paper)** | Oct – early Nov 2026 | 5 | Genuine Casio shells; option C parts; real usage data | Build, test, collect feedback |
| **v15-LCD** | Oct – Nov 2026 | 5 boards (verified ORDER 10/8) | Colour 1.9" LCD (BuyDisplay ER-TFT019-1); bench checks on arrival | Order it, run the bench checks |
| **Pilot (knockoff shells)** | Dec 2026 – Jan 2027 | 4–10 | v15-LCD in 991ES-style clones; testers aged 13+; FCC pre-scan | Support, fixes |
| **First sales** | Mar – Jun 2027 | ~100 | LLC, FCC SDoC, pre-orders (about 36 at $225 fund the batch) | Run the business |
| **Own shell + big battery** | 2027 | 100+ | 3D-printed drop-in shell; 1,200–1,500 mAh cell (13–17 h of screen-on time with the LCD) | CAD + fit |
| **Hundreds** | H2 2027 | 300–1,000 | Custom battery + OEM magnet connector (§6); EMS does PCBA **and** box build; a 3PL ships | Suppliers, software |
| **Thousands + injection molding** | 2028 | 3,000+ | Molds pay off above about 3,300 units (§3.2) | Approve samples, QA |
| **Contract manufacturer** | 2028–29 | 5k–10k+ | The CM buys, builds, tests, packs and ships | Product, firmware, marketing |

### 2C. The $2,000 (`hardware/production/COST_MODEL.xlsx` → Cash budget)

| # | What | When | $ |
|---|---|---|---|
| 1 | v14 JLCPCB order (5 PCBs, 5 assembled, delivered) | paid 10/8 | 275 |
| 2 | Tester parts, 3 shells, shipping, supplies (option C) | now | 389 |
| 3 | v15-LCD panels: 3 × BuyDisplay ER-TFT019-1 + shipping (estimate) | now | 32 |
| 4 | **v15-LCD JLCPCB order**: 5 PCBs, 5 assembled, delivered (estimate) | now | 270 |
| 5 | Knockoff shell samples (4 from 2 sellers) | late Oct | 60 |
| 6 | Bulk samples (camera, bare LCD panels, small + big cells, magnet pair) | after the shells pass | 90 |
| 7 | v15-LCD pilot, 10 units (all parts, freight, scrap; re-uses the 5 v15 boards, so 5 more boards) | Dec – Jan | 679 |
| 8 | Proxy server, 3 months | beta | 75 |
| 9 | Claude API free month: 5 testers + 10 pilot users (Opus 5.5) | beta | 113 |
| 10 | Lawyer / IP clinic (trade dress) | before units leave | 150 |
| 11 | Indiana LLC | before taking money | 97 |
| 12 | Domain + email | beta | 20 |
| | 10 % contingency | | 225 |
| | **Total** | | **≈ 2,475** |
| | **Left from $2,000** | | **≈ −475** |

**To fit inside $2,000:** make the pilot 4 units, all from the v15 order (≈ $62 left), or 5 units with Sonnet 5.5 for the beta (≈ $30 left). The other levers are Sonnet 5.5 for the beta (saves ≈ $62) and option B for the testers (saves ≈ $101). The first 100 public units (LCD) plus the selling-legally bill come to **≈ $7,770**, so take **≈ 36 pre-orders** first.

---

## 3. Per-unit cost (USD per finished calculator)

The board-only ("PCB + PCBA") price breaks come from LCSC, checked 2026-10-03:
- **ESP32-S3-MINI-1-N4R2:** $5.00 (1+), $4.50 (10+), $3.63 (100+), $3.46 (650+), $3.38 (1,300+) [S11]
- **TCA8418:** $1.12, $0.92, $0.69, $0.63 (500+), $0.61 (1,000+) [S12]

Everything else in this table is **an estimate** built from those breaks plus the sourced unit prices in §1.

| Item | 2 | 10 | 50 | 100 | 500 | 1,000 | 5,000 | 10,000 |
|---|---|---|---|---|---|---|---|---|
| PCB + PCBA | $39.52 | $18.00 | $13.00 | $9.00 | $7.90 | $7.30 | $6.60 | $6.20 |
| Camera OV5640 AF | $14.00 | $12.00 | $7.00 | $6.00 | $5.00 | $4.50 | $4.00 | $3.50 |
| E-paper 2.13" | $6.99 | $6.60 | $6.31 | $5.50 | $4.50 | $4.00 | $3.50 | $3.20 |
| Battery 150 mAh (#1317; same price tiers as the #1570) | $5.95 | $5.36 | $5.36 | $4.76 | $2.00 | $1.50 | $1.20 | $1.00 |
| Magnetic connector pair | $6.50 | $5.85 | $5.85 | $5.20 | $2.00 | $1.20 | $0.90 | $0.70 |
| Magnetic USB cable (in the box) | $4.95 | $4.95 | $4.95 | $3.96 | $1.80 | $1.40 | $1.10 | $1.00 |
| Enclosure + keypad | $20.00 | $20.00 | $20.00 | $21.00 | $10.00 | $9.00 | $6.50 | $4.00 |
| Final assembly labour | $0 (you) | $0 | $0 | $0 | $3.00 | $2.50 | $2.00 | $1.50 |
| Packaging + QC | $0 | $1.00 | $2.00 | $2.00 | $1.50 | $1.20 | $0.90 | $0.80 |
| Freight + import duty | $40.00 | $8.00 | $5.00 | $8.00 | $6.00 | $5.00 | $4.20 | $3.80 |
| One-time certification, amortized | $0 | $0 | $0 | $35.00 | $7.00 | $3.50 | $0.70 | $0.35 |
| **Total per unit** | **$137.91** | **$81.76** | **$69.47** | **$100.42** | **$50.70** | **$41.10** | **$31.60** | **$26.05** |
| Total without certification | $137.91 | $81.76 | $69.47 | $65.42 | $43.70 | $37.60 | $30.90 | $25.70 |

How to read it:
- The jump at 100 is the roughly **$3.5k one-time cost** to sell legally: FCC SDoC test, lawyer, LLC. It isn't a parts cost.
- Freight and duty at volume assume about **35 % US duty on China-made content** (the low end of JLCPCB's 35–92.5 % range [S8]). Tariff policy is the single biggest swing factor. A US-assembled box build doesn't avoid duty on the Chinese PCBA.
- **Retail-price rule of thumb:** sell at about **3–4× unit cost** to survive shipping, returns, platform fees and API costs. At about $40 unit cost (1,000 units), that's $119–149 retail **(estimate)**. A plain fx-115ES costs $20 [S6], so the AI features must clearly earn the difference.

### 3.0 Updated per-unit cost by version (2026-10-08, `hardware/production/COST_MODEL.xlsx` → Versions)

The table above is the original 2026-10-03 estimate, kept for the long view (500–10,000 units). The current model uses a knockoff shell instead of a printed one; its figures are:

| Version (before selling-legally costs) | 10 | 100 | 1,000 |
|---|---|---|---|
| E-paper, 150 mAh | $91.56 | $45.66 | $33.68 |
| **LCD (v15-LCD), 150 mAh** | **$91.92** | **$42.22** | **$31.69** |
| E-paper, 1,200–1,500 mAh | $96.51 | $48.71 | $35.83 |
| **LCD, 1,200–1,500 mAh** (long-term target) | **$96.87** | **$45.27** | **$33.84** |

Where the LCD numbers come from:
- **LCD panel prices:** BuyDisplay ER-TFT019-1 (datasheet-verified) $6.22 + ≈ $12 shipping per order at 10. At 100 / 1,000, Alibaba ZJY at $2.50 / $2.30 or Goldenmorning (`hardware/production/lcd_suppliers.json`; search-snippet prices, so confirm by quote and a sample check).
- **v15 board:** 17 e-paper parts removed, J5 / Q4 / Q5 / R21–R23 (R23 15 Ω C22810) / C36 added, ≈ $0.19 cheaper and 1 fewer extended part type (13 instead of 14).
- **Bigger battery:** Adafruit #258 1,200 mAh $9.95 at 10, Alibaba 504060 ~1,500 mAh ≈ $4 / $3 at volume. It fits only where the shell has a big pocket.

Per-unit profit at $225 (incl. selling-legally at 100+): LCD **$106 / $120 / $163** at 10 / 100 / 1,000. The LCD draws ≈ 80–90 mA with the screen on, so it gets ~1.5 h on 150 mAh. That's why the big cell is the long-term pairing.

### 3.1 Which parts SHIFT supplier at scale

| Part | Now (retail) | Switch at | Switch to | Why / catch |
|---|---|---|---|---|
| Battery | Adafruit #1317 150 mAh $5.95 (was #1570) | **~500 units** | A Shenzhen custom LiPo with protection circuit, JST-PH and **UN38.3 + IEC 62133 reports from the supplier**. Alibaba LiPo cells cost $0.61–1.70; MOQ from 3 pcs to 1,000 [S14] | Ask for the UN38.3 test summary **before** paying. Paying for your own UN38.3 costs about $1,500 per model [S13]; IEC 62133 about $6,350 [S17]. |
| Magnetic connector | Adafruit #5358 $6.50 → $5.20 @100 [S2] | **~500** | Shenzhen/Dongguan magnetic pogo pair. Pins cost about $0.23–0.25 at MOQ 1,000 [S18] | **Needs a new footprint** (today J3 is a right-angle SMD female header, C46061768, that takes the #5358's straightened legs). Make the change in a v3 board. |
| Charge cable | Adafruit #5412 $4.95 [S3] | ~500 | OEM magnetic USB-C cable, about $1–1.8 **(estimate)** | Order it together with the connector so they mate |
| Camera | Seeed ~$14 | **~50** | Generic OV5640 AF 24-pin DVP modules, listed at $4.58–$8 [S19] | **Check the FPC pinout and cable length on every new vendor.** J1 is reversed for one specific cable direction. |
| E-paper | Waveshare $6.31–6.99 [S4] | ~100 | Good Display GDEY0213B74 direct (the same panel family; about €6.89 retail [S20]); volume quote by RFQ | Needs a quote by email; MOQ applies |
| **LCD (v15)** | BuyDisplay ER-TFT019-1 (no touch) ~$6–7, $6.22 @10 (datasheet-verified; Adafruit #5394 only a fallback) | **100** | Alibaba ZJY $2.50 @100 / $2.30 @1,000 or Goldenmorning T190X7-C30-01 (plastic frame), after a sample check | Diode-test the pin-1 end on every new vendor (J5 is numbered for one tail direction); avoid DJ9853 clone controllers; ask for an 18–20 mm FPC tail |
| ESP32-S3 | LCSC module $5.00 → $3.38 [S11] | **Keep the module** | Same module, bigger reel | The module carries Espressif's **FCC modular approval**, so you only test as an unintentional radiator (about $1–2.5k) instead of paying for a full intentional-radiator grant (≈ $6.5–10k+ [S10]). Chip-down saves maybe $1–1.50 a unit **(estimate)**, so it only pays off well above 10k units. |
| TCA8418, LDOs, passives | LCSC via JLCPCB | – | Stay on LCSC/JLCPCB | Already cheap; 14 "extended" part types cost $3 each per order **(JLCPCB rule, estimate)**, which disappears into the per-unit cost at 100+ |
| Shell | Donor Casio $20 | **Before any sales** | 3D print → urethane cast → injection mold (§3.2) | Legal reason (§4) as well as cost |

### 3.2 Enclosure options compared (estimate)

| Option | 2 | 10 | 50 | 100 | 500 | 1,000 | 5,000 | 10,000 |
|---|---|---|---|---|---|---|---|---|
| Casio fx-115ES donor | $20 | $20 | $20 | $20 | $20 | $20 | $20 | $20 |
| 3D print + silicone keypad (incl. small keypad tooling ~$500–800) | – | $30 | $27 | $21 | $12 | $10 | $9 | $9 |
| Injection mold (~$25k for 4 tools: front, back, battery door, keypad) | – | – | – | $251 | $51.50 | $26.50 | $6.50 | $4.00 |

Simple molds run about **$3–6k each** in China; one mold adds $5 a part over 1,000 parts but $0.50 over 10,000 [S21]. A calculator needs about 4 tools, so about $15–30k in total **(estimate)**.

**Break-even for injection molding ≈ 3,300 units**: $25,000 ÷ ($9 printed − $1.50 molded). Below that, keep printing or urethane-casting. Above it, mold.

### 3.3 Claude API cost per solve

Model prices (per million tokens, input / output): **Haiku 4.5 $1 / $5, Sonnet 5.5 $2 / $10, Opus 5.5 $4 / $20** (Anthropic price table, cached 2026-09-25).

**Model choice (2026-10-08, `hardware/production/UNIT_ECONOMICS.md` §4):**
- **Opus 5.5:** ≈ $7.52 per typical user a month (with the system prompt cached). It **loses money at the 500-solve cap** ($24.72 a month), so with Opus, lower `MONTHLY_CAP` to ≈ 280.
- **Sonnet 5.5:** ≈ $3.77 typical and $12.40 at the cap, so it covers the cap. It is the proxy's default.

The assumed tokens per solve are an **estimate**:
- **Photo:** about (width × height) ÷ 750 tokens, so a 1280×960 crop is about 1,600.
- **Input total:** with the system prompt, about 2,500 tokens.
- **Output:** about 2,100 tokens including thinking on Sonnet or Opus, about 800 on Haiku.

| Model | Cost per solve | 60 solves/mo (light) | 150/mo (typical) | 400/mo (heavy) |
|---|---|---|---|---|
| Haiku 4.5 | ~$0.0065 | $0.39 | $0.98 | $2.60 |
| **Sonnet 5.5** | **~$0.026** | **$1.56** | **$3.90** | **$10.40** |
| Opus 5.5 | ~$0.052 | $3.12 | $7.80 | $20.80 |
| Haiku first, Sonnet on 20 % of photos | ~$0.010 | $0.62 | $1.56 | $4.16 |

**How to fund it.** A one-time price can't pay for unlimited AI forever.

*Earlier analysis, written before the $225 + $15/month decision (§0). Kept for the numbers; the counter-argument for a cheaper subscription is in `COMPETITOR_ANALYSIS.md` B1.* Recommended plan **(estimate)**:
1. **Include about 3 months** of service in the hardware price. That's about $5 of API cost at the typical rate with Haiku-first routing.
2. Then charge **$3–5 a month or $29–39 a year**, with a fair-use cap (for example 300 solves a month).
3. Optional "bring your own API key" mode for power users.

Must-haves:
- **The device must never contain your API key.** **Done in the stage-13 firmware:** `firmware-prototype/src/claude_client.cpp` talks only to `server/proxy/` with a per-device token (monthly cap, trial, admin tool); the firmware erases any old key from flash. Still to do: deploy the proxy (Render or similar), and a real sign-in on the link page before selling.
- Your local `api_key.txt` is correctly git-ignored. Keep it that way.

---

## 4. Regulatory and business must-haves

| Item | Needed when | Realistic cost | Notes |
|---|---|---|---|
| **FCC Part 15B SDoC** (unintentional radiator) | Before **selling** any unit | **$1,000–2,500**, about 1 lab day [S10] | The ESP32-S3-MINI-1 has its own FCC modular grant from Espressif (grantee code 2AC7Z [S22]; look up the exact FCC ID on fccid.io). Label the product **"Contains FCC ID: 2AC7Z-…"**. Prototypes you keep yourself don't need it. |
| Intentional-radiator testing | Only if you go **chip-down** (no module) | $6,500–10,000+ [S10] | Avoid: keep the module. |
| **UN38.3** battery test summary | Before shipping any unit with a battery (by mail, Amazon, freight) | $0 if the supplier provides it; about $1,500 per model if you pay [S13] | Small cells inside equipment ship as UN3481 "packed in equipment". Get the supplier's test summary and SDS. Amazon asks for both. |
| IEC 62133 (battery safety) | Amazon / big retailers will ask | $0 from the supplier; about $6,350 if you pay [S17] | Pick a battery vendor that already has a CB report. |
| **CPSIA** (children's product) | **Only if you market to kids 12 and under** | Several hundred dollars per test + Children's Product Certificate [S23] | **Market to ages 13+** (high school/college). Then it's a general-use product (General Certificate of Conformity, no third-party test needed). |
| **Exam rules** | Marketing claims | $0 | SAT and ACT ban calculators with **built-in cameras, Wi-Fi/Bluetooth, or CAS** [S16]; AP exams follow College Board rules too. **Never call it exam-approved.** Pitch it as an "AI homework tutor that looks like a calculator", with a **tutor mode** that shows steps to answer teacher and parent pushback. |
| **Casio trademark / trade dress** | Before selling **any** modified Casio | Lawyer consult $0–300 (law-school clinic / SCORE) **(estimate)** | Reselling a **materially altered** branded product under its brand can count as trademark infringement. Casio's logo and the look of the case are theirs. OK: your own prototypes, and maybe free beta units clearly labeled "not affiliated with Casio, logo removed". **Not OK at scale.** The custom shell (§2B) removes the problem. |
| Your own brand | Before the public launch | USPTO $350 per class [S24] | Search the name first. Class 9 (electronics) + class 42 (software service) = $700. |
| **Business entity** | Before taking money from strangers | Indiana LLC **$97 online** [S25] + free EIN | If you're **under 18**, a parent usually has to sign. Stripe, Amazon Seller, Kickstarter and Crowd Supply all need an adult account holder. Register for Indiana sales tax (INBiz). |
| Product liability insurance | First sales | ~$400–1,000 a year **(estimate)** | Lithium battery + students: get it. |

### Sales channels, in order

| Stage | Channel | Fees |
|---|---|---|
| 10–50 | Friends, school, a Discord/beta list | $0 |
| 100 | Preorders on Crowd Supply or Kickstarter. Preorders **fund the next batch** before you pay for it. | ~5–10 % platform + payment fees **(estimate)** |
| 100–1,000 | Own Shopify store + Tindie | ~$39 a month + ~3 % **(estimate)** |
| 1,000+ | Amazon FBA | Fulfillment about **$3.15 per unit** (small standard, 4–8 oz) + 3.5 % fuel surcharge [S26] + referral fee ~8–15 % **(estimate)** |

---

## 5. Path to "it makes itself": autonomous manufacturing

| Stage | Units | Who builds | Who tests | Who ships | What Nirav does |
|---|---|---|---|---|---|
| 1. Now | 2–10 | JLCPCB PCBA; Nirav assembles | Nirav by hand | Nirav | Everything |
| 2. v2 test pads | 10–100 | Same | **Pogo-pin jig** (3D-printed bed + spring pins, about $50–150 in parts **(estimate)**) + a PC script: flash → factory self-test → write the serial number → print a PASS/FAIL label | Nirav | Presses one button per board; about 2 min a unit **(estimate)** |
| 3. Turnkey PCBA + box build | 300–1,000 | JLCPCB, or a small EMS that does final assembly ("box build") with your jig and test firmware | The EMS runs **your** jig; you get a CSV of results | **3PL** (ShipBob, ShipMonk and similar) or Amazon FBA | Supplier email, QA spot checks, firmware |
| 4. Contract manufacturer | 3,000+ | A CM in Shenzhen/Dongguan: parts buying, molding, PCBA, assembly, test, packaging | CM's line, with your golden-sample test spec | Sea freight → FBA/3PL | Product roadmap, OTA updates, marketing, support triage |

Building blocks that make this possible. Build them in this order:
1. **Factory test firmware**: **done** (stage 13 self-test: every key, display pattern, camera, Wi-Fi RSSI, battery and charge STAT, one JSON line over USB).
2. **Test pads on board v2** (3.3 V, GND, EN, IO0, D+/D−, VBAT). Add these when the KiCad session does v2. Don't touch the v1 files now.
3. **Automated flashing**: `esptool` + a Python script. Each board gets a unique device ID and a proxy token at flash time.
4. **OTA updates**: ESP-IDF OTA with two app slots and signed images. Ship bug fixes without returns.
5. **The proxy server** doubles as the fleet backend: subscription check, usage caps, crash logs.
6. **Support**: an FAQ site, a help email, and an AI-assisted reply draft. At 1k+ units, budget about 2–5 % of units needing help or replacement **(estimate)**.

---

## 6. Consumer → bulk pricing steps (concrete thresholds)

| Threshold | Switch | Example price change | Why now |
|---|---|---|---|
| **10 boards** | JLCPCB extended-part fees ($3 × 14 types) are paid once per order, not per board | PCBA $39.52 → ~$18 a board **(estimate)** | Fixed fees get shared |
| **10 / 100** | Adafruit quantity tiers | #5358: $6.50 → $5.85 (10+) → $5.20 (100+) [S2] | Built-in discount |
| **50** | Camera: Seeed → generic OV5640 AF module | ~$14 → $4.58–8 [S19] | Big saving; test one first |
| **100** | ESP32 module reel price | $5.00 → $3.63 [S11] | LCSC break |
| **100** | E-paper: Waveshare retail → Good Display direct | $6.99 → ~$5.50 **(estimate)** | RFQ is worth it now |
| **~500** | Battery: Adafruit → custom LiPo with UN38.3 | $4.76 → ~$2.00 **(estimate)**; Alibaba cells $0.61–1.70 [S14] | Typical MOQ 500–1,000 |
| **~500** | Magnetic connector → OEM pogo pair (+ new footprint) | $5.20 → ~$2 **(estimate)**; pins $0.23–0.25 @ MOQ 1,000 [S18] | MOQ reachable |
| **650 / 1,300** | ESP32 full reel | $3.46 → $3.38 [S11] | LCSC reel break |
| **~500–1,000** | Hand assembly → EMS box build | Your time → ~$2.50–3 a unit **(estimate)** | Your hours are worth more on software and sales |
| **~3,300** | 3D print / urethane cast → injection mold | $9 → $1.50 per set + $25k tooling **(estimate)** | Break-even (§3.2) |
| **5,000+** | Parts bought by the CM through its distributors | Another ~10–20 % off BOM **(estimate)** | Contract manufacturer buying power |

**Money flow tip:** take preorders before each step up, so customers' deposits pay for the MOQ and the tooling, not your savings.

---

## 7. Top risks and what to do about them

| # | Risk | Effect | Mitigation |
|---|---|---|---|
| 1 | **The donor calculators are a different edition**, or a generic part (camera, screen, battery) misbehaves | A tester that doesn't fit or work | Buy the **exact** measured edition; option C keeps the generic parts to units 4–5; beep-test cameras, meter batteries |
| 2 | **v15-LCD panel surprises**: the pin-1 end of the tail, the tail length vs rib B, backlight brightness, the panel at 3.3 V (`hardware/verification/14_v15_final_preorder.md` §6) | Dark or dim display, at worst a part change on the next spin | Two independent drawings already put pin 1 at the J5 silk tick; R23 = 15 Ω gives 16–37 mA. Bench checks on arrival; a half twist of the tail fixes a mirrored end without a board change |
| 2b | **LCD battery life**: ~1.5 h of screen-on time on 150 mAh | Testers and users charge every 1–2 days | Backlight time-out in the firmware; the 1,200–1,500 mAh cell in the custom/knockoff shell (+ ≈ $3 a unit at 100) |
| 2c | **Budget**: with the v15 order the full plan is ≈ $475 over $2,000 | Can't fund a 10-unit pilot | 4–5-unit pilot from the v15 boards, Sonnet 5.5 for the beta, pre-orders before the 100 batch |
| 3 | **Camera ribbon orientation (J1 reversed)** is about 90 % certain (verification 01/03) | No image | Buy the same Seeed module; run the E6 beep test; a 180° cable twist fixes it |
| 4 | **Tariff/DDP cost** higher than expected | +$30–70 per JLCPCB order; +35 %+ at volume [S8] | Put the real 10/8 checkout total into the cost model; budget the high end |
| 5 | **API key in firmware / unlimited API use** | Stolen key, surprise bill | Proxy + per-device token + caps before any unit leaves your hands |
| 6 | Supply of #5412 cable (35 in stock on 10/6 [S3]) | No way to charge | Order the 5 first; Jameco/DigiKey as backup |
| 7 | Legal: selling Casio-branded altered units; marketing to under-13s | Takedown, liability | Custom shell before sales; 13+ only; never "exam-approved" |

---

## Sources (checked 2026-10-03)

- [S1] Adafruit #1317 150 mAh battery $5.95, in stock (https://www.adafruit.com/product/1317, checked 2026-10-06; replaced the #1570 100 mAh, whose Jameco price breaks were $7.85 / $7.45 (10+) / $6.55 (50+))
- [S2] Adafruit #5358 $6.50 / $5.85 (10–99) / $5.20 (100+): https://www.adafruit.com/product/5358
- [S3] Adafruit #5412 magnetic USB cable $4.95, 36 in stock: https://www.adafruit.com/product/5412
- [S4] Waveshare 2.13" raw e-paper $6.31–6.99, SKU 12672: https://www.waveshare.com/epaper?p=4 , https://waveshare.com/2.13inch-e-Paper.htm
- [S5] Seeed OV5640 AF for XIAO ESP32S3 Sense: £11 (The Pi Hut) https://thepihut.com/products/seeed-ov5640-camera-for-xiao-esp32s3-sense ; $45.99 at a Newegg reseller https://www.newegg.com/Seeed-Studio-Camera-Accessories/BrandSubCat/ID-90080-397
- [S6] Casio fx-115ES PLUS at Walmart $19.99–20.34: https://www.walmart.com/ip/418401755
- [S7] JLCPCB Standard PCBA build time ≥4 days: https://jlcpcb.com/solutions/pcb-prototype-assembly , https://jlcpcb.com/help/article/pcb-fabrication-services-and-production-time
- [S8] JLCPCB US tariff FAQ (DDP for individuals; 35–92.5 % total): https://jlcpcb.com/help/article/us-tariff-policy-faq
- [S9] JLCPCB Sept/Oct 2026 holiday notice (closed Oct 1–4, resumes Oct 5): https://jlcpcb.com/news/jlcpcb-september-october-2026-holiday-schedule
- [S10] FCC SDoC/verification $1–2k; unintentional radiator $1.5–2.5k; certified-module projects $6.5–10k: https://compliancetesting.com/fcc-certification-faqs/fcc-certification-cost/ , https://compliancegate.com/fcc-sdoc
- [S11] LCSC C3013941 ESP32-S3-MINI-1-N4R2 price breaks: https://www.lcsc.com/product-detail/C3013941.html
- [S12] LCSC C138713 TCA8418RTWR price breaks: https://www.lcsc.com/product-detail/C138713.html
- [S13] UN38.3 about $1,500 per model: https://tritekbattery.com/un38-3-certified-battery-pack-what-it-means-and-how-to-source/
- [S14] Alibaba custom LiPo $0.61–1.70, MOQ from 3; 603030 cell $2.00 → $1.60 @10k: https://www.alibaba.com/product-detail/_1600903701763.html , https://www.hktdc.com/event/hkelectronicsfairae/en/product/1X24U7QP
- [S15] JLC3DP pricing (from $0.30 a part): https://jlc3dp.com/news/materials-finishing-pricing-update-july2026
- [S16] SAT/ACT calculator rules (no cameras, wireless or CAS): https://collegeprep.uworld.com/sat/sat-approved-calculators-and-policy/ , https://satsuite.collegeboard.org/media/pdf/sat-testing-rules-archived-fall2026.pdf
- [S17] IEC 62133 cell + pack about $6,350 per model: https://www.batterypoweronline.com/?p=2581 (search summary)
- [S18] Magnetic pogo pins $0.23–0.25 @ MOQ 1,000 (Dongguan Lanbo, via search): https://www.accio.ai/find-product/pogo-pin-magnetic
- [S19] Generic OV5640 AF 24-pin DVP listings $4.58–£7.49: https://sourcing.ship4wd.com/product/ov5640-camera-5-million-hd-camera-modulemodule-af-auto-focus-dvp-digital-camera , https://www.ebay.de/itm/187207859030
- [S20] Good Display GDEY0213B74 retail €6.89: https://openelab.io/products/seeed-studio-e-paper-display , https://eckstein-shop.de/GooDisplay-Marken-EN__213_1
- [S21] Injection-mold tooling $3–6k simple; amortization example: https://www.teamrapidtooling.com/injection-molding-cost-1000-10000-parts-a-680.html , https://hlhrapid.com/blog/injection-molding-cost/
- [S22] Espressif FCC certificates (grantee 2AC7Z): https://www.espressif.com/sites/default/files/ESP32-S3-MINI-1 IC Certification.pdf
- [S23] CPSIA testing costs: https://www.compliancegate.com/?p=6748
- [S24] USPTO $350 per class (2025+ fee schedule): https://www.zenind.com/help/post/how-much-does-a-trademark-cost-in-2026-federal-state-and-renewal-fees-explained
- [S25] Indiana LLC $97 online: https://www.zenind.com/help/post/indiana-llc-cost-in-2026-filing-fees-ongoing-costs-and-compliance
- [S26] Amazon FBA 2026 small standard 4–8 oz about $3.15 + 3.5 % fuel surcharge: https://novadata.io/resources/blog/amazon-seller-fees-explained
- Claude API prices: Anthropic model price table (claude-api reference, cached 2026-09-25)
