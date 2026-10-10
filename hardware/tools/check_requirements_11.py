"""Requirements trace for the AI-CALCULATOR v14 board (verification/11). Read-only on hardware/kicad.
Usage: python hardware/tools/check_requirements_11.py   -> one TSV line per requirement
(requirement, source, expected, found on the board, PRESENT/MISSING/DIFFERENT, note)."""
import sys, os, math, re, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import kpcb_11 as kpcb

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
K = os.path.join(HW, 'kicad')
b = kpcb.load(os.path.join(K, 'ai_calc.kicad_pcb'))
F = kpcb.footprints(b)
R = []


def rec(req, src, expect, found, ok, note=''):
    R.append((req, src, expect, found, 'PRESENT' if ok is True else ok, note))


def pad(ref, num):
    for p in F[ref]['pads']:
        if p['num'] == str(num):
            return p
    return None


def net(ref, num):
    p = pad(ref, num)
    return p['net'] if p else None


def lcsc(ref):
    pr = F[ref]['props']
    return pr.get('LCSC') or pr.get('LCSC Part') or ''


def near(a, b_, tol=0.05):
    return abs(a - b_) <= tol


# ---------- board stack-up ----------
gen = kpcb.kid(b, 'general')
th = float(kpcb.val(gen, 'thickness'))
layers = [l for l in kpcb.kid(b, 'layers')[1:] if l[2] == 'signal']
setup = kpcb.kid(b, 'setup')
finish = kpcb.val(kpcb.kid(setup, 'stackup'), 'copper_finish')
rec('2 copper layers, 0.8 mm, ENIG', 'decision log st.1/7', '2 / 0.8 / ENIG',
    f'{len(layers)} / {th} / {finish}', len(layers) == 2 and th == 0.8 and finish == 'ENIG')

# ---------- U1 ----------
rec('U1 = ESP32-S3-MINI-1-N4R2 (C3013941), no substitute', 'HANDOFF, 06 N9, 08 §3',
    'value N4R2, LCSC C3013941', f"{F['U1']['props']['Value']}, {lcsc('U1')}, fp {F['U1']['lib']}",
    F['U1']['props']['Value'] == 'ESP32-S3-MINI-1-N4R2' and lcsc('U1') == 'C3013941')

# U1 pad number -> GPIO name, from the schematic symbol
sch = kpcb.load(os.path.join(K, 'mcu.kicad_sch'))
pinname = {}
def walk(n):
    for c in n:
        if isinstance(c, list):
            if c and c[0] == 'pin' and kpcb.kid(c, 'number'):
                pinname[kpcb.val(c, 'name')] = kpcb.val(c, 'number')
            else:
                walk(c)
for sym in kpcb.kids(kpcb.kid(sch, 'lib_symbols'), 'symbol'):
    if 'ESP32' in sym[1]:
        walk(sym)

def gpio_net(io):
    name = {19: 'USB_D-', 20: 'USB_D+'}.get(io, f'IO{io}')
    return net('U1', pinname[name])

cam = dict(CAM_XCLK=18, CAM_D0=13, CAM_D1=11, CAM_D2=10, CAM_D3=12, CAM_D4=14, CAM_D5=16,
           CAM_D6=17, CAM_D7=21, CAM_VSYNC=36, CAM_HREF=47, CAM_PCLK=15, CAM_PWDN=48,
           CAM_RESET=38, CAM_SIOD=40, CAM_SIOC=39, CAM_PWR_EN=34)
bad = [f'{n}@IO{io}={gpio_net(io)}' for n, io in cam.items() if gpio_net(io) != n]
rec('Camera GPIO map (XCLK 18, D0 13, D1 11, D2 10, D3 12, D4 14, D5 16, D6 17, D7 21, VSYNC 36, HREF 47, PCLK 15, PWDN 48, RESET 38, SIOD 40, SIOC 39, PWR_EN 34)',
    'memory pcb-state "must not undo", stage 6', 'all 17 nets on those U1 pins',
    'all 17 match' if not bad else '; '.join(bad), not bad)

other = dict(EPD_CLK=5, EPD_DIN=6, EPD_CS=8, EPD_DC=41, EPD_RST=42, EPD_BUSY=33, I2C_SDA=1, I2C_SCL=2,
             KEYPAD_INT=4, KEY_ON=7, VBAT_SENSE=9, VBUS_SENSE=37, CHG_STAT=35, USB_DM=19, USB_DP=20)
