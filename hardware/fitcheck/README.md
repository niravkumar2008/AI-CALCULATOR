# Fit check: paper grind maps, dummy board, clearance table

> **Rev F (2026-10-06, `../verification/07_final_review_fitment.md`):** everything in this folder was regenerated. The maps now carry the Shell Grinding Guide's zone numbers (1 solar box, 2 rib B, 3 camera window, 4 LR44 cup = **keep**, 5 magnet notch, 6 antenna relief = only if needed), the **yellow ESP32 antenna tab** for the paper dry fit, and the battery at its rev E place (Adafruit #1317 on the back-cover floor). The old "LR44 cup: TRIM FLAT" line is gone; the cup is drawn green. Throw away older prints.

Regenerate everything after any board change: `"C:\Program Files\KiCad\10.0\bin\python.exe" tools/make_fitcheck.py` (KiCad's python, from `hardware/`). Inputs: `../kicad/ai_calc.kicad_pcb` (read only) and `backcover_fx115es.json` (ribs, rings, solar box, front-shell features, zone names, antenna tab).

## 1. Paper dry fit and grind maps (1:1)
- Print `grind_map_front_shell.svg` and `grind_map_back_cover.svg` from a browser at **100 % / actual size**. Measure the 50 mm line with the calipers; it must read 50 mm.
- **Front-shell map** = the paper dry fit (`../FINAL_STATUS.md` section 1). Cut along the board outline **together with the yellow antenna tab** (4.5 × 15.4 mm on the map's left edge = the ESP32 module's bare antenna PCB, 0.8 mm thick, 4.15 mm past the board edge). Punch out the post holes. Lay it in the front shell, printed side up, pressed onto the posts: every post inside its hole (H6 has the least play, 0.10 mm), and the tab lying flat beside the side wall. If the tab fouls the thin inner rib of that wall, that is grinding zone 6.
- **Orientation:** the map is drawn in KiCad coordinates (component side up). Lying in the front shell seen from the inside, the map's left edge (antenna, J3, solar-box parts) is the calculator's **right-hand side when you use it** (the solar-window side). "Left edge" in the board docs always means the map's / KiCad's left.
- **Back-cover map** is mirrored on purpose. Red dashed = ribs and rings; red filled = zones 1 and 2 to grind; the 7 mm circle with cross-hairs = zone 3 drill centre.
- Colour of the parts = height above the board (component side): red 4+ mm, orange 2–4, blue 1–2. The camera, the battery (rev E spot, under the board edge) and the magnet body are drawn dashed because they are not soldered to the board.

## 1b. Clearance table
- `clearance_table.md`: every component-side part, its height from its 3D model, the back-cover rib/ring/solar box above it, and the margin for 6.0 mm (design, D4) / 5.5 mm (pessimistic) of free height, plus after the grind plan. Bold = needs a grind. The LiPo row is informational (its real limit is the LR44 cup, 0.3 mm, see `../enclosure/final_assembly/assembly_report.md` section 9).
- The complete CAD check (every part vs every shell feature, closed case) is `../enclosure/final_assembly/clearance_table_full.md`.

## 2. Dummy board (3D print)
- `dummy_board.stl`: the full board at real size (outline, holes, 0.8 mm thick) with **every** part at its real height from the KiCad 3D models, including the ESP32 module with its antenna overhang (x 114.75), plus an 8.5 × 8.5 × 5.4 mm camera block on the board and a **loose 26.02 × 19.75 × 3.8 mm block for the #1317 battery** at its rev E place (KiCad x 152.59–178.61, y 60.36–80.11; it lies on the back-cover floor, not on the board). Tallest point: 5.5 mm above the board (battery socket J4).
- Print it flat, parts up, 0.2 mm layers, no supports.
- Marker test: colour the rib tops (and the ground zones), press the dummy onto the posts, put the battery block on the back-cover floor, close the back cover gently, open it. Marks on the dummy = plastic that still touches.

## Don't grind
Screw bosses (6), the 12 pegs, outer walls, window frame, key holes, the LR44 cup. Grind at low speed, a little at a time, keep the floor at least 0.8 mm thick, eye protection + mask. Zones 5b and 6 only if your shell needs them (pre-grind checklist in `../verification/07_final_review_fitment.md`).
