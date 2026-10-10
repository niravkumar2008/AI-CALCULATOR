#!/usr/bin/env python3
"""Stage 5: load every schematic part onto the board (with nets) and place it.

Run:  python3 build_board.py <project_dir>
Reads <project_dir>/ai_calc.kicad_pcb (outline + holes from stage 1) and the netlist
exported from the schematic, adds all footprints with nets and the schematic link
(so KiCad's "Update PCB from schematic" keeps working), places them, saves in place.
Idempotent: footprints with a schematic reference are removed and re-added each run.
"""
import sys, os, csv, xml.etree.ElementTree as ET, subprocess
import pcbnew

P = sys.argv[1] if len(sys.argv) > 1 else '/home/claude/pcb/proj'
PCB = os.path.join(P, 'ai_calc.kicad_pcb')
NET = '/tmp/claude-0/net.xml'
subprocess.run(['kicad-cli', 'sch', 'export', 'netlist', '--format', 'kicadxml', '-o', NET,
                os.path.join(P, 'ai_calc.kicad_sch')], check=True, capture_output=True)
BASE = '/home/claude/pcb/work/ai_calc.stage1.kicad_pcb'
GEOM = '/home/claude/pcb/out/hardware/geometry/keys.csv'
SYSFP = '/usr/share/kicad/footprints'

MM = pcbnew.VECTOR2I_MM

# ---------------------------------------------------------------- placement table
# (x, y, rotation_deg, side)  KiCad mm, top view = seen from the back cover.
# side 'F' = component side (faces the back cover); 'B' = key side.
PLACE = {
    # ESP32 module: antenna to the left board edge, between the two left back-cover pins
    'U1': (127.55, 97.0, 90, 'F'),
    'C1': (121.3, 106.8, 90, 'F'), 'C2': (123.3, 106.15, 90, 'F'), 'C3': (125.0, 106.6, 90, 'F'),
    'R1': (127.6, 87.9, 0, 'F'), 'C4': (125.7, 87.9, 0, 'F'),
    'TP1': (121.0, 110.4, 0, 'F'), 'TP2': (138.0, 86.4, 0, 'F'), 'TP3': (140.4, 86.4, 0, 'F'),
    'TP4': (120.6, 113.1, 0, 'F'),
    # power: charger, ESD, battery path, 3.3 V, all in the strip under the magnet connector
    'J3': (136.0, 66.6, 0, 'F'),
    'U7': (133.6, 83.2, 0, 'F'),
    'U2': (132.5, 72.2, 0, 'F'), 'R2': (132.5, 74.6, 0, 'F'), 'C5': (135.9, 72.0, 90, 'F'),
    'C6': (129.1, 72.0, 90, 'F'),
    'D1': (129.2, 75.2, 0, 'F'),
    'Q1': (136.1, 79.5, 0, 'F'), 'R3': (133.25, 78.6, 90, 'F'),
    'Q2': (135.9, 75.6, 90, 'F'),
    'R4': (124.9, 74.8, 90, 'F'), 'R5': (126.1, 74.8, 90, 'F'),
    'U3': (122.0, 76.8, 0, 'F'), 'C7': (119.0, 76.8, 90, 'F'), 'C8': (125.8, 79.3, 0, 'F'),
    'R6': (136.6, 107.0, 90, 'F'), 'R7': (137.8, 107.0, 90, 'F'), 'C9': (139.0, 107.0, 90, 'F'),
    'D6': (130.3, 78.6, 0, 'F'), 'R20': (130.3, 80.6, 0, 'F'), 'D7': (127.6, 69.3, 0, 'F'),
    'J4': (142.6, 74.6, 0, 'F'),
    # camera: module sits at (150, 95.1); flat FPC runs down the centre to J1
    'J1': (150.0, 161.0, 180, 'F'),
    'U4': (137.4, 157.0, 0, 'F'), 'C10': (134.6, 157.0, 90, 'F'), 'C11': (140.0, 157.0, 90, 'F'),
    'C12': (141.2, 157.0, 90, 'F'), 'FB1': (137.4, 154.4, 0, 'F'), 'C13': (140.0, 154.4, 90, 'F'),
    'U5': (137.4, 160.6, 0, 'F'), 'C14': (134.6, 160.6, 90, 'F'), 'C15': (140.0, 160.6, 90, 'F'),
    'D2': (158.0, 154.0, 0, 'F'), 'C16': (158.0, 151.8, 0, 'F'), 'C17': (158.0, 156.2, 0, 'F'),
    'R8': (136.0, 165.2, 90, 'F'), 'C18': (137.2, 165.2, 90, 'F'), 'R9': (138.4, 165.2, 90, 'F'),
    'R10': (134.6, 163.4, 0, 'F'), 'R18': (139.6, 165.2, 90, 'F'), 'R19': (140.8, 165.2, 90, 'F'),
    # e-paper connector + booster, right of the camera
    'J2': (177.9, 96.4, -90, 'F'),
    'Q3': (163.6, 101.0, 0, 'F'), 'R11': (163.6, 98.8, 0, 'F'), 'R12': (165.4, 103.4, 0, 'F'),
    'L1': (159.8, 105.4, 180, 'F'), 'C19': (157.8, 100.6, 90, 'F'),
    'C20': (165.2, 106.4, 0, 'F'), 'D3': (164.6, 109.8, 0, 'F'), 'D4': (159.5, 109.8, 0, 'F'),
    'D5': (164.6, 112.2, 0, 'F'),
    'C21': (168.6, 89.6, 0, 'F'), 'C22': (168.6, 91.2, 0, 'F'), 'C23': (168.6, 92.8, 0, 'F'),
    'C24': (168.6, 94.4, 0, 'F'), 'C25': (168.6, 96.0, 0, 'F'), 'C26': (168.6, 97.6, 0, 'F'),
    'C27': (160.4, 112.4, 180, 'F'), 'C28': (168.6, 100.8, 0, 'F'), 'C29': (168.6, 102.4, 0, 'F'),
    'C30': (168.6, 104.0, 0, 'F'),
    # keypad scanner, F side above the function keys (keys are on the B side)
    'U6': (126.0, 146.5, 0, 'F'), 'C31': (129.4, 144.2, 90, 'F'),
    'R13': (122.4, 141.0, 90, 'F'), 'R14': (123.6, 141.0, 90, 'F'), 'R15': (124.8, 141.0, 90, 'F'),
    'R16': (126.0, 141.0, 90, 'F'), 'R17': (124.0, 117.6, 0, 'F'),
}

