# Inline SVG builders for the bring-up guides. Colours come from CSS classes defined in CSS below,
# so every drawing follows the page's light / dark theme.

CSS = r"""
/* ---- visual additions (2026-10-10): tools, board map, measurement cards, diagrams ---- */
.primer h2 .n{color:var(--copper)}
.meas{display:grid;grid-template-columns:minmax(0,300px) minmax(0,1fr);gap:14px;align-items:start;background:var(--surface);border:1px solid var(--line);border-radius:10px;padding:12px}
@media (max-width:600px){.meas{grid-template-columns:minmax(0,1fr)}}
.meas img{width:100%;height:auto;border:1px solid var(--line);border-radius:8px;display:block}
.meas h4{margin:0 0 6px;font:700 1rem var(--f-display)}
.meas dl{display:grid;grid-template-columns:max-content minmax(0,1fr);gap:4px 12px;margin:0;font-size:.92rem}
.meas dt{font-family:var(--f-mono);font-size:.74rem;letter-spacing:.06em;text-transform:uppercase;color:var(--muted);padding-top:3px}
.meas dd{margin:0}
.meas dd.exp{font-family:var(--f-mono);font-weight:600;color:var(--ok)}
.meas dd.bad{color:var(--stop)}
.meas .side{display:flex;flex-direction:column;gap:8px}
.meas svg.mtr{max-width:150px}
.cards{display:flex;flex-direction:column;gap:12px}
.dia{width:100%;height:auto;display:block;background:var(--surface);border:1px solid var(--line);border-radius:8px}
.dia text{font-family:var(--f-body);fill:var(--fg)}
.dia .mono{font-family:var(--f-mono)}
.dia .mut{fill:var(--muted)}
.dia .ln{stroke:var(--fg);fill:none}
.dia .lnm{stroke:var(--muted);fill:none}
.dia .bx{fill:var(--bg);stroke:var(--line)}
.dia .bxs{fill:var(--surface);stroke:var(--fg)}
.dia .ok{fill:var(--mask-soft);stroke:var(--ok)}
.dia .okt{fill:var(--ok)}
.dia .no{fill:var(--stop-soft);stroke:var(--stop)}
.dia .not{fill:var(--stop)}
.dia .cu{fill:var(--copper-soft);stroke:var(--copper)}
.dia .cut{fill:var(--copper)}
.dia .red{fill:#e53935;stroke:#e53935}
.dia .blk{fill:#222;stroke:#888}
.dia .gold{fill:#d4a72c}
.dia .pcb{fill:#2f4a36}
.dia .chip{fill:#26292b}
.dia .silk{fill:#f4f4f0}
.dia .tin{fill:#c7cbd1}
.dia .lcd{fill:#b9c9ad}
.dia .lcdt{fill:#1d2a1d;font-family:var(--f-mono)}
.dia .sel{fill:var(--copper);stroke:var(--copper)}
.dia .selt{fill:#fff;font-weight:700}
.scroll{overflow-x:auto;border-radius:8px}
.scroll > svg{min-width:540px}
.gal{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,210px),1fr));gap:12px}
.gal figure{background:var(--surface);border:1px solid var(--line);border-radius:8px;padding:8px}
.gal figcaption code{font-size:.8rem}
.json{display:grid;grid-template-columns:minmax(0,1.1fr) minmax(0,1fr);border:1px solid var(--line);border-radius:8px;overflow:hidden;background:var(--surface);font-size:.88rem}
.json > div{padding:6px 10px;border-bottom:1px solid var(--line)}
.json > div:nth-last-child(-n+2){border-bottom:0}
.json .k{font-family:var(--f-mono);font-size:.8rem;word-break:break-all;background:var(--mask-soft)}
.json .k b{color:var(--copper)}
@media (max-width:600px){.json{grid-template-columns:minmax(0,1fr)}.json .k{border-bottom:0}}
.term{background:#0f1411;color:#d7e3da;border-radius:8px;padding:12px 14px;font:.8rem/1.55 var(--f-mono);overflow-x:auto;white-space:pre;border:1px solid var(--line)}
.term .c{color:#ffd166}.term .g{color:#7ee0a5}.term .m{color:#8aa197}.term .r{color:#ff8a80}
.cmd{display:flex;flex-direction:column;gap:6px}
.cmd code{display:block;background:var(--surface);border:1px solid var(--line);border-radius:6px;padding:8px 10px;overflow-x:auto;white-space:nowrap}
.legend{display:flex;flex-wrap:wrap;gap:6px 14px;font-size:.85rem;color:var(--muted)}
.legend i{display:inline-block;width:12px;height:12px;border-radius:3px;margin-right:5px;vertical-align:-1px}
"""

