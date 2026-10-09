# Stage 15: v15-LCD, the 2.13" e-paper replaced by a 1.9" colour IPS LCD

Date 2026-10-08. Nirav decided on 2026-10-08 to replace the e-paper with a small colour LCD. **v14 is frozen and unchanged** (ordered at JLCPCB, git tag `v14-order`; `hardware/kicad/`, `hardware/fab/`, `hardware/archive/` were not touched). Everything for v15 lives in:

| Where | What |
|---|---|
| `hardware/kicad_v15_lcd/` | The v15 KiCad 10 project (`ai_calc_v15_lcd.kicad_pro/.kicad_pcb/.kicad_sch`, sub-sheets; `lcd.kicad_sch` replaces `epaper.kicad_sch`), `pins_v15_lcd.h`, `DESIGN_SUMMARY.md` |
| `hardware/fab_v15_lcd/` | Gerbers zip, BOM, CPL (rotation-corrected), schematic PDF, assembly PDF, STEP, renders, `cpl_easyeda_check_v15.md` |
| `hardware/tools/v15_lcd/` | The scripts that made v15 (`make_lcd_sheet.py`, `board_step1_cleanup.py`, `board_step2_route.py`, `board_step3_review13.py`, `board_step4_review14.py`), the output script, the CPL and hole-clearance checks |
| `hardware/tools/jlc_cpl.py` | Shared CPL rotation table: v15 parts added (C20917 +180, C2919501 0, C23345 0, C22810 0) |

**Status (2026-10-08 evening): verified "ORDER"** by the final pre-order review `verification/14_v15_final_preorder.md`. The fab files in `hardware/fab_v15_lcd/` were regenerated at 21:38 from the final board; how to order them: `hardware/ORDER_WALKTHROUGH_v15_lcd.md`. Nothing has been ordered yet. The bench checks that are still owed (section 9) are all recoverable on this board and do **not** block the order.

> **Updated 2026-10-08 after review 14** (`verification/14_v15_final_preorder.md`): the panel is now **BuyDisplay ER-TFT019-1** (no touch), checked against its own datasheet pin for pin; **R23 22 Ω → 15 Ω (LCSC C22810)**; the SHIFT (SW1), ALPHA (SW2) and ON (SW50) key pads were moved to match the original Casio board (`verification/15_keypad_vs_casio.md`), and SW1 lost its H3 notch. ERC 0, DRC 0/0/0/0, CPL 20/20 after the changes.

> **Updated 2026-10-08 after review 13** (`verification/13_v15_lcd_review.md`): J5 now uses the mirrored footprint `FPC_30P_P0.5mm_DualContact_C2919501_LcdReversed` at (158.6, 93.1), the tail slot moved to x 180.5–181.5, four of the five LCD SPI roles were swapped, and the CAD fit was re-run on the new board (`enclosure/final_assembly_v15_lcd/REPORT.md`). Sections 3, 4, 6, 8 and 9 below describe the board as it is now; the 14:14 layout they replaced is only mentioned where it explains a choice.

**Result:** DRC **0 errors / 0 warnings / 0 unconnected / 0 schematic-parity issues** (`kicad-cli pcb drc --refill-zones --schematic-parity --severity-all`, KiCad 10.0.6), ERC **0**, every copper item ≥ 0.25 mm from every hole (`check_hole_clearance_v15.py`: 0 violations), CPL rotations verified against the EasyEDA footprints for all 20 polarised / multi-pin parts (20 PASS, 0 FAIL). Silk reads **"AI CALC v15-LCD 2026-10-08"**.

---

## 1. What stayed exactly as in v14

Board outline, all mounting holes and slots, the key pads (except SHIFT/ALPHA/ON, moved in review 14, see below) and the keypad matrix (TCA8418), the camera (J1 `_CamReversed` footprint and the whole camera GPIO map XCLK 18, D0 13, D1 11, D2 10, D3 12, D4 14, D5 16, D6 17, D7 21, VSYNC 36, HREF 47, PCLK 15, PWDN 48, RESET 38, SIOD 40, SIOC 39, PWR_EN 34), the charger, the RT9080 3.3 V regulator, the load-share and reverse-battery FETs, J3 magnet connector (1 VBUS, 2 D−, 3 D+, 4 GND) and J4 battery. No LED anywhere (the backlight is the only light). The magnet contacts still carry only VBUS/GND/D±, never battery voltage (D1, Q2 and R20 are untouched).

Two small items that the v14 reviews had deferred to v15 were done because they cost nothing: the two antenna keep-outs are joined into one (06 M2, no copper under the antenna root any more), the camera-window marker on the KEEPOUTS user layer now says 7 mm, and the title block is updated. **Not done** (left for Nirav, see section 9): R5 20 k → 15 k, the 0.06 mm pin-1 silk marks on J1/J4/L1/Q3 (L1 and Q3 no longer exist; J5/Q4/Q5 got 0.15 mm marks).

