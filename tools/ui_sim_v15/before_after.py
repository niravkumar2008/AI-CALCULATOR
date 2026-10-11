"""Side-by-side sheet: hardware/renders_ui_v15/before_fixes/*.png (old screenshots) next to
the current ones, for every screen whose pixels changed.

    python tools/ui_sim_v15/before_after.py      (after make_screens.py)
    -> hardware/renders_ui_v15/sheet_before_after.png
"""
import glob
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.normpath(os.path.join(HERE, "..", "..", "hardware", "renders_ui_v15"))
BEFORE = os.path.join(OUT, "before_fixes")
FONT = r"C:\Windows\Fonts\arialbd.ttf"
FONT_REG = r"C:\Windows\Fonts\arial.ttf"


def main():
    pairs = []
    for b in sorted(glob.glob(os.path.join(BEFORE, "[0-9][0-9]_*.png"))):
        name = os.path.basename(b)
        a = os.path.join(OUT, name)
        if not os.path.exists(a):
            continue
        ib, ia = Image.open(b).convert("RGB"), Image.open(a).convert("RGB")
        if ib.size == ia.size and np.array_equal(np.asarray(ib), np.asarray(ia)):
            continue
        pairs.append((os.path.splitext(name)[0], ib, ia))
    S, pad, cap = 2, 16, 26
    cw, ch = 320 * S, 170 * S
    pair_w = 2 * cw + pad
    cols = 2
    rows = (len(pairs) + cols - 1) // cols
    W = pad + cols * (pair_w + 2 * pad)
    H = 80 + rows * (ch + cap + pad)
    sheet = Image.new("RGB", (W, H), (245, 245, 243))
    d = ImageDraw.Draw(sheet)
    title = ImageFont.truetype(FONT, 30)
    lab = ImageFont.truetype(FONT_REG, 20)
    d.text((pad, 14), "v15 LCD UI fixes: before (left) / after (right), %d changed screens, x2" % len(pairs),
           font=title, fill=(20, 20, 20))
    for i, (name, ib, ia) in enumerate(pairs):
        x = pad + (i % cols) * (pair_w + 2 * pad)
        y = 70 + (i // cols) * (ch + cap + pad)
        sheet.paste(ib.resize((cw, ch), Image.NEAREST), (x, y))
        sheet.paste(ia.resize((cw, ch), Image.NEAREST), (x + cw + pad, y))
        d.text((x, y + ch + 3), name + "  before", font=lab, fill=(150, 40, 40))
        d.text((x + cw + pad, y + ch + 3), "after", font=lab, fill=(30, 110, 50))
    path = os.path.join(OUT, "sheet_before_after.png")
    sheet.save(path)
    print("%d changed screens -> %s" % (len(pairs), path))


if __name__ == "__main__":
    main()
