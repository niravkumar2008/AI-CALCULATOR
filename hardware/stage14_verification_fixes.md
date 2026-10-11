# Stage 14: fixing the 4-part verification findings

Date 2026-10-04. Board v14 (silk "AI CALC v14 2026-10-04").

**Final state:**
- DRC: **0 errors, 0 warnings, 0 unconnected, 0 schematic-parity issues** (`kicad-cli pcb drc --refill-zones --schematic-parity --severity-all`, KiCad 10.0.6).
- ERC: **0**.
- Every copper item is ≥ 0.25 mm from every hole.
- Unit tests: 548 passed, 0 failed.
- Firmware: clean PlatformIO build, 0 warnings.

**Where things are:**
- Backup of the stage-13b board and schematic: `kicad/.mcp-backups/stage14_pre/`.
- Zip: `Claude outputs/ai_calc_pcb_14.zip`.
- How to order: `ORDER_WALKTHROUGH.md` (short version: `ORDER_CHECKLIST.md`).

Each section below goes finding → fix → how it was verified.

---

## 1. J3 magnet pin order (verification 03 F1, FATAL)

**The finding.** The #5412 cable fixes the contact order, and J3 was wired VBUS, GND, D−, D+. That matched the cable in neither of the two ways the magnet piece can be fitted:
- One way gave no USB.
- The other way put 5 V on D+. That is what the old "turn the piece over if mirrored" advice would have done.

**Re-derived independently:**
- **Adafruit #5412 drawing** (`5412_C17238_4P.pdf`, downloaded from Adafruit; text positions read from the PDF and checked on the render):
  - The face reads **N, − (GND), D+, D−, + (VBUS), S**.
  - Each label has a leader line to its own pogo pin.
- **#5358 drawing** (`geometry/adafruit_5358_MG04254FRA1S1N.pdf`): N and S magnets at the two ends, with 4 contacts at 2.5 mm pitch in a line between them. It gives no pin functions, so the order has to come from the cable.
- **The mirror logic:**
  - The two faces meet face to face. Opposite poles attract, so the cable's N end sits against the board piece's S end.
  - Each contact touches the contact directly opposite it, so positions along the line are kept.
  - So the board piece, read from **its own N end**, is **VBUS, D−, D+, GND**.
  - The legs come straight out behind their contacts, so the leg order is the same.

**The fix:**
- **Order:** J3 = **1 VBUS, 2 USB_DM, 3 USB_DP, 4 GND**, with the piece's **N end at pin 1**. This is exactly the order in the brief.
  - Power and ground are the **outer** pins.
  - A piece glued in backwards gives VBUS↔GND and D+↔D−. D7 clamps the reversed supply and the host port current-limits. 5 V can never reach a data line.
  - No order can make a backwards fit harmless, because the cable itself has power at one end and ground at the other. That's why the meter check before gluing stays mandatory.
- **Schematic** (`connectors.kicad_sch`): the three J3 net labels were swapped, and the value now reads "N end at pin 1: VBUS D- D+ GND".
- **Board:**
  - Pad nets updated.
  - USB_DM: pad 2 → new via (127.3, 74.0) → the existing B.Cu path.
  - USB_DP: pad 3 → the existing via (129.8, 73.1).
  - The old DM via sat under the new DP pad, so it was removed.
  - D+/D− still run as a side-by-side pair into the USBLC6 (U7) and on to the ESP32.
  - Pad 4 now joins the GND pour, plus a stitching via at (131.31, 69.0) under the J3 body.