bad = [f'{n}@IO{io}={gpio_net(io)}' for n, io in other.items() if gpio_net(io) != n]
rec('Rest of pins_final.h (EPD 5/6/8/41/42/33, I2C 1/2, KEYPAD_INT 4, KEY_ON 7 (RTC), VBAT 9, VBUS_SENSE 37, CHG_STAT 35, USB 19/20)',
    'pins_final.h, stage 2', 'nets on those U1 pins', 'all 15 match' if not bad else '; '.join(bad), not bad)
io0 = gpio_net(0)
tp1 = net('TP1', 1)
rec('IO0 (BOOT) only to TP1; IO3/45/46 free', '08 §3, stage 14 §8', 'IO0=TP1 net, 3/45/46 unconnected',
    f'IO0={io0} (TP1 {tp1}); IO3={gpio_net(3)!r} IO45={gpio_net(45)!r} IO46={gpio_net(46)!r}',
    io0 == tp1 and all(not (gpio_net(i) or '').strip() or (gpio_net(i) or '').startswith('unconnected') for i in (3, 45, 46)))

# ---------- antenna overhang / screen-section width ----------
segs = []
for g in kpcb.kids(b, 'gr_line'):
    if kpcb.val(g, 'layer') == 'Edge.Cuts':
        s = kpcb.kid(g, 'start'); e = kpcb.kid(g, 'end')
        segs.append((float(s[1]), float(s[2]), float(e[1]), float(e[2])))

def xcross(y):
    out = []
    for x1, y1, x2, y2 in segs:
        if (y1 - y) * (y2 - y) < 0:
            out.append(round(x1 + (y - y1) * (x2 - x1) / (y2 - y1), 3))
    return sorted(out)

edge_l = xcross(97)[0]
body_left = 135.25 - 20.5  # module 15.4 x 20.5, fab outline right edge 135.25 (pad side)
rec('ESP32 antenna overhangs the left board edge (Espressif placement), ~4.2 mm, y 89.3-104.7',
    'stage 12, 06 M1', 'edge x 118.9, module to ~114.7',
    f'edge x {edge_l}; courtyard to 114.45; overhang {edge_l - body_left:.2f} mm', near(edge_l, 118.9) and 3.9 < edge_l - body_left < 4.5)
ys = (85, 90, 95, 100, 104.5, 110)
w = {y: (xcross(y)[0], xcross(y)[-1]) for y in ys}
rec('Screen section narrowed to x 118.9-183.65 (64.75 wide) past the wall pins/stubs', 'stage 12 (C4/C14)',
    '118.9 / 183.65 for y 85-110', '; '.join(f'y{y}: {a}-{c}' for y, (a, c) in w.items()),
    all(near(a, 118.9) and near(c, 183.65) for a, c in w.values()))
ak = [z for z in kpcb.kids(b, 'zone') if (kpcb.val(z, 'name') or '').startswith('antenna clearance')]
rec('Antenna keep-outs (no copper pour) beside the module root', 'stage 12; 06 M2', 'top + bottom keep-outs present',
    f'{len(ak)} zones: ' + ', '.join(kpcb.val(z, "name") for z in ak), len(ak) == 2,
    '06 M2: 1.25 mm GND sliver between them at the root (v15 item, not a blocker)')

# ---------- camera ----------
j1 = F['J1']
rec('J1 = C6364666 CamReversed footprint, rot 180 at (150, 161)', 'memory "must not undo", stage 6, 05 C4',
    'lib *_CamReversed, C6364666', f"{j1['lib']}, {lcsc('J1')}, ({j1['x']}, {j1['y']}) rot {j1['rot']}",
    j1['lib'].endswith('_CamReversed') and lcsc('J1') == 'C6364666' and j1['rot'] == 180)
expect_j1 = {1: '', 2: 'GND', 3: 'CAM_SIOD', 4: 'CAM_AVDD', 5: 'CAM_SIOC', 6: 'CAM_RESET', 7: 'CAM_VSYNC', 8: 'CAM_PWDN',
             9: 'CAM_HREF', 10: 'CAM_DVDD', 11: 'CAM_2V8', 12: 'CAM_D7', 13: 'CAM_XCLK', 14: 'CAM_D6', 15: 'GND',
             16: 'CAM_D5', 17: 'CAM_PCLK', 18: 'CAM_D4', 19: 'CAM_D0', 20: 'CAM_D3', 21: 'CAM_D1', 22: 'CAM_D2',
             23: 'GND', 24: 'CAM_AF'}
