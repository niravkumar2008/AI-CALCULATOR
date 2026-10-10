# JLCPCB order walkthrough (stage 14 board, silk "AI CALC v14 2026-10-04")

This replaces the walkthrough at the end of `REVIEW_final_2026-10-04.md`. The short version is in `ORDER_CHECKLIST.md`; overall status and what's left: `FINAL_STATUS.md`.

There is only one order, and no second board, so go slowly and check each step.

## 0. Before you start

- **Paper dry fit (the only thing that could still change the board):**
  - Print `../Claude outputs/paper_dry_fit_v14.svg` at 100 % ("actual size"). Measure the printed 50 mm line: it must be 50 mm. Cut it out **together with the yellow antenna tab** on the left edge of the drawing (4.5 × 15.4 mm: the ESP32 antenna is a bare 0.8 mm tab that overhangs the board by 4.15 mm; on the calculator this is the right-hand, solar-window side as you use it). `hardware/fitcheck/grind_map_front_shell.svg` (regenerated 2026-10-06) is the same template with the grinding-guide zone numbers.
  - The antenna tab must clear the shell's side wall; if it rubs on the thin inner rib of that wall, the fix is grinding-guide zone 6 (file 35 mm of that rib, 15–50 mm from the top), not the board; the board's top corner beside the tab must drop in too. Also do the 10-minute pre-grind checklist in `verification/07_final_review_fitment.md`.
  - Lay it in the front shell, on the posts. Each post must sit inside its hole (H2/H3/H6/H9/H13/H14 and the screw posts) without pushing. H6 has the least play (about 0.10 mm); if it pushes only slightly, that's OK (file H6 a little towards the board top later).
  - Ignore any "LR44 cup" grind note: the LR44 holder stays and the battery lies on the back-cover floor.
  - If one is off by more than about 0.5 mm, stop and ask for a hole move.
- **Have these open:**
  - The upload files in `fab/`: `ai_calc_gerbers_JLCPCB.zip`, `ai_calc_BOM_JLCPCB.csv`, `ai_calc_CPL_JLCPCB.csv`.
  - `fab/render_top.png` and `fab/ai_calc_assembly_top.pdf` to compare the preview against.
- **The CPL is already corrected.** `tools/make_outputs.sh` writes it through `tools/jlc_cpl.py`, which adds JLCPCB's rotation offsets:

  | Part | Rotation in the CPL |
  |---|---|
  | Q1 | 180 |
  | Q2 | 270 |
  | U2, U3, U7 | 270 |
  | U4, U5 | 180 |
  | J3 | 180 |

  Every one was checked against JLCPCB's own (EasyEDA) footprints: pin 1 lands on pad 1 (`stage14_verification_fixes.md`, fix 2). **Upload the `fab/` CPL as it is. Don't rotate anything in the preview; only verify it against the table in step 5.** (Independently re-checked: `verification/05_stage14_recheck.md`.)

## 1. Upload the gerbers

- jlcpcb.com → **Order now** → **Add gerber file** → `ai_calc_gerbers_JLCPCB.zip`.
- The viewer should show 2 layers and about **71.7 × 148.9 mm**, with the thin e-paper slot to the right of the screen area.

## 2. PCB options

| Option | Setting |
|---|---|
| Base material | FR-4 |
| Layers | 2 |
| Dimensions | auto |
| PCB qty | **5** |
| Product type | Industrial/Consumer |
| Different design | 1 |
| Delivery format | Single PCB |
| PCB thickness | **0.8 mm** (needed for the keypad stack) |
| PCB colour | Green |
| Silkscreen | White |
| Surface finish | **ENIG** (gold key pads; HASL is too bumpy for the rubber keys) |
| Outer copper weight | 1 oz |
| Via covering | Tented |
| Mark on PCB / remove order number | **Specify a location** (the "JLCJLCJLCJLC" marker is on the component side, away from the keys) |
| Everything else | default |

The four oval NPTH holes and the 1.0 mm e-paper slot are normal. There's no castellation and no impedance control.

## 3. PCB Assembly

| Option | Setting |
|---|---|
| PCBA type | **Standard** (needed for the ESP32 module and the QFN; it includes X-ray) |
| Assembly side | Top side |
| PCBA qty | **2** |
| Tooling holes | Added by JLCPCB |
| Confirm parts placement | **Yes** |
| Edge rails | if asked: **right and bottom edges only** (step 7) |

## 4. BOM and CPL

