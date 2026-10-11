# Verification 01: footprint ↔ symbol ↔ datasheet pin mapping and land patterns

> **⚠ Superseded by stage 14: see `../FINAL_STATUS.md` and `05_stage14_recheck.md`.** This checked the stage-13b board. Its findings were fixed in stage 14 (`../stage14_verification_fixes.md`); v14 was re-checked GO.

Board: `hardware/kicad/ai_calc.kicad_pcb` (stage 13b, saved 2026-10-04 04:09). Checked 2026-10-04, read-only.

## Verdict: **no FATAL and no LIKELY PROBLEM found.** The pin mapping is correct on every active part, on the connectors and on the diodes.

**How this was checked:**
- **Board side:** pcbnew dumped every footprint's pad number, net, absolute and local pad position, and pad size.
- **Schematic side:** a netlist was exported (kicad-cli) from a **copy** of the schematic.
- **Board vs schematic:** all 882 numbered pads compared. **0 mismatches.**
- **Datasheets:** every LCSC number in `fab/ai_calc_BOM_JLCPCB.csv` was resolved on lcsc.com to its manufacturer part, and the datasheet PDF was downloaded and read. Drawings were rendered to images and inspected.
- **Ribbons:** J1 and J2 were re-derived from scratch from the camera module's and the panel's own drawings.

**What is left:**
- A few CHECK items: J1's ~85–90 % confidence, the J4 battery polarity, and the J3 toe pad. None of them is a board-blocking defect.
- One item outside the pin-mapping scope: reverse-battery with USB plugged in.

## Findings by severity

### FATAL
None.

### LIKELY PROBLEM
None.

### CHECK

**C1. J1 camera socket: the CamReversed footprint is correct. My independent confidence is about 85–90 %.**

Sources: Seeed's module drawing, *OV5640_Camera_Module_Specification.pdf* (Mingjiaying MJY5OAF-F3M-V1, the module Seeed ships for the XIAO ESP32S3 Sense), from files.seeedstudio.com.

What the drawing shows:
- **Pin table:** 1 STROBE, 2 AGND, 3 SDA, 4 AVDD 2.8, 5 SCL, 6 RST, 7 VSYNC, 8 PWDN, 9 HSYNC, 10 DVDD 1.5, 11 DOVDD, 12 Y9, 13 MCLK, 14 Y8, 15 GND, 16 Y7, 17 PCLK, 18 Y6, 19 Y2, 20 Y5, 21 Y3, 22 Y4, 23 AF-GND, 24 AFVDD 2.8.
- **The fingers are on the module's base side:**
  - The top view (lens visible) shows the PI stiffener hatch on the finger end.
  - The bottom view (the foam pad visible) shows the gold fingers.
- **Pin 1 position:** in the bottom view the cable runs left from the module, "1" is at the page-bottom edge of the fingers and "24" at the top.

Derivation:
1. The bottom view is the top view flipped about the cable axis (the cable stays on the left). So in the top view, pin 1 is on the page-top edge.
2. On the board, the camera lies lens-up at (150, 95.1) and the cable runs straight in +y to J1 at (150, 161). In KiCad's F-side view the lens faces the viewer and the tip points to the bottom of the screen.
3. Rotating the top view 90° CCW (cable tip pointing to the bottom of the screen) puts pin 1 on the **screen left**, at the smaller x.

What the board has:
- J1 (`…_CamReversed`, rot 180) has pad "1" at **x = 144.25** (left) and pad 24 at x = 155.75.
- The opening faces −y, towards the camera: the tabs are at y 159.42 and the solder tails at y 162.58.
- So finger N lands on schematic pin N, and **CamReversed is right.**

Every function matches (table below). Pin 1 STROBE and pin 6/7 of the panel side are left NC.

Residual risk (why not 100 %):
- the drawing convention (whether the bottom view is a true flip about the cable axis);
- Seeed may ship a different module variant;
- the cable must not be twisted or folded over in the "gentle S" it lies in.

