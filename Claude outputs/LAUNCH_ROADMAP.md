# AI Calculator launch roadmap

Written Saturday 2026-10-03; timeline and JLCPCB cost updated 2026-10-04 for the final v14 board (`hardware/FINAL_STATUS.md`). Prices were checked online on 2026-10-03 unless marked **(estimate)**. Sources are listed at the end, numbered [S1], [S2] and so on.

Chart data (timeline tasks, price-scaling table, API costs) is in `launch_roadmap_data.json` in this folder.

> **Status update 2026-10-06 (docs audit, `hardware/verification/10_docs_consistency.md`):** the JLCPCB order was **not** placed on Mon 10/5; nothing below is ordered yet. Every day the order slips moves the dates from 10/15 on by a day (boards about **10–14 days** door-to-door after ordering, so about **10/17–10/21** for an order on 10/7; economy shipping alone is 7–15 days if express isn't offered). **Cost update 10/6 (`hardware/FAB_VENDOR_COMPARISON.md`):** JLCPCB is still about $135–155 before shipping but now about **$200–240 delivered**: US import duty of about 35–37.5 % is collected at checkout (de-minimis suspended since 2026-06-24) plus $20–40 shipping; the totals below include it. Changes since this was written: the battery is now the **Adafruit #1317, 150 mAh** (same price, same plug; `hardware/enclosure/final_assembly/battery_upgrade.md`); the **API proxy exists** (`server/proxy/`, FastAPI, device tokens, 30-day free trial, 500 solves/month cap) and the stage-13 firmware never holds the API key; the **factory self-test exists** (SHIFT + ALPHA, ON); the camera hole is **7 mm**, not 6; the battery is held with **0.1 mm double-sided tape, no foam**. The decided price is **$225 + $15/month, first month free** (§0; the $3–5/month idea in §3.3 is the earlier analysis, and `COMPETITOR_ANALYSIS.md` B1 argues for a lower subscription: still an open business question).

---

## 0. The short version

| Question | Answer |
|---|---|
| What you spend by Monday night | About **$380** for the core electronics order (JLCPCB ≈ $200–240 delivered + Adafruit + panels + cameras; updated 10/6). About **$430** with the small tools (you already own the calculators and a multimeter). Likely range **$390–460** (estimate). |
| Why it's higher than the old $190–240 | JLCPCB now charges US tariffs up front (DDP). Its FAQ puts the total tariff on Chinese goods at **35–92.5 %** [S8]. The old figure also left out shells, tools, the charge cables and DHL. |
| When you'll have working units | Boards land about 10–14 days after the order (≈ 10/17–10/21 for an order on 10/7). First working calculator about 2 days after the boards land if nothing slips. Plan on a **10/21 buffer**: JLCPCB has only just reopened after China's National Day holiday (closed Oct 1–4, back on Oct 5 [S9]), so there may be a backlog. |
| Cost per unit (estimate) | **$138 at 2 · $65 at 100** (plus about $35 of one-time certification spread over those 100) **· $41 at 1,000 · $26 at 10,000** |
| Claude API cost | About **$0.026 per solve** on Sonnet 5.5, and about **$3.90 per user per month** at 150 solves (estimate). See §3.3. |
| Price (decided 10/4) | **$225** per calculator + **$15/month** subscription, **first month free** (the proxy's `TRIAL_DAYS=30`), fair-use cap 500 solves/month (`MONTHLY_CAP`). The calculator must still work as a normal scientific calculator without a subscription. Full margin tables: `launch_roadmap.html` §0. |
| Biggest legal facts | 1. A camera + Wi-Fi calculator is **banned on the SAT and ACT** [S16], so market it as a homework and study tool. 2. **Don't sell modified Casio shells at scale** (trademark risk). 3. Before selling to the public you need an **FCC Part 15B SDoC** (about $1–2.5k [S10]), and the battery needs a **UN38.3 test summary**. |

---

## 1. Parts list for the first build (2 assembled boards out of 5)

### 1A. Order first, as soon as the paper dry fit passes (was "Monday 10/5"; not ordered as of 10/6; fastest shipping listed)

| # | Item | Vendor and link | Qty | Unit | Total | Lead time to Indiana | Notes |
|---|---|---|---|---|---|---|---|
| 1 | PCB 2-layer, 0.8 mm, ENIG, 5 pcs, **2 assembled** (Standard PCBA) | JLCPCB, jlcpcb.com | 1 order | – | **≈ $135–155 before shipping** (final review 2026-10-04: PCBs, PCBA setup + stencil, 14 Extended fees, parts for 2 boards) | Build about 5–7 days. Standard PCBA alone is "≥4 days" [S7]. DHL/FedEx 3–5 days | Use `hardware/ORDER_WALKTHROUGH.md`. The CPL is rotation-corrected: **verify the preview, rotate nothing.** |
| 1b | DHL Express shipping | JLCPCB checkout | – | – | $20–40 **(estimate)** | – | Pick DHL or FedEx; economy (7–15 days of shipping) only if express isn't offered. **JLCPCB delivered ≈ $200–240.** |
| 1c | US tariff, paid at checkout (DDP for individuals) | JLCPCB checkout | – | – | ≈ $45–55 **(estimate: about 35–37.5 % duty, de-minimis suspended since 2026-06-24; `hardware/FAB_VENDOR_COMPARISON.md`)** | – | The exact figure appears at checkout. |
| 2 | LiPo 3.7 V **150 mAh**, JST-PH (**#1317**, 26 × 19.75 × 3.8 mm; replaced the 100 mAh #1570 on 10/6) | Adafruit, adafruit.com/product/1317 | 3 (1 spare) | $5.95 (in stock 10/6) | $17.85 | Ships from NYC. UPS 2nd-Day ≈ 2–3 days | Peel the lead tape off the top of the cell; meter the polarity before plugging in |
| 3 | DIY magnetic connector, 4-pin right-angle (#5358, both halves) | Adafruit, adafruit.com/product/5358 | 3 | $6.50 [S2] | $19.50 | same box | Also at DigiKey/Mouser for $6.50 [S2] |
| 4 | Magnetic USB charging cable for 4-pin (#5412) | Adafruit, adafruit.com/product/5412 | 2 | $4.95 [S3] | $9.90 | same box | **Only 36 in stock** [S3]. Without it, #5358 has nothing to plug into. |
| 4b | Adafruit shipping (UPS 2nd Day) | – | – | – | ~$15 **(estimate)** | – | – |
| 5 | Waveshare 2.13" e-Paper **V4 raw panel** (B/W, 250×122, SKU 12672) | waveshare.com, or Amazon if a Prime listing exists | 3 (1 spare; the ribbon is fragile) | $6.31–6.99 [S4] | ~$21 | Waveshare direct: DHL 4–8 days **(estimate)**. Amazon Prime: 1–2 days | Make sure it says **V4** (SSD1680 driver). V2/V3 panels need different firmware. |
| 5b | Waveshare shipping | – | – | – | ~$15–25 **(estimate)** | – | – |
| 6 | **Seeed OV5640 AF camera** (24-pin FPC, the "for XIAO ESP32S3 Sense" one) | seeedstudio.com, or a US reseller | 3 (1 spare) | ~$14 **(estimate; £11 in the UK [S5])** | ~$42 | Seeed US warehouse 3–6 days **(estimate)** | Buy **this exact module**. J1's reversed pinout was worked out for it. Avoid the $46 Newegg reseller [S5]. |
| 6b | Camera shipping | – | – | – | ~$10–15 **(estimate)** | – | – |
| | **Core subtotal (Monday)** | | | | **≈ $350–405, midpoint ≈ $380** (updated 10/6 with the import-duty estimate) | | |

### 1B. Buy locally THIS WEEKEND (no shipping wait)

| # | Item | Where | Qty | Unit | Total | Notes |
|---|---|---|---|---|---|---|
| 7 | Casio fx-115ES donor calculator | Walmart / Target / Staples | 2 (plus the one you measured) | $19.99 at Walmart [S6] | ~$43 incl. 7 % IN tax | **Check the box says the same edition you measured** (fx-115ES PLUS vs PLUS **2nd edition**). The insides can differ: posts, ribs, key pads. |
| 8 | Digital calipers (0.01 mm) | Harbor Freight / hardware store | 1 | ~$20–30 **(estimate)** | ~$25 | Needed TODAY for the measurement sheet |
| 9 | Multimeter with continuity beep | same | 1 (if you don't own one) | ~$20–30 **(estimate)** | ~$25 | Needed for camera-cable check E6 and the power rails |
| 10 | Kapton tape, 10–20 mm | Amazon/hardware store | 1 roll | ~$8–10 **(estimate)** | ~$9 | Insulate the battery and the back of the e-paper; mask the plastic before drilling |
| 11 | Dremel bits: carbide burr + sanding drums + **7 mm drill (9/32") and a small pilot bit** | hardware store | 1 set | ~$10–15 **(estimate)** | ~$12 | Grind the solar box, rib B and the magnet notch; drill the 7 mm camera window (Shell Grinding Guide) |
| 12 | Flush cutters + ESD tweezers | hardware store | 1 each | ~$15 **(estimate)** | ~$15 | |
| 13 | USB-A wall charger, or USB-A(female)-to-USB-C adapter for your laptop | anywhere | 1 | ~$6–8 **(estimate)** | ~$7 | The magnetic cable ends in **USB-A**. Flashing also goes through it (native USB on the ESP32-S3). |
| 14 | Thin double-sided tape, **0.1 mm (not foam)** for the camera and battery, about 0.15 mm for the e-paper, + 99 % isopropyl | anywhere | 1 each | ~$10 **(estimate)** | ~$10 | Holds the camera, battery and e-paper; clean the ENIG pads. **No foam anywhere:** there is only 0.3 mm above the battery and 0.5 mm above the camera lens |
| 15 | Matte black vinyl or black sticker paper, 0.08–0.15 mm | craft/office store | 1 sheet | ~$5 **(estimate)** | ~$5 | Window mask inside the display lens (`window_mask.md`) |
| 16 | Epoxy or hot glue | hardware store | 1 | ~$6 **(estimate)** | ~$6 | Glues the magnet piece into the top-wall notch |
| | **Local subtotal** | | | | **≈ $130–150** | Skip whatever you already own |

**Order-day total: about $380 for the core order + about $40–55 local ≈ $430 (estimate). Range $390–460 depending on duty and shipping.**

### 1C. Don't buy yet
- More than 3 of anything. Prototype v1 will change something.
- A soldering station. The design has nothing for you to solder. Only buy one if bring-up shows a bad joint.
- A pogo-pin test jig. That belongs to v2 (see §5).

---

## 2. Timeline

### 2A. Day by day, 10/3 → 10/21 (100 % effort)

| Date | Day | Task | Who | Waits on |
|---|---|---|---|---|
| 10/3 | Sat | Buy donor fx-115ES ×2, calipers, multimeter (§1B) | Nirav | – |
| 10/3 | Sat | Caliper measurements: C6 outline, C12/C13 LCD window, C14 wall stubs, D13 rib heights, plus V3/V5/V9/V11/V12 in `geometry_assumptions.md` | Nirav | calipers |
| 10/3 | Sat | Firmware: freeze `pins.h`; e-paper and keypad drivers on the simulator or dev board | Nirav + Claude | – |
| 10/4 | Sun | Give the measurements to the KiCad session. It moves parts, runs DRC to 0/0/0 and regenerates `fab/` | Claude (KiCad session) | measurements |
| 10/4 | Sun | **Done:** board v14 finished, DRC/ERC 0, independent re-check GO; fab files in `hardware/fab/` | Claude | – |
| 10/4–5 | Sun–Mon | **Paper dry fit** (`hardware/FINAL_STATUS.md` §1): print `grind_map_front_shell.svg` at 100 %, check the 50 mm line, every post inside its hole | Nirav | – |
| 10/4 | Sun | **Done:** the **API proxy** (`server/proxy/`) holds the Claude key; the device only talks to the proxy with its own token. | Claude | – |
| **10/5** | **Mon** | **Order:** Adafruit (UPS 2-day), cameras, e-paper; then **JLCPCB by ~9 pm ET**, after the paper test passes (`hardware/ORDER_WALKTHROUGH.md`). Verify the preview against step 5; rotate nothing. About $135–155 before shipping, $200–240 delivered. **Not done on 10/5 (audit 10/6): order as soon as the paper dry fit passes; every day moves the rows below by a day.** | Nirav | paper dry fit |
| 10/6 | Tue | JLCPCB engineer review. **Answer their emails within hours**: each unanswered question costs about a day. | Nirav | order |
| 10/6–7 | Tue–Wed | Firmware: camera capture → JPEG → Wi-Fi → proxy → answer, on the dev board | Nirav + Claude | proxy |
| 10/7 | Wed | Prepare shells: open, remove the Casio board (keep the rubber keypad and screws), mask with Kapton, grind ribs, drill the camera hole | Nirav | donor calcs |
| 10/8 | Thu | Adafruit box arrives **(estimate)**. Check battery polarity against J4 (sheet E3). **A reversed JST lead is common.** | Nirav | Adafruit |
| 10/9 | Fri | JLCPCB finishes the bare PCBs **(estimate)**. Cameras and e-paper arrive (Waveshare direct may take until 10/12–15). | JLCPCB / vendors | – |
| 10/10–11 | Sat–Sun | **Done early (stage 13):** the **factory self-test** (SHIFT + ALPHA, ON, or `selftest`: keys, display pattern, camera JPEG, battery, Wi-Fi scan, one JSON line). Use the days for the proxy deployment on Render and the shell prep instead. | Nirav + Claude | – |
| 10/12 | Mon | JLCPCB assembly and X-ray done, board ships by DHL **(estimate)**. Camera cable check E6: fingers 2 and 15 should beep to GND. | JLCPCB / Nirav | – |
| **10/15** (≈ 10/17–10/21 if ordered 10/7) | Thu | **Boards arrive (estimate).** Bring up board #1 on USB only, no battery, display or camera: inspect, meter checks, VBUS / SYS / 3.3 V (TP5) / EN (TP7), then flash over the magnetic USB (First Power-Up Guide, steps 2–7). | Nirav | DHL |
| 10/16 | Fri | Plug in the e-paper, then the camera; run the self-test; then battery and charging (check the charge current is about 50 mA) | Nirav | bring-up |
| **10/17** | Sat | **Assemble into shell #1. End-to-end: photo → Claude → answer on the e-paper.** | Nirav | shells + self-test |
| 10/18 | Sun | Board #2 bring-up and assembly. 24-hour battery soak. Write the v2 bug list. | Nirav | – |
| 10/21 | Wed | **Buffer** in case JLCPCB or DHL slips 2–4 days | – | – |

**Critical path:** measurements (10/3) → board v14 (10/4) → paper dry fit → **JLCPCB order (planned Mon 10/5, slipped; every later date moves with it)** → build (≈ order + 5–8 days) → DHL (≈ order + 10–14 days door-to-door) → bring-up → assembled unit (≈ order + 12–16 days).

Only the JLCPCB leg is outside your control. Everything else (firmware, proxy, shell prep) runs in parallel while you wait.

### 2B. Phases after the first units

| Phase | When (estimate) | Units | What happens | What Nirav does |
|---|---|---|---|---|
| **Prototype v1** | Oct 2026 | 2 | Working units in Casio shells | Everything |
| **Prototype v2** | late Oct – Nov 2026 | 5–10 | Fix v1 bugs; **add test pads** (3.3 V, GND, EN, IO0, USB D+/D−, battery) for a pogo jig; second JLCPCB order | Design + test |
| **Custom 3D-printed shell** | Nov 2026 – Jan 2027 | 5–10 | build123d shell printed at JLC3DP (printing from about $0.30 a part, real quote needed [S15]); own keypad (silicone or printed keys + metal domes) | CAD + fit |
| **Pilot batch** | Jan – Mar 2027 | 10–50 | Give units to testers aged 13+; collect feedback; FCC pre-scan at a lab | Support, fixes |
| **First sales** | Mar – Jun 2027 | ~100 | LLC formed, FCC SDoC done, preorders (Crowd Supply / Kickstarter / own site) | Run the business; hand-assemble or pay a local helper |
| **Hundreds** | H2 2027 | 300–1,000 | Custom battery + OEM magnetic connector (§6); JLCPCB or another EMS does PCBA **and** box build; 3PL ships orders | Supplier management, software |
| **Thousands + injection molding** | 2028 | 3,000+ | Steel or aluminium molds pay off above about 3,300 units (§3.2) | Approve samples, QA |
| **Contract manufacturer** | 2028–29 | 5k–10k+ | The contract manufacturer (CM) buys parts, builds, tests, packs and ships to Amazon FBA or a 3PL | Product, firmware, marketing |

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

### 3.1 Which parts SHIFT supplier at scale

| Part | Now (retail) | Switch at | Switch to | Why / catch |
|---|---|---|---|---|
| Battery | Adafruit #1317 150 mAh $5.95 (was #1570) | **~500 units** | A Shenzhen custom LiPo with protection circuit, JST-PH and **UN38.3 + IEC 62133 reports from the supplier**. Alibaba LiPo cells cost $0.61–1.70; MOQ from 3 pcs to 1,000 [S14] | Ask for the UN38.3 test summary **before** paying. Paying for your own UN38.3 costs about $1,500 per model [S13]; IEC 62133 about $6,350 [S17]. |
| Magnetic connector | Adafruit #5358 $6.50 → $5.20 @100 [S2] | **~500** | Shenzhen/Dongguan magnetic pogo pair. Pins cost about $0.23–0.25 at MOQ 1,000 [S18] | **Needs a new footprint** (today J3 is a right-angle SMD female header, C46061768, that takes the #5358's straightened legs). Make the change in a v3 board. |
| Charge cable | Adafruit #5412 $4.95 [S3] | ~500 | OEM magnetic USB-C cable, about $1–1.8 **(estimate)** | Order it together with the connector so they mate |
| Camera | Seeed ~$14 | **~50** | Generic OV5640 AF 24-pin DVP modules, listed at $4.58–$8 [S19] | **Check the FPC pinout and cable length on every new vendor.** J1 is reversed for one specific cable direction. |
| E-paper | Waveshare $6.31–6.99 [S4] | ~100 | Good Display GDEY0213B74 direct (the same panel family; about €6.89 retail [S20]); volume quote by RFQ | Needs a quote by email; MOQ applies |
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
| 1 | **Measurements move a part** after Monday, or the donor calculator is a different edition | A board that doesn't fit = a new order + 2 weeks | Do the 1:1 paper fit check on Sunday. Buy donors of the **exact** measured edition. |
| 2 | **JLCPCB backlog after the holiday** (closed Oct 1–4 [S9]) and DFM questions | +2–4 days | Order as soon as the paper dry fit passes; reply to JLCPCB emails fast; 10/21 buffer |
| 3 | **Camera ribbon orientation (J1 reversed)** is about 90 % certain (verification 01/03) | No image | Buy the same Seeed module; run the E6 beep test; a 180° cable twist fixes it |
| 4 | **Tariff/DDP cost** higher than expected | +$30–70 on this order; +35 %+ at volume [S8] | Check the checkout total Monday; budget the high end |
| 5 | **API key in firmware / unlimited API use** | Stolen key, surprise bill | Proxy + per-device token + caps before any unit leaves your hands |
| 6 | Supply of #5412 cable (35 in stock on 10/6 [S3]) | No way to charge | Order 2 first; Jameco/DigiKey as backup |
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
