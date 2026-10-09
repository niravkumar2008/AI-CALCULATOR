# JLCPCB order walkthrough: v15-LCD board (silk "AI CALC v15-LCD 2026-10-08")

The v15 version of `ORDER_WALKTHROUGH.md` (which stays as the v14 record). The board was verified **"ORDER"** by the final pre-order review `verification/14_v15_final_preorder.md` (ERC 0, DRC 0/0/0/0, copper-to-hole ≥ 0.25 mm, CPL 20/20 against freshly fetched EasyEDA footprints, every part in JLCPCB stock on 2026-10-08 21:30). Overall status: `FINAL_STATUS.md`. v14 stays frozen (git tag `v14-order`, `kicad/`, `fab/`): **don't upload anything from `fab/`.**

The settings below are the ones Nirav used for the v14 order on 10/8. Go slowly and check each step.

## 0. Before you start

- **No paper dry fit and no panel test are needed first.** The outline and all holes are identical to v14 (review 14 §2 check 6), and the bench checks for the panel (section 9) can all be fixed on this board if they come out differently.
- **Panels:** order **3 × BuyDisplay ER-TFT019-1, the version without touch** (~$6–7 each; $6.22 at 10), the same day. Do **not** add BuyDisplay's ER-CON30HT-1 socket: J5 is already on the board. (Adafruit #5394 is only an optional fallback.) Shopping list: `Claude outputs/AI_Calculator_Shopping_List.xlsx`, sheet "v15-LCD prototype".
- **Have these open** (all regenerated **2026-10-08 21:38** from the final board):
  - Upload files in **`hardware/fab_v15_lcd/`**: `ai_calc_v15_lcd_gerbers_JLCPCB.zip`, `ai_calc_v15_lcd_BOM_JLCPCB.csv`, `ai_calc_v15_lcd_CPL_JLCPCB.csv`.
  - To compare the preview against: `fab_v15_lcd/render_top.png` and `fab_v15_lcd/ai_calc_v15_lcd_assembly_top.pdf`.
- **The CPL is already corrected.** `tools/v15_lcd/make_outputs_v15.sh` writes it through `tools/jlc_cpl.py`, which adds JLCPCB's rotation offsets. Every polarised or multi-pin part (20 of them) was placed with JLCPCB's own EasyEDA footprint at the CPL angle and compared pad by pad: 20 PASS (`fab_v15_lcd/cpl_easyeda_check_v15.md`). **Upload the CPL as it is. Don't rotate anything in the preview; only verify it against the table in step 5.**

## 1. Upload the gerbers

- jlcpcb.com → **Order now** → **Add gerber file** → `fab_v15_lcd/ai_calc_v15_lcd_gerbers_JLCPCB.zip`.
- The viewer should show 2 layers and about **71.7 × 148.9 mm** (same outline as v14), with a thin **1.0 × 20 mm slot near the right edge** beside the screen area (x 180.5–181.5). The old e-paper slot further in is gone.

## 2. PCB options

