# Arrival checklist: from boxes to a working calculator

Written 2026-10-06 for board v14 and firmware stage14-2026.10.06. One page. Entry point for everything else: `FINAL_STATUS.md`.

Guides: **Power-Up** (PU) https://claude.ai/artifact/PqTARcDutvwxS5aGB8iEG9 · **Grinding** (GR) https://claude.ai/artifact/9rCWyw8MeHP2z8FWijnyfL · **Assembly** (AS) https://claude.ai/artifact/8BTC57FZ3JFwiDdiLprwtY. Section links below add `#id` to those addresses (sources: `bringup_guide.html`, `enclosure/final_assembly/grinding_manual.html`, `enclosure/final_assembly/assembly_guide.html`).

## 1. What arrives, from whom, and what to check on the box

Order list: `Claude outputs/AI_Calculator_Shopping_List.xlsx`. Open each box the day it lands; problems are easiest to claim within days.

| From | What (qty) | When (after ordering) | Check on arrival |
|---|---|---|---|
| **JLCPCB** (DHL/FedEx) | 5 bare PCBs + 2 assembled boards, v14 | about 10–14 days door-to-door (economy shipping alone 7–15 days); about $200–240 delivered incl. ~35–37.5 % duty paid at checkout | Box and anti-static bags not crushed. Count: 2 assembled + 3 bare. Silk reads **"AI CALC v14 2026-10-04"**. Nothing loose rattling in the bags. Then the full incoming inspection, **unpowered**: `production/QA_TEST_PLAN.md` Part A1–A3 (= PU step 2, `#inspect`). Any short in A3 16–21: photograph and claim with JLCPCB, don't power it |
| **Adafruit** (UPS 2-day) | 3 × #1317 LiPo 150 mAh, 3 × #5358 magnet piece, 2 × #5412 magnet cable | 2–4 days | Battery: pouch flat, not puffy, no dents or punctures, wires intact at the cell; read its voltage (expect about 3.7–4.0 V). Store it away from metal. Polarity is metered later (PU step 4). Magnet pieces: 4 straight-ish legs each. Cable: USB-A end and a magnetic end that snaps onto a #5358 |
| **Seeed Studio** or **DigiKey** | 3 × OV5640 autofocus camera for XIAO ESP32S3 Sense (SKU 114993115) | 3–6 days (DigiKey US faster) | It is the **autofocus** OV5640 for the XIAO Sense (not a fixed-focus or OV2640 module). Ribbon not creased, gold fingers clean. Leave the heat sink off if it makes the module taller than 5.5 mm. Ribbon beep test and length are PU step 5 (`#ribbon`) |
| **Waveshare** or **Amazon** | 2 × 2.13" e-paper raw panel V4 (SKU 12672) (you already own a third in the HAT) | Prime 1–2 days, Waveshare direct 4–8 days | Glass not cracked (look at the corners), ribbon not creased at its root, 24-pin ribbon. Keep it glass-down on a cloth. Not a HAT, not a B/W/red panel |
| **Amazon / hardware store** | Kapton tape; 0.1 mm double-sided tape (camera, battery) + about 0.15 mm (e-paper); epoxy or hot glue; 7 mm (9/32") + pilot drill bits; matte black vinyl; 99 % isopropyl; safety glasses + dust mask; a USB-A charger if you have none | 1–2 days | Tapes are **thin film, not foam** (only 0.3 mm above the battery, 0.5 mm above the lens). Black vinyl 0.08–0.15 mm and matte |
| Already here | 2+ Casio fx-115ES, calipers, multimeter, rotary tool, PC with VS Code + PlatformIO | – | Both calculators the same edition (back label) |

## 2. Before the boards land (while you wait)

- [ ] **AI server online** and one device token per calculator: PU step 10 "Once: put the server online" (`#ai`), details in `server/proxy/README.md`. The Claude API key goes **only** in the host's environment settings; never in the calculator, git or a chat.
- [ ] **Shells** (can be done before or after the boards): take the Casio apart (AS 1, `#s1`), the **10-minute pre-grind checklist** (`verification/07_final_review_fitment.md` §3; GR `#order`), then grind (GR, AS 2 `#s2`) zones **1** solar box (GR `#z1`), **2** rib B (`#z2`), **5** magnet notch (`#z5`); **5b** lip (`#z5b`) only if your cover has one; **6** side-wall relief (`#z6`) only if the paper template's yellow antenna tab won't drop in; **4** LR44 cup (`#z4`): keep. **Hold zone 3** (the 7 mm camera hole, `#z3`) until the camera ribbon is measured.
- [ ] Window mask cut (AS 5, `#s5`, `enclosure/final_assembly/window_mask.md`).

## 3. Day one, in order (one board at a time)

1. **Inspect unpowered:** PU 2 (`#inspect`) + QA plan A1–A3.
2. **Magnet piece meter check** with the real #5412 cable, before any gluing: PU 3 (`#magnet`). VBUS leg into J3 pin 1 (silk N / +).
3. **Battery polarity:** PU 4 (`#battery`). Red wire on J4 "+".
4. **Camera ribbon:** beep 2 ↔ 15 and measure module far edge → tip: PU 5 (`#ribbon`). Under about 70.3 mm: the module shifts up to 1.7 mm towards J1 later; drill the 7 mm hole where the lens really sits (GR `#z3`, AS 6).
5. **First power, computer USB only** (no battery, panel or camera): PU 6 (`#power`).
6. **First flash, always over USB** (the two-slot OTA partition table can't arrive over the air): PU 7 (`#flash`). `status` → `Firmware: stage14-2026.10.06 (slot app0)`. Recovery: TP1 to TP4, tap TP7.
7. **Self-test** (`selftest`, `q` skips keys), save the `SELFTEST_JSON` line (has `firmware_slot`): PU 8 (`#selftest`).
8. **E-paper → self-test, camera → self-test, battery → charging:** PU 9 (`#attach`). Cable out before every ribbon.
9. **AI settings:** PU 10 (`#ai`): `wifi`, `proxy`, `token` in the Serial Monitor, or the phone page (`setup`; on a finished calculator **SHIFT MODE, 6, =**), then `pair` → `<server>/link`.
10. **Hand over:** PU 11 (`#assemble`) → AS 3 "must pass" (`#s3`). Battery unplugged, cable off.
11. **Assembly:** AS 4 prepare (`#s4`), 5 e-paper + mask (`#s5`), 6 camera (`#s6`, thin tape, no foam), 7 magnet into J3 (`#s7`), 8 key mat + board into the shell, key test, glue the magnet (`#s8`; board rocks on the solar-window side → GR zone 6).
12. **Battery and close:** AS 9 (`#s9`, flat on the back-cover floor, 0.1 mm tape, TP1/TP4/TP7 bare), AS 10 (`#s10`).
13. **Final test:** AS 11 (`#s11`): SHIFT + ALPHA held, ON → SELF-TEST PASS with 50/50 keys; viewfinder; phone setup from the keys; AI solve on battery ("Battery low: plug in the cable to use AI solve" = charge first); charging; memory overnight.
14. **QA record:** `production/QA_TEST_PLAN.md` Part B on the closed unit; log the JSON (with `firmware_slot`) in `production/qa_log.csv`.

Stuck? PU 12 (`#trouble`) and AS 12 (`#s12`). Then board #2 the same way.
