# AI Calculator

## Stage 4: the full calculator

The device now works like a Casio fx-300ES PLUS in COMP mode, with the AI photo solver as mode 4 in the MODE menu. The same C++ code runs in three places:

| Where | How |
| --- | --- |
| **Browser** | `sim/web/simulator.html`: double-click to open. Clickable keypad, test bench (demo replies, hotspot/key switches, clock fast-forward, USB unlock). For real scans, pick Photo or Typed problem and paste your API key: it then sends exactly what the ESP32 sends (upright, grayscale, max 1600 px JPEG, same request, model, streaming and error handling). Direct API calls only work in this local file; the Claude artifact page blocks them. |
| **Windows** | `AI_Calc_Simulator.exe`: type on the keyboard (key list at the bottom of the window). |
| **Tester** | `firmware/`: see [firmware/README.md](firmware/README.md). |

What the calculator does:

- **COMP maths:** + − × ÷, exact fractions (1÷3 = 1⌟3, S⇔D for decimal), powers, roots, x², x³, x⁻¹, x!, nPr, nCr, log (with base), ln, eˣ, 10ˣ, sin/cos/tan and inverses, hyp menu, Abs, %, π, e, Ran#, Rnd, ENG. Casio priorities: implicit × binds tighter than ÷ (1÷2π = 1÷(2π)), (−)2² = −4, brackets close themselves at the end. 10 significant digits, Norm1/Norm2.
- **Editing:** cursor with ◀ ▶ (wraps), DEL, SHIFT DEL = INS (overwrite), ▲ ▼ replay of the last 20 sums, Ans / PreAns, carry on from Ans with an operator.
- **Errors:** Math ERROR / Syntax ERROR / Stack ERROR with [AC] Cancel and [◀][▶] Goto.
- **Memory:** STO / RCL into A–F, X, Y, M; ALPHA types a variable; M+ / M−; SHIFT 9 = CLR (Setup / Memory / All).
- **MODE:** 1 COMP, 2 STAT and 3 TABLE (later), 4 AI SOLVE.
- **SETUP (SHIFT MODE):** 1–3 Deg / Rad / Gra, 4–5 Norm1 / Norm2, 6 Wi-Fi & key, 7 Screen look (Plain / LCD dots), 8 Exam mode.
- **AI SOLVE:** = scans; answer, question and steps scroll with ▲ ▼. If the photo is a calculation, Claude also sends it as `expression` and it is **typed into the main screen** for you to check, edit and press = (AC from the answer takes you there). Unclear scans are flagged on the main screen in reverse video.
- **Exam mode:** SETUP 8 then =, or SHIFT, 7, ON while off. AI, camera and Wi-Fi off; black status bar with a padlock and the time it's been on; survives power-off and restarts. Ends only with the USB cable (`exam off` over serial) or after 12 hours. CLR All doesn't end it.
- **Power:** SHIFT AC = OFF, auto-off after 10 minutes idle.

