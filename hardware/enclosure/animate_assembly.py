"""Animated pictures of the finished calculator for the guides and the project hub (render + camera code only: no model
geometry is changed or saved, nothing is exported from Fusion).

    python animate_assembly.py ver=v15 anim=assembly        render the frames in Fusion, then build the GIF
    python animate_assembly.py ver=v14 anim=all             assembly + turntable + xray
    python animate_assembly.py compose ver=v15 [anim=..]    only rebuild the GIFs from frames already rendered (no Fusion)

Animations (hardware/animations/<ver>/):
  assembly   in the order of the build guides (step numbers in the caption bar): the bare board, screen + ribbon onto
             its key side (05), board turned over, camera + ribbon into J1 (06), magnet into J3 (07); then the empty
             faceplate, key mat in and the board onto the posts (08), battery + plug into J4 (09), back cover + screws
             (10); 4 move frames per part, then the camera swings round to the front and the screen comes ON (11).
             v15: real firmware screenshot (hardware/renders_ui_v15/). v14: a 250 x 122 1-bit e-paper image drawn with
             the firmware's 5x7 font (core/font_data.cpp). Both are warped onto a chroma-key active area.
  turntable  the closed calculator turning 360 deg about its long axis (front and back), 36 frames, screen on.
  xray       exploded <-> assembled loop with the shells see-through, 24 frames.

Fusion side: like guide_renders.py. Fusion must be open with the FusionMCPBridge add-in and the matching final-assembly
document loaded (v14: occurrences "Final - ...", v15: "V15 - ..."). The build script of that version is loaded as a module
(no stage runs) for find_design(), place() (occurrence transforms only) and appearance(). Temporary, all restored at the end:
occurrence positions, visibility, the shells' opacity (xray) and, for v15, the LCD active-area marker's appearance
(a green chroma key while rendering, so the screenshot can be pasted in; compose_v15.py does the same).

Raw frames: hardware/animations/_frames/<ver>/<anim>/ (git-ignored) + frames.json (caption, duration, the screen's
four corners in image pixels). Offline side: Pillow + numpy; no ffmpeg needed (an MP4 is written only if ffmpeg is on PATH).
"""
import json, math, os, sys, time, shutil, subprocess, importlib.util

REPO = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR"
ENC = os.path.join(REPO, "hardware", "enclosure")
ANIM = os.path.join(REPO, "hardware", "animations")
RAW = os.path.join(ANIM, "_frames")
SRC = {"v14": os.path.join(ENC, "build_final_assembly.py"),
       "v15": os.path.join(ENC, "final_assembly_v15_lcd", "build_final_assembly_v15.py")}
LOG = os.path.join(ENC, "animate_assembly.log")
UI_PNG = os.path.join(REPO, "hardware", "renders_ui_v15", "05_calc_result_functions.png")
FONT_CPP = os.path.join(REPO, "core", "font_data.cpp")     # the firmware's 5x7 font (v14 e-paper image)
CHROMA = "V15 chroma key"                                  # same custom look as build_final_assembly_v15.py
RW, RH = 1280, 960            # Fusion render size (downscaled to OUT_W x OUT_H with Lanczos: smoother edges)
OUT_W, OUT_H = 960, 720
ANIMS = ["assembly", "turntable", "xray"]

def log(msg):
    with open(LOG, "a", encoding="utf8") as f:
        print(time.strftime("%H:%M:%S"), msg, file=f)

PCB = "PCB (ai_calc_board.step)"
SCREEN = {"v14": "E-paper panel", "v15": "LCD panel"}
SCREEN_FPC = {"v14": "E-paper FPC", "v15": "LCD FPC"}
FACEPLATE = ["Front shell", "Window lens", "Window mask", "Solar cell (dummy)"]
COVER = ["Back cover", "Battery lid", "Screws", "Rubber feet"]
ease = lambda t: t * t * (3 - 2 * t)

def norm(v):
    n = math.sqrt(sum(c * c for c in v))
    return tuple(c / n for c in v)

