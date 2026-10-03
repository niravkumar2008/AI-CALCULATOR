# Graph Report - AI-CALCULATOR  (2026-10-03)

## Corpus Check
- 155 files · ~1,062,898 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 150 file(s) not represented in the graph (top: .stl 63, .kicad_mod 28, .kicad_sch 14)

## Summary
- 2022 nodes · 3860 edges · 114 communities (92 shown, 22 thin omitted)
- Extraction: 89% EXTRACTED · 11% INFERRED · 0% AMBIGUOUS · INFERRED: 418 edges (avg confidence: 0.84)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `bbc516b9`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- make_fitcheck.py
- Num
- ai_calc_pcb_8/hardware/tools/fixroute.py
- Tok
- build_case.py
- win32_main.cpp
- Device
- build_fx115es_replica.py
- Stage 11: e-paper moved to the measured window, calipers applied, review fixes (ai_calc_pcb_11)
- DKey
- Json
- drawCalc
- testFont
- Fit check: dummy board + grind map
- web_main.cpp
- firmware/src/camera.cpp
- firmware-prototype/src/camera.cpp
- previewResume
- device.cpp
- AI Answer Flow
- Stage 4: full calculator
- string
- AI Calculator PCB Handoff
- tests.cpp
- AI Calculator launch roadmap
- ai_calc_pcb_8/hardware/geometry/derive_geometry.py
- gen_font.py
- kicad_summary.py
- firmware-prototype/src/settings.cpp
- firmware-prototype/src/preview.cpp
- App
- Button
- Focus
- exprText
- Router
- pilroute.py
- Framebuffer
- firmware/src/preview.cpp
- firmware-prototype/src/main.cpp
- Independent electrical + manufacturability review (2026-10-03)
- SerialCmd
- Back cover (engraved 'AI CALCULATOR rev A, 6 x M2x10')
- ai_calc_pcb_8/hardware/tools/build_board.py
- AI Calculator Board Concept Layout v2
- Decode
- keys.cpp
- Tester firmware README (Stage 4)
- hardware/tools/build_board.py
- math
- Vars
- Root sheet ai_calc.kicad_sch
- Effort
- CalcError
- AI Calculator Multiple Choice Test
- Decode
- Kind
- View
- SerialCmd
- previewResume
- firmware/src/settings.cpp
- Stage 10: moving the board to the Casio fx-115ES shell
- Breadboard Wiring Diagram (exact, camera up)
- hardware/tools/fixroute.py
- Stage-8 board ai_calc.kicad_pcb (72.3x148.8 mm, 0.8 mm, 2 layers, 140 footprints, 4630 tracks, 322 vias, 92 nets, GND pour)
- firmware-prototype/src/claude_client.h
- string
- Prototype firmware README (board inside the Casio)
- Stage 9: finishing the v8 board (ai_calc_pcb_9)
- sim_state
- Screen
- SolveResult
- firmware/src/main.cpp
- PR_hardware-stage11-roadmap.md
- Keypad 8x10 matrix (ROW0-7, COL0-9, SW1-SW50)
- Router
- Alias
- algorithm
- full.sh script
- Tuning
- Key
- U1 ESP32-S3-MINI-1-N4R2 (MCU module)
- HistoryItem
- fx-115ES shell replica (unbranded)
- Board geometry from the scan: assumptions and confidence
- Photo notes for the fx-115ES replica
- PROGRESS.md
- stage11_progress.md
- Tuning
- Calculator Mode
- ScanNote
- Menu System
- graphify skill trigger (/graphify)
- sim/web/build.sh
- Browser simulator page (page.html)
- hardware/tools/prepass.py
- Router
- AI Calculator PCB — handoff for the KiCad Claude Code session
- Stage 2: electrical plan (pins, power, keypad)
- JLCPCB order checklist (fill in when the VERIFY items are done)
- Stage 3: schematic (done)
- Stages 4 + 5: footprints and placement
- full.sh script
- sys
- hardware/tools/make_outputs.sh

## God Nodes (most connected - your core abstractions)
1. `Device` - 110 edges
2. `Tok` - 91 edges
3. `DKey` - 70 edges
4. `App` - 55 edges
5. `Num` - 45 edges
6. `Parser` - 42 edges
7. `Framebuffer` - 28 edges
8. `Json` - 27 edges
9. `AI Calculator PCB Handoff` - 22 edges
10. `Failure` - 20 edges

## Surprising Connections (you probably didn't know these)
- `4. Keypad (50 contacts = 49 matrix + ON)` --references--> `DKey`  [INFERRED]
  hardware/geometry_assumptions.md → core/device.h
- `Tester hardware: ESP32-S3-CAM (OV3660) + Waveshare 2.13in e-paper HAT V4` --semantically_similar_to--> `U1 ESP32-S3-MINI-1-N4R2 (MCU module)`  [INFERRED] [semantically similar]
  firmware/README.md → Claude outputs/ai_calc_pcb_8/hardware/kicad/DESIGN_SUMMARY.md
