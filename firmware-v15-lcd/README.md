# Board firmware for v15-LCD (colour screen + live camera preview)

Firmware for the **v15-LCD board** (`../hardware/kicad_v15_lcd`): the v14 calculator board with the
2.13" e-paper replaced by a **1.9" 170x320 colour IPS LCD** (panel: **BuyDisplay ER-TFT019-1**, no touch,
ST7789P3, 4-wire SPI) and a PWM backlight (Q5 switch, R23 = 15 Ω: ≈ 26 mA typical at 100 %, 16-37 mA range).
Everything else is the v14 hardware and the v14 behaviour: the full calculator from `../core` (the same
code as the simulators), the whole keypad through the TCA8418, the OV5640 autofocus camera, the 150 mAh
battery and deep sleep, Wi-Fi through the AI proxy, over-the-air updates, the phone setup page and the
factory self-test.

**This is a separate project.** `../firmware-prototype/` is the v14 (e-paper) firmware and is unchanged
for the v14 boards being built now; never flash this one to a v14 board (the e-paper GPIOs are driven as
LCD lines and KEYPAD_INT is on a different pin). `../firmware/` is the old breadboard tester.

**Status (2026-10-08):** builds clean (`pio run`: RAM 28.8 %, flash 68.0 % of the 1.9 MB OTA slot).
**Not yet run on hardware** (the v15 board was verified "ORDER" in `hardware/verification/14_v15_final_preorder.md`
but has not arrived yet): everything marked "verify on the panel" in `PORT_NOTES.md` needs the real LCD.
The GPIO roles below are the final (review 13) ones; `src/pins.h` takes them from the header, so no code
change was needed for review 14 (R23 and the key-pad moves are hardware only; `kMatrix` is unchanged). The v14 project still builds unchanged (RAM 30.7 %, flash 63.4 %).

## What is different from v14

