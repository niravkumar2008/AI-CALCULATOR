# Stage 10: moving the board to the Casio fx-115ES shell

Nirav switched from the fx-300ES Plus shell to an **fx-115ES** shell on 2026-10-02. This stage moves the board to the new shell, using his photos. Caliper readings (Measurement Sheet C6–C15) and a flatbed scan (F6) will confirm it before ordering.

## How the photos were measured
Photos have no scale and are taken at a slight angle. The rubber key mat makes them measurable:

1. **Find the 50 key contacts** (46 keys + 4 pad directions) in a photo. A small script (`blobs.py`) finds the dark round blobs.
2. **Fit a perspective transform** ("homography": the maths that undoes a tilted camera) from the photo onto the board's own key-pad positions. That turns photo pixels into board millimetres.
3. **How good is the fit?** It shows two things at once: whether the fx-115ES key grid matches the board, and how far to trust the post positions.

| Photo | What it shows | Fit to the 50 key pads |
|---|---|---|
| Front shell from inside, mat in place (`478afcb6`) | contacts, post tips, shell | RMS **0.48 mm** |
| Loose mat, contact side up (`6996fcbd`) | contacts + post holes, all on one flat surface | RMS **0.42 mm** |
| Bare Casio board, key side (`0103f1dd`) | the real board holes | fitted to the holes: RMS 0.40 mm |

**Conclusion 1: the key grid is the same.** Two independent photos agree on every row within about 0.5 mm. The one real difference is the 4-way pad: UP sits about 1.2 mm higher and DOWN about 1.0 mm lower (both photos agree). Rows R3 and R5 come out about 0.4 mm off in both photos. That's well inside the 7 × 5.5 mm pads, so they stay.

**Conclusion 2: the posts are different.**

| Post (calculator front view) | Mat photo (mm) | Shell photo (mm) | Old board hole | Stage 10 |
|---|---|---|---|---|
| Row 1/2 screw post, left | (127.6, 126.2) | (128.0, 125.1)* | H1 (128.36, 126.74) | H1 stays (6 mm hole covers it) |
| Row 1/2 screw post, right | (174.0, 126.2) | (173.9, 125.1)* | H3 (171.22, 126.34) | **H3 → (174.0, 126.1)**, 2.8 mm move |
| Row 2/3 pair | (128.8, 136.8) / (171.1, 137.1) | (129.1, 137.1) / (171.4, 136.9) | H8 / H10 | unchanged (within 0.3 mm) |
| Row 3/4, right only | (171.1, 146.5) | (171.7, 146.1) | H4 (171.06, 145.96) | unchanged |
| Row 4/5, left only | none | none | H5 | **removed** |
| Row 5 / number row 1 pair | none | none | H7, H11 | **removed** |
| Number rows 1/2 pair | (130.7, 175.6) / (168.8, 175.6) | (131.0, 175.4) / (169.3, 175.7) | H2 / H9 (y ≈ 176.3) | H2 → (130.85, 175.5), H9 → (169.2, 175.75) |
| Number rows 2/3, left only | (130.4, 187.1) | (130.9, 187.3) | H6 (130.56, 187.67) | H6 → (130.6, 187.2) |
| Number rows 2/3, right | none | none | H12 | **removed** |
| Number rows 3/4 pair | (130.4, 198.1) / (168.9, 198.2) | (130.7, 197.9) / (169.3, 198.2) | none | **new H13 / H14**, 4.2 mm |
| Bottom screw posts | top notches of the mat | (130.9, 210.1) | half-circle notches | unchanged |

\* The screw posts at rows 1/2 are tall. In the shell photo their tips sit above the mat, so the angle shifts them a little. The mat photo has no such error, so it's the one I trust there.

Board coordinates are KiCad millimetres (KiCad top view = looking at the inside of the front shell).