def lerp(a, b, t):
    return tuple(a[k] + (b[k] - a[k]) * t for k in range(len(a)))

# ═════════════════════════════════════════════════════════════════════════════
# Frame lists. Each frame: show (components), ribbons (bodies of "Ribbons and wires" shown), move {comp: (dx,dy,dz)},
# cam (target, dir, up, ext), caption, ms (GIF duration), screen ("off" | "on"), opacity {comp: a}
# ═════════════════════════════════════════════════════════════════════════════
# Assembly camera: faceplate face down on the table (front = +Z points down), seen from above-right; parts drop from
# above (-Z) the way they go in.
A_CAM = dict(target=(0, 0, -16), d=(1.0, -0.55, -0.8), up=(0, 0, -1), ext=150)
HERO = dict(target=(0, 2, 6), d=(0.38, -0.42, 1.0), up=(0, 1, 0), ext=188)
# Board on its own (steps 05-07 of the guides). Key side up first: the A_CAM view turned 180 deg about the model's X axis,
# so the board seems to lie key side up on the table; then it is "turned over" (the camera rotates back) for J1 and J3.
B_T, B_EXT = (0, 3, 2), 165

def rot_x(v, a):
    c, s = math.cos(a), math.sin(a)
    return (v[0], v[1] * c - v[2] * s, v[1] * s + v[2] * c)

B_UP = dict(target=B_T, d=rot_x(A_CAM["d"], math.pi), up=(0, 0, 1), ext=B_EXT)     # key side up
B_DN = dict(target=B_T, d=A_CAM["d"], up=A_CAM["up"], ext=B_EXT)                  # back side up

# Captions use the step numbers of the build guides (final_assembly/assembly_guide.html, ..._v15_lcd.html)
STEP_TXT = {"v14": {5: "Step 5: e-paper screen onto the board (key side)",
                    11: "Step 11: final test: the e-paper shows the sum"},
            "v15": {5: "Step 5: LCD onto the board, tail through the slot into J5",
                    11: "Step 11: final test: screen on"}}