bad = []
for k, v in expect_j1.items():
    n = net('J1', k) or ''
    if v == '' and (n == '' or n.startswith('unconnected')):
        continue
    if n != v:
        bad.append(f'{k}={n}')
p1, p24 = pad('J1', 1), pad('J1', 24)
rec('J1 pad nets = OV5640 module pinout (pad N = finger N)', '06 N1, 08 §2.1', '24 pads as 08 §2.1',
    ('all 24 match' if not bad else ', '.join(bad)) + f'; pad1 x {p1["x"]:.2f}, pad24 x {p24["x"]:.2f}',
    not bad and near(p1['x'], 144.25) and near(p24['x'], 155.75))
circ = [g for g in kpcb.kids(b, 'gr_circle') if kpcb.val(g, 'layer') == 'User.3'
        and near(float(kpcb.kid(g, 'center')[1]), 150) and near(float(kpcb.kid(g, 'center')[2]), 95.1, 0.01)]
r_ = None
if circ:
    c = kpcb.kid(circ[0], 'center'); e = kpcb.kid(circ[0], 'end')
    r_ = math.hypot(float(e[1]) - float(c[1]), float(e[2]) - float(c[2]))
rect = [g for g in kpcb.kids(b, 'gr_rect') if kpcb.val(g, 'layer') == 'User.3' and kpcb.kid(g, 'start')[1:] == ['144', '89.099']]
rec('Camera 7 mm back-cover window centred at KiCad (150, 95.1), 12 x 12 keep-out', 'stage 13, grinding guide',
    'User.3 marker Ø7 at (150, 95.1); 12x12 keep-out', f'circle at (150, 95.099) Ø{2 * r_:.1f}; keep-out rect {"present" if rect else "missing"}; text says "7 mm"',
    'DIFFERENT (doc layer only)' if r_ and abs(2 * r_ - 7) > 0.05 else (bool(circ and rect)),
    'The window is drilled in the shell, not the board; this circle is on User.3 (not fabricated). Cosmetic: drawn Ø6.0, label says 7 mm.')
# keep-out free of parts on F side?
inside = [r for r, f in F.items() if f['layer'] == 'F.Cu' and 144 <= f['x'] <= 156 and 89.1 <= f['y'] <= 101.1]
rec('No F-side parts in the camera keep-out 144-156 x 89.1-101.1', 'stage 13 camera mounting', 'none',
    ', '.join(inside) or 'none', not inside)

# ---------- e-paper ----------
j2 = F['J2']
slot_x = [s for s in segs if near(s[0], s[2]) and 172 < s[0] < 174 and abs(s[3] - s[1]) > 10]
arcs = [g for g in kpcb.kids(b, 'gr_arc') if kpcb.val(g, 'layer') == 'Edge.Cuts']
ay = sorted(float(kpcb.kid(a, 'mid')[2]) for a in arcs)
rec('E-paper FPC through 1.0 x 14 mm slot at x 172.9 to J2 (C6364666)', 'memory "must not undo", stage 4/5, 11',
    'slot x 172.4-173.4, 14.0 long; J2 C6364666', f"slot sides x {[s[0] for s in slot_x]}, ends y {ay} (len {ay[-1] - ay[0]:.2f}); J2 {lcsc('J2')} at ({j2['x']}, {j2['y']}) rot {j2['rot']}",
    len(slot_x) == 2 and near(ay[-1] - ay[0], 14.0) and lcsc('J2') == 'C6364666')
expect_j2 = {2: 'EPD_GDR', 3: 'EPD_RESE', 4: 'EPD_VGL', 5: 'EPD_VGH', 8: 'GND', 9: 'EPD_BUSY', 10: 'EPD_RST', 11: 'EPD_DC',
             12: 'EPD_CS', 13: 'EPD_CLK', 14: 'EPD_DIN', 15: '+3V3', 16: '+3V3', 17: 'GND', 18: 'EPD_VDD', 19: 'EPD_VPP',
             20: 'EPD_VSH', 21: 'EPD_PREVGH', 22: 'EPD_VSL', 23: 'EPD_PREVGL', 24: 'EPD_VCOM'}
