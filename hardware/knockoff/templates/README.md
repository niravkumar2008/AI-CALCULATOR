# Marking templates for the knockoff shell (camera hole + magnet notch)

Made 2026-10-15 from the knockoff CAD model (`../build_knockoff_assembly.py`). One printed cap marks **both** spots on the
closed calculator; the paper version does the same with scissors and tape.

| File | What it is |
|---|---|
| `T1_top_end_cap.stl` | **T1 top-end cap.** Slides over the top (screen) end of the closed calculator. Back face: a Ø7 hole on the camera centre. Top wall: a window that is exactly the magnet U-notch. |
| `T2_centre_plug.stl` | **T2 centre plug.** Pushes into T1's Ø7 hole from outside; its Ø2.1 hole is the camera centre (pilot drill guide). |
| `paper_templates.pdf` | **Paper version**, 2 pages, US Letter, 1:1 (page 1 back cover + camera; page 2 top-edge strip + notch; ruler-only numbers at the end). |
| `render_T1_*.png` | Pictures of the cap on the calculator. |
| `templates_check.json` | Fusion check: 0 overlap with every part of the closed calculator; 0.35 mm clearance round the sides. |
| `build_templates.py`, `make_paper_templates.py` | Rebuild the STLs / the PDF if the model changes. |
| `knockoff_window_mask_template.pdf` / `.svg` / `.dxf` | **Screen cover:** 1:1 cut sheet for the clear lens (65.35 × 29.00, R3.35) and the black vinyl mask with its centred opening (43.72 × 23.70). The mask hides the LCD ribbon so the screen looks centred. DXF is for a laser or vinyl cutter. |
| `knockoff_lcd_position_jig.pdf` / `.svg` | **LCD placement:** 1:1 paper jig laid on the key side of the board; the panel drops into the cut-out window, tail towards the slot. |
| `knockoff_window_mask_template.py`, `knockoff_lcd_position_jig.py` | Rebuild those two sheets (numbers from `../cad/assembly/placement.json` and the v15 KiCad board). |

## The spots (front view = keys facing you)
- **Camera:** centred left to right, **40.1 mm below the top edge**, Ø7 window (camera module 8.5 × 8.5 between back ribs 2 and 3).
- **Magnet notch:** in the top wall, **6.4 to 27.9 mm in from the right-hand outer edge**, from the **seam** (where the two halves meet) **7.95 mm** towards the keys. The back cover's lip behind it is snipped to the same width.

## Printing T1 and T2
- PLA or PETG, 0.2 mm layers, 3 walls, 20 % infill. No supports.
- **T1:** back plate flat on the bed (the walls and the lip point up). The notch window and the lip print as short bridges.
- **T2:** flange on the bed.
- Print time about 1 h; about 9 g of filament.
- If T1 is tight, sand the inside of the side walls; if loose, a strip of tape on the calculator's sides takes the play out.

## Using T1 (≈ 2 minutes per calculator)
1. Close the calculator with its screws in. Slide T1 over the top end, back plate on the back cover, the lip over the top edge of the face. Push it fully home.
2. **Camera:** press T2 into the 7 mm hole. Mark the centre through T2 with a sharp scriber or a 2 mm drill turned by hand. Or trace the 7 mm circle with a fine marker straight through T1.
3. **Magnet notch:** trace the window in T1's top wall with a sharp pencil. The window's bottom edge sits on the seam.
4. Take T1 off, open the calculator, then drill and cut (grinding guide, zones "camera_window" and "magnet_slot"). Drill the camera hole **2 mm first, then 7 mm with a step drill from the outside**.

## Using the paper version
Print `paper_templates.pdf` at **100 % / Actual size** and check that the 50 mm bar measures 50 mm. Then follow the steps on each page.

## Before the first real cut
The positions come from photos (about ±1.5 mm). On the **first** back cover, open it and check from the inside that the camera mark sits **between ribs 2 and 3** (35.0 and 45.2 mm below the top edge). The camera has only about 0.35 mm to spare on each side. If a rib is in the way, file about 10 mm of it flat.
