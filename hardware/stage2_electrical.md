# Stage 2: electrical plan (pins, power, keypad)

> **⚠ History (stage 2).** The battery is now the **Adafruit #1317, 150 mAh** (26 × 19.75 × 3.8 mm, decided 2026-10-06); the #1570 / 100 mAh figures below are the original pick. Charge current stays 50 mA (R2 20 k = 0.33 C). U3 is the RT9080-33GJ5 since stage 11. Current status: `FINAL_STATUS.md`.


Stage 2 of the AI Calculator main board (ai_calc_pcb_3). This plan feeds the schematic in stage 3.
Part numbers (LCSC) are looked up and checked for stock in stage 3. None are invented here.

Sources: ESP32-S3-MINI-1 datasheet v1.7 (Table 3-1 pins, ch. 4 strapping, Table 6-7 sleep current, §9 EN RC);
Adafruit 5358 = Yiwei MG04254FRA1S1N drawing; OV5640 supplies (Linux DT binding: DOVDD 1.8 V, AVDD 2.8 V,
DVDD 1.5 V; PWDN active high, RESET active low); Seeed XIAO ESP32S3 Sense expansion board v1.0 schematic
(camera supplies and 24-pin pinout); Seeed OV5640 module (MJY5OAF-F3M-V1) drawing.

## 1. Pin map (final: `hardware/pins_final.h`)

| GPIO | Use | Notes |
|---|---|---|
| 0 | BOOT test pad (TP1) | Strapping pin (weak pull-up). Normally never needed, because USB flashing resets the chip itself. To force download mode (stage 14 correction): hold TP1 (BOOT) to TP4 (GND), tap TP7 (EN) to GND for ~0.2 s (or unplug the battery first, then plug in with TP1 held low), release TP1, flash. Plugging the cable in alone does not reset the chip while the battery is connected. Backup: UART0 on TP2/TP3 with the same TP1 + TP7 sequence. Never burn the security / USB-disable eFuses. |
| 1 / 2 | I2C SDA / SCL to the keypad scanner | Its own bus with 4.7 k pull-ups. Same pins as the prototype. |
| 3 | unused | Strapping (JTAG source). Left floating. |
| 4 | TCA8418 INT | RTC pin, open drain |
| 5 / 6 / 8 | E-paper CLK / DIN / CS | As proposed |
| 7 | **ON key** | RTC pin. Wakes the chip from deep sleep (EXT0, active low). R17 100 k pull-up. Not in the matrix. |
| 9 | Battery voltage | ADC1_CH8 (ADC1 still works with Wi-Fi on). 1 M / 1 M divider + 100 nF, so only 2 µA drain. |
| 10 | Camera XCLK | |
| 11–18, 48 | Camera D0–D7 (Y2–Y9) = 15, 17, 18, 16, 14, 12, 11, 48 | As proposed |
| 13 / 38 / 47 | Camera PCLK / VSYNC / HREF | As proposed |
| 19 / 20 | USB D− / D+ | Fixed by the chip. Wired to the magnetic connector through the USBLC6-2. |
| 21 | Camera PWDN | High = powered down |
| 26 | — | **Used by the PSRAM on -N4R2 parts. Never connect.** |
| 33 | E-paper BUSY | **Confirmed free on -N4R2.** Only octal-PSRAM parts lose IO33–37. |
| 34 | CAM_PWR_EN | Turns the camera's 2.8 V and 1.5 V regulators on. 100 k pull-down keeps them off in sleep and exam mode. |
| 35 | CHG_STAT | Charger status. Low = charging. Shown on the e-paper, so no LED is needed. |
| 36 | Camera RESET | Active low, 10 k pull-up. Needed because the camera regulators get switched off. |
| 37 | VBUS_SENSE | High when a cable is attached. Used for the exam-mode cable unlock and for the charging icon. |
| 39 / 40 | Camera SIOC / SIOD | Its own bus (see §3) |
| 41 / 42 | E-paper DC / RST | |
| 43 / 44 | UART0 TX / RX | Test pads (debug log) |
| 45 / 46 | unused | Straps. GPIO45 **must stay low** for 3.3 V flash. Left unconnected (internal pull-downs). |

Checks: no pin is used twice, and no strapping pin carries a signal. Free spares: IO3 and the two UART pads.
**Changes from the starting proposal:** none to the camera, e-paper, USB or battery pins. Added: keypad I2C
1/2 + INT 4, ON 7, CAM_PWR_EN 34, CHG_STAT 35, CAM_RESET 36, VBUS_SENSE 37.