def meter(mode, reading, red_jack="V", title=None, bad=False, w=220):
    """Front of a generic multimeter. mode in: off, vdc, vac, ohm, diode, ma, a. red_jack: V | mA | A."""
    import math
    modes = [("off", "OFF", 270), ("vdc", "V⎓", 318), ("ohm", "Ω", 6), ("diode", "→|  •)))", 52),
             ("ma", "mA", 128), ("a", "10A", 172), ("vac", "V~", 222)]
    cx, cy, R = 110, 172, 46
    out = [f'<svg class="dia mtr" viewBox="0 0 220 320" role="img" aria-label="{title or "multimeter"}" style="max-width:{w}px">']
    out.append('<rect x="6" y="6" width="208" height="308" rx="22" class="bxs" stroke-width="2"/>')
    out.append('<rect x="24" y="22" width="172" height="64" rx="6" class="lcd"/>')
    out.append(f'<text x="186" y="68" text-anchor="end" font-size="30" class="lcdt">{reading}</text>')
    out.append(f'<circle cx="{cx}" cy="{cy}" r="{R}" class="bx" stroke-width="2"/>')
    sel_ang = 270
    for key, lab, ang in modes:
        a = math.radians(ang)
        lx, ly = cx + math.cos(a) * (R + 30), cy + math.sin(a) * (R + 30) + 5
        if key == mode:
            sel_ang = ang
            tw = 12 + 9 * len(lab)
            out.append(f'<rect x="{lx - tw/2:.0f}" y="{ly - 18:.0f}" width="{tw}" height="24" rx="6" class="sel"/>')
            out.append(f'<text x="{lx:.0f}" y="{ly:.0f}" text-anchor="middle" font-size="14" class="selt">{lab}</text>')
        else:
            out.append(f'<text x="{lx:.0f}" y="{ly:.0f}" text-anchor="middle" font-size="13" class="mut">{lab}</text>')
    a = math.radians(sel_ang)
    out.append(f'<line x1="{cx}" y1="{cy}" x2="{cx + math.cos(a) * (R - 6):.0f}" y2="{cy + math.sin(a) * (R - 6):.0f}" stroke="var(--copper)" stroke-width="7" stroke-linecap="round"/>')
    out.append(f'<circle cx="{cx}" cy="{cy}" r="9" class="bxs"/>')
    jacks = [("A", "10A", 34), ("mA", "mA", 82), ("COM", "COM", 132), ("V", "VΩ→|", 182)]
    for key, lab, x in jacks:
        cls = "bx"
        if key == "COM": cls = "blk"
        elif key == red_jack: cls = "red"
        out.append(f'<circle cx="{x}" cy="282" r="11" class="{cls}" stroke-width="2"/>')
        out.append(f'<text x="{x}" y="306" text-anchor="middle" font-size="11" class="mono mut">{lab}</text>')
    if bad:
        out.append('<line x1="30" y1="30" x2="190" y2="290" stroke="var(--stop)" stroke-width="10" stroke-linecap="round" opacity=".85"/>')
        out.append('<line x1="190" y1="30" x2="30" y2="290" stroke="var(--stop)" stroke-width="10" stroke-linecap="round" opacity=".85"/>')
    out.append('</svg>')
    return "".join(out)

