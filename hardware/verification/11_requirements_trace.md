# 11 — Requirements trace: is everything we decided actually on the v14 board?

Date 2026-10-06 (docs-cleanup agent, pass 2). **Read-only on `hardware/kicad/`, `fab/`, `bom/`**: nothing there was changed. Every row below was checked by a script against the raw files (`hardware/kicad/ai_calc.kicad_pcb` parsed directly, U1 pin names from the `mcu.kicad_sch` symbol, `fab/ai_calc_BOM_JLCPCB.csv`), not copied from earlier docs. Re-run: `python hardware/tools/check_requirements_11.py` (parser `hardware/tools/kpcb_11.py`).

## Result first

**Nothing MISSING. Nothing DIFFERENT that reaches the copper, drill or silkscreen files. v14 stays GO.** 48 of 50 checks are PRESENT; the two DIFFERENT rows are on non-fabricated layers / metadata:

1. **Camera-window marker on the `User.3` (KEEPOUTS) layer is drawn Ø 6.0 mm** at (150, 95.099), while its label and every current doc say **7 mm**. The window is drilled in the shell, not the board, and User layers are not in `fab/gerbers/` (checked), so nothing changes for JLCPCB. Drill **7 mm** (9/32" OK) at KiCad (150, 95.1). Fix the marker in v15.
2. **Title block** still says "AI Calculator main board (fx-300ES Plus transplant)", rev "0.1-geometry". The drawing frame is not plotted (`plotframeref no`), so it never reaches JLCPCB. Cosmetic; fix in v15.

Leftover text (same class: harmless, but don't let it confuse anyone):
- J4's **value field** reads "Battery JST-PH (Adafruit 1570)" and is copied into the **BOM comment column** in `fab/` and `bom/`. JLCPCB places by LCSC number (C295747), which is right; the same socket takes the #1317.
- `User.3` note "BATTERY BAY: Adafruit 1570 LiPo 31x11.5x3.8 in the cut-out (old LR44 corner…)" and the `User.1` "VERIFY" notes (V1–V8) are stage-1/13 notes. The battery is now the **#1317 150 mAh on the back-cover floor**, the LR44 holder is kept.
- Two B.Cu "no via" keep-out zones are still **named** "key Abs" and "key x^3"; the keypad labels (`User.2`) and the switch values say CALC and ∫dx. Zone names have no effect.

Doc mismatch found and fixed: FINAL_STATUS and QUESTIONS said the antenna tab runs "y 90 to 104.5". The module outline (and `Claude outputs/paper_dry_fit_v14.svg`) is **y 89.3–104.7, x 114.4–118.9 (4.5 × 15.4 mm)**. Both docs now give the board numbers.

## Deliberately not on v14 (decided for a later revision)
| Item | Where it was decided |
|---|---|
| R2 20 k → 4.7 k (faster charge, only for a bigger cell in a custom shell) | FINAL_STATUS decisions log; `enclosure/final_assembly/battery_upgrade.md` |
| R5 20 k → 15 k (VBUS_SENSE < 3.6 V on a 5.5 V adapter) | QUESTIONS "Final review" 6 |
| Join the two antenna keep-outs (remove the 1.25 mm GND sliver at the antenna root) | `06_final_review_pcb.md` M2 |
| Pin-1 silk marks 0.06 → ≥ 0.15 mm on J1, J2, J4, L1, Q3 | `06` M3, `04` C1 |
| TCA8418 RESET to a GPIO | QUESTIONS Firmware 13 (no free non-strap GPIO; R16 pull-up only, by decision) |

## Decisions that live off the board (not checkable here)
Shell grinds (solar box, rib B 14 mm, magnet U-notch 21.5 × 7.95, 5b lip only if present, LR44 holder kept), the 7 mm drill, 0.1 mm tape / no foam, battery on the back-cover floor, magnet piece glued in the top-wall slot, the meter checks (J3 +5 V leg into pin 1, J4 red wire to "+", camera fingers 2↔15), the paper dry fit, firmware rules (Wi-Fi TX cap 11 dBm, 3.6 V AI lock-out, camera off before Wi-Fi, release camera pins before power-off, TCA8418 INT cleared before sleep), and the knockoff-shell plan. Their owners: `FINAL_STATUS.md` §3, the assembly / grinding / power-up guides, `firmware/FINAL_REVIEW_firmware.md`. The firmware pin map is the board's own `pins_final.h` (`firmware-prototype/src/pins.h` only `#include`s it), and all 36 GPIO assignments in it were matched to U1's pad nets below.

## Where the decisions were mined from
`FINAL_STATUS.md` decisions log; `QUESTIONS_AND_ISSUES.md` (all sections); `stage2_electrical.md`, `stage3_schematic.md`, `stage4_5_layout.md`, `stage10_fx115es.md`, `stage11_epaper_calipers.md`, `stage12_measurements.md`, `stage13_heights.md`, `stage13_progress.md`, `stage14_verification_fixes.md`; `measurements_2026-10-03.md` (C8, C10, C12, C13); `verification/01–08`; `REVIEW_independent_2026-10-03.md`, `REVIEW_final_2026-10-04.md`; `Claude outputs/AI_CALC_PCB_HANDOFF.md`; `kicad/DESIGN_SUMMARY.md`; the project memory notes (pcb-state "must not undo" list, enclosure-cad, final-assembly-goal); and a script pass over Nirav's own typed messages in the six past session transcripts (board-relevant ones: "I dont want the light" 10/3 → no LED; switch to the fx-115ES shell 10/3; D4 = 6.0 to the inside of the cover and "stubs don't reach the board" 10/4; "I don't plan to order another pcb" 10/4 → board must be right first time; bigger battery question 10/6 → #1317; "make sure everything we have talked about is on the board" 10/6 → this file).

## The trace (script output, 2026-10-06)

| # | Requirement | Source of the decision | Found on the v14 board (script) | Status | Note |
|---|---|---|---|---|---|
| 1 | 2 copper layers, 0.8 mm, ENIG | decision log st.1/7 | 2 / 0.8 / ENIG | PRESENT |  |
| 2 | U1 = ESP32-S3-MINI-1-N4R2 (C3013941), no substitute | HANDOFF, 06 N9, 08 §3 | ESP32-S3-MINI-1-N4R2, C3013941, fp ai_calc:ESP32-S3-MINI-1 | PRESENT |  |
| 3 | Camera GPIO map (XCLK 18, D0 13, D1 11, D2 10, D3 12, D4 14, D5 16, D6 17, D7 21, VSYNC 36, HREF 47, PCLK 15, PWDN 48, RESET 38, SIOD 40, SIOC 39, PWR_EN 34) | memory pcb-state "must not undo", stage 6 | all 17 match | PRESENT |  |
| 4 | Rest of pins_final.h (EPD 5/6/8/41/42/33, I2C 1/2, KEYPAD_INT 4, KEY_ON 7 (RTC), VBAT 9, VBUS_SENSE 37, CHG_STAT 35, USB 19/20) | pins_final.h, stage 2 | all 15 match | PRESENT |  |
| 5 | IO0 (BOOT) only to TP1; IO3/45/46 free | 08 §3, stage 14 §8 | IO0=BOOT (TP1 BOOT); IO3='unconnected-(U1-IO3-Pad7)' IO45='unconnected-(U1-IO45-Pad41)' IO46='unconnected-(U1-IO46-Pad44)' | PRESENT |  |
| 6 | ESP32 antenna overhangs the left board edge (Espressif placement), ~4.2 mm, y 89.3-104.7 | stage 12, 06 M1 | edge x 118.9; courtyard to 114.45; overhang 4.15 mm | PRESENT |  |
| 7 | Screen section narrowed to x 118.9-183.65 (64.75 wide) past the wall pins/stubs | stage 12 (C4/C14) | y85: 118.9-183.65; y90: 118.9-183.65; y95: 118.9-183.65; y100: 118.9-183.65; y104.5: 118.9-183.65; y110: 118.9-183.65 | PRESENT |  |
| 8 | Antenna keep-outs (no copper pour) beside the module root | stage 12; 06 M2 | 2 zones: antenna clearance bottom, antenna clearance top | PRESENT | 06 M2: 1.25 mm GND sliver between them at the root (v15 item, not a blocker) |
| 9 | J1 = C6364666 CamReversed footprint, rot 180 at (150, 161) | memory "must not undo", stage 6, 05 C4 | ai_calc:FPC_24P_P0.5mm_DualContact_C6364666_CamReversed, C6364666, (150.0, 161.0) rot 180.0 | PRESENT |  |
| 10 | J1 pad nets = OV5640 module pinout (pad N = finger N) | 06 N1, 08 §2.1 | all 24 match; pad1 x 144.25, pad24 x 155.75 | PRESENT |  |
| 11 | Camera 7 mm back-cover window centred at KiCad (150, 95.1), 12 x 12 keep-out | stage 13, grinding guide | circle at (150, 95.099) Ø6.0; keep-out rect present; text says "7 mm" | **DIFFERENT (doc layer only)** | The window is drilled in the shell, not the board; this circle is on User.3 (not fabricated). Cosmetic: drawn Ø6.0, label says 7 mm. |
| 12 | No F-side parts in the camera keep-out 144-156 x 89.1-101.1 | stage 13 camera mounting | none | PRESENT |  |
| 13 | E-paper FPC through 1.0 x 14 mm slot at x 172.9 to J2 (C6364666) | memory "must not undo", stage 4/5, 11 | slot sides x [172.4, 173.4], ends y [85.83, 99.83] (len 14.00); J2 C6364666 at (177.9, 92.83) rot -90.0 | PRESENT |  |
| 14 | J2 pad nets = SSD1680 2.13" 24-pin order (BS1=GND, 4-wire SPI) | 08 §2.2, 06 N8 | all match | PRESENT |  |
| 15 | Screen window 60.65 x 24.3, top edge 24.0 below case top, centred x 150 | C12/C13, stage 11 | present | PRESENT |  |
| 16 | R12 3 Ω booster current-limit (kept) | stage 12 Q12 | 3R (2.13in B/W panel) | PRESENT |  |
| 17 | TCA8418 (C138713) + 50 key pads on B.Cu, unique row/col pairs | stage 2/3, 06 check 13 | U6 TCA8418RTWR C138713; 50 SW, all B.Cu=True, duplicate net pairs=0 | PRESENT |  |
| 18 | ON key on its own RTC pin (KEY_ON = IO7, pad to GND, R17 100 k pull-up) | stage 2, pins_final.h | SW50 ON nets ['GND', 'KEY_ON']; R17 100k C25741 nets +3V3/KEY_ON | PRESENT |  |
| 19 | Key row 2 = CALC and ∫dx (fx-115ES) | stage 14 §7 | values: CALC, ∫dx | PRESENT | User.2 labels say CALC/∫dx; the B.Cu "no via" keep-out zones still carry the old names "key Abs"/"key x^3" (zone names only, no effect). |
| 20 | SW1 (SHIFT) bar notched for H3: KeyPad_6.0x4.5_H3notch | stage 14 §5 | ai_calc:KeyPad_6.0x4.5_H3notch | PRESENT |  |
| 21 | U2 MCP73831 (C424093), R2 20 k on PROG -> 50 mA | stage 2/3, decision log | U2 MCP73831T-2ACI/OT C424093; U2.5=CHG_PROG; R2 20k (50 mA) C25765 nets CHG_PROG/GND | PRESENT |  |
| 22 | U3 = RT9080-33GJ5 (C841192), not AP2112K | stage 11, decision log | RT9080-33GJ5 C841192; VIN SYS EN SYS VOUT +3V3 | PRESENT |  |
| 23 | U7 USBLC6-2SC6 (C7519) on USB, reference pin on +3V3 | stage 3 | USBLC6-2SC6 C7519; pins 1=USB_DP, 2=GND, 3=USB_DM, 4=USB_DM, 5=+3V3, 6=USB_DP | PRESENT |  |
| 24 | D7 SMF5.0A TVS (C193402), cathode (pad 1) on VBUS | stage 3, 08 §1 | SMF5.0A (VBUS TVS) C193402; pad1=VBUS pad2=GND | PRESENT |  |
| 25 | J3 = C46061768 right-angle socket at (127.5, 72.0), pads 1-4 VBUS / D- / D+ / GND | stage 13b (option A), stage 14 §1 | C46061768 at (127.5, 72.0); nets ['VBUS', 'USB_DM', 'USB_DP', 'GND']; pad1 x 123.69, pad4 x 131.31 | PRESENT |  |
| 26 | J3 silk "N" and "+" at pin 1, "-" at pin 4 | stage 14 §1 | N 3.3 mm from pad 1 (at 121.3, 69.7); + 2.5 mm from pad 1; - 2.3 mm from pad 4 | PRESENT |  |
| 27 | J4 JST-PH S2B-PH-SM4-TB (C295747): pad 1 GND, pad 2 BAT+; silk - / + | stage 3, 08 §2.4 | C295747 at (142.6, 74.6); pad1=GND pad2=BAT+; '-' 1.4 mm from pad 1, '+' 1.2 mm from pad 2 | PRESENT | Value field still reads "Battery JST-PH (Adafruit 1570)" (text only; the socket is the same for #1317). |
| 28 | TP1-TP7: BOOT, UART TX, UART RX, GND, +3V3, BAT+, EN (recovery = TP1->TP4, tap TP7) | stage 11, stage 14 §8 | {'TP1': 'BOOT', 'TP2': 'UART_TX', 'TP3': 'UART_RX', 'TP4': 'GND', 'TP5': '+3V3', 'TP6': 'BAT+', 'TP7': 'EN'} | PRESENT |  |
| 29 | No LED (exam mode; Nirav "I dont want the light") | chat 2026-10-03 04:20, stage 2 | none | PRESENT |  |
| 30 | Magnet contacts never carry battery voltage (D1 VBUS->SYS, Q2 gate on VBUS, charger blocks) | stage 2, decision log, 06 N3 | J3 nets {'USB_DP', 'VBUS', 'GND', 'USB_DM'}; D1 1=SYS 2=VBUS; Q2 1(G)=VBUS 2(S)=SYS 3(D)=VBAT_P; VBUS pads: C5.1, D1.2, D7.1, J3.1, Q2.1, R4.1, U2.4 | PRESENT |  |
| 31 | Q1 AO3401A reverse-battery FET between J4 BAT+ and VBAT_P | stage 2/3, 06 N3 | Q1 C15127 G=Q1_G S=VBAT_P D=BAT+ | PRESENT |  |
| 32 | R20 STAT pull-up goes to VBUS_SENSE (no back-feed without cable) | stage 11 (review S1) | R20 100k C25741 nets VBUS_SENSE/CHG_STAT | PRESENT |  |
| 33 | VBUS_SENSE divider 10 k / 20 k (R4/R5) -> 3.33 V at 5 V | 06 N7 | R4 10k VBUS/VBUS_SENSE; R5 20k VBUS_SENSE/GND | PRESENT |  |
| 34 | R17 = 100 k (C25741) | stage 14 §6 | 100k C25741 | PRESENT |  |
| 35 | C32 / C33 / C34 = 22 µF 0805 (C45783); C33/C34 on +3V3 at the ESP32 | stage 11 (C32), 13b (C33/C34) | {'C32': ('22uF', 'C45783', '+3V3', 'GND'), 'C33': ('22uF', 'C45783', '+3V3', 'GND'), 'C34': ('22uF', 'C45783', '+3V3', 'GND')} | PRESENT | C32 is on +3V3/GND |
| 36 | Camera LDOs U4 ME6211C28 (C53099) / U5 ME6211C15 (C53100), CE = CAM_PWR_EN, R10 100 k pull-down | stage 2/3, 06 N3 | U4 C53099 CE=CAM_PWR_EN; U5 C53100 CE=CAM_PWR_EN; R10 100k CAM_PWR_EN/GND | PRESENT |  |
| 37 | SCCB pull-ups R18/R19 4.7 k to CAM_2V8 | stage 3 | R18 4.7k CAM_2V8/CAM_SIOD; R19 4.7k CAM_2V8/CAM_SIOC | PRESENT |  |
| 38 | EN RC: R1 10 k + C4 1 µF | 06 check 10 | R1 10k +3V3/EN; C4 1uF EN/GND | PRESENT |  |
| 39 | Mount holes H1/H3 Ø6.0 screw posts; H4/H6/H8/H10 Ø4.2; H2/H9/H13/H14 slots; H6 at y 186.99 (stage 12/13 positions) | stage 10/12/13, DESIGN_SUMMARY | H1 (127.5, 126.1) Ø6; H3 (174.0, 126.1) Ø6; H2 (130.85, 176.25) Ø4.2x5; H9 (169.2, 176.2) Ø4.4x4.8; H4 (171.06, 145.96) Ø4.2; H6 (130.6, 186.99) Ø4.2; H8 (128.85, 137.0) Ø4.2; H10 (171.25, 136.8) Ø4.2; H13 (130.5, 198.325) Ø4.2x4.45; H14 (169.1, 198.375) Ø4.2x4.65 | PRESENT |  |
| 40 | Measured spacings: H1-H3 46.5 (C8), H8-H10 42.3 (C8; board 42.40), H2-H9 38.3, H13-H14 38.6 (photo pairs) | measurements C8, 06 N4 | H1H3 46.50, H8H10 42.40, H2H9 38.35, H13H14 38.60 | PRESENT |  |
| 41 | Bottom screw posts handled by edge notches (no H5/H7/H11/H12) | stage 10/11 ("bottom notches unchanged") | H5/H7/H11/H12 present: none; outline crossings at y 208: [118.481, 127.193, 133.803, 166.016, 172.924, 181.471] | PRESENT |  |
| 42 | 3 fiducials (stage 12) | stage 12 | [('FID1', 121.5, 203.5), ('FID2', 178.5, 203.5), ('FID3', 177.0, 112.5)] | PRESENT |  |
| 43 | Silk "AI CALC v14 2026-10-04" + JLC order-number mark | stage 14, 06 check 23 | ['AI CALC v14 2026-10-04', 'JLCJLCJLCJLC'] | PRESENT |  |
| 44 | Title block | - | title 'AI Calculator main board (fx-300ES Plus transplant)', rev '0.1-geometry' | **DIFFERENT (cosmetic)** | Says "fx-300ES Plus transplant", rev "0.1-geometry"; frame is not plotted (plotframeref no), so nothing reaches JLCPCB. |
| 45 | #1317 battery envelope (x 147.4-180.9, y 60.0-80.7, on the back-cover floor) has no board parts under it | battery_upgrade.md §8 (memory pcb-state) | parts: none; outline right edge at y 63-75 = [147.498, 147.502, 147.505] | PRESENT |  |
| 46 | Bottom screw-post notches ~39.0 apart (C10 42.85 o-o) | stage 11 C10 | notch centres x 130.50 / 169.47 at y 208 -> 38.97 | PRESENT |  |
| 47 | UART backup on TP2/TP3 = TXD0/RXD0 (IO43/44) | stage 11, stage 14 §8 | TXD0=UART_TX, RXD0=UART_RX | PRESENT |  |
| 48 | I2C pull-ups R13/R14 4.7 k, KEYPAD_INT R15 10 k, TCA_RESET R16 10 k (not wired to the ESP32) | stage 3, Firmware Q13 | R13 4.7k +3V3/I2C_SDA, R14 4.7k +3V3/I2C_SCL, R15 10k +3V3/KEYPAD_INT, R16 10k +3V3/TCA_RESET | PRESENT |  |
| 49 | VBAT sense 1 M / 1 M + 100 nF on IO9; STAT through D6 clamp | stage 2, pins_final.h | R6 VBAT_P/VBAT_SENSE, R7 VBAT_SENSE/GND, D6 CHG_STAT_RAW->CHG_STAT | PRESENT |  |
| 50 | fab BOM carries the decided LCSC parts | fab/ai_calc_BOM_JLCPCB.csv | all present | PRESENT | BOM comment for J4 still reads "Battery JST-PH (Adafruit 1570)" (copied from the footprint value; JLCPCB ignores it). |

Coordinates are KiCad mm (board origin as in the .kicad_pcb, y down). "PRESENT" = the requirement matches the board file exactly (positions to 0.01–0.05 mm, nets by name, LCSC by number).
