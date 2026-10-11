"""v15-LCD pictures that need image work (pure Python, Pillow + numpy; reads the Fusion renders, writes renders/):

  1. mock_ui_320x170.png   a mock colour calculator screen at the panel's real resolution (320 x 170, landscape)
  2. *_screen_on.png       the mock UI pasted into the chroma-key renders (front, display close-up, hero) with a
                           perspective fit to the four corners of the green active area
  3. compare_v14_v15_*.png side by side with the v14 e-paper renders (read only from ../final_assembly/renders)

    python compose_v15.py
"""
import os
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
RD = os.path.join(HERE, "renders")
V14 = os.path.join(HERE, "..", "final_assembly", "renders")      # read only
FONT = r"C:\Windows\Fonts\consola.ttf"
FONTB = r"C:\Windows\Fonts\consolab.ttf"
SANS = r"C:\Windows\Fonts\arialbd.ttf"


def mock_ui():
    W, H = 320, 170
    im = Image.new("RGB", (W, H), (12, 16, 28))
    d = ImageDraw.Draw(im)
    f_s = ImageFont.truetype(SANS, 11)
    f_in = ImageFont.truetype(FONT, 20)
    f_res = ImageFont.truetype(FONTB, 34)
    f_t = ImageFont.truetype(SANS, 10)
    # status bar
    d.rectangle([0, 0, W, 17], fill=(28, 36, 60))
    d.text((5, 2), "MATH  DEG", font=f_s, fill=(120, 200, 255))
    d.text((112, 2), "AI", font=f_s, fill=(255, 190, 60))
    d.rectangle([272, 4, 300, 13], outline=(200, 220, 200)); d.rectangle([300, 7, 302, 10], fill=(200, 220, 200))
    d.rectangle([274, 6, 294, 11], fill=(90, 220, 110))
    for i, h in enumerate((3, 6, 9)):
        d.rectangle([250 + i * 5, 14 - h, 253 + i * 5, 14], fill=(200, 220, 255))
    # input line (natural display)
    d.text((8, 28), "∫", font=ImageFont.truetype(FONT, 34), fill=(240, 240, 240))
    d.text((24, 22), "2", font=f_t, fill=(240, 240, 240)); d.text((24, 52), "0", font=f_t, fill=(240, 240, 240))
    d.text((36, 34), "x²·sin(x) dx", font=f_in, fill=(240, 240, 240))
    d.line([185, 36, 185, 58], fill=(255, 255, 255), width=2)            # cursor
    # result
    d.text((W - 10, 108), "= 2.4694", font=f_res, fill=(110, 230, 140), anchor="rs")
    # AI hint bar
    d.rounded_rectangle([6, 120, W - 6, 146], 6, fill=(36, 48, 90))
    d.text((14, 126), "Photo solved: integration by parts, 2 steps", font=f_t, fill=(200, 215, 255))
    # soft keys
    for i, (lbl, col) in enumerate((("STEPS", (255, 190, 60)), ("GRAPH", (120, 200, 255)), ("CAM", (255, 120, 150)), ("HIST", (170, 170, 190)))):
        x0 = 6 + i * 78
        d.rounded_rectangle([x0, 151, x0 + 72, 166], 4, outline=col)
        d.text((x0 + 36, 159), lbl, font=f_t, fill=col, anchor="mm")
    return im


def green_mask(a):
    r, g, b = a[..., 0].astype(int), a[..., 1].astype(int), a[..., 2].astype(int)
    return (g > r + 25) & (g > b + 25)


