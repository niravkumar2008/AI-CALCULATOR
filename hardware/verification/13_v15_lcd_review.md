# 13 — Independent review of the v15-LCD board, re-derived from the raw files

Date: 2026-10-08. Scope: `hardware/kicad_v15_lcd/` and `hardware/fab_v15_lcd/` as left by stage 15 (14:14 board), plus the CAD requests in `enclosure/final_assembly_v15_lcd/REPORT.md` §5. Method as in reviews 01–08/11: every number below was re-derived from the `.kicad_pcb` / `.kicad_sch` (parsed with scripts, never loaded whole), `kicad-cli` 10.0.6, `easyeda2kicad` fetches of the JLCPCB footprints, and the vendor documents cited in §2. `hardware/kicad/`, `hardware/fab/` and `hardware/archive/` (v14) were only read (`git status` on them: clean). **Nothing was committed.** Scripts: `hardware/tools/v15_lcd/board_step3_review13.py` (the board changes, re-runnable after `board_step2_route.py`); throw-away parsers stayed in the scratchpad.

## Verdict

**As received (14:14 board): DRC/ERC/parity 0 and the netlist is right pin-for-pin against the vendor's 4-SPI circuit, but J5 was placed so that panel finger 1 arrives at the WRONG END of the socket (the J1 camera story again), and the 36.6 mm tail had no sensible route. Both are fixed on the board now (J5 numbering mirrored + GPIO roles swapped; slot and J5 moved as the CAD asked). After the changes: ERC 0, DRC 0/0/0, parity 0, holes 0, CPL 20/20 PASS, outputs regenerated. Still do NOT order v15: the pin-1 end must be confirmed with the diode test on a real tail (§S1), the backlight current is a guess until a panel is measured (§M1), and the CAD model has to be rebuilt from the new board STEP.**

---

## 1. Findings

### FATAL — none

### SERIOUS

