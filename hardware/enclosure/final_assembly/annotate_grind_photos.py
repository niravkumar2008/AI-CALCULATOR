"""Annotate Nirav's shell photos for the Shell Grinding Guide (grinding_manual.html).

Reads the original phone photos from the Claude uploads folders, draws red (remove), orange (only if needed)
and green (keep) marks, and writes JPEGs to renders/photos/grind_*.jpg.

Coordinates are given in a 750 x 1000 frame (photo resized to 1000 px tall, EXIF-rotated) and scaled to the
output size. Positions on the photos are approximate: the printed grind maps and the calipers decide.

    python annotate_grind_photos.py
"""
import glob
import os

from PIL import Image, ImageDraw, ImageFont, ImageOps

UP = os.path.expanduser("~/.claude/uploads")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "renders", "photos")
FONT = "C:/Windows/Fonts/arialbd.ttf"
OUT_W = 900  # output width; height follows the photo
RED, ORANGE, GREEN, YELLOW, WHITE, INK = "#e0241b", "#f08c00", "#1f9d4a", "#f5c400", "#ffffff", "#1b1f2a"


def load(key):
    f = glob.glob(os.path.join(UP, "*", key + "*"))[0]
    im = ImageOps.exif_transpose(Image.open(f)).convert("RGB")
    w, h = im.size
    base = im.resize((w * 1000 // h, 1000))  # the frame the coordinates were read in
    s = OUT_W / base.width
    return im.resize((OUT_W, round(1000 * s)), Image.LANCZOS), s


class Ann:
    def __init__(self, key):
        self.im, self.s = load(key)
        self.d = ImageDraw.Draw(self.im)

    def p(self, *xy):
        return [v * self.s for v in xy]

    def font(self, size):
        return ImageFont.truetype(FONT, round(size * self.s))

    def box(self, x0, y0, x1, y1, col=RED, w=4, dash=False):
        x0, y0, x1, y1 = self.p(x0, y0, x1, y1)
        if not dash:
            self.d.rectangle([x0, y0, x1, y1], outline=col, width=w)
            return
        for a, b in (((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)), ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0))):
            self._dash(a, b, col, w)

    def _dash(self, a, b, col, w, on=12, off=8):
        (ax, ay), (bx, by) = a, b
        n = max(abs(bx - ax), abs(by - ay))
        if n == 0:
            return
        t = 0
        while t < n:
            t2 = min(t + on, n)
            self.d.line([(ax + (bx - ax) * t / n, ay + (by - ay) * t / n), (ax + (bx - ax) * t2 / n, ay + (by - ay) * t2 / n)], fill=col, width=w)
            t += on + off

    def dline(self, x0, y0, x1, y1, col=RED, w=5):
        self._dash(tuple(self.p(x0, y0)), tuple(self.p(x1, y1)), col, w)

    def circle(self, x, y, r, col=RED, w=4):
        x, y, r = *self.p(x, y), r * self.s
        self.d.ellipse([x - r, y - r, x + r, y + r], outline=col, width=w)

    def fill(self, x0, y0, x1, y1, col=RED, alpha=70):
        x0, y0, x1, y1 = self.p(x0, y0, x1, y1)
        ov = Image.new("RGBA", self.im.size, (0, 0, 0, 0))
        r, g, b = Image.new("RGB", (1, 1), col).getpixel((0, 0))
        ImageDraw.Draw(ov).rectangle([x0, y0, x1, y1], fill=(r, g, b, alpha))
        self.im = Image.alpha_composite(self.im.convert("RGBA"), ov).convert("RGB")
        self.d = ImageDraw.Draw(self.im)

    def arrow(self, x0, y0, x1, y1, col=RED, w=4):
        import math
        x0, y0, x1, y1 = self.p(x0, y0, x1, y1)
        self.d.line([x0, y0, x1, y1], fill=col, width=w)
        a = math.atan2(y1 - y0, x1 - x0)
        L = 16 * self.s
        pts = [(x1, y1), (x1 - L * math.cos(a - 0.45), y1 - L * math.sin(a - 0.45)), (x1 - L * math.cos(a + 0.45), y1 - L * math.sin(a + 0.45))]
        self.d.polygon(pts, fill=col)

    def label(self, x, y, text, col=RED, size=17, anchor="la"):
        f = self.font(size)
        x, y = self.p(x, y)
        bb = self.d.multiline_textbbox((x, y), text, font=f, anchor=anchor, spacing=3)
        pad = 5 * self.s
        self.d.rectangle([bb[0] - pad, bb[1] - pad, bb[2] + pad, bb[3] + pad], fill=WHITE, outline=col, width=3)
        self.d.multiline_text((x, y), text, font=f, fill=INK, anchor=anchor, spacing=3)

    def tag(self, x, y, text, col=RED, size=20):
        """Round number badge."""
        f = self.font(size)
        x, y = self.p(x, y)
        r = 17 * self.s
        self.d.ellipse([x - r, y - r, x + r, y + r], fill=col, outline=WHITE, width=2)
        self.d.text((x, y), text, font=f, fill=WHITE, anchor="mm")

    def save(self, name):
        path = os.path.join(OUT, name)
        self.im.save(path, quality=80, optimize=True, progressive=True)
        print(name, self.im.size, os.path.getsize(path) // 1024, "KB")


def back_cover():
    a = Ann("2f54d6c7")  # navy back cover, inside up, top end at the top
    # zone 1 solar box
    a.fill(343, 122, 503, 184, RED, 60)
    a.box(343, 122, 503, 184, RED, 5)
    a.tag(325, 112, "1")
    a.label(12, 12, "1  Solar box: grind it ALL\n    flat to the floor (5-5.5 mm)", RED, 15)
    a.arrow(200, 52, 340, 140)
    # zone 2 rib B, 14 mm centred on the camera spot
    a.fill(338, 256, 413, 274, RED, 90)
    a.box(338, 256, 413, 274, RED, 4)
    a.tag(318, 245, "2")
    # zone 3 camera window
    a.circle(375, 265, 19, RED, 4)
    a.tag(432, 245, "3")
    a.label(196, 286, "2  Rib B: only these 14 mm, 1 mm down\n3  Drill 7 mm here (on the centre line)", RED, 14)
    # zone 5b lip
    a.box(452, 96, 566, 112, ORANGE, 4, dash=True)
    a.tag(470, 80, "5b", ORANGE, 15)
    a.label(735, 40, "5b  Lip under the magnet notch:\nonly if your rim has one here", ORANGE, 14, "ra")
    a.arrow(600, 78, 540, 96, ORANGE)
    # keep features
    for (x, y) in ((518, 136), (258, 413), (488, 413)):
        a.circle(x, y, 17, GREEN, 4)
    a.label(735, 452, "LEAVE the screw posts", GREEN, 14, "ra")
    a.arrow(640, 445, 507, 418, GREEN)
    a.circle(365, 505, 62, GREEN, 3)
    a.label(470, 590, "LEAVE the rings,\nrib C and the other ribs", GREEN, 14)
    # v15 DCXYX tuck bay
    a.box(352, 330, 398, 376, GREEN, 3, dash=True)
    a.label(196, 336, "v15 DCXYX camera:\nribbon S-fold bay -\nkeep clean and flat", GREEN, 13)
    a.arrow(330, 352, 351, 352, GREEN)
    a.save("grind_backcover_zones_ann.jpg")


def front_shell():
    a = Ann("76dd27d2")  # silver front shell, face down, top end at the top: solar side on the LEFT
    # zone 5 magnet notch on the top wall
    a.fill(240, 38, 333, 62, RED, 80)
    a.box(240, 38, 333, 62, RED, 4)
    a.tag(222, 26, "5")
    # zone 4 keep
    a.circle(492, 78, 30, GREEN, 4)
    a.label(735, 120, "4  LR44 cup: KEEP", GREEN, 15, "ra")
    # zone 6 band, solar side (left)
    a.box(200, 118, 232, 300, RED, 4, dash=True)
    a.tag(180, 210, "6")
    # 6b both walls
    a.box(200, 110, 232, 310, RED, 2)
    a.box(552, 110, 584, 310, RED, 2)
    a.tag(604, 210, "6b")
    # 6c lower ends
    a.circle(205, 330, 18, ORANGE, 4)
    a.circle(578, 330, 18, ORANGE, 4)
    a.tag(604, 352, "6c", ORANGE, 15)
    a.label(15, 395, "5  Magnet U-notch in the top wall\n    21.5 wide x 7.95 deep from the rim\n    6.4-27.9 mm from THIS (left) outer edge\n"
                      "6  Solar side: file the inner rib down\n    to 7.0 mm below the rim, 14-49 mm\n    from the rim's top edge above the rib\n"
                      "6b Snip pins + hooks, BOTH walls\n6c Rib lower ends: only if needed", RED, 13)
    a.label(735, 940, "Front shell, keys down, top end away.\nThe solar window is on your LEFT now.\nPositions are rough: the printed map decides.", INK, 13, "rd")
    a.save("grind_frontshell_zones_ann.jpg")


def solar_wall():
    a = Ann("32a2eea0")  # solar-window side wall, front shell face down, top at the top
    for (x, y, r) in ((285, 288, 30), (270, 360, 26)):
        a.circle(x, y, r, RED, 5)
    for (x, y) in ((255, 437), (255, 565), (262, 712)):
        a.circle(x, y, 32, RED, 5)
    a.label(330, 236, "6b  U-hooks: snip flush", RED, 16)
    a.arrow(330, 252, 305, 275)
    a.label(355, 470, "6b  Round pins: snip flush\n      with the rib (3 here)", RED, 16)
    a.arrow(355, 500, 290, 552)
    a.arrow(355, 470, 284, 440)
    a.dline(303, 281, 303, 785, ORANGE, 6)
    a.label(330, 800, "6  The thin inner rib these grow from:\n    file its top edge down to 7.0 mm\n    below the rim, 14-49 mm from the\n    rim's top edge right above this rib", ORANGE, 15)
    a.arrow(330, 812, 306, 760, ORANGE)
    a.label(16, 30, "Solar-window side wall (old wires still in)", INK, 15)
    a.label(16, 960, "Outer skin + channel floor: don't touch", GREEN, 14, "ld")
    a.save("grind_solar_wall_6_6b_ann.jpg")


def j2_wall():
    a = Ann("96ae606b")  # J2-side wall with the paper template in, face down
    a.circle(412, 182, 24, RED, 4)
    for (x, y) in ((412, 222), (405, 320), (400, 465)):
        a.circle(x, y, 24, RED, 4)
    a.circle(375, 590, 30, RED, 4)
    a.label(470, 250, "6b  Snip every pin\n      and hook flush", RED, 15, "ra")
    a.label(80, 660, "6c  The rib's lower end: file only\n      if the paper's wide corner\n      rides up here after 6b", ORANGE, 14)
    a.circle(365, 650, 26, ORANGE, 4)
    a.label(30, 40, "J2-side wall: 6b snips (6c only if needed)", INK, 15)
    a.label(20, 520, "Paper edge runs right\nthrough the pin bosses:\nthey must go", RED, 14)
    a.arrow(200, 560, 380, 470)
    a.save("grind_j2_wall_6b_ann.jpg")


def paper_tab():
    a = Ann("31dc0015")  # paper template in the front shell, solar side, before zone 6
    a.box(262, 418, 318, 592, YELLOW, 5)
    a.label(330, 610, "Yellow antenna tab: lies ON the rib now.\nAfter zone 6 + 6b it must drop in flat.", ORANGE, 15)
    a.arrow(330, 625, 300, 590, ORANGE)
    a.box(268, 212, 332, 348, RED, 4, dash=True)
    a.label(20, 150, "Board's top corner\n(15-27 mm from the top):\nalso over the rib", RED, 14)
    a.arrow(150, 215, 265, 260)
    for (x, y) in ((245, 378), (245, 470), (243, 588)):
        a.circle(x, y, 16, RED, 3)
    a.label(20, 450, "pins (6b)", RED, 14)
    a.arrow(105, 462, 228, 470)
    a.save("grind_paper_tab_ann.jpg")


def paper_riding():
    a = Ann("f863a147")  # paper template riding up in the front shell (cropped: no laptop screen)
    a.arrow(300, 330, 226, 428, RED, 5)
    a.label(240, 300, "BAD: the paper lifts here =\nsomething is still in the way", RED, 17)
    a.label(30, 810, "Find the high spot, take a little more off,\nlay the paper in again. Repeat until flat.", INK, 16)
    a.im = a.im.crop((0, round(285 * a.s), a.im.width, round(870 * a.s)))
    a.save("grind_paper_not_flat_ann.jpg")


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    back_cover()
    front_shell()
    solar_wall()
    j2_wall()
    paper_tab()
    paper_riding()
