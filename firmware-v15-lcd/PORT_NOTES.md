# Port notes: v14 e-paper firmware → v15-LCD (2026-10-08)

Rules followed: `firmware-prototype/` (v14) and `core/` were **not modified** (`pio run` in
`firmware-prototype` still gives RAM 30.7 %, flash 63.4 %, byte-identical numbers to before; core tests
not re-run because core/ is untouched). Nothing committed. No API key anywhere. No eFuse touched.

## Build result

| Project | RAM | Flash (1.9 MB OTA slot) |
| --- | --- | --- |
| `firmware-v15-lcd` (`env:v15lcd`) | **28.8 %** (94,396 B) | **68.3 %** (1,343,269 B) (after the 2026-10-10 UI fixes; was 94,372 B / 1,336,201 B) |
| `firmware-prototype` (v14, unchanged) | 30.7 % (100,440 B) | 63.4 % (1,245,549 B) |

LovyanGFX 1.2.32 adds ~90 KB of flash over GxEPD2; internal RAM went *down* because the 320x170 canvas
lives in PSRAM and the e-paper library's buffers are gone. No compiler warnings from `src/`.

## What changed, file by file

- **`platformio.ini`**: `env:v15lcd`, `-DBOARD_V15_LCD`, LovyanGFX instead of GxEPD2, four panel flags
  (`LCD_ROTATION`, `LCD_INVERT`, `LCD_RGB_ORDER`, `PREVIEW_SWAP_BYTES`) for the bench.
- **`pins.h`**: includes `hardware/kicad_v15_lcd/pins_v15_lcd.h` (the port was written against the
  14:14 roles RST 5, CS 6, SCK 8, DC 41, MOSI 42; review 13 swapped them to the final **MOSI 5, DC 6,
  SCK 8, CS 41, RST 42**, PWR_N 33, BL 4, KEYPAD_INT 3. The sources use only `PIN_*` constants, so they
  follow the header automatically: checked in review 14, no bare GPIO numbers).
- **`lcd.*` (new)**: LovyanGFX `Panel_ST7789` (170x320 native, `memory_width 240`, `offset_x 35`,
  rotation 1 → 320x170, `invert` on, `bus_shared false`, SPI2 40 MHz DMA). Power-up: all LCD lines low
  → LCD_PWR_N low → 10 ms → `init()` (reset pulse, SLPOUT, COLMOD, INVON, DISPON) → black → backlight.
  Power-down: backlight 0 → `sleep()` (SLPIN/DISPOFF) → lines low → LCD_PWR_N high. Panel sleep /
  wake for the idle policy. Backlight: LEDC channel 2 (timer 1; the camera's XCLK owns timer 0 /
  channel 0), 5 kHz, 8-bit, squared duty curve, NVS namespace `lcd` (`bright`, `dim_s`, `off_s`,
  `sleep_s`).
- **`fbtext.*` (new)**: decodes the core's 1-bit `Framebuffer` into 7x25 text cells by matching each
  5x7 cell against `core/font` (plus inverse for inverted cells, cursor bar stripped, underline from
  the gap row). This is the display abstraction: `core/` keeps rendering its e-paper picture and knows
  nothing about the LCD.
- **`glyphdraw.h` (new)**: the core font scaled on an LGFX sprite (2x for menus, 3x expression, 4x result).
- **`ui.*` (new)**: status bar (SHIFT amber, ALPHA red, D/R/G, EXAM = red bar, AI, Wi-Fi icon, battery
  icon + %), calculator view (expression re-flowed to 18 columns with a blinking cursor computed from
  `Device::cursor()` and `tokText`, result right-aligned green, scan note / UNCLEAR bar), every other
  view re-typeset from the decoded rows (selected menu items on an accent block, headers underlined),
  soft-key bar per view, off screen with "Charging N %", line screens (setup by phone, update, self-test)
  and a progress bar for OTA. Redraws only when the picture, status or cursor phase changes (FNV hash).
- **`vf_lcd.*` (new)**: viewfinder overlay drawn *into the camera's frame buffer* through an
  `LGFX_Sprite::setBuffer` view (zero copies), then `pushImage` of rows 35..204. Corner brackets, dashed
  "photo continues" marks, centre cross, amber box around writing found by `core/viewfinder`, focus state,
  mode, battery, key hints, fps / time-out countdown. `greyThumb()` makes the 80x60 grey copy that
  `Viewfinder::feed()` analyses (sharpness, motion, text box, 30 s time-out: all still core/).
- **`camera.*`**: new driver mode `Rgb565` (QVGA, 2 fbs in PSRAM, GRAB_LATEST, colour) next to the
  v14 `Jpeg` scan mode; `bringUp()` / `switchMode()` (deinit + init + AF firmware reload); `applyAll`
  skips `framesize` / `grayscale` in preview mode; `cameraCapture`, `cameraFrame`, `cameraSelfTest`,
  `cameraAutoTune` call `ensureJpeg()` first; `cameraHoldEstimateMs()` adds 800 ms when a switch is
  due; `cameraRefocus()` writes the OV5640 AF command (0x3023 = 1, 0x3022 = 0x03 single-shot);
  `cameraFocusState()` from the AF status register. `cameraPreviewFrame()` (grey JPEG decode) is kept
  but unused.
- **`power.*`**: `powerBegin` releases the LCD pins and parks them (lines low, PWR_N high) before
  `lcd::begin`; `powerDeepSleep` holds the five SPI lines low, isolates IO4 (R22 → off) and floats
  IO33 (R21 → off). EPD hold levels removed.
- **`main.cpp`**: grab task hands over the driver's `camera_fb_t*` (no copy) and waits for the loop to
  return it; `serviceViewfinder` feeds the grey thumb to `Viewfinder`, builds the overlay, pushes, logs
  fps; `press()` adds ▶ = refocus, SHIFT ▲▼ = brightness, "key only wakes the screen" when the light
  is off; idle policy serviced every loop (`keepOn` during viewfinder / scan / setup / update);
  `lcd::powerOff()` before deep sleep; `status` prints a Screen line; `bright` / `screen` commands; OTA
  shows a progress bar.
- **`selftest.cpp`**: LCD step (colour bars, grey ramp, border, id text; backlight sweep 0→100 %);
  JSON `lcd_ok`, `lcd_push_ms`, `backlight_ok`, `backlight_pct` + the old `display_ok` /
  `display_refresh_ms` as aliases.
- **`settings.*`**: `SerialCmd::Bright`, `SerialCmd::Screen`, help text.
- Removed from this project: `screen.cpp/.h` (GxEPD2), all partial / full refresh, ghosting budget, BUSY
  polling, hibernate, RTC-kept "shown hash" (the LCD is redrawn on every wake anyway).

## Preview frame-rate estimate

- Sensor: OV5640 at QVGA RGB565 with the esp32-camera driver's sub-VGA PLL setting: ~20-25 fps into
  PSRAM by DMA, no CPU.
- Per frame on the loop (core 1): 80x60 grey thumb (on the grab task, ~1 ms), `Viewfinder::feed`
  (~3-5 ms), overlay (~2 ms), `pushImage` 320x170x2 = 108.8 KB at 40 MHz ≈ 22 ms plus the PSRAM
  read through LovyanGFX's DMA bounce buffer (~5 ms).
- Expected: **12-18 fps**, camera- and SPI-bound; the fps is shown in the bottom band and logged every
  10 s. If it is lower, the first suspects are the PSRAM → SPI path (LovyanGFX chunk size) and the grab
  task's 5 ms poll loop (replace with a semaphore).
- Mode switch on `=` (RGB565 → JPEG, AF firmware reload): estimated 0.5-1 s, added to the hold-still
  countdown as 800 ms; measure and adjust `kModeSwitchMs` in `camera.cpp`.
- PSRAM budget in preview: 2 x 153.6 KB camera buffers + 109 KB canvas ≈ 416 KB; the scan frees the
  preview buffers before allocating its 630 KB QXGA buffer (as v14). Free PSRAM is in `status`.

