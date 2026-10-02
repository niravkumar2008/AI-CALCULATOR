# Graph Report - AI-CALCULATOR  (2026-10-02)

## Corpus Check
- 122 files · ~389,862 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 44 file(s) not represented in the graph (top: .kicad_mod 11, (none) 10, .kicad_sch 7)

## Summary
- 2249 nodes · 4229 edges · 123 communities (111 shown, 12 thin omitted)
- Extraction: 90% EXTRACTED · 10% INFERRED · 0% AMBIGUOUS · INFERRED: 426 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `5d65ef55`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Parser
- Parser
- Tok
- Tok
- Claude outputs/sim/web/web_main.cpp
- Device
- Device
- DKey
- DKey
- Framebuffer
- Json
- Json
- Framebuffer
- sim/web/web_main.cpp
- claudeSolve
- firmware/src/camera.cpp
- firmware/src/main.cpp
- Claude outputs/core/device.cpp
- AI Answer Flow
- core/device.cpp
- App
- tests/tests.cpp
- App
- AI Answer Flow
- Claude outputs/firmware/src/claude_client.h
- firmware/src/preview.cpp
- previewResume
- core/claude_api.cpp
- firmware/src/settings.cpp
- arduino
- Claude outputs/firmware/src/main.cpp
- core/app.cpp
- Claude outputs/core/app.cpp
- AI Calculator Simulator page (keypad + test bench)
- View
- View
- SerialCmd
- Breadboard Wiring Diagram (exact, camera up)
- Kind
- fusion_run.py
- Stage 4: full calculator
- Event
- string
- Tester firmware (Stage 4)
- Event
- string
- Vars
- Claude outputs/firmware/src/camera.cpp
- Claude outputs/firmware/src/settings.cpp
- SerialCmd
- Vars
- Decode
- keys.cpp
- CalcError
- SolveResult
- Focus
- firmware-prototype/src/main.cpp
- SolveResult
- firmware-prototype/src/camera.cpp
- firmware-prototype/src/preview.cpp
- firmware-prototype/src/settings.cpp
- errorText
- AI SOLVE mode (MODE 4)
- Decode
- core static library (core/*.cpp, C++17)
- Screen
- Failure
- Screen
- Failure
- loop
- Key
- Key
- Stage 2: Windows simulator AI flow
- wndProc
- HistoryItem
- build_case.py
- SerialCmd
- Button
- Ask
- Com
- keyCalc
- Calculator Mode
- Line
- CalcError
- wndProc
- Com
- Claude outputs/tests/tests.cpp
- Calculator Mode
- AngleUnit
- Mode
- ScanNote
- Menu System
- Kind
- Mode
- ScanNote
- Effort
- Menu System
- Line
- press
- Event
- press
- equals
- Prototype firmware (the board inside the Casio)
- graphify skill trigger (/graphify)
- render
- Glyph
- Claude outputs/sim/web/build.sh
- render
- sim/web/build.sh
- Tuning
- HistoryItem
- AngleUnit
- Button
- cmath
- Tuning
- Kind
- SendStep
- NormMode
- Glyph
- NormMode
- onWifiEvent

## God Nodes (most connected - your core abstractions)
1. `Device` - 104 edges
2. `Device` - 99 edges
3. `Tok` - 71 edges
4. `Tok` - 71 edges
5. `DKey` - 52 edges
6. `DKey` - 52 edges
7. `App` - 50 edges
8. `App` - 45 edges
9. `Parser` - 42 edges
10. `Parser` - 42 edges

## Surprising Connections (you probably didn't know these)
- `Simulator page (Claude outputs, older: no whereBanner/Scan now)` --semantically_similar_to--> `AI Calculator Simulator page (keypad + test bench)`  [INFERRED] [semantically similar]
  Claude outputs/sim/web/page.html → sim/web/page.html
- `directScan()` --semantically_similar_to--> `src/claude_client (HTTPS streaming to Claude)`  [INFERRED] [semantically similar]
  sim/web/page.html → firmware/README.md
- `Tester firmware README (Claude outputs, older copy without preview/scan pipeline)` --semantically_similar_to--> `Tester firmware (Stage 4)`  [INFERRED] [semantically similar]
  Claude outputs/firmware/README.md → firmware/README.md
- `CMake build config (Claude outputs copy)` --semantically_similar_to--> `core static library (core/*.cpp, C++17)`  [INFERRED] [semantically similar]
  Claude outputs/CMakeLists.txt → CMakeLists.txt
- `directScan()` --conceptually_related_to--> `Error screens (CLAUDE ERROR, BUSY, NO CONNECTION, TIMED OUT, CAMERA ERROR)`  [INFERRED]
  sim/web/page.html → README.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **AI Calculator Hardware Circuit (ESP32-S3-CAM + e-paper + buttons on two breadboards)** — claude_outputs_bb_exact_esp32_s3_cam, claude_outputs_bb_exact_epaper_2_13, claude_outputs_bb_exact_calculator_buttons, claude_outputs_bb_exact_long_breadboard, claude_outputs_bb_exact_medium_breadboard [EXTRACTED 1.00]
- **Browser simulator scan request flow** — sim_web_page_servicerequests, sim_web_page_startscan, sim_web_page_directscan, sim_web_page_claudescan, sim_web_page_demoscan, sim_web_page_wasm_api_bridge [EXTRACTED 1.00]
- **Same C++ core runs in browser, Windows and ESP32** — cmakelists_core_library, sim_web_simulator_bundle, readme_windows_simulator_exe, firmware_readme_tester_firmware [EXTRACTED 1.00]
- **AI Solve Pipeline** — claude_outputs_tests_golden_ready_ready_screen, claude_outputs_tests_golden_busy_reading_busy_reading_screen, claude_outputs_tests_golden_busy_solving_busy_solving_screen, claude_outputs_tests_golden_answer_streaming_answer_streaming_screen, claude_outputs_tests_golden_ai_answer_ai_answer_screen [INFERRED 0.75]
- **Exam Mode Activation Flow** — claude_outputs_tests_golden_exam_confirm_exam_confirm_screen, claude_outputs_tests_golden_exam_wifi_off_exam_wifi_off_screen, claude_outputs_tests_golden_exam_status_bar_exam_status_bar_screen [INFERRED 0.75]
- **Problem capture quality: hold orientation, preview, preprocessing** — claude_outputs_how_to_hold_camera_orientation, claude_outputs_how_to_hold_preview_check, claude_outputs_cmp_image_preprocessing, claude_outputs_cmp_sample_math_problems [INFERRED 0.75]
- **AI Capture-to-Answer Pipeline** — tests_golden_ai_ready_ai_ready_screen, tests_golden_ai_hold_countdown_ai_hold_countdown_screen, tests_golden_ai_hold_captured_ai_hold_captured_screen, tests_golden_ai_hold_done_ai_hold_done_screen, tests_golden_busy_reading_busy_reading_screen, tests_golden_busy_solving_busy_solving_screen, tests_golden_answer_streaming_answer_streaming_screen, tests_golden_ai_answer_ai_answer_screen [INFERRED 0.85]
- **Exam Mode Lockdown** — tests_golden_exam_confirm_exam_mode_confirm_screen, tests_golden_exam_status_bar_exam_status_bar_screen, tests_golden_exam_wifi_off_exam_wifi_off_screen [INFERRED 0.85]
- **Firmware capture and send pipeline** — firmware_readme_main, firmware_readme_camera, firmware_readme_scan_pipeline, firmware_readme_claude_client, firmware_readme_root_ca [INFERRED 0.85]
- **Same wiring drawn in three orientations** — claude_outputs_bb_exact_wiring_diagram, claude_outputs_bb_real_wiring_diagram, claude_outputs_bb_top_wiring_diagram [INFERRED 0.95]

## Communities (123 total, 12 thin omitted)

### Community 0 - "Parser"
Cohesion: 0.07
Nodes (46): addOk(), Alias, text, tok, asInt(), endsOperand(), errorText(), evaluate() (+38 more)

### Community 1 - "Parser"
Cohesion: 0.08
Nodes (40): addOk(), Alias, text, tok, asInt(), endsOperand(), evaluate(), exactPlus() (+32 more)

### Community 2 - "Tok"
Cohesion: 0.03
Nodes (71): Tok, Abs, Acos, Acosh, Add, Ans, Asin, Asinh (+63 more)

### Community 3 - "Tok"
Cohesion: 0.03
Nodes (71): Tok, Abs, Acos, Acosh, Add, Ans, Asin, Asinh (+63 more)

### Community 4 - "Claude outputs/sim/web/web_main.cpp"
Cohesion: 0.06
Nodes (52): base64Encode(), buildSolveRequest(), classifyFailure(), jsonString(), peekAnswer(), solveInstructions(), solveSchema(), StreamReader (+44 more)

### Community 5 - "Device"
Cohesion: 0.04
Nodes (36): Device, alpha_, angle_, app_, back_, blink_, clrChoice_, continueWith_ (+28 more)

### Community 6 - "Device"
Cohesion: 0.03
Nodes (38): Device, alpha_, angle_, app_, back_, battery_, blink_, charging_ (+30 more)

### Community 7 - "DKey"
Cohesion: 0.04
Nodes (52): DKey, AC, Add, Alpha, Ans, Close, Cos, Count (+44 more)

### Community 8 - "DKey"
Cohesion: 0.04
Nodes (52): DKey, Abs, AC, Add, Alpha, Ans, Close, Cos (+44 more)

### Community 9 - "Framebuffer"
Cohesion: 0.09
Nodes (30): drawCalc, decodeUtf8(), encodeUtf8(), findGlyph(), hasGlyph(), lookup(), normalizeCodepoint(), Framebuffer (+22 more)

### Community 10 - "Json"
Cohesion: 0.08
Nodes (17): Json, a_, b_, n_, o_, parse, s_, JsonParser (+9 more)

### Community 11 - "Json"
Cohesion: 0.08
Nodes (17): Json, a_, b_, n_, o_, parse, s_, JsonParser (+9 more)

### Community 12 - "Framebuffer"
Cohesion: 0.09
Nodes (29): drawCalc, decodeUtf8(), encodeUtf8(), findGlyph(), hasGlyph(), lookup(), normalizeCodepoint(), Framebuffer (+21 more)

### Community 13 - "sim/web/web_main.cpp"
Cohesion: 0.10
Nodes (35): jsonEscape(), numText(), sim_active_request(), sim_alloc(), sim_api_url(), sim_api_version(), sim_build_request(), sim_classify() (+27 more)

### Community 14 - "claudeSolve"
Cohesion: 0.13
Nodes (12): claudeSolve(), lower(), readLine(), writeAll(), claudeSolve(), lower(), readLine(), writeAll() (+4 more)

### Community 15 - "firmware/src/camera.cpp"
Cohesion: 0.12
Nodes (25): applyAll(), autoTuneLocked(), cameraAutoTune(), cameraBegin(), cameraCapture(), cameraFocusScore(), cameraFrame(), cameraHoldEstimateMs() (+17 more)

### Community 16 - "firmware/src/main.cpp"
Cohesion: 0.10
Nodes (30): cameraHoldUntil(), applySettings(), connectWifi(), Event, confidence, failure, id, kind (+22 more)

### Community 17 - "Claude outputs/core/device.cpp"
Cohesion: 0.15
Nodes (21): continuesAns(), clearEntry, endExam, insert, keyAi, leaveAi, notice, onKey (+13 more)

### Community 18 - "AI Answer Flow"
Cohesion: 0.07
Nodes (34): AI Answer Screen, AI Answer Flow, AI Error States, AI Hold-to-Capture Flow, AI Hold Captured Screen, AI Hold Countdown Screen, AI Hold Done Screen, AI Ready Screen (+26 more)

### Community 19 - "core/device.cpp"
Cohesion: 0.15
Nodes (23): continuesAns(), clearEntry, endExam, insert, keyAi, keyCalc, leaveAi, notice (+15 more)

### Community 20 - "App"
Cohesion: 0.07
Nodes (18): App, activeId_, busySinceMs_, captured_, expression_, hasKey_, holdUntilMs_, kUnclearThreshold (+10 more)

### Community 21 - "tests/tests.cpp"
Cohesion: 0.16
Nodes (20): calcText(), keys(), main(), press(), readFile(), sampleStream(), snapshot(), snapshotFb() (+12 more)

### Community 22 - "App"
Cohesion: 0.08
Nodes (17): App, activeId_, busySinceMs_, captured_, expression_, hasKey_, kUnclearThreshold, lines_ (+9 more)

### Community 23 - "AI Answer Flow"
Cohesion: 0.11
Nodes (27): AI Answer Screen, Scroll Indicator Icons, AI Ready Screen, Chemistry Answer Screen, Chemistry Answer Steps Screen, Physics Answer End Screen, Physics Answer Screen, Answer Streaming Screen (+19 more)

### Community 24 - "Claude outputs/firmware/src/claude_client.h"
Cohesion: 0.09
Nodes (15): SolveCallbacks, cancelled, fail, partial, reply, SolveCallbacks, cancelled, fail (+7 more)

### Community 26 - "firmware/src/preview.cpp"
Cohesion: 0.37
Nodes (16): authorised(), autotuneHandler(), detailHandler(), focusHandler(), frameHandler(), lastHandler(), noCache(), pageHandler() (+8 more)

### Community 28 - "previewResume"
Cohesion: 0.29
Nodes (9): endPreview(), jsonText(), previewResume(), previewSetResult(), previewStart(), previewSuspend(), startNetwork(), startServers() (+1 more)

### Community 29 - "core/claude_api.cpp"
Cohesion: 0.14
Nodes (13): base64Encode(), buildSolveRequest(), classifyFailure(), jsonString(), peekAnswer(), StreamReader, buf_, errorMessage_ (+5 more)

### Community 30 - "firmware/src/settings.cpp"
Cohesion: 0.18
Nodes (15): cameraName(), setup(), statusText(), Ask, Key, Nothing, Password, Ssid (+7 more)

### Community 31 - "arduino"
Cohesion: 0.13
Nodes (7): draw(), screenShow(), draw(), screenShow(), draw(), screenBegin(), screenShow()

### Community 32 - "Claude outputs/firmware/src/main.cpp"
Cohesion: 0.20
Nodes (13): cameraCapture(), applySettings(), connectWifi(), handleEvent(), loop(), pollButtons(), post(), press() (+5 more)

### Community 33 - "core/app.cpp"
Cohesion: 0.18
Nodes (15): accepts, effortParam, onCaptured, onFailure, onKey, onPartialAnswer, onReply, scrollBy (+7 more)

### Community 34 - "Claude outputs/core/app.cpp"
Cohesion: 0.20
Nodes (14): accepts, onCaptured, onFailure, onKey, onPartialAnswer, onReply, scrollBy, setLines (+6 more)

### Community 35 - "AI Calculator Simulator page (keypad + test bench)"
Cohesion: 0.32
Nodes (15): claudeScan(), demoScan(), directScan(), done(), draw(), loop(), press(), serviceRequests() (+7 more)

### Community 36 - "View"
Cohesion: 0.13
Nodes (14): View, Ai, Calc, Clr, ClrConfirm, Error, ExamInfo, Hyp (+6 more)

### Community 37 - "View"
Cohesion: 0.13
Nodes (14): View, Ai, Calc, Clr, ClrConfirm, Error, ExamInfo, Hyp (+6 more)

### Community 38 - "SerialCmd"
Cohesion: 0.13
Nodes (13): SerialCmd, Changed, ExamOff, Keys, None, Preview, PreviewOff, Scan (+5 more)

### Community 39 - "Breadboard Wiring Diagram (exact, camera up)"
Cohesion: 0.23
Nodes (14): Calculator Buttons (=, AC, Up, Down), 2.13in E-Paper Display, ESP32-S3-CAM Board, LONG Breadboard, MEDIUM Breadboard, Breadboard Wiring Diagram (exact, camera up), Breadboard Wiring Diagram (real orientation, USB away), Breadboard Wiring Diagram (top, USB toward you) (+6 more)

### Community 40 - "Kind"
Cohesion: 0.40
Nodes (5): Kind, Captured, Fail, Partial, Reply

### Community 42 - "Stage 4: full calculator"
Cohesion: 0.16
Nodes (14): src/claude_client (HTTPS streaming to Claude), src/main.cpp (buttons, serial keys, Wi-Fi, capture+send task), src/pins.h (pin map), src/root_ca.h (api.anthropic.com certificates), src/screen (125x61 to 250x122 e-paper, Plain/LCD dots), src/settings (hotspot, key, exam state in flash), COMP maths engine (exact fractions, Casio priorities, 10 sig digits), core/calc_engine (tokens, parser, fractions, formatting) (+6 more)

### Community 43 - "Event"
Cohesion: 0.21
Nodes (11): Event, confidence, failure, id, kind, text, failureForWinHttp(), photoToJpeg() (+3 more)

### Community 44 - "string"
Cohesion: 0.23
Nodes (11): clockText(), drawMenu, drawStatus, engText, onReply, pullScan, render, resultText (+3 more)

### Community 45 - "Tester firmware (Stage 4)"
Cohesion: 0.17
Nodes (13): Tester firmware README (Claude outputs, older copy without preview/scan pipeline), AI Calculator README (Claude outputs, older Stage 4 copy), Simulator page (Claude outputs, older: no whereBanner/Scan now), simulator.html bundle (Claude outputs), simulator.html (Claude outputs root, identical to sim/web copy), ESP32-S3-CAM board (OV3660, N16R8), PlatformIO build/upload, Preview mode (board AP, live camera, Send to Claude, Focus score) (+5 more)

### Community 46 - "Event"
Cohesion: 0.17
Nodes (11): Event, confidence, failure, id, kind, text, Kind, Captured (+3 more)

### Community 47 - "string"
Cohesion: 0.23
Nodes (11): clockText(), drawMenu, drawStatus, engText, onReply, pullScan, render, resultText (+3 more)

### Community 48 - "Vars"
Cohesion: 0.17
Nodes (12): Vars, a, ans, b, c, d, e, f (+4 more)

### Community 49 - "Claude outputs/firmware/src/camera.cpp"
Cohesion: 0.32
Nodes (3): cameraBegin(), configFor(), sensorName()

### Community 50 - "Claude outputs/firmware/src/settings.cpp"
Cohesion: 0.26
Nodes (11): cameraName(), setup(), statusText(), screenBegin(), examLoad(), handle(), help(), save() (+3 more)

### Community 51 - "SerialCmd"
Cohesion: 0.17
Nodes (10): SerialCmd, Changed, ExamOff, Keys, None, Snap, Settings, apiKey (+2 more)

### Community 52 - "Vars"
Cohesion: 0.17
Nodes (12): Vars, a, ans, b, c, d, e, f (+4 more)

### Community 53 - "Decode"
Cohesion: 0.17
Nodes (11): Decode, cx, cy, fullH, fullW, jpeg, lastYield, out (+3 more)

### Community 54 - "keys.cpp"
Cohesion: 0.16
Nodes (11): beginOn(), keysBegin(), keysPoll(), pollOn(), Repeat, key, next, repeats() (+3 more)

### Community 55 - "CalcError"
Cohesion: 0.20
Nodes (10): CalcError, Argument, Math, None, Stack, Syntax, EvalResult, error (+2 more)

### Community 56 - "SolveResult"
Cohesion: 0.20
Nodes (8): SolveResult, answer, confidence, expression, readable, readAs, steps, unclear

### Community 57 - "Focus"
Cohesion: 0.11
Nodes (24): analyseFocus(), Crop, h, use, w, x, y, detailCrop() (+16 more)

### Community 58 - "firmware-prototype/src/main.cpp"
Cohesion: 0.07
Nodes (33): cameraName(), batteryPercent(), charging(), Event, confidence, failure, id, kind (+25 more)

### Community 59 - "SolveResult"
Cohesion: 0.18
Nodes (9): SolveResult, answer, choice, confidence, expression, readable, readAs, steps (+1 more)

### Community 60 - "firmware-prototype/src/camera.cpp"
Cohesion: 0.10
Nodes (30): applyAll(), autoTuneLocked(), cameraAutoTune(), cameraBegin(), cameraCapture(), cameraFocusScore(), cameraFrame(), cameraHoldEstimateMs() (+22 more)

### Community 61 - "firmware-prototype/src/preview.cpp"
Cohesion: 0.31
Nodes (16): authorised(), autotuneHandler(), detailHandler(), focusHandler(), frameHandler(), lastHandler(), noCache(), pageHandler() (+8 more)

### Community 62 - "firmware-prototype/src/settings.cpp"
Cohesion: 0.21
Nodes (11): Ask, Key, Nothing, Password, Ssid, handle(), help(), save() (+3 more)

### Community 63 - "errorText"
Cohesion: 0.22
Nodes (4): errorText(), Fail, e, pos

### Community 64 - "AI SOLVE mode (MODE 4)"
Cohesion: 0.22
Nodes (9): BEST GUESS (x% sure) display, Careful scan (exposure/gain/contrast sweep scored on ink edges), Scan photo pipeline (careful scan, hold still, 8 frames sharpest, zoom+enhance), AI SOLVE mode (MODE 4), Unclear-scan warning (confidence < 0.8), core/app (AI SOLVE screens), core/claude_api (instructions, schema, request, stream reader, error mapping), Error screens (CLAUDE ERROR, BUSY, NO CONNECTION, TIMED OUT, CAMERA ERROR) (+1 more)

### Community 65 - "Decode"
Cohesion: 0.18
Nodes (10): Decode, cx, cy, fullH, fullW, lastYield, out, outH (+2 more)

### Community 66 - "core static library (core/*.cpp, C++17)"
Cohesion: 0.29
Nodes (8): AI_Calc_Simulator.html (identical copy of sim/web/simulator.html), CMake build config (Claude outputs copy), core static library (core/*.cpp, C++17), Win32 simulator executable (sim/win32_main.cpp), tests executable (tests/tests.cpp), Browser simulator (Emscripten build of core), AI_Calc_Simulator.exe (Windows simulator), simulator.html (page.html + Emscripten-compiled core)

### Community 67 - "Screen"
Cohesion: 0.25
Nodes (7): Screen, Busy, Message, Off, Ready, Result, Warning

### Community 68 - "Failure"
Cohesion: 0.25
Nodes (8): Failure, ApiBusy, ApiError, BadReply, Camera, NoApiKey, NoConnection, Timeout

### Community 69 - "Screen"
Cohesion: 0.25
Nodes (7): Screen, Busy, Message, Off, Ready, Result, Warning

### Community 70 - "Failure"
Cohesion: 0.25
Nodes (8): Failure, ApiBusy, ApiError, BadReply, Camera, NoApiKey, NoConnection, Timeout

### Community 71 - "loop"
Cohesion: 0.12
Nodes (26): cameraHoldUntil(), cameraSleep(), keysSleepUntilPress(), applySettings(), connectWifi(), loop(), previewEnded(), scanWifi() (+18 more)

### Community 72 - "Key"
Cohesion: 0.29
Nodes (7): Key, AC, Down, Eq, Off, On, Up

### Community 73 - "Key"
Cohesion: 0.29
Nodes (7): Key, AC, Down, Eq, Off, On, Up

### Community 74 - "Stage 2: Windows simulator AI flow"
Cohesion: 0.29
Nodes (7): src/camera (grayscale 1600x1200 JPEG), Claude Sonnet 5.5 model, Photo preprocessing (upright, grayscale, max 1600 px JPEG), Sample mode fallback (samples/ replies when no api_key.txt), Stage 2: Windows simulator AI flow, DEMOS canned replies, toJpeg()

### Community 75 - "wndProc"
Cohesion: 0.22
Nodes (4): exifOrientation(), handleEvent(), paint(), wndProc()

### Community 76 - "HistoryItem"
Cohesion: 0.33
Nodes (3): HistoryItem, expr, value

### Community 77 - "build_case.py"
Cohesion: 0.06
Nodes (54): appearance(), _blocks(), body_named(), chamfer(), check_outline(), circle(), cm(), comp_named() (+46 more)

### Community 78 - "SerialCmd"
Cohesion: 0.13
Nodes (13): SerialCmd, Changed, ExamOff, Keys, None, Preview, PreviewOff, Scan (+5 more)

### Community 79 - "Button"
Cohesion: 0.40
Nodes (5): Button, key, lastMs, pin, pressed

### Community 80 - "Ask"
Cohesion: 0.40
Nodes (5): Ask, Key, Nothing, Password, Ssid

### Community 82 - "keyCalc"
Cohesion: 0.48
Nodes (5): equals, keyCalc, showError, showValue, valueForMemory

### Community 83 - "Calculator Mode"
Cohesion: 0.40
Nodes (5): Calc Editing Screen, Calculator Mode, Calc Math Error Screen, Calc Fraction Result Screen, Calc STO Screen

### Community 84 - "Line"
Cohesion: 0.40
Nodes (3): Line, header, text

### Community 85 - "CalcError"
Cohesion: 0.20
Nodes (10): CalcError, Argument, Math, None, Stack, Syntax, EvalResult, error (+2 more)

### Community 86 - "wndProc"
Cohesion: 0.22
Nodes (4): exifOrientation(), handleEvent(), paint(), wndProc()

### Community 88 - "Claude outputs/tests/tests.cpp"
Cohesion: 0.18
Nodes (15): keys(), main(), press(), readFile(), sampleStream(), snapshot(), snapshotFb(), testDevice() (+7 more)

### Community 89 - "Calculator Mode"
Cohesion: 0.40
Nodes (5): Calculator Editing Screen, Calculator Math Error Screen, Calculator Fraction Result Screen, Calculator STO Screen, Calculator Mode

### Community 90 - "AngleUnit"
Cohesion: 0.50
Nodes (4): AngleUnit, Deg, Gra, Rad

### Community 91 - "Mode"
Cohesion: 0.50
Nodes (3): Mode, Ai, Comp

### Community 92 - "ScanNote"
Cohesion: 0.50
Nodes (4): ScanNote, None, Ok, Unclear

### Community 93 - "Menu System"
Cohesion: 0.50
Nodes (4): HYP Menu Screen, Menu System, Mode Menu Screen, Setup Menu Screen

### Community 94 - "Kind"
Cohesion: 0.40
Nodes (5): Kind, Captured, Fail, Partial, Reply

### Community 95 - "Mode"
Cohesion: 0.50
Nodes (3): Mode, Ai, Comp

### Community 96 - "ScanNote"
Cohesion: 0.50
Nodes (4): ScanNote, None, Ok, Unclear

### Community 97 - "Effort"
Cohesion: 0.40
Nodes (4): Effort, Careful, Max, Normal

### Community 98 - "Menu System"
Cohesion: 0.50
Nodes (4): Hyperbolic Menu Screen, Mode Menu Screen, Setup Menu Screen, Menu System

### Community 99 - "Line"
Cohesion: 0.67
Nodes (3): Line, header, text

### Community 100 - "press"
Cohesion: 0.33
Nodes (3): loadSettings(), press(), WinMain()

### Community 101 - "Event"
Cohesion: 0.15
Nodes (12): advanceRequest(), Event, confidence, failure, id, kind, text, failureForWinHttp() (+4 more)

### Community 102 - "press"
Cohesion: 0.22
Nodes (5): advanceRequest(), loadSettings(), press(), readFile(), WinMain()

### Community 103 - "equals"
Cohesion: 0.40
Nodes (4): equals, showError, showValue, valueForMemory

### Community 104 - "Prototype firmware (the board inside the Casio)"
Cohesion: 0.40
Nodes (4): Differences from the tester, Hardware it expects (from the schematic), Prototype firmware (the board inside the Casio), Upload

### Community 107 - "Glyph"
Cohesion: 0.67
Nodes (3): Glyph, cp, rows

### Community 111 - "Tuning"
Cohesion: 0.40
Nodes (5): Tuning, def, max, min, name

### Community 112 - "HistoryItem"
Cohesion: 0.33
Nodes (3): HistoryItem, expr, value

### Community 113 - "AngleUnit"
Cohesion: 0.50
Nodes (4): AngleUnit, Deg, Gra, Rad

### Community 114 - "Button"
Cohesion: 0.33
Nodes (6): Button, key, lastMs, nextRepeat, pin, pressed

### Community 116 - "Tuning"
Cohesion: 0.40
Nodes (5): Tuning, def, max, min, name

### Community 117 - "Kind"
Cohesion: 0.40
Nodes (5): Kind, Captured, Fail, Partial, Reply

### Community 118 - "SendStep"
Cohesion: 0.50
Nodes (4): SendStep, Idle, Solving, WaitWifi

### Community 119 - "NormMode"
Cohesion: 0.67
Nodes (3): NormMode, Norm1, Norm2

### Community 120 - "Glyph"
Cohesion: 0.67
Nodes (3): Glyph, cp, rows

### Community 121 - "NormMode"
Cohesion: 0.67
Nodes (3): NormMode, Norm1, Norm2

## Ambiguous Edges - Review These
- `AI Hold-to-Capture Flow` → `Exam Mode`  [AMBIGUOUS]
  tests/golden/exam_confirm.txt · relation: conceptually_related_to

## Knowledge Gaps
- **802 isolated node(s):** `On`, `Off`, `AC`, `Eq`, `Up` (+797 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 1028 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **12 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `AI Hold-to-Capture Flow` and `Exam Mode`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **Why does `Device` connect `Device` to `View`, `string`, `Framebuffer`, `string`, `HistoryItem`, `Claude outputs/core/device.cpp`, `keyCalc`, `Claude outputs/firmware/src/claude_client.h`, `Mode`, `ScanNote`?**
  _High betweenness centrality (0.096) - this node is a cross-community bridge._
- **Why does `Device` connect `Device` to `ScanNote`, `View`, `equals`, `Framebuffer`, `string`, `HistoryItem`, `core/device.cpp`, `string`, `Mode`, `errorText`?**
  _High betweenness centrality (0.060) - this node is a cross-community bridge._
- **Why does `Tok` connect `Tok` to `string`?**
  _High betweenness centrality (0.054) - this node is a cross-community bridge._
- **What connects `On`, `Off`, `AC` to the rest of the system?**
  _802 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Parser` be split into smaller, more focused modules?**
  _Cohesion score 0.07116104868913857 - nodes in this community are weakly interconnected._
- **Should `Parser` be split into smaller, more focused modules?**
  _Cohesion score 0.0761904761904762 - nodes in this community are weakly interconnected._