## 2. Power

```
 magnetic connector (5358)                                     ┌─ AP2112K-3.3 ──► 3V3: ESP32, e-paper, TCA8418
  VBUS ─┬─ USBLC6-2 (VBUS) ─┬─ MCP73831 VIN                    │   (600 mA)
        │                   │      VBAT ──────────┐            │
        │                   └─ Schottky ──► SYS ──┼────────────┼─ LDO 2.8 V (EN = CAM_PWR_EN) ──► AVDD, DOVDD, AF
        └─ 100 k / 100 k ─► VBUS_SENSE (IO37)    │            └─ LDO 1.5 V (EN = CAM_PWR_EN) ──► DVDD
  D− / D+ ─ USBLC6-2 ─► IO19 / IO20               │
  GND                                             │
 battery JST-PH socket ─► P-FET (reverse) ─► VBAT_P ─┴─ P-FET (load share, gate = VBUS) ─► SYS
                                              └─ 1 M / 1 M ─► IO9
```

| Block | Part (stage 3 checks the LCSC number and stock) | Why |
|---|---|---|
| 3.3 V | **AP2112K-3.3** (as specified) | 600 mA covers Wi-Fi peaks (355 mA per datasheet) plus the e-paper. The camera does **not** run from it. |
| Charger | **MCP73831**, RPROG 20 k → **50 mA** | 0.5 C for the 100 mAh cell (gentle on a small cell in a closed case; full charge ≈ 2.5 h). Its STAT pin goes to IO35. |
| USB protection | **USBLC6-2SC6** on D+, D− and VBUS | The magnetic contacts are exposed, so they need ESD protection |
| Reverse battery | **P-MOSFET in the battery + line**, gate to GND through 100 k | A reversed plug gets blocked by the body diode, and the FET stays off. With the right polarity the FET turns fully on (low-threshold part, Vgs(th) under 1.3 V), so charging current flows back through it. |
| USB / battery switchover | Schottky VBUS → SYS + P-MOSFET VBAT_P → SYS, gate = VBUS | When plugged in, the board runs from USB and the battery charges. When unplugged, it runs from the battery with no glitch. |
| Camera supplies | **Copied from Seeed's XIAO ESP32S3 Sense camera board**: 2.8 V LDO (SGM2036-2.8 there) for AVDD, DOVDD and AF, plus a 1.3–1.5 V LDO for DVDD (the OV5640 has its own 1.5 V core regulator, so this pin is not critical). Both fed from SYS, enables on IO34. AF supply: FPC pin 24 from 2.8 V through a Schottky + 2.2 µF, pin 23 to GND, as Seeed does. RESET 10 k pull-up, PWDN 10 k pull-down. | Seeed's OV5640 module is designed for exactly this circuit, so it's proven. Feeding it from SYS keeps the camera off the 3.3 V regulator. |
| E-paper | Booster (inductor, MOSFET, 3 Schottky diodes, ~1 Ω sense resistor, 1 µF/25 V caps), same as the Waveshare HAT | Bare-panel drive, copied from the tested HAT design |
| ESP32 | 22 µF + 0.1 µF at the module, **EN = 10 k + 1 µF** (datasheet §9) | |

### Why the magnetic contacts never carry battery voltage

With the cable off, each exposed contact ties to:
- **VBUS:** the Schottky's anode (it blocks backwards), the MCP73831 VIN (it has reverse-discharge
  protection, so it never drives VIN from the battery), the USBLC6 VBUS pin (no source), and the
  load-share FET's gate. **The 100 k + 100 k divider holds this contact at 0 V.** Nothing on the board
  connects the battery to it.
- **GND:** ground.
- **D− / D+:** the ESP32's USB pins (3.3 V domain) behind the USBLC6. While the chip is awake, the USB block
  can hold D+ at 3.3 V through its internal 1.5 k pull-up. That's a logic level, not battery voltage, and a
  short is limited to about 2 mA. The firmware turns USB off when VBUS_SENSE is low, which also saves
  power, so the contacts are dead while unplugged and the chip is asleep.
- **Pin order (stage 14):** J3 = {VBUS, D−, D+, GND} with the magnet piece's N end at pin 1. The Adafruit #5412 cable fixes the contact order (its face reads GND, D+, D−, VBUS from its N end), so the old {VBUS, GND, D−, D+} matched neither way of fitting the piece and could put 5 V on D+. Power and ground are now the outer pins: a piece fitted backwards swaps VBUS/GND (D7 clamps, the host current-limits), never 5 V on a data line. Meter-check before gluing (`ORDER_WALKTHROUGH.md`).