If the board is wrong, the mirror puts board CAM_DVDD (pad 10) onto camera GND (pin 15). The ME6211 is current-limited, so nothing burns, but the camera won't work. **Fix with no board change:** one 180° twist along the cable length (J1 is dual-contact).

**Keep the meter test:** fingers 2 ↔ 15 beep (AGND/GND). Mirrored, they would be 23 ↔ 10 (AF-GND ↔ DVDD), which should not beep.

**C2. J2 e-paper socket: plain numbering is correct. Confidence about 90 %.**

Source: Waveshare *2.13inch_e-Paper_V4_Specification.pdf*, p. 6–7. The front view has the active area 122×250 and the COG IC visible, the FPC exits **left**, pin 1 is at the top and 24 at the bottom.

Derivation:
1. Seen from the back, which is the KiCad F-side view because the panel is on the B side facing out: the FPC exits **right** and pin 1 is still at the **top** (smaller y).
2. The ribbon then makes four bends, all about the ribbon's width axis:
   - the 180° fold behind the panel;
   - the 90° bend up through the 1.0 × 14 slot at x 172.9;
   - the 90° bend into J2, whose opening faces −x.
3. The ribbon's (direction, normal) pair goes (+x, −z) → (−x, +z) → (+z, +x) → (+x, −z). That is the same orientation it started with, so the lateral order is unchanged: **pin 1 at the smaller y.**

What the board has:
- J2 (rot −90) has pad 1 at **y = 87.08** and pad 24 at y = 98.58, with the opening facing −x. ✓
- Ribbon thickness 0.30 (stiffener) and width 12.50 suit the connector (0.3 mm FFC, 24 P).

**C3. J4 battery polarity can't be proven from the datasheet. This is the same as the earlier review.**
- The footprint is the same as KiCad's stock `JST_PH_S2B-PH-SM4-TB` (pad 1 at x −1, same side). Only the signal pads are 3.8 long instead of 3.5, which is harmless.
- The board has pin 1 = GND and pin 2 = BAT+.
- JST doesn't define wire colour, so **keep the meter check of the Adafruit cell's red wire before plugging in.**
- Out of scope, noted only: with the cell reversed **and** USB connected, Q1 (gate to GND, S = VBAT_P) can be partly turned on by the charger output. The ≤ 5–50 mA charge current then flows backwards into the reversed cell. With the battery alone, Q1 protects correctly.

**C4. J3 (C46061768, Hanxia HX PM2.54-1x4P WT): the pads are slightly back from the foot.**
- The drawing gives:
  - pads 1.02 × 3.0 at 2.54 pitch (the footprint matches);
  - legs 3.20 ± 0.25 from the body end, with a flat foot 1.30 ± 0.3.
- In the footprint:
  - the body ends at local y −1.6 and the pad spans −1.5 to +1.5;
  - so the foot tip (nominal +1.6) sits about 0.1 mm past the pad end;
  - about 1.2 mm of the foot length lies on copper.
- It will solder: heel and side fillets are fine, the toe fillet is minimal.
- Pin order 1..4 = VBUS, GND, D−, D+ depends only on how the magnet piece is fitted, which is already covered by the meter check (`REVIEW_final` §10). No change recommended for this order.

**C5. Camera DOVDD runs at 2.8 V.**
- The module table says "DOVDD 1.8V", but the OV5640 accepts 1.7–3.0 V.
- 2.8 V keeps the camera's outputs above the ESP32-S3's V_IH (0.75 × 3.3 = 2.475 V).
- The ESP32 drives XCLK/PWDN/RESET at 3.3 V into the 2.8 V I/O. That's common practice (ESP32-CAM designs do the same) and SIOD/SIOC pull up to CAM_2V8.
- Not a mapping error, just noted.

**C6. The TCA8418 exposed pad is 2.6 × 2.6** (KiCad HVQFN-24 4×4).
- TI RTW0024B gives an EP of 2.45 ± 0.1, a land pattern of 0.24 × 0.6 pads and a 3.8 span.
- On the footprint, the EP-to-signal-pad gap is 0.20 mm and the signal pads are 0.25 × 0.875, extending further out than TI's.
- It's a widely used substitute and fine for JLCPCB. The pin-1 corner is right (see the table).

