# Independent electrical + manufacturability review (2026-10-03)

Read-only review before the JLCPCB PCBA order planned for Monday 2026-10-05. I did not change any design file.

**Inputs:** HANDOFF.md, stage2/stage3 notes, pins_final.h, the BOM, DESIGN_SUMMARY.md, a fresh netlist exported with `kicad-cli` from a *copy* of the six schematic sheets, and targeted greps of `ai_calc.kicad_pcb` (saved 00:05).

**Datasheets checked:**
- ESP32-S3-MINI-1/1U datasheet v1.7 (Espressif)
- MCP73831 DS20001984H (Microchip)
- AP2112 (Diodes Inc.)
- TCA8418 (TI)

**Parts:** live LCSC product pages for every BOM line.

## Plain-English summary

The circuit is in good shape. I traced every net in the netlist and **found no wiring mistake that would stop the board from working**:
- power path, charger, reverse-battery FET, load-share FET
- USB, ESP32 straps
- camera, e-paper booster, keypad scanner

All 33 LCSC numbers still exist and are in stock at LCSC today.

There is **one blocker, and it's about process, not the circuit**. The files in `fab/` (gerbers, placement file) and the last DRC report are older than the board file. Another session has moved parts since then. For example, the e-paper socket J2 sits at y = 92.83 in the board but at y = 96.4 in the CPL. Regenerate everything and re-run DRC before uploading.

