# 03 — External interfaces, end to end (stage 13b board)

> **⚠ Superseded by stage 14: see `../FINAL_STATUS.md` and `05_stage14_recheck.md`.** F1 (J3 pin order) is fixed: J3 = 1 VBUS, 2 D−, 3 D+, 4 GND, N end at pin 1. Its "Do not order yet" verdict applied to stage 13b; v14 was re-checked GO.

Read-only check, 2026-10-04. Nothing in the design or firmware was changed.

**Method**
- Netlist: exported with `kicad-cli sch export netlist` from a scratch copy of the schematic.
- Pad positions: KiCad 10 `pcbnew` python on a scratch copy of `ai_calc.kicad_pcb`.
- Datasheets and photos:
  - Seeed/Mingjiaying OV5640 module drawing MJY5OAF-F3M-V1, rendered from its vector PDF.
  - XIAO ESP32S3 Exp. Board v1.0 schematic, rendered from its vector PDF.
  - Waveshare 2.13" e-Paper V4 spec (rev 4.0, SSD1680Z8) and the 2.13" HAT schematic.
  - Adafruit #5412 cable drawing `5412_C17238_4P.pdf`.
  - Adafruit #5358 drawing (MG04254FRA1S1N, in `geometry/`).
  - Nirav's photos: `fb0c138c` (HAT), `2a97120d` (key mat), `e59eff71` (fx-115ES face).

## Verdict

**Do not order yet. One FATAL item: the USB magnet pin order on J3.** The Adafruit #5412 cable's contact order is GND, D+, D−, VBUS (from its N end). However the board-side magnet piece is turned, J3 sees one of these two orders:
- VBUS, D−, D+, GND
- GND, D+, D−, VBUS

J3 is wired VBUS, GND, D−, D+, so it matches neither:
- **One way:** USB never works, and the board's ground reaches the host only through the ESD diodes.
- **The other way:** 5 V lands on USB_DP and is clamped into +3V3 through the USBLC6 (ESP32 damage).

The fix is a net swap on J3. It's small, but it needs a schematic and board change.

Everything else traces correctly from part to GPIO to firmware:
- **Camera:** pin map, power rails and the reversed J1 footprint are all right. I now rate the footprint ~90 %, up from 75 %.
- **E-paper:** pin map, BS1 = 4-wire, booster = HAT reference, pin 1 lands on pad 1 after the fold.
- **Keypad:** 50 pads on B.Cu, unique row/column pairs, firmware map = schematic, ON on RTC IO7.

There are also three LIKELY PROBLEMs:
- The camera ribbon has almost no spare length (the docs say 10–12 mm).
- The firmware and silk use fx-300ES key names for two fx-115ES keys.
- The e-paper FPC stiffener versus the slot-to-J2 distance.

## Findings (most severe first)

### FATAL

**F1. J3 magnet pin order can't match the #5412 cable in either orientation.**
- **Cable drawing** (`5412_C17238_4P.pdf`). Text positions taken from the PDF along the pin axis: N 441, "−" 430, "D+" 421, "D−" 413, "+" 406, S 398. So the cable face reads, N → S: **GND, D+, D−, VBUS**. Its own "+" is next to S.
- **When mated,** the cable's N faces the board piece's S (opposite poles attract), and the pin line is preserved. So the board-side #5358 piece reads, from **its** N end: **VBUS, D−, D+, GND**.
- **On the board,** the piece's legs go into J3, and either end can face pin 1 (the legs are straightened into a plain header, so nothing keys it). Netlist: J3 1 = VBUS, 2 = GND, 3 = USB_DM, 4 = USB_DP. Pads at x 123.69 / 126.23 / 128.77 / 131.31.

