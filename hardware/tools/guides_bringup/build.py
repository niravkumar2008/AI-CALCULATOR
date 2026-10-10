# Injects the visual material into both First Power-Up guides. Run on the ORIGINAL files (orig_v14/15.html).
import os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from svgs import CSS, meter, tools_svgs, danger_svg, mA_series_svg, usb_meter_svg
from svgs2 import *

SCR = os.path.dirname(os.path.abspath(__file__))
HW = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware"

def card(img, title, mode, reading, red_jack, mode_txt, red, black, expect, wrong, do, alt):
    m = meter(mode, reading, red_jack, title=f"meter set to {mode_txt}", w=150)
    return f'''<div class="meas">
  <div class="side"><a href="{img}" target="_blank" rel="noopener"><img src="{img}" alt="{alt}" loading="lazy" width="620"></a><span class="eyebrow">tap to enlarge</span></div>
  <div><h4>{title}</h4>
  <div style="display:grid;grid-template-columns:minmax(0,1fr) 120px;gap:10px;align-items:start">
  <dl>
    <dt>Mode</dt><dd>{mode_txt}</dd>
    <dt>Red</dt><dd>{red}</dd>
    <dt>Black</dt><dd>{black}</dd>
    <dt>Expect</dt><dd class="exp">{expect}</dd>
    <dt>Wrong</dt><dd class="bad">{wrong}</dd>
    <dt>Do</dt><dd>{do}</dd>
  </dl>{m}</div></div>
</div>'''

def insert_before(html, anchor, block, nth=1):
    i = -1
    for _ in range(nth):
        i = html.index(anchor, i + 1)
    return html[:i] + block + "\n" + html[i:]

def insert_after(html, anchor, block):
    i = html.index(anchor) + len(anchor)
    return html[:i] + "\n" + block + html[i:]