bad = [f'{k}={net("J2", k)}' for k, v in expect_j2.items() if net('J2', k) != v]
rec('J2 pad nets = SSD1680 2.13" 24-pin order (BS1=GND, 4-wire SPI)', '08 §2.2, 06 N8', 'as 08 §2.2',
    'all match' if not bad else ', '.join(bad), not bad)
win = [g for g in kpcb.kids(b, 'gr_rect') if kpcb.val(g, 'layer') == 'User.3' and kpcb.kid(g, 'start')[1:] == ['119.675', '80.95']]
rec('Screen window 60.65 x 24.3, top edge 24.0 below case top, centred x 150', 'C12/C13, stage 11', 'User.3 rect 119.675-180.325 x 80.95-105.25',
    'present' if win else 'missing', bool(win))
rec('R12 3 Ω booster current-limit (kept)', 'stage 12 Q12', '3R', F['R12']['props']['Value'], F['R12']['props']['Value'].startswith('3R'))

# ---------- keypad ----------
sw = {r: f for r, f in F.items() if r.startswith('SW')}
onb = all(f['layer'] == 'B.Cu' for f in sw.values())
pairs = collections.Counter()
for r, f in sw.items():
    nets = tuple(sorted({p['net'] for p in f['pads'] if p['net']}))
    pairs[nets] += 1
dups = [k for k, v in pairs.items() if v > 1]
rec('TCA8418 (C138713) + 50 key pads on B.Cu, unique row/col pairs', 'stage 2/3, 06 check 13',
    'U6 TCA8418RTWR C138713; 50 SW on B.Cu', f"U6 {F['U6']['props']['Value']} {lcsc('U6')}; {len(sw)} SW, all B.Cu={onb}, duplicate net pairs={len(dups)}",
    F['U6']['props']['Value'] == 'TCA8418RTWR' and lcsc('U6') == 'C138713' and len(sw) == 50 and onb and not dups)
on = F['SW50']
rec('ON key on its own RTC pin (KEY_ON = IO7, pad to GND, R17 100 k pull-up)', 'stage 2, pins_final.h', 'SW50 ON = KEY_ON / GND',
    f"SW50 {on['props']['Value']} nets {sorted({p['net'] for p in on['pads'] if p['net']})}; R17 {F['R17']['props']['Value']} {lcsc('R17')} nets {net('R17', 1)}/{net('R17', 2)}",
    {p['net'] for p in on['pads'] if p['net']} == {'KEY_ON', 'GND'} and F['R17']['props']['Value'] == '100k' and lcsc('R17') == 'C25741' and 'KEY_ON' in (net('R17', 1), net('R17', 2)))
lab = {r: f['props']['Value'] for r, f in sw.items()}
rec('Key row 2 = CALC and ∫dx (fx-115ES)', 'stage 14 §7', 'SW values / keypad silk CALC, ∫dx',
    'values: ' + ', '.join(sorted(v for v in lab.values() if v in ('CALC', '∫dx', 'Abs', 'x^3', 'x³'))),
    'CALC' in lab.values() and '∫dx' in lab.values(),
    'User.2 labels say CALC/∫dx; the B.Cu "no via" keep-out zones still carry the old names "key Abs"/"key x^3" (zone names only, no effect).')
sw1 = F['SW1']
rec('SW1 (SHIFT) bar notched for H3: KeyPad_6.0x4.5_H3notch', 'stage 14 §5', 'lib *_H3notch', sw1['lib'], sw1['lib'].endswith('_H3notch'))

# ---------- power ----------
rec('U2 MCP73831 (C424093), R2 20 k on PROG -> 50 mA', 'stage 2/3, decision log', 'U2 C424093; R2 20k on U2 pin 5',
    f"U2 {F['U2']['props']['Value']} {lcsc('U2')}; U2.5={net('U2', 5)}; R2 {F['R2']['props']['Value']} {lcsc('R2')} nets {net('R2', 1)}/{net('R2', 2)}",
    lcsc('U2') == 'C424093' and F['R2']['props']['Value'].startswith('20k') and net('U2', 5) in (net('R2', 1), net('R2', 2)))
