# AI Calculator PCB: handoff for the next Claude session (from stage 14)

Updated 2026-10-04. **Update 2026-10-08 evening:** v14 was ordered on 10/8 (tag `v14-order`, frozen; the rest of this file describes v14). The next board, **v15-LCD** (`kicad_v15_lcd/`, `fab_v15_lcd/`, `stage15_lcd.md`), is verified "ORDER" in `verification/14_v15_final_preorder.md`: BuyDisplay ER-TFT019-1 panel, J5 `LcdReversed` at (158.6, 93.1), slot x 180.5–181.5, LCD GPIOs MOSI 5 / DC 6 / SCK 8 / CS 41 / RST 42, LCD_PWR_N 33, backlight PWM 4, KEYPAD_INT 3, R23 15 Ω (C22810), SW1/SW2/SW50 moved to the Casio contacts; order steps `ORDER_WALKTHROUGH_v15_lcd.md`. Must-not-undo for v15 as well: J5 mirrored like J1. Read `hardware/FINAL_STATUS.md` first (status, open items, decisions, history). This file is the short context a new session needs to continue.

Repo: `C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR`; all hardware is under `hardware/`. KiCad 10 (`kicad-cli` on Windows). Nirav is new to PCB design: explain non-obvious choices in 1–2 plain sentences, no screenshots of his screen. Deliver each stage as a numbered zip `Claude outputs/ai_calc_pcb_N.zip`; he commits and pushes himself. Query the graph first (`graphify query "…" --budget 800`) and run `graphify update .` after changes.

## Where things stand

