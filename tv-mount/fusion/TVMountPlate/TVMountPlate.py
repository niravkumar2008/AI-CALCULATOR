"""Fusion 360 script: VESA adapter plate for a TV (100x100, M6) on a Suptek MD6TB plate (75x75, M4).

Stack, front to back: TV | this plate | Suptek steel plate | arm head.
- M6 x 16 bolts pass through this plate into the TV; their heads sit in pockets on the Suptek-side face.
- M4 x 15 screws pass from the arm side through the Suptek plate's 75x75 holes into this plate.

Run it from Utilities > Add-Ins > Scripts and Add-Ins. It builds the plate in a new design,
then checks the finished body against the expected size and volume and reports the result.
Print it TV-face down (the face on the XY plane); pockets and holes then open upward.
"""
import math
import traceback

import adsk.core
import adsk.fusion

# All sizes in mm. Change a value and run the script again to make a variant.
P = {
    "plate_size": 125.0,     # square outline
    "plate_thick": 14.0,
    "corner_r": 6.0,
    "tv_pitch": 100.0,       # TV VESA pattern
    "m6_clear": 6.6,         # M6 clearance hole
    "pocket_d": 13.0,        # fits M6 hex/socket/button heads and a 12 mm washer
    "pocket_depth": 8.0,     # leaves a 6 mm floor, so an M6 x 16 enters the TV 10 mm
    "sup_pitch": 75.0,       # Suptek plate holes used for the M4 screws
    "m4_pilot": 3.6,         # through hole; an M4 x 15 cuts its own thread and stops 1 mm short of the TV
}


def cm(mm):
    return mm / 10.0  # the Fusion API works in centimetres


def corners(pitch):
    h = pitch / 2
    return [(sx * h, sy * h) for sx in (-1, 1) for sy in (-1, 1)]


def circle_sketch(root, plane, name, z_mm, circles):
    sk = root.sketches.add(plane)
    sk.name = name
    for (x, y), d in circles:
        centre = sk.modelToSketchSpace(adsk.core.Point3D.create(cm(x), cm(y), cm(z_mm)))
        sk.sketchCurves.sketchCircles.addByCenterRadius(centre, cm(d) / 2)
    profiles = adsk.core.ObjectCollection.create()
    for p in sk.profiles:
        profiles.add(p)
    return profiles


def expected_volume_mm3():
    s, t, r = P["plate_size"], P["plate_thick"], P["corner_r"]
    area = s * s - (4 - math.pi) * r * r
    floor = t - P["pocket_depth"]
    hole = lambda d, h: math.pi * (d / 2) ** 2 * h
    return (area * t
            - 4 * hole(P["m6_clear"], floor)
            - 4 * hole(P["pocket_d"], P["pocket_depth"])
            - 4 * hole(P["m4_pilot"], t))


def run(context):
    ui = None
    try:
        app = adsk.core.Application.get()
        ui = app.userInterface
        app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
        design = adsk.fusion.Design.cast(app.activeProduct)
        design.designType = adsk.fusion.DesignTypes.ParametricDesignType
        root = design.rootComponent
        extrudes = root.features.extrudeFeatures
        t = P["plate_thick"]

        # 1. Square plate, TV face on the XY plane.
        sk = root.sketches.add(root.xYConstructionPlane)
        sk.name = "Outline"
        half = cm(P["plate_size"] / 2)
        sk.sketchCurves.sketchLines.addCenterPointRectangle(
            adsk.core.Point3D.create(0, 0, 0), adsk.core.Point3D.create(half, half, 0))
        ext = extrudes.addSimple(sk.profiles.item(0), adsk.core.ValueInput.createByReal(cm(t)),
                                 adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
        body = ext.bodies.item(0)
        body.name = "VESA adapter plate"

        # 2. Round the four vertical corner edges.
        edges = adsk.core.ObjectCollection.create()
        for e in body.edges:
            a, b = e.startVertex.geometry, e.endVertex.geometry
            if abs(a.x - b.x) < 1e-6 and abs(a.y - b.y) < 1e-6:
                edges.add(e)
        fillets = root.features.filletFeatures
        fi = fillets.createInput()
        radius = adsk.core.ValueInput.createByReal(cm(P["corner_r"]))
        try:
            fi.edgeSetInputs.addConstantRadiusEdgeSet(edges, radius, True)
        except AttributeError:
            fi.addConstantRadiusEdgeSet(edges, radius, True)
        fillets.add(fi)

        # 3. Through holes: M6 clearance at 100x100, M4 pilot at 75x75.
        holes = [(c, P["m6_clear"]) for c in corners(P["tv_pitch"])]
        holes += [(c, P["m4_pilot"]) for c in corners(P["sup_pitch"])]
        profiles = circle_sketch(root, root.xYConstructionPlane, "Through holes", 0.0, holes)
        cut = extrudes.createInput(profiles, adsk.fusion.FeatureOperations.CutFeatureOperation)
        cut.setDistanceExtent(False, adsk.core.ValueInput.createByReal(cm(t)))
        extrudes.add(cut)

        # 4. Bolt-head pockets cut down from the Suptek-side face.
        plane_in = root.constructionPlanes.createInput()
        plane_in.setByOffset(root.xYConstructionPlane, adsk.core.ValueInput.createByReal(cm(t)))
        top = root.constructionPlanes.add(plane_in)
        top.name = "Suptek face"
        pockets = [(c, P["pocket_d"]) for c in corners(P["tv_pitch"])]
        profiles = circle_sketch(root, top, "Bolt pockets", t, pockets)
        cut = extrudes.createInput(profiles, adsk.fusion.FeatureOperations.CutFeatureOperation)
        cut.setOneSideExtent(
            adsk.fusion.DistanceExtentDefinition.create(adsk.core.ValueInput.createByReal(cm(P["pocket_depth"]))),
            adsk.fusion.ExtentDirections.NegativeExtentDirection)
        extrudes.add(cut)

        # 5. Check the finished body.
        body = root.bRepBodies.item(0)
        bb = body.boundingBox
        size = [(bb.maxPoint.x - bb.minPoint.x) * 10, (bb.maxPoint.y - bb.minPoint.y) * 10,
                (bb.maxPoint.z - bb.minPoint.z) * 10]
        vol = body.volume * 1000  # cm3 -> mm3
        want = expected_volume_mm3()
        size_ok = all(abs(a - b) < 0.01 for a, b in zip(size, (P["plate_size"], P["plate_size"], t)))
        vol_ok = abs(vol - want) / want < 0.002
        ui.messageBox(
            "VESA adapter plate built.\n\n"
            f"Size: {size[0]:.2f} x {size[1]:.2f} x {size[2]:.2f} mm  ({'OK' if size_ok else 'MISMATCH'})\n"
            f"Volume: {vol / 1000:.2f} cm3, expected {want / 1000:.2f} cm3  ({'OK' if vol_ok else 'MISMATCH'})\n"
            f"Faces: {body.faces.count}\n\n"
            "Measure it yourself with Inspect > Measure.\n"
            "Export for printing: right-click the body > Save As Mesh > 3MF.",
            "TV Mount Plate")
    except Exception:
        if ui:
            ui.messageBox("Failed:\n{}".format(traceback.format_exc()), "TV Mount Plate")