rec('U3 = RT9080-33GJ5 (C841192), not AP2112K', 'stage 11, decision log', 'RT9080-33GJ5 C841192',
    f"{F['U3']['props']['Value']} {lcsc('U3')}; VIN {net('U3', 1)} EN {net('U3', 3)} VOUT {net('U3', 5)}",
    F['U3']['props']['Value'] == 'RT9080-33GJ5' and lcsc('U3') == 'C841192')
rec('U7 USBLC6-2SC6 (C7519) on USB, reference pin on +3V3', 'stage 3', 'C7519; pin 5 = +3V3; DP/DM through',
    f"{F['U7']['props']['Value']} {lcsc('U7')}; pins " + ', '.join(f'{i}={net("U7", i)}' for i in range(1, 7)),
    lcsc('U7') == 'C7519' and net('U7', 5) == '+3V3' and {net('U7', 1), net('U7', 3)} >= {'USB_DP'} | {'USB_DM'} or {net('U7', i) for i in range(1, 7)} >= {'USB_DP', 'USB_DM', '+3V3', 'GND'})
rec('D7 SMF5.0A TVS (C193402), cathode (pad 1) on VBUS', 'stage 3, 08 §1', 'C193402; pad1 VBUS, pad2 GND',
    f"{F['D7']['props']['Value']} {lcsc('D7')}; pad1={net('D7', 1)} pad2={net('D7', 2)}",
    lcsc('D7') == 'C193402' and net('D7', 1) == 'VBUS' and net('D7', 2) == 'GND')
j3 = F['J3']
j3n = [net('J3', i) for i in range(1, 5)]
rec('J3 = C46061768 right-angle socket at (127.5, 72.0), pads 1-4 VBUS / D- / D+ / GND', 'stage 13b (option A), stage 14 §1',
    'C46061768; VBUS, USB_DM, USB_DP, GND', f"{lcsc('J3')} at ({j3['x']}, {j3['y']}); nets {j3n}; pad1 x {pad('J3', 1)['x']:.2f}, pad4 x {pad('J3', 4)['x']:.2f}",
    lcsc('J3') == 'C46061768' and j3n == ['VBUS', 'USB_DM', 'USB_DP', 'GND'] and near(j3['x'], 127.5) and near(j3['y'], 72.0))
texts = [(t[1], float(kpcb.kid(t, 'at')[1]), float(kpcb.kid(t, 'at')[2])) for t in kpcb.kids(b, 'gr_text') if kpcb.val(t, 'layer') == 'F.SilkS']
def closest(txt, x, y):
    c = [(math.hypot(tx - x, ty - y), tx, ty) for t, tx, ty in texts if t == txt]
    return min(c) if c else None
p1 = pad('J3', 1); p4 = pad('J3', 4)
cN, cP, cM = closest('N', p1['x'], p1['y']), closest('+', p1['x'], p1['y']), closest('-', p4['x'], p4['y'])
rec('J3 silk "N" and "+" at pin 1, "-" at pin 4', 'stage 14 §1', 'silk next to pads 1 / 4',
    f"N {cN[0]:.1f} mm from pad 1 (at {cN[1]}, {cN[2]}); + {cP[0]:.1f} mm from pad 1; - {cM[0]:.1f} mm from pad 4",
    cN[0] < 4 and cP[0] < 4 and cM[0] < 4)
j4 = F['J4']
rec('J4 JST-PH S2B-PH-SM4-TB (C295747): pad 1 GND, pad 2 BAT+; silk - / +', 'stage 3, 08 §2.4', 'C295747; 1 GND, 2 BAT+',
    f"{lcsc('J4')} at ({j4['x']}, {j4['y']}); pad1={net('J4', 1)} pad2={net('J4', 2)}; '-' {closest('-', pad('J4', 1)['x'], pad('J4', 1)['y'])[0]:.1f} mm from pad 1, '+' {closest('+', pad('J4', 2)['x'], pad('J4', 2)['y'])[0]:.1f} mm from pad 2",
    lcsc('J4') == 'C295747' and net('J4', 1) == 'GND' and net('J4', 2) == 'BAT+',
    'Value field still reads "Battery JST-PH (Adafruit 1570)" (text only; the socket is the same for #1317).')
