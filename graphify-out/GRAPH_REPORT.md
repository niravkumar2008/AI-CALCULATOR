# Graph Report - AI-CALCULATOR  (2026-10-10)

## Corpus Check
- 414 files · ~5,903,937 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 327 file(s) not represented in the graph (top: .kicad_mod 81, .stl 69, .step 39)

## Summary
- 4604 nodes · 8949 edges · 276 communities (230 shown, 46 thin omitted)
- Extraction: 90% EXTRACTED · 10% INFERRED · 0% AMBIGUOUS · INFERRED: 905 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `095057e5`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DIR
- device.cpp
- mm
- Tok
- build_case.py
- win32_main.cpp
- Device
- geometry
- Stage 11: e-paper moved to the measured window, calipers applied, review fixes (ai_calc_pcb_11)
- DKey
- Json
- Ann
- JLCPCB order walkthrough (stage 14 board, silk "AI CALC v14 2026-10-04")
- Fit check: paper grind maps, dummy board, clearance table
- web_main.cpp
- firmware/src/camera.cpp
- battery_envelope.py
- Full tables
- firmware-prototype/src/main.cpp
- AI Answer Flow
- Stage 4: full calculator
- make_screens.py
- AI Calculator PCB Handoff
- check_requirements_11.py
- AI Calculator launch roadmap
- tests.cpp
- build_fx115es_replica.py
- stage_front
- firmware-prototype/src/setup_portal.cpp
- db.py
- App
- 12: Inner rib / pins / hooks re-check against the v14 board (2026-10-06, before ordering)
- Focus
- Stage 12: second caliper batch, new photos, CAD fit check (ai_calc_pcb_12)
- Vars
- Router
- Framebuffer
- firmware/src/preview.cpp
- Json
- Independent electrical + manufacturability review (2026-10-03)
- hardware/geometry/derive_geometry.py
- Back cover (engraved 'AI CALCULATOR rev A, 6 x M2x10')
- .function
- AI Calculator Board Concept Layout v2
- F
- hardware/tools/build_board.py
- pilroute.py
- 08 — Second-opinion PCB check (v14), re-derived from the raw files
- app.py
- font.h
- Root sheet ai_calc.kicad_sch
- solve.py
- firmware-prototype/src/camera.cpp
- selfTestRun
- AI Calculator Multiple Choice Test
- hardware/tools/fixroute.py
- stage_fitcheck
- View
- Competitor analysis: AI calculators (checked 2026-10-03)
- firmware-prototype/src/power.cpp
- claudeSolve
- Stage 10: moving the board to the Casio fx-115ES shell
- Breadboard Wiring Diagram (exact, camera up)
- ai_calc_pcb_8/hardware/geometry/derive_geometry.py
- Stage-8 board ai_calc.kicad_pcb (72.3x148.8 mm, 0.8 mm, 2 layers, 140 footprints, 4630 tracks, 322 vias, 92 nets, GND pour)
- policy.py
- stage_cell
- host_hw.cpp
- Prototype firmware README (board inside the Casio)
- Stage 9: finishing the v8 board (ai_calc_pcb_9)
- Nirav's caliper readings, fx-115ES shell (2026-10-03, second batch)
- D
- os
- build_final_assembly.py
- SolveResult
- make_lcd_sheet.py
- Num
- PR_hardware-stage11-roadmap.md
- Tester firmware README (Stage 4)
- gen.py
- ai_calc_pcb_8/hardware/tools/build_board.py
- Overlay
- SerialCmd
- load_info
- Viewfinder
- algorithm
- Fit check: our PCB in the fx-115ES replica
- fit_2d
- full.sh script
- Stage 12 progress notes (working log)
- firmware-v15-lcd/src/main.cpp
- Stage 14: fixing the 4-part verification findings
- U1 ESP32-S3-MINI-1-N4R2 (MCU module)
- calc_engine.cpp
- 03 — External interfaces, end to end (stage 13b board)
- build_jigs.py
- fx-115ES shell replica (unbranded)
- Board geometry from the scan: assumptions and confidence
- Photo notes for the fx-115ES replica
- clearance_table.md
- Replica progress (for resuming after a cut-off)
- stage11_progress.md
- Fit check: our PCB in the fx-115ES replica
- Stage 13: the three height problems (J4 battery socket, J3 magnet, camera)
- Final pre-order review (2026-10-04, stage 13b board)
- ai_calc_pcb_8/hardware/tools/prepass.py
- guide_renders.py
- firmware-prototype/src/claude_client.cpp
- stage13_progress.md
- Calculator Mode
- 04 — Manufacturing verification (JLCPCB PCB + PCBA), stage 13b
- Final firmware review (board v14, ESP32-S3-MINI-1-N4R2)
- lgfx_host_platform.cpp
- SerialCmd
- firmware/src/screen.cpp
- Battery upgrade study: a bigger LiPo without changing the board (2026-10-06)
- build
- Docs consistency audit (2026-10-06, 01:20–01:40 EDT, stopped early to save usage)
- Menu System
- test_proxy.py
- pinmap_crossref.py
- Final assembly progress (resume here)
- Firmware stage-13 work: progress notes
- Event
- graphify skill trigger (/graphify)
- firmware-prototype/src/keys.cpp
- AI Calculator PCB - KiCad design summary
- make_bringup.py
- firmware-prototype/src/screen.cpp
- v15-LCD final assembly: does everything fit? (final board + ER-TFT019-1)
- sim/web/build.sh
- Browser simulator page (page.html)
- ui.cpp
- Part B: end-of-line test, every finished unit
- arduino
- Decode
- AI Calculator PCB: handoff for the next Claude session (from stage 14)
- Stage 2: electrical plan (pins, power, keypad)
- JLCPCB order checklist (stage 14, silk "AI CALC v14 2026-10-04")
- Stage 3: schematic (done)
- Stages 4 + 5: footprints and placement
- full.sh script
- Verification 02: power, boot, recovery, protection, analog (stage 13b)
- Final assembly check: does everything fit?
- CameraPins
- 05: Independent re-check of stage 14 (v14) before the one-shot order
- AI Calculator PCB - KiCad design summary
- Settings
- Panel
- CameraPins
- hardware/tools/make_outputs.sh
- hardware/tools/prepass.py
- OtaInfo
- bootMessage
- AI Calculator board: final status (start here)
- Slide case mods (LATER: not needed for testing)
- Stage 14 progress (verification fixes)
- calc_engine.h
- clearance_table_full.md
- glyphdraw.h
- Grinding jigs (3D-printed) for the donor fx-115ES shell
- SolveCallbacks
- assembly_howto_pictures.py
- Who should build the board? JLCPCB vs the alternatives
- spline
- firmware-prototype/src/settings.cpp
- Stage 15: v15-LCD, the 2.13" e-paper replaced by a 1.9" colour IPS LCD
- firmware-prototype/src/preview.cpp
- archive/README.md
- 13 — Independent review of the v15-LCD board, re-derived from the raw files
- lcd.cpp
- Knockoff shell plan: measuring a 991ES-style clone and refitting the board (v15)
- rrect_pts
- build_cost_model.py
- firmware-v15-lcd/src/keys.cpp
- 06 — Final adversarial review of PCB v14 (before ordering)
- firmware.py
- Goldenmorning T190X7-C30-01H: does it fit the v15-LCD build?
- Unit economics: what each calculator costs and earns
- Keypad 8x10 matrix (ROW0-7, COL0-9, SW1-SW50)
- Arrival checklist: from boxes to a working calculator
- 11 — Requirements trace: is everything we decided actually on the v14 board?
- LCD panel options for the v15-LCD board
- window_mask_template_v15.py
- check_hole_clearance.py
- firmware-v15-lcd/src/claude_client.cpp
- f
- Router
- viewfinder.cpp
- firmware-v15-lcd/src/camera.cpp
- make_outputs_v15.sh
- Ask
- sys
- Firmware for the stage-13 board
- SolveCallbacks
- Findings
- firmware-v15-lcd/src/preview.cpp
- SolveCallbacks
- make_fitcheck.py
- onWifiEvent
- stage_parts
- SerialCmd
- selftest_tu.cpp
- t218/clearance_table_full.md
- Event
- 14 — Final pre-order verification of the v15-LCD board (ER-TFT019-1 panel)
- Failure
- final_assembly_v15_lcd/clearance_table_full.md
- SetupInfo
- Kind
- v15 colour LCD: real UI screenshots
- testClaudeApi
- FrameInfo
- CamFocus
- Button
- JLCPCB order walkthrough: v15-LCD board (silk "AI CALC v15-LCD 2026-10-08")
- restoreState
- 15. v15-LCD key pads vs the original Casio fx-115ES keyboard PCB
- build_final_assembly_v15.py
- Board firmware for v15-LCD (colour screen + live camera preview)
- SetupInfo
- Decode
- cstdint
- Wake
- ScanNote
- SendStep
- harness.cpp
- IdlePolicy
- 2026-10-10
- Wake
- t203/clearance_table_full.md
- OtaInfo
- string
- firmware/src/settings.cpp
- firmware/src/main.cpp
- Tuning
- Com
- Line
- cameraCapture
- firmware-v15-lcd/src/setup_portal.cpp
- previewStart
- HistoryItem
- annotate_guide.py
- key_pills.md
- onWifiEvent
- app.h
- check_hole_clearance_v15.py
- Tuning
- cameraPreviewRelease
- Tuning
- SendStep
- firmware-v15-lcd/src/power.cpp

