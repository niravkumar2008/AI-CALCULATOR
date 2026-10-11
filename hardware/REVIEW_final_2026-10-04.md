# Final pre-order review (2026-10-04, stage 13b board)

> **⚠ Superseded by stage 14: see `FINAL_STATUS.md`, `ORDER_WALKTHROUGH.md` and `verification/05_stage14_recheck.md`.** This review is of the stage-13b board and is kept only as history. **Do not follow its ordering advice.** In particular:
> - **WRONG: "rotate Q1–Q3, U2–U5, U7, J4 by 180° in the preview"** (F1, and Final review #2 in QUESTIONS). The `fab/` CPL is now rotation-corrected by `tools/jlc_cpl.py` and checked against JLCPCB's EasyEDA footprints. **Rotate nothing in the preview; only verify** it against `ORDER_WALKTHROUGH.md` step 5.
> - **WRONG: "U1 / J3 / J4 may show up shifted"** (F2). The re-check found the positions correct; don't ask for or make shifts.
> - **WRONG and dangerous: "J3: 1 VBUS, 2 GND, 3 USB_DM, 4 USB_DP"** and any advice to flip or turn the magnet piece over if it seems mirrored. With that wiring a flipped piece put 5 V on D+. The v14 board is **J3 = 1 VBUS, 2 D−, 3 D+, 4 GND, N end at pin 1**, and the piece is fitted by a meter check of the real cable before gluing (`ORDER_WALKTHROUGH.md` → "Magnet piece").
> - Two FATAL items this review missed (the J3 pin order, verification 03 F1; the CPL rotations, verification 04 M1/M2) were found by `verification/01–04` and fixed in stage 14 (`stage14_verification_fixes.md`).
> - Still valid: the cost estimate (**$135–155 before shipping, $165–220 delivered**), the L1–L5 low items and the electrical checks other than J3.

Board: `kicad/ai_calc.kicad_pcb` (saved 04:09), silk "AI CALC v13". Fab files: `fab/` (generated 04:11). The order is planned for Monday 2026-10-05: JLCPCB, 5 PCBs, assembly on 2 of them.

## Verdict: **ready to order: YES**, with three checks in JLCPCB's parts preview

**Why yes:**
- The design checks are clean:
  - **DRC: 0 errors, 0 warnings, 0 unconnected, 0 schematic-parity issues.** Run fresh with `--refill-zones --schematic-parity --severity-all`.
  - **ERC: 0.**
- The fab files match the board exactly:
  - The BOM regenerated from the schematic is byte-identical to `fab/ai_calc_BOM_JLCPCB.csv`.
  - The CPL matches the board's positions and rotations for all 77 parts.
- **All 30 LCSC numbers exist and are in stock at JLCPCB today** (live JLCPCB parts API):

  | Part | In stock |
  |---|---|
  | ESP32 module | 1,893 |
  | Magnet header | 8,753 |
  | Everything else | ≥ 5,250 |

- **Every item from the 2026-10-03 independent review is fixed or handed to firmware or a meter check.** The table is below.
- I re-checked these pin by pin against the datasheets:
  - the new regulator U3: RT9080 pinout from Richtek DS9080-09, so VOUT is pin 5 and NC is pin 4
  - R20
  - the TVS D7 direction
  - J3
  - the camera GPIO map
  - the e-paper booster
  - USB D+/D−

**What still needs a human eye (not a board change):**
1. **JLCPCB's parts preview.** ~~Several will probably show up rotated by 180° and need a click to fix; U1, J3, J4 may show up shifted.~~ **Superseded (stage 14): the CPL is corrected; verify only, rotate nothing.**
2. **The paper dry fit** of the locating holes, before you pay (QUESTIONS → PCB stage 12, item 5). This is the only mechanical item left that could change the board.
3. **After the boards arrive, use a meter before plugging anything in:**
   - the battery polarity at J4
   - the magnet cable's VBUS on J3 pin 1
   - camera cable fingers 2 and 15

**Expected price:**
- About **$135–155 before shipping** (5 PCBs + 2 assembled). The breakdown is in walkthrough step 9.
- About **$165–220 delivered**, depending on the shipping method and import fees.

## Findings, by severity

Nothing is BLOCKING. No electrical mistakes were found.

### Check during the order (preview / engineer questions)

**F1. ~~Parts that will probably look rotated in JLCPCB's preview.~~ WRONG, superseded by stage 14 (CPL corrected by `tools/jlc_cpl.py`; rotate nothing).**
- KiCad and JLCPCB draw some packages with different zero orientations. These are usually off by 180°:
  - **SOT-23:** Q1, Q2
  - **SOT-323:** Q3
  - **SOT-23-5:** U2, U3, U4, U5
  - **SOT-23-6:** U7
  - **JST socket:** J4
- The **QFN U6** is sometimes off by 90° or 270°.
- The **custom footprints** J1, J2, J3 and U1 can't be predicted.
- I did **not** edit the CPL. JLCPCB's library orientation for these exact part numbers can't be confirmed from here. Their preview is the authority, and rotating there is free.
- The walkthrough gives each part's pin-1 position in board coordinates, so you can tell right from wrong.

**F2. ~~Three parts' KiCad origin isn't the body centre, so the preview may show them shifted.~~ Superseded by stage 14 (positions verified; don't shift anything).**
- **U1 ESP32:** the origin is the centre of the pad area, not the whole module. The module is 2.55 mm longer on the antenna side, so the preview may put it 2.55 mm off along x.
- **J3 header:** the origin is the pad row. The body runs 1.6–10.1 mm in front of the pads, towards the top edge.
- **J4:** the origin is the middle between the signal pads and the tabs. That's probably the same as JLCPCB's.
- Ask JLCPCB to align by the **pads** (copper), not the body outline. Their engineers do this routinely.

**F3. Overhanging parts.**
- U1's antenna end hangs about **4.5 mm past the left edge** on purpose: Espressif's recommended placement, with no copper under the antenna.
- J3's socket body sticks out **0.4 mm past the top edge** (y 61.52 vs edge 61.94).
- JLCPCB may ask about "component exceeds board edge" or about edge rails. Reply that it's intended. Ask for no rails, or rails that leave the left edge and the J3 area free.

### Low (nice to fix later, no action for this order)

**L1. VBUS_SENSE can reach 3.67 V with a 5.5 V adapter.**
- That's above the ESP32's VDD + 0.3 V. Previous review N2 is still open.
- The current is limited by 10 k, so it's harmless in practice.
- Fix in a later revision: R5 20 k → 15 k (C25756, Basic).

**L2. Solder-mask expansion is 0 mm.**
- The project uses `pad_to_mask_clearance 0`.
- JLCPCB applies its own process mask opening, and 0.5 mm-pitch parts (J1, J2, U6) get mask dams of ≥ 0.2 mm, which they can make.
- No change. They may just merge the dams between the FPC pads, which is fine.

**L3. The drill-map files (`*-drl_map.gbr`) are inside the gerber zip.**
- JLCPCB ignores them, but it may list them as "unknown layers".
- If it complains, delete those two files from the zip and re-upload.

**L4. Stale "(blocks order)" lines.**
- `QUESTIONS_AND_ISSUES.md` "Measurements" items 1, 2, 4 and "PCB (stage 11)" item 1 are still marked "(blocks order)".
- They were answered in stages 12/13: C8 = 46.5 pair, C9 = 68.1, C4 = the board is narrowed past the wall pins, D1–D4 measured.
- Only the paper dry fit (stage-12 item 5) and E7 (ribbon length, 13.5–15 mm works) are really open.

**L5. Firmware items carried over from the previous review (S4/S6/N3).**
- Cap Wi-Fi TX power at about 11 dBm.
- Clear the TCA8418 interrupt before sleep.
- Release the camera pins before turning the camera off.
- Never enable IO35's internal pull-up while VBUS is absent: it would put ~0.4 V on the magnet's VBUS contact through R20/R4.
- Power up the camera in this order: RESET low → enable → 5 ms → release RESET.

None of these change the board.

## Previous review (2026-10-03): status of every item

| Item | Status |
|---|---|
| B1 stale fab files | **Fixed.** fab (04:11) is newer than the board (04:09) and the schematic; BOM and CPL verified identical to fresh exports. |
| S1 R20 back-feed | **Fixed (option B).** R20 = VBUS_SENSE ↔ CHG_STAT (netlist: R20.1 VBUS_SENSE, R20.2 CHG_STAT). |
| S2 regulator quiescent current | **Fixed.** U3 = RT9080-33GJ5 C841192. Pinout checked against DS9080-09: 1 VIN = SYS, 2 GND, 3 EN = SYS (datasheet: "EN may be tied directly to VIN"), 4 NC (unconnected), 5 VOUT = +3V3. Input cap C7 4.7 µF next to it. Output ≥ 1 µF needed, the board has about 110 µF on +3V3 (C1/C32/C33/C34 22 µF, C3/C8 10 µF, …), which is fine (any ceramic ≥ 1 µF is stable). |
| S3 D7 direction | **Correct on the board.** D7 pad 1 (cathode, band) = VBUS, at x 120.65 (the right-hand pad, rotation 180). Must be confirmed in the preview (walkthrough). |
| S4 Wi-Fi brown-out | **Board part fixed:** C33/C34 22 µF added (66 µF next to U1). Firmware TX-power cap still to do (L5). |
| S5 J4 battery polarity | **Matches the usual Adafruit/SparkFun convention as far as I can verify.** J4 pin 1 = GND (left), pin 2 = BAT+ (right), seen from the component side with the opening towards you. Silk "−" on the left, "+" on the right. The KiCad pad numbering matches the stock KiCad JST footprint. Sources disagree on how to describe it, so **meter-check before plugging in**. If it's reversed, Q1 protects the board. Then swap the two crimp contacts in the battery plug (lift the plastic tabs with a needle), no soldering. |
| S6 firmware standby | Open, firmware (L5). |
| N1 D1 leakage | Open: measure on the first board. |
| N2 R5 15 k | Open (L1), optional. |
| N3 camera power order | Open, firmware. |
| N4 +3V3 trunk width | **Fixed:** the trunk is 0.5 mm (PowerMain class). |
| N5 antenna | **OK:** module overhangs the edge, no copper under the antenna (B.Cu GND pour stops at about x 120, the module's pad area). |
| N6 stock | **OK:** C1525 has 21.6 M at JLCPCB; ESP32 1,893. |
| N7 cost | Updated below. |
| N8 J2 / e-paper fold | Unchanged; the paper-strip mock-up is still a good idea (E7). |

## What I checked and found OK

**JLCPCB file format**
- BOM columns are `Comment, Designator, Footprint, LCSC Part #`.
- CPL columns are `Designator, Mid X, Mid Y, Layer, Rotation`, all Top.
- There are 77 designators, and they're the same set in both files.
- No DNP parts.
- Test points, fiducials, key pads and holes are correctly left out.

**Gerber zip contents**
- F/B copper, mask, silk and paste
- Edge.Cuts (one outline plus one inner cut-out = the e-paper slot)
- PTH drill (0.3 mm vias only) and NPTH drill
- Drill and gerbers share the same origin; coordinates were checked against each other.

**Holes and slots**
- **E-paper slot:** a routed **inner cut-out in Edge.Cuts**, 1.0 × 14.0 mm with round ends (x 172.4–173.4, y 85.83–99.83). JLCPCB mills it as part of the outline: 1.0 mm is their minimum slot width, so it's within capability.
  - The straight part is 13.0 mm, enough for a 12.5 mm-wide 24-pin 0.5 mm ribbon.
  - No copper within 0.35 mm of it.
- **H2/H9/H13/H14:** NPTH oval slots (G85) of 4.2–4.4 × 4.45–5.0, fine for JLCPCB. The other holes are round NPTH (4.2 and 6.0).

**Board vs JLCPCB 2-layer capability**

| Item | This board | JLCPCB |
|---|---|---|
| Track width | ≥ 0.2 mm (2,529 × 0.2, 356 × 0.25, 35 × 0.3, 113 × 0.5) | 0.1 |
| Clearance | 0.15 mm (rule), 0.2 on the camera bus | 0.1 |
| Vias | all 332 are 0.6 / 0.3 mm, annular ring 0.15 | 0.13 |
| Copper to edge | ≥ 0.3 mm | 0.3 for routed edges |
| Hole to copper | ≥ 0.25 | — |

- Silk: no silk over pads (DRC check on, 0 hits). Minimum text height 0.8 mm.
- 3 fiducials (FID1–3, 1 mm copper, 1 mm mask opening).
- "JLCJLCJLCJLC" order-number marker on the F silk at (160, 133), on the component side, away from the key pads.
- Board thickness is set to 0.8 mm.
- B.Paste is empty (no parts on the key side), which is correct.

**Electrical** (netlist dumped from the board, which equals the schematic per the parity check)
- **Camera GPIO map:** exactly as required. XCLK 18, D0 13, D1 11, D2 10, D3 12, D4 14, D5 16, D6 17, D7 21, VSYNC 36, HREF 47, PCLK 15, PWDN 48, RESET 38, SIOD 40, SIOC 39, PWR_EN 34. It matches `pins_final.h`. J1 is still `…_CamReversed`.
- **J3 (magnet):** ~~1 VBUS, 2 GND, 3 USB_DM, 4 USB_DP~~ **(wrong order, fixed in stage 14: 1 VBUS, 2 USB_DM, 3 USB_DP, 4 GND, N end at pin 1).**
- **VBUS net:** C5, D1 anode, D7 cathode, Q2 gate, R4, U2 VDD only, so **no battery-voltage path to the contacts**:
  - D1 blocks SYS→VBUS.
  - Q2's body diode points VBAT_P→SYS.
  - The charger blocks reverse current.
- **USB:** D− = IO19 (pad 23), D+ = IO20 (pad 24). USBLC6 pins 1/6 = D+, 3/4 = D−, pin 5 = +3V3.
- **Power path:**
  - Q1 (G = Q1_G with 100 k to GND, S = VBAT_P, D = BAT+) blocks a reversed cell.
  - Q2 (G = VBUS, S = SYS, D = VBAT_P) is the load share.
  - U2 MCP73831: 1 STAT, 2 GND, 3 VBAT_P, 4 VBUS, 5 PROG (20 k = 50 mA).
- **Camera rails:**
  - U4 ME6211 2.8 V and U5 1.5 V from SYS, both enabled by CAM_PWR_EN (R10 100 k pull-down).
  - Caps C11/C12, C13 (AVDD after FB1), C15.
  - D2 2.8 V → AF.
- **E-paper booster:**
  - L1 +3V3 → SW. Q3 SI1308: G = GDR (R11 10 k), S = RESE (R12 3 Ω), D = SW.
  - D3 SW → PREVGH, C20 SW–PUMP, D4 PUMP → GND, D5 PREVGL → PUMP. The diode directions give the + and − pumps.
  - All panel caps 50 V.
  - J2: BS1 (8) = GND, 6/7 NC.

## What I changed

**No design files changed.** Board, schematic, CPL and BOM are untouched, so DRC stays 0/0/0 and nothing needed regenerating. Nothing I found was both clearly wrong and safe to change blind:
- the rotations depend on JLCPCB's library;
- the R5 tweak is optional.

New or edited files:
- this review
- `QUESTIONS_AND_ISSUES.md` → "## Final review"

---

## JLCPCB ordering walkthrough

**Moved and rewritten in stage 14: see `ORDER_WALKTHROUGH.md`.** The old walkthrough that was here is withdrawn. Two of its instructions were wrong:
- It told you to click-rotate parts in JLCPCB's preview. The CPL in `fab/` is now rotation-corrected by `tools/jlc_cpl.py`, so nothing should need rotating.
- It said to "turn the magnet piece over" if the cable was mirrored. With the stage-13b wiring, that would have put 5 V on D+. J3 is now VBUS, D−, D+, GND, and a multimeter check of the real cable is done before gluing.
