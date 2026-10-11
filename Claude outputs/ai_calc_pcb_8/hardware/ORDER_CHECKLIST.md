# JLCPCB order checklist (fill in when the VERIFY items are done)

**Don't order until** the calipers/clay checks in `geometry_assumptions.md` §7 are done, especially V5/V12 (heights), V9 (e-paper ribbon length), V3 (battery bay) and V11 (centre rib). Any of them can move a part, and moving a part means re-running the layout.

## Files (all in `fab/`)
| Upload where | File |
|---|---|
| "Add gerber file" | `ai_calc_gerbers_JLCPCB.zip` |
| PCB Assembly → BOM | `ai_calc_BOM_JLCPCB.csv` |
| PCB Assembly → CPL | `ai_calc_CPL_JLCPCB.csv` |

## PCB options
| Option | Value | Why |
|---|---|---|
| Layers | 2 | |
| Dimensions | filled in automatically (about 72.3 × 148.8 mm) | |
| PCB qty | 5 | |
| Thickness | **0.8 mm** | Same as Casio's board, so it fits the keypad stack |
| Surface finish | **ENIG** | Gold key pads; HASL is too bumpy for rubber key contacts |
| Mask colour | green (cheapest, fastest) | |
| Remove order number | "Specify a location" is free. Or pick "Yes" for a small fee | Otherwise JLCPCB prints a number, possibly on a key pad |
| Everything else | default | |

## Assembly options
| Option | Value |
|---|---|
| PCBA type | **Standard** (the ESP32 module needs X-ray inspection, which Economic doesn't do) |
| Assembly side | Top |
| PCBA qty | 2 |
| Tooling holes | Added by JLCPCB |
| Confirm parts placement | **Yes**. Check every part's pin-1 dot in their 3D preview |

## In the parts-placement preview, check these by eye
- **U1 ESP32**: the antenna end (the plain end with no pads) points at the left board edge.
- **J1 and J2** (ribbon sockets): the flip lid faces away from the opening. J1's opening faces up (towards the camera), J2's opening faces the slot.
- **J4** battery socket: the opening faces the battery bay.
- **U2/U3/U4/U5** (SOT-23-5), **U7** (SOT-23-6), **Q1–Q3**, **D1–D5**: pin 1 / the diode stripe lines up with the dot on the silkscreen. JLCPCB sometimes rotates these by 180° in the preview. If one is off, rotate it there; the file doesn't need changing.
- **U6** TCA8418: the pin-1 dot matches.

## After it arrives
1. Battery check (sheet item E3).
2. Camera ribbon pin-1 check (sheet item E6).
3. First power-up from USB with no battery, display and camera unplugged. Then plug in the e-paper, then the camera.
