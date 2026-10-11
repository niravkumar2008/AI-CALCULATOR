# Board firmware (the calculator inside the Casio shell)

This is the firmware for the custom board (`../hardware/kicad`, ESP32-S3-MINI-1-N4R2): the full
calculator from `../core` (the same code as the simulators), the whole keypad, the e-paper, the OV5640
autofocus camera, the 150 mAh battery and deep sleep, Wi-Fi through the AI proxy (`../server/proxy`),
over-the-air updates and a phone setup page. **`../firmware/` is the old breadboard tester** (different
pins, different board): never flash it to this board.

**Status (stage 14, 2026-10-06):** builds clean (`pio run`: RAM 30.7 %, flash 63 % of the 1.9 MB OTA
slot). Not yet run on hardware: start at `../hardware/ARRIVAL_CHECKLIST.md`, then `../hardware/bringup_guide.html`
(step 10 = AI server + token); the day-one script in `../firmware/FINAL_REVIEW_firmware.md` is the same path. Version string: `kFirmwareVersion` in `src/claude_client.h`.

## Hardware it expects (from the schematic)

| Part | What |
| --- | --- |
| Module | ESP32-S3-MINI-1-N4R2 (4 MB flash, 2 MB quad PSRAM) |
| Camera | OV5640 autofocus on J1; power through CAM_PWR_EN (GPIO 34), PWDN 48, RESET 38 (open-drain); data pins in `hardware/pins_final.h` |
| Screen | 2.13" e-paper panel (SSD1680, 250x122) on J2 with the board's boost circuit; BUSY on GPIO 33 (weak internal pull-down so a missing panel doesn't stall the bench) |
| Keys | 49 keys as an 8 x 10 matrix (ROW0-7, COL0-9) into a TCA8418 (I2C on GPIO 1/2, INT 4); ON on GPIO 7 |
| Power | Adafruit #1317 150 mAh LiPo on J4, MCP73831 charger at 50 mA (STAT on GPIO 35, pulled up to VBUS_SENSE), battery sense GPIO 9 (1 M / 1 M), USB sense GPIO 37 |
| USB | the chip's own USB on GPIO 19/20, through the magnetic plug (J3) |

