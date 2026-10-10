"""Real screenshots of the v15 colour-LCD UI, drawn by the firmware's own code.

    python tools/ui_sim_v15/make_screens.py          (from anywhere; needs Pillow + numpy,
                                                      the MSVC Build Tools, and LovyanGFX in
                                                      firmware-v15-lcd/.pio/libdeps: run
                                                      pio run -e v15lcd there once)

Steps:
  1. sample photo: a handwritten maths problem on lined paper, made with Pillow, at the
     camera's preview size (320 x 240)  -> hardware/renders_ui_v15/sample_photo_*.png
  2. build.bat compiles ui_sim_v15.exe (firmware ui.cpp / fbtext.cpp / vf_lcd.cpp /
     selftest.cpp unchanged + core/ + LovyanGFX's LGFX_Sprite) and runs it: every screen
     the panel would receive is saved as a 320 x 170 PPM
  3. PNGs: 320 x 170 (exact panel pixels) and x3 nearest-neighbour for print
  4. sheets: status-bar variants, a contact sheet
  5. framed: screenshots placed into the calculator window of the v15 Fusion renders
     (the chroma-key "_key" renders, same perspective fit as compose_v15.py)
"""
import glob
import os
import random
import shutil
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(ROOT, "hardware", "renders_ui_v15")
X3 = os.path.join(OUT, "x3")
FRAMED = os.path.join(OUT, "framed")
BUILD = os.path.join(HERE, "build")
FRAMES = os.path.join(BUILD, "frames")
ASM = os.path.join(ROOT, "hardware", "enclosure", "final_assembly_v15_lcd")
sys.path.insert(0, ASM)
import compose_v15  # noqa: E402  (green_mask / corners / persp_coeffs: the chroma-key fit)

FONTS = r"C:\Windows\Fonts"
LABEL = os.path.join(FONTS, "arialbd.ttf")
LABEL_REG = os.path.join(FONTS, "arial.ttf")


# ---------------------------------------------------------------- 1. sample photo
def sample_photo():
    """A phone-camera-like picture of a handwritten problem: lined notebook paper under a
    warm desk lamp, a little rotated, soft, with sensor noise. Nothing else in the frame
    (core's text finder boxes any 8x8 block with edges both ways, so clutter widens the box). Drawn at 4x, then reduced to
    the OV5640 preview size (320 x 240)."""
    rnd = random.Random(15)
    W, H = 1280, 960
    paper = Image.new("RGB", (W, H), (236, 232, 220))
    d = ImageDraw.Draw(paper)
    for y in range(90, H, 72):  # ruled lines + margin
        d.line([(0, y), (W, y)], fill=(150, 182, 214), width=3)
    d.line([(150, 0), (150, H)], fill=(222, 128, 128), width=3)
    ink = (28, 40, 92)
    script = ImageFont.truetype(os.path.join(FONTS, "segoepr.ttf"), 62)
    big = ImageFont.truetype(os.path.join(FONTS, "segoeprb.ttf"), 132)

    def scrawl(xy, text, font, jitter=4):
        x, y = xy
        for ch in text:
            dy = rnd.uniform(-jitter, jitter)
            d.text((x, y + dy), ch, font=font, fill=ink)
            x += d.textlength(ch, font=font) + rnd.uniform(-2, 3)
        return x

    scrawl((200, 210), "4)  Solve for x:", script)
    scrawl((290, 360), "3x + 7 = 22", big, jitter=7)
    d.line([(300, 530), (930, 545)], fill=ink, width=5)  # underline, a little crooked
    scrawl((215, 600), "(show your work)", ImageFont.truetype(os.path.join(FONTS, "segoepr.ttf"), 48))

    # a little rotated; drawn larger than the frame so the paper fills it after the turn
    img = paper.rotate(-3.5, resample=Image.BICUBIC, expand=False, fillcolor=(236, 232, 220))
    img = img.crop((40, 30, W - 40, H - 30)).resize((W, H), Image.BICUBIC)
    # warm lamp from the upper left + vignette
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    light = 1.05 - 0.22 * np.hypot((xx - 0.25 * W) / W, (yy - 0.2 * H) / H)
    a = np.asarray(img).astype(np.float32) * light[..., None]
    a *= np.array([1.04, 1.0, 0.9], np.float32)  # warm white balance
    img = Image.fromarray(np.clip(a, 0, 255).astype(np.uint8))
    img = img.filter(ImageFilter.GaussianBlur(2.2)).resize((320, 240), Image.LANCZOS)
    a = np.asarray(img).astype(np.float32)
    a += np.random.default_rng(15).normal(0, 3.0, a.shape)
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8))


