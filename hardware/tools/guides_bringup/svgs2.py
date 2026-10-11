# Board-specific inline SVG diagrams.

def good_bad(title_good, title_bad, body_good, body_bad, vb="0 0 200 120"):
    def one(t, body, ok):
        cls, tcls, mark = ("ok", "okt", "✓ GOOD") if ok else ("no", "not", "✗ WRONG")
        return (f'<figure><svg class="dia" viewBox="{vb}" role="img" aria-label="{mark}: {t}">'
                f'<rect x="2" y="2" width="196" height="116" rx="8" class="{cls}" stroke-width="2"/>{body}'
                f'<text x="10" y="20" font-size="13" font-weight="700" class="{tcls}">{mark}</text></svg>'
                f'<figcaption>{t}</figcaption></figure>')
    return one(title_good, body_good, True) + one(title_bad, body_bad, False)

def diode_body(band_right):
    bx = 128 if band_right else 64
    return ('<rect x="40" y="48" width="18" height="32" class="tin"/><rect x="142" y="48" width="18" height="32" class="tin"/>'
            '<rect x="56" y="44" width="88" height="40" rx="4" class="tin" style="filter:brightness(.82)"/>'
            f'<rect x="{bx}" y="44" width="10" height="40" fill="#555"/>'
            '<text x="168" y="70" font-size="12">J3 →</text><text x="100" y="104" text-anchor="middle" font-size="11" class="mut">D7 (grey SMF body)</text>')

def chip_body(dot_tl):
    # SOT-23-5 as on the render: 3 pins on the left, 2 on the right, pin-1 dot top-left, silk triangle beside pin 1
    g = ('<rect x="62" y="38" width="16" height="9" class="tin"/><rect x="62" y="61" width="16" height="9" class="tin"/><rect x="62" y="84" width="16" height="9" class="tin"/>'
         '<rect x="122" y="44" width="16" height="9" class="tin"/><rect x="122" y="78" width="16" height="9" class="tin"/>'
         '<rect x="76" y="32" width="48" height="66" rx="3" class="chip"/><circle cx="85" cy="41" r="4" class="silk"/>')
    if not dot_tl:
        g = f'<g transform="rotate(180 100 65)">{g}</g>'
    return (g + '<path d="M58 24 l6 9 l6 -9 z" class="silk" stroke="var(--fg)" stroke-width=".7"/>'
            '<text x="100" y="69" text-anchor="middle" font-size="11" fill="#ddd">U3</text><text x="146" y="30" font-size="10" class="mut">silk ▼ = pin 1</text>'
            f'<text x="100" y="112" text-anchor="middle" font-size="11" class="mut">{"3 pins left, dot top-left" if dot_tl else "3 pins on the right: turned"}</text>')

def esp_body(dot_ok):
    dot = (66, 92) if dot_ok else (178, 34)
    return ('<rect x="20" y="28" width="38" height="70" class="gold" opacity=".55"/>'
            '<rect x="56" y="28" width="128" height="70" rx="3" class="tin"/>'
            f'<circle cx="{dot[0]}" cy="{dot[1]}" r="5" fill="#111"/>'
            '<text x="120" y="66" text-anchor="middle" font-size="11" fill="#333">U1 can</text><text x="24" y="112" font-size="10" class="mut">antenna on the LEFT board edge</text>')

