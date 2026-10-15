"""Builds the website copy of the simulator from simulator.html.

Writes ../../website/simulator/index.html: the calculator only, for an
<iframe> on the public site. The test bench is hidden, there is no API key
field, every network call is blocked, and AI SOLVE (MODE 4, =) cycles
through the three showcase demo replies. Run after make_page.py."""
import os

here = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(here, "simulator.html"), encoding="utf-8").read()


def swap(text, old, new):
    if old not in text:
        raise SystemExit("make_embed: can't find %r in simulator.html" % old[:60])
    return text.replace(old, new, 1)


# Calculator only, transparent so it sits on the site's own background.
css = """<style>
html, body { background: transparent !important; }
.top, .bench { display: none !important; }
.wrap { padding: 8px 0 !important; max-width: 420px; }
.grid { display: block !important; }
.calc { margin-inline: auto; }
</style>"""
out = swap(src, "</head>", css + "</head>")

# Public page: nothing leaves the browser.
out = swap(out, "<script>", "<script>window.fetch = () => Promise.reject(new Error('offline demo'));</script>\n<script>")

# AI SOLVE always uses the showcase demos, in turn.
out = swap(out, 'DEMOS[Number($("demoPick").value)] || DEMOS[0]',
           'DEMOS[(window.__demoTurn = ((window.__demoTurn ?? -1) + 1) % 3)]')

# Labels that belong to the test build, not the product page.
out = swap(out, "AI·CALC<small>S4</small>", "AI·CALC")
out = swap(out, '<span>2.13" E-PAPER</span>', "<span></span>")
out = swap(out, "Exam mode shortcut: while off, press SHIFT, 7, then ON",
           "Try AI Solve: press MODE, 4, then = (demo answers)")
out = swap(out, "<title>AI Calculator Simulator</title>", "<title>Try the calculator</title>")

dest = os.path.join(here, "..", "..", "website", "simulator")
os.makedirs(dest, exist_ok=True)
with open(os.path.join(dest, "index.html"), "w", encoding="utf-8") as f:
    f.write(out)
print("wrote website/simulator/index.html")
