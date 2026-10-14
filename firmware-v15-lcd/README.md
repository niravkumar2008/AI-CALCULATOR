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
but has not arrived yet; 2026-10-11: the board has been ordered, delivery date not recorded): everything marked "verify on the panel" in `PORT_NOTES.md` needs the real LCD.
The GPIO roles below are the final (review 13) ones; `src/pins.h` takes them from the header, so no code
change was needed for review 14 (R23 and the key-pad moves are hardware only; `kMatrix` is unchanged). The v14 project still builds unchanged (RAM 30.7 %, flash 63.4 %).

## What is different from v14

| | v14 (`firmware-prototype`) | v15 (this) |
| --- | --- | --- |
| Screen | 2.13" e-paper, 250x122, 1-bit, ~0.3 s partial refresh, ghosting | 1.9" IPS LCD, 320x170, 65k colours, ~22 ms per frame |
| Driver | GxEPD2 (`screen.cpp`) | LovyanGFX `Panel_ST7789` (`lcd.cpp`): SPI2 at 40 MHz with DMA, column offset 35, canvas in PSRAM |
| UI | the core's 125x61 picture drawn at 2x | the same picture read back as text (`fbtext.cpp`) and re-typeset: calculator in the Casio layout (indicator row, large expression (3x font) and result (4x), no soft keys); Apps in colour with a status bar (battery % / Wi-Fi) and soft-key bar (`ui.cpp`) |
| Viewfinder | grey, dithered, 2-3 fps | **live colour preview**, 320x240 RGB565 from the sensor, middle 320x170 shown 1:1, framing guide, focus state, ▶ to refocus (`vf_lcd.cpp`) |
| Backlight | none | LEDC PWM on IO4 (R23 15 Ω: ≈ 26 mA at 100 %, ≈ 9 mA average at the 60 % default, because the duty is squared to 36 %): brightness setting in NVS, dim after 20 s, off after 60 s, panel sleep after 120 s, any key wakes it |
| Power down | hold e-paper lines | backlight 0 → SLPIN → every LCD line low → LCD_PWR_N high (panel unpowered, < 2 µA) |
| Pins | `hardware/pins_final.h` | `hardware/kicad_v15_lcd/pins_v15_lcd.h` (KEYPAD_INT on **IO3**) |
| Self-test | e-paper checkerboard, `display_ok` from the BUSY timing | colour bars + grey ramp + border, backlight sweep: `lcd_ok`, `lcd_push_ms`, `backlight_ok`, `backlight_pct` (and `display_ok` / `display_refresh_ms` kept as aliases) |
| Serial | | new: `bright N`, `screen dim|off|sleep N`; `status` has a Screen line |
| Keys | | SHIFT ▲ / SHIFT ▼ = brightness (like the Casio's contrast); ▶ in the viewfinder = refocus |

Nothing in `../core/` changed. The firmware files that are byte-identical to v14: `claude_client.*`, `ota.*`,
`keys.*` (the pin comes from the header), `preview.*` (the laptop page), `setup_portal.*`, `log.h`.
`root_ca.h` (the HTTPS trust list) is no longer identical: since 2026-10-11 it holds 11 roots covering every
CA Cloudflare uses for `*.workers.dev` and custom domains (Google Trust Services, Let's Encrypt, SSL.com,
Sectigo) plus the DigiCert / Amazon roots of the older hosts. Re-check it with
`python tools/tls_check/check_roots.py --live` (fingerprints, CA flag, expiry, live handshakes).

## Files

| File | What |
| --- | --- |
| `src/main.cpp` | the loop (as v14) + the LCD viewfinder, backlight idle policy, brightness keys, the Apps' typed solve, flash storage (history / notes), Settings ↔ backlight, link code |
| `src/lcd.*` | ST7789 driver setup (LovyanGFX), power sequence, panel sleep, canvas, backlight PWM + idle policy (NVS `lcd`) |
| `src/ui.*` | the two looks (mono LCD for the calculator, colour for Apps), status bar, calculator, menus / AI pages / notices (from the core's render), toasts, setup / update / self-test pages |
| `src/ui_draw.h` | internal: palettes, shared pieces (soft keys, lists, status bar), the app screens' entry points |
| `src/ui_apps.cpp` | Apps carousel + vector icons, Type it keyboard, History, Notes, Settings, Confirm, Link |
| `src/ui_geo.cpp` | Geography: the filled world map from the core's polygons, card, list, letter finder, quiz |
| `src/ui_periodic.cpp` | Periodic Table: coloured table, details, list, finder, property graph, colour key |
| `src/fbtext.*` | reads the core's 1-bit render back into text cells (glyph matching against the core font) |
| `src/glyphdraw.h` | draws the core's 5x7 font scaled on an LGFX sprite (one font everywhere, every maths symbol) |
| `src/vf_lcd.*` | the live preview overlay (brackets, focus state, text box, hints) drawn into the camera frame, then pushed |
| `src/camera.*` | v14's capture + **preview mode** (`cameraPreviewBegin / Grab / Release`, `cameraRefocus`, `cameraFocusState`): RGB565 QVGA ↔ JPEG mode switch |
| `src/power.*` | as v14, with the LCD lines held low and LCD_PWR_N / backlight floated in deep sleep |
| `src/selftest.*` | as v14 with the LCD / backlight step |
| `src/settings.*` | as v14 + `bright`, `screen` commands; `api`, `register`, `enroll` (Worker), NVS `factory` |
| `src/claude_client.*` | HTTPS to the AI server in use (proxy or Worker): solve stream, small JSON calls, firmware download |
| `src/worker_client.*` | the Worker's register / status / link-code calls (bodies and replies: `core/worker_api.*`) |
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

MODE 4 opens the Apps menu; **=** on AI SOLVE (app 1) starts the camera in preview mode (about 1 s). The screen shows what the camera sees
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

- Settings > Brightness (◀ ▶), `bright N` (10-100, saved) or SHIFT ▲ / SHIFT ▼ on the calculator screen
  (one step per SHIFT ▲, like any SHIFT function; all three show the same value). Default 60 %. The duty is
  squared (50 % looks half as bright). Full current at 100 % is set by R23 = 15 Ω: ≈ 26 mA typical,
  16-37 mA depending on the panel's LED voltage (ER-TFT019-1: 2.8-3.2 V at 60 mA); measure it as
  V(R23) / 15 (bring-up guide step 10).
- Settings > Sleep timer (1, 2, 5, 10, 15, 30 min) sets when the light goes off (dimmed at half of it, at
  most 20 s; the panel sleeps a minute later). Or `screen dim 20`, `screen off 60`, `screen sleep 120`
  (seconds without a key; 0 = never; Settings then shows the nearest choice). While the
  viewfinder, a scan, setup by phone or an update is on, the light stays full. When the light is off, the
  next key only wakes the screen (ON still counts).
- Off (SHIFT AC or 10 min idle) = deep sleep as in v14; the panel is unpowered, so the whole board stays
  at the ≤ 40 µA target. The screen is redrawn from the calculator's saved state on wake (~150 ms).

## Apps menu (MODE 4), and the LCD look vs colour

Spec and owner decisions: `docs/MODE4_APPS_SPEC.md`. Screenshots of every screen:
`hardware/renders_ui_v15/apps_contact_sheet.png` (from `tools/ui_sim_v15/make_screens.py`).

**Two looks.** The calculator (COMP, MODE menu, SETUP, errors, exam mode, the charging screen) looks
like a classic mono calculator LCD: flat grey-green glass (#C7D1B5), near-black ink (10:1 contrast;
secondary text 7.3:1), no colour anywhere. **Since 2026-10-11 it is laid out exactly like the Casio
fx-115ES / fx-300ES** (owner: "exactly like the Casio until you are in the apps screen"): **no soft-key
bar**; a thin indicator row on the glass itself (no bar, no rule) with only what the core's status row
holds: S, A, M, STO / RCL, D / R / G, and ▲▼ on the right when the core sets them. No battery %, no
Wi-Fi icon, no "AI", no title. The only extras: a small battery glyph when the battery is ≤ 20 % and a
small bolt + battery while USB power is in. The content runs to the bottom of the glass (the result sits
on the bottom line, as on the Casio). Error pages show the core's own "Syntax ERROR / [AC] :Cancel /
[◀][▶]:Goto" lines; MODE and SETUP show only their numbered items; calculator notices end with the
Casio's "Press [AC] key". Exam mode keeps its inverted bar (ink bar, LCD-green padlock + EXAM clock) so
it shows across a room.
SETUP > Screen look > LCD dots draws the large expression and result as separate dots (plain = solid).
**Full colour only inside Apps** (the carousel and every app, AI Solve and its viewfinder included):
navy #10141c, brand orange #e07a4f, white / grey text. AC out of Apps returns to the LCD look. The
info pages both sides share (Wi-Fi & key, Screen look, Exam mode, notices) keep the look of the screen
they were opened from (from SETUP: Casio layout, no hint line; from Settings: the app status bar, the
"AC:back" hint and the soft-key bar). The Apps look keeps its status bar (title, battery %, Wi-Fi) and
soft-key bar unchanged.

**The apps.** MODE 4 → carousel (◀ ▶, 1-7, =, AC back to the calculator): 1 AI Solve, 2 Type it,
3 History, 4 Notes, 5 Geography, 6 Periodic Table, 7 Settings. All off in exam mode ("APPS ARE OFF").
- **Type it**: on-screen keyboard (arrows + =, digit / operator keys type directly, DEL, AC asks before
  discarding). SEND: the core renders the text as a PNG; `main.cpp` takes it with
  `takeTypedImage()` right after `takeRequest()` and sends it through the same solve request as a photo
  (no camera, no viewfinder; `buildSolveRequest()` marks it `image/png`). The answer shows like a photo's.
- **History / Notes** live in flash: the core's `saveApps()` blob as `/apps.bin` on the `spiffs` data
  partition (LittleFS, 128 KB, already in `min_spiffs.csv`, so OTA is unaffected). NVS was too small: 20
  answers with their steps reach tens of KB. Written 2 s after the last change (and before deep sleep),
  via a temp file + rename; a damaged file is ignored. History is cleared when the proxy reports the
  calculator unlinked.
- **Settings**: brightness and sleep timer drive the backlight (NVS `lcd`, shared with `bright` /
  `screen` / SHIFT ▲▼), Wi-Fi & key (= sets up from a phone, as SETUP 6), Link calculator, Screen look,
  Exam mode, Battery, About (version).
- **Link calculator**: opening it joins the hotspot and asks the proxy (`/v1/device/status`, then
  `/v1/pair/start` if not linked); the code shows big with "Go to <site>/link". The serial `pair` and
  `account` commands feed the same screen. The linked state is kept in NVS `link`.
- **Geography**: the world map is filled from the core's polygons (177 countries, 7.6k points;
  scan-line, even-odd), borders when zoomed in, the selected country orange with its capital as a white
  dot and ring; = card, 1/0 zoom, 2 list, 3 letter finder, 4 continent filter, 5 capitals quiz.
  Names are folded to ASCII for the 5x7 font ("Brasília" → "Brasilia").
- **Periodic Table**: 118 tiles coloured by category (10 colours, the key on 0), the cursor tile framed,
  an info box in the table's gap, footer "26 Fe Iron 55.845"; = details, 1 list (2 sorts), 3 find,
  4 filter (others dimmed), 5 property graph (▲▼ property, ◀▶ element).
- **Toasts** ("Saved", "Deleted", "History cleared") are a pill above the soft keys for 1.5 s (in the
  LCD look, at the bottom of the glass). The empty History page hides its explanation while one shows.

## Calculator modes: STAT, TABLE, CALC, SOLVE, ∫dx, d/dx (2026-10-11)

All calculator screens (the classic LCD look, Casio indicator row only, no soft-key bar):

- **MODE 2: STAT** (`core/stat.*`): type list (1-VAR, A+BX, _+CX², ln X, e^X, A·B^X, A·X^B, 1/X), the
  data editor (X, Y, FREQ columns), the stat calculation line, SHIFT 1 menus (Type, Data, Edit, Sum, Var,
  Reg, MinMax). "STAT" shows in the indicator row on every screen of the mode. AC on the type list goes
  back to COMP.
- **SETUP ▼**: a second SETUP page, "1:STAT" → "Frequency? 1:ON 2:OFF" (as the fx-115ES); a change
  clears the STAT data, as on the Casio.
- **MODE 3: TABLE** (`core/table.*`): f(X)=, Start? End? Step?, the X / F(X) table (30 rows, ▲▼ in the
  indicator row). "TABLE" shows in the indicator row.
- **CALC** (`core/numeric.*`): prompts each variable ("X?"), then the result; = loops; "Y=X+1" assigns Y.
  **SHIFT CALC (SOLVE)**: "X?" (the guess), then "X=" and "L−R="; the root goes into X, Ans is unchanged;
  no root: "Can't Solve". **ALPHA CALC** types "=".
- **∫dx** types `∫(` (∫(f,a,b[,tol])), **SHIFT ∫dx** types `d/dx(` (d/dx(f,x[,tol])); ordinary = evaluation.
- Exam mode allows all of these (calculator features on the real Casio); the apps stay off.
- STAT's data and TABLE's f(X) / range survive deep sleep (`saveState()`'s mode block); MODE 4 Apps over
  STAT / TABLE come back to the mode with its data.
- `ui.cpp`: `drawModeScreen()` re-typesets the module rows and draws the 2x result (STAT result, CALC
  result, a prompt's current value) large and right-aligned like the COMP result; CALC / SOLVE prompts get
  a blinking cursor. Screenshots: `hardware/renders_ui_v15/M*.png` (`tools/ui_sim_v15`).

## MODE 5: EQN, the equation solver (2026-10-12)

The MODE menu is now `1:COMP 2:STAT / 3:TABLE 4:APPS / 5:EQN` (4 stays Apps, as decided). EQN
(`core/eqn.*`) works like the fx-115ES's, in the classic LCD look:

- **Type list**: `1:anX+bnY=cn`, `2:anX+bnY+cnZ=dn`, `3:aX²+bX+c=0`, `4:aX³+bX²+cX+d=0`.
- **Coefficient editor**: columns a b c (d), one row per equation (a polynomial has one row). Arrows
  move; a value key types an expression for the cell (any calculator expression, also A-F / X / Y / M /
  Ans), = stores it and moves on (right, then the next row); DEL zeroes the cell; = (not typing) solves.
  AC goes to the type list (AC there comes back; picking another type zeroes the grid).
- **Results**, one per screen with ▼▲ in the indicator row: `X=` `Y=` (`Z=`); `X1=` `X2=` (`X3=`), or
  `X=` for a double / triple root; complex roots as `a+bi` (as the fx-115ES shows them in EQN);
  quadratics then show `X-Value Minimum=` / `Y-Value Minimum=` (Maximum when a < 0), as the newer Casio
  models do. Fractions where they fit (`1⌟2`), S⇔D for decimals. = / ▼ step, = after the last and AC go
  back to the editor. Singular systems say `No Solution` / `Infinite Solution`; a = 0 is `Math ERROR`.
- Results are **not** stored in Ans (the Casio doesn't either). No "EQN" indicator (the fx-ES has none).
- Exam mode allows EQN. The type and coefficients survive deep sleep (`saveState()`'s 'E' block). SHIFT ▲▼
  brightness works there too. Screenshots: `hardware/renders_ui_v15/E*.png`.

## MODE 6: CMPLX, complex numbers (2026-10-12)

The MODE menu is now `1:COMP 2:STAT / 3:TABLE 4:APPS / 5:EQN 6:CMPLX`. CMPLX (`core/cmplx.*`) is the
COMP screen with complex values, like the fx-115ES's, with **CMPLX** in the indicator row:

- **Keys**: ENG types `i`; SHIFT (−) types `∠` (polar entry `r∠θ`, θ in the angle unit); SHIFT 2 opens
  `1:arg 2:Conjg 3:▶r∠θ 4:▶a+bi`; SHIFT hyp is Abs (|z|). + − × ÷, implicit ×, x², x³, x⁻¹ and ^ with an
  integer exponent work on complex values; every other function, x!, %, ⌟, ⁿ√, nPr / nCr, a non-integer
  or complex exponent are `Math ERROR` with a non-real value. `√(−1)` is `Math ERROR` (as on the Casio).
  A real value gives exactly what COMP gives (same engine: `1÷3` is `1⌟3`, `sin(30)` is `1⌟2`).
- **Display**: `3+4i`, `−i`, `1⌟2−1⌟2i` (S⇔D for decimals), or `5∠53.13010235` when SETUP ▼ `2:CMPLX` is
  `2:r∠θ` (or the expression ends in `▶r∠θ`; `▶a+bi` forces a+bi). A real result shows as in COMP. A value
  too long for one row (25 characters) shows its real part (or r) with **R⇔I**; SHIFT = shows the
  imaginary part (or ∠θ) and back.
- **Ans, A-F X Y M, PreAns** and the replay (▲) hold complex values; STO / RCL / M+ / M− too.
- **Choices to confirm**: leaving CMPLX keeps only the real parts (Ans, PreAns, the memories) and clears
  the replay (the Casio's replay is per mode); ∫dx, d/dx, CALC and SOLVE do nothing in CMPLX (the real-only
  runners can't take `i`; the fx-115ES PLUS has CALC in CMPLX); a real result stays real in r∠θ mode
  unless `▶r∠θ` asks; R⇔I sits left of the result on its bottom line (the Casio has it in the indicator
  row, which here has no room next to the exam clock).
- Exam mode allows CMPLX. The mode, the memories' imaginary parts and the replay survive deep sleep
  (`saveState()`'s 'C' block); the r∠θ choice is a SETUP flag. Screenshots: `hardware/renders_ui_v15/C*.png`.

## MODE 7: BASE-N, binary / octal / hex integers (2026-10-12)

The MODE menu is now `1:COMP 2:STAT / 3:TABLE 4:APPS / 5:EQN 6:CMPLX / 7:BASE-N`. BASE-N (`core/basen.*`)
is the COMP screen with 32-bit two's-complement integers, like the fx-115ES's, with the number mode
(**Dec**, **Hex**, **Bin** or **Oct**) in the indicator row:

- **Keys**: x² DEC, x^□ HEX, log BIN, ln OCT (DEC whenever BASE-N opens); the shown result is converted.
  (−) °'" hyp sin cos tan type the hex digits A-F (ALPHA still gives the memories). SHIFT 3 opens
  `1:and 2:or 3:xnor 4:xor 5:Not 6:Neg`, ▼ `1:d 2:h 3:b 4:o` (a prefix gives the next number's base:
  `d10+h10+b10+o10` = 36). + − × ÷, brackets, implicit ×, Ans, STO / RCL / M+ / M− work; the other
  function keys, `.`, ×10ˣ, S⇔D, ENG, CALC, SOLVE, ∫dx do nothing.
- **Ranges** (the fx-ES guide's table): DEC −2147483648..2147483647, HEX 00000000..FFFFFFFF, OCT
  00000000000..37777777777 (the upper half negative), BIN 16 bits 0000000000000000..1111111111111111
  (−32768..32767). Results show zero-padded to 8 / 16 / 11 digits, right-aligned; DEC as COMP shows integers.
- **Errors**: a number or a step outside 32 bits, a result outside the current base's range (DEC 100000
  then BIN), ÷ 0: `Math ERROR`. A digit the base hasn't (2 in BIN, A in DEC without `h`): `Syntax ERROR`.
  ÷ cuts the fraction off (7÷2 = 3, −7÷2 = −3); a memory from COMP is cut to its integer part.
- **Priorities** (fx-ES): sign and prefixes, implicit ×, × ÷, + −, and, then or / xor / xnor.
- **Choices to confirm**: `1:and 2:or 3:xnor 4:xor` numbering (the guide lists and, or, xor, xnor);
  the base name sits in the indicator row where STAT / CMPLX go; operators show without spaces
  (`1010and1100`); − typed where a value belongs is its sign (the (−) key is A here); a value from before a
  change to BIN that doesn't fit shows `Math ERROR` (also in the replay); intermediate steps may use the
  full 32 bits in BIN (only the result must fit 16).
- Exam mode allows BASE-N. The mode, the base and the replay survive deep sleep (`saveState()`'s 'N'
  block). Screenshots: `hardware/renders_ui_v15/B*.png`. Flash +5.5 KB (85.1 %).

## MODE 8 / 9: MATRIX and VECTOR (2026-10-12)

The MODE menu is now `1:COMP 2:STAT / 3:TABLE 4:APPS / 5:EQN 6:CMPLX / 7:BASE-N 8:MATRIX / 9:VECTOR`
(five rows: it fits one screen, also with the exam line on row 6). `core/matvec.*`, after the fx-115ES PLUS
user's guide ("Matrix Calculations (MATRIX)", "Vector Calculations (VECTOR)", "Errors"):

- **MATRIX** (indicator `MAT`): MatA MatB MatC up to 3×3 and MatAns. MODE 8 opens Dim's `Matrix?` list,
  then the size (`m×n?` 1:3×3 2:3×2 3:3×1 4:2×3 5:2×2 6:2×1, ▼ 1:1×3 2:1×2 3:1×1), then the editor (every
  element 0). SHIFT 4 (MATRIX): `1:Dim 2:Data 3:MatA 4:MatB 5:MatC 6:MatAns 7:det 8:Trn`. + − × (sizes must
  agree), k × M and M ÷ k, det( (square), Trn(, x⁻¹ (inverse, `Math ERROR` when singular), x² x³ (square),
  Abs( (each element). Elements stay exact fractions: the inverse of [[1,2],[3,4]] is [[−2,1],[3⌟2,−1⌟2]].
- **VECTOR** (indicator `VCT`): VctA VctB VctC of 2 or 3 dimensions and VctAns. SHIFT 5 (VECTOR):
  `1:Dim 2:Data 3:VctA 4:VctB 5:VctC 6:VctAns 7:Dot`. + −, k × V, V ÷ k, × of two 3D vectors (the cross
  product), `·` the dot product, Abs( the magnitude. The angle between two vectors is the guide's
  cos⁻¹((A·B)÷(Abs(A)Abs(B))).
- **Editors**: arrows move; a value key types an expression for the element (bottom row), = stores it and
  moves right, then to the next row; DEL sets 0; AC back to the calculation screen. STO then A / B / C
  ((−) °'" hyp) copies what an editor shows to MatA / MatB / MatC. A matrix / vector result opens the
  **MatAns / VctAns** screen (read only; + − × ÷ x⁻¹ x² x³ / + − × ÷ there give "MatAns" and the key on the
  calculation screen). AC from it shows the calculation with no value; = shows the matrix again.
  Scalar results (det, ·, Abs of a vector) show as in COMP and go to Ans.
- **Errors**: `Dim ERROR` (the guide's Dimension ERROR) for sizes that don't agree or a variable without a
  size; `Syntax ERROR` for a matrix where only a number goes (k + M, M^(2), sin(M), ÷ M); `Stack ERROR`
  past the nesting budget or 8 matrices held at once.
- Exam mode allows both. Deep sleep keeps the variables, MatAns and the replay (`saveState()`'s 'X' / 'V'
  block, 1.8 KB worst case of 2 KB). Screenshots: `hardware/renders_ui_v15/X*.png`. Flash +18.1 KB (86.0 %).
- **Choices to confirm**: entering MODE 8 / 9 clears the variables (as MODE 2 / 5 do here); Ref / Rref (only on
  the 2nd-edition guide) are not in; scalar ± matrix and M^(n) are `Syntax ERROR`; 2D × 2D is `Dim ERROR`;
  the message says `Dim ERROR`; the dot product is drawn `·`.

## Server: proxy or Worker

The calculator talks to one of two AI servers (NVS `calc/api`). **The proxy stays the default** until
the Worker is deployed and Nirav switches a board over.

| | proxy (`server/proxy`, default) | Worker (`web/api`, contract `docs/web/openapi.yaml`) |
| --- | --- | --- |
| Set up | `proxy` + `token` (typed, from the proxy's admin tool) | `api worker https://api.<domain>`: nothing else |
| Token | typed in | the calculator registers itself on its first Wi-Fi connection (`POST /v1/device/register`) and keeps the `dt_...` token in NVS (`wtoken`) |
| Solve body | Messages-API body (+ `tutor`) | `{"images":[{"media_type","data"}],"tutor":true}`: the Worker owns prompt, model and effort |
| Link screen | status + `pair/start` once | status (its `pair_code` / `link_url`, or on an older Worker the code read from its `reason`), `pair/start` only if there is none; then status every 5 s while the code is up (max 15 min), "Linked" toast when the owner enters it; once linked, a plan line from `plan_reason` |
| Firmware | `/v1/firmware` | `/v1/firmware?family=v15lcd` (the board family's channel); the manifest's `path` (with its `?family=`) is downloaded as given; an image of another family (its `family` field or version prefix) is still skipped before download |

Serial commands: `api` (show), `api worker <url>` (switch; a new address drops the old token),
`api proxy` (switch back; the proxy's address and token were kept), `register` (Worker: a new,
unlinked token and code now), `account` / `pair` (both servers). `forget` erases servers and tokens
but never the enrolment.

**Enrolment (QA, security review S1).** Only an enrolled board can start the free month. At QA run
`node web/api/scripts/enroll-board.mjs <hw_id> https://api.<domain>` (needs `ADMIN_TOKEN`; the
hw_id is the `id calc-...` in `status`) and type the line it prints, `enroll hs_...`, into the Serial
Monitor. The secret goes to NVS namespace `factory` (Forget / Unlink never erase it, it is never
printed) and is sent only in the registration; `enroll` drops the current Worker token so the board
registers again, enrolled. An unenrolled board (dev board) registers and links, but must subscribe.

**What the screen shows.** Worker errors map to the existing failures with the server's own
110-character message (never a vendor name): not linked (with the code; the Link screen gets it too),
free month over, subscription ended, no free month, monthly limit, too many solves → AI ACCOUNT;
`spend_cap` / `unavailable` / `upstream_error` / 5xx → AI BUSY; a photo too large / damaged → AI
ERROR. `401 unauthorized` (token unknown) → the token is dropped and the calculator registers again;
`401 device_revoked` → saved answers cleared, token dropped, registers again (a new code to link).

Bench build against a local `wrangler dev` on the same network: add `-DCALC_ALLOW_HTTP=1` to
`build_flags` (plain `http://` addresses are refused otherwise). PC check without a board:
`tools/worker_live/build.bat`, then `worker_live register http://127.0.0.1:8787`, `status`, `poll`,
`solve`, `typed "3x-5=16"` (a Type it PNG), `firmware` (it uses the same `core/worker_api` code; unit
tests in `tests/worker_api_tests.cpp`).

**Optional server fields (2026-10-11).** `GET /v1/device/status` may carry `pair_code` ("XXXX-XXXX",
`null` when linked), `link_url` (`<site>/link/`; a QR would be `link_url?code=<pair_code>`,
`pairQrUrl()`) and `plan_reason`. The calculator prefers them and falls back to reading `reason` when
an older server leaves them out. Settings > Link calculator, when linked, shows the plan in a pill
under "Already linked" (`Device::setLinkPlan`, text from `planStatusText()`): trial "Free month
active", active "AI tutor plan active", past_due "Payment problem: check your account",
free_month_over "Free month over: subscribe at <site>", inactive "Plan ended: subscribe at <site>",
no_trial "No free month for this calculator"; an unknown or missing `plan_reason` shows the server's
`reason`. It is the plan state, not "AI works now" (`can_solve`; the monthly limit is in `reason`).

## Self-test

`selftest` (or SHIFT + ALPHA + ON): colour bars, a grey ramp, a white border and the device id, then the
backlight sweeps 0 → 100 %. The JSON line adds `lcd_ok` (driver initialised and a frame pushed in
< 200 ms — the panel cannot answer back, so look at it), `lcd_push_ms`, `backlight_ok`, `backlight_pct`;
`display_ok` / `display_refresh_ms` are kept so the v14 bring-up checklist still reads. Other fields are
unchanged.