The main electrical findings are about **standby battery life**. The goal is months, and today it's about 6 weeks. Two cheap fixes, both parts-list-only with no re-layout, take it to roughly 4–6 months:
1. Leave R20 off the board (firmware uses the chip's internal pull-up instead).
2. Swap the 3.3 V regulator U3 for a pin-compatible low-power one.

Beyond that, I'd check three things before paying:
- **TVS diode D7 direction.** Placed backwards, it shorts the charging cable.
- **Battery plug polarity**, with a meter.
- **Wi-Fi current from the tiny 100 mAh cell.** This one is handled in firmware.

---

## BLOCKER

### B1. `fab/` outputs and the DRC report are stale relative to the board file
- **Evidence:**
  - `kicad/ai_calc.kicad_pcb` was saved 2026-10-03 00:05.
  - `fab/*` was generated 2026-10-02 22:33, and `ai_calc_drc_violations.json` 22:28.
  - The CPL disagrees with the current board:

    | Part | CPL y | Board y |
    |---|---|---|
    | J2 | −96.4 | 92.83 |
    | L1 | −105.4 | 101.83 |
    | Q3 | −101.0 | 97.43 |

    So the e-paper group moved by about 3.6 mm.
  - JLCPCB would place parts at the old positions on a new copper pattern.
- **Fix:** once the editing session is finished:
  1. Re-run DRC with schematic parity (expect 0/0/0).
  2. Re-run `tools/make_outputs.sh`.
  3. Re-diff `fab/ai_calc_BOM_JLCPCB.csv` against `bom/`.
  4. Re-check the 3D/CPL preview on JLCPCB.
- **Schematic change:** no.
- **Also still open from HANDOFF.md:** the "don't order yet" calipers items (V3/V5/V9/V11/V12, C6/C12–C14). I didn't re-review these; they're mechanical.

No electrical BLOCKERs found.

---

## SHOULD-FIX

### S1. R20 (CHG_STAT pull-up to +3V3) back-feeds the charger and leaks about 20 µA in standby
- **Evidence (netlist):**
  - `CHG_STAT = D6.A, R20.2, U1.IO35`
  - `CHG_STAT_RAW = D6.K, U2.STAT`
  - R20.1 is on `+3V3`, which is always on.
- **What happens with no cable:**
  - U2's VDD = VBUS is held at 0 V by R4 + R5 (30 kΩ).
  - So the path is +3V3 → R20 (100 k) → D6 (forward) → STAT pin → the internal clamp from STAT to VDD → VBUS → 30 k → GND.
  - The MCP73831 STAT pin is a push-pull ("tri-state") output. Its absolute maximum is **−0.3 V to VDD + 0.3 V** (DS20001984H, Absolute Maximum Ratings, "All inputs and outputs w.r.t. VSS").
- **Estimated effect:**
  - About (3.3 − 0.15 − 0.6) / 130 kΩ ≈ **20 µA** continuous drain.
  - About **0.5 V on the exposed magnet VBUS contact**. That's small, but it's a battery-derived voltage on the contact.
  - The STAT pin sits outside its absolute-maximum rating, though the current is small.
- **Fix A (recommended, BOM only, no layout change):** mark **R20 DNP** (remove it from the BOM and CPL). In firmware:
  - Enable IO35's internal pull-up only while VBUS_SENSE (IO37) is high.
  - Disable it before sleep.

  Reading is unchanged: low = charging, high = done.
- **Fix B (schematic):** move R20's top end from `+3V3` to `VBUS_SENSE`. Then IO35 sits at about 3.1 V when done, about 0.2 V when charging, and 0 V with no cable.
- **Schematic change:** A = no (BOM/CPL only). B = yes, one wire.

### S2. U3 AP2112K quiescent current (55 µA typ, 80 µA max) is about 60 % of standby drain
- **Evidence:**
  - AP2112 datasheet: IQ = 55 µA typ / 80 µA max at VIN = 3.6 V.
  - ESP32-S3-MINI-1 deep sleep is only 7–8 µA (datasheet Table 6-7).
  - See the budget table below.
- **Fix (BOM only):** replace U3 with **Richtek RT9080-33GJ5, LCSC C841192**:
  - Ground current 2 µA, 600 mA output.
  - TSOT-23-5 with the same pin order: 1 VIN, 2 GND, 3 EN, 4 NC, 5 VOUT. It fits the existing SOT-23-5 land.
  - 39,775 in stock at LCSC today. It's extended, the same as AP2112K, so no extra fee.
  - **Trade-off:** dropout is 360 mV vs 250 mV at 600 mA. The firmware's 3.5 V AI cut-off already covers this; consider raising it to 3.6 V.
  - HANDOFF says AP2112K was "as specified". This is the owner's call, but it's the single biggest standby lever.
- **Schematic change:** value/LCSC field only. No net change.

### S3. TVS D7 (SMF5.0A) must be placed cathode-to-VBUS: backwards = dead short across the cable
- **Evidence:**
  - LCSC C193402 is an MDD SMF5.0A, **unidirectional**, package SOD-123FL.
  - The symbol uses bidirectional-style pin names A1/A2. A1 = pin 1 = VBUS, A2 = GND.
  - Pad 1 of `Diode_SMD:D_SMF` is the cathode, so the netlist is correct.
  - However, a mirrored or rotated placement would forward-bias the TVS straight across the 5 V cable.
- **Fix:**
  1. In the JLCPCB placement preview, confirm the cathode band is on the pad that connects to VBUS (J3 pin 1 / D1 anode side).
  2. Add a "K" or bar marker on the silkscreen if one is missing.
  3. Confirm the SOD-123FL body fits the D_SMF land. Both are about 2.7 × 1.8 mm with similar pads, so I expect it's fine.
- **Note:** its 400 µA maximum leakage only applies while a cable is plugged in, so it isn't a standby issue.
- **Schematic change:** no.

### S4. 100 mAh cell vs Wi-Fi peaks (brown-out risk in AI-solve mode)
- **Evidence:**
  - ESP32-S3 Wi-Fi TX peaks are about 350–500 mA, which is 3.5–5 C for the Adafruit #1570.
  - SYS has only C7 4.7 µF + C10/C14 1 µF. +3V3 has about 47 µF.
  - Cell resistance + protection FET + Q1/Q2 ≈ 0.3–0.6 Ω, so expect 0.15–0.3 V sag.
  - U3 dropout adds to this, so 3V3 can dip during TX bursts below about 3.7 V battery.
- **Fix:**
  - **Firmware:** limit TX power (for example `esp_wifi_set_max_tx_power(44)` ≈ 11 dBm) and keep the low-battery AI lock-out (about 3.6 V).
  - **Optional (layout):** add a 47–100 µF 0805/1206 cap on +3V3 next to U1 if space allows (for example C45783-class 22 µF ×2, basic).
- **Schematic change:** firmware only, unless you add the cap.

### S5. Confirm battery polarity at J4 before ordering
- **Evidence:** J4.1 = GND, J4.2 = BAT+. The Adafruit #1570 polarity is still listed as open in stage3_schematic.md. I couldn't confirm Adafruit's pin-1 convention from a primary source.
- **Why it matters:** Q1 protects the board from a reversed battery, but the owner can't re-crimp or solder. A mismatch means the battery is unusable on this board.
- **Fix:** with the actual battery, use a meter to find which plug contact is red (+). Compare it with the J4 footprint pin-1 marking in the 3D view before ordering.
- **Schematic change:** only if it's wrong (swap the J4 pin nets).

### S6. Firmware standby hygiene (needed to reach the standby numbers below)
Each of these can silently add hundreds of µA:
- **Drain the TCA8418 FIFO and clear INT_STAT before deep sleep.** Otherwise INT (IO4) stays low and R15 (10 k) draws **330 µA**.
- **Before turning the camera off:** set all camera GPIOs (IO10–18, 21, 36, 38–40, 47, 48) to disabled with no pulls, then pull IO34 low. Already noted in stage3; keep it.
  - IO39/IO40 are JTAG pins (MTCK/MTDO). Make sure no pull is left on them, or 3.3 V back-powers the camera through R18/R19.
  - Never drive PWDN (IO48) high while CAM_2V8 is off: about 330 µA through R9 and into the unpowered sensor.
- **Switch off USB-Serial-JTAG while VBUS_SENSE is low** (already noted).

---

## NICE (optional)

**N1. D1 (B5819W) reverse leakage.**
- SYS → D1 → VBUS → 30 kΩ → GND, with 3.7 V reverse bias in battery mode.
- The JSCJ part page doesn't list IR. Generic 1 A Schottkys show a few µA at 25 °C and about 10× that at 50–60 °C.
- Measure it on the first board (VBUS-to-GND voltage / 30 kΩ). If it's above about 5 µA, consider a low-leakage Schottky with ≥ 0.5 A rating.
- No schematic change.

**N2. VBUS_SENSE overshoot.**
- 10 k / 20 k gives 3.67 V on IO37 with a 5.5 V adapter, which is above VDD + 0.3 V.
- Change **R5 20 k → 15 k (C25756, 0402, 546 k in stock, basic)**. That gives 3.0 V at 5 V and 2.64 V at 4.4 V. Both are still above VIH = 2.48 V.
- BOM only.

**N3. OV5640 power-up order.**
- U4 (2.8 V) and U5 (1.5 V) share one enable (CAM_PWR_EN), so DOVDD, AVDD and DVDD rise together. The datasheet prefers DOVDD → AVDD → DVDD.
- Seeed's own boards do the same and work, and C18 + R8 give about 1 ms of reset delay.
- Suggested firmware order:
  1. IO38 (RESET) driven low, IO48 low.
  2. Enable power, wait 5 ms.
  3. Release RESET, wait 20 ms.
  4. Start SCCB.

**N4. +3V3 trunk is 0.25 mm everywhere.**
- About 2 mΩ/mm, so roughly 25 mV drop at 0.5 A from U3 to U1. That's acceptable.
- Widen it to 0.4–0.5 mm if you're editing that area anyway.

**N5. Antenna.**
- Keep-out (114.75–119.85, both layers, plus clearance zones) matches the datasheet's 5.05 mm antenna area. The antenna end is 0.9 mm from the board edge. Good.
- Keep the back-cover ribs near U1 plastic-only, and don't add screws or metal within about 10 mm of x < 120, y 85–109.

**N6. JLCPCB stock.**
- C1525 (100 nF 0402, a JLC *basic* part) shows only 100 pcs on the LCSC page. JLC's assembly warehouse is separate and normally holds millions. Confirm at BOM upload.
- ESP32-S3-MINI-1-N4R2 (C3013941): 1,148 at LCSC. Fine for 2 boards, but confirm on JLC.
- I couldn't read live JLC stock or basic/extended flags (the pages are JS-rendered). Re-confirm both on the BOM-match screen.

**N7. Cost.**
- Extended parts cost about $3 each in loading fees (14 types ≈ $42 per order).
- No safe basic substitutes for the ICs, connectors or ESP32.
- The RT9080 swap in S2 is cost-neutral (extended → extended).

**N8. E-paper J2 orientation.**
- I re-derived the FPC fold independently: 180° fold behind the panel, up through the slot, 90° into J2.
- Fingers end up facing the board, and panel pin 1 lands on the −y side. That matches J2 pad 1 at (179.48, 87.08) vs pad 24 at y 98.58.
- This agrees with stage4_5_layout.md. A paper-strip mock-up is still worth five minutes, because a mirrored e-paper tail can't be fixed by twisting.

---

## Standby (deep-sleep) current budget

| Always-on item | As designed | After S1 + S2 |
|---|---|---|
| ESP32-S3-MINI-1 deep sleep, RTC periph on for EXT0/EXT1 (datasheet Table 6-7) | 8 µA | 8 µA |
| U3 LDO quiescent current | 55 µA (80 max) | 2 µA (RT9080) |
| R20 → D6 → STAT → VBUS divider leak (S1) | ~20 µA | 0 |
| TCA8418 idle (datasheet: 3 µA) | 3 µA | 3 µA |
| VBAT divider R6/R7 1 M + 1 M | 1.9–2.1 µA | 2 µA |
| D1 reverse leakage (estimate, room temperature) | 1–10 µA | 1–10 µA |
| MCP73831 battery discharge, VDD < VSTOP (datasheet ≤ 1–2 µA) | ≤ 2 µA | ≤ 2 µA |
| E-paper SSD1680 deep sleep + Q3 / USBLC6 / TVS / FET leakage | ~1–2 µA | ~1–2 µA |
| U4 / U5 ME6211 disabled | ~0.2 µA | ~0.2 µA |
| Battery protection IC inside #1570 (typical DW01-class) | ~3 µA | ~3 µA |
| **Total** | **≈ 95–105 µA → about 6 weeks** from 100 mAh | **≈ 21–32 µA → about 4–6 months** (after LiPo self-discharge) |

These need firmware S6 (no stuck INT, no camera back-power). The 4 × 4.7 k I2C/SCCB pull-ups, the 100 k ON pull-up and the 10 k RESET pull-ups draw nothing while their lines are idle.

---

## Checked and OK (no action)

- **ESP32-S3-MINI-1-N4R2 pins** (datasheet v1.7, Table 3-1):
  - Pad numbers match the netlist (IO26 = 26, IO47 = 27, IO33 = 28, IO34 = 29, IO48 = 30, IO35–37 = 31–33, IO38–42 = 34–38).
  - Footnote b: on -N4R2 **only IO26** goes to the PSRAM. It's unconnected here.
  - IO33–37, IO47 and IO48 are free on N4R2. They're in the VDD_SPI domain, which is 3.3 V with GPIO45 low.
  - CAM_PWR_EN (IO34) has R10 100 k to GND, so the camera stays off when VDD_SPI powers down in sleep.
  - The settled camera map, VBUS_SENSE (IO37) and CHG_STAT (IO35) are all legal.
- **Straps:** GPIO0 (weak pull-up) goes only to TP1; GPIO3 floating; GPIO45/46 unconnected (weak pull-downs → 3.3 V VDD_SPI, normal boot). Table 4-1 confirms this.
- **EN:** R1 10 k + C4 1 µF is exactly the datasheet recommendation. Decoupling is 22 µF + 100 nF + 10 µF.
- **USB:** D− = IO19, D+ = IO20. USBLC6 pins 1/6 = D+ and 3/4 = D−, with its VBUS pin on +3V3 (good: no back-feed onto the magnet contact). Magnet order VBUS / GND / D− / D+.
- **Power path:**
  - Q1 AO3401A: D = BAT+, S = VBAT_P, gate 100 k to GND. This correctly blocks a reversed cell.
  - Q2: D = VBAT_P, S = SYS, G = VBUS. Its body diode feeds SYS from the battery. With a cable in, Vgs ≈ +0.3 V, so it's off.
  - D1 feeds VBUS → SYS. Magnet VBUS reaches only D1's anode, Q2's gate, U2 VDD, D7, C5 and the divider (apart from the S1 leak).
- **MCP73831-2ACI:** PROG 20 k → 1000 V / 20 kΩ = **50 mA** (0.5 C). VDD and VBAT have 4.7 µF each, as the datasheet asks. STAT is clamped by D6.
- **AP2112K:** EN tied to VIN (SYS). 4.7 µF in and ≥ 22 µF out (stable with ≥ 1 µF). VIN maximum 6 V is fine on SYS ≤ 4.7 V.
- **Camera:**
  - The 24-pin map matches Seeed's (pin 1 NC, AF on 24 via D2, GND on 2/15/23).
  - DOVDD and AVDD (via FB1, 200 mA / 0.45 Ω, fine) on 2.8 V; DVDD on 1.5 V.
  - SCCB pull-ups go to CAM_2V8. RESET has 10 k to CAM_2V8 plus 100 nF; PWDN has a 10 k pull-down.
  - XCLK 20 MHz is within OV5640's input range.
  - The 6.3 V 2.2 µF caps (C12530) are only on ≤ 2.8 V rails, which is fine.
- **E-paper booster:** matches the Waveshare/Good Display topology.
  - L1 (68 µH, Isat 0.6 A) from +3V3 to SW; Q3 Si1308EDL (30 V, G1/S2/D3) with GDR 10 k pull-down; RESE R12 = 3 Ω (B/W panel).
  - D3 SW → PREVGH; C20 SW → PUMP; D4 PUMP → GND (anode on PUMP); D5 PREVGL → PUMP. The diode directions give positive and negative pumps.
  - C20 4.7 µF 50 V X7R 1206 (C29823); all panel caps 1 µF 50 V (C15849).
  - BS1 to GND (4-wire SPI); TSCL/TSDA NC.
- **TCA8418:** pinout matches the datasheet (ROW7..0 = pins 1–8, COL0–9 = pins 9–18, RESET 20, VCC 21, SDA 22, SCL 23, INT 24). INT and RESET have 10 k pull-ups, I2C has 4.7 k, address 0x34, idle current 3 µA.
- **PCB / DFM:**
  - All vias 0.6 / 0.3 mm. Tracks ≥ 0.2 mm (0.15 mm clearance only in the J1 fan-out). That's JLCPCB 2-layer standard capability, no surcharge.
  - GND pours on both layers.
  - The settled J1 CamReversed footprint was not re-litigated.

## LCSC check (2026-10-03, lcsc.com product pages; all exist and are active)

| LCSC | Part | LCSC stock |
|---|---|---|
| C3013941 | ESP32-S3-MINI-1-N4R2 | 1,148 |
| C424093 | MCP73831T-2ACI/OT | 8,886 |
| C51118 | AP2112K-3.3TRG1 | 38,605 |
| C53099 / C53100 | ME6211C28M5G-N / ME6211C15M5G-N | 30,340 / 12,770 |
| C138713 | TCA8418RTWR | 28,376 |
| C7519 | USBLC6-2SC6 | 30,105 |
| C193402 | SMF5.0A (MDD, unidirectional, SOD-123FL) | 573,380 |
| C8598 | B5819W SL (JSCJ, 1 A 40 V) | 182,940 |
| C15127 | AO3401A | 391,775 |
| C469327 | SI1308EDL-T1-GE3 | 39,970 |
| C135265 | SMNR4020-68UH (Isat 0.6 A, DCR 1.38 Ω) | 5,020 |
| C6364666 | SHOU HAN FPC 0.5-24P dual-contact | 16,870 |
| C42379197 | HX 1x4P 2.54 SMD socket | 16,037 |
| C295747 | JST S2B-PH-SM4-TB | 19,760 |
| C1002 | Sunlord GZ1608D601TF 600 Ω | JLC page OK |
| C23157 | 3 Ω 0603 1 % | 35,800 |
| C29823 | 4.7 µF 50 V X7R 1206 | 213,430 |
| C15849 | 1 µF 50 V X5R 0603 | 474,200 |
| C45783 / C19702 / C19666 / C52923 / C12530 | 22 µF 25 V 0805 / 10 µF 10 V 0603 / 4.7 µF 16 V 0603 / 1 µF 25 V 0402 / 2.2 µF 6.3 V 0402 | 1.3 M / 3.2 M / 471 k / 2.6 M / 3.7 M |
| C1525 | 100 nF 16 V X7R 0402 (JLC basic) | **100 at LCSC**; check JLC (N6) |
| C25744 / C25765 / C25741 / C26083 / C25900 | 10 k / 20 k / 100 k / 1 M / 4.7 k 0402 | all > 1 M |
| *Proposed* C841192 | RT9080-33GJ5 (S2) | 39,775 |
| *Proposed* C25756 | 15 k 0402 (N2) | 546,100 |
