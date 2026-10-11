# Who should build the board? JLCPCB vs the alternatives

Written 2026-10-06 for the v14 order (5 bare PCBs, 2 assembled). Research only: no quotes were placed, no accounts created. Prices are estimates from published price lists and public reports; only an uploaded quote is exact.

## Verdict (one line)

**Stay with JLCPCB.** For this order it is the cheapest of the services that can quote and start today, the fastest door-to-door from China, the only one that stocks all 30 of our LCSC parts in its own warehouse, and the only one our CPL rotations were checked against. It is *not* the cheapest if a free-assembly promotion (NextPCB, ALLPCB) applies, and it is *not* the fastest if money is no object (US quick-turn houses).

## The order, as it actually is

- **Board:** 2 layers, FR-4, **0.8 mm**, **ENIG**, outline **71.7 × 148.9 mm** (measured from `fab/gerbers/ai_calc-Edge_Cuts.gm1`; the "82 × 160 mm" figure in the brief is out of date). It is bigger than 100 × 100 mm, so no "$2 for 5" promo price anywhere.
- **Features:** 1.0 × 14 mm routed slot (e-paper ribbon), oval NPTH locating slots, screw holes. The 7 mm camera hole is drilled by hand in the *case*, not in the board. The ESP32 antenna and J3 overhang the board edges on purpose.
- **Assembly:** top side only, 77 placements, 34 BOM lines = **30 unique LCSC parts**, of which 14 are "Extended". The trickiest parts: ESP32-S3-MINI-1-**N4R2** module (C3013941, JLCPCB lists it as **"Standard PCBA only", X-ray required**), TCA8418 4 × 4 mm QFN-24, two 24-pin 0.5 mm-pitch FPC sockets, SOT-23/SOT-323 parts.
- **Files:** BOM and CPL in JLCPCB format with LCSC numbers. `tools/jlc_cpl.py` adds **JLCPCB-specific** rotation offsets (Q1 180, Q2 270, U2/U3/U7 270, U4/U5 180, J3 180), and J1 has deliberately reversed pin numbers. These offsets were checked 77/77 against JLCPCB's EasyEDA footprints. **They are wrong for any other assembler**, which would want the plain KiCad rotations instead.

## Comparison table

Prices are door-to-door to the US East Coast in USD for **5 PCBs + 2 assembled**, including likely import duty. Days are calendar days from paying to boards in hand, assuming parts are in stock and engineer questions get answered within a day.

