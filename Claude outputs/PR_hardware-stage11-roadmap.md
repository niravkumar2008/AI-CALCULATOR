Title: Hardware under version control, stage-11 PCB (in progress) and launch roadmap

## What changed
- **`hardware/` is committed for the first time**: KiCad 10 project, fab outputs (gerbers, BOM, CPL, STEP), fit-check files, Fusion enclosure scripts and exports, stage docs (2–10), tools.
- **Stage 11 PCB, work in progress.** The e-paper/booster block moved up 3.57 mm to match the measured LCD window (calipers C12 = 60.65 × 24.3 mm, C13 = 24.0 mm). **The re-route isn't finished in this commit, so DRC isn't clean yet.** Follow-up commits add `hardware/stage11_epaper_calipers.md` and regenerated `fab/`.
- **Independent design review**: `hardware/REVIEW_independent_2026-10-03.md`. No wiring errors found. Should-fix items: R20 back-feed into VBUS, swap AP2112K → RT9080 for about 4× longer standby, check the D7 TVS orientation.
- **Launch roadmap**: `Claude outputs/LAUNCH_ROADMAP.md`, chart data JSON and the HTML page (https://claude.ai/artifact/K5hMpGzzf4gS9b6nGKSoM9): Monday order list, day-by-day plan to 10/17, cost per unit from 2 to 10k, regulatory steps.
- Refreshed graphify output, a graph-refresh hook, and the older stage-8 PCB extract in `Claude outputs/ai_calc_pcb_8/`.

## Why
Until now the hardware lived only on one PC. Committing it makes it reviewable and lets cloud sessions work on it.

## Reviewer notes
- **Don't order boards from this commit.** Wait for the stage-11 follow-up (DRC 0/0/0, regenerated `fab/`) and the remaining measurements (C14, C15, D-depths, E6/E7).
- Large binaries: STEP/STL/F3D files of 8–15 MB each, all under GitHub's limits.
- No API keys are included (`api_key.txt` stays git-ignored).

🤖 Generated with [Claude Code](https://claude.com/claude-code)
