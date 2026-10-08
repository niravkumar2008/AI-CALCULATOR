# AI Calculator board: final status (start here)

Rewritten 2026-10-06 (board unchanged since stage 14, 2026-10-04). **This is the single entry point.** If another doc disagrees with this one, this one (and the stage-14 / verification docs it links) wins. The short open list lives in `QUESTIONS_AND_ISSUES.md` → "⭐ Start here".

## Status

- **Board v14 = GO to order, unchanged.** Silk "AI CALC v14 2026-10-04". DRC 0 errors / 0 warnings / 0 unconnected / 0 schematic-parity issues, ERC 0, every copper item ≥ 0.25 mm from every hole.
- **Reviews:** re-check of stage 14 `verification/05_stage14_recheck.md` (GO); final review `verification/06_final_review_pcb.md` and independent second opinion `verification/08_second_opinion_pcb.md` (2026-10-06: no FATAL, no SERIOUS; pads vs netlist, datasheets, CPL rotations vs EasyEDA 77/77, connector pin orders, strapping pins, power budget, keypad, holes vs calipers); requirements trace `verification/11_requirements_trace.md` (every decision checked on the board file: nothing missing). Firmware: `../firmware/FINAL_REVIEW_firmware.md` (the board firmware is `firmware-prototype/`; `firmware/` is the breadboard tester, never flash it to the board). Docs audit: `verification/10_docs_consistency.md`.
- **Not ordered yet** (as of Tue 2026-10-06; the Mon 10/5 order didn't happen). Boards land about **10–14 days** after the order, door-to-door (about 10/17–10/21 for an order on 10/7; longer with economy shipping, 7–15 days of shipping alone); every day it slips moves the roadmap dates by a day. Why JLCPCB and not another fab: `FAB_VENDOR_COMPARISON.md`.
- **Fab files** in `fab/`, zipped in `Claude outputs/ai_calc_pcb_14.zip`. The CPL is already rotation-corrected for JLCPCB (`tools/jlc_cpl.py`), so in JLCPCB's preview you **only verify, never rotate**.
- **Battery:** Adafruit **#1317, 150 mAh** (26 × 19.75 × 3.8 mm, decided 10/6, `enclosure/final_assembly/battery_upgrade.md`), lying flat on the back-cover floor.

## 1. Before you pay JLCPCB (3 things)

**a. Paper dry fit, with the antenna tab** (Nirav, when he's back). The only check that could still change the board.
1. Open `Claude outputs/paper_dry_fit_v14.svg` in a browser and print it at **100 % / "actual size"** (not "fit to page").
2. Measure the printed **50 mm line** with the calipers. It must read 50 mm; if not, fix the print scaling and print again.
3. Cut out the board outline **together with the yellow antenna tab** on the left edge (4.5 × 15.4 mm, KiCad x 114.4–118.9, y 89.3–104.7: the ESP32 antenna is a bare 0.8 mm PCB tab that sticks out 4.15 mm past the board edge). Cut or punch out the post holes.
4. Lay it in the **front shell** (the half with the keys), printed side as drawn, pressed flat onto the posts.
5. **Every post must sit inside its hole** (H2, H3, H6, H9, H13, H14 and the screw posts) without pushing the paper sideways. **H6 has the least play** (about 0.10 mm), so check it first; if H6 pushes slightly, that's OK (the plan is to file it a little towards the top of the board).
6. **The antenna tab must clear the side wall.** On the calculator this is the **right-hand side as you use it** (the solar-window side: the map lies in the shell mirror-image to the front view). That wall is an outer skin, a wire channel and a thin inner rib; if the rib reaches the board plane the tab fouls it by about 0.4 mm and the board's own top corner (x 114.3–114.8, y 72–83, which was never narrowed) by up to 0.85 mm. If either rubs, the fix is grinding-guide **zone 6** (file the inner rib away 15–50 mm from the top), not the board.
7. Any post off by more than about 0.5 mm → **stop, don't order**, ask Claude for a hole move.
8. Ignore any "LR44 cup" grind note: the LR44 holder stays; the battery lies on the back-cover floor.
9. **Then the 10-minute pre-grind checklist** in `verification/07_final_review_fitment.md` section 3: depth of that inner rib, the strip between the solar window and the display window (≥ 2.6 mm: the e-paper's top edge lies there), floor thickness at the moulded label on the back, the back-cover lip at the magnet notch, screw length.

(`fitcheck/grind_map_front_shell.svg`, regenerated 2026-10-06, is the same template with the yellow tab, the grinding-guide zone numbers and the LR44 cup marked KEEP; either print works.)

**b. E7 / e-paper stiffener:** glass edge → ribbon tip, and the length of the stiff end. Stiff end ≤ 5 mm: fine. More than 5 mm: tell Claude before ordering (rule: `stage14_verification_fixes.md` §6; J2 moves +1 mm only if E7 ≥ 15.3). Push the ribbon fully home either way (review 07: the CAD path needs 12.7 of the 14.3 mm, about 1.6 mm nominal spare, 0.3 worst case: build as if there were none).

**c. JLCPCB stock of U1, ESP32-S3-MINI-1-N4R2 (C3013941)**, at the BOM upload step. If it's out of stock, order the bare PCBs and ask for a pre-order of C3013941. **Accept no substitute** (an N8R8 loses IO33–37, which the board uses).

## 2. Order steps

Follow **`ORDER_WALKTHROUGH.md`** step by step (one-page version: `ORDER_CHECKLIST.md`).
- Upload `fab/ai_calc_gerbers_JLCPCB.zip`, `fab/ai_calc_BOM_JLCPCB.csv` and `fab/ai_calc_CPL_JLCPCB.csv` exactly as they are.
- 2 layers, qty 5, 0.8 mm, ENIG; Standard PCBA, top side, qty 2, "confirm parts placement" yes.
- Placement preview: **don't rotate anything.** Compare each part with the table in walkthrough step 5. If something disagrees, stop and ask before clicking anything.
- Paste the order note (walkthrough step 6) and answer the engineers' emails within a day (step 7).
- Expected cost: about **$135–155 before shipping**, about **$200–240 delivered** ($20–40 express shipping + US import duty about 35–37.5 % collected at checkout (the de-minimis exemption has been suspended since 2026-06-24)). Much more means something is mis-ticked.
- Order the Adafruit parts (#5412 cable first, only ~35 in stock), cameras and e-paper panels the same day: `Claude outputs/AI_Calculator_Shopping_List.xlsx`.

## 3. Day-one sequence (when the parts arrive)

**Start with `ARRIVAL_CHECKLIST.md`**: what arrives from whom, what to check on each box, and this sequence with links to the exact guide sections.

Guides (published): **Power-Up** https://claude.ai/artifact/PqTARcDutvwxS5aGB8iEG9 (source `bringup_guide.html`) · **Calculator Assembly** https://claude.ai/artifact/8BTC57FZ3JFwiDdiLprwtY (source `enclosure/final_assembly/assembly_guide.html`) · **Grinding** https://claude.ai/artifact/9rCWyw8MeHP2z8FWijnyfL (source `enclosure/final_assembly/grinding_manual.html`) · **Roadmap** https://claude.ai/artifact/K5hMpGzzf4gS9b6nGKSoM9.

1. **Before the boards:** take the Casio apart (Assembly 1). Grind (Grinding guide; Assembly 2): **solar box** flat to the floor, **rib B** flat for **14 mm** over the camera, **magnet U-notch** in the top wall (21.5 × 7.95 mm, 6.4–27.9 mm from the right side edge seen from the front), **5b**, the back-cover lip at the notch, **only if your cover has one**, and **6**, 35 mm of the faceplate's inner side-wall rib beside the board's top corner and the ESP32 antenna (15–50 mm from the top, solar-window side), **only if the paper template won't drop in there**. **Don't grind the LR44 holder.** Fitment review and full CAD clearance table: `verification/07_final_review_fitment.md`. Never grind into the floor; keep at least 0.8 mm. **Hold the 7 mm camera hole** until the ribbon is measured. Make the window mask (Assembly 5, `enclosure/final_assembly/window_mask.md`). **Do the 10-minute pre-grind checklist first** (`verification/07_final_review_fitment.md` §3). **Put the AI server online** (Power-Up 10, `server/proxy/README.md`): the Claude API key goes only in the host's environment settings, never in the calculator or git; make one device token per calculator (`python admin.py new-device "calc 1"`).
2. **Boards arrive:** inspect unpowered (Power-Up 2).
3. **Magnet piece meter check, before gluing** (Power-Up 3 = `ORDER_WALKTHROUGH.md` step 9), with the real #5412 cable. J3 is **1 VBUS, 2 D−, 3 D+, 4 GND**, silk "N"/"+" at pin 1, "−" at pin 4. Find the +5 V leg with the meter and put it in pin 1; never trust the N/S marks alone. (J3 itself, C46061768, is a plain symmetric socket: the "N" end is set by how you plug the Adafruit #5358 piece in.)
4. **J4 battery polarity** (Power-Up 4): red wire on J4 "+" (pad 2). If reversed, swap the crimp pins (lift the tabs with a needle). Q1 protects the board meanwhile.
5. **Camera ribbon** (Power-Up 5): beep test, fingers 2 and 15 to GND (E6), and measure module far edge → tip. Under about 70.3 mm: tape the module up to 1.7 mm towards J1. Then drill the **7 mm window** (9/32" OK) over the lens where it actually sits (nominal KiCad (150.0, 95.1)). Only about 1.4 mm of ribbon spare is expected.
6. **First power, computer USB only:** VBUS, SYS, TP5 3.3 V, TP7 EN, TP6 BAT (Power-Up 6).
7. **Flash** (Power-Up 7). The first flash of every board is **over USB** (two-slot OTA partition table; an over-the-air update can't write it); `status` then shows `slot app0`. **Recovery** if flashing ever fails: hold **TP1 (BOOT) to TP4 (GND)**, tap **TP7 (EN)** to GND, then flash (or unplug the battery first). UART backup on **TP2/TP3**. Never burn the security or USB-disable eFuses.
8. `selftest`, save the JSON (Power-Up 8; fields incl. `firmware_slot`, `battery_ok`, `camera_autofocus`). Then e-paper → selftest; camera → selftest; battery → charging (Power-Up 9).
   - **AI settings** (Power-Up 10): `wifi`, `proxy`, `token` in the Serial Monitor, or the phone page (`setup`; on a finished calculator SHIFT MODE, 6, =); `pair` → `<server>/link`. Below 3.6 V on battery the screen says "Battery low: plug in the cable to use AI solve" (camera + Wi-Fi off until 3.7 V or a cable).
9. **Hand over** (Power-Up 11 → Assembly 3 "must pass" list, battery unplugged).
10. Assembly 4–8: prepare; e-paper + mask; camera (0.1 mm double-sided tape, **no foam**; Kapton under it); magnet into J3; key mat, board, key test on cable power, glue the magnet.
11. Assembly 9–10: **battery** flat on the back-cover floor, held with **0.1 mm tape, no foam** (only about 0.3 mm above it), kept off TP1/TP4/TP7; close.
12. Assembly 11: self-test by key combo (SHIFT + ALPHA held, ON), preview, phone setup from the keys (SHIFT MODE, 6, =), AI solve on battery, charging, overnight memory.
13. End-of-line QA on the closed unit: `production/QA_TEST_PLAN.md` Part B (log the JSON incl. `firmware_slot`).

## 4. Files

| File | Purpose |
|---|---|
| `hardware/FINAL_STATUS.md` | This page: status, what's left, decisions, history |
| `hardware/QUESTIONS_AND_ISSUES.md` | "⭐ Start here" = the short open list + closed answers; below it the full Q&A history |
| `hardware/ARRIVAL_CHECKLIST.md` | One page for the day the boxes arrive: who ships what, package checks, numbered day-one sequence with guide links |
| `hardware/FAB_VENDOR_COMPARISON.md` | JLCPCB vs other fabs (verdict: stay with JLCPCB), delivered cost and door-to-door time |
| `hardware/production/QA_TEST_PLAN.md` | Incoming inspection (Part A) and end-of-line test (Part B) |
| `server/proxy/README.md` | The AI server: deploy, device tokens, pairing, over-the-air firmware |
| `firmware-prototype/README.md` | Board firmware: build, USB flash, recovery, phone setup, OTA, serial commands |
| `hardware/ORDER_WALKTHROUGH.md` / `ORDER_CHECKLIST.md` | Click-by-click JLCPCB order, preview table, engineer answers, meter checks / one-page version |
| `Claude outputs/paper_dry_fit_v14.svg` | Print-at-100 % dry-fit template with the antenna tab and 50 mm check line |
| `hardware/HANDOFF.md` | Context for a future Claude session (state, must-not-undo list, lessons) |
| `hardware/verification/05_stage14_recheck.md`, `06_final_review_pcb.md`, `08_second_opinion_pcb.md` | The GO reviews of v14 |
| `hardware/verification/11_requirements_trace.md` | Every past decision checked against the board file (+ `tools/check_requirements_11.py`) |
| `hardware/verification/10_docs_consistency.md` | Docs audit (pass 1 + pass 2) |
| `hardware/verification/01–04_*.md` | The 4-part verification of stage 13b that found the J3 and CPL problems (fixed in stage 14) |
| `hardware/stage14_verification_fixes.md` | What stage 14 changed; ribbon decision rules (§6), recovery (§8) |
| `hardware/stage13_heights.md` | Height stack-up, grind list (§6), magnet slot, camera mounting (partly superseded: see its banner) |
| `hardware/fab/` | Upload files (gerbers zip, BOM, CPL), renders, assembly PDF, board STEP |
| `Claude outputs/ai_calc_pcb_14.zip` | Zip of the v14 project and outputs |
| `hardware/kicad/` | KiCad 10 project (board, schematic sheets, `ai_calc.pretty`) |
| `hardware/tools/make_outputs.sh`, `jlc_cpl.py` | Regenerate `fab/` (BOM first, then the CPL with JLCPCB rotation offsets per LCSC number) |
| `hardware/tools/check_cpl_easyeda.py`, `check_hole_clearance.py` | Re-run checks: CPL vs EasyEDA footprints; copper-to-hole clearance |
| `hardware/pins_final.h` | Source of truth for the GPIO map, J3 order, recovery notes (`firmware-prototype/src/pins.h` includes it) |
| `hardware/fitcheck/` | Paper grind maps (front shell / back cover), clearance table, dummy-board STL |
| `hardware/bringup_guide.html` | Power-Up guide source |
| `hardware/enclosure/final_assembly/` | Whole-calculator Fusion model, `assembly_report.md`, assembly and grinding guide sources, `battery_upgrade.md`, `window_mask.md`, `slide_case_mods.md` |
| `hardware/enclosure/replica/` | Unbranded fx-115ES replica CAD from the calipers |
| `hardware/measurements_2026-10-03.md` | Nirav's caliper readings (raw) |
| `hardware/bom/ai_calc_BOM_JLCPCB.csv` | Copy of the BOM (same as `fab/`) |
| `firmware/FIRMWARE_STAGE13.md`, `firmware/FINAL_REVIEW_firmware.md` | Stage-13 firmware notes (history; superseded by `firmware-prototype/README.md`) and the final firmware review with the 10/6 fixes (OTA, phone setup, low-battery gate) |
| `Claude outputs/AI_Calculator_Shopping_List.xlsx` | Prototype shopping list (parts, tools, consumables, totals) |
| `Claude outputs/Alibaba_Bulk_Sourcing.xlsx` | Bulk cost per unit at 10 / 100 / 1,000 |
| `Claude outputs/LAUNCH_ROADMAP.md` (+ `launch_roadmap.html`) | Timeline, costs and business plan |
| `Claude outputs/COMPETITOR_ANALYSIS.md` | VovoCorp and other competitors (pricing question) |

## 5. Decisions log (key decisions and why)

| Decision | Why | Stage |
|---|---|---|
| fx-115ES shell (not fx-300ES Plus) | Nirav's chosen donor; holes re-derived from photos and calipers | 10 |
| Board in an unmodified-looking shell, only grinds and one 7 mm hole | Keeps the calculator look; no soldering for Nirav | 1, 13 |
| JLCPCB, 2 layers, 0.8 mm, ENIG, 5 PCBs / 2 assembled | 0.8 mm for the key stack; ENIG because HASL is too bumpy for the rubber keys | 1, 7 |
| No LED | Exam mode; Nirav doesn't want a light | 2 |
| Camera GPIO map re-ordered (XCLK 18, D0 13, D1 11, D2 10, D3 12, D4 14, D5 16, D6 17, D7 21, VSYNC 36, HREF 47, PCLK 15, PWDN 48, RESET 38, SIOD 40, SIOC 39, PWR_EN 34) | The bus leaves the module in J1's pin order, so routing works on 2 layers | 6 |
| J1 = `…_CamReversed` (mirrored pad numbers) | A flat ribbon going straight in lands mirrored (about 90 % confidence after verification 01/03/06; a 180° cable twist is the fallback); JLCPCB must not rotate J1 | 6 |
| E-paper FPC through a 1.0 × 14 mm slot (x 172.9) to J2 | Panel on the key side, connector on the component side; 14.3 mm ribbon budget | 4/5, 11 |
| U3 = RT9080-33GJ5 (not AP2112K) | 2 µA vs 55 µA quiescent: standby about 4–6 months instead of 6 weeks, and cheaper | 11 |
| R20 pulls STAT up to VBUS_SENSE | Nothing back-feeds the charger or the magnet contacts without a cable | 11 |
| Magnet contacts never carry battery voltage (D1, Q2, charger) | Exposed contacts on the outside of the case | 2 |
| Charge current 50 mA (R2 20 k) | 150 mAh cell (#1317), 0.33 C; v15 with a bigger cell: R2 → 4.7 k | 2 |
| Board narrowed past the wall pins (x 118.9–183.65); ESP32 antenna overhangs the left edge by ~4.2 mm | C4/C14 wall readings; Espressif's preferred antenna placement | 12 |
| Mount holes: H1/H3 Ø 6.0 screw posts (46.5 apart, C8), H8–H10 42.4, H2/H9/H13/H14 slotted, H6 moved to y 186.99 | Fit both the photo and the caliper post positions | 12, 13 |
| Grind solar box and rib B rather than change parts | J4 (5.5) and the camera (5.4) fit under 6.0 mm after grinding | 13 |
| J3 = right-angle SMD socket C46061768 (option A), magnet glued in a top-wall U-notch | The magnet face is taller than the space; no soldering | 13b |
| C33/C34 22 µF on +3V3 | Wi-Fi brown-out margin on a small cell | 13b |
| J3 = 1 VBUS, 2 D−, 3 D+, 4 GND, N end at pin 1 | Matches the #5412 cable when mated; power on the outer pins, so a backwards fit never puts 5 V on a data line | 14 |
| CPL rotations fixed in `tools/jlc_cpl.py` (Q1 180, Q2 270, U2/U3/U7 270, U4/U5 180, J3 180) | JLCPCB places with EasyEDA footprints, whose zero angles differ from KiCad's | 14 |
| SW1 bar notched (`KeyPad_6.0x4.5_H3notch`) | SHIFT pad was 0.055 mm from H3; now ≥ 0.25 mm | 14 |
| J1 and J2 not moved | Moving J1 only helps a short ribbon; moving J2 breaks the 14.3 = 14.3 ribbon budget. Decision rules in stage 14 §6 | 14 |
| R17 stays 100 k (C25741) | Schematic, BOM and board agree; the "10 k" in verification 02 / the brief was wrong | 14 |
| Key row 2 = CALC and ∫dx (fx-115ES); Abs = SHIFT hyp, x³ = SHIFT x², ∛ = SHIFT √ | Matches the real fx-115ES key mat | 14 |
| Recovery = TP1 to TP4, tap TP7 | Plugging the cable in doesn't reset the chip while the battery is connected | 14 |
| Battery on the back-cover floor; LR44 holder kept (not ground); 0.1 mm tape, no foam | In the stage-13 bay it hits the solar-window frame and its leads can't reach J4; only ~0.3 mm above the cell | final assembly |
| Battery = Adafruit #1317, 150 mAh (was #1570, 100 mAh) | 50 % more capacity, same price and plug, fits with no new grind | 10/6 |
| C46061768 "which end is N" closed | It is a plain symmetric socket; N is set by the plugged-in #5358 piece and the meter check. No CPL change | 10/6 (08) |
| Antenna tab added to the paper dry fit | The overhang wasn't on the old map (06 M1) | 10/6 (06) |
| v14 ordered unchanged; small items deferred to v15 (antenna keep-outs joined, pin-1 silk ≥ 0.15 mm, R5 → 15 k, Ø 7 marker and title block) | No FATAL or SERIOUS finding; none of these affect function | 10/6 (06, 11) |

## 6. Open questions

The current list is `QUESTIONS_AND_ISSUES.md` → "⭐ Start here". In short: before paying, the paper dry fit with the antenna tab, E7 / stiffener, U1 stock; when parts arrive, camera ribbon length, magnet meter check, J4 polarity + E6; business (pricing vs VovoCorp, proxy deploy and real sign-in) doesn't block the order.

## 7. Version history

| Stage | One line |
|---|---|
| 1 | Board geometry from the fx-300ES scan (`ai_calc_pcb_1/2`) |
| 2 | Pin map and power design (`pins_final.h`, `stage2_electrical.md`) |
| 3 | Schematic, ERC 0, live-checked LCSC numbers, review fixes |
| 4/5 | Footprints and placement, 0 overlaps |
| 6 | Routing; camera GPIO map re-ordered to J1's pin order |
| 7 | `fab/` outputs via `make_outputs.sh`; first ORDER_CHECKLIST |
| 8 | v8 board: camera bus re-done for the 0.6 mm ribbon |
| 9 | v8 finished: DRC 0/0/0 with parity (`stage9_finish.md`) |
| 10 | Moved to the fx-115ES shell: holes from photos, rib/ring map (`stage10_fx115es.md`) |
| 11 | E-paper at the measured window, 4.2 mm holes, RT9080, R20 → VBUS_SENSE, TP5–TP7 |
| 12 | Second caliper batch: narrowed screen section, fiducials, height table, grind map |
| 13 | Heights: grind plan, 7 mm camera window, slotted holes; 13b: J3 right-angle header, C33/C34 |
| 14 | Verification fixes: J3 = VBUS/D−/D+/GND, CPL rotation tool, SW1/H3 notch, recovery, CALC/∫dx keys; independent re-check GO |
| 10/6 | No board change: final review + second opinion (GO), requirements trace, battery #1317, antenna tab on the dry fit, docs cleanup |
