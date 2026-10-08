# Firmware for the stage-13 board

> **Superseded for day-one use (2026-10-06).** This page is the stage-13 history. The current firmware is
> stage14-2026.10.06 with over-the-air updates (two app slots: the **first flash of every board must be over
> USB**), phone setup (SHIFT MODE, 6, = or the `setup` command), the low-battery gate (camera + Wi-Fi off
> below 3.6 V, back above 3.7 V) and `firmware_slot` in `status` / `SELFTEST_JSON`. Use
> **`firmware-prototype/README.md`** (build, flash, setup, OTA), **`hardware/bringup_guide.html`** (power-up,
> self-test fields, step 10 = server + token) and **`server/proxy/README.md`** (deploy). Start at
> `hardware/ARRIVAL_CHECKLIST.md`. The example `SELFTEST_JSON` below predates `firmware_slot`, `battery_ok`
> and `camera_autofocus`.

Plain-English notes for the firmware that goes on the custom board when it arrives (around 2026-10-15).
The board firmware is in **`firmware-prototype/`**. The breadboard tester keeps its own firmware in
`firmware/`; it still builds, and it only got two small compiler-warning fixes.

Status (updated with the camera viewfinder): **builds with no warnings** (`pio run`; our code is also clean with `-Wall -Wextra`, only the third-party OV5640 autofocus library shows 3 there). The PC tests pass (541 checks),
and so do the proxy tests (9). **None of it has run on the real board yet**, so follow the bring-up checklist
at the end.

## What changed