## Board changes made in stage 10 (DRC: 0 errors, 0 unconnected, 0 schematic mismatches)
| Change | Why |
|---|---|
| H3 → (174.0, 126.1) | The row 1/2 screw post on that side is 2.8 mm further out on the fx-115ES. A GND stitching via under it was removed (the pours still join there). |
| H1 left at (128.36, 126.74) | The photos say about 0.6 mm left/up. The 6 mm hole has that much play, and moving it would cut I2C_SCL and KEYPAD_INT. |
| H2 → (130.85, 175.5), H9 → (169.2, 175.75), H6 → (130.6, 187.2) | Number-row posts sit 0.5–0.9 mm higher. H9 is nudged 0.15 mm to keep COL1 clear. |
| H5, H7, H11, H12 removed | The fx-115ES has no posts there. |
| H13 (130.5, 198.0), H14 (169.1, 198.1) added, 4.2 mm | New post pair between number rows 3 and 4. A GND stitching via under H14 was removed. |
| SW46 (UP) → y 119.0, SW47 (DOWN) → y 131.9 | The 4-way pad's UP/DOWN contacts sit 1.2 / 1.0 mm further out (both photos agree). Their finger links moved with them, and the COL7 lead was re-routed under DOWN. |
| Top-left corner cut-out widened | The fx-115ES corner screw post looks about 4 mm higher than the fx-300ES one (≈ 118, 65.5 after the parallax correction). The cut-out now covers both spots with ≥ 0.75 mm play. Nothing on the board was there. |
| DRC rule `npth_inside_courtyard` set to ignore | Key-pad courtyards are drawn bigger than the copper, and the real Casio posts sit that close to the keys. The real check (hole-to-copper clearance) still runs. |

## The back cover (photo `2f54d6c7`, fitted on its 6 screw holes, about ±1 mm)
The back cover's ribs pushed on the old LCD and the Casio board. They now meet our parts. Positions are in `fitcheck/backcover_fx115es.json`, and they're drawn dashed on `fitcheck/grind_map_back_cover.svg`.

| Feature | Board position | Parts underneath | Plan |
|---|---|---|---|
| Battery opening | centre (168.3, 69.0) | battery bay | ✓ matches |
| Ribbed box behind the solar cell | x 121–154, y 64–76 | magnet connector J3 | grind flat (as planned for the fx-300ES) |
| Upper rib grid: 3 ribs across at y 82 / 95 / 107, 2 along at x 140 / 161 | screen section | **camera (y 95 rib runs right through it)**, U1 (ESP32), J2, L1, U7, Q1, C1–C3, R5–R7, TP2/TP3 | grind the grid flat where it crosses parts; the camera spot must be clear |
| Small ring, Ø 16.8 | centre (150.2, 124.7) | nothing | keep (it supports the 4-way pad) |
| Big ring, Ø 24.8 | centre (150.5, 142.7) | C16 and D2 sit right at its wall | grind that arc of the ring, or move C16/D2 once the ring height is known |
| Lower rib 1, y 161 | x 137–163 | camera socket J1 (2 mm), U5, C15 | grind |
| Lower ribs 2–4 (y 172 / 183 / 194) + centre rib | keypad section | nothing | keep: they back up the keys |

## Still from the old shell (not measurable in the photos)
- **The LCD window.** Two photos agree it's about **60 × 22 mm, centred near (150.5, 94.4)**. The board assumes 57 × 21 at (149.9, 96.6), so the e-paper may need to move up about 2 mm. That's a big re-route, so it waits for **C12/C13**.
- **Board outline.** The bare-board photo puts the Casio board's right edge at 182.2 (old 182.3) and its bottom at 209.9 (old 211.0). The left edge is hidden by the solar cell and a wire. That's ±1 mm, too loose to move the outline, so **C6 (Casio board width × length)** decides it.
- **Top-corner screw posts and the screen section's walls** (C9, C11, C14).
- **Hole sizes.** H1/H3 stay 6.0 mm and the others 4.4/4.2 mm until **C7** gives the real post diameter.

## Things in the way, seen in the photos
- **Round stubs on both side walls beside the screen** (three per side, from the wall photos). They held the old LCD. They stick in where the board's screen section reaches the walls. They're safe to grind off; **C14** says how big they are.
- **Back cover:** two rings (about 8 and 20 mm across) and ribs in the middle, plus a ribbed box behind the solar cell. They'll press on parts at the top of the keypad. They show up on the grind map once the board is final.
