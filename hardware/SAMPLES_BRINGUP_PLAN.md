# Samples bring-up plan: from 5 boards in a box to 5 working, filmed, enrolled samples

Written 2026-10-11 for the **v15-LCD** board (silk "AI CALC v15-LCD 2026-10-08") and firmware `firmware-v15-lcd/` (version `v15lcd-2026.10.10`). Companion: `production/SCALE_UP_PLAN.md` (what happens after the 5 samples).

**What this file is:** one plan that strings together the steps that already exist in other files, adds go/no-go gates, a log template, what to film, and a day-by-day schedule. It does not replace the step-by-step guides: use the **v15 Power-Up guide** (PU15, `bringup_guide_v15_lcd.html`, https://claude.ai/artifact/BeQnacuLhchaHz492CnLRT) and the **v15 Assembly guide** (AS15, `enclosure/final_assembly_v15_lcd/assembly_guide_v15_lcd.html`, https://claude.ai/artifact/FU8YVqaksDPk2cfiD2cz3f) at the bench.

**What is NOT known (don't guess):** the v15 order's vendor, quantity of PCBs and assembled boards, total paid and delivery date are **not recorded** (`Claude outputs/BUSINESS_STATUS_2026-10-11.md`, `ORDER_WALKTHROUGH_v15_lcd.md` banner). The planning figure everywhere is "JLCPCB, 5 PCBs, 5 assembled, ≈ $270 delivered (estimate)". This plan therefore counts days from **Day 0 = the day the boards arrive**. If fewer than 5 assembled boards arrive, cut the plan to that number.

---

## 0. Before Day 0 (while you wait)

Do these now; every one of them shortens the schedule later.

| # | Task | Why / source | Done |
|---|---|---|---|
| 0.1 | **Record the v15 order** (vendor, qty, total incl. shipping and duty, order date, expected delivery) in `ORDER_WALKTHROUGH_v15_lcd.md` §8 | Every cash number uses the $270 estimate until you do (`BUSINESS_STATUS` action 1) | [ ] |
| 0.2 | **Panels: you need 5 + 1 spare ER-TFT019-1 (no touch).** The walkthrough only said "order 3" (`ORDER_WALKTHROUGH_v15_lcd.md` §0); check what was bought | `BUSINESS_STATUS` action 2 | [ ] |
| 0.3 | **Other sample parts × 5 (+1 spare where cheap):** OV5640 **autofocus** camera for XIAO ESP32S3 Sense (Seeed 114993115), Adafruit #1317 150 mAh LiPo, #5358 magnet piece, #5412 magnet cable (or Yiwei MG04254FRA1S1N + MG0425-UB-60-4P) | `ARRIVAL_CHECKLIST.md` §1; budget row "Parts for 5 v15 samples" $369.60 (`production/UNIT_ECONOMICS.md` §8.5, estimate) | [ ] |
| 0.4 | **Shells: 5 genuine Casio fx-115ES of the same edition** (you own 2; check the back labels match) | `ARRIVAL_CHECKLIST.md` §1. Samples only: never sold, and the Casio logo is **never shown on camera** (`production/KNOCKOFF_SHELL_PLAN.md` §5) | [ ] |
| 0.5 | **Print the jigs** A, A2, A3, B (C only for the battery upgrade) from `enclosure/jigs/` (PETG/PLA, 0.2 mm layers, 4 walls) | Cuts grinding from ≈ 25–34 min to ≈ 9–11 min per shell (`enclosure/jigs/README.md`) | [ ] |
| 0.6 | **Pre-grind all 5 shells** (AS15 step 2): solar box (zone 1), rib B (zone 2), magnet notch (zone 5, + 5b lip if present), side-wall relief (6/6b). **Hold the 7 mm camera hole (zone 3)** until each camera ribbon is measured | `ARRIVAL_CHECKLIST.md` §2 | [ ] |
| 0.7 | **Cut 5 window masks** from `enclosure/final_assembly_v15_lcd/window_mask_template_v15.svg` (opening 43.72 × 23.70 mm) and print the LCD position jig `lcd_position_jig_v15.svg` at 100 % | `stage15_lcd.md` §9 item 4; `REPORT.md` §7 | [ ] |
| 0.8 | **Supplies:** 0.1 mm double-sided tape (thin film, **not foam**), Kapton, matte black vinyl, 99 % IPA, 7 mm + 2 mm drill bits, loupe, multimeter with a µA/mA range, an inline USB power meter, **one JST-PH 2-pin extension lead** you can cut for battery-current readings (§7) | `ARRIVAL_CHECKLIST.md` §1, `production/QA_TEST_PLAN.md` Part B | [ ] |
| 0.9 | **AI server ready.** Today the firmware's default is the **Python proxy** (`server/proxy`); the Cloudflare **Worker** is built but **not deployed** (`BUSINESS_STATUS`). Have one proxy device token per board ready (`python admin.py new-device "sample 1"`, `server/proxy/README.md`). The Claude API key lives only in the server's environment, never in the calculator, git or a chat | `firmware-v15-lcd/README.md` "Server: proxy or Worker" | [ ] |
| 0.10 | **PC ready:** VS Code + PlatformIO opens `firmware-v15-lcd`, `pio run` builds clean; create `production/qa_log.csv` (`QA_TEST_PLAN.md` header) and one copy of the §9 log per sample | | [ ] |
| 0.11 | **Make the focus target card** (`QA_TEST_PLAN.md` B4) | Needed for the photo-solve test | [ ] |
| 0.12 | **Filming kit:** phone tripod or overhead arm, a plain mat, a strip of matte black vinyl to **cover the Casio logo** on every shell before it goes near a camera | `CONTENT_CALENDAR.md` "Every post" rule | [ ] |

---

## 1. Day 0: arrival inspection (all boards, unpowered, ≈ 5 min each)

Source: `ARRIVAL_CHECKLIST.md` §4, PU15 step 2, `production/QA_TEST_PLAN.md` Part A1–A3.

- [ ] Box and anti-static bags not crushed; count the boards and write the real count in the order record.
- [ ] Silk reads **"AI CALC v15-LCD 2026-10-08"**.
- [ ] **J5** (30-pin LCD socket) sits right of the camera area, opening towards the 1 × 20 mm slot; Q4 below it, Q5 and R23 above it.
- [ ] **R23 is marked "15R"/"150" (15 Ω), not 22.**
- [ ] **D7** band on the pad towards J3 (backwards, it shorts the charging cable: `ORDER_WALKTHROUGH_v15_lcd.md` §5).
- [ ] U1 antenna overhangs the left edge, J3 overhangs the top edge (intended).
- [ ] Under the loupe: no bridges on U1, U6 (TCA8418), U3; no tombstoned passives; compare against JLCPCB's placement photos.
- [ ] Meter, board alone (QA A3 rows 16–25): TP5→TP4 **> 1 kΩ** (wait 5 s); TP6→TP4 **> 10 kΩ**; J3 pin 1→TP4 > 10 kΩ, no beep; J3 pin 4→TP4 beeps; TP7 and TP1 → TP4 > 10 kΩ; J1 fingers 2 and 15 → TP4 beep.
- [ ] **Any fail in rows 16–21: don't power that board.** Photograph it and claim with the vendor within their window (QA A3).
- [ ] Panels: glass not cracked at the corners, tail not creased at its root, 30 fingers, **no touch layer**. Store glass-down on a cloth.
- [ ] Number the boards **S1–S5** with a paint pen on the back, away from pads; start one log per board (§9).

**Film (M2, calendar idea #4 "The boards arrived"):** unboxing the JLCPCB box, a slow pan over one board, a macro of the silk. No prices shown unless blurred.

**Gate M2 (go/no-go):** GO if ≥ 3 boards pass A1–A3. With 1–2 good boards, continue with them and file the claim for the rest. With 0, stop and contact the vendor.

---

## 2. Board S1 first: power, flash, self-test (Day 1)

One board at a time until S1 has passed everything through §4. Then the others go faster.

### 2.1 Pre-power checks (PU15 steps 3–5)
- [ ] **Magnet piece meter check** with the real cable before any gluing (QA B6 table): outer legs +5.0 ± 0.25 V; inner legs 0–0.6 V; mark the leg for J3 pin 1 (VBUS).
- [ ] **Battery polarity:** red wire on J4 "+"; cell reads about 3.7–4.0 V; pouch flat.
- [ ] **Camera ribbon beep test** (fingers 2 ↔ 15) and measure module far edge → tip. Under about 70.3 mm: the module shifts up to 1.7 mm towards J1; drill the camera hole where the lens really sits.
- [ ] **LCD tail diode test on the first panel, before plugging it in:** meter on diode, red on finger 20, black on finger 22 (the 11th and 9th fingers from the pin-30 end) → an LED reading. Mark the finger-1 corner. Plugged in with no twist, finger 1 must land at the **J5 silk tick (bottom end)**. Otherwise a half twist of the tail fixes it; nothing is damaged.
- [ ] **Tail length** glass edge → tip: expect **36.6 ± 0.3 mm**. Over about 37.0 mm: rib B needs about 15 mm more grind towards the slot side (`REPORT.md` §4).

### 2.2 First power (PU15 step 6, QA A4)
Computer USB port only (it cuts the current fast on a fault). **No battery, no panel, no camera.**
- [ ] 5 V current **5–60 mA** idle; **> 150 mA = unplug at once**.
- [ ] TP5 **3.25–3.35 V**; TP7 > 2.5 V.
- [ ] COM port appears within 5 s.
- [ ] After 60 s, U2–U5 warm at most (too hot to hold 3 s = reject).

### 2.3 First flash (PU15 step 7) — always over USB
- [ ] VS Code → Open Folder `firmware-v15-lcd` → **Upload** (or `pio run -t upload`). **Never flash `firmware-prototype/` to a v15 board.** Never burn an eFuse.
- [ ] Serial Monitor 115200 → `status` shows firmware `v15lcd-2026.10.10`, slot `app0`, an id like `calc-…` (write it in the log: this is the hw_id used for enrolment later).
- [ ] No COM port / flashing fails: **recovery** = short **TP1 (BOOT) to GND (TP4)**, tap **TP7 (EN)**, release, flash again (`firmware-v15-lcd/README.md` "Build and flash").

### 2.4 Factory self-test, nothing attached (PU15 step 8)
- [ ] `selftest` (`q` skips keys). Save the whole `SELFTEST_JSON` line in the log. Expect `memory_ok`, `keypad_scanner_ok` true; camera / LCD fields fail because nothing is attached yet. That is normal at this step.

### 2.5 Attach LCD, then camera, then battery (PU15 step 9). Cable out before every ribbon.
- [ ] LCD into J5 → `selftest`: colour bars, grey ramp, white border, device id, then the backlight sweeps 0 → 100 %. JSON: `lcd_ok` true, `lcd_push_ms` < 200, `backlight_ok` true (`firmware-v15-lcd/README.md` "Self-test"). **The panel cannot answer back, so look at it.**
- [ ] Camera into J1 → `selftest`: `camera` contains "OV5640", `camera_jpeg_bytes` > 1,024, `camera_ok` true, **`camera_autofocus` true** (QA B2 step 5).
- [ ] Battery into J4 → `status`: battery 3,000–4,300 mV; cable on → `vbus` true and "charging"/"full".

**Film (M3, idea #5 "First power-on. Will it boot?"):** the moment the colour bars appear and the backlight sweep. If it fails, the calendar says post the failure honestly.

**Gate M3 (go/no-go):** GO when S1 flashes over USB, `memory_ok` and `keypad_scanner_ok` are true, and the colour bars show. NO-GO → §8 (what to do if a board fails).

---

## 3. Panel bench checks on S1 (Day 1–2)

From `firmware-v15-lcd/PORT_NOTES.md` "Needs the real panel to verify" (items 1–11) and its "Bench checks for this entry", plus `ARRIVAL_CHECKLIST.md` §4. Each check has the fix next to it; most fixes are a build flag in `firmware-v15-lcd/platformio.ini` or `hardware/kicad_v15_lcd/pins_v15_lcd.h`, then re-flash.

| # | Check | How | Pass | If not |
|---|---|---|---|---|
| P1 | Orientation / offset | Look at the self-test border | Border unbroken on all 4 sides, no 35-px noise strip, picture the right way up | Flip `LCD_ROTATION` 1 ↔ 3; strip at one edge: try `offset_x 0` + `memory_width 170`, or 35 on the other side via `offset_rotation` |
| P2 | Colours / inversion | Colour bars | Red, green, blue in the right order; not a washed-out negative | Negative-looking: flip `LCD_INVERT`. Red/blue swapped: flip `LCD_RGB_ORDER` |
| P3 | Calculator palette | COMP screen | Grey-green "LCD glass" #C7D1B5 with near-black ink looks like a calculator, readable at arm's length | Tweak the palette for the IPS panel's gamma (`PORT_NOTES` "Bench checks for this entry") |
| P4 | Preview byte order | MODE 4 → 1 AI Solve → viewfinder | Viewfinder colours natural while the bars are right | Set `PREVIEW_SWAP_BYTES=1` |
| P5 | SPI at 40 MHz | Viewfinder and menus for 2 min | No tearing, no noise | Drop `LCD_SPI_HZ` to 26.6 or 20 MHz in `pins_v15_lcd.h` (full frame then 33 / 44 ms) |
| P6 | Panel at 3.3 V | Look + meter | Clean picture, no flicker; panel IDD ≤ 20 mA | Note it; it is a next-revision item, not a stop |
| P7 | **Backlight current** | `bright 100`, meter mV across R23, ÷ 15 | **16–37 mA (≈ 26 typical)**; it cannot exceed the 60 mA rating | < 16 mA: check Q5/R23 joints and the tail's LED fingers 20–24 |
| P8 | Backlight low end | `bright 10` | No visible flicker | Raise the minimum duty in `dutyFor` (`PORT_NOTES` item 6) |
| P9 | Panel wake | Let it sleep (`screen sleep 30`), press a key | No white flash | Keep the backlight at 0 until the first frame after `wake()` (`PORT_NOTES` item 7) |
| P10 | **Map frame time** | Open MODE 4 → 5 Geography; log `millis()` around `ui::showDevice` on the map (one temporary log line) | World view redraw **≤ 40 ms** total (estimate was 10–20 ms draw + 22 ms push) | Note the number; a slow map is a firmware task, not a board fault |
| P11 | **LittleFS mount time on a key wake** | Save a note, SHIFT AC (off), press ON; read the boot log timing around `LittleFS.begin` | Record ms; first-ever boot formats once (~1 s) and is excluded. Wake should still feel instant (screen redraw ≈ 150 ms) | If it adds a noticeable delay, note it for the firmware list |
| P12 | Camera mode switch | Viewfinder → `=` scan; read `[cam] scan (JPEG) mode in N ms` and `preview … in N ms` | Record both; set `kModeSwitchMs` in `camera.cpp` to the measured value. Log shows "autofocus on"; ▶ really moves the lens | If single-shot AF doesn't hold, use continuous (0x04) only (`PORT_NOTES` item 8) |
| P13 | Brown-out worst case | Viewfinder at `bright 100` while Wi-Fi joins | No `ESP_RST_BROWNOUT` in the boot message | Set the backlight to 25 % while Wi-Fi is not connected (`PORT_NOTES` item 9) |
| P14 | UI legibility | Walk every menu and app once | 2x menu text readable; **no cell drawn as raw tiny pixels** (an unrecognised glyph) | Note the screen; `kTextScale` in `ui.cpp` is the one knob |
| P15 | Link screen | Settings → Link calculator | The ~1–2 s blocking fetch is acceptable | Note it |
| P16 | Sleep current | Off (SHIFT AC), meter in series with the battery (§7) | ≤ **40 µA** board total; the LCD block adds < 2 µA | Check IO33 floats (R21) and the 5 SPI lines read 0 V in sleep |
| P17 | Moved keys | Key test in `selftest` | SHIFT, ALPHA, ON feel and register like the rest | Note it; pads were moved in review 14 (`REPORT.md` §5) |

**Freeze the flags.** When P1–P5 pass, write the four flag values into this file's log and keep them for every board (the panels are the same part). Re-check P2 on the first panel from any **other** supplier.

---

## 4. Wi-Fi, server, linking and real solves (Day 2–3)

### 4.1 Phase A: on the proxy (works today)
- [ ] `wifi` (hotspot name + password), `proxy` (https address), `token` (the device token for this board), or the phone page: `setup`, or on a closed calculator **SHIFT MODE, 6, =** (`ARRIVAL_CHECKLIST.md` §3 step 9).
- [ ] `status`: Wi-Fi connected, server reachable.
- [ ] `pair` → a code → link it on `<server>/link` with **your own adult account** (the free month starts at first link; `BUSINESS_STATUS`).
- [ ] `account`: linked, plan state, solves left.

### 4.2 Phase B: on the Worker (once it is deployed; software stage 2 in `BUSINESS_STATUS`)
- [ ] `api worker https://api.<domain>` (a new address drops the old token). The board registers itself on its first Wi-Fi connection.
- [ ] **Enrolment (QA step, security review S1):** on the PC, with `ADMIN_TOKEN` set **only as an environment variable** (never in a file, a chat or git): `node web/api/scripts/enroll-board.mjs calc-xxxxxxxxxxxx https://api.<domain>` (the script lives in the launch-site worktree, `web/api/scripts/enroll-board.mjs`). It prints one line `enroll hs_…`; type it into the Serial Monitor. The secret goes to NVS `factory`, is never printed, and survives `forget` and unlinking. `api` then shows "board enrolled". Don't paste the `hs_…` line into the log; record only "enrolled: yes, date".
- [ ] Settings → Link calculator shows a code and "Go to <site>/link"; enter it on the site; the calculator shows a "Linked" toast within ~5 s (`README` table).
- [ ] An unenrolled board can still link but must subscribe; the free month needs enrolment. Test both once on S1 if you can.

### 4.3 End-to-end solves (both phases)
- [ ] **Typed solve:** MODE 4 → 2 Type it → type `3x^2+5x-2=0` → SEND. Answer shows like a photo's.
- [ ] **Photo solve:** MODE 4 → 1 AI Solve → focus card at **15 cm** (QA B4) → FOCUSED within 3 s → `=`. Answer **x = 1/3, x = −2** and "✓ calculator agrees". Log the time: **< 25 s** (QA B4, limit set there). Repeat at 25 cm.
- [ ] History shows both; a note survives off/on (LittleFS).
- [ ] On battery below 3.6 V: AI Solve refuses with the "plug in the cable" message (README "Using the live preview").
- [ ] Error mapping: unplug the hotspot mid-solve → a clean error, no hang (send idle timeout 150 s, max 300 s: `PORT_NOTES` "Solve idle timeout").

**Film (M5, ideas #9 "Snap a problem. See every step.", #18 Type it, #10 handwriting it can't read):** your own handwriting, overhead shot, real screen (no mock-ups). **No real link codes visible** (calendar note for #23). Never frame it as use in a test.

**Gate M5:** GO when S1 does ≥ 5 photo solves and ≥ 3 typed solves end to end with no restart, and the linked state survives a power cycle.

---

## 5. Exam mode verification (teacher-visible) (Day 3)

Behaviour from `README.md` (root) "Exam mode", `core/device.cpp` `startExam` / `endExam`, `firmware-v15-lcd/src/main.cpp` `serviceExam`, `docs/MODE4_APPS_SPEC.md` "Exam mode wins". Run on the **closed** sample.

| # | Check | Pass |
|---|---|---|
| E1 | Start from SETUP: SHIFT MODE → 8 Exam mode → `=` | Confirm screen, then "EXAM MODE ON / AI, camera and Wi-Fi are off. Unlock: USB cable, or wait 12 hours." |
| E2 | Start the other way: while off, **SHIFT, 7, ON** | Same notice |
| E3 | **Teacher view:** inverted (dark) bar with padlock and elapsed clock | Readable by eye from **3 m**, and recognisable from **5 m** (across a classroom). Photograph from both distances for the record |
| E4 | MODE 4 | Shows only "APPS ARE OFF / Exam mode is on." Nothing opens |
| E5 | Wi-Fi really off | The calculator drops off the phone hotspot's client list within ~1 min; a solve cannot be started |
| E6 | Camera really off | No viewfinder anywhere; `selftest` refuses ("not in exam mode") |
| E7 | Phone setup refused | `setup` / SETUP 6 refused |
| E8 | Maths still works | COMP, fractions, STAT and TABLE work normally |
| E9 | CLR All | Does **not** end exam mode |
| E10 | Off and on (SHIFT AC, then ON) | Still in exam mode; clock continued |
| E11 | Hard reset (tap TP7 with the back open, or let the battery run out on a test unit) | Still in exam mode after restart (elapsed time saved every 60 s) |
| E12 | Plug the USB cable in **without** typing anything | Exam mode stays on (only the serial command unlocks) |
| E13 | Teacher unlock: Serial Monitor → `exam off` | "EXAM MODE OFF / Unlocked by USB cable." AI and Wi-Fi come back |
| E14 | 12-hour end (once, on one unit, overnight) | Ends by itself after 12 h, including time spent asleep; the log says it ran out |

**Film (M6, ideas #11 "Exam mode, from the teacher's view" shot from across a room, #12 "Try to use AI Solve in exam mode", #13 "How exam mode ends"):** adult hands only; say "not allowed on the SAT, ACT or AP; for school tests ask your teacher" (`MARKETING_GUIDE.md` §13). **Never call it exam-approved** (`LAUNCH_ROADMAP.md` §4).

**Gate M6:** GO when E1–E13 all pass on one closed sample. E14 can follow within the week.

---

## 6. Assembly into the shell (Day 4–5 for S1)

Follow AS15 steps 1–11 and its QA checklist. Highlights and v15-specific points:

- [ ] AS15 3 "test the bare board" passed (§2–§4 above). Battery unplugged, cable off.
- [ ] Camera hole (zone 3) drilled **now**, where this board's lens really sits: jig A + A2 bush, 2 mm pilot, then 7 mm step drill from the outside (`enclosure/jigs/README.md`).
- [ ] LCD onto the board with the position jig, or centred on the slot with the backlight end **4.34 mm** from the slot's near edge; tail down the slot, **one gentle U-bow**, into J5. **Never fold the tail** (`REPORT.md` §4, §7). Check the step-10 marker dot: bow not squeezed against rib B.
- [ ] Window mask, 0.1 mm tape only (1.27 mm between panel top and front plate: `stage15_lcd.md` §9 item 5).
- [ ] Camera on 0.1 mm tape, no foam. Magnet piece into J3 (metered leg to pin 1), glued only after the key test.
- [ ] Key mat + board into the front shell; full key test (`selftest`, 50/50). Board rocks on the solar side → GR zone 6.
- [ ] Battery flat on the back-cover floor, 0.1 mm tape; TP1/TP4/TP7 left bare (recovery stays possible).
- [ ] Close. **Cover the Casio logo with matte black vinyl before any filming.**
- [ ] **Final test** = `production/QA_TEST_PLAN.md` Part B on the closed unit (B1 look/feel, B2 self-test via SHIFT + ALPHA + ON, B3 charge 40–60 mA, B4 focus card, B6 magnet polarity). B5 is e-paper only: replace it with P1–P3 on the closed unit (no pressure marks, no dust under the lens, panel square in the window).

**Film (M4, ideas #6 "Fitting a whole computer into a calculator shell" with the logo covered or out of frame, #7 "The keypad test", #24 "Same keys you already know", #22 "Airplane mode: still a full calculator"):** time-lapse of grinding with jigs (safety glasses on), the tail going down the slot, the first closed power-on.

**Gate M4:** GO when the closed S1 passes B1–B4 and B6, self-test PASS 50/50 keys, no rattle.

---

## 7. Battery life measurement method

The LCD version's estimate is **≈ 1.5 h of continuous screen-on** on the 150 mAh cell, "charge every 1–2 days" (`production/UNIT_ECONOMICS.md` §2; `stage15_lcd.md` §9 item 6). These tests replace the estimate with real numbers. **These numbers are for internal planning only:** the marketing compliance checklist bans battery-life claims in posts until they are approved facts (`MARKETING_GUIDE.md` §13).

### 7.1 Current per state (one session, S1, back open)
Cut one wire of a JST-PH extension lead and put the meter in series (mA range; µA range for sleep). Battery at 3.8–4.0 V, cable **off**.

| State | How to get there | Record (mA) |
|---|---|---|
| Calculator, backlight 60 % (default) | COMP screen, no keys | |
| Calculator, backlight 100 % | `bright 100` (set over USB first, then unplug) | |
| Backlight dimmed / off, panel awake | wait for dim, then off | |
| Viewfinder (camera + LCD) | MODE 4 → 1 | |
| Solve burst (Wi-Fi) | peak and average during a solve | |
| Deep sleep (SHIFT AC) | µA range, wait 30 s | target ≤ 40 µA |

Estimated hours for a state = 150 mAh × 0.8 (usable) ÷ mA. Write all of them in the log.

### 7.2 Rundown tests (real time, closed unit)
1. Charge until `status` says "full". Unplug.
2. **Screen-on test:** `screen dim 0`, `screen off 0`, `screen sleep 0` (0 = never; set over USB, then unplug), `bright 60`. Film the screen with a phone time-lapse until it shuts down. Note the minutes. Repeat at `bright 100`. Restore the defaults afterwards (`screen dim 20`, `screen off 60`, `screen sleep 120`).
3. **Mixed "study session" test:** a fixed script every 10 min: 2 min of typing sums, 1 photo solve, then idle with default timeouts. Count solves until "Battery low" stops AI Solve (3.6 V) and until shutdown.
4. **Standby test:** fully charged, off, left 7 days. Pass: wakes with memory and notes intact; record the % lost per day.
5. **Charge time:** from AI cut-off to "full" with the inline USB meter (expect 40–60 mA charge current, QA B3).

### 7.3 Real-use log (samples in use)
Each sample user records per day: solves, hours used, charges. This also replaces the 150-solves/month guess (`BUSINESS_STATUS` action 10).

---

## 8. What to do if a board fails

**Rule:** one fault at a time, smallest fix first, write everything in the log (a fixed board's first failing JSON is data for the next revision: `QA_TEST_PLAN.md` Part C).

| Symptom | Likely cause (source) | Rework / action |
|---|---|---|
| Short TP5→TP4 (< 100 Ω) before power | Bridged U3 or cracked 22 µF (QA A3 row 16) | **Don't power.** Photograph, claim with the vendor |
| > 150 mA on first USB power, or a hot chip | Short / reversed part | Unplug. Check D7 orientation, U2–U5 by touch, loupe. Claim if it's an assembly fault |
| No COM port | USB path or boot state | **Recovery:** TP1 (BOOT) to GND (TP4), tap TP7 (EN), release, flash. Try another cable and port. Never burn an eFuse |
| `memory_ok` false | Wrong module variant or bad U1 reflow (QA B2) | Claim. **Never accept a U1 substitute** (N8R8 loses IO33–37: `ORDER_WALKTHROUGH_v15_lcd.md` §4) |
| `keypad_scanner_ok` false | U6 bridge, I2C pull-ups (QA B2) | Loupe on U6; R13/R14 ≈ 4.7 kΩ to TP5 (QA A3 row 25); touch-up with flux + iron if you're comfortable, otherwise claim |
| Two keys in one row/column dead | A TCA8418 pin (QA B2 step 7) | Reflow that U6 pin |
| One key dead | Dirty mat / torn dome / scratched ENIG pad | Clean pad and pill with IPA; swap mat from the spare shell |
| LCD black, `lcd_ok` true | Tail upside down, backlight path | Re-do the diode test; **half twist** of the tail; check P7 (R23 current). `LCD_PWR_N` (IO33) must be low when on |
| Picture wrong colours / negative / offset / noise | Build flags | §3 P1–P5 |
| No camera ID | Ribbon reversed (J1 is mirrored on purpose) or fingers 2/15 open (QA B2 step 5) | Re-seat; check latch; beep 2 ↔ 15; try the spare camera |
| `camera_autofocus` false | Fixed-focus or wrong module | Wrong part: replace with Seeed 114993115 |
| Boot loop with `ESP_RST_BROWNOUT` | Viewfinder + Wi-Fi + backlight peak (`PORT_NOTES` item 9) | Backlight 25 % while Wi-Fi joins; check the cell isn't weak (Wi-Fi sag ≤ 250 mV, QA B2 step 6) |
| `vbus` false with the cable on | Magnet piece reversed or J3 joint (QA B2 step 3) | Re-do the B6 meter check before re-gluing |
| Battery 0 mV with cable | Q1 or J4 (QA B2 step 3) | Meter J4 "+" to TP6 (QA A3 row 21) |
| Wi-Fi RSSI worse than −60 dBm at 2 m | Antenna overhang touching the wall or metal over it (QA B2 step 6) | Re-seat the board; nothing metallic near the left edge |
| Self-test fine, closed unit fails | Mechanical | Reopen per AS15 step 12; check the bow (rib B), tape thickness, key mat seating |

**When to retire a board:** two failed rework attempts on the same fault, or any damage near U1. Keep it as the "bench board" for firmware work and filming close-ups.

---

## 9. Per-sample log (copy once per board: `hardware/samples/S1.md` … `S5.md`)

```
# Sample S_ — log
Board silk: AI CALC v15-LCD 2026-10-08     Board # (paint pen): S_
hw_id (status → id): calc-____________    Firmware: v15lcd-________  slot: app_
Panel: supplier ______ lot/date ______    Camera: ______    Battery: ______    Shell: fx-115ES edition ______

| Step | Date | Result | Numbers | Notes / photos |
|---|---|---|---|---|
| Day-0 inspection A1–A3 | | pass/fail | TP5–TP4 ___ Ω, TP6–TP4 ___ Ω | |
| Magnet meter check | | | outer ___ V | |
| Battery polarity / voltage | | | ___ V | |
| Camera ribbon beep + length | | | ___ mm | |
| LCD tail diode test / length | | | ___ mm, finger-1 at silk tick? | |
| First power | | | ___ mA, TP5 ___ V | |
| First flash | | | | |
| Self-test bare (JSON pasted below) | | | | |
| LCD / camera / battery attached | | | lcd_push_ms ___ | |
| Panel flags (P1–P5) | | | ROT _ INV _ RGB _ SWAP _ SPI ___ MHz | |
| Backlight current P7 | | | ___ mV / 15 = ___ mA | |
| Map frame P10 / LittleFS P11 / mode switch P12 | | | ___ ms / ___ ms / ___ ms | |
| Sleep current P16 | | | ___ µA | |
| Proxy: Wi-Fi + token + pair + link | | | | |
| Worker: register + enrolled + linked | | | enrolled yes/no (never paste hs_) | |
| Typed solves (n / ok) | | | ___ / ___ | |
| Photo solves (n / ok, time) | | | ___ / ___, ___ s | |
| Exam mode E1–E14 | | | | |
| Assembly (AS15 1–11) | | | tail bow OK? | |
| QA Part B (B1–B4, B6) | | | charge ___ mA | |
| Battery: currents (7.1) / rundown (7.2) | | | ___ min at 60 % | |
| Filmed (which idea #) | | | | |
| Who has it now / loan dates | | | | |

SELFTEST_JSON (paste whole lines, every run):
```

Also add one line per run to `production/qa_log.csv` (`serial, date, board batch, A-result, B-result, selftest JSON, notes`).

---

## 10. Gates summary (go/no-go)

| Gate | Milestone (`CONTENT_CALENDAR.md`) | GO when | If NO-GO |
|---|---|---|---|
| **M2** | v15 boards arrive | ≥ 3 boards pass Day-0 A1–A3 | Claim the bad ones; continue with good ones |
| **M3** | First power-on of sample 1 | S1 flashes over USB, `memory_ok` + `keypad_scanner_ok`, colour bars visible | §8; try S2 while S1 is diagnosed |
| (bench) | Panel checks | P1–P5, P7 pass; flags frozen | Flag changes and re-flash; no board change expected (review 14 §6) |
| **M5** | AI Solve on the real device | ≥ 5 photo + ≥ 3 typed solves end to end, link survives power cycle | Server side first (`status`, `account`), then Wi-Fi, then camera |
| **M6** | Exam mode on the real device | E1–E13 on a closed unit | Firmware fix; don't film until it passes |
| **M4** | Sample 1 in the shell | Closed S1 passes QA B1–B4, B6, 50/50 keys | Reopen per AS15 step 12 |
| **M7** | All 5 samples built | 5 closed units pass Part B; ≥ 4 enrolled on the Worker (if deployed); battery data from ≥ 1 rundown | Ship the samples you have; a 4-sample set still supports the loaner plan (2 home, rest loaned: `MARKETING_GUIDE.md` §4.4) |

Note the calendar's order (M4 before M5/M6) differs from the bench order here (solves and exam mode are first proved on the open board, then re-checked closed). Post each milestone **only when it really happened**; use the backup bank otherwise.

---

## 11. Day-by-day schedule (Day 0 = boards arrive)

Assumes all parts in §0 are in hand on Day 0 and about 2–4 hours of bench time per day (school days are lighter). Shift days as needed; the order matters more than the dates.

| Day | Bench work | Film / post |
|---|---|---|
| **0** | Unbox, Day-0 inspection of every board (§1), number S1–S5, start logs | M2: unboxing (#4) |
| **1** | S1: magnet, battery, ribbon and tail checks; first power; first flash; bare self-test; attach LCD → camera → battery (§2) | M3: first power-on (#5) |
| **2** | S1 panel bench checks P1–P17 (§3); freeze flags; battery currents (§7.1) | close-ups of the colour bars, viewfinder |
| **3** | S1 Wi-Fi + proxy + link; typed and photo solves (§4); exam mode E1–E13 on the open board (§5); start E14 overnight | M5 footage (#9, #18) |
| **4** | Drill S1's camera hole; assemble S1 (§6, AS15 4–10) | M4: fitting (#6), time-lapse |
| **5** | S1 final test (QA Part B); exam mode re-check closed (E3 photos from 3 m / 5 m); start screen-on rundown (§7.2) | M4: keypad test (#7); M6: exam mode teacher view (#11, #12, #13) |
| **6** | Buffer: fix anything S1 found; firmware tweaks; re-flash S1 | backup bank if nothing new |
| **7** | S2 + S3: inspection already done; ribbon/tail checks, first power, flash, self-test with parts (flags known) | Sunday build update |
| **8** | S2 + S3: short panel check (P1, P2, P7, P16), proxy link, 2 solves each, exam E1/E4/E13 | |
| **9–10** | Assemble S2 + S3; QA Part B | "Building samples 2 and 3" (calendar wk 10) |
| **11** | S4 + S5 bring-up (as Day 7–8) | |
| **12–13** | Assemble S4 + S5; QA Part B | |
| **14** | All 5: standby test starts (§7.2 step 4); mixed-use test on one unit | |
| **15–21** | Worker deploy (if ready): `api worker`, enrol all 5 (§4.2), re-link via the site, one solve each. Rundown repeats; real-use logging starts with adult testers | M7: "Unboxing sample 5 of 5" (#30), all five lined up |

If the Worker isn't deployed by Day 15, the samples stay on the proxy and enrolment moves to the day it is; M7 can still be called with 5 closed, working units.

---

## 12. Filming rules for every step (from `MARKETING_GUIDE.md` §13 and `CONTENT_CALENDAR.md`)

- [ ] **Casio logo never visible:** vinyl over it before filming, or frame it out. Never say or write "Casio", "fx-115" etc. in posts.
- [ ] No faces of minors; adult hands only. Your own handwriting.
- [ ] Real screens only. CAD renders labelled "CAD render".
- [ ] No battery-life, ship-date, "in stock" or accuracy claims; "AI can make mistakes" if accuracy comes up.
- [ ] No real link codes, tokens, `hs_…` lines, Wi-Fi passwords or the admin token on screen (blur the Serial Monitor).
- [ ] Homework / practice framing only; never "in the exam".
- [ ] Safety glasses on in any grinding shot.