### OK (verified)

- **U1 ESP32-S3-MINI-1-N4R2 (C3013941).** Checked against the Espressif datasheet v1.7 (Fig. 3-1, Table 3-1, Fig. 11-1).
  - **Land pattern:** all 65 pads match. 60 × 0.4 × 0.8 at 0.85 pitch, 11.9 per side, rows 14 mm apart (±7). 4 corner pads 0.8 × 0.8 (62–65). Centre GND: 3×3 of 1.2 mm at 1.65 pitch (4.5 overall), with the pin-1 chamfered sub-pad.
  - **Numbering:** counter-clockwise from the top view. Pin 1 is top-left next to the antenna end. The 46–60 GND row is along the antenna edge.
  - **Antenna:** the keep-out is local −y. At rot 90 it points to −x (the left edge, overhang intended).
  - **Body:** the footprint body (y −12.8…+7.7 = 20.5 × 15.4) matches.
  - **IO26 (pad 26):** left NC. On N4R2 it is the PSRAM CS, so this is correct.
  - **IO33–37:** free on quad-PSRAM N4R2, so their use (EPD_BUSY, CAM_PWR_EN, CHG_STAT, CAM_VSYNC, VBUS_SENSE) is OK.
- **U2 MCP73831T-2ACI/OT (C424093):** 1 STAT, 2 VSS, 3 VBAT, 4 VDD, 5 PROG ✓.
- **U3 RT9080-33GJ5 (C841192):** TSOT-23-5. 1 VIN = SYS, 2 GND, 3 EN = SYS, 4 NC (open), 5 VOUT = +3V3 ✓.
- **U4/U5 ME6211C28/C15M5G-N (C53099/C53100):** 1 VIN, 2 VSS, 3 CE, 4 NC, 5 VOUT ✓.
- **U6 TCA8418RTWR (C138713):** pins 1–24 as listed in TI's Pin Functions table. Pin 1 top-left, counter-clockwise. EP 25 = GND ✓.
- **U7 USBLC6-2SC6 (C7519):** 1/6 I/O1 = D+, 2 GND, 3/4 I/O2 = D−, 5 VBUS = +3V3 ✓.
- **Q1/Q2 AO3401A (C15127):** SOT-23 1 G, 2 S, 3 D. The nets match the reverse-battery / load-share topology ✓.
- **Q3 Si1308EDL (C469327):** SC-70-3. 1 G, 2 S, 3 D (Vishay top view) ✓.
  - The custom footprint is the stock SOT-23 geometry rotated 180°, not mirrored.
  - Pads 0.8 × 0.9 against Vishay AN826's 0.56 × 0.65 minimum. The 1.3 pitch and 2.4 span match; it's larger but solderable.
- **D1–D6 B5819W SL (C8598, SOD-123):** KiCad pad 1 = cathode (band). The nets give the intended directions (VBUS→SYS, 2V8→AF, SW→PREVGH, PUMP→GND, PREVGL→PUMP, STAT clamp) ✓.
- **D7 SMF5.0A (C193402):** unidirectional, the band is the cathode. The KiCad D_SMF pad 1 = K = VBUS, A = GND ✓.
  - The schematic symbol uses A1/A2 pin names (a bidirectional TVS symbol). That's harmless, because the footprint's pad 1 sets the polarity.
  - Check the band position in JLCPCB's preview, as the earlier review said.
- **L1 SMNR4020-68UH (C135265):**
  - Datasheet land pattern: 0.95 × 3.3 pads, 2.1 gap.
  - Footprint: 1.874 × 3.52 pads, 1.99 gap.
  - Larger, with the gap within 0.11 of the recommendation. No polarity.