def _hull(pts):
    pts = sorted(set(pts))
    cross = lambda o, a, b: (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lo, up = [], []
    for p in pts:
        while len(lo) >= 2 and cross(lo[-2], lo[-1], p) <= 0: lo.pop()
        lo.append(p)
    for p in reversed(pts):
        while len(up) >= 2 and cross(up[-2], up[-1], p) <= 0: up.pop()
        up.append(p)
    return lo[:-1] + up[:-1]


def corners(m):
    """Four corners of the (projected rectangle) green area: the largest-area quad on its convex hull,
    clockwise on screen, starting at the corner nearest the image's top-left."""
    ys, xs = np.nonzero(m)
    h = _hull(list(zip(xs.tolist(), ys.tolist())))
    if len(h) > 40:
        h = [h[int(i * len(h) / 40)] for i in range(40)]
    area = lambda q: abs(sum(q[i][0] * q[(i + 1) % 4][1] - q[(i + 1) % 4][0] * q[i][1] for i in range(4))) / 2
    n = len(h); best = None
    for a in range(n):
        for b in range(a + 1, n):
            for c in range(b + 1, n):
                for d in range(c + 1, n):
                    q = [h[a], h[b], h[c], h[d]]; s = area(q)
                    if best is None or s > best[0]: best = (s, q)
    q = best[1]
    cx, cy = sum(p[0] for p in q) / 4, sum(p[1] for p in q) / 4
    q.sort(key=lambda p: np.arctan2(p[1] - cy, p[0] - cx))          # clockwise on screen (y down)
    L = lambda i: np.hypot(q[(i + 1) % 4][0] - q[i][0], q[(i + 1) % 4][1] - q[i][1])
    long_ = [i for i in range(4) if L(i) >= L((i + 1) % 4)]            # edges along the screen's 320-px side
    k = min(long_, key=lambda i: q[i][1] + q[(i + 1) % 4][1])          # the upper one on screen = the display's top edge
    return q[k:] + q[:k]


def persp_coeffs(dst, src):
    """PIL PERSPECTIVE coefficients mapping output (dst) points to input (src) points."""
    A, B = [], []
    for (x, y), (u, v) in zip(dst, src):
        A.append([x, y, 1, 0, 0, 0, -u * x, -u * y]); B.append(u)
        A.append([0, 0, 0, x, y, 1, -v * x, -v * y]); B.append(v)
    return np.linalg.solve(np.array(A, float), np.array(B, float)).tolist()


def screen_on(src, dst, ui):
    im = Image.open(os.path.join(RD, src)).convert("RGB")
    a = np.array(im)
    m = green_mask(a)
    if m.sum() < 100:
        raise SystemExit("no chroma area in " + src)
    c = corners(m)
    W, H = ui.size
    big = ui.resize((W * 4, H * 4), Image.LANCZOS)
    co = persp_coeffs(c, [(0, 0), (W * 4, 0), (W * 4, H * 4), (0, H * 4)])
    warped = im.size and big.transform(im.size, Image.PERSPECTIVE, co, Image.BICUBIC)
    # light lens tint + slight glow so it reads as a lit screen behind the grey lens
    warped = Image.blend(warped, Image.new("RGB", im.size, (20, 20, 24)), 0.12)
    mk = Image.fromarray((m * 255).astype(np.uint8)).filter(ImageFilter.MaxFilter(3))
    im.paste(warped, (0, 0), mk)
    im.save(os.path.join(RD, dst))
    return c


def label(im, text, size=34):
    d = ImageDraw.Draw(im)
    f = ImageFont.truetype(SANS, size)
    d.rectangle([0, 0, im.size[0], size + 20], fill=(255, 255, 255))
    d.text((im.size[0] // 2, 10 + size // 2), text, font=f, fill=(20, 20, 20), anchor="mm")
    return im


def side_by_side(a_path, b_path, out, la, lb):
    A = Image.open(a_path).convert("RGB"); B = Image.open(b_path).convert("RGB")
    h = min(A.size[1], B.size[1])
    A = A.resize((A.size[0] * h // A.size[1], h)); B = B.resize((B.size[0] * h // B.size[1], h))
    out_im = Image.new("RGB", (A.size[0] + B.size[0] + 20, h + 60), (255, 255, 255))
    out_im.paste(label(A, la), (0, 60 - 60)); out_im.paste(label(B, lb), (A.size[0] + 20, 0))
    out_im.save(os.path.join(RD, out))


if __name__ == "__main__":
    ui = mock_ui()
    ui.save(os.path.join(RD, "mock_ui_320x170.png"))
    ui.resize((1280, 680), Image.NEAREST).save(os.path.join(RD, "mock_ui_320x170_x4.png"))
    for s in ("front", "display", "hero"):
        c = screen_on("%s_screen_key.png" % s, "%s_screen_on.png" % s, ui)
        print(s, "corners", c)
    side_by_side(os.path.join(V14, "mask_after_display.png"), os.path.join(RD, "display_screen_on.png"),
                 "compare_v14_v15_display.png", "v14: 2.13in e-paper (masked)", "v15: 1.9in colour IPS LCD (masked)")
    side_by_side(os.path.join(V14, "compare", "front_cad.png"), os.path.join(RD, "front_screen_on.png"),
                 "compare_v14_v15_front.png", "v14 e-paper", "v15-LCD")
    side_by_side(os.path.join(V14, "section_screen_fpc_end.png"), os.path.join(RD, "section_lcd_fpc_end.png"),
                 "compare_v14_v15_fpc_end_section.png", "v14: e-paper FPC fold + J2", "v15: LCD FPC Z-fold + J5")
    print("ok")
