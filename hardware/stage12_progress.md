# Stage 12 progress notes (working log)

Backups of the stage-11 board + schematics: scratchpad backup_stage11/ (C:\Users\r_kas\AppData\Local\Temp\claude\...\scratchpad\backup_stage11)
Also copied to hardware/kicad/backup_stage11/ (delete before zipping? no - keep out of zip).

## Log
- start 2026-10-03: read docs. photo_notes says C4 4.7 "wall" = 1.2 skin + ~2.5 wire channel + 1.0 inner rib, 3 horizontal pins/side.
- New photos 2026-10-03 19:22 in uploads/0abf7810.../: 2a97120d (front shell+mat straight), fd923128 (Casio board+LCD), e59eff71 (front w/ calipers 159.8), fb0c138c (Waveshare HAT V4).
- 2a97120d homography on 50 contacts: RMS 0.37 mm (scratchpad blobs.py/homog.py/fit2a.py; H2a.npy). Top row residual ~0.7.
  Post (global H) mm: H1 mat-hole 127.49,126.05 tip 127.21,125.94 | H3 mat 172.62,126.08 tip 173.01,125.99 | H8 128.83,137.40 | H10 171.2,137.0
  H4 171.2,146.5 | H2 130.55,175.9 | H9 168.98,176.04 | H6 130.41,187.73 | H13 130.39,198.27 | H14 169.0,198.66  (y ~+0.4 systematic)
- Decision draft: H8/H10 -> spacing 42.3 (C8) centred 150.07: H8 128.92, H10 171.22. H1 -> ~127.33 (C8 46.5 with H3 173.83~174.0). Lower holes stay (<0.5, mixed sign).
- Measurement sheet defs (artifact SzK4...): D1 = board TOP face to front floor (5.5). D4 = A (front rim->back of LCD shield) + B (back rim->inside face), NOT board.
  C4 = jaws pinch wall at its top edge (4.7 = skin+channel+inner rib). C14 = stub sticks out from wall / height above floor (5.45 / 5).
  C15 sheet asked for centre-from-inside-top; Nirav did outside->"outside of holes".
- STACK: D10 11.75 top = f(~1.5) + 5.5 + G + b(1.0) -> G_top ~3.75-4.25. D4 cross-check: LCD 3.75 on floor -> shield back 1.75 below board top -> 5.5-1.75 = 3.75. => free height above F side ~4.0 (+-0.3), NOT 5.5.
  Rings 4 high in keypad => G_kp ~3.8-4.0 (rings press board).
- Window from photos (keys-referenced): front e59eff71 centre y 93.6 raw -> ~92.75 parallax-corr; inside 2a97 -> 93.36 corr. Board 93.1 => keep. Window x centre ~150.0 => Q4 confirmed.
- C15 fit: centre convention + top ref T=56.11 gives residuals -1.0..+1.0 (RMS 0.6); H2->H6 interval 9.75 vs photos 11.7 (internal error). Not used to move holes.
- WALL FEATURES (2a97 photo, H2a, +-0.5..1 at walls): stub tips L 116.8/117.0/117.4 at y 86.8/94.3/105.0; R 184.0 at y 86.6/94.1/105.0 (C14 5.45 -> R 184.2 agrees).
  wire hooks: L (118.2, 84.0), L (118.3, 112.2-114.2); R (182.5, 84.5), R (184.75,114.3); clips (120.8,77.5) L top, (183.8,77.45) R.
  inner thin rib L ~114.3-114.8, R ~186.1-186.6; rim skin out L 111.55 R 188.98. U1 body x 114.75-135.25 (antenna 114.75-119.85, pads from 120.15).
- PLAN: narrow right edge to ~181.8 (y 78.3-116), left edge to ~118.9 (U1 antenna overhangs, Espressif option 1), maybe shift U1 +1.0.
- E-PAPER datasheet (Waveshare 2.13 V4 spec p6, scratchpad v4spec.pdf/mech.png): outline 59.2+-0.2 x 29.2+-0.2 x 1.0; FPC 14.30+-0.3 from glass edge, tail 12.5 wide, edge at 8.15 from top.
  Vertical: PS 0.5 / EPL+1.45 / VA+0.40 / AA+0.40 -> AA vertical margins 2.75 both sides (centred). Horizontal: AA margin 2.70 at NON-FPC end, ~7.95 at FPC end
  => AA is 2.6 mm off-centre toward the non-FPC end. On our board AA = 123.17..171.72 (59.0 panel 120.47..179.47) -> 2.55 left of window centre: COSMETIC, image fully visible (margins 3.5/8.6).