- Upload `fab/ai_calc_BOM_JLCPCB.csv` and `fab/ai_calc_CPL_JLCPCB.csv`.
- All 30 lines should match, with no shortfall. Every designator is listed separately (C32,C33,C34, not C32-C34), so all 77 parts match the CPL.
  - 14 Extended part types: U1–U7 (6 types), Q3, D7, L1, R12, J1/J2 (one type), J3, J4.
  - Everything else is Basic.
- **Ignore the "not in BOM" note** for TP1–TP7, FID1–3, H* and SW1–SW50. They are bare copper, not parts.
- If any line says "consign" or "pre-order", stop and ask.

## 5. Placement preview: what each part must look like

"Top-left" etc. is as seen in `render_top.png` (component side, board top edge at the top of the picture).

| Part (KiCad x, y) | Must look like |
|---|---|
| **U2, U3, U4, U5** (SOT-23-5) | Pin-1 dot **top-left** (3-pin side on the left) |
| **U7** (SOT-23-6, 133.6, 83.2) | Pin-1 dot **top-left** |
| **Q1** (136.1, 79.5) | Pins 1–2 on the **left**, the single pin 3 on the **right** |
| **Q2** (135.9, 75.6) | The single pin 3 at the **top**, pins 1/2 at the bottom |
| **Q3** (163.6, 97.4) | The single pin 3 on the **left** |
| **U6** TCA8418 (126.0, 146.5) | Pin-1 dot **top-left** |
| **D1–D6** | Band on the **left** pad |
| **D7** TVS (119.2, 73.4) | Band on the **right** pad, towards J3. **This one matters most:** backwards, it shorts the charging cable. |
| **U1** ESP32 (127.55, 97.0) | The antenna end (no pads) hanging off the **left** board edge. If the model sits sideways from the pads, ask them to align it to the pads. |
| **J1** camera socket (150, 161) | Opening facing **up**, towards the camera area. JLCPCB's pin-1 mark will be at the **right** end: that is correct. **J1's pin numbering is intentionally reversed. Don't rotate it.** Align by the pads and the two mounting tabs. |
| **J2** e-paper socket (177.9, 92.8) | Opening facing **left**, towards the slot |
| **J3** magnet header (127.5, 72.0) | The 4 socket openings face the **top edge**; the body hangs about 0.4–1 mm past the edge. The 4 contacts are identical, so "pin 1" doesn't matter here. Only the direction does: body towards the edge, never over D1/D6/U7. |
| **J4** battery socket (142.6, 74.6) | Opening facing **down**; the two small pins at the top, the big tabs at the bottom |

**Don't rotate anything.** The files are already set up to match this table, and the re-check confirmed it. If a part seems to disagree, stop and ask Claude (or JLCPCB's engineer) before clicking anything: it's more likely a reading mistake than a wrong file.

## 6. Order notes (paste into the "remark" box)

> J1 pin numbering is intentionally reversed vs. the library footprint: please align J1 to its pads and mounting tabs, do not rotate it. U1 antenna and J3 body overhang the board edges on purpose (left and top edges). If rails are needed, please put them on the right and bottom edges only. Please X-ray U1 and U6. OK to pull back copper locally if any copper is too close to a hole.

## 7. If JLCPCB's engineers email you

Answer within a day, or the order waits.

| They ask about | Answer |
|---|---|
| "Component exceeds the board edge" (U1, J3) | **Intended, please keep.** The U1 antenna overhang is Espressif's recommended placement. J3 overhangs for the magnet connector. |
| Edge rails / conveyor clearance | Rails on the **right and bottom edges only**. Keep the left edge (antenna) and the top edge (J3) free. A small rail fee is fine. |
| Copper too close to a hole / NPTH / board edge | **OK to pull back copper locally.** Stage 14 already keeps everything ≥ 0.25 mm from every hole. |
| J1 pin 1 doesn't match the library | **Intentionally reversed.** Align by pads and mounting tabs; don't rotate. |
| D7 polarity | Cathode band on the pad towards J3 (KiCad x 120.65, the VBUS pad). |
| Pin-1 silk marks too thin to print | Fine. Use the pads / the assembly drawing. |
| Oval holes / the slot | Intended non-plated slots; the 1.0 mm slot is a routed cut-out. |
| Drill-map files are "unknown layers" | Ignore them, or delete the two `*-drl_map.gbr` files from the zip and re-upload. |
| A part is out of stock / needs a substitute | Stop and check with Claude before accepting a substitute. |

## 8. Expected cost (2026-10 prices; check the final quote)

