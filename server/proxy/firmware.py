"""Firmware images for over-the-air updates (the calculator's firmware-prototype/src/ota.cpp).

One image at a time, in FIRMWARE_DIR: `firmware.bin` and `firmware.json` ({"version", "size",
"sha256"}), written by `python admin.py firmware <firmware.bin> <version>`. The calculator asks
GET /v1/firmware with its own version in the x-firmware header; the proxy answers "update": true
when the stored version differs, with the size and SHA-256 the calculator checks before it
installs. The image itself is GET /v1/firmware/image. Both need the device token, like every
other calculator endpoint, so only registered calculators can fetch firmware.
"""
import hashlib
import json
import os
import shutil

import config

IMAGE = "firmware.bin"
MANIFEST = "firmware.json"
IMAGE_PATH = "/v1/firmware/image"
MIN_IMAGE_BYTES = 100_000       # an ESP32-S3 Arduino image is well over this
MAX_IMAGE_BYTES = 0x1E0000      # the app slot size in min_spiffs.csv (1.9 MB)

_cache = {"mtime": None, "manifest": None}


def _paths(dest_dir=None):
    d = dest_dir or config.FIRMWARE_DIR
    return os.path.join(d, IMAGE), os.path.join(d, MANIFEST)


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def publish(bin_path, version, dest_dir=None):
    """Copies the image into FIRMWARE_DIR and writes its manifest. Returns the manifest."""
    version = (version or "").strip()
    if not version or len(version) > 40 or any(c in version for c in ' \t"\\'):
        raise ValueError("version must be 1-40 characters with no spaces or quotes (e.g. stage14-2026.10.06)")
    size = os.path.getsize(bin_path)
    if size < MIN_IMAGE_BYTES:
        raise ValueError(f"{bin_path} is only {size} bytes: not a firmware image")
    if size > MAX_IMAGE_BYTES:
        raise ValueError(f"{bin_path} is {size} bytes: bigger than the calculator's {MAX_IMAGE_BYTES} byte app slot")
    with open(bin_path, "rb") as f:
        magic = f.read(1)
    if magic != b"\xe9":
        raise ValueError(f"{bin_path} doesn't start with the ESP32 image magic byte (0xE9)")
    image, manifest = _paths(dest_dir)
    os.makedirs(os.path.dirname(image), exist_ok=True)
    tmp = image + ".tmp"
    shutil.copyfile(bin_path, tmp)
    os.replace(tmp, image)  # calculators mid-download keep the old file handle; new ones get the new image
    m = {"version": version, "size": size, "sha256": sha256_of(image)}
    with open(manifest, "w", encoding="utf-8") as f:
        json.dump(m, f)
    _cache["mtime"] = None
    return m


def current(dest_dir=None):
    """The published manifest (with "path" to the image), or None when nothing is published."""
    image, manifest = _paths(dest_dir)
    try:
        mtime = (os.path.getmtime(manifest), os.path.getmtime(image))
    except OSError:
        return None
    if dest_dir is None and _cache["mtime"] == mtime:
        return _cache["manifest"]
    try:
        with open(manifest, encoding="utf-8") as f:
            m = json.load(f)
        if os.path.getsize(image) != int(m["size"]) or len(m["sha256"]) != 64 or not m["version"]:
            return None
    except (OSError, ValueError, KeyError, TypeError):
        return None
    m = {"version": str(m["version"]), "size": int(m["size"]), "sha256": str(m["sha256"]).lower(), "file": image}
    if dest_dir is None:
        _cache["mtime"], _cache["manifest"] = mtime, m
    return m


def family(version):
    """The board family: the part before the first "-" ("stage14-..." = v14 e-paper, "v15lcd-..." = v15 LCD)."""
    return (version or "").strip().split("-", 1)[0]


def same_family(device_version, image_version):
    return bool(family(device_version)) and family(device_version) == family(image_version)


def decide(device_version, dest_dir=None):
    """What GET /v1/firmware answers a calculator running `device_version`."""
    m = current(dest_dir)
    if m is None:
        return {"update": False, "version": ""}
    dev = (device_version or "").strip()
    # The board family is the part before the first "-" ("stage14-…" = v14 e-paper,
    # "v15lcd-…" = v15 LCD). Never offer one board's image to the other.
    if not same_family(dev, m["version"]):
        return {"update": False, "version": m["version"]}
    return {"update": m["version"] != dev, "version": m["version"],
            "size": m["size"], "sha256": m["sha256"], "path": IMAGE_PATH}