| Option | Setting |
|---|---|
| Base material | **FR-4** |
| Layers | **2** |
| Dimensions | auto |
| PCB qty | **5** |
| Product type | Industrial/Consumer |
| Different design | 1 |
| Delivery format | Single PCB |
| PCB thickness | **0.8 mm** (needed for the keypad stack) |
| PCB colour | Green |
| Silkscreen | White |
| Surface finish | **ENIG, 1U"** (gold key pads; HASL is too bumpy for the rubber keys) |
| Outer copper weight | 1 oz |
| Via covering | **Tented** |
| Min via hole size / diameter | **0.3 mm** (the board's vias are 0.3 mm) |
| Mark on PCB | **Remove mark** |
| Confirm production file | **Yes** |
| Everything else | default |

The oval NPTH holes and the 1.0 mm slot are normal. There's no castellation and no impedance control.

## 3. PCB Assembly

| Option | Setting |
|---|---|
| PCBA type | **Standard** (needed for the ESP32 module and the QFN; it includes X-ray) |
| Assembly side | **Top side** |
| PCBA qty | **5** (as for v14; 2 is the minimum and saves roughly $40–55) |
| Edge rails / fiducials | **Added by JLCPCB**, on the **right and bottom edges only** (say so in the remark, step 6) |
| Depanel boards before delivery | **Yes** (rails removed) |
| Confirm parts placement | **Yes** |
| Photo confirmation | **Yes** |
| Review before payment | **Yes** (pay only after JLCPCB's engineers have reviewed the files) |

## 4. BOM and CPL

- Upload `fab_v15_lcd/ai_calc_v15_lcd_BOM_JLCPCB.csv` and `fab_v15_lcd/ai_calc_v15_lcd_CPL_JLCPCB.csv`.
- **32 BOM lines, 65 placed parts**, all matched, no shortfall. Every designator is listed separately, so the BOM matches the 65 CPL rows.
  - **13 extended part types** (v14 had 14): U1 C3013941, U2 C424093, U3 C841192, U4 C53099, U5 C53100, U6 C138713, U7 C7519, D7 C193402, J1 C6364666, J3 C46061768, J4 C295747, **J5 C2919501**, **R23 C22810** (15 Ω, new in review 14). L1, Q3, R12 and the second C6364666 socket (J2) are gone.
  - Everything else is Basic (Q4 is the same AO3401A C15127 as Q1/Q2; Q5 AO3400A C20917 is Basic).
- Stock on 2026-10-08 21:30 (review 14): **U1 ESP32-S3-MINI-1-N4R2 C3013941: 1,218** (the tightest), J5 C2919501: 2,242, R23 C22810: 314,584. **If U1 is out of stock, accept no substitute** (an N8R8 loses IO33–37, which the board uses): order the bare PCBs and ask for a pre-order of C3013941.
- **Ignore the "not in BOM" note** for TP1–TP7, FID1–3, H* and SW1–SW50. They are bare copper, not parts.
- The LCD panel is **not** on the BOM: it plugs into J5 by hand.
- If any line says "consign" or "pre-order", stop and ask.

## 5. Placement preview: what each part must look like

"Top-left" etc. is as seen in `fab_v15_lcd/render_top.png` (component side, board top edge at the top of the picture). Coordinates are KiCad (x, y) in mm.

| Part (KiCad x, y) | Must look like |
|---|---|
| **J5** LCD socket (158.6, 93.1) | Opening facing **right**, towards the slot (x 180.5–181.5); the row of 30 pads on the **left**. **J5's pin numbering is intentionally reversed, like J1's.** JLCPCB's pin-1 mark will be at the **top** end; our pad 1 and the silk tick are at the **bottom** (157.4, 100.35). That is correct. **Don't rotate it.** Align by the pads and the two mounting tabs. |
| **Q4** LCD power switch, AO3401A (159.5, 103.9), just below J5 | The single pin 3 at the **top**, pins 1/2 at the bottom (same as Q2) |
| **Q5** backlight switch, AO3400A (157.3, 82.3), just above J5 | The single pin 3 at the **bottom**, pins 1/2 at the top |
| **D7** TVS (119.2, 73.4) | Band on the **right** pad, towards J3 (KiCad x 120.65, the VBUS pad). **This one matters most:** backwards, it shorts the charging cable. |
| **U1** ESP32 (127.55, 97.0) | The antenna end (no pads) hanging off the **left** board edge. If the model sits sideways from the pads, ask them to align it to the pads. |
| **U6** TCA8418 (126.0, 146.5) | Pin-1 dot **top-left** |
| **J1** camera socket (150, 161) | Opening facing **up**, towards the camera area. JLCPCB's pin-1 mark will be at the **right** end: that is correct. **J1's pin numbering is intentionally reversed. Don't rotate it.** Align by the pads and the two mounting tabs. |
| **J3** magnet header (127.5, 72.0) | The 4 socket openings face the **top edge**; the body hangs about 0.4–1 mm past the edge. The 4 contacts are identical, so "pin 1" doesn't matter here. Only the direction does: body towards the edge, never over D1/D6/U7. |
| **J4** battery socket (142.6, 74.6) | Opening facing **down**; the two small pins at the top, the big tabs at the bottom |
| U2, U3, U4, U5 (SOT-23-5) | Pin-1 dot **top-left** (3-pin side on the left), as v14 |
| U7 (SOT-23-6, 133.6, 83.2) | Pin-1 dot **top-left** |
| Q1 (136.1, 79.5) | Pins 1–2 on the **left**, the single pin 3 on the **right** |
| Q2 (135.9, 75.6) | The single pin 3 at the **top**, pins 1/2 at the bottom |
| D1, D2, D6 | Band on the **left** pad |
| R23 (161.0, 80.6), 15 Ω 0603 | Not polarised; just check it sits on its two pads |

**Don't rotate anything.** If a part seems to disagree, stop and ask Claude (or JLCPCB's engineer) before clicking anything: it's more likely a reading mistake than a wrong file.

## 6. Order notes (paste into the "remark" box)

> J1 and J5 pin numbering is intentionally reversed vs. the library footprints: please align J1 and J5 to their pads and mounting tabs, do not rotate them. U1 antenna and J3 body overhang the board edges on purpose (left and top edges). Edge rails: please add them on the right and bottom edges only, and remove them before delivery (depanel). Please X-ray U1 and U6. Please do not substitute U1 (ESP32-S3-MINI-1-N4R2, C3013941). OK to pull back copper locally if any copper is too close to a hole or the slot.

## 7. If JLCPCB's engineers email you

Answer within a day, or the order waits.

| They ask about | Answer |
|---|---|
| "Component exceeds the board edge" (U1, J3) | **Intended, please keep.** The U1 antenna overhang is Espressif's recommended placement. J3 overhangs for the magnet connector. |
| Edge rails / conveyor clearance | Rails on the **right and bottom edges only**, removed before delivery. Keep the left edge (antenna) and the top edge (J3) free. A small rail fee is fine. |
| Copper too close to a hole / NPTH / slot / board edge | **OK to pull back copper locally.** Everything is already ≥ 0.25 mm from every hole and ≥ 0.30 mm from the slot. |
| J1 or J5 pin 1 doesn't match the library | **Intentionally reversed** (the ribbons arrive mirrored). Align by pads and mounting tabs; don't rotate. |
| D7 polarity | Cathode band on the pad towards J3 (KiCad x 120.65, the VBUS pad). |
| Pin-1 silk marks too thin to print | Fine. Use the pads / the assembly drawing. |
| Oval holes / the slot | Intended non-plated slots; the 1.0 × 20 mm slot is a routed cut-out for the LCD ribbon. |
| Drill-map files are "unknown layers" | Ignore them, or delete the two `*-drl_map.gbr` files from the zip and re-upload. |
| A part is out of stock / needs a substitute | Stop and check with Claude before accepting a substitute. **Never** for U1. R23 may only be another **15 Ω** 0603 (not 10, 12, 20 or 22 Ω). |

## 8. Expected cost (2026-10 prices; check the final quote)

v14 (5 PCBs, 5 assembled) came to about **$275 delivered**. v15 has **one fewer extended part type** (13 × $3 instead of 14 × $3; R23 C22810 is new, but L1, Q3 and R12 left) and a board BOM about **$0.19 cheaper** per board, so expect about the same:

| Item (5 PCBs, 5 assembled) | Estimate |
|---|---|
| 5 PCBs, 2-layer, 71.7 × 148.9, 0.8 mm, ENIG, remove mark | $25–40 |
| PCBA setup (Standard) + stencil | about $33 |
| Extended part fees (13 × $3) | $39 |
| Parts (5 boards; ESP32 module ≈ $5 each) | $70–75 |
| Solder joints, edge rails, depanel, misc | about $5–12 |
| **Subtotal** | **about $172–199** |
| Shipping (DHL/FedEx express) | $20–35 |
| US import duty, collected at checkout (about 35–37.5 %) | about $60–65 |
| **Total delivered** | **about $255–285 (≈ $270)** |

With only 2 assembled the total drops to about $195–235. Timing: about **10–14 days door-to-door**. If the quote is far above this (for example over $230 before shipping), something is mis-ticked: look for a consigned part, "Economic" instead of "Standard", or a PCB qty other than 5. The cost model uses $270 (`production/COST_MODEL.xlsx`, Inputs "v15-LCD JLCPCB order"); replace it with the real total.

## 9. After the boards and panels arrive (bench checks)

None of these blocked the order; all are recoverable on this board (review 14 §6). Step by step: the **v15 Power-Up guide** `bringup_guide_v15_lcd.html` (https://claude.ai/artifact/BeQnacuLhchaHz492CnLRT) and the **v15 Assembly guide** (https://claude.ai/artifact/FU8YVqaksDPk2cfiD2cz3f); checklist: `ARRIVAL_CHECKLIST.md`, v15 section.

- **Magnet piece, battery polarity, camera ribbon:** exactly as v14 (`ORDER_WALKTHROUGH.md` step 9; J3 is 1 VBUS, 2 D−, 3 D+, 4 GND).
- **Pin-1 end of the panel tail: diode test on the first ER-TFT019-1, before it is plugged in.** The LED sits between finger 20 (anode) and finger 22 (cathode), i.e. the 11th and 9th fingers from the pin-30 end. Mark the finger-1 corner. Plugged in with no twist, finger 1 must land at the **J5 silk tick (bottom)**, the LED fingers in the top half. If not: nothing is damaged; a half twist of the tail fixes it.
- **Backlight current:** volts across R23 ÷ 15 at 100 %. Expect **16–37 mA (≈ 26 mA typical)**; it cannot exceed the 60 mA rating.
- **Panel at 3.3 V:** picture quality (3.3 V is the top of its 2.4–3.3 V range); IDD ≤ 20 mA.
- **Colours:** `LCD_INVERT` / `LCD_RGB_ORDER` build flags (`firmware-v15-lcd/PORT_NOTES.md`).
- **Tail length 36.6 ± 0.3 mm** and the bow's clearance to rib B (assembly guide, marker dot).
- **Feel of the three moved keys** SHIFT (SW1), ALPHA (SW2) and ON (SW50), now centred on the Casio contacts.
- **Recovery** if a board won't take firmware: as v14 (TP1 to TP4, tap TP7, flash). Never burn an eFuse. Flash only `firmware-v15-lcd/` to a v15 board.
