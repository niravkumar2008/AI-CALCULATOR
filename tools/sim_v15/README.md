# Proofline v15 simulator (Windows)

Try the v15 calculator on your PC without the board. The screen you see is drawn by the
**real firmware code**: the calculator, the Apps (AI Solve, Type it, History, Notes,
Geography, Periodic Table, Settings), exam mode and every screen come from the same
source files that go onto the calculator. Only the hardware around them is pretend.

![the simulator](../../hardware/renders_ui_v15/01_calc_home.png)

## Open it

1. Double-click **`Proofline_Simulator_v15.exe`** in the top folder of the project.
2. Use the calculator by clicking its keys, or type on your PC keyboard (press **F1** in
   the simulator to see the key list).

If the `.exe` is missing or you changed the firmware code, double-click
**`tools\sim_v15\build.bat`**. It rebuilds the `.exe` in about a minute (the first build
takes a little longer). It needs the free Microsoft "Build Tools for Visual Studio"
(Desktop development with C++), which is already installed on this PC. Close the
simulator before rebuilding, or the build can't replace the `.exe`.

## The PC keyboard

| PC key | Calculator key |
|---|---|
| `0`-`9` `.` `+` `-` `*` `/` `^` `(` `)` | as printed (`*` is ×, `/` is ÷) |
| Enter | `=` |
| Backspace | DEL |
| Esc | AC |
| arrow keys | ◀ ▶ ▲ ▼ |
| `[` / `]` | SHIFT / ALPHA |
| `m` / `o` | MODE / ON |
| `s` `c` `t` | sin cos tan |
| `l` `n` | log, ln |
| `r` `q` `i` | √, x², x⁻¹ |
| `f` `~` `E` `p` | fraction, (−), ×10ˣ, π |
| `h` `b` `k` `w` `M` `a` `g` | hyp, Abs, RCL, S⇔D, M+, Ans, logₐb |
| `!` `%` `,` | x!, percent, comma |

On the **Type it** and **Notes** keyboard: letters, space, `=` and `,` type that
character (the simulator moves the cursor to it and presses `=`, like you would), small
`x` and `y` are the maths x and y. **Tab** jumps to SEND (SAVE for a note).

Other keys: **F1** key help, **F2** save a screenshot, **F5** restart, **F6** power off / on.

Quick tour: **MODE 4** opens the Apps (◀ ▶ to choose, `=` to open, AC to go back).
**SHIFT AC** switches off, **ON** switches on. **SHIFT 7 ON** while off starts exam mode.
**SHIFT MODE** is SETUP. **SHIFT ▲ / ▼** changes the brightness.

## Test controls (the panel on the right)

| Control | What it does |
|---|---|
| Battery slider | The battery level. At 4 % or less (on battery) the calculator locks out AI, camera and Wi-Fi like the real one, until it is back above 10 % or the cable is in. |
| USB cable plugged in | Charging; lifts the low-battery lock-out; when off, the screen shows "Charging" instead of going dark. |
| Wi-Fi hotspot reachable | Whether your phone's hotspot is on and in range. Untick it to see what happens with no connection. |
| Linked to an account / Link code | What the AI server would say in Settings > Link calculator. Unlinking clears History (as on the real one). |
| Start exam mode | Does SHIFT AC, then SHIFT 7 ON, for you. |
| Teacher USB unlock | What the teacher's USB cable does ("exam off"): ends exam mode. |
| Advance time +1 min / +10 min / +12 h | Jumps the clock: the screen dims and goes dark (Settings > Sleep timer), the calculator switches itself off after 10 minutes, exam mode ends after 12 hours. |
| Power off / on | ON when off, SHIFT AC when on. |
| Reset (cold boot) | Restarts the calculator. History, notes, memories and settings come back. |
| Factory reset | Erases everything the simulator saved (asks first). |
| AI reply | What the pretend AI does next: a normal answer, an unclear (low-confidence) answer, "unreadable", a timeout, a server error, or "busy". |
| Choose photo... | What the pretend camera sees in AI Solve (any JPG or PNG). The default is a photo of "3x + 7 = 22". |
| Shaky hand | Moves the camera picture, so you can see the "hold still" warning. |
| Save screenshot | Saves the calculator screen as a PNG in `tools\sim_v15\screenshots\` (Open screenshots opens that folder). |
| Screen size 2x / 3x | Makes the screen bigger. |
| The box at the bottom | A log of what the calculator is doing (keys, Wi-Fi, AI requests, saving), like the real one's USB log. |

## What is real and what is pretend

**Real (the same code as the calculator):** the whole calculator and its maths, the MODE
and SETUP menus, the Apps and their screens, History and Notes, exam mode, the colours
and layout of every screen, the camera viewfinder's overlay (focus, "hold still", the box
around the writing), the status bar, the brightness and sleep-timer behaviour, the
answer screens and their failure messages.

**Pretend:**
- **The AI.** No internet is used. After about 1.5 seconds a made-up answer arrives in the
  exact format the real AI server sends. For **Type it** it works out arithmetic
  (`2+2`) and simple equations in x (`3x+7=22`) with the calculator's own maths;
  anything else gets a polite "demo answer". For **photos** of the default picture it
  answers "x = 5"; for your own pictures it cycles through a few sample problems.
- **The camera.** The viewfinder shows the picture you chose instead of a live camera.
- **Wi-Fi.** A tick box; joining takes 1.5 seconds.
- **The battery.** A slider.
- **Saving.** What the calculator keeps in its memory chips is saved in files in
  `%APPDATA%\ProoflineSim` (`apps.bin` History and notes, `state.bin` memories, Ans,
  SETUP and exam mode, `settings.txt` brightness, sleep timer and link state). On the
  real calculator a reset or flat battery loses the memories and Ans; the simulator keeps
  them on "Reset (cold boot)" so you don't lose your test setup.
- **Timings are shorter:** the pretend AI times out after 4 s (the real one waits up to
  2 minutes) and "no connection" shows after 5 s (12-20 s on the real one).

**Not simulated:** "setup by phone" (the screen shows, but no phone can join), firmware
updates, the factory self-test (SHIFT + ALPHA + ON), the Serial Monitor commands.

## For developers

- `board.cpp` is `firmware-v15-lcd/src/main.cpp`'s loop redone for the PC (main.cpp
  needs FreeRTOS, the camera, Wi-Fi and LittleFS): each `service...()` mirrors the
  firmware function of the same name. Keep it in step when main.cpp changes.
- `host_hw.cpp` replaces `lcd.cpp` (the frame goes to the window) and the Arduino clock.
  The LovyanGFX host shims are the screenshot harness's (`tools/ui_sim_v15/shim`,
  `lgfx_host_*.cpp`), used as they are.
- `win_main.cpp` is the window, the keypad and the test panel; `wic_image.cpp` loads
  pictures and writes PNGs (Windows' own WIC).
- `Proofline_Simulator_v15.exe --selftest [folder]` runs a scripted tour (boot, 2+3=,
  MODE 4, every app, Type it, AI Solve by photo, a failure, exam mode, +12 h, restart),
  saves each screen as a PNG in `tools\sim_v15\selftest_out\` and returns 0 if every
  check passed (`start /wait Proofline_Simulator_v15.exe --selftest`, then read
  `selftest_out\selftest_log.txt`). It uses its own data folder, not yours.
- `--window-shot <file.png> [keys]` opens the window, types `keys` (`?` = help overlay),
  and saves a picture of the whole window.