def fpc_body(opening, label):
    # opening: 'up','down','left','right' = side the ribbon enters from
    if opening in ("up", "down"):
        body = '<rect x="40" y="40" width="120" height="30" class="silk" stroke="#999"/><rect x="40" y="70" width="120" height="12" fill="#555"/>'
        pins = "".join(f'<rect x="{44 + i * 7}" y="84" width="3" height="10" class="tin"/>' for i in range(17))
        g = body + pins
        if opening == "down":
            g = f'<g transform="rotate(180 100 62)">{g}</g>'
            g += '<path d="M100 104 v-12 m-6 6 l6 -6 l6 6" class="ln" stroke-width="2"/><text x="112" y="102" font-size="10">ribbon in</text>'
        else:
            g += '<path d="M100 22 v14 m-6 -6 l6 6 l6 -6" class="ln" stroke-width="2"/><text x="112" y="30" font-size="10">ribbon in</text>'
    else:
        body = '<rect x="78" y="22" width="30" height="84" class="silk" stroke="#999"/><rect x="108" y="22" width="14" height="84" fill="#555"/>'
        pins = "".join(f'<rect x="124" y="{25 + i * 6}" width="10" height="3" class="tin"/>' for i in range(14))
        g = body + pins
        if opening == "right":
            g = f'<g transform="rotate(180 100 64)">{g}</g>'
            g += '<path d="M156 64 h-20 m6 -6 l-6 6 l6 6" class="ln" stroke-width="2"/><text x="146" y="54" font-size="10">ribbon</text>'
        else:
            g += '<path d="M50 64 h20 m-6 -6 l6 6 l-6 6" class="ln" stroke-width="2"/><text x="20" y="54" font-size="10">ribbon</text>'
    return g + f'<text x="100" y="112" text-anchor="middle" font-size="11" class="mut">{label}</text>'

def solder_gallery():
    def joint(kind):
        base = '<rect x="0" y="86" width="200" height="34" class="pcb"/><rect x="30" y="80" width="56" height="8" class="cu"/><rect x="114" y="80" width="56" height="8" class="cu"/>'
        if kind == "good":
            s = '<path d="M30 80 Q46 52 62 50 L62 80 Z" class="tin"/><path d="M114 80 Q130 52 146 50 L146 80 Z" class="tin"/>'
            s += '<rect x="60" y="44" width="84" height="22" class="chip"/>'
        elif kind == "bridge":
            s = '<path d="M30 80 Q46 52 62 50 L62 80 Z" class="tin"/><path d="M114 80 Q130 52 146 50 L146 80 Z" class="tin"/>'
            s += '<rect x="60" y="44" width="84" height="22" class="chip"/><path d="M56 80 Q100 56 150 80 Z" class="tin"/><circle cx="100" cy="72" r="14" fill="none" stroke="var(--stop)" stroke-width="2"/>'
        elif kind == "tomb":
            s = '<path d="M30 80 Q46 52 62 50 L62 80 Z" class="tin"/><g transform="rotate(-38 62 80)"><rect x="60" y="58" width="84" height="22" class="chip"/></g>'
        else:  # ball / cold
            s = '<rect x="60" y="44" width="84" height="22" class="chip"/><circle cx="46" cy="70" r="13" class="tin" style="filter:brightness(.8)"/><path d="M114 80 Q130 52 146 50 L146 80 Z" class="tin"/><circle cx="176" cy="104" r="6" class="tin"/>'
        return base + s
    items = [("good", "Good: smooth, shiny, concave fillet climbing the pin", True),
             ("bridge", "Bridge: solder joins two pins. Don't power; photo, ask", False),
             ("tomb", "Tombstone: part stood up on one end (missing joint)", False),
             ("ball", "Dull ball / loose solder ball: a joint that may not connect", False)]
    out = []
    for k, cap, ok in items:
        cls, tcls, mark = ("ok", "okt", "✓") if ok else ("no", "not", "✗")
        out.append(f'<figure><svg class="dia" viewBox="0 0 200 120" role="img" aria-label="{cap}"><rect x="2" y="2" width="196" height="116" rx="8" class="{cls}" stroke-width="2"/>{joint(k)}<text x="10" y="22" font-size="15" font-weight="700" class="{tcls}">{mark}</text></svg><figcaption>{cap}</figcaption></figure>')
    return "".join(out)