LABEL2KEY = {}
for r in csv.DictReader(open(GEOM)):
    LABEL2KEY[r['label']] = (float(r['x_mm']), float(r['y_mm']))
LABEL2KEY["deg-min-sec"] = LABEL2KEY.pop('o\'"')


def fp_path(lib):
    if lib == 'ai_calc':
        return os.path.join(P, 'ai_calc.pretty')
    return os.path.join(SYSFP, lib + '.pretty')


SLOT = (172.9, 96.4, 1.0, 14.0)   # e-paper FPC slot: centre x, y, width, length


def add_epaper_slot(board):
    # stadium-shaped through-slot (Edge.Cuts): the panel FPC folds behind the panel and comes up here
    cx, cy, w, l = SLOT
    r = w / 2; y0 = cy - l / 2 + r; y1 = cy + l / 2 - r
    def seg(a, b):
        s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_SEGMENT); s.SetLayer(pcbnew.Edge_Cuts)
        s.SetStart(MM(*a)); s.SetEnd(MM(*b)); s.SetWidth(pcbnew.FromMM(0.05)); board.Add(s)
    def arc(c, start, mid, end):
        s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_ARC); s.SetLayer(pcbnew.Edge_Cuts)
        s.SetArcGeometry(MM(*start), MM(*mid), MM(*end)); s.SetWidth(pcbnew.FromMM(0.05)); board.Add(s)
    seg((cx - r, y0), (cx - r, y1)); seg((cx + r, y0), (cx + r, y1))
    arc((cx, y0), (cx - r, y0), (cx, y0 - r), (cx + r, y0))
    arc((cx, y1), (cx + r, y1), (cx, y1 + r), (cx - r, y1))


def add_edge_keepout(board, width=0.35):
    # rule area along every board edge (and the slot): no tracks or vias within 0.35 mm.
    # Copper pours are still allowed (they keep their own edge clearance).
    import json
    from shapely.geometry import Polygon as SP
    F = json.load(open('/home/claude/pcb/out/hardware/geometry/features.json'))
    outer = SP(F['board_outline_mm']).buffer(0)
    cx, cy, w, l = SLOT
    slot = SP([(cx - w/2, cy - l/2), (cx + w/2, cy - l/2), (cx + w/2, cy + l/2), (cx - w/2, cy + l/2)])
    ring = outer.difference(outer.buffer(-width)).union(slot.buffer(width).intersection(outer))
    for g in getattr(ring, 'geoms', [ring]):
        z = pcbnew.ZONE(board)
        z.SetIsRuleArea(True); z.SetDoNotAllowTracks(True); z.SetDoNotAllowVias(True)
        z.SetDoNotAllowZoneFills(False); z.SetDoNotAllowPads(False); z.SetDoNotAllowFootprints(False)
        z.SetLayerSet(pcbnew.LSET.AllCuMask())
        z.SetZoneName('edge keep-out')
        ol = z.Outline(); ol.NewOutline()
        for x, y in list(g.exterior.coords)[:-1]:
            ol.Append(MM(x, y))
        for hole in g.interiors:
            ol.NewHole()
            for x, y in list(hole.coords)[:-1]:
                ol.Append(MM(x, y), 0, ol.HoleCount(0) - 1)
        board.Add(z)


