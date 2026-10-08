# Questions and issues for Nirav

Started 2026-10-03 (overnight session). Each item says what I assumed in the meantime, so nothing is blocked while you sleep. Answer them in any order. Items once marked **(blocks order)** have all been answered; only the "⭐ Start here" list below is current.

## ⭐ Start here (rewritten 2026-10-06 after the final review)
**Board v14 is GO to order, unchanged** (`verification/06_final_review_pcb.md` + second opinion `verification/08_second_opinion_pcb.md`, 2026-10-06: no FATAL, no SERIOUS; earlier re-check `verification/05_stage14_recheck.md`): DRC 0/0/0 with parity, ERC 0. **Not ordered yet** as of Tue 10/6. Status, decisions and history: **`FINAL_STATUS.md`**. Ordering: `ORDER_WALKTHROUGH.md`. This is the whole list of what is really open:

**Before paying JLCPCB**
1. **Paper dry fit, now with the antenna tab** (Nirav, when he's back): print **`Claude outputs/paper_dry_fit_v14.svg`** at 100 %, check the 50 mm line, cut it out **with the yellow antenna tab**, lay it in the front shell: every post must sit inside its hole. H6 has the least play (about 0.10 mm). Off by more than about 0.5 mm → stop and ask. **New (review 06 M1):** the ESP32 antenna sticks out about 4.2 mm past the board's left edge; the tab (4.5 × 15.4 mm, KiCad x 114.4–118.9, y 89.3–104.7) must clear the side wall (if it rubs, the fix is filing the plastic, not the board). **Review 07 (2026-10-06):** the tab is now printed on `fitcheck/grind_map_front_shell.svg`; the map's left edge is the calculator's right-hand side in use (solar-window side); if the tab or the board's top corner beside it fouls the wall's thin inner rib, that is grinding-guide zone 6 (35 mm of the rib, 15–50 mm from the top). Then do the 10-minute pre-grind checklist in `verification/07_final_review_fitment.md` §3 before any grinding. Steps: `FINAL_STATUS.md` §1a.
2. **E7 / e-paper stiffener:** glass edge → ribbon tip, and the length of the stiff end. Stiff end ≤ 5 mm: fine as is. More than 5 mm: tell Claude before ordering (rule: `stage14_verification_fixes.md` §6; J2 moves +1 mm only if E7 ≥ 15.3). Settled by `verification/07_final_review_fitment.md` M2: about 1.6 mm nominal spare, about 0.3 mm worst case; build as if there were none and push the ribbon fully home.
3. **JLCPCB stock of U1, ESP32-S3-MINI-1-N4R2 (C3013941)**, at the BOM upload step. If it is out of stock, order the bare PCBs and ask for a pre-order of C3013941; **accept no substitute** (an N8R8 loses IO33–37, which the board uses).

**When the parts arrive** (no board change either way; start at `ARRIVAL_CHECKLIST.md`, details in the Power-Up guide https://claude.ai/artifact/PqTARcDutvwxS5aGB8iEG9)
4. **Camera ribbon length:** module far edge → tip. Under about 70.3 mm: tape the module up to 1.7 mm towards J1 and drill the 7 mm window over the lens where it actually sits. Only about 1.4 mm of spare is expected.
5. **Magnet meter check before gluing** (mandatory): J3 = 1 VBUS, 2 D−, 3 D+, 4 GND. Find the +5 V leg of the mated #5412 cable with the meter and put it in pin 1 (board silk "N"/"+"); never trust the N/S marks alone.
6. **J4 battery polarity** (meter: red wire on "+"; if reversed, swap the crimp pins) and **E6**, the camera-ribbon beep test (fingers 2 and 15 to GND).

**Business / later (nothing blocks the order)**
- Pricing vs VovoCorp (`Claude outputs/COMPETITOR_ANALYSIS.md`); plan is $225 + $15/month, first month free.
- Proxy: deploy it (Render or similar; `server/proxy/README.md`) **before Power-Up step 10**, which puts its https address and a device token into each calculator (API key only in the host's environment settings). Real sign-in on the link page before selling (Firmware 1–2 below; `server/proxy/FINAL_REVIEW_server.md` S3).
- Donor edition check (Business 8), CAD photo requests (side profile, slide case) only matter for the replica model.

### Closed on 2026-10-06 (answer and source)
| Question | Answer | Source |
|---|---|---|
| Which end of the JLC-placed magnet socket C46061768 is "N"? | Not a question any more: C46061768 is a plain symmetric 4-pin 2.54 mm right-angle socket with no magnet. The "N" end belongs to the Adafruit #5358 piece Nirav plugs in; it is set by the board's "N" silk at pin 1 and the meter check (item 5). No CPL change. | `verification/08_second_opinion_pcb.md` "Resolution of open item (a)"; `06_final_review_pcb.md` check 8 |
| Is the board still GO after a from-scratch review? | Yes, v14 unchanged: pads vs netlist 0 mismatches, every IC pin vs datasheet, CPL rotations vs EasyEDA 77/77, J1–J4 pin orders, strapping pins, power tree, keypad matrix, holes vs calipers. Minor only: antenna tab (item 1), 1.25 mm of GND under the antenna root (v15), 0.06 mm pin-1 silk (cosmetic). | `verification/06_final_review_pcb.md`, `08_second_opinion_pcb.md` |
| Does the firmware block bring-up? | No. `firmware-prototype/` (the board firmware; `firmware/` is the breadboard tester, never flash it to the board) builds clean, all pins match the schematic. Its fixes were applied the same day (OTA with rollback, phone setup page via SETUP 6, battery gate for camera + Wi-Fi, EPD BUSY pull-down, `firmware_slot` in status); the first flash of each board must be over USB. | `../firmware/FINAL_REVIEW_firmware.md` |
| A bigger cell? | Decided: **Adafruit #1317, 150 mAh, 26 × 19.75 × 3.8**, same price and plug, lies on the back-cover floor, no new grind. Charge current stays 50 mA (0.33 C). | `enclosure/final_assembly/battery_upgrade.md` §8; FINAL_STATUS decisions log |
| Camera-ribbon spare: 12 mm or 1.4 mm? | About **1.4 mm** (the 70.5 mm on the drawing includes the 8.5 mm module). The 12 mm in the old model notes is wrong. | `stage14_verification_fixes.md` §6; `06_final_review_pcb.md` N2 |
| Tape under the camera and battery | **0.1 mm double-sided tape, no foam** (0.3 mm above the battery, 0.5 mm above the lens); e-paper on about 0.15 mm. The "≤ 0.2 mm" in Heights 3 below is stage-13 wording. | assembly guide; `battery_upgrade.md` §8 |
| Camera window size | **7 mm** (9/32" OK) at KiCad (150, 95.1), drilled after the ribbon is measured. "6 mm" in older docs is stale. | `stage13_heights.md`; grinding guide |
| Stage-14 "fix 4" references | The ribbon rules (camera and e-paper) are **§6** of `stage14_verification_fixes.md`; §4 is recovery/R17. | `verification/10_docs_consistency.md` #7 |
| JLCPCB preview: rotate parts? | **No.** The CPL in `fab/` is rotation-corrected (`tools/jlc_cpl.py`); verify against `ORDER_WALKTHROUGH.md` step 5, rotate and move nothing. | `verification/05`, `08` §1 |
| Does the calculator work without the subscription? | Yes (roadmap §0): it is a normal scientific calculator; only AI SOLVE needs the subscription. | `Claude outputs/LAUNCH_ROADMAP.md` §0 |
| Proxy built? Factory self-test exists? | Both done: `server/proxy/` (FastAPI, device tokens, TRIAL_DAYS=30, MONTHLY_CAP=500); self-test = SHIFT + ALPHA held, ON, or `selftest` on serial. Switches `ROUTING`, `FREE_DAILY`, `REFUSAL_FALLBACK` exist with the defaults assumed in Firmware 3–5 below. | `firmware/FIRMWARE_STAGE13.md`; `verification/10_docs_consistency.md` #12–13 |
| Order date | The Mon 10/5 order did not happen. Boards land about 10–14 days after the order (about 10/17–10/21 for an order on 10/7; `FAB_VENDOR_COMPARISON.md`); later roadmap dates slip by the same amount. | `verification/10_docs_consistency.md` §0 |
| Battery position / LR44 cup | On the back-cover floor; the LR44 cup is **not** ground. | `enclosure/final_assembly/assembly_report.md` |
| Recovery | Hold TP1 (BOOT) to TP4 (GND), tap TP7 (EN), flash. UART backup TP2/TP3. Never burn eFuses. | `stage14_verification_fixes.md` §8; `pins_final.h` |
| Waveshare "V4" | Buy raw panel SKU 12672; the page may not print "V4" (that is the HAT's name). Same panel: 250 × 122, 59.2 × 29.2 × 1.05, SSD1680. | `verification/10_docs_consistency.md` #19 |
| R17 10 k or 100 k? | 100 k (schematic, BOM C25741, board, `pins_final.h` agree). | `stage14_verification_fixes.md` §6 (R17 note) |
| Is every decision we ever made actually on the v14 board? | Yes: 50 checks run on the board file (pin map, connectors, power parts, holes, slot, antenna, silk, BOM), nothing missing. Only cosmetic leftovers on non-fabricated layers (camera marker drawn Ø 6 on User.3, old title block, J4 value text "Adafruit 1570"). | `verification/11_requirements_trace.md` |

Everything below is the full history; ✅ = answered, ~~struck~~ = superseded.

## Measurements

1. ✅ **Answered (C8 46.5 / 42.3, stage 12).** **C8: "61.8 outside to outside".** Which post pair is that? From your photos the pairs are about 46.4 (beside SHIFT/ON), 42.3 (under row 2), 38.3 and 38.6 mm apart, centre to centre. 61.8 − 3.85 = 57.95 doesn't match any of them. *Assumed: holes stay at the photo-measured positions.*
2. ✅ **Answered (C9 68.1, posts clear of board).** **C9: "3.7 outside to outside / 49.65".** Is 3.7 the post's width and 49.65 the outside-to-outside distance between the two top-corner posts? That would make them about 45.8 mm apart, centre to centre. The photos suggested about 62.7. *Assumed: photo positions kept.*
3. ✅ **Answered (stubs stop above the board; stage 12 narrowed the board anyway).** **C4: wall thickness 4.7 / 5.5.** That's thick for a side wall. Was it measured at the screen section where the round stubs are? *Assumed: 4.7 is the stub zone and the bare wall is thinner.*
4. ✅ **Mostly answered (C14, C15, D1–D13 in); E6/E7 still open.** **Still to measure :** C14 (wall stubs), C15 (post distances from the top edge), depths D1–D4, D11, D13, the ribbon checks E6/E7 and, if you can, the F6 flatbed scan (the best single source of exact positions).

## Electrical (from the independent review, `REVIEW_independent_2026-10-03.md`)

5. **R20 leakage fix.** R20 leaked about 20 µA into the charge contacts when no cable is in. The PCB worker is moving its pull-up to VBUS_SENSE (or leaving R20 off the board). Nothing for you to do, just know it changed.
6. **Regulator swap, AP2112K → RT9080.** Standby goes from about 6 weeks to about 4–6 months. Same footprint. Say if you'd rather keep the AP2112K.
7. **Battery polarity.** Before ordering, check the Adafruit battery's red wire against the J4 + mark (sheet item E3). Reversed JST leads are common.

## Business (from the roadmap)

8. **Calculator edition.** Are your two calculators the same edition (fx-115ES, **PLUS** or **PLUS 2nd edition**) as the one you measured? The insides can differ, so it's worth a quick look at the back labels.
9. **Age.** If you're under 18, a parent needs to be the account holder for the LLC, Stripe and preorder platforms. Worth sorting out early.
10. **Subscription.** Should the calculator work as a normal scientific calculator without the $15 subscription? I recommend yes.


## CAD replica
(fx-115ES shell replica, `enclosure/build_fx115es_replica.py`, details in `enclosure/replica/README.md`)

1. ~~**Width (C2 = 79.3):** is that the front shell or the back cover?~~ **Answered (2026-10-03):** C2 = front shell at the screen section, C3 = back cover, both 79.3. The model now has straight 79.3 sides and the front shell 77.4 in the keypad section. Original question: The photos show the navy back cover wrapping round the front shell along the keypad section, so the model makes the back cover 79.3 wide and the front shell ~1.8 mm narrower there (78.4 max). If the front shell itself is 79.3, say so and the tray walls get thinner.
2. **Partly answered:** total thickness is now known (D10: 11.8 top / 11.3 keypad; board at D1/D4). Still open: the side photo, for the parting line (now guessed 3.5), tray-wall height (9.8) and round-overs. Original question: **Profile from the side:** no side-on photo. Please take one straight-on photo of the long side (with a ruler). It decides the parting-line height (guessed 2.6 mm from the back), how high the back cover's tray walls come up (guessed 11.3 mm), and the edge round-overs (guessed 2.0 front / 1.2 back).
3. **Still open (face plate now 1.3, checked against D3):** **Key height:** how far do the keys stick up above the face (guessed 1.5 mm)? And the face-plate thickness.
4. **Battery:** the back shows a round LR44 hole in a shallow rectangular recess. Is there a lid or sticker over it? Size of the recess? (guessed 16 x 15 mm, 0.6 mm deep, thin lid modelled).
5. **7th hole:** the back has a small hole in the centre over the small ring (photo e82c982d). Screw or something else? (modelled as a plain 2 mm hole.)
6. **Feet:** where are the rubber feet and how big? (guessed four 7 mm round feet, now 0.3 mm proud.)
7. ~~**Corner screw posts:**~~ **Answered (2026-10-03):** C9 (68.1 c-c) + C15 (8.9 row) put them at (±34.05, 74.05); with a 1.4 skin C11 then checks out at 4.20 / 6.02. They are outside our board. Original note: with C11 (4.2 / 6.1 from the inside walls) the model puts them at (±32.7, 73.2) front-view mm, which agrees with the photo estimate within ~1 mm. Fine for the PCB's cut-outs, but check H-hole clearance once the board is final.
8. **Slide hard cover:** not measured at all; the model's cover is a placeholder (plain lid with side walls). Calipers on the real cover (length, width, height, wall thickness, how it grips) would fix it.
9. **Fixed (2026-10-03):** the json is now one continuous loop (450 points; `build_case.fx115_outline()` parses it unchanged). Original note: `enclosure/fx115es_outline.json` stores the outline as 3 runs out of order. `build_fx115es_replica.py` re-chains them; `build_case.py` reads the file as-is, which only works because Fusion builds profiles from the crossing lines. Worth fixing in the json or in `build_case.fx115_outline()`.
10. **(new) C15 reading convention.** "From the outside of the calculator to the outside of the holes": I read it as case top edge to the *far* edge of the hole inside each post (pilot Ø1.7 / bore ~Ø1.2). That fits the photos to ≤ 1.05 mm; reading it as post centres gives up to 1.65 mm. Is that what you measured? It moves H2/H9 1.0–1.2 mm lower than the board has them.
11. **(new) Row 3 (H8/H10, ~80 mm down)** was not in C15. Please read it the same way.
12. ~~**(new) D4: to the outside of the back, or to the inside of the floor?**~~ **Answered (2026-10-03):** D4 = 6.0 from the board to the back cover's *inside* face; D1 = 5.5 from the front shell's *rim* down to the board. Stack 1.0 + 6.0 + 0.8 + 4.0 = 11.8 (3.5 front side at the keys = 11.3). Replica rev B2 uses it; the camera and J4 now clear the floor (they still hit the solar box / rib B). Original question: The model takes D1 and D4 to the outside faces (5.5 + 0.8 + 5.5 = 11.8 = D10), so the space behind the board is only **4.5**. Then the 5.4 camera, J4 (5.5) and J3 do not fit (see `enclosure/replica/fitcheck_report.md`). If D4 is the inside gap, the case would be 12.8 thick, which disagrees with D10. A quick check: depth from the back cover's rim to its floor, and rim to the board with the board in.
13. ~~**(new) Stubs and the inner rib in Z:**~~ **Answered (2026-10-03):** they stop above the board plane (photo 871567b6). Modelled down to Z 8.0, 0.2 above our board, so no board-edge clash any more. Original question (C14 "5 high"): 5 high from the inside of the face, so they reach down to the board line? Our board (72.3 wide) runs 1.2–2.0 mm into them at the screen section. If they stop above the board, there is no clash.
14. **Mostly answered:** the stack-up adds up to 11.8 / 11.3 at the face, so D10 is over the face, not the keys. Still open: do the jaws sit on the feet? Original question: **(new) D10 "11.3 overall":** measured over the key tops or over the face? The model takes the face (keys 1.5 above it). And do the jaws sit on the feet (model: feet 0.3 extra)?
15. **(new) Where does the face step** from 11.3 to 11.8? The model ramps it between REPLAY and the display bezel (guess).
16. **(new) Mid screw posts:** C8 gives 46.5 between them; the board's H1–H3 are 45.6 apart, so the H1 post rubs its hole (−0.06). The pair's left-right centre is still from the mat photo (−0.8).
17. **(new) Fit check, for the PCB agent:** re-run `python build_fx115es_replica.py fitcheck fitcheck_shots` after `ai_calc_board.step` is regenerated. Today's run (STEP 18:10): camera module 0.9 into the back floor; J4 1.0 and J3 0.5 into the floor and in the solar box; solar box (5–5.5 high) over the board's top-right corner (27 × 13 mm) plus U3, D1, D7, Q2, C6, the LiPo; locating posts H2 −0.54, H9 −0.29, H6 −0.11 in their 4.2 holes; board edge in the screen-section inner rib 1.2 and stubs up to 2.0. ESP32-S3 and U6 3D models are missing from the STEP export (proxies used).
18. **Handled (stage 13; LR44 cup later dropped: battery on the back-cover floor):** grind list = solar box, rib B (camera), LR44 cup; locating holes H2/H9/H13/H14 slotted and H6 moved (fit both post readings); STEP regenerated. Original: **(new) For the PCB agent: rev B2 fit check on the stage-12 STEP (23:02).** What remains is all solar-box / rib / cup items that grinding fixes (camera 0.31 on rib B, J4 inside the solar box, J3 1.0, U3 1.05, D1 0.75, Q2 0.70, LiPo 3.2 in the solar box and 2.0 into the front shell's LR44 cup), plus the locating posts H2 −0.54 / H9 −0.29 / H6 −0.11 against their holes. The STEP board is 0.71 thick; KiCad says 0.8. Details: `enclosure/replica/fitcheck_report.md`.
19. **Done (stage 13):** the table now uses 6.0 (limit 5.7) and has camera, LiPo and magnet rows. It still counts any rib within 1.5 mm as on top of a part, on purpose (photo rib positions are ±1 mm). Original: **`hardware/fitcheck/clearance_table.md` disagrees with the replica.** It still uses a 4.5 free height (now 6.0). It adds the rib height on top of the part height wherever a rib is within 1.5 mm, so U2, C5, R2, R4, D2 and C16 show as clashes; in the replica's exact geometry they clear. It has no camera row. With 6.0, its "after grind" J4 −1.0 / J3 −0.5 become +0.5 / +1.0.

## PCB (stage 11)
(details in `stage11_epaper_calipers.md`; board is DRC 0/0/0 with schematic parity)

1. ✅ **Answered (stubs stop above board; stage 12 narrowed anyway).** **C4 wall 5.5 / 4.7 mm at the screen section.** The board's top part is 72.3 mm wide (x 113.84–186.16); with a 79.3 case and 4.7–5.5 mm walls the inside is only 68.3–69.9 mm. Please measure the *inside* width of the front shell at the screen section, wall to wall, at board height, and say where the stubs are. *Assumed: the thick reading is the stubs / wire holder, the plain wall is thin, so the outline is unchanged.* If the wall really is that thick, the ESP32 has to move about 2 mm inward.
2. ✅ **Answered (C8 = 46.5 / 42.3, stage 12).** **C8 "61.8 outside to outside" — which two posts?** No post pair on the board matches (57.95 or 58.9 c-c). *Assumed: holes stay at the photo positions.*
3. ✅ **Answered (C9 = 68.1, posts clear of the board).** **C9 "3.7 / 49.65" — which posts, and is 3.7 a diameter?** *Assumed: photo positions kept.*
4. **Window position left-right.** Only its size (60.65 × 24.3) and top edge (24.0) were measured. *Assumed: centred on the case (x = 150).*
5. **Window wider than the panel glass** (60.65 vs 59.2): about 0.5–0.8 mm of board shows at each end of the glass. *Assumed: fine; a black mask sticker can hide it.*
6. **E7 e-paper ribbon length** (panel edge to tip). *Assumed: 14.3 mm per the drawing; 13.5–15 mm works.*
7. **Panel active area centred on the glass?** *Assumed yes (as since stage 1); if your panel's driver end is wider, the picture sits off-centre by that amount.*
8. **Screw vs locating posts.** From the bare Casio board photo: the two row-1/2 posts (H1/H3) and the bottom notches are screw posts (holes 6.0 mm, notches unchanged); all others are 2.9 mm locating posts (holes now 4.2 mm). *Tell me if any small post is actually a screw.*
9. **R20 now on VBUS_SENSE, U3 now RT9080-33GJ5, new C32 22 µF, new TP5/TP6/TP7.** Nothing to do unless you want the AP2112K back. Firmware: read CHG_STAT only while VBUS_SENSE is high (it reads low with no cable).
10. **Firmware to-do (review S4/S6):** cap Wi-Fi TX power (~11 dBm) and lock AI below ~3.6 V; drain the TCA8418 FIFO / clear INT_STAT before deep sleep; release camera GPIOs (no pulls) before cutting CAM power; USB-Serial-JTAG off without cable.
11. **JLCPCB preview:** check D7's cathode band is on the VBUS pad (towards J3). *Assumed: KiCad 0° = JLCPCB 0° for this SOD-123FL part.*

## PCB (stage 12)
(details in `stage12_measurements.md`; board is DRC 0/0/0 with schematic parity, ERC 0)

1. ~~**(blocks order) J4 battery socket**~~ **Answered (stage 13):** with D4 = 6.0 J4 fits (0.5 mm) once the solar box is ground flat; no board change. Original: **J4 battery socket is 5.5 mm tall; the space above the board there is about 4.0–4.6 mm** (from D1 + D10 + D5; D4 read literally would give 5.5). Even with the solar box ground flat, it's ~1 mm too tall. No JST-PH SMD socket on JLCPCB is lower. Please measure **D11** (space above the board line beside the battery bay, front + back halves added). Options if it really is ~4.5: (a) a 3.6 mm MX1.25 socket (C7430468, in stock; footprint already in the library) with a 1.25 mm-plug battery, e.g. a 100 mAh cell sold with a Molex 51021 plug; (b) leave J4 off the assembly and solder the battery leads to TP6/TP4 (you said no soldering); (c) cut a small window in the back cover over J4. *Assumed: J4 unchanged until you answer.*
2. ✅ **Answered: you chose option A, done in stage 13b.** **Still blocks order, now a decision (stage 13 → Heights item 1).** Original: **J3 magnet socket (5.0) plus the magnet's legs and body don't fit under the back cover** at the top edge (−0.5 mm after grinding, worse with the magnet itself). This needs the Adafruit 5358 in hand (P5) to see how it sits in the top-wall slot. *Assumed: J3 unchanged. If it doesn't fit, a board-edge notch so the magnet sits beside the board is the likely fix.*
3. ~~**(blocks order) Camera height.**~~ **Answered (stage 13):** 5.4 fits under 6.0 (0.5–0.6 mm) once rib B is ground for 14 mm over it; 7 mm window; the lens no longer has to go into the hole. Original: **Camera height.** The module is 5.4 tall; the space is ~4.0–4.6 (5.5 if D4 is right). It only fits if the lens barrel goes into the 6 mm back-cover hole. Please photograph the module from the side next to a ruler (F3) so I can see how tall the square body is without the lens, and how wide the lens barrel is. *Assumed: body ≤ 4 mm, barrel ≤ 6 mm; rib B ground away over the camera.*
4. ~~**D4 vs D1: which faces?**~~ **Answered (2026-10-03):** D4 = 6.0 to the back cover's inside face; D1 = 5.5 from the front-shell rim to the board. Original: **D4 vs D1: which faces?** Was D4 measured from the inside of the back cover, and D1 to the board's top face (the side facing the back cover)? This decides 4.5 vs 5.5 mm above the board, and with it items 1–3. *Assumed: 4.5 design, 4.0 worst case.*
5. **Dry fit before ordering:** print `fitcheck/grind_map_front_shell.svg` at 100 %, cut it out (outline and holes), and lay it on the posts in the front shell. Check that H3 (right screw post beside SHIFT) and H2/H6/H9 drop over their posts without pushing. The new photo puts the H3 post up to 1.4 mm left of the hole centre (other photos and C8 say it's right). *Assumed: holes at the photo/C8 positions.*
6. ~~**Snip the wire hook**~~ **Not needed (stage 13):** the clips stop above the board plane (photo 871567b6). Original: **Snip the wire hook at the top right of the screen section** (inside the wall, level with the top of the window, about 1 mm into where the board goes). It only held Casio's wires. The other hooks and the side pins are now outside the board. *Assumed: OK to snip.*
7. **Final grind list (see `FINAL_STATUS.md`):** solar box, rib B 14 mm, 7 mm camera window, magnet U-notch (5b lip only if present); the LR44 cup is no longer ground (battery on the back-cover floor). **Updated in stage 13:** grind list is now solar box flat, rib B over the camera, LR44 cup (front shell); the big ring no longer needs grinding. Original: **Grinding plan (back cover):** solar box flat to the floor; the big ring's arc next to D2/C16 down to 2 mm; rib B where it crosses the camera; the LR44 cup in the front shell. All shown in red on `fitcheck/grind_map_back_cover.svg`. *Assumed: you're happy to grind these (D6/D13 said yes).*
8. **C15 readings:** the H2/H9 and H6 rows came out +0.9 and −1.0 mm from the photos, in opposite directions only 11 mm apart, so I think one reading slipped. If you re-measure, please do just those two rows, to the centre of the post. *Assumed: photo positions.*
9. **E-paper picture sits 2.6 mm off-centre in the window** (the panel's active area is not in the middle of the glass, per the Waveshare V4 drawing). Fully visible, just not centred. Firmware can shift it; a later board can move the panel. *Assumed: OK for v12.*
10. **E7 (ribbon length) still unmeasured.** You can measure it on the panel while it's still in the HAT: glass edge to ribbon tip. 13.5–15 mm works. *Assumed: 14.3 per the drawing.*
11. **Regulator:** RT9080 kept (C841192, Extended, $0.12, in stock); AP2112K is also Extended and costs more. Nothing to do.
12. **R12 3 Ω could become a Basic 2.2 Ω part (saves $3)**, but it changes the e-paper booster's current limit. Measuring E5 (the resistor next to the inductor on your HAT) would settle it. *Assumed: stays 3 Ω.*
13. **The ESP32 antenna now hangs ~4 mm past the board edge** into the gap beside the wall (Espressif's preferred layout). Nothing to do, but don't bend the module when fitting the board.

## Heights (stage 13)
(details in `stage13_heights.md`; board is DRC 0/0/0 with schematic parity, ERC 0. Free height behind the board 6.0, design limit 5.7.)

1. ~~**Magnet connector: pick A or B.**~~ **Answered (2026-10-04): A, done in stage 13b.**
   - J3 = C46061768 at (127.5, 72.0).
   - The magnet sits in front of the board edge, so no notch was needed.
   - D7, U2 and C6 moved; the corner was re-routed; DRC 0/0/0.
   - Slot: U-notch in the top wall, 21.5 wide × 7.95 from the rim, 6.4–27.9 mm from the right side edge (seen from the front).

   Original question: **(blocks order) Magnet connector: pick A or B.** Its face is 7 mm tall, more than the 5.7 mm above the board, and its legs can't reach the 5 mm J3 socket, so it must sit across the board edge. Grinding can't fix this.
   - **A (no soldering, recommended):** a notch in the board's top edge, plus a right-angle SMD female header (C46061768, $0.18, 8,753 in stock). You straighten the 4 legs with pliers, push them in and glue the magnet into the top-wall slot. The charger parts move about 10 mm. I need about 2–3 h for that after your yes.
   - **B (smallest board change):** 4 plated holes where J3 is now, and a 22 × 5 mm window in the back cover's top edge. Someone has to solder 4 joints (JLCPCB through-hole service via Global Sourcing, a makerspace, or you).
   - *Assumed: J3 unchanged until you choose.*
2. **Grind list** (print `fitcheck/grind_map_back_cover.svg` / `grind_map_front_shell.svg` at 100 %):
   - solar box flat to the floor (5–5.5 mm),
   - rib B flat for 14 mm over the camera (1 mm),
   - ~~LR44 cup in the front shell trimmed flat (5–6 mm),~~ **superseded:** the battery now lies on the back-cover floor, so the LR44 cup is not ground (`enclosure/final_assembly/assembly_report.md`),
   - magnet U-notch in the top wall, and the back-cover lip at the notch (5b) only if your cover has one,
   - drill the camera window, Ø 7 mm at KiCad (150.0, 95.1).

   Never grind into the floor: keep ≥ 0.8 mm. *Assumed: OK (you said you can grind anything).*
3. **Camera mounting:** Kapton under it, then thin (≤ 0.2 mm; **now 0.1 mm**, see ⭐ Closed) double-sided tape, not foam. If the "with heat sink" version is taller than 5.5 with the sink on top, leave the sink off. *Assumed: module 8.5 × 8.5 × 5.4, lens centred.*
4. **Locating holes H2/H9/H13/H14 are now slots and H6 moved 0.2 mm**, so each fits the post at both the photo and the C15/CAD position (0.10–0.61 mm of room). At the dry fit, if H6 pushes, file it a little toward the top of the board. *Assumed: posts Ø 2.9.*
5. **When the magnet part arrives (P5):** check that its pins leave the body on the centre line of the 7 mm face, and measure the body depth (drawing: ~4 mm). Option A depends on it.
6. **D3 "about 5 at the top edge":** probably the back cover curving at the very end. J4 is ≥ 8 mm from the top edge, so it shouldn't matter; if you can, check the depth right above J4 (KiCad 142.6, 74.6) with the solar box ground.
7. ~~**(new, 13b) Magnet pin order.**~~ **Superseded by stage 14 (#1 below).** The old order (VBUS, GND, D−, D+) matched the cable in neither orientation, and the "turn the magnet piece over" advice would have put 5 V on D+. J3 is now VBUS, D−, D+, GND with the piece's N end at pin 1.
8. **(new, 13b) Leg length.** The drawing gives about 5.3 mm of straightened leg, and J3 takes about 4.5 mm. If the face can't sit flush, trim the legs a little. *Assumed: per the drawing; check when P5 arrives.*
9. **(new, 13b) Extra decoupling.** C33/C34 (22 µF 0805, Basic C45783) were added on the +3.3 V trunk just above the ESP32. Nothing to do.


## Final review
(2026-10-04, `REVIEW_final_2026-10-04.md`. Verdict: ready to order. DRC 0/0/0 with parity, ERC 0, fab files current, all 30 LCSC parts in stock at JLCPCB.)

1. **Paper dry fit before paying.** Print `fitcheck/grind_map_front_shell.svg` at 100 % and check the posts drop through H2/H3/H6/H9/H13/H14. This is the only remaining check that could change the board. *Assumed: holes are right.*
2. ~~**JLCPCB preview rotations.**~~ **Superseded by stage 14:** the CPL in `fab/` is now rotation-corrected (`tools/jlc_cpl.py`); check the preview against `ORDER_WALKTHROUGH.md` step 5 instead. Original: Expect to click-rotate several parts by 180°: Q1–Q3, U2–U5, U7, J4. U6 may be off by 90°. The walkthrough in the review says where pin 1 must be on each part. **D7's band must be on the right-hand pad (towards J3).** *Assumed: fixed in the preview, CPL unchanged.*
3. ~~**U1 / J3 may show up shifted in the preview.**~~ **Superseded by stage 14:** the CPL positions and rotations were checked against JLCPCB's EasyEDA footprints (`verification/05_stage14_recheck.md`); verify the preview against `ORDER_WALKTHROUGH.md` step 5, don't move or rotate anything. Original: Their KiCad origin isn't the body centre (U1: 2.55 mm, J3: about 6 mm). Ask JLCPCB to align them to the pads. Also answer "intended" to any "component exceeds board edge" question about the U1 antenna or J3. *Assumed: the engineer aligns to the pads.*
4. **Stale "(blocks order)" items.** Measurements 1, 2 and 4 and PCB (stage 11) item 1 were answered in stages 12–13 (C8/C9/C4/D-depths). Only the dry fit and E7 are really open. *Assumed: you agree they're closed.*
5. **Battery polarity (still a meter check).** J4 is − on the left and + on the right, seen from the component side with the opening towards you, which matches the usual Adafruit/SparkFun convention. If your battery turns out reversed, swap the two crimp contacts in its plug (lift the tabs with a needle). No soldering, and Q1 protects the board meanwhile.
6. **Later revision (optional).** R5 20 k → 15 k (C25756) keeps VBUS_SENSE under 3.6 V with a 5.5 V adapter. Not needed for this order.
7. **Firmware to-do (from both reviews).**
   - Cap Wi-Fi TX power at about 11 dBm.
   - Clear the TCA8418 INT before sleep.
   - Release the camera pins before turning the camera off.
   - Don't enable IO35's internal pull-up without VBUS.
   - Camera power-up order: RESET low → PWR_EN → 5 ms → RESET high.

## Stage 14
(2026-10-04, `stage14_verification_fixes.md`. Fixes every finding of `verification/01–04`. Board v14: DRC 0/0/0 with parity, ERC 0. Tests 548/548, firmware 0 warnings.)

1. **Magnet (J3) pin order changed to VBUS, D−, D+, GND, with the piece's N end at pin 1** (silk "N" and "+" by pin 1, "−" by pin 4).
   - Why: the Adafruit #5412 cable's face reads GND, D+, D−, VBUS from its N end. Mated face to face, the board piece reads VBUS, D−, D+, GND from its own N end.
   - Power and ground are now the outer pins, so a piece fitted backwards can never put 5 V on a data line. It would swap VBUS and GND instead: D7 clamps that, and the host port current-limits.
   - **Do the meter check before gluing** (`ORDER_WALKTHROUGH.md`). *Assumed: Adafruit's drawing is right (~85 %). The meter check catches it if not.*
2. **The CPL now has JLCPCB's rotations built in** (`tools/jlc_cpl.py`, run by `make_outputs.sh`).
   - Checked against JLCPCB's own EasyEDA footprints: 22/22 polarised parts put pin 1 on pad 1.
   - Upload `fab/ai_calc_CPL_JLCPCB.csv` as is.
3. **Camera ribbon (verification 03 L1).** The ribbon is about 62 mm past the module, so there's only about 1.4 mm to spare, not 10 mm. **No board change (J1 stays).** Moving J1 2 mm closer would help only if the ribbon is short, and a flat ribbon can't take up extra slack if it's long.
   - **Decision rule:** if your measurement (module far edge → tip) is under about 70.3 mm, tape the module up to 1.7 mm towards J1 (still inside its keep-out) and drill the 7 mm window over the lens where it actually sits.
   - *Please measure it when the camera arrives.*
4. **E-paper stiffener (verification 03 L3).** **No board change (J2 stays).** Moving J2 1 mm away from the slot would make the ribbon too short, because the length budget already has no spare (14.3 = 14.3).
   - **Decision rule:** if the thick, stiff end is ≤ 5 mm, it's fine as is.
   - If it's > 5 mm **and** the glass-edge-to-tip length (E7) is ≥ 15.3 mm, tell me before ordering, and J2 moves +1 mm.
   - If it's > 5 mm and E7 < 15.3, neither position is clearly right: tell me before ordering, and don't guess. A partly inserted ribbon isn't an acceptable fix.
   - *Please measure both before ordering if you can.*
5. **H3 / SW1:** the SHIFT key pad's bottom bar was 0.055 mm from the H3 screw hole.
   - It's now thinned on that side only (new footprint `KeyPad_6.0x4.5_H3notch`). The finger contacts the carbon dot touches are unchanged.
   - Every copper item on the board is now ≥ 0.25 mm from every hole (custom check).
6. **R17 is 100 k, not 10 k.** Verification 02 (C12) and the stage-14 brief said 10 k, but the schematic, BOM (C25741) and board all say 100 k, and `pins_final.h` already said 100 k. Nothing changed; it now names R17. *Assumed: 100 k stays (only matters while ON is held).*
7. **Key names:** row 2 is now CALC and ∫dx (fx-115ES), in the schematic and in the firmware.
   - On the 115ES, Abs is SHIFT hyp, x³ is SHIFT x² and ∛ is SHIFT √. The firmware now does that.
   - **CALC/SOLVE and ∫dx/d/dx aren't implemented yet.** They show "Not supported yet" instead of doing the wrong thing. *Assumed: OK for the prototype; say if you want them implemented.*
   - Other fx-115ES SHIFT functions that the firmware ignores, as before: ∠, ←, a b/c⇔d/c, Σ.
8. **Recovery procedure corrected.** Plugging the cable in doesn't reset the chip while the battery is connected. Now:
   1. Hold TP1 (BOOT) to TP4 (GND).
   2. Tap TP7 (EN) to GND.
   3. Flash.
   - Or unplug the battery first.
   - UART backup on TP2/TP3.
   - Never burn the security or USB-disable eFuses.
9. **Web simulator:** its page now has the 115ES labels, but the built `simulator.html` / `AI_Calc_Simulator.html` (and the Windows exe) still have the old keys until they're rebuilt (Emscripten / MinGW aren't on this PC).

## Firmware

Stage-13 firmware notes: `firmware/FIRMWARE_STAGE13.md`. Each item says what I assumed; tell me if it's wrong.

1. **Where should the proxy run, and under which address?** The calculator needs an https:// address for `proxy`. *Assumed: Render (or Railway / Fly.io) with its own https address for now; a domain later.*
2. **The link page is only "code + e-mail".** Anyone who sees the 6-letter code on a calculator could link it to their own e-mail. That's fine for testing, but it needs a real sign-in (e-mail magic link or shop account) before selling. *Assumed: OK for the 2 prototype boards.*
3. **Haiku-first routing:** on or off? It saves money on easy photos, but hard ones are a few seconds slower because they're tried twice. *Assumed: off (`ROUTING=main`, always Sonnet 5.5) until you've compared costs.*
4. **Free tier:** how many free solves a day without a subscription, if any? *Assumed: 0 (off); the switch is `FREE_DAILY`.*
5. **Refusal fallback** (`REFUSAL_FALLBACK=1`): a beta API feature that lets another model answer if Sonnet 5.5's safety filter wrongly declines a homework photo. *Assumed: on. If your account rejects it (HTTP 400 on every solve), set it to 0.*
6. **The tester (`firmware/`) and both simulators still send your API key straight to Claude.** They are your own development tools, and `api_key.txt` stays git-ignored. *Assumed: OK. Only the board firmware is the product.*
7. **Wi-Fi is only on in AI SOLVE.** The first = after opening AI SOLVE may wait a few seconds for the hotspot (it joins while the photo is taken). *Assumed: worth it for battery life.*
8. **Self-test key combo:** SHIFT + ALPHA held, then ON. *Assumed: fine; it can't clash with SHIFT, 7, ON (exam mode).*
9. **Wake grace time:** after a key wakes the chip while it's off, it stays awake 4 s before sleeping again. That's what makes SHIFT, 7, ON and the self-test combo work. *Assumed: 4 s.*
10. **Memory when the battery is pulled:** memories, Ans and history live in RTC memory and are lost if the battery is unplugged (like the Casio). Storing them in flash on every power-off would keep them. *Assumed: RTC only. Exam mode is in flash.*
11. **AI low-battery cut-off:** 3.6 V on battery (review S2/S4). *Assumed: 3.6 V; `kAiMinMillivolts` in `core/battery.h`.*
12. **Battery calibration:** compare `status` mV with a meter on TP6 (BAT+) once, then set `kBatteryCal` in `firmware-prototype/src/power.cpp`. *Assumed: 1.00 until measured.*
13. **TCA8418 RESET isn't wired to the ESP32** (R16 pull-up only). The firmware re-writes the chip's registers at every start, and a hung scanner would need a battery pull. No free non-strap GPIO is left, so no change is proposed. *Assumed: OK.*
14. **Charging while off isn't shown.** Plugging the cable in doesn't wake the chip (VBUS_SENSE isn't an RTC pin). Charging still works, and the icon shows at the next ON. *Assumed: OK.*
15. **Sleep current must be measured on the first board** (bring-up step 2 in FIRMWARE_STAGE13.md). Target ≤ 40 µA with RT9080 and R20 → VBUS_SENSE. If it's higher, send me the number.
16. **Web simulator:** rebuild it (`sim/web/build.sh`) to get tutor mode and the ✓/⚠ check there too. *Not done here: no Emscripten on this PC.*

## CAD (3-part shell + slide case)

Rev C2 (2026-10-05): the body is modelled as **two parts** (silver faceplate + navy back cover) plus the **navy slide case** as a separate accessory. Details: `enclosure/final_assembly/assembly_report.md` section 7, `slide_case_mods.md`, `window_mask.md`. Each item says what the model assumes now.

1. **Side profile photo, please (most useful).** Lay the closed calculator on its long side next to a ruler and photograph it straight on, both long sides. It shows where the navy wall starts to rise and how high it goes. *Assumed: the split line ramps from the rim at about the small ring (51 mm below the top) to 9.8 mm high at the big ring (74 mm below the top), then stays at 9.8 round the keypad and the bottom end.*
2. **Is there a groove along the long sides** (where the case's edges would ride)? A close-up photo of one long side edge, with the case off, would settle it. *Assumed: a shallow groove 1.0 tall, 0.35 deep, 5.9-6.9 mm up from the back, along both long sides. None of the photos so far shows one, so it may not exist (the case may just grip the body's edges).*
3. **Photos of the hard case:** inside (looking into it), both long edges close up (the lips / rails), both ends (is one end open? is there a finger notch?), and one from the side. *Assumed: open end at the display end, closed end at the key end (Casio's manual: "slide its hard case downwards to remove it"), wall 1.0, floor 1.2, 0.3 clearance, no notch.*
4. **Case size with calipers:** outside length × width × depth, wall and floor thickness, and how far the edge lips stick in. *Assumed: 162.3 × 81.9 outside, floor inside face 0.3 above the key tops when stored.*
5. **Faceplate skirt depth:** with the back cover off, how far does the silver part's edge go down below the face in the keypad section (calipers from the face to the bottom of the silver edge)? *Assumed: 3.0 mm (to Z 8.3; the face is at 11.3).*
6. **Navy wall thickness low down** in the keypad section (calipers across the wall about halfway down)? C5 = 0.8 was probably at the top edge. *Assumed: 0.8 the whole height.*
7. **Snap tabs:** the photos show small clips ("III" marks) in the middle of the top and bottom ends of the silver part. *Not modelled; harmless for the fit, but tell me if they matter to you.*
8. **Window mask:** after the first test, is any white edge visible on one side? *Assumed: Waveshare's active area 48.55 × 23.70, centred where the KiCad keep-out says; trim 0.2 mm from that side of the opening if needed.*
9. **Will you ever drill the case?** Only matters later: it covers the camera in use (8 mm hole, `slide_case_mods.md`). The magnet stays clear in both positions.