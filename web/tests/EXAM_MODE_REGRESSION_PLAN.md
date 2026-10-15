# Exam mode regression plan (stage 2, `tests/tests.cpp`)

PLAN.md section 5 says exam mode "stays exactly as it is", and every new provisioning
screen must respect it. Stage 2 (firmware-agent) adds a Wi-Fi sub-menu, SETUP 9 Account,
register/pair screens and automatic OTA. This file lists the behaviour to pin **before**
those changes land, so a regression fails `tests.cpp` instead of reaching a classroom.

Source read: `core/device.h`, `core/device.cpp` (commit 48d422a), `tests/tests.cpp`
`testExam()` (lines ~696-743) and the state round-trip checks (~lines 900-912),
`firmware-prototype/src/settings.cpp` (serial `exam off` -> `SerialCmd::ExamOff`) and
`main.cpp` (`if (g_dev.examActive()) g_dev.usbUnlock();`).

## A. Behaviour that exists today (pin it; most is already covered)

| # | Behaviour | Where in core | Covered today? |
|---|---|---|---|
| A1 | SHIFT, 7, ON (in that order, while off) powers on in exam mode | `Device::onKey`, `offSequence_` | yes (`testExam`) |
| A2 | 7 then ON without SHIFT is a plain power-on | same | yes |
| A3 | SHIFT, 7, *other key*, ON does **not** start exam mode (sequence resets) | `offSequence_ = 0` on other keys | **no** -> add |
| A4 | SETUP (SHIFT MODE) 8 opens ExamInfo; `=` starts exam mode | `View::ExamInfo` | yes |
| A5 | In ExamInfo while exam is on, `=` does not restart the timer | `!exam_` guard | **no** -> add |
| A6 | `radiosAllowed()` is false while exam is on, true after | `radiosAllowed()` | yes (start), **no** (after end) -> add |
| A7 | `setOnline(true)` is ignored while exam is on (`ai().online()` stays false) | `setOnline` | partly -> add explicit check |
| A8 | MODE 4 (AI SOLVE) is refused with the "AI SOLVE IS OFF" notice; mode stays COMP | `ModeMenu` case 4 | yes |
| A9 | `startScan()` (the camera key path) returns false and starts no request | `startScan` | **no** -> add |
| A10 | Starting exam mode while in AI mode leaves AI and cancels a request in flight | `startExam` -> `leaveAi()` | **no** -> add |
| A11 | Ends by itself after 12 h of awake time (`kExamMaxMs`) | `tick` | yes |
| A12 | 12 h counts across sleep: `restoreState(s, asleepMs)` resumes with elapsed + asleep, and ends it if that reaches 12 h | `restoreState` / `restoreExam` | yes (~900-912) |
| A13 | Power off / on keeps exam mode | | yes |
| A14 | CLR 3 (All) and CLR 1 (Setup) do **not** end exam mode | `ClrConfirm` comment | All: yes; Setup: **no** -> add |
| A15 | `usbUnlock()` (the serial `exam off` command over the cable) ends it with a notice | `usbUnlock` | yes |
| A16 | `usbUnlock()` when exam is off does nothing (no notice) | `endExam` guard | **no** -> add |
| A17 | Status bar shows the padlock + clock while exam is on (golden `exam_status_bar`) | `drawStatus` | yes (snapshot) |
| A18 | There is no VBUS-based unlock: plugging the cable alone does not end exam mode | (no such code) | **no** -> add once core gets a `setOnCable()`/charging input |

## B. New in stage 2: must all be blocked while exam mode is on

| # | New entry point | Required behaviour in exam mode |
|---|---|---|
| B1 | SETUP 6 "Wi-Fi" sub-menu (1 Type on keys, 2 Set up with phone, 3 Forget Wi-Fi) | Opens the existing "Off in exam mode." screen (golden `exam_wifi_off`) or a notice. None of the three items is reachable; no scan, no SoftAP, no TextEntry view. Forget Wi-Fi must not run either (it is harmless, but "no Wi-Fi actions" is the simpler rule to test). |
| B2 | TextEntry (multi-tap password entry) | Unreachable. If it was open when exam mode started (impossible today: exam starts from SETUP 8 or power-on), it closes. |
| B3 | SETUP 9 "Account" (shows masked e-mail, plan, solves left; 1 = Unlink and get new code) | Notice "Off in exam mode." No network call, and "Unlink" is unreachable. |
| B4 | Register / pairing screens reached from MODE 4 = (no Wi-Fi / not registered / not linked / no plan) | MODE 4 is refused before any of these steps, so the pairing screen, the QR and the 5 s status polling never start. |
| B5 | Automatic OTA (check after connecting, at most daily; install only on cable or >= 3.8 V; never during a solve) | Never checks and never installs while exam mode is on (radios are off anyway, but the install decision must also say no). If an update was downloaded before exam mode began, it is not applied until exam mode ends. |
| B6 | Any new menu shown on the e-paper | `radiosAllowed()` is consulted, not a copy of the flag. |

The platform side (`firmware-prototype`) must also gate the actual radio calls on
`radiosAllowed()`; core tests can only pin the decisions core makes. The proposal below
therefore asks core to own the decisions (`wantsNetwork()`, `otaInstallAllowed(...)`) so
they are testable without hardware.

## C. Proposed test code (tests.cpp style)

Uses the existing helpers `keys(d, {...})`, `snapshot(name, d)`, `CHECK(...)`, `K::...`,
`View::...`. Names marked *(new)* are the APIs/views firmware-agent is about to add; adjust
the names to whatever they land as, but keep the assertions.