def tools_svgs():
    usb = '''<svg class="dia" viewBox="0 0 420 150" role="img" aria-label="The magnetic USB cable: USB-A plug at one end for the computer, a round magnetic head at the other that snaps onto the magnet piece in J3">
<rect x="14" y="52" width="70" height="46" rx="5" class="bxs" stroke-width="2"/><rect x="84" y="60" width="38" height="30" class="tin"/>
<rect x="92" y="67" width="24" height="5" class="gold"/><rect x="92" y="78" width="24" height="5" class="gold"/>
<path d="M14 75 C-10 75 -10 75 14 75" class="ln"/>
<path d="M84 75 L150 75 C190 75 200 30 250 30 C300 30 300 75 330 75" class="ln" stroke-width="6" stroke-linecap="round"/>
<rect x="330" y="56" width="64" height="38" rx="19" class="bxs" stroke-width="2"/><rect x="384" y="62" width="10" height="26" rx="3" class="cu"/>
<text x="50" y="122" text-anchor="middle" font-size="13">USB-A: into the PC</text>
<text x="362" y="122" text-anchor="middle" font-size="13">magnet head</text>
<text x="362" y="138" text-anchor="middle" font-size="12" class="mut">snaps on one way only</text>
<text x="210" y="140" text-anchor="middle" font-size="12" class="mut">Adafruit #5412 (or Yiwei MG0425-UB-60-4P)</text></svg>'''
    tweez = '''<svg class="dia" viewBox="0 0 420 150" role="img" aria-label="Tweezers: hold small parts and lift socket lids; metal tips can short neighbouring pads">
<path d="M20 60 L330 70 L395 74 L330 76 L20 82 Z" class="tin" stroke="var(--muted)"/>
<path d="M20 64 L330 72 M20 78 L330 74" class="lnm"/>
<circle cx="398" cy="74" r="12" fill="none" stroke="var(--stop)" stroke-width="2"/>
<text x="20" y="112" font-size="13">Tweezers: lift lids, steer ribbons, hold a wire on a pad.</text>
<text x="20" y="132" font-size="13" class="not">The metal tip conducts: never touch two pads at once with power on</text>
<text x="20" y="146" font-size="12" class="mut">(TP1 and TP4 are only 2.7 mm apart: that is useful in recovery, dangerous elsewhere).</text></svg>'''
    esd = '''<svg class="dia" viewBox="0 0 420 170" role="img" aria-label="Static safety: touch a grounded metal object, hold boards by the edges, keep them on the anti-static bag">
<rect x="20" y="30" width="110" height="80" rx="6" class="bx"/><text x="75" y="66" text-anchor="middle" font-size="13">PC case /</text><text x="75" y="84" text-anchor="middle" font-size="13">radiator</text>
<path d="M150 70 C170 50 190 90 210 70" class="ln" stroke-width="3"/>
<text x="180" y="50" text-anchor="middle" font-size="12">touch first</text>
<rect x="240" y="40" width="160" height="70" rx="6" class="pcb"/><rect x="232" y="34" width="176" height="82" rx="8" fill="none" stroke="var(--muted)" stroke-dasharray="4 4"/>
<circle cx="246" cy="75" r="9" class="tin"/><circle cx="394" cy="75" r="9" class="tin"/>
<text x="320" y="132" text-anchor="middle" font-size="12" class="mut">hold by the edges, on its pink/silver bag</text>
<text x="20" y="150" font-size="13">1 Touch bare grounded metal first. 2 Edges only, never the gold.</text>
<text x="20" y="166" font-size="13">3 No carpet, no fleece. 4 Ribbons and panels stay in their bags until used.</text></svg>'''
    return usb, tweez, esd

def danger_svg():
    return ('<svg class="dia" viewBox="0 0 420 200" role="img" aria-label="Danger: red lead left in the mA socket while measuring volts makes the meter a short circuit">'
            '<rect x="14" y="20" width="392" height="164" rx="10" class="no"/>'
            '<text x="30" y="50" font-size="17" font-weight="700" class="not">Never measure volts with the red lead in mA</text>'
            '<text x="30" y="78" font-size="14">In the mA socket the meter is almost a wire (a few ohms).</text>'
            '<text x="30" y="98" font-size="14">Touch 3V3 and GND like that and it shorts the supply:</text>'
            '<text x="30" y="118" font-size="14">the meter fuse blows, or the regulator / USB port is hurt.</text>'
            '<text x="30" y="148" font-size="14" font-weight="700">Rule: after every current reading, red lead back to VΩ first.</text>'
            '<text x="30" y="170" font-size="13" class="mut">A meter that suddenly reads 0 mA everywhere has a blown fuse.</text></svg>')