def magnet_svg():
    return '''<svg class="dia" viewBox="0 0 600 260" role="img" aria-label="Magnet piece check: the cable snapped onto the piece, the charger on, black probe on one outer leg, red probe on the other outer leg reads plus 5 volts; that leg is VBUS. Inner legs read 0 to 0.6 volts.">
<rect x="150" y="30" width="200" height="70" rx="10" class="bxs" stroke-width="2"/><text x="250" y="60" text-anchor="middle" font-size="14">#5358 magnet piece</text><text x="250" y="80" text-anchor="middle" font-size="12" class="mut">(cable snapped on the other face)</text>
<g stroke-width="7" stroke-linecap="round"><path d="M180 100 v80" stroke="#e53935"/><path d="M226 100 v80" class="lnm"/><path d="M272 100 v80" class="lnm"/><path d="M318 100 v80" stroke="#444"/></g>
<text x="180" y="206" text-anchor="middle" font-size="13" font-weight="700">VBUS</text><text x="226" y="206" text-anchor="middle" font-size="13">D−</text><text x="272" y="206" text-anchor="middle" font-size="13">D+</text><text x="318" y="206" text-anchor="middle" font-size="13" font-weight="700">GND</text>
<text x="180" y="224" text-anchor="middle" font-size="11" class="mut">→ J3 pin 1 (N, +)</text><text x="318" y="224" text-anchor="middle" font-size="11" class="mut">→ J3 pin 4 (−)</text>
<path d="M180 182 L110 240" stroke="#e53935" stroke-width="9" stroke-linecap="round"/><path d="M318 182 L390 240" stroke="#222" stroke-width="9" stroke-linecap="round"/>
<text x="60" y="254" font-size="12" class="not">red +</text><text x="400" y="254" font-size="12">black COM</text>
<rect x="420" y="40" width="166" height="140" rx="10" class="ok"/>
<text x="432" y="64" font-size="13" font-weight="700">Meter: DC volts</text>
<text x="432" y="88" font-size="13">outer ↔ outer:</text><text x="432" y="110" font-size="20" font-weight="700" class="okt">+5 V ± 0.25</text>
<text x="432" y="134" font-size="13">inner ↔ GND leg:</text><text x="432" y="156" font-size="18" font-weight="700" class="okt">0 – 0.6 V</text>
<text x="432" y="174" font-size="11" class="mut">−5 V? swap probes, not legs</text>
<text x="20" y="40" font-size="12" class="mut">1 charger on</text><text x="20" y="58" font-size="12" class="mut">2 piece snapped</text><text x="20" y="76" font-size="12" class="mut">3 legs NOT in J3</text>
</svg>'''

def usba_svg():
    return '''<svg class="dia" viewBox="0 0 600 170" role="img" aria-label="USB-A plug face: four contacts, pin 1 VBUS and pin 4 GND are the long outer ones, pins 2 D minus and 3 D plus are the inner ones">
<rect x="150" y="30" width="300" height="90" rx="6" class="tin" stroke="var(--muted)"/><rect x="170" y="70" width="260" height="34" fill="#f2f2f2" stroke="#999"/>
<g><rect x="190" y="74" width="40" height="12" class="gold"/><rect x="250" y="76" width="34" height="10" class="gold"/><rect x="316" y="76" width="34" height="10" class="gold"/><rect x="370" y="74" width="40" height="12" class="gold"/></g>
<text x="210" y="58" text-anchor="middle" font-size="13" font-weight="700">1 VBUS</text><text x="267" y="58" text-anchor="middle" font-size="13">2 D−</text><text x="333" y="58" text-anchor="middle" font-size="13">3 D+</text><text x="390" y="58" text-anchor="middle" font-size="13" font-weight="700">4 GND</text>
<text x="300" y="146" text-anchor="middle" font-size="13">Plug face, contacts up. Continuity (beep), charger UNPLUGGED:</text>
<text x="300" y="164" text-anchor="middle" font-size="12" class="mut">VBUS leg → 1 · inner leg next to VBUS → 2 · inner leg next to GND → 3 · GND leg → 4 and the shell</text></svg>'''