- **FB1 (C1002), 0603:** no polarity.
- **J1/J2 land pattern vs the SHOU HAN "FPC 0.5-24P HYH2.0" drawing (C6364666):**
  - Signal pads 0.3 × 1.2 at 0.5 pitch ✓.
  - Tabs 1.8 × 2.0 ✓.
  - Pad row to tab row 4.75 overall, i.e. 3.15 between centres; the footprint has 3.16 ✓.
  - Tab outer edge to pin-1 centre 2.55; the footprint has 2.55 ✓.
  - The solder tails are at the back, so the opening is on the tab side (local +y) ✓.
  - The connector is double-sided (both contact faces), so both ribbons connect whichever face is down. Only the lateral order matters (C1/C2).
- **J2 functions vs the Waveshare V4 pin table:** all match.
  - Pin 4 is "NC" and pin 5 is "VSH2" on V4. The board puts only a 1 µF cap to GND on each (nets EPD_VGL/EPD_VGH), which is harmless.
  - Pins 21/23 (VGH/VGL) take the PREVGH/PREVGL pump outputs, as on Waveshare's HAT.
  - BS1 = GND (4-wire SPI). Pins 6/7 (TSCL/TSDA) are NC.
- **Key pads (SW1–SW50):** all 50 are on B.Cu with two distinct, non-empty nets ✓.
- **Polarised passives:** none. Every capacitor is an MLCC, so there's no electrolytic or tantalum orientation risk.
- **All LCSC numbers resolve to the intended manufacturer parts.** lcsc.com titles:
  - ESP32-S3-MINI-1-N4R2, MCP73831T-2ACI/OT, RT9080-33GJ5, ME6211C28M5G-N / C15M5G-N, TCA8418RTWR, USBLC6-2SC6, AO3401A
  - SI1308EDL-T1-GE3, B5819W SL, SMF5.0A (MDD), SMNR4020-68UH, S2B-PH-SM4-TB(LF)(SN), HX PM2.54-1x4P WT, FPC 0.5-24P HYH2.0

## Full tables

The net is as on the board, which equals the schematic (0 parity mismatches over 882 pads).

### U1 ESP32-S3-MINI-1-N4R2 (Espressif DS v1.7, Table 3-1)

| Pad | Datasheet | Net | | Pad | Datasheet | Net |
|---|---|---|---|---|---|---|
| 1 | GND | GND | | 31 | IO35 | CHG_STAT |
| 2 | GND | GND | | 32 | IO36 | CAM_VSYNC |
| 3 | 3V3 | +3V3 | | 33 | IO37 | VBUS_SENSE |
| 4 | IO0 | BOOT | | 34 | IO38 | CAM_RESET |
| 5 | IO1 | I2C_SDA | | 35 | IO39 | CAM_SIOC |
| 6 | IO2 | I2C_SCL | | 36 | IO40 | CAM_SIOD |
| 7 | IO3 | NC | | 37 | IO41 | EPD_DC |
| 8 | IO4 | KEYPAD_INT | | 38 | IO42 | EPD_RST |
| 9 | IO5 | EPD_CLK | | 39 | TXD0 | UART_TX |
| 10 | IO6 | EPD_DIN | | 40 | RXD0 | UART_RX |
| 11 | IO7 | KEY_ON | | 41 | IO45 | NC |
| 12 | IO8 | EPD_CS | | 42 | GND | GND |
| 13 | IO9 (ADC1_CH8) | VBAT_SENSE | | 43 | GND | GND |
| 14 | IO10 | CAM_D2 | | 44 | IO46 | NC |
| 15 | IO11 | CAM_D1 | | 45 | EN | EN |
| 16 | IO12 | CAM_D3 | | 46–60 | GND | GND |
| 17 | IO13 | CAM_D0 | | 61 (9 sub-pads) | GND (EPAD) | GND |
| 18 | IO14 | CAM_D4 | | 62–65 | GND (corners) | GND |
| 19 | IO15 | CAM_PCLK | | | | |
| 20 | IO16 | CAM_D5 | | | | |
| 21 | IO17 | CAM_D6 | | | | |
| 22 | IO18 | CAM_XCLK | | | | |
| 23 | IO19 / USB_D− | USB_DM | | | | |
| 24 | IO20 / USB_D+ | USB_DP | | | | |
| 25 | IO21 | CAM_D7 | | | | |
| 26 | IO26 (PSRAM CS on N4R2) | NC ✓ | | | | |
| 27 | IO47 | CAM_HREF | | | | |
| 28 | IO33 | EPD_BUSY | | | | |
| 29 | IO34 | CAM_PWR_EN | | | | |
| 30 | IO48 | CAM_PWDN | | | | |