The LCD sits in the same place the e-paper did: on the **key side** of the board (B side), display facing the front window, with the ribbon going through a slot to a socket on the component side. The outline will change with the knock-off shell later: the whole LCD block (J5, Q4, Q5, R21–R23, C19, C36, the slot) is one cluster between x 156 and 178, y 80 to 114 in board coordinates, so it can be moved as a unit. (After review 13 the cluster spans x 153–182, y 79–107.)

**Key pads moved in review 14** (`verification/15_keypad_vs_casio.md`, applied by `tools/v15_lcd/board_step4_review14.py`): the top-row outer keys were 1.1–1.5 mm off the contact positions on the original Casio keyboard board. **SW1 SHIFT** (176.623, 120.795) → **(177.75, 119.80)**, now the plain `KeyPad_6.0x4.5` footprint (no H3 notch; 1.74 mm copper-to-hole); **SW2 ALPHA** (166.026, 120.795) → **(167.15, 120.80)**; **SW50 ON** (123.377, 120.795) → **(122.45, 119.55)**. Pad sizes and nets unchanged; the other 47 key pads are exactly as in v14.

## 2. The LCD choice (plain English)

The window in the fx-115ES front shell is **60.65 × 24.3 mm** (caliper C12). Whatever we put behind it must have an active (lit) area no taller than about 23 mm. That rules out every "2.0 inch and up" TFT (their short side is 30 mm or more). The candidates:

| | **A. 1.9" 170×320 IPS, ST7789V3, 30-pin FPC** (chosen) | B. 1.9" 170×320 IPS, ST7789V2, 8/12-pin FPC | C. 2.0" 240×320 IPS HS20HS072RX (JLCPCB C5329582) | D. bar-type 0.96"–1.14" (160×80 / 135×240) |
|---|---|---|---|---|
| Active area | **42.72 × 22.70 mm** (fits the window height with 0.8 mm to spare) | same glass, 42.72 × 22.70 | 40.8 × 30.6: **too tall** for the 24.3 mm window | 21.7 × 10.8 or 24.4 × 13.5: tiny |
| Outline | 25.8 × 49.72 × 1.43 mm (backlight incl.), glass 24.8 × 48.52 | ~27.3 × 51.2 (module) | 30.6 × 40.8 | small |
| Pixel pitch | 0.1335 mm (320 px across 42.7 mm) | same | 0.1275 | 0.135 / 0.101 |
| Interface | 4-wire SPI **or** 8080 8-bit (IM1/IM2 select); SPI chosen | SPI only | SPI | SPI |
| FPC | 30 pins, 0.5 mm pitch, 0.3 mm thick, 15.5 mm wide, 36.6 mm long, 4.5 mm stiffener | 8 or 12 pins, 0.5 mm; pinout differs between sellers | 18–40 pins | 8–12 pins |
| Backlight | 4 white LEDs in parallel, rated 60 mA at 3.0–3.4 V, ~350 cd/m² at full current | 2–3 LEDs | 2 LEDs | 1–2 LEDs |
| Documentation | **Full spec sheet** (Unvision/Newvisio 190-1732TBWPG01, used in LilyGO T-Display-S3 and Heltec HT-VMT190: pinout, drawing, electrical limits) | Pinouts only from seller listings | LCSC page | LCSC page |
| Where to buy | AliExpress / Taobao as "1.9 inch 170×320 30PIN T-Display-S3 screen", about $3–6; Heltec HT-VMT190 spares | AliExpress ~$3–7 | LCSC / JLCPCB $3.59 | LCSC $1.8–2.5 |
| JLCPCB-placeable | No (the socket is; the panel is plugged in by hand, like the e-paper was) | No | Yes | Yes |

**Chosen: A**, the 30-pin 1.9" panel. Reasons: it is the only size that fits the window; it is the exact panel behind two mass-produced boards (LilyGO T-Display-S3, Heltec Vision Master T190), so it will stay available and the spec sheet is public (`SPEC N190-1732TBWPG01-C30 VER B.pdf` on resource.heltec.cn); the 30-pin tail has a documented pinout, whereas the 8/12-pin variants (B) differ from seller to seller, which is how the J1 camera trouble started. The extra 8080 pins are simply grounded in SPI mode (the vendor's own 4-SPI reference circuit). A wider "bar" display that fills the 60 mm window does not exist in this height class; the 42.7 mm picture leaves a 9 mm black border each side under the window mask (new mask: `enclosure/final_assembly_v15_lcd/window_mask_template_v15.svg`, opening 43.72 × 23.70 mm, centred).

