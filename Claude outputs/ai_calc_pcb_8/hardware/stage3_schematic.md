# Stage 3: schematic (done)

ERC: **0 errors, 0 warnings** (KiCad 10.0.6, all six sheets). Every placed part has an LCSC number, checked live against the JLCPCB parts list on 2026-10-02.
Keypad pads (SW1-SW50) and test pads (TP1-TP4) are copper only: they are excluded from the BOM and the pick-and-place file.

## Changes made in the final review
| Change | Why |
|---|---|
| Added R18/R19 4.7k pull-ups on the camera SCCB lines (to CAM_2V8) | The camera I2C bus had no pull-ups. Pulling up to the camera's own 2.8 V rail means nothing leaks into the camera while its power is off. |
| USBLC6 reference pin moved from VBUS to +3V3 | Otherwise the ESP32's USB pull-up leaks through the ESD chip onto the magnet's VBUS pin (~2.6 V with no cable) and confuses the cable-detect pin. |
| Charge current 100 mA -> 50 mA (R2 20k, C25765 basic) | 100 mA is 1 C for the 100 mAh Adafruit cell; 0.5 C is easier on it inside a closed case. Full charge takes about 2.5 h. |
| Cable-detect divider R4 100k -> 51k (basic part) | With 100k/100k a 4.75 V cable only gives 2.37 V, below the ESP32's guaranteed "high" (2.48 V). Now 3.1-3.5 V. |
| Both FPC sockets -> flip-lid 24P 0.5 mm (first AFC01-S24FCA-00; in stage 4 changed to the dual-contact SHOU HAN C6364666, see stage4_5_layout.md) | The same socket the Seeed OV5640 board uses, so the cable's pin order is known-good. A flip lid is easier to plug in than a slide lock, and it's one part number for both sockets. |
| C20 booster pump cap -> 4.7 uF 25 V 0805 (C1779, basic) | The 0603 part was only rated 16 V; the pump node swings to about +20 V. |
| Footprints for J1/J2/J4/L1/Q3 taken from LCSC's own models | These match exactly what JLCPCB places (pads and 3D models in ai_calc.pretty / ai_calc.3dshapes). |
| L1 is SMNR4020-68UH (C135265, 4x4 mm) | This is the part that LCSC number actually is. |
| sym-lib-table uses ${KIPRJMOD} | So the project opens on any computer. |

## Netlist sanity check (done by script)
- Power path: VBUS -> D1 -> SYS; battery -> Q1 (reverse protection) -> VBAT_P -> Q2 body diode / channel -> SYS. Q2's gate is on VBUS, so the battery is cut off from SYS whenever a cable is in. The charger output is on VBAT_P.
- The magnet contacts carry no battery voltage: VBUS only reaches D1's anode, Q2's gate, the charger's input, and the 151k divider to GND.
- Camera pinout: Seeed 24-pin (AF on 24, GND on 23). E-paper pinout: Waveshare 24-pin (BS1 = GND, 4-wire SPI).

## Still open (needs your hardware, not blocking the board)
- RESE 0.47 ohm suits "config B" panels (most current 2.13" V3/V4). Check the Display Config switch on your Waveshare HAT.
- JST-PH polarity of the Adafruit #1570 battery. Q1 protects the board if it's reversed (the board just won't power on).

## Design-review fixes (2026-10-02, independent review of the whole schematic)
| Change | Why |
|---|---|
| **D6 (B5819W) + R20 100k** between the charger's STAT pin and IO35 | The MCP73831's STAT pin *drives* 5 V when charging is done (it isn't open-drain), which would push 5 V into a 3.3 V ESP32 pin. Now STAT can only pull IO35 low, and R20 provides the high. Reading is unchanged: low = charging, high = done / no cable. |
| **R12 (e-paper current sense) 0.47 Ω → 3 Ω** (C23157) | The V4 black-and-white panel's reference circuit and Waveshare's 2.13" HAT both use 3 Ω. 0.47 Ω is for the colour panels, and with it the current limit would be 6× too high for the 0.6 A inductor. |
| **D7 SMF5.0A TVS on VBUS** at the magnet connector (C193402) | The exposed VBUS contact had no surge or ESD protection. Hot-plugging a magnetic contact can ring well above 5 V, and the charger is rated 7 V absolute maximum. |
| **VBUS sense divider 51k/100k → 10k/20k** (both basic parts) | Keeps VBUS firmly at 0 V when no cable is in, even with diode leakage when warm. Reads 3.17–3.5 V with a cable in. |
| **C20 (booster pump) → 4.7 µF 50 V 1206** (C29823, basic) | It runs at about 20 V, where a 25 V X5R part loses most of its capacitance. Both references use 50 V. |

Checked and found OK in the same review: every ESP32 pin against the datasheet's Table 3-1 (IO26 left unconnected for -N4R2; IO33–37 free; straps fine), all 24 camera pins against Seeed's module table, the booster topology and diode directions, all TCA8418 pins (address 0x34), all LCSC numbers and footprints, and the power path (Q1 reverse protection, Q2 load sharing, 50 mA charging).

### Notes for the firmware
- **Turning the camera off:** call `esp_camera_deinit()`, then set IO10–18, 21, 36, 38–40, 47 and 48 to `GPIO_MODE_DISABLE` with no pulls, *then* pull CAM_PWR_EN low. Don't use `gpio_reset_pin()`: it turns on a pull-up, which would power the switched-off camera through its own pins.
- **USB:** switch off USB-Serial-JTAG while VBUS_SENSE is low. Its D+ pull-up otherwise puts 3.3 V on the exposed contact (harmless, but pointless).
- **Waking from sleep:** IO35 and IO37 aren't RTC pins, so plugging in a cable can't wake the chip from deep sleep. ON (IO7) and a key press (IO4) can.
- **Cable:** the magnetic cable has only 4 wires and no CC resistor. A USB-C charger only switches on if the C end of the cable has a 5.1 kΩ pull-down from CC to GND, so make the cable from a USB-A lead, or add that resistor.
- **Camera clock:** start with `xclk_freq_hz = 20000000`. If photos show stripes or broken frames, drop to 10 MHz. The camera bus runs about 70 mm over the keypad area, where the ground under it is broken up by the key pads.