tpexp = {'TP1': 'BOOT', 'TP2': 'UART_TX', 'TP3': 'UART_RX', 'TP4': 'GND', 'TP5': '+3V3', 'TP6': 'BAT+', 'TP7': 'EN'}
tpgot = {t: net(t, 1) for t in tpexp}
rec('TP1-TP7: BOOT, UART TX, UART RX, GND, +3V3, BAT+, EN (recovery = TP1->TP4, tap TP7)', 'stage 11, stage 14 §8',
    str(tpexp), str(tpgot), all(tpexp[t] in (tpgot[t] or '') or (t == 'TP1' and tpgot[t] == io0) or (t == 'TP7' and tpgot[t] == net('U1', pinname['EN'])) for t in tpexp))
leds = [r for r, f in F.items() if r.startswith('LED') or 'LED' in f['props'].get('Value', '').upper() or 'LED' in f['lib'].upper()]
rec('No LED (exam mode; Nirav "I dont want the light")', 'chat 2026-10-03 04:20, stage 2', 'no LED part', ', '.join(leds) or 'none', not leds)

# battery never on the magnet contacts: J3 nets vs battery nets, and parts touching VBUS
vbus_parts = sorted({f'{r}.{p["num"]}' for r, f in F.items() for p in f['pads'] if p['net'] == 'VBUS'})
bat_nets = {'BAT+', 'VBAT_P', 'SYS'}
rec('Magnet contacts never carry battery voltage (D1 VBUS->SYS, Q2 gate on VBUS, charger blocks)', 'stage 2, decision log, 06 N3',
    'J3 has no BAT+/VBAT_P/SYS; D1 A=VBUS K=SYS; Q2 G=VBUS',
    f"J3 nets {set(j3n)}; D1 1={net('D1', 1)} 2={net('D1', 2)}; Q2 1(G)={net('Q2', 1)} 2(S)={net('Q2', 2)} 3(D)={net('Q2', 3)}; VBUS pads: {', '.join(vbus_parts)}",
    not (set(j3n) & bat_nets) and net('D1', 2) == 'VBUS' and net('D1', 1) == 'SYS' and net('Q2', 1) == 'VBUS')
rec('Q1 AO3401A reverse-battery FET between J4 BAT+ and VBAT_P', 'stage 2/3, 06 N3', 'Q1 S=VBAT_P, D=BAT+',
    f"Q1 {lcsc('Q1')} G={net('Q1', 1)} S={net('Q1', 2)} D={net('Q1', 3)}", net('Q1', 2) == 'VBAT_P' and net('Q1', 3) == 'BAT+')
rec('R20 STAT pull-up goes to VBUS_SENSE (no back-feed without cable)', 'stage 11 (review S1)', 'R20 100k between CHG_STAT-side and VBUS_SENSE',
    f"R20 {F['R20']['props']['Value']} {lcsc('R20')} nets {net('R20', 1)}/{net('R20', 2)}", 'VBUS_SENSE' in (net('R20', 1), net('R20', 2)))
rec('VBUS_SENSE divider 10 k / 20 k (R4/R5) -> 3.33 V at 5 V', '06 N7', 'R4 10k, R5 20k',
    f"R4 {F['R4']['props']['Value']} {net('R4', 1)}/{net('R4', 2)}; R5 {F['R5']['props']['Value']} {net('R5', 1)}/{net('R5', 2)}",
    F['R4']['props']['Value'] == '10k' and F['R5']['props']['Value'] == '20k' and 'VBUS_SENSE' in (net('R4', 1), net('R4', 2)))
rec('R17 = 100 k (C25741)', 'stage 14 §6', '100k C25741', f"{F['R17']['props']['Value']} {lcsc('R17')}", F['R17']['props']['Value'] == '100k' and lcsc('R17') == 'C25741')
cc = {c: (F[c]['props']['Value'], lcsc(c), net(c, 1), net(c, 2)) for c in ('C32', 'C33', 'C34')}
rec('C32 / C33 / C34 = 22 µF 0805 (C45783); C33/C34 on +3V3 at the ESP32', 'stage 11 (C32), 13b (C33/C34)', '22uF C45783',
    str(cc), all(v[0] == '22uF' and v[1] == 'C45783' for v in cc.values()) and all('+3V3' in v[2:] for k, v in cc.items() if k != 'C32'),
    f"C32 is on {cc['C32'][2]}/{cc['C32'][3]}")