def rule_area(board, name, rect, layers, tracks=True, vias=True, pours=False):
    x0, y0, x1, y1 = rect
    z = pcbnew.ZONE(board); z.SetIsRuleArea(True); z.SetZoneName(name)
    z.SetDoNotAllowTracks(tracks); z.SetDoNotAllowVias(vias); z.SetDoNotAllowZoneFills(pours)
    z.SetDoNotAllowPads(False); z.SetDoNotAllowFootprints(False)
    ls = pcbnew.LSET()
    for l in layers:
        ls.AddLayer(l)
    z.SetLayerSet(ls)
    ol = z.Outline(); ol.NewOutline()
    for x, y in ((x0, y0), (x1, y0), (x1, y1), (x0, y1)):
        ol.Append(MM(x, y))
    board.Add(z)


def add_rule_areas(board):
    both = (pcbnew.F_Cu, pcbnew.B_Cu)
    # antenna: keep copper (incl. pour) away above and below the module's own keep-out
    rule_area(board, 'antenna clearance top', (113.5, 85.0, 119.85, 89.3), both, pours=True)
    rule_area(board, 'antenna clearance bottom', (113.5, 104.7, 119.85, 109.0), both, pours=True)
    # key pads: no vias on the bare gold (a via there is an open hole in the contact surface)
    for fp in board.GetFootprints():
        if fp.GetReference().startswith('SW'):
            xs, ys = [], []
            for p in fp.Pads():
                b = p.GetBoundingBox()
                xs += [pcbnew.ToMM(b.GetLeft()), pcbnew.ToMM(b.GetRight())]
                ys += [pcbnew.ToMM(b.GetTop()), pcbnew.ToMM(b.GetBottom())]
            rule_area(board, 'key ' + fp.GetValue(), (min(xs), min(ys), max(xs), max(ys)), (pcbnew.B_Cu,), tracks=False)


def patch_rules():
    # pcbnew rewrites the .kicad_pro on save; put the JLCPCB rules + Power netclass back
    import json
    pro = os.path.join(P, 'ai_calc.kicad_pro')
    p = json.load(open(pro))
    r = p['board']['design_settings']['rules']
    r.update({'min_clearance': 0.15, 'min_copper_edge_clearance': 0.3, 'min_track_width': 0.15,
              'min_via_diameter': 0.5, 'min_through_hole_diameter': 0.3, 'min_hole_clearance': 0.25,
              'min_hole_to_hole': 0.25, 'min_resolved_spokes': 1})
    ns = p['net_settings']
    d = ns['classes'][0]
    d.update({'clearance': 0.15, 'track_width': 0.2, 'via_diameter': 0.6, 'via_drill': 0.3})
    pw = dict(d); pw.update({'name': 'Power', 'track_width': 0.25, 'clearance': 0.15, 'priority': 1})
    pm = dict(d); pm.update({'name': 'PowerMain', 'track_width': 0.5, 'clearance': 0.2, 'priority': 0})
    cm = dict(d); cm.update({'name': 'Camera', 'track_width': 0.2, 'clearance': 0.2, 'priority': 3})
    ck = dict(d); ck.update({'name': 'CamClock', 'track_width': 0.2, 'clearance': 0.2, 'priority': 2})
    gn = dict(d); gn.update({'name': 'Ground', 'track_width': 0.25, 'clearance': 0.15, 'priority': 1})
    ns['classes'] = [d, pw, pm, gn, cm, ck]
    ns['netclass_patterns'] = ([{'netclass': 'PowerMain', 'pattern': n} for n in ('SYS', 'VBAT_P', 'BAT+', 'VBUS')]
                               + [{'netclass': 'Power', 'pattern': n} for n in POWER_NETS
                                  if n not in ('GND', 'SYS', 'VBAT_P', 'BAT+', 'VBUS')]
                               + [{'netclass': 'Ground', 'pattern': 'GND'}]
                               + [{'netclass': 'CamClock', 'pattern': n} for n in ('CAM_XCLK', 'CAM_PCLK')]
                               + [{'netclass': 'Camera', 'pattern': n} for n in
                                  ['CAM_D%d' % i for i in range(8)] + ['CAM_VSYNC', 'CAM_HREF', 'CAM_SIOC', 'CAM_SIOD']])
    json.dump(p, open(pro, 'w'), indent=2)


POWER_NETS = ['GND', '+3V3', 'SYS', 'VBAT_P', 'BAT+', 'VBUS', 'CAM_2V8', 'CAM_DVDD', 'CAM_AVDD',
              'CAM_AF', 'EPD_SW', 'EPD_RESE', 'EPD_PUMP']


