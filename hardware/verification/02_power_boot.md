# Verification 02: power, boot, recovery, protection, analog (stage 13b)

> **⚠ Superseded by stage 14: see `../FINAL_STATUS.md` and `05_stage14_recheck.md`.** This checked the stage-13b board. Now fixed: recovery = hold TP1 (BOOT) to TP4 (GND), tap TP7 (EN); J3 = 1 VBUS, 2 D−, 3 D+, 4 GND (the "J3: pin 1 VBUS, 2 GND, 3 USB_DM, 4 USB_DP" below is the old order). R17 is 100 k on the board, not 10 k as C12 says. The battery is now the #1317 150 mAh (C1's 100 mAh #1570 numbers are the worst case).

Date: 2026-10-04. This was a read-only check. No design file was changed.

**Netlist source:** `kicad-cli sch export netlist` run on a scratch copy of `hardware/kicad/*.kicad_sch` (KiCad 10). The pin-to-net tables below come from that export. The PCB was only queried for footprint positions; it was never read whole.

**Datasheets used:**
- Richtek DS9080-09 (RT9080, read from the PDF text)
- ESP32-S3-MINI-1 v1.7 and ESP32-S3 datasheets
- MCP73831
- ME6211
- Vishay Si1308EDL
- LCSC part pages for every capacitor and L1
- Adafruit pages for #1570 (battery) and #5358 (magnet connector)

## Verdict

**No FATAL item. The board can be ordered as is.**

- Every power, boot and protection path checks out against the datasheets and the netlist.
- **Recovery:** the board can always be put back into download mode with the case open, over two independent paths:
  - native USB, using TP1 BOOT + TP7 EN + TP4 GND;
  - UART0, using TP2/TP3 and a 3.3 V USB-serial adapter.
- **One LIKELY PROBLEM, and it isn't on the board:** the documented recovery procedure ("short BOOT to GND while you plug the cable in") **does not work while the battery is connected**. Plugging in USB never resets the chip, so the procedure has to change (L1 below).
- Everything else is CHECK (measure on the first board, or a firmware rule) or OK.

## Findings, by severity

### FATAL

None.

### LIKELY PROBLEM

**L1. The documented "force download mode" procedure fails whenever the battery is plugged in.** Procedure fix, no board change.

- **Evidence:**
  - U3 (RT9080) EN = SYS, and SYS is fed from the battery through Q2. So +3V3 and EN stay up from the battery all the time.
  - Plugging the magnet cable in only adds D1 → SYS. It never makes EN go low.
  - GPIO0 is only sampled at reset. Holding TP1 low while plugging in therefore does nothing; the app just keeps running.
- **Where the wrong procedure is written:**
  - `firmware/FIRMWARE_STAGE13.md` step 3
  - `pins_final.h` line 75
  - `stage2_electrical.md` GPIO0 row
- **Fix (text only):**
  1. Back cover off.
  2. Hold a wire from **TP1 (BOOT) to TP4 (GND)**.
  3. Tap a second wire from **TP7 (EN) to TP4 (GND)** for about 0.2 s. That resets the chip with BOOT low, so the ROM USB downloader enumerates on the magnet USB.
  4. Release TP1.
  5. Flash, then tap TP7 to GND again to run the new firmware.
  - Alternative: unplug the battery (J4), hold TP1 to GND, then plug in the cable.
  - Last resort: if native USB is ever unusable, the ROM also listens on UART0. Connect a 3.3 V USB-UART adapter to TP2 (TX) / TP3 (RX) / TP4 (GND), then do the same TP1 + TP7 sequence.
- **Board change:** no. TP1, TP4 and TP7 already exist on the F side, labelled, and reachable with the back cover off (stage12_measurements.md).
  - Keep the battery and any tape or foam **off TP1/TP4/TP7** in the enclosure.
  - Never burn the eFuses `DIS_USB_SERIAL_JTAG`, `DIS_DOWNLOAD_MODE` or `ENABLE_SECURITY_DOWNLOAD`, and don't enable secure boot or flash encryption on this board. Those are the only things that could make download mode unreachable.

### CHECK (measure on board 1, a firmware rule, or a low-probability risk)

**C1. Brown-out margin on the 100 mAh cell during Wi-Fi.** Firmware-mitigated.