- `src/claude_client (HTTPS streaming to Claude)` --semantically_similar_to--> `directScan() (browser fetch to Claude API)`  [INFERRED] [semantically similar]
  firmware/README.md → sim/web/page.html
- `kMatrix in src/keys.cpp (key at each matrix crossing)` --semantically_similar_to--> `TOP / FN / NUM key layout tables`  [INFERRED] [semantically similar]
  firmware-prototype/README.md → sim/web/page.html
- `3D-printed front shell` --semantically_similar_to--> `Casio fx-300ES Plus original case/PCB`  [INFERRED] [semantically similar]
  hardware/enclosure/renders/exploded_iso_front.png → Claude outputs/overlay_front.png

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **AI Calculator Hardware Circuit (ESP32-S3-CAM + e-paper + buttons on two breadboards)** — claude_outputs_bb_exact_esp32_s3_cam, claude_outputs_bb_exact_epaper_2_13, claude_outputs_bb_exact_calculator_buttons, claude_outputs_bb_exact_long_breadboard, claude_outputs_bb_exact_medium_breadboard [EXTRACTED 1.00]
- **Custom enclosure assembly stack (lens, front shell, keymat, back cover)** — hardware_enclosure_acrylic_lens, hardware_enclosure_front_shell, hardware_enclosure_tpu_keymat, hardware_enclosure_back_cover [EXTRACTED 1.00]
- **Ribbon Cable Orientation Risk Mitigation** — claude_outputs_ai_calc_pcb_8_hardware_stage4_5_layout_j1_cam_reversed_footprint, claude_outputs_ai_calc_pcb_8_hardware_stage4_5_layout_shou_han_c6364666, claude_outputs_ai_calc_pcb_8_hardware_stage4_5_layout_fpc_pin1_beep_check, claude_outputs_ai_calc_pcb_8_hardware_stage4_5_layout_epaper_fpc_slot, claude_outputs_ai_calc_pcb_8_hardware_stage4_5_layout_j2_epaper_socket [EXTRACTED 1.00]
- **Simulator scan request flow** — sim_web_page_servicerequests, sim_web_page_startscan, sim_web_page_demoscan, sim_web_page_directscan, sim_web_page_claudescan, sim_web_page_wasm_sim_api [EXTRACTED 1.00]
- **Stage 6 Layout Build Pipeline (full.sh)** — claude_outputs_ai_calc_pcb_8_hardware_stage4_5_layout_full_sh, claude_outputs_ai_calc_pcb_8_hardware_tools_build_board, claude_outputs_ai_calc_pcb_8_hardware_tools_placement_check, claude_outputs_ai_calc_pcb_handoff_route_sh, claude_outputs_ai_calc_pcb_8_hardware_tools_add_gnd, claude_outputs_ai_calc_pcb_8_hardware_tools_cleanup, claude_outputs_ai_calc_pcb_8_hardware_tools_make_outputs [EXTRACTED 1.00]
- **Problem capture quality: hold orientation, preview, preprocessing** — claude_outputs_how_to_hold_camera_orientation, claude_outputs_how_to_hold_preview_check, claude_outputs_cmp_image_preprocessing, claude_outputs_cmp_sample_math_problems [INFERRED 0.75]
- **Camera/OCR solver validation suite (printed + handwritten rounds scored against answer keys)** — claude_outputs_ai_calculator_multiple_choice_test, claude_outputs_ai_calculator_test_worksheet, claude_outputs_ai_calculator_test_worksheet_scan_protocol, claude_outputs_ai_calculator_multiple_choice_test_answer_key, claude_outputs_ai_calculator_test_worksheet_answer_key [INFERRED 0.85]
- **AI Capture-to-Answer Pipeline** — tests_golden_ai_ready_ai_ready_screen, tests_golden_ai_hold_countdown_ai_hold_countdown_screen, tests_golden_ai_hold_captured_ai_hold_captured_screen, tests_golden_ai_hold_done_ai_hold_done_screen, tests_golden_busy_reading_busy_reading_screen, tests_golden_busy_solving_busy_solving_screen, tests_golden_answer_streaming_answer_streaming_screen, tests_golden_ai_answer_ai_answer_screen [INFERRED 0.85]
- **Battery/USB power path** — claude_outputs_ai_calc_pcb_8_hardware_kicad_design_summary_j3_magnetic_usb, claude_outputs_ai_calc_pcb_8_hardware_kicad_design_summary_u2_mcp73831, claude_outputs_ai_calc_pcb_8_hardware_kicad_design_summary_power_path_pfets, claude_outputs_ai_calc_pcb_8_hardware_kicad_design_summary_u3_ap2112k, claude_outputs_ai_calc_pcb_8_hardware_kicad_design_summary_j4_battery_jst [INFERRED 0.85]
- **ESP32-S3 peripherals (camera, e-paper, keypad scanner)** — claude_outputs_layout_concept_v2_esp32_s3_mini_1, claude_outputs_layout_concept_v2_ov5640_camera, claude_outputs_layout_concept_v2_epaper_display, claude_outputs_layout_concept_v2_tca8418 [INFERRED 0.85]
- **Board power path (magnet connector, charger/3.3V, LiPo)** — claude_outputs_layout_concept_v2_magnetic_connector, claude_outputs_layout_concept_v2_charger_3v3_esd, claude_outputs_layout_concept_v2_lipo_401230 [INFERRED 0.85]
- **Section views verifying internal fit** — hardware_enclosure_renders_section_display, hardware_enclosure_renders_section_keys, hardware_enclosure_renders_section_left, hardware_enclosure_renders_section_iso_left [INFERRED 0.85]
- **Exam Mode Lockdown** — tests_golden_exam_confirm_exam_mode_confirm_screen, tests_golden_exam_status_bar_exam_status_bar_screen, tests_golden_exam_wifi_off_exam_wifi_off_screen [INFERRED 0.85]
- **Keypad layout shared across board, firmware, simulator and case** — claude_outputs_ai_calc_pcb_8_hardware_kicad_design_summary_keypad_matrix, firmware_prototype_readme_kmatrix, sim_web_page_key_tables, hardware_enclosure_readme_keymat, hardware_enclosure_readme_key_layout_mismatch [INFERRED 0.85]
- **Magnet Contacts Never Carry Battery Voltage** — claude_outputs_ai_calc_pcb_handoff_magnet_no_battery_voltage, claude_outputs_ai_calc_pcb_8_hardware_stage2_electrical_adafruit_5358_magnetic_connector, claude_outputs_ai_calc_pcb_8_hardware_stage2_electrical_load_sharing_power_path, claude_outputs_ai_calc_pcb_8_hardware_stage3_schematic_vbus_sense_divider, claude_outputs_ai_calc_pcb_8_hardware_stage3_schematic_usblc6_ref_on_3v3, claude_outputs_ai_calc_pcb_8_hardware_stage3_schematic_d7_smf5_0a_tvs, claude_outputs_ai_calc_pcb_8_hardware_stage2_electrical_usblc6_2 [INFERRED 0.85]
- **Enclosure assembly stack (front shell, keymat, display, PCB, back cover)** — hardware_enclosure_renders_side_front_shell, hardware_enclosure_renders_section_keys_tpu_keymat, hardware_enclosure_renders_section_display_epaper_stack, hardware_enclosure_renders_section_left_pcb, hardware_enclosure_renders_iso_back_back_cover [INFERRED 0.95]
- **Same wiring drawn in three orientations** — claude_outputs_bb_exact_wiring_diagram, claude_outputs_bb_real_wiring_diagram, claude_outputs_bb_top_wiring_diagram [INFERRED 0.95]