rec('Camera LDOs U4 ME6211C28 (C53099) / U5 ME6211C15 (C53100), CE = CAM_PWR_EN, R10 100 k pull-down', 'stage 2/3, 06 N3',
    'CE on CAM_PWR_EN', f"U4 {lcsc('U4')} CE={net('U4', 3)}; U5 {lcsc('U5')} CE={net('U5', 3)}; R10 {F['R10']['props']['Value']} {net('R10', 1)}/{net('R10', 2)}",
    lcsc('U4') == 'C53099' and lcsc('U5') == 'C53100' and net('U4', 3) == 'CAM_PWR_EN' and net('U5', 3) == 'CAM_PWR_EN')
rec('SCCB pull-ups R18/R19 4.7 k to CAM_2V8', 'stage 3', '4.7k to CAM_2V8',
    f"R18 {F['R18']['props']['Value']} {net('R18', 1)}/{net('R18', 2)}; R19 {F['R19']['props']['Value']} {net('R19', 1)}/{net('R19', 2)}",
    'CAM_2V8' in (net('R18', 1), net('R18', 2)) and 'CAM_2V8' in (net('R19', 1), net('R19', 2)))
rec('EN RC: R1 10 k + C4 1 µF', '06 check 10', '10k / 1uF on EN', f"R1 {F['R1']['props']['Value']} {net('R1', 1)}/{net('R1', 2)}; C4 {F['C4']['props']['Value']} {net('C4', 1)}/{net('C4', 2)}",
    F['R1']['props']['Value'] == '10k' and F['C4']['props']['Value'] == '1uF')

# ---------- holes ----------
hexp = {'H1': (127.5, 126.1, '6'), 'H3': (174.0, 126.1, '6'), 'H2': (130.85, 176.25, '4.2x5'), 'H9': (169.2, 176.2, '4.4x4.8'),
        'H4': (171.06, 145.96, '4.2'), 'H6': (130.6, 186.99, '4.2'), 'H8': (128.85, 137.0, '4.2'), 'H10': (171.25, 136.8, '4.2'),
        'H13': (130.5, 198.325, '4.2x4.45'), 'H14': (169.1, 198.375, '4.2x4.65')}
bad = []
got = []
for h, (x, y, d) in hexp.items():
    f = F[h]; p = f['pads'][0]
    dr = 'x'.join(v for v in p['drill'] if v != 'oval')
    dr = dr if 'x' in dr else dr
    ok = near(f['x'], x, 0.01) and near(f['y'], y, 0.01) and dr.replace('.0', '') == d
    got.append(f"{h} ({f['x']}, {f['y']}) Ø{dr}")
    if not ok:
        bad.append(h)
rec('Mount holes H1/H3 Ø6.0 screw posts; H4/H6/H8/H10 Ø4.2; H2/H9/H13/H14 slots; H6 at y 186.99 (stage 12/13 positions)',
    'stage 10/12/13, DESIGN_SUMMARY', 'positions and drills as stage 13', '; '.join(got), not bad, ('mismatch: ' + ', '.join(bad)) if bad else '')
dist = lambda a, c: math.hypot(F[a]['x'] - F[c]['x'], F[a]['y'] - F[c]['y'])
sp = dict(H1H3=dist('H1', 'H3'), H8H10=dist('H8', 'H10'), H2H9=dist('H2', 'H9'), H13H14=dist('H13', 'H14'))
rec('Measured spacings: H1-H3 46.5 (C8), H8-H10 42.3 (C8; board 42.40), H2-H9 38.3, H13-H14 38.6 (photo pairs)', 'measurements C8, 06 N4',
    '46.5 / 42.3-42.4 / 38.3 / 38.6', ', '.join(f'{k} {v:.2f}' for k, v in sp.items()),
    near(sp['H1H3'], 46.5, 0.1) and near(sp['H8H10'], 42.35, 0.1) and near(sp['H2H9'], 38.3, 0.1) and near(sp['H13H14'], 38.6, 0.1))
mh = [r for r in F if r in ('H5', 'H7', 'H11', 'H12')]
rec('Bottom screw posts handled by edge notches (no H5/H7/H11/H12)', 'stage 10/11 ("bottom notches unchanged")', 'notches in outline at y ~208',
    f"H5/H7/H11/H12 present: {mh or 'none'}; outline crossings at y 208: {xcross(208)}", not mh and len(xcross(208)) >= 6)