**The panel to buy (decided 2026-10-08, review 14): BuyDisplay / East Rising ER-TFT019-1, the version without touch**, about $6–7 each ($6.22 at 10, $5.71 at 100). Its own datasheet (rev 2.0) was read and matches J5 pin for pin (ST7789P3, 4-wire SPI with IM1 = IM2 = 1, 25.80 × 49.72 × 1.43 mm, tail 36.6 ± 0.3 mm, finger 1 arriving at the bottom of J5). Adafruit #5394 (the breakout with the same glass inside) is now only an optional fallback. For bulk: Alibaba Goldenmorning T190X7-C30-01 or ZJY (~$2.30–2.50) after a sample check; ask for the plastic-frame (~1.4 mm) build. BuyDisplay's ER-CON30HT-1 socket is **not** needed (J5 is dual contact). Details: `LCD_PANEL_OPTIONS.md`.

**Socket: J5 = HDGC 0.5K-HX-30PWB (LCSC C2919501, $0.36, 2,200+ in stock, JLCPCB extended part)**: 30-pin 0.5 mm, 1.0 mm high, front insert / rear flip, **dual contact** (top and bottom) so it does not matter which face of the ribbon carries the gold fingers, same idea as the C6364666 sockets on J1/J2 in v14. Its EasyEDA footprint was fetched with `easyeda2kicad` and placed into `ai_calc.pretty` (`FPC_30P_P0.5mm_DualContact_C2919501`, the two mounting tabs renamed `MP`). Review 13 replaced it on the board with an identical copy whose pad numbers are mirrored, `..._LcdReversed` (pad N sits where socket contact 31−N is), so that panel finger 1 meets pad 1 at the end the tail really arrives at (section 6).

## 3. Electrical design

Sheet `lcd.kicad_sch` (generated by `tools/v15_lcd/make_lcd_sheet.py`, so it is reproducible).

**Panel pins (J5 pad N = panel pin N):** 1 GND · 2 VDD · 3 IM2 · 4 IM1 (3 and 4 tied to VDD = 4-line SPI) · 5 RESET · 6 CS · 7 SCL · 8 RS (D/C) · 9 RD → GND · 10 SDA (MOSI) · 11–18 DB0–DB7 → GND · 19 SDO open · 20 LEDA · 21–24 LEDK1–4 · 25 GND · 26–29 touch-panel pins, open · 30 GND · MP tabs GND.

**GPIOs.** The six e-paper GPIOs are re-used, but their roles moved so the board's five-line SPI bus meets the socket without a single crossing (see section 4), and one more pin was needed for the LCD power switch. **Review 13 swapped four of the five SPI roles** when J5's pad numbering was mirrored (section 6): the B.Cu lanes keep their west→east order IO5, IO6, IO8, IO41, IO42 but now meet the pads in the opposite order. Table as on the board now (`mcu.kicad_sch`, `pins_v15_lcd.h`):

| Signal | v15 GPIO | v14 use of that GPIO | 14:14 layout (superseded) |
|---|---|---|---|
| LCD_MOSI | IO5 | EPD_CLK | was LCD_RST |
| LCD_DC | IO6 | EPD_DIN | was LCD_CS |
| LCD_SCK | IO8 | EPD_CS | unchanged |
| LCD_CS | IO41 | EPD_DC | was LCD_DC |
| LCD_RST | IO42 | EPD_RST | was LCD_MOSI |
| LCD_PWR_N (active low) | IO33 | EPD_BUSY | unchanged |
| LCD_BL_EN (PWM) | IO4 | KEYPAD_INT | unchanged |
| KEYPAD_INT | **IO3** | free | unchanged |

Any GPIO works for SPI through the ESP32-S3 GPIO matrix (40 MHz is fine; 80 MHz would need the IOMUX pins, which the camera uses). IO3 is a strapping pin only if the JTAG-select eFuse is burned, which this project never does (the recovery notes in `pins_final.h` forbid burning any eFuse), so it is a plain RTC GPIO here: KEYPAD_INT on IO3 still wakes the chip from deep sleep (EXT1). IO3 is the only free, usable GPIO on the module (IO45/IO46 are hard straps, IO26 is the PSRAM); it could not be routed to the LCD area itself, because U1's south row is boxed in by the I2C, keypad and e-paper fan-out, so the keypad interrupt took it and IO4's existing escape became the backlight line. The v15 firmware is its own project, `firmware-v15-lcd/`; its `src/pins.h` includes `hardware/kicad_v15_lcd/pins_v15_lcd.h`, so it follows the review-13 roles automatically (the pin table in its README still shows the 14:14 roles: the header is the truth).

**LCD power switch (Q4, AO3401A, C15127):** +3V3 → Q4 → `LCD_VDD` → panel VDD/IM1/IM2, C36 100 nF + C19 4.7 µF at the socket. Gate = `LCD_PWR_N` with R21 100 k pull-up to +3V3: in deep sleep IO33 floats, the pull-up keeps Q4 off and the panel is completely unpowered. Why: the ST7789's own standby still draws about 20 µA (panel spec), which would eat half the ≤ 40 µA sleep budget. With Q4 off the LCD block adds < 2 µA (FET leakage). Rule for the firmware: drive the five SPI lines and BL low (or input, no pull) **before** raising LCD_PWR_N, otherwise a high SPI line back-feeds the unpowered panel through its ESD diodes (same rule as the camera).

