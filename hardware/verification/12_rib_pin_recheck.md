# 12: Inner rib / pins / hooks re-check against the v14 board (2026-10-06, before ordering)

**Verdict: ORDER AS-IS with grinds 6, 6b and 6c (no board-outline change needed). `hardware/kicad/*` untouched.**

> **Correction 2026-10-10 (top-edge datum only; hits, depths and verdict unchanged).** The "mm from the top" figures in this report were measured from the middle of the top end. The grinding manual now measures zones 6/6b/6c from the shell's own top outer edge **at that wall**, i.e. from the rim's top outer edge right above the side rib, which is about 1 mm lower; every figure moves by −1 mm: zone 6 relief 15–50 → **14–49**, board corner strip 15–26/27 → **14–25/26** (the two short reliefs: 14–26 and 30–49), antenna tab 31–50 / 32–48 → **30–49 / 31–47**, pins 26/35/45 → **25/34/44**, hooks 17/22/56–59 → **16/21/55–58**, zone 6c rib lower ends 56–63 → **55–62**, board widens 59.5–63 → **58.5–62**, modelled rib end 58.4 → **57.4**, diagonals cross the rib line at 61.4/62.1 → **60.4/61.1**, photo rib end 59.5–61.5 → **58.5–60.5**. KiCad y values are unchanged. Source: `enclosure/final_assembly/grinding_manual.html` (zones 6, 6b, 6c).

- **Zone 6 (rib relief, solar-window side)** is now **mandatory**, not conditional, but smaller than the guide said: the rib's top edge is 4.5–5.0 mm below the rim, so only 2.0–2.5 mm of rib height has to come off (file it to **7.0 mm below the rim**, 6.5 is the bare minimum) over 15–50 mm from the top outer edge.
- **Zone 6b (new): snip all three round pins and all three U-shaped wire hooks flush with the inner rib on BOTH side walls** (flush cutters, 1–2 minutes). Every one of them crosses the board plane (depth rod: pins 5.1–6.1, hooks 5.5 below the rim; the board sits 5.5–6.3 below the rim) and, in plane, they sit 0.5 mm from the board edge on the J2 side and 0.7–1.4 mm *inside* the board / antenna tab on the solar side. They held the Casio solar-cell wires; nothing of ours runs along those walls.
- **Zone 6c (new, conditional): if the paper's wide corners still do not lie flat after 6b, file the lower end of each inner rib back 2–3 mm** (the board's wide corners cross the rib line at KiCad y 117.9 / 118.6 = 61.4 / 62.1 mm from the top; the model has the rib ending at 58.4 mm, the photo suggests 60–62 ± 2).
- Optional board insurance for v15 (not needed to order today): a copper-free chamfer of both wide corners, coordinates in §4.

Scope: the shell features Nirav measured today with the depth rod (ruler across the faceplate rim), both side walls, against the v14 Edge.Cuts outline (parsed from `hardware/kicad/ai_calc.kicad_pcb` by script), the ESP32-S3-MINI-1 antenna tab (bare 0.8 mm PCB, KiCad x 114.75–120.4, y 89.29–104.71, Z 6.17–7.0), the ESP32 shield, the e-paper panel (x 120.47–179.47, y 78.39–107.59, key side) and the e-paper FPC through the slot (x 172.4–173.4, y 86.3–99.3). Z from the outside of the back cover: rim 1.5, board component face 7.0 (5.5 below the rim), board key face 7.8 (6.3 below the rim), plate underside 10.5. KiCad low x = solar-window / antenna side (the right-hand side in use); KiCad y 56.5 = the top outer edge of the shell ("mm from the top" = y − 56.5).

---

## 1. What changed in the model (replica rev G)

