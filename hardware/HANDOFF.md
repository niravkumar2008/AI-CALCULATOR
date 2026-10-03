# AI Calculator PCB — handoff for the KiCad Claude Code session

> **Shell switch (2026-10-02):** Nirav now uses a **Casio fx-115ES** shell, not the fx-300ES Plus. Same key grid, but it has paired screw posts (not 12 pegs), stubs on the side walls beside the screen, and rings and ribs in the back cover. The geometry below is still from the fx-300ES scan: the post holes, outline and key pads must be re-derived from the new readings and a flatbed scan (measurement sheet C7–C15, F6) before ordering.

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
| 6 routing | **ai_calc_pcb_10.zip (fx-115ES holes) = complete: DRC 0 errors, 0 unconnected, 0 schematic-parity warnings**, with the v8 camera bus (0.6 mm ribbon) and the stage-6 GPIO map. See stage9_finish.md |
| 7 outputs | `fab/` regenerated from the v10 board by `tools/make_outputs.sh` (now also runs on Windows, see its header). ORDER_CHECKLIST.md written |

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

## Stage 9 (ai_calc_pcb_9): v8 finished
All the v8 leftovers are fixed. Details and the reasons are in `stage9_finish.md`. The v7 fallback (`v7_drc_clean/`) is no longer needed, but it's kept for reference.
Firmware: `firmware-prototype/src/pins.h` now uses the stage-6 camera map. It builds.

## Stage 10 (ai_calc_pcb_10): moved to the fx-115ES shell
The holes now follow the fx-115ES posts, measured from Nirav's photos with a key-pad-fitted perspective transform (about 0.5 mm).
- H3 moved, H2/H6/H9 shifted, H5/H7/H11/H12 removed, H13/H14 added.
- The 4-way pad's UP/DOWN contacts are spread further apart.
- The top-left post cut-out is widened.
- DRC is 0/0/0, and `fab/` and the fit-check files are regenerated.

Back-cover ribs and rings were mapped onto the board (several sit over the camera, ESP32 and J1; grind plan in the doc). Still waiting on calipers: board outline (C6), LCD window and e-paper position (C12/C13, probably about 2 mm higher), wall stubs (C14) and rib heights (D13). Details in `stage10_fx115es.md`.

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
