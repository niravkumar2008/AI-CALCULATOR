# QA test plan: incoming inspection and end-of-line test

Written 2026-10-06 for the v14/v15 board (`hardware/FINAL_STATUS.md`), the board firmware's self-test (`firmware-prototype/`, version stage14-2026.10.06: `src/selftest.cpp`, fields listed in `hardware/bringup_guide.html` step 8) and the meter checks in `hardware/ORDER_WALKTHROUGH.md` §9. Pass/fail numbers below are the firmware's own thresholds where it has them; everything else is marked **(limit set here)** and can be tightened once the first 10 units give real numbers.

Record every unit on one line of `hardware/production/qa_log.csv` (create it on day one): `serial, date, board batch, A-result, B-result, selftest JSON, notes`. The self-test JSON **is** the test record; paste it whole.

---

## Part A: incoming inspection of the JLCPCB boards (before any power)

Do this to every board, assembled or bare, on the day the box arrives. 5 min per board. Tools: 10× loupe (or phone macro), multimeter, anti-static mat, the JLCPCB placement photos they email you.

### A1. Visual, no tools
| # | Look at | Pass |
|---|---|---|
| 1 | Silk reads **"AI CALC v14 2026-10-04"** (or the v15 string) | Matches the version you ordered |
| 2 | Board outline and the slot for the e-paper ribbon (1.0 × 14 mm near x 172.9) | Slot is open and clean, no fibre fuzz |
| 3 | Locating slots H2/H9/H13/H14 and hole H6, screw notches at the bottom (38.94 c-c) | Open, no copper burr. **Keep a bare board as the gauge** for the paper dry fit |
| 4 | ENIG finish on the 50 key pads (B side) | Even gold, no scratches or dull patches; a dull pad = oxidised nickel, reject |
| 5 | Edge of the ESP32 antenna overhang (left edge) | Not chipped; the antenna keep-out has no copper |

### A2. Under the loupe (assembled boards)
Compare each item with the preview table in `ORDER_WALKTHROUGH.md` step 5.
| # | Part | Check | Pass |
|---|---|---|---|
| 6 | U1 ESP32-S3-MINI-1 | Pin-1 dot at the silk mark, all castellations wetted, no bridge between neighbours | Every castellation shows a fillet |
| 7 | U6 TCA8418 (QFN-24) | Orientation dot matches silk; look along each side for bridges; exposed pad not floating (board flat) | No bridge, package flat |
| 8 | U2, U3, U4, U5, U7 (SOT-23-5/6) and Q1–Q3 | Orientation per the walkthrough table (these are the parts the CPL rotates: Q1 180, Q2 270, U2/U3/U7 270, U4/U5 180) | Pin 1 where the table says |
| 9 | J1 (camera FPC, `CamReversed`) and J2 (e-paper FPC) | Latch closed and intact, 24 pins all soldered, no solder in the slot | Latch flips freely once |
| 10 | J3 magnet header (C46061768, right angle) | Opening faces the top edge; silk "N"/"+" beside pin 1, "−" beside pin 4 | 4 pins wetted |
| 11 | J4 JST-PH | Mouth faces the keypad (down); "+" silk visible | Both pins and the two anchors wetted |
| 12 | D1–D7 (SOD-123 / SMF) | Cathode band at the silk bar | All 7 bands agree with silk |
| 13 | C1, C32, C33, C34 (22 µF 0805), C20 (1206) | Present, no cracks, no tombstones | — |
| 14 | L1 (68 µH 4020) | Present, flat, both ends wetted | — |
| 15 | Whole board | Tombstones, solder balls, missing 0402s (count against the BOM: 77 designators), flux pools near J1/J2 | None |

