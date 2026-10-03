# Nirav's caliper readings, fx-115ES shell (2026-10-03, second batch)

These are verbatim except that typos are fixed. The interpretation notes in *italics* are Claude's first read. Workers must check them against the photos in `C:\Users\r_kas\.claude\uploads\` (all subfolders; newer uploads may appear) before relying on them.

## Case, with calipers
| ID | Item | Reading | Notes |
|---|---|---|---|
| C1 | Front shell outer length | 159.9 | |
| C2 | Front shell outer width, screen section | 79.3 | |
| C3 | Back cover outer length × width | 161 × 79.3 | |
| C4 | Wall thickness, screen section (L/R) | 5.5 / 5.5 including a small plastic part that holds the wires; 4.7 without it | Nirav will upload a picture. *If the walls are really 4.7 thick, the inside width there is 79.3 − 9.4 = 69.9, against a board width of 72.3. Check with photos and C14.* |
| C5 | Wall thickness, keypad section | 0.8 | |
| C6 | Casio board width × length | 65.05 × 97.9 | |
| C7 | Post diameters | 3.85 for all screw posts; 2.9 for all non-screw (locating) posts | |
| C8 | Post-pair spacing, top to bottom | "Start at the hole below the screw hole: **46.5** / the 2nd row only has one peg, on the right side but parallel to the last one / **42.3** / the 5th row only has one, on the left side / 4th, 5th and 6th rows all parallel" | *46.5 matches the photo pair-1 spacing (46.4) and 42.3 matches pair 2 (42.3). So the stage-10 photo holes are confirmed. The earlier "61.8" reading is superseded.* |
| C9 | Top-corner screw posts, centre to centre | **68.1**. "The top holes are in the corners, nowhere near the board." | *The board probably doesn't need corner cut-outs for these. Check the board outline against the posts plus clearance.* |
| C10 | Bottom screw posts, centre to centre | about 39–40 | Matches the board notches (38.94) |
| C11 | Top-left corner post: centre to inside of left wall / top wall | 4.2 / 6.1 | |
| C12 | Window opening | 60.65 × 24.3 | |
| C13 | Window top edge to case top edge | 24.0 | |
| C14 | Side-wall stubs beside the screen: how far they stick in / how high | **5.45 / 5** | *How this relates to C4 is unclear. They may be the "wire holder" bits.* |
| C15 | How far down each post row is | "From the outside of the calculator to the outside of the holes": **8.9 / 69.95 / 89.6 / 120.45 / 130.2 / 142.4 / 154.6**. "There are 8 rows of holes in total, but row 4 only has a hole on the right side of the PCB and row 6 only on the left. Look at the uploaded picture to cross-reference." | *7 numbers for 8 rows, so one row is missing (probably the row 2/3 pair at about 80 mm). "Outside of holes" may mean the hole's far edge, so centre = reading − r. First mapping, with the case top edge at KiCad y ≈ 56.9: 8.9 = top corners; 69.95 = H1/H3 (photo y ≈ 126.2 → 69.3); 89.6 = H4 (146.5 → 89.6); 120.45 = H2/H9 (175.6 → 118.7); 130.2 = H6 (187.2 → 130.3); 142.4 = H13/H14 (198.1 → 141.2); 154.6 = bottom posts (210.1 → 153.2). Residuals are −0.1 to +1.75, so work out the right edge convention and top reference.* |

## Depths and heights
| ID | Item | Reading | Notes |
|---|---|---|---|
| D1 | Board height above the front floor | 5.5 | |
| D2 | Battery bay depth (old coin-cell corner) | not measured. "You do the math with the rest." | |
| D3 | Space above the board line at the top edge | about 5 | Needs 3.5, or 6 without the notch |
| D4 | Space behind the board at the camera spot | 5.5 | Camera needs 5.4 + 0.3. **Tight.** |
| D5 | Back cover thickness at the camera spot | about 1 (edges are rounded) | |
| D6 | Solar box height on the back cover | frame 5–6. "I can grind it with a Dremel." | |
| D7 | Coin-cell holder rib height | 5–6 | |
| D8 | LCD module thickness | 3.75 (glass and metal shield, about half each) | |
| D9 | Key mat: top of the button rubber to the black contact | 1.5 | |
| D10 | Closed calculator thickness | 11.3; the top part is a little more, 11.7–11.8 | **Not 13.8.** The CAD replica must use these. |
| D12 | Back-cover centre rib start/stop | unknown. "You figure it out." | Use photo 2f54d6c7 |
| D13 | Back-cover rib and ring heights | 1 / 4 / 4 / 1. The solar panel grid is 5 high; "will probably have to grind it". | Order of the D13 sheet: rib grid / small ring / big ring / lower ribs (see stage10_fx115es.md) |

## Parts already in hand
| ID | Item | Reading |
|---|---|---|
| P3 | E-paper outline × thickness | 59.0 × 29.2 × unknown (Waveshare drawing: about 1.0) |
| P5 | Magnetic connector | not here yet |

## Photos
- F6 flatbed scan: none. Nirav asked whether Purdue has a scanner. *Purdue libraries have free flatbed scanners. Also, any phone scan app on a flat surface with a ruler in frame works.*

## Decisions
| ID | Question | Answer |
|---|---|---|
| Q1 | Magnetic cable: solder a USB cable or buy ready-made? | "Whichever is cheaper and easier." → **buy the ready-made Adafruit #5412** |
| Q2 | 3.3 V regulator | "Whichever is cheaper and easier." Stage 11 already switched to RT9080 (about 4–6 months standby vs about 6 weeks). Check whether it's a JLCPCB extended part (+$3 per order) vs AP2112K. |
| Q3 | JST-PH 2.0 mm battery plug | **Yes** |

Still to do (must): D11, E1, E6, E7, F1, F3, F4.