def battery_svg():
    return '''<svg class="dia" viewBox="0 0 600 220" role="img" aria-label="Battery plug polarity: hold the plug as it will go into J4, opening towards you; red probe into the contact that meets J4 plus (right), black into the left; read plus 3.0 to 4.2 volts">
<rect x="40" y="70" width="140" height="70" rx="10" class="cu"/><text x="110" y="102" text-anchor="middle" font-size="14">LiPo</text><text x="110" y="122" text-anchor="middle" font-size="12" class="mut">150 mAh</text>
<path d="M180 92 C230 92 240 100 290 100" stroke="#e53935" stroke-width="5" fill="none"/><path d="M180 118 C230 118 240 110 290 110" stroke="#222" stroke-width="5" fill="none"/>
<rect x="290" y="80" width="90" height="50" rx="4" class="silk" stroke="#999"/><text x="335" y="150" text-anchor="middle" font-size="12" class="mut">JST-PH plug</text>
<rect x="380" y="88" width="30" height="34" fill="#555"/><circle cx="395" cy="96" r="5" class="gold"/><circle cx="395" cy="114" r="5" class="gold"/>
<text x="420" y="100" font-size="13" font-weight="700" class="not">+ (red wire)</text><text x="420" y="122" font-size="13">−</text>
<rect x="440" y="140" width="146" height="66" rx="10" class="ok"/><text x="452" y="162" font-size="13" font-weight="700">DC volts, red on +</text><text x="452" y="190" font-size="20" font-weight="700" class="okt">+3.0 – 4.2 V</text>
<text x="20" y="30" font-size="13">Looking at J4 from the component side, opening towards you: <tspan font-weight="700">− left, + right</tspan>.</text>
<text x="20" y="50" font-size="13">Turn the plug the way it will go in, then meter it. A minus sign = reversed: swap the crimps.</text>
<text x="20" y="190" font-size="12" class="not">Probe tips must not touch each other: a LiPo shorted even briefly gets hot.</text></svg>'''

def cam_ribbon_svg():
    f = []
    for i in range(24):
        n = i + 1
        x = 40 + i * 21
        cls = "gold"
        if n in (2, 15): cls = "sel"
        f.append(f'<rect x="{x}" y="60" width="13" height="60" rx="2" class="{cls}"/>')
        if n in (1, 2, 10, 15, 23, 24):
            f.append(f'<text x="{x + 6}" y="142" text-anchor="middle" font-size="12" class="mono">{n}</text>')
    return ('<svg class="dia" viewBox="0 0 600 220" role="img" aria-label="Camera ribbon, 24 gold fingers; fingers 2 and 15 (both GND) must beep together">'
            '<rect x="28" y="50" width="520" height="80" rx="6" class="cu"/>' + "".join(f) +
            '<path d="M76 56 C140 10 330 10 361 56" class="ln" stroke-width="2" stroke-dasharray="5 4"/>'
            '<text x="220" y="26" text-anchor="middle" font-size="13" font-weight="700">2 ↔ 15 must BEEP (both GND)</text>'
            '<text x="28" y="172" font-size="13">Meter on beep. Count from the end you think is pin 1. One probe on 2, the other on 15.</text>'
            '<text x="28" y="192" font-size="13">No beep? Try 2 ↔ 10: if that beeps you counted from the far end. Neither beeps: stop, ask.</text>'
            '<text x="28" y="210" font-size="12" class="mut">Sewing pins on the probes help: the fingers are 0.5 mm apart.</text></svg>')