## Communities (114 total, 22 thin omitted)

### Community 0 - "make_fitcheck.py"
Cohesion: 0.15
Nodes (3): main(), newest(), ring()

### Community 1 - "Num"
Cohesion: 0.10
Nodes (34): addOk(), asInt(), endsOperand(), exactPlus(), exactTimes(), formatDecimal(), formatResult(), fractionText() (+26 more)

### Community 2 - "ai_calc_pcb_8/hardware/tools/fixroute.py"
Cohesion: 0.18
Nodes (4): anchor_points(), items_geom(), raster(), route()

### Community 3 - "Tok"
Cohesion: 0.03
Nodes (71): Tok, Abs, Acos, Acosh, Add, Ans, Asin, Asinh (+63 more)

### Community 4 - "build_case.py"
Cohesion: 0.07
Nodes (55): appearance(), _blocks(), body_named(), chamfer(), check_outline(), circle(), cm(), comp_named() (+47 more)

### Community 5 - "win32_main.cpp"
Cohesion: 0.05
Nodes (35): Failure, ApiBusy, ApiError, BadReply, Camera, NoApiKey, NoConnection, Timeout (+27 more)

### Community 6 - "Device"
Cohesion: 0.04
Nodes (38): Device, alpha_, angle_, app_, back_, battery_, blink_, charging_ (+30 more)

### Community 7 - "build_fx115es_replica.py"
Cohesion: 0.05
Nodes (63): appearance(), area(), body_named(), cavity(), chain(), check(), circle(), circle_pts() (+55 more)

### Community 8 - "Stage 11: e-paper moved to the measured window, calipers applied, review fixes (ai_calc_pcb_11)"
Cohesion: 0.22
Nodes (8): Assumptions made in this stage, Caliper readings, how each was used, Firmware to-do (from the review), Questions for Nirav, Stage 11: e-paper moved to the measured window, calipers applied, review fixes (ai_calc_pcb_11), The e-paper ribbon (FPC) length budget, Things I checked and left alone (robustness review), What changed, and why

