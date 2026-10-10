# 14 — Final pre-order verification of the v15-LCD board (ER-TFT019-1 panel)

Date 2026-10-08 (evening). Scope: `hardware/kicad_v15_lcd/`, `hardware/fab_v15_lcd/`, the CAD fit in `enclosure/final_assembly_v15_lcd/`, `pins_v15_lcd.h`, `firmware-v15-lcd/`, against the panel Nirav will buy, **BuyDisplay ER-TFT019-1** (datasheet rev 2.0, `~/Downloads/ER-TFT019-1_Datasheet.pdf`, pages 5–9 rendered with pymupdf and read), the ST7789P3 datasheet, and the J5 socket drawing (HDGC 0.5K-HX, fetched from LCSC). Every number below was re-derived from the raw `.kicad_pcb` / `.kicad_sch` / gerbers / netlist with scripts, not taken from stage15_lcd.md or review 13. v14 (`hardware/kicad/`, `hardware/fab/`, `hardware/archive/`) was only read. **Nothing was committed.**

## Verdict

**ORDER** — after the two changes made in this pass (R23 22 Ω → 15 Ω; SW1/SW2/SW50 key pads moved per verification 15), the board is clean: ERC 0, DRC `--refill-zones --schematic-parity --severity-all` **0 errors / 0 warnings / 0 unconnected / 0 parity**, copper-to-hole ≥ 0.25 mm (0 violations), copper-to-slot 0.30 mm, CPL rotations 20/20 PASS against freshly fetched EasyEDA footprints, every LCSC part in JLCPCB stock, fab files regenerated from the final board and byte-checked. J5's pad order matches the ER-TFT019-1 pin table pin for pin, and the ER-TFT019-1 outline drawing confirms (independently of the Unvision drawing used in review 13) that finger 1 arrives at the **bottom** of J5 (pad 1, y 100.35) with the gold fingers facing **up** (away from the board), which suits both the dual-contact HDGC socket and the "top contact" socket the panel maker specifies. Nothing found that needs a panel in hand *before* ordering; the three bench checks in §6 are all recoverable on this board if they come out differently.

## 1. Findings

### FATAL — none.

### SERIOUS — none open. (Two things were changed; neither would have broken the board, see §3.)

### MINOR

- **M1 — Backlight current was a lottery with 22 Ω (fixed: R23 = 15 Ω, LCSC C22810).** ER-TFT019-1 §4.4: VLED 2.8 / 3.0 / 3.2 V (min/typ/max) at 60 mA, ILED max 80 mA. With a Vf-vs-I model for 4 parallel white LEDs (Vf = Vf60 + 0.075·ln(I/15 mA) + 4 Ω·(I−15 mA) per LED, which gives 2.88 V at 20 mA total for a 3.0 V part) and Q5 Rds(on) 0.05 Ω:

  | R23 | Vf60 = 2.8 V (min) | 3.0 V (typ) | 3.2 V (max) | hard bound (flat Vf 2.6 V, rail 3.37 V) |
  |---|---|---|---|---|
  | 22 Ω (was) | 26.9 mA | 19.3 mA | 12.1 mA | 35 mA |
  | **15 Ω (now)** | **37.1 mA** | **26.3 mA** | **16.1 mA** | **51 mA** |
  | 12 Ω | 44.6 | 31.3 | 18.9 | 64 mA (over the 60 mA rating) |
  | 10 Ω | 51.7 | 36.0 | 21.5 | 77 mA (over) |

  15 Ω is the only value that puts the typical panel in the 20–35 mA "fine indoors" band and cannot exceed 60 mA even with a pessimistic 2.6 V Vf and a 3.37 V rail (10 Ω and 12 Ω can). R23 dissipation ≤ 39 mW (0603 = 100 mW). 3V3 budget: Wi-Fi 355 + panel IDD 20 mA max (ER §4.3; the old doc assumed 50) + backlight ≤ 51 mA worst = **426 mA < 600 mA** RT9080. C22810 is a JLCPCB **extended** part (314,584 in stock, $0.0013 + the one-off ~$3 extended fee); no basic-library 15/16/18 Ω 0603 exists (checked: 0603WAF150/160/180JT5E are all extended; 10 Ω C22859 and 20 Ω C22950 are basic but 10 Ω breaks the 60 mA bound and 20 Ω is barely better than 22). Dimming is PWM on IO4, so the firmware's default 60 % duty still applies.

