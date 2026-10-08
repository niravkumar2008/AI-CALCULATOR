"""Minimal read-only KiCad s-expression parser used by check_requirements_11.py."""
import re, math

TOK = re.compile(r'\(|\)|"(?:[^"\\]|\\.)*"|[^\s()]+')


def parse(text):
    stack = [[]]
    for m in TOK.finditer(text):
        t = m.group(0)
        if t == '(':
            stack.append([])
        elif t == ')':
            x = stack.pop()
            stack[-1].append(x)
        else:
            if t[0] == '"':
                t = t[1:-1].replace('\\"', '"').replace('\\\\', '\\')
            stack[-1].append(t)
    return stack[0][0]


def kids(node, name):
    return [c for c in node if isinstance(c, list) and c and c[0] == name]


def kid(node, name):
    k = kids(node, name)
    return k[0] if k else None


def val(node, name, i=1, default=None):
    k = kid(node, name)
    return k[i] if k and len(k) > i else default


def load(path):
    return parse(open(path, encoding='utf-8').read())


def footprints(board):
    out = {}
    for fp in kids(board, 'footprint'):
        at = kid(fp, 'at')
        x, y = float(at[1]), float(at[2])
        rot = float(at[3]) if len(at) > 3 else 0.0
        props = {p[1]: p[2] for p in kids(fp, 'property')}
        ref = props.get('Reference', '?')
        pads = []
        for p in kids(fp, 'pad'):
            pat = kid(p, 'at')
            px, py = float(pat[1]), float(pat[2])
            prot = math.radians(-rot)
            # KiCad: rotate local pad pos by footprint rotation (clockwise in screen coords)
            ax = x + px * math.cos(prot) - py * math.sin(prot)
            ay = y + px * math.sin(prot) + py * math.cos(prot)
            net = kid(p, 'net')
            netname = net[-1] if net else ''
            size = kid(p, 'size')
            drill = kid(p, 'drill')
            pads.append(dict(num=p[1], type=p[2], shape=p[3], x=ax, y=ay, net=netname,
                             size=size[1:] if size else None, drill=drill[1:] if drill else None,
                             layers=(kid(p, 'layers') or [None])[1:]))
        texts = []
        for t in kids(fp, 'fp_text'):
            texts.append((t[1], t[2], val(t, 'layer')))
        for p in kids(fp, 'property'):
            texts.append(('property:' + p[1], p[2], val(p, 'layer')))
        out[ref] = dict(ref=ref, lib=fp[1], layer=val(fp, 'layer'), x=x, y=y, rot=rot,
                        props=props, pads=pads, texts=texts,
                        attr=(kid(fp, 'attr') or [None])[1:])
    return out


def gr_items(board, kind):
    return kids(board, kind)
