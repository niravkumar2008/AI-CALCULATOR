# Final firmware review (board v14, ESP32-S3-MINI-1-N4R2)

Date: 2026-10-06, 01:20–01:50 USEDT. Review stopped early by the coordinator (usage budget).
**Nothing in the repo was changed by this review itself** (see "Partial run"). **Every finding below was then
fixed in a second session the same day: see "Fixes applied 2026-10-06" at the end.**

## Verdict (one line)

**The board firmware in `firmware-prototype/` builds clean, every pin matches the schematic, and nothing found so far would stop the board coming up on day one; no board change is requested.**

## Board changes requested (for the PCB agent)

**None.** No strapping-pin conflict, missing pull-up or wrong divider was found. Everything below is firmware or documentation.

## Important correction to the review brief

The brief says `firmware/` is the board firmware and `firmware-prototype/` is older. It is the other way round:
- **`firmware-prototype/`** = the firmware for the custom board (ESP32-S3-MINI-1, OV5640, TCA8418, deep sleep, proxy). `firmware/FIRMWARE_STAGE13.md`, `hardware/bringup_guide.html` and `pins_final.h` line 16 all say so.
- **`firmware/`** = the breadboard tester (ESP32-S3-CAM dev board + 5 buttons, its own pins, still stores a Claude API key with the `key` command). It must **not** be flashed to the board.

This review covers `firmware-prototype/` + `core/` + `server/proxy/`.

## Findings

### FATAL
None found.

### SERIOUS

**S1. No way to update units in the field (no OTA).**
- Evidence: `firmware-prototype/platformio.ini:16` uses `huge_app.csv` (one app partition, 3 MB; no `app1`). Nothing in `src/` calls `Update`/`esp_ota_*`.
- Failure mode: every firmware fix for a sold unit needs the back cover off and the magnetic USB cable. Fine for day one, a problem for customers.
- Fix (proposed, not applied): switch to `min_spiffs.csv` (two 1.9 MB app slots; current build is 1.2 MB so it fits) and add a `/v1/firmware` download from the proxy with `Update.h`, only on USB power with battery > 3.8 V. About 80 lines; `firmware/README.md` and the proxy need matching notes.

**S2. Camera and Wi-Fi run at the same time in AI SOLVE, even on a nearly flat cell.**
- Evidence: `main.cpp:352-359` (`serviceWifi` wants Wi-Fi whenever the AI screen is up) and `main.cpp:435-457` (the viewfinder powers the camera on the same screen). `batteryOkForAi()` (`power.cpp:97`, 3.6 V lock-out) is only checked in `queueRequest` (`main.cpp:160`), i.e. after `=` is pressed.
- Failure mode: `hardware/verification/02_power_boot.md` C1 says camera + Wi-Fi join (uncapped PHY calibration burst) at ≈3.6 V can pull +3V3 to 2.8–3.0 V → brown-out restart just by opening MODE 4 on a low battery.
- Fix (proposed): in `serviceViewfinder` and `serviceWifi`, treat `!batteryOkForAi()` like exam mode (no camera, no Wi-Fi) and let the AI screen show "BATTERY LOW" (`core/app.cpp:238` already has the text). 4 lines.

**S3. Wi-Fi provisioning needs a PC.**
- Evidence: `settings.cpp:101-109`: SSID/password/proxy/token only over the USB Serial Monitor. The preview page (`preview.cpp:453`) makes an access point but has no Wi-Fi setup form.
- Failure mode: a customer can't enter a hotspot password on the keypad; Nirav must configure each unit over USB. OK for day one.
- Fix (proposed): a `/setup` form on the preview access point (SSID, password, proxy, token), reached from a SETUP menu entry that starts the AP and shows the network name + address on the e-paper.

**S4. A missing e-paper makes every screen draw take ~10–20 s (bench-only slowness).**
- Evidence: `pins_final.h:48` EPD_BUSY = IO33 input, no pull; `power.cpp` sets no pull; GxEPD2 waits for BUSY with a 10 s timeout per command. The bring-up guide flashes and runs `status`/`selftest` before the panel is attached.
- Failure mode: on the bench without the panel the first screen draw and the self-test "Screen pattern" step stall for tens of seconds and `display_ok` reads false (the guide says to expect false, so the test result itself is correct).
- Fix (proposed): `pinMode(PIN_EPD_BUSY, INPUT_PULLDOWN)` in `screenBegin` (SSD1680 BUSY is push-pull, so a weak pull-down is harmless). 1 line.