def assembly_frames(ver):
    scr, fpc = SCREEN[ver], SCREEN_FPC[ver]
    leads = ["LiPo lead red", "LiPo lead black"]
    BOARD = [PCB, scr, "Camera module", "Magnet connector"]
    N = 4
    fr = []
    state = dict(show=[], rib=[])

    def stage(cap, comps, rbodies, off, rib_moves, cam):
        """Parts in comps fly in from offset off and seat (N+1 frames)."""
        for k in range(N + 1):                 # k = 0: appears at the full offset; k = N: seated
            t = 1 - ease(k / N)
            mv = {}
            for c in comps:
                o = off
                if c == "Screws":
                    o = (off[0], off[1], off[2] * 1.35)
                mv[c] = tuple(x * t for x in o)
            if rib_moves:
                mv["Ribbons and wires"] = tuple(x * t for x in off)
            landed = k == N
            rb = state["rib"] + (rbodies if (landed or rib_moves) else [])
            fr.append(dict(show=state["show"] + comps + (["Ribbons and wires"] if rb else []),
                           ribbons=list(rb), move=mv, cam=cam, caption=cap,
                           ms=900 if landed else (260 if k == 0 else 110), screen="off"))
        state["show"] = state["show"] + comps
        state["rib"] = state["rib"] + rbodies

    # Steps 05-07: parts onto the bare board
    state["show"] = [PCB]
    fr.append(dict(show=[PCB], ribbons=[], move={}, cam=B_UP, caption="Start: the bare board, key side up", ms=1200,
                   screen="off"))
    stage(STEP_TXT[ver][5], [scr], [fpc], (0, 0, 45), True, B_UP)
    M_ = 5                                     # turn the board over (the camera turns, the model stays put)
    for k in range(1, M_ + 1):
        a = math.pi * ease(k / M_)
        cam = dict(target=B_T, d=rot_x(B_UP["d"], a), up=rot_x(B_UP["up"], a), ext=B_EXT)
        fr.append(dict(show=state["show"] + ["Ribbons and wires"], ribbons=list(state["rib"]), move={}, cam=cam,
                       caption="Turn the board over", ms=120, screen="off"))
    stage("Step 6: camera + ribbon into J1", ["Camera module"], ["Camera FPC"], (0, 0, -50), False, B_DN)
    stage("Step 7: magnet piece into J3", ["Magnet connector"], [], (0, 22, -40), False, B_DN)
    # Step 08: key mat into the faceplate, then the finished board drops onto the posts
    board_rib = list(state["rib"])
    state["show"], state["rib"] = list(FACEPLATE), []
    fr.append(dict(show=list(FACEPLATE), ribbons=[], move={}, cam=A_CAM, caption="Step 8: the front shell, face down",
                   ms=1000, screen="off"))
    stage("Step 8: key mat into the front shell", ["Keymat", "Keycaps"], [], (0, 0, -45), False, A_CAM)
    stage("Step 8: board onto the posts", BOARD, board_rib, (0, 0, -60), True, A_CAM)
    stage("Step 9: battery in, plug into J4", ["LiPo battery"], leads, (0, 0, -50), False, A_CAM)
    stage("Step 10: close up: back cover + screws", COVER, [], (0, 0, -70), False, A_CAM)
    # swing round to the front (hero view); the screen comes on near the end
    full = state["show"] + ["Ribbons and wires"]
    rib = state["rib"]
    M_ = 8
    for k in range(1, M_ + 1):
        t = ease(k / M_)
        d = norm(lerp(norm(A_CAM["d"]), norm(HERO["d"]), t))
        up = norm(lerp(A_CAM["up"], HERO["up"], t))
        cam = dict(target=lerp(A_CAM["target"], HERO["target"], t), d=d, up=up, ext=A_CAM["ext"] + (HERO["ext"] - A_CAM["ext"]) * t)
        on = k >= M_ - 1
        fr.append(dict(show=full, ribbons=rib, move={}, cam=cam, caption=STEP_TXT[ver][11] if on else "Finished",
                       ms=3000 if k == M_ else 120, screen="on" if on else "off"))
    return fr

def turntable_frames(ver):
    full = FACEPLATE + ["Keymat", "Keycaps", SCREEN[ver], PCB, "Camera module", "Magnet connector", "LiPo battery",
                        "Ribbons and wires"] + COVER
    rib = [SCREEN_FPC[ver], "Camera FPC", "LiPo lead red", "LiPo lead black"]
    fr = []
    for k in range(36):
        a = math.radians(20 + k * 10)          # starts a little right of straight-on front
        d = (math.sin(a), 0.28, math.cos(a))
        fr.append(dict(show=full, ribbons=rib, move={}, cam=dict(target=(0, 0, 6), d=d, up=(0, 1, 0), ext=192),
                       caption=None, ms=110, screen="on"))
    return fr

def xray_frames(ver, M):
    full = FACEPLATE + ["Keymat", "Keycaps", SCREEN[ver], PCB, "Camera module", "Magnet connector", "LiPo battery",
                        "Ribbons and wires"] + COVER
    rib = [SCREEN_FPC[ver], "Camera FPC", "LiPo lead red", "LiPo lead black"]
    ghost = {"Front shell": 0.22, "Back cover": 0.22, "Battery lid": 0.3, "Keymat": 0.45, "Solar cell (dummy)": 0.5}
    cam = dict(target=(0, 0, 4), d=(1.0, -0.6, 0.75), up=(0, 0, 1), ext=205)
    fr = []
    n = 12
    for k in range(n):                         # 0 = assembled ... 11 = exploded; then back (24 frames)
        t = ease(k / (n - 1))
        mv = {c: (0, 0, M.EXPLODE.get(c, 0) * t) for c in full}
        fr.append(dict(show=full, ribbons=rib, move=mv, cam=cam, caption=None, ms=1100 if k in (0, n - 1) else 90,
                       screen="on", opacity=ghost))
    return fr + [dict(f) for f in reversed(fr)]

