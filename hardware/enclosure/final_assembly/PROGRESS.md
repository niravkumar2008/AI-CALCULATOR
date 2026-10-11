# Final assembly progress (resume here)
- Script: hardware/enclosure/build_final_assembly.py (loads build_fx115es_replica.py as a module for the shell)
- [x] 23:45 read docs; plan: stages new shell grind pcb parts looks check export renders
- [x] 04:05 script written (resumed after usage limit; hard stop 08:30)
- [x] 04:25 stages new shell grind pcb parts OK (old STEP 23:37); first renders look right; running check
- [x] 04:15 check run 2: real = LiPo vs solar frame/cell, LiPo leads vs board edge (0.4 gap); magnet pins vs OLD J3 (wait 13b STEP). Fixed: J4 plug side (opening faces -Y, checked in section), ribbon ends, lead overlaps
- [x] 04:25 NEW STEP 04:11 (stage 13b) landed; magnet params updated to 13b (x 127.5, U-notch 21.5x7.95, legs 5.3); full rebuild
- [x] 04:28 rebuilt on 13b STEP, check after grind = 12 rows (3 real: LiPo vs solar frame 2.3, LiPo vs solar cell 0.4, LiPo leads vs board edge); before-grind check done; clearances.json. NEXT renders, export, report
- [x] 04:45 renders reviewed: fixed explode (no snapshot: it snaps the grounded front shell back), sequential grind before/after, cutter hiding; README draft written
- [x] 04:55 report + README written. NEXT graphify update, PROGRESS final
- [x] 05:00 DONE. Built on STEP 2026-10-04 04:11 (13b). f3d/step/28 renders, assembly_report.md, README.md, graphify update.
  Real collisions: LiPo (stage13 position) vs solar-window frame 2.3 / solar cell 0.4 / leads vs board edge 0.7 -> fix: LiPo on back floor (lipo_alt dz=-5.6: 0 hits).
  Open: Fusion cloud save failed (doc "Untitled"); back-cover lip at magnet notch (replica guess) also cut.
  Re-run after a board change: python build_final_assembly.py pcb parts check export renders
- [ ] 17:30 stage 14 re-check: LiPo on back floor, rerun pcb parts check export + hero/back_cover_off renders
- [x] 17:45 stage 14 re-check done: 0 real collisions with LiPo on back floor; report section 6; hero/back_cover_off/section renders refreshed

