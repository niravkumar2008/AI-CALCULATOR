# 08 — Second-opinion PCB check (v14), re-derived from the raw files

Date: 2026-10-06 (01:20–08:40 USEDT). Scope cut by the coordinator to checks 1–3 (CPL rotations, connector pin orders, ESP32 pin table); the gerber render and the DRC/ERC rerun were handed to the other agent (my one DRC/ERC run is still reported in §5 because it had already finished).
Nothing under `hardware/kicad/` was modified. Scripts saved: `hardware/tools/check_cpl_easyeda_08.py`, `hardware/tools/check_fp_outline_08.py` (run with KiCad 10 python; EasyEDA footprints fetched with `uvx --from easyeda2kicad easyeda2kicad --footprint --symbol`).

## Verdict

**No FAIL that blocks ordering.** Every CPL part lands pin-for-pin on the KiCad pads at the CPL rotation; the ESP32 pin table has no strapping/USB/PSRAM conflict; DRC/ERC are 0/0/0. Two items are **UNVERIFIABLE from the files** and must be settled on the bench (both are already called out in the earlier docs, so no disagreement): (a) which end of the JLC-placed magnet piece C46061768 is the N magnet, (b) the ribbon fold that decides whether J1's mirrored numbering is right. One minor disagreement with 05 (see last section).

---

## 1. CPL rotation vs EasyEDA footprint — PASS (77/77 parts)

Method: for each CPL line, `easyeda2kicad --lcsc_id` fetched the EasyEDA footprint for the BOM's LCSC number; the footprint was placed at the CPL (x, −y, rotation) with pcbnew and every EasyEDA pad was compared with the KiCad pad of the same number (centre offset) and hit-tested against any KiCad pad.

Numbers (worst same-number offset in mm; "landing" = EasyEDA pad → KiCad pad it sits on):

| Part(s) | LCSC | EasyEDA fp | KiCad rot | CPL rot | offset applied | worst Δ (mm) | Result |
|---|---|---|---|---|---|---|---|
| C1–C34, R1–R20, FB1 | passives | C0402/0603/0805/1206, R0402/0603, L0603 | = CPL | | 0 | 0.05–0.115 | PASS (symmetric) |
| D1–D6 | C8598 | SOD-123_L2.7-W1.6-LS3.7-RD-1 | 0 | 0 | 0 | 0.05 | PASS (EE pin1 "−" = K → KiCad pad 1 K) |
| D7 | C193402 | SOD-123FL_L2.7-W1.8-LS3.8-RD | 180 | 180 | 0 | 0.18 | PASS (EE pin1 = C on VBUS, pin2 = A on GND: correct for a unidirectional TVS) |
| Q1, Q2 | C15127 | SOT-23 …-BR | 0 / 90 | 180 / 270 | +180 | 0.212 | PASS (1→1 G, 2→2 S, 3→3 D; 0.21 is a land-pattern difference, a wrong rotation would be ≥1.9 mm) |
| Q3 | C469327 | SOT-323 …-BR | 0 | 0 | 0 | 0.00 | PASS |
| U2 | C424093 | SOT-23-5 …-BL | 0 | 270 | +270 | 0.013 | PASS |
| U3 | C841192 | TSOT-23-5 …-BL | 0 | 270 | +270 | 0.013 | PASS |
| U4, U5 | C53099/C53100 | SOT-23-5 …-BR | 0 | 180 | +180 | 0.163 | PASS |
| U7 | C7519 | SOT-23-6 …-BL | 0 | 270 | +270 | 0.013 | PASS |
| U6 | C138713 | WQFN-24 4x4 P0.5 | 0 | 0 | 0 | 0.083 | PASS (all 25 incl. EP) |
| U1 | C3013941 | BULETM-SMD_ESP32-S3-MINI-1-N8 | 90 | 90 | 0 | 0.03 | PASS (65 numbered + 8 EE "GND" thermal pads all on KiCad GND pads; EE antenna silk x 114.6–120.5 lies off the board-edge side as intended) |
| L1 | C135265 | IND-SMD SMNR4020 | 180 | 180 | 0 | 0.00 | PASS |
| J2 | C6364666 | FPC-SMD_24P-P0.50 | −90 | 270 | 0 | 0.00 | PASS, pin N → pad N, tabs 25/26 → MP |
| J4 | C295747 | S2B-PH-SM4-TB | 0 | 0 | 0 | 0.00 | PASS, 1→1, 2→2, 3/4 → MP |
| J1 | C6364666 | FPC-SMD_24P-P0.50 | 180 | 180 | 0 | 11.5 (by number) / 0.00 (geometry) | PASS geometrically; EE pin k lands on KiCad pad 25−k by design (`_CamReversed`) — see §2.1 |
| J3 | C46061768 | CONN-SMD_4P-P2.54_HX-PM2.54-1X4PWT | 0 | 180 | +180 | 7.62 (by number) / 0.00 (geometry) | PASS geometrically; EE pin 1 lands on KiCad pad 4 — see §2.3 |

