# Port notes: v14 e-paper firmware → v15-LCD (2026-10-08)

Rules followed: `firmware-prototype/` (v14) and `core/` were **not modified** (`pio run` in
`firmware-prototype` still gives RAM 30.7 %, flash 63.4 %, byte-identical numbers to before; core tests
not re-run because core/ is untouched). Nothing committed. No API key anywhere. No eFuse touched.

## Build result

| Project | RAM | Flash (1.9 MB OTA slot) |
| --- | --- | --- |
| `firmware-v15-lcd` (`env:v15lcd`) | **28.8 %** (94,372 B) | **68.0 %** (1,336,201 B) |
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
    pixels at 2x (still correct, just small). Run through every menu once and look for them.
12. OTA from a v14 image to v15 is **not** safe (different hardware): keep separate version strings and
    proxy images per board (bump `kFirmwareVersion` in `claude_client.h` to a `v15-` prefix before the
    first OTA upload).

## Open items / later

- Touch panel pins 26-29 are unused (no spare GPIOs).
- `cameraPreviewFrame()` (JPEG → grey) is dead code in this project; remove once the RGB565 path is
  proven, or keep as the fallback path (`cameraFrame` for the laptop preview page still uses JPEG).
- `kFirmwareVersion` still says `stage14-2026.10.06`: change to a v15 string before the first OTA.
- The core's `View::Look` (Plain / LCD dots) has no effect on the LCD; it could become the brightness page.
- `graphify update .` was run after the port (AST only).