# ═════════════════════════════════════════════════════════════════════════════
# Fusion side
# ═════════════════════════════════════════════════════════════════════════════
def load_build(ver):
    spec = importlib.util.spec_from_file_location("build_" + ver, SRC[ver])
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)                 # no ARGS in its globals: definitions only, no stage runs
    m.app = app
    m.ARGS = {}
    m.design = m.find_design()
    m.wire_replica()
    return m

def strip(nm):
    return nm.replace(M.PREFIX, "").replace("Replica - ", "")

def screen_quad():
    """The screen's active-area corners (model mm, front-view order TL, TR, BR, BL) at its assembled position."""
    b = [b for b in M.comp_named(SCREEN[VER]).bRepBodies if "active area" in b.name][0]
    bb = b.boundingBox
    x0, x1, y0, y1, z = bb.minPoint.x * 10, bb.maxPoint.x * 10, bb.minPoint.y * 10, bb.maxPoint.y * 10, bb.maxPoint.z * 10
    return b, [(x0, y1, z), (x1, y1, z), (x1, y0, z), (x0, y0, z)]

def render(ver, anim, frames):
    import adsk.core
    outdir = os.path.join(RAW, ver, anim)
    if os.path.isdir(outdir):                   # empty it (OneDrive can lock the folder itself, so keep the folder)
        shutil.rmtree(outdir, ignore_errors=True)
    os.makedirs(outdir, exist_ok=True)
    vp = app.activeViewport
    root = M.design.rootComponent
    grid_was = M.set_grid(False)
    cam0 = vp.camera
    ab, quad = screen_quad()
    look0 = ab.appearance
    op0 = {}
    for occ in root.occurrences:
        for b in occ.component.bRepBodies:
            op0[b.entityToken] = (b, b.opacity)
    # green key on the active area, replaced offline: v15 by the firmware screenshot (on) or dark glass (off),
    # v14 by the e-paper image (on) or a blank e-paper (off)
    M.R.CUSTOM_LOOKS.setdefault(CHROMA, ("Plastic - Matte (Green)", (0, 255, 0), ("opaque_albedo",)))
    M.appearance(ab, CHROMA)
    meta = []
    try:
        cam = vp.camera
        if cam.cameraType != adsk.core.CameraTypes.OrthographicCameraType:
            cam.cameraType = adsk.core.CameraTypes.OrthographicCameraType
            cam.isSmoothTransition = False
            vp.camera = cam
            adsk.doEvents()
        for i, f in enumerate(frames):
            M.place({})
            for occ in root.occurrences:
                occ.isLightBulbOn = strip(occ.component.name) in f["show"]
                for b in occ.component.bRepBodies:
                    if "cutter" in b.name:
                        b.isLightBulbOn = False
                    elif strip(occ.component.name) == "Ribbons and wires":
                        b.isLightBulbOn = b.name in f["ribbons"]
                    else:
                        b.isLightBulbOn = True
                    a = (f.get("opacity") or {}).get(strip(occ.component.name), 1.0)
                    if abs(b.opacity - a) > 1e-3:
                        b.opacity = a
            M.hide_cutters()
            M.place(f["move"])
            c_ = f["cam"]
            t = c_["target"]; d = norm(c_["d"])
            cam = vp.camera
            cam.isFitView = False
            cam.target = M.P(*t); cam.eye = M.P(*(t[k] + d[k] * 300 for k in range(3)))
            cam.upVector = adsk.core.Vector3D.create(*c_["up"])
            cam.viewExtents = M.cm(c_["ext"] * min(RW, RH) / RH)
            cam.isSmoothTransition = False
            vp.camera = cam
            adsk.doEvents(); vp.refresh(); adsk.doEvents()
            fn = "f%03d.png" % i
            vp.saveAsImageFile(os.path.join(outdir, fn), RW, RH)
            c = vp.modelToViewSpace(M.P(*t))
            sc = min(RW, RH) / min(2 * c.x, 2 * c.y)
            dz = f["move"].get(SCREEN[ver], (0, 0, 0))
            q = []
            for p in quad:
                v = vp.modelToViewSpace(M.P(p[0] + dz[0], p[1] + dz[1], p[2] + dz[2]))
                q.append([round(RW / 2 + (v.x - c.x) * sc, 1), round(RH / 2 + (v.y - c.y) * sc, 1)])
            meta.append(dict(file=fn, caption=f["caption"], ms=f["ms"], screen=f["screen"],
                             quad=q if SCREEN[ver] in f["show"] else None))
        json.dump(dict(ver=ver, anim=anim, size=[RW, RH], frames=meta), open(os.path.join(outdir, "frames.json"), "w"), indent=1)
    finally:
        M.place({})
        for occ in root.occurrences:
            occ.isLightBulbOn = not strip(occ.component.name).startswith("Slide case")
            for b in occ.component.bRepBodies:
                b.isLightBulbOn = "cutter" not in b.name
        for b, a in op0.values():
            if abs(b.opacity - a) > 1e-3:
                b.opacity = a
        if look0:
            ab.appearance = look0
        M.hide_cutters()
        cam0.isSmoothTransition = False
        vp.camera = cam0
        if grid_was is not None:
            M.set_grid(grid_was)
    return len(meta)

