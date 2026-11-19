# Graph Report - product-setup-checkout-b042cf  (2026-11-19)

## Corpus Check
- 285 files · ~696,025 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 39 file(s) not represented in the graph (top: (none) 14, .avif 8, .css 7)

## Summary
- 3908 nodes · 7958 edges · 204 communities (182 shown, 22 thin omitted)
- Extraction: 93% EXTRACTED · 7% INFERRED · 0% AMBIGUOUS · INFERRED: 556 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `f1046934`
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
- Subscriptions and the online store: the plan, step by step
- Json
- Json
- Framebuffer
- sim/web/web_main.cpp
- Claude outputs/firmware/src/claude_client.h
- firmware/src/camera.cpp
- firmware/src/main.cpp
- keyCalc
- AI Answer Flow
- core/device.cpp
- App
- tests/tests.cpp
- App
- AI Answer Flow
- video/package.json
- Proofline marketing guide
- firmware/src/preview.cpp
- string
- Focus
- core/claude_api.cpp
- firmware/src/settings.cpp
- arduino
- Claude outputs/firmware/src/main.cpp
- core/app.cpp
- manager.cjs
- AI Calculator Simulator page (keypad + test bench)
- View
- View
- SerialCmd
- Breadboard Wiring Diagram (exact, camera up)
- Proofline market research
- Claude outputs/tools/gen_font.py
- Stage 4: full calculator
- sim/win32_main.cpp
- Claude outputs/core/device.cpp
- Tester firmware (Stage 4)
- Event
- string
- Vars
- Claude outputs/firmware/src/camera.cpp
- Claude outputs/firmware/src/settings.cpp
- SerialCmd
- Framebuffer
- Decode
- keys.cpp
- CalcError
- SolveResult
- compilerOptions
- firmware-prototype/src/main.cpp
- SolveResult
- firmware-prototype/src/camera.cpp
- firmware-prototype/src/preview.cpp
- firmware-prototype/src/settings.cpp
- loop
- AI SOLVE mode (MODE 4)
- Proofline content calendar: first 12 weeks
- core static library (core/*.cpp, C++17)
- Screen
- Failure
- Screen
- Failure
- helpers.ts
- check-site.mjs
- Key
- Stage 2: Windows simulator AI flow
- decodeUtf8
- 2. Campaign page copy (paste-ready)
- studio_renders.py
- SerialCmd
- Part A. The brief (send this to the creator)
- Line
- Com
- main.cjs
- Calculator Mode
- loop
- CalcError
- drawCalc
- crypto.ts
- Claude outputs/tests/tests.cpp
- Calculator Mode
- game.js
- Mode
- ScanNote
- Menu System
- farm.test.cjs
- Mode
- ScanNote
- Effort
- Menu System
- account.ts
- Ask
- _this
- 3. Preorder launch sequence (5 emails)
- lib.mjs
- Prototype firmware (the board inside the Casio)
- graphify skill trigger (/graphify)
- Proofline: decisions needed
- Part B. Notes for you
- Claude outputs/sim/web/build.sh
- render
- sim/web/build.sh
- Key
- HttpError
- AngleUnit
- Launch site (stage 4, `ai_calc_web_4`, multi-page since stage 3)
- Remotion video
- run-local.mjs
- stage3.test.mjs
- Prototype photo shot list
- Kind
- Decode
- yearly.test.ts
- Launch site copy deck
- QR linking: server side (2026-10-16)
- Brand name ideas
- app.js
- Website assets
- Vars
- Claude outputs/core/claude_api.cpp
- solve.ts
- portal.js
- env.ts
- tasks.cjs
- Tuning
- billing.ts
- ai_calc_web_3: payments, subscriptions and the store (2026-10-11)
- run-all.mjs
- Claude outputs/sim/win32_main.cpp
- tests/package.json
- AngleUnit
- contract.mjs
- business.test.cjs
- stripe-setup.mjs
- api/package.json
- compilerOptions
- call
- api/src/index.ts
- AI Calculator API (`web/api`)
- agents.cjs
- browserHeaders
- firmware-prototype/src/claude_client.cpp
- calculator.mjs
- Tuning
- tests/README.md
- Brief for every page of the Proofline site
- testFont
- Exam mode regression plan (stage 2, `tests/tests.cpp`)
- guard.cjs
- stage1/TEST_RESULTS.md
- planner.cjs
- stage1/CHANGELOG.md
- stage1/MANUAL_STEPS.md
- Kind
- cmath
- Line
- ai_calc_web_3: test results (2026-10-11, re-run after the Proofline / $10 / yearly changes; Windows 11, Node 24.21)
- Com
- ai_calc_web_3: your steps
- back-office.test.ts
- Product photos of the Proofline with an iPhone 15 Pro Max
- The farm manager (phase 1)
- office.cjs
- Button
- claudeSolve
- 1. The agent loop
- Shipping setup (US, from Indiana)
- penny.cjs
- run.cjs
- setup
- Should Proofline be an LLC? What you gain
- Proofline Farm: plan for the always-on AI ecosystem app
- 5. Build order (updated for your decisions)
- HistoryItem
- Back office: business e-mail, Penny, and Business HQ
- Stripe integration review: 2026-10-14
- Wi-Fi watchdog
- wifi-watchdog.ps1
- Kind
- prompts.cjs
- 2. Where it runs without paying for a website
- 3b. Keeping the Wi-Fi alive overnight
- Weekly usage budget: always leave enough for your own use
- NormMode
- parseSolveResult
- task-example.md

## God Nodes (most connected - your core abstractions)
1. `Device` - 104 edges
2. `Device` - 99 edges
3. `Tok` - 71 edges
4. `Tok` - 71 edges
5. `HttpError` - 67 edges
6. `DKey` - 52 edges
7. `DKey` - 52 edges
8. `App` - 50 edges
9. `App` - 45 edges
10. `call()` - 43 edges

## Surprising Connections (you probably didn't know these)
- `API (web/api, documented in docs/web/openapi.yaml)` --references--> `linkToUser()`  [INFERRED]
  docs/web/stage2/LINK_QR.md → web/api/src/account.ts
- `P2: later (Kickstarter, schools, preorders)` --references--> `invoiceSubscriptionId()`  [INFERRED]
  docs/web/STRIPE_REVIEW_2026-10-14.md → web/api/src/stripe.ts
- `Simulator page (Claude outputs, older: no whereBanner/Scan now)` --semantically_similar_to--> `AI Calculator Simulator page (keypad + test bench)`  [INFERRED] [semantically similar]
  Claude outputs/sim/web/page.html → sim/web/page.html
- `2. Chosen stack (decision D1)` --references--> `stripe()`  [INFERRED]
  docs/web/PLAN.md → web/api/scripts/stripe-setup.mjs
- `P0: fix before taking any real money` --references--> `billingPortal()`  [INFERRED]
  docs/web/STRIPE_REVIEW_2026-10-14.md → web/api/src/billing.ts

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

## Communities (204 total, 22 thin omitted)

### Community 0 - "Parser"
Cohesion: 0.07
Nodes (45): addOk(), Alias, text, tok, asInt(), endsOperand(), errorText(), exactPlus() (+37 more)

### Community 1 - "Parser"
Cohesion: 0.07
Nodes (44): addOk(), Alias, text, tok, asInt(), endsOperand(), errorText(), evaluate() (+36 more)

### Community 2 - "Tok"
Cohesion: 0.03
Nodes (71): Tok, Abs, Acos, Acosh, Add, Ans, Asin, Asinh (+63 more)

### Community 3 - "Tok"
Cohesion: 0.03
Nodes (71): Tok, Abs, Acos, Acosh, Add, Ans, Asin, Asinh (+63 more)

### Community 4 - "Claude outputs/sim/web/web_main.cpp"
Cohesion: 0.10
Nodes (33): solveInstructions(), solveSchema(), jsonEscape(), numText(), sim_active_request(), sim_alloc(), sim_api_url(), sim_api_version() (+25 more)

### Community 5 - "Device"
Cohesion: 0.03
Nodes (39): Device, alpha_, angle_, app_, back_, blink_, clrChoice_, continueWith_ (+31 more)

### Community 6 - "Device"
Cohesion: 0.03
Nodes (38): Device, alpha_, angle_, app_, back_, battery_, blink_, charging_ (+30 more)

### Community 7 - "DKey"
Cohesion: 0.04
Nodes (52): DKey, AC, Add, Alpha, Ans, Close, Cos, Count (+44 more)

### Community 8 - "DKey"
Cohesion: 0.04
Nodes (52): DKey, Abs, AC, Add, Alpha, Ans, Close, Cos (+44 more)

### Community 9 - "Subscriptions and the online store: the plan, step by step"
Cohesion: 0.09
Nodes (22): 0. Where things stand, 10. What the security review found and what changed (stage 3), 1. How the money flows (in plain words), 2. Try the whole thing on your computer (free, no accounts), 3. The Stripe account **(you)**, 4.1 Products and prices, 4.2 Public details, 4.3 Billing settings (important) (+14 more)

### Community 10 - "Json"
Cohesion: 0.08
Nodes (17): Json, a_, b_, n_, o_, parse, s_, JsonParser (+9 more)

### Community 11 - "Json"
Cohesion: 0.08
Nodes (17): Json, a_, b_, n_, o_, parse, s_, JsonParser (+9 more)

### Community 12 - "Framebuffer"
Cohesion: 0.13
Nodes (19): Framebuffer, clear, drawText, drawTextPx, fillRect, get, icons_, invertRect (+11 more)

### Community 13 - "sim/web/web_main.cpp"
Cohesion: 0.10
Nodes (35): jsonEscape(), numText(), sim_active_request(), sim_alloc(), sim_api_url(), sim_api_version(), sim_build_request(), sim_classify() (+27 more)

### Community 14 - "Claude outputs/firmware/src/claude_client.h"
Cohesion: 0.09
Nodes (15): SolveCallbacks, cancelled, fail, partial, reply, SolveCallbacks, cancelled, fail (+7 more)

### Community 15 - "firmware/src/camera.cpp"
Cohesion: 0.10
Nodes (25): applyAll(), autoTuneLocked(), cameraAutoTune(), cameraBegin(), cameraCapture(), cameraFocusScore(), cameraFrame(), cameraHoldEstimateMs() (+17 more)

### Community 16 - "firmware/src/main.cpp"
Cohesion: 0.08
Nodes (28): Button, key, lastMs, nextRepeat, pin, pressed, Event, confidence (+20 more)

### Community 17 - "keyCalc"
Cohesion: 0.18
Nodes (16): evaluate(), continuesAns(), clearEntry, equals, insert, keyAi, keyCalc, onKey (+8 more)

### Community 18 - "AI Answer Flow"
Cohesion: 0.07
Nodes (34): AI Answer Screen, AI Answer Flow, AI Error States, AI Hold-to-Capture Flow, AI Hold Captured Screen, AI Hold Countdown Screen, AI Hold Done Screen, AI Ready Screen (+26 more)

### Community 19 - "core/device.cpp"
Cohesion: 0.13
Nodes (27): continuesAns(), clearEntry, endExam, equals, insert, keyAi, keyCalc, leaveAi (+19 more)

### Community 20 - "App"
Cohesion: 0.07
Nodes (19): App, activeId_, busySinceMs_, captured_, expression_, hasKey_, holdUntilMs_, kUnclearThreshold (+11 more)

### Community 21 - "tests/tests.cpp"
Cohesion: 0.16
Nodes (20): calcText(), keys(), main(), press(), readFile(), sampleStream(), snapshot(), snapshotFb() (+12 more)

### Community 22 - "App"
Cohesion: 0.08
Nodes (31): App, accepts, activeId_, busySinceMs_, captured_, expression_, hasKey_, kUnclearThreshold (+23 more)

### Community 23 - "AI Answer Flow"
Cohesion: 0.11
Nodes (27): AI Answer Screen, Scroll Indicator Icons, AI Ready Screen, Chemistry Answer Screen, Chemistry Answer Steps Screen, Physics Answer End Screen, Physics Answer Screen, Answer Streaming Screen (+19 more)

### Community 24 - "video/package.json"
Cohesion: 0.05
Nodes (58): eslint, prettier, react, react-dom, remotion, @remotion/cli, @remotion/eslint-config-flat, @remotion/google-fonts (+50 more)

### Community 25 - "Proofline marketing guide"
Cohesion: 0.05
Nodes (41): 0. The strategy in five lines, 10. Preorder platform: recommendation, 11. PR and press angles, 12. Community building, 13. Compliance checklist (print this), 14. What NOT to do, 15. Weekly checklist for the founder (about 5 to 7 hours a week), 1.1 One sentence (+33 more)

### Community 26 - "firmware/src/preview.cpp"
Cohesion: 0.34
Nodes (16): authorised(), autotuneHandler(), detailHandler(), focusHandler(), frameHandler(), lastHandler(), noCache(), pageHandler() (+8 more)

### Community 27 - "string"
Cohesion: 0.11
Nodes (9): Glyph, cp, rows, NormMode, Norm1, Norm2, Glyph, cp (+1 more)

### Community 28 - "Focus"
Cohesion: 0.11
Nodes (24): analyseFocus(), Crop, h, use, w, x, y, detailCrop() (+16 more)

### Community 29 - "core/claude_api.cpp"
Cohesion: 0.14
Nodes (13): base64Encode(), buildSolveRequest(), classifyFailure(), jsonString(), peekAnswer(), StreamReader, buf_, errorMessage_ (+5 more)

### Community 30 - "firmware/src/settings.cpp"
Cohesion: 0.17
Nodes (16): cameraName(), setup(), statusText(), screenBegin(), Ask, Key, Nothing, Password (+8 more)

### Community 31 - "arduino"
Cohesion: 0.15
Nodes (6): draw(), screenShow(), draw(), screenShow(), draw(), screenShow()

### Community 32 - "Claude outputs/firmware/src/main.cpp"
Cohesion: 0.26
Nodes (10): cameraCapture(), applySettings(), connectWifi(), handleEvent(), loop(), pollButtons(), post(), serviceExam() (+2 more)

### Community 33 - "core/app.cpp"
Cohesion: 0.24
Nodes (13): accepts, effortParam, onFailure, onKey, onPartialAnswer, onReply, scrollBy, setLines (+5 more)

### Community 34 - "manager.cjs"
Cohesion: 0.09
Nodes (66): ensureFacts(), loadAll(), addFollowUps(), agentFeed(), applyActions(), applyDecisions(), budget(), buildSummary() (+58 more)

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

### Community 40 - "Proofline market research"
Cohesion: 0.12
Nodes (16): 0. The short version, 10. Sources, 1. Market size signals, 2.1 Table, 2.2 What to learn from each, 2. Competitors, 3. Customer segments and jobs-to-be-done, 4. Objections and honest answers (+8 more)

### Community 41 - "Claude outputs/tools/gen_font.py"
Cohesion: 0.12
Nodes (4): Full, Full, main(), patch()

### Community 42 - "Stage 4: full calculator"
Cohesion: 0.16
Nodes (14): src/claude_client (HTTPS streaming to Claude), src/main.cpp (buttons, serial keys, Wi-Fi, capture+send task), src/pins.h (pin map), src/root_ca.h (api.anthropic.com certificates), src/screen (125x61 to 250x122 e-paper, Plain/LCD dots), src/settings (hotspot, key, exam state in flash), COMP maths engine (exact fractions, Casio priorities, 10 sig digits), core/calc_engine (tokens, parser, fractions, formatting) (+6 more)

### Community 43 - "sim/win32_main.cpp"
Cohesion: 0.09
Nodes (20): advanceRequest(), Event, confidence, failure, id, kind, text, exifOrientation() (+12 more)

### Community 44 - "Claude outputs/core/device.cpp"
Cohesion: 0.15
Nodes (21): clockText(), drawCalc, drawMenu, drawStatus, endExam, engText, leaveAi, notice (+13 more)

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
Cohesion: 0.28
Nodes (3): cameraBegin(), configFor(), sensorName()

### Community 50 - "Claude outputs/firmware/src/settings.cpp"
Cohesion: 0.42
Nodes (6): handle(), help(), save(), settingsLoad(), settingsPoll(), trim()

### Community 51 - "SerialCmd"
Cohesion: 0.17
Nodes (10): SerialCmd, Changed, ExamOff, Keys, None, Snap, Settings, apiKey (+2 more)

### Community 52 - "Framebuffer"
Cohesion: 0.13
Nodes (19): Framebuffer, clear, drawText, drawTextPx, fillRect, get, icons_, invertRect (+11 more)

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

### Community 57 - "compilerOptions"
Cohesion: 0.14
Nodes (13): compilerOptions, esModuleInterop, forceConsistentCasingInFileNames, jsx, lib, module, moduleResolution, noEmit (+5 more)

### Community 58 - "firmware-prototype/src/main.cpp"
Cohesion: 0.07
Nodes (33): cameraName(), batteryPercent(), charging(), Event, confidence, failure, id, kind (+25 more)

### Community 59 - "SolveResult"
Cohesion: 0.18
Nodes (9): SolveResult, answer, choice, confidence, expression, readable, readAs, steps (+1 more)

### Community 60 - "firmware-prototype/src/camera.cpp"
Cohesion: 0.11
Nodes (30): applyAll(), autoTuneLocked(), cameraAutoTune(), cameraBegin(), cameraCapture(), cameraFocusScore(), cameraFrame(), cameraHoldEstimateMs() (+22 more)

### Community 61 - "firmware-prototype/src/preview.cpp"
Cohesion: 0.31
Nodes (16): authorised(), autotuneHandler(), detailHandler(), focusHandler(), frameHandler(), lastHandler(), noCache(), pageHandler() (+8 more)

### Community 62 - "firmware-prototype/src/settings.cpp"
Cohesion: 0.19
Nodes (12): Ask, Key, Nothing, Password, Ssid, examSave(), handle(), help() (+4 more)

### Community 63 - "loop"
Cohesion: 0.15
Nodes (22): cameraHoldUntil(), applySettings(), connectWifi(), loop(), previewEnded(), scanWifi(), serviceExam(), serviceSend() (+14 more)

### Community 64 - "AI SOLVE mode (MODE 4)"
Cohesion: 0.22
Nodes (9): BEST GUESS (x% sure) display, Careful scan (exposure/gain/contrast sweep scored on ink edges), Scan photo pipeline (careful scan, hold still, 8 frames sharpest, zoom+enhance), AI SOLVE mode (MODE 4), Unclear-scan warning (confidence < 0.8), core/app (AI SOLVE screens), core/claude_api (instructions, schema, request, stream reader, error mapping), Error screens (CLAUDE ERROR, BUSY, NO CONNECTION, TIMED OUT, CAMERA ERROR) (+1 more)

### Community 65 - "Proofline content calendar: first 12 weeks"
Cohesion: 0.11
Nodes (17): After week 12 (preview), Backup bank (use when a gate isn't met), How to use this, Proofline content calendar: first 12 weeks, Week 10 (14 to 20 Dec): more samples, Week 11 (21 to 27 Dec): holiday week (light), Week 12 (28 Dec to 3 Jan): new-year reset, Week 1 (12 to 18 Oct): set up and say hello (+9 more)

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

### Community 71 - "helpers.ts"
Cohesion: 0.10
Nodes (32): vitest, MAX_OUTPUT_TOKENS_PER_SEC, download(), status(), ADMIN_TOKEN, BASE, CallOpts, CallResult (+24 more)

### Community 72 - "check-site.mjs"
Cohesion: 0.06
Nodes (19): args, base, localSha, out, positional, q, u, here (+11 more)

### Community 73 - "Key"
Cohesion: 0.29
Nodes (7): Key, AC, Down, Eq, Off, On, Up

### Community 74 - "Stage 2: Windows simulator AI flow"
Cohesion: 0.29
Nodes (7): src/camera (grayscale 1600x1200 JPEG), Claude Sonnet 5.5 model, Photo preprocessing (upright, grayscale, max 1600 px JPEG), Sample mode fallback (samples/ replies when no api_key.txt), Stage 2: Windows simulator AI flow, DEMOS canned replies, toJpeg()

### Community 75 - "decodeUtf8"
Cohesion: 0.36
Nodes (5): decodeUtf8(), encodeUtf8(), textLength(), wrapParagraph(), wrapText()

### Community 76 - "2. Campaign page copy (paste-ready)"
Cohesion: 0.08
Nodes (24): 0. Before you can launch (gates), 1. Title and subtitle options, 2.10 FAQ, 2.1 The short version, 2.2 Why we built it, 2.3 What it does, 2.4 What's real today, and what's still to come, 2.5 How it's made [Kickstarter requires this for hardware [K1]] (+16 more)

### Community 77 - "studio_renders.py"
Cohesion: 0.11
Nodes (9): camera(), hidden(), log(), name_of(), ray_traced_shot(), restore(), run_all(), set_scene() (+1 more)

### Community 78 - "SerialCmd"
Cohesion: 0.13
Nodes (13): SerialCmd, Changed, ExamOff, Keys, None, Preview, PreviewOff, Scan (+5 more)

### Community 79 - "Part A. The brief (send this to the creator)"
Cohesion: 0.11
Nodes (18): A1. Who we work with, A2. What Proofline is (the facts you can use), A3. Must-say (somewhere in every video), A4. What to show, and what not to show, A5. Disclosure (required by law and by the platforms), A6. Talking points (use your own words), A7. Our review, C1. STEM or study creator (+10 more)

### Community 80 - "Line"
Cohesion: 0.40
Nodes (3): Line, header, text

### Community 82 - "main.cjs"
Cohesion: 0.06
Nodes (54): electron, ART, chunk(), crc32(), ico(), PALETTE, png(), STATUS (+46 more)

### Community 83 - "Calculator Mode"
Cohesion: 0.40
Nodes (5): Calc Editing Screen, Calculator Mode, Calc Math Error Screen, Calc Fraction Result Screen, Calc STO Screen

### Community 84 - "loop"
Cohesion: 0.13
Nodes (25): cameraHoldUntil(), cameraSleep(), keysSleepUntilPress(), applySettings(), connectWifi(), loop(), previewEnded(), scanWifi() (+17 more)

### Community 85 - "CalcError"
Cohesion: 0.20
Nodes (10): CalcError, Argument, Math, None, Stack, Syntax, EvalResult, error (+2 more)

### Community 86 - "drawCalc"
Cohesion: 0.24
Nodes (10): drawCalc, decodeUtf8(), encodeUtf8(), findGlyph(), hasGlyph(), lookup(), normalizeCodepoint(), textLength() (+2 more)

### Community 87 - "crypto.ts"
Cohesion: 0.28
Nodes (11): base64url(), DEVICE_TOKEN_RE, enc, LINK_TOKEN_RE, newDeviceToken(), newLinkToken(), newPairCode(), normalisePairCode() (+3 more)

### Community 88 - "Claude outputs/tests/tests.cpp"
Cohesion: 0.15
Nodes (16): parseSolveResult(), keys(), main(), press(), readFile(), sampleStream(), snapshot(), snapshotFb() (+8 more)

### Community 89 - "Calculator Mode"
Cohesion: 0.40
Nodes (5): Calculator Editing Screen, Calculator Math Error Screen, Calculator Fraction Result Screen, Calculator STO Screen, Calculator Mode

### Community 90 - "game.js"
Cohesion: 0.10
Nodes (50): Biz, bubble(), buildBg(), bush(), checkBg(), crop(), desiredFor(), disc() (+42 more)

### Community 91 - "Mode"
Cohesion: 0.50
Nodes (3): Mode, Ai, Comp

### Community 92 - "ScanNote"
Cohesion: 0.50
Nodes (4): ScanNote, None, Ok, Unclear

### Community 93 - "Menu System"
Cohesion: 0.50
Nodes (4): HYP Menu Screen, Menu System, Mode Menu Screen, Setup Menu Screen

### Community 94 - "farm.test.cjs"
Cohesion: 0.05
Nodes (28): fs, link(), managerEnv(), os, path, assert, crew, farm (+20 more)

### Community 95 - "Mode"
Cohesion: 0.50
Nodes (3): Mode, Ai, Comp

### Community 96 - "ScanNote"
Cohesion: 0.50
Nodes (4): ScanNote, None, Ok, Unclear

### Community 97 - "Effort"
Cohesion: 0.33
Nodes (5): Effort, Careful, Max, Normal, 4. Settings

### Community 98 - "Menu System"
Cohesion: 0.50
Nodes (4): Hyperbolic Menu Screen, Mode Menu Screen, Setup Menu Screen, Menu System

### Community 99 - "account.ts"
Cohesion: 0.12
Nodes (29): activeDevices(), claim(), ClaimGate, ClaimTarget, DEVICE_COLS, DeviceBrief, deviceJson(), getMe() (+21 more)

### Community 100 - "Ask"
Cohesion: 0.40
Nodes (5): Ask, Key, Nothing, Password, Ssid

### Community 102 - "3. Preorder launch sequence (5 emails)"
Cohesion: 0.13
Nodes (14): 0.1 CAN-SPAM (US law) [S55], 0.2 Double opt-in (do this before the first send), 0.3 Our own content rules, 0. Rules for every email, 1. Welcome email (sent right after confirming), 2. Monthly update (first Tuesday of each month), 3. Preorder launch sequence (5 emails), 4. After launch (+6 more)

### Community 103 - "lib.mjs"
Cohesion: 0.09
Nodes (53): raw(), assertNoTrial(), hw(), NO_TRIAL(), status(), publish(), publishTo(), ADMIN_TOKEN (+45 more)

### Community 104 - "Prototype firmware (the board inside the Casio)"
Cohesion: 0.40
Nodes (4): Differences from the tester, Hardware it expects (from the schematic), Prototype firmware (the board inside the Casio), Upload

### Community 106 - "Proofline: decisions needed"
Cohesion: 0.17
Nodes (10): A. Before the samples arrive (this week), B. Before taking any money, C. Before launch, D. Product polish (calculator behaviour; firmware "choices to confirm"), Proofline: decisions needed, How to try it, Next 10 actions, in order, Proofline: morning brief, 2026-10-12 (+2 more)

### Community 107 - "Part B. Notes for you"
Cohesion: 0.20
Nodes (9): B1. Printable layout, B2. Open questions before this goes out, B3. Rules for teacher work, C1. Forum or group post, C2. Email to a teacher who asked for information, Part A. The one-pager (paste-ready), Part B. Notes for you, Part C. Outreach message for teacher communities (+1 more)

### Community 111 - "Key"
Cohesion: 0.29
Nodes (7): Key, AC, Down, Eq, Off, On, Up

### Community 112 - "HttpError"
Cohesion: 0.13
Nodes (52): P1: before launch, deleteMe(), revokeDevice(), clearCookie(), confirm(), login(), LOGIN_TTL_SEC, logout() (+44 more)

### Community 113 - "AngleUnit"
Cohesion: 0.50
Nodes (4): AngleUnit, Deg, Gra, Rad

### Community 114 - "Launch site (stage 4, `ai_calc_web_4`, multi-page since stage 3)"
Cohesion: 0.20
Nodes (9): Checks, Design system, Launch site (stage 4, `ai_calc_web_4`, multi-page since stage 3), Pages (30, plus the `/try/` simulator frame), Placeholders still to replace, Rebuild the assets, Run it locally, Shared header and footer (+1 more)

### Community 115 - "Remotion video"
Cohesion: 0.29
Nodes (6): Commands, Docs, Help, Issues, License, Remotion video

### Community 116 - "run-local.mjs"
Cohesion: 0.09
Nodes (21): base, [hwId, urlArg], out, u, esc(), FAKE_STRIPE_KEY, nowSec(), page() (+13 more)

### Community 117 - "stage3.test.mjs"
Cohesion: 0.21
Nodes (11): now(), customerOf(), event(), me(), seedSub(), sign(), status(), STRIPE (+3 more)

### Community 118 - "Prototype photo shot list"
Cohesion: 0.33
Nodes (5): After shooting, Don'ts, Prototype photo shot list, Setup (10 minutes, no special gear), The shots (in priority order)

### Community 119 - "Kind"
Cohesion: 0.40
Nodes (5): Kind, Captured, Fail, Partial, Reply

### Community 120 - "Decode"
Cohesion: 0.18
Nodes (10): Decode, cx, cy, fullH, fullW, lastYield, out, outH (+2 more)

### Community 121 - "yearly.test.ts"
Cohesion: 0.14
Nodes (21): signWebhook(), deviceStatus(), me(), webhook(), ORIGIN, event(), fakeStripe, listMatches() (+13 more)

### Community 122 - "Launch site copy deck"
Cohesion: 0.40
Nodes (4): Key lines and alternatives, Launch site copy deck, Messages, Words we don't use

### Community 123 - "QR linking: server side (2026-10-16)"
Cohesion: 0.33
Nodes (5): API (web/api, documented in docs/web/openapi.yaml), Local, QR linking: server side (2026-10-16), Site (web/site), Tests

### Community 125 - "app.js"
Cohesion: 0.07
Nodes (67): agentById(), agentName(), ago(), applyRoute(), attnGo(), attnKey(), attnLabel(), attnText() (+59 more)

### Community 128 - "Vars"
Cohesion: 0.17
Nodes (12): Vars, a, ans, b, c, d, e, f (+4 more)

### Community 129 - "Claude outputs/core/claude_api.cpp"
Cohesion: 0.13
Nodes (18): base64Encode(), buildSolveRequest(), classifyFailure(), jsonString(), peekAnswer(), StreamReader, buf_, errorMessage_ (+10 more)

### Community 131 - "solve.ts"
Cohesion: 0.05
Nodes (87): formatPairCode(), authDevice(), checkLimits(), checkLinkedAndEntitled(), familyParam(), FIRMWARE_PATH, firmwareCheck(), firmwareImage() (+79 more)

### Community 132 - "portal.js"
Cohesion: 0.15
Nodes (45): api(), busy(), clearPending(), closeDialog(), drawQr(), fallbackKeys(), fallbackMsg(), fieldError() (+37 more)

### Community 133 - "env.ts"
Cohesion: 0.11
Nodes (23): 3. Gaps, in priority order, P0: fix before taking any real money, P2: later (Kickstarter, schools, preorders), HW_SECRET_RE, MAX_IMAGE_BYTES, MIN_ADMIN_TOKEN_CHARS, MIN_IMAGE_BYTES, FAMILY_RE (+15 more)

### Community 134 - "tasks.cjs"
Cohesion: 0.14
Nodes (22): create(), FIELD_ORDER, formatValue(), fs, load(), nextId(), parse(), parseValue() (+14 more)

### Community 135 - "Tuning"
Cohesion: 0.40
Nodes (5): Tuning, def, max, min, name

### Community 136 - "billing.ts"
Cohesion: 0.12
Nodes (33): BLOCKING, chargeRefunded(), checkoutCompleted(), closeAtStripe(), customerDeleted(), customerKey(), disputeChanged(), ensureCustomer() (+25 more)

### Community 137 - "ai_calc_web_3: payments, subscriptions and the store (2026-10-11)"
Cohesion: 0.29
Nodes (6): Added: local development (`web/dev`), Added: server (`web/api`), Added: website (`web/site`), ai_calc_web_3: payments, subscriptions and the store (2026-10-11), Changed, Security review

### Community 138 - "run-all.mjs"
Cohesion: 0.09
Nodes (32): anthropicError(), imagesOf(), markerOf(), MARKERS, MOCK_ANSWER, MOCK_INPUT_TOKENS, MOCK_OUTPUT_TOKENS, sse() (+24 more)

### Community 139 - "Claude outputs/sim/win32_main.cpp"
Cohesion: 0.09
Nodes (21): keysForChar(), advanceRequest(), Event, confidence, failure, id, kind, text (+13 more)

### Community 140 - "tests/package.json"
Cohesion: 0.10
Nodes (19): ajv, ajv-formats, yaml, description, devDependencies, ajv, ajv-formats, yaml (+11 more)

### Community 141 - "AngleUnit"
Cohesion: 0.50
Nodes (4): AngleUnit, Deg, Gra, Rad

### Community 142 - "contract.mjs"
Cohesion: 0.21
Nodes (18): ajv, assertMatches(), assertRequestValid(), assertSchema(), compileAll(), CONTRACT_PATH, deref(), doc (+10 more)

### Community 143 - "business.test.cjs"
Cohesion: 0.10
Nodes (17): assert, BIZ, fs, http, office, os, path, penny (+9 more)

### Community 144 - "stripe-setup.mjs"
Cohesion: 0.10
Nodes (32): 0. Approved decisions (Nirav, 2026-10-08), 10. Recon summary (2026-10-08), 1. What we are replacing, and what we keep, 2. Chosen stack (decision D1), 3. Data model (D1), 4. Device-to-backend protocol, 4b. Shared build conventions (backend ↔ tests), 4c. Security review stage 1: decisions (2026-10-08) (+24 more)

### Community 145 - "api/package.json"
Cohesion: 0.08
Nodes (24): @cloudflare/vitest-plugin, @cloudflare/workers-types, postal-mime, wrangler, dependencies, postal-mime, devDependencies, @cloudflare/vitest-plugin (+16 more)

### Community 146 - "compilerOptions"
Cohesion: 0.14
Nodes (13): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noImplicitOverride, noUncheckedIndexedAccess (+5 more)

### Community 147 - "call"
Cohesion: 0.20
Nodes (17): customerAccount(), call(), dbFirst(), Device, enrolBoard(), expectError(), nextHwId(), registerDevice() (+9 more)

### Community 148 - "api/src/index.ts"
Cohesion: 0.09
Nodes (50): enrolHardware(), publishFirmware(), requireAdmin(), storeStatus(), unitsLeft(), hex(), error(), json() (+42 more)

### Community 149 - "AI Calculator API (`web/api`)"
Cohesion: 0.20
Nodes (9): 1. Install (once), 2. Run it on your computer, 3. Test, 5. Deploy on the FREE plan (workers.dev address), 5b. Enrol each board at QA (once, during the first USB flash), 6. What costs money, and when, 7. How the spend cap works (short version), AI Calculator API (`web/api`) (+1 more)

### Community 150 - "agents.cjs"
Cohesion: 0.14
Nodes (13): addLesson(), chatLog(), chatSession(), DEFAULT_ROSTER, ensureGoals(), fs, LATE_JOINERS, lessons() (+5 more)

### Community 151 - "browserHeaders"
Cohesion: 0.18
Nodes (9): AGE_MSG, browserHeaders(), freshIp(), buy(), headers(), webhook(), headers(), join() (+1 more)

### Community 152 - "firmware-prototype/src/claude_client.cpp"
Cohesion: 0.19
Nodes (8): claudeSolve(), lower(), readLine(), writeAll(), claudeSolve(), lower(), readLine(), writeAll()

### Community 153 - "calculator.mjs"
Cohesion: 0.31
Nodes (14): call(), cmdCode(), cmdNew(), cmdScan(), cmdSolve(), cmdStatus(), FILE, headers() (+6 more)

### Community 154 - "Tuning"
Cohesion: 0.40
Nodes (5): Tuning, def, max, min, name

### Community 155 - "tests/README.md"
Cohesion: 0.29
Nodes (6): PlanState, Contract change proposals (not applied; for the lead), Conventions the Worker must honour (beyond openapi.yaml), Files, Run, Skipped, and why

### Community 156 - "Brief for every page of the Proofline site"
Cohesion: 0.33
Nodes (5): Brief for every page of the Proofline site, Facts (the only facts), Hard rules, How pages are built, Voice

### Community 157 - "testFont"
Cohesion: 0.62
Nodes (5): findGlyph(), hasGlyph(), lookup(), normalizeCodepoint(), testFont()

### Community 158 - "Exam mode regression plan (stage 2, `tests/tests.cpp`)"
Cohesion: 0.33
Nodes (5): A. Behaviour that exists today (pin it; most is already covered), B. New in stage 2: must all be blocked while exam mode is on, C. Proposed test code (tests.cpp style), D. Manual checks (hardware, stage 2 MANUAL_STEPS), Exam mode regression plan (stage 2, `tests/tests.cpp`)

### Community 159 - "guard.cjs"
Cohesion: 0.18
Nodes (12): BLOCKED_COMMANDS, decide(), fs, inside(), norm(), path, assert, { decide } (+4 more)

### Community 160 - "stage1/TEST_RESULTS.md"
Cohesion: 0.40
Nodes (4): Fixes made while merging, Not tested (needs real accounts, or comes later), Security review, The 19 skipped tests in the main pass

### Community 161 - "planner.cjs"
Cohesion: 0.24
Nodes (12): agentCyclePrompt(), chatSystemPrompt(), context(), filterTasks(), foremanPrompt(), isDuplicate(), parseDoneWhen(), RULES (+4 more)

### Community 162 - "stage1/CHANGELOG.md"
Cohesion: 0.50
Nodes (3): Added, Changed, Not in this stage

### Community 163 - "stage1/MANUAL_STEPS.md"
Cohesion: 0.50
Nodes (3): Later (not now), Part A: put it in your repo and run the tests (about 10 minutes), Part B: put the server online for free (when you're ready; about 30 minutes)

### Community 164 - "Kind"
Cohesion: 0.40
Nodes (5): Kind, Captured, Fail, Partial, Reply

### Community 168 - "Line"
Cohesion: 0.67
Nodes (3): Line, header, text

### Community 169 - "ai_calc_web_3: test results (2026-10-11, re-run after the Proofline / $10 / yearly changes; Windows 11, Node 24.21)"
Cohesion: 0.40
Nodes (4): ai_calc_web_3: test results (2026-10-11, re-run after the Proofline / $10 / yearly changes; Windows 11, Node 24.21), Checked by mutation, End to end, in the browser, on the local stack (`node web/dev/run-local.mjs`), Not tested

### Community 171 - "ai_calc_web_3: your steps"
Cohesion: 0.50
Nodes (3): ai_calc_web_3: your steps, Later (when you're ready to take money), Now (free, no accounts)

### Community 172 - "back-office.test.ts"
Cohesion: 0.26
Nodes (9): admin, dbAll(), mime(), now(), OFFICE, oneMail(), receive(), seedOrder() (+1 more)

### Community 173 - "Product photos of the Proofline with an iPhone 15 Pro Max"
Cohesion: 0.18
Nodes (10): 1. Set up the phone once (5 minutes), 2. Gear (about $40), 3. Light: a window is your best light, 4. Lens and distance: no fish-eye calculators, 5. Focus and exposure (the screen is the hard part), 6. The shot list (what the website and ads need), 7. Editing (free), 8. Exporting for the website and social (+2 more)

### Community 174 - "The farm manager (phase 1)"
Cohesion: 0.20
Nodes (8): Everyday use, First-time setup (about 5 minutes), Not in phase 1 yet, Proofline Farm tools, Rules it follows (your decisions, 2026-10-18), Tests, The farm game (open "Proofline Farm" on the desktop), The farm manager (phase 1)

### Community 175 - "office.cjs"
Cohesion: 0.31
Nodes (8): alerts(), client(), days(), ID_RE, planningNotes(), settings(), setupSteps(), usd()

### Community 176 - "Button"
Cohesion: 0.25
Nodes (8): dkeyName(), Button, key, lastMs, pin, pressed, press(), pressButton()

### Community 177 - "claudeSolve"
Cohesion: 0.44
Nodes (4): claudeSolve(), lower(), readLine(), writeAll()

### Community 178 - "1. The agent loop"
Cohesion: 0.22
Nodes (9): 1. The agent loop, Budget and usage-limit awareness, The approval queue (mailbox), The backlog: one file per task, The checker (inspector), The parts, The planner (sometimes), The worker (farmhand) (+1 more)

### Community 179 - "Shipping setup (US, from Indiana)"
Cohesion: 0.22
Nodes (8): Before the first real order, Each order, step by step (5 minutes), Labels: Pirate Ship (free), Read this first: Thanksgiving and Christmas, Returns and refunds (the draft policy on the site), Shipping setup (US, from Indiana), The calculator has a lithium battery, What to buy (about $60 to start)

### Community 180 - "penny.cjs"
Cohesion: 0.31
Nodes (7): CATEGORIES, customerLines(), draftPrompt(), fmtDate(), fs, parseDraft(), path

### Community 181 - "run.cjs"
Cohesion: 0.36
Nodes (6): fs, killTree(), parseReset(), runClaude(), { spawn, spawnSync }, summarizeInput()

### Community 182 - "setup"
Cohesion: 0.33
Nodes (6): cameraName(), setup(), statusText(), typeKeys(), screenBegin(), examLoad()

### Community 183 - "Should Proofline be an LLC? What you gain"
Cohesion: 0.29
Nodes (6): Cost in Indiana, Right now, Should Proofline be an LLC? What you gain, Steps (about 1–2 evenings), What an LLC gives you, What it doesn't do

### Community 184 - "Proofline Farm: plan for the always-on AI ecosystem app"
Cohesion: 0.29
Nodes (7): 3. The app UI, 4. Your decisions (2026-10-18), How buttons work without a server, Proofline Farm: plan for the always-on AI ecosystem app, The idea in one paragraph, The summary you asked for (decision 4), Things to check before building

### Community 185 - "5. Build order (updated for your decisions)"
Cohesion: 0.29
Nodes (7): 5. Build order (updated for your decisions), Phase 0 — done, Phase 1 — farmhands work overnight on their own (weekend), Phase 2 — phone view + mailbox (moved up, decision 5), Phase 3 — the full app on the PC + planner, Phase 4 — the second laptop, Phase 5 — polish

### Community 186 - "HistoryItem"
Cohesion: 0.33
Nodes (3): HistoryItem, expr, value

### Community 187 - "Back office: business e-mail, Penny, and Business HQ"
Cohesion: 0.33
Nodes (5): Back office: business e-mail, Penny, and Business HQ, How Penny works, Part 1: get the e-mail address (about 45 minutes, about $11/year), Part 2: connect the farm (2 minutes), Worker API (all need the `x-admin-token` header)

### Community 188 - "Stripe integration review: 2026-10-14"
Cohesion: 0.33
Nodes (5): 1. Planner summary (Stripe integration planner, guide `iguide_61VZspl4YBYPFo7H441FMsMHP8Dwz`, accepted), 2. What is already good (keep it), 4. Sandbox setup checklist (what the code expects), 5. Top 5 recommendations, Stripe integration review: 2026-10-14

### Community 189 - "Wi-Fi watchdog"
Cohesion: 0.33
Nodes (6): Install (easiest way, about 1 minute), Or set it up by hand in Task Scheduler, Reading what happened, Test it safely (doesn't touch the adapter), Things to know, Wi-Fi watchdog

### Community 191 - "Kind"
Cohesion: 0.40
Nodes (5): Kind, Captured, Fail, Partial, Reply

### Community 192 - "prompts.cjs"
Cohesion: 0.50
Nodes (4): checkerPrompt(), indent(), workerPrompt(), workerRules()

### Community 193 - "2. Where it runs without paying for a website"
Cohesion: 0.50
Nodes (4): 2. Where it runs without paying for a website, Subscription vs API tokens (rough numbers, check current prices), Where the work runs, Where you look at it (no paid hosting)

### Community 194 - "3b. Keeping the Wi-Fi alive overnight"
Cohesion: 0.50
Nodes (4): 3b. Keeping the Wi-Fi alive overnight, Fix 1: the watchdog (the automatic off-and-on), Fix 2: the farm handles drops calmly, Fix 3: stop the drops at the source (do these once, cheapest first)

### Community 195 - "Weekly usage budget: always leave enough for your own use"
Cohesion: 0.50
Nodes (4): The rules, Weekly usage budget: always leave enough for your own use, What you see, Where the "% used" numbers come from

### Community 196 - "NormMode"
Cohesion: 0.67
Nodes (3): NormMode, Norm1, Norm2

## Ambiguous Edges - Review These
- `AI Hold-to-Capture Flow` → `Exam Mode`  [AMBIGUOUS]
  tests/golden/exam_confirm.txt · relation: conceptually_related_to

## Knowledge Gaps
- **1411 isolated node(s):** `On`, `Off`, `AC`, `Eq`, `Up` (+1406 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 1744 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **22 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `AI Hold-to-Capture Flow` and `Exam Mode`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **Why does `Device` connect `Device` to `View`, `string`, `Claude outputs/core/device.cpp`, `Claude outputs/firmware/src/claude_client.h`, `keyCalc`, `Mode`, `ScanNote`?**
  _High betweenness centrality (0.042) - this node is a cross-community bridge._
- **Why does `status()` connect `solve.ts` to `portal.js`, `account.ts`, `api/src/index.ts`?**
  _High betweenness centrality (0.024) - this node is a cross-community bridge._
- **Why does `newCode()` connect `portal.js` to `manager.cjs`, `solve.ts`?**
  _High betweenness centrality (0.023) - this node is a cross-community bridge._
- **What connects `On`, `Off`, `AC` to the rest of the system?**
  _1411 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Parser` be split into smaller, more focused modules?**
  _Cohesion score 0.07236154649947753 - nodes in this community are weakly interconnected._
- **Should `Parser` be split into smaller, more focused modules?**
  _Cohesion score 0.06941323345817728 - nodes in this community are weakly interconnected._