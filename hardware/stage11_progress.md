# Stage 11 progress notes (working log, safe to delete after delivery)

State at 2026-10-03 ~01:05: board DRC (refill + schematic parity) = 0 violations / 0 unconnected / 0 parity.

Done:
1. Short/clearance fixes near C19/L1, then full rip-up and re-route of the e-paper block (J2 fan-out by hand,
   booster local nets + MCU signals with tools/pilroute.py). R11/R12 swapped around Q3 (R12 above, R11 below).
2. Review S1: R20 top end moved +3V3 -> VBUS_SENSE (power.kicad_sch label + PCB re-route).
3. Review S2: U3 = RT9080-33GJ5, LCSC C841192 (pinout checked in Richtek DS9080-09: 1 VIN, 2 GND, 3 EN, 4 NC, 5 VOUT).
4. Review S4: C32 22 uF 0805 (C45783, basic) on +3V3 at (118.4, 81.0), mcu sheet.
5. Antenna: new rule area "antenna clearance edge strip" x 113-115.2, y 89-105 (no copper/pour beside the antenna).
6. GND stitching: tools/gnd_islands.py, every pour island >= 1 mm2 has >= 2 vias.
7. Holes: locating posts (2.9 mm) H2/H4/H6/H8/H9/H10 4.4 -> 4.2 mm; screw posts H1/H3 stay 6.0 mm.

Still to do: KEEPOUTS-layer e-paper/window drawings, S3 D7 check, silk/test-point review, outputs
(make_outputs, fitcheck, kicad_summary, graphify), stage11 doc, QUESTIONS_AND_ISSUES.md, HANDOFF/ORDER_CHECKLIST, zip.
Backups: scratchpad step*.kicad_pcb; original WIP %TEMP%\ai_calc_backup_stage11wip.kicad_pcb.

01:30 update: KEEPOUTS drawings moved (panel -3.57, window = measured 60.65 x 24.3 at y 80.95-105.25).
Added TP5 +3V3 (123.6,82.35), TP6 BAT+ (140.4,80.3), TP7 EN (128.0,84.6) (schematic mcu sheet + PCB).
tools/smooth.py straightened 63 staircase chains (1780 -> 302 segments). DRC still 0/0/0. Next: outputs + docs.

10:20 DONE: outputs regenerated, docs written, zip built. Final DRC 0/0/0.