- H1 plan: move to (127.5,126.1). Needs KEYPAD_INT & I2C_SCL diagonals moved (c=x+y: SDA 246.9, SCL->247.7, INT->248.5), INT vertical x124.8->124.1, GND B.Cu vertical x124.8->124.1, remove GND vias (124.14,123.98),(123.38,124.55).
- Edge plan: left x=118.9 for y 82.9..115.6 (remove GND vias (116,82),(116,112),(116,118)?); right x=184.2 y 78.27..116. Right pins/hook must be snipped (J2 fan-out copper to 183.65).
- 20:10 APPLIED (DRC 0/0/0, copied to repo): left notch x118.9 y82.9..114.0 then slant to (114.212,119.588); right edge x184.2 y78.277..116 then to (185.858,119.17);
  removed GND vias (116,82),(116,112),(116,118),(124.14,123.98),(123.38,124.55); H1->(127.5,126.1) with INT/SCL/GND(B) reroute, SCL via->(123.4,125.95);
  H8->(128.85,137.0), H10->(171.25,136.8) (max shift keeping 0.28 hole clr; spacing 42.40); edge keep-out ring rebuilt; antenna strip zone deleted, top/bottom zones trimmed to x>=118.4;
  U1 silk trimmed (instance + ai_calc.pretty lib); TP1 ref moved; KEEPOUTS panel 59.0 + AA per datasheet; silk "AI CALC v12 2026-10-03" at (155.5,136).
  Scripts in scratchpad (edit1-3.py, lib12.py). TODO: gnd_islands, heights, parts/BOM, silk/fiducials, outputs, docs.
- 20:40 edit4: J2 down-group fan-out pitch 0.5->0.4 (PREVGH 181.95, VSH 182.35, VPP 182.75, VDD 183.15), right edge -> x183.65. Slots for H9/H6 tried & dropped (edge clr to key pads). DRC 0/0/0, in repo. CAD fitcheck_report read: its stack gives 4.59 above F side (top).
- 21:20 3D models: U1 -> ai_calc.3dshapes/ESP32-S3-MINI-1_C3013941.step (EasyEDA, offset 0,2.55,0 verified body 114.75-135.25), U6 -> HVQFN-24-1EP_4x4mm_P0.5mm.step (KiCad lib copy). DRC 0/0/0, in repo.
- HEIGHTS (clear.py): G=4.59 (CAD) fails: J4 5.5 (-0.91), J3 5.0 (-0.41), D2/C16 under big ring wall (ring 4 tall) -> grind ring arc. Everything else >=0.6 margin even under 1-mm ribs.
  No PH2.0 SMD socket lower than 5.45 on JLC. MX1.25 C7430468 (3.6 tall, EasyEDA fp converted: scratchpad ee/mx.pretty) = alternative, needs a 1.25-plug battery -> DECISION for Nirav, J4 NOT changed.
  J3: magnet legs into 5 mm header puts magnet body ~4+ mm above board -> stack can't close; needs P5 in hand. BLOCKING.
- JLC API (jlc.py): RT9080 C841192 extended $0.12 stock 39.6k; AP2112K C51118 extended $0.171 -> keep RT9080. 14 extended types, no same-footprint basic subs. All in stock (min L1 5250, ESP32 1893).
- 22:00 fiducials FID1-3 (board-only, excl BOM/pos, pad clr 0.75) at (121.5,203.5),(178.5,203.5),(177,112.5); TP function labels + J4 +/- silk; gnd_islands: 0 added. DRC 0/0/0 in repo. NEXT: make_fitcheck rework, outputs, docs.
- 22:30 make_fitcheck.py rewritten (3D-model heights, clearance_table.md, grind = solar box flat + big ring arc to 2 mm; after-grind J4 -1.0, J3 -0.5 remain). NEXT: outputs, docs, zip.
- 23:10 outputs regenerated (fab, BOM, schematic.pdf, summary), DRC/ERC 0. stage12_measurements.md written. NEXT: QUESTIONS, HANDOFF, ORDER_CHECKLIST, graphify, zip.
- 23:30 DONE: docs, QUESTIONS stage 12, HANDOFF, ORDER_CHECKLIST, graphify update, zip Claude outputs/ai_calc_pcb_12.zip (123 files).