`hardware/tools/jlc_cpl.py` offsets (+180 Q1/Q2/U4/U5/J3, +270 U2/U3/U7, 0 otherwise) are all confirmed; no edit to it was needed.

## 2. Connector pad-by-pad netlists

Netlist exported with `kicad-cli sch export netlist`; pad positions from pcbnew.

### 2.1 J1 — OV5640 24-pin FPC (`_CamReversed`, rot 180, pads at y 162.58, pad 1 at x 144.25, pad 24 at x 155.75)

KiCad pad → net, against the OV5640 24-pin DVP module table (camthink/Waveshare-type module: 1 NC, 2 AGND, 3 SDA, 4 AVDD, 5 SCL, 6 RESET, 7 VSYNC, 8 PWDN, 9 HREF, 10 DVDD, 11 DOVDD, 12 D9, 13 MCLK, 14 D8, 15 GND, 16 D7, 17 PCLK, 18 D6, 19 D2, 20 D5, 21 D3, 22 D4, 23 NC/GND, 24 AF):

1 NC | 2 GND | 3 CAM_SIOD | 4 CAM_AVDD | 5 CAM_SIOC | 6 CAM_RESET | 7 CAM_VSYNC | 8 CAM_PWDN | 9 CAM_HREF | 10 CAM_DVDD (1.5 V) | 11 CAM_2V8 (DOVDD) | 12 CAM_D7=Y9 | 13 CAM_XCLK | 14 CAM_D6=Y8 | 15 GND | 16 CAM_D5=Y7 | 17 CAM_PCLK | 18 CAM_D4=Y6 | 19 CAM_D0=Y2 | 20 CAM_D3=Y5 | 21 CAM_D1=Y3 | 22 CAM_D2=Y4 | 23 GND | 24 CAM_AF | MP GND.

**All 24 match the module table by pad number — PASS.** D0..D7 = Y2..Y9 follows esp32-camera naming and `pins_final.h`.

Physical mapping: the socket's own contact k (EasyEDA numbering, contact 1 at x 155.75) carries module pin 25−k. That is right only if the ribbon reaches the socket so that the module's finger 1 is at the x 144.25 end, i.e. the "two bottom-contact sockets facing each other, same-side-contact cable, no fold" arrangement. **This cannot be settled from the files** (it depends on where the module sits in the shell). If it is wrong the camera is simply dead (CAM_DVDD on module GND, no over-voltage: AVDD/DOVDD would land on data pins at ≤2.8 V) and the fix is a 180° twist of the ribbon or a cable with contacts on the other face, no board change. Keep the meter test in 01 (fingers 2↔15 beep).

### 2.2 J2 — 2.13" e-paper 24-pin FPC (rot −90, pad 1 at (179.48, 87.08), pad 24 at (179.48, 98.58), contact k = pad k)

