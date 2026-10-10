# Stage 13 progress (heights)

- 2026-10-03: started. Read stage12 docs, clearance table, fitcheck report.

- 23:30 Coordinator updates: D4 = 6.0 to back-cover INSIDE face, D1 = 5.5 from the front-shell rim to the board F side. Free height behind board = 6.0, design limit 5.7 (0.3 clearance), minus ribs (D13 1/4/4/1, solar box 5-5.5 grindable). Stubs stop above board plane.
- Findings: J4 5.5 -> fits after solar box ground flat (0.5 to the face). No PH-compatible RA SMD socket lower than 5.5 on JLC (checked C295747 5.5, C3029440 5.6, C2905019 5.6, C2845442 5.5, C52204942 5.5); no sink-mount PH on LCSC.
- Camera 5.4 -> fits (0.6) if rib B ground over it (x 143.5-156.5). Window hole ~7 mm (FOV calc). No need for lens into hole.
- J3: magnet body (Yiwei MG04254FRA1S1N) face 21 x 7 => 7 mm in Z, cannot sit above the board (5.7). Needs a board-edge notch so the body straddles the board, and a connection that doesn't need solder: SMD right-angle female header HX PM2.54-1x4P WT C46061768 (stock 8753, $0.18; body 2.5 tall, contact centre ~1.3 above board, 8.5 long, pads 1.02x3.0 @2.54). Legs straightened with pliers, pushed in horizontally. Requires moving the charger cluster (U2,C5,C6,Q2,R2,D7,D1...).
- Tools: scratchpad jlc.py (JLC search API), plot.py (PIL region plot with ribs).
- 00:20 Locating holes fixed (text edit of the .kicad_pcb, pcbnew SWIG flaky): H2 slot 4.2x5.0 @y176.25, H9 slot 4.4x4.8 @176.2, H13 slot 4.2x4.45 @198.325, H14 slot 4.2x4.65 @198.375, H6 moved to y186.99 (dia 4.2). Radial play vs BOTH the photo and the CAD (C15) post: H2 0.30, H9 0.46, H6 0.10, H13 0.45, H14 0.60 (copper >= 0.3). New lib fps ai_calc:Post_Slot_*. DRC 0/0/0 (zones not yet saved refilled).
- backcover json: gap design 6.0 / pessimistic 5.5, rib B split, after-grind = solar box + rib B camera part; _front_shell LR44 cup.
- J3 decision pending (options A WT header + cluster relayout / B THT via JLC or friend / C USB-C). J4 + camera need no board change.
- 01:10 stage13_heights.md written; measurements D1/D4/C14 updated. NEXT: QUESTIONS, outputs, summary, zip, graphify.
- 23:45 (real clock) fab/ regenerated (STEP 23:37), DESIGN_SUMMARY, zip ai_calc_pcb_13.zip, QUESTIONS + HANDOFF updated. NEXT: graphify update, handback.

## Stage 13b (option A chosen by Nirav)
- Backup: kicad/.mcp-backups/stage13b_pre/ (board + schematics before 13b).
- Yiwei drawing read properly (rendered via Windows.Data.Pdf): face 21x7 (1.0 plate), 0.7 plate, neck 3.5 tall, rear block 1.0 thick x 5.0 tall; total depth 4.0. Pins exit rear block on the body centre line, horizontal to 5.5 from the face front, bend down; tip 1.8 below the rear block (4.3 below centre). Straightened leg ~5.3 behind the rear block.
- Placement: magnet face against the top wall (outer y ~57.3), body y 57.3-61.3, entirely in front of the board edge (y ~62) -> NO notch needed. Body centre Z = socket centre = 1.3 above F side.
- New fp ai_calc:PinSocket_1x04_P2.54mm_SMD_RA_C46061768 (origin = pad row; body y -10.1..-1.6; pads 1.02x3.0 @2.54) + STEP box ai_calc.3dshapes/HX_PM2.54-1x4P_WT_box.step (scratch stepbox.py).
- J3 at (127.5, 72.0) rot 0 -> entry y 61.9, magnet x 117-138 (slot x 116.75-138.25). D7 -> (119.2,73.4) rot 180, U2 -> (135.9,67.5), C6 -> (139.6,66.0) rot 90. Schematic J3 footprint/LCSC/MPN/datasheet updated.
- Ripped 143 tracks (VBUS, USB_DM/DP, VBAT_P, CHG_STAT_RAW, CHG_PROG, some GND vias) in x114-148.5 y60-81. NEXT: rip SYS tracks crossing J3 pads, reroute with tools/pilroute.py connect_all.
- 04:15 13b routed with pilroute (VBUS,SYS,VBAT_P,USB_DP/DM,CHG_PROG,CHG_STAT_RAW); J3 silk/ref fixed; DRC 0/0/0 parity 0.
- 04:45 C33/C34 22uF 0805 (C45783) added at (121.7,84.75) r180 / (124.55,85.35) r270 on the +3V3 trunk 5 mm above U1 (schematic mcu sheet beside C32). GND vias via gnd_islands --pads. DRC 0/0/0, ERC 0.
- 05:00 fitcheck regenerated (magnet row now beside-board 4.8 tall; SOT-23 height cap so J4's model doesn't spill onto U2).
- 04:11 fab regenerated (STEP incl. J3 box), DESIGN_SUMMARY. NEXT: docs (stage13_heights, ORDER_CHECKLIST, QUESTIONS), zip, graphify.
- 04:12 docs updated (stage13_heights, ORDER_CHECKLIST, QUESTIONS, HANDOFF).
- 04:13 13b DONE: DRC 0/0/0, ERC 0, fab+STEP, fitcheck, summary, zip (133 files), graphify updated.
