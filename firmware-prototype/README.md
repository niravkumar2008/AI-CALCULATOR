# Prototype firmware (the board inside the Casio)

Runs the full calculator on the custom board: the same calculator code as the simulators and the
tester (`../core`), with the real keypad, battery and autofocus camera. The tester keeps its own
firmware in `../firmware`, untouched by this one.

**Status:** builds; not yet run on hardware (the board doesn't exist yet).

## Hardware it expects

| Part | What |
| --- | --- |
| Module | ESP32-S3-WROOM-1-N16R8 (16 MB flash, 8 MB PSRAM: the same as the tester) |
| Camera | OV5640 with autofocus, 24-pin connector, power-down on GPIO 21 (an OV3660 also works, without autofocus) |
| Screen | Waveshare 2.13" e-paper panel, driver parts on the board |
| Keys | the Casio's pads as a 7 x 7 matrix into a TCA8418 keypad scanner (I2C); ON on its own pin |
| Power | 150 mAh LiPo, MCP73831 charger, battery voltage on GPIO 9 through 2 x 100 kOhm |
| USB | the chip's own USB on GPIO 19/20, through the magnetic plug |

Every pin is in `src/pins.h`; which key sits where in the matrix is `kMatrix` in `src/keys.cpp`.
The board's schematic follows those two.

## Upload

Open this folder (`firmware-prototype`) in VS Code with PlatformIO and click Upload, with the
magnetic USB cable plugged in. Setup over the Serial Monitor is the same as the tester:
`wifi`, `key`, `status`, `preview`, `keys ...`, `exam off` (see `../firmware/README.md`).

## Differences from the tester

| | Tester | Prototype |
| --- | --- | --- |
| Keys | =, AC, ▲, ▼, BOOT | whole keypad; hold ▲ ▼ ◀ ▶ DEL to repeat |
| Starts in | AI SOLVE | COMP, like the Casio (MODE 4 = AI SOLVE) |
| Photo | 1600x1200 + enhanced close-up | 2560x1920 after autofocus, one photo |
| Power | USB | LiPo; low-battery icon at 20%; off = Wi-Fi and camera off and light sleep, any key wakes it |
| Serial Monitor | COM/UART port | the magnetic USB plug |

Battery reading: if the % disagrees with a multimeter, adjust `kBatteryCal` in `src/main.cpp`.
Off, the chip light-sleeps (about 3 weeks on a charge); memory and settings stay.