## Visual guide rework (2026-10-05)
- [x] read guide; plan: A) 'steps' render stage in build_final_assembly.py -> renders/steps/*.png; B) Pillow annotate -> *_ann.png + renders/photos/*.jpg; C) rewrite assembly_guide.html
- [x] A done: stage `steps` in build_final_assembly.py (STEP_SHOTS list; `python build_final_assembly.py steps [only=s05,s07]`) -> renders/steps/s01..s11 *.png (16 shots) + anchors.json (viewport px of labelled points + "_c" target; image px = W/2 + (q - c) * H / (2*c.y)). Grid hidden during the shots. All checked visually.
- [x] B done: final_assembly/annotate_guide.py (Pillow) -> renders/steps/*_ann.png (16) + renders/photos/{teardown,shell_keymat,epaper_hat,front}.jpg and *_ann.jpg. Checked on contact sheets. NEXT: C rewrite assembly_guide.html
- [x] C done: assembly_guide.html rewritten (glossary, time + "You'll need" + "Done when" per step, 25 pictures with "Look for" captions, tap to enlarge, one action per checkbox = 113 boxes, same localStorage keys). Image weight 4.3 MB. Not published.

## 3-part shell + slide case + window mask (2026-10-05)
- [x] photos re-read (88e03c27, e82c982d, 2f54d6c7, 84e63d95, 478afcb6, 66bb7a5f, faf9599f, 4e287737, e59eff71, 3d608fdb); issues seen: front shell rendered cream not silver; navy tray wall starts too low (58 mm from bottom; photos: rises from ~small-ring level ~50-55 mm from top, ramping); no side groove; slide cover placeholder.
- priorities (Nirav): 1 shell right + assembly/interference without case; 2 case body (informational); 3 window mask
- [x] 14:40 replica rev C2 built (ramp split line, faceplate skirt Z 8.3 below Y 8, groove Z 5.9-6.9 x0.35, custom silver/navy, slide case stored). NEXT: inspect shots, then final assembly all + mask + case.
- [x] 14:55 final assembly rebuilt: new shell grind pcb parts mask case OK (mask opening 47.75x22.90 @ (2.555,45.954), lens +0.095, mask->panel 1.79; case hole d8 at 38.26 from open end, centred). NEXT check, case_check, export, renders.
- [x] 15:00 check: 243 bodies, 532 pairs, 8 overlaps all intended (FPC in J2 x2, magnet pins in J3 x4, LiPo leads in JST plug x2); case_check 0 hits both poses; clearances.json refreshed. NEXT export, renders, case_renders, steps.
- [x] 15:05 export f3d/step, renders, case_renders (case_*, mask_*, compare/*_cad + cmp_*.jpg) done and reviewed. NEXT replica check/export/shots, steps+annotate, docs (mask svg/dxf/md, slide_case_mods, README, report, questions, photo_notes), graphify.
- [x] 15:20 steps + annotate regenerated (mask added to SHELL list; grind renders redone with mask hidden); window_mask_template.py -> .svg/.dxf. NEXT docs: window_mask.md, slide_case_mods.md, README, assembly_report, QUESTIONS, photo_notes, replica README, graphify.
- [x] 15:35 docs: window_mask.md, slide_case_mods.md, README (parts table), assembly_report section 7, QUESTIONS (CAD 3-part + case), photo_notes rev C, replica README/PROGRESS. NEXT graphify update. DONE after that.

## Faceplate refinement (2026-10-05 evening)
- [x] read docs + photos e82c982d / 2a97120d / 2f54d6c7 (crops). Plan: replica: LR44 holder (cup with side gaps + I-bar box + contact rib, D7 5.5 kept), comb snap tabs top (X +5, 9 wide) / bottom (X +3.5, 7 wide) + back-cover lip notch (top) and catch block (bottom), hooks A (Y 63.5) / B (58.1) + hook shape on the C4 holder (Y 26), collars (0.0-0.75, 0.8 tall), row-rib beads + rib under the bottom row, crush ribs on locating posts. Final: drop GRIND lr44_cup. Then replica rebuild, final all-but-renders, check, clearances, export, compare, steps.
- [x] 17:15 replica rev D built + checked (0 overlaps; fixed: collar rings had never been built - centroid filter; REPLAY collar kept 0.35 tall over its domes) + exported (f3d/step/stl) + shots. Final script: GRIND lr44_cup removed (GRINDS list), CLEAR_PAIRS + LiPo/ribbons/PCB vs front shell. NEXT: final new shell grind pcb parts mask case check case_check export, clearances, compare renders.
- [x] 17:20 final rebuilt (rev D shell, 4 grinds), check 243 bodies 532 pairs 8 overlaps all intended, case_check, export f3d/step. NEXT clearances, compare renders
- [ ] 21:35 Fusion had been restarted (no docs): rebuilding final (new..case) for clearances + renders
- [x] 21:36 rebuilt in Fusion; clearances.json: J4 0.41, cam 0.71, H6 0.10, magnet notch 0.25 / back 0.71 / board 0.71, LiPo-front shell (LR44 holder) 0.30, LiPo floor 0.0 / board 2.24. NEXT compare renders, steps s01,s08,s09,s10
- [x] 21:40 compare: case_renders only=compare + cmp_faceplate_inside_e82c982d / cmp_front_e59eff71 / cmp_front_4e287737 (right panel replaced, scratch cmp.py). NEXT steps s01,s08,s09,s10 + annotate
- [x] 21:45 steps s01,s08a-c,s09,s10a-b re-rendered + annotated; assembly_report section 8; photo_notes rev D. DONE (graphify update run).

## Battery upgrade study + jigs (2026-10-06, night)
- [x] 23:50 Fusion was closed: relaunched it, rebuilt `new shell grind pcb parts mask case` (rev D, nothing exported)
- [x] offline envelope tool `battery_envelope.py` (0.1 mm raster, 0.3 margins, lead channel to J4 reserved) -> battery_envelope.json, renders/battery_upgrade/envelope_*.png; candidate list battery_cells.json
  rev D (holder stays) 33.5 x 20.7 x 3.8 @ front X -30.9..2.6 Y 58.2..78.9 Z 1.0-4.8; holder TRIMMED 1.9 mm (to Z 7.0) 33.5 x 20.7 x 5.7 (board limits);
  full removal adds only 32.4 x 17.6 x 6.8 / 23.4 x 18.2 x 9.1 (no real cell); custom shell keypad pocket 70 x 41.2 x 5.4
- [x] new stages in build_final_assembly.py (NOT in "all"): `lr44 mode=trim|remove|off` (feature "GRIND lr44_holder (...)"), `cell name= L= W= T= X= Y=` (candidate + boolean/min-gap check -> battery_fit_fusion.json, `name=off` restores the #1570)
- [x] Fusion checks: #1317 in rev D 0 overlaps (0.30 to holder); EEMB 502030 / worst 33x20.5x5.3 / 552033 with trim: 0 overlaps (>=0.48 faceplate, >=0.5 board)
- [x] renders/battery_upgrade: lr44_holder_before/after(+_iso), cell_before/after_top/iso, cell_after_section(_x)
- [x] jigs: ../jigs/build_jigs.py (A plate + A2 drill bush + A3 gauge, B magnet saddle, C LR44 sled), 0 overlaps in place (jigs_check.json), STLs + renders, ../jigs/README.md
- [x] battery_upgrade.md written (envelopes, cells, grind times, recommendation, charge / brown-out, trim zone spec, parts)
- [x] Fusion model reset to rev D (cell off, lr44 off, jigs removed). Rev D files untouched. Guides' HTML not edited.
- [x] 00:30 REV E: LIPO = #1317 26.02 x 19.75 x 3.8 at KiCad (165.6, 70.24); parts check clearances export; renders back_cover_off/exploded/section; steps s04,s09 + annotate (labels #1317 150 mAh); battery_upgrade.md section 8 (placement for the guide), assembly_report section 9. Check 8 overlaps / 0 unintended; battery top 0.30 under the holder, sides 2.6 / 1.0 / 0.83 / 7.0.
- [ ] 2026-10-06 resume (rev E verify): Fusion model is in rev E (cell -28.61..-2.59, 58.83..78.58, Z1.0-4.8); exports 00:29 + renders checked OK. Found: cell spans the battery-lid opening in the floor (floor-top opening ~X -24..-11.5, Y 64..77; lid top Z 0.58 = 0.42 under the cell). Decision now 0.1 mm double-sided tape (not Kapton over the top): tape on solid floor only -> rewrite sec 8 tape, labels '150 mAh #1317', re-annotate s04/s09, report sec 9 note.
- [x] resume done: labels '150 mAh #1317' + re-annotated s04_parts_ann / s09_battery_ann (checked); battery_upgrade.md sec 8 renamed 'Placement for the guide', tape = 0.1 mm double-sided L (6x18 + 4x16) off the battery lid, lid-opening landmark, gap ~0.20 with tape; parts list; report sec 9 tape/lid notes. Geometry unchanged so f3d/step (00:29) not re-exported.

## Rev F: fitment review (2026-10-06, verification/07)
- [x] replica `INNER_RIB_FULL` (inner ribs rim -> plate), final: grind zone 6 `antenna_relief` (ANT_RELIEF_K 71.5-106.5), stage `fulltable`, antenna renders, U1 clearance pairs; `regrind6` stage to re-cut zone 6 in place
- [x] rebuilt `all` + `check grind=off`: after grinds 18 overlaps / 0 unintended (8 intended + 10 navy-tick vs rib artefacts); before grinds 54 (U1 tab 0.81, board corner 0.71 into the rib); fulltable 621 pairs, 1 FAIL (H6 0.10, known)
- [x] fitcheck regenerated (make_fitcheck.py patched: #1317 block, antenna tab, zone labels, keep list, drill mark); guides + FINAL_STATUS/HANDOFF/ORDER docs updated; assembly_report section 10
- Fusion doc still "Untitled" (cloud save fails: saveAs first); f3d/step exported 10:35