def mA_series_svg(cur_label, note):
    return f'''<svg class="dia" viewBox="0 0 560 210" role="img" aria-label="Measuring current: the meter goes IN the battery wire, in series, red lead in the mA socket">
<rect x="20" y="60" width="120" height="80" rx="10" class="cu"/><text x="80" y="96" text-anchor="middle" font-size="14">LiPo cell</text><text x="80" y="116" text-anchor="middle" font-size="12" class="mut">150 mAh</text>
<path d="M140 80 L230 80" stroke="#e53935" stroke-width="5"/><path d="M140 120 L420 120" stroke="#222" stroke-width="5"/>
<rect x="230" y="40" width="110" height="80" rx="10" class="bxs" stroke-width="2"/><rect x="242" y="50" width="86" height="30" rx="4" class="lcd"/>
<text x="320" y="72" text-anchor="end" font-size="18" class="lcdt">{cur_label}</text>
<circle cx="258" cy="104" r="8" class="red"/><text x="258" y="136" text-anchor="middle" font-size="11" class="mono mut">mA</text>
<circle cx="312" cy="104" r="8" class="blk"/><text x="312" y="136" text-anchor="middle" font-size="11" class="mono mut">COM</text>
<path d="M230 80 C240 80 250 90 258 100" stroke="#e53935" stroke-width="4" fill="none"/>
<path d="M312 104 C340 100 360 80 420 80" stroke="#e53935" stroke-width="4" fill="none" stroke-dasharray="7 4"/>
<rect x="420" y="50" width="120" height="100" rx="10" class="pcb"/><rect x="430" y="70" width="40" height="60" rx="3" class="silk"/>
<text x="480" y="170" text-anchor="middle" font-size="13">board J4</text>
<text x="450" y="88" text-anchor="middle" font-size="11" class="mono">+</text><text x="450" y="124" text-anchor="middle" font-size="11" class="mono">GND</text>
<text x="20" y="28" font-size="14" font-weight="700">The meter becomes part of the + wire. Nothing else changes.</text>
<text x="20" y="196" font-size="12" class="mut">{note}</text></svg>'''

def usb_meter_svg():
    return '''<svg class="dia" viewBox="0 0 560 170" role="img" aria-label="First power with a current check: computer USB port, then an inline USB power meter, then the magnetic cable, then the board">
<rect x="14" y="40" width="110" height="80" rx="8" class="bx"/><text x="69" y="76" text-anchor="middle" font-size="14">Computer</text><text x="69" y="96" text-anchor="middle" font-size="12" class="mut">USB port</text>
<path d="M124 80 L170 80" class="ln" stroke-width="4"/>
<rect x="170" y="48" width="130" height="64" rx="8" class="bxs" stroke-width="2"/><rect x="180" y="56" width="110" height="34" rx="4" class="lcd"/>
<text x="284" y="72" text-anchor="end" font-size="13" class="lcdt">5.08V</text><text x="284" y="87" text-anchor="end" font-size="13" class="lcdt">0.032A</text>
<text x="235" y="104" text-anchor="middle" font-size="11" class="mut">inline USB meter</text>
<path d="M300 80 L400 80" class="ln" stroke-width="4"/><rect x="392" y="66" width="24" height="28" rx="12" class="bxs"/>
<rect x="420" y="40" width="126" height="80" rx="8" class="pcb"/><text x="483" y="84" text-anchor="middle" font-size="13" fill="#fff">board</text>
<text x="20" y="146" font-size="13"><tspan font-weight="700" class="okt">5–60 mA</tspan> idle with nothing attached = fine.  <tspan font-weight="700" class="not">Over 150 mA = unplug now</tspan> (a short).</text>
<text x="20" y="164" font-size="12" class="mut">No USB meter? Then the fingertip heat check is your current limit: a computer port shuts off on a hard short.</text></svg>'''