Screen: 125×61 pixels (the 2.13" e-paper at 2×) = 25 characters × 7 rows; row 0 is the status bar (S, A, M, STO/RCL, D/R/G, Wi-Fi, AI, ▲▼).

New code: `core/calc_engine.*` (tokens, parser, exact fractions, formatting), `core/device.*` (keys, menus, modes, exam, drawing), `sim/web/` (browser build). Tests: 422 checks, `./build/tests`.

Rebuild the browser simulator after changing `core/`: install [Emscripten](https://emscripten.org), then `sim/web/build.sh`.

## Stage 2: Windows simulator (AI flow)

Software for a modified Casio fx-300ES Plus that does one thing: take a picture of a problem and show Claude's answer and the steps to it.

Stage 2 runs the whole flow in a Windows simulator with **real Claude calls**: a photo file stands in for the camera.

## Run it

1. Extract the zip anywhere.
2. Create `api_key.txt` next to `AI_Calc_Simulator.exe` containing your Claude API key (from [console.anthropic.com](https://console.anthropic.com), *API Keys*). One line, nothing else. Setting a monthly spend limit in the Console is a good idea.
3. Double-click `AI_Calc_Simulator.exe`. (Windows may show a SmartScreen warning for an unsigned app: *More info → Run anyway*.)
4. Press Enter and pick a photo of a problem (JPG, PNG, HEIC if Windows can open it).

What happens: the photo is rotated upright, shrunk to at most 1600 px, turned grayscale, and sent to Claude Sonnet 5.5. Claude may think first on harder problems. The answer appears as soon as it arrives, and the steps follow a moment later.

**No `api_key.txt`?** The simulator falls back to Stage 1's sample mode and plays back the hand-written replies in `samples/`. The key file is re-read on every request, so you can add or fix it without restarting.

**Privacy:** the photo is sent only when you press Enter and pick a file. It is kept in memory only. `api_key.txt` is listed in `.gitignore`, so it is never committed to your repo.

| Laptop key | Calculator key | What it does |
| --- | --- | --- |
| Enter or `=` | `=` | Calculate; in AI SOLVE: take a picture / retake / retry |
| Esc | `AC` | Clear, back, or cancel while solving |
| Backspace | `DEL` | Delete |
| Arrows | REPLAY pad | Cursor, history, scroll |
| `m` then `4` | MODE 4 | AI SOLVE |
| Other keys | see `keysForChar` in `core/device.h` | s c t = sin cos tan, `[` SHIFT, `]` ALPHA ... |
| F2 / F3 | — | Simulate hotspot / API key on or off |
| F4 | `ON` / `SHIFT`+`AC` | Power on/off |
| F5 | — | USB cable: ends exam mode |

If Claude isn't sure it read the photo correctly (confidence below 0.8), you see what was unclear first: press `=` to retake, or ↓ to see the answer anyway.

## Messages you might see

| Screen | Meaning |
| --- | --- |
| CLAUDE ERROR / API key was rejected | Check `api_key.txt` |
| CLAUDE BUSY | Rate limit or Claude overloaded; try again shortly |
| NO CONNECTION | No internet, or the connection dropped mid-reply |
| TIMED OUT | No reply within 2 minutes |
| CAMERA ERROR / Couldn't open that image | Windows can't decode that file type |

## What's in here

```
AI_Calc_Simulator.exe   Windows simulator (built from the source below)
samples/                Hand-written example replies in Claude's reply format
core/                   Portable C++17 device logic, shared with the future ESP32 firmware
  device.*              The whole calculator: keys, COMP editing, menus, AI SOLVE, exam mode, status bar
  calc_engine.*         Tokens, Casio-order parser, exact fractions, number formatting, typed-text parser
  app.*                 AI SOLVE screens (Ready, Solving, Warning, Answer, Message)
  claude_api.*          Claude's instructions, reply schema, request body, stream reader, error mapping
  framebuffer.*         125x61 1-bit screen buffer (the e-paper at 2x)
  font*.cpp/h           5x7 font (X11 misc-fixed, public domain) incl. Greek, sub/superscripts
  text.*                Word wrap for 25-character lines
  json.*, solve_result.*  Reading Claude's reply
sim/win32_main.cpp      Simulator: Win32 window, file dialog as camera, WIC photo shrinking, WinHTTP
sim/web/                Browser simulator: web_main.cpp (JS bridge), page.html, build.sh -> simulator.html
tests/                  Unit tests + screen snapshots (tests/golden)
tools/gen_font.py       Regenerates core/font_data.cpp
firmware/               ESP32-S3 tester firmware (PlatformIO), Stage 4
```

## Reply format

Claude replies with exactly this JSON, enforced by the API's structured output. The field order is fixed, so the answer streams in before the steps:

```json
{
  "readable": true,
  "confidence": 0.95,
  "unclear": ["line 1: '7' could be '1'"],
  "expression": "sqrt(2*9.81*5.0)",
  "answer": "v ≈ 9.90 m/s",
  "read_as": "the question as understood",
  "steps": ["step 1", "step 2"]
}
```

The instructions (in `core/claude_api.cpp`) tell Claude the screen is 25×6 characters, so the answer is kept to about 2 lines, steps to about 75 characters, in calculator notation with no LaTeX. `expression` is the calculation in typed form (`*`, `/`, `^`, `sqrt(`, `pi`, `E` for ×10ˣ), or empty for word problems.

## Build from source (optional)

The tests run on Linux or macOS: `cmake -S . -B build && cmake --build build && ./build/tests` (run from this folder).
The exe is cross-compiled with MinGW:
`x86_64-w64-mingw32-g++ -std=c++17 -O2 -static -mwindows -o AI_Calc_Simulator.exe sim/win32_main.cpp core/*.cpp -lgdi32 -lwinhttp -lwindowscodecs -lole32 -loleaut32 -lcomdlg32`