| | v14 (`firmware-prototype`) | v15 (this) |
| --- | --- | --- |
| Screen | 2.13" e-paper, 250x122, 1-bit, ~0.3 s partial refresh, ghosting | 1.9" IPS LCD, 320x170, 65k colours, ~22 ms per frame |
| Driver | GxEPD2 (`screen.cpp`) | LovyanGFX `Panel_ST7789` (`lcd.cpp`): SPI2 at 40 MHz with DMA, column offset 35, canvas in PSRAM |
| UI | the core's 125x61 picture drawn at 2x | the same picture read back as text (`fbtext.cpp`) and re-typeset in colour: status bar with battery % / Wi-Fi, large expression (3x font) and result (4x), soft-key bar (`ui.cpp`) |
| Viewfinder | grey, dithered, 2-3 fps | **live colour preview**, 320x240 RGB565 from the sensor, middle 320x170 shown 1:1, framing guide, focus state, ▶ to refocus (`vf_lcd.cpp`) |
| Backlight | none | LEDC PWM on IO4 (R23 15 Ω: ≈ 26 mA at 100 %, ≈ 9 mA average at the 60 % default, because the duty is squared to 36 %): brightness setting in NVS, dim after 20 s, off after 60 s, panel sleep after 120 s, any key wakes it |
| Power down | hold e-paper lines | backlight 0 → SLPIN → every LCD line low → LCD_PWR_N high (panel unpowered, < 2 µA) |
| Pins | `hardware/pins_final.h` | `hardware/kicad_v15_lcd/pins_v15_lcd.h` (KEYPAD_INT on **IO3**) |
| Self-test | e-paper checkerboard, `display_ok` from the BUSY timing | colour bars + grey ramp + border, backlight sweep: `lcd_ok`, `lcd_push_ms`, `backlight_ok`, `backlight_pct` (and `display_ok` / `display_refresh_ms` kept as aliases) |
| Serial | | new: `bright N`, `screen dim|off|sleep N`; `status` has a Screen line |
| Keys | | SHIFT ▲ / SHIFT ▼ = brightness (like the Casio's contrast); ▶ in the viewfinder = refocus |

Nothing in `../core/` changed. The firmware files that are byte-identical to v14: `claude_client.*`, `ota.*`,
`keys.*` (the pin comes from the header), `preview.*` (the laptop page), `setup_portal.*`, `root_ca.h`, `log.h`.

## Files

| File | What |
| --- | --- |
| `src/main.cpp` | the loop (as v14) + the LCD viewfinder, backlight idle policy, brightness keys |
| `src/lcd.*` | ST7789 driver setup (LovyanGFX), power sequence, panel sleep, canvas, backlight PWM + idle policy (NVS `lcd`) |
| `src/ui.*` | the colour screens: status bar, calculator, menus / AI pages / notices (from the core's render), setup / update / self-test pages |
| `src/fbtext.*` | reads the core's 1-bit render back into text cells (glyph matching against the core font) |
| `src/glyphdraw.h` | draws the core's 5x7 font scaled on an LGFX sprite (one font everywhere, every maths symbol) |
| `src/vf_lcd.*` | the live preview overlay (brackets, focus state, text box, hints) drawn into the camera frame, then pushed |
| `src/camera.*` | v14's capture + **preview mode** (`cameraPreviewBegin / Grab / Release`, `cameraRefocus`, `cameraFocusState`): RGB565 QVGA ↔ JPEG mode switch |
| `src/power.*` | as v14, with the LCD lines held low and LCD_PWR_N / backlight floated in deep sleep |
| `src/selftest.*` | as v14 with the LCD / backlight step |
| `src/settings.*` | as v14 + `bright`, `screen` commands |
| `src/pins.h` | includes `hardware/kicad_v15_lcd/pins_v15_lcd.h`; refuses to build without `-DBOARD_V15_LCD` |
| `platformio.ini` | `env:v15lcd`; the four panel flags to confirm on the bench (`LCD_ROTATION`, `LCD_INVERT`, `LCD_RGB_ORDER`, `PREVIEW_SWAP_BYTES`) |

## Build and flash

1. VS Code + PlatformIO. **File > Open Folder** > `firmware-v15-lcd`.
2. Plug the magnetic USB cable in; click **Upload**. Command line:
   `%USERPROFILE%\.platformio\penv\Scripts\pio run -t upload` from this folder (`pio run` only builds).
3. **Serial Monitor** at 115200, type `help`. Recovery (no COM port) is exactly as in
   `../firmware-prototype/README.md` (TP1 to GND, tap TP7). **Never burn an eFuse.**

Dependencies (fetched by PlatformIO): `lovyan03/LovyanGFX@^1.2.0` (built with 1.2.32), the
`ESP32-OV5640-AF` library, and `../core` via symlink. Partition table `min_spiffs.csv` (two OTA slots),
same as v14, so an OTA image built here installs on a v15 board that was first flashed over USB.

## Pins (from `hardware/kicad_v15_lcd/pins_v15_lcd.h`)

| Signal | GPIO | Note |
| --- | --- | --- |
| LCD_MOSI | 5 | SDO (MISO) not wired: write-only panel |
| LCD_DC | 6 | |
| LCD_SCK | 8 | SPI2 through the GPIO matrix, 40 MHz |
| LCD_CS | 41 | |
| LCD_RST | 42 | |
| LCD_PWR_N | 33 | low = panel powered (Q4 P-FET); R21 pull-up keeps it off in sleep |
| LCD_BL_EN | 4 | backlight PWM (Q5), R22 pull-down = off in sleep |
| KEYPAD_INT | **3** | was IO4 on v14; still an RTC pin (deep-sleep wake works) |
| Camera, I2C, ON, battery, USB | unchanged | |

`python tools/check_pins.py` (copied from v14) compares the header with a netlist exported from the
v15 schematic.

## Using the live preview

MODE 4 (AI SOLVE) starts the camera in preview mode (about 1 s). The screen shows what the camera sees
in colour, the middle 170 of the 240 rows: the photo that will be sent continues above and below the
dashed marks. Top band: the focus state (**FOCUSING** amber / **FOCUSED** green / **HOLD STILL** when
the picture moves), the effort / tutor setting, battery. Bottom band: `= Scan`, `▶ Focus`, `▲▼ Effort`,
`AC Back`, and the frame rate (or "off in 10 s" near the 30 s idle time-out). An amber box marks writing
the analysis found. Keys: **=** takes the full-resolution photo (the sensor switches to JPEG mode for the
scan, ~1 s added to the hold-still time; the viewfinder comes back on the next visit to AI SOLVE),
**▶** triggers a single-shot autofocus, **▲▼** effort, **1** tutor, **AC** back. Rules kept from v14: no
preview, camera or Wi-Fi below 3.6 V on battery (3.7 V to resume, or a cable); camera off before Wi-Fi
bursts; 30 s without a key turns the camera off (▶ turns it on again).

## Screen and backlight

- `bright N` (10-100, saved) or SHIFT ▲ / SHIFT ▼ on the calculator screen. Default 60 %. The duty is
  squared (50 % looks half as bright). Full current at 100 % is set by R23 = 15 Ω: ≈ 26 mA typical,
  16-37 mA depending on the panel's LED voltage (ER-TFT019-1: 2.8-3.2 V at 60 mA); measure it as
  V(R23) / 15 (bring-up guide step 10).
- `screen dim 20`, `screen off 60`, `screen sleep 120` (seconds without a key; 0 = never). While the
  viewfinder, a scan, setup by phone or an update is on, the light stays full. When the light is off, the
  next key only wakes the screen (ON still counts).
- Off (SHIFT AC or 10 min idle) = deep sleep as in v14; the panel is unpowered, so the whole board stays
  at the ≤ 40 µA target. The screen is redrawn from the calculator's saved state on wake (~150 ms).

## Self-test

`selftest` (or SHIFT + ALPHA + ON): colour bars, a grey ramp, a white border and the device id, then the
backlight sweeps 0 → 100 %. The JSON line adds `lcd_ok` (driver initialised and a frame pushed in
< 200 ms — the panel cannot answer back, so look at it), `lcd_push_ms`, `backlight_ok`, `backlight_pct`;
`display_ok` / `display_refresh_ms` are kept so the v14 bring-up checklist still reads. Other fields are
unchanged.