| Area | Before | Now |
| --- | --- | --- |
| Pins | `firmware-prototype/src/pins.h` was a copy | **One file:** `hardware/pins_final.h`, which the firmware includes directly. Every pin carries its schematic net name. The build fails if two signals share a pin or a pin can't be used. `python firmware-prototype/tools/check_pins.py` compares it with the schematic (all 32 match) |
| Off | Light sleep, about 2 weeks on the battery | **Deep sleep.** The chip draws about 8 µA, which is months of standby once review fixes S1/S2 are on the board. ON or any key wakes it in about 0.3 s. Memories, Ans, the replay history, SETUP, mode, tutor and exam mode survive |
| Before sleeping | — | The keypad chip's queue is emptied and its interrupt cleared (otherwise R15 draws 330 µA). The camera is unpowered and every camera pin is left floating with no pull-up (SIOD/SIOC too). The e-paper's pins are held still. USB is switched off when no cable is in |
| Camera | Powered up at every start | Powered up only for the first scan, in the order from review N3 (reset low, power, 5 ms, release reset). PWDN is never driven high |
| Wi-Fi | On whenever the calculator was on | **On only in AI SOLVE** (and for `pair`/`account`). COMP maths never uses the radio. TX power is capped at **11 dBm** for the small cell (review S4; tuned for the original 100 mAh cell, the board now carries the Adafruit #1317 150 mAh) |
| Battery | Straight line from 3.3 to 4.2 V | **LiPo discharge curve** (`core/battery.cpp`, tested), averaging 8 readings. AI is refused below **3.6 V** on battery: "BATTERY LOW", maths still works |
| Charging | STAT read with an internal pull | STAT is read **only while VBUS_SENSE is high**, because R20 now pulls up to VBUS_SENSE. No internal pulls on IO35 |
| Claude key | Stored in the calculator (`key` command) | **The calculator never holds the key.** It sends the photo to **our proxy** (`server/proxy/`) with its own device token. The proxy holds the key. On first start the new firmware **erases any old key** left in flash |
| Accounts | — | Pairing code, then link page, then first month free. The server keeps the subscription state ($15/month), a fair-use cap of **500 solves/month**, and an optional free daily tier |
| Self-test | — | Factory self-test: hold **SHIFT + ALPHA, press ON** (or type `selftest`). It prints one JSON line |
| AI answers | — | **✓ verified:** Claude also sends `check`, a calculation for its final number. The calculator's own engine re-computes it and shows ✓ when they agree, or **⚠ CALCULATOR GETS …** when they don't |
| Viewfinder | — | **Live camera preview** in AI SOLVE (partial refresh, ~2–2.5 fps), with brackets, a crosshair, focus / hold-still status and a text box. = takes the photo. AC or 30 s idle turns the camera off. See "Camera preview" below |
| Tutor mode | — | On the AI SOLVE screen, **1** switches tutor mode on or off. The reply shows the question, then one hint per **=**, and the answer last |

## Flash it over the magnetic USB

1. Plug the magnetic cable into the board and the PC.
2. In VS Code, open the folder `firmware-prototype` (PlatformIO extension installed). Click **Upload** (→ in the
   blue bar).
3. If no COM port appears, or the upload says "no serial data", force the chip's built-in download mode
   (back cover off; corrected in stage 14). Plugging the cable in does **not** reset the chip while the battery
   is connected, so holding BOOT while plugging in does nothing on its own.
   1. Hold a wire from the **BOOT pad (TP1)** to **GND (TP4)**.
   2. Tap a second wire from **EN (TP7)** to **GND (TP4)** for about 0.2 s. The chip restarts in download mode.
      (Or unplug the battery at J4 first, then hold TP1 to GND while you plug the cable in.)
   3. Release TP1 and click Upload.
   4. When it's done, tap TP7 to GND once more to start the new firmware.
   - Backup if the USB path ever fails: a 3.3 V USB-serial adapter on **TP2 (TX) / TP3 (RX) / TP4 (GND)**, then
     the same TP1 + TP7 sequence, and upload over that COM port.
   - **Never burn the security or USB-disable eFuses** (`DIS_USB_SERIAL_JTAG`, `DIS_DOWNLOAD_MODE`,
     `ENABLE_SECURITY_DOWNLOAD`) and never enable secure boot or flash encryption on this board. They are the
     only things that could lock you out for good.
4. Open the **Serial Monitor** (plug icon), set to 115200. Type `help`.

Command line instead: `cd firmware-prototype` then `%USERPROFILE%\.platformio\penv\Scripts\pio run -t upload`.

Note: with the cable unplugged the calculator deep-sleeps a few seconds after you switch it off. In deep sleep
the USB port disappears. Press ON (or plug in and press a key) to bring the port back.

## First setup (Serial Monitor)

| Type | Then |
| --- | --- |
| `wifi` | your phone hotspot's name and password (2.4 GHz; iPhone: Maximize Compatibility) |
| `proxy` | the proxy's address, e.g. `https://calc-proxy.onrender.com` |
| `token` | this calculator's device token. Make it on the server with `python admin.py new-device "board 1"` |
| `pair` | prints a 6-letter code. Open `<proxy>/link`, type the code and your e-mail. The free month starts |
| `account` | linked e-mail, subscription (trial / active / inactive), solves this month |
| `status` | firmware, camera, Wi-Fi, proxy, token (masked), battery mV and %, charging |

Then press MODE 4 (AI SOLVE). Wi-Fi switches on and joins the hotspot while you aim. Press **=**.

If the calculator isn't linked yet, the AI screen says "AI ACCOUNT – Link this calculator:
<proxy>/link code ABC123". That message is the pairing code too.

## Factory self-test

Start it in one of two ways:
- With the calculator off (or right after connecting the battery), **hold SHIFT and ALPHA, then press ON**.
- Type `selftest` in the Serial Monitor.

It runs these checks in order:
1. chip and memory (4 MB flash, 2 MB PSRAM)
2. keypad chip (TCA8418) answers on I2C
3. battery voltage and %, cable present, CHG_STAT level and charge state
4. **screen pattern**: checkerboard + border. The refresh time proves BUSY and the booster work (pass: 0.3–9 s). Look at it for dead lines.
5. **camera**: powers up, reads the sensor ID (OV5640), takes one VGA JPEG (pass: > 1 KB)
6. **Wi-Fi scan**: number of networks, best signal (pass: at least one network, better than −80 dBm), and the battery voltage while the radio is on (a big dip there = a weak cell)
7. **every key**: press all 50 keys once (ON too). The screen counts them. AC twice in a row, or `q` in the Serial Monitor, stops early. The test times out after 90 s.

At the end it prints one line, for example:
```
SELFTEST_JSON {"firmware":"stage13-2026.10.04","device_id":"calc-…","memory_ok":true,"keypad_scanner_ok":true,
"battery_mv":3987,"battery_pct":75,"vbus":true,"chg_stat_high":false,"charge":"charging","display_refresh_ms":2140,
"display_ok":true,"camera":"Final board (OV5640 AF), OV5640","camera_jpeg_bytes":31877,"camera_ok":true,
"wifi_networks":7,"wifi_best_rssi":-48,"battery_mv_radio_on":3951,"wifi_ok":true,"keys_ok":50,"keys_missing":[],
"keys_all_ok":true,"pass":true}
```
The screen shows **SELF-TEST PASS/FAIL** with a summary. Any key restarts the calculator.

## Using the new AI features

- **Tutor mode:** AI SOLVE screen, press **1**. The status line shows "1:Tutor on". After a scan you see the
  question. Each **=** shows the next hint, and the answer comes after the last hint. Then **=** takes a new photo.
- **✓ / ⚠:** when the answer is one number, the calculator re-computes Claude's `check` calculation.
  "ANSWER ✓" plus "✓ calculator agrees" means the arithmetic is confirmed (within 1 %, for rounding).
  "⚠ CALCULATOR GETS 24.525 = 9.81×2.5" means Claude's number and its own working disagree, so check it.
  Word answers, several parts, or answers in x show neither.

## Camera preview (viewfinder)

Open **AI SOLVE** (MODE 4) and the screen becomes a camera viewfinder, like a phone's camera:

```
+---------------------------+-------------+
| ┌                       ┐ | AI CAMERA   |
|   (live picture, dithered | ✓ SHARP     |  <- or HOLD STILL / Focusing...
|    black-and-white)       | [#####|   ] |  <- focus bar; the tick = "sharp enough"
|        - + -   crosshair  | focus       |
|   ┌ - - - - ┐ text found  | text found  |
| └                       ┘ | Normal      |  <- effort (+Tutor if on)
|                           | =:photo     |
|                           | AC:back     |
|                           | off in 27 s |
+---------------------------+-------------+
```

- **The picture is exactly what is sent:** the whole 4:3 photo, shrunk to the left 162 × 122 pixels. The corner
  brackets mark its edges, and the crosshair marks the middle (Claude solves the problem nearest the middle
  unless you circle one).
- **= takes the photo** at full resolution (2048 × 1536, autofocus, sharpest of 4 frames), just as before.
- **AC** goes back to the calculator and **switches the camera off**.
- **30 s without a key** also switches the camera off, to save the battery. The normal AI SOLVE screen comes
  back. Press **▶** to start the viewfinder again (or = to take the photo straight away).
- **▲ ▼** (effort) and **1** (tutor) still work. The side panel shows them.
- **HOLD STILL** means the picture moved a lot since the last frame. **Focusing...** means the autofocus isn't
  done, or the writing is still soft. **✓ SHARP** means it's a good moment to press =.
- The **dashed box** is where it thinks the writing is. It's a cheap edge count, not real reading.

How it works:
- A separate task takes a small JPEG (320 × 240), decodes it at half size (160 × 120 gray), and drops a stale
  frame first so you see "now".
- The loop stretches the contrast, dithers it with a 4 × 4 Bayer pattern and sends it with a **partial refresh**.
  The Bayer pattern is stable from frame to frame, so only real changes flicker.
- Every 10th frame is a full refresh, which clears ghosting. So is any frame after a lot of pixels have
  flipped (a big scene change).
- Code: `core/viewfinder.*` (all image maths, tested on the PC), `firmware-prototype/src/main.cpp`
  (`serviceViewfinder`, `grabTask`), `camera.cpp` (`cameraPreviewFrame`), `screen.cpp` (`screenShowPanel`).
- Memory: the 2 MB PSRAM frame buffer is the one already sized for the photo, so nothing new is allocated
  there. The panel image is bit-packed (3.9 KB). The firmware uses 8 KB more RAM than before.

**Speed (estimate, measure on the board):**

| Step | Time per frame |
| --- | --- |
| Camera frame + decode (in parallel with the screen) | ~70–150 ms |
| Contrast stretch, dither, overlay | under 10 ms |
| E-paper partial refresh | ~300–400 ms (this is the limit) |

That gives about **2.5 frames a second**, or about 2 counting the full refresh every 10th frame (~1.5 s).
`fullEvery` in `core/viewfinder.h` trades ghosting for speed.

**Battery cost (estimate, measure it):**

| Part | Current |
| --- | --- |
| Camera streaming | ~35–45 mA |
| ESP32 working | ~40 mA |
| Wi-Fi joined to the hotspot (it's on in AI SOLVE) | ~15–25 mA |
| E-paper refreshing | ~5 mA |
| **Total** | **≈ 95–115 mA** |

That's about **1.6–1.9 mAh a minute, roughly 1.2 % of the 150 mAh cell (Adafruit #1317; 2 % of the earlier 100 mAh one) per minute.** The 30 s timeout keeps a
forgotten viewfinder to about 1 %.

The Serial Monitor prints the real numbers every 10 s:
`Viewfinder: 2.4 fps, sharpness 140, battery 3912 mV (-6 mV in 10 s)`.
For a true mA figure, put a meter in the battery lead.

**Needs tuning on the real board:**
- `kSharpMin` (when "✓ SHARP" shows) and `kMotionMax` (when "HOLD STILL" shows), in `core/viewfinder.h`.
  Watch the "sharpness" numbers in the Serial Monitor while you focus on a worksheet.
- `fullEvery` (ghosting vs speed). This panel's partial refresh may ghost more or less than expected.
- The camera's QVGA frame rate and exposure in dim light.
- Whether the picture needs mirroring (`hmirror`/`vflip` in the preview page's tuning) once the camera's real
  orientation in the case is known.
- The scan still waits its "settle" time (1.5 s) and the careful light-tuning after =. With a viewfinder you
  may prefer `careful 0` and a shorter `settle` (phone preview page, Save).

**Simulator:** the Windows simulator shows the same viewfinder on the AI SOLVE screen.
- **F7** picks a picture for its fake camera; without one it shows a made-up worksheet.
- **F8** switches the fake camera between steady, shaky and out of focus.

Golden-screen tests cover the overlay: `viewfinder_starting`, `_sharp`, `_moving` and `_soft`.

## Power facts

- Deep sleep: ON, or any key, wakes the chip. Plugging in the cable does **not** wake it, because VBUS_SENSE
  isn't an RTC pin, but charging works anyway.
- After waking for a key while off, the chip stays awake 4 s so SHIFT, 7, ON (exam mode) and
  SHIFT + ALPHA + ON (self-test) can be typed. Then it sleeps again.
- With the cable in, the chip stays awake while off (radios off) so the Serial Monitor keeps working.
- Memory kept in deep sleep lives in the chip's RTC memory. Pulling the battery clears it, like the Casio.
  Exam mode is also stored in flash, so pulling the battery doesn't end it.
- Battery % wrong compared with a multimeter? Set `kBatteryCal` in `firmware-prototype/src/power.cpp`
  to (meter mV) ÷ (`status` mV).

## Deploy the proxy (short version; full steps in `server/proxy/README.md`)

1. Make a free account on Render (or Railway / Fly.io). Create a **Web Service** from this repo, with root
   directory `server/proxy` (the Dockerfile is ready). Add a 1 GB disk mounted at `/data`.
2. In the service's **Environment** settings, add `ANTHROPIC_API_KEY` (your key: here and nowhere else) and
   `PUBLIC_URL` (the https address Render gives you). The other settings are listed in `.env.example`.
3. Open the service's **Shell** and run `python admin.py new-device "board 1"`. Copy the token.
4. Calculator: `proxy` → the address, `token` → the token, `pair` → code → `<address>/link`.

Switches you can change without new firmware:
- `MONTHLY_CAP` (500)
- `TRIAL_DAYS` (30)
- `FREE_DAILY` (free solves/day without a subscription; 0 = off)
- `ROUTING=haiku-first` (try Claude Haiku 4.5 first; the main model is Sonnet 5.5 when Haiku is unsure)
- `REFUSAL_FALLBACK` (server-side fallback when Sonnet 5.5 refuses a harmless photo)

Until payments are wired up, mark someone as paid with `python admin.py paid email 2026-12-31`.

## Files

| File | What |
| --- | --- |
| `hardware/pins_final.h` | every pin (single source), compile-time checks |
| `firmware-prototype/src/pins.h` | just `#include`s the file above |
| `firmware-prototype/tools/check_pins.py` | schematic vs header check (uses kicad-cli on a temp copy) |
| `firmware-prototype/src/power.*` | battery, charging, Wi-Fi cap, deep sleep, wake cause |
| `firmware-prototype/src/selftest.*` | factory self-test |
| `firmware-prototype/src/claude_client.*` | HTTPS to the proxy (solve, pair, account) |
| `firmware-prototype/src/settings.*` | hotspot, proxy URL, device token in NVS; erases any old API key |
| `firmware-prototype/src/keys.*`, `camera.*`, `screen.*`, `main.cpp` | wake handling, held keys, camera power order, picture kept through sleep |
| `core/battery.*` | LiPo curve, charge state (tested) |
| `core/viewfinder.*` | camera viewfinder: panel image, contrast stretch, Bayer dither, focus / motion / text box, overlay, full-refresh rule (tested, goldens) |
| `core/device.*` | `saveState()` / `restoreState()` for deep sleep, 1 = tutor on the AI screen |
| `core/app.*` | tutor mode, ✓/⚠ verification, account / low-battery messages |
| `core/claude_api.cpp`, `solve_result.*` | `check` field in the reply format; proxy error types |
| `server/proxy/` | the proxy (FastAPI + SQLite), admin tool, tests, Dockerfile, README |

The Windows and web simulators still talk to Claude directly with `api_key.txt` on your PC. That's fine for
development; the key file is git-ignored. Rebuild the web simulator (`sim/web/build.sh`) to get tutor mode
and ✓/⚠ there too.

## Bring-up checklist for the first board

1. Before the battery: USB only. Run `status` and then `selftest`, and save the JSON.
2. With a battery and **no cable**: switch off (SHIFT AC). Wait 10 s and measure the battery current with a
   meter in series. Target after review fixes S1/S2: **≤ 40 µA**. If it's ~330 µA, KEYPAD_INT is stuck low; if
   ~0.3–1 mA, check camera pins and CAM_PWR_EN.
3. ON wakes it, and so does a key (e.g. SHIFT). Store 7 into A (7 SHIFT RCL (−)), switch off, wait, switch on,
   then RCL A: it should still be 7.
4. AI SOLVE on battery: watch for brown-out restarts (`status` → "Last restart: brownout"). If they happen,
   lower `kWifiMaxTxQuarterDbm` in `power.h` (34 = 8.5 dBm).
5. Charging: plug in. `status` should say "(charging)", then "(USB, full)" when done.
