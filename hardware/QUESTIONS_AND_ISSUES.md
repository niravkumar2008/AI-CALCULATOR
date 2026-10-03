# Questions and issues for Nirav

Started 2026-10-03 (overnight session). Each item says what I assumed in the meantime, so nothing is blocked while you sleep. Answer them in any order. Items marked **(blocks order)** need an answer before Monday's JLCPCB order.

## Measurements

1. **(blocks order) C8: "61.8 outside to outside".** Which post pair is that? From your photos the pairs are about 46.4 (beside SHIFT/ON), 42.3 (under row 2), 38.3 and 38.6 mm apart, centre to centre. 61.8 − 3.85 = 57.95 doesn't match any of them. *Assumed: holes stay at the photo-measured positions.*
2. **(blocks order) C9: "3.7 outside to outside / 49.65".** Is 3.7 the post's width and 49.65 the outside-to-outside distance between the two top-corner posts? That would make them about 45.8 mm apart, centre to centre. The photos suggested about 62.7. *Assumed: photo positions kept.*
3. **C4: wall thickness 4.7 / 5.5.** That's thick for a side wall. Was it measured at the screen section where the round stubs are? *Assumed: 4.7 is the stub zone and the bare wall is thinner.*
4. **Still to measure (blocks order):** C14 (wall stubs), C15 (post distances from the top edge), depths D1–D4, D11, D13, the ribbon checks E6/E7 and, if you can, the F6 flatbed scan (the best single source of exact positions).

## Electrical (from the independent review, `REVIEW_independent_2026-10-03.md`)

5. **R20 leakage fix.** R20 leaked about 20 µA into the charge contacts when no cable is in. The PCB worker is moving its pull-up to VBUS_SENSE (or leaving R20 off the board). Nothing for you to do, just know it changed.
6. **Regulator swap, AP2112K → RT9080.** Standby goes from about 6 weeks to about 4–6 months. Same footprint. Say if you'd rather keep the AP2112K.
7. **Battery polarity.** Before ordering, check the Adafruit battery's red wire against the J4 + mark (sheet item E3). Reversed JST leads are common.

## Business (from the roadmap)

8. **Calculator edition.** Are your two calculators the same edition (fx-115ES, **PLUS** or **PLUS 2nd edition**) as the one you measured? The insides can differ, so it's worth a quick look at the back labels.
9. **Age.** If you're under 18, a parent needs to be the account holder for the LLC, Stripe and preorder platforms. Worth sorting out early.
10. **Subscription.** Should the calculator work as a normal scientific calculator without the $15 subscription? I recommend yes.


## CAD replica
(fx-115ES shell replica, `enclosure/build_fx115es_replica.py`, details in `enclosure/replica/README.md`)

1. **Width (C2 = 79.3):** is that the front shell or the back cover? The photos show the navy back cover wrapping round the front shell along the keypad section, so the model makes the back cover 79.3 wide and the front shell ~1.8 mm narrower there (78.4 max). If the front shell itself is 79.3, say so and the tray walls get thinner.
2. **Profile from the side:** no side-on photo. Please take one straight-on photo of the long side (with a ruler). It decides the parting-line height (guessed 2.6 mm from the back), how high the back cover's tray walls come up (guessed 11.3 mm), and the edge round-overs (guessed 2.0 front / 1.2 back).
3. **Key height:** how far do the keys stick up above the face (guessed 1.5 mm)? And the face-plate thickness (guessed 1.5 mm).
4. **Battery:** the back shows a round LR44 hole in a shallow rectangular recess. Is there a lid or sticker over it? Size of the recess? (guessed 16 x 15 mm, 0.6 mm deep, thin lid modelled).
5. **7th hole:** the back has a small hole in the centre over the small ring (photo e82c982d). Screw or something else? (modelled as a plain 2 mm hole.)
6. **Feet:** where are the rubber feet and how big? (guessed four 7 mm round feet, 0.5 mm proud.)
7. **Corner screw posts:** with C11 (4.2 / 6.1 from the inside walls) the model puts them at (±32.7, 73.2) front-view mm, which agrees with the photo estimate within ~1 mm. Fine for the PCB's cut-outs, but check H-hole clearance once the board is final.
8. **Slide hard cover:** not measured at all; the model's cover is a placeholder (plain lid with side walls). Calipers on the real cover (length, width, height, wall thickness, how it grips) would fix it.
9. Found while working: `enclosure/fx115es_outline.json` stores the outline as 3 runs out of order. `build_fx115es_replica.py` re-chains them; `build_case.py` reads the file as-is, which only works because Fusion builds profiles from the crossing lines. Worth fixing in the json or in `build_case.fx115_outline()`.

## PCB (stage 11)
(details in `stage11_epaper_calipers.md`; board is DRC 0/0/0 with schematic parity)

1. **(blocks order) C4 wall 5.5 / 4.7 mm at the screen section.** The board's top part is 72.3 mm wide (x 113.84–186.16); with a 79.3 case and 4.7–5.5 mm walls the inside is only 68.3–69.9 mm. Please measure the *inside* width of the front shell at the screen section, wall to wall, at board height, and say where the stubs are. *Assumed: the thick reading is the stubs / wire holder, the plain wall is thin, so the outline is unchanged.* If the wall really is that thick, the ESP32 has to move about 2 mm inward.
2. **C8 "61.8 outside to outside" — which two posts?** No post pair on the board matches (57.95 or 58.9 c-c). *Assumed: holes stay at the photo positions.*
3. **C9 "3.7 / 49.65" — which posts, and is 3.7 a diameter?** *Assumed: photo positions kept.*
4. **Window position left-right.** Only its size (60.65 × 24.3) and top edge (24.0) were measured. *Assumed: centred on the case (x = 150).*
5. **Window wider than the panel glass** (60.65 vs 59.2): about 0.5–0.8 mm of board shows at each end of the glass. *Assumed: fine; a black mask sticker can hide it.*
6. **E7 e-paper ribbon length** (panel edge to tip). *Assumed: 14.3 mm per the drawing; 13.5–15 mm works.*
7. **Panel active area centred on the glass?** *Assumed yes (as since stage 1); if your panel's driver end is wider, the picture sits off-centre by that amount.*
8. **Screw vs locating posts.** From the bare Casio board photo: the two row-1/2 posts (H1/H3) and the bottom notches are screw posts (holes 6.0 mm, notches unchanged); all others are 2.9 mm locating posts (holes now 4.2 mm). *Tell me if any small post is actually a screw.*
9. **R20 now on VBUS_SENSE, U3 now RT9080-33GJ5, new C32 22 µF, new TP5/TP6/TP7.** Nothing to do unless you want the AP2112K back. Firmware: read CHG_STAT only while VBUS_SENSE is high (it reads low with no cable).
10. **Firmware to-do (review S4/S6):** cap Wi-Fi TX power (~11 dBm) and lock AI below ~3.6 V; drain the TCA8418 FIFO / clear INT_STAT before deep sleep; release camera GPIOs (no pulls) before cutting CAM power; USB-Serial-JTAG off without cable.
11. **JLCPCB preview:** check D7's cathode band is on the VBUS pad (towards J3). *Assumed: KiCad 0° = JLCPCB 0° for this SOD-123FL part.*
