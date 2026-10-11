# Stage 14 progress (verification fixes)

Backup: hardware/kicad/.mcp-backups/stage14_pre/ (board, all sheets, pro, dru, sym) — taken at start.

## Status
- [x] 0 backup, read 01–04 verification docs
- [x] 1 J3 pin order
- [x] 2 CPL rotation post-processor + EasyEDA check
- [x] 3 SW1/H3 + board-wide hole clearance
- [x] 4 recovery docs
- [x] 5 key names (sch + firmware)
- [x] 6 J1/J2 ribbon
- [x] 7 order checklist / walkthrough
- [x] 8 regenerate outputs, v14 silk, graphify
- [x] 9 stage14_verification_fixes.md + QUESTIONS

## Notes
- J3 derivation DONE: #5412 drawing (cdn-shop.adafruit.com/product-files/5412/5412_C17238_4P.pdf) text along pin axis: N, -, D+, D-, +, S.
  Mated face-to-face: cable N meets board-piece S, contact positions preserved -> board piece from ITS N end: VBUS, D-, D+, GND.
  => J3 = 1 VBUS, 2 USB_DM, 3 USB_DP, 4 GND, N at pin 1. (Backwards fit = reversed supply on the outer pins, D7 forward-clamps; never 5 V on data.)
- connectors.kicad_sch EDITED: labels at x44.45 y59.69->USB_DM, 62.23->USB_DP, 64.77->GND; value text updated. ERC 0 after edit.
- Scratch: scratchpad/s14 (NOT scratchpad root: it has a stray inspect.py shadowing stdlib). Use bash + cygpath -w for python paths.
- [x] FIX 1 board DONE: J3 pads 1 VBUS/2 USB_DM/3 USB_DP/4 GND; DM new via (127.3,74.0)->old B.Cu path; DP pad3->old via (129.8,73.1); GND pad4 -> via (131.31,69.0);
  silk N (121.3,69.7), + (121.3,71.3), - (133.6,71.6). DRC 0/0/0 parity 0, ERC 0. Docs for the magnet advice still TODO.
- [x] FIX 3 DONE: new lib fp ai_calc:KeyPad_6.0x4.5_H3notch (COL bar split: 4.423x0.6 @local(-0.7885,1.95) + 1.677x0.35 @local(2.1615,1.825)); SW1 uses it in keypad.kicad_sch + board.
  scratchpad/s14/holeclr.py = board-wide copper-to-hole check: before 1 violation (H3/SW1 0.055), after 0 (all NPTH >= 0.250). DRC 0/0/0 parity 0, ERC 0.
- [x] FIX 6 DECIDED (no measurements in measurements_2026-10-03.md / QUESTIONS): leave J1 and J2. J1 -2 mm only helps the short-ribbon case and adds 3-12 mm unusable slack in the long case (flat FPC can't take in-plane slack; arch would need >5 mm height) + dense fanout rework; short case is absorbed by the camera keep-out (module may sit <=1.7 mm toward J1) and drilling the window after taping. J2 +1 mm breaks the 14.3=14.3 ribbon budget. Write decision rule in docs.
- [x] FIX 2 DONE: tools/jlc_cpl.py (ROT_BY_LCSC table, expands BOM ranges, refuses missing LCSC) called from make_outputs.sh (BOM now generated before CPL).
  Output == verification corrected CSV for all 77 rows. easyeda2kicad (scratch venv s14/venv; EasyEDA API went 403 after 1st batch, files in s14/ee/lib.pretty) check: 22/22 PASS
  (table saved to verification/stage14_cpl_check.md.tmp); negative test with raw KiCad rotations: Q1,Q2,U2,U3,U4,U5,U7 FAIL (2.1-3.1 mm). J3 body: CPL 180 -> EasyEDA courtyard y 60.9-69.5 (over top edge, OK), 0 would be y 74.5-83.1.
- NOTE R17: schematic/BOM/board = 100k (C25741). Brief + 02 C12 say 10k: that is WRONG; pins_final.h "100 k" already matches the board. Keep 100 k, record discrepancy.
- [x] FIX 4 DONE: pins_final.h (recovery + eFuse + R17 named + MAG order VBUS,D-,D+,GND), FIRMWARE_STAGE13.md step 3, stage2_electrical.md GPIO0 + ON rows.
- FIX 5 in progress: SW4 "Abs"->"CALC", SW5 "x^3"->"∫dx" in keypad.kicad_sch + board values + geometry/keys.csv + derive_geometry.py (DRC 0/0/0, ERC 0). Firmware next: DKey Abs->Calc, Cube->Integral; SHIFT hyp=Abs, SHIFT x²=x³, SHIFT √=∛; Calc/Integral -> "not supported yet" notice.
- [x] FIX 5 firmware DONE: core DKey Calc/Integral (names "CALC","int_dx"), SHIFT hyp=Abs, SHIFT x²=x³, SHIFT √=∛, 'b'=SHIFT hyp; CALC/SOLVE/∫dx/d/dx -> notice "Not supported yet".
  keys.cpp kMatrix positions unchanged. tests: 548 passed 0 failed (new golden notice_calc only; md5 of all others unchanged). MSVC build via s14/tb/build.bat.
  pio firmware-prototype clean build 0 warnings (197 compiles); firmware/ tester 0 warnings; win32 sim compiles. sim/web/page.html labels updated (built simulator.html needs Emscripten rebuild).
- [x] silk "AI CALC v14 2026-10-04"; KEYPADS labels CALC/∫dx; KEEPOUTS magnet note has pin order. DRC 0/0/0.
- [x] FIX 8 partial: fab/ regenerated 14:44 (CPL == corrected CSV), fitcheck + DESIGN_SUMMARY regenerated. Zip + graphify still TODO (do last).
- [x] FIX 7 DONE: ORDER_WALKTHROUGH.md (new), ORDER_CHECKLIST.md rewritten, REVIEW_final walkthrough replaced by pointer + stage-14 note, stage2_electrical pin order, HANDOFF stage-14 row.
- TODO: QUESTIONS (Stage 14 section, Start here, Heights #7, Final review #2), stage14_verification_fixes.md (fold in verification/stage14_cpl_check.md.tmp then delete tmp), zip, graphify update.
- [x] EXTRA: BOM ranges (C32-C34 etc.) -> make_outputs uses --ref-range-delimiter ''; fab regenerated 14:50; BOM 77 == CPL 77; bom/ copy refreshed (was stale since stage 13: old J3).
- [x] FIX 9 DONE: stage14_verification_fixes.md, QUESTIONS (Start here + Stage 14 + superseded #7/#2). Zip ai_calc_pcb_14.zip built (146 files). check scripts kept in tools/.
- TODO: graphify update; final handback.
- [x] graphify update done (2737 nodes). STAGE 14 COMPLETE 2026-10-04 ~14:55.