| Item | Estimate |
|---|---|
| 5 PCBs, 2-layer, 71.7 × 148.9, 0.8 mm, ENIG | $25–40 |
| PCBA setup (Standard) + stencil | about $33 |
| Extended part fees (14 × $3) | $42 |
| Parts (2 boards) | $28–30 |
| Solder joints, misc, possible edge-rail fee | about $2–10 |
| **Subtotal** | **about $135–155** |
| Shipping (DHL/FedEx express) | $20–40 |
| US import duty, collected at checkout (about 35–37.5 %; de-minimis suspended since 2026-06-24) | about $45–55 |
| **Total delivered** | **about $200–240** |

Timing: about **10–14 days door-to-door** (build 5–8 days + express 3–6 days). If express isn't offered, economy shipping takes 7–15 days on its own. Other fabs compared in `FAB_VENDOR_COMPARISON.md` (verdict: stay with JLCPCB).

If the quote is far above this (for example over $200 before shipping), something is mis-ticked. Look for a consigned part, "Economic" instead of "Standard", or a PCB qty other than 5.

## 9. After the boards arrive: meter checks before anything is plugged in

### Battery
- Meter the Adafruit cell's red wire against J4 "+" (right-hand pin, seen from the component side with the opening towards you).
- If it's reversed, swap the two crimp pins in the plug: lift the tabs with a needle. No soldering.

### Camera ribbon
- Fingers 2 and 15 must both beep to GND.

### Magnet piece (do this **before gluing**, with the real #5412 cable)

J3 is wired, from pin 1 (silk "N" and "+", the leftmost pin seen from the component side with the top edge away from you): **VBUS, D−, D+, GND**.

Alternates (added 2026-10-10): the Yiwei **MG04254FRA1S1N** piece is the same part as the #5358, and the Yiwei **MG0425-UB-60-4P** cable is the equivalent of the #5412 (its face reads GND, D+, D−, V+ from its N end, like the #5412). Do exactly the same steps with whichever cable you actually use.

1. Straighten the 4 legs of the #5358 piece, but don't fit it yet. Snap the cable's magnet end onto it. The magnets only latch one way.
2. Plug the cable's USB-A end into a **phone charger**. Set the meter to DC volts.
   - Black probe on one **outer** leg, red on the other outer leg. You should see about **+5 V**. The red-probe leg is **VBUS**; mark it with a pen.
   - If you read −5 V, swap the probes.
   - The two inner legs must read **0–0.6 V** to the black (GND) leg. If 5 V appears on an inner leg, **stop**: the cable is wired differently from Adafruit's drawing. Don't fit it, and ask.
3. Unplug the charger. Set the meter to continuity (beep).
   - The VBUS leg beeps to one **outer** contact of the USB-A plug (USB pin 1). The GND leg beeps to the other outer contact (USB pin 4), and usually to the metal shell.
   - The inner leg **next to VBUS** must beep to the USB-A contact next to pin 1 (USB pin 2 = **D−**).
   - The inner leg **next to GND** must beep to the contact next to pin 4 (USB pin 3 = **D+**).
   - If the inner legs are the other way round, stop and ask. It wouldn't damage anything, but USB data wouldn't work.
4. Fit the piece with the **VBUS leg in J3 pin 1** (silk "N" / "+") and the **GND leg in pin 4** (silk "−"). By Adafruit's drawing, that puts the piece's N magnet at the pin-1 end.
5. With the piece pushed in (not glued), meter on the board:
   - J3 pin 4's leg beeps to **TP4 (GND)**.
   - J3 pin 1's leg does **not** beep to TP4.
6. Only now glue the body into the top-wall slot.

*Why this matters:* if the piece went in backwards, the cable's 5 V would land on the board's GND contact. D7 clamps that, and a computer's USB port cuts the current quickly, but a phone charger can push 2 A or more through D7 and the charger chip, so damage is possible (`verification/05_stage14_recheck.md` C1). Never test it. (Stage 14 wiring puts power and ground on the outer pins, so a backwards fit can never put 5 V on a data line. The old advice "turn the magnet piece over if mirrored" is withdrawn.)

### First power-up
- USB only, with nothing else plugged in.
- Probe TP5 = 3.3 V and TP7 = EN high. TP6 = battery (0 V, or 4.1–4.25 V from the charger, with no cell).
- First flash over USB (always; the OTA partition table can't arrive over the air), then the e-paper, then the camera, then the AI settings. The full sequence is the Power-Up guide (`bringup_guide.html`); start at `ARRIVAL_CHECKLIST.md`.

### If the board ever won't take new firmware
See `firmware-prototype/README.md` §1, "Recovery" (and Power-Up guide step 7):
1. Hold TP1 (BOOT) to TP4 (GND).
2. Tap TP7 (EN) to GND.
3. Flash.

The UART backup is TP2/TP3. Never burn the security or USB-disable eFuses.