| Service (where built) | Est. total delivered | Build | Ship | Door-to-door | Can it get every BOM part, incl. ESP32-S3-MINI-1-N4R2? | Accuracy / DFM | Rework our files? |
|---|---|---|---|---|---|---|---|
| **JLCPCB** (China) | **~$200–240** (~$135–155 + shipping ~$20–40 + DDP duty ~35–37.5 %) | 5–8 d (PCB 3–4, Standard PCBA 3–4) | 3–6 d express, 7–15 d economy | **~10–14 d** | **Yes, all 30 from its own LCSC stock.** C3013941: 517 in stock at LCSC today, ~$5.02 each | Engineer DFM + placement confirmation, X-ray on the QFN and module. Main risk is rotation errors, which our checked CPL already deals with | **None.** Upload as is |
| PCBWay (China) | ~$230–330 | 10–15 d (human quote 1–2 d, parts buying 3–7 d, PCBA 3–5 d) | 3–6 d | ~14–21 d | Very likely (buys by part number from DigiKey/Mouser/LCSC), but parts are bought for you at market price + minimum quantities | Strongest hands-on review in this price class: engineers check polarity and send photos before soldering | **Yes:** add MPN column; send **un-corrected KiCad rotations** + assembly PDF; re-explain J1 |
| NextPCB "Rev 0 PCBA" (China) | ~$110–170 *if* the "free assembly on first order (up to $500)" applies; otherwise ~$200–280 | Not published; parts from HQ Online (600k in stock) | 3–6 d | ~12–20 d (estimate) | Probably (HQ Online stocks ESP32-S3 modules), not checked line by line | New "no-touch" automated flow (launched May 2026): little human review, a poor match for our reversed J1 and edge overhangs | **Yes:** MPN BOM + their CPL rules; rotations re-checked from scratch |
| ALLPCB free PCBA proto (China) | ~$50–90 (PCB, parts, assembly free; you pay freight + duty) | Not published; needs application approval | 3–6 d | Unknown, likely 3+ weeks | Yes (they must buy all parts) | Unknown for this board | **Yes** (as above) |
| Seeed Fusion (China) | ~$220–320 | 7 working days only if *every* part is in their small Open Parts Library; **~20 working days otherwise** (our BOM won't be all-OPL) | 3–6 d | ~4–5 weeks | Yes eventually (they source outside parts), slowly | Decent, smaller-volume line | **Yes** |
| Elecrow (China) | ~$180–280 ($10 engineering fee; "no soldering fee ≤ 10 pcs" promo, which may have ended) | ~10–20 d with parts buying | 3–6 d | ~3–4 weeks | Yes (sourced per BOM) | Human review | **Yes** |
| MacroFab (Houston, US) | ~$350–700 (public reports: about $150+ per board for ~80-part prototypes, plus fab) | Fast "Prototype Class" (10 days) needs **< 20 unique SMT lines: we have 30**, so standard lead time (several weeks) | 1–3 d US ground | ~3–5 weeks | Yes (US distributors) | Good; US engineers, no import step on the finished board | **Yes:** their own BOM/XYRS upload, rotations from KiCad |
| Screaming Circuits / Sierra Circuits Turnkey Pro (US) | ~$800–2,000 (quick-turn premium; no public small-order list prices) | 5 working days possible (Sierra "as fast as 5 days"; Screaming Circuits 24 h–5 d assembly) | 1–2 d | **~7–10 d, fastest possible** | Yes (DigiKey/Mouser/Arrow) | Best-in-class DFM / DFA review | **Yes** |
| OSH Park (US) | Bare boards only: 0.8 mm "thin" 2-layer ENIG, $5/sq in per 3 → ~$83 per 3, **~$166 for 6** | 12–21 d for the 0.8 mm option | 2–5 d | ~3–4 weeks | **No assembly service or partner**; you'd need a separate assembler | Excellent fab quality | Bare boards only. Nirav doesn't solder, so not workable on its own |
| AISLER (Germany) | ~$300–450 (fixed job fee + per-part fee + parts; EU goods pay ~15 % US duty) | 6 business days assembly | ~5–10 d (free standard, express extra) | ~2–3 weeks | Likely: live sourcing from LCSC, DigiKey, Mouser and others. 0.8 mm board option not confirmed | Good, European | **Yes:** prefers ODB++/native KiCad upload; rotations from KiCad |
| Eurocircuits (Belgium) | ~$500–900 | ~1–2 weeks | ~5–10 d | ~2–4 weeks | Yes | Very high quality, built for professional prototypes | **Yes** |

## Why JLCPCB wins this particular order

1. **Parts are already settled.** Every line of the BOM is an LCSC number, and JLCPCB assembles straight from LCSC stock: no buying lead time, no minimum-quantity surprises, no substitution. The ESP32-S3-MINI-1-**N4R2** is the part that matters (an N8R8 loses IO33–37, which the board uses), and JLCPCB is the one place where "C3013941" means exactly that part with no translation step.
2. **The placement files were checked against JLCPCB.** Three independent reviews confirmed the CPL rotations against JLCPCB's own EasyEDA footprints (77/77). Anyone else would need a fresh, un-corrected CPL and a fresh orientation check. That would bring back the single biggest error source in low-cost assembly, just to save tens of dollars.
3. **Speed.** About 10–14 days door-to-door, against 2–5 weeks for the other low-cost options. Only the US quick-turn houses are faster, at roughly 4–10× the price, and the extra days saved are smaller than the slack in the roadmap.
4. **Price.** About $200–240 delivered. Without a promotion, no other service is clearly cheaper. The two that might be (NextPCB's free first assembly, ALLPCB's free proto) need file rework, have uncertain approval or review, and ALLPCB's offer is for "enterprises" only.

**Where JLCPCB is not the best:**
- **Cheapest:** a NextPCB or ALLPCB promo *could* save roughly $50–120. Worth considering for a **later** spin (v15), once there's time to make a second, KiCad-native CPL. Not worth it for this first build.
- **Fastest:** Sierra Circuits or Screaming Circuits could deliver in about a week, for $800+.
- **Most hand-holding:** PCBWay's engineers review more closely and send photos. For a beginner that's a real plus, but our files were tuned for JLCPCB.

## Correction to the cost in the order docs

`ORDER_WALKTHROUGH.md` §8 expects **$30–65** for shipping and import fees. That looks too low today:
- JLCPCB collects US duty up front (DDP). Its published rate is **35 % for PCB/SMT** (since 2026-03-17). Since **2026-07-24**, the 10 % Section 122 surcharge has been replaced by a **12.5 % Section 301 duty** on China-origin goods, on top of the existing 25 %. So expect roughly **35–37.5 % of the order value**, about **$50–58** on a ~$145 order.
- The US **de minimis exemption is suspended for all countries** (CBP interim final rules, 2026-06-24), and it will be abolished by law on 2027-07-01. Small parcels get no duty-free treatment from any country, EU suppliers included (about 15 %).
- Shipping is about $20–40.
- **So expect about $200–240 delivered, not $165–220.** Check the duty line at checkout. A total much above ~$260 still means something is mis-ticked.
- Small fee detail: on **Standard** PCBA, JLCPCB now charges feeder loading of **$1.53 for every unique part, Basic and Extended** (30 × $1.53 ≈ $46). The docs assumed 14 Extended × $3 = $42, so the total barely changes.

## Risks with JLCPCB and how to reduce them

| Risk | What to do |
|---|---|
| **U1 (C3013941) out of stock** or swapped for another variant | Check stock on the BOM page before paying. If it's short, use JLCPCB's "pre-order" for C3013941 (LCSC had 517 in stock on 10/6). **Accept no substitute.** The order note already says so. |
| **Rotation or polarity mistake** (D7 most of all: backwards, it shorts the charge cable) | In the placement preview, **only verify, don't rotate** (walkthrough §5 table). Tick "confirm parts placement" so an engineer checks it and you approve their photo or render before soldering. |
| **J1 reversed pin numbering "fixed" by an engineer** | Keep the order note: *"J1 pin numbering is intentionally reversed… align to pads, do not rotate."* Reply to any query with the walkthrough §7 answer. |
| **Hidden joints on the QFN (TCA8418) and the ESP32 module** | Standard PCBA includes X-ray for these; the order note asks for X-ray on U1 and U6. |
| **0.5 mm FPC sockets bridged or lifted** | Common and usually fine at JLCPCB. Inspect both J1 and J2 with a loupe on day one (Power-Up guide step 2), before any power. |
| **Edge-overhang query** (antenna, J3) delays the order | Answer within a day: rails on the right and bottom only (walkthrough §7). |
| **Shipping option**: JLCPCB has restricted DHL/FedEx Express for some US *individual* (non-company) accounts | If express isn't offered at checkout, the DDP economy line takes about 7–15 days. Allow for this in the roadmap, or order on a company account if one exists. |
| **Duty bill higher than expected** | Under DDP, the duty is shown and paid at checkout, with no surprise bill at the door. Don't choose a non-DDP method unless you're ready to pay the courier's brokerage fee as well. |
| **Post-holiday backlog** (China National Day; JLCPCB resumed 2026-10-05) | Expect the slow end of the lead-time range this week. |

## Sources (all accessed 2026-10-06)

- JLCPCB PCBA price breakdown (setup $25.56 Standard, stencil $8.21, feeder $1.53/type on Standard, updated 2026-09-09): https://jlcpcb.com/help/article/pcb-assembly-price and https://jlcpcb.com/help/article/40-pcb-assembly-price
- JLCPCB US tariff FAQ (DDP, 35 %–92.5 % range, updated 2026-09-09): https://jlcpcb.com/help/article/us-tariff-policy-faq
- JLCPCB tariff/logistics updates (PCB/SMT DDP 35 % from 2026-03-17; express restrictions): https://jlcpcb.com/news/logistics-update-us-china-tariff
- JLCPCB Sept/Oct 2026 holiday schedule (resumed 10/5): https://jlcpcb.com/news/jlcpcb-september-october-2026-holiday-schedule
- JLCPCB part page C3013941 ("Standard only", X-ray required): https://jlcpcb.com/partdetail/EspressifSystems-ESP32_S3_MINI_1N4R2/C3013941
- LCSC C3013941 stock/price (517 pcs, $5.02 @1): https://www.lcsc.com/product-detail/C3013941.html
- JLCPCB Economic vs Standard capabilities: https://jlcpcb.com/capabilities/pcb-assembly-capabilities
- CBP de minimis indefinite suspension, all countries (2026-06-24): https://www.federalregister.gov/documents/2026/06/24/2026-12670/indefinite-suspension-of-the-de-minimis-exemption-for-merchandise-arriving-through-all-modes-other and https://www.thompsonhinesmartrade.com/2026/06/cbp-issues-interim-final-rules-indefinitely-suspending-the-de-minimis-exemption-for-imports/
- Section 122 expiry and new Section 301 rates from 2026-07-24 (China 12.5 % on top of existing 301): https://tariff.gatewaylines.com/section-122 and https://carraglobe.com/section-301-tariffs-2026/
- PCBWay assembly overview ("from $29", 3–5 day PCBA, free stencil): https://www.pcbway.com/pcb-assembly.html
- JLCPCB vs PCBWay quoting model (instant vs human RFQ): https://atlaspcb.com/blog/pcbway-vs-jlcpcb-comparison
- NextPCB Rev 0 PCBA (May 2026, free first assembly up to $500): https://www.cnx-software.com/2026/05/13/nextpcb-officially-launches-rev-0-pcba-an-automated-no-touch-prototype-assembly-service-built-for-speed-and-predictability/
- NextPCB ESP32-S3 accelerator (2 free PCBA, 20 projects; 2025 offer, may be closed): https://www.cnx-software.com/2025/03/10/nextpcb-offers-free-esp32-s3-pcba-prototypes-for-original-designs/
- ALLPCB free PCBA prototype (Oct–Dec, max 100 × 150 mm, enterprises only): https://www.allpcb.com/activity/PCBAFreeProto
- Seeed Fusion lead times (7 working days OPL / ~20 otherwise): https://support.seeed.cc/portal/en/kb/articles/what-is-the-lead-time-for-seeed-fusion-pcb-assembly-orders
- Elecrow PCBA ($10 engineering fee; ≤ 10 pcs soldering promo): https://www.elecrow.com/pcb-assembly.html and https://elecrow.com/blog/elecrow-special-pcba-promotion-0-soldering-fees-for-pcba-sample-orders-of-10pcs-or-below.html
- MacroFab Prototype Class rules (< 20 unique SMT lines): https://www.macrofab.com/blog/macrofab-now-offers-10-day-prototyping ; user price reports: https://community.sparkfun.com/t/anyone-tried-circuithub-or-macrofab/31339
- Sierra Circuits Turnkey Pro (US, as fast as 5 days): https://www.protoexpress.com/products/turnkey-pro/
- Screaming Circuits (no NRE, 24 h quick-turn): https://www.electronicdesign.com/boards/are-you-screaming-help-prototypes
- OSH Park services (0.8 mm thin, $5/sq in per 3, 12–21 days, ENIG; no assembly listed): https://docs.oshpark.com/services/
- AISLER assembly (6 business days, live LCSC/DigiKey/Mouser sourcing): https://aisler.net/products/assembly?lang=en-IT