### A3. Meter before first power (board alone, nothing plugged in)
Resistance mode first, then diode mode. Probe the test pads (TP1 BOOT, TP2 UART TX, TP3 UART RX, TP4 GND, TP5 +3V3, TP6 BAT+, TP7 EN).
| # | Between | Pass | Why |
|---|---|---|---|
| 16 | TP5 (+3V3) to TP4 (GND), resistance | **> 1 kΩ** after the meter settles (the 22 µF caps charge slowly; wait 5 s). A reading **< 100 Ω = short, reject** | A dead short here is a bridged U3 or a cracked 22 µF |
| 17 | TP6 (BAT+) to TP4, resistance | **> 10 kΩ**. Not a dead short | The 1 M/1 M sense divider plus Q1 off |
| 18 | J3 pin 1 (VBUS) to TP4 | **> 10 kΩ**; diode mode red-on-GND shows D7's clamp ≈ 0.6–0.8 V one way | TVS and charger input |
| 19 | J3 pin 4 to TP4 | **Beeps** (continuity) | Pin 4 is GND |
| 20 | J3 pin 1 to TP4 | **No beep** | Pin 1 is VBUS |
| 21 | J4 "+" pin to TP6 | Diode mode: conducts through Q1's body diode one way (≈ 0.6 V), open the other way | Reverse-battery MOSFET present |
| 22 | TP7 (EN) to TP4 | > 10 kΩ | Pull-up only |
| 23 | TP1 (BOOT) to TP4 | > 10 kΩ | Pull-up only |
| 24 | J1 fingers 2 and 15 to TP4 | **Beep** on both | Camera GND pins (`ORDER_WALKTHROUGH` §9) |
| 25 | I2C SDA/SCL (R13/R14 top ends) to TP5 | ≈ 4.7 kΩ | Pull-ups fitted |

Any fail in 16–21: set the board aside, photograph it, and claim it with JLCPCB within their window. Do not power it.

### A4. First power (USB only, no battery, no panels)
Bench supply or a phone charger through the #5412 cable on a **temporarily pushed-in** magnet piece (not glued; polarity-metered first, §B6 below). **Day one / first boards: a computer USB port, not a charger** (Power-Up guide rule 3: it cuts the current quickly on a fault).
| # | Measure | Pass |
|---|---|---|
| 26 | Current from the 5 V source | **5–60 mA** idle **(limit set here)**; **> 150 mA with nothing plugged in = stop** |
| 27 | TP5 | **3.25–3.35 V** |
| 28 | TP7 (EN) | **> 2.5 V** |
| 29 | TP6 with no cell | ≈ 0 V, or 4.1–4.25 V (the MCP73831 regulating into nothing); both are fine |
| 30 | Computer sees a USB device (ESP32-S3 USB-Serial-JTAG) | COM port appears within 5 s |
| 31 | Touch U3, U4, U5, U2 after 60 s | Warm at most; **too hot to hold 3 s = reject** |

Then flash the production firmware **over USB** (always the first flash: the two-slot OTA partition table can only be written over USB; `firmware_slot` then reads `app0`), run `status` and `selftest` over USB with the e-paper and camera attached, and save the JSON. Put in the hotspot, proxy address and this unit's device token (`server/proxy`: `python admin.py new-device "<serial>"`; Power-Up guide step 10). That board is now "bring-up pass" and goes to assembly.

---

## Part B: end-of-line test, every finished unit

Done on the closed calculator (shell on, battery in, magnet piece glued). 6–8 minutes a unit including the key sweep. Needed on the bench: the #5412 cable on a USB power meter (any $10 inline USB meter), a phone hotspot or the bench Wi-Fi visible, the **focus target card** (§B4), a loupe, a 0.5 mm pencil, and the QA log.

Order of operations: B1 → B2 (self-test) → B3 → B4 → B5 → B6 → label.