**Backlight (Q5, AO3400A, C20917 + R23 15 Ω 0603, C22810):** LEDA ← R23 ← +3V3; the four cathode pins are tied together (`LCD_LEDK`) and switched to GND by Q5 (on the ER-TFT019-1 only pin 22 is the cathode, 21/23/24 are NC: harmless). Gate = `LCD_BL_EN` (IO4) with R22 100 k pull-down, so the light is off in sleep, at reset and before the firmware runs. Current at 100 % duty = (3.3 V − Vf) / 15 Ω. **Review 14 (M1) changed R23 from 22 Ω to 15 Ω**: the ER-TFT019-1 datasheet gives VLED 2.8 / 3.0 / 3.2 V (min/typ/max) at 60 mA, ILED max 80 mA; with a Vf-vs-current model the 22 Ω resistor gave 12–27 mA (19 mA typical), the 15 Ω one gives **≈ 26 mA typical, 16–37 mA across the Vf range, ≤ 51 mA under any assumption** (flat Vf 2.6 V and a 3.37 V rail), always under the 60 mA rating. 12 Ω and 10 Ω could exceed 60 mA in the worst case. C22810 is a JLCPCB **extended** part (~$3 one-off setup fee; no basic 15/16/18 Ω 0603 exists). R23 dissipation ≤ 39 mW (0603 = 100 mW). The v15 bring-up guide (`bringup_guide_v15_lcd.html` step 10) measures it: current = V(R23) / 15, expected 16–37 mA. Dimming is ordinary LEDC PWM (a few kHz) on IO4, default 60 % duty. If a measured panel is below ~18 mA and too dim: 12 Ω (C22791, extended) only if its measured Vf ≥ 2.75 V; otherwise the constant-current driver (AW9364DNR, LCSC C401007, the part the panel vendor recommends) on the next spin.

**Removed (the SSD1680 e-paper driver, 17 parts, all from `epaper.kicad_sch`):** J2 (24-pin FPC socket, C6364666), L1 68 µH SMNR4020 (C135265), Q3 SI1308EDL (C469327), R11 10 k, R12 3 Ω (C23157), D3, D4, D5 B5819W (C8598), C20 4.7 µF/50 V 1206 (C29823), C21–C30 1 µF/50 V 0603 (C15849, 10 pieces). The 1.0 × 14 mm ribbon slot at x 172.9 and the e-paper's +3V3 feed are gone too. C19 (4.7 µF, C19666) keeps its name but now sits on `LCD_VDD`.

**Added:**

| Ref | Part | LCSC | Price (1 off, JLCPCB 2026-10-08) | Stock |
|---|---|---|---|---|
| J5 | HDGC 0.5K-HX-30PWB 30-pin 0.5 mm FPC socket, dual contact, 1.0 mm | C2919501 | $0.36 | 2,242 |
| Q4 | AO3401A P-MOSFET, SOT-23 (LCD power switch) | C15127 | $0.10 (basic part) | 727 k |
| Q5 | AO3400A N-MOSFET, SOT-23 (backlight switch) | C20917 | $0.09 (basic part) | 892 k |
| R21, R22 | 100 k 0402 | C25741 | $0.002 | basic |
| R23 | **15 Ω** 0603 (was 22 Ω C23345 until review 14) | **C22810** | $0.0013 + ~$3 extended-part setup fee | 314,584 (extended) |
| C36 | 100 nF 0402 | C1525 | $0.002 | basic |
| C19 | 4.7 µF 0603 (kept, re-used) | C19666 | — | basic |
| (panel) | **BuyDisplay ER-TFT019-1** (no touch), 1.9" 170×320 IPS ST7789P3, 30-pin; same glass as the 190-1732TBWPG01 family | not on LCSC/JLCPCB; buydisplay.com (bulk: Alibaba Goldenmorning / ZJY) | ~$6–7 ($6.22 @10, $5.71 @100); bulk ~$2.30–2.50 | — |

Net part count 147 → 135 footprints; the board BOM gets cheaper by roughly $1 (the booster inductor, the 50 V capacitors and the second FPC socket go; one socket and two FETs come), and the panel itself costs about the same as the 2.13" e-paper ($4–7).

**3V3 budget (RT9080, 600 mA):** Wi-Fi TX burst ≈ 355 mA (as in v14) + panel IDD ≤ 20 mA (ER-TFT019-1 §4.3; the earlier 50 mA was the Unvision "normal display" figure) + backlight ≤ 51 mA worst case (≈ 26 mA typical) = **≤ 426 mA worst case** (review 14), about 40 mA more than v14's 385 mA (the e-paper drew 30 mA only while refreshing). The camera (≤ 150 mA) is still on SYS, not on the 3V3 rail. Near an empty cell the v14 firmware rules stay: 3.6 V AI lock-out, 11 dBm TX cap, camera off before Wi-Fi; add "backlight to 25 % during a Wi-Fi burst" if brown-outs show up on the bench. C33/C34 (2 × 22 µF on +3V3) are unchanged.