## God Nodes (most connected - your core abstractions)
1. `Device` - 122 edges
2. `Tok` - 91 edges
3. `DKey` - 80 edges
4. `App` - 70 edges
5. `Num` - 48 edges
6. `Viewfinder` - 45 edges
7. `Parser` - 42 edges
8. `loop()` - 37 edges
9. `loop()` - 35 edges
10. `Framebuffer` - 33 edges

## Surprising Connections (you probably didn't know these)
- `Firmware changes requested` --references--> `StreamReader`  [INFERRED]
  server/proxy/FINAL_REVIEW_server.md → core/claude_api.h
- `4. Keypad (50 contacts = 49 matrix + ON)` --references--> `DKey`  [INFERRED]
  hardware/geometry_assumptions.md → core/device.h
- `Tester hardware: ESP32-S3-CAM (OV3660) + Waveshare 2.13in e-paper HAT V4` --semantically_similar_to--> `U1 ESP32-S3-MINI-1-N4R2 (MCU module)`  [INFERRED] [semantically similar]
  firmware/README.md → Claude outputs/ai_calc_pcb_8/hardware/kicad/DESIGN_SUMMARY.md
- `Files` --references--> `App`  [INFERRED]
  hardware/renders_ui_v15/README.md → core/app.h
- `v15 colour LCD: real UI screenshots` --references--> `App`  [INFERRED]
  hardware/renders_ui_v15/README.md → core/app.h

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

## Communities (276 total, 46 thin omitted)

### Community 0 - "DIR"
Cohesion: 0.22
Nodes (10): closedir(), DIR, e, fd, first, h, dirent, d_name (+2 more)

### Community 1 - "device.cpp"
Cohesion: 0.10
Nodes (40): evaluate(), clockText(), continuesAns(), clearEntry, drawMenu, drawStatus, endExam, engText (+32 more)

### Community 2 - "mm"
Cohesion: 0.09
Nodes (20): net_pts(), pad_pts(), Router, arc(), rect(), seg(), utext(), net() (+12 more)

### Community 3 - "Tok"
Cohesion: 0.03
Nodes (71): Tok, Abs, Acos, Acosh, Add, Ans, Asin, Asinh (+63 more)

### Community 4 - "build_case.py"
Cohesion: 0.06
Nodes (65): appearance(), _blocks(), body_named(), chamfer(), check_outline(), circle(), cm(), comp_named() (+57 more)

### Community 5 - "win32_main.cpp"
Cohesion: 0.10
Nodes (17): advanceRequest(), exifOrientation(), fakeCameraFrame(), handleEvent(), loadSettings(), onAiReady(), paint(), photoToJpeg() (+9 more)

### Community 6 - "Device"
Cohesion: 0.04
Nodes (39): errorText(), Device, alpha_, angle_, app_, back_, battery_, blink_ (+31 more)

### Community 7 - "geometry"
Cohesion: 0.08
Nodes (21): area(), case_plan(), check(), clip_above(), clip_below(), front_fun(), geometry(), key_open() (+13 more)

### Community 8 - "Stage 11: e-paper moved to the measured window, calipers applied, review fixes (ai_calc_pcb_11)"
Cohesion: 0.22
Nodes (8): Assumptions made in this stage, Caliper readings, how each was used, Firmware to-do (from the review), Questions for Nirav, Stage 11: e-paper moved to the measured window, calipers applied, review fixes (ai_calc_pcb_11), The e-paper ribbon (FPC) length budget, Things I checked and left alone (robustness review), What changed, and why

### Community 9 - "DKey"
Cohesion: 0.04
Nodes (52): DKey, AC, Add, Alpha, Ans, Calc, Close, Cos (+44 more)

### Community 10 - "Json"
Cohesion: 0.07
Nodes (17): Json, a_, b_, n_, o_, parse, s_, JsonParser (+9 more)

### Community 11 - "Ann"
Cohesion: 0.16
Nodes (8): Ann, back_cover(), front_shell(), j2_wall(), load(), paper_riding(), paper_tab(), solar_wall()

### Community 12 - "JLCPCB order walkthrough (stage 14 board, silk "AI CALC v14 2026-10-04")"
Cohesion: 0.12
Nodes (16): 0. Before you start, 1. Upload the gerbers, 2. PCB options, 3. PCB Assembly, 4. BOM and CPL, 5. Placement preview: what each part must look like, 6. Order notes (paste into the "remark" box), 7. If JLCPCB's engineers email you (+8 more)

### Community 13 - "Fit check: paper grind maps, dummy board, clearance table"
Cohesion: 0.33
Nodes (5): 1. Paper dry fit and grind maps (1:1), 1b. Clearance table, 2. Dummy board (3D print), Don't grind, Fit check: paper grind maps, dummy board, clearance table

### Community 14 - "web_main.cpp"
Cohesion: 0.10
Nodes (33): solveInstructions(), solveSchema(), sim_active_request(), sim_alloc(), sim_api_url(), sim_api_version(), sim_build_request(), sim_classify() (+25 more)

### Community 15 - "firmware/src/camera.cpp"
Cohesion: 0.12
Nodes (25): applyAll(), autoTuneLocked(), cameraAutoTune(), cameraBegin(), cameraCapture(), cameraFocusScore(), cameraFrame(), cameraHoldEstimateMs() (+17 more)

### Community 16 - "battery_envelope.py"
Cohesion: 0.14
Nodes (13): bbox(), board_outline(), envelopes(), fit(), height_map(), inside(), max_rect(), near() (+5 more)