### Community 9 - "DKey"
Cohesion: 0.04
Nodes (52): DKey, Abs, AC, Add, Alpha, Ans, Close, Cos (+44 more)

### Community 10 - "Json"
Cohesion: 0.09
Nodes (16): Json, a_, b_, n_, o_, s_, JsonParser, err_ (+8 more)

### Community 11 - "drawCalc"
Cohesion: 0.30
Nodes (7): drawCalc, decodeUtf8(), encodeUtf8(), textLength(), wrapParagraph(), wrapText(), testWrap()

### Community 12 - "testFont"
Cohesion: 0.42
Nodes (8): findGlyph(), Glyph, cp, rows, hasGlyph(), lookup(), normalizeCodepoint(), testFont()

### Community 13 - "Fit check: dummy board + grind map"
Cohesion: 0.40
Nodes (4): 1. Grind map (paper, 1:1), 2. Dummy board (3D print), Don't grind, Fit check: dummy board + grind map

### Community 14 - "web_main.cpp"
Cohesion: 0.05
Nodes (60): effortParam, base64Encode(), buildSolveRequest(), classifyFailure(), jsonString(), peekAnswer(), solveInstructions(), solveSchema() (+52 more)

### Community 15 - "firmware/src/camera.cpp"
Cohesion: 0.10
Nodes (25): applyAll(), autoTuneLocked(), cameraAutoTune(), cameraBegin(), cameraCapture(), cameraFocusScore(), cameraFrame(), cameraHoldEstimateMs() (+17 more)

### Community 16 - "firmware-prototype/src/camera.cpp"
Cohesion: 0.11
Nodes (30): applyAll(), autoTuneLocked(), cameraAutoTune(), cameraBegin(), cameraCapture(), cameraFocusScore(), cameraFrame(), cameraHoldEstimateMs() (+22 more)

### Community 17 - "previewResume"
Cohesion: 0.33
Nodes (8): previewEnded(), endPreview(), jsonText(), previewResume(), previewSetResult(), previewStart(), startNetwork(), startServers()

### Community 18 - "device.cpp"
Cohesion: 0.14
Nodes (28): evaluate(), continuesAns(), clearEntry, endExam, equals, insert, keyAi, keyCalc (+20 more)

### Community 19 - "AI Answer Flow"
Cohesion: 0.07
Nodes (34): AI Answer Screen, AI Answer Flow, AI Error States, AI Hold-to-Capture Flow, AI Hold Captured Screen, AI Hold Countdown Screen, AI Hold Done Screen, AI Ready Screen (+26 more)

