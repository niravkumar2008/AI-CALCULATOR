# Autonomous work log

Running log for the autonomous task list of 2026-10-10 (tasks: Goldenmorning fit, magnet alternates, open items, v15 firmware).

## 2026-10-10

### Task 1: Goldenmorning T190X7-C30-01H fit check: DONE
- **Result:** fits with conditions. Details in `hardware/enclosure/final_assembly_v15_lcd/REPORT_goldenmorning.md`.
  - **Free space under the plate:** 0.67 mm (2.03) / 0.52 mm (2.18).
  - **Tail bow ↔ rib B at 37.1:** 0.30 / 0.375 mm.
  - **Interference:** 0 errors, only the intended lumps.
  - **Clearance table:** the one known FAIL (H6, 0.10 mm).
- **Open:** confirm the thickness and the tail datum with Goldenmorning. That needs a supplier contact, so it waits for Nirav.
- **No datasheet attached in the request.** The run used the stated -01H numbers plus the related T190X7-C30-01 V1.0 datasheet (thickness placement, tail exit).

### Task 4: v15 firmware build and warnings: DONE (no code change needed)
- **`pio run -e v15lcd`:** SUCCESS, **0 warnings**. RAM 28.8 % (94,372 B), flash 68.0 % (1,336,201 B of the 1.9 MB slot).
- **Stricter check:** `-Wextra -Wsign-compare -Wunused-parameter -Wshadow=local` on `firmware-v15-lcd/src` and `core/` gave 0 warnings in our code. The only warnings are inside downloaded libraries (LovyanGFX unused parameters; ESP32-OV5640-AF has an always-false `uint8_t < 0` error check), so those were left alone.
- **v14 firmware:** `firmware-prototype` still builds: SUCCESS, 0 warnings, RAM 30.7 %, flash 63.4 %.
- **Tests:** core 548 passed / 0 failed; proxy 15 passed.
- **New:** `tests/msvc/build_and_run.bat` plus `tests/msvc/shim/` builds and runs the core tests on Windows with the MSVC Build Tools (no GCC). Verified: 548 passed, 0 failed.

### Task 2: magnet connector alternates: DONE
- **What:** Yiwei MG04254FRA1S1N (the same part as Adafruit #5358) and the Yiwei MG0425-UB-60-4P cable (the equivalent of #5412; it reads GND, D+, D−, V+ from its N end, like #5412) are recorded as alternates.
- **Docs:** v15 BOM notes (`stage15_lcd.md`), FINAL_STATUS §2/§3 (additions only), both order walkthroughs, the arrival checklist, HANDOFF, the QA plan, unit economics, and the parts lists of the four guides.
- **Spreadsheets:** notes in the shopping list; notes and two supplier rows in the bulk-sourcing sheet. Totals re-checked in Python and unchanged (A/B/C $662.33 / $571.75 / $663.97; LCD per-unit $91.92 / $42.22 / $31.69).
- **Unchanged:** J3 = 1 VBUS, 2 D−, 3 D+, 4 GND, with the piece's N end at pin 1. The meter check before gluing is kept.
- **Search:** no live "turn the magnet piece over" advice anywhere; only warnings against it.
- **Open:** prices not checked; no supplier contacted.

### Task 3: open items: DONE (what can be done without hands or money)
- **v15 re-check (report only, following review 14):** KiCad 10.0.6 on a scratch copy gives ERC 0, DRC 0/0/0 with schematic parity, and copper-to-hole 0 violations. The board and fab files are unchanged since review 14.
- **Closed with evidence:**
  - E7 / stiffener (v14 ordered; v15 has no e-paper).
  - U1 stock for v14 (ordered; the v15 stock check is in its walkthrough).
  - The firmware to-dos from the reviews (all in the source; file and line listed).
  - The stale "blocks order" housekeeping item.
- **Marked "needs Nirav":**
  - V1–V6 bench checks.
  - The paper dry fit (v15 shares the holes).
  - Camera ribbon length.
  - Magnet meter check.
  - J4 polarity and E6.
  - Decisions: battery plan, R5 → 15 k (needs new Gerbers), touch panel.
  - Pricing.
  - Proxy deploy.
  - Donor edition and photos.
- **Noticed, not changed (protected file):** FINAL_STATUS "Status" (line 38) and §6, and HANDOFF.md line 10, still say v14 is "not ordered yet", while "Current plan" says it was ordered 10/8.

### Extra: DCXYX OV5640-AF camera (DCXYX-LZTKQJ-5M-339) on v15: FITS with a tuck
- **Ribbon:** 70.0 mm beyond the module (Seeed: 62), so about 11.3 mm has to be stored.
- **Fit:** fits with one S-fold (r 1.2 mm) in the empty bay at KiCad y 108–116, x 147–153 (6 mm free height). Closest gaps: floor 0.44, small-ring wall 0.53, board 0.54. Hold the fold with Kapton.
- **Checks:** interference 0 errors (only intended contacts). Lens ↔ window 0.71 (0.58 if the camera is 5.6 mm tall).
- **Fallback:** move the camera and its 7 mm hole 8 mm away from J1, to (150, 87.1). That is a shell drill change only; the maximum move is about 8.9 mm.
- **No change** to the board or the baseline outputs.
- **Data:** `hardware/enclosure/final_assembly_v15_lcd/variant_camera_dcxyx/`.
- **Still to confirm on the real part:** the height (the drawing is unreadable; checked at 5.4 and 5.6) and the pin-1 beep test.

### Docs consistency 2026-10-10
- **Datums:** the front-shell zone 6/6b/6c figures are now measured from the rim's top edge, right above the side rib. That shifts them −1 mm:

  | Item | Now |
  | --- | --- |
  | Relief | 14–49 |
  | Board corner strip | 14–25/26 |
  | Antenna tab | 30–49 / 31–47 |
  | Pins | 25/34/44 |
  | Hooks | 16/21/55–58 |
  | Zone 6c (rib lower ends) | 55–62 |
  | Board widens | 58.5–62 |
  | Rib end | 57.4 |
  | Diagonals cross the rib | 60.4/61.1 |

  - Zone 1 frame (6.3–19.3) and the camera hole (38.0) are measured from the back cover's own top outer edge.
  - Updated: FINAL_STATUS (numbers only), ARRIVAL_CHECKLIST, ORDER_WALKTHROUGH, QUESTIONS_AND_ISSUES, the jigs README and assembly_report. Verification 07 and 12 got dated correction notes.
  - `slide_case_mods.md` keeps 38.3, because the case datum is the whole outline top; a note explains this.
- **Battery tape:** strip 2 now starts 0.5 mm above the cell's bottom edge, which leaves about 0.7 mm below the lid opening (`battery_upgrade.md` §8).
- **Key mat:** KNOCKOFF_SHELL_PLAN now says the contact dots are on the board side.
- **TW302030:** added to the shopping list, the Alibaba bulk sheet, KNOCKOFF_SHELL_PLAN and UNIT_ECONOMICS. The formula check found 549 formula cells, 0 changed.
- **Open:**
  - FINAL_STATUS §1a step 6 and §3 step 1 still call zone 6 conditional; the grinding manual and verification 12 say it is settled.
  - The v15 guide says the TW302030 is "150 mAh"; it is 140.
  - The grind-map SVG and the photo labels still say 15–50.
  - The guide pictures and text still draw strip 2 at 1 mm. Fix in progress.
