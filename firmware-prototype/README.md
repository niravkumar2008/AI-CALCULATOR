# Prototype firmware (the board inside the Casio)

Runs the full calculator on the custom board (`../hardware/kicad`): the same calculator code as the
simulators and the tester (`../core`), with the real keypad, battery and autofocus camera. The tester
keeps its own firmware in `../firmware`, untouched by this one.

**Status:** builds; not yet run on hardware. Pins and key matrix checked against the schematic
(`hardware/kicad/ai_calc.kicad_sch`) on 2026-10-02.

## Hardware it expects (from the schematic)

| Part | What |
| --- | --- |
| Module | ESP32-S3-MINI-1-N4R2 (4 MB flash, 2 MB quad PSRAM) |
| Camera | OV5640 autofocus on J1; power through CAM_PWR_EN (GPIO 34), PWDN 48, RESET 38; data pins in `src/pins.h` (board stage-6 order) |
| Screen | 2.13" e-paper panel on J2 with the board's boost circuit; BUSY on GPIO 33 |
| Keys | 49 keys as an 8 x 10 matrix (ROW0-7, COL0-9) into a TCA8418 (I2C on GPIO 1/2, INT 4); ON on GPIO 7 |
| Power | LiPo on J4, MCP73831 charger (STAT on GPIO 35), battery sense GPIO 9 (1 M / 1 M), USB sense GPIO 37 |
| USB | the chip's own USB on GPIO 19/20, through the magnetic plug (J3) |

Every pin is in `src/pins.h`; which key sits at which matrix crossing is `kMatrix` in `src/keys.cpp`.
If the schematic changes, those two files change with it.

## Upload

Open this folder (`firmware-prototype`) in VS Code with PlatformIO and click Upload, with the
magnetic USB cable plugged in. If the board doesn't appear: hold the BOOT test pad to GND while
plugging in. Setup over the Serial Monitor is the same as the tester: `wifi`, `key`, `status`,
`preview`, `keys ...`, `exam off` (see `../firmware/README.md`). `status` also shows the battery.

## Differences from the tester

| | Tester | Prototype |
| --- | --- | --- |
| Keys | =, AC, ▲, ▼, BOOT | whole keypad; hold ▲ ▼ ◀ ▶ DEL to repeat |
| Key row 2 | — | the newer Casio keypad: Abs, x³, x⁻¹, logₐb; nPr / nCr on SHIFT × / ÷ (serial `keys`: `b` Abs, `g` logₐb) |
| Starts in | AI SOLVE | COMP, like the Casio (MODE 4 = AI SOLVE) |
| Photo | 1600x1200 + enhanced close-up | 2048x1536 after autofocus, best of 4 frames, one photo (2 MB PSRAM: one frame buffer) |
| Power | USB | LiPo; battery icon at 20% or below and while charging; off = Wi-Fi and camera powered down and light sleep (any key or plugging in USB wakes it; stays awake while USB is in) |
| Serial Monitor | COM/UART port | the magnetic USB plug |
| Preview "last scan photo" | yes | no (no room in 2 MB next to the request) |

Battery reading: if the % disagrees with a multimeter, adjust `kBatteryCal` in `src/main.cpp`.
Off, the chip light-sleeps (about 3 weeks on a charge); memory and settings stay.
