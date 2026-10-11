# Docs consistency audit (2026-10-06, 01:20–01:40 EDT, stopped early to save usage)

Scope: every document Nirav will hold when the boards arrive. The three HTML guides, the order docs, QUESTIONS_AND_ISSUES, the roadmap (md + html), the two spreadsheets, the final-assembly notes, the firmware guide, and the JLCPCB BOM/CPL. Live price/stock checks were limited to the 5 key retail parts. **Not done (stopped by the coordinator):** the QUESTIONS_AND_ISSUES.md rewrite, the one-page FINAL_STATUS.md rewrite, the spreadsheet updates. Those are listed under "Still to do".

## 0. The one fact that overrides everything

**The board is not ordered** (as of Tue 2026-10-06). The roadmap, the shopping list and the bring-up guide all assumed a Mon 10/5 order and "boards about 10/15". Boards land about 10 days after the order, so about **10/17 for an order on 10/7**; every day the order slips moves every later date by a day. HANDOFF.md was the only doc that said "not ordered"; FINAL_STATUS.md, both roadmap files and the bring-up guide now say so too.

## 1. Disagreements between documents

| # | Topic | Doc A says | Doc B says | Truth / action |
|---|---|---|---|---|
| 1 | Battery | `LAUNCH_ROADMAP.md` §1, `launch_roadmap.html`, `launch_roadmap_data.json`, shopping-list note "switched on 10/6", `Alibaba_Bulk_Sourcing.xlsx`, `fitcheck/README.md`, `stage13_heights.md`: Adafruit **#1570, 100 mAh, 31 × 11.5 × 3.8** | `battery_upgrade.md` §8, `assembly_report.md` §9, assembly guide, FINAL_STATUS decisions log, HANDOFF, shopping list row 4: **#1317, 150 mAh, 26 × 19.75 × 3.8** | #1317 (decided 10/6). Fixed in both roadmap files. Still wrong: `launch_roadmap_data.json`, `Alibaba_Bulk_Sourcing.xlsx` ("max 31 × 11.5 × 3.8" spec), `fitcheck/README.md` and the **dummy_board.stl battery block** (grinding guide now says so). |
| 2 | Order status / dates | Roadmap: "Order Mon 10/5", boards 10/15, first unit 10/17 | HANDOFF: "Not ordered yet" | Not ordered. Fixed in FINAL_STATUS, HANDOFF, both roadmap files, bring-up guide header. Shopping list "Order by Mon 10/5" still stale. |
| 3 | Camera drill | Roadmap §1B item 11 and html: "**6 mm** drill" | Everything else (stage 13, grinding guide, assembly guide, ORDER_CHECKLIST): **7 mm** (9/32" ok) at KiCad (150, 95.1) | 7 mm. Fixed in both roadmap files. `launch_roadmap_data.json` still says "~6mm". |
| 4 | Tape under battery/camera | Roadmap item 14: "thin double-sided **foam** tape" | Assembly guide, battery_upgrade §8, grinding guide: **0.1 mm tape, no foam** (0.3 mm above the battery, 0.5 mm above the lens); e-paper on ≈ 0.15 mm | No foam. Fixed in both roadmap files. `QUESTIONS_AND_ISSUES.md` Heights §3 still says "≤ 0.2 mm" for the camera (stage-13 wording; 0.1 is the current rule). |
| 5 | Camera ribbon spare | Grinding guide test-fit: "spare about **12 mm**, gentle loop"; `assembly_report.md` §3 and `final_assembly/README.md`: 12 mm / ~11 mm loop | Stage 14 fix 6, ORDER_CHECKLIST, FINAL_STATUS, bring-up and assembly guides: only about **1.4 mm** spare; under 70.3 mm → move the camera ≤ 1.7 mm towards J1 | 1.4 mm (the 70.5 mm on the drawing runs from the module's far edge). Fixed in the grinding guide. `assembly_report.md` §3 and `final_assembly/README.md` still carry the 12 mm figure (model notes, harmless). |
| 6 | E-paper ribbon spare | Stage 14 fix 6 / HANDOFF: "no spare, 14.3 = 14.3" | Assembly guide: "the 3D model finds about 1.6 mm" (flagged *docs disagree* in the guide) | Unresolved on paper; the guide already tells him to push the ribbon fully in. Measure E7 before ordering. |
| 7 | Stage-14 fix numbering | FINAL_STATUS §6 and HANDOFF: "fix 4" = E7/stiffener rule, "fix 6" = camera | `stage14_verification_fixes.md` headings: §4 = recovery/R17, **§6 = both ribbon rules** | §6. Fixed in HANDOFF; FINAL_STATUS §6 item 2 still says "fix 4". |
| 8 | Grind-zone numbering | Grinding guide and assembly guide: 1 solar box, 2 rib B, 3 camera window, 4 LR44 (skip), 5 magnet notch, 5b lip | `jigs/README.md` and `battery_upgrade.md` §3: 4 = magnet slot, 5a/5b = LR44 trim/removal | Guides win. Cross-references added to the jigs README and battery_upgrade.md. |
| 9 | Camera-ribbon orientation confidence | Roadmap html risks table: "~75 % certain" | Roadmap md, FINAL_STATUS, HANDOFF, assembly guide: ~90 % after verification 01/03 | 90 %. Fixed in the html. |
| 10 | Which JLCPCB doc to follow / rotate in preview | Roadmap html §1: "Follow ORDER_CHECKLIST.md. Check every pin-1 in the 3D preview"; "v13 board" | Walkthrough, checklist, FINAL_STATUS: v14, CPL rotation-corrected, **verify, never rotate** | Fixed in the html. |
| 11 | Tariff line | Roadmap html: tariff "$28–73" + DHL "$20–30" (sums to $48–103) | Roadmap md and order docs: shipping + tariff together $30–65, delivered $165–220 | Fixed in the html to match. |
| 12 | Proxy | Roadmap §2A/§3.3/§5: "build an API proxy", "today claude_client.cpp calls the API directly" | `server/proxy/` exists (FastAPI, tokens, TRIAL_DAYS=30, MONTHLY_CAP=500); FIRMWARE_STAGE13: firmware never holds the key | Done. Fixed in both roadmap files. Still open: deploy it (Render), real sign-in on the link page. |
| 13 | Factory self-test | Roadmap: "start 10/10" | FIRMWARE_STAGE13: exists (SHIFT + ALPHA, ON, or `selftest`) | Done. Fixed in both roadmap files. |
| 14 | Self-test JSON fields | Bring-up guide table: memory_ok, keypad_scanner_ok, vbus, battery_mv, charge, display_refresh_ms, camera_ok, camera_jpeg_bytes, wifi_ok, keys_ok, keys_missing, pass | `selftest.cpp` also emits battery_ok, battery_pct, camera, camera_autofocus, camera_ms, chip_rev, device_id, flash_mb, psram_kb, keys_total, keys_all_ok, reset_reason, trigger, wifi_networks, wifi_best_rssi, battery_mv_radio_on | Guide fields all exist in the firmware; names agree. No change needed. |
| 15 | Test count | FIRMWARE_STAGE13: "541 checks" | QUESTIONS_AND_ISSUES stage 14, stage14 doc: 548/548 | 548 is newer (one golden added in stage 14). Not edited. |
| 16 | Subscription price | Roadmap §0/html: **$225 + $15/mo, first month free** (matches proxy TRIAL_DAYS=30) | Roadmap §3.3: "$3–5 a month or $29–39 a year"; COMPETITOR_ANALYSIS B1: $4.99/mo | §3.3 is the earlier analysis; now labelled as such in the md. The $15 stays an open business question. |
| 17 | Bring-up voltages | Roadmap 10/15 row: "measure 3.3 V / 2.8 V / 1.5 V" (no 2.8/1.5 test pads exist; those rails are camera LDOs behind CAM_PWR_EN) | Bring-up guide: VBUS, SYS, TP5 3.3 V, TP7 EN, TP6 BAT | Guide is right. Roadmap rows fixed. |
| 18 | Charge current / recovery / key combo / J3 order / magnet notch (21.5 × 7.95, 6.4–27.9) / grinds / D1 5.5 / D4 6.0 / TP numbers / window mask (47.75 × 22.90, 2.55 right) / prices $135–155 → $165–220 | — | — | **Agree everywhere checked** (FINAL_STATUS, walkthrough, checklist, HANDOFF, pins_final.h, FIRMWARE_STAGE13, all three guides, window_mask.md, jigs README). |
| 19 | E-paper "V4" | Roadmap, shopping list, HANDOFF: "Waveshare 2.13" **V4** raw panel, SKU 12672" | Waveshare's raw-panel page (SKU 12672) does not print "V4" (that suffix is the HAT/module name); specs match: 250 × 122, 59.2 × 29.2 × 1.05, SSD1680 per `pins_final.h` | Same part. Tell him: buy SKU 12672, the page may not say V4. Not edited. |
| 20 | Guide links | FINAL_STATUS §3 and the bring-up guide linked only the grinding guide; the assembly guide (published 10/6) was linked nowhere outside itself | Assembly guide links both others | Assembly-guide link added to FINAL_STATUS (status note), HANDOFF, bring-up guide (step 10 + footer), grinding guide footer. Artifact URLs verified against the account's artifact list: Power-Up PqTARcDutvwxS5aGB8iEG9, Grinding 9rCWyw8MeHP2z8FWijnyfL, Assembly 8BTC57FZ3JFwiDdiLprwtY, Roadmap K5hMpGzzf4gS9b6nGKSoM9. |

## 2. Shopping list verdict (live, 2026-10-06 ~01:30 EDT)

| Part | List says | Live | Verdict |
|---|---|---|---|
| Adafruit #1317 150 mAh LiPo | $5.95 × 3 | $5.95, in stock, 19.75 × 26.02 × 3.8 mm, JST-PH, lead 127.5 mm | OK |
| Adafruit #5358 magnet connector | $6.50 × 3 | $6.50, in stock, both halves, 0.1" pitch | OK |
| Adafruit #5412 magnet cable | $4.95 × 2, "~36 in stock" | $4.95, **35 in stock**, USB-A, 60 cm | OK; order first |
| Seeed OV5640 AF for XIAO ESP32S3 Sense (114993115) | "~$14" × 3 | Seeed $12.99 (with heat sink), in stock; DigiKey US $11.99, 673 in stock | OK; the list's SKU cell is a search link, write 114993115; DigiKey is the faster US source |
| Waveshare 2.13" raw panel SKU 12672 | $6.99 × 2 | $6.99 (1–9), $6.31 (100+); 59.2 × 29.2 × 1.05, 250 × 122 | OK (see #19 about "V4") |
| JLCPCB LCSC parts | BOM in `fab/` = `bom/` (identical) | C46061768 (J3, hanxia PM2.54-1x4P right-angle SMD, Extended), C295747 (J4, JST S2B-PH-SM4-TB), C6364666 (J1/J2, 24P 0.5 mm dual-contact flip-top), C3013941 (ESP32-S3-MINI-1-N4R2), C841192 (RT9080-33GJ5), C138713 (TCA8418RTWR): all resolve to the parts the docs assume. Stock numbers are not shown to anonymous fetches | Same part numbers as the docs. Check stock in the JLCPCB BOM upload step (walkthrough step 4) |

Quantities for 2 complete prototypes: 2 assembled boards (+3 bare), 3 cameras, 3 panels (1 owned + 2), 3 batteries, 3 magnet pieces, 2 cables: **complete, spares on the fragile parts.** Missing from the spreadsheet (they are in the roadmap or the guides): 7 mm drill + pilot bit and burr/sanding drum, matte black vinyl for the window mask, 99 % isopropyl, safety glasses/dust mask, USB-A charger / USB-A→C adapter. Spreadsheet total as it stands ≈ $380; with those ≈ $410, against the roadmap's "Monday ≈ $400" and the $2,000 budget. The spreadsheet itself was **not** edited (stopped).

## 3. Day-one sequence (one list, guide and step)

Guides: Power-Up = `hardware/bringup_guide.html`; Grind = `grinding_manual.html`; Assembly = `assembly_guide.html`.

1. Before the boards: take the Casio apart (Assembly 1); grind zones 1, 2, 5, 5b (Grind; Assembly 2); hold the 7 mm hole (zone 3) until the ribbon is measured; make the window mask (Assembly 5 / `window_mask.md`); deploy the proxy (FIRMWARE_STAGE13 "Deploy the proxy").
2. Boards arrive: inspect unpowered (Power-Up 2).
3. Magnet piece meter check with the real cable, before gluing (Power-Up 3 = ORDER_WALKTHROUGH step 9).
4. Battery polarity (Power-Up 4).
5. Camera ribbon beep test + length (Power-Up 5).
6. First power, computer USB only: VBUS, SYS, TP5, TP7, TP6 (Power-Up 6).
7. Flash; recovery = TP1→TP4 held, tap TP7 (Power-Up 7).
8. `selftest`, save the JSON (Power-Up 8).
9. E-paper → selftest; camera → selftest; battery → charging (Power-Up 9).
10. Hand-over: Power-Up 10 → Assembly 3 "must pass" list, battery unplugged. (Hand-over is now explicit in both directions.)
11. Assembly 4 (prepare), 5 (e-paper + mask), 6 (camera, drill if held), 7 (magnet into J3).
12. Assembly 8 (mat, board, key test on cable power, glue magnet).
13. Assembly 9 (battery) and 10 (close).
14. Assembly 11 (self-test by key combo, preview, proxy pairing, AI solve on battery, charging, overnight memory).

Nothing is done twice (the bare-board checks are done once in Power-Up and only ticked again in Assembly 4); nothing is skipped. The only soft spot is #6 above (e-paper ribbon spare), which no doc settles.

## 4. QUESTIONS_AND_ISSUES.md (audit only; rewrite not done)

Truly open: paper dry fit; E7 + stiffener length; camera ribbon length; magnet meter check; J4 polarity; E6; proxy host; $15/mo pricing; donor edition check; the CAD photo requests (side profile, slide case) which matter only later. Closed but still in the ⭐ list or elsewhere as open: "bigger cell?" (decided: #1317); "Business 10, calculator works without subscription" (roadmap html §0 says yes); Firmware items 3–5 (proxy switches exist with the assumed defaults). Stale wording: Heights §3 "≤ 0.2 mm tape" (now 0.1, no foam); PCB (stage 12) §7 duplicates the grind list twice.

## 5. Files changed (46 scripted edits + 1 manual)

- `hardware/bringup_guide.html`: header date; step 10 renamed "Hand over to the Assembly Guide" with link; grinding link reworded; tick c5 split and a new tick c6 (write down ribbon length + JSON); footer links. **Republish** PqTARcDutvwxS5aGB8iEG9.
- `hardware/enclosure/final_assembly/assembly_guide.html`: camera parts line (70.5 mm is the drawing, measure yours); footer entry-point link. **Republish** 8BTC57FZ3JFwiDdiLprwtY.
- `hardware/enclosure/final_assembly/grinding_manual.html`: camera-ribbon spare 12 mm → 1.4 mm rule; dummy-board battery-block note (#1570 block in the STL); footer links. **Republish** 9rCWyw8MeHP2z8FWijnyfL.
- `Claude outputs/launch_roadmap.html`: eyebrow, wave-1 date, JLCPCB row (v14, walkthrough, no rotating), tariff row, battery row (#1317), #5412 stock 35, Dremel row (7 mm), tape row (no foam) + new mask/glue row, 10/5 order row (slipped), proxy row (done), 10/15 row, J1 risk 90 %, cable risk, sources S1/S3, factory test firmware done. **Republish** K5hMpGzzf4gS9b6nGKSoM9.
- `Claude outputs/LAUNCH_ROADMAP.md`: status-update block after the intro; price row in §0; battery row; Dremel row; tape row + rows 15/16; 10/4 proxy row; 10/5 row; 10/10–11 row; 10/15 row; §3 battery rows; §3.3 labelled as pre-decision; API-key must-have; §5 item 1.
- `hardware/HANDOFF.md`: "not ordered (still true 10/6)"; fix-4 → fix-6 references; open items (bigger cell decided) + guide links + audit link.
- `hardware/FINAL_STATUS.md`: one status block under the title (not ordered, #1317, assembly-guide link, audit link). The one-page rewrite was not done.
- `hardware/enclosure/jigs/README.md`, `hardware/enclosure/final_assembly/battery_upgrade.md`: grind-zone cross-references.
- Not touched: `hardware/kicad/*`, firmware, both spreadsheets, `launch_roadmap_data.json`, `QUESTIONS_AND_ISSUES.md`.

## 6. Still to do (next session)

1. Rewrite `QUESTIONS_AND_ISSUES.md` ⭐ list per §4.
2. Shopping list: dates "ASAP", camera SKU 114993115 + $12.99/$11.99, add the 4 missing tool/consumable rows, "V4" note; Alibaba sheet battery envelope → 33.5 × 20.7 × 3.8 (#1317).
3. FINAL_STATUS §6 item 2 "fix 4" → "fix 6"; optional one-page rewrite.
4. `launch_roadmap_data.json` (#1570, 6 mm), `fitcheck/README.md` and regenerate `dummy_board.stl` with the #1317 block.

## Pass 2 (2026-10-06, 09:30–10:00 EDT)

Finishes §6 and adds the requirements trace. Read-only on `hardware/kicad/*`, `fab/`, `bom/`, firmware, `server/`, `hardware/enclosure/**`, `hardware/fitcheck/**`. Nothing committed. A first pass-2 agent was stopped mid-way; its edits (Q&I ⭐ rewrite, both spreadsheets, `launch_roadmap_data.json`, part of `launch_roadmap.html`, written 09:41–09:44) were checked line by line and kept: neither `QUESTIONS_AND_ISSUES.md` nor `FINAL_STATUS.md` was left half-edited.

**Requirements trace:** `verification/11_requirements_trace.md` (+ `tools/check_requirements_11.py`, `tools/kpcb_11.py`). 50 checks on the board file: **nothing MISSING**; 2 DIFFERENT, both off the fabricated layers (User.3 camera marker drawn Ø 6.0 though labelled 7 mm; stale title block "fx-300ES Plus transplant / 0.1-geometry"), plus leftover text (J4 value / BOM comment "Adafruit 1570", User.3 battery-bay note, keep-out zone names "key Abs"/"key x^3"). All for v15; v14 stays GO.

Changes, file by file:
- `hardware/QUESTIONS_AND_ISSUES.md`: ⭐ Start here = only the open items (paper dry fit + antenna tab, E7/stiffener, U1 stock; on arrival camera ribbon, magnet meter check, J4 polarity + E6; business items), "Closed on 2026-10-06" table with answer + source (17 rows, incl. the C46061768 "which end is N" closure and the trace). Pass 2 also: dry-fit item now points at `Claude outputs/paper_dry_fit_v14.svg` and the exact tab outline (x 114.4–118.9, y 89.3–104.7; the old text said y 90–104.5); Heights 3 tape wording "now 0.1 mm"; the history below is unchanged.
- `hardware/FINAL_STATUS.md`: rewritten as the entry point: status, "before you pay" (dry fit with the v14 template and antenna tab, E7, U1 stock), order steps, day-one sequence with the four artifact links (Power-Up, Assembly, Grinding, Roadmap), file index (adds 06/08/10/11, the dry-fit SVG, both spreadsheets, firmware review), decisions log (adds no LED, #1317, the C46061768 closure, antenna tab, v15 deferrals, hole spacings), open questions (points to ⭐), version history (+ 10/6 row). Fixed: "1–2 mm foam pad" under the battery → 0.1 mm tape, no foam; "fix 4" → §6; "LR44 cup isn't ground" → LR44 holder kept.
- `hardware/ORDER_WALKTHROUGH.md`, `hardware/ORDER_CHECKLIST.md`: dry-fit step uses `paper_dry_fit_v14.svg` with the antenna tab.
- `hardware/verification/06_final_review_pcb.md`: M1 gets the exact tab outline (note marked "added by 11").
- `Claude outputs/AI_Calculator_Shopping_List.xlsx` (stopped agent, verified): rows 15–19 added in the existing format (`=F*G` formulas, blue inputs, "This week"): 7 mm drill + 2–3 mm pilot + deburring tool $12, matte black vinyl $8, 99 % isopropyl $6, safety glasses + dust mask $10, USB-A 5 V charger / adapter $8; totals `=SUM(H5:H23)`, JLCPCB `=SUM(H5:H7)`. Recomputed in Python: **total $426.20** (JLCPCB part $210.00, everything else $216.20). Also: dates "ASAP (was Mon 10/5)", camera SKU 114993115 at $12.99 (DigiKey $11.99), the "V4" / SKU 12672 note, U1-stock note. No multimeter or calculators added.
- `Claude outputs/Alibaba_Bulk_Sourcing.xlsx` (stopped agent, verified; formulas kept): battery row = LiPo ~150 mAh, Adafruit #1317 class 26 × 19.75 × 3.8, max 33.5 × 20.7 × 3.8 incl. PCM; supplier rows, RFQ template and price note follow. Pass 2: "Checks before bulk" row 7 said "Red = + at J4 pin 1 side": wrong (pad 1 is GND) → "Red = + on J4 pad 2 (the "+" silk; pad 1 is GND)". Per-unit totals recomputed: $91.56 / $45.66 / $33.68 at 10 / 100 / 1,000.
- `Claude outputs/launch_roadmap_data.json` (stopped agent, verified, valid JSON): #1317 150 mAh, 7 mm hole after the ribbon is measured, "rotate nothing", status line.
- `Claude outputs/launch_roadmap.html`: **changed again in pass 2 → republish K5hMpGzzf4gS9b6nGKSoM9.** "Order Monday 10/5" heading → "Order first, as soon as the paper dry fit passes (was Monday 10/5; not ordered as of Tue 10/6)"; lede "what to order Monday" → "first (once the paper dry fit passes)"; risk row "Order Monday" → "as soon as the dry fit passes".
- `Claude outputs/LAUNCH_ROADMAP.md`: source [S1] #1570 → #1317 (Jameco #1570 prices kept as history); risks 2 and 6 ("Order Monday", "36 in stock") → "as soon as the dry fit passes", "35 in stock on 10/6".
- History banners / notes (no body edits): `stage2_electrical.md`, `stage3_schematic.md` (new banner: battery now #1317, RT9080, R20); `stage12_measurements.md` (camera window 7 mm, not 6); `REVIEW_independent_2026-10-03.md`, `verification/02_power_boot.md` (battery now #1317); `Claude outputs/AI_CALC_PCB_HANDOFF.md` (fx-115ES, 7 mm, #1317, RT9080).

Leftover scan (.md/.html outside the excluded folders) for #1570 / 100 mAh, "ordered 10/5", 6 mm camera drill, foam under the battery, "rotate parts", 75 % J1: every remaining hit is either history under a banner, a correct "rotate nothing / don't rotate J1" instruction, "75 %" print scaling in `geometry_assumptions.md`, a 6 mm **screw-post** hole, or the archived `Claude outputs/ai_calc_pcb_8/` snapshot (left as is).

**Still open (other owners):** `hardware/fitcheck/README.md` still names the #1570 (fitcheck owner; `dummy_board.stl` was regenerated at 09:57 by that agent, not checked here); `enclosure/final_assembly/assembly_report.md` §3 and `final_assembly/README.md` carry the 12 mm ribbon-spare model note (enclosure owner); v15 cosmetic board items listed in 11. **Republish** the roadmap artifact (K5hMpGzzf4gS9b6nGKSoM9) from `Claude outputs/launch_roadmap.html`; the Power-Up, Assembly and Grinding guide sources were not touched in pass 2.
