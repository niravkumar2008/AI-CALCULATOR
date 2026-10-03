# Fit check: dummy board + grind map

Regenerate everything after any board change: `python tools/make_fitcheck.py` (KiCad's python).

## 1. Grind map (paper, 1:1)
- Print `grind_map_front_shell.svg` and `grind_map_back_cover.svg` from a browser at **100 % / actual size**. Measure the 50 mm line; it must be 50 mm.
- Cut along the outline. Lay the front-shell copy in the front shell, the mirrored copy in the back cover.
- Colour = how tall the part is above the board (back-cover side). Any rib over a coloured zone that's taller than the space left (depth readings D2-D4, D11) gets ground down.
- Battery, camera and magnet are drawn too, even though they aren't soldered to the board.

## 2. Dummy board (3D print)
- `dummy_board.stl`: the full board at real size: outline, holes, 0.8 mm thick, with **every** part on it at its real height, plus a 12 x 12 x 5.4 mm camera block and the 31 x 11.5 x 3.8 mm battery (a separate piece that sits in the battery bay). Tallest point: 6.3 mm (battery socket J4).
- Print it flat, parts up, 0.2 mm layers, no supports needed.
- Marker test: colour the rib tops, press the dummy in, close the back cover gently, open it. Marks on the dummy = ribs that touch.

## Don't grind
Screw bosses (6), the 12 pegs, outer walls, window frame, key holes. Grind at low speed, a little at a time, keep plastic at least 0.8 mm thick, eye protection + mask.