# ═════════════════════════════════════════════════════════════════════════════
# Offline side: Pillow composition -> GIF (+ MP4 if ffmpeg exists)
# ═════════════════════════════════════════════════════════════════════════════
LIMIT_MB = {"assembly": 6.0, "turntable": 4.0, "xray": 6.0}

def epd_image():
    """v14 e-paper content: a 250 x 122, 1-bit black-on-white calculator screen drawn with the firmware's own 5x7 font
    (core/font_data.cpp, cell 5 x 7 incl. the 1 px gap). Status line, the expression, the result right-aligned."""
    import re
    from PIL import Image, ImageDraw
    G = {}
    for cp, rows in re.findall(r"\{0x([0-9A-Fa-f]+),\s*\{([^}]*)\}\}", open(FONT_CPP, encoding="utf8").read()):
        G[int(cp, 16)] = [int(r, 16) for r in rows.split(",")]
    im = Image.new("1", (250, 122), 1)
    d = ImageDraw.Draw(im)

    def text(x, y, s, k=1):
        for ch in s:
            rows = G.get(ord(ch), G[ord("?")])
            for r, bits in enumerate(rows):
                for c in range(5):
                    if bits & (0x10 >> c):
                        d.rectangle([x + c * k, y + r * k, x + c * k + k - 1, y + r * k + k - 1], fill=0)
            x += 5 * k
        return x

    text(4, 3, "DEG  NORM")
    d.rectangle([206, 2, 227, 10], outline=0); d.rectangle([228, 4, 229, 8], fill=0)   # battery, 3 of 4 bars
    for i in range(3):
        d.rectangle([208 + i * 5, 4, 211 + i * 5, 8], fill=0)
    d.rectangle([186, 1, 200, 11], fill=0)
    x = 188
    for ch in "AI":                                                         # inverted "AI" badge
        for r, bits in enumerate(G[ord(ch)]):
            for c in range(5):
                if bits & (0x10 >> c):
                    d.point((x + c, 3 + r), fill=1)
        x += 6
    d.line([0, 14, 249, 14], fill=0)
    text(4, 24, "√(144)+2^3×1.5", 2)
    text(4, 84, "=", 3)
    res = "24"
    text(246 - len(res) * 5 * 6 + 6, 70, res, 6)
    d.line([0, 116, 249, 116], fill=0)
    return im