- **Silk** (F.Silkscreen, off the pads): **"N"** and **"+"** beside pin 1, **"−"** beside pin 4.
- **Dangerous advice removed** from:
  - `QUESTIONS_AND_ISSUES.md` (Heights #7)
  - `REVIEW_final_2026-10-04.md` (walkthrough step 10)
  - `stage13_heights.md` §3
  - `ORDER_CHECKLIST.md`
  - `stage2_electrical.md`
  - `pins_final.h` (`MAG_PIN_ORDER`)

  It's replaced by a multimeter procedure on the real cable before gluing (`ORDER_WALKTHROUGH.md` → "Magnet piece"):
  1. On a phone charger, find the +5 V leg and check the inner legs read ~0 V.
  2. Unpowered, beep VBUS/GND to the outer USB-A contacts and D−/D+ to USB-A pins 2/3.
  3. Fit the +5 V leg in pin 1 and the GND leg in pin 4.
  4. Beep pin 4 to TP4.

**Verified:**
- Netlist (kicad-cli): J3 1 VBUS, 2 USB_DM, 3 USB_DP, 4 GND. U7 is unchanged (1/6 D+, 3/4 D−).
- Pad dump from the board matches. DRC 0/0/0 with parity. ERC 0.
- A render of the corner shows the silk marks and the GND thermal spokes on pad 4.
- Not changed: the magnet contacts still carry only VBUS/GND/D± (D1, Q2 and R20 are untouched).

---

## 2. CPL rotations (verification 04 M1/M2, FATAL)

**The finding.** JLCPCB places parts using EasyEDA footprints with different zero angles. Seven SOT parts and J3 would have gone on wrong.

**The fix (permanent):**
- New `tools/jlc_cpl.py`, run by `tools/make_outputs.sh`. The BOM is now generated first.
- It holds a rotation-offset table **per LCSC number**:

  | Offset | Parts |
  |---|---|
  | +180 | C15127 (Q1/Q2), C53099 (U4), C53100 (U5), C46061768 (J3) |
  | +270 | C424093 (U2), C841192 (U3), C7519 (U7) |
  | 0 (listed explicitly) | every other polarised part |

- It expands the BOM's "C32-C34" ranges, and refuses to write a CPL if a part has no LCSC number.

**The result in `fab/ai_calc_CPL_JLCPCB.csv`:**

| Part | Rotation |
|---|---|
| Q1 | 0 → 180 |
| Q2 | 90 → 270 |
| U2, U3, U7 | 0 → 270 |
| U4, U5 | 0 → 180 |
| J3 | 0 → 180 (the J3 change didn't move the footprint, so this still holds) |

It is identical, all 77 rows, to verification 04's hand-corrected CSV.

**Independent check** (`tools/check_cpl_easyeda.py`):
- Every polarised or multi-pin part's EasyEDA footprint was fetched with `easyeda2kicad --footprint` (scratch venv).
- Each one was placed at the CPL position and rotation with KiCad's own transform, then compared with the board's pads.
- **Negative test:** with KiCad's raw rotations, the same check fails Q1, Q2, U2, U3, U4, U5 and U7 (pin 1 2.1–3.1 mm off), so it really discriminates.
- **J3's direction:** its 4 pads are symmetric, so pad matching can't tell 0° from 180°. At 180° the EasyEDA courtyard sits at y 60.9–69.5, over the top edge like the KiCad body (61.9–70.4). At 0° it would be at y 74.5–83.1, over D1/D6/U7.

| Ref | LCSC | EasyEDA footprint | CPL rot | EasyEDA pin 1 → KiCad pad 1 (mm) | worst same-number pad offset (mm) | every EasyEDA pad on a KiCad pad | Result |
|---|---|---|---|---|---|---|---|
| D1–D6 | C8598 | SOD-123…-RD-1 | 0 | 0.05 | 0.05 | yes | PASS |
| D7 | C193402 | SOD-123FL…-RD | 180 | 0.18 | 0.18 | yes | PASS |
| J1 | C6364666 | FPC-0.5-24P-HYH2.0 | 180 | 11.50 | 11.50 | yes | PASS: pin numbers intentionally mirrored (CamReversed), geometry exact |
| J2 | C6364666 | FPC-0.5-24P-HYH2.0 | 270 | 0.00 | 0.00 | yes | PASS |
| J3 | C46061768 | HX-PM2.54-1X4PWT | 180 | 7.62 | 7.62 | yes | PASS: 4 identical contacts; body direction checked separately (above) |
| J4 | C295747 | S2B-PH-SM4-TB | 0 | 0.00 | 0.00 | yes | PASS |
| L1 | C135265 | SMNR4020 | 180 | 0.00 | 0.00 | yes | PASS |
| Q1 | C15127 | SOT-23…-BR | 180 | 0.21 | 0.21 | yes | PASS |
| Q2 | C15127 | SOT-23…-BR | 270 | 0.21 | 0.21 | yes | PASS |
| Q3 | C469327 | SOT-323…-BR | 0 | 0.00 | 0.00 | yes | PASS |
| U1 | C3013941 | ESP32-S3-MINI-1 | 90 | 0.02 | 0.03 | yes | PASS |
| U2 | C424093 | SOT-23-5…-BL | 270 | 0.01 | 0.01 | yes | PASS |
| U3 | C841192 | TSOT-23-5…-BL | 270 | 0.01 | 0.01 | yes | PASS |
| U4 | C53099 | SOT-23-5…-BR | 180 | 0.16 | 0.16 | yes | PASS |
| U5 | C53100 | SOT-23-5…-BR | 180 | 0.16 | 0.16 | yes | PASS |
| U6 | C138713 | WQFN-24…-TL | 0 | 0.08 | 0.08 | yes | PASS |
| U7 | C7519 | SOT-23-6…-BL | 270 | 0.01 | 0.01 | yes | PASS |

The residuals of 0.05–0.21 mm are only pad-size differences between the two libraries.

**One caveat:** EasyEDA's API started returning HTTP 403 after the first batch, so all footprints came from one download into one folder. They were mapped to LCSC numbers by package name, which is unique except that U4 and U5 share the same "-BR" SOT-23-5. That matches verification 04's per-number download.

---

## 3. SW1 key pad vs hole H3 (verification 04 M3)

**The finding.** SW1's COL0 bottom bar (6.0 × 0.6 mm, B.Cu) was **0.055 mm** from the 6.0 mm NPTH hole H3. JLCPCB wants ≥ 0.2 mm, and the brief asked for ≥ 0.25 mm. KiCad's DRC doesn't check this pair.

**The fix.** New footprint `ai_calc:KeyPad_6.0x4.5_H3notch`, used by SW1 only (schematic and board):
- The 1.7 mm of the bar nearest H3 is thinned from 0.6 to 0.35 mm, with its edge pulled away from the hole. The rest of the bar and all 8 fingers are unchanged.
- The carbon dot (about 4.5 mm on this row, centred on the key) only reaches the bar near its middle. The removed sliver (x 173.6–175.3, y 122.8–123.0) is outside the dot, so the contact area is unchanged.

**Verified:**
- New `tools/check_hole_clearance.py` measures every copper item (pads, tracks, vias and zone fill, both layers) against every hole edge: NPTH against any net, plated holes and vias against other nets.
- Before the fix: 1 violation (H3/SW1 0.055 mm). That reproduces verification 04 exactly.
- After the fix: **0 violations board-wide**. Minimums: H1, H3, H4, H6, H8, H10 at 0.250 (the zone-fill rule); H2, H9, H13, H14 at 0.300.
- DRC is clean, including the library-footprint match.

---

## 4. Recovery procedure (verification 02 L1) and the R17 comment

**The finding.** "Hold BOOT to GND while plugging in" doesn't work while the battery is connected. The battery keeps 3.3 V and EN up, so plugging the cable in never resets the chip.

**The fix.** The procedure is corrected in `firmware/FIRMWARE_STAGE13.md` (step 3), `hardware/pins_final.h` and `hardware/stage2_electrical.md`:
1. Hold **TP1 (BOOT) → TP4 (GND)**.
2. Tap **TP7 (EN) → GND** for about 0.2 s (or unplug the battery at J4 first).
3. Release TP1 and flash.
4. Tap TP7 again to run the new firmware.

Also added:
- The UART backup on **TP2/TP3**.
- **"Never burn the security / USB-disable eFuses"** (DIS_USB_SERIAL_JTAG, DIS_DOWNLOAD_MODE, ENABLE_SECURITY_DOWNLOAD, secure boot, flash encryption).

**R17 discrepancy.** The brief (from verification 02 C12) said R17 is 10 k on the board. It isn't:
- The schematic, the BOM (C25741, "100k") and the board footprint all say **100 k**.
- Verification 03 agrees.

`pins_final.h` already said 100 k, so it already matched the board. The comment now names R17 and its LCSC number; no value was changed.

**Verified:** text review. R17 was confirmed in the schematic file, the netlist and the BOM.

---

## 5. fx-115ES key names (verification 03 L2)

**Checked against the photo** (`e59eff71`: the face plate with the keys out, so the SHIFT legends are readable):
- Row 2: SOLVE over **CALC**, d/dx over **∫dx**, x! over x⁻¹, Σ over log□□.
- Row 3: ∛ over √, x³ over x².
- Row 4: Abs over hyp.
- Row 5: STO over RCL, ← over ENG, % over "(".

**Hardware fix** (no wiring change, SW4 and SW5 stay at R1C6 and R1C7):
- SW4 value "Abs" → **"CALC"**, SW5 "x^3" → **"∫dx"**. Changed in `keypad.kicad_sch`, on the board (KEYPADS user-layer labels too), in `geometry/keys.csv` and in `derive_geometry.py`.
- There are no key labels on the silkscreen (B.Silk stays empty over the key pads).

**Firmware fix:**
- `core`: `DKey::Abs` → `DKey::Calc` and `DKey::Cube` → `DKey::Integral`. `keys.cpp kMatrix` keeps the same positions.
- **CALC / SHIFT SOLVE / ∫dx / SHIFT d/dx are not implemented.** They now show a "CALC (or SOLVE, ∫dx, d/dx) — Not supported yet — Press [AC] key" notice instead of doing something else.
- SHIFT layer per the 115ES:
  - **SHIFT hyp = Abs**
  - **SHIFT x² = x³**
  - **SHIFT √ = ∛**
  - STO, ←, %, comma, M−, x!, ˣ√, 10ˣ, eˣ, nPr, nCr: already the same.
- Still ignored, as before: ∠, ←, a b/c⇔d/c, Σ.
- The serial `b` shortcut is now SHIFT hyp.
- The web simulator page labels are updated. The built simulator files and the Windows exe need a rebuild (no Emscripten or MinGW here).

**Verified:**
- New test lines for SHIFT hyp → |−7| = 7, SHIFT x² → 2³ = 8, SHIFT √ → ∛27 = 3, and both notices.
- **548 passed, 0 failed.** The only golden change is the new `notice_calc` snapshot; the md5 of every other golden file is unchanged.
- `pio run` for `firmware-prototype`: clean build, 197 files compiled, **0 warnings**.
- The `firmware/` tester builds with 0 warnings, and the Windows simulator compiles.

---

## 6. Ribbon lengths (verification 03 L1 / L3)

No measurements had arrived yet: `measurements_2026-10-03.md` and QUESTIONS were checked. So I asked whether each suggested move is safe in **both** the short and the long case.

**J1, 2 mm towards the camera: not safe both ways, so left where it is.**
- **Short ribbon** (verification 03's reading: about 62 mm past the module, 1.4 mm spare): the move would help. But the camera isn't soldered, so the module can instead sit up to about 1.7 mm towards J1 inside its 12 × 12 keep-out, and the 7 mm window gets drilled after taping.
- **Long ribbon** (70.5 mm past the module): the move adds 2 mm to 10–12 mm of slack. A flat FPC can't bend sideways in the board plane, and an arch of that length would need several mm of height under ribs that leave 2 mm.
- The move would also mean re-routing the densest 0.5 mm-pitch fan-out on the board.
- **Decision rule:** measure the module from its far edge to the tip. If it's under about 70.3 mm, shift the module towards J1 (≤ 1.7 mm) and drill the window over the lens where it ends up.

**J2, 1 mm away from the slot: not safe both ways, so left where it is.**
- The ribbon length budget has no spare (14.3 = 14.3, stage 4). Moving J2 out would make the nominal and short ribbons too short. (Review 07, 2026-10-06: the Fusion path from the glass edge into J2 is 12.7 mm with 2 mm inside J2, i.e. about 1.6 mm nominal spare and 0.3 worst case; the no-spare rule stands.)
- **Decision rule** (measure the stiff end and E7, glass edge → tip):

  | Stiff end | E7 | What happens |
  |---|---|---|
  | ≤ 5 mm | any | no change |
  | > 5 mm | ≥ 15.3 mm | J2 moves +1 mm (board change before ordering) |
  | > 5 mm | < 15.3 mm | tell Claude before ordering |

The camera-cable text in `stage13_heights.md` §4 ("10 mm to spare, gentle S") is corrected.

---

## 7. Order documents

**`ORDER_WALKTHROUGH.md`** (new, replaces the walkthrough in REVIEW_final) covers:
- the corrected CPL;
- an expected-look table for every polarised part;
- **edge rails on the right and bottom edges only**;
- **"J1 pin numbering is intentionally reversed, don't rotate"**;
- the order-notes text to paste;
- an answer table for JLCPCB engineering emails (overhang intended, OK to pull back copper, J1, D7, slots, drill maps, substitutes);
- the expected cost: **about $135–160 before shipping, $165–225 delivered**;
- the after-arrival meter checks, including the magnet procedure.

Also:
- `ORDER_CHECKLIST.md` is rewritten to match.
- `REVIEW_final_2026-10-04.md` now carries a stage-14 note, and its old walkthrough is replaced by a pointer.

---

## 8. Regenerated outputs

**Extra fix found while regenerating: BOM designator ranges.**
- kicad-cli wrote ranged designators into the JLCPCB BOM ("C32-C34", "C5-C7", "C21-C30", "D1-D5"). JLCPCB expects plain comma lists, so those parts could have shown up as "not in BOM".
- `make_outputs.sh` now passes `--ref-range-delimiter ''`.
- Verified: the BOM lists 77 designators, exactly the CPL's 77.
- The stale copy `bom/ai_calc_BOM_JLCPCB.csv` (still showing the old 5 mm J3, C42379197) is refreshed from `fab/`.


- `fab/` via `tools/make_outputs.sh`: gerbers, drills, BOM, the corrected CPL, the schematic and assembly PDFs, `ai_calc_board.step` and the renders.
- `fitcheck/` via `tools/make_fitcheck.py`.
- `kicad/DESIGN_SUMMARY.md` via `kicad_summary.py`.
- Silk "AI CALC v14 2026-10-04".
- `Claude outputs/ai_calc_pcb_14.zip`.
- graphify updated.

## Left open
- The paper dry fit of the post holes (unchanged from stage 13).
- The ribbon measurements (decision rules above).
- The magnet meter check when the parts arrive (it's mandatory).
- The 85 % confidence in Adafruit's cable drawing (the meter check catches it if the drawing is wrong).
- Rebuilding the simulators.
