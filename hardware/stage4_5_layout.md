# Stages 4 + 5: footprints and placement

All parts are on the **F side** (it faces the back cover), so JLCPCB assembles one side only. The **B side** carries only the 50 bare key pads and the e-paper panel.
Picture: `geometry/placement_v1.png`. Check script: `work/placement_check.py`. It reports 0 courtyard overlaps, 0 parts in the case keep-outs and 0 parts over the edge.

## Footprints (all real JLCPCB/LCSC land patterns)
| Part | LCSC | Footprint | Note |
|---|---|---|---|
| J1 camera socket, J2 e-paper socket | C6364666 (SHOU HAN, flip-lid, **dual contact**, 16.9k in stock) | `FPC_24P_P0.5mm_DualContact_C6364666` (+ `_CamReversed` for J1) | It makes contact on both faces, so a cable that goes in upside down still connects. See the FPC section below. |
| J3 magnet socket | C42379197, 1×4 2.54 mm **SMD** female header, 5 mm tall | `PinSocket_1x04_P2.54mm_SMD_H5.0_C42379197` | It replaces the 8.5 mm through-hole header: 3.5 mm lower, and no hand-soldering fee because JLCPCB places it with the other SMD parts. The 5358's 0.6 mm legs push into it. |
| J4 battery socket | C295747 JST S2B-PH-SM4-TB | from LCSC | Opening faces down the board (+y), so the plug and its wire bend have room; the battery leads run from the bay's left end to the plug. |
| D7 VBUS TVS | C193402 SMF5.0A | KiCad D_SMF | Next to the magnet socket. |
| L1, Q3 | C135265, C469327 | from LCSC | Footprint pads match the real parts. |
| U1 | C3013941 | Espressif official, with its own antenna keep-out zone | |
| Key pads | — | `KeyPad_9.0x7.0` (number keys), `KeyPad_6.0x4.5` (function keys, top rows, bottom row), `KeyPad_5.0x4.0` (REPLAY) | The function and bottom-row pads were shrunk from 7.0 × 5.5 so the copper stays ≥ 0.3 mm from the plastic posts and the edge. That still leaves ±1 mm around a 4 mm rubber pill. |

## The two ribbon cables (the riskiest part of the board)
**Camera (Seeed OV5640, 24-pin, 70.5 mm).** From Seeed's module drawing: the gold fingers are on the underside (the same side as the module's base), and when you look at the fingers with the tip pointing away from you, **pin 1 is on the left**.
The camera lies lens-up on the F side, and its cable runs flat straight down the centre to J1 at y ≈ 161. That puts the fingers face-down. The JUSHUO/SHOU HAN pad numbering expects pin 1 on the *right* (from both the AFC01 datasheet's top view and its recommended-FPC drawing). So a straight run lands **mirrored**.
J1 therefore uses `_CamReversed`: the same land pattern, with pad numbers flipped so that cable pin N lands on schematic pin N. An independent check came to the same answer (about 75 % confidence at the time; raised to about 90 % by verification 01/03 in stage 14).
The one thing that argues against it: Seeed's own XIAO board uses this camera with an AFC01 and plain numbering. That only fits if Seeed's footprint is numbered from the other end, which can't be confirmed from their PDF.

**Safety net:** J1 is dual-contact. If the reading is wrong, giving the cable one 180° twist along its length flips which face touches the contacts. That also flips the order, so the camera works with no board change. (Mid-twist the cable stands on edge, 6 mm, so do the twist where there's height.)
**Check before the first plug-in:** fingers 2 and 15 are both GND. Count from the end you think is pin 1, and if 2 ↔ 15 beeps you have the right end.

**E-paper (2.13" V4, 24-pin, FPC 14.3 mm long).** From Waveshare's drawing: the fingers are on the display side, and pin 1 is on the right when you look at the fingers with the tip pointing away. That matches plain numbering on a bottom-contact socket.
Route: the panel sits on the B side with its FPC pointing to the right in this view (your left when you hold the calculator). The FPC folds behind the panel, runs back about 7 mm, comes up through a **1.0 × 14 mm slot at x = 172.9** and goes into J2, whose opening faces the slot. This is how every e-paper module folds its ribbon.
Length budget: 1.3 (fold) + 7.0 (behind the panel, from the fold to the slot centre) + 1.5 (through the slot) + 1.5 (slot centre to J2's mouth) + 3.0 (into the socket) = 14.3 mm. J2's mouth is 1.0 mm clear of the slot edge.

## Placement (KiCad view = looking at the board from the back cover)
- **ESP32-S3**: left side, antenna pointing at the left wall, between the two left back-cover pins. The antenna strip stays outside the e-paper panel's outline.
- **Power** (charger, ESD, battery FETs, 3.3 V LDO): the strip under the magnet connector, with the JST socket at the battery bay's edge.
- **E-paper booster + 10 panel capacitors**: right of the camera, between the camera and the slot.
- **Camera supplies** (2.8 V, 1.5 V, AF diode, pull-ups): beside J1 at the bottom of the cable run.
- **TCA8418**: above the function keys, left of the camera cable.

These stay clear: a 12 × 12 mm camera module area, a 7 mm wide corridor for the camera cable, the 5 back-cover pins (+0.5 mm), all holes (+0.6 mm), the magnet body, and the 3 back-cover rib lines.

# Stage 6: routing (first complete pass, 2026-10-02)
- **DRC: 0 errors, 0 warnings, 0 unconnected** (KiCad 10.0.6, every severity on).
- Signal tracks are 0.2 mm, power tracks 0.3 mm (net class "Power"). Vias are 0.3 mm drill / 0.6 mm pad, all tented (covered by solder mask).
- No tracks or vias within 0.35 mm of any board edge or of the e-paper slot.
- Both layers have a GND pour, joined by about 85 stitching vias plus a via beside each ground pad.
- Freerouting 1.9 routed everything, ground included, then the pours went on top.
- The NPTH holes now have a solder-mask opening the same size as the hole, so tracks can pass at the normal hole clearance.
- B side: the 50 key pads, the GND pour, and some matrix tracks between the pads, all covered by solder mask except the gold pads themselves.
- Renders: `geometry/board_render_top.png` (F side, faces the back cover) and `geometry/board_render_bottom.png` (key side).

## How it's built (repeatable)
Run `tools/full.sh`. It does: place (`build_board.py`) → check (`placement_check.py`) → autoroute (`route.sh`) → GND pours and vias (`add_gnd.py`) → tidy up (`cleanup.py`) → DRC. `tools/make_outputs.sh` then writes the JLCPCB files into `fab/`.
The scripts use the cloud workspace's paths. They're kept here so every layout decision is written down, not as something you need to run.