The camera GPIO map matches `pins_final.h`: XCLK 18, D0–D7 = 13/11/10/12/14/16/17/21, VSYNC 36, HREF 47, PCLK 15, PWDN 48, RESET 38, SIOD 40, SIOC 39, PWR_EN 34.

### Regulators, charger, ESD

| Part | Pin | Datasheet function | Pad | Net | OK |
|---|---|---|---|---|---|
| U2 MCP73831 SOT-23-5 | 1 | STAT | 1 | CHG_STAT_RAW | ✓ |
| | 2 | VSS | 2 | GND | ✓ |
| | 3 | VBAT | 3 | VBAT_P | ✓ |
| | 4 | VDD | 4 | VBUS | ✓ |
| | 5 | PROG | 5 | CHG_PROG (R2 20 k) | ✓ |
| U3 RT9080-33GJ5 TSOT-23-5 | 1 | VIN | 1 | SYS | ✓ |
| | 2 | GND | 2 | GND | ✓ |
| | 3 | EN | 3 | SYS | ✓ |
| | 4 | NC | 4 | open | ✓ |
| | 5 | VOUT | 5 | +3V3 | ✓ |
| U4 ME6211C28 SOT-23-5 | 1 | VIN | 1 | SYS | ✓ |
| | 2 | VSS | 2 | GND | ✓ |
| | 3 | CE | 3 | CAM_PWR_EN | ✓ |
| | 4 | NC | 4 | open | ✓ |
| | 5 | VOUT | 5 | CAM_2V8 | ✓ |
| U5 ME6211C15 SOT-23-5 | 1 | VIN | 1 | SYS | ✓ |
| | 2 | VSS | 2 | GND | ✓ |
| | 3 | CE | 3 | CAM_PWR_EN | ✓ |
| | 4 | NC | 4 | open | ✓ |
| | 5 | VOUT | 5 | CAM_DVDD | ✓ |
| U7 USBLC6-2SC6 SOT-23-6 | 1 | I/O1 | 1 | USB_DP | ✓ |
| | 2 | GND | 2 | GND | ✓ |
| | 3 | I/O2 | 3 | USB_DM | ✓ |
| | 4 | I/O2 | 4 | USB_DM | ✓ |
| | 5 | VBUS | 5 | +3V3 | ✓ |
| | 6 | I/O1 | 6 | USB_DP | ✓ |

### U6 TCA8418RTWR (TI, RTW0024B)

| Pins | Function | Net | Position on the footprint |
|---|---|---|---|
| 1–8 | ROW7…ROW0 | ROW7…ROW0 | 1–6 left side top→bottom; 7–8 bottom side |
| 9–18 | COL0…COL9 | COL0…COL9 | 9–12 bottom side left→right; 13–18 right side bottom→top |
| 19 | GND | GND | top side, right end |
| 20 | RESET | TCA_RESET (10 k to 3V3) | top side |
| 21 | VCC | +3V3 | top side |
| 22 | SDA | I2C_SDA | top side |
| 23 | SCL | I2C_SCL | top side |
| 24 | INT | KEYPAD_INT | top side, left end |
| 25 | EP | GND | centre (2.6 × 2.6, 9 paste windows) |

Every pin matches the datasheet.

### Discretes

