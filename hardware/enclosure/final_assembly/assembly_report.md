# Final assembly check: does everything fit?

Model: `ai_calculator_final_assembly.f3d` / `.step` (Fusion document "AI Calculator - Final Assembly 1:1").
Board: **`hardware/fab/ai_calc_board.step` of 2026-10-04 04:11 (stage 13b)**: right-angle magnet header J3 (C46061768), no board notch.
Check run: 2026-10-04 04:36, 242 solid bodies, 541 pairs whose boxes touch, each tested by a real 3D overlap (boolean intersection).

**Short answer:** with the 5 grinds of the plan, the board, camera, e-paper, magnet connector and J4 all fit.
**One thing doesn't: the battery (LiPo) where stage 13 puts it.** It hits the rib frame round the solar window and the
solar cell, and its leads have no room to get round the board edge. Putting the battery flat on the back-cover floor instead
fixes all three (checked in the model). Details and the fix are in section 1.

How to read the numbers: **depth** = how far two parts would have to move apart to stop touching. Positions are given
two ways: "front view" X/Y (mm from the board centre, display up) and KiCad x/y (the board file). Z = height from the
outside of the back cover (the board's component side is at Z 7.0).

## 1. Real collisions (need a decision)

| # | What hits what | Where | Depth | Plain English |
| --- | --- | --- | --- | --- |
| 1 | **LiPo battery** ↔ front shell: the rib frame behind the solar window | KiCad x 147.9–157.2, y 64–74 (the battery's end nearest the board); Z 8.1–10.4 | **2.3 mm** | The battery sits against the inside of the front plate (stage 13 section 7: z_rel −3.4 to +0.4). Its right-hand 9 mm (front view) runs under the solar window, where a 2.5 mm deep rib frame hangs down from the plate. |
| 2 | **LiPo battery** ↔ solar cell (the original Casio cell, still glued in its window) | same spot, Z 10.0–10.4 | **0.4 mm** | Same place: the cell itself is in the way too. |
| 3 | **LiPo leads** ↔ board edge | board edge at KiCad x 147.5 next to the battery end | **0.7 mm** | The battery's end is only **0.4 mm** from the board's cut-out edge. Its two wires (about 1.1 mm thick) come out of that end and have to bend round the board to reach J4 on the back side. There is no room for that. |

**Recommended fix (no board change): lay the battery flat on the back-cover floor**, under the board's level, instead of
against the front plate (z_rel +2.2 to +6.0, i.e. Z 1.0–4.8). I moved it there in the model and re-checked
(`stage lipo_alt`): **0 collisions**, even with the LR44 cup *not* ground (the cup's bottom is still 0.3 mm above the
battery). It also clears the frame and the solar cell by more than 3 mm, and the wires leave the battery at the same
level as J4's plug, under the board, so they no longer have to bend round the board edge. It needs:
- the solar-box grind (already on the list: the battery's right end sits where the box was),
- a 1–2 mm foam pad between the battery and the board/front shell to stop it rattling (instead of between battery and back cover),
- note that the battery then lies over the old LR44 hole in the back floor; the battery lid still closes it.

Other options if you'd rather keep it at the front: grind the left 9 mm of the solar-window frame (front view, the end
nearest the board) flush with the plate **and** take the solar cell out (cover the window with a sticker), **and** move the
battery 1.5 mm towards the side wall so its wires fit (there is 1.7 mm of room to the corner screw post).

To change the model: `LIPO_ZREL` (and `LIPO_K` for a sideways move) at the top of `build_final_assembly.py`, then
`python build_final_assembly.py parts check`.

## 2. Contacts that are meant to be there (not problems)

| What | Why it shows up | Depth |
| --- | --- | --- |
| Magnet connector legs ↔ J3 | the four straightened legs are pushed 4.7 mm into the right-angle header (its 3D model is a solid box, so the check sees an overlap) | 0.6 (the leg's thickness) |
| E-paper ribbon ↔ J2, camera ribbon ↔ J1 | ribbon tips 2 mm inside the connectors; J2's model has a slightly narrower mouth than a 12.5 mm ribbon | 0.12 |
| Battery wires ↔ JST plug | the wires come out of the plug | 0.07–0.15 |

## 3. Tight spots (they fit, but check them on the real parts)

| Where | Gap | What it means |
| --- | --- | --- |
| J4 (battery socket) ↔ back-cover floor | **0.41 mm** | After the solar box is ground flat. Leave **no stub**: a 0.5 mm stub left over would hit J4. The model's board is 0.09 mm thinner than the real one, which doesn't change this side. |
| Camera lens ↔ edge of the 7 mm window | 0.71 | Lens top is 0.5 mm below the inside of the floor. Fine, but use thin tape (≤ 0.1 mm) under the module, not foam. |
| Camera ribbon ↔ the mid rib (4 mm tall) at KiCad y ≈ 118.7 | 0.48 | The ribbon lies on the tallest part in its path (1.46 mm below the board). It passes over the mid rib with half a millimetre to spare; don't let the spare loop sag there. |
| Magnet connector ↔ U-notch in the top wall | 0.25 | The notch is 21.5 wide for a 21.0 face: glue fills it. |
| Magnet connector ↔ board edge | 0.71 | The body ends just in front of the board edge, as stage 13b planned. |
| Magnet connector ↔ back cover | 0.71 | Below the body (the lip at the notch is cut, see section 4). |
| Board ↔ locating post H6 | 0.10 | Same as stage 13 found: if it binds, file the hole a little towards the top of the board. |
| E-paper ↔ solar-window frame | 0.29 | The panel's top edge (KiCad y 78.4) is just below the frame. Fine. |
| E-paper ribbon at the board slot | — | J2's mouth overhangs the slot (KiCad x 172.73 vs slot 172.4–173.4). The ribbon has to hug the slot's outer wall (x ≈ 172.5) and turn 90° straight into J2. It works in the model with a 0.25 mm bend radius; be gentle. Ribbon path from the panel into J2 is 12.7 mm (tip 2 mm inside J2), the panel's is 14.3 ± 0.3: about 1.6 mm spare nominally, 0.3 if the ribbon is short and J2 wants 3 mm of insertion. Review 07: build as if there were no spare (the stage-14 "14.3 = 14.3" budget). |
| Camera ribbon length | — | path module → J1 is 58.7 mm (from the module's near edge, tip 2 mm inside J1). Seeed's 70.5 mm is measured from the module's **far** edge (stage 14, verification 03 L1), so only about 62 mm is ribbon: **1.4–3.3 mm spare, not 12**. Lay it straight. (Corrected in review 07.) |

## 4. What each grind fixes (same check with the shell *before* grinding)

| Grind (stage 13 list) | Without it, these hit (worst depth) |
| --- | --- |
| 1. Solar box flat to the floor | **J4 4.6 mm**, **J3 2.1**, U3 1.1, D1 0.85, Q2 0.8, J4's plug 0.2, D7 0.1 |
| 2. Rib B flat over the camera (14 mm) | camera lens 0.5 |
| 3. Camera window, Ø7 drill | nothing hits; it's the camera's view |
| 4. LR44 cup in the front shell | LiPo 3.8 (only if the battery stays at the front: not needed if you lay it on the back floor, section 1) |
| 5. Magnet U-notch in the top wall | magnet face 1.0 into the wall |
| 5b. **New: the back cover's lip at the magnet notch** | magnet body 0.5 into the lip. The replica has a 1.5 mm locating lip round the back cover's rim (a guess, not measured); stage 13b assumed it clears by 0.2. **Check your back cover:** if it has a lip under the magnet, notch it 21.5 mm wide too. |

The big ring was **not** needed (D2/C16 clear it), as stage 13 said.
Floor everywhere stays 1.0 mm (limit 0.8).

## 5. What the model is sure about, and what it isn't

- Sure: board, part positions and heights (from the STEP), shell sizes from your calipers, magnet face/pins (Yiwei drawing),
  e-paper and battery outline sizes (vendor data).
- Estimated: the camera's internal split (lens barrel Ø6), the magnet housing width behind the face, the e-paper's
  layer split, the cable plug's shape, screws (2 × 8 assumed), the back cover's lip, the replica's "guess" rows
  (`../replica/README.md`).
- Raw data: `interference.json` (after grinding), `interference_before_grind.json`, `clearances.json`, `placement.json`.

## 6. Stage 14 re-check (2026-10-04 17:40)

Board STEP of **2026-10-04 14:50** (stage 14: J3 rewired, same place; SW1 notched pad; new silk; no part moved).
Battery now **on the back-cover floor** (Z 1.0–4.8, the fix from section 1); everything else as above.

| | Stage 13b, battery at the front | Stage 14, battery on the back floor |
| --- | --- | --- |
| LiPo ↔ solar-window frame | **2.3 mm** | none (frame is 3.3 mm above it) |
| LiPo ↔ solar cell | **0.4 mm** | none |
| LiPo leads ↔ board edge | **0.7 mm** | none (leads stay under the board, 2.2 mm below it) |
| Real collisions in total | 3 | **0** |
| Intended contacts (legs in J3, ribbons in J1/J2, wires in the plug) | 9 | 8 |

Battery gaps now: sits on the floor (0.0, intended, put a thin foam pad under it if it rattles), 2.2 mm to the board's
level, plug ↔ floor 0.8. J4 ↔ floor still 0.41 after the solar-box grind. With the battery here the **LR44-cup grind
(item 3) isn't needed** any more; the model still shows it ground, and the check found nothing within 0.3 mm of the cup.
Raw data: `interference.json` (stage 14), `interference_stage13b.json` (before). Renders refreshed: `hero_*`,
`front/back`, `back_cover_off*`, `section_*`.


## 7. 2-part body re-check (2026-10-05)

**What changed in the shell (rev C2, from Nirav's photos and his "faceplate + back cover" note):**
- **Silver faceplate** (component "Front shell"): keeps its deep walls only in the screen section (stubs, clips,
  wire channel, top wall, LR44 cup, solar frame). Below the SHIFT/ALPHA row and round the bottom end it is a
  faceplate with a short skirt (down to Z 8.3) that drops into the navy back cover. Colour now a neutral silver.
- **Navy back cover** (component "Back cover"): its long walls rise along a ramp from the small ring (Y 31) to full
  height (Z 9.8) at the big ring (Y 8) and stay full height round the keypad section and the bottom end (photos
  2f54d6c7 / 84e63d95 / 050df9b7 / 7c6e8116). It is the side wall there (C5 0.8).
- Shallow rail groove along both long sides (Z 5.9-6.9, 0.35 deep) for the slide case: **a guess**, not in any photo.
- New: **window mask** (black sticker inside the lens, `window_mask.md`); the **slide case** as its own navy part
  (two components, both hidden in the main configuration; `slide_case_mods.md`).
- Unchanged: every measured post, rib, ring, window and key opening, the board placement (D1 / D4), the 5 grind zones,
  the stage-14 battery position (on the back-cover floor).

**Check:** 2026-10-05 14:52:58, board STEP `hardware/fab/ai_calc_board.step` of 2026-10-04 14:50 (v14),
243 bodies (slide case hidden), 532 pairs with touching boxes, each tested by a 3D boolean intersection.
**Result: 8 overlaps, 0 unintended.** Errors: 0.

| Part A | Part B | Overlap (mm³) | Depth (mm) | Intended? |
| --- | --- | --- | --- | --- |
| PCB / J2 | Ribbons and wires / E-paper FPC | 0.036 | 0.12 | E-paper ribbon tip 2 mm inside J2 (connector model is a solid box): intended |
| PCB / J2 | Ribbons and wires / E-paper FPC | 0.090 | 0.12 | E-paper ribbon tip 2 mm inside J2 (connector model is a solid box): intended |
| PCB / J3 | Magnet connector / Magnet pins (straightened) | 1.321 | 0.60 | Magnet connector leg pushed into the J3 header (solid-box model): intended |
| PCB / J3 | Magnet connector / Magnet pins (straightened) | 1.321 | 0.60 | Magnet connector leg pushed into the J3 header (solid-box model): intended |
| PCB / J3 | Magnet connector / Magnet pins (straightened) | 1.321 | 0.60 | Magnet connector leg pushed into the J3 header (solid-box model): intended |
| PCB / J3 | Magnet connector / Magnet pins (straightened) | 1.321 | 0.60 | Magnet connector leg pushed into the J3 header (solid-box model): intended |
| Ribbons and wires / LiPo lead red | LiPo battery / JST-PH plug (in J4) | 0.008 | 0.07 | battery wire entering the JST plug: intended |
| Ribbons and wires / LiPo lead black | LiPo battery / JST-PH plug (in J4) | 0.026 | 0.15 | battery wire entering the JST plug: intended |

The same 8 contacts as stage 14 (connector models are solid boxes, so a ribbon or leg inside them shows as an overlap).
Nothing touches the new faceplate skirt, the ramped walls or the groove.

**Key clearances (measured in the model, `clearances.json`):**

| Clearance | Gap (mm) | Note |
| --- | --- | --- |
| J4 ↔ back-cover floor (after the solar-box grind) | 0.41 | unchanged: leave no stub when grinding zone 1 |
| Camera module ↔ back cover / 7 mm window | 0.71 | lens 0.5 below the inside of the floor, centred on the window |
| Magnet face ↔ U-notch (faceplate top wall) | 0.25 | face flush with the outer wall (face Y 81.67 = wall surface at X 22.5) |
| Magnet ↔ back cover / board edge | 0.71 / 0.71 | |
| Board ↔ faceplate (locating post H6) | 0.1 | H6 radial clearance 0.10 (post Ø2.9 in a Ø4.2 hole, 0.55 off-centre); others 0.53-0.91 |
| Board ↔ back cover | 1.6 | |
| LiPo pouch ↔ back-cover floor / board | 0.0 / 2.24 | lies on the floor (0 = resting on it), 2.2 under the board |
| JST plug ↔ back cover | 0.8 | |
| E-paper ↔ faceplate (solar frame) | 0.29 | |
| E-paper top film ↔ window mask | 1.79 | mask never touches the panel |
| Camera ribbon ↔ back cover (mid rib) | 0.48 | |
| Closed thickness | 11.8 screen section / 11.3 keypad (+0.3 feet, +1.5 keys) | = D10 (11.7-11.8 / 11.3) |

**Grind zones: which part each one is in (the notch did not move):**

| Zone | Part | Note |
| --- | --- | --- |
| 1 solar_box | navy back cover | frame + grid flat to the floor |
| 2 rib_b | navy back cover | 14 mm of upper rib B over the camera |
| 3 camera_window | navy back cover | 7 mm hole through the floor |
| 4 lr44_cup | silver faceplate | still in the model; stage 14 says it can stay (battery on the floor) |
| 5 magnet_slot | **silver faceplate's top wall** + the navy back cover's thin locating lip behind it | the top end is in the screen section, where the silver part is deep (photos 2a97120d / 66bb7a5f: the silver top wall runs down to the parting line, the navy cover has only a low rim there). Same U-notch, 21.5 × 7.95 from the rim, 6.4-27.9 mm from the right edge |

**Slide case (informational, not used in testing):** in use (on the back) it covers the 7 mm camera window
(fix: an 8 mm hole, `slide_case_mods.md`); it never blocks the magnet (open end at the top in both positions);
0 overlaps in both positions (`case_fit.json`). Adds 1.5 mm (0.3 feet + 1.2 floor): 13.3 / 14.3 mm over the keys in use.

## 8. Faceplate refinement re-check (rev D, 2026-10-05 evening)

**What changed in the silver faceplate** (from photos e82c982d, 2a97120d, 2f54d6c7; details in `../replica/photo_notes.md`, "Rev D"):
- **LR44 holder now present, not ground.** Stage 14 put the battery on the back-cover floor, so grind zone 4 is
  "don't grind": the `GRIND lr44_cup` feature is gone from `build_final_assembly.py` (4 grinds left). The holder is
  modelled like the photo: Ø12.4 cup with a 0.8 wall, **5.5 tall (D7, unchanged)**, open 4 mm on its left and right
  (clip slots), an I-bar either side (±9.0 from the centre) joined by short lugs, and a low contact rib across it.
- **Comb snap tabs** mid top (X +5, 9 wide) and mid bottom (X +3.5, 7 wide), 3 teeth each. Top: the teeth reach 0.4
  below the rim into a **new notch in the navy lip** (0.2 clearance). Bottom: they hang inside the navy end wall down to
  Z 2.4, 0.2 above a **new catch block** at the foot of that wall (photo 2f54d6c7). The hook itself is not modelled.
- **Side clips:** per side 3 pins (unchanged, C14) + **3 wire hooks** (Y 63.5, 58.1 and the C4 holder at 26) with a
  return leg; tips stay at the C4 5.5 depth.
- **Key collars 0.8 tall** (were 0.5, and the rings had in fact never been built: only the webs between keys). REPLAY's
  rim is 0.35 so it clears its direction domes. Beads on the row ribs, a rib under the last row, crush ribs on the
  locating posts. Screw posts unchanged.
- Unchanged: C1–C15, the window, every post position and diameter, the back cover apart from the notch + catch, and all
  other bodies.

**Check:** 2026-10-05 17:18:57, same board STEP (v14), 243 bodies, 532 pairs, 3D boolean intersection each.
**Result: 8 overlaps, 0 unintended, 0 errors**, exactly the 8 intended contacts of section 7 (magnet legs in J3 ×4,
e-paper ribbon in J2 ×2, LiPo leads in the JST plug ×2). Nothing touches the LR44 holder, snap tabs, notch, catch,
hooks, collars, beads or crush ribs. Slide case (`case_fit.json`): 0 overlaps in both positions. Replica on its own
(`../replica/interference.txt`): 0 overlaps (Casio board and LCD reference parts included).

**Key clearances (`clearances.json`, 21:35):**

| Clearance | Section 7 | Rev D |
| --- | --- | --- |
| J4 ↔ back-cover floor (after the solar-box grind) | 0.41 | **0.41** |
| Camera module ↔ back cover / 7 mm window | 0.71 | **0.71** |
| Board ↔ locating post H6 | 0.10 | **0.10** |
| Magnet face ↔ U-notch / back cover / board edge | 0.25 / 0.71 / 0.71 | **0.25 / 0.71 / 0.71**; face Y 81.67 = outer wall (flush) |
| **LiPo pouch ↔ LR44 holder (now present)** | (cup ground) | **0.30**: the holder's bottom (Z 5.1) over the pouch top (Z 4.8), at X -16.7, Y 63.9 |
| LiPo pouch ↔ back floor / board | 0.0 / 2.24 | 0.0 / 2.24 |
| JST plug ↔ back cover | 0.8 | 0.8 |
| E-paper ↔ faceplate (solar frame) | 0.29 | 0.29 |
| Camera ribbon ↔ back cover (mid rib) | 0.48 | 0.48 |
| E-paper ribbon ↔ faceplate | — | 1.89 |
| Keymat ↔ board | — | 0.11 |

**Battery under the LR44 holder: it fits, with 0.3 mm.** No trim is needed. But the earlier advice of a 1-2 mm foam
pad on top of the battery (section 1) no longer works at its left end, where the holder is: use a pad of 0.2 mm or
less there (or a strip of tape to the floor instead), so nothing presses the pouch against the holder.
If a pouch is fatter than 3.8 mm (they swell with age), trim the holder's ring and I-bars back by the excess: e.g.
grinding 1.0 mm off the bottom of the holder (to 4.5 tall) gives 1.3 mm of room and keeps the cup.

Files: `interference.json`, `clearances.json`, `case_fit.json`, `ai_calculator_final_assembly.f3d/.step`,
`../replica/fx115es_replica.f3d/.step/stl`. Pictures: `renders/compare/cmp_faceplate_inside_e82c982d.jpg`,
`cmp_front_e59eff71.jpg`, `cmp_front_4e287737.jpg`, `compare/*_cad.png`, and the step renders that show the faceplate
(`renders/steps/s01*`, `s08*`, `s09*`, `s10*` + `_ann`). The grind renders and the grinding manual still show the old
LR44 step: ignore it (zone 4 is not ground).


## 9. Rev E: battery changed to the Adafruit #1317, 150 mAh (2026-10-06)

**What changed:** only the battery. The **Adafruit #1317** (150 mAh, 26.02 × 19.75 × 3.8, same JST-PH plug and polarity)
replaces the #1570 (100 mAh, 31 × 11.5 × 3.8). It lies on the back-cover floor in the same corner, centred at front
(−15.6, 68.7) = KiCad (165.6, 70.24), Z 1.0–4.8. Long side left-right, lead end towards J4. The LR44 holder is kept and
there is no new grind. The leads are re-routed from the cell's right end down past rib A, then right, into the plug from
below. Why and which cells were compared: `battery_upgrade.md`. Exact placement for the guide: its section 8.

**Check:** 2026-10-06 00:29:03, board STEP v14, 243 bodies, 532 pairs, 3D boolean each. **8 overlaps, 0 unintended,
0 errors**: the same 8 intended contacts as sections 7–8 (magnet legs in J3 ×4, e-paper ribbon in J2 ×2, battery leads in
the JST plug ×2).

**Key clearances (`clearances.json`, 00:29):**

| Clearance | Rev D (#1570) | Rev E (#1317) |
| --- | --- | --- |
| J4 ↔ back-cover floor | 0.41 | **0.41** |
| Camera module ↔ back cover / window | 0.71 | **0.71** |
| Board ↔ locating post H6 | 0.10 | **0.10** |
| Magnet face ↔ U-notch / back cover / board edge | 0.25 / 0.71 / 0.71 | **0.25 / 0.71 / 0.71** |
| Battery top ↔ LR44 holder | 0.30 | **0.30** in the model (at front X −12.4, Y 67.9); **≈ 0.20** with the 0.1 mm double-sided tape under the cell |
| Battery bottom ↔ battery lid (cell bridges the old lid opening) | — | **0.42** (lid top Z 0.58) |
| Battery ↔ floor / board | 0.0 / 2.24 | **0.0 / 2.20** (the board edge overlaps the cell by 1.9 mm, 2.2 above it) |
| Battery left ↔ corner screw boss / screw | — | **2.6 / 3.54** |
| Battery bottom ↔ rib A | — | **1.0** |
| Battery top edge ↔ top-wall lip / snap teeth | — | **0.83** |
| Battery right (lead end) ↔ J4 | — | **7.0** |
| Battery leads ↔ back cover / board | — | 1.72 / 1.59 |
| JST plug ↔ back cover | 0.8 | 0.8 |
| E-paper ↔ faceplate, camera ribbon ↔ mid rib, board ↔ keymat | 0.29, 0.48, 0.11 | unchanged |

Exported: `ai_calculator_final_assembly.f3d/.step` (00:29). Re-rendered: `back_cover_off*.png` (3), `exploded_front/back.png`,
`section_*.png` (all 7, incl. `section_j4_battery`, `section_battery_lengthwise`), `steps/s04_parts(_ann).png`,
`steps/s09_battery(_ann).png` (labels now say "150 mAh #1317"). `hero_*`, `front`, `back`, s10/s11 don't show the
battery (cover closed): not redone.

**Fixing (decided):** 0.1 mm double-sided tape under the cell, no foam. The cell lies over the old battery-lid opening
in the floor (opening ≈ front X −24…−11.5, Y 64…77), so the tape goes only on the solid floor round it: an L of
6 × 18 mm under the lead end + 4 × 16 mm along the bottom edge (battery_upgrade.md section 8). The tape is not modelled
as a body; it lifts the cell 0.1 mm, which leaves ≈ 0.20 mm under the LR44 holder. The model, check and exports keep
the cell straight on the floor (Z 1.0–4.8), as the measured envelope says.

**Re-verified 2026-10-06 (resume):** Fusion model still in rev E (cell X −28.61…−2.59, Y 58.83…78.58, Z 1.0–4.8; plug in
J4, mouth down, leads in from below); exports and renders of 00:29 match it. `s04_parts_ann.png` / `s09_battery_ann.png`
re-annotated with "150 mAh #1317".


## 10. Rev F: side-wall inner rib to the rim, zone 6, full clearance table (2026-10-06, review 07)

**What changed:** only the faceplate's screen-section **inner ribs** (the thin walls the three round pins stick out of): they now run from the
parting line down to the plate (`INNER_RIB_FULL`), because C4 (4.7 / 5.5) was read on the rim and photo 871567b6 shows the rib's top edge
level with the Casio board's back face. Until the depth rod says otherwise this is the conservative case. With that rib at board depth two
things on the KiCad-low-x side (the calculator's right-hand, solar-window side) run into it: the ESP32's bare 0.8 mm antenna tab
(x 114.75–118.9: 0.4 mm) and the board's own un-narrowed top corner (x 114.3–114.8, y 72–83: up to 0.85 mm). New conditional grind
**zone 6 "side-wall relief"**: the 1 mm rib removed between KiCad y 71.5 and 106.5 (15–50 mm from the top outer edge) from the rim down
to the stub level (6.5 mm below the rim), pins and hooks kept. Full story, pre-grind checklist and the file list: `../../verification/07_final_review_fitment.md`.

**Check (2026-10-06 10:33, board STEP v14, 243 bodies, 532 pairs, 3D boolean each):** 18 overlaps, **0 unintended**: the 8 intended
contacts of sections 7–9 plus 10 overlaps between the navy cover's wall ticks (guessed positions, `TICK_Y`) and the now full-height inner
rib — Casio part against Casio part, both guesses, they mate by design and touch nothing of ours. Before the grinds (`interference_before_grind.json`):
54 overlaps, among them U1 (antenna tab) 0.81 mm into the rib and the board corner 0.71 mm (3.1 mm³) — the two that zone 6 removes.

**Full clearance table** (`clearance_table_full.md`, new stage `fulltable`): every board body and every loose part against every shell
body, 621 pairs, **620 PASS, 1 FAIL** = the board against locating post H6 (0.10 mm, known since stage 13).

| Clearance | Rev E | Rev F |
| --- | --- | --- |
| J4 ↔ back-cover floor | 0.41 | **0.41** |
| Camera module ↔ back cover / window | 0.71 | **0.71** |
| Board ↔ locating post H6 | 0.10 | **0.10** |
| Magnet face ↔ U-notch / back cover / board edge | 0.25 / 0.71 / 0.71 | **0.25 / 0.71 / 0.71** |
| Battery top ↔ LR44 holder | 0.30 | **0.30** |
| E-paper ↔ faceplate (solar frame, in-plane) | 0.29 | **0.29** |
| **U1 (ESP32 incl. antenna tab) ↔ faceplate** | not measured (rib absent at that depth) | **1.09** after zone 6 (0.4 overlap without it) |
| U1 ↔ back cover / e-paper / key mat | — | **2.50 / 1.02 / 12.35** |
| Board ↔ faceplate elsewhere | — | H6 only; the top corner clears the relieved rib |
| J1 / J2 / L1 / D2 ↔ back cover | — | 0.95 / 2.92 / 2.91 / 0.66 |
| Camera ribbon ↔ mid rib, board ↔ key mat | 0.48, 0.11 | unchanged |

Pictures: `renders/section_antenna.png`, `section_antenna_wide.png`, `antenna_corner.png`, `grind_antenna_relief_before/after.png`.
Exported `ai_calculator_final_assembly.f3d/.step` (10:35). Everything else of rev E (battery, grinds 1–5b, mask, case) is unchanged.