- **Board v14** (silk "AI CALC v14 2026-10-04"), `kicad/ai_calc.kicad_pcb` + sheets. DRC 0/0/0 with schematic parity (`--refill-zones --schematic-parity --severity-all`), ERC 0. Every copper item ≥ 0.25 mm from every hole.
- **Independent re-check: GO** (`verification/05_stage14_recheck.md`). Fab files in `fab/`, zip `Claude outputs/ai_calc_pcb_14.zip`. Backup of 13b: `kicad/.mcp-backups/stage14_pre/`.
- **Not ordered yet (still true on 2026-10-06).** The plan was Monday 2026-10-05 after Nirav's paper dry fit (`FINAL_STATUS.md` §1); every day the order slips moves the board arrival (about 10–14 days door-to-door after ordering; JLCPCB ≈ $200–240 delivered incl. ~35–37.5 % duty, `FAB_VENDOR_COMPARISON.md`). Ordering: `ORDER_WALKTHROUGH.md`.
- Product: ESP32-S3-MINI-1 board inside a ground **Casio fx-115ES** shell; OV5640 AF camera (J1), Waveshare 2.13" V4 e-paper (J2, through a 1.0 × 14 mm slot), TCA8418 keypad, MCP73831 + RT9080-33GJ5 power, Adafruit #1317 150 mAh LiPo (26 × 19.75 × 3.8), Adafruit #5358/#5412 magnetic USB on J3 (alternates: Yiwei MG04254FRA1S1N = #5358, Yiwei MG0425-UB-60-4P cable = #5412 equivalent, GND, D+, D−, V+ from its N end). No LED, nothing for Nirav to solder. JLCPCB: 2 layers, 0.8 mm, ENIG, 5 PCBs, 2 assembled.
- Open items: paper dry fit; E7 / e-paper stiffener length; camera ribbon length; magnet meter check; J4 polarity; E6. Business: pricing vs VovoCorp. (Bigger cell: decided 2026-10-06, Adafruit #1317 150 mAh for the donor shells; see `enclosure/final_assembly/battery_upgrade.md`.)
- Guides (published): First Power-Up https://claude.ai/artifact/PqTARcDutvwxS5aGB8iEG9 · Shell Grinding https://claude.ai/artifact/9rCWyw8MeHP2z8FWijnyfL · Calculator Assembly https://claude.ai/artifact/8BTC57FZ3JFwiDdiLprwtY · Launch Roadmap https://claude.ai/artifact/K5hMpGzzf4gS9b6nGKSoM9.
- Docs consistency audit of 2026-10-06: `verification/10_docs_consistency.md`.

## Stage table

| Stage | State |
|---|---|
| 1–9 | Geometry, pin map, schematic, placement, routing, camera GPIO re-order (6), outputs (7), v8 camera bus (8/9). Details: stage2/3/4_5/9 docs |
| 10 | fx-115ES shell: holes from photos (`stage10_fx115es.md`) |
| 11 | E-paper at the measured window, RT9080, R20 → VBUS_SENSE, TP5–TP7 (`stage11_epaper_calipers.md`) |
| 12 | Second caliper batch, narrowed screen section, height table, grind map (`stage12_measurements.md`) |
| 13 / 13b | Heights and grind plan, slotted holes, 7 mm camera window; J3 right-angle header, C33/C34 (`stage13_heights.md`) |
| 14 | Verification fixes: J3 order, CPL rotation tool, SW1/H3 notch, recovery, key names (`stage14_verification_fixes.md`); re-check GO |

Older "don't order yet" notes (stages 11–12: J4/J3 heights, C4 wall, D11) are all resolved; ignore them.

## Decisions you must not undo

- **J1** camera socket = `ai_calc:FPC_24P_P0.5mm_DualContact_C6364666_CamReversed` (pad numbers mirrored; ~90 % confidence after verification 01/03; fallback is a 180° cable twist). Tell JLCPCB **not to rotate J1**.
- **Camera GPIO map** (stage 6): XCLK 18, D0 13, D1 11, D2 10, D3 12, D4 14, D5 16, D6 17, D7 21, VSYNC 36, HREF 47, PCLK 15, PWDN 48, RESET 38, SIOD 40, SIOC 39, PWR_EN 34. `pins_final.h` and firmware use it.
- **E-paper:** FPC through the 1.0 × 14 mm slot at x 172.9 into J2 (177.9, 92.83); ribbon budget 14.3 = 14.3 (CAD path 12.7 → about 1.6 mm nominal spare, 0.3 worst case; treat as none), so **J2 doesn't move** without the E7/stiffener rule in `stage14_verification_fixes.md` fix 6 (ribbon lengths). **J1 doesn't move** either (same fix 6).
- **Power:** U3 = RT9080-33GJ5 (C841192); R20 pulls STAT up to VBUS_SENSE; R12 3 Ω; C20 4.7 µF 50 V; VBUS divider 10 k / 20 k; charge 50 mA (R2 20 k); C32–C34 22 µF on +3V3; **R17 = 100 k** (C25741; the "10 k" in verification 02 was wrong). The magnet contacts never carry battery voltage.
- **Outline and holes:** screen section narrowed past the wall pins (x 118.9–183.65); ESP32 antenna (a bare 0.8 mm tab, 4.15 mm past the edge) overhangs the KiCad-left edge on purpose (= the calculator's right-hand, solar-window side in use); it and the board's un-narrowed top corner (x 114.3–114.8, y 72–83) may need grinding zone 6 (35 mm of the faceplate's inner side-wall rib, `verification/07_final_review_fitment.md`; v15: narrow that corner to x 118.9 as well); H2/H9/H13/H14 slotted, H6 moved (stage 13).
- **J3** = right-angle SMD header C46061768 at (127.5, 72.0), magnet glued in a top-wall U-notch. **Pin order 1 VBUS, 2 USB_DM, 3 USB_DP, 4 GND, piece's N end at pin 1** (silk N / + / −). Never re-introduce "turn the magnet piece over" advice.
- **CPL:** always generate with `tools/make_outputs.sh` (BOM first, then `tools/jlc_cpl.py`, which adds JLCPCB offsets per LCSC: Q1 180, Q2 270, U2/U3/U7 270, U4/U5 180, J3 180). In JLCPCB's preview, verify only.
- **SW1** uses `KeyPad_6.0x4.5_H3notch` (clearance to H3).
- **Keys:** row 2 = CALC, ∫dx (fx-115ES); firmware: Abs = SHIFT hyp, x³ = SHIFT x², ∛ = SHIFT √.
- **Recovery:** hold TP1 (BOOT) to TP4 (GND), tap TP7 (EN); UART on TP2/TP3. Never burn security or USB-disable eFuses.
- **Battery** lies on the back-cover floor; the LR44 cup is not ground. Grinds: solar box, rib B 14 mm, 7 mm camera drill, magnet U-notch (5b lip only if present).

## Lessons (stage 14)

- **Verify CPL rotations against the EasyEDA footprints** JLCPCB actually uses (fetch per LCSC number; `tools/check_cpl_easyeda.py`), not against KiCad or guesses. Include a negative test (raw KiCad rotations must fail).
- **Verify connector pinouts against the mating part** (here the #5412 cable drawing, mated face to face), not just the board-side part's drawing.
- Run the copper-to-hole check (`tools/check_hole_clearance.py`) as well as DRC; DRC missed SW1 at 0.055 mm from H3.
- After any change: regenerate `fab/`, the fit-check files and the zip; confirm BOM designators == CPL designators (77); re-run DRC with parity and ERC.

## Tools (`hardware/tools/`)

`make_outputs.sh` (fab outputs), `jlc_cpl.py`, `check_cpl_easyeda.py`, `check_hole_clearance.py`, `make_fitcheck.py`, `check_keys.py`, `placement_check.py`; routing helpers `pilroute.py`, `gnd_islands.py`, `smooth.py` (Windows) and the older cloud-box scripts (`build_board.py`, `prepass.py`, `gridroute.py`, `route.sh`, `add_gnd.py`, `cleanup.py`, `fixroute.py`).