def build(v):
    v15 = v == "15"
    R = f"renders_bringup/v{v}/"
    src = open(os.path.join(SCR, f"orig_v{v}.html"), encoding="utf8").read()
    h = src if src.startswith("<meta charset") else '<meta charset="utf-8">' + chr(10) + '<meta name="viewport" content="width=device-width,initial-scale=1">' + chr(10) + src
    scr = "LCD" if v15 else "e-paper"
    sock = "J5" if v15 else "J2"

    # ---------------------------------------------------------------- CSS
    h = h.replace("</style>", CSS + "</style>", 1)

    # ---------------------------------------------------------------- PRIMER A: tools
    usb, tweez, esd = tools_svgs()
    tools = f'''<section id="tools" class="primer">
  <h2><span class="n">A</span>Meet your tools</h2>
  <p>Ten minutes here saves a board later. You only need four meter settings all day. Your meter's dial may look different; look for the same <strong>symbols</strong>, not the same positions.</p>
  <div class="gal">
    <figure>{meter("vdc", "3.30", "V", "meter on DC volts")}<figcaption><strong>DC volts (V\u2393 or V with a straight line).</strong> Red lead in <strong>V\u03a9</strong>, black in <strong>COM</strong>. Used for every rail: VBUS, SYS, 3V3, EN, BAT. Pick the 20 V range if it isn't auto-ranging. The meter is just "looking": it can't hurt anything in this mode.</figcaption></figure>
    <figure>{meter("ohm", "1.2k", "V", "meter on resistance")}<figcaption><strong>Resistance (\u03a9).</strong> Same sockets. <strong>Board unpowered only.</strong> Used before first power to prove 3V3 and BAT aren't shorted to GND. The number climbs for a few seconds while the capacitors charge: wait 5 s, then read.</figcaption></figure>
    <figure>{meter("diode", "2.62", "V", "meter on diode / beep")}<figcaption><strong>Diode / beep (\u2192| and \u2022))).</strong> Same sockets, unpowered. <strong>Beep</strong> = the two points are connected (under ~30 \u03a9). <strong>Diode</strong> shows the voltage a diode or LED needs ({"the LCD backlight test, step 5" if v15 else "used in the QA plan"}). "OL" or "1" = open, no connection.</figcaption></figure>
    <figure>{meter("ma", "50.2", "mA", "meter on milliamps, red lead in the mA socket")}<figcaption><strong>Current (mA).</strong> The <strong>red lead moves to the mA socket</strong>, and the meter goes <em>in</em> the wire (in series), never across two points. Only for the optional charge and sleep-current steps.</figcaption></figure>
  </div>
  {danger_svg()}
  <h3>How the leads go in, every time</h3>
  <div class="tablewrap"><table>
    <thead><tr><th>Measuring</th><th>Dial</th><th>Black lead</th><th>Red lead</th></tr></thead>
    <tbody>
      <tr><td>Volts (a rail)</td><td>V\u2393, 20 V</td><td>COM \u2192 TP4 GND</td><td>V\u03a9 \u2192 the point named in the step</td></tr>
      <tr><td>Short check / beep / diode</td><td>\u03a9 or \u2192|\u2022)))</td><td>COM</td><td>V\u03a9</td></tr>
      <tr><td>Current</td><td>mA</td><td>COM</td><td><strong>mA socket</strong>, then straight back to V\u03a9 afterwards</td></tr>
    </tbody></table></div>
  <h3>The other tools</h3>
  <div class="gal">
    <figure>{usb}<figcaption>The magnetic cable is also the data cable: flashing and the Serial Monitor go through it. A <strong>USB-C</strong> PC port needs a USB-A\u2192C adapter; any data-capable one works.</figcaption></figure>
    <figure>{tweez}<figcaption>Fine-tip tweezers, plastic-tipped if you have them for the socket lids.</figcaption></figure>
    <figure>{esd}<figcaption>Static (ESD) can kill the ESP32 or the {scr} without any visible mark. <span class="unsure">general practice</span></figcaption></figure>
  </div>
  <ul class="check" data-step="tools">
    <li><input type="checkbox" id="t1"><label for="t1">Meter check: set it to beep and touch the probe tips together. It beeps. Set DC volts: it reads 0.00. (Dead battery in the meter = strange readings all day.)</label></li>
    <li><input type="checkbox" id="t2"><label for="t2">Sewing pins pushed onto the probe tips for the 0.5 mm ribbon fingers; a strip of tape so the two pins can't touch.</label></li>
  </ul>
</section>
'''
    # ---------------------------------------------------------------- PRIMER B: board map
    bmap = f'''<section id="map" class="primer">
  <h2><span class="n">B</span>Board map</h2>
  <p>Every name used below, on the real board (3D render of the {"v15-LCD" if v15 else "v14"} files). Yellow tags = test pads, blue = sockets, green = chips, red = parts whose turn matters most. Hold the board the same way: <strong>component side up, magnet header (J3) at the top</strong>. Tap a picture to open it full size.</p>
  <div class="figs">
    <figure><a href="{R}map_top.jpg" target="_blank" rel="noopener"><img src="{R}map_top.jpg" alt="Labelled render of the component side: test pads TP1 to TP7, J1 camera, {sock} {scr}, J3 magnet header with pins VBUS, D minus, D plus, GND, J4 battery, U1 ESP32, U2 charger, U3 regulator, U7 USB protection, U6 keypad chip" loading="lazy" width="1000" height="2033"></a><figcaption>Component side. TP4 (GND) is at the left edge below the ESP32, next to TP1.</figcaption></figure>
    <figure><a href="{R}map_bottom.jpg" target="_blank" rel="noopener"><img src="{R}map_bottom.jpg" alt="Render of the key side with the gold key pads" loading="lazy" width="700" height="1423"></a><figcaption>Key side, seen from below (so left and right are swapped). Nothing to measure here; keep the gold pads clean.</figcaption></figure>
  </div>
  <h3>Zoomed in</h3>
  <div class="figs">
    <figure><a href="{R}zoom_power.jpg" target="_blank" rel="noopener"><img src="{R}zoom_power.jpg" alt="Zoom A: power corner with J3 pins 1 to 4 labelled VBUS, D minus, D plus, GND; D7 with its band on the right; U3, U2, D1, J4 pin 1 GND and pin 2 plus; Q1, Q2, U7; TP5, TP6, TP7, TP2, TP3" loading="lazy" width="900" height="753"></a><figcaption><strong>A \u00b7 Power corner.</strong> J3 pin 1 (VBUS) is the left pin, beside the silk N and +. J4: pin 1 GND left, pin 2 + right.</figcaption></figure>
    <figure><a href="{R}zoom_esp32.jpg" target="_blank" rel="noopener"><img src="{R}zoom_esp32.jpg" alt="Zoom B: the ESP32 module U1 with its pin-1 dot at the lower left, and the pads TP1 BOOT, TP4 GND, TP7 EN, TP5 3V3" loading="lazy" width="700" height="952"></a><figcaption><strong>B \u00b7 ESP32 and the recovery pads.</strong> TP1 and TP4 sit 2.7 mm apart.</figcaption></figure>
    <figure><a href="{R}zoom_screen.jpg" target="_blank" rel="noopener"><img src="{R}zoom_screen.jpg" alt="Zoom D: the {scr} socket {sock}" loading="lazy" width="800"></a><figcaption><strong>D \u00b7 {scr} socket {sock}.</strong> {"Pad 1 is the bottom end at the silk tick; Q5 and R23 above, Q4 and C19 below; the long slot on the right." if v15 else "Contacts on the right, opening on the left; pad 1 at the top."}</figcaption></figure>
    <figure><a href="{R}zoom_camera.jpg" target="_blank" rel="noopener"><img src="{R}zoom_camera.jpg" alt="Zoom C: camera socket J1 with its camera regulators U4 and U5" loading="lazy" width="900" height="519"></a><figcaption><strong>C \u00b7 Camera socket J1</strong>, near the bottom of the board, with its two small regulators.</figcaption></figure>
  </div>
  <div class="tablewrap"><table>
    <thead><tr><th>Pad / part</th><th>What it is</th><th>You use it for</th></tr></thead>
    <tbody>
      <tr><td><span class="m">TP4 GND</span></td><td>Ground, 0 V</td><td>Black probe, every voltage reading</td></tr>
      <tr><td><span class="m">TP5 3V3</span></td><td>The 3.3 V rail from U3</td><td>Short check, first-power reading</td></tr>
      <tr><td><span class="m">TP6 BAT</span></td><td>Battery + after Q1</td><td>Short check, battery reading</td></tr>
      <tr><td><span class="m">TP7 EN</span></td><td>ESP32 enable (reset when pulled to GND)</td><td>Reading; the "tap" in recovery</td></tr>
      <tr><td><span class="m">TP1 BOOT</span></td><td>GPIO0; held low at reset = download mode</td><td>Recovery</td></tr>
      <tr><td><span class="m">TP2 TX</span> <span class="m">TP3 RX</span></td><td>Spare serial port</td><td>Last-resort flashing with a 3.3 V USB-serial adapter</td></tr>
      <tr><td>J3</td><td>Magnet header: 1 VBUS (N end), 2 D\u2212, 3 D+, 4 GND</td><td>Power and USB data</td></tr>
      <tr><td>J4</td><td>Battery socket: 1 GND (left), 2 + (right)</td><td>The LiPo, after step 4</td></tr>
      <tr><td>U1 / U2 / U3 / U7</td><td>ESP32-S3 / LiPo charger / 3.3 V regulator / USB protection</td><td>Heat check at first power</td></tr>
      <tr><td>J1 / {sock}</td><td>Camera socket / {scr} socket</td><td>Ribbons, step 9</td></tr>
    </tbody></table></div>
</section>
'''
    h = insert_before(h, '<section id="inspect">', tools + bmap)

    # ---------------------------------------------------------------- inspection gallery
    fpc2 = fpc_body("right", "J5: lid left, opening faces the slot") if v15 else fpc_body("left", "J2: opening faces left (the slot)")
    fpc2b = fpc_body("left", "turned round: opening away from the slot") if v15 else fpc_body("right", "turned round: opening faces right")
    gal = f'''<h3>What good and wrong look like (loupe checklist)</h3>
  <p>Use a 10\u00d7 loupe or the phone camera at 2\u20133\u00d7 zoom with the torch on. Compare each part with the drawings, then with zoom A / B / C / D in the board map. JLCPCB also e-mails placement photos: compare those too.</p>
  <div class="gal">
    {good_bad("D7: grey body, dark band on the RIGHT (towards J3)", "D7 band on the left: VBUS meets the wrong end", diode_body(True), diode_body(False))}
    {good_bad("U2 / U3 / U4 / U5: dot top-left, 3 pins on the left", "Dot at the other corner: chip turned 180\u00b0", chip_body(True), chip_body(False))}
    {good_bad("U1 ESP32: the dot sits at the lower left of the can (antenna on the left edge)", "Dot elsewhere: the module is rotated", esp_body(True), esp_body(False))}
    {good_bad("J1 camera: opening faces up, contacts along the bottom", "J1 turned round: contacts on top", fpc_body("up", "J1 as on the render"), fpc_body("down", "turned 180\u00b0"))}
    {good_bad(("J5" if v15 else "J2") + " as on the render", "Socket turned round", fpc2, fpc2b)}
  </div>
  <div class="gal">{solder_gallery()}</div>
  <p class="note">Real renders of these parts are in the board map zooms A\u2013D. A wrongly turned part on a JLCPCB-assembled board is rare, but D7, the ICs and the two sockets are the ones where it would matter.</p>
'''
    h = insert_before(h, '<ul class="check" data-step="inspect">', gal)

    short_cards = f'''<h3>The short check, with pictures (board unpowered)</h3>
  <p>These four readings catch the faults that could hurt the board or the PC on first power (QA plan A3 #16, 17, 19, 20). Nothing is plugged in, no battery.</p>
  <div class="cards">
  {card(R+"m_short_3v3.jpg", "TP5 3V3 to TP4 GND: not shorted", "ohm", "1.2k", "V", "\u03a9 (resistance), or beep", "TP5 3V3", "TP4 GND", "&gt; 1 k\u03a9 after 5 s (climbing at first is normal). No steady beep.", "&lt; 100 \u03a9 or a steady beep = short: bridged U3 or a cracked 22 \u00b5F capacitor.", "Don't power it. Photograph U3 and the caps near TP5; claim with JLCPCB.", "Red probe on TP5, black probe on TP4, meter on ohms")}
  {card(R+"m_short_bat.jpg", "TP6 BAT to TP4 GND", "ohm", "1.9M", "V", "\u03a9 (resistance)", "TP6 BAT", "TP4 GND", "&gt; 10 k\u03a9 (often around 1\u20132 M\u03a9: the battery-sense divider)", "Near 0 \u03a9 = short on the battery side.", "Don't power or fit a battery. Photo of Q1, J4, U2; ask.", "Red probe on TP6, black on TP4, ohms")}
  {card(R+"m_beep_j3gnd.jpg", "J3 pin 4 is GND", "diode", "0.00", "V", "beep (continuity)", "J3 pin 4 (the \u2212 end)", "TP4 GND", "BEEPS", "No beep = pin 4 not soldered, or you are on pin 1.", "Recount the pins from the N end; look at the J3 joints.", "Red probe on J3 pin 4, black on TP4, beep mode")}
  {card(R+"m_beep_j3vbus.jpg", "J3 pin 1 is NOT GND", "diode", "OL", "V", "beep (continuity)", "J3 pin 1 (the N / + end)", "TP4 GND", "NO beep (a short chirp is fine)", "Steady beep = VBUS shorted to GND (D7 or U7 bridged / reversed).", "Don't power. Photo of D7 and U7; ask.", "Red probe on J3 pin 1, black on TP4, beep mode")}
  </div>
  <ul class="check" data-step="inspect">
    <li><input type="checkbox" id="i9"><label for="i9">TP6 \u2194 TP4 above 10 k\u03a9, J3 pin 4 beeps to TP4, J3 pin 1 does not.</label></li>
  </ul>
'''
    h = insert_after(h, "<p class=\"stop\">If a part is turned the wrong way or bridged: don't power it. Photograph it and ask.</p>", short_cards)

    # ---------------------------------------------------------------- magnet, battery, camera ribbon
    h = insert_before(h, '<ul class="check" data-step="magnet">', f'<figure>{magnet_svg()}<figcaption>Step g2\u2013g3: the piece snapped onto the cable, charger on, legs in the air. Mark the VBUS leg with a pen.</figcaption></figure>\n  <figure>{usba_svg()}<figcaption>Step g4: which USB-A contact each leg must beep to.</figcaption></figure>')
    h = insert_before(h, '<ul class="check" data-step="battery">', f'<figure>{battery_svg()}<figcaption>Polarity before anything else. On the board, J4 + is pin 2, the right-hand one (zoom A).</figcaption></figure>')
    h = insert_before(h, '<ul class="check" data-step="ribbon">', f'<figure>{cam_ribbon_svg()}<figcaption>Camera ribbon, gold side up. Orange = the two GND fingers.</figcaption></figure>', 1)
    if v15:
        h = insert_before(h, '<p class="note"><strong>Where the dot must end up:</strong>', f'<figure>{lcd_diode_svg()}<figcaption>Step l3\u2013l4 on one picture. The meter in diode mode pushes about 1 mA: enough for a faint glow, never enough to harm the LEDs. <span class="unsure">2.4\u20132.9 V: typical white LED at low current, not from the datasheet</span></figcaption></figure>\n  <div class="gal"><figure>{meter("diode", "2.62", "V", "meter on diode mode")}<figcaption>Diode mode: red in V\u03a9, black in COM.</figcaption></figure></div>')

    # ---------------------------------------------------------------- first power
    cur = f'''<h3>Current limit: watch the current while you plug in</h3>
  <figure>{usb_meter_svg()}<figcaption>An inline USB power meter (about $10) between the PC and the cable shows the board's current at once (QA plan A4 #26). <span class="unsure">limit set in the QA plan, not a measured value</span></figcaption></figure>
'''
    h = insert_before(h, '<ul class="check" data-step="power">', cur)
    pc = f'''<h3>Each reading, with a picture</h3>
  <p>Black probe stays on <span class="m">TP4 GND</span> for all five (put a sewing pin on it and tape the lead down so it can't slide). Meter on DC volts.</p>
  <div class="cards">
  {card(R+"m_vbus.jpg", "VBUS: power from the cable", "vdc", "5.06", "V", "DC volts, 20 V", "the J3 pin 1 leg (N / + end)", "TP4 GND", "\u2248 5.0 V (4.75\u20135.25)", "Near 0 or negative: magnet piece backwards. Below 4.75: weak port or cable.", "Negative or 0: unplug at once, redo step 3. Low: try another PC port.", "Red probe on J3 pin 1, black on TP4, DC volts")}
  {card(R+"m_sys.jpg", "SYS: after diode D1", "vdc", "4.63", "V", "DC volts, 20 V", "D1's band end (left pad)", "TP4 GND", "4.55\u20134.7 V (VBUS minus about 0.35 V)", "0 V with good VBUS: D1 open or not soldered. Equal to VBUS: you are on the wrong pad.", "Unplug if anything is warm; photo D1; ask.", "Red probe on the left pad of D1, black on TP4")}
  {card(R+"m_3v3.jpg", "+3V3: the main rail", "vdc", "3.30", "V", "DC volts, 20 V", "TP5 3V3", "TP4 GND", "3.25\u20133.35 V", "0 V: U3 not running (or no SYS). Under 3.2 V: overloaded. <strong>Above 3.6 V: unplug now.</strong>", "0 V: measure SYS. Above 3.6 V: unplug, redo the step 3 inner-leg checks, ask.", "Red probe on TP5, black on TP4")}
  {card(R+"m_en.jpg", "EN: the chip is allowed to run", "vdc", "3.29", "V", "DC volts, 20 V", "TP7 EN", "TP4 GND", "\u2248 3.3 V (above 2.5 V)", "Low (under 1 V): chip held in reset.", "Look for a bridge or solder ball near TP7 and the EN capacitor; ask.", "Red probe on TP7, black on TP4")}
  {card(R+"m_bat.jpg", "BAT: battery pin, no cell yet", "vdc", "0.02", "V", "DC volts, 20 V", "TP6 BAT", "TP4 GND", "\u2248 0 V, or 4.1\u20134.25 V (both normal with no cell)", "1\u20134 V, or above 4.3 V.", "Note the value and ask before fitting a battery.", "Red probe on TP6, black on TP4")}
  </div>
'''
    h = insert_before(h, '<p class="stop"><strong>Wrong reading or hot part:</strong>', pc)

    # ---------------------------------------------------------------- flashing
    folder = "firmware-v15-lcd" if v15 else "firmware-prototype"
    env = "v15lcd" if v15 else "prototype"
    ver = "v15lcd-2026.10.10" if v15 else "stage14-2026.10.10"
    term_extra = '\n<span class="m">Screen: 320x170 LCD ok, brightness 60 % (now 60 %), dim after 20 s, ...</span>' if v15 else ""
    flash = f'''<h3>What it looks like</h3>
  <figure>{pio_svg(env, folder)}<figcaption>The PlatformIO buttons live in the blue bar at the bottom of VS Code. If the bar doesn't show <code>env:{env}</code>, the wrong folder is open.</figcaption></figure>
  <div class="cmd">
    <p><strong>Or type it</strong> (VS Code: <em>Terminal &gt; New Terminal</em>, from the repo folder):</p>
    <code>cd {folder}</code>
    <code>%USERPROFILE%\\.platformio\\penv\\Scripts\\pio run -t upload</code>
    <code>%USERPROFILE%\\.platformio\\penv\\Scripts\\pio device monitor -b 115200</code>
  </div>
  <p>A good upload ends with <code>Hard resetting via RTS pin...</code> and <code>[SUCCESS]</code>. Then the Serial Monitor; type in the box at the bottom and press Enter:</p>
  <pre class="term"><span class="c">&gt; status</span>
<span class="g">Firmware: {ver} (slot app0)  id calc-3f9a21</span>      <span class="m">&lt;- right build, first USB flash</span>
Camera: none
Wi-Fi: not set (type wifi, or = on SETUP 6)          <span class="m">&lt;- normal until step {"11" if v15 else "10"}</span>
AI proxy: not set (type proxy)
Device token: not set (type token)
Battery: 0% (0 mV)                                     <span class="m">&lt;- no cell yet</span>{term_extra}</pre>
  <p class="eyebrow">Example; the id and some numbers differ on your board.</p>
'''
    h = insert_before(h, '<h3>If no COM port appears, or "no serial data"</h3>', flash)
    rec = f'''<div class="figs">
    <figure><img src="{R}recovery_1.jpg" alt="Panel 1: a wire held from TP1 BOOT to TP4 GND" loading="lazy" width="460" height="762"><figcaption>1 \u00b7 Hold TP1 to TP4 (a wire, or the tweezers' tip across both pads).</figcaption></figure>
    <figure><img src="{R}recovery_2.jpg" alt="Panel 2: keep TP1 held, briefly tap a second wire from TP7 EN to TP4 GND" loading="lazy" width="460" height="762"><figcaption>2 \u00b7 Keep holding; tap TP7 to TP4 for about 0.2 s. The chip restarts in download mode.</figcaption></figure>
    <figure><img src="{R}recovery_3.jpg" alt="Panel 3: both wires off, click Upload; afterwards tap TP7 to TP4 once" loading="lazy" width="460" height="762"><figcaption>3 \u00b7 Let go of TP1, click Upload. When it finishes, tap TP7 once more to start the new firmware.</figcaption></figure>
  </div>
  <p class="note">In download mode the Serial Monitor shows <code>waiting for download</code> and the COM port number may change. Nothing you do on these pads can damage the board: they are pulled up through resistors. <strong>Never touch TP5 (3V3) to TP4.</strong></p>
'''
    h = insert_before(h, '<p class="note">With a battery connected later', rec)

    # ---------------------------------------------------------------- self-test annotated JSON
    if v15:
        fields = [("firmware", '"v15lcd-2026.10.10"', "The build that is running. Must start with v15lcd."),
                  ("firmware_slot", '"app0"', "app0 after a USB flash; app1 after the first over-the-air update."),
                  ("device_id", '"calc-\u2026"', "This board's id. Write it on the bag."),
                  ("reset_reason", '"\u2026"', "Why it last restarted. 'brownout' = power dipped."),
                  ("memory_ok", "true", "4 MB flash + 2 MB PSRAM found. False = wrong module or bad U1 joint."),
                  ("keypad_scanner_ok", "true", "U6 answers on I\u00b2C. False = U6 bridge or pull-ups."),
                  ("battery_mv", "3987", "Cell voltage. 3000\u20134300 = ok. 0 with a cable = Q1/J4."),
                  ("vbus / charge", 'true / "charging"', "Cable seen; charger running. No cell: may say full."),
                  ("lcd_ok / lcd_push_ms", "true / 24", "A full frame went out in under 200 ms. Does NOT prove the panel shows it: look at the bars."),
                  ("backlight_ok / backlight_pct", "true / 60", "The PWM sweep ran and returned to 60 %. Your eyes check the light."),
                  ("camera_ok / camera_jpeg_bytes", "true / 31877", "OV5640 found and a JPEG over 1000 bytes taken. False with no camera: expected."),
                  ("camera_autofocus", "true", "Autofocus firmware loaded: the right (AF) camera."),
                  ("wifi_networks / wifi_best_rssi", "7 / -48", "At least one network better than \u221280 dBm."),
                  ("keys_ok / keys_missing", "50 / []", "Every key pressed. 'q' skips: then false, fine on the bench."),
                  ("pass", "true", "Everything above. On the bare board it is false: that's fine here.")]
    else:
        fields = [("firmware", '"stage14-2026.10.10"', "The build that is running."),
                  ("firmware_slot", '"app0"', "app0 after a USB flash; app1 after the first over-the-air update."),
                  ("device_id", '"calc-\u2026"', "This board's id. Write it on the bag."),
                  ("reset_reason", '"\u2026"', "Why it last restarted. 'brownout' = power dipped."),
                  ("memory_ok", "true", "4 MB flash + 2 MB PSRAM found. False = wrong module or bad U1 joint."),
                  ("keypad_scanner_ok", "true", "U6 answers on I\u00b2C. False = U6 bridge or pull-ups."),
                  ("battery_mv", "3987", "Cell voltage. 3000\u20134300 = ok. About 4200 with no cell and a cable = charger idling."),
                  ("vbus / charge", 'true / "charging"', "Cable seen; charger running."),
                  ("display_refresh_ms / display_ok", "2140 / true", "E-paper refresh 300\u20139000 ms (typ. 1800\u20132500). < 300 = BUSY never seen; > 9000 = booster."),
                  ("camera_ok / camera_jpeg_bytes", "true / 31877", "OV5640 found and a JPEG over 1000 bytes taken."),
                  ("camera_autofocus", "true", "Autofocus firmware loaded: the right (AF) camera."),
                  ("wifi_networks / wifi_best_rssi", "7 / -48", "At least one network better than \u221280 dBm."),
                  ("keys_ok / keys_missing", "50 / []", "Every key pressed. 'q' skips: then false, fine on the bench."),
                  ("pass", "true", "Everything above. On the bare board it is false: that's fine here.")]
    js = "".join(f'<div class="k"><b>{k}</b>: {val}</div><div>{note}</div>' for k, val, note in fields)
    st = f'''  <h3>The JSON line, field by field</h3>
  <p>The line is long; this is the same example split up. Left: the field and a good value. Right: what it tells you. Paste your real line into a text file and go down this list.</p>
  <div class="json">{js}</div>
'''
    h = insert_before(h, '</section>\n\n<section id="attach">', st)

    # ---------------------------------------------------------------- attach: current measurements, colour bars
    sleep = f'''<h3>Optional current readings (meter in mA / \u00b5A)</h3>
  <figure>{mA_series_svg("50.2 mA" if not v15 else "0.035 mA", "Needs a JST-PH extension lead with the red wire cut and both ends stripped, or a breakout. Red lead back to V\u03a9 afterwards! <-- general practice")}<figcaption>Charge current: about <span class="m">50 mA</span> mid-charge, about 5 mA below ~3 V, tapering near full (R2 = 20 k on both boards). Sleep current (no cable, switched off, after 10 s): <span class="m">\u2264 40 \u00b5A</span>; use the \u00b5A range if your meter has one. <span class="unsure">lead method: general practice</span></figcaption></figure>
'''
    sleep = sleep.replace(" <-- general practice", "")
    if v15:
        h = insert_before(h, '</section>\n\n<section id="backlight">', sleep)
        sims = [("lcd_good.png", "All good: white, yellow, cyan, green, magenta, red, blue, black; smooth grey ramp; full border.", "nothing"),
                ("lcd_rb_swap.png", "Red and blue swapped (and yellow/cyan): the bar second from the right is red.", "-DLCD_RGB_ORDER=1"),
                ("lcd_inverted.png", "Negative colours: the white bar is black, the ramp runs backwards.", "-DLCD_INVERT=0"),
                ("lcd_rotated.png", "Upside down: black bar on the left, text at the bottom.", "-DLCD_ROTATION=3"),
                ("lcd_offset.png", "Noise strip on one edge, border cut off: memory offset.", "none yet: note the edge and width, ask (PORT_NOTES 1)"),
                ("lcd_sparkle.png", "Sparkles, torn rows: SPI too fast.", "LCD_SPI_HZ 26 or 20 MHz in pins_v15_lcd.h"),
                ("lcd_glow_only.png", "Backlight on, no picture: tail reversed, not fully in, or lid open.", "no flag: cable off, redo the diode test, reseat"),
                ("lcd_dark.png", "Totally dark, even during the backlight sweep.", "no flag: reseat; then the R23 reading (step 10)")]
        g = "".join(f'<figure><img src="{R}{f}" alt="{c}" loading="lazy" width="360" height="244"><figcaption>{c}<br>Fix: <code>{fx}</code></figcaption></figure>' for f, c, fx in sims)
        bars = f'''<h3>The colour bars: pictures of each symptom</h3>
  <p>Simulated pictures of the self-test screen. Find the one that looks like yours. The flags are in <code>firmware-v15-lcd/platformio.ini</code> under <code>build_flags</code>; change one value, Upload, look again. <span class="unsure">simulated, not photos of a real panel</span></p>
  <div class="gal">{g}</div>
  <pre>build_flags =
  ...
  -DLCD_ROTATION=1        ; 3 = upside down
  -DLCD_INVERT=1          ; 0 if the colours look negative
  -DLCD_RGB_ORDER=0       ; 1 if red and blue are swapped
  -DPREVIEW_SWAP_BYTES=0  ; 1 if only the live preview has wrong colours</pre>
'''
        h = insert_before(h, '<div class="tablewrap">\n    <table>\n      <thead><tr><th>What you see</th>', bars)
        bl = f'''<div class="cards">
  {card(R+"m_r23.jpg", "Backlight current: volts across R23", "vdc", "392m", "V", "DC volts, 2 V (or mV) range", "R23's right end (the +3V3 side)", "R23's left end (the backlight side) \u2014 not TP4 this time", "240\u2013560 mV = 16\u201337 mA (\u00f7 15 \u03a9). 26 mA typical = 390 mV", "Under 150 mV (10 mA): dim. Negative: probes swapped. 0: backlight off (check bright 100, screen dim 0).", "Write it down with TP5. Example: 440 mV \u00f7 15 = 29 mA.", "Red probe on the right pad of R23, black on its left pad")}
  {card(R+"m_c19.jpg", "LCD supply: C19 (only if the screen is dark)", "vdc", "3.29", "V", "DC volts, 20 V", "C19's top end (LCD_VDD), beside Q4", "TP4 GND", "\u2248 3.3 V while the screen is on", "0 V: Q4 not switching on (LCD_PWR_N, R21) or the screen is in sleep.", "Press a key to wake the screen, measure again; still 0: ask.", "Red probe on C19's top pad")}
  </div>
'''
        h = insert_before(h, '<p class="note">Rough guide: under 10 mA', bl)
        h = insert_before(h, '<p class="note">Screen messages:', f'<figure><img src="{R}preview_mock.png" alt="Mock-up of the live preview: FOCUSED in the top band, corner brackets, an amber box around the printed equation, a centre cross and the frame rate in the bottom band" loading="lazy" width="700" height="486"><figcaption>What step w1\u2013w3 should look like (a drawing, not a screenshot). <span class="unsure">layout from vf_lcd.cpp / README, fps an estimate</span></figcaption></figure>')
    else:
        h = insert_before(h, '</section>\n\n<section id="ai">', sleep)

    # ---------------------------------------------------------------- AI flow
    flow = f'<figure>{flow_ai_svg(scr)}<figcaption>The whole AI setup on one page. The phone form\'s four fields are exactly the ones in the firmware\'s setup page; the token starts with <code>dt_</code>, the device id (for <code>revoke</code>) with <code>dev_</code>.</figcaption></figure>'
    h = insert_before(h, '<ul class="check" data-step="ai">', flow, 1)

    # ---------------------------------------------------------------- troubleshooting flowchart
    fc = f'''  <p>Start at the top and stop at the first "wrong" answer. The table below has the details.</p>
  <div class="scroll">{flowchart_svg(v15)}</div>
'''
    m = re.search(r'<section id="trouble">\n  <h2>.*?</h2>\n', h)
    h = h[:m.end()] + fc + h[m.end():]

    # ---------------------------------------------------------------- footer
    h = h.replace("<br>Linked:", f"<br>Pictures: <code>hardware/renders_bringup/v{v}/</code>, made from the KiCad files with <code>kicad-cli pcb render</code> and labelled from the footprint positions; diagrams are drawings, the LCD symptom pictures are simulations.<br>Linked:", 1)
    out = os.path.join(HW, "bringup_guide_v15_lcd.html" if v15 else "bringup_guide.html")
    nl = "\r\n" if b"\r\n" in open(os.path.join(SCR, f"orig_v{v}.html"), "rb").read() else "\n"
    open(out, "w", encoding="utf8", newline=nl).write(h)
    print(out, len(src), "->", len(h))

if __name__ == "__main__":
    build("14"); build("15")