```cpp
// A3, A5, A6, A7, A9, A10, A14, A16: gaps in today's testExam().
static void testExamEdges() {
  {  // A3: SHIFT, 7, another key, ON is a normal power-on.
    Device d;
    d.tick(0);
    keys(d, {K::Shift, K::AC, K::Shift, K::D7, K::D1, K::On});
    CHECK(!d.examActive() && !d.isOff());
  }
  {  // A10: starting exam while solving cancels the request and leaves AI.
    Device d;
    d.tick(0);
    d.setOnline(true);
    keys(d, {K::Mode, K::D4, K::Eq});
    CHECK(d.takeRequest() != 0 && d.activeRequest() != 0);
    keys(d, {K::AC, K::Shift, K::Mode, K::D8, K::Eq});
    CHECK(d.examActive() && d.activeRequest() == 0 && d.mode() == Mode::Comp);
    // A7: going online is ignored while exam is on.
    d.setOnline(true);
    CHECK(!d.ai().online() /*(new) trivial getter: App has setOnline() but no online() today*/);
    // A9: the camera key path refuses.
    CHECK(!d.startScan() && d.activeRequest() == 0);
    // A5: ExamInfo '=' again does not restart the clock.
    d.tick(60000);
    keys(d, {K::AC, K::Shift, K::Mode, K::D8, K::Eq});
    CHECK(d.examElapsedMs() == 60000);
    keys(d, {K::AC});
    // A14: CLR 1 (Setup) keeps exam mode too.
    keys(d, {K::Shift, K::D9, K::D1, K::Eq, K::AC});
    CHECK(d.examActive());
    // A6: radios come back after the cable unlock.
    d.usbUnlock();
    CHECK(!d.examActive() && d.radiosAllowed());
    keys(d, {K::AC});
    // A16: a second unlock is a no-op (no notice).
    const View before = d.view();
    d.usbUnlock();
    CHECK(d.view() == before);
  }
}

// B1-B6: every stage-2 entry point is blocked in exam mode.
static void testExamBlocksProvisioning() {
  Device d;
  d.tick(0);
  d.setOnline(true);
  keys(d, {K::Shift, K::Mode, K::D8, K::Eq, K::AC});
  CHECK(d.examActive());

  // B1: SETUP 6 Wi-Fi sub-menu: no item reachable.
  keys(d, {K::Shift, K::Mode, K::D6});
  snapshot("exam_wifi_off", d);                    // same golden as today
  for (DKey k : {K::D1, K::D2, K::D3, K::Eq}) {
    keys(d, {k});
    CHECK(d.view() != View::WifiMenu /*(new)*/ && d.view() != View::TextEntry /*(new)*/);
    CHECK(!d.wantsNetwork() /*(new): core's "please scan / start SoftAP / connect" request*/);
  }
  keys(d, {K::AC, K::AC});

  // B3: SETUP 9 Account: notice, and "1:Unlink" does nothing.
  keys(d, {K::Shift, K::Mode, K::D9});
  CHECK(d.view() == View::Notice || d.view() == View::Account /*(new) showing "Off in exam mode."*/);
  keys(d, {K::D1});
  CHECK(!d.unlinkRequested() /*(new)*/);
  keys(d, {K::AC, K::AC});

  // B4: MODE 4 = never reaches registration or the pairing screen.
  keys(d, {K::Mode, K::D4, K::Eq});
  CHECK(d.view() != View::Pairing /*(new)*/ && d.mode() == Mode::Comp);
  CHECK(d.takeRequest() == 0 && !d.wantsNetwork());
  keys(d, {K::AC});

  // B5: OTA install decision says no in exam mode, even on cable with a full battery.
  CHECK(!d.otaInstallAllowed(/*onCable=*/true, /*vbat=*/4.2f) /*(new)*/);
  d.usbUnlock();
  keys(d, {K::AC});
  CHECK(d.otaInstallAllowed(true, 4.2f));
  CHECK(!d.otaInstallAllowed(false, 3.7f));        // D5: battery >= 3.8 V or cable
  keys(d, {K::Mode, K::D4, K::Eq});                // a solve in flight blocks install
  CHECK(!d.otaInstallAllowed(true, 4.2f));
}

// A12 extended: exam survives a sleep/restore *with the new stage-2 state fields* in the blob.
static void testExamStateRoundTripStage2() {
  Device x;
  x.tick(0);
  keys(x, {K::Shift, K::Mode, K::D8, K::Eq, K::AC});
  x.tick(11 * 3600 * 1000u);
  const std::string s = x.saveState();
  Device y;
  CHECK(y.restoreState(s, 30 * 60 * 1000) && y.examActive() && !y.radiosAllowed());
  Device z;
  CHECK(z.restoreState(s, 61 * 60 * 1000) && !z.examActive() && z.radiosAllowed());
}
```

Register the three functions in `main()` next to `testExam()`. The goldens they create
(none new above except reuse of `exam_wifi_off`) go in `tests/golden/`.

## D. Manual checks (hardware, stage 2 MANUAL_STEPS)

1. SHIFT 7 ON on the board: Wi-Fi LED/scan stops (check with a phone's Wi-Fi analyser that
   the SoftAP SSID `AI-Calc-XXXX` is gone) and the camera does not power up on the scan key.
2. With exam on, connect USB without sending `exam off`: exam mode stays on.
3. Send `exam off` over serial: notice "EXAM MODE OFF / Unlocked by USB cable."
4. Publish a new firmware, start exam mode, connect to Wi-Fi before (not possible during)
   and confirm no OTA happens until exam mode ends.