- **M2 — VDD 3.3 V is the top of the ER-TFT019-1 operating range** (2.4–3.3 V, abs max 4.6 V). RT9080-33 tolerance puts it at ≤ 3.37 V: within abs max with 1.2 V to spare, 70 mV over the "operating max". Same as review 13 M2; Adafruit #5394 and LilyGO drive this glass at 3.3 V. Accept; do not raise the rail. (Bench: if the panel misbehaves at 3.3 V, a 2.8 V LDO in place of Q4 is a one-part change on the next spin; nothing on this board prevents it.)

- **M3 — Key pads SW1/SW2/SW50 were 1.1–1.5 mm off the Casio contact positions** (verification 15). Moved in this pass (§3). SW1 is back on the plain `KeyPad_6.0x4.5` footprint (no H3 notch): its nearest copper (finger corner at (174.9, 121.45)) is 4.74 mm from the H3 centre, i.e. **1.74 mm copper-to-hole** (bus-bar corner 1.41 mm), far above the 0.25 mm rule.

- **M4 — Tail is ≈ 0.5 mm off the panel's centre line, towards finger 1 (+y)**, measured on the ER drawing (p.6, pixel measurement: 4.9 mm margin on the 30 side, 6.0 mm on the 1 side; N.T.S. drawing, ±0.3 mm). The CAD modelled it centred. The 20 mm slot leaves 2.25 mm each end, so 0.5 mm is absorbed, and the skew from slot to J5 over 20 mm is 1.4°. No change; the `lcd_position_jig_v15.svg` is still right (it positions the panel, not the tail).

- **M5 — Documents now stale** (not changed here): `stage15_lcd.md` §3/§4/§9.3 (R23 22 Ω, "20–25 mA", 50 mA panel current), `LCD_PANEL_OPTIONS.md` last table row ("R23 = 10 Ω is the likely fix"), `enclosure/final_assembly_v15_lcd/REPORT.md` (key-pad positions in `board_snapshot.json`/`geometry.json` are the old ones; copper only, no 3D effect), `tools/check_requirements_11.py` (expects the H3-notch footprint on SW1: a v14 requirement that no longer applies to v15), the PCB title-block comment said "AW9364 backlight" (fixed: "R23/Q5 switched backlight"). `verification/15_keypad_vs_casio.md` says "nothing edited": true when it was written.

- **M6 — ER-TFT019-1 pin 19 / 21 / 23 / 24 are "NC"** (the Unvision drawing calls them SDO / LEDK1–4). On the board 21/23/24 are tied to `LCD_LEDK` (Q5 drain) and 19 is open: harmless on the ER module (no internal connection) and correct on the Unvision/LilyGO glass (four cathodes). Pins 26–29 (CTP) open: correct for the no-touch part.

## 2. Check list (all PASS unless marked)