| Ref | Part | Pin | Function | Net |
|---|---|---|---|---|
| Q1 | AO3401A | 1 | G | Q1_G (100 k to GND) |
| | | 2 | S | VBAT_P |
| | | 3 | D | BAT+ |
| Q2 | AO3401A | 1 | G | VBUS |
| | | 2 | S | SYS |
| | | 3 | D | VBAT_P |
| Q3 | Si1308EDL | 1 | G | EPD_GDR |
| | | 2 | S | EPD_RESE |
| | | 3 | D | EPD_SW |
| D1 | B5819W | 1 | K | SYS |
| | | 2 | A | VBUS |
| D2 | B5819W | 1 | K | CAM_AF |
| | | 2 | A | CAM_2V8 |
| D3 | B5819W | 1 | K | EPD_PREVGH |
| | | 2 | A | EPD_SW |
| D4 | B5819W | 1 | K | GND |
| | | 2 | A | EPD_PUMP |
| D5 | B5819W | 1 | K | EPD_PUMP |
| | | 2 | A | EPD_PREVGL |
| D6 | B5819W | 1 | K | CHG_STAT_RAW |
| | | 2 | A | CHG_STAT |
| D7 | SMF5.0A | 1 | K | VBUS (x 120.65) |
| | | 2 | A | GND |
| L1 | SMNR4020 68 µH | 1 | – | +3V3 |
| | | 2 | – | EPD_SW |
| FB1 | 600 Ω bead | 1 | – | CAM_2V8 |
| | | 2 | – | CAM_AVDD |

### J1 camera (OV5640 MJY5OAF-F3M-V1), CamReversed, rot 180, pad row y = 162.58

| Pin / pad | Camera function | Board net | Pad x |
|---|---|---|---|
| 1 | STROBE | NC | 144.25 |
| 2 | AGND | GND | 144.75 |
| 3 | SDA | CAM_SIOD | 145.25 |
| 4 | AVDD 2.8 | CAM_AVDD | 145.75 |
| 5 | SCL | CAM_SIOC | 146.25 |
| 6 | RST | CAM_RESET | 146.75 |
| 7 | VSYNC | CAM_VSYNC | 147.25 |
| 8 | PWDN | CAM_PWDN | 147.75 |
| 9 | HSYNC | CAM_HREF | 148.25 |
| 10 | DVDD 1.5 | CAM_DVDD | 148.75 |
| 11 | DOVDD | CAM_2V8 | 149.25 |
| 12 | Y9 | CAM_D7 | 149.75 |
| 13 | MCLK | CAM_XCLK | 150.25 |
| 14 | Y8 | CAM_D6 | 150.75 |
| 15 | GND | GND | 151.25 |
| 16 | Y7 | CAM_D5 | 151.75 |
| 17 | PCLK | CAM_PCLK | 152.25 |
| 18 | Y6 | CAM_D4 | 152.75 |
| 19 | Y2 | CAM_D0 | 153.25 |
| 20 | Y5 | CAM_D3 | 153.75 |
| 21 | Y3 | CAM_D1 | 154.25 |
| 22 | Y4 | CAM_D2 | 154.75 |
| 23 | AF-GND | GND | 155.25 |
| 24 | AFVDD 2.8 | CAM_AF (via D2) | 155.75 |
| MP ×2 | shell tabs | GND | y 159.42 |

All 24 match. The 8-bit bus D0..D7 = Y2..Y9.

### J2 e-paper (Waveshare 2.13" V4, SSD1680), plain numbering, rot −90, pad column x = 179.48

| Pin / pad | V4 function | Board net | Pad y |
|---|---|---|---|
| 1 | NC | NC | 87.08 |
| 2 | GDR | EPD_GDR | 87.58 |
| 3 | RESE | EPD_RESE (3 Ω) | 88.08 |
| 4 | NC | EPD_VGL (cap only) | 88.58 |
| 5 | VSH2 | EPD_VGH (cap only) | 89.08 |
| 6 | TSCL | NC | 89.58 |
| 7 | TSDA | NC | 90.08 |
| 8 | BS1 | GND | 90.58 |
| 9 | BUSY | EPD_BUSY | 91.08 |
| 10 | RES# | EPD_RST | 91.58 |
| 11 | D/C# | EPD_DC | 92.08 |
| 12 | CS# | EPD_CS | 92.58 |
| 13 | SCL | EPD_CLK | 93.08 |
| 14 | SDA | EPD_DIN | 93.58 |
| 15 | VDDIO | +3V3 | 94.08 |
| 16 | VCI | +3V3 | 94.58 |
| 17 | VSS | GND | 95.08 |
| 18 | VDD | EPD_VDD (cap) | 95.58 |
| 19 | VPP | EPD_VPP (cap) | 96.08 |
| 20 | VSH1 | EPD_VSH (cap) | 96.58 |
| 21 | VGH | EPD_PREVGH (pump +) | 97.08 |
| 22 | VSL | EPD_VSL (cap) | 97.58 |
| 23 | VGL | EPD_PREVGL (pump −) | 98.08 |
| 24 | VCOM | EPD_VCOM (cap) | 98.58 |