def lcd_diode_svg():
    f = []
    for i in range(30):
        n = 30 - i
        x = 22 + i * 19
        cls = "gold"
        if n == 20: cls = "red"
        elif n == 22: cls = "blk"
        elif 21 <= n <= 24: cls = "cu"
        elif n == 1: cls = "okt"
        f.append(f'<rect x="{x}" y="70" width="11" height="56" rx="2" class="{cls}"/>')
        if n in (30, 22, 20, 1):
            f.append(f'<text x="{x + 5}" y="146" text-anchor="middle" font-size="12" class="mono">{n}</text>')
    xr = 22 + 10 * 19 + 5; xb = 22 + 8 * 19 + 5
    return ('<svg class="dia" viewBox="0 0 600 260" role="img" aria-label="LCD tail diode test: meter on diode mode, red probe on the 11th finger from the 30 end (finger 20, LED plus), black on the 9th (finger 22, LED minus); expect about 2.4 to 2.9 volts">'
            '<rect x="12" y="60" width="576" height="76" rx="6" class="cu"/>' + "".join(f) +
            f'<path d="M{xr} 70 L{xr + 40} 18" stroke="#e53935" stroke-width="8" stroke-linecap="round"/><path d="M{xb} 70 L{xb - 40} 18" stroke="#222" stroke-width="8" stroke-linecap="round"/>'
            f'<text x="{xr + 46}" y="24" font-size="13" font-weight="700" class="not">red: 11th from the 30 end = finger 20 (LED +)</text>'
            f'<text x="{xb - 46}" y="24" text-anchor="end" font-size="13" font-weight="700">black: 9th = finger 22 (LED −)</text>'
            '<text x="16" y="52" font-size="12" class="mut">30 end</text><text x="584" y="52" text-anchor="end" font-size="12" class="okt">finger 1 end (mark it)</text>'
            '<rect x="12" y="166" width="300" height="84" rx="10" class="ok"/><text x="24" y="190" font-size="13" font-weight="700">Diode mode → reads</text>'
            '<text x="24" y="222" font-size="24" font-weight="700" class="okt">≈ 2.4 – 2.9 V</text><text x="24" y="242" font-size="12" class="mut">maybe a faint glow: then end A is the 30 end</text>'
            '<rect x="324" y="166" width="264" height="84" rx="10" class="no"/><text x="336" y="190" font-size="13" font-weight="700">"OL" / 1 / nothing</text>'
            '<text x="336" y="212" font-size="12">wrong end: count from the other end.</text><text x="336" y="230" font-size="12">Both ends nothing: swap probes once,</text><text x="336" y="246" font-size="12">then stop and ask.</text></svg>')

def pio_svg(env, folder):
    return f'''<svg class="dia" viewBox="0 0 640 230" role="img" aria-label="VS Code with PlatformIO: the blue status bar at the bottom; the tick builds, the right arrow uploads, the plug opens the serial monitor, and the env name shows which project is open">
<rect x="10" y="10" width="620" height="150" rx="8" class="bx"/>
<rect x="10" y="10" width="620" height="26" rx="8" class="bxs"/><text x="22" y="28" font-size="12" class="mono">EXPLORER: {folder}</text>
<text x="40" y="70" font-size="12" class="mono mut">platformio.ini</text><text x="40" y="90" font-size="12" class="mono mut">src/</text><text x="40" y="110" font-size="12" class="mono mut">README.md</text>
<rect x="10" y="134" width="620" height="28" fill="#1f6fd1"/>
<g fill="#fff" font-size="14"><text x="22" y="153">⌂</text><text x="50" y="153">✓</text><text x="80" y="153">→</text><rect x="110" y="141" width="10" height="13" rx="1" fill="none" stroke="#fff" stroke-width="1.5"/><path d="M107 141 h16" stroke="#fff" stroke-width="1.5"/><path d="M141 140 v5 M147 140 v5 M138 145 h12 v4 a6 6 0 0 1 -12 0 z M144 155 v4" stroke="#fff" stroke-width="1.5" fill="none"/><text x="170" y="153">&gt;_</text><text x="226" y="153" class="mono" font-size="12">env:{env} ({folder})</text></g>
<g class="ln" stroke-width="1.5"><path d="M54 162 v22"/><path d="M84 162 v40"/><path d="M144 162 v58"/><path d="M290 162 v22"/></g>
<text x="58" y="190" font-size="12">Build (check it compiles)</text>
<text x="88" y="208" font-size="13" font-weight="700" class="cut">Upload = flash the board</text>
<text x="148" y="226" font-size="12">Serial Monitor (115200)</text>
<text x="300" y="190" font-size="12">must say env:{env}</text></svg>'''

