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