### J3 / J4

| Ref | Pad | Net | Note |
|---|---|---|---|
| J3 (HX PM2.54-1x4P WT) | 1 | VBUS | x 123.69 |
| | 2 | GND | |
| | 3 | USB_DM | |
| | 4 | USB_DP | x 131.31 |
| J4 (S2B-PH-SM4-TB) | 1 | GND | left (x 141.6) |
| | 2 | BAT+ | right; opening faces +y |
| | MP | GND | |

Pads are 1.02 × 3.0 at 2.54 on J3 (matches the drawing) and the same as stock KiCad on J4.

### Land-pattern comparison summary

| Footprint | Datasheet recommendation | Footprint | Verdict |
|---|---|---|---|
| ESP32-S3-MINI-1 | 0.4 × 0.8 @ 0.85, rows 14, corners 0.8², EPAD 4.5 (9 × 1.2 @ 1.65) | identical | OK |
| FPC_24P…C6364666 (+CamReversed) | 0.3 × 1.2 @ 0.5; tabs 1.8 × 2.0; 4.75 overall; 2.55 tab edge → pin-1 centre | 0.3 × 1.2 @ 0.5; tabs 1.8 × 2.0 @ ±7.4; rows ±1.58 | OK |
| JST_PH_S2B-PH-SM4-TB_LCSC | KiCad stock: 1.0 × 3.5 @ y −2.85; MP 1.5 × 3.4 @ ±3.35, y 2.9 | 1.0 × 3.8 @ y −2.93; MP same | OK |
| PinSocket_1x04…RA_C46061768 | 1.02 × 3.0 @ 2.54 | same | CHECK (toe +0.1 mm, C4) |
| SOT-323_SI1308EDL | AN826: 0.56 × 0.65, span 2.44, pitch 1.24 | 0.9 × 0.8, span 2.4, pitch 1.3 | OK (oversize) |
| L_SMNR4020 | 0.95 × 3.3, gap 2.1 | 1.874 × 3.52, gap 1.99 | OK |
| HVQFN-24 4×4 (TCA8418) | EP 2.45, pads 0.24 × 0.6, span 3.8 | EP 2.6, pads 0.25 × 0.875 | OK / CHECK (C6) |
| KeyPad_* | n/a (bare ENIG interdigitated) | 2 nets each, B.Cu, no paste | OK |

## Sources

- Espressif, *ESP32-S3-MINI-1 & MINI-1U Datasheet v1.7*: documentation.espressif.com/esp32-s3-mini-1_mini-1u_datasheet_en.pdf
- LCSC datasheets via lcsc.com/datasheet/<C#>.pdf:
  - C6364666 SHOU HAN FPC 0.5-24P HYH2.0 drawing
  - C46061768 Hanxia HX PM2.54-1x4P WT
  - C295747 JST
  - C193402 MDD SMF5.0A
  - C841192 Richtek RT9080
  - C469327 Vishay Si1308EDL + AN826
  - C138713 TI TCA8418
  - C135265 SXN SMNR4020
  - C53099/C53100 Microne ME6211
  - C424093 Microchip MCP73831
  - C7519 ST USBLC6-2SC6
  - C15127 AOS AO3401A
  - C8598 JSCJ B5819W
- Seeed / Mingjiaying *OV5640_Camera_Module_Specification.pdf* (files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/new-res/)
- Waveshare *2.13inch_e-Paper_V4_Specification.pdf* (files.waveshare.com/upload/4/4e/)

Working files (pad dump, netlist, rendered drawings) are in the session scratchpad only. No design file was modified.
