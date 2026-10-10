# Firmware stage-13 work: progress notes

Read this first when resuming. Newest entries at the bottom.

## Setup facts (2026-10-03)
- Real-board firmware = `firmware-prototype/` (env `prototype`). `firmware/` = breadboard tester (own pins). Both build with
  `~/.platformio/penv/Scripts/pio.exe run` from each folder.
- Host tests: no g++/cmake on this PC. MSVC build script lives in the session scratchpad (`tb/build.bat`, with a dirent.h shim
  and a `__builtin_*_overflow` shim, forced-included). Run from repo root: `tests.exe` -> must print `N passed, 0 failed`.
- Baseline: both firmwares build; tests 462/465 -- 3 stale goldens (ready, ready_offline, ai_ready lacked the "Effort:" line).
  Regenerated those 3 goldens -> 465/465.
- Netlist (kicad-cli export from a scratch copy of the schematic) confirms every U1 pin in hardware/pins_final.h.
  TCA_RESET is NOT wired to the MCU (R16 pull-up only).

## Plan
1. pins: hardware/pins_final.h = single source; firmware-prototype/src/pins.h just includes it; compile-time checks.
2. power.cpp: battery curve (core/battery.*, tested), charge state (STAT only while VBUS), Wi-Fi TX cap 11 dBm,
   Wi-Fi only in AI mode, low-battery AI lock-out, deep sleep (state in RTC memory via Device::saveState/restoreState).
3. selftest.cpp: SHIFT+ALPHA held + ON (or serial `selftest`) -> keys, display, camera, Wi-Fi RSSI, battery, JSON.
4. proxy: device token + proxy URL in NVS (no API key ever), server/proxy (FastAPI) with fair-use cap, Haiku-first option.
5. pairing code + subscription state server-side.
6. Extras from coordinator (after core goals): verified answer (recompute `expression` with core), tutor mode
   (steps one at a time), server free daily tier flag.
7. Docs: firmware/FIRMWARE_STAGE13.md, hardware/QUESTIONS_AND_ISSUES.md "## Firmware".

## Log
- 2026-10-04 04:00 resumed. DONE: pins_final.h rewritten as single source (net names, TCA_RESET=-1, kCameraGpios,
  static_asserts); firmware-prototype/src/pins.h = include only; firmware-prototype/tools/check_pins.py (netlist vs header) passes.
  Coordinator hard stop: everything built + FIRMWARE_STAGE13.md by 08:30.
- 04:40 core DONE: core/battery.* (LiPo curve, charge state, robust average); Device::saveState/restoreState;
  Failure::Account/LowBattery + proxy error types in classifyFailure; schema field "check" + verifyAnswer (✓/⚠);
  tutor mode (App Key::Tutor, DKey 1 on AI home); glyphs ✓ ⚠; tests testStage13 (520 pass). Goldens for new
  snapshots must be (re)generated in scratch run dir with UPDATE_GOLDEN and copied into tests/golden.
- 05:20 firmware-prototype rewritten & BUILDS: power.cpp (deep sleep/battery/Wi-Fi cap), keys (held set, prepareSleep,
  wake), camera (N3 power order, lazy start, safe pins, cameraSelfTest), screen (RTC hash), settings (proxy/token, purges
  old key), claude_client (proxy), selftest.cpp, main.cpp (Wi-Fi only in AI mode, low-battery lock, pair/account cmds).
  NEXT: server/proxy (FastAPI), docs, goldens, final rebuild + check_pins.
- 04:30 server/proxy DONE (FastAPI app, policy, solve, db, admin.py, tests 9/9 with stdlib unittest, README,
  Dockerfile, .env.example). fastapi not installed locally (didn't pip install): app.py only py_compile-checked.
  NEXT: goldens regen, docs (FIRMWARE_STAGE13.md, QUESTIONS), README updates, final builds, check_pins, graphify update.
- 04:30 DONE. Both firmwares build with 0 warnings (-Wall -Wextra, own code); tests 520/520; proxy tests 9/9;
  Windows sim compiles (MSVC check); check_pins OK after the other agent's 04:06 mcu-sheet edit.
  Docs: firmware/FIRMWARE_STAGE13.md, server/proxy/README.md, prototype README, QUESTIONS "## Firmware" (16 items).
  Not done: web sim rebuild (no Emscripten), FastAPI app not run locally (deps not installed), nothing run on hardware.
- 09:15 NEW TASK: camera preview (viewfinder). Plan: core/viewfinder.* (Panel 250x122, stretch, Bayer, Laplacian focus, motion, text box, overlay, ghosting->full refresh), camera.cpp cameraPreviewFrame (QVGA JPEG -> 160x120 gray via JPG_SCALE_2X), screen.cpp screenShowPanel, main.cpp preview state (AI Ready screen, 30 s timeout, RIGHT resumes), tests+goldens, sim if feasible, docs.
- viewfinder DONE: core/viewfinder.* (+21 tests, 4 goldens; 541 pass), firmware grabTask + serviceViewfinder
  (camera QVGA JPEG -> 160x120 gray, partial refresh, full every 10 / ghost budget, 30 s timeout, RIGHT resumes,
  = captures full-res, AC/timeout -> cameraSleep), sim F7/F8 fake camera (MSVC-compiled, not run interactively).
  Builds 0 warnings (own code). Docs: FIRMWARE_STAGE13.md "Camera preview". Needs HW tuning: kSharpMin, kMotionMax, fullEvery.
