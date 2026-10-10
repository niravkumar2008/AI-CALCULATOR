#!/usr/bin/env python3
"""Pre-route the nets that matter before the autorouter fills in the rest (all tracks locked).

1. SYS: 0.5 mm, around the ESP32 module (never under it), from the power area to U3 and the
   camera regulators.
2. +3V3 trunk: 0.5 mm from U3 to the ESP32 3V3 pin.
3. Camera bus: 0.2 mm tracks kept 0.4 mm apart (0.6 mm pitch) except right at the pads,
   F.Cu preferred so the B side under it stays as ground as far as the key pads allow.
usage: prepass.py <pcb>
"""
import sys, math
import pcbnew
from shapely.geometry import box
sys.path.insert(0, '/home/claude/pcb/work')
from gridroute import Router, pad_pts, net_pts, LAYERS, mm

pcb = sys.argv[1]
b = pcbnew.LoadBoard(pcb)
R = Router(b, G=0.1)


def pad(ref, num):
    fp = b.FindFootprintByReference(ref)
    for p in fp.Pads():
        if p.GetNumber() == num:
            return p
    raise KeyError(ref + '.' + num)


u1 = b.FindFootprintByReference('U1')
bb = u1.GetBoundingBox(False)
module = box(mm(bb.GetLeft()) - 0.3, mm(bb.GetTop()) - 0.3, mm(bb.GetRight()) + 0.3, mm(bb.GetBottom()) + 0.3)
around_module = [(l, module) for l in LAYERS]
under = box(mm(bb.GetLeft()) + 1.2, mm(bb.GetTop()) + 1.2, mm(bb.GetRight()) - 1.2, mm(bb.GetBottom()) - 1.2)


def connect_tree(netname, order, width, clear, relief=0.9, forbid=None, b_cost=1.5):
    """connect the pads in `order` one after another to the copper already on the net"""
    net = b.FindNet(netname)
    first = pad(*order[0])
    done = pad_pts(first)
    ok = True
    for ref, num in order[1:]:
        tgt = pad(ref, num)
        have = net_pts(b, net.GetNetCode()) or done
        have += [q for q in done]
        res = R.route(net, pad_pts(tgt), have, width, clear, relief=relief, relief_clear=0.17,
                      extra_forbid=forbid, b_cost=b_cost)
        if res is None:
            print('  FAILED', netname, ref, num); ok = False
        else:
            done += pad_pts(tgt)
    return ok



# 3. camera bus, in the order the pins sit on J1 (left to right)
cam = ['CAM_SIOD', 'CAM_SIOC', 'CAM_RESET', 'CAM_VSYNC', 'CAM_PWDN', 'CAM_HREF', 'CAM_D7', 'CAM_XCLK',
       'CAM_D6', 'CAM_D5', 'CAM_PCLK', 'CAM_D4', 'CAM_D0', 'CAM_D3', 'CAM_D1', 'CAM_D2']
j1 = b.FindFootprintByReference('J1')
def j1x(n):
    for p in j1.Pads():
        if p.GetNetname() == n:
            return mm(p.GetPosition().x)
    return 0
cam.sort(key=j1x, reverse=True)   # innermost (right end of J1) first: the bus comes in from the right
extra = {'CAM_RESET': [('R8', '2'), ('C18', '1')], 'CAM_PWDN': [('R9', '1')],
         'CAM_SIOD': [('R18', '2')], 'CAM_SIOC': [('R19', '2')]}
jb = j1.GetBoundingBox(False)
# the socket body (above its pad row): the bus must come in from below so the order stays straight
body = box(mm(jb.GetLeft()), mm(jb.GetTop()), mm(jb.GetRight()), 161.9)
leftside = box(127.0, 108.0, 143.8, 172.0)   # keep the bus on the right of J1
# AF supply first: J1 pin 24 is at the right end, under the bus's entry
print('CAM_AF', 'ok' if R.route(b.FindNet('CAM_AF'), pad_pts(pad('J1', '24')), pad_pts(pad('D2', '1')), 0.25, 0.2,
                                 relief=1.0, relief_clear=0.16, b_cost=3) else 'FAILED')