## Needs the real panel to verify (bench checklist)

1. **Orientation / offset**: `LCD_ROTATION` 1 vs 3 (ribbon side), `offset_x 35` (a 170-px panel; if a
   35-px strip of noise shows at one edge, try `offset_x 0` with `memory_width 170`, or 35 on the other
   side via `offset_rotation`).
2. **Colours** (the ER-TFT019-1 is an ST7789P3; same command set, column offset 35): `LCD_INVERT` (IPS: usually needed; a washed-out negative picture = flip it),
   `LCD_RGB_ORDER` (red/blue swapped on the colour bars = flip it).
3. **Preview byte order**: if the viewfinder shows wrong / garish colours while the bars are right, set
   `PREVIEW_SWAP_BYTES=1` (the sprite view assumes the camera emits the high byte first).
4. **40 MHz through the GPIO matrix**: if the picture tears or shows noise, drop `LCD_SPI_HZ` to 26.6 or
   20 MHz in `pins_v15_lcd.h` (full frame then 33 / 44 ms).
5. **Panel at 3.3 V** (the top of the ER-TFT019-1's 2.4-3.3 V range): check the image quality and
   that the panel's IDD is ≤ 20 mA. **Sleep current** with the panel: ≤ 40 µA target; the LCD block should add < 2 µA (Q4 off). If it is
   higher, check that IO33 is really floating (R21) and the five SPI lines read 0 V in sleep.
6. **Backlight** (panel BuyDisplay ER-TFT019-1, R23 = 15 Ω since review 14): measure V(R23) / 15 at 100 %,
   expect ≈ 26 mA (16-37 mA; the panel is rated 60 mA); visible brightness at 60 % default, flicker at 10 % (raise the minimum duty in
   `dutyFor` if the LEDs flicker), the self-test sweep.
7. **Panel wake**: 120 ms after SLPOUT may show a white flash on some ST7789s; if so, keep the backlight
   at 0 until the first frame after `wake()` is pushed.
8. **Mode switch**: measure `[cam] scan (JPEG) mode in N ms` and `preview ... in N ms` in the log and set
   `kModeSwitchMs`; confirm the AF firmware reloads (log "autofocus on") and that `cameraRefocus()`
   (0x3022 = 0x03) really moves the lens; if single-shot does not hold, use 0x04 (continuous) only.
9. **Preview fps and brown-out**: the viewfinder at full backlight with Wi-Fi joining is the new worst
   case (≤ 426 mA worst case on 3V3: Wi-Fi 355 + panel ≤ 20 + backlight ≤ 51; review 14). Watch for `ESP_RST_BROWNOUT` in the boot message; if seen,
   set the backlight to 25 % while `WiFi.status()` is not connected (one line in `serviceIdle`'s caller).
10. **UI legibility** at 0.13 mm pixels: 2x menu text (10x14 px) may be small; `kTextScale` in `ui.cpp`
    is the single knob (3x fits 18 columns; the core wraps at 25, so rows would need re-flowing).
11. The decoder (`fbtext`) must recognise every glyph the core draws; an unrecognised cell shows as raw
    pixels at 2x (still correct, just small). Run through every menu once and look for them. (Since
    2026-10-10 it also knows core's replacement glyphs and icons; see "UI fixes" below. The PC
    screenshot harness `tools/ui_sim_v15/` shows every screen without the board.)
12. OTA from a v14 image to v15 is **not** safe (different hardware): keep separate version strings and
    proxy images per board (done: `kFirmwareVersion` = `v15lcd-2026.10.10`; the proxy stores one image per
    family and refuses cross-family downloads).

## UI fixes from the PC screenshots (2026-10-10)

Found with `tools/ui_sim_v15/make_screens.py` (real `ui.cpp` / `fbtext.cpp` / `vf_lcd.cpp` /
`selftest.cpp` on PC LovyanGFX). Before/after: `hardware/renders_ui_v15/sheet_before_after.png`
(old images kept in `hardware/renders_ui_v15/before_fixes/`). `core/` and `firmware-prototype/`
unchanged; core tests 548 passed; `pio run -e v15lcd` no warnings from `src/`.

- **Replacement glyphs** (`fbtext.cpp` `drawnGlyphs()` / `match()`): core/font.cpp swaps some glyphs
  for hand-drawn ones (its private `kExtra`: √ ≈ ⌟ ▲ ▼ ◀ ▶ ✓ ⚠, padlock U+E000, Wi-Fi U+E001).
  The decoder now reads them back through core's public `findGlyph()` / `hasGlyph()` and matches them
  first. √ no longer prints `?` in the expression; the ▲▼ scroll arrows (AI pages, calculator history)
  appear; ✓ is drawn green and ⚠ amber (`ui.cpp` `symbolColor()`).
- **Real √** (`glyphdraw.h` `radical()`): at 2x and up, √ is drawn as an anti-aliased radical whose bar
  runs into the next cell.
- **HOLD STILL countdown** (`fbtext::decodeScaled()`, `ui.cpp` `drawGenericRows()`): core draws it at
  2x with `drawTextPx`. A row with raw cells is tried as 2x text (every font pixel must be a solid 2x2
  block, so normal text never passes); found → the number at 5x and the unit at 3x, amber.
- **Status bar** (`ui.cpp` `rightCluster()`, `statusFromGrid()`): laid out right to left from measured
  widths (battery icon, % with the tight advance, Wi-Fi). "AI" and the exam padlock + clock are packed
  against it. The exam clock comes from `Device::examElapsedMs()` (its last digit runs into the core's
  battery outline in the 1-bit picture, so it was cut to "0:0").
- **Soft keys** (`ui.cpp` `softKeys()`): measured; tight advance if needed; a key that would not fit
  whole is left out. Calculator: `MODE Menu`, `MODE 4 AI`, `SHIFT 2nd`. AI ready: `= Scan`, `▶ Camera`,
  `AC Back` (Effort ▲▼ and 1:Tutor are on the page itself).
- **Long lines** (`glyphdraw.h` `textFit()`): `showLines` / `showProgress` / status-bar titles use the
  tight advance when a line is too long, then cut with "..". "Downloading v15lcd-2026.10.20" now fits.
- **Self-test caption** (`selftest.cpp` `lcdPattern()`): "SELF-TEST: bars, ramp" left, the device id
  right-aligned, measured.
- **Viewfinder bands** (`vf_lcd.cpp` `showFrame()`): top band = right cluster first, then the focus text,
  then the mode centred in the space left (falls back to "FIXED", then drops the mode). Bottom band =
  fps / "off in" placed first; hints `= Scan`, `▶ Focus`, `AC Back` (Effort dropped when it would collide;
  the mode text shows it).

## Proxy follow-ups (2026-10-10, from server/proxy/FINAL_REVIEW_server.md)

Same changes in `firmware-prototype/` (v14) except the system-prompt flag. Version now `v15lcd-2026.10.10`.
Build: RAM 28.8 % (94,404 B), flash 68.3 % (1,343,637 B), no warnings from `src/` or `core/`.

- **Preview Send timeout is an idle limit now** (`src/main.cpp`, `SendStep::Solving`): was a 120 s *total*
  (`kSendSolveTimeoutMs`), so a long `xhigh`/`max` solve still streaming at 2 minutes was reported "took too
  long". Now `kSendSolveIdleMs` = 150 s without a byte to or from the proxy (above `kIdleMs` = 120 s in
  `claude_client.cpp`, which normally reports first and stays unchanged) and `kSendSolveMaxMs` = 300 s overall.
  `claude_client.cpp` stores `millis()` of every byte sent/received (`claudeLastActivityMs()`, atomic, read
  by the loop task); the proxy's `: ping` every 15 s keeps it moving while Claude thinks. Only activity after
  the Send started counts. The message text is unchanged (no screenshot re-render needed).
- **`rate_limited`** (core `classifyFailure`): the proxy's burst limit shows its message ("wait a minute")
  instead of "busy". Older firmware gets `fair_use_exceeded` from the proxy (chosen by `x-firmware` date).
- **No `system` in the upload** (`-DCALC_SEND_SYSTEM_PROMPT=0` in `platformio.ini`; core
  `buildSolveRequest(..., withSystem)`): ~5 KB less per solve; the proxy owns the prompt. Needs the proxy of
  2026-10-10 or later (older proxies forwarded the device's prompt). For the proxy's `PROMPT_SOURCE=device`
  bench mode build with `-DCALC_SEND_SYSTEM_PROMPT=1`. `output_config.format` is still sent. The simulators
  call Claude directly and still send `system` (default `true`).

## MODE 4 Apps: colour screens, LCD look, wiring (2026-10-11)

Spec: `docs/MODE4_APPS_SPEC.md`. Version `v15lcd-2026.10.11`. Build: RAM 29.2 % (95,540 B), flash 78.6 %
(1,544,601 B of the 1,966,080 B OTA slot; was 73.9 %), no warnings from `src/`. v14 (`firmware-prototype`)
still builds (RAM 30.9 %, flash 68.7 %). Core tests: 911 passed (+ geo 550, periodic 199, engine 34).

- **Two looks** (`ui.cpp`, `ui_draw.h` `Palette`): the calculator and its menus / SETUP / errors / exam /
  charging screen in a mono LCD palette (grey-green #C7D1B5, ink #1E2318 10:1, secondary #343B2C 7.3:1, no
  colour roles, soft keys outlined, exam bar inverted); Apps and every app in the brand colours (`ui::color`
  is now navy #10141c / orange #e07a4f, so the viewfinder overlay and self-test follow). The shared info
  pages (Notice, NetInfo, Look, ExamInfo) keep the look they were opened from (sticky flag in `showDevice`).
  "LCD dots" (core `dotMatrix()`, default on) now does something: glyphs at 3x+ are drawn as dots with a
  1-px gap (`glyphdraw::lcdDots()`), only in the LCD look.
- **App screens** are drawn from the Device's public state (`ui_apps.cpp`, `ui_geo.cpp`,
  `ui_periodic.cpp`), not from the decoded 125x61 render; the core render is still made every loop and its
  hash decides when to redraw (it reflects every state change, toasts included). AI Solve pages keep the
  decoded-text typesetting, with an app title bar ("AI SOLVE" / "TYPE IT" / "HISTORY").
- **Geography map speed**: every country scan-line filled from the core polygons each redraw (no cache;
  the map only redraws on a key). PC: 0.28 ms per world frame (harness, 200 runs). ESP32-S3 estimate: the
  world view is ~1.5 M simple float operations (each scan row tests every edge of the countries it crosses;
  one division per crossing), ~10-20 ms at 240 MHz, plus ~2 ms of span writes to the PSRAM canvas, then the
  22 ms panel push: inside the 40 ms aim. Zoomed views skip off-screen countries by bounding box and add
  ~7.6k short border lines (~3 ms). **Measure on the bench** (log `millis()` around `ui::showDevice` on the
  map) before calling it done.
- **Type it** (`main.cpp` `queueRequest` / `solveTask`): `takeTypedImage()` right after `takeRequest()`; the
  PNG rides the existing pre-photo hand-over (`prePhotoSet`), so the solve task skips the camera, never
  touches the viewfinder, and sends it through `claudeSolve` like a photo (`buildSolveRequest` marks
  `image/png`). It waits up to 20 s for the hotspot (no photo time to join in). `=` on its answer resends.
- **History / notes storage**: LittleFS file `/apps.bin` on the existing `spiffs` partition (128 KB) instead
  of an NVS blob: the NVS partition is 20 KB and Wi-Fi uses part of it, while 20 saved answers (≤ 6 KB each
  in the core) can reach well over that. Temp file + rename, 2 s debounce (`kAppsSaveDelayMs`), saved before
  deep sleep, damaged file ignored (`restoreApps` false). First boot formats the partition (~1 s, once).
  The last app selected is saved with it.
- **Settings ↔ backlight**: `serviceScreenSettings()` applies Settings > Brightness at once and the sleep
  timer as the idle policy (off = N min, dim = half of it ≤ 20 s, panel sleep = off + 60 s); NVS `lcd` is
  written 2 s after the last change. `bright` / `screen` serial commands and SHIFT ▲▼ update Settings.
- **SHIFT ▲▼ fix**: the key is now passed on to the core after the brightness step. Core `keyCalc` takes
  SHIFT ▲/▼ as a no-op that only clears SHIFT (`onKey` clears the latch before dispatch), so SHIFT no longer
  stays latched; each step needs SHIFT again, like every SHIFT function.
- **Link**: `printProxyJson` feeds `setLinkInfo()` from `/v1/pair/start` (`code`, `link_url` minus "/link",
  `already_linked`) and `/v1/device/status` (`linked`); opening Settings > Link calculator joins the hotspot
  and asks automatically (blocking ~1-2 s of TLS in the loop, on that screen only). Linked state in NVS
  `link`; linked → unlinked clears History (`clearHistory()`).
- **Exam flag**: kept the one NVS-vs-restored comparison; `examExpiredOnRestore()` only words the log.
- `setVersion(kFirmwareVersion)` for About; `setRandomSource(esp_random)` was already there (quiz varies).
- Core fix (`core/calc_engine.cpp` `parseExpression`): typed "2^3²" was auto-closed before the ² (64); the
  close now waits for postfix x², x³, x⁻¹, !, % (Casio type A binds tighter than ^): 2^(3²) = 512. Tests added.
- `drawOff` centred "Charging 64%" with the core's leading spaces (off-centre); trimmed now.
- Screenshots: `tools/ui_sim_v15` builds `ui*.cpp`; new `A*` (apps) and `L*` (LCD look) screens and
  `hardware/renders_ui_v15/apps_contact_sheet.png`. The harness's AI flows now open AI Solve with MODE 4 =.

Bench checks for this entry: the LCD palette on the real IPS panel (grey-green may need a tweak for its
gamma, `LCD_INVERT`), map frame time, LittleFS mount time on a key wake (adds to the wake latency), the
Link screen's blocking fetch, typed solve end to end against the proxy.

## Fuzz campaign fixes (2026-10-11)

A fuzz campaign over `core/` (random keys, replies, persistence mutants, PNG decoding) found 11 bugs.
All fixed, each with a test in `tests/tests.cpp` `testFuzzFindings()`; the harnesses themselves are now
`tests/fuzz_tests.cpp` (runs in every `build_and_run.bat`, ~5 s) and `tools/fuzz/run_long.bat`
(millions of steps; `tools/fuzz/README.md`).

- **B-6 (high) History blob vs the 128 KB LittleFS**: `saveApps()` could reach ~120-146 KB (20 replies of
  6 KB, written as tmp + rename). Now a saved reply is at most 2 KB (`Device::kMaxSavedJson`; bigger: the
  question and answer only), the list text 256 B, and the blob at most 40 KB (`kMaxAppsBytes`: older answers
  saved short, oldest dropped if still over; worst case measured 35 KB). Older blobs with 6 KB replies still
  restore (shortened). `main.cpp` `appsWrite()`: deletes a stale `/apps.tmp` first; if two copies don't fit,
  removes the old file and writes the new one in place (the data is complete in RAM); a failure shows the
  toast "Couldn't save" once and retries every 30 s (`kAppsRetryMs`).
- **B-1 / B-7 restoreState()** accepted any number: an exact LLONG_MIN crashed the maths, NaN showed "nan".
  `Reader::num()` now refuses non-finite, |x| >= 1E100, and exact values outside the engine's limits
  (den <= 0, |num| or den > 1E15, num/den disagreeing with the value); history items over 99 keys too.
  `gcdLL()` works in unsigned arithmetic (LLONG_MIN safe).
- **B-2** ON on a typed problem's (or a History answer's) AI screen now goes to the photo home screen with
  the typed text dropped: = there takes a photo instead of resending the old PNG.
- **B-9** `Device::setTypedSolveSupported()` (default false): only a platform that sends
  `takeTypedImage()`'s PNG may offer Type it; elsewhere TYPE IT shows "Not available on this model". The
  v15 `main.cpp` (and the UI harness) set it; v14, the old tester and the simulators keep the default. The
  typed PNG is freed when its request is cancelled or fails.
- **B-3 / B-8** MODE / SETUP from Link, a Confirm question or an error now come back to it; `backView()`
  never returns to a deleted note, and a restart into exam mode clears what MODE would return to.
- **B-10** MODE 4 from AI SOLVE leaves AI mode (a scan in flight is cancelled): no "AI" in the status bar,
  ON goes to the calculator.
- **B-11** an answer arriving keeps History's selection on the same row; a pending "Clear history?" is asked
  again. `restoreApps()` closes a note being edited.
- **B-4** ENG stops where the mantissa no longer fits 10 digits (it showed wrong digits, then inf).
- **B-5** byte cuts on user text use `utf8Prefix()` (`core/text.h`): History summaries, error details.
- **No AI vendor name on screen** (owner rule): "Working on it", "AI BUSY", "AI ERROR", "The tutor took too
  long to reply.", "The tutor declined this one.", the preview page ("Send to the tutor", "The answer"),
  `failureText()`, the setup page and the serial key warning. Comments, identifiers and log lines unchanged.
- **Type it's first SEND showed NO CONNECTION** (found by `tools/sim_v15`): Wi-Fi was only wanted in AI mode.
  `serviceWifi()` now also wants it while Type it's keyboard is open, so the hotspot joins while the problem is
  typed and the loop's `setOnline()` (true while joining) lets SEND go; the solve task waits `kTypedWifiWaitMs`.
  Mirrored in `tools/sim_v15/board.cpp`; its `--selftest` now asserts the first SEND solves. `ui_apps.cpp`:
  two int-to-float conversions made explicit.
- Screenshots re-rendered (`23_ai_solving`, new `27_ai_failure_busy`, `28_ai_failure_timeout`, apps
  contact sheet). Build: RAM 29.2 % (95 540 B), flash 78.7 % (1 547 769 B).

## Calculator screens exactly like the Casio (2026-10-11)

Owner: "take the bottom bar with the shift and apps stuff out of the firmware, it needs to be exactly like
the Casio until you are in the apps screen".

- `ui.cpp`: every LCD-look screen (COMP empty / typing / result / error, MODE, SETUP, Hyp, CLR + confirm,
  Screen look, Wi-Fi & key and Exam-mode info opened from SETUP, calculator notices) has **no soft-key bar**
  ("MODE Menu / MODE 4 Apps / SHIFT 2nd", "AC Back", "AC Clear / ◀▶ Go to" are gone). `statusFromGrid()`
  is replaced by `casioStatus()`: an 18 px indicator row on the glass (no bar fill, no rule) with the core's
  cells 0-11 only (S, A, M, STO / RCL, D / R / G) in the ink at 2x, ▲▼ at the right (moved there from the
  content's right edge). No battery %, Wi-Fi, "AI" or title; `casioBattery()` draws a small battery only at
  ≤ 20 % or with USB power in (plus a bolt while charging). Exam mode keeps the inverted bar + padlock +
  clock. Content: `kCalcTop` 22 to `kCalcBottom` 166, 24 px a core row (was 20 px between two bars); the
  result sits on the bottom line; toasts in the LCD look sit at the bottom of the glass.
- `ui.cpp` `rowUnderlined()`: a row counts as a header only if every character is underlined. SETUP's
  selection marks reach one pixel into the gap of the row above and drew a stray rule under "1:Deg 2:Rad"
  and "3:Gra 4:Norm1" (visible in the old `11_setup.png`).
- Core (`core/device.cpp`, so v14's e-paper and the fbtext readback match): the info pages shared with the
  Settings app (Screen look, Wi-Fi & key, Exam mode) draw "AC:back" only when opened from Settings; from
  SETUP the exam-mode page asks "[=]  :Start / [AC] :Cancel" (as CLR's confirm). Calculator notices (EXAM
  MODE ON / OFF, STAT / TABLE "Coming in a later update", APPS ARE OFF from MODE 4) end with "Press [AC] key"
  instead of "AC:continue" / "AC:back". App and AI views keep their hints. MODE / SETUP had no hint lines.
  Goldens re-generated with `UPDATE_GOLDEN=1` (3 snapshot checks changed: the exam-mode pages / notice).
- `ui_apps.cpp`: the "History cleared" toast no longer overlaps the empty-History text (the explanation is
  left out while a toast shows).
- The Apps look (carousel, every app, AI Solve, Type it, History, Notes, Geography, Periodic, Settings,
  Link, Confirm, toasts) is unchanged.
- Results: core tests 1211 passed, 0 failed; `pio run` RAM 29.2 % (95 540 B), flash 78.7 % (1 547 189 B);
  v14 `firmware-prototype` builds (RAM 30.9 %, flash 68.8 %); `Proofline_Simulator_v15.exe --selftest`
  25 screenshots, 0 failures. Screenshots and `apps_contact_sheet.png` re-rendered.
- Not changed: the core's own status row still holds the Wi-Fi glyph and "AI" for v14's e-paper; the v15
  indicator row ignores them.

## Stage 2: the Cloudflare Worker API (2026-10-11)

- New `core/worker_api.*` (host-testable, no networking): register body (`hw_id`, `hw_secret` only when
  well-formed), `SolveRequest` body (images + `tutor`, PNG for typed problems), parsers for register /
  status / `pair/start` / `/v1/firmware`, and `classifyWorkerFailure` (error type → `Failure` + an action:
  re-register, revoked, unlinked with the code). Vendor names are stripped from any message for the screen.
  `ota.cpp` now uses the shared manifest parser for both servers, and skips an image of another board family
  before downloading (the Worker announces whatever was published last).
- Firmware: `Settings::api` (NVS `api` = proxy | worker, `wurl`, `wtoken`, `wdevid`; the proxy's keys are
  untouched), `src/worker_client.*`, `claude_client.cpp` sends the Worker body and maps its errors,
  `main.cpp` registers on the first connection (retry 2 min, 15 min after a 429), polls the status every
  5 s on Settings > Link calculator, and acts on unlinked / revoked / unknown token. Serial: `api`,
  `register`, `enroll hs_...` (NVS `factory/hw_secret`, never printed or erased by `forget`). Phone setup
  page: in Worker mode the address field edits the Worker's address and the token field is hidden.
  `-DCALC_ALLOW_HTTP=1` (off by default) allows a plain-http Worker for a bench test against wrangler dev.
- Tests: `tests/worker_api_tests.cpp` (`int runWorkerApiTests()`, 130 checks, examples from openapi.yaml,
  the Worker's `http.ts` messages and `cannedSse` / mid-stream error fixtures) — not yet called from
  `tests.cpp`. Live: `tools/worker_live` (WinHTTP + the same core code) against the local Worker:
  register 201 → status (not linked, code from `reason`) → `pair/start` (new code, status follows it) →
  solve 402 `device_not_linked` → AI ACCOUNT with the code → firmware "no update". Linking and an
  entitled solve need a signed-in owner on the site (not done by the agent).
- Build: `pio run` RAM 30.3 % (99 428 B), flash 82.4 % (1 619 737 B); the `CALC_ALLOW_HTTP=1` variant builds too.
- Contract notes for the lead: `DeviceStatus` has no `pair_code` / `link_url` (the firmware reads the code
  out of `reason`); `pair/start` answers 200 with `code`, but `web/dev/calculator.mjs cmdCode` reads
  `pair_code` and checks for 201; no `rollback` flag in the Worker's firmware manifest (older images can't
  be pushed back); the root bundle (`root_ca.h`) still lacks the SSL.com roots PLAN section 4 asks for.

## STAT, TABLE, CALC, SOLVE, ∫dx, d/dx wired in (2026-10-11)

- Core `device.*`: MODE 2 / MODE 3 open `StatApp` / `TableApp` (new views `Stat`, `Table`); CALC and SHIFT
  CALC start `numeric::CalcRunner` / `SolveRunner` (views `CalcRun`, `SolveRun`); ALPHA CALC types `Tok::Eq`,
  ∫dx `Tok::Integ`, SHIFT ∫dx `Tok::Deriv`. The "not supported yet" / "Coming in a later update" notices are
  gone. `Mode` gained `Stat` / `Table` (`mode()`), kept under AI SOLVE and the Apps (a separate
  `calcMode_`). SETUP has a second page (▼): 1:STAT → `View::SetupFreq` "Frequency? 1:ON 2:OFF".
  `backView()` now uses an explicit app-view range, so the new views (after `Link` in the enum) are not
  treated as apps in exam mode. MODE ends a running CALC / SOLVE. `Device` is non-copyable now (the modes
  and runners point at its variables).
- Status row: "STAT" / "TABLE" in cells 11-15; in exam mode the clock moves to cell 17 (padlock + h:mm).
  `ui.cpp casioStatus()` draws cells 0-15 when a mode tag is there and shortens its exam tail to padlock + clock.
- `saveState()`: mode byte 0-3; flag 32 = STAT Frequency; after the history an optional mode block
  ('S' type, rows, X [Y] [FREQ] doubles; 'T' f(X) tokens + Start / End / Step), at most ~650 bytes,
  reserved before the history fills the 2 KB. Older states (no block) still restore; damaged blocks are refused.
- Decisions: SOLVE leaves Ans unchanged and stores the root in X (the runner's default, as the Casio);
  d/dx at a corner (d/dx(Abs(X),0)) stays a Math ERROR where the real fx-115ES shows 0 (a deliberate,
  safer difference documented in `core/numeric.h`).
- Build after the change: `pio run` RAM 30.3 % (99 428 B), flash 82.4 % (1 620 921 B). v14
  `firmware-prototype` builds: RAM 32.0 %, flash 71.1 %.
- Optional `main.cpp` hook (not done, networking agent's file): SHIFT ▲▼ brightness is only handled on
  `View::Calc`; adding `View::Stat` / `View::Table` / `View::CalcRun` / `View::SolveRun` there would keep it
  working in those modes (today SHIFT ▲▼ there goes to the mode as a cursor key).

## Worker: optional status / firmware fields, live end-to-end (2026-10-11)

- `core/worker_api`: `WorkerStatus` gains `linkUrl` and `planReason` (the 7 known codes, else "");
  `pairCode` / `site` come from `pair_code` / `link_url` when sent (`pair_code` must be a string: null
  when linked), else from `reason` as before. New `planStatusText()`, `pairQrUrl()`,
  `versionFamily()` (was private in worker_api.cpp and ota.cpp), `firmwareCheckPath()`.
  `FirmwareManifest` gains `family`; `path` must now match the contract
  (`/v1/firmware/image` optionally `?family=<1-24 alnum>`); a `family` other than ours is skipped.
- `ota.cpp`: `GET /v1/firmware?family=v15lcd` (the old proxy ignores the query); the image is fetched
  at the manifest's `path` as given; the downgrade guard (`a < b && !info.rollback`) is unchanged.
- `Device::setLinkPlan()` / `linkPlan()` (additive): shown only while linked; mono render adds the
  lines under "Already linked." (golden `settings_link_plan`), the colour `drawLink` shows a 2-line
  accent pill in place of "This calculator is on your account." (harness shot `A47_link_plan`).
  `main.cpp workerRefreshLink()` sets it from the status and takes `link_url`'s site when sent.
- Tests: `worker_api_tests.cpp` new-field cases (present / absent / null / unknown / wrong types, every
  plan line, family check path, family manifest, rollback, legacy null family, bad paths).
  Suite: 1317 passed, 0 failed. `pio run`: RAM 30.4 % (99 452 B), flash 82.6 % (1 624 069 B).
- Live run against the local Worker (dev, fake AI + fake Stripe) with `tools/worker_live`: register →
  status `plan_reason=not_linked`, `link_url=http://127.0.0.1:8787/link/`, code = `pair_code` → linked
  on the local site with a test account (poll saw it in 60 s) → `no_trial` ("No free month") and solve
  402 AI ACCOUNT → fake-Stripe yearly subscription → `plan_reason=active`, photo solve and typed PNG
  solve answered by the fake AI → `GET /v1/firmware?family=v15lcd` 200, nothing published for the
  family (`update: false`). Board enrolment was not run (the script needs the admin token).
  `worker_live` no longer prints any part of the token.

## TLS roots for the Cloudflare Worker (2026-10-11)

- `src/root_ca.h` (still one concatenated PEM string, `kRootCAs`, passed to `setCACert`; mbedTLS reads
  every block) now trusts what Cloudflare can issue from. Cloudflare's "Certificate authorities" page
  (last updated 2026-04-16): Let's Encrypt, Google Trust Services, SSL.com, Sectigo (backup certificates
  only). Live today: `workers.dev`, `workers.cloudflare.com`, `api.anthropic.com` = leaf ← WE1 ← GTS Root R4
  (the server also sends R4 cross-signed by GlobalSign Root CA, not needed); Cloudflare's own SSL.com chain
  (`one.one.one.one`) = leaf ← SSL.com SSL Intermediate CA ECC R2 ← SSL.com Root Certification Authority ECC.
- Added: SSL.com Root CA ECC + RSA (2016-2041), SSL.com TLS ECC Root CA 2022 (to 2046, the successor root),
  Sectigo Public Server Authentication Root E46 (to 2046; Sectigo DV moved to E46/R46 in 2025). Kept: GTS R1,
  GTS R4, ISRG X1, ISRG X2, DigiCert Global Root G2, DigiCert Global Root CA, Amazon Root CA 1 (identical
  bytes). Left out to stay small: GlobalSign Root CA (expires 2028), SSL.com TLS RSA 2022, Sectigo R46,
  ISRG Root YE/YR (not in trust stores yet; their chains are cross-signed to X2/X1).
- Every root was downloaded from its CA's repository and its SHA-256 matched Mozilla CCADB's included-roots
  report (SSL.com also matched the repository page); fingerprints, sources and dates are in the header comments.
- ESP32: the framework's mbedTLS has ECDSA with P-256 and P-384, so the P-384 roots (GTS R4, ISRG X2, SSL.com
  ECC, Sectigo E46) are usable. The bundle is 11 roots, 13 931 B of PEM (was 7 roots, 9 225 B); mbedTLS
  parses it into internal heap at each connect (roughly DER size + ~0.5 KB per root, freed on disconnect),
  so the TLS connect needs ~5 KB more free internal heap than before. Not measured on hardware.
- Check: `python tools/tls_check/check_roots.py --live` (stdlib only): 11/11 parse, fingerprints match the
  comments, all are self-signed CAs, none expires before 2031; live handshakes trusting ONLY the bundle
  succeed for workers.dev, workers.cloudflare.com, developers.cloudflare.com (GTS WE1), one.one.one.one
  (SSL.com ECC R2), letsencrypt.org (Let's Encrypt YE2 → Root YE → X2 cross-sign), api.anthropic.com.
  Removing GTS Root R4 makes workers.dev fail ("unable to get local issuer certificate"), so the bundle is
  what is being tested. Host-side OpenSSL, not the ESP32's mbedTLS: confirm one solve over the Worker on a
  board (PLAN section 4 also asks to check which CA signed the live custom-domain certificate at stage 1).
- Build: `pio run` RAM 30.4 % (99 452 B), flash 82.8 % (1 628 773 B).

## Bug hunt fixes: stack, ∫ / d/dx / SOLVE, STAT, state, JSON (2026-10-11)

- **Stack (R-1).** The parser recursed through 7 frames per bracket / function (Xtensa -Os: `power` 1040 B
  with `function()` inlined, `perm` 256, `addSub` 224, `mulDiv` / `implicitMul` / `neg` 160): ~2 KB a level,
  and an ∫ / d/dx started a fresh parser with its own 24 levels (+ `derivative` 992 B, Ridders' 10×10
  table). Now the precedence levels inline into one `expr()` frame, everything with big locals is out of
  line (`apply`, `number`, `combine`, the maths), a run of (−) is counted instead of recursed, Ridders keeps
  one column (160 B), and ONE nesting budget (`calc::kMaxNesting` = 24, the Casio's) is shared by an
  ∫ / d/dx and its f (the template counts `kTemplateDepth` = 4 more: its ~1.8 KB of numeric frames).
  One more level = Stack ERROR. Xtensa frames now (from this build with `-fstack-usage`): `expr` 288,
  `operand` 64, `primary` 64, `function` 128, `addSub` 32 → **576 B per function level, 448 per bracket**
  (was ~2 000); ∫ chain `calculus` 256 + `evalTemplate` 112 + `integrateExpr` 480 + `integrate` 416 +
  `addPieces` 432 + `Bound::at` 112 + `evaluate` 160.
- **Loop task 16 → 32 KB** (`SET_LOOP_TASK_STACK_SIZE`, main.cpp). Worst case summed from the .su files:
  `loopTask` 32 + `loop` 1 056 + `keysPoll` 64 + `handleEvent` 32 + `press` 32 + `Device::onKey` 432 +
  SOLVE (`keyRunner` 48 + `SolveRunner::onKey`/`run` 144 + `solveExpr` 544 + `solve` 288 + lambdas 96 +
  `Bound::at` 112 + `evaluate` 160) = ~3.1 KB, + 24 × 576 = 13.8 KB, + innermost leaf (`apply` 800 + libm)
  ~1.2 KB, + C++ exception unwinding at the throw ~2 KB (estimate) = **~20 KB**; the plain COMP path is
  ~19.6 KB. 32 KB leaves ~12 KB. Before: ~48 KB for 24 levels, ~96 KB with an ∫ (16 KB task: overflow at
  ~7 levels). The stack comes from internal heap at boot: +16 KB (static RAM unchanged, 30.4 %).
  Not measured on hardware (`uxTaskGetStackHighWaterMark` on the bench would confirm).
- **Time budget + AC (R-3).** One run (the outermost `evaluate()` or `solveExpr()`, `calc::EvalRun`) may
  evaluate f at most `kMaxFunctionEvals` = 25 000 times (SOLVE over an ∫ included): then ∫ / d/dx give
  Math ERROR and SOLVE "Can't Solve" (no "Time Out" on the fx-ES). ~1.2 µs an f evaluation on a PC
  (60-step f) → ~5 s on the ESP32 at worst. `calc::setCancelHook()` is polled at the first and every 64th
  evaluation; main.cpp wires it to `keysCancelPressed()` (keys.cpp): reads the TCA8418's queued events into
  a 16-event buffer that `keysPoll()` delivers first, in order, and returns true for AC (or ON). The AC
  then clears the error screen as usual. The PC simulators don't need it (25 000 evaluations < 0.1 s).
- **d/dx (R-2):** first step 1E-3·max(1, |x0|) (was 0.1·max(1, |x0|)), ÷8 up to 10 times while the error
  estimate is above tol, best kept; corner check with the best step, shrinking until the sides agree; a
  tiny value that shrinks with the step is 0 (X·Abs(X) at 0). cos X at 100, e^X at 100, sin(50X) at 0,
  Deg sin X at 3600, ln X at 1E-4, X^X at 50 work; Abs X at 0 is still Math ERROR.
- **∫ (R-7, R-8):** never accepted from one piece: 4 equal pieces, plus over a range wider than 1000 break
  points 1, 10, 100 … from 0 (or the end nearer 0) — ∫(1/(1+X²),−1E10,1E10) was 1.5708, now π. Where f is
  undefined at a node the piece is split there (≤ 8 points): ∫(ln Abs X,−1,1) = −2, ∫(1/√(Abs X),−1,1) = 4.
- **SOLVE (R-10):** convergence and the slope step are relative to |x| (floors 1E-12 / 1E-12 of the
  guess's size): X²−1E−30 from 1 gives 1E-15 (was 9E-13 with L−R = 0), X³−1E−30 now solves too.
- **STAT (R-4, R-5, R-9):** editor cells try %g and %e per precision (−0.000152 in 6 = "-2E-4", was
  "######"); spreads and fits shift by the first value and treat rounding noise (n·(8ε·max|u|)²) as no
  spread: 0.1 × 3 has σx = 0, all-equal x is Math ERROR for every regression, all-equal y has no r;
  A·B^X keeps ln B for x̂ / ŷ. **TABLE (R-6):** X = start + i·step snapped (0 below 1E-9 of a step, else 15
  significant digits): −0.3 + 3 × 0.1 shows 0.
- **Saved state (R-11):** version 2 = version 1 + an Adler-32 at the end (RTC memory that lost a bit is
  refused); version 1 is still read. STAT / TABLE doubles with 0 < |x| < 1E-99 (denormals) are refused;
  memories that small become 0. **SETUP (R-16):** angle unit / Norm / Frequency changes and a restore call
  `syncModes()` at once, so STAT / TABLE draw with them immediately.
- **JSON / Worker (R-12 – R-15):** nesting limit 64 → `Json::kMaxDepth` = 20 (bounds parse, copy and
  destroy recursion); a high surrogate needs a real low one, lone surrogates / control characters (but \n;
  \t \r → space) / invalid UTF-8 bytes become '?'; JSON numbers are range-checked before every integer cast
  (`intIn`); `neutralMessage` is one case-insensitive pass over at most 220 bytes (cut at a word).
- Checks: `tests\msvc\build_and_run.bat` 1348 passed, 0 failed (new tests in numeric / stat_table /
  worker_api / tests.cpp); the bug hunter's harnesses: repro all fixed, numeric accuracy (INT 0 wrong,
  DER 0 unexpected errors, was 23), device fuzz 1M steps (debug build) 0 failures, JSON fuzz 200 000
  documents 0 findings. Build: `pio run` RAM 30.4 % (99 492 B), flash 83.1 % (1 634 629 B).
- Left: ∫(cos X,0,1E4) still needs more than the 400 pieces (Math ERROR); SOLVE X²−1E−98 from 1 stops
  at ~8E-25 (below 1E-12 of the guess's size the step floor applies); the v14 firmware (`firmware-prototype`,
  `firmware`) keeps its 16 KB loop task: ~20 KB worst case there, raise it the same way before shipping it.

## MODE 5 EQN wired in (2026-10-12)

- New core module `core/eqn.{h,cpp}` (`EqnApp`, the STAT / TABLE module pattern: `onKey` / `render` /
  `wantsExit` / `resume` / `setEnv`; cells via `cellText`, entry via `drawEntry`, keys via `modeKeyToken`).
  Solvers in `calc::eqn`: `solveLinear` (row + column equilibration, partial pivoting, pivot tolerance
  1E-12, rank test → No Solution / Infinite Solution), `solveQuadratic` (power-of-2 scaling, fma-compensated
  discriminant snapped to 0 within 16 ulp, cancellation-free roots, vertex from the same discriminant),
  `solveCubic` (depressed cubic; triple / double roots detected with rounding-noise bounds on P, Q and Δ,
  so they carry no spurious imaginary part; Cardano / trigonometric roots polished by Newton; the complex
  pair from the deflated quadratic; |im| < 1E-12 of the roots' size snapped to 0).
- `device.*`: `Mode::Eqn` (value 4) and `View::Eqn` (after `SetupFreq`, so not an app view: allowed in
  exam mode), MODE menu row 3 `5:EQN`, `enterEqn()` / `keyEqn()`, `syncModes()` / ON / `backView()` /
  `modeView()` / `openMenu()` cover it. No mode tag in the status row (the fx-ES has no EQN indicator).
- `saveState()` stays version 2: mode byte up to 4 and an 'E' block (type or 0xFF, then rows x cols
  doubles; at most 98 bytes) after the history; damaged blocks (bad type, non-holdable value, short block)
  are refused. Older firmware refuses a state whose mode byte is 4 (as it did for STAT / TABLE).
- Fix found by the screenshots: in exam mode the ▲▼ of STAT / TABLE / EQN were drawn black on the
  inverted exam bar (invisible, and unreadable to `ui.cpp`); the arrow cell is now drawn plain and
  re-inverted.
- `ui.cpp`: `View::Eqn` goes through `drawModeScreen()`; `bigValueOf()` returns the shown result when it
  fits the glass at 2x (27 characters), else the core's one / two grid rows show it. `main.cpp` and both
  simulators: SHIFT ▲▼ brightness on `View::Eqn`.
- Tests: `tests/eqn_tests.cpp` (`runEqnTests()`, 189 checks: systems incl. singular / inconsistent /
  scaled / Hilbert, quadratics real / double / complex / scaled, cubics three real / one real + pair /
  double / triple / scaled + 800 random, editor keys, every screen as a golden, exam mode, save / restore
  and damaged states); fuzz: EQN in the mode fuzzer, the device invariants and the persist mutants.
  Desktop sim self-test step 7d (EQN quadratic). Screenshots `hardware/renders_ui_v15/E01-E31`.
- Build after the change: `pio run` RAM 30.6 % (100 212 B), flash 84.0 % (1 651 633 B); v14
  `firmware-prototype` RAM 32.2 %, flash 72.1 %.

## MODE 6 CMPLX wired in (2026-10-12)

- New core module `core/cmplx.{h,cpp}`: `evaluateComplex()` is a small parser with the real one's
  structure (same priorities; ∠ at nPr / nCr's level; implicit ×; auto-closed brackets; the shared
  `kMaxNesting` and an `EvalRun`) over the same tokens. Every all-real step goes back to the real engine
  through the new, additive `calc::realOp()` / `calc::realNumber()` (a public `Parser::op()` /
  `numberAt()` wrapping the existing `combine` / `postfix` / `apply` / `number`), so real values get
  exactly COMP's results; `evaluate()` itself is unchanged (it gives Syntax ERROR on the new tokens).
  Complex: + − × ÷ (exact while the parts are; Smith's division when c² + d² is out of range), x² x³ x⁻¹,
  ^ integer (binary powering, |n| ≤ 1E9), Abs (exact √(a²+b²) when it is), arg (atan2, (−180°, 180°], the
  real engine's `clean`), Conjg, r∠θ (the real cos / sin: cos 90° = 0). The rest is Math ERROR with a
  non-real value. An inexact part below 1E-14 of the other is dropped as rounding noise ((1∠30)³ = i).
- Tokens added at the end of `Tok` (saved values keep their numbers): `ImagI Angle Arg Conjg ToPolar
  ToRect` ("i" "∠" "arg(" "Conjg(" "▶r∠θ" "▶a+bi"). Font: ⇔ added (R⇔I), drawn unlike the font's #.
- `device.*`: `Mode::Cmplx` (5), `View::CmplxMenu` / `View::SetupCmplx` (after `Eqn`: not app views,
  allowed in exam mode). CMPLX is `View::Calc`: `equalsCmplx()`, STO / RCL / M+ / replay with
  `ImagVars imag_` beside `Vars`; `setCalcMode()` is now the only way the calculator mode changes (leaving
  CMPLX drops the imaginary parts and the shown result's, entering or leaving clears the replay).
  `complexSplit()` / `complexImagShown()`: R⇔I and SHIFT =. Status row: "CMPLX" in cells 11-15 (the
  v15 `casioStatus()` picks it up like STAT / TABLE). `keysForChar`: j = ENG (i), @ = SHIFT (−) (∠).
- `saveState()` stays version 2: mode byte up to 5, flag 64 = r∠θ, a 'C' block (11 imaginary parts, the
  history count, then each replay item's imaginary part and form, newest first; sized inside the history
  fit, ≤ 2 KB with 20 items). Wrong count, bad form byte, non-holdable parts, another mode's block:
  refused. Older firmware refuses mode byte 5.
- Stack (Xtensa -Os -fstack-usage, this code): `CxParser::expr` 464, `operand` 32, `primary` 48,
  `function` 32 (its first argument goes straight into the caller's slot; a second one gets
  `secondArg` 96) → **576 B per function level, 544 per bracket** (COMP: 576 / 448), so 24 levels stay
  within the ~20 KB worst case measured on 2026-10-11. Leaves: `div` 464, `pow` 336, `apply` 192 (+ the
  real engine's `op` 224 + `apply` 784 for a real function). No f re-evaluation in CMPLX, so the
  cancel hook is never needed there.
- `ui.cpp`: `drawCalc()` shows "R⇔I" left of a split result (the result's scale leaves room for it).
- Tests: `tests/cmplx_tests.cpp` (`runCmplxTests()`, 301 checks: arithmetic and identities, i² = −1,
  (3+4i)(1−2i) = 11−2i, division, powers, conjugate, Abs / arg in Deg / Rad / Gra, ∠ entry, ▶r∠θ /
  ▶a+bi, real values identical to COMP, Math / Syntax / Stack ERRORs and positions, formats and the R⇔I
  split, Device keys, Ans / PreAns / STO / RCL / M+, leaving CMPLX, exam mode, Apps round trip,
  save / restore incl. damaged blocks, 10 goldens); fuzz: CMPLX in the device sessions (and its persist
  mutants), invariants (CmplxMenu only in CMPLX, no imaginary part outside it, finite), and 20 000 random
  token lists through `evaluateComplex()` / `complexText()` / `complexParts()`. Goldens changed:
  `menu_mode*`, `eqn_menu_mode*` (6:CMPLX), `setup_page2` (2:CMPLX). Desktop sim self-test step 7e;
  keypad labels ∠ (SHIFT (−)) and CMPLX (SHIFT 2) in both simulators. Screenshots C01-C20.
- Build after the change: `pio run` RAM 30.9 % (101 340 B), flash 84.8 % (1 667 581 B); v14
  `firmware-prototype` RAM 32.6 %, flash 73.0 %.

## MODE 7 BASE-N wired in (2026-10-12)

- New core module `core/basen.{h,cpp}`: `evaluateBaseN(toks, vars, base)` is a small recursive parser
  over the same tokens (no real engine involved): or / xor / xnor < and < + − < × ÷ < implicit × < sign
  and the d h b o prefixes < brackets, `Not(`, `Neg(` (the fx-ES priority table). int64 intermediates,
  every step checked against 32 bits (Math ERROR), the final result against the current base's range
  (`baseFits()`: BIN 16 bits). Literals are read as bits: BIN up to FFFF (8000 and up negative), HEX / OCT
  up to FFFFFFFF, DEC up to 2147483647; a digit the base doesn't have is a Syntax ERROR at its token.
  Memories / Ans: `trunc()` (a COMP fraction), outside 32 bits Math ERROR. Shares `kMaxNesting` (Stack
  ERROR) and an `EvalRun`. `baseText()` pads HEX 8, BIN 16, OCT 11 digits; DEC uses COMP's "−".
- Tokens added at the end of `Tok`: `And Or Xnor Xor Not BNeg PreD PreH PreB PreO HexA..HexF` ("and"
  "or" "xnor" "xor" "Not(" "Neg(" "d" "h" "b" "o" "A".."F"); `evaluate()` gives Syntax ERROR on them.
  No font change (all ASCII).
- `device.*`: `Mode::BaseN` (6), `View::BaseMenu` (after `SetupCmplx`: not an app view, so exam mode
  allows it). BASE-N is `View::Calc`: `equals()` / `valueForMemory()` / RCL / M+ / the replay go through
  `evaluateBaseN()` when `baseN()`; `keyCalc()` maps x² x^□ log ln to `setBase()` (Math ERROR when the
  shown result doesn't fit the new base), (−) °'" hyp sin cos tan to A-F, SHIFT 3 to the menu, and drops
  every key `baseNKey()` doesn't list. `setCalcMode()` clears the replay when entering / leaving BASE-N
  and resets the base to DEC on leaving; entering is DEC (the guide). Status row: `baseName()` at cell 11
  (the mode tag). `saveState()`: mode byte 6 + an 'N' block (the base byte); `restoreState()` refuses a
  bad base, a block under another mode, and (under BASE-N) a replay value that isn't a 32-bit integer.
  `keysForChar('"')` = the °'" key (B). `continuesAns()` takes and / or / xnor / xor ("Ans and").
- `ui.cpp`: nothing new needed (the result is `resultText()`, sized like any long one; the mode tag is
  read from the grid); comments only.
- Tests: `tests/basen_tests.cpp` (`runBaseNTests()`, 282 checks: the guide's examples, widths, two's
  complement, truth tables, priorities, ranges / overflow / ÷ 0 / bad digits / positions, nesting,
  memories, Device keys, menu pages and ▲▼, Ans continuation, replay, STO / RCL / M+ / M−, leaving,
  exam mode, Apps round trip, save / restore incl. damaged blocks, 8 goldens). Fuzz: BASE-N in the
  persist sessions, invariants (BaseMenu only in BASE-N, a non-DEC base only in BASE-N, the result text
  matches its base's format), 20 000 random token lists through `evaluateBaseN()` / `baseText()`. Goldens
  changed: `menu_mode*`, `eqn_menu_mode*`, `cmplx_menu_mode` (7:BASE-N). Desktop sim self-test step 7f;
  keypad labels DEC HEX BIN OCT (teal, right of SHIFT) and BASE (SHIFT 3) in both simulators (A-F were
  already the red ALPHA letters). Screenshots B01-B20.
- Build after the change: `pio run` RAM 30.9 % (101 340 B, +0), flash 85.1 % (1 673 105 B, +5 524 B =
  +0.28 % of the OTA slot); v14 `firmware-prototype` RAM 32.6 %, flash 73.3 %.

## MODE 8 MATRIX / MODE 9 VECTOR wired in (2026-10-12)

- Source: fx-115ES PLUS / fx-991ES PLUS C User's Guide (2nd edition), casioeducation.com PDF
  (`_files/ugd/3cfc42_7829a5fe35b144d181d5deb75f532ac6.pdf`, pp. 73-77 MATRIX, 79-82 VECTOR, 93-94 errors) and
  support.casio.com's `fx-115ESPLUS_991ESPLUSC_en` matrix / vector pages. That edition adds Ref / Rref (left
  out: the menu asked for, and the original fx-115ES PLUS, have 8 items).
- New core module `core/matvec.{h,cpp}`: `evaluateMatVec(toks, vars, mv, vectors, unit, random)`, a recursive
  parser with calc_engine's priorities (the dot product at × ÷'s level; implicit ×; auto-closed brackets;
  `kMaxNesting`; an `EvalRun`). Scalars go through `realOp()` / `realNumber()`, so elements stay exact
  fractions; det by cofactors, inverse by the adjugate (exact; an inexact determinant below 1E-13 of its
  products is 0 = singular = Math ERROR), products, transpose, cross / dot, magnitude via `realOp(Sqrt)`.
  Matrices live in a parser-owned pool of `kMvSlots` = 8 (the Casio's "matrix stack": one more = Stack
  ERROR); the parser (~2.4 KB) is on the heap, so the recursion frames hold no matrix.
- `MatVecApp` (same file): Dim's variable / size lists, the editors (EQN's editing rules; elements typed as
  expressions through `evaluate()`), the MatAns / VctAns screen, the SHIFT 4 / SHIFT 5 menu, STO copies; it
  tells the Device what to do next with `takeAction()` (back to the calculation screen / insert a token).
- Tokens appended to `Tok`: `MatA MatB MatC MatAns VctA VctB VctC VctAns Det Trn DotP` ("det(" "Trn(" "·");
  `CalcError::Dim` appended ("Dim ERROR"). `evaluate()` gives Syntax ERROR on the new tokens.
- `device.*`: `Mode::Matrix` (7), `Mode::Vector` (8), `View::MatVec` (after `BaseMenu`: not an app view, so
  exam mode allows it). The calculation screen is `View::Calc` (`equalsMatVec()`; a matrix result sets
  `resultForm_` 2: `matrixResultShown()`, `resultText()` "", MatAns screen; `continueWith_` MatAns);
  `valueForMemory()` takes scalars only (a matrix: Syntax ERROR). `setCalcMode()` clears the replay and the
  variables when entering / leaving. Status row: `MAT` / `VCT` at cell 11. MODE menu row 4-5:
  `7:BASE-N 8:MATRIX` / `9:VECTOR` (the exam line moved to row 6). CLR Memory clears the variables too.
- `saveState()` stays version 2: mode bytes 7 / 8 and an 'X' / 'V' block (four size bytes rows<<4|cols with
  their elements as Nums, then the history count and a form byte per replay item, 0xFF scalar / 2 matrix).
  Worst case (four 3×3 of inexact values + 20 replay items) measured 1 837 bytes of the 2 048 RTC bytes, so
  no move to the apps blob was needed. Refused: a size over 3, columns without rows, a 2-row or 1-D vector,
  another mode's block, a bad form byte, a count that isn't the history's.
- Stack (Xtensa -Os -fstack-usage): `MvParser::expr` 320, `operand` 32, `primary` 48, `function` 80 → 400 B per
  bracket, 480 per function level (COMP 448 / 576). Leaves: `matPostfix` 640 + `inverse` 224 + `det` 160.
- `ui.cpp`: `View::MatVec` drawn by `drawModeScreen()` like EQN (the core's rows; the cursor cell inverted).
  `main.cpp` and both simulator boards: SHIFT ▲▼ (brightness) also on `View::MatVec` (the app ignores it).
- Tests: `tests/matvec_tests.cpp` (`runMatVecTests()`, 373 checks: the guide's examples 1-8 / 1-7, 3×3 det /
  inverse (A·A⁻¹ = I exactly; decimals within 1E-12), singular, transpose, Dim / Syntax / Math / Stack errors
  and positions, the matrix stack, dot / cross / Abs, cell text, Device keys and screens, editor rules, STO
  copies, replay, exam mode, Apps round trip, CLR, save / restore incl. damaged blocks and the worst case, 21
  goldens). Fuzz: MATRIX / VECTOR in the persist sessions, invariants (MatVec only in its modes, sizes, cursor,
  nothing kept outside), a `MatVecApp` in the mode-keys run, 20 000 random lists through `evaluateMatVec()`.
  Goldens changed: `menu_mode*`, `eqn_menu_mode*`, `cmplx_menu_mode`, `basen_menu_*` (8:MATRIX 9:VECTOR).
  Desktop sim self-test step 7g; keypad labels MATRIX (SHIFT 4) and VECTOR (SHIFT 5) in both simulators.
  Screenshots X01-X22.
- Build after the change: `pio run` RAM 31.4 % (102 948 B, +1 608), flash 86.0 % (1 691 233 B, +18 128 B =
  +0.92 % of the OTA slot); v14 `firmware-prototype` RAM 33.1 %, flash 74.2 %.

## Open items / later

- Touch panel pins 26-29 are unused (no spare GPIOs).
- `cameraPreviewFrame()` (JPEG → grey) is dead code in this project; remove once the RGB565 path is
  proven, or keep as the fallback path (`cameraFrame` for the laptop preview page still uses JPEG).
- ~~`kFirmwareVersion` still says `stage14-2026.10.06`~~: done, `v15lcd-2026.10.10` (the proxy keeps one
  image per board family since 2026-10-10, so v14 and v15 can be updated at the same time).
- ~~The core's `View::Look` (Plain / LCD dots) has no effect on the LCD~~: done 2026-10-11 (dotted large
  text in the LCD look).
- `graphify update .` was run after the port (AST only).