### Community 20 - "Stage 4: full calculator"
Cohesion: 0.09
Nodes (25): core static library (core/*.cpp, C++17), Win32 simulator executable (sim/win32_main.cpp), tests executable (tests/tests.cpp), AI Calculator project, AI SOLVE mode (MODE 4), Browser simulator (Emscripten build of core), Casio fx-300ES PLUS (COMP mode emulation), Claude Sonnet 5.5 model (+17 more)

### Community 21 - "string"
Cohesion: 0.17
Nodes (3): Line, header, text

### Community 22 - "AI Calculator PCB Handoff"
Cohesion: 0.06
Nodes (63): Board Geometry Assumptions, Battery Bay (LR44 corner), Firmware Row-2 Key Mismatch (Abs, x^3, x^-1, log_a b), Keypad Edge = Casio Edge - 0.25 mm, keys.csv / features.json, KiCad Top View = Back-Cover View; Keys on B.Cu, Magnetic Connector Top-Wall Slot, Scale 7.55 px/mm (+55 more)

### Community 23 - "tests.cpp"
Cohesion: 0.24
Nodes (12): calcText(), keys(), main(), readFile(), sampleStream(), snapshot(), testDevice(), testDeviceAi() (+4 more)

### Community 24 - "AI Calculator launch roadmap"
Cohesion: 0.10
Nodes (19): 0. The short version, 1. Parts list for the first build (2 assembled boards out of 5), 1A. Must be ordered MONDAY 10/5 (fastest shipping listed), 1B. Buy locally THIS WEEKEND (no shipping wait), 1C. Don't buy yet, 2. Timeline, 2A. Day by day, 10/3 → 10/21 (100 % effort), 2B. Phases after the first units (+11 more)

### Community 25 - "ai_calc_pcb_8/hardware/geometry/derive_geometry.py"
Cohesion: 0.11
Nodes (17): board_polygon(), front_silhouette(), keys(), main(), mm(), overlays(), poly_mm(), to_back() (+9 more)

### Community 27 - "kicad_summary.py"
Cohesion: 0.42
Nodes (7): board_summary(), kids(), main(), natkey(), parse(), props(), sheet_summary()

### Community 28 - "firmware-prototype/src/settings.cpp"
Cohesion: 0.16
Nodes (15): cameraName(), setup(), screenBegin(), Ask, Key, Nothing, Password, Ssid (+7 more)

### Community 29 - "firmware-prototype/src/preview.cpp"
Cohesion: 0.34
Nodes (16): authorised(), autotuneHandler(), detailHandler(), focusHandler(), frameHandler(), lastHandler(), noCache(), pageHandler() (+8 more)

### Community 30 - "App"
Cohesion: 0.09
Nodes (34): App, accepts, activeId_, busySinceMs_, captured_, expression_, hasKey_, holdUntilMs_ (+26 more)

### Community 31 - "Button"
Cohesion: 0.33
Nodes (6): Button, key, lastMs, nextRepeat, pin, pressed

### Community 32 - "Focus"
Cohesion: 0.11
Nodes (24): analyseFocus(), Crop, h, use, w, x, y, detailCrop() (+16 more)

### Community 33 - "exprText"
Cohesion: 1.00
Nodes (3): exprText(), tokText(), testEngine()

### Community 35 - "pilroute.py"
Cohesion: 0.09
Nodes (7): connect_all(), island_at(), islands(), item_pts(), net_pts(), pad_pts(), polyline()

### Community 36 - "Framebuffer"
Cohesion: 0.08
Nodes (25): render, Framebuffer, clear, drawText, drawTextPx, fillRect, get, icons_ (+17 more)

### Community 37 - "firmware/src/preview.cpp"
Cohesion: 0.34
Nodes (16): authorised(), autotuneHandler(), detailHandler(), focusHandler(), frameHandler(), lastHandler(), noCache(), pageHandler() (+8 more)

### Community 38 - "firmware-prototype/src/main.cpp"
Cohesion: 0.07
Nodes (46): keysForChar(), cameraHoldUntil(), cameraSleep(), keysSleepUntilPress(), applySettings(), batteryPercent(), charging(), connectWifi() (+38 more)

### Community 39 - "Independent electrical + manufacturability review (2026-10-03)"
Cohesion: 0.12
Nodes (15): B1. `fab/` outputs and the DRC report are stale relative to the board file, BLOCKER, Checked and OK (no action), Independent electrical + manufacturability review (2026-10-03), LCSC check (2026-10-03, lcsc.com product pages; all exist and are active), NICE (optional), Plain-English summary, S1. R20 (CHG_STAT pull-up to +3V3) back-feeds the charger and leaks about 20 µA in standby (+7 more)

### Community 40 - "SerialCmd"
Cohesion: 0.13
Nodes (13): SerialCmd, Changed, ExamOff, Keys, None, Preview, PreviewOff, Scan (+5 more)

### Community 41 - "Back cover (engraved 'AI CALCULATOR rev A, 6 x M2x10')"
Cohesion: 0.13
Nodes (22): Front view render (enclosure), Display window with 'AI CALCULATOR' wordmark, Round D-pad (4-way cursor key), Casio-style keypad layout (SHIFT/ALPHA/D-pad/MODE/ON, Abs x^3 x^-1 log_a b row, 5-col number block), Isometric back render (back cover), Back cover (engraved 'AI CALCULATOR rev A, 6 x M2x10'), 6x M2x10 countersunk screw fastening, Back cover camera window (OV5640) (+14 more)

### Community 42 - "ai_calc_pcb_8/hardware/tools/build_board.py"
Cohesion: 0.26
Nodes (7): add_edge_keepout(), add_epaper_slot(), add_rule_areas(), fp_path(), main(), patch_rules(), rule_area()

### Community 43 - "AI Calculator Board Concept Layout v2"
Cohesion: 0.16
Nodes (20): AI Calculator Board Concept Layout v2, Charger + 3.3 V regulator + ESD block, E-paper socket + driver and window, ESP32-S3 MINI-1 module, Keypad layout (fx-300ES style key grid), LiPo 401230 battery (~110 mAh), Magnetic connector (5358) in solar-cell slot, OV5640 camera + 24-pin socket (+12 more)

### Community 44 - "Decode"
Cohesion: 0.17
Nodes (11): Decode, cx, cy, fullH, fullW, jpeg, lastYield, out (+3 more)

### Community 45 - "keys.cpp"
Cohesion: 0.15
Nodes (12): dkeyName(), beginOn(), keysBegin(), keysPoll(), pollOn(), Repeat, key, next (+4 more)

### Community 46 - "Tester firmware README (Stage 4)"
Cohesion: 0.13
Nodes (19): Tester firmware README (Stage 4), src/claude_client (HTTPS streaming to Claude), core/ shared calculator logic, Exam mode (Wi-Fi off, persisted, USB unlock or 12 h), src/screen (125x61 calc screen on 250x122 e-paper), Tester hardware: ESP32-S3-CAM (OV3660) + Waveshare 2.13in e-paper HAT V4, buildKeys(), claudeScan() (scan via host sampleFn) (+11 more)

### Community 47 - "hardware/tools/build_board.py"
Cohesion: 0.23
Nodes (7): add_edge_keepout(), add_epaper_slot(), add_rule_areas(), fp_path(), main(), patch_rules(), rule_area()

### Community 48 - "math"
Cohesion: 0.08
Nodes (13): full.sh build pipeline, add_via(), free(), obstacles(), via_near_pad(), connect_tree(), pad(), stub() (+5 more)

### Community 49 - "Vars"
Cohesion: 0.17
Nodes (12): Vars, a, ans, b, c, d, e, f (+4 more)

### Community 50 - "Root sheet ai_calc.kicad_sch"
Cohesion: 0.14
Nodes (18): Stage-8 KiCad Design Summary (routed board), U4/U5 ME6211 camera LDOs (2.8 V / 1.5 V), E-paper boost circuit (L1 68uH, Q3 SI1308, D3-D5, R12 3R), J1 Camera FPC 24P dual-contact (OV5640, reversed), J2 E-paper FPC 24P dual-contact (2.13in V4), kicad_summary.py (summary generator), Net +3V3, Net SYS (load-shared system rail) (+10 more)

### Community 52 - "Effort"
Cohesion: 0.40
Nodes (4): Effort, Careful, Max, Normal

### Community 53 - "CalcError"
Cohesion: 0.11
Nodes (14): CalcError, Argument, Math, None, Stack, Syntax, errorText(), EvalResult (+6 more)

### Community 54 - "AI Calculator Multiple Choice Test"
Cohesion: 0.17
Nodes (13): AI Calculator Multiple Choice Test, Multiple Choice Answer Key and Score Sheet, CHOICE + option label output format, Closest-option selection when exact answer absent, MC Handwritten Round (H1-H4), Option label styles (A-D row, A-D stacked, (a)-(e), 1-4), AI Calculator Test Worksheet, Worksheet Answer Key and Score Sheet (+5 more)

### Community 55 - "Decode"
Cohesion: 0.18
Nodes (10): Decode, cx, cy, fullH, fullW, lastYield, out, outH (+2 more)

### Community 56 - "Kind"
Cohesion: 0.40
Nodes (5): Kind, Captured, Fail, Partial, Reply

### Community 57 - "View"
Cohesion: 0.14
Nodes (14): View, Ai, Calc, Clr, ClrConfirm, Error, ExamInfo, Hyp (+6 more)

### Community 58 - "SerialCmd"
Cohesion: 0.13
Nodes (13): SerialCmd, Changed, ExamOff, Keys, None, Preview, PreviewOff, Scan (+5 more)

### Community 59 - "previewResume"
Cohesion: 0.25
Nodes (10): previewEnded(), endPreview(), jsonText(), previewResume(), previewSetResult(), previewStart(), previewSuspend(), startNetwork() (+2 more)

### Community 60 - "firmware/src/settings.cpp"
Cohesion: 0.17
Nodes (16): cameraName(), setup(), statusText(), screenBegin(), Ask, Key, Nothing, Password (+8 more)

### Community 61 - "Stage 10: moving the board to the Casio fx-115ES shell"
Cohesion: 0.22
Nodes (8): Board changes made in stage 10 (DRC: 0 errors, 0 unconnected, 0 schematic mismatches), Caliper readings used so far (2026-10-03), How the photos were measured, Stage 10: moving the board to the Casio fx-115ES shell, Stage 11a: e-paper moved to the measured window (calipers C12/C13), Still from the old shell (not measurable in the photos), The back cover (photo `2f54d6c7`, fitted on its 6 screw holes, about ±1 mm), Things in the way, seen in the photos

### Community 62 - "Breadboard Wiring Diagram (exact, camera up)"
Cohesion: 0.23
Nodes (14): Calculator Buttons (=, AC, Up, Down), 2.13in E-Paper Display, ESP32-S3-CAM Board, LONG Breadboard, MEDIUM Breadboard, Breadboard Wiring Diagram (exact, camera up), Breadboard Wiring Diagram (real orientation, USB away), Breadboard Wiring Diagram (top, USB toward you) (+6 more)

### Community 63 - "hardware/tools/fixroute.py"
Cohesion: 0.11
Nodes (8): net_pts(), pad_pts(), anchor_points(), items_geom(), raster(), route(), net_pts(), pad_pts()

### Community 64 - "Stage-8 board ai_calc.kicad_pcb (72.3x148.8 mm, 0.8 mm, 2 layers, 140 footprints, 4630 tracks, 322 vias, 92 nets, GND pour)"
Cohesion: 0.19
Nodes (11): Custom footprint library ai_calc.pretty (stage 8), Stage-8 board ai_calc.kicad_pcb (72.3x148.8 mm, 0.8 mm, 2 layers, 140 footprints, 4630 tracks, 322 vias, 92 nets, GND pour), AI Calculator custom case (rev A) README, build_case.py (parametric Fusion case from KiCad), fusion_run.py (sends script to Fusion), FusionMCPBridge add-in, TPU keymat (46 caps + REPLAY rocker), Case Z stack-up (77.7 x 156.2 x 13.6 mm) (+3 more)

### Community 65 - "firmware-prototype/src/claude_client.h"
Cohesion: 0.12
Nodes (10): SolveCallbacks, cancelled, fail, partial, reply, SolveCallbacks, cancelled, fail (+2 more)

### Community 67 - "string"
Cohesion: 0.29
Nodes (9): clockText(), drawMenu, drawStatus, engText, render, resultText, setNetInfo, pad() (+1 more)

### Community 68 - "Prototype firmware README (board inside the Casio)"
Cohesion: 0.24
Nodes (12): D7 SMF5.0A VBUS TVS, J3 Magnetic USB (Adafruit 5358), J4 Battery JST-PH (Adafruit 1570), Net BAT+, Net VBUS, Sheet: Connectors - magnet USB, battery (connectors.kicad_sch), U2 MCP73831 LiPo charger, U7 USBLC6-2SC6 USB ESD protection (+4 more)

### Community 69 - "Stage 9: finishing the v8 board (ai_calc_pcb_9)"
Cohesion: 0.40
Nodes (4): Firmware (done in this stage), Stage 9: finishing the v8 board (ai_calc_pcb_9), Still open before ordering, What changed, and why

### Community 71 - "sim_state"
Cohesion: 0.12
Nodes (11): AngleUnit, Deg, Gra, Rad, NormMode, Norm1, Norm2, Mode (+3 more)

### Community 73 - "Screen"
Cohesion: 0.25
Nodes (7): Screen, Busy, Message, Off, Ready, Result, Warning

### Community 74 - "SolveResult"
Cohesion: 0.14
Nodes (10): parseSolveResult(), SolveResult, answer, choice, confidence, expression, readable, readAs (+2 more)

### Community 79 - "firmware/src/main.cpp"
Cohesion: 0.08
Nodes (36): cameraHoldUntil(), applySettings(), connectWifi(), Event, confidence, failure, id, kind (+28 more)

### Community 80 - "PR_hardware-stage11-roadmap.md"
Cohesion: 0.50
Nodes (3): Reviewer notes, What changed, Why

### Community 81 - "Keypad 8x10 matrix (ROW0-7, COL0-9, SW1-SW50)"
Cohesion: 0.31
Nodes (9): Keypad 8x10 matrix (ROW0-7, COL0-9, SW1-SW50), Sheet: Keypad and TCA8418 (keypad.kicad_sch), U6 TCA8418 keypad scanner (I2C), kMatrix in src/keys.cpp (key at each matrix crossing), Serial Monitor commands (wifi, key, status, preview, scan, snap, forget, keys, exam off), keys command character-to-key table, Check-on-real-hardware list (VERIFY knobs: EPD_T, TRAVEL, PILOT_D, ...), Key layout mismatch check (#10: board row 2 vs firmware key table) (+1 more)

### Community 90 - "Alias"
Cohesion: 0.67
Nodes (3): Alias, text, tok

### Community 93 - "full.sh script"
Cohesion: 0.38
Nodes (4): drc(), full.sh script, route.sh script, run.sh script

### Community 95 - "Tuning"
Cohesion: 0.40
Nodes (5): Tuning, def, max, min, name

### Community 96 - "Key"
Cohesion: 0.29
Nodes (7): Key, AC, Down, Eq, Off, On, Up

### Community 97 - "U1 ESP32-S3-MINI-1-N4R2 (MCU module)"
Cohesion: 0.60
Nodes (6): Camera DVP bus (CAM_D0-7, PCLK, HREF, VSYNC, XCLK, SIOC/SIOD), E-paper SPI (EPD_CLK/DIN/CS/DC/RST/BUSY), I2C bus (I2C_SCL/I2C_SDA, KEYPAD_INT), Net CHG_STAT (via D6 level clamp from CHG_STAT_RAW), U1 ESP32-S3-MINI-1-N4R2 (MCU module), firmware-prototype src/pins.h

### Community 99 - "HistoryItem"
Cohesion: 0.50
Nodes (3): HistoryItem, expr, value

### Community 101 - "fx-115ES shell replica (unbranded)"
Cohesion: 0.25
Nodes (7): Every dimension and where it came from, Files, fx-115ES shell replica (unbranded), Known simplifications, Measurements that would make it perfect, Rebuild, What is in the model

### Community 102 - "Board geometry from the scan: assumptions and confidence"
Cohesion: 0.20
Nodes (9): 1. What the scan shows (orientation), 2. Scale, 3. Geometry table, 4. Keypad (50 contacts = 49 matrix + ON), 5. Choices made where two readings were possible, 6. Carry-forward findings, 7. VERIFY list (lowest confidence first), Board geometry from the scan: assumptions and confidence (+1 more)

### Community 110 - "Tuning"
Cohesion: 0.40
Nodes (5): Tuning, def, max, min, name

### Community 114 - "Calculator Mode"
Cohesion: 0.40
Nodes (5): Calculator Editing Screen, Calculator Math Error Screen, Calculator Fraction Result Screen, Calculator STO Screen, Calculator Mode

### Community 121 - "ScanNote"
Cohesion: 0.50
Nodes (4): ScanNote, None, Ok, Unclear

### Community 123 - "Menu System"
Cohesion: 0.50
Nodes (4): Hyperbolic Menu Screen, Mode Menu Screen, Setup Menu Screen, Menu System

### Community 138 - "hardware/tools/prepass.py"
Cohesion: 0.25
Nodes (3): connect_tree(), pad(), stub()

### Community 142 - "AI Calculator PCB — handoff for the KiCad Claude Code session"
Cohesion: 0.20
Nodes (9): AI Calculator PCB — handoff for the KiCad Claude Code session, Key decisions you must not undo, Open questions for Nirav, Product (fixed requirements), Stage 10 (ai_calc_pcb_10): moved to the fx-115ES shell, Stage 11 (ai_calc_pcb_11): e-paper at the measured window, calipers, review fixes, Stage 9 (ai_calc_pcb_9): v8 finished, Tools (in `hardware/tools/`; written for the cloud box, paths need adapting) (+1 more)

### Community 143 - "Stage 2: electrical plan (pins, power, keypad)"
Cohesion: 0.25
Nodes (7): 1. Pin map (final: `hardware/pins_final.h`), 2. Power, 3. Keypad, 4. Open items for later stages, Battery life (120 mAh, ~100 mAh usable), Stage 2: electrical plan (pins, power, keypad), Why the magnetic contacts never carry battery voltage

### Community 144 - "JLCPCB order checklist (fill in when the VERIFY items are done)"
Cohesion: 0.29
Nodes (6): After it arrives, Assembly options, Files (all in `fab/`), In the parts-placement preview, check these by eye, JLCPCB order checklist (fill in when the VERIFY items are done), PCB options

### Community 145 - "Stage 3: schematic (done)"
Cohesion: 0.29
Nodes (6): Changes made in the final review, Design-review fixes (2026-10-02, independent review of the whole schematic), Netlist sanity check (done by script), Notes for the firmware, Stage 3: schematic (done), Still open (needs your hardware, not blocking the board)

### Community 146 - "Stages 4 + 5: footprints and placement"
Cohesion: 0.29
Nodes (6): Footprints (all real JLCPCB/LCSC land patterns), How it's built (repeatable), Placement (KiCad view = looking at the board from the back cover), Stage 6: routing (first complete pass, 2026-10-02), Stages 4 + 5: footprints and placement, The two ribbon cables (the riskiest part of the board)

### Community 147 - "full.sh script"
Cohesion: 0.38
Nodes (4): drc(), full.sh script, route.sh script, run.sh script

### Community 152 - "sys"
Cohesion: 0.10
Nodes (4): crect(), rect(), crect(), rect()

## Ambiguous Edges - Review These
- `AI Hold-to-Capture Flow` → `Exam Mode`  [AMBIGUOUS]
  tests/golden/exam_confirm.txt · relation: conceptually_related_to

## Knowledge Gaps
- **571 isolated node(s):** `make_outputs.sh script`, `On`, `Off`, `AC`, `Eq` (+566 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 838 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **22 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `AI Hold-to-Capture Flow` and `Exam Mode`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **Why does `Device` connect `Device` to `Num`, `Tok`, `string`, `win32_main.cpp`, `HistoryItem`, `sim_state`, `firmware-prototype/src/main.cpp`, `drawCalc`, `keys.cpp`, `firmware/src/main.cpp`, `Vars`, `device.cpp`, `CalcError`, `string`, `tests.cpp`, `ScanNote`, `App`, `View`?**
  _High betweenness centrality (0.094) - this node is a cross-community bridge._
- **Why does `Tok` connect `Tok` to `Num`, `exprText`, `HistoryItem`, `Device`, `device.cpp`, `string`, `Alias`?**
  _High betweenness centrality (0.050) - this node is a cross-community bridge._
- **Why does `App` connect `App` to `firmware-prototype/src/claude_client.h`, `Framebuffer`, `Device`, `Screen`, `SolveResult`, `web_main.cpp`, `Effort`, `string`, `tests.cpp`?**
  _High betweenness centrality (0.037) - this node is a cross-community bridge._
- **What connects `make_outputs.sh script`, `On`, `Off` to the rest of the system?**
  _571 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Num` be split into smaller, more focused modules?**
  _Cohesion score 0.0958904109589041 - nodes in this community are weakly interconnected._
- **Should `Tok` be split into smaller, more focused modules?**
  _Cohesion score 0.028169014084507043 - nodes in this community are weakly interconnected._