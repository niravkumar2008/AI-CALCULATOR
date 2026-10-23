# TV mount adapter plate

Adapts a TV with a 100x100 VESA pattern and M6 inserts to a Suptek MD6TB monitor plate
(75x75 / 100x100, drilled for M4).

```
TV  |  printed plate (14 mm)  |  Suptek steel plate (2 mm)  |  arm head  |  pole
       M6 x 16 bolts into TV      M4 x 15 screws into the printed plate
```

| Feature | Size | Position from centre |
|---|---|---|
| M6 clearance holes (through) | 6.6 mm | ±50 mm (100x100) |
| Bolt-head pockets (Suptek side) | 13 mm wide, 8 mm deep | ±50 mm |
| M4 pilot holes (through) | 3.6 mm | ±37.5 mm (75x75) |
| Outline | 125 x 125 x 14 mm, 6 mm corner radius | |

## Build it in Fusion 360

1. In Fusion: **Utilities > Add-Ins > Scripts and Add-Ins**.
2. Click **+** next to *My Scripts* and pick the `fusion/TVMountPlate` folder.
3. Select **TVMountPlate** and click **Run**.

The script builds the plate in a new design, then checks its size and volume
(expected 125 x 125 x 14 mm, 212.68 cm³) and shows the result.
Export with right-click on the body > **Save As Mesh** > 3MF.

To make a variant, edit the values in `P` at the top of `TVMountPlate.py` and run it again.

## Ready-made files

- `tv_vesa_adapter_plate.3mf` – the plate, centred on a 256 mm bed.
- `fit_template_2mm.3mf` – 2 mm template with the same hole positions, for a quick fit check.

`mesh/` holds the scripts that generated these files (`gen_plate.py`), measure the holes in them
(`verify.py`), and check the assembly against common M6 and M4 hardware (`assembly_check.py`).
They need `manifold3d`, `trimesh` and `scipy`.

## Printing (Bambu X1C, PLA)

`0.20mm Strength` preset, 6 walls, 6 top / 6 bottom layers, 40 % gyroid infill,
supports off, 5 mm brim. Print TV face down, as the files load.