# ---------- fiducials, silk ----------
fid = sorted((r, F[r]['x'], F[r]['y']) for r in F if r.startswith('FID'))
rec('3 fiducials (stage 12)', 'stage 12', 'FID1-3 on F.Cu', str(fid), len(fid) == 3)
ver = [t for t in texts if t[0].startswith('AI CALC')]
rec('Silk "AI CALC v14 2026-10-04" + JLC order-number mark', 'stage 14, 06 check 23', 'present', str([t[0] for t in texts if t[0].startswith(('AI CALC', 'JLC'))]),
    any(t[0] == 'AI CALC v14 2026-10-04' for t in ver) and any(t[0].startswith('JLC') for t in texts))
tb = kpcb.kid(b, 'title_block')
rec('Title block', '-', 'current', f"title {kpcb.val(tb, 'title')!r}, rev {kpcb.val(tb, 'rev')!r}", 'DIFFERENT (cosmetic)',
    'Says "fx-300ES Plus transplant", rev "0.1-geometry"; frame is not plotted (plotframeref no), so nothing reaches JLCPCB.')

# ---------- battery area ----------
bay = [r for r, f in F.items() if f['layer'] == 'F.Cu' and 147.4 <= f['x'] <= 180.9 and 60.0 <= f['y'] <= 80.7]
rec('#1317 battery envelope (x 147.4-180.9, y 60.0-80.7, on the back-cover floor) has no board parts under it', 'battery_upgrade.md §8 (memory pcb-state)',
    'no F-side footprints in the envelope; board corner cut out', f"parts: {bay or 'none'}; outline right edge at y 63-75 = {[xcross(y)[-1] for y in (63, 70, 75)]}",
    not bay)

nx = xcross(208)
c1, c2 = (nx[1] + nx[2]) / 2, (nx[3] + nx[4]) / 2
rec('Bottom screw-post notches ~39.0 apart (C10 42.85 o-o)', 'stage 11 C10', '≈ 38.94-39.0', f'notch centres x {c1:.2f} / {c2:.2f} at y 208 -> {c2 - c1:.2f}', near(c2 - c1, 38.97, 0.15))
rec('UART backup on TP2/TP3 = TXD0/RXD0 (IO43/44)', 'stage 11, stage 14 §8', 'U1 TXD0 = TP2 net, RXD0 = TP3 net',
    f"TXD0={net('U1', pinname['TXD0'])}, RXD0={net('U1', pinname['RXD0'])}", net('U1', pinname['TXD0']) == net('TP2', 1) and net('U1', pinname['RXD0']) == net('TP3', 1))
rec('I2C pull-ups R13/R14 4.7 k, KEYPAD_INT R15 10 k, TCA_RESET R16 10 k (not wired to the ESP32)', 'stage 3, Firmware Q13', '4.7k/4.7k/10k/10k to +3V3',
    ', '.join(f"{r} {F[r]['props']['Value']} {net(r, 1)}/{net(r, 2)}" for r in ('R13', 'R14', 'R15', 'R16')),
    [F[r]['props']['Value'] for r in ('R13', 'R14', 'R15', 'R16')] == ['4.7k', '4.7k', '10k', '10k'])
rec('VBAT sense 1 M / 1 M + 100 nF on IO9; STAT through D6 clamp', 'stage 2, pins_final.h', 'R6/R7 1M, C9 100nF, D6 CHG_STAT_RAW->CHG_STAT',
    f"R6 {net('R6', 1)}/{net('R6', 2)}, R7 {net('R7', 1)}/{net('R7', 2)}, D6 {net('D6', 1)}->{net('D6', 2)}",
    net('R6', 2) == 'VBAT_SENSE' and net('D6', 2) == 'CHG_STAT')
bom = open(os.path.join(HW, 'fab', 'ai_calc_BOM_JLCPCB.csv'), encoding='utf-8').read()
need = ['C3013941', 'C841192', 'C424093', 'C7519', 'C193402', 'C46061768', 'C295747', 'C6364666', 'C138713', 'C25741', 'C45783', 'C25765']
miss = [n for n in need if n not in bom]
rec('fab BOM carries the decided LCSC parts', 'fab/ai_calc_BOM_JLCPCB.csv', ', '.join(need), 'all present' if not miss else 'missing ' + ', '.join(miss), not miss,
    'BOM comment for J4 still reads "Battery JST-PH (Adafruit 1570)" (copied from the footprint value; JLCPCB ignores it).')

for row in R:
    print('\t'.join(str(c) for c in row))