def main():
    # always start from the stage-1 board (outline, holes, user layers) so reruns are clean
    board = pcbnew.LoadBoard(BASE)
    # NPTH holes: solder-mask opening = hole size, so tracks may pass at the normal hole clearance
    for f in board.GetFootprints():
        if f.GetReference().startswith('H'):
            for pad in f.Pads():
                pad.SetSize(pad.GetDrillSize())

    t = ET.parse(NET)
    nets = {}
    for n in t.iter('net'):
        name = n.get('name')
        for node in n.iter('node'):
            nets.setdefault(node.get('ref'), {})[node.get('pin')] = name
    netobj = {}

    def net(name):
        if name not in netobj:
            ni = board.FindNet(name)
            if ni is None:
                ni = pcbnew.NETINFO_ITEM(board, name)
                board.Add(ni)
            netobj[name] = ni
        return netobj[name]

    missing = []
    for c in t.iter('comp'):
        ref = c.get('ref'); val = c.findtext('value'); fpid = c.findtext('footprint')
        lib, name = fpid.split(':')
        fp = pcbnew.FootprintLoad(fp_path(lib), name)
        if fp is None:
            raise SystemExit(f'footprint not found {fpid}')
        fp.SetFPIDAsString(fpid)
        fp.SetReference(ref); fp.SetValue(val)
        sp = c.find('sheetpath').get('tstamps')
        fp.SetPath(pcbnew.KIID_PATH(sp + c.findtext('tstamps')))
        for fld in c.iter('field'):
            if fld.get('name') in ('LCSC', 'MPN'):
                fp.SetField(fld.get('name'), fld.text or '')
        props = {p.get('name'): p.get('value') for p in c.iter('property')}
        attrs = fp.GetAttributes()
        if ref.startswith(('SW', 'TP')):
            attrs |= pcbnew.FP_EXCLUDE_FROM_BOM | pcbnew.FP_EXCLUDE_FROM_POS_FILES
        fp.SetAttributes(attrs)
        board.Add(fp)
        for pad in fp.Pads():
            pn = pad.GetNumber()
            if pn in nets.get(ref, {}):
                pad.SetNet(net(nets[ref][pn]))
            elif pn and pn not in ('MP',) and pad.GetAttribute() != pcbnew.PAD_ATTRIB_NPTH:
                pass
        # MP pads of connectors -> GND
        for pad in fp.Pads():
            if pad.GetNumber() == 'MP':
                pad.SetNet(net(nets[ref].get('MP', 'GND')) if 'MP' in nets.get(ref, {}) else net('GND'))
        # placement
        if ref.startswith('SW'):
            x, y = LABEL2KEY[val]
            fp.SetPosition(MM(x, y))
            fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
            fp.Reference().SetVisible(False)
            fp.Value().SetVisible(False)
        elif ref in PLACE:
            x, y, rot, side = PLACE[ref]
            fp.SetPosition(MM(x, y)); fp.SetOrientationDegrees(rot)
            if side == 'B':
                fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
        else:
            missing.append(ref)
            fp.SetPosition(MM(200, 60 + 2 * len(missing)))
        # silk: only the reference, and only for parts you'd look for while debugging
        for fld in fp.GetFields():
            if not fld.IsReference():
                fld.SetVisible(False)
        for it in fp.GraphicalItems():
            if isinstance(it, pcbnew.PCB_TEXT) and it.GetLayer() in (pcbnew.F_SilkS, pcbnew.B_SilkS):
                it.SetVisible(False) if hasattr(it, 'SetVisible') else None
                fp.Remove(it)
        r = fp.Reference()
        r.SetTextSize(MM(0.8, 0.8)); r.SetTextThickness(pcbnew.FromMM(0.12))
        r.SetVisible(ref[0] in 'UJQL' or ref.startswith('TP'))
    add_epaper_slot(board)
    add_edge_keepout(board)
    add_rule_areas(board)
    # JLCPCB puts its order number where this text is (free option "Specify a location")
    for txt, (x, y) in (('JLCJLCJLCJLC', (160.0, 133.0)), ('AI CALC v1  2026', (160.0, 136.0))):
        t = pcbnew.PCB_TEXT(board); t.SetText(txt); t.SetLayer(pcbnew.F_SilkS)
        t.SetPosition(MM(x, y)); t.SetTextSize(MM(1.0, 1.0)); t.SetTextThickness(pcbnew.FromMM(0.15))
        board.Add(t)
    board.Save(PCB)
    patch_rules()
    print('placed', len(list(board.GetFootprints())), 'footprints; unplaced:', missing)


if __name__ == '__main__':
    main()
