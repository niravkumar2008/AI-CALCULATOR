# Stage 9: finishing the v8 board (ai_calc_pcb_9)

Result: `kicad-cli pcb drc --schematic-parity` gives **0 errors, 0 warnings, 0 unconnected, 0 parity issues**. The ERC is unchanged, because the schematic was not touched.

## What changed, and why
| Fix | Plain explanation |
|---|---|
| New file `kicad/ai_calc.kicad_dru`: rule **"J1 fanout clearance"** (0.15 mm) inside a rule area called `J1 fanout` around the camera socket | J1's pads are 0.5 mm apart, so the tracks leaving them can't keep the camera netclass's 0.2 mm gap. 0.15 mm is still above JLCPCB's 0.1 mm minimum, and it only applies right at the socket. |
| Camera bus fan-in under J1 re-routed (D5, D7, SIOC, PCLK, XCLK, VSYNC) and D1/D2 at U1 | These were the 11 clearance errors. The track ends were respaced so each one meets its pad cleanly. |
| 16 dangling stubs and the old RESET/PWDN/SIOC branches above J1 removed | Leftover track bits that went nowhere. The long branches above the socket blocked the power pins. |
| **CAM_AVDD** (pin 4) goes up under the socket body, through a via, and over to C13 | Under the socket body is fine for power: the cable never touches the board there. |
| **CAM_DVDD** (pin 10) goes up, passes over AVDD's via, threads between two vias, and comes down beside the left mounting tab to C15/U5 | Both supplies leave to the left from pins in the "wrong" order. One of them has to cross over the other, and AVDD's via is that crossing. |
| CAM_2V8 (pin 11) goes straight up to a moved via (149.5, 157.75) | This frees the column above pin 10 for DVDD. |
| GND pins 2/15/23 connected. Pin 2 runs left under DVDD to the GND via at (140, 161.9), then to U5's GND via | Pin 2 is under the ")" key pad, where vias aren't allowed, so it travels sideways instead. |
| RESET, PWDN, SIOC pull-up branches (R8, R9, R19) re-joined | SIOC uses a via at (145.65, 164.2) next to pin 5. SIOD now jogs left early to make room. |
| CAM_PWR_EN to R10 connected | It was a missing link to the pull-down resistor. |
| **R5** (VBUS divider bottom) moved to (126.07, 81.0) | It now sits right on its two traces, VBUS_SENSE and GND, instead of having its own wires. |
| **H11** case-post hole 4.4 → **4.2 mm** (new footprint `ai_calc:Post_Hole_4.2mm`) | Key pad SW21 came within 0.18 mm of the hole edge, and the rule is 0.25 mm. The Casio peg is 2.9–3.3 mm, so there's still ≥ 0.45 mm of play all round. **Measure that peg** (measurement sheet). |
| Footprints copy the schematic's Description/Datasheet fields. The 12 post holes are marked "board only / not in BOM or CPL" | This cleared the 140 schematic-parity warnings. Nothing electrical changed. |

## Firmware (done in this stage)
`firmware-prototype/src/pins.h` now uses the stage-6 camera map from `pins_final.h`. CHG_STAT reads as a plain input, because the board has its own 100 k pull-up. `pio run -e prototype` builds.

## Still open before ordering
- The measurement sheet items (heights, ribbon lengths, peg sizes). Any of them can still move parts.
- Freerouting needs Java 17+. This PC has Java 1.8.0_503, so `tools/route.sh` can't run here. Nothing in stage 9 needed it.
