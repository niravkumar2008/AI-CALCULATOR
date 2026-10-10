# 07: Final fitment review, v14 board in the ground fx-115ES shell (2026-10-06)

> **2026-10-06 (later): superseded on S1 / zone 6 / checklist items 1, 8, 9 by [`12_rib_pin_recheck.md`](12_rib_pin_recheck.md).** Nirav's depth rod put the inner rib's top edge 4.5–5.0 mm below the rim, the round pins at 5.1–6.1 and the U hooks at 5.5 (both walls): the rib, every pin and every hook sit at board height. Zone 6 is now mandatory (notch to 7 mm below the rim), zone 6b snips all pins and hooks on both walls, zone 6c (rib ends) is conditional. Verdict there: order the board as-is.

> **Correction 2026-10-10 (top-edge datums only; findings and verdict unchanged).** The "mm from the top" figures below were taken from the top of the whole outline (model Y 82.1) or from the middle of the top end. The guides now measure each part from its **own** top outer edge. Front shell, zone 6: measured along the solar-window wall from the rim's top outer edge right above the side rib (about 1 mm lower than the middle of the top end), so zone 6 relief 15–50 → **14–49 mm**, board corner strip 15–26/27 → **14–25/26**, antenna tab 31–50 / 32–48 → **30–49 / 31–47** (relief / tab itself). Back cover: zone 1 outer frame **6.3–19.3 mm** down from the back cover's own top outer edge, straight down at the middle of the box (older notes: 7.2–19.2 from the outline top); camera hole **38.0 mm** from the back cover's own top outer edge on the centre line (38.3 was to the outline top, where the silver front shell stands about 0.3 mm proud). KiCad coordinates are unchanged. Source: `enclosure/final_assembly/grinding_manual.html` (zones 1, 3, 6).

**Verdict: GO for the shell work, with one new conditional grind (zone 6, antenna relief) and a 10-minute pre-grind checklist that must be done on the real shell before the first cut. Nothing found needs a board change; `hardware/kicad/*` is untouched.**

Scope: every guessed shell dimension, outline vs faceplate, heights on both sides, e-paper window/mask/FPC, camera, magnet, the grinding manual + jigs, the assembly guide, the fit-check files, and a from-scratch Fusion check of the whole closed assembly (every board part and every loose part against every shell feature). Inputs: `FINAL_STATUS.md`, `measurements_2026-10-03.md` (C1–C15, D1–D13), `stage12_measurements.md`, `stage13_heights.md`, `stage14_verification_fixes.md` §6, `enclosure/final_assembly/*` (report §7–9, PROGRESS, battery_upgrade, window_mask, slide_case_mods, renders, both scripts, both guides), `enclosure/jigs/README.md`, `enclosure/replica/photo_notes.md` + `fitcheck_report.md`, `fitcheck/*`, verification 06 (M1) and 10, the KiCad board parsed by script (Edge.Cuts, holes, U1/J1–J4 pads, rule areas), all 29 of Nirav's photos (both upload folders, every file opened and compared with `renders/compare/*` and the grinding-manual pictures), and the live Fusion model (`ai_calculator_final_assembly.f3d`, rebuilt as rev F).

Frames: KiCad x/y as in the board file (component side up). Front view X = 150 − x, Y = 138.94 − y, Z from the outside of the back cover (board component face Z 7.0, key face 7.8, faceplate rim Z 1.5, plate underside Z 10.5 in the screen section). "Depth below the rim" = Z − 1.5. On the real calculator, **KiCad low x = the right-hand side as you use it** (solar-window side); the paper map lies in the shell mirror-image to the front view.

---

## 1. Findings

### FATAL
None.

### SERIOUS