1 NC | 2 EPD_GDR | 3 EPD_RESE | 4 EPD_VGL | 5 EPD_VGH | 6 NC (TSCL) | 7 NC (TSDA) | 8 GND (BS1 → 4-wire SPI) | 9 EPD_BUSY | 10 EPD_RST | 11 EPD_DC | 12 EPD_CS | 13 EPD_CLK | 14 EPD_DIN | 15 +3V3 (VDDIO) | 16 +3V3 (VCI) | 17 GND (VSS) | 18 EPD_VDD | 19 EPD_VPP | 20 EPD_VSH | 21 EPD_PREVGH | 22 EPD_VSL | 23 EPD_PREVGL | 24 EPD_VCOM.

This is the SSD1680 GDEY0213B74 / Waveshare 2.13 V4 order (1 NC, 2 GDR, 3 RESE, 4 VGL, 5 VGH, 6 TSCL, 7 TSDA, 8 BS1, 9 BUSY, 10 RES#, 11 D/C#, 12 CS#, 13 SCL, 14 SDA, 15 VDDIO, 16 VCI, 17 VSS, 18 VDD, 19 VPP, 20 VSH, 21 PREVGH, 22 VSL, 23 PREVGL, 24 VCOM) — **PASS**, from memory of the Good Display spec; the Waveshare wiki page returned 404 tonight, so re-confirm pins 4/5 against the GDEY0213B74 PDF when it is to hand. Booster parts: Q3 SI1308EDL G=GDR, S=RESE, D=SW; L1 3V3→SW; D3 SW→PREVGH; D4 PUMP→GND (K=GND); D5 PREVGL→PUMP; R12 3 Ω in RESE — the Good Display reference circuit. The socket is dual-contact, so the ribbon face does not matter; the pin-1 end does (same bench check as J1: panel finger 1 must be at the y 87.08 end).

### 2.3 J3 — magnet USB piece (KiCad pad 1 at x 123.69 … pad 4 at x 131.31, y 72.00)

Nets: 1 VBUS, 2 USB_DM, 3 USB_DP, 4 GND; silk "N"/"+" at pad 1, "−" at pad 4.
Cable: the Adafruit #5412 drawing (fetched from cdn-shop.adafruit.com) shows N at top, S at bottom, pins 2.54 pitch, 7.62 span, with the "−" label next to the N end and "+" next to the S end, D+/D− between. Mated face to face (cable N on piece S) the piece reads VBUS, D−, D+, GND from its own N end — **matches pads 1–4. PASS.** Backwards piece/plug: VBUS↔GND swap, D7 conducts forward, host current-limits, D+/D− swap — survivable, no 5 V on a data line.

**Open (unverifiable from files):** JLC will place part C46061768 (Hanxia HX PM2.54-1x4P WT) with its EasyEDA pin 1 at x 131.31 = KiCad pad 4 (GND). The CPL +180 was derived from the body side only (body on −y of the pads, which both footprints agree on). The LCSC datasheet fetch returned no drawing, so I could not tell whether the Hanxia part's N magnet is at its pin 1 or pin 4. If N is at EE pin 1, the placed piece has N at GND and the cable will only mate reversed. **Cheapest fix if it turns out wrong:** nothing on the board — swap the two outer wires in the cable or fit the Adafruit #5358 piece by hand (the docs already plan a meter check before gluing). If the PCB agent can read the Hanxia drawing on LCSC and N is at pin 1, change `ROT_BY_LCSC["C46061768"]` in `jlc_cpl.py` from 180 to 0 (geometry is symmetric so pads still land) — only then.

### 2.4 J4 — JST-PH 2-pin (pad 1 at x 141.60 = GND, pad 2 at x 143.60 = BAT+; EE pin 1 at the same place)

Adafruit/SparkFun convention: S2B-PH pin 1 = black/negative, pin 2 = red/positive (a SparkFun forum note warns the Eagle origin on pin 1 is often misread as "+" when it is the negative pin). **PASS**, consistent with Adafruit #1317/#1570 packs. Q1 (AO3401A, S=VBAT_P, D=BAT+, G=Q1_G) protects a reversed pack anyway.

## 3. ESP32-S3-MINI-1-N4R2 pin table — PASS

From the netlist (U1 pad → net): IO0 BOOT (TP only), IO1 I2C_SDA, IO2 I2C_SCL, IO3 NC, IO4 KEYPAD_INT, IO5 EPD_CLK, IO6 EPD_DIN, IO7 KEY_ON, IO8 EPD_CS, IO9 VBAT_SENSE, IO10 CAM_D2, IO11 CAM_D1, IO12 CAM_D3, IO13 CAM_D0, IO14 CAM_D4, IO15 CAM_PCLK, IO16 CAM_D5, IO17 CAM_D6, IO18 CAM_XCLK, IO19 USB_DM, IO20 USB_DP, IO21 CAM_D7, IO26 NC, IO33 EPD_BUSY, IO34 CAM_PWR_EN, IO35 CHG_STAT, IO36 CAM_VSYNC, IO37 VBUS_SENSE, IO38 CAM_RESET, IO39 CAM_SIOC, IO40 CAM_SIOD, IO41 EPD_DC, IO42 EPD_RST, TXD0/RXD0 UART test pads, IO45 NC, IO46 NC, IO47 CAM_HREF, IO48 CAM_PWDN, EN, 3V3, all GND pads GND.

- Strapping 0/3/45/46: only IO0 used, and only as a test pad with no pull — PASS (IO45 floats low internally = 3.3 V flash; IO46 default pull-down fine).
- USB 19/20: only USB_DM/USB_DP, through U7 (1/6 = DP, 3/4 = DM, 5 = +3V3 — USBLC6 VBUS pin on 3V3 is fine, it only sets the clamp level) — PASS.
- PSRAM on N4R2 (quad): IO26 unused (NC) — PASS; IO33–37 are free on quad modules and are used (BUSY, PWR_EN, CHG_STAT, VSYNC, VBUS_SENSE) — PASS, but note the BOM must stay N4R2 (an octal N8R8 substitution would kill those five).
- JTAG 39/40/41/42 reused as SCCB and EPD DC/RST — PASS (USB-Serial-JTAG is the debug path; JTAG pins are plain GPIO after boot).
- Input-only: the S3 has none — PASS.
- All 35 GPIOs in `pins_final.h` match the netlist exactly (every name/number checked above). No conflict found.

## 4. Gerber sanity — not run (cut from scope by the coordinator; the pcbnew outline check that did run found 139 Edge.Cuts segments, every endpoint shared by exactly two segments, bbox 71.75 × 148.86 mm, i.e. the outline is closed).

## 5. DRC / ERC (ran before the scope cut)

`kicad-cli pcb drc --schematic-parity --refill-zones --severity-all`: **0 violations, 0 unconnected, 0 parity issues** (no exclusions in the .kicad_pro). `kicad-cli sch erc --severity-all`: **0** on all 7 sheets.

## Disagreements with verification/01–05 and 06

- **05 line 136 "J3 … PASS: body direction checked by outline (C5)"**: agreed on the body, but 05 treats the N/S end of the JLC-placed Hanxia part as settled by the silk; it is not derivable from the files (see §2.3). Downgrade to CHECK-on-bench, which 01/03 already require.
- **01 C1 (J1 CamReversed, 85–90 % confidence)**: same conclusion by number; I could not improve the confidence from the files either.
- **03 line 20 "FATAL J3 order"**: superseded, confirmed fixed in v14 (1 VBUS, 2 D−, 3 D+, 4 GND).
- No disagreement with 02, 04 (rotation table matches mine exactly), or 06.

## Resolution of open item (a), 2026-10-06 (coordinator)

LCSC C46061768 is the Hanxia **HX PM2.54-1x4P WT**: a plain 4-pin 2.54 mm right-angle SMD female socket with no magnet and no keyed pin 1 (https://www.lcsc.com/product-detail/C46061768.html). Its 4 pads are symmetric, so the CPL rotation (180 or 0) places an identical part either way. **No change to jlc_cpl.py.** The "N" end belongs to the Adafruit #5358 magnet piece Nirav plugs into this socket: it is set by the board's "N" silk at pin 1 (VBUS) and the meter check before gluing. Item (a) is CLOSED for ordering.