# ---------------------------------------------------------------- 2. run the firmware code
def run_harness(photo_ppm):
    r = subprocess.run(["cmd", "/c", os.path.join(HERE, "build.bat")], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-4000:], r.stderr[-2000:])
        raise SystemExit("build failed")
    shutil.rmtree(FRAMES, ignore_errors=True)
    os.makedirs(FRAMES)
    r = subprocess.run([os.path.join(BUILD, "ui_sim_v15.exe"), FRAMES, photo_ppm], capture_output=True, text=True)
    print(r.stdout)
    if r.returncode != 0:
        print(r.stderr)
        raise SystemExit("harness failed")


# Self-test frames are saved in push order: "Screen pattern...", the colour bars,
# "Camera...", "Wi-Fi scan...", the key count (several), the result.
def selftest_names(frames):
    names = {frames[0]: "60_selftest_start", frames[1]: "61_selftest_colour_bars",
             frames[2]: "62_selftest_camera", frames[3]: "63_selftest_wifi"}
    keys = frames[4:-1]
    if keys:
        names[keys[len(keys) // 2]] = "64_selftest_keys"
    names[frames[-1]] = "65_selftest_pass"
    return names


# ---------------------------------------------------------------- 3-4. PNGs and sheets
def label_font(size, bold=True):
    return ImageFont.truetype(LABEL if bold else LABEL_REG, size)


def status_sheet(shots):
    rows = [("30_status_full_no_wifi", "100 %, Wi-Fi off (no icon)"),
            ("31_status_wifi_joining", "Wi-Fi on, joining (outline bars)"),
            ("32_status_wifi_connected", "Wi-Fi connected (solid bars)"),
            ("33_status_charging", "charging: blue fill + bolt"),
            ("34_status_low_battery", "15 %: red fill and red %"),
            ("35_status_battery_unknown", "battery not read yet: outline only"),
            ("36_low_battery_ai_screen", "6 %, AI locked out"),
            ("42_exam_mode_on", "exam mode: red bar"),
            ("06_calc_shift", "SHIFT on (amber S)")]
    S = 3
    bar_h = 24 * S
    W = 320 * S + 560
    sheet = Image.new("RGB", (W, 60 + len(rows) * (bar_h + 16)), (255, 255, 255))
    d = ImageDraw.Draw(sheet)
    d.text((20, 14), "v15 LCD status bar (top 24 px of the real screens, x3)", font=label_font(26), fill=(20, 20, 20))
    y = 60
    for name, text in rows:
        im = shots[name].crop((0, 0, 320, 24)).resize((320 * S, bar_h), Image.NEAREST)
        sheet.paste(im, (20, y))
        d.text((40 + 320 * S, y + bar_h // 2), text, font=label_font(22, False), fill=(30, 30, 30), anchor="lm")
        y += bar_h + 16
    return sheet


def contact_sheet(shots, names):
    S, cols, pad, cap = 2, 4, 18, 30
    cw, ch = 320 * S, 170 * S
    rows = (len(names) + cols - 1) // cols
    sheet = Image.new("RGB", (pad + cols * (cw + pad), 70 + rows * (ch + cap + pad)), (245, 245, 243))
    d = ImageDraw.Draw(sheet)
    d.text((pad, 18), "v15 colour LCD: every screen, drawn by the firmware's own code (x2)", font=label_font(30),
           fill=(20, 20, 20))
    for i, n in enumerate(names):
        x = pad + (i % cols) * (cw + pad)
        y = 70 + (i // cols) * (ch + cap + pad)
        sheet.paste(shots[n].resize((cw, ch), Image.NEAREST), (x, y))
        d.text((x, y + ch + 4), n, font=label_font(20, False), fill=(40, 40, 40))
    return sheet


# ---------------------------------------------------------------- 5. framed
def framed(key_png, ui, tint=0.10):
    """The screenshot warped into the green (chroma-key) active area of a v15 render, as
    compose_v15.screen_on() does, minus its mock-up: the same corner fit and lens tint."""
    im = Image.open(key_png).convert("RGB")
    m = compose_v15.green_mask(np.array(im))
    c = compose_v15.corners(m)
    W, H = ui.size
    big = ui.resize((W * 4, H * 4), Image.NEAREST)  # keep the pixels crisp before the warp
    co = compose_v15.persp_coeffs(c, [(0, 0), (W * 4, 0), (W * 4, H * 4), (0, H * 4)])
    warped = big.transform(im.size, Image.PERSPECTIVE, co, Image.BICUBIC)
    warped = Image.blend(warped, Image.new("RGB", im.size, (20, 20, 24)), tint)
    mk = Image.fromarray((m * 255).astype(np.uint8)).filter(ImageFilter.MaxFilter(3))
    im.paste(warped, (0, 0), mk)
    return im


def main():
    for p in (OUT, X3, FRAMED, BUILD):
        os.makedirs(p, exist_ok=True)
    photo = sample_photo()
    photo.save(os.path.join(OUT, "sample_photo_320x240.png"))
    photo.resize((960, 720), Image.NEAREST).save(os.path.join(X3, "sample_photo_320x240_x3.png"))
    ppm = os.path.join(BUILD, "photo.ppm")
    photo.save(ppm)
    run_harness(ppm)

    raw = sorted(glob.glob(os.path.join(FRAMES, "*.ppm")))
    st = [p for p in raw if os.path.basename(p).startswith("60_selftest_")]
    rename = selftest_names(st)
    shots = {}
    for p in raw:
        if p in st and p not in rename:
            continue  # the other key-count frames
        name = rename.get(p, os.path.splitext(os.path.basename(p))[0])
        im = Image.open(p).convert("RGB")
        shots[name] = im
    for old in glob.glob(os.path.join(OUT, "[0-9][0-9]_*.png")) + glob.glob(os.path.join(X3, "[0-9][0-9]_*.png")):
        os.remove(old)
    for name, im in shots.items():
        im.save(os.path.join(OUT, name + ".png"))
        im.resize((960, 510), Image.NEAREST).save(os.path.join(X3, name + "_x3.png"))

    status_sheet(shots).save(os.path.join(OUT, "sheet_status_bar_variants.png"))
    contact_sheet(shots, sorted(shots)).save(os.path.join(OUT, "sheet_all_screens.png"))

    picks = ["03_calc_result", "05_calc_result_functions", "10_menu_mode", "20_ai_ready", "24_ai_answer_page1",
             "50_setup_by_phone", "53_ota_progress_42", "61_selftest_colour_bars", "72_camera_focused",
             "40_off_charging"]
    for f in glob.glob(os.path.join(FRAMED, "*.png")):
        os.remove(f)
    renders = os.path.join(ASM, "renders")
    for view in ("front", "display"):
        for n in picks:
            framed(os.path.join(renders, "%s_screen_key.png" % view), shots[n]).save(
                os.path.join(FRAMED, "%s_%s.png" % (view, n)))
    print("%d screens -> %s" % (len(shots), OUT))


if __name__ == "__main__":
    main()
