# Proofline v15 in the browser (website "Try it" page)

`dist/index.html` is one self-contained page (about 510 KB, 224 KB gzipped) that runs the
v15 calculator in a web browser. The screen is drawn by the **real firmware code**, compiled
to WebAssembly: `core/` (the calculator, the Apps, exam mode) and the v15 LCD drawing code
(`firmware-v15-lcd/src/ui*.cpp`, `fbtext.cpp`, `vf_lcd.cpp`) drawing into LovyanGFX's own
sprite. The same code as the Windows simulator (`tools/sim_v15`), with a public-demo board
around it. It goes on the website as `/try/index.html` (the site's loader looks for `.calc`).

## Build

Double-click **`build.bat`** (or run `tools\web_sim_v15\build.bat nopause`). It needs:

- the Emscripten SDK in `%USERPROFILE%\emsdk` (`emsdk install latest`, `emsdk activate latest`;
  `build.bat` runs `emsdk_env.bat` itself);
- Python 3 with Pillow (for `make_page.py`);
- LovyanGFX from the firmware's PlatformIO libdeps (`pio run -e v15lcd` once in
  `firmware-v15-lcd`), as for the Windows simulator.

Steps: LovyanGFX for WebAssembly (once, into `build\lgfx`; delete it to rebuild), then the
board + firmware drawing code + core linked into `build\sim.js` with the WebAssembly inlined
(`-sSINGLE_FILE`, `-Os`, no threads, no file system, `--closure 1`), then `make_page.py`
inlines it and the sample photo into `page.html` and writes `dist\index.html`.

`make_page.py` **fails the build** if a vendor name (claude, anthropic, sk-ant, openai,
stripe, chatgpt; any case) appears in the page source *or* in the decoded WebAssembly.

## What the demo does

- **The calculator**, exactly as on the device (classic grey-green LCD look).
- **MODE 4 → Apps**, in colour:
  - **AI Solve**: the viewfinder shows the bundled sample photo ("3x + 7 = 22"); `=` takes
    the "photo" and a demo answer arrives after about 3 s. The answers rotate through three
    worked problems (the photo's x = 5, a falling-ball speed, moles of water).
  - **Type it**: worked out offline by the calculator's own maths: arithmetic (`2+2`) and
    linear equations in x (`3x+7=22`); anything else gets a friendly "(demo answer)" that
    says the real tutor would solve it.
  - **History** and **Notes**: kept in this visitor's browser (`localStorage`, key
    `proofline.v15.apps`; every access in try/catch, works without storage).
  - **Geography**, **Periodic Table**: as on the device.
  - **Settings**: brightness and sleep timer work (on the drawn screen); Wi-Fi & key shows
    "demo"; Link calculator shows the code `DEMO-123`.
- Battery fixed at 80 %, no USB, Wi-Fi always "reachable" (joins in about 1 s).
- **No network**: `fetch`, `XMLHttpRequest`, `WebSocket`, `EventSource` and `sendBeacon`
  are blocked at the top of the page; nothing is ever sent. Works under the site's CSP
  (`script-src 'self' 'unsafe-inline' 'wasm-unsafe-eval'`).
- Not offered: setup by phone (= on Wi-Fi & key does nothing), firmware updates, the test
  panel. Exam mode still works with the device's own key combination (not advertised).

## The PC keyboard

Digits, `. + - * / ^ ( )` as printed; Enter `=` (in Type it / a note: SEND / SAVE);
Backspace DEL; Esc AC; arrows; `m` MODE, `o` ON, `[` `]` SHIFT / ALPHA, and the rest of
`calc::keysForChar()` (`s c t` sin cos tan, `l n` log ln, ...). On the Type it / note keyboard,
letters and space type themselves (small `x` is the maths x); Tab moves to SEND.

## Files

- `web_main.cpp`: the JS ↔ WebAssembly bridge (`sim_init`, `sim_step`, `sim_key`,
  `sim_type_char`, the RGBA frame, the photo buffer, a screen summary for screen readers).
- `board.cpp/.h`: `firmware-v15-lcd/src/main.cpp`'s loop for the demo, adapted from
  `tools/sim_v15/board.cpp` (keep the two in step when main.cpp changes).
- `host_hw.cpp`, `sim_host.h`: the LCD / clock stand-ins (from `tools/sim_v15`, browser clock).
- `lgfx_host_platform.cpp`: LovyanGFX's platform functions without threads (from
  `tools/ui_sim_v15`). The SDL stand-in header, panel and font stubs are used from
  `tools/ui_sim_v15` as they are.
- `page.html`: the page (calculator body in HTML/CSS, keys as buttons, the 320 x 170 screen
  as a `<canvas>`); `make_page.py`: inlines and checks it.