def compose(ver, anim):
    import numpy as np
    from PIL import Image, ImageDraw, ImageFont, ImageFilter
    sys.path.insert(0, os.path.join(ENC, "final_assembly_v15_lcd"))
    from compose_v15 import persp_coeffs
    src = os.path.join(RAW, ver, anim)
    J = json.load(open(os.path.join(src, "frames.json")))
    sx = OUT_W / J["size"][0]
    if ver == "v15":
        ui = Image.open(UI_PNG).convert("RGB")
        UW, UH = ui.size
        big = ui.resize((UW * 4, UH * 4), Image.LANCZOS)
        off_rgb, dim = (34, 36, 40), 0.12          # screen off: dark glass
    else:
        ep = epd_image()
        ep.save(os.path.join(src, "epd_screen.png"))
        UW, UH = ep.size
        ink, paper = (30, 30, 34), (222, 222, 212)   # e-paper: dark grey ink on a warm light-grey film
        big = Image.fromarray(np.where(np.asarray(ep.resize((UW * 4, UH * 4), Image.NEAREST))[..., None],
                                       np.array(paper, np.uint8), np.array(ink, np.uint8)))
        off_rgb, dim = paper, 0.06                   # e-paper off: blank film
    font = ImageFont.truetype(r"C:\Windows\Fonts\segoeuib.ttf", 26)
    small = ImageFont.truetype(r"C:\Windows\Fonts\segoeui.ttf", 17)
    title = {"v14": "AI Calculator v14 (e-paper)", "v15": "AI Calculator v15 (colour LCD)"}[ver]
    frames, durs = [], []
    for f in J["frames"]:
        im = Image.open(os.path.join(src, f["file"])).convert("RGB")
        if f["quad"]:
            a = np.asarray(im).astype(int)
            # inside the screen's own outline a weaker test is safe (the lens greys the key at grazing angles)
            g = (a[..., 1] > a[..., 0] + 12) & (a[..., 1] > a[..., 2] + 12)
            poly = Image.new("L", im.size, 0)
            ImageDraw.Draw(poly).polygon([tuple(p) for p in f["quad"]], fill=255)
            poly = poly.filter(ImageFilter.MaxFilter(9))
            m = Image.fromarray((g * 255).astype(np.uint8)).filter(ImageFilter.MaxFilter(3))
            m = Image.fromarray(np.minimum(np.asarray(m), np.asarray(poly)))
            if np.asarray(m).any():
                if f["screen"] == "on":
                    co = persp_coeffs(f["quad"], [(0, 0), (UW * 4, 0), (UW * 4, UH * 4), (0, UH * 4)])
                    layer = big.transform(im.size, Image.PERSPECTIVE, co, Image.BICUBIC)
                    layer = Image.blend(layer, Image.new("RGB", im.size, (20, 20, 24)), dim)
                else:
                    layer = Image.new("RGB", im.size, off_rgb)
                im.paste(layer, (0, 0), m)
        im = im.resize((OUT_W, OUT_H), Image.LANCZOS)
        d = ImageDraw.Draw(im)
        d.text((16, 12), title, font=small, fill=(110, 106, 98))
        if f["caption"]:
            d.rectangle([0, OUT_H - 50, OUT_W, OUT_H], fill=(29, 28, 26))
            d.text((20, OUT_H - 25), f["caption"], font=font, fill=(255, 255, 255), anchor="lm")
        frames.append(im)
        durs.append(f["ms"])
    out = os.path.join(ANIM, ver)
    os.makedirs(out, exist_ok=True)
    gif = os.path.join(out, "%s_%s.gif" % (anim, ver))
    # one shared, optimised palette from a sample of the frames (stable colours, small deltas between frames)
    sample = Image.new("RGB", (OUT_W, OUT_H * 4))
    for k, i in enumerate(np.linspace(0, len(frames) - 1, 4).astype(int)):
        sample.paste(frames[i], (0, OUT_H * k))
    for colors in (255, 160, 96, 64):
        pal = sample.quantize(colors=colors, method=Image.Quantize.MEDIANCUT)
        q = [fr.quantize(palette=pal, dither=Image.Dither.NONE) for fr in frames]
        q[0].save(gif, save_all=True, append_images=q[1:], duration=durs, loop=0, optimize=True, disposal=1)
        mb = os.path.getsize(gif) / 1e6
        if mb <= LIMIT_MB[anim]:
            break
    frames[-1].save(os.path.join(out, "%s_%s_poster.jpg" % (anim, ver)), quality=88)
    res = ["%s %.2f MB (%d frames, %d colours)" % (os.path.basename(gif), mb, len(frames), colors)]
    ff = shutil.which("ffmpeg")
    if ff:
        tmp = os.path.join(src, "_mp4")
        os.makedirs(tmp, exist_ok=True)
        lst = []
        for i, (fr, ms) in enumerate(zip(frames, durs)):
            fr.save(os.path.join(tmp, "m%03d.png" % i))
            lst.append("file 'm%03d.png'\nduration %.3f" % (i, ms / 1000))
        lst.append("file 'm%03d.png'" % (len(frames) - 1))
        open(os.path.join(tmp, "list.txt"), "w").write("\n".join(lst))
        mp4 = os.path.join(out, "%s_%s.mp4" % (anim, ver))
        subprocess.run([ff, "-y", "-f", "concat", "-safe", "0", "-i", "list.txt", "-vsync", "vfr", "-pix_fmt", "yuv420p",
                        "-c:v", "libx264", "-crf", "22", mp4], cwd=tmp, check=True, capture_output=True)
        res.append("%s %.2f MB" % (os.path.basename(mp4), os.path.getsize(mp4) / 1e6))
    else:
        res.append("(no ffmpeg on PATH: MP4 skipped)")
    return res

