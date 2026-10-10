# Replica progress (for resuming after a cut-off)
- Script: hardware/enclosure/build_fx115es_replica.py (offline `python build_fx115es_replica.py` = 2D geometry + checks; `... all` = all Fusion stages)

## Rev B (2026-10-03, caliper batch 2: hardware/measurements_2026-10-03.md)
- [x] Read measurements, docs, 4 new photos (0abf7810/: 2a97120d shell inside, e59eff71 front w/ calipers, fd923128 board+LCD, fb0c138c e-paper HAT = ignore)
- [x] fx115es_outline.json re-chained into one loop (450 pts, no self-intersections, build_case.fx115_outline parses: 79.24 x 161.0)
- Stack-up decision: D1 (5.5) and D4 (5.5) are both to the OUTSIDE faces: 5.5 + 0.8 + 5.5 = 11.8 = D10 top part.
  Board back Z=5.5, front 6.3, back floor 1.0 (D5) so the inner gap behind the board is 4.5, rings 1+4=5.0 (0.5 under board),
  solar grid 1+5=6.0 (hits our PCB: matches "will have to grind"). Face 11.8 screen section, 11.3 keypad (D10).
- C15 convention: reading = case top -> far edge of the post's HOLE (pilot r 0.85 / locating-post bore r 0.6): residuals <=1.05.
- C11 showed the trace's top corners too soft: Y_STRAIGHT up to 77 (~5 mm corner), WALL_KEY 1.4, WALL_TOP 1.2 -> C9/C11/C15 all agree.
- [x] script params + geometry + stages edited (rev B); Fusion new..check OK: 60 bodies, 0 interferences (19:40)
- [x] fitcheck + fitcheck_shots stages (19:51): 29 overlaps, report in fitcheck_report.md (STEP 18:10:31)
- [x] export (f3d/step/stl 19:49) + all renders regenerated (export drops the Fit-check comps; doc now has the PCB loaded)
- [x] README.md, photo_notes.md, QUESTIONS_AND_ISSUES.md (CAD replica Q1/Q7/Q9 answered, Q10-17 new), graphify update. DONE (rev B)
- To re-run after the PCB agent's new STEP: `python build_fx115es_replica.py fitcheck fitcheck_shots` (replica doc open in Fusion).

## Rev B2 (23:25): Nirav's answers D4 = 6.0 to the INSIDE of the floor, D1 = 5.5 from the front shell RIM, stubs stop above the board
- Board F.Cu face Z 7.0, key side 7.8; parting line Z 1.5; plate 1.2; stubs/inner rib/holders down to Z 8.0. Sum 1+6+0.8+4.0 = 11.8 / 3.5 -> 11.3.
- Replica rebuilt + exported + all renders (0 interferences). Stage-11 baseline from git HEAD files -> replica/fitcheck_report_stage11.md.
- Stage-12 STEP (23:02) fit check: 21 overlaps, report has before/after + comparison with fitcheck/clearance_table.md; section_camera_*, section_j4_* renders (stage fitcheck_cuts).
- When the stage-13 STEP lands: `python build_fx115es_replica.py fitcheck fitcheck_shots fitcheck_cuts` (compares against the stage-11 baseline; pass prev=<path> to compare with another report).

## Rev C2 (2026-10-05): 2-part split from the photos + slide case
- [x] split-line ramp (SIDE_RAMP_Y 31->8), silver step-in Y 18->-2, faceplate skirt Z_SKIRT 8.3 below Y 8, rail groove (guess), custom silver/navy, slide case (stored) replaces the cover. new..parts, check (0 overlaps, 162 pairs), export f3d/step/stl, shots (side, iso_*, front, back, top_end, inside_*, with_case_stored_*), explode_shots. fitcheck_* renders/report NOT re-run (the final assembly supersedes them).