### Battery life (120 mAh, ~100 mAh usable)

| State | Current | Per charge |
|---|---|---|
| Off (deep sleep) | ESP32 7–8 µA + AP2112K 55 µA + TCA8418 ~3 µA + divider 2 µA ≈ **70 µA** | ~2 months |
| Calculator on, idle (light sleep between keys) | ~0.3 mA (240 µA + 40 µA PSRAM per datasheet) | ~2 weeks of leaving it on (auto-off is 10 min) |
| Calculator, typing | ~20–30 mA bursts | 15–25 h of active use |
| AI solve | ~0.6–1.5 mAh each | ~55–140 solves (100 mAh cell) |

AP2112K's 55 µA is most of the off-state drain. A 1–2 µA regulator would stretch "off" to over a year,
but you specified AP2112K, so I kept it. I can switch it if you like. Low battery: below ~3.5 V the firmware
refuses AI solves, because Wi-Fi peaks would brown out a small cell. The calculator keeps working.

**Battery (my pick):** Adafruit #1570, 3.7 V 100 mAh, 31 × 11.5 × 3.8 mm, protection board built in, JST-PH 2-pin (2.0 mm)
plug with Adafruit's fixed polarity. The board's battery socket becomes a JST-PH to match (stage 3), wired the same way as
Adafruit's own boards. The reverse-polarity FET stays as a backstop.

## 3. Keypad

**Scanner:** TCA8418 (I2C 0x34, 8 rows × 10 columns, built-in debounce and a 10-event queue, INT to IO4,
RESET tied high).
**Bus choice:** a separate bus (IO1/IO2), not shared with the camera's control lines. The camera's supplies get
switched off, and an unpowered camera can drag a shared bus low, which would kill the keypad, including in
exam mode. A separate bus costs nothing, because the pins were free. The addresses (camera 0x3C, keypad 0x34)
wouldn't clash either way.

**Matrix:** rows follow the physical key rows. ROW1 combines the Abs/x³/x⁻¹/log□ row with the fraction/√ row
under it, so traces stay short. All 18 lines get used. ON sits outside the matrix on IO7.

| | COL0 | COL1 | COL2 | COL3 | COL4 | COL5 | COL6 | COL7 | COL8 | COL9 |
|---|---|---|---|---|---|---|---|---|---|---|
| **ROW0** | SHIFT | ALPHA | · | · | MODE | · | UP | DOWN | LEFT | RIGHT |
| **ROW1** | a/b | sqrt | x^2 | x^n | log | ln | Abs | x^3 | x^-1 | log_a |
| **ROW2** | (-) | o'" | hyp | sin | cos | tan | · | · | · | · |
| **ROW3** | RCL | ENG | ( | ) | S<>D | M+ | · | · | · | · |
| **ROW4** | 7 | 8 | 9 | DEL | AC | · | · | · | · | · |
| **ROW5** | 4 | 5 | 6 | x | / | · | · | · | · | · |
| **ROW6** | 1 | 2 | 3 | + | - | · | · | · | · | · |
| **ROW7** | 0 | . | x10^x | Ans | = | · | · | · | · | · |

Columns follow the keys left to right as seen from the **front**. For example, COL0 is SHIFT / a/b / (−) / RCL / 7 / 4 / 1 / 0.
The firmware's `kMatrix` in `firmware-prototype/src/keys.cpp` needs this table, plus `DKey::Abs` and `DKey::LogBase`
in place of `NCr` and `Pol` (separate firmware task). The matrix grows from 7 × 7 to 8 × 10: write `0xFF` to
KP_GPIO1 (rows) and `0xFF` / `0x03` to KP_GPIO2 / KP_GPIO3 (columns).

Exam-mode entry (SHIFT + 7 while pressing ON when off) works: ON wakes the chip, and the TCA8418 stays powered,
so the firmware can read which keys are held.

## 4. Open items for later stages

- Stage 3: LCSC parts (prefer JLCPCB basic parts; extended ones get flagged), the e-paper booster values,
  and the camera LDO part choice.
- Stage 4: footprints from the chosen parts' drawings (camera socket = the same AFC01-S24FCC-00 type Seeed uses; Seeed's module ribbon is 70.5 mm long,
  so the socket position has to suit that length).
- The magnetic connector's **cable side** needs a USB cable attached to the other half. That's a
  wiring job, so it needs soldering or a ready-made magnetic cable. Let me know which you prefer.