- **Battery:** Adafruit #1570 is 100 mAh, with a protection circuit that cuts out at 3.0 V. Adafruit gives no discharge C-rate.
- **Supply path:**
  - The supply path is cell DCIR + PCM + Q1 + Q2 (AO3401A, ≤ 80 mΩ each at Vgs = −2.5 V), about 0.6–1.0 Ω in total.
  - RT9080 3.3 V dropout at 600 mA is **0.31 V typ / 0.53 V max** (DS9080-09 table). It scales roughly with load.
- **With TX capped at 11 dBm** (`kWifiMaxTxQuarterDbm = 44`, already in `power.h`) and the camera off:
  - Peak ≈ 220 mA.
  - At 3.6 V open-circuit: SYS ≈ 3.38–3.47 V, so +3V3 ≈ 3.19–3.36 V. That's above the ESP32's 3.0 V minimum. **OK.**
- **Uncapped bursts** (PHY calibration at Wi-Fi start, 802.11b at 21 dBm ≈ 340–355 mA) or the camera on at the same time (+~150 mA from SYS):
  - Total ≈ 0.5 A, which is 5 C.
  - Sag 0.3–0.5 V, so +3V3 can drop to about 2.8–3.0 V near 3.6 V open-circuit.
- **Firmware rules:**
  - Keep the 3.6 V AI lock-out.
  - Never run the camera and Wi-Fi at the same time. Call `cameraSleep()` before `WiFi.begin()`.
  - Set the TX cap before the first `WiFi.begin()`.
- **Board 1 test:** run an AI solve at about 3.65 V (bench supply on TP6 through a 0.5 Ω resistor to mimic the cell) and watch TP5 with a scope.
- **Board change:** no.

**C2. Hot-plug overshoot on the magnet VBUS vs. absolute-maximum ratings.**

- **Ratings involved:**
  - MCP73831 VDD: 7.0 V absolute max.
  - RT9080 VIN: **6.5 V** absolute max (DS9080-09). RT9080 sits on SYS = VBUS − D1 Vf.
  - ME6211 VIN: 6.5 V absolute max.
  - D7 SMF5.0A: VBR 6.4–7.07 V, VC 9.2 V at 21.7 A.
- **Risk:**
  - A ceramic-only VBUS (C5 4.7 µF / 16 V) on an inductive cable can ring towards 2 × VBUS.
  - The TVS starts conducting at about 6.4 V and holds low-energy plug-in ringing at roughly 6.5–7 V.
  - After D1's drop, SYS sees about 6.1–6.6 V for microseconds. That's at RT9080's absolute max.
  - Feather-class boards (MCP73831 + 3.3 V LDO on VBUS, no TVS at all) survive this routinely, so the risk is low.
- **Board 1 test:** scope VBUS and SYS (TP on C7) during 20 magnet plug-ins.
  - If SYS exceeds 6.5 V, the fix for a later revision is a 1 Ω 0603 in series with VBUS before C5, or a polymer/tantalum 10 µF on VBUS for damping.
- **Board change:** not now.

**C3. Camera DVP input margin: 2.8 V DOVDD into 3.3 V ESP32 inputs.**

- **Numbers:**
  - ESP32-S3 VIH(min) = 0.75 × VDD = 2.475 V at 3.3 V (2.51 V at 3.35 V).
  - OV5640 VOH(min) is about 0.9 × DOVDD = 2.52 V.
  - ME6211 accuracy is ±2%.
  - So the guaranteed margin is only tens of mV. The real ESP32 switching threshold is about 1.6 V, and Seeed's XIAO-S3-Sense / ESP32-S3-EYE-class designs use the same arrangement, so it works in practice.