### MINOR

**M1. Stale recovery advice in `firmware-prototype/README.md`** ("hold the BOOT test pad to GND while plugging in"). With a battery fitted that does nothing (`02_power_boot.md` L1). The correct TP1 → TP4 hold + TP7 tap is in `FIRMWARE_STAGE13.md`, `pins_final.h` and the bring-up guide. Fix: copy the stage-14 text into the README.

**M2. Battery size inconsistency.** `hardware/HANDOFF.md` and `FINAL_STATUS.md` say Adafruit #1317 **150 mAh**; `power.h:1`, `core/battery.h:1`, `FIRMWARE_STAGE13.md`, `firmware-prototype/README.md` and the BOM say #1570 **100 mAh**. The firmware limits (11 dBm cap, 3.6 V lock-out) were tuned for 100 mAh, so they are conservative for 150 mAh: safe, just update the comments.

**M3. CAM_RESET driven push-pull 3.3 V into a 2.8 V net.** `camera.cpp:365-372` uses `pinMode(OUTPUT)`; `02_power_boot.md` C3 asks for open-drain (R8 10 k pulls RESET up to CAM_2V8). Pushes ~50 µA into the 2.8 V rail, within the OV5640 input rating (DOVDD + 1 V). Fix: `OUTPUT_OPEN_DRAIN` for RESET (esp_camera's own reset pulse stays push-pull; harmless for 20 ms).

**M4. `SerialCmd::SelfTest` case falls through into `case None`** (`main.cpp:751-754`). Harmless because `selfTestRun` is `[[noreturn]]`, but add `break;` for readers.

**M5. Polish wanted by Nirav (not done):** named constants for timing numbers (`kOffGraceMs` etc. are fine; `30000` battery refresh, `150` ms combo window, `12000`/`25000`/`120000` Wi-Fi waits in `main.cpp` are bare), a one-page `firmware/README.md` for build/flash/configure, consistent `[module]` serial log prefixes.

### NOTE

**N1. If the camera module ends up reversed, the firmware cannot tell beyond "Camera not found".** With the ribbon in backwards, pin 10 (DVDD 1.5 V from U5) lands on a GND finger: U5 is shorted while powered (it has current and thermal limits; the self-test powers the camera for a few seconds only, so the "try a 180° twist" step in the guide is survivable, but check U5 isn't hot). 2.8 V rails land on data GPIOs: no damage. A mirrored *image* (module the right way round but upside down) is handled by `hmirror`/`vflip` on the preview page (`camera.cpp:47-48`), saved to NVS.

**N2. Reading the `check` field / tutor mode**: `core/app.cpp` re-computes `check` with the engine and shows ✓ or "CALCULATOR GETS …"; refusal (`stop_reason == refusal`), `max_tokens`, mid-stream error and dropped connection each map to a distinct screen (`core/claude_api.cpp:231-288`). Looks right; not run on hardware.

## Every check and result

| # | Check | Result |
|---|---|---|
| 1 | `pins_final.h` vs schematic nets, all 32 pins (`python firmware-prototype/tools/check_pins.py`) | **PASS** – "All pins match the schematic" |
| 1 | TCA8418: address 0x34, INT IO4 (R15 10 k), RESET not wired (R16) → registers rewritten each boot (`keys.cpp:112-127`); KP_GPIO 0xFF/0xFF/0x03 = 8 rows × 10 cols; event code row×10+col+1 decoded (`keys.cpp:164`) | PASS |
| 1 | Keypad matrix `kMatrix` vs `keypad.kicad_sch` | PASS (verification 03 Table D and 05 OK7 confirm; CALC/∫dx rename only) |
| 1 | E-paper SPI pins CLK 5 / DIN 6 / CS 8 / DC 41 / RST 42 / BUSY 33, 4 MHz | PASS |
| 1 | Camera 16 pins + PWR_EN 34 (`kCameraVariants`, `kCameraGpios`) | PASS, unchanged as instructed |
| 1 | Battery divider 1 M/1 M → ×2, ADC1_CH8, 11 dB attenuation (`power.cpp:79,88`); robust average of 8 | PASS. (R17 100 k is the ON pull-up, not the divider.) |
| 1 | VBUS_SENSE IO37 digital (10 k/20 k) ; CHG_STAT IO35 read only while VBUS high, no internal pulls (`power.cpp:77-78,95`) | PASS |
| 1 | ON key IO7, R17 100 k, INPUT_PULLUP while awake, counts on release after 50 ms, EXT1 wake | PASS |
| 1 | Strapping pins 0/3/45/46: TP1 only / unconnected; no peripheral drives them; `pins_check::legal()` refuses them at compile time | PASS |
| 2 | Camera power order: RESET low, PWDN low, PWR_EN high, 5 ms, RESET high, 20 ms, then SCCB (`camera.cpp:365-373`) = review N3/C8 order; PWDN never driven high | PASS (M3: RESET push-pull) |
| 2 | XCLK 20 MHz LEDC; fb in PSRAM; 2048×1536 on 2 MB PSRAM (`g_lowMem`) | PASS |
| 2 | OV5640 AF: library `ESP32-OV5640-AF` 0.1.1 downloads focus firmware over SCCB (address from esp_camera, 0x3C), continuous AF, scan waits for FW_STATUS 0x10 up to 2 s; falls back to fixed focus if the download fails (`camera.cpp:88-99`) | PASS |
| 2 | Camera absent: "Camera not found. Check its ribbon is latched." on screen, regulators off, pins floated, retried at the next scan (`camera.cpp:410-413`) | PASS |
| 2 | Camera reversed / mirrored | N1 above |
| 3 | SSD1680 driver `GxEPD2_213_GDEY0213B74` (GxEPD2 1.6.9), BUSY active-high handled by the library, 2 ms reset, rotation 1 = 250×122 landscape, 125×61 framebuffer ×2 fills it exactly | PASS (orientation to confirm on glass: `setRotation(3)` if upside down) |
| 3 | Partial refresh every change, full every 30 partials or after wake (`screen.cpp:71-78`); viewfinder full refresh every 10th frame or big change; `hibernate()` after each draw (no burn-in, keeps picture unpowered) | PASS |
| 3 | Refresh-rate rule: draws are blocking (~0.3 s partial), so no faster than the panel allows; viewfinder bursts ≤ 30 s | PASS |
| 4 | Deep sleep: `ESP_EXT1_WAKEUP_ANY_LOW` exists for S3 in the installed SDK (arduino-esp32 2.0.17 / IDF 4.4.7, `esp_sleep.h:31`); wake on ON (IO7) and TCA8418 INT (IO4) only if INT is really high | PASS |
| 4 | What stays powered: TCA8418 (~3 µA), SSD1680 in deep sleep, camera regulators off via R10, USB pad off without cable, every camera pin disabled/no pull, I2C/EPD pins held; chip ~8 µA → board 21–34 µA (≤ 40 µA target) | PASS on paper; measure |
| 4 | Brown-out: TX capped at 44 × 0.25 = 11 dBm before `WiFi.begin` (`power.cpp:99-102`, `main.cpp:343`), modem sleep on, AI refused < 3.6 V on battery, reset reason "brownout" reported | PASS, but S2 |
| 4 | Charging state: `chargeState(vbus, vbus && stat)` → "(charging)" / "(USB, full)"; self-test `chg_stat_high`, `charge` | PASS |
| 5 | Self-test fields vs bring-up guide: firmware, device_id, memory_ok, keypad_scanner_ok, battery_mv, battery_pct, vbus, chg_stat_high, charge, display_refresh_ms, display_ok, camera, camera_jpeg_bytes, camera_ok, wifi_networks, wifi_best_rssi, wifi_ok, keys_ok, keys_missing, keys_all_ok, pass — all present (`selftest.cpp`) | PASS |
| 5 | Trigger: SHIFT + ALPHA held while ON (at boot, `main.cpp:688-695`, and while running, `main.cpp:238`) and `selftest` command; `q` skips the key test; 50 keys = `DKey::Count` incl. ON | PASS |
| 5 | Serial console: 115200, USB-CDC on boot (`platformio.ini:21`); with no PC attached writes are dropped, never block (HWCDC) | PASS |
| 5 | Recovery path documented TP1→TP4 + tap TP7; UART backup TP2/TP3; "never burn eFuses" in `pins_final.h:84` and docs | PASS (M1: README stale) |
| 6 | Proxy auth: `authorization: Bearer <device token>`; no API key anywhere in `firmware-prototype/` or `core/`; `token` command refuses `sk-ant-` keys; old key erased from NVS on first boot (`settings.cpp:157`) | PASS |
| 6 | Root CAs (`root_ca.h`): GTS R1/R4, ISRG X1/X2, DigiCert G2 + Global Root, Amazon Root CA 1 — covers Render/Railway/Fly/Cloudflare front ends; earliest expiry 2031 | PASS |
| 6 | TLS without NTP: `CONFIG_MBEDTLS_HAVE_TIME_DATE` not set in this SDK → no certificate date check, no clock needed | PASS |
| 6 | Timeouts: 15 s connect, 120 s idle; proxy retries upstream (`max_retries=2`); device shows "Claude is busy" on 429/5xx | PASS |
| 6 | Offline: "NO CONNECTION – Turn on your phone's hotspot." on screen; Wi-Fi failure reasons on serial | PASS |
| 6 | Model: proxy default `MAIN_MODEL=claude-sonnet-5-5` (`config.py:40`), optional Haiku-first; proxy rebuilds the request so the device's `kModel` is not trusted | PASS |
| 6 | Image: 2048×1536 JPEG q12 (~200–400 KB) ≤ Claude's 2576 px long edge, no server resize; `MAX_BODY_BYTES` 8 MB | PASS |
| 7 | Build: `pio run` in `firmware-prototype` → **SUCCESS**, RAM 30.4 %, flash 38.3 % (1.20 MB of 3 MB), 0 errors | PASS |
| 7 | NVS: `calc` (ssid, pass, proxy, token, exam, examMs), `cam` (tuning); RTC memory for calculator state | PASS |
| 7 | Watchdog: task WDT 5 s on CPU0 idle; JPEG decode yields every 50 ms (`camera.cpp:228`) | PASS |
| 7 | OTA | **S1** |
| 7 | Wi-Fi provisioning | **S3** |
| 7 | Exam mode: radios + camera off, saved every minute, survives battery pull; no LED on the board | PASS |
| 8 | Tester (`firmware/`) vs board firmware: tester still stores an API key (`key`), 15 dBm TX, no deep sleep, UART serial; `preview.cpp` is byte-identical in both. Nothing in the tester is newer than the prototype | PASS – nothing to carry over |

## First power-on script (day one)

1. **Before any ribbon or battery**, cable only: meter TP5 (+3V3) ≈ 3.3 V.
2. Open **`firmware-prototype`** (not `firmware`) in VS Code → PlatformIO → **Upload**. If no COM port: hold TP1 → TP4, tap TP7 → TP4, release TP1, Upload, tap TP7 again.
3. Serial Monitor **115200**. Type `help`, then `status`. Expect "Camera: off until the first scan", "Wi-Fi: not set", battery ~4200 mV (charger regulation, no cell).
4. Type `selftest`, then `q` at the key test. Save the `SELFTEST_JSON` line. Expect `memory_ok` true, `keypad_scanner_ok` true, `display_ok` and `camera_ok` false (nothing attached; with the S4 fix the screen step no longer stalls without a panel).
5. Unplug. E-paper into J2. Plug in, `selftest`: `display_ok` true, checkerboard visible. If the picture is upside down, change `setRotation(1)` to `3` in `screen.cpp:55`.
6. Unplug. Camera into J1 straight. Plug in, `selftest`: `camera_ok` true, `camera_autofocus` true. If false: reseat; then the 180° twist (watch U5 for heat).
7. Unplug. Battery into J4 (polarity metered). Plug in: `status` → "(charging)". Compare its mV with the meter on TP6; set `kBatteryCal` in `power.cpp:18` if off by more than 2 %.
8. `wifi` → hotspot name, password. `proxy` → `https://…` (the Render address). `token` → from `python admin.py new-device "board 1"` on the server. (Or, once the keypad is on: SETUP > 6 > = and do all three from a phone, S3 fix.) `pair` → code → open `<proxy>/link` on the phone.
9. MODE 4 (AI SOLVE), aim, `=`. Watch the serial log: photo size, "answer after … ms".
10. Unplug cable, SHIFT AC (off), wait 10 s, battery current in series: target ≤ 40 µA. ~330 µA = KEYPAD_INT stuck low; 0.3–1 mA = camera pins.
11. ON wakes it; `7 SHIFT RCL (−)`, off, on, `RCL A` = 7 proves RTC memory.
12. Full self-test with the keypad: hold SHIFT + ALPHA, press ON; press all 50 keys.

## Partial run — changes made

The review was stopped by the coordinator before any edit began.

- **Files changed: none.** (The `core/*`, `firmware-prototype/*` and other modifications in `git status` were already present when this session started.)
- **Build run:** `pio run` in `firmware-prototype` (success; the `.pio/` build output is git-ignored).
- **Not done:** the proposed fixes S2 (battery gate for viewfinder/Wi-Fi), S4 (BUSY pull-down), M1, M3, M4, the polish items (M5), OTA (S1), AP-based Wi-Fi setup (S3), and the "clear error screens" review of every message. All are listed with file:line above so they can be applied in a later session.

## Fixes applied 2026-10-06

Second session, same day. Scope: `firmware-prototype/` + `core/` + `server/proxy/` + docs. Not touched:
`hardware/kicad/*`, `firmware/` (tester) apart from a one-line README note, eFuses (never), and no API key
anywhere. Nothing was committed.

### Serious

| # | Fix | Where |
|---|---|---|
| S1 | **OTA with rollback.** Partition table `min_spiffs.csv` (two 1.9 MB app slots; NVS at the same offset as before, so settings survive the one USB flash). New `src/ota.{h,cpp}`: `GET /v1/firmware` manifest (version, size, sha256, path), streamed download through the existing TLS client (`proxyDownload` in `claude_client.cpp`), SHA-256 computed on the fly and compared with the manifest before `Update.end()`, written to the other slot. `verifyRollbackLater()` (C linkage) stops the Arduino core from confirming blindly; `main.cpp: serviceOtaConfirm()` calls `esp_ota_mark_app_valid_cancel_rollback()` 15 s after boot once the TCA8418 answers, so a crashing image is rolled back by the bootloader (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` is on in this SDK). Triggers: serial `update` (cable, or > 3.8 V), and automatic: switched off on a cable → 20 s later check + install (then daily). E-paper: `UPDATING … 45 %`, `UPDATE DONE`, `UPDATE FAILED` (old firmware keeps running). `status` and `SELFTEST_JSON` report the slot (`firmware_slot`, an added field; all fields in the bring-up guide are unchanged). | `platformio.ini`, `src/ota.*`, `src/claude_client.*`, `src/main.cpp`, `src/selftest.cpp` |
| S1 (server) | **Proxy `/v1/firmware` + `/v1/firmware/image`** (device token required; `x-firmware` header compared with the published version), `firmware.py` (publish / current / decide; checks the 0xE9 magic, size ≤ app slot, tampered image not served), `admin.py firmware <bin> <version>`, `FIRMWARE_DIR` (Docker: `/data/firmware`), README section. **6 new tests** (4 rule tests, 2 HTTP tests with `TestClient`). `python -m pytest test_proxy.py`: **15 passed** (was 9). | `server/proxy/firmware.py`, `app.py`, `admin.py`, `config.py`, `Dockerfile`, `.env.example`, `.gitignore`, `README.md`, `test_proxy.py` |
| S2 | **Camera and Wi-Fi gated together on the battery.** `main.cpp: serviceBattery()` keeps `g_aiBatteryOk` with hysteresis (off below 3.6 V on battery, back above 3.7 V, `kAiResumeMillivolts` in `core/battery.h`; a cable lifts it at once). `serviceViewfinder()` and `serviceWifi()` both require it, as do `queueRequest()` and the preview Send. `core/app.cpp` got `setLowBattery()`: the AI SOLVE screen reads **"Battery low: plug in the cable to use AI solve."** and = shows **BATTERY LOW** instead of starting a scan. | `src/main.cpp`, `core/app.{h,cpp}`, `core/device.h`, `core/battery.h` |
| S3 | **Setup without a PC.** New `src/setup_portal.{h,cpp}`: = on SETUP > 6 (or serial `setup`) scans for networks, starts `AI-Calc-xxxx` with an 8-character random password (both on the e-paper: `SETUP BY PHONE`), a captive portal (DNS wildcard + 302 for every other URL, so the phone pops the sign-in sheet) with a form: hotspot (dropdown of what was seen + free text), password, proxy URL, token (empty = keep). Validation shared with the serial path (`settingsCheckWifi/Proxy/Token`), same NVS keys (`settingsSave`). Saved settings are handed to the loop under a mutex and applied; the network closes 3 s after the "Saved" page; AC or 10 min ends it; exam mode refuses it. The `token` rule still refuses `sk-ant-` keys. | `src/setup_portal.*`, `src/settings.*`, `src/main.cpp` |
| S4 | `pinMode(PIN_EPD_BUSY, INPUT_PULLDOWN)` before `g_epd.init()` and `gpio_pulldown_en()` after it (GxEPD2's `init()` re-runs `pinMode(INPUT)`, which would have cleared the pull). | `src/screen.cpp` |

### Minor

| # | Fix |
|---|---|
| M1 | `firmware-prototype/README.md` rewritten: build, flash, the TP1→TP4 hold + TP7 tap recovery (and the UART backup on TP2/TP3, and the eFuse warning), first setup (phone and serial), using it, OTA, serial log. `firmware/README.md` now starts with a one-line note that it is the tester and points to `firmware-prototype/`. |
| M2 | #1570 / 100 mAh → Adafruit #1317 / 150 mAh (charge 50 mA) in `power.h`, `core/battery.h`, `main.cpp`, the README and `FIRMWARE_STAGE13.md` (with a note that the 11 dBm cap and 3.6 V lock-out were tuned for 100 mAh and are conservative now). `hardware/bom` was left alone (hardware's file). |
| M3 | `PIN_CAM_RESET` is `OUTPUT_OPEN_DRAIN` before the power-up sequence and set back to open-drain right after `esp_camera_init()` (which pulses it push-pull for 20 ms), so R8 holds it at 2.8 V afterwards instead of the pin pushing 3.3 V. |
| M4 | `break;` after `SerialCmd::SelfTest`. |
| M5 | Named constants: `main.cpp` has a timing block (`kBatteryRefreshMs`, `kSolveWifiWaitMs`, `kSerialWifiWaitMs/HoldMs`, `kSendWifiWaitMs`, `kSendSolveTimeoutMs`, `kViewfinderLogEveryMs`, `kOta*`, `kSetupAfterSaveMs`, …); `camera.cpp` (`kCamPowerSettleMs`, `kCamResetReleaseMs`, `kFocusWaitMs`), `selftest.cpp` (thresholds and screen timings), `settings.cpp` (length limits), `power.cpp` (`kBatterySamples`, `kDividerRatio`), `screen.cpp` (`kEpdSpiHz`). Serial log: `src/log.h` `LOGF("module", …)` → every line is `[keys] …`, `[wifi] …`, `[cam] …`, `[solve] …`, `[ota] …`, `[setup] …`, `[power] …`, `[preview] …`, `[selftest] …`; `SELFTEST_JSON` and the `status` block are unchanged. |
| Polish | E-paper messages: AI SOLVE home now says **"Point the camera at the problem and press ="**; not set up → "Not set up yet: SETUP 6 shows how (or USB help)"; low battery as in S2; off on a cable → **"Charging 64%"** (there is no charge LED); `SETUP BY PHONE` screen; update screens; NetInfo's last line is "=: set up from a phone". Offline screens unchanged (`No connection` / `NO CONNECTION`). |

### Build and test results

- `pio run` (firmware-prototype): **SUCCESS**, 0 errors, 0 warnings in our code. RAM 30.7 % (100,440 B), flash **1,245,569 B = 63.4 % of the 1,966,080 B OTA slot** (was 1,204,885 B in a 3 MB single slot; +40 KB for OTA, portal, DNS, SHA-256).
- `python -m pytest test_proxy.py` (server/proxy): **15 passed** (9 existing + 6 new).
- PC tests of `core` (`tests/tests.cpp`): **548 passed, 0 failed.** No GCC on this machine, so they were compiled with the MSVC Build Tools plus a 2-file shim outside the repo (`__builtin_*_overflow`, `dirent.h`); nothing in `core/` or `tests/` was changed for that. Three goldens were regenerated (`UPDATE_GOLDEN=1`) because the AI SOLVE home text changed on purpose: `ready.txt`, `ready_offline.txt`, `ai_ready.txt` (the diff is the text rows only).
- Not run on hardware (the board is not built yet).

### Still open (not firmware)

- Server review S1–S5 (default model/fallback, per-device rate limit, `/link` sign-in, billing) are server-side and were out of this scope.
- The bring-up guide's sample `SELFTEST_JSON` line predates `firmware_slot`; every field it names is still produced.