**Battery impact with the 150 mAh #1317 cell (document only, nothing changes on the power sheet):**
- Deep sleep: unchanged, ≤ 40 µA target holds (Q4 and Q5 are off; the pull-up/pull-down draw nothing).
- Screen on, idle calculator: ESP32-S3 at 80–240 MHz without radio ≈ 30–45 mA, LCD ≤ 20 mA, backlight ≈ 26 mA at 100 % (≈ 9 mA average at the 60 % default, which the firmware squares to 36 % duty) → **≈ 80–90 mA, about 1.5 h of continuous screen-on time** (the e-paper calculator ran ~3 h, because its display cost nothing once drawn). With a 30 s backlight time-out and the LCD put to sleep after 2 min, a school day of normal use is realistic, but Nirav should expect to charge every 1–2 days instead of weekly.
- AI solve (camera + Wi-Fi): unchanged in current, so the number of solves per charge is the same.
- The custom shell with a 1,200–1,500 mAh cell gives 13–17 h screen-on, which is the real target; R2 → 4.7 k (faster charge) stays deferred to that board.

## 4. Layout (as on the board after review 13)

- **J5** at **(158.6, 93.1)**, rotated 90°, mirrored footprint `..._LcdReversed` (section 6): pad row at x 157.4 facing west, FPC mouth at x 160.53 facing east towards the slot, courtyard x 157.63–160.53 (pads reach x 156.73), y 84.6–101.6. **Pad 1 at the bottom (157.4, 100.35)**, pad 30 at the top (157.4, 85.85), GND tabs at (159.8, 84.85 / 101.35). The camera module ends at x 154.25 and its keep-out at x 156, so the "body east of x 156.5" limit from the CAD is met with 0.23 mm to spare; "10 mm west" (the first plan) would not have fitted. Pads 1, 9, 11–18, 25, 30 and the tabs are GND, tied with short bars to vias at the socket (a 0.3 mm pad at 0.5 mm pitch cannot get a thermal spoke from the pour).
- **Slot** 1.0 × 20 mm (stadium) at **x 180.5–181.5** (centre 181.0), y 83.1–103.1: 2.15 mm to the board edge at x 183.65. It is 20 mm long because the 15.5 mm tail is not centred on the panel and its exact offset is ±1 mm on the drawing. Copper keeps 0.30 mm from it (0.3005 measured) and a 0.35 mm keep-out ring zone (tracks/vias, both layers) was added around it in review 13. The panel's backlight end is at x 176.16, so the slot's near edge is 4.3 mm from it (the number the assembly guide uses to place the panel).
- **Cluster:** south band Q4 (159.5, 103.9) r90, C36 (157.3, 102.8), C19 (155.6, 103.6), R21 (159.5, 106.3); north band Q5 (157.3, 82.3) r270, R22 (156.9, 79.6), R23 (161.0, 80.6). The strip from J5's mouth to the slot (x 160.6–180.5, y 85–101.2) has no parts at all (KEEPOUTS rectangle "FPC path on F side").
- **Ribbon path (one gentle bow, no Z-fold):** panel on the key side → tail runs 4.8 mm from the glass edge (x 176.2) to the slot → straight down through the board → one U-shaped bow under the board towards the back cover → back up into J5's mouth at x 160.53, the 4.5 mm stiffener straight in, tip 2 mm inside. Shortest route 30.2 mm against the 36.6 mm tail: **6.4 mm spare, taken up by the bow**. The CAD model (`enclosure/final_assembly_v15_lcd/REPORT.md`) puts the bow's lowest face at Z 2.25 (4.75 mm below the board's component face), with 1.2 mm bend radii (4 × the tail thickness) and a 1.0 mm bend at the slot's top edge. The limit is **rib B of the back cover**, where it is not ground (zone 2 grinds it only over the camera): 0.25 mm between the bow and the rib top. A 2.0 mm bow radius would need Z 1.73 and hits that rib, so the bow corners must stay at about 1.2 mm; each extra mm of tail length deepens the bow about 0.5 mm. The old 14:14 plan (slot x 177.9, J5 x 165.0, a ~15–19 mm spare as a flat Z-fold with 0.18 mm creases) is gone.
- **Signals:** each SPI pad escapes west of the pads on F.Cu to a via (three staggered columns at x 154.9 / 155.6 / 156.3), drops to B.Cu and runs south in five lanes (x 153.0–154.4) to the existing B.Cu bus at y 109–112, cut at the new junctions. The lanes keep the bus order from U1, IO5, IO6, IO8, IO41, IO42 west→east; with the mirrored pad numbering they now meet pins 10, 8, 7, 6, 5 = MOSI, DC, SCK, CS, RST (section 3): zero crossings, no extra vias.
- **LCD_VDD** column at x 159.0 to C36/C19 and the panel (pins 2–4). **+3V3** via (153.48, 94.3) → B.Cu y 94.3 → via (161.6, 94.3) → F.Cu column x 161.6 feeding R23 and Q4. **LCD_PWR_N** B.Cu y 94.85 → x 157.4 → R21/Q4. **LCD_BL_EN** B.Cu y 113.5 → x 162.2 → y 79.5 → Q5/R22. LEDA/LEDK bars to R23/Q5. Ten GND stitching vias around the socket. KEYPAD_INT's vertical at x 127.55 starts from U1 pad 7 (IO3) with a 1 mm jog (unchanged from the 14:14 board).
- **Heights** (`stage13_heights.md` rules: ≤ 5.7 mm on the component side): J5 about 1.1 mm in its STEP model, Q4/Q5 1.1 mm, passives ≤ 0.6 mm. On the key side the panel is 1.43 mm thick (e-paper 1.05) and the tail no longer folds under it; the CAD leaves 1.27 mm between the panel's top and the front plate with 0.1 mm tape (section 9.5).
- **GND pour** refilled on both layers.
- **User layers:** KEEPOUTS shows the panel outline (49.72 × 25.8 centred on (151.3, 93.1)), its active area (42.72 × 22.70, centred on the window centre (150.0, 93.1)), the 7 mm camera window and the FPC strip. These are not printed on the board; `enclosure/final_assembly_v15_lcd/lcd_position_jig_v15.svg` is the same outline as a 1:1 paper jig.

## 5. Checks run

| Check | How | Result |
|---|---|---|
| ERC | `kicad-cli sch erc --severity-all` on all 7 sheets | 0 |
| DRC | `kicad-cli pcb drc --refill-zones --schematic-parity --severity-all` | 0 / 0 / 0 unconnected / 0 parity |
| Library match | part of DRC (`lib_footprint_mismatch`) | 0: every new footprint is the library copy |
| Copper to holes | `tools/v15_lcd/check_hole_clearance_v15.py` (≥ 0.25 mm, NPTH vs any net) | 0 violations (minimums 0.25–0.30, same as v14) |
| CPL rotations | `tools/v15_lcd/check_cpl_easyeda_v15.py` + `easyeda2kicad --footprint` for all 20 polarised/multi-pin LCSC parts, placed at the CPL position/angle and compared pad by pad | 20 PASS, 0 FAIL (`fab_v15_lcd/cpl_easyeda_check_v15.md`). New entries in `jlc_cpl.py`: **C20917 +180** (Q5, the EasyEDA SOT-23 "-BR" zero angle, same as C15127), C2919501 0, C23345 0, and in review 14 **C22810 0** (R23 15 Ω; re-run 20/20 on the 21:38 CPL). Negative test: without the C20917 entry the check fails Q5 by 2.71 mm, so it discriminates. |
| Schematic ↔ board pins | the parity check above covers every pad incl. J5's five intentionally open pins | 0 |
| Fab files | `tools/v15_lcd/make_outputs_v15.sh` (same steps as v14's `make_outputs.sh`, project name parameterised) | `fab_v15_lcd/`: 14 gerber/drill files zipped, BOM 65 CPL rows, PDFs, STEP, renders |

Not done, on purpose: no paper dry fit (the holes did not move). (The firmware was ported afterwards: `firmware-v15-lcd/`, builds clean; review 14 checked its pin use against the header.)

## 6. Pin-1 of the ribbon at J5 (the J1 lesson, applied in review 13)

J5 pad N is panel pin N, and JLCPCB places the socket exactly as drawn (EasyEDA pin 1 lands on KiCad pad 1, checked). The question is **which end of the real tail finger 1 is on when the tail arrives at J5**. Review 13 read it off the vendor drawing (spec N190-1732TBWPG01-C30 p.5, display-face view, tail down, fingers "30 … 1" left to right): with the panel face-up on the key side and the tail towards board +x, finger 1 is at the **large-y end** of the tail. Going down the slot and bending back towards J5 does not change the y order of the fingers (the dual-contact socket takes care of the *face*, not the *end*). So finger 1 arrives at the **bottom** of J5 (y ≈ 100), and the LED fingers 20–24 in the **top half** (y ≈ 88.9–90.9). **Review 14 confirmed this independently from the ER-TFT019-1's own drawing** (p.6 full view: fingers "30 … 1" left to right, display face; fingers face up, away from the board, at J5 — fine for the dual-contact socket).

The 14:14 board had pad 1 at the top (y 85.85), the wrong end, so review 13 changed the board, like J1's `_CamReversed`:
- New footprint `ai_calc:FPC_30P_P0.5mm_DualContact_C2919501_LcdReversed` (same geometry and 3D model, pad numbers mirrored). **Pad 1 is now at (157.4, 100.35), pad 30 at (157.4, 85.85)**; the GND tabs at (159.8, 84.85 / 101.35).
- Silk pin-1 tick at the **bottom** end, (155.5–156.1, 100.35). The old note "pin 1 to the top, tick at (162.5, 85.3)" is obsolete.
- The GPIO roles were swapped to keep the copper crossing-free (section 3). Electrically J5 pad N = panel pin N is unchanged; the CPL rotation of C2919501 stays 0 (the EasyEDA check treats J5 like J1: numbers mirrored on purpose).

Still to confirm on a real tail (the drawing does not say in words which face carries the fingers):
1. **Diode test before powering** (meter in diode mode, panel unplugged): the backlight LEDs sit between finger 20 (anode) and fingers 21–24 (cathodes), i.e. 7–11 fingers in from the pin-30 end; the pin-1 end has GND, VDD, IM2, IM1. Red probe on the 11th finger from one end, black on the 9th: an LED reading (about 2.5 V, maybe a faint glow) means that end is the 30 end. Mark the finger-1 corner on the stiffener. Plugged in with no twist, that mark must land at the **bottom** of J5, at the silk tick, with the LED fingers in the top half. Step by step: `bringup_guide_v15_lcd.html` step 5.
2. If it comes out the other way (two independent drawings say it will not): nothing is damaged (all signals are 3.3 V-level; the display just stays dark). Fix with a half twist of the tail (about 6.4 mm spare now, section 4), or swap the footprint on the next spin (`tools/v15_lcd/board_step3_review13.py`).

## 7. Firmware TODO (written before the port; now done in `firmware-v15-lcd/`, see its README and PORT_NOTES; kept for the reasoning)

1. `firmware-prototype/src/pins.h`: include `hardware/kicad_v15_lcd/pins_v15_lcd.h` for v15 builds (build flag `BOARD_V15_LCD`); run `tools/check_pins.py` against a netlist exported from `kicad_v15_lcd`.
2. Replace GxEPD2 with an ST7789 driver: **LovyanGFX** (`lgfx::Panel_ST7789`, `panel_width 170`, `offset_x 35`, `offset_rotation` for landscape, `pin_cs/dc/rst`, `bus_shared false`, SPI2 at 40 MHz) or TFT_eSPI (`ST7789_DRIVER`, `TFT_WIDTH 170`, `TFT_HEIGHT 320`, `TFT_RGB_ORDER` to be checked on the bench, the usual `CGRAM_OFFSET`). `screen.cpp`: the `calc::Panel` framebuffer is drawn with `pushImage`/`drawBitmap` scaled to 320 × 170 instead of `g_epd.drawBitmap`; no more `hibernate()` / BUSY polling; `display()` becomes a plain flush.
3. Power sequencing (`power.cpp`): on wake `LCD_PWR_N` low → wait 10 ms → RST pulse → init → backlight PWM up. Before deep sleep: backlight duty 0 → `SLPIN` → all LCD GPIOs low / input-no-pull → `LCD_PWR_N` high (R21 holds it). Remove the EPD hold-levels. `kLcdGpios[]` in the header lists the pins.
4. Backlight: LEDC channel on IO4, 5 kHz, 8-bit; default 60 % duty; **time-out** to 10 % after 20 s without a key, off after 60 s, LCD to sleep after 2 min, deep sleep as before; a `bright` serial command and a SHIFT-MODE setting.
5. **Colour viewfinder** for the camera: stream the OV5640 at QVGA/RGB565 into a 320 × 170 crop (or letter-box 320 × 240 → 320 × 180 scaled) at ~10–15 fps through the GPIO-matrix SPI (40 MHz ≈ 5 MB/s ≈ 45 fps of 320 × 170 × 2 bytes; the PSRAM frame copy is the limit, not SPI). Keep the "camera off before Wi-Fi" rule; the backlight at 100 % during the viewfinder is fine (≈ 26 mA with R23 = 15 Ω).
6. KEYPAD_INT moved to IO3: update the EXT1 wake mask and the TCA8418 ISR pin; the 11 dBm TX cap and the 3.6 V AI lock-out stay; add the Wi-Fi-burst backlight dimming if the bench shows brown-outs.
7. Self-test (`selftest.cpp`): replace the e-paper checker-board with a colour bar pattern and a backlight sweep; log `lcd_ok` in the JSON.
8. UI: the 320 × 170 colour screen gives room for a two-line input + result like the real fx-115ES plus a status bar (battery, Wi-Fi, charge); fonts need redoing for 0.13 mm pixels (the e-paper was 0.19 mm).

## 8. How to re-make v15 from scratch (for the next session)

```
cp -r hardware/kicad (v14) hardware/kicad_v15_lcd; rename ai_calc.* -> ai_calc_v15_lcd.*, epaper.kicad_sch -> lcd.kicad_sch
python hardware/tools/v15_lcd/make_lcd_sheet.py hardware/kicad_v15_lcd           # writes lcd.kicad_sch + symbols
# mcu.kicad_sch labels: IO5 LCD_MOSI, IO6 LCD_DC, IO8 LCD_SCK, IO41 LCD_CS, IO42 LCD_RST, IO33 LCD_PWR_N, IO4 LCD_BL_EN, IO3 KEYPAD_INT (review-13 roles)
"C:/Program Files/KiCad/10.0/bin/python.exe" hardware/tools/v15_lcd/board_step1_cleanup.py
"C:/Program Files/KiCad/10.0/bin/python.exe" hardware/tools/v15_lcd/board_step2_route.py
"C:/Program Files/KiCad/10.0/bin/python.exe" hardware/tools/v15_lcd/board_step3_review13.py   # review 13: mirrored J5, slot + J5 moved, roles swapped
"C:/Program Files/KiCad/10.0/bin/python.exe" hardware/tools/v15_lcd/board_step4_review14.py   # review 14: SW1/SW2/SW50 moved, SW1 plain footprint (R23 15 Ω comes from make_lcd_sheet.py / step 2)
PY=".../python.exe" sh hardware/tools/v15_lcd/make_outputs_v15.sh hardware/kicad_v15_lcd hardware/fab_v15_lcd
```

## 9. Open questions for Nirav (status after review 14, 2026-10-08 evening)

**Resolved:** 1 (panel), 2 (tail route), 3 (R23 value), 4 (mask), 5 (shell depth), 8 (board side of pin 1). **Still open, none blocks the order:** the bench checks below (do them when the boards and panels arrive), plus the decisions 6, 7, 9.

**Bench checks after arrival** (review 14 §6; `ARRIVAL_CHECKLIST.md` v15 section):
- Diode test on the first ER-TFT019-1 tail: finger 1 must land at the J5 silk tick (bottom), LED fingers in the top half.
- Backlight current = V(R23) / 15: expect 16–37 mA (≈ 26 mA typical).
- Panel image quality at 3.3 V (top of its 2.4–3.3 V range); IDD ≤ 20 mA.
- `LCD_INVERT` / `LCD_RGB_ORDER` build flags (colours, inversion).
- Tail length 36.6 ± 0.3 mm and the bow's clearance to rib B (assembly guide, marker dot).
- Feel of the three moved keys SHIFT, ALPHA, ON.

1. **Panel source: RESOLVED (review 14).** **BuyDisplay ER-TFT019-1, no touch**, ~$6–7 each ($6.22 @10, $5.71 @100), checked against its own datasheet pin for pin. Buy 3 for the prototypes. Adafruit #5394 (breakout, $17.50) is only an optional fallback now. Bulk: Alibaba Goldenmorning T190X7-C30-01 / ZJY (~$2.30–2.50) after a sample check, plastic-frame build. The ER-CON30HT-1 socket is not needed. Details: `LCD_PANEL_OPTIONS.md`.
2. **Spare tail length: RESOLVED on the board.** Review 13 moved the slot out to x 181.0 and J5 west to x 158.6; the 6.4 mm spare is now one gentle bow, checked in CAD (section 4). No Z-fold. Still measure the real tail (36.6 ± 0.3) at assembly.
3. **Brightness: RESOLVED on paper (review 14), confirm on the bench.** R23 is now **15 Ω (C22810)**: ≈ 26 mA typical, 16–37 mA range, cannot exceed the 60 mA rating. Measure V(R23)/15 at 100 % (`bringup_guide_v15_lcd.html` step 10).
4. **Window mask: done.** `enclosure/final_assembly_v15_lcd/window_mask_template_v15.svg/.dxf`: opening 43.72 × 23.70 mm (active area + 0.5 mm), centred; about 8.5 mm of black each side.
5. **Front-shell depth at the window: checked in CAD.** 1.27 mm between the panel's top and the front plate with 0.1 mm tape, 1.48 mm to the mask; no tail under the panel any more. Use 0.1 mm tape, not foam.
6. **Battery life.** About 1.5 h of continuous screen-on on the 150 mAh cell; charging every 1–2 days with normal use. Acceptable for the prototype, or should the 1,200–1,500 mAh custom-shell cell come first? (unchanged)
7. **R5 → 15 k** (VBUS_SENSE margin on 5.5 V adapters, review item from v14) is still not done. Say yes and it goes in with the next regeneration. (unchanged)
8. **Pin-1 end of the tail: RESOLVED on the board (reviews 13 and 14, two independent drawings), bench confirmation at arrival.** J5 is mirrored (section 6): finger 1 must arrive at the **bottom** of J5 (silk tick), the LED fingers 20–24 in the top half. Do the diode test on the first real tail before plugging it in; if it comes out the other way, a half twist fixes it.
9. **A touch panel?** The 30-pin tail reserves pins 26–29 for a capacitive touch controller; not wired on v15 (no spare GPIOs). Say if that matters for the roadmap. (unchanged)
