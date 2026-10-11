# AI Calculator PCB — handoff for the KiCad Claude Code session

Paste or attach this file in the new chat. It sums up a long cloud session (2026-10-01/02).
Repo: `C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR`. All hardware lives under `hardware/`.
Nirav is new to PCB design: explain non-obvious choices in 1–2 plain sentences. No screenshots of his screen. Deliver each stage as a sequential zip `ai_calc_pcb_N.zip` (he commits and pushes himself). Use KiCad 10.

## Product (fixed requirements)
- New board inside an unmodified Casio fx-300ES Plus case; the only drilled hole is a ~6 mm camera hole in the back cover.
- Parts: ESP32-S3-MINI-1-N4R2, Seeed OV5640 AF camera (24-pin FPC), Waveshare 2.13" V4 bare e-paper (24-pin FPC, on-board booster), TCA8418 keypad scanner, ON key on its own RTC pin.
- Power: MCP73831 charger, AP2112K-3.3, USBLC6-2, SMF5.0A TVS, reverse-battery FET + load-sharing FET.
- Battery: Adafruit #1570, 100 mAh, JST-PH. Charging comes in through an Adafruit #5358 magnetic connector.
- The magnet contacts must never carry battery voltage. No LED (exam mode). Nothing for Nirav to solder.
- JLCPCB build: 2 layers, 0.8 mm, ENIG, assemble 2 of 5. Use real LCSC numbers only and flag extended parts.
- Board geometry comes from the scan (`hardware/scan/9C102_A.pdf`, `hardware/geometry/derive_geometry.py`, `features.json`).
- KiCad top view = seen from the back cover. F.Cu is the component side (faces the back cover). B.Cu carries the 50 bare key pads and the e-paper panel.

## Where things stand
| Stage | State |
|---|---|
| 1 geometry | Done (ai_calc_pcb_1/2) |
| 2 pins/power | Done. `hardware/pins_final.h` is the source of truth (camera pins changed in stage 6, see below) |
| 3 schematic | Done. ERC 0/0. Every part has a live-checked LCSC number. Independent review fixes applied (stage3_schematic.md) |
| 4/5 footprints + placement | Done. Placement check: 0 overlaps / keep-out hits (stage4_5_layout.md) |
| 6 routing | **ai_calc_pcb_7.zip = complete, DRC 0/0** (autorouted). A layout review then asked for a cleaner camera bus and better decoupling, so **ai_calc_pcb_8.zip = work in progress** (see "Unfinished" below) |
| 7 outputs | Script ready (`hardware/tools/make_outputs.sh`). `fab/` in zip 7 has Gerbers/BOM/CPL for the DRC-clean v7 board. ORDER_CHECKLIST.md written |

**Don't order yet:** Nirav's caliper measurements (measurement-sheet artifact) can still move parts. Items V3/V5/V9/V11/V12 in geometry_assumptions.md need checking first.

## Key decisions you must not undo
- **Camera socket J1** uses `ai_calc:FPC_24P_P0.5mm_DualContact_C6364666_CamReversed`: the same land pattern with pad numbers mirrored. A flat cable going straight in lands mirrored on standard numbering; two independent checks agreed (~75 %).
  - Both FPC sockets are SHOU HAN C6364666 dual-contact parts, so if the reading is wrong, a 180° twist in the cable fixes it.
  - Before the first plug-in, check the cable: fingers 2 and 15 should beep (both GND).
- **E-paper:** the FPC folds behind the panel, comes up through a **1.0 × 14 mm slot at x = 172.9**, and plugs into J2 (x 177.9, opening facing the slot). Length budget is 14.3 mm.
- **Camera GPIO map re-ordered in stage 6** so the bus leaves the module in J1's pin order: XCLK 18, D0 13, D1 11, D2 10, D3 12, D4 14, D5 16, D6 17, D7 21, VSYNC 36, HREF 47, PCLK 15, PWDN 48, RESET 38 (SIOD 40, SIOC 39, PWR_EN 34 unchanged). The schematic (mcu sheet labels) and pins_final.h are already updated. **Firmware must use these.**
- **Review fixes:**
  - STAT → IO35 goes through Schottky D6 + 100 k pull-up R20, because STAT drives 5 V.
  - R12 = 3 Ω (B/W panel). C20 = 4.7 µF 50 V 1206.
  - VBUS divider = 10 k / 20 k. Charge current = 50 mA (R2 20 k).
  - SCCB pull-ups R18/R19 go to CAM_2V8. The USBLC6 reference pin is on +3V3.
- **Magnet socket J3** = 5 mm SMD female header C42379197 (no THT fee). Function-key and bottom-row pads are KeyPad_6.0x4.5.

## Unfinished (ai_calc_pcb_8 board)
- Layout-review fixes are placed: C27 next to D3, L1 rotated, decoupling at U1 pad 3, EN parts near the EN pin, U7 in line, J4 opening +y, TCA8418 moved to (126, 146.5), test pads moved, antenna clearance rule areas, no vias on key pads.
- The camera bus is pre-routed by `tools/prepass.py` as a clean 0.6 mm-pitch ribbon: from U1's right side, around the right of J1, entering J1 from below. That's good, but:
  - **11 clearance errors** where the bus stubs fan in under J1 (y ≈ 163.6–164.7). The grid router put the stub ends too close. Fix by hand: spread the last 1–2 mm into J1 pads 3–21.
  - **16 tiny dangling stubs** (0.004–0.06 mm leftovers at the stub/route joins). Delete them.
  - **Unconnected:** J1 pins 4 (CAM_AVDD), 10 (CAM_DVDD), 2/15/23 (GND), R10 CAM_PWR_EN, R5 GND. The bus now covers the area under J1's pads. Route these power/GND pins out *above*, under the connector body (allowed for power/GND), or drop to B.Cu with vias right below their pads before the bus passes.
  - After the fixes: refill zones, DRC 0, then `make_outputs.sh` → zip 9.
  - If this gets messy, the fallback is the v7 board (zip 7, also copied to `hardware/v7_drc_clean/`), which is DRC-clean. It uses the **old** camera GPIO map, so restore zip 7's `mcu.kicad_sch` and `pins_final.h` along with it. Its only weakness is a tighter camera bus (0.2 mm spacing); in that case drop XCLK to 10 MHz in firmware.

## Tools (in `hardware/tools/`; written for the cloud box, paths need adapting)
- `build_board.py`: places every part from the schematic netlist (placement table inside) and adds the slot, keep-outs, rule areas and netclasses.
- `prepass.py` + `gridroute.py`: A* pre-router for the camera bus, SYS and the +3V3 trunk.
- `route.sh`: Freerouting 1.9 headless.
- `add_gnd.py`: GND pours, via stitching, and a 2-vias-per-island rule.
- `cleanup.py`: via merging and silk placement.
- `fixroute.py`: leftover connections.
- `placement_check.py`, `check_keys.py`, `make_outputs.sh`.

## Open questions for Nirav
- Measurement sheet: https://claude.ai/artifact/SzK4TuLjkGAZ9tvzqEtqkq (42 items, including camera-cable continuity E6 and e-paper ribbon length E7).
- Cost: ~$190–240 total. JLCPCB fees ~$59.50, parts $9.77/board, 14 extended part types (doc: https://claude.ai/code/artifact/6bda3922-c9f2-47b0-8e37-4b1de94ca7f4).
- Next generation: a custom shell from a build123d script (3D print first, then a manufacturer). Not started.