| Orientation | J3-1 (VBUS) | J3-2 (GND) | J3-3 (USB_DM) | J3-4 (USB_DP) | Result |
|---|---|---|---|---|---|
| N at pin 1 (as `pins_final.h` documents) | VBUS ✓ | host D− | host D+ | **host GND** | No USB. The board's GND returns to the host only through the D+ line's ESD diodes (USBLC6, ESP32 pad), so the charge current flows through the clamp diodes. |
| N at pin 4 (the "turn it over" advice in QUESTIONS #7 and REVIEW_final step 2) | **host GND** | host D+ | host D− | **5 V** | 5 V on USB_DP. USBLC6 I/O1 → its VBUS pin (= +3V3) forward-biases, so +3V3 is pulled to about 4.3 V (ESP32 max 3.6 V). **Likely kills the ESP32 and the panel.** |

- **Fix (board change):** J3 = 1 VBUS, 2 USB_DM, 3 USB_DP, 4 GND, with the piece's N end at pin 1. Or keep the nets and use the mirror order GND / USB_DP / USB_DM / VBUS with N at pin 4.
  - VBUS then sits beside D− instead of GND, which the `pins_final.h` comment tried to avoid. The cable fixes the order, so this can't be helped. D7 and the USBLC6 still clamp.
- **Also:**
  - Update the commented `MAG_PIN_ORDER` in `pins_final.h`.
  - Withdraw the "if mirrored, turn the magnet piece over" advice (QUESTIONS_AND_ISSUES #7, REVIEW_final ordering step 2).
  - Meter-check the real cable (N-end pin to USB-A pin 4 = GND) before the first plug-in. My confidence in the cable order is ~85 % (a vendor drawing, not a measurement).

### LIKELY PROBLEM

**L1. Camera ribbon: about 1 mm spare, not 10–12 mm.**
- **The 70.5 includes the module.** The module drawing's 70.5 ± 0.2 runs from the **module's far edge** to the FPC tip. I checked the dimension's extension lines in the rendered vector drawing. The ribbon beyond the module is 62 mm.
- **What the docs assumed:**
  - stage13 §4: "about 10 mm to spare, let it lie in a gentle S"
  - `final_assembly/assembly_report.md`: "path 58.7, cable 70.5, ~12 mm loop"
  - Both treat 70.5 as the ribbon alone.
- **The path:**
  - The module is centred at (150, 95.1), so its far edge is at y 90.85.
  - The J1 body front (mouth) is at y 157.46 (footprint silk, rot 180).
  - The drawing's white insertion-depth line sits 2.5 mm from the tip.
  - Fully seated, the tip sits at about 159.96, which needs 69.1 mm laid dead straight. That leaves **≈ 1.4 mm** spare, before the extra length for climbing over parts and the rib (assembly report: it lies on the tallest part in its path).
- **Effect:** it probably still reaches, but only laid straight and taut. The "S" loop isn't possible.
  - If it comes up short, the module ends up 0.5–1 mm towards J1 from the window centre. The 7 mm window gives about ±0.85 mm.
  - Before ordering, measure the real module (far edge → tip). If it's under 70.3, or the path is longer than modelled, move J1 about 2 mm towards the camera (y ≈ 159).

**L2. fx-115ES key names: the firmware and schematic use fx-300ES names for two keys.**
- **What the real face shows:** photo `e59eff71` (fx-115ES, shift legends visible). Row 2 is **CALC (SOLVE), ∫dx (d/dx), x⁻¹ (x!), log□□ (Σ)**.
- **What the board and firmware say:** schematic values and `keys.cpp kMatrix[1][6..9]` = **Abs, x³, x⁻¹, log_a b** (the 300ES Plus row).
- **On the 115ES,** Abs is SHIFT+hyp and x³ is SHIFT+x².
- **Effect:** wiring is unaffected (SW4 at R1C6 and SW5 at R1C7 still sit under those two keys). But pressing CALC gives Abs and ∫ gives x³, and the SHIFT layer for the 115ES differs (e.g. hyp→Abs, x²→x³, ((→%, ENG→←, RCL→STO).
- **Fix:** firmware only (DKey names and the SHIFT table), plus the silk/schematic value text for SW4/SW5. No board change.

**L3. The e-paper FPC stiffener may not leave room to bend into the slot.**
- **The panel:** the V4 spec drawing (p. 6) gives the FPC as 14.30 ± 0.3 long with a 0.30-thick STIFFENER of **6.00 ± 0.5**. I read the 6.00 as the stiffener's length, but the drawing is ambiguous.
- **The board:**
  - The J2 mouth (silk) is at x 174.36 and the slot's far wall at x 172.4.
  - The contacts are at about 3 mm insertion, so the stiff part would reach x ≈ 171.4–171.9. That is **0.5–1 mm past the slot**, but the riser must bend inside the slot.
  - The stage 4 length budget (14.3 = 14.3) also has no spare.
- **Counter-evidence:** on Nirav's HAT (`fb0c138c`) the ribbon wraps the PCB edge only ~1.3 mm before the connector mouth, so the stiff end may be shorter than 6 mm.
- **Action before ordering:** measure on the real panel how long the thick, stiff end is, and do the paper mock-up (E7). If it's more than about 5 mm, move J2 about 1 mm away from the slot (+x).

### CHECK

- **C1. Camera DVDD** (module pin 10, "DVDD 1.5 V / internally supplied"). The OV5640's own 1.5 V regulator is on. We also feed 1.5 V from U5 (the XIAO feeds 1.3 V from an SGM2036-1.3 on that pin). Two regulators at the same voltage in parallel, where neither sinks current, is normal for these modules. No change, but if the camera misbehaves, U5 is the suspect (it could be left unpopulated).
- **C2. Camera DOVDD.** The module drawing labels pin 11 "DOVDD 1.8 V" (in red). We supply 2.8 V, exactly as the XIAO Exp board does (pin 11 = VCC_2V8). The OV5640 allows 1.71–3.0 V. The 3.3 V ESP32 outputs into 2.8 V I/O stay within DOVDD + 1 V, as on the XIAO. OK.
- **C3. E-paper pins 4/5 naming.** The V4 spec names pin 4 NC ("keep open") and pin 5 VSH2. VGL and VGH are on 23 and 21. The board labels 4/5 EPD_VGL/EPD_VGH with 1 µF each to GND. This is exactly the Waveshare HAT reference (C1/C2 on pins 4/5), so it works. Only the net names are misleading.
- **C4. E-paper image orientation.** `setRotation(1)` with the panel's FPC on the left as seen from the front. The picture may come out upside down. That's a one-line firmware fix, no hardware risk.
- **C5. J2 mouth position.** The footprint silk puts the body front at x 174.36. `assembly_report.md` (STEP model) says 172.73. The STEP probably includes the open lid. Settle it with the real part before deciding L3.
- **C6. B5819W instead of MBR0530** in the booster (40 V / 1 A vs 30 V / 0.5 A Schottky). More capacitance, works the same. OK.
- **C7. Ghosting.** The TCA8418 matrix has no diodes. Two-key chords (SHIFT/ALPHA + key) can't ghost. A three-key rectangle could (e.g. SHIFT + ALPHA + a/b gives a phantom √). It's rare on a calculator, so firmware can ignore ≥ 3 simultaneous keys.

### OK

- **Camera pin map:** all 24 J1 pins match the module drawing and the XIAO Exp board connector (same pin numbering, same rails). GPIOs match `pins_final.h` and the firmware `kCameraVariants` (table A).
- **J1 reversed footprint (~90 %):**
  - The module drawing's *Bottom view* shows the gold fingers on the module's base side. Its "1" and "24" labels put **pin 1 on the right with the tip pointing down**, i.e. on the left when you look at the fingers with the tip pointing away.
  - The module is lens-up with the ribbon flat towards +y, so the fingers face the board. Seen from above, pin 1 is at **smaller x**.
  - J1 pad 1 is at x 144.25 and pad 24 at 155.75, so cable pin N lands on pad N.
  - C6364666 is "double-sided contacts" (LCSC), so fingers-down is fine.
  - The XIAO uses the standard numbering because there the module folds back over a top-contact AFC01-S24FCC-00 with its fingers up, the mirror of our case. The old "argues against" point is explained.
  - The 2 ↔ 15 beep check still applies (23 is also GND).
- **Camera power:**
  - AVDD 2.8 V via FB1 + C13. DOVDD 2.8 V (U4).
  - DVDD 1.5 V (U5). AF 2.8 V via D2 Schottky, like the XIAO's MSK4005.
  - Pin 23 AF-GND = GND, pin 1 STROBE = NC.
  - Both regulators from SYS, enabled by IO34 with a 100 k pull-down.
  - RESET: 10 k to 2V8 + 100 nF. PWDN: 10 k pull-down. SCCB pull-ups 4.7 k to 2V8.
  - The firmware power-up order (RESET low → PWR_EN → 5 ms → RESET high) is fine.
- **E-paper pin map** (table B): every pin matches the V4 spec and the HAT schematic. BS1 (8) = GND = 4-wire SPI. VDDIO/VCI (15/16) = +3V3. VSS = GND. 1, 6 and 7 are NC.
- **E-paper booster = HAT reference:**
  - L1 68 µH +3V3 → SW. Q3 SI1308: G = GDR, S = RESE, D = SW.
  - R11 10 k GDR→GND. R12 3 Ω RESE→GND (the B/W value).
  - C20 4.7 µF 50 V SW→PUMP.
  - D3 SW→PREVGH (pin 21). D4 PUMP→GND. D5 PREVGL→PUMP (pin 23). The diode directions match the HAT netlist.
  - 1 µF 50 V on 4, 5, 18–24. 4.7 µF on +3V3 at L1.
- **E-paper FPC orientation:**
  - Spec block diagram and drawing: contacts on the display side, pin 1 on the left with the display facing you and the tip pointing down. Nirav's HAT photo agrees: the "1" mark is on the left, contacts facing out after the wrap.
  - The display faces out of the B side, the FPC leaves the panel's +x edge, folds 180°, rises through the slot and turns +x into J2. Tracking the faces through the folds, the contacts end up facing the board (down), which suits the dual-contact socket. Pin 1 stays at **smaller y**.
  - J2 pad 1 is at y 87.08 and pad 24 at 98.58, so pin N lands on pad N.
- **E-paper firmware:** `GxEPD2_213_GDEY0213B74` is the SSD1680 V4 panel. SPI: CLK 5, MOSI 6, CS 8, DC 41, RST 42, BUSY 33 at 4 MHz (write limit 20 MHz). These match the netlist.
- **Keypad pads:**
  - All 50 SW footprints are on B.Cu: the pads are on B.Cu + B.Mask with no paste and nothing on F.Cu.
  - Every centre equals `keys.csv`. UP and DOWN are deliberately moved −1.27 / +1.04 mm for the fx-115ES (stage 10, two photo fits at RMS 0.42–0.48 mm).
  - Mat photo `2a97120d` check: UP sits just above the SHIFT row, LEFT/RIGHT between, DOWN level with row 2, all as on the board.
  - Pad sizes: number keys 9.0 × 7.0; function, top and bottom rows 6.0 × 4.5; the 4-way 5.0 × 4.0. Each pad has interdigitated fingers 0.45 mm wide with a 0.3 mm gap, so any part of the carbon dot bridges them.
  - Dots in the photo: about 4.5 mm on the small keys and about 6 mm on the number and bottom rows. Each dot covers its pad with about ±1 mm of margin. The bottom row's big dots on 6.0 × 4.5 pads are fine (a bigger dot only helps).
- **Keypad wiring:**
  - 49 keys on 49 unique (ROW, COL) pairs, ROW0–7 × COL0–9, with no duplicates.
  - The firmware `kMatrix` equals the schematic for every key.
  - The TCA8418 event code (row × 10 + col + 1) is decoded correctly. KP_GPIO = FF/FF/03 enables 8 rows × 10 columns.
  - ON = SW50 between KEY_ON (IO7, an RTC pin, 100 k pull-up R17) and GND. EXT1 wake on IO7 and IO4 is valid.
- **USB to the GPIOs:** USB_DM goes to U1 pad 23 (IO19, D−) and USB_DP to pad 24 (IO20, D+), both fixed by the chip. USBLC6: I/O1 (1/6) = D+, I/O2 (3/4) = D−, pin 2 = GND, pin 5 = +3V3. D7 TVS sits on VBUS. (The J3 order is the problem, F1.)

## Table A — camera trace

Module pin = FPC finger (MJY5OAF-F3M-V1). J1 pad = same number (CamReversed, pads at y 162.58, x = 144.25 + 0.5·(n−1)).

| Pin | Module signal | J1 net | ESP32 | Firmware | XIAO Exp board pin |
|---|---|---|---|---|---|
| 1 | STROBE | NC | – | – | NC |
| 2 | AGND | GND | – | – | AGND |
| 3 | SDA | CAM_SIOD | IO40 (pad 36) | SIOD 40 ✓ | IO40 CAM_SDA |
| 4 | AVDD 2.8 | CAM_AVDD (FB1 ← CAM_2V8) | – | – | AVCC_2V8 |
| 5 | SCL | CAM_SIOC | IO39 (35) | SIOC 39 ✓ | IO39 |
| 6 | RST | CAM_RESET | IO38 (34) | RESET 38 ✓ | RC to 3V3 |
| 7 | VSYNC | CAM_VSYNC | IO36 (32) | VSYNC 36 ✓ | IO38 |
| 8 | PWDN | CAM_PWDN | IO48 (30) | PWDN 48 ✓ | 10 k to GND |
| 9 | HSYNC/HREF | CAM_HREF | IO47 (27) | HREF 47 ✓ | IO47 |
| 10 | DVDD 1.5 / internal | CAM_DVDD (U5 1.5 V) | – | – | "VCC_1V8" = 1.3 V LDO |
| 11 | DOVDD | CAM_2V8 (U4) | – | – | VCC_2V8 |
| 12 | Y9 | CAM_D7 | IO21 (25) | D7 21 ✓ | IO48 |
| 13 | MCLK | CAM_XCLK | IO18 (22) | XCLK 18 ✓ | IO10 |
| 14 | Y8 | CAM_D6 | IO17 (21) | D6 17 ✓ | IO11 |
| 15 | GND | GND | – | – | DGND |
| 16 | Y7 | CAM_D5 | IO16 (20) | D5 16 ✓ | IO12 |
| 17 | PCLK | CAM_PCLK | IO15 (19) | PCLK 15 ✓ | IO13 |
| 18 | Y6 | CAM_D4 | IO14 (18) | D4 14 ✓ | IO14 |
| 19 | Y2 | CAM_D0 | IO13 (17) | D0 13 ✓ | IO15 |
| 20 | Y5 | CAM_D3 | IO12 (16) | D3 12 ✓ | IO16 |
| 21 | Y3 | CAM_D1 | IO11 (15) | D1 11 ✓ | IO17 |
| 22 | Y4 | CAM_D2 | IO10 (14) | D2 10 ✓ | IO18 |
| 23 | AF-GND | GND | – | – | GND |
| 24 | AF VDD 2.8 | CAM_AF (D2 ← CAM_2V8) | – | – | VCC_2V8 via MSK4005 |

Power-enable: CAM_PWR_EN goes to IO34 (pad 29) and the U4/U5 CE pins. The firmware PWR_EN is 34 ✓.

## Table B — e-paper trace

V4 spec pin → J2 pad (same number, x 179.48, y = 87.08 + 0.5·(n−1)) → net → destination.

| Pin | V4 spec | HAT Rev2.1 | Board net | Goes to |
|---|---|---|---|---|
| 1 | NC | HLT_CTL (NC) | NC | – |
| 2 | GDR | GDR | EPD_GDR | Q3 G, R11 10 k |
| 3 | RESE | RESE | EPD_RESE | Q3 S, R12 3 Ω |
| 4 | NC | "VGL" + 1 µF | EPD_VGL | C21 1 µF |
| 5 | VSH2 | "VGH" + 1 µF | EPD_VGH | C22 1 µF |
| 6 | TSCL | NC | NC | – |
| 7 | TSDA | NC | NC | – |
| 8 | BS1 | BS (jumper, default GND) | GND | 4-wire SPI ✓ |
| 9 | BUSY | BUSY | EPD_BUSY | IO33 ✓ |
| 10 | RES# | RST | EPD_RST | IO42 ✓ |
| 11 | D/C# | D/C | EPD_DC | IO41 ✓ |
| 12 | CS# | CS | EPD_CS | IO8 ✓ |
| 13 | SCL | SCLK | EPD_CLK | IO5 ✓ |
| 14 | SDA | SDI | EPD_DIN | IO6 ✓ |
| 15 | VDDIO | 3V3 | +3V3 | |
| 16 | VCI | 3V3 | +3V3 | |
| 17 | VSS | GND | GND | |
| 18 | VDD | cap | EPD_VDD | C24 1 µF |
| 19 | VPP | cap | EPD_VPP | C25 1 µF |
| 20 | VSH1 | cap | EPD_VSH | C26 1 µF |
| 21 | VGH | PREVGH | EPD_PREVGH | C27, D3 K |
| 22 | VSL | cap | EPD_VSL | C28 1 µF |
| 23 | VGL | PREVGL | EPD_PREVGL | C29, D5 A |
| 24 | VCOM | cap | EPD_VCOM | C30 1 µF |

## Table C — USB

| J3 pad (x) | Board net | → | #5412 contact if the piece's N is at pad 1 | if the piece's N is at pad 4 |
|---|---|---|---|---|
| 1 (123.69) | VBUS | D1/D7/U2/Q2/R4/C5 | VBUS ✓ | **GND** ✗ |
| 2 (126.23) | GND | | **D−** ✗ | **D+** ✗ |
| 3 (128.77) | USB_DM | IO19 (pad 23), USBLC6 3/4 | **D+** ✗ | D− ✓ |
| 4 (131.31) | USB_DP | IO20 (pad 24), USBLC6 1/6 | **GND** ✗ | **5 V** ✗✗ |

Required order: VBUS, D−, D+, GND from the piece's N end (pads 1 → 4).

## Table D — keypad (matrix as built = firmware `kMatrix`)

| | C0 | C1 | C2 | C3 | C4 | C5 | C6 | C7 | C8 | C9 |
|---|---|---|---|---|---|---|---|---|---|---|
| R0 | SHIFT | ALPHA | – | – | MODE | – | UP | DOWN | LEFT | RIGHT |
| R1 | a/b | √ | x² | x^□ | log | ln | Abs → **CALC** on 115ES | x³ → **∫dx** on 115ES | x⁻¹ | log□□ |
| R2 | (−) | °'" | hyp | sin | cos | tan | | | | |
| R3 | RCL | ENG | ( | ) | S⇔D | M+ | | | | |
| R4 | 7 | 8 | 9 | DEL | AC | | | | | |
| R5 | 4 | 5 | 6 | × | ÷ | | | | | |
| R6 | 1 | 2 | 3 | + | − | | | | | |
| R7 | 0 | . | ×10ˣ | Ans | = | | | | | |

ON: its own pin (IO7).
