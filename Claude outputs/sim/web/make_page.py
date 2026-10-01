"""Inlines calc.js into page.html.

Writes simulator.html (a standalone page: double-click to open) and
dist/simulator_page.html (the same content without the document skeleton,
for publishing as a Claude artifact)."""
import os

here = os.path.dirname(os.path.abspath(__file__))
page = open(os.path.join(here, "page.html"), encoding="utf-8").read()
js = open(os.path.join(here, "calc.js"), encoding="utf-8").read()
if "</script" in js.lower():
    raise SystemExit("calc.js contains </script; can't inline it")
content = page.replace("/*CALC_JS*/", js)

os.makedirs(os.path.join(here, "dist"), exist_ok=True)
with open(os.path.join(here, "dist", "simulator_page.html"), "w", encoding="utf-8") as f:
    f.write(content)
with open(os.path.join(here, "simulator.html"), "w", encoding="utf-8") as f:
    f.write('<!doctype html>\n<html lang="en"><head><meta charset="utf-8">'
            '<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">'
            '<style>body{margin:0}[hidden]{display:none!important}</style></head><body>\n'
            + content + "\n</body></html>\n")
print("wrote simulator.html and dist/simulator_page.html")
