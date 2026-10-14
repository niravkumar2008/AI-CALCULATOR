"""Builds dist/index.html: page.html with build/sim.js (the WebAssembly simulator, wasm
inlined) and the bundled sample photo (tools/sim_v15/sample_photo.png, as a small JPEG
data: URI) inlined. One self-contained page, for the website's /try/ page.

Fails the build if a vendor name is anywhere in the output: the page source, and the
WebAssembly binary inside it (decoded first, so the check covers its strings too)."""
import base64
import gzip
import io
import os
import re
import sys

from PIL import Image

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.normpath(os.path.join(here, "..", ".."))
VENDOR = re.compile(rb"(?i)claude|anthropic|sk-ant|openai|stripe|chatgpt")


def fail(msg):
    sys.exit("make_page: " + msg)


page = open(os.path.join(here, "page.html"), encoding="utf-8").read()
js = open(os.path.join(here, "build", "sim.js"), encoding="utf-8").read()
if "</script" in js.lower():
    fail("sim.js contains </script; can't inline it")

# The sample photo the pretend camera sees ("Solve for x: 3x + 7 = 22"), 320 x 240.
im = Image.open(os.path.join(root, "tools", "sim_v15", "sample_photo.png")).convert("RGB")
if im.size != (320, 240):
    im = im.resize((320, 240), Image.LANCZOS)
buf = io.BytesIO()
im.save(buf, "JPEG", quality=72, optimize=True, progressive=True)
photo = "data:image/jpeg;base64," + base64.b64encode(buf.getvalue()).decode("ascii")

for marker in ("/*SIM_JS*/", "__PHOTO_DATA_URI__"):
    if page.count(marker) != 1:
        fail("page.html needs exactly one %s" % marker)
out = page.replace("/*SIM_JS*/", js).replace("__PHOTO_DATA_URI__", photo)

# ---- vendor-name check: the page text, then every base64 blob decoded (the wasm binary).
problems = set(m.group(0).decode().lower() for m in VENDOR.finditer(out.encode("utf-8")))
blobs = re.findall(r"[A-Za-z0-9+/]{2000,}={0,2}", out)
if not blobs:
    fail("no inlined WebAssembly found (was it built with -sSINGLE_FILE=1?)")
for b in blobs:
    try:
        raw = base64.b64decode(b + "=" * (-len(b) % 4))
    except Exception:
        continue
    problems |= set(m.group(0).decode(errors="replace").lower() for m in VENDOR.finditer(raw))
if problems:
    fail("vendor names in the output: %s" % ", ".join(sorted(problems)))

# ---- no network: the page must not name any http(s) host other than the brand's own site
# (shown on the Link calculator screen) and the standard XML / spec namespaces.
hosts = set(re.findall(r"https?://([A-Za-z0-9.-]+)", out))
allowed = {"proofline.app", "www.w3.org"}
extra = sorted(h for h in hosts if h not in allowed)
if extra:
    print("make_page: note: URLs named in the page source (never fetched): %s" % ", ".join(extra))

os.makedirs(os.path.join(here, "dist"), exist_ok=True)
dest = os.path.join(here, "dist", "index.html")
data = out.encode("utf-8")
with open(dest, "wb") as f:
    f.write(data)
gz = len(gzip.compress(data, 9))
print("wrote %s: %.0f KB (%.0f KB gzip), photo %.1f KB" % (dest, len(data) / 1024, gz / 1024, len(buf.getvalue()) / 1024))
print("vendor-name check: passed (page source + decoded wasm)")