### B1. Look and feel (1 min)
| # | Check | Pass |
|---|---|---|
| 1 | Shell closed: all screws in, no gap at the parting line | Gap **≤ 0.3 mm** anywhere (feeler gauge or a sheet of paper doesn't slide in) |
| 2 | Camera window (7 mm) | Lens centred in the hole by eye (no ring of plastic visible on one side only), no burrs, window edge **not** touching the lens barrel |
| 3 | Magnet piece | Flush with the top wall ± 0.3 mm; the cable snaps on in the right orientation and **won't seat upside down** |
| 4 | Keys | Every key springs back; no key sits lower than its neighbours; SHIFT and ALPHA don't rub their holes |
| 5 | Screen window | E-paper square to the window, no dust under the lens, no pressure marks |
| 6 | Shake it | Nothing rattles (a loose battery or a stray screw fails) |
| 7 | Branding | **No Casio name or logo anywhere** (see `KNOCKOFF_SHELL_PLAN.md` §5); our label present |

### B2. Factory self-test (3–4 min)
With the calculator off: **hold SHIFT and ALPHA, press ON.** (Or type `selftest` on the serial monitor if the cable is on.) The screen shows "SELF-TEST" and walks through 7 steps; it prints one `SELFTEST_JSON` line on the serial port. Pass/fail is the firmware's own:

| Step | Field | Pass (firmware threshold) | What a fail usually means |
|---|---|---|---|
| 1 chip + memory | `memory_ok` | true (4 MB flash, 2 MB PSRAM) | Wrong module variant or a bad U1 reflow |
| 2 keypad chip | `keypad_scanner_ok` | true (TCA8418 answers at 0x34) | U6 bridge, I2C pull-ups missing |
| 3 battery | `battery_mv` | **3000–4300 mV** → `battery_ok` true. For shipping, additionally **3700–4100 mV** (limit set here: ship at 40–80 %, not full) | < 3000: cell unplugged or reversed at J4. ≈ 4200 with no cable: fine. 0 with a cable: Q1 or J4 |
| 3 charging | `vbus`, `charge` | With the cable on: `vbus` true and `charge` = "charging" or "full". Without: `vbus` false, "no_cable" | `vbus` false with the cable on = magnet piece polarity or a bad J3 joint |
| 4 e-paper | `display_refresh_ms`, `display_ok` | **300 < ms < 9000**, `display_ok` true. Typical **1800–2500 ms** | < 300: BUSY never asserted (J2 not latched); > 9000: booster (L1/C20/D2–D5) |
| 4 e-paper, by eye | the pattern | Checkerboard squares crisp, border unbroken on all 4 sides, **no missing row or column**, no grey half-tone bands | A missing line = cracked panel or one J2 pin dry |
| 5 camera | `camera`, `camera_jpeg_bytes`, `camera_ok`, `camera_autofocus` | contains "OV5640", bytes **> 1 024** (typical 20–40 KB at VGA), `camera_ok` true, **`camera_autofocus` true** (limit set here: a fixed-focus module is a wrong part) | Sensor ID missing = ribbon in backwards (J1 is `CamReversed`) or fingers 2/15 open |
| 6 Wi-Fi | `wifi_networks`, `wifi_best_rssi`, `wifi_ok` | networks **≥ 1**, best RSSI **> −80 dBm**; with the bench AP 2 m away expect **−35 to −55 dBm** (limit set here: **> −60** on the bench, else the antenna overhang is touching the wall or something metallic sits over it) | |
| 6 Wi-Fi sag | `battery_mv` − `battery_mv_radio_on` | **≤ 250 mV** with the 150 mAh #1317 (limit set here; the brown-out study in `battery_upgrade.md` §5 expects 200–280 mV for a 100 mAh cell and less for 150). **> 350 mV = weak cell or bad J4 crimp, reject** | |
| 7 keys | `keys_ok`, `keys_missing`, `keys_all_ok` | **50 / 50**, `keys_missing` empty. Press every key once, in reading order, with a normal thumb press (not a fingernail). 90 s limit | A named missing key = key mat dirty/dome torn, or a scratched ENIG pad; **two keys in one row or column** = a TCA8418 pin |
| Overall | `pass` | **true**, screen says **SELF-TEST PASS** | Any false: fix, re-run, log both runs |

Record `device_id`, `firmware` and `firmware_slot` from the JSON on the label.

### B3. Charge check (2 min, can overlap with B4/B5)
Cable on, through the inline USB meter.
| # | Measure | Pass |
|---|---|---|
| 1 | Current at 5 V, battery between 3.6 and 4.0 V, calculator idle | **40–60 mA** with R2 = 20 k (50 mA set point) **(v15 with R2 = 4.7 k and the big cell: 190–230 mA)**. **< 20 mA = STAT stuck / cell full / bad J4; > 80 mA (v14) = wrong R2 or a short** |
| 2 | `status` on the serial monitor (or the battery icon) | "charging", `vbus` true |
| 3 | Pull the cable | `vbus` false within 1 s; calculator keeps running on the cell |
| 4 | Magnet pull-off | Cable pulls off sideways without lifting the piece; the piece doesn't move (glue) |
| 5 | Heat | Magnet piece and the shell over U2 at most warm after 2 min |

### B4. Camera focus target card
Make one card and keep it at the bench (print at 100 %): a white A6 card with
- a **12-pt** line of printed text: `3x^2 + 5x - 2 = 0  solve for x` (what a real homework line looks like),
- an **8-pt** line below it: `the quick brown fox 0123456789`,
- a 1-mm-pitch ruled block (10 lines) in the corner, and
- a thick 5 mm black cross at the centre.

Test: open **MODE 4 (AI SOLVE)**, hold the calculator so the card fills the brackets at the usual reading distance (**15 cm**, use a 15 cm block or a stacked book as the spacer), wait for the focus bar, press **=**.
| # | Pass |
|---|---|
| 1 | The viewfinder shows the cross centred within the inner bracket (lens not pushed off-axis by the window) |
| 2 | The focus bar reaches the "focused" mark within **3 s** of holding still |
| 3 | On the serial monitor, `focus` / scan quality score **≥ the kSharpMin you set after the first 10 units** (limit set here: record the score for the first 10 good units and set the floor at 70 % of their median). Until then: the **8-pt line is readable in the saved JPEG** (look at it on the laptop camera page: `preview` in the Serial Monitor; `snap` only logs the size) |
| 4 | Repeat at **25 cm**: the 12-pt line still readable |
| 5 | No bright crescent or haze on one side of the frame (light leaking through the window gap; add the window mask, `final_assembly/window_mask.md`) |

Then, with the card, do one real solve over Wi-Fi: the answer screen shows **x = 1/3, x = −2** (or equivalent) and "✓ calculator agrees". That proves the device token, the proxy and the pairing path work. Log the solve time: **< 25 s** (limit set here).

### B5. E-paper, beyond the self-test pattern
| # | Check | Pass |
|---|---|---|
| 1 | Normal calculator screen after the self-test restarts | Text sharp, full black, white background with no grey |
| 2 | Partial refresh: type `12345` then AC, three times | Digits appear in **≤ 0.5 s** each; no ghost digits left after AC |
| 3 | Full refresh (the viewfinder does one every 10th frame) | Screen flashes black/white and comes back clean |
| 4 | Tilt under the bench light | No pressure marks, no bright spot (window lens pressing on the glass) |

### B6. Magnet cable polarity meter check
This is **mandatory before the piece is glued** (assembly), and repeated **on the finished unit** as a 20 s confirmation. The full procedure is `ORDER_WALKTHROUGH.md` §9 "Magnet piece"; the numbers:

Before gluing (cable on a phone charger, piece snapped to the cable, legs not yet in J3):
| # | Measure | Pass |
|---|---|---|
| 1 | DC volts, outer leg to outer leg | **+5.0 ± 0.25 V** with the red probe on the leg you will put in J3 pin 1 (mark it) |
| 2 | Each inner leg to the GND leg | **0–0.6 V**; **5 V on an inner leg = stop, wrong cable** |
| 3 | Continuity, charger unplugged: VBUS leg → USB-A pin 1; GND leg → USB-A pin 4 and the shell; inner leg next to VBUS → USB pin 2 (D−); inner leg next to GND → USB pin 3 (D+) | All four beep as listed |
| 4 | Piece pushed into J3: pin 4 leg to TP4 | Beeps |
| 5 | Pin 1 leg to TP4 | Does **not** beep |

On the finished unit (shell closed): snap the cable on, charger plugged in, meter on the two **outer** contacts of the cable's own magnet face seen from the calculator side: the unit must show `vbus` true in `status`, and the inline USB meter must read **4.75–5.25 V**, **40–60 mA** (B3). If the cable only seats one way and B3 passes, polarity is proven; a reversed piece would read ≈ 0 mA and `vbus` false (D7 clamps, the charger current-limits).

### B7. Label and pack
- Serial label inside the battery-lid recess: `serial / device_id / firmware / QA date / initials`.
- Battery at **40–80 %** for shipping (B2 step 3).
- In the box: calculator, #5412 cable (or OEM cable from the same supplier as the piece), quick-start card with "not affiliated with Casio" and, once you have it, the FCC SDoC line.
- Run `pair` once to confirm the pairing code prints, but **don't link it** (there is no unlink command yet): the buyer links it at `<proxy>/link` and starts their own free month. Note the unit's device id (`python admin.py devices`) next to its serial.

---

## Part C: sample sizes and when to stop the line
| Batch | Part A (boards) | Part B (units) | Stop the line if |
|---|---|---|---|
| 2–10 | every board | every unit | any single fail in A3 16–21 or B6 |
| 100 | every board (A1–A3 take 5 min); A4 on every assembled board | every unit | 3 units in a row fail the same step, or **> 5 %** fail B2 on first run |
| 1,000 | A1–A3 on **every** board (it's still only 5 min and it catches the expensive faults); AOI from JLCPCB on file | every unit for B2, B3, B6; B4 on **1 in 5** after the first 100 pass cleanly | first-pass yield **< 95 %** |

A fail that's fixed and re-passed counts as a pass in the log but keep the first JSON: those are the data for the next board revision.