**S1 — Panel finger 1 lands at the bottom of J5, pad 1 was at the top (FIXED on the board, bench confirmation still required).**
Evidence: the vendor drawing (Unvision/Newvisio `SPEC N190-1732TBWPG01-C30 VER B`, p.5, mirrored at https://resource.heltec.cn/download/HT-VMT190/SPEC%20N190-1732TBWPG01-C30%20VER%20B.pdf) shows the display-face ("FULL VIEW") drawing with the tail hanging down and the fingers labelled **"30 … 1" left to right**, i.e. finger 1 at the right-hand edge of the tail when you look at the picture with the tail down. The panel sits on the key side (B) with the picture facing the window and the tail toward board +x. Seen from the B side, +x is to the viewer's left, so the picture is rotated 90° clockwise relative to the drawing: finger 1 goes to the viewer's bottom = board **+y (large y)**. The tail then goes through the slot and folds 180° about a y-parallel axis; neither the slot nor the fold changes the y-order of the fingers (dual-contact handles the *face*, not the *end*). So finger 1 arrives at y ≈ 100, the bottom of J5, while the 14:14 board had pad 1 at y 85.85 (top). REPORT.md §5.4 ("pin 1 is unaffected either way, J5 is dual-contact") is wrong for the same reason. Consequence if left: display dark, no damage (3.3 V signals land on DB/GND pins), fix by a half twist — exactly as the designer wrote in stage15 §6, but with the drawing in hand the default should be the correct end.
Fix applied: new footprint `ai_calc:FPC_30P_P0.5mm_DualContact_C2919501_LcdReversed` (identical geometry, pad N at the position of socket contact 31−N; `kicad_sym`, `lcd.kicad_sch` and `make_lcd_sheet.py` updated). Pad 1 is now at (157.4, 100.35), pad 30 at (157.4, 85.85). Because the five B.Cu SPI lanes keep their west→east order (U1 IO5, IO6, IO8, IO41, IO42) and now meet the pads in the opposite order, the GPIO **roles** were swapped in `mcu.kicad_sch` so the copper stays crossing-free: **IO5 = LCD_MOSI, IO6 = LCD_DC, IO8 = LCD_SCK (unchanged), IO41 = LCD_CS, IO42 = LCD_RST**; `pins_v15_lcd.h` follows (the `firmware-v15-lcd/` tree includes that header, so it picks the change up). J5 pad N = panel pin N is unchanged electrically. CPL: C2919501 keeps rotation offset 0; the EasyEDA check now treats J5 like J1 (numbers mirrored on purpose, geometry only, 14.50 mm "by number", 0.00 mm geometry).
Residual: the drawing does not say in words which face carries the fingers (it is read from the front/rear views: fingers drawn on the front view, stiffener hatched on the rear view). **Do the diode test from stage15 §6 on the first real tail: the LED drop sits between finger 20 and fingers 21–24, i.e. 7–11 fingers from the pin-30 end. With the new board the LED fingers must be at the TOP half of J5 (y ≈ 88.9–90.9) and finger 1 at the bottom.** If it comes out the other way, the half twist fixes it (there is now ~7 mm of spare tail), or swap the footprint back.

**S2 — Ribbon ~19 mm too long for the 14:14 route (FIXED: route lengthened as REPORT.md §5 asked).**
Spec tail 36.6 ± 0.3 mm (p.5). Old route (CAD) 17.5 mm. Applied: slot x 177.4–178.4 → **180.5–181.5** (centre 181.0, y 83.1–103.1 unchanged, 1.0 × 20 mm; 2.15 mm to the board edge at x 183.65; REPORT says 3.45 mm to the rib); J5 (165.0, 93.1) → **(158.6, 93.1)**, rot 90, body x 156.73–160.53 (the camera-module limit is "east of 156.5", which allows 6.4 mm, not the 7 mm the report hoped for), mouth at x 160.53 facing the slot. New route: glass edge 176.2 → slot 181.0 (4.8) + through the 1.6 mm board and two bends (~2.5) + 181.0 → mouth 160.53 (20.5) + 2.0 mm insertion ≈ **29.8 mm**, i.e. ≈ 6.8 mm spare, one gentle bow instead of a Z-fold. The strip x 160.6–180.5, y 85–101.2 has no parts (checked: 0 footprints in it; the KEEPOUTS rectangle and text were updated). **`build_final_assembly_v15.py` must be re-run on the new `fab_v15_lcd/ai_calc_v15_lcd_board.step`** to confirm the bow height and the camera-module clearance; I did not run the CAD.

### MINOR

**M1 — Backlight current is a Vf lottery with a 22 Ω resistor from 3.3 V.** Spec p.5/p.9: four white LEDs in parallel, 60 mA total at **3.0–3.4 V** (15 mA/LED), 15–20 mA/LED recommended, constant-current driver (AW9364) recommended; LilyGO's T-Display-S3 drives the same panel with an AW9364 from a switched 3.3 V rail. (3.3 V − Vf)/22 Ω gives 27 mA at Vf 2.7 V, 14 mA at 3.0 V, 4.5 mA at 3.2 V — the "20–25 mA" in stage15 §3 assumes Vf ≈ 2.8 V at low current, which the spec does not give. Brightness could be anywhere from "fine indoors" to "barely lit". Not a board blocker; measure Vf on the real panel at 20 mA and pick R23 (10 Ω gives 60/30/10 mA at Vf 2.7/3.0/3.2 V, still ≤ the 60 mA rating), or use the vendor's AW9364DNR (C401007) if constant brightness matters. (Nirav's open question stage15 §9.3; this is the data behind it.)

**M2 — VDD = 3.3 V is the top of the panel's operating range** (spec p.10: VDD 2.4 / 2.8 typ / 3.3 V max, abs max 4.6 V). RT9080-33 is ±2 % → up to 3.37 V. LilyGO and Adafruit run the panel at 3.3 V too; accept, but do not raise the rail.

**M3 — stage15 §4 claims the new slot has "the same 0.3 mm keep-out ring as v14's slot"; it did not** (v14 had a second "edge keep-out" zone around its slot, v15's board had only the board-edge one). Nothing was wrong on the copper (nearest copper to the slot was exactly 0.300 mm, the `min_copper_edge_clearance` rule), so documentation only. A keep-out ring zone (tracks/vias, F+B, 0.35 mm) was added around the new slot so the board matches the text.

**M4 — Documents now stale after this review:** `stage15_lcd.md` §3 (GPIO table), §4 (J5 at 165.0, slot at 177.9, via columns, lane order), §6 ("pin 1 must go to the top of J5", silk tick at 162.5), §9.2/9.8; `enclosure/final_assembly_v15_lcd/REPORT.md` §5.4; `LCD_PANEL_OPTIONS.md` "no pin-order change" (true for pad N = pin N, but the physical end is mirrored). `kicad_v15_lcd/DESIGN_SUMMARY.md` was regenerated.

**M5 — Q4 is a hard switch into 4.8 µF (C19 + C36).** Charge-sharing dip on +3V3 ≈ 3.3 × 4.8/(44 + 4.8) ≈ 0.3 V for < 1 µs, far above the ESP32-S3 brown-out level (≈ 2.5 V); RT9080 recovers in µs. Acceptable; a 1 kΩ in series with the gate would soften it if the bench ever shows a reset on LCD power-up.

**M6 — Spec inconsistencies worth knowing:** p.6 calls pin 19 "NC" while the p.5 drawing calls it SDO (left open on the board, fine either way); p.5 note 7 says "2-chip LED, 40 mA" while the circuit diagram says four LEDs, 60 mA; luminance 350 (p.5) vs 650 cd/m² (p.11). The TFT1901 copy of the same drawing (LCD_PANEL_OPTIONS.md) says 80 mA. None of this changes the board.

## 2. The LCD interface, pad by pad (check 2)

Source: spec p.6 pin table and p.8 "4-SPI" reference circuit (S1 above), cross-checked with the LilyGO T-Display-S3 schematic (https://raw.githubusercontent.com/Xinyuan-LilyGO/T-Display-S3/main/schematic/T_Display_S3.pdf, CN1) and the Adafruit #5394 Eagle files (LCD_PANEL_OPTIONS.md). Netlist exported with `kicad-cli sch export netlist`; J5 pad N = panel pin N.

| Pin | Spec | J5 net | OK |
|---|---|---|---|
| 1, 25, 30 | GND | GND | ✓ |
| 2 | VDD 2.4–3.3 V (no separate IOVDD pin) | LCD_VDD (switched 3V3) | ✓ |
| 3, 4 | IM2, IM1: both = 1 → 4-line SPI (p.6); tied to VDD in the p.8 circuit | LCD_VDD | ✓ (LilyGO grounds them = 8080 8-bit, as expected for its design) |
| 5 | RESET, active low | LCD_RST | ✓ |
| 6 | CS | LCD_CS | ✓ |
| 7 | D/C in 8080 = **SCL** in SPI | LCD_SCK | ✓ |
| 8 | WR in 8080 = **RS (D/C)** in SPI | LCD_DC | ✓ |
| 9 | RD, "must be connected to ground when serial interface is selected" | GND | ✓ |
| 10 | SDA | LCD_MOSI | ✓ |
| 11–18 | DB0–DB7, grounded in the 4-SPI circuit | GND | ✓ |
| 19 | NC / SDO, open in both reference circuits | open (no-connect) | ✓ |
| 20 | LEDA | LCD_LEDA (← R23 22 Ω ← +3V3) | ✓ |
| 21–24 | LEDK1–4 | LCD_LEDK (→ Q5 drain) | ✓ |
| 26–29 | T / TP-SCL / TP-SDA / TP-INT, "let open" (no touch on this part) | open (no-connect) | ✓ |
| MP tabs | — | GND | ✓ |

Logic levels: VIH ≥ 0.7·IOVDD with IOVDD internal to the panel (from VDD): 3.3 V ESP32 outputs are fine. Dual contact confirmed by LCSC's attributes for C2919501 ("double-sided, top and bottom entry", flip lock, 0.3 mm FPC) and the HDGC 0.5K-HX series drawing (section A-A, upper and lower beams); the panel's fingers face the display side per the drawing, which the dual-contact socket makes irrelevant. Pin-1 orientation vs the ribbon: §S1.

## 3. Power (check 3)

- **Q4 AO3401A** (AOS datasheet rev 3.1): Vgs(th) −0.5/−0.9/−1.3 V; Rds(on) 60 typ / 85 max mΩ at Vgs −2.5 V, 47/60 at −4.5 V → ≈ 55 mΩ at −3.3 V. Drop at the panel's 50 mA "normal display" (spec p.10): 3 mV. Gate: R21 100 k to +3V3, IO33 drives it low to turn on. IO33 is **not** an RTC GPIO (RTC = IO0–21), so in deep sleep it goes high-impedance and R21 holds Vgs = 0 → off (threshold ≥ 0.5 V margin). At power-on before the firmware runs: same. Inrush: §M5.
- **Q5 AO3400A** (rev 3.1): Vgs(th) 0.65/1.05/1.45 V; Rds(on) 24/48 mΩ at 2.5 V → at 27 mA the drop is < 2 mV, irrelevant. R22 100 k pull-down, IO4 (RTC GPIO, PWM-capable LEDC) → off in sleep/reset. LED current: §M1 (4.5–27 mA at 100 % duty depending on Vf; the panel's 60 mA rating cannot be exceeded with 22 Ω).
- **Sleep:** Q4 off → panel unpowered (its own standby would be 20 µA, spec p.10) → LCD block = FET leakage, ≤ 1 µA each for Q4/Q5; R21/R22 carry nothing; R23 sits at 3.3 V with the cathodes open. ≤ 40 µA budget unaffected. The "drive all LCD GPIOs low before LCD_PWR_N goes high" rule (back-feed through ESD diodes) is correct and documented in the header.
- **3V3 budget:** Wi-Fi TX 355 mA (v14 figure) + panel 50 mA typ (spec) + backlight ≤ 27 mA = ≈ 432 mA < RT9080 600 mA. OK.

## 4. ESP32-S3 pins (check 4)

Netlist, U1 (ESP32-S3-MINI-1-N4R2, per the BOM value): pad 7 IO3 KEYPAD_INT, 8 IO4 LCD_BL_EN, 9 IO5 LCD_MOSI, 10 IO6 LCD_DC, 12 IO8 LCD_SCK, 28 IO33 LCD_PWR_N, 37 IO41 LCD_CS, 38 IO42 LCD_RST (roles after §S1).
- **IO3** is a strapping pin only when the `EFUSE_STRAP_JTAG_SEL` eFuse is burned (JTAG source select); the project never burns eFuses, so it is a plain GPIO/RTC_GPIO3 here. At reset the TCA8418 INT is open-drain and idle high; the net has R15 10 k to +3V3 (unchanged from v14), so even with the eFuse burned the pin would read a steady 1 — no boot conflict. RTC GPIO → EXT1 wake works (IO3 and IO4 are both RTC pins; `pins_v15_lcd.h` static-asserts it).
- **PSRAM/flash/USB on the N4R2 (quad PSRAM):** IO26 (PSRAM CS) unconnected; IO33–37 are only taken by octal PSRAM (R8 modules), so IO33/35/36/37 are usable as v14 already did; USB D−/D+ on IO19/20 untouched; IO45/46 straps unconnected.
- **Camera map unchanged:** XCLK 18, D0 13, D1 11, D2 10, D3 12, D4 14, D5 16, D6 17, D7 21, VSYNC 36, HREF 47, PCLK 15, PWDN 48, RESET 38, SIOD 40, SIOC 39, PWR_EN 34 — confirmed from the v15 netlist; J1 footprint still `FPC_24P_P0.5mm_DualContact_C6364666_CamReversed`, pads 1–24 nets identical to v14.

## 5. Manufacturing (check 5)

- BOM ↔ CPL: 65 CPL rows, every designator in the BOM, none extra (J5/Q4/Q5/R21/R22/R23/C36 present, C19 under "4.7uF" C19666). Removed e-paper parts (J2, L1, Q3, R11, R12, D3–D5, C20–C30) are absent from netlist, BOM, CPL and board; no EPD_* net survives; the only single-pin nets are the intentional no-connects (J5 19/26–29, J1 1, U1 IO26/45/46, U3–U5 NC).
- LCSC (seen 2026-10-08 on lcsc.com): C2919501 HDGC 0.5K-HX-30PWB **2,240 in stock**, $0.36 at 5+; C20917 AO3400A **299,845**, $0.09. C15127, C23345, C25741, C1525, C19666 are JLCPCB basic parts (designer's stock figures not re-checked).
- CPL rotations re-derived two ways: my own placement of the EasyEDA pads at the CPL position/angle (J5 0.00 mm, Q4 0.21, Q5 0.06, R23 0.07 mm worst same-number offset), and the designer's `check_cpl_easyeda_v15.py` on the **regenerated** CPL: 20 PASS / 0 FAIL (`fab_v15_lcd/cpl_easyeda_check_v15.md` rewritten). `jlc_cpl.py` offsets unchanged (C20917 +180, C2919501 0, C23345 0); Q4 90→270, Q5 −90→90 after the re-placement.
- Fab files: the 14:19 gerbers were byte-identical to a fresh export of the 14:14 board (only the drill-file date lines differed); all outputs were regenerated at 17:12 from the final board (gerbers+drills zipped, BOM, CPL, schematic PDF, assembly PDF, STEP now including the J5 model, renders).
- Silk: title "AI CALC v15-LCD 2026-10-08"; pin-1 ticks re-drawn for J5 (bottom end, (155.5–156.1, 100.35)), Q4, Q5; DRC silk checks clean. Fiducials FID1–3 unmoved.
- Slot: 1.0 × 20 mm stadium (two lines + two arcs), copper ≥ 0.30 mm all round (re-measured after the move: 0.3005 mm, zone fill).

## 6. Diff vs v14 (check 6)

Footprints: removed C20–C30, D3–D5, J2, L1, Q3, R11, R12; added C36, J5, Q4, Q5, R21–R23; **moved: C19 only** (plus the new parts). Every other footprint (U1, U6, J1, J3, J4, charger, keys, holes, fiducials, test pads) is at the same position/rotation with the same pad nets. Edge.Cuts: only the ribbon slot differs (v14 1.0 × 14 mm at x 172.9; v15 1.0 × 20 mm, now at x 181.0); outline extents identical (114.212–185.858 × 61.987–210.743). Holes/NPTH: identical set. Zones: the two antenna keep-outs merged into one (118.4–119.85 × 85–109, as stage15 §1 says); key keep-outs unchanged; the old slot ring gone (§M3), a new one around the new slot. Schematic sheets camera/connectors/keypad/power differ from v14 only in the project name inside instance paths; lcd.kicad_sch replaces epaper.kicad_sch; mcu.kicad_sch differs only in the eight labels listed in §4. Netlist diff: exactly the 8 U1 pins above and C19 (+3V3 → LCD_VDD).

## 7. Changes made by this review (all under `hardware/kicad_v15_lcd/`, `hardware/fab_v15_lcd/`, `hardware/tools/`)

1. `ai_calc.pretty/FPC_30P_P0.5mm_DualContact_C2919501_LcdReversed.kicad_mod` — new, pad numbers 31−N, same geometry/model.
2. `ai_calc.3dshapes/FPC_30P_HDGC_0.5K-HX-30PWB.step` — installed (easyeda2kicad C2919501, the name the footprint already referenced); renders and the STEP export now show the socket, actuator on the pad side, mouth toward the slot.
3. `ai_calc_v15_lcd.kicad_pcb` via `tools/v15_lcd/board_step3_review13.py`: slot +3.1 mm; J5 re-placed with the mirrored footprint at (158.6, 93.1); Q4 (159.5, 103.9) r90, C36 (157.3, 102.8), C19 (155.6, 103.6), R21 (159.5, 106.3) in the south band; Q5 (157.3, 82.3) r270, R22 (156.9, 79.6), R23 (161.0, 80.6) in the north band; SPI escapes west of the pads to vias at x 154.9/155.6/156.3, B.Cu lanes x 153.0–154.4 to the bus (cut at the new junctions); GND bars/vias at the socket; LCD_VDD column x 159.0; +3V3 via (153.48, 94.3) → B.Cu y 94.3 → via (161.6, 94.3) → F.Cu column x 161.6 feeding R23 and Q4; LCD_PWR_N B.Cu y 94.85 → x 157.4 → R21/Q4; LCD_BL_EN B.Cu y 113.5 → x 162.2 → y 79.5 → Q5/R22; LEDA/LEDK bars; 10 GND stitch vias; slot keep-out ring; KEEPOUTS strip/text updated; silk ticks. Zones refilled.
4. `mcu.kicad_sch` — four global labels swapped (IO5 LCD_MOSI, IO6 LCD_DC, IO41 LCD_CS, IO42 LCD_RST).
5. `lcd.kicad_sch`, `ai_calc.kicad_sym`, `tools/v15_lcd/make_lcd_sheet.py` — J5 footprint name; the GPIO-role text.
6. `pins_v15_lcd.h` — PIN_LCD_MOSI 5, PIN_LCD_DC 6, PIN_LCD_CS 41, PIN_LCD_RST 42 (+ comment).
7. `tools/v15_lcd/check_cpl_easyeda_v15.py` — J5 treated like J1; `tools/jlc_cpl.py` — comment only.
8. `fab_v15_lcd/*` — all regenerated with `make_outputs_v15.sh`; `cpl_easyeda_check_v15.md` rewritten; `kicad_v15_lcd/DESIGN_SUMMARY.md` regenerated.
Not changed: anything under `hardware/kicad/`, `hardware/fab/`, `hardware/archive/`; `stage15_lcd.md`, `REPORT.md`, `LCD_PANEL_OPTIONS.md` (see §M4); the CAD model.

## 8. Check list

| # | Check | How | Result |
|---|---|---|---|
| 1a | ERC, all sheets, `--severity-all` | kicad-cli 10.0.6 | 0 (before and after) |
| 1b | DRC `--refill-zones --schematic-parity --severity-all` | kicad-cli | 0 errors / 0 warnings / 0 unconnected / 0 parity (before and after; interim runs had 1 clearance + 2 courtyard + silk items, all fixed in the script) |
| 1c | Copper to holes ≥ 0.25 mm, copper to slot/edge ≥ 0.30 | `check_hole_clearance_v15.py` (0 violations) + own stadium-distance script (0.3005 mm) | PASS |
| 2 | J5 pad-by-pad vs 190-1732TBWPG01 p.6/p.8, IM pins, RD/DB ties, SDO, LED pins, TP pins, dual contact, pin-1 end | netlist + vendor spec + LilyGO schematic + LCSC attributes | nets PASS; pin-1 end was mirrored → fixed (§S1), bench test still owed |
| 3 | Q4/Q5 switches, R23 current, PWM pin, sleep current, 3V3 budget | AOS datasheets, panel spec p.5/9/10 | PASS with §M1/M2/M5 notes |
| 4 | Changed GPIOs, IO3 strap/RTC, N4R2 pin conflicts, camera map, J1 CamReversed | netlist, ESP32-S3 datasheet knowledge, v14 netlist diff | PASS |
| 5 | BOM = CPL, stock, CPL rotations (own + designer's script on regenerated CPL), silk, fiducials, slot geometry, e-paper remnants | fab files, easyeda2kicad, LCSC | PASS (20/20) |
| 6 | Diff vs v14: footprints, Edge.Cuts, holes, zones, sheets, nets | own parser, both boards + both netlists | only the LCD/power/KEYPAD_INT area changed |
| 7 | CAD requests: slot → x 181.0, J5 ~7 mm west with body ≥ 156.5, cluster re-placed, re-routed, DRC 0, J5 3D model | `board_step3_review13.py`, renders | applied (6.4 mm west, body 156.73); CAD rebuild still owed |
| — | `hardware/LCD_PANEL_OPTIONS.md` | read | recommends the same panel and the same slot/J5 move; no further board change needed. Its "no pin-order change" holds for pad N = pin N; the physical pin-1 end is the §S1 correction. The 24-pin alternative (C2919499, Adafruit #4520) is not recommended there and would need a new socket, pin map and slot. |

## 9. What Nirav still has to do before ordering v15

1. Buy one panel (Adafruit #5394 or the AliExpress 30-pin bare panel per LCD_PANEL_OPTIONS.md) and do the diode test: LED fingers (20–24) must be at the **top** half of J5, finger 1 at the **bottom** (silk tick at (155.5–156.1, 100.35)). If not, half-twist the tail or tell the next session to swap the footprint back (20-minute change, script exists).
2. Measure the backlight Vf at 20 mA and choose R23 (22 Ω / 10 Ω / AW9364), §M1.
3. Re-run `enclosure/final_assembly_v15_lcd/build_final_assembly_v15.py` on the new board STEP (slot 181.0, J5 158.6) and check the tail bow and the camera-module gap (J5 body now 0.23 mm east of the 156.5 limit).
4. Update stage15_lcd.md §3/§4/§6 from §7 above (or let the next regeneration do it).
