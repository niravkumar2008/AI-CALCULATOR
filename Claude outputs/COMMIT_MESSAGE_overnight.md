PCB stages 12–14 (verified GO to order), 1:1 final assembly, stage-13 firmware

Board (hardware/kicad, fab/, ai_calc_pcb_13.zip):
- Stage 12: screen section narrowed to clear the wall stubs, H1 moved so
  H1–H3 = 46.5 (C8), e-paper panel 59.0, fiducials, v12 silk, part heights
  from 3D models.
- Stage 13: heights checked against the measured 6.0 mm behind the board;
  J4 and the camera fit once the solar box and rib B are ground; locating
  holes H2/H9/H13/H14 slotted, H6 moved 0.2 mm.
- Stage 13b: the magnet connector moves to the board edge on a
  right-angle SMD header (C46061768) with no board notch; D7/U2/C6 moved;
  C33/C34 22 uF added for Wi-Fi bursts.
- DRC with refilled zones and schematic parity: 0/0/0. ERC 0.
- Final review (REVIEW_final_2026-10-04.md): ready to order, with an
  ordering walkthrough.

- Stage 14: a 4-part verification (hardware/verification/01–04) found two
  fatal issues, now fixed:
  - J3 re-wired to VBUS, D-, D+, GND to match the mated #5412 cable.
  - JLCPCB CPL rotations corrected for Q1, Q2, U2, U3, U4, U5, U7 and J3,
    with tools/jlc_cpl.py making the corrections permanent.
  Also: SW1 hole clearance, recovery docs, the fx-115ES key names
  (CALC, integral dx), and individual BOM designators.
- Independent re-check (05_stage14_recheck.md): GO.
- Camera viewfinder firmware: dithered live preview on the e-paper.

CAD (hardware/enclosure):
- fx-115ES replica rev B at the measured 11.3/11.8 mm thickness.
- New 1:1 final assembly (build_final_assembly.py): ground shell, board,
  e-paper, camera, LiPo, magnet, keys and screws. No collisions once the
  battery lies on the back-cover floor.

Firmware:
- pins_final.h is the single pin source.
- Deep sleep, Wi-Fi TX cap, battery curve.
- Factory self-test (SHIFT+ALPHA+ON).
- Proxy server in server/proxy so the calculator never holds the API key.
- Pairing codes, verified answers and tutor mode.
- 520/520 PC tests and 9/9 proxy tests pass.

Docs: measurements_2026-10-03.md, stage11–13 notes, QUESTIONS_AND_ISSUES.md
("Start here" section), COMPETITOR_ANALYSIS.md, roadmap updates.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
