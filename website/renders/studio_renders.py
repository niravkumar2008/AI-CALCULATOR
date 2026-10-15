"""Studio product shots of the finished calculator, for the website.

Run with the final assembly open in Fusion (hardware/enclosure/final_assembly/
ai_calculator_final_assembly.f3d) and the FusionMCPBridge add-in running:

    python hardware/enclosure/fusion_run.py <this file> [mode=render|viewport] [only=hero_front,exploded]

mode=render (default) uses Fusion's ray-traced local renderer, one shot after another.
It takes minutes per shot, so the bridge times out but Fusion keeps going;
progress is written to studio_renders.log next to the images.
mode=viewport saves anti-aliased viewport images with a transparent background instead
(seconds per shot, no reflections or soft shadows). The script never saves the design.
"""
import os, time, traceback
import adsk.core, adsk.fusion

ARGS = globals().get("ARGS", {})
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals()
                   else r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\.claude\worktrees\install-ui-ux-pro-max-skill-f85942\website\renders", "out")
os.makedirs(OUT, exist_ok=True)
LOG = os.path.join(OUT, "studio_renders.log")

app = adsk.core.Application.get()
design = adsk.fusion.Design.cast(app.activeProduct)
P = lambda x, y, z: adsk.core.Point3D.create(x / 10, y / 10, z / 10)      # mm -> cm

# Not part of the product shot.
HIDE = ["Slide case", "Magnetic cable plug", "cutter", "Solar cell (dummy)"]
# Same offsets (mm, along Z) as EXPLODE in build_final_assembly.py.
EXPLODE = {"Rubber feet": -42, "Screws": -58, "Battery lid": -38, "Back cover": -30, "E-paper panel": 12,
           "Keymat": 22, "Keycaps": 34, "Front shell": 50, "Window mask": 57, "Window lens": 64}

# name: (eye, target, up, view extent mm, exploded?)  -- model units are mm, calculator centred near the origin.
SHOTS = {
    "hero_front":    ((95, -150, 140), (0, 0, 6), (0, 0, 1), 185, False),
    "hero_back":     ((-110, -130, -120), (0, 0, 6), (0, 0, 1), 185, False),
    "front_flat":    ((0, 0, 400), (0, 0, 6), (0, 1, 0), 175, False),
    "low_angle":     ((160, -260, 60), (0, 10, 4), (0, 0, 1), 175, False),
    "exploded":      ((150, -190, 150), (0, 0, 8), (0, 0, 1), 235, True),
}


def log(msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write(time.strftime("%H:%M:%S ") + msg + "\n")
    print(msg)


def name_of(occ):
    return occ.component.name


def hidden(nm):
    return any(h.lower() in nm.lower() for h in HIDE)


def set_scene(exploded):
    root = design.rootComponent
    for occ in root.occurrences:
        nm = name_of(occ)
        occ.isLightBulbOn = not hidden(nm)
        for b in occ.component.bRepBodies:
            if "cutter" in b.name:
                b.isLightBulbOn = False
    for comp in design.allComponents:
        for skt in comp.sketches:
            skt.isLightBulbOn = False
    if exploded:
        for occ in root.occurrences:
            dz = next((v for k, v in EXPLODE.items() if k in name_of(occ)), 0)
            if dz:
                m = occ.transform2
                t = m.translation
                t.z += dz / 10
                m.translation = t
                occ.isGrounded = False
                occ.transform2 = m


def restore(moved):
    # Put exploded parts back without touching the timeline.
    for occ, m in moved:
        occ.transform2 = m


def camera(eye, target, up, ext):
    vp = app.activeViewport
    cam = vp.camera
    cam.isFitView = False
    cam.cameraType = adsk.core.CameraTypes.PerspectiveCameraType
    cam.target = P(*target)
    cam.eye = P(*eye)
    cam.upVector = adsk.core.Vector3D.create(*up)
    cam.viewExtents = ext / 10
    cam.isSmoothTransition = False
    vp.camera = cam
    adsk.doEvents(); vp.refresh(); adsk.doEvents()
    return vp.camera


def viewport_shot(fname):
    vp = app.activeViewport
    opts = adsk.core.SaveImageFileOptions.create(os.path.join(OUT, fname + ".png"))
    opts.width, opts.height = 3200, 2400
    opts.isAntiAliased = True
    opts.isBackgroundTransparent = True
    vp.saveAsImageFileWithOptions(opts)


def ray_traced_shot(fname, cam):
    rm = design.renderManager
    r = rm.rendering
    r.resolution = adsk.fusion.RenderResolutions.CustomRenderResolution
    r.resolutionWidth, r.resolutionHeight = 2400, 1800
    r.isBackgroundTransparent = True
    try:
        r.renderQuality = int(ARGS.get("quality", 75))
    except Exception:
        pass
    fut = r.startLocalRender(os.path.join(OUT, fname + ".png"), cam)
    t0 = time.time()
    while fut.renderState != adsk.fusion.LocalRenderStates.FinishedLocalRenderState:
        if fut.renderState == adsk.fusion.LocalRenderStates.FailedLocalRenderState:
            raise RuntimeError("Fusion reported the render failed")
        if time.time() - t0 > 20 * 60:
            raise RuntimeError("render took over 20 minutes")
        adsk.doEvents()
        time.sleep(1)
    log("  %s done in %d s" % (fname, time.time() - t0))


def run_all():
    mode = ARGS.get("mode", "render")
    only = [s for s in ARGS.get("only", "").split(",") if s]
    log("start, mode=%s, design=%s" % (mode, app.activeDocument.name))
    originals = [(o, o.transform2) for o in design.rootComponent.occurrences]
    try:
        for fname, (eye, tgt, up, ext, exploded) in SHOTS.items():
            if only and fname not in only:
                continue
            restore(originals)
            set_scene(exploded)
            cam = camera(eye, tgt, up, ext)
            log("shot %s" % fname)
            if mode == "viewport":
                viewport_shot(fname)
            else:
                ray_traced_shot(fname, cam)
    except Exception:
        log("FAILED\n" + traceback.format_exc())
    finally:
        restore(originals)
        log("finished")


run_all()
