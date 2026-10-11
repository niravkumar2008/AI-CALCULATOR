"""Checks hardware/pins_final.h against the schematic, so firmware and board can't drift apart.

Exports a netlist from a temporary COPY of hardware/kicad (the design files are never touched),
reads which net each ESP32 (U1) pad is on, and compares it with the [NET] named in the comment of
every `constexpr int PIN_x = N;  // [NET] ...` line in pins_final.h.

Usage (from the repo root):  python firmware-prototype/tools/check_pins.py [path/to/kicad-cli]
Exit code 0 = every pin matches.
"""
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KICAD_DIR = os.path.join(ROOT, "hardware", "kicad")
PINS = os.path.join(ROOT, "hardware", "pins_final.h")


def find_kicad_cli():
    if len(sys.argv) > 1:
        return sys.argv[1]
    found = shutil.which("kicad-cli")
    if found:
        return found
    for c in sorted(glob.glob(r"C:\Program Files\KiCad\*\bin\kicad-cli.exe"), reverse=True):
        return c
    sys.exit("kicad-cli not found: pass its path as the first argument")


def export_netlist(cli):
    tmp = tempfile.mkdtemp(prefix="pins_check_")
    for pat in ("*.kicad_sch", "*.kicad_sym", "*.kicad_pro", "*-lib-table"):
        for f in glob.glob(os.path.join(KICAD_DIR, pat)):
            shutil.copy(f, tmp)
    out = os.path.join(tmp, "net.net")
    subprocess.run([cli, "sch", "export", "netlist", "--format", "kicadsexpr", "-o", out,
                    os.path.join(tmp, "ai_calc.kicad_sch")], check=True, capture_output=True)
    text = open(out, encoding="utf-8").read()
    shutil.rmtree(tmp, ignore_errors=True)
    return text


def u1_nets(net_text):
    """GPIO number -> net name for every U1 IOxx pad (plus 'USB_D-'/'USB_D+' pads as 19/20)."""
    gpio = {}
    nets = net_text[net_text.find("(nets"):]
    for block in re.split(r"\(net\s+\(code", nets)[1:]:
        name = re.search(r'\(name\s+"([^"]*)"\)', block).group(1)
        for ref, _pin, func in re.findall(
                r'\(node\s+\(ref\s+"([^"]+)"\)\s+\(pin\s+"([^"]+)"\)(?:\s+\(pinfunction\s+"([^"]*)"\))?', block):
            if ref != "U1" or not func:
                continue
            m = re.match(r"IO(\d+)_", func)
            if m:
                gpio[int(m.group(1))] = name
            elif func.startswith("USB_D-"):
                gpio[19] = name
            elif func.startswith("USB_D+"):
                gpio[20] = name
            elif func.startswith("TXD0"):
                gpio[43] = name
            elif func.startswith("RXD0"):
                gpio[44] = name
    return gpio


def header_pins():
    """(constant, gpio, net) for each PIN_ line whose comment names a [NET]."""
    out = []
    for line in open(PINS, encoding="utf-8"):
        m = re.match(r"\s*constexpr int (PIN_\w+)\s*=\s*(-?\d+);\s*//\s*\[([^\]]+)\]", line)
        if m:
            out.append((m.group(1), int(m.group(2)), m.group(3)))
    return out


def main():
    gpio = u1_nets(export_netlist(find_kicad_cli()))
    bad = 0
    for const, pin, net in header_pins():
        actual = gpio.get(pin)
        ok = actual == net
        bad += not ok
        print(f"{'ok ' if ok else 'BAD'} {const:16s} IO{pin:<3d} header [{net}]  schematic [{actual}]")
    used = {p for _, p, _ in header_pins()}
    for p, net in sorted(gpio.items()):
        if p not in used and not net.startswith("unconnected") and net not in ("BOOT", "UART_TX", "UART_RX"):
            print(f"BAD IO{p} carries [{net}] in the schematic but has no PIN_ line in pins_final.h")
            bad += 1
    print("All pins match the schematic." if not bad else f"{bad} mismatch(es)!")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