- **SCCB:** 4.7 k pull-ups to 2.8 V, so the high level is 2.8 V. That's above 2.475 V: **OK.**
- **XCLK:** GPIO18 drives 3.3 V into a 2.8 V-supplied input, within the OV5640's DOVDD + ~1 V input absolute max. Common practice: OK.
- **Firmware:**
  - Drive CAM_RESET (IO38) and CAM_PWDN (IO48) **open-drain** (or input/low only). R8 already pulls RESET up to 2.8 V, so a push-pull 3.3 V high just pushes current into the 2.8 V rail.
  - Leave the internal pull-ups off on IO39/IO40 (esp32-camera's SCCB init turns them on). Otherwise they back-power CAM_2V8 through R18/R19 when the camera is off.
- **Board change:** no.

**C4. D1 (B5819W) reverse leakage in battery mode.**
- SYS → D1 → VBUS → R4 + R5 (30 k) → GND, with 3.0–4.2 V reverse bias.
- A few µA at 25 °C, about 10× that warm. It adds to standby drain and lifts VBUS by about 0.1 V (harmless for Q2's gate).
- **Measure:** VBUS voltage / 30 k on board 1. **Board change:** no. (This is earlier review item N1.)

**C5. VBUS_SENSE overshoot.**
- 10 k / 20 k gives 3.33 V at 5.0 V, 3.5 V at 5.25 V and 3.67 V at 5.5 V on IO37. VDD + 0.3 V = 3.6 V.
- The current is limited by 10 k, so it's harmless.
- **Optional BOM tweak:** R5 → 15 k (C25756).
- **Board change:** no. (This is earlier review N2 / L1.)

**C6. Camera regulator dissipation.**
- U5 (ME6211 1.5 V) runs from SYS = 4.6 V on USB.
- At the OV5640's external-DVDD draw (tens of mA up to about 80 mA) that's up to (4.6 − 1.5) × 0.08 ≈ 0.25 W. In SOT-23-5 (θJA ≈ 250 °C/W) that's a junction rise of about 60 °C during a long live preview on USB.
- It's thermally protected, and the 30 s preview time-out bounds it.
- **Board 1 check:** finger or thermocouple U5 after 30 s of preview on USB.
- **Board change:** no.

**C7. RT9080 thermal on USB.**
- (4.6 − 3.3) V × ~0.15 A average during Wi-Fi ≈ 0.2 W. θJA = 189 °C/W (JEDEC; Richtek's EVB figure is 101 °C/W), so the rise is about 25–38 °C.
- 600 mA peaks are short. Over-temperature protection is at 150 °C. **OK** for this duty cycle.

**C8. Camera rail sequencing.**
- U4 (2.8 V) and U5 (1.5 V) share CAM_PWR_EN, so DOVDD, AVDD and DVDD rise together. The OV5640 prefers DOVDD → AVDD → DVDD.
- Seeed's module/board does the same and works.
- **Firmware order:**
  1. RESET low, PWDN low (both open-drain).
  2. PWR_EN high, wait 5 ms.
  3. Release RESET, wait 20 ms.
  4. Start SCCB.
  - Power-down is the reverse: set the pins to Hi-Z first.
- R9 10 k pulls PWDN low and R10 100 k pulls PWR_EN low, so the camera stays off through reset and deep sleep (IO34 is in the VDD_SPI domain).
- **Board change:** no.

**C9. Reversed battery while USB is plugged in.**
- With the battery reversed, Q1 is off and its body diode blocks. **Battery alone is safe.**
- With USB also plugged in, the charger drives VBAT_P positive, which turns Q1 partly on (G = 0 V, S = VBAT_P). Q1 then settles in linear mode at VBAT_P ≈ Vth and passes the charger's preconditioning current (≈ 5 mA) **into the reversed cell**.
- Adafruit cells come in Adafruit polarity and J4 follows it (J4.1 = GND, J4.2 = BAT+). This only happens with a third-party cell.
- **Rule:** meter-check the polarity once (already in ORDER_CHECKLIST), and never charge a cell of unknown polarity.
- **Board change:** no.

**C10. Magnet plugged in reversed.**
- Adafruit #5358 says its magnets are polarized ("If you try to connect the contacts backwards, they will repel!"), so a 180° flip can't latch.
- If it's forced, VBUS lands on D+ and cable GND on D−. USBLC6 then steers about 5 V into +3V3 through its I/O → VBUS-pin diode, so +3V3 could reach about 3.6 V (the ESP32 absolute max).
- So the real risk is the **cable-side wiring** of the magnet half, not flipping it. Keep the existing meter check: VBUS on J3 pin 1 before first use.
- **Board change:** no.

**C11. MLCC DC-bias derating** (all values are LCSC-confirmed parts; full table below).
- Smallest effective values:
  - C11/C12 on CAM_2V8: 2.2 µF / 6.3 V 0402 at 2.8 V ≈ 1.1 µF effective.
  - C6 (VBAT_P): 4.7 µF / 16 V 0603 at 4.2 V ≈ 3 µF. MCP73831 asks for ≥ 4.7 µF for stability with no battery.
  - C10/C14 (LDO inputs): 1 µF 0402 ≈ 0.8 µF at 4.6 V.
- All are at or near the minimums. Fine in practice, so **no change for this order.**
- In a later revision: C6 → 10 µF 0603 (C19702 is 10 V, OK at 4.2 V).

**C12. pins_final.h says R17 (KEY_ON pull-up) is 100 k, but the schematic/BOM has 10 k.**
- Either works. 10 k draws 330 µA only while ON is held. Fix the comment.

### OK (checked, with evidence)

**Power path** (netlist pin-by-pin):

- **D7 TVS:** A1 = VBUS, A2 = GND. Cathode to VBUS.
- **U7 USBLC6:**
  - 1/6 = USB_DP, 3/4 = USB_DM, 2 = GND, 5 = +3V3.
  - Pin 5 on +3V3 means there's no back-feed to the contact.
- **U2 MCP73831-2ACI:**
  - Pins: 1 STAT, 2 GND, 3 VBAT_P, 4 VBUS, 5 PROG.
  - **R2 = 20 k → I = 1000 V / 20 kΩ = 50 mA** (0.5 C). That's within Adafruit's "100 mA or less".
  - Dissipation ≤ (5 − 3) × 0.05 = 0.1 W.
- **Q1 AO3401A** (1 G, 2 S, 3 D; that pinout matches AO3401A):
  - S = VBAT_P, D = BAT+, G = 100 k to GND.
  - **Correct cell:** the body diode (D → S) conducts first, then Vgs = −3.0 to −4.2 V. Vth is −0.5 to −1.3 V, so it's fully on (≤ 80 mΩ at −2.5 V).
  - **Reversed cell:** the body diode is reverse-biased and Vgs = 0, so it's blocked.
- **Q2 AO3401A** (S = SYS, D = VBAT_P, G = VBUS):
  - **Battery only:** VBUS ≈ 0 through R4 + R5, so Vgs = −SYS → on. The body diode gives a glitch-free hand-over.
  - **USB only:** SYS = VBUS − Vf(D1) ≈ 4.55–4.7 V and Vgs ≈ +0.35 V → off. The charger output idles at 4.2 V with no cell.
  - **Both:** the system runs from USB through D1, and the charger sees only the cell, so termination works.
- **U3 RT9080-33GJ5:**
  - Pins: 1 VIN = SYS, 2 GND, 3 EN = SYS, 4 NC open, 5 VOUT.
  - IQ = 2 µA typ / 4 µA max.
  - Current limit: 0.61 A min, 1.1 A typ, fold-back 0.6 A.
  - **Output capacitor:** "any output capacitor meeting the minimum 1 mΩ ESR … larger than 1 µF may be used". There's no maximum, so about 110 µF on +3V3 is fine.
  - 3V3-rail load peak is about 355 mA Wi-Fi + 30 mA e-paper + a few mA, well under 600 mA. The camera is not on this rail.
  - The VOUT ≤ VIN + 0.3 V rule can't be violated, because SYS only feeds LDOs.
- **U4/U5 ME6211** (1 VIN = SYS, 2 GND, 3 CE = CAM_PWR_EN, 5 VOUT):
  - 6.5 V absolute-max VIN.
  - 100 mV dropout at 100 mA, so 2.8 V holds down to SYS ≈ 2.95 V.
- **CHG_STAT:** D6 (K = STAT_RAW, A = CHG_STAT) plus R20 100 k to VBUS_SENSE.
  - Low (charging) ≈ 0.25 V.
  - High (done) ≈ 3.3 V.
  - With no cable, 0 V, so read it only while VBUS_SENSE is high (FIRMWARE_STAGE13 already does this).
  - The 5 V STAT high is blocked by D6.

**E-paper booster** (matches the Waveshare HAT topology):
- L1 68 µH: LCSC SMNR4020-68UH, Isat 600 mA, DCR 1.38 Ω.
- **Q3 Si1308EDL:**
  - VDS 30 V, VGS ±12 V, RDS(on) 0.185 Ω at 2.5 V. Pinout 1 G, 2 S, 3 D matches.
  - G = GDR with R11 10 k pull-down; S = RESE through R12 3 Ω (the small-panel value).
- **Diode reverse voltages:**
  - SW peaks at about PREVGH + Vf ≈ 22 V.
  - D3, D4 and D5 each see at most about 22 V reverse. B5819W is rated 40 V.
  - The −22 V pump (C20 → D4/D5) has the right polarity.
- **Caps:**
  - C21–C30 are CL10A105KB8NNNC, 1 µF / 50 V X5R 0603. At 20–22 V that's about 0.4–0.5 µF effective, still more than the 25 V parts in the reference designs.
  - C20 is FH 1206B475K500NT, 4.7 µF / 50 V X7R 1206, about 2 µF at 22 V.
- **J2:** BS1 (pin 8) = GND; 6/7 NC; 15/16 = +3V3.

**Capacitor voltage ratings** (from LCSC):

| Ref | Part | Rating | Net (max V) | OK |
|---|---|---|---|---|
| C1, C32–C34 | CL21A226MAQNNNE 22 µF X5R 0805 | 25 V | +3V3 | yes |
| C3, C8 | CL10A106KP8NNNC 10 µF X5R 0603 | 10 V | +3V3 | yes |
| C5, C6, C7, C19 | CL10A475KO8NNNC 4.7 µF X5R 0603 | 16 V | VBUS 5.5 V, VBAT_P 4.2 V, SYS 4.7 V, +3V3 | yes |
| C4, C10, C14 | CL05A105KA5NQNC 1 µF X5R 0402 | 25 V | EN, SYS | yes |
| C11, C13, C15, C16 | CL05A225MQ5NSNC 2.2 µF X5R 0402 | 6.3 V | 2.8 V / 1.5 V / AF | yes |
| C2, C9, C12, C17, C18, C31 | C1525 100 nF 0402 | 16 V | ≤ 3.3 V | yes |
| C20 | 1206B475K500NT 4.7 µF X7R | 50 V | ±22 V pump | yes |
| C21–C30 | CL10A105KB8NNNC 1 µF X5R 0603 | 50 V | ±15…±22 V panel rails | yes |

**Boot and strapping:**
- **GPIO0:** TP1 only, internal weak pull-up, so normal SPI boot.
- **GPIO3:** floating (JTAG-source strap, only matters if the eFuse is set).
- **GPIO45:** unconnected, internal pull-down, so VDD_SPI = 3.3 V. That's correct for this module's 3.3 V flash.
- **GPIO46:** unconnected, internal pull-down.
- **EN:** R1 10 k to +3V3, C4 1 µF to GND, so τ = 10 ms (about 8.5 ms with DC bias). This is the Espressif recommendation.
- **TP7 = EN** gives a manual reset.

**USB:**
- J3: pin 1 VBUS, 2 GND, 3 USB_DM, 4 USB_DP.
- USB_DM goes to U1 pad 23 (IO19, D−) and USB_DP to pad 24 (IO20, D+), straight through with the USBLC6 shunting to GND/3V3.
- The magnet passes all four contacts as pogo pins at 0.1" pitch, which is fine for 12 Mbit/s full speed.
- `platformio.ini`: ARDUINO_USB_MODE=1 with CDC_ON_BOOT=1, i.e. the USB-Serial-JTAG peripheral. esptool auto-reset works while the app is alive.
- `power.cpp` clears USB_PAD_ENABLE only right before deep sleep. The deep-sleep wake resets that register.

**Pins** (ESP32-S3-MINI-1-N4R2):
- **IO26 is unconnected.** It's the in-package PSRAM CS, the only pin N4R2 loses.
- No GPIO27–32 exist on the module.
- IO33–37, IO47 and IO48 are free on the quad-PSRAM N4R2. They're in the VDD_SPI 3.3 V domain.
- IO19/20 are used only for USB, and no pin is used twice.
- **JTAG pins** IO39–42 are used as normal GPIO (SCCB, EPD DC/RST). That's fine while JTAG goes over USB, which is the default eFuse.
- **Deep-sleep wake:** ON = IO7 and KEYPAD_INT = IO4 are both RTC GPIOs (≤ IO21). EXT1 ANY_LOW is used in `power.cpp`.
- **ADC:**
  - VBAT_SENSE = IO9 = ADC1_CH8, which keeps working with Wi-Fi on.
  - VBUS_SENSE = IO37 isn't an ADC pin. It's used as a digital input, which is correct.

**Analog:**
- **VBAT divider:** R6/R7 1 M / 1 M with C9 100 nF.
  - 4.2 V → 2.1 V, so use 12 dB attenuation (0–~3.1 V range).
  - 2.1 µA drain. τ = 0.5 MΩ × 100 nF = 50 ms.
  - About 25 mV error per 50 nA of pin leakage, so a one-point calibration is worth it.
- **I2C:** 4.7 k pull-ups to +3V3 (TCA8418 VCC is 3V3), so no level shift is needed.
- **SCCB:** 4.7 k pull-ups to 2.8 V. The ESP32 open-drain pulls low and 2.8 V is above VIH, so no level shift is needed (see C3).
- **XCLK:** comes from the ESP32 LEDC on IO18. There's no oscillator to fit.

**Protection:**
- The USBLC6 protects D+/D− against ESD. D7 protects VBUS.
- GND is a direct contact.
- The cell has its own protection board: short circuit, over-charge, over-discharge cut at 3.0 V (Adafruit).
- Q1 handles a reversed cell (with the C9 caveat).
- No path puts the battery on the magnet contacts:
  - D1 blocks.
  - Q2's body diode points VBAT_P → SYS.
  - The MCP73831 has reverse-discharge blocking.

## Power budget

**Deep sleep** (ON or key wake, camera off, firmware S6 hygiene done):

| Item | µA |
|---|---|
| ESP32-S3-MINI-1 deep sleep with RTC IO wake | 8 |
| RT9080 IQ (typ / max) | 2 / 4 |
| TCA8418 idle | ~3 |
| R6 + R7 divider | 2.1 |
| D1 reverse leakage (25 °C) | 1–10 |
| MCP73831 VBAT leakage with VDD absent | ≤ 2 |
| #1570 protection IC | ~3 |
| SSD1680 deep sleep, ME6211 off, FET / USBLC6 leakage | ~1–2 |
| **Total** | **≈ 21–34 µA** |

That's about 3,000 h (4 months) from about 90 mAh usable, or about 3 months once LiPo self-discharge is counted.

**Active:**
- **Typing:** about 25–40 mA at 80 MHz, or about 0.3 mA in light sleep between keys.
- **3V3-rail peak:** about 385 mA uncapped (355 mA Wi-Fi + e-paper + TCA). The cap of 600 mA leaves about 35% margin.
- **Battery peak:** about 0.25 A with the TX cap and the camera off, or about 0.5 A worst case (camera + full-power TX). See C1.
- **Per AI solve:**
  - viewfinder about 10 s × ~260 mA ≈ 0.7 mAh
  - Wi-Fi upload/wait about 10 s × ~120 mA ≈ 0.35 mAh
  - total ≈ 1–1.5 mAh, so **about 60–90 solves per charge**.
- **Charge time:** 50 mA CC/CV → about 2.5 h from empty.

## What needs a board change

Nothing for this order. Later-revision ideas, all optional:
- R5 → 15 k
- C6 → 10 µF
- 1 Ω series damping on VBUS, if C2 measures SYS > 6.5 V

## Sources

- [Richtek RT9080 DS9080-09](https://www.richtek.com/assets/product_file/RT9080/DS9080-09.pdf)
- [Adafruit #1570](https://www.adafruit.com/product/1570)
- [Adafruit #5358](https://www.adafruit.com/product/5358)
- [ME6211 datasheet (LCSC)](https://wmsc.lcsc.com/wmsc/upload/file/pdf/v2/lcsc/2002261905_MICRONE-Nanjing-Micro-One-Elec-ME6211C15M5G-N_C487906.pdf)
- [Si1308EDL (Vishay)](https://www.rlocman.ru/datasheet/pdf.html?di=177283)
- LCSC pages: C29823, C15849, C135265, C45783, C12530, C19666, C19702, C52923