### Community 17 - "Full tables"
Cohesion: 0.11
Nodes (17): CHECK, Discretes, FATAL, Findings by severity, Full tables, J1 camera (OV5640 MJY5OAF-F3M-V1), CamReversed, rot 180, pad row y = 162.58, J2 e-paper (Waveshare 2.13" V4, SSD1680), plain numbering, rot −90, pad column x = 179.48, J3 / J4 (+9 more)

### Community 18 - "firmware-prototype/src/main.cpp"
Cohesion: 0.12
Nodes (44): cameraName(), accountCommand(), applySettings(), chargeText(), drawSetupScreen(), effortName(), failureText(), handleEvent() (+36 more)

### Community 19 - "AI Answer Flow"
Cohesion: 0.07
Nodes (34): AI Answer Screen, AI Answer Flow, AI Error States, AI Hold-to-Capture Flow, AI Hold Captured Screen, AI Hold Countdown Screen, AI Hold Done Screen, AI Ready Screen (+26 more)

### Community 20 - "Stage 4: full calculator"
Cohesion: 0.09
Nodes (25): core static library (core/*.cpp, C++17), Win32 simulator executable (sim/win32_main.cpp), tests executable (tests/tests.cpp), AI Calculator project, AI SOLVE mode (MODE 4), Browser simulator (Emscripten build of core), Casio fx-300ES PLUS (COMP mode emulation), Claude Sonnet 5.5 model (+17 more)

### Community 21 - "make_screens.py"
Cohesion: 0.10
Nodes (15): corners(), green_mask(), _hull(), label(), persp_coeffs(), screen_on(), side_by_side(), contact_sheet() (+7 more)

### Community 22 - "AI Calculator PCB Handoff"
Cohesion: 0.06
Nodes (64): Board Geometry Assumptions, Battery Bay (LR44 corner), Firmware Row-2 Key Mismatch (Abs, x^3, x^-1, log_a b), Keypad Edge = Casio Edge - 0.25 mm, keys.csv / features.json, KiCad Top View = Back-Cover View; Keys on B.Cu, Magnetic Connector Top-Wall Slot, Scale 7.55 px/mm (+56 more)

### Community 23 - "check_requirements_11.py"
Cohesion: 0.08
Nodes (32): board_summary(), kids(), main(), natkey(), parse(), props(), sheet_summary(), board_summary() (+24 more)

### Community 24 - "AI Calculator launch roadmap"
Cohesion: 0.09
Nodes (22): 0. The short version, 1. What to buy now (`AI_Calculator_Shopping_List.xlsx`), 1A. The JLCPCB order (done 10/8), 1B. Tester parts: options A / B / C (5 testers, 1 spare of each kind), 1C. v15-LCD order and panels (verified "ORDER", 2026-10-08), 1D. Don't buy yet, 2. Timeline, 2A. From the order to v15 (estimate) (+14 more)

### Community 25 - "tests.cpp"
Cohesion: 0.17
Nodes (18): exprText(), tokText(), calcText(), fakeFrame(), keys(), main(), page(), readFile() (+10 more)

### Community 26 - "build_fx115es_replica.py"
Cohesion: 0.11
Nodes (14): appearance(), board_info(), find_design(), send(), set_visible(), _sexp_blocks(), side_band(), stage_explode() (+6 more)

### Community 27 - "stage_front"
Cohesion: 0.20
Nodes (24): body_named(), build_slide_case(), circle(), cm(), combine(), comp_named(), cut_groove(), edge_loop_at() (+16 more)

### Community 28 - "firmware-prototype/src/setup_portal.cpp"
Cohesion: 0.13
Nodes (16): escape(), field(), formPage(), hexVal(), route(), savedPage(), scanNetworks(), setupSavedAgoMs() (+8 more)

### Community 29 - "db.py"
Cohesion: 0.16
Nodes (20): main(), link_submit(), account_by_email(), claim_pair_code(), conn(), count_solve(), create_device(), current_pair_code() (+12 more)

### Community 30 - "App"
Cohesion: 0.07
Nodes (43): App, accepts, activeId_, busySinceMs_, captured_, computed_, effortParam, expression_ (+35 more)

### Community 31 - "12: Inner rib / pins / hooks re-check against the v14 board (2026-10-06, before ordering)"
Cohesion: 0.10
Nodes (19): 07: Final fitment review, v14 board in the ground fx-115ES shell (2026-10-06), 1. Findings, 2.1 ESP32 module (U1) vs its neighbours, 2.2 Every part vs every shell feature, 2.3 Before the grinds (what each grind fixes, rev F), 2. Fusion rev F: the full check (closed case, grinds applied), 3. Pre-grind checklist (10 minutes, real shell + calipers with the depth rod, before the first cut), 4. Every check made (with the passes) (+11 more)

### Community 32 - "Focus"
Cohesion: 0.12
Nodes (23): analyseFocus(), Crop, h, use, w, x, y, detailCrop() (+15 more)

### Community 33 - "Stage 12: second caliper batch, new photos, CAD fit check (ai_calc_pcb_12)"
Cohesion: 0.20
Nodes (9): 1. What changed on the board, and why, 2. The screen-section width (was blocking), 3. Heights: what's behind the board (F side faces the back cover), 4. E-paper (P3: 59.0 × 29.2), 5. C15 and residuals (hole positions), 6. CAD fit-check collisions (fitcheck_report.md, STEP of 18:10) and what was done, 7. Parts and cost (checked live on JLCPCB's parts API, 2026-10-03), 8. Robustness checks (+1 more)

### Community 34 - "Vars"
Cohesion: 0.17
Nodes (12): Vars, a, ans, b, c, d, e, f (+4 more)

### Community 35 - "Router"
Cohesion: 0.17
Nodes (3): poly_pts(), polyline(), Router

### Community 36 - "Framebuffer"
Cohesion: 0.12
Nodes (21): render, Framebuffer, clear, drawText, drawTextPx, fillRect, get, icons_ (+13 more)

### Community 37 - "firmware/src/preview.cpp"
Cohesion: 0.25
Nodes (20): authorised(), autotuneHandler(), detailHandler(), focusHandler(), frameHandler(), getUri(), jsonText(), lastHandler() (+12 more)

### Community 38 - "Json"
Cohesion: 0.07
Nodes (15): crect(), rect(), full.sh build pipeline, add_via(), free(), obstacles(), via_near_pad(), crect() (+7 more)

### Community 39 - "Independent electrical + manufacturability review (2026-10-03)"
Cohesion: 0.12
Nodes (15): B1. `fab/` outputs and the DRC report are stale relative to the board file, BLOCKER, Checked and OK (no action), Independent electrical + manufacturability review (2026-10-03), LCSC check (2026-10-03, lcsc.com product pages; all exist and are active), NICE (optional), Plain-English summary, S1. R20 (CHG_STAT pull-up to +3V3) back-feeds the charger and leaks about 20 µA in standby (+7 more)

### Community 40 - "hardware/geometry/derive_geometry.py"
Cohesion: 0.27
Nodes (8): board_polygon(), front_silhouette(), keys(), main(), mm(), overlays(), poly_mm(), to_back()

### Community 41 - "Back cover (engraved 'AI CALCULATOR rev A, 6 x M2x10')"
Cohesion: 0.13
Nodes (22): Front view render (enclosure), Display window with 'AI CALCULATOR' wordmark, Round D-pad (4-way cursor key), Casio-style keypad layout (SHIFT/ALPHA/D-pad/MODE/ON, Abs x^3 x^-1 log_a b row, 5-col number block), Isometric back render (back cover), Back cover (engraved 'AI CALCULATOR rev A, 6 x M2x10'), 6x M2x10 countersunk screw fastening, Back cover camera window (OV5640) (+14 more)

### Community 42 - ".function"
Cohesion: 0.25
Nodes (7): exactPlus(), exactTimes(), gcdLL(), makeExact(), mulOk(), ofDouble, ofInt

### Community 43 - "AI Calculator Board Concept Layout v2"
Cohesion: 0.16
Nodes (20): AI Calculator Board Concept Layout v2, Charger + 3.3 V regulator + ESD block, E-paper socket + driver and window, ESP32-S3 MINI-1 module, Keypad layout (fx-300ES style key grid), LiPo 401230 battery (~110 mAh), Magnetic connector (5358) in solar-cell slot, OV5640 camera + 24-pin socket (+12 more)

### Community 44 - "F"
Cohesion: 0.11
Nodes (21): appearance(), clear_comp(), comp_named(), F(), mask_geom(), move_body(), new_sketch(), offline_numbers() (+13 more)

### Community 45 - "hardware/tools/build_board.py"
Cohesion: 0.23
Nodes (7): add_edge_keepout(), add_epaper_slot(), add_rule_areas(), fp_path(), main(), patch_rules(), rule_area()

### Community 46 - "pilroute.py"
Cohesion: 0.21
Nodes (6): connect_all(), island_at(), islands(), item_pts(), net_pts(), pad_pts()

### Community 47 - "08 — Second-opinion PCB check (v14), re-derived from the raw files"
Cohesion: 0.14
Nodes (13): 08 — Second-opinion PCB check (v14), re-derived from the raw files, 1. CPL rotation vs EasyEDA footprint — PASS (77/77 parts), 2.1 J1 — OV5640 24-pin FPC (`_CamReversed`, rot 180, pads at y 162.58, pad 1 at x 144.25, pad 24 at x 155.75), 2.2 J2 — 2.13" e-paper 24-pin FPC (rot −90, pad 1 at (179.48, 87.08), pad 24 at (179.48, 98.58), contact k = pad k), 2.3 J3 — magnet USB piece (KiCad pad 1 at x 123.69 … pad 4 at x 131.31, y 72.00), 2.4 J4 — JST-PH 2-pin (pad 1 at x 141.60 = GND, pad 2 at x 143.60 = BAT+; EE pin 1 at the same place), 2. Connector pad-by-pad netlists, 3. ESP32-S3-MINI-1-N4R2 pin table — PASS (+5 more)

### Community 48 - "app.py"
Cohesion: 0.14
Nodes (13): admin_new_device(), billing_webhook(), device_from(), device_status(), err(), firmware_image(), firmware_manifest(), healthz() (+5 more)

### Community 49 - "font.h"
Cohesion: 0.16
Nodes (15): drawCalc, decodeUtf8(), encodeUtf8(), findGlyph(), Glyph, cp, rows, hasGlyph() (+7 more)

### Community 50 - "Root sheet ai_calc.kicad_sch"
Cohesion: 0.14
Nodes (18): Stage-8 KiCad Design Summary (routed board), U4/U5 ME6211 camera LDOs (2.8 V / 1.5 V), E-paper boost circuit (L1 68uH, Q3 SI1308, D3-D5, R12 3R), J1 Camera FPC 24P dual-contact (OV5640, reversed), J2 E-paper FPC 24P dual-contact (2.13in V4), kicad_summary.py (summary generator), Net +3V3, Net SYS (load-shared system rail) (+10 more)

### Community 51 - "solve.py"
Cohesion: 0.12
Nodes (13): solve_endpoint(), counted(), relay(), BadRequest, clean_request(), client(), error_body(), haiku_params() (+5 more)

### Community 52 - "firmware-prototype/src/camera.cpp"
Cohesion: 0.10
Nodes (23): applyAll(), autoTuneLocked(), cameraAutoTune(), cameraBegin(), cameraFrame(), cameraHoldUntil(), cameraLastDetail(), cameraLastPhoto() (+15 more)

### Community 53 - "selfTestRun"
Cohesion: 0.18
Nodes (12): keysScannerOk(), bootMessage(), serviceOtaConfirm(), otaConfirm(), otaPendingVerify(), otaSlotName(), resetReasonText(), Json (+4 more)

### Community 54 - "AI Calculator Multiple Choice Test"
Cohesion: 0.17
Nodes (13): AI Calculator Multiple Choice Test, Multiple Choice Answer Key and Score Sheet, CHOICE + option label output format, Closest-option selection when exact answer absent, MC Handwritten Round (H1-H4), Option label styles (A-D row, A-D stacked, (a)-(e), 1-4), AI Calculator Test Worksheet, Worksheet Answer Key and Score Sheet (+5 more)

### Community 55 - "hardware/tools/fixroute.py"
Cohesion: 0.20
Nodes (4): anchor_points(), items_geom(), raster(), route()

### Community 56 - "stage_fitcheck"
Cohesion: 0.16
Nodes (7): F(), fit_compare(), fit_proxies(), fit_vs_clearance_table(), _parse_report(), stage_fitcheck(), write_fit_report()

### Community 57 - "View"
Cohesion: 0.09
Nodes (21): View, Ai, Calc, Clr, ClrConfirm, Error, ExamInfo, Hyp (+13 more)

### Community 58 - "Competitor analysis: AI calculators (checked 2026-10-03)"
Cohesion: 0.12
Nodes (15): 0. The short version, 1.1 Product facts, 1.2 What customers say (their own review page, 63 reviews, 4.8★) [S4], 1.3 Areas where VovoCorp beats us (ranked by how much it matters), 1.4 Where VovoCorp is weak (our openings), 1. VovoCorp in detail, 2. Other competitors, 3(a) PCB / hardware: what's possible before Monday's order (+7 more)

### Community 59 - "firmware-prototype/src/power.cpp"
Cohesion: 0.13
Nodes (20): serviceBattery(), batteryMillivolts(), batteryOkForAi(), batteryOkForOta(), batteryPercent(), chargeState(), floatPin(), holdLevel() (+12 more)

### Community 60 - "claudeSolve"
Cohesion: 0.38
Nodes (4): claudeSolve(), lower(), readLine(), writeAll()

### Community 61 - "Stage 10: moving the board to the Casio fx-115ES shell"
Cohesion: 0.22
Nodes (8): Board changes made in stage 10 (DRC: 0 errors, 0 unconnected, 0 schematic mismatches), Caliper readings used so far (2026-10-03), How the photos were measured, Stage 10: moving the board to the Casio fx-115ES shell, Stage 11a: e-paper moved to the measured window (calipers C12/C13), Still from the old shell (not measurable in the photos), The back cover (photo `2f54d6c7`, fitted on its 6 screw holes, about ±1 mm), Things in the way, seen in the photos

### Community 62 - "Breadboard Wiring Diagram (exact, camera up)"
Cohesion: 0.23
Nodes (14): Calculator Buttons (=, AC, Up, Down), 2.13in E-Paper Display, ESP32-S3-CAM Board, LONG Breadboard, MEDIUM Breadboard, Breadboard Wiring Diagram (exact, camera up), Breadboard Wiring Diagram (real orientation, USB away), Breadboard Wiring Diagram (top, USB toward you) (+6 more)

### Community 63 - "ai_calc_pcb_8/hardware/geometry/derive_geometry.py"
Cohesion: 0.09
Nodes (14): board_polygon(), front_silhouette(), keys(), main(), mm(), overlays(), poly_mm(), to_back() (+6 more)

### Community 64 - "Stage-8 board ai_calc.kicad_pcb (72.3x148.8 mm, 0.8 mm, 2 layers, 140 footprints, 4630 tracks, 322 vias, 92 nets, GND pour)"
Cohesion: 0.19
Nodes (11): Custom footprint library ai_calc.pretty (stage 8), Stage-8 board ai_calc.kicad_pcb (72.3x148.8 mm, 0.8 mm, 2 layers, 140 footprints, 4630 tracks, 322 vias, 92 nets, GND pour), AI Calculator custom case (rev A) README, build_case.py (parametric Fusion case from KiCad), fusion_run.py (sends script to Fusion), FusionMCPBridge add-in, TPU keymat (46 caps + REPLAY rocker), Case Z stack-up (77.7 x 156.2 x 13.6 mm) (+3 more)

### Community 65 - "policy.py"
Cohesion: 0.23
Nodes (8): subscription_state(), used(), decide(), Decision, _deny(), link_hint(), short_url(), status_json()

### Community 66 - "stage_cell"
Cohesion: 0.17
Nodes (7): adsk_refresh(), poly_pts(), stage_cell(), tbm_add(), tube(), P(), rect()

### Community 67 - "host_hw.cpp"
Cohesion: 0.10
Nodes (6): autoSave(), fill(), flush(), pushed(), pushRaw(), save()

### Community 68 - "Prototype firmware README (board inside the Casio)"
Cohesion: 0.24
Nodes (12): D7 SMF5.0A VBUS TVS, J3 Magnetic USB (Adafruit 5358), J4 Battery JST-PH (Adafruit 1570), Net BAT+, Net VBUS, Sheet: Connectors - magnet USB, battery (connectors.kicad_sch), U2 MCP73831 LiPo charger, U7 USBLC6-2SC6 USB ESD protection (+4 more)

### Community 69 - "Stage 9: finishing the v8 board (ai_calc_pcb_9)"
Cohesion: 0.40
Nodes (4): Firmware (done in this stage), Stage 9: finishing the v8 board (ai_calc_pcb_9), Still open before ordering, What changed, and why

### Community 70 - "Nirav's caliper readings, fx-115ES shell (2026-10-03, second batch)"
Cohesion: 0.29
Nodes (6): Case, with calipers, Decisions, Depths and heights, Nirav's caliper readings, fx-115ES shell (2026-10-03, second batch), Parts already in hand, Photos

### Community 71 - "D"
Cohesion: 0.09
Nodes (21): D, depth_rod(), esc(), good_bad(), rim_method(), tool_angle(), v15_bow(), v15_sfold() (+13 more)

### Community 72 - "os"
Cohesion: 0.06
Nodes (21): export_netlist(), find_kicad_cli(), header_pins(), main(), u1_nets(), export_netlist(), find_kicad_cli(), header_pins() (+13 more)

### Community 73 - "build_final_assembly.py"
Cohesion: 0.05
Nodes (63): adsk_refresh(), all_bodies(), appearance(), band(), bodies_under(), _case_vs_all(), _check(), clear_comp() (+55 more)

### Community 74 - "SolveResult"
Cohesion: 0.12
Nodes (12): parseSolveResult(), SolveResult, answer, check, choice, confidence, expression, readable (+4 more)

### Community 78 - "make_lcd_sheet.py"
Cohesion: 0.17
Nodes (11): block(), instance(), label(), lib_symbol_custom(), main(), place(), no_connect(), pins_of() (+3 more)

### Community 79 - "Num"
Cohesion: 0.18
Nodes (11): asInt(), isInt(), Num, den, exact, num, v, Parser (+3 more)

### Community 80 - "PR_hardware-stage11-roadmap.md"
Cohesion: 0.50
Nodes (3): Reviewer notes, What changed, Why

### Community 81 - "Tester firmware README (Stage 4)"
Cohesion: 0.13
Nodes (19): Tester firmware README (Stage 4), src/claude_client (HTTPS streaming to Claude), core/ shared calculator logic, Exam mode (Wi-Fi off, persisted, USB unlock or 12 h), src/screen (125x61 calc screen on 250x122 e-paper), Tester hardware: ESP32-S3-CAM (OV3660) + Waveshare 2.13in e-paper HAT V4, buildKeys(), claudeScan() (scan via host sampleFn) (+11 more)

### Community 82 - "gen.py"
Cohesion: 0.11
Nodes (29): all_meas(), badge(), bezel(), board_map(), board_map_bottom(), callout(), edge_point(), F() (+21 more)

### Community 85 - "ai_calc_pcb_8/hardware/tools/build_board.py"
Cohesion: 0.26
Nodes (7): add_edge_keepout(), add_epaper_slot(), add_rule_areas(), fp_path(), main(), patch_rules(), rule_area()

### Community 86 - "Overlay"
Cohesion: 0.10
Nodes (19): bind(), bracket(), FocusState, Fixed, Focused, Focusing, None, greyThumb() (+11 more)

### Community 87 - "SerialCmd"
Cohesion: 0.13
Nodes (13): SerialCmd, Changed, ExamOff, Keys, None, Preview, PreviewOff, Scan (+5 more)

### Community 88 - "load_info"
Cohesion: 0.19
Nodes (16): explode(), grind_renders(), hide_cutters(), load_info(), place(), section(), set_grid(), set_grind() (+8 more)

### Community 89 - "Viewfinder"
Cohesion: 0.08
Nodes (22): Viewfinder, flipped_, frameH_, frameW_, fullEvery, ghostBudget, haveShown_, image_ (+14 more)

### Community 90 - "algorithm"
Cohesion: 0.11
Nodes (13): Charge, Charging, Full, NoCable, chargeState(), Point, mv, pct (+5 more)

### Community 91 - "Fit check: our PCB in the fx-115ES replica"
Cohesion: 0.25
Nodes (7): Clearances at board height (2D, exact), Collisions (3D), Display, Fit check: our PCB in the fx-115ES replica, Parts not in the STEP, Placement, Stack-up used

### Community 92 - "fit_2d"
Cohesion: 0.23
Nodes (10): circle_pts(), densify(), fit_2d(), clearance(), in_board(), fit_label(), d(), inside() (+2 more)

### Community 93 - "full.sh script"
Cohesion: 0.38
Nodes (4): drc(), full.sh script, route.sh script, run.sh script

### Community 95 - "firmware-v15-lcd/src/main.cpp"
Cohesion: 0.12
Nodes (46): cameraIsOn(), cameraName(), accountCommand(), applySettings(), chargeText(), drawSetupScreen(), effortName(), failureText() (+38 more)

### Community 96 - "Stage 14: fixing the 4-part verification findings"
Cohesion: 0.18
Nodes (10): 1. J3 magnet pin order (verification 03 F1, FATAL), 2. CPL rotations (verification 04 M1/M2, FATAL), 3. SW1 key pad vs hole H3 (verification 04 M3), 4. Recovery procedure (verification 02 L1) and the R17 comment, 5. fx-115ES key names (verification 03 L2), 6. Ribbon lengths (verification 03 L1 / L3), 7. Order documents, 8. Regenerated outputs (+2 more)

### Community 97 - "U1 ESP32-S3-MINI-1-N4R2 (MCU module)"
Cohesion: 0.60
Nodes (6): Camera DVP bus (CAM_D0-7, PCLK, HREF, VSYNC, XCLK, SIOC/SIOD), E-paper SPI (EPD_CLK/DIN/CS/DC/RST/BUSY), I2C bus (I2C_SCL/I2C_SDA, KEYPAD_INT), Net CHG_STAT (via D6 level clamp from CHG_STAT_RAW), U1 ESP32-S3-MINI-1-N4R2 (MCU module), firmware-prototype src/pins.h

### Community 98 - "calc_engine.cpp"
Cohesion: 0.11
Nodes (24): addOk(), Alias, text, tok, endsOperand(), Fail, e, pos (+16 more)

### Community 99 - "03 — External interfaces, end to end (stage 13b board)"
Cohesion: 0.17
Nodes (11): 03 — External interfaces, end to end (stage 13b board), CHECK, FATAL, Findings (most severe first), LIKELY PROBLEM, OK, Table A — camera trace, Table B — e-paper trace (+3 more)

### Community 100 - "build_jigs.py"
Cohesion: 0.18
Nodes (9): arc(), band(), build(), check(), clip_half(), export(), main(), renders() (+1 more)

### Community 101 - "fx-115ES shell replica (unbranded)"
Cohesion: 0.22
Nodes (8): Every dimension and where it came from, Files, Fit check: our PCB in the shell, fx-115ES shell replica (unbranded), Known simplifications, Rebuild, Remaining guesses (the things that would make it exact), What is in the model

### Community 102 - "Board geometry from the scan: assumptions and confidence"
Cohesion: 0.20
Nodes (9): 1. What the scan shows (orientation), 2. Scale, 3. Geometry table, 4. Keypad (50 contacts = 49 matrix + ON), 5. Choices made where two readings were possible, 6. Carry-forward findings, 7. VERIFY list (lowest confidence first), Board geometry from the scan: assumptions and confidence (+1 more)

### Community 103 - "Photo notes for the fx-115ES replica"
Cohesion: 0.29
Nodes (6): Batch 2 (2026-10-03, uploads/0abf7810-...), Conclusions used in the model, Photo notes for the fx-115ES replica, Rev B conclusions, Rev C / C2 re-read (2026-10-05): split line, silver vs navy, side walls, grooves, case, Rev D re-read (2026-10-05 evening): faceplate details

### Community 105 - "Replica progress (for resuming after a cut-off)"
Cohesion: 0.40
Nodes (4): Replica progress (for resuming after a cut-off), Rev B2 (23:25): Nirav's answers D4 = 6.0 to the INSIDE of the floor, D1 = 5.5 from the front shell RIM, stubs stop above the board, Rev B (2026-10-03, caliper batch 2: hardware/measurements_2026-10-03.md), Rev C2 (2026-10-05): 2-part split from the photos + slide case

### Community 107 - "Fit check: our PCB in the fx-115ES replica"
Cohesion: 0.20
Nodes (9): Before / after: `fitcheck_report_stage11.md` vs this run, Clearances at board height (2D, exact), Collisions (3D), Compared with `hardware/fitcheck/clearance_table.md` (PCB agent), Display, Fit check: our PCB in the fx-115ES replica, Parts not in the STEP, Placement (+1 more)

### Community 108 - "Stage 13: the three height problems (J4 battery socket, J3 magnet, camera)"
Cohesion: 0.20
Nodes (9): 1. The new numbers, 2. Results per part, 3. The magnet connector (J3): option A, done in stage 13b, 4. Camera: window and mounting, 5. Other fixes in this stage, 6. Grind list (print `fitcheck/grind_map_back_cover.svg` and `grind_map_front_shell.svg` at 100 %), 7. Off-board parts for the Fusion assembly (KiCad coordinates), 8. Files (+1 more)

### Community 109 - "Final pre-order review (2026-10-04, stage 13b board)"
Cohesion: 0.20
Nodes (9): Check during the order (preview / engineer questions), Final pre-order review (2026-10-04, stage 13b board), Findings, by severity, JLCPCB ordering walkthrough, Low (nice to fix later, no action for this order), Previous review (2026-10-03): status of every item, Verdict: **ready to order: YES**, with three checks in JLCPCB's parts preview, What I changed (+1 more)

### Community 110 - "ai_calc_pcb_8/hardware/tools/prepass.py"
Cohesion: 0.25
Nodes (3): connect_tree(), pad(), stub()

### Community 111 - "guide_renders.py"
Cohesion: 0.07
Nodes (25): drop_candidate(), log(), main(), measure(), old_bodies(), overlap(), piece(), proxies() (+17 more)

### Community 112 - "firmware-prototype/src/claude_client.cpp"
Cohesion: 0.24
Nodes (14): lower(), open(), parseUrl(), proxyDownload(), proxyRequest(), readBody(), readHead(), readLine() (+6 more)

### Community 114 - "Calculator Mode"
Cohesion: 0.40
Nodes (5): Calculator Editing Screen, Calculator Math Error Screen, Calculator Fraction Result Screen, Calculator STO Screen, Calculator Mode

### Community 115 - "04 — Manufacturing verification (JLCPCB PCB + PCBA), stage 13b"
Cohesion: 0.18
Nodes (10): 04 — Manufacturing verification (JLCPCB PCB + PCBA), stage 13b, CHECK, FATAL (if not corrected), Findings, by severity, Instructions, LIKELY PROBLEM, OK (verified), Renders (`hardware/verification/renders/`) (+2 more)

### Community 116 - "Final firmware review (board v14, ESP32-S3-MINI-1-N4R2)"
Cohesion: 0.11
Nodes (17): Board changes requested (for the PCB agent), Build and test results, Every check and result, FATAL, Final firmware review (board v14, ESP32-S3-MINI-1-N4R2), Findings, First power-on script (day one), Fixes applied 2026-10-06 (+9 more)

### Community 117 - "lgfx_host_platform.cpp"
Cohesion: 0.19
Nodes (12): beginTransaction(), endTransaction(), init(), readBytes(), readRegister8(), release(), restart(), transactionRead() (+4 more)

### Community 118 - "SerialCmd"
Cohesion: 0.13
Nodes (15): SerialCmd, Account, Changed, ExamOff, Keys, None, Pair, Preview (+7 more)

### Community 119 - "firmware/src/screen.cpp"
Cohesion: 0.29
Nodes (3): draw(), screenBegin(), screenShow()

### Community 120 - "Battery upgrade study: a bigger LiPo without changing the board (2026-10-06)"
Cohesion: 0.18
Nodes (10): 1. The space: battery envelopes (all clearances ≥ 0.3 mm; the cell lies on the back-cover floor), 2. Candidate cells (dims include the protection board (PCM) and tape; "margin" = gap per side incl. the 0.3), 3. Hand time per unit for each grind (estimates; Dremel freehand vs. with the printed jigs in `../jigs/`), 4. Recommendation (production time in mind), 5. Charging and brown-out, 6. The LR44 trim zone (spec, for the battery upgrade only), 7. How to re-run, 8. Placement for the guide (rev E, 2026-10-06: the Adafruit #1317 is now fitted) (+2 more)

### Community 121 - "build"
Cohesion: 0.14
Nodes (23): build(), card(), insert_after(), insert_before(), battery_svg(), cam_ribbon_svg(), chip_body(), diode_body() (+15 more)

### Community 122 - "Docs consistency audit (2026-10-06, 01:20–01:40 EDT, stopped early to save usage)"
Cohesion: 0.20
Nodes (9): 0. The one fact that overrides everything, 1. Disagreements between documents, 2. Shopping list verdict (live, 2026-10-06 ~01:30 EDT), 3. Day-one sequence (one list, guide and step), 4. QUESTIONS_AND_ISSUES.md (audit only; rewrite not done), 5. Files changed (46 scripted edits + 1 manual), 6. Still to do (next session), Docs consistency audit (2026-10-06, 01:20–01:40 EDT, stopped early to save usage) (+1 more)

### Community 123 - "Menu System"
Cohesion: 0.50
Nodes (4): Hyperbolic Menu Screen, Mode Menu Screen, Setup Menu Screen, Menu System

### Community 124 - "test_proxy.py"
Cohesion: 0.07
Nodes (6): calculator_body(), fake_image(), Firmware, FirmwareHttp, Policy, Requests

### Community 125 - "pinmap_crossref.py"
Cohesion: 0.10
Nodes (26): doc_gpio_claims(), doc_text(), gpio_of_pinname(), iter_blocks(), kicad_cli_netlist(), kid(), kids(), main() (+18 more)

### Community 126 - "Final assembly progress (resume here)"
Cohesion: 0.29
Nodes (6): 3-part shell + slide case + window mask (2026-10-05), Battery upgrade study + jigs (2026-10-06, night), Faceplate refinement (2026-10-05 evening), Final assembly progress (resume here), Rev F: fitment review (2026-10-06, verification/07), Visual guide rework (2026-10-05)

### Community 127 - "Firmware stage-13 work: progress notes"
Cohesion: 0.40
Nodes (4): Firmware stage-13 work: progress notes, Log, Plan, Setup facts (2026-10-03)

### Community 128 - "Event"
Cohesion: 0.13
Nodes (14): Event, confidence, failure, id, kind, text, Kind, Captured (+6 more)

### Community 130 - "firmware-prototype/src/keys.cpp"
Cohesion: 0.15
Nodes (17): beginOn(), keysBegin(), keysHeld(), keysPoll(), keysPrepareSleep(), keysWokeByOn(), pollOn(), Repeat (+9 more)

### Community 131 - "AI Calculator PCB - KiCad design summary"
Cohesion: 0.18
Nodes (10): AI Calculator PCB - KiCad design summary, Board (`ai_calc.kicad_pcb`), Custom footprint library `ai_calc.pretty`, Hierarchy, Sheet: Camera OV5640 (`camera.kicad_sch`), Sheet: Connectors (magnet USB, battery) (`connectors.kicad_sch`), Sheet: E-paper driver (`epaper.kicad_sch`), Sheet: ESP32-S3 MCU (`mcu.kicad_sch`) (+2 more)

### Community 132 - "make_bringup.py"
Cohesion: 0.16
Nodes (16): arrow(), column_labels(), edge_bbox(), fit(), font(), footprints(), holes_px(), labels() (+8 more)

### Community 133 - "firmware-prototype/src/screen.cpp"
Cohesion: 0.12
Nodes (11): Camera preview (viewfinder), cameraSleep(), stopViewfinder(), powerCameraPinsSafe(), draw(), fnv1a(), screenBegin(), screenEndPanel() (+3 more)

### Community 134 - "v15-LCD final assembly: does everything fit? (final board + ER-TFT019-1)"
Cohesion: 0.18
Nodes (10): 1. Interference (closed case, every body pair, 3D boolean), 2. Clearances (mm), 3. Height stack (Z from the outside of the back cover): unchanged, 4. Tail route and the rib-B margin, 5. Moved key pads vs pills (`key_pills.md`, `renders/key_pills_top_row.png`), 6. Pin 1 and backlight (from report 14, no CAD change), 7. What changes for Nirav, 8. Files (+2 more)

### Community 138 - "ui.cpp"
Cohesion: 0.07
Nodes (49): UI fixes from the PC screenshots (2026-10-10), Cell, cp, inverted, raw, rows, underline, decode() (+41 more)

### Community 139 - "Part B: end-of-line test, every finished unit"
Cohesion: 0.11
Nodes (16): A1. Visual, no tools, A2. Under the loupe (assembled boards), A3. Meter before first power (board alone, nothing plugged in), A4. First power (USB only, no battery, no panels), B1. Look and feel (1 min), B2. Factory self-test (3–4 min), B3. Charge check (2 min, can overlap with B4/B5), B4. Camera focus target card (+8 more)

### Community 140 - "arduino"
Cohesion: 0.14
Nodes (8): hexOf(), lower(), otaCheck(), otaInstall(), hexOf(), lower(), otaCheck(), otaInstall()

### Community 141 - "Decode"
Cohesion: 0.17
Nodes (11): Decode, cx, cy, fullH, fullW, jpeg, lastYield, out (+3 more)

### Community 142 - "AI Calculator PCB: handoff for the next Claude session (from stage 14)"
Cohesion: 0.29
Nodes (6): AI Calculator PCB: handoff for the next Claude session (from stage 14), Decisions you must not undo, Lessons (stage 14), Stage table, Tools (`hardware/tools/`), Where things stand

### Community 143 - "Stage 2: electrical plan (pins, power, keypad)"
Cohesion: 0.25
Nodes (7): 1. Pin map (final: `hardware/pins_final.h`), 2. Power, 3. Keypad, 4. Open items for later stages, Battery life (120 mAh, ~100 mAh usable), Stage 2: electrical plan (pins, power, keypad), Why the magnetic contacts never carry battery voltage

### Community 144 - "JLCPCB order checklist (stage 14, silk "AI CALC v14 2026-10-04")"
Cohesion: 0.17
Nodes (11): After it arrives (before anything is plugged in), Assembly options, Before closing the case, Before paying, Expected cost, Files, If the engineers email, In the placement preview (+3 more)

### Community 145 - "Stage 3: schematic (done)"
Cohesion: 0.29
Nodes (6): Changes made in the final review, Design-review fixes (2026-10-02, independent review of the whole schematic), Netlist sanity check (done by script), Notes for the firmware, Stage 3: schematic (done), Still open (needs your hardware, not blocking the board)

### Community 146 - "Stages 4 + 5: footprints and placement"
Cohesion: 0.29
Nodes (6): Footprints (all real JLCPCB/LCSC land patterns), How it's built (repeatable), Placement (KiCad view = looking at the board from the back cover), Stage 6: routing (first complete pass, 2026-10-02), Stages 4 + 5: footprints and placement, The two ribbon cables (the riskiest part of the board)

### Community 147 - "full.sh script"
Cohesion: 0.38
Nodes (4): drc(), full.sh script, route.sh script, run.sh script

### Community 148 - "Verification 02: power, boot, recovery, protection, analog (stage 13b)"
Cohesion: 0.18
Nodes (10): CHECK (measure on board 1, a firmware rule, or a low-probability risk), FATAL, Findings, by severity, LIKELY PROBLEM, OK (checked, with evidence), Power budget, Sources, Verdict (+2 more)

### Community 149 - "Final assembly check: does everything fit?"
Cohesion: 0.17
Nodes (11): 10. Rev F: side-wall inner rib to the rim, zone 6, full clearance table (2026-10-06, review 07), 1. Real collisions (need a decision), 2. Contacts that are meant to be there (not problems), 3. Tight spots (they fit, but check them on the real parts), 4. What each grind fixes (same check with the shell *before* grinding), 5. What the model is sure about, and what it isn't, 6. Stage 14 re-check (2026-10-04 17:40), 7. 2-part body re-check (2026-10-05) (+3 more)

### Community 150 - "CameraPins"
Cohesion: 0.11
Nodes (18): CameraPins, d0, d1, d2, d3, d4, d5, d6 (+10 more)

### Community 151 - "05: Independent re-check of stage 14 (v14) before the one-shot order"
Cohesion: 0.15
Nodes (12): 05: Independent re-check of stage 14 (v14) before the one-shot order, 1. Regression diff, stage 13b backup vs. v14, 2. J3 magnet connector, 3. CPL rotations (EasyEDA footprints, fetched per LCSC number), 4. DRC / ERC (on copies), 5 and 6. Board-wide geometry and keypad, Board, Findings (+4 more)

### Community 152 - "AI Calculator PCB - KiCad design summary"
Cohesion: 0.18
Nodes (10): AI Calculator PCB - KiCad design summary, Board (`ai_calc_v15_lcd.kicad_pcb`), Custom footprint library `ai_calc.pretty`, Hierarchy, Sheet: Camera OV5640 (`camera.kicad_sch`), Sheet: Connectors (magnet USB, battery) (`connectors.kicad_sch`), Sheet: ESP32-S3 MCU (`mcu.kicad_sch`), Sheet: Keypad and TCA8418 (`keypad.kicad_sch`) (+2 more)

### Community 153 - "Settings"
Cohesion: 0.29
Nodes (5): Settings, deviceToken, password, proxyUrl, ssid

### Community 154 - "Panel"
Cohesion: 0.15
Nodes (12): Panel, clear, drawText, fillRect, get, kBytesPerRow, kHeight, kStride (+4 more)

### Community 155 - "CameraPins"
Cohesion: 0.11
Nodes (18): CameraPins, d0, d1, d2, d3, d4, d5, d6 (+10 more)

### Community 157 - "hardware/tools/prepass.py"
Cohesion: 0.25
Nodes (3): connect_tree(), pad(), stub()

### Community 158 - "OtaInfo"
Cohesion: 0.29
Nodes (6): OtaInfo, available, path, sha256, size, version

### Community 159 - "bootMessage"
Cohesion: 0.38
Nodes (6): keysScannerOk(), bootMessage(), serviceOtaConfirm(), otaConfirm(), otaPendingVerify(), otaSlotName()

### Community 160 - "AI Calculator board: final status (start here)"
Cohesion: 0.18
Nodes (10): 1. Before you pay JLCPCB (3 things), 2. Order steps, 3. Day-one sequence (when the parts arrive), 4. Files, 5. Decisions log (key decisions and why), 6. Open questions, 7. Version history, AI Calculator board: final status (start here) (+2 more)

### Community 161 - "Slide case mods (LATER: not needed for testing)"
Cohesion: 0.33
Nodes (5): Results (model, 2026-10-05, `case_fit.json`), Slide case mods (LATER: not needed for testing), What the model assumes (all guesses until the case is measured), Zone C1: camera hole in the case (in use), Zone C2: magnet

### Community 162 - "Stage 14 progress (verification fixes)"
Cohesion: 0.50
Nodes (3): Notes, Stage 14 progress (verification fixes), Status

### Community 163 - "calc_engine.h"
Cohesion: 0.08
Nodes (23): AngleUnit, Deg, Gra, Rad, CalcError, Argument, Math, None (+15 more)

### Community 165 - "glyphdraw.h"
Cohesion: 0.25
Nodes (12): advance(), bits(), glyph(), radical(), text(), textCentered(), textFit(), textRight() (+4 more)

### Community 166 - "Grinding jigs (3D-printed) for the donor fx-115ES shell"
Cohesion: 0.33
Nodes (5): A + A2 + A3: back-cover plate (solar box, rib B, camera window), B: magnet-notch saddle, C: LR44 trim sled (battery upgrade only), Grinding jigs (3D-printed) for the donor fx-115ES shell, Time summary (per unit, estimates)

### Community 169 - "SolveCallbacks"
Cohesion: 0.40
Nodes (4): SolveCallbacks, fail, partial, reply

### Community 170 - "assembly_howto_pictures.py"
Cohesion: 0.09
Nodes (31): arrow(), Canvas, dcxyx(), details(), diff_mask(), font(), grind(), keys_vs_pills() (+23 more)

### Community 172 - "Who should build the board? JLCPCB vs the alternatives"
Cohesion: 0.22
Nodes (8): Comparison table, Correction to the cost in the order docs, Risks with JLCPCB and how to reduce them, Sources (all accessed 2026-10-06), The order, as it actually is, Verdict (one line), Who should build the board? JLCPCB vs the alternatives, Why JLCPCB wins this particular order

### Community 174 - "firmware-prototype/src/settings.cpp"
Cohesion: 0.21
Nodes (16): deviceId(), handle(), help(), tokenHint, settingsCheckProxy(), settingsCheckToken(), settingsCheckWifi(), settingsLoad() (+8 more)

### Community 175 - "Stage 15: v15-LCD, the 2.13" e-paper replaced by a 1.9" colour IPS LCD"
Cohesion: 0.18
Nodes (10): 1. What stayed exactly as in v14, 2. The LCD choice (plain English), 3. Electrical design, 4. Layout (as on the board after review 13), 5. Checks run, 6. Pin-1 of the ribbon at J5 (the J1 lesson, applied in review 13), 7. Firmware TODO (written before the port; now done in `firmware-v15-lcd/`, see its README and PORT_NOTES; kept for the reasoning), 8. How to re-make v15 from scratch (for the next session) (+2 more)

### Community 176 - "firmware-prototype/src/preview.cpp"
Cohesion: 0.37
Nodes (17): authorised(), autotuneHandler(), detailHandler(), focusHandler(), frameHandler(), lastHandler(), noCache(), pageHandler() (+9 more)

### Community 178 - "13 — Independent review of the v15-LCD board, re-derived from the raw files"
Cohesion: 0.13
Nodes (14): 13 — Independent review of the v15-LCD board, re-derived from the raw files, 1. Findings, 2. The LCD interface, pad by pad (check 2), 3. Power (check 3), 4. ESP32-S3 pins (check 4), 5. Manufacturing (check 5), 6. Diff vs v14 (check 6), 7. Changes made by this review (all under `hardware/kicad_v15_lcd/`, `hardware/fab_v15_lcd/`, `hardware/tools/`) (+6 more)

### Community 179 - "lcd.cpp"
Cohesion: 0.07
Nodes (26): Build result, Needs the real panel to verify (bench checklist), Open items / later, Port notes: v14 e-paper firmware → v15-LCD (2026-10-08), Preview frame-rate estimate, applyLevel(), begin(), dutyFor() (+18 more)

### Community 180 - "Knockoff shell plan: measuring a 991ES-style clone and refitting the board (v15)"
Cohesion: 0.15
Nodes (12): 1. Ordering samples, 2. Caliper list for the clone sample, 3. What changes on the board for v15, 4. Go / no-go rule for the clone, 5. Trade-dress caution, 6. Order of work, Camera, magnet and battery spots, Case, with calipers (+4 more)

### Community 181 - "rrect_pts"
Cohesion: 0.33
Nodes (3): fit_features(), rrect_pts(), shape_pts()

### Community 182 - "build_cost_model.py"
Cohesion: 0.18
Nodes (3): dline(), put(), vline()

### Community 183 - "firmware-v15-lcd/src/keys.cpp"
Cohesion: 0.13
Nodes (19): onOnChange(), onBootChange(), beginOn(), keysBegin(), keysPoll(), keysPrepareSleep(), keysWokeByOn(), onOnChange() (+11 more)

### Community 184 - "06 — Final adversarial review of PCB v14 (before ordering)"
Cohesion: 0.20
Nodes (9): 06 — Final adversarial review of PCB v14 (before ordering), Checks done and results, FATAL, Findings, ranked, MINOR, NOTE, SERIOUS, Verdict: **GO** (no FATAL, no SERIOUS found; nothing in the files needs to change before ordering) (+1 more)

### Community 185 - "firmware.py"
Cohesion: 0.15
Nodes (4): _load_dotenv(), _paths(), publish(), sha256_of()

### Community 186 - "Goldenmorning T190X7-C30-01H: does it fit the v15-LCD build?"
Cohesion: 0.29
Nodes (6): 1. Free space, 2. Tail bow ↔ rib B (gap in mm), 3. Interference and other clearances (closed case, after grinds), 4. Ask Goldenmorning, Goldenmorning T190X7-C30-01H: does it fit the v15-LCD build?, What was modelled

### Community 187 - "Unit economics: what each calculator costs and earns"
Cohesion: 0.22
Nodes (8): 1. The 5 testers (v14 e-paper, genuine Casio shells), 2. What one calculator costs at volume (e-paper vs LCD, small vs big battery), 3. What you keep on a $225 sale, 4. Does $15 a month cover the Claude API?, 5. Where the $2,000 goes (new order of steps), 6. Next sourcing actions, 7. Numbers to replace with real data first, Unit economics: what each calculator costs and earns

### Community 188 - "Keypad 8x10 matrix (ROW0-7, COL0-9, SW1-SW50)"
Cohesion: 0.31
Nodes (9): Keypad 8x10 matrix (ROW0-7, COL0-9, SW1-SW50), Sheet: Keypad and TCA8418 (keypad.kicad_sch), U6 TCA8418 keypad scanner (I2C), kMatrix in src/keys.cpp (key at each matrix crossing), Serial Monitor commands (wifi, key, status, preview, scan, snap, forget, keys, exam off), keys command character-to-key table, Check-on-real-hardware list (VERIFY knobs: EPD_T, TRAVEL, PILOT_D, ...), Key layout mismatch check (#10: board row 2 vs firmware key table) (+1 more)

### Community 189 - "Arrival checklist: from boxes to a working calculator"
Cohesion: 0.33
Nodes (5): 1. What arrives, from whom, and what to check on the box, 2. Before the boards land (while you wait), 3. Day one, in order (one board at a time), 4. v15-LCD boards (added 2026-10-08 evening), Arrival checklist: from boxes to a working calculator

### Community 190 - "11 — Requirements trace: is everything we decided actually on the v14 board?"
Cohesion: 0.29
Nodes (6): 11 — Requirements trace: is everything we decided actually on the v14 board?, Decisions that live off the board (not checkable here), Deliberately not on v14 (decided for a later revision), Result first, The trace (script output, 2026-10-06), Where the decisions were mined from

### Community 191 - "LCD panel options for the v15-LCD board"
Cohesion: 0.17
Nodes (11): Alibaba.com suppliers (2026-10-08), Board changes needed, BuyDisplay ER-TFT019-1: checked against its datasheet (2026-10-08), Comparison, Fit limits used (from REPORT.md and stage15_lcd.md), LCD panel options for the v15-LCD board, Original research verdict (2026-10-08 morning, superseded), Practical notes (+3 more)

### Community 192 - "window_mask_template_v15.py"
Cohesion: 0.25
Nodes (3): dxf(), rrect_path(), svg()

### Community 193 - "check_hole_clearance.py"
Cohesion: 0.38
Nodes (3): inter(), sd(), ss()

### Community 194 - "firmware-v15-lcd/src/claude_client.cpp"
Cohesion: 0.24
Nodes (14): lower(), open(), parseUrl(), proxyDownload(), proxyRequest(), readBody(), readHead(), readLine() (+6 more)

### Community 195 - "f"
Cohesion: 0.20
Nodes (8): cavity(), chain(), crossings(), log(), stage_check(), stage_export(), wall_fun(), f()

### Community 197 - "viewfinder.cpp"
Cohesion: 0.25
Nodes (10): bracket(), focusMeasure(), hLine(), diff, invert, fps, render, renderStarting (+2 more)

### Community 198 - "firmware-v15-lcd/src/camera.cpp"
Cohesion: 0.09
Nodes (44): What changed, file by file, Files, applyAll(), autoTuneLocked(), beginIn(), bringUp(), cameraAutoTune(), cameraBegin() (+36 more)

### Community 209 - "Ask"
Cohesion: 0.33
Nodes (6): Ask, Nothing, Password, Proxy, Ssid, Token

### Community 211 - "Firmware for the stage-13 board"
Cohesion: 0.10
Nodes (19): Bring-up checklist for the first board, Deploy the proxy (short version; full steps in `server/proxy/README.md`), Factory self-test, Files, Firmware for the stage-13 board, First setup (Serial Monitor), Flash it over the magnetic USB, Power facts (+11 more)

### Community 212 - "SolveCallbacks"
Cohesion: 0.40
Nodes (4): SolveCallbacks, fail, partial, reply

### Community 213 - "Findings"
Cohesion: 0.25
Nodes (7): Changes made, FATAL, Findings, Firmware changes requested, SERIOUS, Server-side review: server/proxy (stopped early, 2026-10-06 ~01:35 USEDT), Verdict

### Community 214 - "firmware-v15-lcd/src/preview.cpp"
Cohesion: 0.18
Nodes (29): cameraSaveTuning(), previewEnded(), authorised(), autotuneHandler(), detailHandler(), endPreview(), focusHandler(), frameHandler() (+21 more)

### Community 215 - "SolveCallbacks"
Cohesion: 0.33
Nodes (5): SolveCallbacks, cancelled, fail, partial, reply

### Community 216 - "make_fitcheck.py"
Cohesion: 0.12
Nodes (5): main(), newest(), box(), cy_box(), ring_path()

### Community 218 - "stage_parts"
Cohesion: 0.15
Nodes (14): band(), cam_route(), mk(), fit_bow(), obround_pts(), _opt(), path_len(), round_path() (+6 more)

### Community 219 - "SerialCmd"
Cohesion: 0.12
Nodes (17): SerialCmd, Account, Bright, Changed, ExamOff, Keys, None, Pair (+9 more)

### Community 220 - "selftest_tu.cpp"
Cohesion: 0.14
Nodes (4): cameraSelfTest(), deviceId(), hostRunSelfTest(), keysHeld()

### Community 222 - "Event"
Cohesion: 0.13
Nodes (14): Event, confidence, failure, id, kind, text, Kind, Captured (+6 more)

### Community 223 - "14 — Final pre-order verification of the v15-LCD board (ER-TFT019-1 panel)"
Cohesion: 0.15
Nodes (12): 14 — Final pre-order verification of the v15-LCD board (ER-TFT019-1 panel), 1. Findings, 2. Check list (all PASS unless marked), 3. Changes made in this pass (all logged; v14 untouched), 4. The J5 pin-1 derivation, step by step (check 2), 5. Backlight (check 3), numbers, 6. Bench checks (none blocks the order), 7. Files touched (absolute) (+4 more)

### Community 224 - "Failure"
Cohesion: 0.08
Nodes (22): Failure, Account, ApiBusy, ApiError, BadReply, Camera, LowBattery, NoApiKey (+14 more)

### Community 226 - "SetupInfo"
Cohesion: 0.40
Nodes (4): SetupInfo, apName, apPass, url

### Community 227 - "Kind"
Cohesion: 0.40
Nodes (5): Kind, Captured, Fail, Partial, Reply

### Community 228 - "v15 colour LCD: real UI screenshots"
Cohesion: 0.40
Nodes (4): Fixed in the firmware (2026-10-10), How these can differ from the real panel, Regenerate, v15 colour LCD: real UI screenshots

### Community 229 - "testClaudeApi"
Cohesion: 0.18
Nodes (19): base64Encode(), buildSolveRequest(), classifyFailure(), jsonString(), peekAnswer(), StreamReader, buf_, errorMessage_ (+11 more)

### Community 230 - "FrameInfo"
Cohesion: 0.12
Nodes (17): Focus3, Focusing, Moving, Sharp, Starting, FrameInfo, hi, lo (+9 more)

### Community 231 - "CamFocus"
Cohesion: 0.50
Nodes (4): CamFocus, Focused, Focusing, NoAf

### Community 232 - "Button"
Cohesion: 0.33
Nodes (6): Button, key, lastMs, nextRepeat, pin, pressed

### Community 233 - "JLCPCB order walkthrough: v15-LCD board (silk "AI CALC v15-LCD 2026-10-08")"
Cohesion: 0.17
Nodes (11): 0. Before you start, 1. Upload the gerbers, 2. PCB options, 3. PCB Assembly, 4. BOM and CPL, 5. Placement preview: what each part must look like, 6. Order notes (paste into the "remark" box), 7. If JLCPCB's engineers email you (+3 more)

### Community 234 - "restoreState"
Cohesion: 0.46
Nodes (4): restoreState, Reader, ok, pos

### Community 235 - "15. v15-LCD key pads vs the original Casio fx-115ES keyboard PCB"
Cohesion: 0.18
Nodes (10): 15. v15-LCD key pads vs the original Casio fx-115ES keyboard PCB, Changes to make (for the v15 board editor), Every pad, How it was measured, Images, Key mapping vs the firmware, Other observations (no action requested), REPLAY ring and ON (+2 more)

### Community 236 - "build_final_assembly_v15.py"
Cohesion: 0.09
Nodes (20): all_bodies(), bodies_under(), _case_vs_all(), _check(), collision_table(), grind_cuts(), info_path(), leaves() (+12 more)

### Community 237 - "Board firmware for v15-LCD (colour screen + live camera preview)"
Cohesion: 0.25
Nodes (7): Board firmware for v15-LCD (colour screen + live camera preview), Build and flash, Pins (from `hardware/kicad_v15_lcd/pins_v15_lcd.h`), Screen and backlight, Self-test, Using the live preview, What is different from v14

### Community 239 - "SetupInfo"
Cohesion: 0.40
Nodes (4): SetupInfo, apName, apPass, url

### Community 241 - "Decode"
Cohesion: 0.18
Nodes (10): Decode, cx, cy, fullH, fullW, lastYield, out, outH (+2 more)

### Community 242 - "cstdint"
Cohesion: 0.09
Nodes (6): HostEsp, HostRestart, HostSerial, printf, quiet, esp_chip_info()

### Community 243 - "Wake"
Cohesion: 0.40
Nodes (5): Wake, ColdBoot, Keypad, OnKey, Other

### Community 244 - "ScanNote"
Cohesion: 0.50
Nodes (4): ScanNote, None, Ok, Unclear

### Community 245 - "SendStep"
Cohesion: 0.50
Nodes (4): SendStep, Idle, Solving, WaitWifi

### Community 246 - "harness.cpp"
Cohesion: 0.13
Nodes (11): powerOff, setOnline, tick, cameraFrame(), key(), loadPpm(), main(), setup() (+3 more)

### Community 247 - "IdlePolicy"
Cohesion: 0.40
Nodes (5): IdlePolicy, dimSeconds, offSeconds, sleepSeconds, setIdlePolicy()

### Community 248 - "2026-10-10"
Cohesion: 0.22
Nodes (8): 2026-10-10, Autonomous work log, Docs consistency 2026-10-10, Extra: DCXYX OV5640-AF camera (DCXYX-LZTKQJ-5M-339) on v15: FITS with a tuck, Task 1: Goldenmorning T190X7-C30-01H fit check: DONE, Task 2: magnet connector alternates: DONE, Task 3: open items: DONE (what can be done without hands or money), Task 4: v15 firmware build and warnings: DONE (no code change needed)

### Community 249 - "Wake"
Cohesion: 0.40
Nodes (5): Wake, ColdBoot, Keypad, OnKey, Other

### Community 251 - "OtaInfo"
Cohesion: 0.29
Nodes (6): OtaInfo, available, path, sha256, size, version

### Community 253 - "firmware/src/settings.cpp"
Cohesion: 0.16
Nodes (16): cameraName(), setup(), statusText(), Ask, Key, Nothing, Password, Ssid (+8 more)

### Community 254 - "firmware/src/main.cpp"
Cohesion: 0.07
Nodes (42): dkeyName(), keysForChar(), cameraHoldUntil(), applySettings(), connectWifi(), Event, confidence, failure (+34 more)

### Community 255 - "Tuning"
Cohesion: 0.40
Nodes (5): Tuning, def, max, min, name

### Community 257 - "Line"
Cohesion: 0.67
Nodes (3): Line, header, text

### Community 258 - "cameraCapture"
Cohesion: 0.12
Nodes (21): cameraCapture(), cameraFocusScore(), cameraHoldEstimateMs(), cameraPreviewFrame(), Decode, cx, cy, data (+13 more)

### Community 259 - "firmware-v15-lcd/src/setup_portal.cpp"
Cohesion: 0.07
Nodes (43): Ask, Nothing, Password, Proxy, Ssid, Token, deviceId(), examLoad() (+35 more)

### Community 261 - "previewStart"
Cohesion: 0.21
Nodes (11): previewEnded(), endPreview(), getUri(), jsonText(), previewResume(), previewSetResult(), previewStart(), previewSuspend() (+3 more)

### Community 263 - "HistoryItem"
Cohesion: 0.50
Nodes (3): HistoryItem, expr, value

### Community 265 - "annotate_guide.py"
Cohesion: 0.10
Nodes (15): arrow(), draw(), font(), label_box(), px_points(), run(), trim(), arrow() (+7 more)

### Community 271 - "app.h"
Cohesion: 0.07
Nodes (23): Effort, Careful, Max, Normal, Key, AC, Down, Eq (+15 more)

### Community 279 - "check_hole_clearance_v15.py"
Cohesion: 0.38
Nodes (3): inter(), sd(), ss()

### Community 285 - "Tuning"
Cohesion: 0.40
Nodes (5): Tuning, def, max, min, name

### Community 288 - "cameraPreviewRelease"
Cohesion: 0.60
Nodes (4): cameraPreviewGrab(), cameraPreviewMode(), cameraPreviewRelease(), grabTask()

### Community 289 - "Tuning"
Cohesion: 0.40
Nodes (5): Tuning, def, max, min, name

### Community 291 - "SendStep"
Cohesion: 0.50
Nodes (4): SendStep, Idle, Solving, WaitWifi

### Community 294 - "firmware-v15-lcd/src/power.cpp"
Cohesion: 0.09
Nodes (34): lipoPercent(), cameraSleep(), keysHeld(), serviceBattery(), setup(), batteryMillivolts(), batteryOkForAi(), batteryOkForOta() (+26 more)

## Ambiguous Edges - Review These
- `AI Hold-to-Capture Flow` → `Exam Mode`  [AMBIGUOUS]
  tests/golden/exam_confirm.txt · relation: conceptually_related_to

## Knowledge Gaps
- **1230 isolated node(s):** `make_outputs.sh script`, `On`, `Off`, `AC`, `Eq` (+1225 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 1986 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **46 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `AI Hold-to-Capture Flow` and `Exam Mode`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **Why does `Device` connect `Device` to `Failure`, `device.cpp`, `Vars`, `Tok`, `calc_engine.h`, `HistoryItem`, `restoreState`, `ui.cpp`, `Num`, `font.h`, `ScanNote`, `harness.cpp`, `tests.cpp`, `View`, `App`?**
  _High betweenness centrality (0.047) - this node is a cross-community bridge._
- **Why does `Viewfinder` connect `Viewfinder` to `v15 colour LCD: real UI screenshots`, `viewfinder.cpp`, `FrameInfo`, `firmware-prototype/src/screen.cpp`, `firmware-v15-lcd/src/camera.cpp`, `ui.cpp`, `firmware-prototype/src/main.cpp`, `Overlay`, `harness.cpp`, `Panel`, `string`, `firmware-v15-lcd/src/main.cpp`?**
  _High betweenness centrality (0.032) - this node is a cross-community bridge._
- **Why does `DKey` connect `DKey` to `device.cpp`, `firmware-prototype/src/keys.cpp`, `calc_engine.h`, `win32_main.cpp`, `firmware-v15-lcd/src/power.cpp`, `Board geometry from the scan: assumptions and confidence`, `Button`, `firmware-prototype/src/main.cpp`, `selftest_tu.cpp`, `harness.cpp`, `firmware-v15-lcd/src/keys.cpp`, `tests.cpp`, `string`, `firmware/src/main.cpp`, `firmware-v15-lcd/src/main.cpp`?**
  _High betweenness centrality (0.027) - this node is a cross-community bridge._
- **Are the 4 inferred relationships involving `Device` (e.g. with `Files` and `sim_state()`) actually correct?**
  _`Device` has 4 INFERRED edges - model-reasoned connections that need verification._
- **Are the 2 inferred relationships involving `App` (e.g. with `Files` and `v15 colour LCD: real UI screenshots`) actually correct?**
  _`App` has 2 INFERRED edges - model-reasoned connections that need verification._
- **What connects `make_outputs.sh script`, `On`, `Off` to the rest of the system?**
  _1230 weakly-connected nodes found - possible documentation gaps or missing edges._