Every pin is in **`hardware/pins_final.h`** (`src/pins.h` only includes it; it is the single source of
truth and `python tools/check_pins.py` compares it with the schematic's netlist). Which key sits at which
matrix crossing is `kMatrix` in `src/keys.cpp`.

## Files

| File | What |
| --- | --- |
| `src/main.cpp` | the loop: keys, screen, Wi-Fi on/off, battery lock-out, viewfinder, Send, setup mode, updates, deep sleep |
| `src/settings.*` | NVS settings (hotspot, proxy, token) and the Serial Monitor commands |
| `src/setup_portal.*` | phone setup: the calculator's own Wi-Fi network + sign-in page that writes the settings |
| `src/ota.*` | over-the-air update: manifest, download, SHA-256 check, second app slot, rollback |
| `src/claude_client.*` | HTTPS to the proxy: the photo solve (streamed), small JSON calls, the firmware download |
| `src/camera.*`, `src/preview.*` | OV5640 capture / tuning, and the laptop preview page (`preview` command) |
| `src/keys.*`, `src/screen.*`, `src/power.*` | TCA8418 keypad + ON, e-paper, battery / charging / deep sleep |
| `src/selftest.*` | factory self-test (`SELFTEST_JSON` line; fields listed in `hardware/bringup_guide.html`) |
| `src/log.h` | `LOGF("module", ...)`: every log line is `[module] text` |
| `platformio.ini` | board settings; `min_spiffs.csv` partition table = two 1.9 MB app slots for OTA |

## 1. Build and flash (USB)

1. VS Code with the PlatformIO extension. **File > Open Folder** > `firmware-prototype` (this folder).
2. Plug the magnetic USB cable into the board and the PC.
3. Click **Upload** (the arrow in the blue bar). First build downloads the toolchain (5-10 minutes).
   Command line: `%USERPROFILE%\.platformio\penv\Scripts\pio run -t upload` from this folder.
4. **Serial Monitor** (plug icon) at **115200**. Type `help`.

The first upload after this version also writes the new partition table (two app slots). Saved settings
survive it (NVS sits at the same place in both tables). **The first flash of every board is this USB
upload**: an over-the-air update only writes the other app slot, never the partition table. After it,
`status` shows `slot app0`.

### Recovery: no COM port, or "no serial data"

The chip is in normal boot and must be forced into download mode. Plugging the cable in does **not**
reset the chip while the battery is connected (the battery keeps 3.3 V and EN up), so holding BOOT while
plugging in does nothing on its own. Back cover off:

1. Hold a wire from **TP1 (BOOT)** to **TP4 (GND)**.
2. Tap a second wire from **TP7 (EN)** to **TP4 (GND)** for about 0.2 s. The chip restarts in download mode.
   (Or unplug the battery at J4 first, then hold TP1 to GND while you plug the cable in.)
3. Release TP1 and click Upload.
4. When it finishes, tap TP7 to GND once more to start the new firmware.

Backup path if the USB path ever fails: the ROM also listens on UART0. A 3.3 V USB-serial adapter on
**TP2 (TX) / TP3 (RX) / TP4 (GND)**, then the same TP1 + TP7 sequence, and upload over that COM port.

**Never burn the security or USB-disable eFuses** (`DIS_USB_SERIAL_JTAG`, `DIS_DOWNLOAD_MODE`,
`ENABLE_SECURITY_DOWNLOAD`), and never enable secure boot or flash encryption on this board: they are
the only things that could lock it out for good.

## 2. First setup

The calculator needs three things to use AI: a phone hotspot (name + password), the AI server address
(`https://...`, your deployed `server/proxy`) and its device token (`python admin.py new-device "board 1"`
on the server). The maths works with none of them. It never holds a Claude API key: the `token` command
refuses anything starting with `sk-ant-`, and an old key left by earlier firmware is erased at first boot.

**From a phone (no PC):** SHIFT MODE (SETUP) > **6: Wi-Fi & key** > press **=**. The screen shows
`SETUP BY PHONE` with a network name (`AI-Calc-xxxx`) and an 8-character password. Join that network on
the phone; the setup form pops up as the network's sign-in page (or open `http://192.168.4.1`). Pick the
hotspot from the list the calculator saw, type its password, the server address and the token, **Save**.
The calculator closes the network a few seconds later. AC leaves setup mode; it also ends by itself after
10 minutes. (The serial command `setup` does the same and prints the details; `setup off` stops it.)

**From the Serial Monitor:** `wifi`, `proxy`, `token`. Then `pair` for the 6-letter code and open
`<server>/link` on the phone (an unlinked calculator also shows the code when you first press = in AI
SOLVE), and `account` for the subscription state.

Other commands: `status`, `scan`, `snap`, `preview` (laptop camera page), `selftest`, `update`,
`forget`, `keys 1/3=` (press keys from the keyboard: digits . + - * / ^ ( ) =; s c t sin cos tan,
l log, n ln, r sqrt, q x², i x⁻¹, b Abs, g logₐb, f fraction, ~ (−), E ×10ˣ, h hyp, k RCL, w S⇔D, M M+,
a Ans, # DEL, $ AC, m MODE, o ON, [ SHIFT, ] ALPHA, < > u d arrows, p π, ! x!, % percent, , comma),
`exam off` (the teacher's USB unlock).

## 3. Using it

- **MODE 4 = AI SOLVE.** The viewfinder runs on the e-paper; **point the camera at the problem and
  press =**. ▲▼ change the effort, **1** toggles tutor mode (one hint per =), AC goes back to the
  calculator, ▶ restarts the viewfinder after its 30 s idle timeout.
- **Messages on the glass:** `No connection` / `NO CONNECTION` (turn the phone's hotspot on),
  `Not set up` (see First setup), `Battery low: plug in the cable to use AI solve` (below 3.6 V the
  camera and Wi-Fi stay off together until the cell is back above 3.7 V or a cable is in; maths keeps
  working), `AI ACCOUNT` (link / subscription / monthly cap, with the server's own sentence), the black
  `EXAM` bar (exam mode), and while off on a cable: `Charging 64%`.
- **Off** (SHIFT AC, or 10 minutes idle) is deep sleep (~25-35 µA for the board). ON or any key wakes
  it; memories, history and settings come back from RTC memory. With a cable in it stays awake so the
  Serial Monitor keeps working.
- **Exam mode:** SHIFT, 7, ON while off (or SETUP 8). Wi-Fi and camera off until a USB cable's
  `exam off` or 12 hours. Survives a battery pull.
- Battery reading off by more than 2 % from a meter on TP6? Adjust `kBatteryCal` in `src/power.cpp`.

## 4. Firmware updates over the air (OTA)

1. Build (`pio run`). The image is `.pio/build/prototype/firmware.bin`; bump `kFirmwareVersion` in
   `src/claude_client.h` first (the proxy compares version strings, so any change counts as "different").
2. On the server: `python admin.py firmware .pio/build/prototype/firmware.bin stage14-2026.10.10`.
   That copies the image into `FIRMWARE_DIR/stage14/` with its size and SHA-256. The proxy keeps one
   image per board family, so a v15 LCD image (`v15lcd-...`) can be published at the same time and
   is never offered to these boards (nor this one to v15 boards).
3. On a calculator: either type `update` in the Serial Monitor, or just **switch it off (SHIFT AC)
   with the cable in**: 20 s later it joins the hotspot, asks the proxy (`GET /v1/firmware` with its
   version), and if the server has a different image it shows `UPDATING ... 45%`, checks the SHA-256,
   writes the other app slot and restarts. It checks again once a day while it stays on the cable.
   On battery an update needs more than 3.8 V (`update` command only; the automatic check needs a cable).
4. The new firmware **confirms itself** 15 s after boot once the keypad scanner answers
   (`esp_ota_mark_app_valid_cancel_rollback`). If it crashes before that, the bootloader boots the
   previous slot (`status` shows the slot and "not yet confirmed").

The image travels over TLS to the root-pinned proxy and is accepted only when its hash matches the
manifest; there is no signing key in the firmware or the repo (nothing secret is in either).

### Proxy follow-ups in `stage14-2026.10.10` (small, safe changes)

- **Preview Send no longer gives up on a long answer.** `kSendSolveTimeoutMs` (120 s *total*) became an
  idle limit, `kSendSolveIdleMs` = 150 s without a byte to or from the proxy (the client's own `kIdleMs`
  of 120 s still reports first), plus an overall cap `kSendSolveMaxMs` = 300 s (`src/main.cpp`). The
  proxy's `: ping` every 15 s counts as activity, so an `xhigh`/`max` solve that is still thinking or
  streaming isn't called "took too long" at 2 minutes. `src/claude_client.cpp` notes the time of the last
  byte sent or received (`claudeLastActivityMs()`).
- **Burst-limit message.** The proxy can now answer its burst limit as `429 rate_limited`; `core`'s
  `classifyFailure` shows its message like the other account errors. The proxy sends the old
  `fair_use_exceeded` to firmware older than 2026-10-10 (by `x-firmware`), so boards still on
  `stage14-2026.10.06` keep showing the message too.
- The request still carries the system prompt on v14 (`CALC_SEND_SYSTEM_PROMPT` defaults to 1), so these
  boards also work with an older proxy. Build result unchanged: RAM 30.7 %, flash 63.4 %, no warnings.

## 5. Serial log

Every line starts with its module: `[keys] 7`, `[wifi] connected: 172.20.10.3`, `[cam] focus ...`,
`[solve] [3] answer after 6120 ms`, `[ota] ...`, `[setup] ...`, `[power] deep sleep`. Two lines are
different on purpose: the `status` block, and `SELFTEST_JSON {...}` (one JSON line the bring-up guide
asks you to save). Interactive prompts (`Hotspot name:`) have no prefix.

## Differences from the tester

| | Tester (`../firmware`) | This board |
| --- | --- | --- |
| Keys | =, AC, ▲, ▼, BOOT | whole keypad; hold ▲ ▼ ◀ ▶ DEL to repeat |
| Key row 2 | — | fx-115ES legends: CALC, ∫dx, x⁻¹, logₐb (CALC / ∫dx show "Not supported yet") |
| Starts in | AI SOLVE | COMP, like the Casio (MODE 4 = AI SOLVE) |
| Photo | 1600x1200 + enhanced close-up | 2048x1536 after autofocus, best of 4 frames, one photo |
| Claude | API key in the device (never ship it) | proxy + device token, no key on the device |
| Power | USB | LiPo; deep sleep when off; Wi-Fi only in AI SOLVE, capped at 11 dBm; AI off below 3.6 V |
| Setup | Serial Monitor | Serial Monitor or the phone setup page |
| Updates | USB | USB or over the air with rollback |
| Serial Monitor | COM/UART port | the magnetic USB plug |