def flow_ai_svg(screen_word):
    box = lambda x, y, w, h, cls, lines: (f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" class="{cls}" stroke-width="1.5"/>' +
        "".join(f'<text x="{x + 10}" y="{y + 20 + i * 17}" font-size="{13 if i == 0 else 12}" {"font-weight=\"700\"" if i == 0 else "class=\"mut\""}>{t}</text>' for i, t in enumerate(lines)))
    arrow = lambda x1, y1, x2, y2: f'<path d="M{x1} {y1} L{x2} {y2}" class="ln" stroke-width="2" marker-end="url(#ah)"/>'
    s = ['<svg class="dia" viewBox="0 0 640 600" role="img" aria-label="AI setup flow: server online with the API key, make a device token, then either type wifi, proxy and token in the serial monitor or use the phone setup page from SETUP 6; then pair and link.">',
         '<defs><marker id="ah" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto"><path d="M0 0 L10 5 L0 10 z" fill="var(--fg)"/></marker></defs>']
    s.append(box(20, 16, 290, 74, "cu", ["1 Server online (once)", "Render/Railway/Fly: server/proxy", "ANTHROPIC_API_KEY lives ONLY here"]))
    s.append(box(330, 16, 290, 74, "cu", ["2 Token for this calculator", 'python admin.py new-device "calc 1"', "prints dt_… once; copy it"]))
    s.append(arrow(310, 53, 328, 53))
    s.append(arrow(475, 90, 475, 120)); s.append(arrow(165, 90, 165, 120))
    s.append(box(20, 122, 290, 92, "bx", ["3a PC way: Serial Monitor", "wifi  → hotspot name + password", "proxy → https://&lt;your address&gt;", "token → paste dt_…"]))
    s.append(box(330, 122, 290, 92, "bx", ["3b Phone way: SETUP 6", "keys: SHIFT MODE, 6, =  (serial: setup)", f"{screen_word} shows SETUP BY PHONE", "network AI-Calc-xxxx + 8-char password"]))
    s.append(arrow(475, 214, 475, 236))
    s.append(box(330, 238, 290, 60, "bx", ["Phone: join AI-Calc-xxxx", "the form pops up (or open 192.168.4.1)"]))
    s.append(arrow(475, 298, 475, 318))
    # phone form mock
    s.append('<rect x="360" y="320" width="230" height="200" rx="18" class="bxs" stroke-width="2"/><text x="375" y="344" font-size="13" font-weight="700">AI Calculator setup</text>')
    fields = ["Phone hotspot (2.4 GHz) ▾", "Hotspot password", "AI server address https://", "Device token dt_…"]
    for i, t in enumerate(fields):
        y = 354 + i * 34
        s.append(f'<rect x="375" y="{y}" width="200" height="26" rx="5" class="bx"/><text x="383" y="{y + 18}" font-size="11" class="mut">{t}</text>')
    s.append('<rect x="375" y="492" width="200" height="22" rx="6" fill="var(--mask)"/><text x="475" y="507" text-anchor="middle" font-size="12" fill="#fff" font-weight="700">Save to the calculator</text>')
    s.append('<text x="360" y="540" font-size="11" class="mut">Empty field = keep the old value. AC cancels;</text><text x="360" y="555" font-size="11" class="mut">it closes by itself after 10 minutes.</text>')
    s.append(arrow(165, 214, 165, 330)); s.append(arrow(360, 420, 312, 380))
    s.append(box(20, 332, 290, 76, "ok", ["4 status shows it all", "Wi-Fi: \"your hotspot\" …", "AI proxy: https://…  Device token: dt_ab…9f"]))
    s.append(arrow(165, 408, 165, 430))
    s.append(box(20, 432, 290, 76, "ok", ["5 pair → 6-letter code", "phone: https://&lt;address&gt;/link", "code + e-mail: free month starts"]))
    s.append(arrow(165, 508, 165, 530))
    s.append(box(20, 532, 290, 56, "ok", ["6 account shows the plan", "then MODE 4 (AI SOLVE), point, ="]))
    s.append('</svg>')
    return "".join(s)