ux0, uy0, ux1, uy1 = mm(bb.GetLeft()), mm(bb.GetTop()), mm(bb.GetRight()), mm(bb.GetBottom())


def stub(p, net, length):
    """straight F.Cu track out of a pad, away from its footprint; returns the end point"""
    q = p.GetPosition(); x, y = mm(q.x), mm(q.y)
    if p.GetParentFootprint().GetReference() == 'J1':
        dx, dy = 0, 1                       # J1 pads face down (+y)
    elif abs(x - ux1) < 1.5:
        dx, dy = 1, 0                       # U1 right column
    elif abs(y - uy0) < 1.5:
        dx, dy = 0, -1                      # U1 top row
    else:
        dx, dy = 0, 1                       # U1 bottom row
    ex, ey = x + dx * length, y + dy * length
    t = pcbnew.PCB_TRACK(b); t.SetStart(q); t.SetEnd(pcbnew.VECTOR2I_MM(ex, ey)); t.SetWidth(pcbnew.FromMM(0.2))
    t.SetLayer(LAYERS[0]); t.SetNet(net); b.Add(t)
    return [(ex, ey, LAYERS[0])], t


for n in cam:
    net = b.FindNet(n)
    pu = [p for p in u1.Pads() if p.GetNetname() == n][0]
    pj = [p for p in j1.Pads() if p.GetNetname() == n][0]
    su, tu = stub(pu, net, {'CAM_RESET': 2.0, 'CAM_SIOC': 2.7, 'CAM_SIOD': 3.4}.get(n, 1.3))
    sj, tj = stub(pj, net, 1.4)
    res = None
    for clear in ((0.45, 0.35, 0.25) if n in ('CAM_XCLK', 'CAM_PCLK') else (0.4, 0.3, 0.22)):
        res = R.route(net, su, sj, 0.2, clear, relief=1.0, relief_clear=0.16, b_cost=2.5, via_cost=60,
                      extra_forbid=[(LAYERS[0], body)] + [(l, under) for l in LAYERS] + [(l, leftside) for l in LAYERS])
        if res:
            break
    print(n, ('ok, spacing %.2f' % clear) if res else 'FAILED (left to the autorouter)')
    if not res:
        b.Remove(tu); b.Remove(tj)


# pull-ups / pull-downs after the whole bus is in, so they can't block a bus track
for n, lst in extra.items():
    net = b.FindNet(n)
    pj = [p for p in j1.Pads() if p.GetNetname() == n][0]
    for ref, num in lst:
        tgt = net_pts(b, net.GetNetCode()) or pad_pts(pj)
        r2 = R.route(net, pad_pts(pad(ref, num)), tgt, 0.2, 0.25, relief=0.8, relief_clear=0.16, b_cost=2.0)
        print('  +', n, ref, 'ok' if r2 else 'FAILED')

# 1. SYS
corridor = box(133.0, 88.0, 147.5, 160.0)
print('SYS'); connect_tree('SYS', [('D1', '1'), ('Q2', '2'), ('U3', '1'), ('C7', '1'), ('U3', '3'),
                                    ('U4', '1'), ('C10', '1'), ('U5', '1'), ('C14', '1')],
                           0.5, 0.25, forbid=around_module, b_cost=0.6)
# 2. +3V3 trunk
print('+3V3'); connect_tree('+3V3', [('U3', '5'), ('C8', '1'), ('U1', '3')], 0.5, 0.2, forbid=None)


n_lock = 0
for t in b.GetTracks():          # the board has no other tracks at this point
    t.SetLocked(True); n_lock += 1
b.Save(pcb)
print('locked pre-routed items:', n_lock)