# ═════════════════════════════════════════════════════════════════════════════
def send(args):
    import urllib.request, urllib.error
    script = "ARGS = %r\n" % args + open(os.path.abspath(__file__), encoding="utf8").read()
    secret = open(os.path.join(os.path.expanduser("~"), ".fusion-mcp-secret")).read().strip()
    req = urllib.request.Request("http://127.0.0.1:7654/execute", data=json.dumps({"script": script}).encode(),
                                 headers={"Authorization": "Bearer " + secret, "Content-Type": "application/json"})
    start = os.path.getsize(LOG) if os.path.exists(LOG) else 0
    tag = "run %s %s" % (args["ver"], args["anim"])
    try:
        r = json.load(urllib.request.urlopen(req, timeout=60))
    except urllib.error.HTTPError as e:
        r = json.load(e)
    except Exception as e:
        r = {"error": "timeout: %s" % e}
    if "timeout" in str(r.get("error", "")).lower():
        tail = lambda: open(LOG, "rb").read()[start:].decode("utf8", "replace") if os.path.exists(LOG) else ""
        t0 = time.time()
        while tag + " done" not in tail() and "FAILED" not in tail() and time.time() - t0 < 3600:
            time.sleep(4)
        print(tail()[-3000:])
        if "FAILED" in tail():
            sys.exit(1)
        return
    print(r.get("result") or r)
    if r.get("traceback") or r.get("error"):
        sys.exit(1)

if "ARGS" in globals():
    import adsk.core, adsk.fusion, traceback
    VER = ARGS["ver"]
    tag = "run %s %s" % (VER, ARGS["anim"])
    log(tag)
    try:
        M = load_build(VER)
        for a in (ANIMS if ARGS["anim"] == "all" else ARGS["anim"].split(",")):
            fr = {"assembly": lambda: assembly_frames(VER), "turntable": lambda: turntable_frames(VER),
                  "xray": lambda: xray_frames(VER, M)}[a]()
            if ARGS.get("pick"):                 # quick look: pick=0,12,30 renders only those frames
                fr = [fr[int(i)] for i in ARGS["pick"].split(",") if int(i) < len(fr)]
            n = render(VER, a, fr)
            log("%s %s: %d frames" % (VER, a, n))
            print(VER, a, n, "frames")
    except Exception:
        log(tag + " FAILED " + traceback.format_exc())
        raise
    log(tag + " done")
elif __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    kw = dict(a.split("=", 1) for a in sys.argv[1:] if "=" in a)
    ver, anim = kw.get("ver", "v15"), kw.get("anim", "all")
    if "compose" not in sys.argv[1:]:
        send(dict(kw, ver=ver, anim=anim))
    for a in (ANIMS if anim == "all" else anim.split(",")):
        for line in compose(ver, a):
            print(line)
