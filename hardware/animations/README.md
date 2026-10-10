# Animations of the finished calculator

Animated GIFs for the build guides and the project hub (`hardware/project_hub/index.html`, section "See it come together").
They are rendered from the 1:1 Fusion 360 final-assembly models. Only the camera and the part positions are animated: no geometry was changed and nothing was saved in Fusion.

All files are 960 × 720 and loop forever. The `*_poster.jpg` files are the last frame of each GIF. The hub shows them instead of the GIF when "reduce motion" is turned on.

| File | What it shows | Frames | Size |
|---|---|---|---|
| `v15/assembly_v15.gif` | Empty faceplate, then each part drops in with a caption per stage: 1 key mat, 2 screen + ribbon, 3 board onto the posts, 4 camera + ribbon, 5 magnet into J3, 6 battery, 7 back cover + screws. Then the view swings to the front and the colour screen comes on. | 44 | 1.72 MB |
| `v15/turntable_v15.gif` | Closed calculator turning 360° (front and back), screen on | 36 | 1.43 MB |
| `v15/xray_v15.gif` | See-through shells, assembled ↔ exploded and back | 24 | 2.93 MB |
| `v14/assembly_v14.gif` | Same steps for the e-paper build | 44 | 1.75 MB |
| `v14/turntable_v14.gif` | Closed v14, 360° | 36 | 1.38 MB |
| `v14/xray_v14.gif` | v14 x-ray, exploded ↔ assembled | 24 | 2.91 MB |
| `v15/*_poster.jpg`, `v14/*_poster.jpg` | Still of the last frame | 1 | 51–65 kB each |

No MP4 files: ffmpeg is not installed on this PC. If `ffmpeg` is on PATH, the script also writes `<anim>_<ver>.mp4` next to each GIF.

The v15 screen image is the real firmware screenshot `hardware/renders_ui_v15/05_calc_result_functions.png` (`sin(30)+π×2²−log(100)` = 11.06637061). Fusion renders the LCD's active area in chroma-key green. Pillow then fits the screenshot into it with a perspective warp, using the screen's four corners projected from the model (the same method as `compose_v15.py`). While the calculator is still being built, the screen is shown as dark glass (off).

The order follows the steps as you asked for them. The guides do it slightly differently: screen, camera and magnet go onto the board first (steps 05–07), then the key mat and board go into the shell (step 08).

## Regenerate

Fusion 360 must be open with the FusionMCPBridge add-in and both final-assembly documents loaded: v14 (`Final - …`) and v15 (`V15 - …`). Opening `final_assembly/ai_calculator_final_assembly.f3d` and `final_assembly_v15_lcd/ai_calculator_v15_lcd_final_assembly.f3d` is enough. Then run:

```
cd hardware/enclosure
python animate_assembly.py ver=v15 anim=all      # about 6 min: render in Fusion, then build the GIFs
python animate_assembly.py ver=v14 anim=all      # about 1 min
python animate_assembly.py compose ver=v15       # rebuild GIFs only, from frames already rendered (no Fusion)
python animate_assembly.py ver=v15 anim=assembly pick=0,20,43   # quick look at a few frames
```

Then copy the GIFs and posters to `hardware/project_hub/img/` as `anim_<anim>_<ver>.gif` / `anim_<anim>_<ver>_poster.jpg`.

- Raw frames (1280 × 960 PNG plus `frames.json` with the captions, durations and screen corners) go to `_frames/<ver>/<anim>/`. That folder is git-ignored and takes about 50 MB.
- Every change made in Fusion is restored at the end of a run: part positions, visibility, the x-ray opacity and the screen marker's appearance. The document is never saved.
- Fusion's progress is logged to `hardware/enclosure/animate_assembly.log`.
- The GIFs use one shared, optimised palette (median cut, no dithering). If a GIF goes over its limit (assembly 6 MB, turntable 4 MB), the script retries with fewer colours.
