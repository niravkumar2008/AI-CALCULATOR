# Goldenmorning T190X7-C30-01H: does it fit the v15-LCD build?

**Verdict: it fits, with conditions.** No new grind, no board change, and the 0.1 mm tape can stay. A thicker panel actually gives the tail's bow *more* room above rib B than the baseline ER-TFT019-1 had. Conditions:
1. **Thickness:** confirm 2.03 mm vs 1.43 mm with Goldenmorning. Both fit.
2. **Tail length:** measure it from the TFT glass edge to the tip. The bow keeps ≥ 0.1 mm above rib B up to 37.5 mm (2.03 panel) or 37.6 mm (2.18 panel), and starts touching at ≈ 37.7 / 37.85 mm.
3. **Tail datum:** on the related -01 drawing, the 36.6 ± 0.5 is measured from the module (frame) edge. That is ≈ 0.6 mm beyond the glass, so measured from the glass the tail is ≈ 37.2 nominal (36.7–37.7). At 37.7 that touches (2.03) or leaves 0.075 mm (2.18). Ask, or measure the sample.

Run on 2026-10-09 with `build_final_assembly_v15.py out=variant_goldenmorning lcd_dt=0.60 | 0.75`. The new options (`out=`, `lcd_dt=`, `lcd_tape=`) default to the baseline, so the ER-TFT019-1 run is unchanged: its `placement.json` and `geometry.json` are byte-identical to before. Data: `variant_goldenmorning/results.json`, plus `t203/` and `t218/`, each with checks and 5 section renders. No board files changed.

## What was modelled
- **Same as the ER-TFT019-1:**
  - outline 25.80 × 49.72
  - active area 22.7 × 42.72, same placement
  - J5 pinout and finger-1 end
  - tail 15.5 × 0.3, 4.5 mm stiffener, 0.5 mm off-centre towards finger 1
- **Extra thickness:** +0.60 (2.03 total) or +0.75 (2.18 worst case), put in the backlight/frame. The glass stack, ledge and tail exit all move up by that amount.
- **Source:** this matches the related Goldenmorning T190X7-C30-01 V1.0 datasheet (p.5: 0.15 mm stainless frame on the backlight side, tail leaving at TFT-glass level under the CF glass).

## 1. Free space

| | ER-TFT019-1 (1.43) | 2.03 | 2.18 |
| --- | --- | --- | --- |
| Panel top Z | 9.33 | 9.93 | 10.08 |
| **Panel ↔ front-plate underside (Z 10.6)** | 1.27 | **0.67** | **0.52** |
| Panel ↔ mask | 1.48 | 0.88 | 0.73 |
| Panel ↔ lens | 1.57 | 0.97 | 0.82 |
| Tail on the ledge ↔ plate | 1.42 | 0.83 | 0.68 |
| Tail on the ledge ↔ mask | 1.61 | 1.01 | 0.86 |

The 0.1 mm tape is still fine; a 0.15 mm tape would also work.

## 2. Tail bow ↔ rib B (gap in mm)

| Tail (glass edge → tip) | ER-TFT019-1 | 2.03 | 2.18 |
| --- | --- | --- | --- |
| 36.1 | — | 0.80 | 0.875 |
| 36.6 | 0.25 | 0.55 | 0.625 |
| **37.1** | ≈ 0 (touch from ≈ 37.1) | **0.30** | **0.375** |
| 37.4 | — | 0.15 | 0.225 |
| 37.6 | — | 0.05 | 0.125 |
| 37.8 | — | touch (0.74 mm³) | 0.025 |
| 38.0 | — | touch (2.27 mm³) | touch (1.12 mm³) |

**Why it improves:** the tail leaves the ledge 0.60 / 0.75 mm higher, which raises the bow by about half that.

**Unchanged:**
- **Bow corners:** r 1.2 mm; top bend r 1.0 mm.
- **Slot:** walls 0.35 mm each side; ends 2.75 / 1.75 mm.
- **J5:** tip 2.0 mm inside.
- **Q4:** tail ↔ Q4 1.32 mm.
- **Shortest route:** 30.8 / 30.95 mm (baseline 30.2).

## 3. Interference and other clearances (closed case, after grinds)
- **Interference:** 194 bodies, 410 pairs, 68 lumps, 0 errors. These are the same intended lumps as the baseline: tail tip in J5 (62), magnet legs in J3 (4), LiPo leads in their plug (2).
- **Full clearance table:** 527 rows, 1 FAIL. That is the known board ↔ post H6 at 0.10 mm, as on v14.
- **Rows that change:**
  - Active area ↔ window frame: 1.43 → 0.95 / 0.85
  - CF glass ↔ plate: 1.40 → 0.80 / 0.65
  - Solar dummy: 3.33 → 3.12 / 3.08
  - Bow ↔ foot 2: grows to 2.17
- **Unchanged:** U1 ↔ panel 0.97, the key mat, and everything away from the LCD.

## 4. Ask Goldenmorning
1. The real total thickness of the -01H (2.03 ± 0.15 or 1.43). Is the frame plastic or metal?
2. Where the extra thickness sits, and the tail exit height (bonded on the TFT-glass ledge under the CF glass?).
3. Where the 36.6 ± 0.5 tail length is measured from (frame edge or glass edge), plus the tail's lateral offset and tolerance.
4. Confirm the outline 25.80 × 49.72 (including the backlight) and the active-area position 22.7 × 42.72.