| # | Check | How | Result |
|---|---|---|---|
| 1a | Board files vs fab outputs | regenerated to a temp dir with `make_outputs_v15.sh`, diffed: 14 gerber/drill files differed only in the 2 date lines each; BOM and CPL byte-identical | PASS (before changes); after the changes the outputs were regenerated again (21:38) and re-diffed: B.Cu/B.Mask/B.Silk differ only in the key-pad region + B.Cu pour, F.Cu geometry identical after aperture normalisation, BOM differs only in the R23 row, CPL identical |
| 1b | Board vs the STEP the CAD used (17:27) | `fab` STEP (17:12) and `board/` STEP (17:27): same size, file header date differs, **CARTESIAN_POINT set identical (15,153 points)**; after this pass's changes the 21:38 STEP still has the identical point set (the changes are copper and a resistor value: no 3D body moved) | PASS, CAD fit conclusions hold |
| 1c | ERC `--severity-all` | kicad-cli 10.0.6 | 0 (before and after) |
| 1d | DRC `--refill-zones --schematic-parity --severity-all` | kicad-cli | **0 / 0 / 0 / 0** (before and after) |
| 1e | Copper to hole / slot / edge | `check_hole_clearance_v15.py` 0 violations (min 0.25–0.30); own stadium-distance script: nearest zone fill 0.3005 mm from the slot, nearest track/via end 5.46 mm; DRC edge clearance 0.30 | PASS |
| 2a | J5 pad N ↔ ER-TFT019-1 pin table (p.8–9) | netlist export, all 30 pads: 1/25/30 GND, 2 LCD_VDD, 3 IM2 + 4 IM1 = LCD_VDD (datasheet: IM1=1, IM2=1 → 4-line SPI), 5 LCD_RST, 6 LCD_CS, 7 LCD_SCK (D/C = SPI-SCL), 8 LCD_DC (W/R = SPI-RS), 9 RD = GND ("must be connected to ground when serial"), 10 LCD_MOSI (SDA), 11–18 D0–D7 = GND, 19 NC open, 20 LCD_LEDA, 21–24 LCD_LEDK, 26–29 open, MP tabs GND | PASS |
| 2b | Which end finger 1 is on | ER p.6 FULL VIEW (display face, drawing rotated upright): tail at the bottom, "30" at the left, "1" at the right (= "tail to the right: 1 top, 30 bottom"). Panel face-up on the key side (B), display toward the window, tail toward +x: seen from the B side +x is to the viewer's left, so the drawing is rotated 90° cw → finger 1 to the viewer's bottom = **large y**. Down the slot (x 181.0) and the U-bow back west do not change the y order. At J5 (pad row x 157.4, mouth x 160.53 facing east) finger 1 arrives at y ≈ 100.35 = **pad 1 (157.4, 100.35), silk tick (155.5–156.1, 100.35)**; LED fingers 20–24 at y 88.85–90.85 (top half). Same conclusion as review 13, from an independent drawing | PASS |
| 2c | Which face the fingers are on | ER p.6: fingers drawn on the FULL VIEW (display face); the rear view shows the hatched 4.5 mm stiffener and the "A K" backlight pads, mirrored neck. Fingers face −z on the key side → after the 90° bend into the slot they face +x → after the bow they face **+z (up, away from the board)** at J5. The HDGC 0.5K-HX section A-A shows **upper and lower contact beams (true dual contact**, LCSC: "double-sided contacts, top and bottom entry"), so either face works; a top-contact socket (what ER's own ER-CON30HT-1 is) would also work | PASS |
| 2d | Socket geometry | HDGC drawing: 1.0 mm high, 0.5 mm pitch, front insert / rear flip ("前插后掀盖"), housing depth 2.90 mm + actuator (3.8 overall), 0.3 mm FPC, 0.5 A per contact. SMD leads at the rear under the actuator = the pad row at x 157.4; pegs (MP tabs) at the front corners near the mouth = x 159.8. The 4.5 mm stiffener is longer than the ~2–2.4 mm insertion: 2 mm stiff tip inside (as the CAD models), the rest outside the mouth — normal. Actuator flips up/back over the pads toward the camera module, which ends at x 154.25 (2.4 mm gap) and is operated with the case open; back cover 3.92 mm above J5 when closed | PASS |
| 3a | IM1/IM2, RD, D0–D7, NC pins | §2a, §M6 | PASS |
| 3b | VDD, logic levels, reset | VDD 3.3 V (§M2); VIH 0.7·VDD = 2.31 V vs ESP32-S3 VOH ≥ 2.64 V; VIN max VDD+0.3 → the "SPI lines low before LCD_PWR_N goes high" rule in `pins_v15_lcd.h` is required and present in `lcd.cpp`/`power.cpp`; RESET on IO42 (non-RTC: floats in sleep, but the panel is unpowered then) | PASS |
| 3c | Q4 high-side switch | AO3401A Vgs(th) −0.5…−1.3 V, ≈ 55 mΩ at −3.3 V → 1 mV drop at 20 mA; R21 100 k to +3V3 keeps it off at reset and in deep sleep (IO33 floats; not an RTC pin); inrush into C19 + C36 = 4.8 µF: ≈ 0.3 V/<1 µs dip on 2 × 22 µF, above brown-out | PASS |
| 3d | Backlight | §M1; Q5 AO3400A R22 100 k pull-down, off at reset/sleep; IO4 RTC/LEDC | PASS (changed) |
| 3e | 3V3 budget | 426 mA worst < 600 mA | PASS |
| 4a | GPIO map schematic ↔ `pins_v15_lcd.h` ↔ firmware | netlist U1: IO3 KEYPAD_INT, IO4 LCD_BL_EN, IO5 LCD_MOSI, IO6 LCD_DC, IO8 LCD_SCK, IO33 LCD_PWR_N, IO41 LCD_CS, IO42 LCD_RST, IO1/2 I2C, IO7 KEY_ON, IO9 VBAT_SENSE, IO35 CHG_STAT, IO37 VBUS_SENSE, IO19/20 USB, IO0 BOOT, IO43/44 UART; header identical; `firmware-v15-lcd/src/pins.h` includes the header and the sources use only `PIN_*` constants (grep: 18 names, no bare GPIO numbers); `lcd.cpp` maps pin_sclk/mosi/dc/cs/rst to PIN_LCD_SCK/MOSI/DC/CS/RST; wake mask = PIN_KEY_ON | PIN_KEYPAD_INT (EXT1, both RTC) | PASS |
| 4b | Strapping pins at reset | IO0: TP1 only, internal pull-up → SPI boot. IO3: R15 10 k to +3V3 + TCA8418 open-drain INT idle high → 1 (and only a strap if the JTAG eFuse is burned, which it is not). IO45/IO46: unconnected, internal pull-downs → 3.3 V flash / normal boot. EN: R1 10 k + C4 | PASS |
| 4c | Camera map, J1, USB, PSRAM | netlist: XCLK 18, D0 13, D1 11, D2 10, D3 12, D4 14, D5 16, D6 17, D7 21, VSYNC 36, HREF 47, PCLK 15, PWDN 48, RESET 38, SIOD 40, SIOC 39, PWR_EN 34; J1 = `FPC_24P_P0.5mm_DualContact_C6364666_CamReversed`, pads identical to v14; USB_D−/D+ on module pins 23/24 (IO19/20); IO26 unconnected (PSRAM CS on the N4R2); v14→v15 netlist diff = exactly the 8 U1 pins above + C19 (+3V3 → LCD_VDD) + the removed/added LCD parts | PASS |
| 5a | BOM ↔ CPL ↔ schematic | 65 CPL rows, every designator in the BOM, none extra, no e-paper remnant; the only single-pin nets are the intentional no-connects | PASS |
| 5b | Stock (JLCPCB API, 2026-10-08 21:30) | basic: C45783 3.8 M, C1525 20.8 M, C19702 11.8 M, C52923 8.2 M, C19666 2.3 M, C12530 4.9 M, C8598 428 k, C1002 612 k, C15127 724 k, C20917 890 k, C25744/25765/25741/26083/25900 2.3–19.6 M, C23345 8.7 M (no longer used). Extended: C193402 567 k, C6364666 16,788, C46061768 8,688, C295747 14,865, **C2919501 2,242**, **C3013941 ESP32-S3-MINI-1-N4R2 1,218** ($4.97), C424093 4,540, C841192 32,466, C53099 31,894, C53100 13,038, C138713 31,270, C7519 41,671, **C22810 314,584** (new). (LCSC retail shows 0 for C1525/C19666 — irrelevant, JLCPCB assembles from its own stock.) | PASS |
| 5c | CPL rotations | `check_cpl_easyeda_v15.py` on the regenerated CPL with EasyEDA footprints fetched **fresh** in this session (`easyeda2kicad` v1.0.1, 19 LCSC numbers): 20 PASS / 0 FAIL; J5 0.00 mm (numbers mirrored on purpose), Q4 0.21, Q5 0.06, U1 0.03, U6 0.08, D7 0.18, J1 11.50 (mirrored on purpose), J3 7.62 (unpolarised), J4 0.00, R23 (C22810 R0603) 0.07 | PASS |
| 5d | Fiducials, silk, slot/NPTH, finish | FID1–3 at (121.5, 203.5) / (178.5, 203.5) / (177.0, 112.5), unchanged from v14; silk pin-1 tick at J5 pad 1 (155.5–156.1, 100.35), Q4/Q5 ticks, title "AI CALC v15-LCD 2026-10-08"; slot = 2 lines + 2 arcs on Edge.Cuts (x 180.5–181.5, y 83.1–103.1), the 10 NPTH/PTH holes identical to v14 and present in the drill files; stackup 0.8 mm FR4, 2 layers, ENIG, as v14; rails: v14 review 04 M4 applies unchanged (ask JLCPCB for rails on the right + bottom edges, keep J3 and the antenna overhang free) | PASS |
| 6 | Diff vs v14 (`v14-order`) | footprints removed C20–C30, D3–D5, J2, L1, Q3, R11, R12; added C36, J5, Q4, Q5, R21–R23; changed C19 (moved, +3V3 → LCD_VDD), U1 (8 pad nets), **SW1/SW2/SW50 (moved this pass; SW1 footprint plain)**; all other 123 footprints identical in position, rotation, library and pad nets (incl. all holes H*, J1, J3, J4, charger, camera, TCA8418, remaining 47 keys, fiducials, test pads). Edge.Cuts: 139 items each, only the 4 slot items differ; extents 114.212–185.858 × 61.987–210.743 identical. Zones: antenna keep-outs merged, old slot ring gone, new slot ring (180.15–181.85 × 82.75–103.45) | PASS |
| 7a | Panel outline ER vs CAD | ER: 25.80 × 49.72 × 1.43 (±0.1; spec table rounds to 1.4), glass 24.8 × 48.52, V.A. 23.695 × 43.72, A.A. 22.695 × 42.72, tail 36.6 ± 0.3 × 15.5, 0.3 thick, 4.5 stiffener, 3 mm finger length — identical to the Unvision numbers the CAD used | PASS |
| 7b | Tail length vs the bow | CAD: 30.2 mm shortest route, 6.4 mm spare in the bow, 0.25 mm to rib B, ≈ 0.5 mm of bow depth per mm of tail. 36.9 mm worst case → 0.10 mm to rib B: still clears, but with no margin for a tail that is bent at more than 1.2 mm radius. Harmless if it touches (REPORT §4). Assembly step 10 (marker dot) is the check | PASS (bench check, §6) |
| 7c | Height stack | panel 1.43 (+0.1 max) under 1.27 mm of free space to the plate: OK; 0.1 mm tape | PASS |
| 7d | Mask opening | `window_mask_template_v15`: 43.72 × 23.70 = ER visual area 43.72 × 23.695 exactly (active + 0.5 each side); centred on the window | PASS (a panel placed > 0.3 mm off shows the V.A. bezel edge: use the jig) |
| 8 | Other | J5 contact rating 0.5 A ≥ 51 mA backlight; keypad: 49 unique ROW/COL crossings + ON on KEY_ON/GND, nets unchanged after the pad moves, `kMatrix` in `keys.cpp` unchanged (its comment now cites the v15 sheet) | PASS |

## 3. Changes made in this pass (all logged; v14 untouched)

1. **R23 22 Ω → 15 Ω, LCSC C23345 → C22810** (§M1): `kicad_v15_lcd/lcd.kicad_sch` (Value, LCSC, the backlight note text), `kicad_v15_lcd/ai_calc_v15_lcd.kicad_pcb` (R23 properties; title-block comment "AW9364 backlight" → "R23/Q5 switched backlight"), `tools/v15_lcd/make_lcd_sheet.py`, `board_step2_route.py`, `board_step3_review13.py` (so the board is still reproducible), `tools/jlc_cpl.py` (C22810: 0), `tools/v15_lcd/check_cpl_easyeda_v15.py` (C22810 → R0603), `kicad_v15_lcd/pins_v15_lcd.h` (comment), `bringup_guide_v15_lcd.html` (step 10: ÷ 15, expected 16–37 mA).
2. **Key pads (verification 15)** by the new script `tools/v15_lcd/board_step4_review14.py` (KiCad python, re-runnable after step 3): SW1 SHIFT (176.623, 120.795) → **(177.75, 119.80)**, footprint `KeyPad_6.0x4.5_H3notch` → `KeyPad_6.0x4.5` (comb bus-bar tracks re-created from SW3's pattern; ROW0 and COL0 feeds re-attached); SW50 ON (123.377, 120.795) → **(122.45, 119.55)** (comb tracks and KEYPADS outline/text moved with it; the old GND feed that ran up x 120.066 through the new KEY_ON fingers was deleted and replaced by a straight stub from the GND bar to the GND via at (121.139, 123.976); CAM_PWR_EN's B.Cu run at x 119.35 jogged to x 119.15 for y 116.6–123.0 to keep 0.35 mm to the moved pads, 0.22 mm to the GND via at (118.527, 120.945), > 2 mm to the chamfered edge); SW2 ALPHA (166.026, 120.795) → **(167.15, 120.80)** (comb tracks, outline, the three feeds incl. the COL1 via stub). `kicad_v15_lcd/keypad.kicad_sch`: SW1 Footprint field → `ai_calc:KeyPad_6.0x4.5` (parity 0). Pad sizes and every net unchanged (checked pad-by-pad before/after). `firmware-v15-lcd/src/keys.cpp`: the comment above `kMatrix` now cites `hardware/kicad_v15_lcd/keypad.kicad_sch`.
3. **Regenerated** `fab_v15_lcd/*` (gerbers + drills zip, BOM, CPL, schematic PDF, assembly PDF, STEP, renders) at 21:38 with `make_outputs_v15.sh`; `cpl_easyeda_check_v15.md` rewritten (20/20); `kicad_v15_lcd/DESIGN_SUMMARY.md` regenerated.
4. After every change: ERC 0, DRC 0/0/0/0, hole clearance 0, CPL 20/20 (all re-run on the final files).

Not changed: `stage15_lcd.md`, `LCD_PANEL_OPTIONS.md`, `REPORT.md`, review 15, the CAD model (§M5), anything in `hardware/kicad/`, `hardware/fab/`, `hardware/archive/`.

## 4. The J5 pin-1 derivation, step by step (check 2)

1. ER p.6 "FULL VIEW" is the display face (active area drawn, fingers drawn). Upright (tail hanging down) the fingers read **30 … 1 left to right**; the 3 ± 0.2 dimension at the tail end is the **finger length**, not a lateral offset. The rear view (stiffener hatched, "A K" LED pads) has the neck mirrored, confirming it is the back.
2. Board frame: KiCad +x right, +y down, viewed from the F (component) side. The panel is on the B side with the picture facing −z (the window). A B-side viewer sees +x to their left. The tail must point to board +x = the viewer's left → rotate the upright drawing 90° clockwise → finger 1 (right edge) goes to the viewer's bottom = **+y**. Finger 1 at y ≈ 93.1 + 7.25 − 0.5(offset, §M4) ≈ 100, finger 30 at ≈ 85.6, i.e. the tail spans y ≈ 83.7–98.7 inside the slot (83.1–103.1).
3. The tail bends 90° into the slot (direction +x → +z) and 180° under the board (→ −x). Both rotations are about y-parallel axes: the y order is unchanged. Finger 1 arrives at the **bottom of J5**. J5 (`_LcdReversed`, rot 90, at (158.6, 93.1)): pad 1 (157.4, **100.35**), pad 30 (157.4, 85.85), pads 20–24 at y 90.85–88.85, MP tabs (159.8, 84.85 / 101.35). **Match.**
4. Face: normal (−z on the key side) → +x in the slot → **+z** at J5 = fingers up, stiffener against the board. Dual contact: irrelevant; top-contact: also fine. The only way to get this wrong is a half twist in the tail, which the assembly guide forbids.

## 5. Backlight (check 3), numbers

Rail 3.3 V (3.37 max), R23 15 Ω, Q5 0.05 Ω: I = (Vrail − Vf(I)) / 15.05 Ω → **26 mA typical (Vf ≈ 2.90 V), 16 mA with a 3.2 V-max panel, 37 mA with a 2.8 V-min panel; ≤ 51 mA under any assumption**; the panel rating is 60 mA (80 abs). Measure on the first board: V(R23) / 15 (bring-up guide step 10). If a measured panel sits at the low end (< 18 mA) and is too dim, 12 Ω (C22791, extended) is the next step only if its measured Vf ≥ 2.75 V; otherwise the AW9364 driver.

## 6. Bench checks (none blocks the order)

| Check | When | Risk if skipped |
|---|---|---|
| **Pin-1 end: diode test on the first ER-TFT019-1 tail** (LED between finger 20 and 22, i.e. 11th and 9th finger from the pin-30 end; guide step 5). Expected: finger 1 at the J5 silk tick (bottom), LED fingers in the top half | before first power-up | if reversed (two independent drawings say it is not): display stays dark, nothing is damaged (3.3 V signals land on GND/D pins); fix = half twist of the tail (uses ~1 mm of the 6.4 mm spare) or swap the footprint on the next spin |
| **Tail length 36.6 ± 0.3** and the bow's clearance to rib B (assembly guide step 10, marker dot) | at assembly | a 36.9 mm tail leaves ~0.1 mm; a touch is harmless unless the closed cover squeezes it → grind zone 2 15 mm further (REPORT §4) |
| **Backlight current** (V across R23 / 15) and brightness | bring-up | too dim/bright only; 16–37 mA expected, cannot exceed 60 mA |
| **Panel at 3.3 V** (image quality, IDD ≤ 20 mA) | bring-up | if the panel objects, a 2.8 V LDO replaces Q4 on the next spin |
| Colour inversion / RGB order (`LCD_INVERT`, `LCD_RGB_ORDER` build flags) | bring-up | firmware flag only |
| Key feel on the moved SHIFT / ALPHA / ON pads (verification 15 method, or just press them) | first assembly | pads were 85–100 % under the pill before the move; now centred |

## 7. Files touched (absolute)

- `C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\kicad_v15_lcd\ai_calc_v15_lcd.kicad_pcb`, `lcd.kicad_sch`, `keypad.kicad_sch`, `pins_v15_lcd.h`, `DESIGN_SUMMARY.md`
- `...\hardware\fab_v15_lcd\*` (all regenerated 21:38), `cpl_easyeda_check_v15.md`
- `...\hardware\tools\v15_lcd\board_step4_review14.py` (new), `make_lcd_sheet.py`, `board_step2_route.py`, `board_step3_review13.py`, `check_cpl_easyeda_v15.py`; `...\hardware\tools\jlc_cpl.py`
- `...\hardware\bringup_guide_v15_lcd.html`; `...\firmware-v15-lcd\src\keys.cpp` (comment)
- this file. Nothing committed.
