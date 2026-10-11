"""Firmware images for over-the-air updates (the calculator's src/ota.cpp), one per board family.

The board family is the part of the version before the first "-": "stage14-..." = the v14
e-paper board (firmware-prototype/), "v15lcd-..." = the v15 LCD board (firmware-v15-lcd/).
Each family has its own image, so both fleets can be offered an update at the same time:

  FIRMWARE_DIR/<family>/firmware.bin + firmware.json ({"version", "size", "sha256"})

written by `python admin.py firmware <firmware.bin> <version>` (the version picks the folder).
A calculator asks GET /v1/firmware with its own version in the x-firmware header; the proxy
answers "update": true when its family's published version differs, with the size and SHA-256
the calculator checks before it installs. The image itself is GET /v1/firmware/image, chosen
by the same header. One board's image is never offered or served to the other board. Both
need the device token, like every other calculator endpoint.

Older proxies kept a single image in FIRMWARE_DIR itself; it is still served to calculators of
its own family until that family gets a folder of its own.
"""
import hashlib
import json
import os
import re
import shutil

import config

IMAGE = "firmware.bin"
MANIFEST = "firmware.json"
IMAGE_PATH = "/v1/firmware/image"
MIN_IMAGE_BYTES = 100_000       # an ESP32-S3 Arduino image is well over this
MAX_IMAGE_BYTES = 0x1E0000      # the app slot size in min_spiffs.csv (1.9 MB)
FAMILY_RE = re.compile(r"^[a-z0-9]{1,20}$")
KNOWN_FAMILIES = {"stage14": "v14 e-paper (firmware-prototype)", "v15lcd": "v15 LCD (firmware-v15-lcd)"}

_cache = {}  # manifest path -> (mtimes, manifest)


def family(version):
    """The board family: the part before the first "-" ("stage14-..." = v14 e-paper, "v15lcd-..." = v15 LCD)."""
    return (version or "").strip().split("-", 1)[0]


def same_family(device_version, image_version):
    return bool(family(device_version)) and family(device_version) == family(image_version)


def _valid_family(fam):
    return bool(FAMILY_RE.match(fam or ""))


def _paths(fam, dest_dir=None):
    """(image, manifest) of one family's folder; fam None = the legacy single image."""
    d = dest_dir or config.FIRMWARE_DIR
    if fam is not None:
        d = os.path.join(d, fam)
    return os.path.join(d, IMAGE), os.path.join(d, MANIFEST)


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def publish(bin_path, version, dest_dir=None):
    """Copies the image into its family's folder and writes its manifest. Returns the manifest."""
    version = (version or "").strip()
    if not version or len(version) > 40 or any(c in version for c in ' \t"\\/'):
        raise ValueError("version must be 1-40 characters with no spaces, slashes or quotes (e.g. stage14-2026.10.06)")
    fam = family(version)
    if not _valid_family(fam):
        raise ValueError(f"board family {fam!r} (the part before the first '-') must be 1-20 lowercase letters or digits")
    size = os.path.getsize(bin_path)
    if size < MIN_IMAGE_BYTES:
        raise ValueError(f"{bin_path} is only {size} bytes: not a firmware image")
    if size > MAX_IMAGE_BYTES:
        raise ValueError(f"{bin_path} is {size} bytes: bigger than the calculator's {MAX_IMAGE_BYTES} byte app slot")
    with open(bin_path, "rb") as f:
        magic = f.read(1)
    if magic != b"\xe9":
        raise ValueError(f"{bin_path} doesn't start with the ESP32 image magic byte (0xE9)")
    image, manifest = _paths(fam, dest_dir)
    os.makedirs(os.path.dirname(image), exist_ok=True)
    tmp = image + ".tmp"
    shutil.copyfile(bin_path, tmp)
    os.replace(tmp, image)  # calculators mid-download keep the old file handle; new ones get the new image
    m = {"version": version, "size": size, "sha256": sha256_of(image)}
    tmp = manifest + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(m, f)
    os.replace(tmp, manifest)
    _cache.clear()
    return m


def _load(image, manifest):
    try:
        mtime = (os.path.getmtime(manifest), os.path.getmtime(image))
    except OSError:
        return None
    hit = _cache.get(manifest)
    if hit and hit[0] == mtime:
        return hit[1]
    try:
        with open(manifest, encoding="utf-8") as f:
            m = json.load(f)
        if os.path.getsize(image) != int(m["size"]) or len(m["sha256"]) != 64 or not m["version"]:
            return None
    except (OSError, ValueError, KeyError, TypeError):
        return None
    m = {"version": str(m["version"]), "size": int(m["size"]), "sha256": str(m["sha256"]).lower(), "file": image}
    _cache[manifest] = (mtime, m)
    return m


def current(fam=None, dest_dir=None):
    """The manifest (with "file", the image path) published for board family `fam`, or None.
    fam None: the only published image when exactly one family has one (old callers and
    calculators that send no x-firmware), else None."""
    if fam is None:
        found = published(dest_dir)
        return next(iter(found.values())) if len(found) == 1 else None
    if not _valid_family(fam):
        return None
    m = _load(*_paths(fam, dest_dir))
    if m is not None and family(m["version"]) == fam:
        return m
    if m is None and not os.path.exists(_paths(fam, dest_dir)[1]):
        legacy = _load(*_paths(None, dest_dir))  # the single image of an older proxy
        if legacy is not None and family(legacy["version"]) == fam:
            return legacy
    return None


def published(dest_dir=None):
    """{family: manifest} for every family with a valid image (admin.py firmware)."""
    d = dest_dir or config.FIRMWARE_DIR
    out = {}
    try:
        names = sorted(os.listdir(d))
    except OSError:
        return out
    for name in names:
        if _valid_family(name) and os.path.isdir(os.path.join(d, name)):
            m = current(name, dest_dir)
            if m is not None:
                out[name] = m
    legacy = _load(*_paths(None, dest_dir))
    if legacy is not None and family(legacy["version"]) not in out and _valid_family(family(legacy["version"])):
        out[family(legacy["version"])] = legacy
    return out


def decide(device_version, dest_dir=None):
    """What GET /v1/firmware answers a calculator running `device_version`. Only its own board
    family's image is ever offered; an unknown or missing version gets no update."""
    dev = (device_version or "").strip()
    fam = family(dev)
    m = current(fam, dest_dir) if fam else None
    if m is None:
        return {"update": False, "version": ""}
    return {"update": m["version"] != dev, "version": m["version"],
            "size": m["size"], "sha256": m["sha256"], "path": IMAGE_PATH}


def image_for(device_version, dest_dir=None):
    """The manifest whose image GET /v1/firmware/image serves: the calculator's own family
    (x-firmware), or, when it sends none, the only published image (else None)."""
    dev = (device_version or "").strip()
    if not dev:
        return current(None, dest_dir)
    return current(family(dev), dest_dir)