**S1. On the solar-window side the board's own top corner (up to 0.85 mm) and the ESP32 antenna tab (0.4 mm) run into the faceplate's inner side-wall rib if that rib reaches the board plane, and the model had never looked.**
- Evidence: U1 body from the STEP spans KiCad x 114.74–135.26, y 89.29–104.71; the part past the board edge (x < 118.9) is the module's bare antenna PCB, **0.8 mm thick** (Z 6.1–6.9 in the model, i.e. flat on the board's component face), not the 2.4 mm shield (verification 06 M1 assumed the full module height; the tab is 0.8). The traced shell outline is 79.0–79.1 wide at y 89–105; C4 says the screen-section wall is 4.7 (5.5 at the wire holders), so the inner face of the thin inner rib is at x ≈ 115.1–115.2. Module tip 114.75 → **0.35–0.45 mm overlap over 15.4 mm, 0.8 mm tall**. In the replica (rev C–E) the inner rib was modelled only from Z 8.0 up to the plate ("the stubs stop above the board plane"), so the component-side depth was empty wall and every check passed. But C4 was read with the calipers **on the rim**, and photo 871567b6 (Casio board in the shell) shows that rib's top edge level with the board's back face, so the rib is there at board depth.
- The same rib also catches the **board's top corner strip**: Edge.Cuts keeps the full width there (x 114.28 at y 82.9 → 114.77 at y 72.0; stage 12 narrowed the board to x 118.9 only from y 82.9 down, below the LCD), and the rib (inner face x ≈ 115.15, modelled Y 24–66 = KiCad y 72.9–114.9 because the two wire hooks hang from it at y 75.4 / 80.8) is up to **0.85 mm inside that strip over 10 mm** (rev F before-grind check: 3.1 mm³, Z 7.0–7.71). This one is a rigid board edge, not a 0.8 mm tab, so it is the more certain of the two.
- Failure mode: the board sits 0.4–0.85 mm proud at that corner, cocked; J4 (0.41 mm to the floor) and the camera lens (0.71) lose their margin, the rim gaps. Nothing electrical.
- Fix applied: (a) replica rev F: `INNER_RIB_FULL = True` (inner ribs from the rim to the plate, pins/hooks unchanged); (b) new **grind zone 6 "side-wall relief"** in `build_final_assembly.py` (`ANT_RELIEF_K` = KiCad y 71.5–106.5 = 15–50 mm from the top outer edge: the 1 mm rib removed from the rim down to the stub level 6.5 mm below the rim, 35 mm long, pins and hooks kept; after it the tab has 0.9–1.1 mm and the board corner ≥ 0.6 mm in the model), in the grinding manual (new article, pictures `grind_antenna_relief_before/after.png`, `section_antenna.png`), the assembly guide (zone table + step-8 check), both grind maps (dashed red zone 6 + the yellow tab), FINAL_STATUS, HANDOFF, ORDER_WALKTHROUGH/CHECKLIST, jigs README; (c) the paper dry fit now carries the tab printed on the map (the coordinator's `paper_dry_fit_v14.svg` tab, x 114.4–118.9 / y 89.3–104.7, is **correct**: 0.35 mm past the module tip, same y as the body). Zone 6 is conditional: the paper template (with the tab) or the depth rod (checklist item 1) decides it. For v15 the board corner should simply be narrowed to x 118.9 all the way up (noted in HANDOFF).
- Measurement to take: checklist item 1.

**S2. The e-paper panel's top edge sits 0.29 mm from the solar-window frame, and the frame's position and depth are photo guesses (±0.5 mm).**
- Evidence: `clearances.json` "E-paper panel ↔ Front shell 0.29 at front (27.25, 60.55, 8.1)" = KiCad (122.75, 78.39), the panel's corner against the rib between the solar window and the display window; `SOLAR_FRAME = (0.8, 2.5)` and `SOLAR_C` are "photo (stage 10)" / "guess" in `build_fx115es_replica.py`. The panel (59.0 × 29.2, y 78.39–107.59) is 4.9 mm taller than the window opening (C12 24.3) and extends 2.56 mm above the window's top edge (C13 → y 80.95) into that strip.
- Failure mode: if the real strip between the two openings is narrower than about 2.6 mm (inside, at the rib), the frame rib lands on the panel's glass edge when the board is pressed onto the posts: the panel is pushed off its tape or cracked.
- Fix applied: pre-grind checklist item 2 (measure the strip); assembly guide step 8 now says what to do if it touches (slide the panel ≤ 0.5 mm towards the keys before the tape sets: the picture stays inside the window, 3.5 mm margin, and inside the mask's 0.4 mm margins only if ≤ 0.4; the mask template is cut after this check anyway). No CAD change: the model keeps the photo value.

### MINOR

**M1. Verification 06 M1 and FINAL_STATUS §1 described the antenna as "left edge (front view)" and 2.4 mm tall.** It is the KiCad-left edge = the right-hand side in use, and the overhanging part is 0.8 mm. Fixed in FINAL_STATUS §1 (items 3, 6, 9), fitcheck/README, ORDER_WALKTHROUGH, HANDOFF, the map legend. 06 is left as written (history); this file supersedes it.

**M2. E-paper ribbon spare: docs disagreed (0 vs 1.6 mm). Settled.** The Fusion path from the glass edge into J2 is 12.7 mm with 2.0 mm inside J2 (fold hugging the panel end at r 0.25, 7.5 mm back under the panel, 1.9 mm down through the slot, 1.8 + 2.0 mm into J2); the ribbon is 14.3 ± 0.3 → **1.6 mm nominal, ≈ 0.3 mm if the ribbon is short and J2 wants 3 mm of insertion**. Rule kept: build as if there were none; E7 and the stiffener rule (stage 14 §6) stand. Fixed in: assembly guide step 5 ("docs disagree" label removed), FINAL_STATUS §1b, HANDOFF, stage14 §6, assembly_report §3.

**M3. Camera ribbon spare: docs disagreed (12 mm vs 1.4 mm). Settled.** The Fusion path from the module's near edge into J1 is 58.7 mm (tip 2 mm inside J1, lying 1.46 mm under the board on the tallest part in its corridor). Seeed's 70.5 mm is read from the module's **far** edge (stage 14 / verification 03 L1), so only ≈ 62 mm is ribbon: **1.4–3.3 mm spare**, never 12. The "under 70.3 mm → shift the module ≤ 1.7 mm towards J1 and drill where the lens is" rule stands. Fixed in assembly_report §3, final_assembly/README (which said "~11 mm loop"), grinding manual test-fit.

**M4. Zone numbering was inconsistent** (jigs README and battery_upgrade §3 called the magnet notch "zone 4" and the LR44 trim "5a/5b"; the guides use 1–5, 5b). Everything now uses the guide's numbers: 1 solar box, 2 rib B, 3 camera window, 4 LR44 cup (**keep**), 5 magnet notch, 5b lip, 6 antenna relief; the LR44 trim of the battery study is "4a/4b". Fixed in jigs README, battery_upgrade §3, both grind maps (zone labels printed on the map), fitcheck README.

**M5. Fit-check files were stale** (battery block #1570 at the old bay position, "LR44 cup: TRIM FLAT", no antenna tab, no drill mark for zone 3). Regenerated with `tools/make_fitcheck.py` (patched): `dummy_board.stl` now has the loose #1317 block at KiCad x 152.59–178.61, y 60.36–80.11 (z 0–3.8, verified in the STL) and the antenna overhang is in it through U1's 3D model (verified: vertices to x 114.75, 0.8 mm tall); `grind_map_front_shell.svg` has the yellow 4.5 × 15.4 tab, the LR44 cup in green "zone 4 (KEEP)", zone 6 dashed, zone numbers in the legend, and the **50 mm line unchanged** (x 112.162 → 162.162 in the 1:1 viewBox); `grind_map_back_cover.svg` has the 7 mm zone-3 drill circle with cross-hairs; `clearance_table.md` LiPo row now describes the floor position.

**M6. The faceplate's LR44 holder leaves only 0.30 mm over the #1317 (0.20 with the 0.1 mm tape), and the holder's height is D7 "5–6" modelled as 5.5.** If the real holder is 5.8 tall it touches the cell (a pouch is soft, so this is a rattle/pressure issue, not a crush). Checklist item 3 measures it; the fallback (jig C trim, 1.9 mm) already exists.

**M7. The back cover's moulded label (photo e82c982d outside, 2f54d6c7 inside: the faint rectangle 32–48 mm from the top edge, x ≈ 135–170 KiCad) contains the camera window spot and the rib B grind.** Debossed lettering means the floor can be thinner than D5's "about 1.0" there. Checklist item 4 (floor thickness at the label ≥ 0.9 before grinding rib B flat; never below 0.8). The second, grained rectangle lower down (between low ribs 1 and 2, ≈ 95–104 mm from the top, over J1/D2) is a mould texture, not a pad: nothing to do.

**M8. Grinding manual zone 4 still carried the sentence "the printed map says TRIM FLAT, ignore it"** and the test-fit text about the old STL block. Updated. The manual's zone 5b "lip" is still a model guess; the pre-grind checklist item 5 settles it with the cover in hand.

### NOTE

**N1. Camera optics.** Lens top 0.5 mm below the floor's inside face, 7 mm hole in a 1.0 mm floor: the unvignetted half-angle is atan((3.5 − 1.4) / 1.5) ≈ 54° centred, ≈ 36° at 1.0 mm offset, ≈ 22° at 1.5 mm; the OV5640 module's diagonal half-FOV is ≈ 34–39°, so **centre the module within ≈ 0.8 mm of the hole** (added to the assembly guide step 6). Minimum focus of the AF module ≈ 10 cm: the calculator reads a page held 15–30 cm above it, back down; it cannot focus lying flat on the page (added). Light leak: a 0.5 mm ring beside the 6 mm barrel lets light into the case; it lights the board only (e-paper is not light-sensitive) — cosmetic, nothing to do. Tilt: the module's base is the board; a tilt only comes from uneven tape, which the "even ring all round" check catches.

**N2. Magnet connector.** U-notch 21.5 × 7.95 in the top wall, 6.4–27.9 mm from the right edge in use (= KiCad x 116.75–138.25) ✓ consistent in all docs and the jig B window. Face flush (model 0.25 per side, 0.71 to the back cover, 0.71 to the board edge). N end = pin 1 = VBUS (stage 14 order VBUS, D−, D+, GND; meter check mandatory before gluing, both guides) ✓. The top-corner screw post (x ≈ 116, y ≈ 65, Ø3.85) overlaps the notch's x range by 1.2 mm but sits 5 mm behind the wall: it is not cut, only taped (manual says so). Leg straightening 5.3 mm, 4.5 mm into J3, trim 0.5–1 mm if they bottom out ✓. Slide case: open end at the top in both positions, magnet never covered; 8 mm case hole over the 7 mm window (`slide_case_mods.md`) — all case sizes are guesses, not used for testing ✓.

**N3. Posts, screws, snap tabs, hooks.** Locating posts Ø2.9 in Ø4.2 holes/slots: H6 0.10 mm (known; file the hole, never the post), others 0.46–0.91. Screw posts Ø3.85 in the Ø6.0 H1/H3 holes (0.91); the 6 screws run from the back cover into the faceplate's posts, the board carries no screw, so screw length is Casio's own (guide asks to measure it; model assumes 2 × 8). Comb snap tabs: top one at front X +5 (KiCad x 140.5–149.5) is 2.5 mm from the magnet body's end and clear of J3; bottom one at X +3.5 sits inside the navy end wall below the board's bottom edge. Side-wall pins (C14, 3 per side) and the three hooks per side stay above the board plane per Nirav; the board's narrowed screen section (x 118.9–183.65) clears the pins by ≥ 0.35 mm (right) / 1.5 mm (left) even if they were at board height; the e-paper panel (x 120.5–179.5) is well inside them.

**N4. Heights, component side (closed, after grinds):** J4 0.41 to the floor (solar box flat, **no stub**), camera lens 0.71 to the window edge (0.1 mm tape, no foam), J3 header 3.42, magnet body 0.71, U1 shield 2.5, J1/J2/L1 ≥ 3.0, D2 under the big ring 0.74 (no ring grind), camera ribbon over the mid rib 0.48, JST plug 0.8, battery leads 1.7. Key side: e-paper 1.05 + 0.15 tape on the board, 1.5 mm under the plate, mask 1.79 above the film, lens lifted 0.1 by the mask; key mat 0.11 from the board (lies on it), domes/travel as Casio's since the board sits where Casio's did (checklist item 7: Casio board thickness). LR44 holder 0.30 over the cell. Closed thickness 11.8 / 11.3 = D10 ✓.

**N5. Photo review vs CAD.** All 29 photos opened. New things the CAD did not have: (a) the inner rib's depth (S1); (b) the moulded label (M7); (c) the mat's four small rectangular feet at its bottom corners (photo 28bb9c4b) — they land on bare board at y > 205, harmless; (d) photo 7c6e8116's full-width cross rib between the display and the oval keys is on the key side between the panel (ends y 107.6) and the mat (starts ≈ y 113): no part there. Everything else matches `renders/compare/*` (split line, snap tabs, hooks, holder, ribs, rings, solar box, ticks).

---

## 2. Fusion rev F: the full check (closed case, grinds applied)

Model rebuilt from scratch (`python build_final_assembly.py all`, then `check grind=off`): replica rev F shell (inner ribs to the rim), board STEP v14 (2026-10-04 14:50), 243 bodies, 5 grinds (solar box, rib B, camera window, magnet U-notch + lip, antenna relief), battery rev E.

**Check (after grinds): 243 bodies, 532 body pairs with touching boxes, 18 overlaps, 0 errors — 0 unintended.** Before the grinds: 54 overlaps (table 2.3).

| Part A | Part B | Depth (mm) | mm³ | KiCad x, y | Z | What it is |
| --- | --- | --- | --- | --- | --- | --- |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 75.9 | 1.50–3.00 | model artefact: Casio navy wall ticks (guessed positions) vs the rev F full-height inner rib; both are Casio parts that mate by design |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 113.9 | 1.50–3.00 | model artefact: Casio navy wall ticks (guessed positions) vs the rev F full-height inner rib; both are Casio parts that mate by design |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 111.9 | 1.50–3.00 | model artefact: Casio navy wall ticks (guessed positions) vs the rev F full-height inner rib; both are Casio parts that mate by design |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 95.9 | 1.50–3.00 | model artefact: Casio navy wall ticks (guessed positions) vs the rev F full-height inner rib; both are Casio parts that mate by design |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 114.5, 113.9 | 1.50–3.00 | model artefact: Casio navy wall ticks (guessed positions) vs the rev F full-height inner rib; both are Casio parts that mate by design |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 93.9 | 1.50–3.00 | model artefact: Casio navy wall ticks (guessed positions) vs the rev F full-height inner rib; both are Casio parts that mate by design |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 114.5, 111.9 | 1.50–3.00 | model artefact: Casio navy wall ticks (guessed positions) vs the rev F full-height inner rib; both are Casio parts that mate by design |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 77.9 | 1.50–3.00 | model artefact: Casio navy wall ticks (guessed positions) vs the rev F full-height inner rib; both are Casio parts that mate by design |
| PCB/J3 | Magnet connector/Magnet pins (straightened) | 0.60 | 1.32 | 126.2, 64.2 | 5.40–6.00 | intended (legs pushed into the J3 header; solid-box model) |
| PCB/J3 | Magnet connector/Magnet pins (straightened) | 0.60 | 1.32 | 123.8, 64.2 | 5.40–6.00 | intended (legs pushed into the J3 header; solid-box model) |
| PCB/J3 | Magnet connector/Magnet pins (straightened) | 0.60 | 1.32 | 128.8, 64.2 | 5.40–6.00 | intended (legs pushed into the J3 header; solid-box model) |
| PCB/J3 | Magnet connector/Magnet pins (straightened) | 0.60 | 1.32 | 131.2, 64.2 | 5.40–6.00 | intended (legs pushed into the J3 header; solid-box model) |
| Front shell/Front shell | Back cover/Back cover | 0.50 | 0.38 | 185.3, 107.2 | 1.50–2.00 | model artefact: Casio navy wall ticks (guessed positions) vs the rev F full-height inner rib; both are Casio parts that mate by design |
| Ribbons and wires/LiPo lead black | LiPo battery/JST-PH plug (in J4) | 0.15 | 0.03 | 142.6, 80.2 | 3.59–4.08 | intended (wire entering the plug) |
| PCB/J2 | Ribbons and wires/E-paper FPC | 0.12 | 0.04 | 173.8, 86.8 | 5.86–5.97 | intended (ribbon tip 2 mm inside J2; solid-box model) |
| PCB/J2 | Ribbons and wires/E-paper FPC | 0.12 | 0.09 | 173.7, 99.0 | 5.86–5.98 | intended (ribbon tip 2 mm inside J2; solid-box model) |
| Ribbons and wires/LiPo lead red | LiPo battery/JST-PH plug (in J4) | 0.07 | 0.01 | 140.6, 80.2 | 4.33–4.72 | intended (wire entering the plug) |
| Front shell/Front shell | Back cover/Back cover | 0.05 | 0.03 | 115.0, 107.0 | 1.50–2.00 | model artefact: Casio navy wall ticks (guessed positions) vs the rev F full-height inner rib; both are Casio parts that mate by design |

Key clearances (`clearances.json`, rev F):

| Pair | Gap (mm) | At (front X, Y, Z) | KiCad x, y |
| --- | --- | --- | --- |
| Camera module/* ↔ Back cover/* | 0.71 | 3.0, 43.8, 1.5 | 147.0, 95.1 |
| PCB/J4 ↔ Back cover/* | 0.41 | 8.7, 63.5, 1.4 | 141.3, 75.4 |
| PCB/J3 ↔ Back cover/* | 3.42 | 27.8, 68.5, 4.4 | 122.2, 70.4 |
| PCB/U1 ↔ Back cover/* | 2.50 | 16.7, 44.2, 4.5 | 133.3, 94.8 |
| Magnet connector/* ↔ Back cover/* | 0.71 | 15.5, 80.7, 2.2 | 134.5, 58.3 |
| Magnet connector/* ↔ Front shell/* | 0.25 | 29.5, 80.7, 9.2 | 120.5, 58.3 |
| Magnet connector/* ↔ PCB/board | 0.71 | 14.0, 77.7, 7.7 | 136.0, 61.3 |
| LiPo battery/LiPo pouch (Adafruit #1317) ↔ Back cover/* | 0.00 | -17.2, 77.1, 1.0 | 167.2, 61.8 |
| LiPo battery/LiPo pouch (Adafruit #1317) ↔ PCB/board | 2.20 | -3.6, 58.8, 4.8 | 153.6, 80.1 |
| LiPo battery/JST-PH plug (in J4) ↔ Back cover/* | 0.80 | 11.4, 58.7, 2.0 | 138.6, 80.3 |
| E-paper panel/* ↔ Front shell/* | 0.29 | 27.2, 60.5, 8.1 | 122.8, 78.4 |
| E-paper panel/* ↔ Keymat/* | 9.43 | 29.5, 31.4, 8.6 | 120.5, 107.6 |
| Ribbons and wires/Camera FPC ↔ Back cover/* | 0.48 | 0.4, 20.2, 5.5 | 149.6, 118.7 |
| Ribbons and wires/E-paper FPC ↔ Front shell/* | 1.89 | -29.9, 39.7, 8.8 | 179.9, 99.2 |
| PCB/board ↔ Front shell/* | 0.10 | 19.4, -46.0, 7.7 | 130.6, 184.9 |
| PCB/board ↔ Back cover/* | 1.60 | -24.9, 15.7, 7.0 | 174.9, 123.2 |
| PCB/* ↔ Back cover/* | 0.41 | 8.7, 63.5, 1.4 | 141.3, 75.4 |
| Keymat/* ↔ PCB/board | 0.11 | 17.9, -46.5, 7.8 | 132.1, 185.5 |
| LiPo battery/LiPo pouch (Adafruit #1317) ↔ Front shell/* | 0.30 | -12.4, 67.9, 4.8 | 162.4, 71.0 |
| LiPo battery/* ↔ Front shell/* | 0.30 | -12.4, 67.9, 4.8 | 162.4, 71.0 |
| Ribbons and wires/* ↔ Front shell/* | 1.89 | -29.9, 39.7, 8.8 | 179.9, 99.2 |
| PCB/* ↔ Front shell/* | 0.10 | 19.4, -46.0, 7.7 | 130.6, 184.9 |
| LiPo battery/LiPo pouch (Adafruit #1317) ↔ Screws/* | 3.54 | -28.6, 74.0, 1.2 | 178.6, 64.9 |
| LiPo battery/LiPo pouch (Adafruit #1317) ↔ PCB/J4 | 7.04 | -2.6, 68.7, 1.5 | 152.6, 70.2 |
| Ribbons and wires/LiPo lead red ↔ Back cover/* | 1.72 | 9.4, 58.1, 3.7 | 140.6, 80.8 |
| Ribbons and wires/LiPo lead red ↔ PCB/board | 1.59 | -2.0, 54.7, 5.4 | 152.0, 84.3 |
| PCB/U1 ↔ Front shell/* | 1.09 | 35.2, 49.6, 6.9 | 114.8, 89.3 |
| PCB/U1 ↔ Keymat/* | 12.35 | 14.8, 34.2, 6.9 | 135.2, 104.7 |
| PCB/U1 ↔ E-paper panel/* | 1.02 | 29.5, 34.5, 6.9 | 120.5, 104.4 |
| PCB/U1 ↔ Screws/* | 19.95 | 22.4, 34.2, 6.1 | 127.5, 104.7 |
| PCB/J2 ↔ Front shell/* | 4.32 | -28.1, 51.9, 6.9 | 178.1, 87.0 |
| PCB/J1 ↔ Back cover/* | 0.95 | -0.5, -17.1, 4.9 | 150.5, 156.1 |
| PCB/J2 ↔ Back cover/* | 2.92 | -27.4, 44.2, 4.9 | 177.3, 94.7 |
| PCB/L1 ↔ Back cover/* | 2.91 | -11.4, 38.7, 4.9 | 161.4, 100.2 |
| PCB/D2 ↔ Back cover/* | 0.66 | -6.8, -14.4, 5.7 | 156.8, 153.4 |
| Window mask/* ↔ E-paper panel/* | 1.79 | 26.1, 57.4, 10.8 | 123.9, 81.5 |
| Magnet connector/* ↔ Keymat/* | 50.48 | 23.8, 72.4, 6.0 | 126.2, 66.6 |

### 2.1 ESP32 module (U1) vs its neighbours

| U1 (ESP32-S3-MINI-1 incl. antenna tab) vs | Min gap (mm) | At (front X, Y, Z) | KiCad x, y | Direction |
| --- | --- | --- | --- | --- |
| Back cover | 2.50 | 16.7, 44.2, 4.5 | 133.3, 94.8 | Z (rib B left, 1 mm rib over the shield) |
| Front shell | 1.09 | 35.2, 49.6, 6.9 | 114.8, 89.3 | X (side wall, after zone 6) / Z (pins above) |
| Keymat | 12.35 | 14.8, 34.2, 6.9 | 135.2, 104.7 | Y (mat starts below the screen section) |
| E-paper panel | 1.02 | 29.5, 34.5, 6.9 | 120.5, 104.4 | Z/Y (panel on the other side of the board) |
| Screws | 19.95 | 22.4, 34.2, 6.1 | 127.5, 104.7 | — |

Module body in the model (from the STEP): KiCad x 114.74–135.26, y 89.29–104.71; shield 2.46 tall over x 121.2–135.3, the antenna end (x 114.74–120.4) a 0.83 mm PCB tab lying on the board face. Before zone 6 (rev F shell, rib to the rim) the tab overlapped the inner rib by 0.4 mm and the board corner (x 114.3–114.8, y 72–83) by up to 0.85 mm; see 2.3.

Pictures: `renders/section_antenna.png` (cut across the tab and the side wall), `section_antenna_wide.png`, `antenna_corner.png` (back cover off), `grind_antenna_relief_before/after.png`.

### 2.2 Every part vs every shell feature

`enclosure/final_assembly/clearance_table_full.md` / `.json` (generated by the new `fulltable` stage): minimum distance of every board body (by designator) and every loose part (battery pouch + plug + leads, camera module bodies, e-paper glass/film/driver, e-paper FPC, camera FPC, magnet face/body/pins, window mask) against the faceplate, back cover, key mat, key caps, lens, mask, battery lid, screws, solar cell and feet, with PASS/FAIL (FAIL = gap < 0.2 mm or overlap, unless the contact is intended).

**621 pairs checked, 620 PASS, 1 FAIL.** The one FAIL is the board against locating post H6 (0.10 mm radial play, known since stage 13: file the board's hole towards the top of the board if it binds, never the post).

FAIL rows:

| Part | Body | Shell part | Shell body | Gap | At | KiCad | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- |
| PCB | board | Front shell | Front shell | 0.10 | 19.4, -46.0, 7.7 | 130.6, 184.9 | FAIL (< 0.2 mm) |

Intended contacts (PASS):

| Part | Body | Shell part | Shell body | Gap | At | KiCad | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- |
| LiPo battery | LiPo pouch (Adafruit #1317) | Back cover | Back cover | 0.00 | -17.2, 77.1, 1.0 | 167.2, 61.8 | PASS (intended contact: rests on the floor (0.1 mm tape under it, not modelled)) |
| PCB | board | Keymat | Keymat (rubber) | 0.11 | 20.9, -46.5, 7.7 | 129.1, 185.5 | PASS (intended contact: mat lies on the key side of the board (0.11 in the model)) |
| Magnet connector | Magnet face flange | Front shell | Front shell | 0.25 | 29.5, 80.7, 9.2 | 120.5, 58.3 | PASS (intended contact: face glued in the U-notch (0.25 per side, glue fills it)) |

### 2.3 Before the grinds (what each grind fixes, rev F)

Shell as delivered (every grind feature suppressed): 54 overlaps in total; the ones that are not the intended contacts or the tick artefact:

| Part A | Part B | Depth (mm) | mm³ | KiCad x, y | Z |
| --- | --- | --- | --- | --- | --- |
| Back cover/Back cover | PCB/J4 | 4.59 | 34.03 | 141.6, 72.4 | 1.41–6.00 |
| Back cover/Back cover | PCB/J3 | 2.08 | 44.68 | 127.5, 67.0 | 4.42–6.50 |
| Back cover/Back cover | LiPo battery/LiPo pouch (Adafruit #1317) | 1.91 | 58.26 | 153.5, 70.0 | 1.00–4.80 |
| Back cover/Back cover | PCB/U3 | 1.14 | 1.83 | 122.0, 75.9 | 5.36–6.50 |
| Front shell/Front shell | Magnet connector/Magnet face flange | 1.01 | 126.08 | 127.5, 57.8 | 2.19–9.21 |
| Back cover/Back cover | Ribbons and wires/LiPo lead red | 1.00 | 0.95 | 152.0, 70.0 | 4.31–5.41 |
| Back cover/Back cover | Ribbons and wires/LiPo lead red | 1.00 | 0.95 | 152.0, 76.0 | 4.31–5.41 |
| Back cover/Back cover | Ribbons and wires/LiPo lead black | 1.00 | 0.95 | 152.0, 76.0 | 2.91–4.01 |
| Back cover/Back cover | PCB/D1 | 0.85 | 1.31 | 129.2, 75.2 | 5.66–6.50 |
| Front shell/Front shell | PCB/U1 | 0.81 | 3.74 | 125.0, 97.0 | 6.11–6.92 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 75.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 114.5, 75.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 114.5, 77.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 114.5, 93.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 114.5, 95.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 114.5, 111.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 113.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 111.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 95.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 93.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 185.4, 77.9 | 1.50–3.00 |
| Front shell/Front shell | Back cover/Back cover | 0.80 | 1.20 | 114.5, 113.9 | 1.50–3.00 |
| Back cover/Back cover | PCB/Q2 | 0.79 | 1.75 | 135.9, 75.7 | 5.71–6.50 |
| Front shell/Front shell | PCB/board | 0.71 | 3.10 | 150.0, 136.4 | 7.00–7.71 |
| Back cover/Back cover | PCB/J4 | 0.71 | 1.93 | 141.6, 75.9 | 4.06–6.50 |
| Back cover/Back cover | Magnet connector/Magnet pins (straightened) | 0.60 | 0.28 | 126.2, 64.0 | 5.40–6.00 |
| Back cover/Back cover | Magnet connector/Magnet pins (straightened) | 0.60 | 0.28 | 123.8, 64.0 | 5.40–6.00 |
| Back cover/Back cover | Magnet connector/Magnet pins (straightened) | 0.60 | 0.57 | 128.8, 65.1 | 5.40–6.00 |
| Back cover/Back cover | Magnet connector/Magnet pins (straightened) | 0.60 | 0.28 | 131.2, 64.0 | 5.40–6.00 |
| Front shell/Front shell | Back cover/Back cover | 0.50 | 0.38 | 185.3, 107.2 | 1.50–2.00 |
| Front shell/Front shell | Magnet connector/Magnet plate | 0.50 | 23.27 | 125.2, 58.5 | 2.50–8.90 |
| Back cover/Back cover | PCB/J4 | 0.50 | 0.57 | 140.6, 70.3 | 3.53–6.00 |
| Back cover/Back cover | PCB/J4 | 0.50 | 0.57 | 142.6, 70.3 | 3.53–6.00 |
| Back cover/Back cover | Camera module/Camera lens barrel | 0.50 | 2.99 | 150.0, 95.1 | 1.50–2.00 |
| Back cover/Back cover | Magnet connector/Magnet plate | 0.50 | 3.31 | 127.5, 58.6 | 2.50–3.00 |
| Back cover/Back cover | PCB/J4 | 0.35 | 0.53 | 138.5, 75.9 | 4.37–6.50 |
| Back cover/Back cover | Camera module/Camera lens glass | 0.31 | 0.85 | 150.0, 95.1 | 1.49–1.80 |
| Back cover/Back cover | PCB/J4 | 0.25 | 0.38 | 144.7, 75.9 | 4.37–6.50 |
| Back cover/Back cover | LiPo battery/JST-PH plug (in J4) | 0.22 | 5.94 | 141.6, 76.4 | 1.91–6.41 |
| Back cover/Back cover | PCB/J4 | 0.21 | 0.02 | 140.6, 75.6 | 3.56–3.95 |
| Back cover/Back cover | PCB/J4 | 0.21 | 0.02 | 142.6, 75.6 | 3.56–3.95 |
| Back cover/Back cover | PCB/D7 | 0.09 | 0.06 | 120.5, 73.4 | 5.91–6.50 |
| Front shell/Front shell | Magnet connector/Magnet contact pads | 0.06 | 0.11 | 128.8, 57.3 | 4.95–6.45 |
| Front shell/Front shell | Magnet connector/Magnet contact pads | 0.06 | 0.08 | 126.2, 57.3 | 4.95–6.45 |
| Front shell/Front shell | Magnet connector/Magnet contact pads | 0.06 | 0.11 | 131.2, 57.3 | 4.95–6.45 |
| Front shell/Front shell | Back cover/Back cover | 0.05 | 0.03 | 115.0, 107.0 | 1.50–2.00 |

---

## 3. Pre-grind checklist (10 minutes, real shell + calipers with the depth rod, before the first cut)

Write every number on the printed template. Reference for all depths: a steel ruler laid across the faceplate's rim (the parting-line face); "below the rim" = depth-rod reading from the ruler.

| # | Measure | Where | Decides | Expected / limit |
|---|---|---|---|---|
| 1 | *(answered 2026-10-06: 4.5–5.0 → zone 6 needed; see verification 12)* **Depth from the rim to the top edge of the thin inner rib** of the screen-section side wall (the wall the three round pins stick out of), on the **solar-window side**, 32–48 mm from the top outer edge | faceplate inside | **zone 6** | > 5.5 mm: rib stops short of the board plane → no zone 6. ≤ 5.5 (or level with the rim): file the rib away 15–50 mm from the top (35 mm, zone 6; corrected 2026-10-06, was "19 mm"; the board corner at 15–26 mm needs it too, so also check there; *2026-10-10 datum: 14–49 and 14–26 mm, see the correction note at the top*). Also tells you the same thing: lay the paper template in with its yellow tab |
| 2 | **Width of the flat strip between the solar-window opening and the display-window opening**, measured on the inside at the rib between them | faceplate inside | e-paper top edge | ≥ 2.6 mm: fine (model 2.85). 2.3–2.6: slide the panel ≤ 0.3 mm towards the keys when taping. < 2.3: file 0.5 mm off that rib's lower face (keep the solar cell's seat) |
| 3 | **Height of the LR44 holder** above the plate (tips of the ring and I-bars) | faceplate inside | battery pressure | ≤ 5.5 mm (D7): 0.3 mm over the #1317. 5.6–5.8: no foam, 0.1 tape only (already the rule). > 5.8: jig C trim (1.9 mm, `battery_upgrade.md` §6) |
| 4 | **Floor thickness at the moulded label** on the back (outside, 32–48 mm from the top; the camera window and rib B are inside it): caliper jaws on the outside and inside at the deepest letter | back cover | zones 2, 3 | ≥ 0.9 mm: grind rib B flat to the floor as planned. 0.8–0.9: stop rib B 0.1–0.2 mm proud (camera margin is 0.6). < 0.8: use the spare cover |
| 5 | **Lip at the top end of the back cover's rim** where the magnet sits (6.4–27.9 mm from the right edge in use) | back cover | zone 5b | no lip: skip 5b. Lip: cut it 21.5 wide, same as the notch |
| 6 | **Screw length** (one screw) and that it is self-tapping | screws | nothing (Casio's own) | write it down; model assumes 2 × 8 |
| 7 | **Casio board thickness** | old board | key feel only | ≈ 0.8 (ours is 0.8); up to 1.0 changes nothing but the dome preload |
| 8 | **Inside width at the screen section** between the two inner ribs at board depth (5 mm below the rim), at 40 mm from the top | faceplate inside | board edge / antenna | ≥ 69.5 mm (nominal 69.9): board 64.75 wide + tab 4.15 fits with ≥ 0.6 after zone 6. < 69.5: tell Claude (an extra 0.5 mm off the rib is the fix, not the board) |
| 9 | *(answered 2026-10-06: 5.1–5.5 outer, 6.0–6.1 middle, hooks 5.5 → at board height, zone 6b snips them; verification 12)* **Depth from the rim to the round pins** (top of the pin) | faceplate inside | nothing if > 5.5 | Nirav: they stop above the board plane. If a pin is < 5.5 below the rim it is at board height: the board still passes between the pin tips (67.4 > 64.75), but note it |
| 10 | **Paper dry fit** with the regenerated `fitcheck/grind_map_front_shell.svg` (or `Claude outputs/paper_dry_fit_v14.svg`): 50 mm line = 50 mm, cut out with the yellow tab, every post in its hole, tab flat | faceplate | zone 6, hole positions | H6 least play (0.10); any post off by > 0.5 mm → stop, don't order |

Then grind in the order 1 → 2 → (3 held until the ribbon is measured) → 5 → 5b → 6, exactly as the grinding guide.

---

## 4. Every check made (with the passes)

| # | Check | Result |
|---|---|---|
| 1 | Guessed dimensions: side groove (not in any photo; only the unused slide case rides in it) | PASS, informational: nothing in the build touches it |
| 2 | Skirt depth Z 8.3 (guess): the faceplate skirt inside the navy wall in the keypad section | PASS: 0 overlaps with the board, mat or cover; a deeper skirt only overlaps the navy wall more |
| 3 | Snap clips (top X +5 9 wide, bottom X +3.5 7 wide, teeth 0.4 below the rim) | PASS: 0 overlaps; top tab 2.5 mm from the magnet body; the hook itself is not modelled, it engages the navy cover only |
| 4 | Key height / collars 0.8 / travel (D9 1.5, DOME_GAP 0.3 guess) | PASS: caps rest on domes at Z 8.1; board at Casio's position; checklist 7 |
| 5 | Lip 5b (guess 1.5 mm) | conditional grind, checklist 5 |
| 6 | Inner rib depth (new guess, now conservative) | S1 → zone 6 + checklist 1 |
| 7 | Solar-frame strip (photo) | S2 → checklist 2 |
| 8 | LR44 holder height D7 5–6 | M6 → checklist 3 |
| 9 | Floor at the label | M7 → checklist 4 |
| 10 | Board outline vs faceplate inner skin (C4 4.7) | PASS beside the LCD: board edge x 118.9 / 183.65 vs rib faces 115.2 / 184.8 → 3.7 / 1.15 mm; FAIL above y 82.9 on the low-x side without zone 6 (S1) |
| 11 | Antenna overhang and the board's top corner vs the wall's inner rib | S1 (0.4 / 0.85 overlap before zone 6; 1.09 / ≥ 0.6 after) |
| 12 | C14 stubs (pins) vs board edge | PASS: pins reach x 116.8–117.4 / 184.0–184.2, board stops 1.5 / 0.35 short even at board height; they are above the board per Nirav |
| 13 | Posts in holes (H1–H14) | PASS: H6 0.10 (known), others 0.46–0.91, screw posts 0.91 |
| 14 | Screw length through the 0.8 board | PASS by construction (posts pass through Ø6 holes; screws don't touch the board) |
| 15 | J4 height 5.5 vs floor after zone 1 | PASS 0.41 (no stub) |
| 16 | J3 + magnet heights | PASS 3.42 / 0.71 |
| 17 | Camera 5.4 + 0.1 tape vs floor/window after zone 2 | PASS 0.71 |
| 18 | ESP32 shield 2.4 vs back cover ribs | PASS 2.5 |
| 19 | E-paper 1.05 + 0.15 tape + FPC fold on the key side | PASS 1.5 to the plate, 1.79 to the mask, fold inside the tape gap |
| 20 | Key pads vs pills / mat | PASS 0.11 (mat on the board, as Casio) |
| 21 | LR44 holder over the #1317 | PASS 0.30 (0.20 with tape), checklist 3 |
| 22 | E-paper window vs active area | PASS: active area 48.55 × 23.70 centred 2.55 right of the lens centre, inside the 60.65 × 24.3 opening (3.5 / 8.6 mm margins sideways, 0.3 top/bottom) |
| 23 | Mask template 47.75 × 22.90, 2.55 right, 11.35 / 6.24 / 3.05 bands | PASS, consistent in window_mask.md, template .svg/.dxf, assembly guide |
| 24 | FPC through the 1.0 × 14 slot (x 172.4–173.4, y 86.3–99.3) to J2 (pads x 176.3–179.5, y 85.4–100.2) | PASS: 12.5 wide ribbon in the 14 slot, hugs x 172.55, 0.25 mm bend radius at the slot, 90° into J2 |
| 25 | E-paper ribbon spare | M2 settled: 1.6 nominal, treat as 0 |
| 26 | Camera lens vs 7 mm drill | PASS 0.71 radial, N1 for centring |
| 27 | OV5640 AF minimum focus / page on a desk | N1 (≈ 10 cm; hold 15–30 cm) |
| 28 | Camera tilt / light leak | N1 |
| 29 | Camera ribbon path / length | M3 settled: 58.7 needed, 1.4–3.3 spare |
| 30 | Magnet U-notch vs J3, leg straightening, N orientation, slide case | PASS (N2) |
| 31 | Grinding manual vs CAD/photos: missing grinds | zone 6 added (conditional); zone 4 confirmed "keep"; big ring confirmed not needed (D2 0.74) |
| 32 | Unnecessary grinds | none in the manual; the old "big-ring arc" and "LR44 trim" are gone from the regenerated maps |
| 33 | Zone numbering vs jigs README | M4 fixed |
| 34 | Jig B window = zone 5 (21.5 × 7.95, X 22.5 = KiCad 127.5) | PASS |
| 35 | Jig A windows = zone 1 outline + Ø16 over the camera; A3 gauge 7.2 = plate top 8.4 − floor 1.0 − 0.2 | PASS |
| 36 | Assembly guide step order | PASS: ribbons before battery, magnet meter check before J3, key test before glue, battery last, cover test-fit before screws; added pre-grind checklist, antenna/e-paper checks in step 8, camera centring/focus in step 6, Casio board thickness in step 1 |
| 37 | Tape thicknesses | PASS: camera ≤ 0.1, e-paper ≈ 0.15, battery 0.1 (two strips off the lid), no foam anywhere, consistent in both guides and battery_upgrade §8 |
| 38 | Rev E battery steps | PASS: #1317 26.02 × 19.75 × 3.8 at KiCad (165.6, 70.24), lead end towards J4, 2.6 / 1.0 / 0.8 / 7.0 mm to boss / rib A / lip / J4, lead-tape peel, polarity (Adafruit = J4) |
| 39 | dummy_board.stl | M5 fixed (#1317 block at rev E, antenna in U1's model) |
| 40 | grind_map_front_shell.svg antenna tab + 50 mm line + LR44 note | M5 fixed |
| 41 | `Claude outputs/paper_dry_fit_v14.svg` tab vs module body | PASS: tab 114.4–118.9 × 89.3–104.7 vs body 114.75–135.26 × 89.29–104.71 |
| 42 | Docs consistency after this pass (ribbon spares, zone numbers, antenna side) | fixed where found (list in §5) |

---

## 5. Files changed (none under `hardware/kicad/`; nothing committed)

| File | Change |
|---|---|
| `hardware/verification/07_final_review_fitment.md` | this review |
| `hardware/enclosure/build_fx115es_replica.py` | `INNER_RIB_FULL` (rev F: inner ribs from the rim to the plate); 2D fit-check lists the rib as a board-edge feature |
| `hardware/enclosure/build_final_assembly.py` | grind zone 6 `antenna_relief` (`ANT_RELIEF_K`, cut in `stage_grind`, `GRINDS`/`GRIND_ORDER`/`GRIND_VIEWS`); U1/J1/J2/L1/D2/mask clearance pairs; new stage `fulltable` → `clearance_table_full.md/.json`; antenna renders (`section_antenna*`, `antenna_corner`); `clearances` + `fulltable` in `all` |
| `hardware/enclosure/final_assembly/ai_calculator_final_assembly.f3d` / `.step` | rebuilt rev F |
| `hardware/enclosure/final_assembly/interference.json`, `interference_before_grind.json`, `clearances.json`, `clearance_table_full.md/.json`, `case_fit.json`, `placement.json`, `geometry.json` | regenerated |
| `hardware/enclosure/final_assembly/renders/*` | regenerated (hero, back_cover_off*, exploded*, section_* incl. new `section_antenna`, `section_antenna_wide`, `antenna_corner`, `grind_*` incl. new `grind_antenna_relief_before/after`, case/mask/compare) |
| `hardware/enclosure/final_assembly/grinding_manual.html` | zone 6 article + nav + order list, pre-grind pointer, zone 4 map note, dummy-board text, camera-ribbon spare, checklist items c6b/c6c — **republish 9rCWyw8MeHP2z8FWijnyfL** |
| `hardware/enclosure/final_assembly/assembly_guide.html` | zone 6 row, pre-grind callout, ribbon-spare text settled, camera centring/focus, step-8 antenna + e-paper checks, step-1 board thickness, solar-cell "docs disagree" removed — **republish 8BTC57FZ3JFwiDdiLprwtY** |
| `hardware/bringup_guide.html` | not changed — no republish needed |
| `hardware/enclosure/final_assembly/assembly_report.md` | §3 camera/e-paper ribbon rows corrected; §10 (rev F) added |
| `hardware/enclosure/final_assembly/README.md`, `battery_upgrade.md` | ribbon spare; zone numbers |
| `hardware/enclosure/jigs/README.md` | zone numbers, zone 6 row |
| `hardware/fitcheck/backcover_fx115es.json` | zone names, keep list, antenna tab, zone-3 drill, zone-6 box |
| `hardware/tools/make_fitcheck.py` | rev E battery block, antenna tab, zone labels, keep (green) features, drill marks, LiPo margin maths |
| `hardware/fitcheck/dummy_board.stl`, `grind_map_front_shell.svg`, `grind_map_back_cover.svg`, `clearance_table.md`, `README.md` | regenerated / rewritten |
| `hardware/FINAL_STATUS.md` | §1 items 3, 6, 8, 9, b; §3 grinds line |
| `hardware/HANDOFF.md`, `ORDER_WALKTHROUGH.md`, `ORDER_CHECKLIST.md`, `stage14_verification_fixes.md` | antenna side/thickness/zone 6, ribbon spare |
| `Claude outputs/paper_dry_fit_v14.svg` | **not changed** (verified correct); `fitcheck/grind_map_front_shell.svg` is the maintained equivalent |
