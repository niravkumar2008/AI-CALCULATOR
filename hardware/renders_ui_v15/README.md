# v15 colour LCD: real UI screenshots

These are **real screenshots, not mock-ups**. The firmware's own drawing code drew every image on a PC:

- `firmware-v15-lcd/src/ui.cpp`, `fbtext.cpp`, `vf_lcd.cpp` and `selftest.cpp` are compiled **unchanged**.
- They draw into a real **LovyanGFX `LGFX_Sprite`** (the library version the firmware builds with, compiled for the PC).
- The real **core/** calculator feeds them (`calc::Device`, `App`, `Viewfinder`).

Only the panel itself is simulated. `lcd::flush()` and `lcd::pushRaw()` copy the 320 × 170 RGB565 frame into a file instead of sending it over SPI.

Old mock-up, for comparison: `hardware/enclosure/final_assembly_v15_lcd/renders/mock_ui_320x170.png`. It was a mock-up and is **not** what the firmware draws.

## Regenerate

```
python tools/ui_sim_v15/make_screens.py
```

What it needs:

- Pillow and numpy
- the MSVC Build Tools (found the same way as `tests/msvc/build_and_run.bat`)
- LovyanGFX in `firmware-v15-lcd/.pio/libdeps`. If it isn't there, run `pio run -e v15lcd` once in `firmware-v15-lcd`.

What it does:

1. It makes the sample photo.
2. It builds `tools/ui_sim_v15/build/ui_sim_v15.exe` with `build.bat` and runs it.
3. It writes every PNG in this folder.

It takes about 1 minute; the first LovyanGFX build takes about 2 minutes. Nothing in `firmware-*` or `core/` is changed. The harness only adds host stand-ins:

| File | What it stands in for |
|---|---|
| `shim/fw/Arduino.h`, `WiFi.h`, `esp_chip_info.h`, `esp_camera.h` | The ESP32 Arduino core and ESP-IDF headers |
| `host_hw.cpp` | `lcd::`, using a fake clock |
| `shim/lgfx_host/SDL.h` | Type names only, so LovyanGFX picks its desktop platform. No real SDL and no window. |
| `lgfx_host_*.cpp` | LovyanGFX's platform functions |

## Files

All files are 320 × 170, which is exactly the panel's pixels. `x3/<name>_x3.png` is the same image ×3, nearest-neighbour, for print.

| File | Screen | Drawn by |
|---|---|---|
| `01_calc_home` | Calculator after ON (cursor) | `ui::showDevice` ← `Device` |
| `02_calc_typing` | Typing `2×(3+4)²÷7+√16` | same |
| `03_calc_result` | …and `=` → 18 | same |
| `04_calc_typing_functions` / `05_calc_result_functions` | `sin(30)+π×2²−log(100)` (wraps to 2 lines) → 11.06637061 | same |
| `06_calc_shift` | SHIFT on: amber S, soft keys change | same |
| `07_calc_result_fraction` | `1÷3+1÷4` → 7⌟12 | same |
| `08_calc_syntax_error` | Syntax ERROR page (amber) | same |
| `10_menu_mode` | MODE menu | same |
| `11_setup` | SETUP (SHIFT MODE) | same |
| `20_ai_ready` | AI SOLVE home (MODE 4) | same (`App`) |
| `21_ai_hold_still` | `=`: HOLD STILL countdown | same |
| `22_ai_reading`, `23_ai_solving` | Waiting pages | same |
| `24`–`26_ai_answer_page1..3` | Claude's answer (x = 5, ✓ calculator agrees, steps), scrolled with ▼ | same |
| `30`–`35_status_*` | Status bar: Wi-Fi off / joining / connected, charging, 15 % (red), battery unknown | same, different `ui::Status` |
| `36_low_battery_ai_screen` | AI SOLVE at 6 %, AI locked out | same (`setLowBattery`) |
| `37_low_battery_notice` | BATTERY LOW notice after `=` | same |
| `40_off_charging` | Off with the cable in: "Charging 64%" | same (`drawOff`) |
| `41_off_blank` | Off, no cable (black) | same |
| `42_exam_mode_on`, `43_exam_mode_calc` | Exam mode (SHIFT 7 ON while off): red bar | same |
| `50_setup_by_phone`, `51_setup_by_phone_saved` | Setup-by-phone pages | `ui::showLines` with `main.cpp`'s strings * |
| `52`–`54_ota_progress_0/42/100` | Firmware update progress | `ui::showProgress` with `main.cpp`'s strings * |
| `55_ota_done`, `56_ota_failed` | Update result pages | `ui::showLines` * |
| `60_selftest_start` … `65_selftest_pass` | The whole factory self-test: colour bars + grey ramp + border, camera, Wi-Fi, key count, PASS | `selftestRun()` run whole, with stubbed sensors (every check passes) |
| `70_camera_starting` | Camera powering up | `vflcd::showStarting` |
| `71_camera_focusing` | Viewfinder: FOCUSING (amber), white brackets | `vflcd::showFrame` over the sample photo; box from core `Viewfinder` |
| `72_camera_focused` | FOCUSED (green brackets), amber box around the writing | same |
| `73_camera_hold_still` | Picture moved 14 px → core reports motion → HOLD STILL | same |
| `74_camera_careful_tutor` | Mode text "Careful +Tutor" | same |
| `75_camera_fixed_focus_timeout` | Fixed-focus module, 18 % battery (red), "off in 8s" | same |
| `sample_photo_320x240.png` | The made-up camera picture: a handwritten "3x + 7 = 22" on lined paper, made with Pillow, at the preview size | `make_screens.py` |
| `sheet_status_bar_variants.png` | The top 24 px of the status-bar screens, ×3, labelled | composed |
| `sheet_all_screens.png` | Contact sheet of every screen, ×2 | composed |
| `framed/front_*.png`, `framed/display_*.png` | 10 screens placed in the calculator window of `final_assembly_v15_lcd/renders/{front,display}_screen_key.png` | Same chroma-key corner fit as `compose_v15.py` (its functions are imported); slight lens tint |

\* `main.cpp` (Wi-Fi, FreeRTOS, OTA) is not compiled. Its `showLines` / `showProgress` calls are made with the same strings it passes. The network name and password are made up, in `setup_portal.cpp`'s format. The update version `v15lcd-2026.10.20` is invented.

## How these can differ from the real panel

- **Colour.** The PNGs show the RGB565 values exactly, expanded to 8 bits. The real IPS panel adds its own gamma, viewing angle and backlight level. Also, `LCD_INVERT`, `LCD_RGB_ORDER` and `PREVIEW_SWAP_BYTES` in `platformio.ini` are unconfirmed (see `PORT_NOTES.md`). If one is wrong on the real board, colours will be inverted or red/blue swapped until it is set.
- **Camera pictures.** The photo is synthetic, and it is fed in at the right size and byte order. A real OV5640 frame will have its own white balance, noise and sharpness, so the focus state and the writing box will vary. The fps (14.3) and the time-out are fixed inputs, not measured.
- **The cursor.** It blinks on the real screen. Every screenshot is taken in its "on" phase.
- **Fake time.** The self-test runs on a fake clock. The "86 s left" and the key count (`21 of 50`) are what a fast tester would see.

### Things the screenshots show about the firmware itself

The firmware was not changed. These are worth fixing in `ui.cpp`, `fbtext.cpp` and `vf_lcd.cpp`:

1. **√ shows as `?` in the expression** (`02`, `03`). `fbtext::match()` only searches `calc::kGlyphs[]`. It does not search the replacement glyphs in `core/font.cpp` (`kExtra`: √ ≈ ⌟ ▲ ▼ ◀ ▶ padlock ✓ ⚠ Wi-Fi), so those cells decode as raw. `drawCalc()` turns raw cells into `?`. Menu and answer rows draw raw cells as pixels, so ✓ looks right there.
2. **The ▲▼ scroll arrows never appear** (`24`–`26`). This has the same cause: `statusFromGrid()` looks for `0x25B2` / `0x25BC`, but those cells are raw.
3. **The HOLD STILL countdown digits are garbled** (`21`). Core draws them at 2× with `drawTextPx`, which the 5 × 7 cell decoder can't read.
4. **Status bar overlaps.** The battery % text overlaps the Wi-Fi icon, for example `.64%` in `31`–`33`. The AI indicator overlaps the % text in `20` and `36`.
5. **Text clipped at the right edge:**
   - the calculator soft keys ("SHIFT 2nd" is cut to "SH")
   - "Downloading v15lcd-2026.10.20" (29 characters at 11 px)
   - the self-test caption "…grey ramp, border A1B2C3"
6. **Viewfinder text collisions:**
   - In the top band, a long focus text runs into the centred mode text ("FIXED FOCUSNormal" in `75`; "FOCUSED" touches "Careful +Tutor" in `74`).
   - In the bottom band, the "AC Back" hint runs into the fps / "off in" text on the right.
