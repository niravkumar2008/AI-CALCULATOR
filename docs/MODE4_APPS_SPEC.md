# MODE 4 → Apps menu (v15 firmware): spec

Written 2026-10-11 from Nirav's request ("regular calculator on startup, MODE 4 opens a custom menu with
all the options") and a frame-by-frame look at a competitor's 23-second video (VoVo Corp: a JS-82MS-A
shell with a mono LCD and "ChatGPT" built in, Hebrew UI).

## What the competitor shows
| Time | Screen |
|---|---|
| 0–6 s | Home: world-map wallpaper; a **language picker** ("Language", the current language centred) with a key legend on the left: `=` navigate, `×` select, `AC` back; small toasts when the choice changes |
| 7–9 s | **Main menu**: horizontal carousel, one app per screen, big icon + name, page dots under it. Apps: **ChatGPT**, **Settings** (gear), **Notes** (pages icon) |
| 9.5–12 s | **Inside the AI app**: a row of 4 icons with a selection box: **camera**, **keyboard**, **history** (clock), **settings** (gear) |
| 10–11 s | **Keyboard**: an on-screen 3-row character grid; a cursor box moves over it and a key types the letter |
| 12.5–15 s | "Preparing camera / One moment", then a **live viewfinder** on the LCD |

Not copied: the language picker (Nirav: **English only**), the world-map wallpaper, the vendor's AI branding.

## Owner decisions (2026-10-11)
- **Match the competitor's capabilities, not its interface.** Same kinds of features; our own layout, icons,
  wording, colours, key legend and order. Never copy their screens, icons, wallpaper or names.
- **English only.** No language picker.
- **Colour only inside Apps.** On the v15 colour LCD, the calculator (COMP, MODE menu, SETUP, errors, exam
  mode screens) looks like a classic mono calculator LCD: a flat grey-green "LCD" background with near-black
  pixels and no colour accents. **2026-10-11:** laid out exactly like the Casio: no soft-key bar, only the
  Casio indicators (S A M STO RCL D/R/G ▲▼) on top, no battery % / Wi-Fi / "AI" (a small battery only when
  low or charging; exam mode keeps its inverted EXAM bar). Full colour starts when
  the Apps menu opens (MODE 4) and stays in every app (AI Solve, Type it, History, Notes, Geography, Periodic
  Table, Settings); leaving Apps returns to the classic look.
- **Apps (7):** 1 AI Solve, 2 Type it, 3 History, 4 Notes, 5 Geography (world map, countries, capitals,
  quiz), 6 Periodic Table (TI-84 Plus CE "Periodic"-style: table, element details, list/sort, search,
  category highlight, property graphs), 7 Settings. All off in exam mode.

## What we build
**Startup and the calculator do not change.** The calculator starts in COMP exactly as today.
MODE still opens the Casio-style mode list (1:COMP 2:STAT 3:TABLE 4:…); **4 is now "4:APPS"** and
opens the Apps menu instead of jumping straight into AI Solve. (2026-10-12: the list gained `5:EQN`, the
fx-115ES equation solver, after APPS: 4 stays APPS. See firmware-v15-lcd/README.md.)

### Apps menu (carousel)
- One app per screen: large icon, name, one-line description, page dots, and a key legend at the bottom:
  `◀ ▶ move   = open   AC back`. Digits 1–5 jump straight to an app. It remembers the last app.
- `AC` (or MODE 1) goes back to the calculator with its expression intact.
- Order:
  1. **AI Solve**: the existing photo flow, unchanged ("Getting the camera ready…", viewfinder, `=` to shoot,
     answer + steps, tutor mode, etc.). AC from AI Solve returns to the Apps menu.
  2. **Type it**: on-screen keyboard (grid of A–Z, 0–9, space and maths symbols `+ − × ÷ = ( ) ^ √ π x y . ,`,
     plus cells for ⌫ and SEND). Arrows move the cursor box, `=` types the cell, `DEL` deletes, digit keys
     type digits directly, AC leaves (asks first if text was typed). Max 160 characters, shown wrapped.
     SEND renders the typed text as a clean black-on-white PNG (the core's font, scaled up) and sends it
     through the **existing photo solve request**, so the server, its limits and the contract don't change.
     The answer shows exactly like a photo answer. Only on a platform that sends that PNG
     (`setTypedSolveSupported(true)`: the v15); elsewhere TYPE IT says "Not available on this model".
  3. **History**: the last 20 AI answers saved on the calculator, newest first (one line each: the
     answer, short); `=` opens the full answer and steps; "Clear history?" at the end of the list, with a
     confirm. Saved in flash (survives power-off), cleared when the calculator is unlinked.
  4. **Notes**: up to 10 notes of up to 160 characters, typed with the same keyboard. List (first line of
     each), `=` opens, then EDIT / DELETE; "+ New note" at the top. Saved in flash.
  5. **Settings**: brightness, screen sleep timer, Wi-Fi (status; set up), link code (the existing
     SETUP 9 screen), battery, software version and update, about ("Proofline"). These reuse the
     existing SETUP pages wherever one exists; nothing is duplicated.
- **Toasts**: one-line pop-ups for 1.5 s ("Saved", "Deleted", "Wi-Fi connected", "No Wi-Fi").

### Exam mode wins
While exam mode is on, MODE 4 shows "APPS ARE OFF / Exam mode is on." and nothing else. AI Solve, Type it,
History and Notes can't be opened and **stored notes and history can't be read** (a stored note is an
exam cheat sheet). Starting exam mode closes any app. Exam mode's existing behaviour is unchanged.

### Persistence (platform)
History and notes are stored by the platform (NVS on the ESP32) as small serialized blobs the core produces
and reads back (`saveApps()` / `restoreApps()`), separate from the 2 KB RTC state. A damaged blob is
ignored, never a crash. The blob is at most 40 KB (`Device::kMaxAppsBytes`; a saved reply at most 2 KB,
older answers saved as question and answer only when needed); a failed save shows "Couldn't save".

## Done means
- Core: state machine + text render in `core/`, unit tests in `tests/tests.cpp` (all old 557 tests still
  pass), golden screens for the new views.
- v15: colour screens in `firmware-v15-lcd/src/ui.cpp` (icons drawn as vector shapes, the brand's
  orange `#e07a4f` accent on navy `#10141c`), NVS persistence and the typed-solve path in `main.cpp`,
  screenshots of every new screen from `tools/ui_sim_v15`, and `pio run` builds if PlatformIO is installed.
- v14 (`firmware-prototype`) still builds and still works (its e-paper shows the core's text render).