def flowchart_svg(v15):
    scr = "LCD" if v15 else "e-paper"
    rows = [
        ("Plugged in (PC port). Anything hot or smelly?", True, "UNPLUG. Let it cool. Re-inspect that part's turn and bridges (step 2). Don't re-power until found."),
        ("VBUS at J3 pin 1 ≈ 5 V?", False, "≈ 0 V or negative: magnet piece backwards / bad cable. Unplug, redo step 3 (pin 4 leg beeps to TP4)."),
        ("TP5 = 3.25–3.35 V?", False, "0 V: measure SYS at D1 (4.55–4.7): missing = D1; present = U3. Over 3.6 V: unplug, redo the inner-leg checks."),
        ("COM port appears on the PC?", False, "Press ON (deep sleep hides it). Then recovery: TP1↔TP4 held, tap TP7. Still none: D−/D+ legs swapped? (step 3)."),
        ("Stays up? (no restart loop in the monitor)", False, "Boot loop: read the 'Last restart' / reset reason. Wrong project flashed? Re-flash the right folder. TP5 dips when it restarts = supply."),
        ("Stays up on battery during Wi-Fi / AI?", False, "Brownout: status says 'Last restart: brownout'. Charge the cell; test cell sag (QA B2); lower Wi-Fi power; send the number."),
        (f"{'Colour bars right' if v15 else 'display_ok true, checkerboard clean'}?", False,
         "Cable off, reseat the tail in J5 (finger-1 dot at the tick); redo the diode test; colour faults = platformio.ini flags (step 9)." if v15 else
         "Cable off, reseat J2 fully, lid flat. Refresh 0.3–9 s. < 0.3 s = BUSY never seen (not latched); > 9 s = booster L1/D3–D5."),
        ("camera_ok true?", False, "Cable off. Redo the 2↔15 beep, reseat J1 straight; then one try with a 180° twist. 'OV5640' missing = ribbon backwards."),
        ("keypad_scanner_ok true, 50/50 keys?", False, "Scanner false: U6 bridge / lifted pin, I²C pull-ups (QA A3 #25). One key missing: mat or pad dirty. A whole row/column: a U6 pin."),
    ]
    H = 30 + len(rows) * 96 + 40
    s = [f'<svg class="dia" viewBox="0 0 680 {H}" role="img" aria-label="Troubleshooting flowchart: answer each question top to bottom; a no (or a yes on the first) sends you to the box on the right.">',
         '<defs><marker id="ah2" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto"><path d="M0 0 L10 5 L0 10 z" fill="var(--fg)"/></marker></defs>']
    import textwrap
    for i, (q, yes_bad, fix) in enumerate(rows):
        y = 20 + i * 96
        s.append(f'<path d="M24 {y + 34} L170 {y} L316 {y + 34} L170 {y + 68} Z" class="cu" stroke-width="1.5"/>')
        ql = textwrap.wrap(q, 26)
        for j, t in enumerate(ql):
            s.append(f'<text x="170" y="{y + 34 - (len(ql) - 1) * 8 + j * 16 + 4}" text-anchor="middle" font-size="12.5" font-weight="700">{t}</text>')
        s.append(f'<path d="M316 {y + 34} L352 {y + 34}" class="ln" stroke-width="2" marker-end="url(#ah2)"/>')
        s.append(f'<text x="322" y="{y + 28}" font-size="11" font-weight="700" class="not">{"YES" if yes_bad else "NO"}</text>')
        fl = textwrap.wrap(fix, 44)
        s.append(f'<rect x="354" y="{y + 2}" width="314" height="{max(64, 14 + 16 * len(fl))}" rx="8" class="no"/>')
        for j, t in enumerate(fl):
            s.append(f'<text x="364" y="{y + 20 + j * 16}" font-size="12">{t}</text>')
        if i < len(rows) - 1:
            s.append(f'<path d="M170 {y + 68} L170 {y + 94}" class="ln" stroke-width="2" marker-end="url(#ah2)"/>')
            s.append(f'<text x="178" y="{y + 86}" font-size="11" font-weight="700" class="okt">{"NO" if yes_bad else "YES"}</text>')
    y = 20 + len(rows) * 96
    s.append(f'<rect x="60" y="{y - 4}" width="220" height="34" rx="17" class="ok"/><text x="170" y="{y + 18}" text-anchor="middle" font-size="13" font-weight="700" class="okt">All yes: bring-up passed</text>')
    s.append('</svg>')
    return "".join(s)