| Feature | Rev F model | Measured today (below the rim, both sides) | Rev G model (Z) |
|---|---|---|---|
| Thin inner side rib, top edge | from the rim (Z 1.5) to the plate | 4.5–5.0 | `INNER_RIB_Z0` = 6.0 (worst case 4.5) → crosses the board (7.0–7.8) by the full board thickness |
| Round pins, outer two (y 82.9 and 101.6) | top at Z 8.0 (0.2 above the board's key face) | 5.1–5.5 | `PIN_Z0` = 6.6 → cross the board, and reach 0.4 mm into the antenna tab's Z range |
| Round pin, middle (y 91.4) | Z 8.0 | 6.0–6.1 | 7.5 → inside the board thickness (0.3 mm) |
| U-shaped wire hooks (y 73.4–75.8, 78.8–81.2) and the C4 holder (y 111.9–115.5) | Z 8.0 | 5.5 | `HOOK_Z0` = 7.0 → top level with the component face, cross the whole board |

In-plane positions are unchanged from rev F (C4 4.7 wall → rib inner face at x 115.05 / 184.95, 1.0 thick; C14 5.45 → pin tips at x 115.8 / 184.2; hooks at x 115.85 / 184.15 with 0.5 mm return legs; pin Y from photo 2a97120d). Photo cross-check (today's paper template, print verified 49.9/50; scale from the U1 box 15.4 mm = 345 px and the J2 box 14.8 mm = 400 px):

| From the photos | Estimate | Uncertainty | Model |
|---|---|---|---|
| Rib thickness (31dc0015, 96ae606b: the bright top edge) | 0.9–1.1 mm | ± 0.2 | 1.0 |
| Rib inner face from the outer wall (31dc0015: the paper's top corner, x 114.3–114.8, lies ON the rib) | ≥ 4.3 from the outside, i.e. inner face at x ≥ 114.8 | ± 0.3 | 4.7 → x 115.05 |
| Pin protrusion from the rib face (96ae606b: boss + bar) | 1.0–2.0 mm | ± 0.5 (perspective: the pins are 1 mm deeper than the rib edge) | 0.75 |
| J2-side pin tips vs the board edge x 183.65 (96ae606b: the paper edge runs through the pin bosses) | 0.0 ± 0.4 mm (touching) | ± 0.4 | 0.55 clear |
| Solar-side pin tips vs the antenna tab tip x 114.75 (31dc0015: "the tab ends just short of the pins") | the pins reach at least to the tab tip, probably 0.5–1.5 mm past it | ± 0.5 | 1.05 past it |
| C4 holder on the J2 side (96ae606b: the small block beside the paper's kink) | KiCad y 112.3–115.9 | ± 0.5 | 111.9–115.5 |
| Lower end of the thick wall / rib (96ae606b, 3e98c73b) | KiCad y 116–118 (59.5–61.5 from the top) | ± 2 | 114.94 (58.4) |

The photo numbers make the pins **worse** than the model, never better: whatever the exact protrusion, they are at board height and must go.

## 2. Hits and near misses (< 0.2 mm, plus every pin/hook pair) — shell as delivered, v14 board

Signed in-plane gap from the 2D check (`−` = penetration depth); Z overlap = how much of the part's height the feature cuts through. KiCad coordinates.

| # | Shell feature (both tops from the depth rod) | Against | KiCad x, y of the feature | In-plane | Z overlap | Fix |
|---|---|---|---|---|---|---|
| 1 | Solar side, U hook B (arm + leg), top 5.5 below the rim | board corner strip (x 114.63–114.77) | x 114.95–115.85, y 78.84–81.24 | **−1.39 (X)** over 2.4 (Y) | 0.8 (whole board) | 6b snip |
| 2 | Solar side, U hook A | board corner strip | x 114.95–115.85, y 73.44–75.84 | **−1.15 (X)** over 2.4 | 0.8 | 6b snip |
| 3 | Solar side, outer pin (y 101.6), top 5.1 below the rim | ESP32 antenna tab (0.8 mm bare PCB, Z 6.17–7.0) | x 114.95–115.8, y 100.89–102.39 | **−1.05 (X)** over 1.5 | 0.4 (into the tab) + the full board | 6b snip |
| 4 | Solar side, middle pin (y 91.4), top 6.0 below the rim | antenna tab | x 114.95–115.8, y 90.69–92.19 | −1.05 (X) | clear in Z by 0.5 (pin starts 0.5 under the tab's face) — but it is inside the board's Z (7.5–7.8) where the board edge is at x 118.9: 3.1 clear | 6b snip anyway (photo: tips are probably 1 mm longer) |
| 5 | Solar side, outer pin (y 82.9), top 5.1 | board corner strip (edge x 114.28 at y 82.19–82.9) | x 114.95–115.8, y 82.19–83.69 | **−0.71 (X)** over 0.7 (Y) | 0.8 | 6b snip |
| 6 | Solar side, inner rib, top 4.5 below the rim | board corner strip x 114.28–114.77, y 72.0–82.9 | rib x 114.05–115.05, y 72.9–114.9 | **−0.69 (X)** at y 82.1 (−0.28 at y 72) over 10.9 (Y) | 0.8 | **6 rib relief 15–27 mm from the top** |
| 7 | Solar side, inner rib | antenna tab tip x 114.75, y 89.29–104.71 | same rib | **−0.30 (X)** over 15.4 | 0.83 (tab Z 6.17–7.0 vs rib from 6.0) | **6 rib relief 31–50 mm from the top** |
| 8 | J2 side, C4 holder arm, top 5.5 | board edge x 183.65 | x 184.15–185.05, y 111.94–113.94 | +0.50 (model) / 0.0 ± 0.4 (photo) | 0.8 | 6b snip |
| 9 | J2 side, U hook B arm + leg | board edge x 183.65 (board top edge y 78.28) | x 184.15–185.05, y 78.84–81.24 | +0.50 / ~0 (photo) | 0.8 | 6b snip |
| 10 | J2 side, C4 holder return leg | board edge, and the wide-corner diagonal from (183.65, 116) | x 184.15–184.65, y 113.93–115.54 | +0.50 (0.46 at y 115.54) | 0.8 | 6b snip |
| 11 | J2 side, three pins (y 82.9 / 91.4 / 101.6), tops 5.1 / 6.0 / 5.1 | board edge x 183.65 | tips x 184.2 | +0.55 (model) / 0.0 ± 0.4 (photo: paper edge through the bosses) | 0.8 / 0.3 / 0.8 | 6b snip |
| 12 | Both sides, lower end of the inner rib (position ± 2 mm) | board wide corners: (118.9, 114)→(114.21, 119.59) crosses the rib face x 115.05 at **y 118.6**; (183.65, 116)→(185.86, 119.17) crosses x 184.95 at **y 117.9** | rib ends at y 114.94 in the model | +0 to −0.9 depending on where the thick wall really stops | 0.8 | 6c (conditional on the paper dry fit) |
| — | J2 side, U hook A (y 73.4–75.8) | board (top edge y 78.28 on that side) | x 184.15–185.05 | 2.4 clear (Y) | — | snipped with 6b anyway |
| — | Any pin/hook/rib | ESP32 shield (x ≥ 121.2), e-paper panel (x 120.47–179.47, key side Z 7.95–9.0), e-paper FPC (x 172.4–173.4) | — | ≥ 4.7 / ≥ 5.4 / ≥ 11.5 | — | none |
| — | Rib remnant above the relief (Z 8.0 if filed to 6.5 below the rim) | board corner strip key face Z 7.8 | — | — | 0.2 clear in Z | file to **7.0 below the rim** → 0.7 clear |

Nothing else on the board is within 0.5 mm of a side-wall feature: the board is 118.9–183.65 wide beside the display (rib faces 115.05 / 184.95 → 3.85 / 1.30 clear), the panel and the FPC are further in.

3D check (Fusion, rev G shell, `build_final_assembly.py new shell grind pcb parts check` + `check grind=off`): see §5 (filled in from `interference_before_grind.json` / `interference.json` when the run finished; the 2D numbers above are the authoritative in-plane figures, the 3D run confirms the Z side).

## 3. Fixes (all shell, all quick)

**Zone 6 — rib relief, solar-window side (now mandatory).** Front shell face down, top end away: the antenna side is on your left. Mark 15 and 50 mm from the top outer edge on that wall *(2026-10-10 datum: mark 14 and 49 mm from the rim's top outer edge right above the side rib; see the correction note at the top)*. File the thin inner rib (1 mm, the one the pins stick out of) down from its top edge until it is **7.0 mm below the rim** (ruler across the rim, depth rod; 6.5 is the minimum, the board's key face is at 6.3). That is only 2.0–2.5 mm of rib height to remove; the rib stays above that, holding the plate. Do not touch the outer skin or the channel floor. Two short reliefs (14.5–27 for the board corner, 31–50 for the antenna tab) are fine too *(2026-10-10 datum: 14–26 and 30–49)*.

**Zone 6b — pins and hooks, both walls.** With flush cutters snip the 3 round pins and the 3 U-hooks (incl. the lowest, wider wire holder) off each inner rib, flush with the rib; a stroke with a needle file takes the stubs down. Check with a fingernail along the rib: nothing proud. Why both walls: J2 side pins/holder are 0–0.5 mm from the board edge at board height; solar side everything is inside the board / tab. There is no wire to hold: the LiPo leads plug into J4 next to the battery.

**Zone 6c — rib lower ends (only if the paper's wide corners still sit up).** The board widens from 64.75 to its full width between 59.5 and 63 mm from the top (the diagonals; *2026-10-10 datum: 58.5–62 mm from the top outer edge at that wall*). If, after 6b, a wide corner of the paper (or the dummy board) still rides on the end of the thick wall, file that rib's lower end back until the corner drops: at most 2–3 mm of rib, down to 7 mm below the rim, on whichever side needs it.

**Order:** 1 → 2 → (3 held) → 5 → 5b → 6 → 6b → 6c-if-needed, then the paper dry fit with the yellow tab (both wide corners flat, tab flat, every post in its hole), then the dummy board.

## 4. Board-outline alternative (NOT required; recorded for v15 or if Nirav prefers certainty on the wide corners)

Copper check by script (segments, vias, pads with footprint rotation, courtyards; the GND pours refill after an outline change and are ignored): no track, via, pad or courtyard within 0.5 mm of either new edge below.

- J2-side wide corner: replace the diagonal (183.65, 116.0)→(185.858, 119.17) by (183.65, 116.0)→(183.65, 118.6)→(185.387, 120.92), dropping the vertex (185.858, 119.17). Nearest copper: pours only (0.17).
- Solar-side wide corner: replace (118.9, 114.0)→(114.212, 119.588) by (118.9, 114.0)→(118.9, 116.6)→(114.613, 120.92), dropping (114.212, 119.588). Nearest copper: pours only (0.10–0.13).
- Not recommended: moving the J2-side edge from x 183.65 to 183.2 (the pins) — the F.Cu pour reaches the edge there and the snip is simpler. Not needed: the solar-side corner strip (x 114.28–114.77, y 72–82.9) carries a via at x 116.0, y 76.0 (0.5 mm from x 115.2), so narrowing that strip to x 118.9 (the v15 note in HANDOFF) must keep that via.

These corner chamfers only help against zone 6c's uncertainty (the rib end position); they do nothing for the pins, hooks or the rib relief. Order as-is.

## 5. Fusion rev G run (3D, closed case, every body pair)

`python build_final_assembly.py new shell grind pcb parts check` then `check grind=off` (17:33–17:40): replica rev G shell (rib from Z 6.0, pins from 6.6 / 7.5, hooks from 7.0), board STEP v14, all off-board parts (camera, e-paper panel + FPC, LiPo + leads + plug, magnet), 242 bodies, 531 touching pairs, 0 errors. (The `mask`, `export` and `renders` stages were not run: `ai_calculator_final_assembly.f3d/.step` and the pictures on disk are still rev F.)

**Shell as delivered (grinds suppressed): 40 overlaps.** The ones that are the subject of this file (the rest are the known zone 1/2/3/5 items and the intended contacts, same as 07 §2.3):

| Part A | Part B | Depth | mm³ | KiCad x, y (centre) | Z | Extent | What |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Front shell | PCB/U1 (incl. antenna tab) | 0.81 | 4.09 | 125.0, 97.0 | 6.11–6.92 | tab vs rib + the y 101.6 pin (0.35 mm³ more than rev F's rib-only 3.74) | hits 3, 7 |
| Front shell | PCB/board | 0.71 | 5.53 | 115.1, 77.9 | 7.00–7.71 | x 114.3–115.85, y 72.9–82.9: rib + both hooks + the y 82.9 pin in one lump (rev F rib-only: 3.10 mm³) | hits 1, 2, 5, 6 |

No overlap on the J2 side (pins/hooks 0.5–0.55 from the board edge in the model; see §1 for why they are snipped anyway) and none with the e-paper panel, the FPC or the shield. The rib's lower end (model y 114.94) is clear of the wide corners in the model (hit 12 is the position uncertainty, not a model overlap).

**After zone 6 (rib to Z 8.0 between y 71.5–106.5) + zone 6b (all pins and hooks cut) + the zone 1/2/3/5 grinds: 8 overlaps, all intended** (4 magnet legs in J3, 2 FPC tip in J2, 2 LiPo wires in their plug) — identical to 07 §2 minus the navy-tick artefacts, which disappear now that the rib no longer reaches the rim. Nothing of the front shell touches the board, the tab, the panel or the FPC. Closest remaining front-shell point to the board corner strip is the rib remnant 0.2 mm above the key face (Z 8.0 vs 7.8), hence the instruction to file to 7.0 below the rim (Z 8.5) rather than the model's 6.5.

## 6. Files changed (nothing under `hardware/kicad/`, nothing committed)

| File | Change |
|---|---|
| `hardware/enclosure/build_fx115es_replica.py` | rev G: `INNER_RIB_Z0`, `PIN_Z0`, `HOOK_Z0`; rib / pins / hooks extruded from their measured depths |
| `hardware/enclosure/build_final_assembly.py` | grind `pins_hooks` (zone 6b) added to `GRINDS` / `GRIND_ORDER`, cut in `stage_grind` |
| `hardware/enclosure/final_assembly/grinding_manual.html` | zone 6 rewritten (mandatory, 7 mm, 2–2.5 mm of rib), zone 6b/6c added, checklist c6b |
| `hardware/fitcheck/grind_map_front_shell.svg`, `backcover_fx115es.json` | pins + hooks marked "snip" on both walls, zone 6 note updated |
| `hardware/verification/07_final_review_fitment.md` | pointer to this file (S1, checklist 1 and 9) |
