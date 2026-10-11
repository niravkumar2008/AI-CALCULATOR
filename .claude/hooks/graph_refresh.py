"""Claude Code Stop hook: keep graphify-out/ current without anyone running a command.

After each Claude reply, if any project file is newer than graph.json:
  1. regenerate DESIGN_SUMMARY.md for KiCad projects whose files changed
     (graphify can't read .kicad_* itself), then
  2. start `graphify update .` in the background (AST only, no tokens, ~20 s).
Docs/images still need `/graphify --update` for semantic extraction.
"""
import os, subprocess, sys, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GRAPH = ROOT / 'graphify-out' / 'graph.json'
LOCK = ROOT / 'graphify-out' / '.refresh.lock'
SKIP = {'.git', 'graphify-out', '.pio', 'node_modules', '.mcp-backups', 'export', 'renders'}
KICAD_DIRS = [ROOT / 'hardware' / 'kicad', ROOT / 'Claude outputs' / 'ai_calc_pcb_8' / 'hardware' / 'kicad']


def newest(top, suffixes=None):
    best = 0.0
    for dirpath, dirs, files in os.walk(top):
        dirs[:] = [d for d in dirs if d not in SKIP]
        for f in files:
            if suffixes is None or f.endswith(suffixes):
                best = max(best, os.path.getmtime(os.path.join(dirpath, f)))
    return best


def main():
    if not GRAPH.exists():
        return
    if LOCK.exists() and time.time() - LOCK.stat().st_mtime < 300:  # a refresh is already running
        return
    for d in KICAD_DIRS:
        summary = d / 'DESIGN_SUMMARY.md'
        if d.exists() and newest(d, ('.kicad_sch', '.kicad_pcb')) > (summary.stat().st_mtime if summary.exists() else 0):
            subprocess.run([sys.executable, str(ROOT / 'hardware' / 'kicad' / 'kicad_summary.py'), str(d)],
                           cwd=ROOT, capture_output=True, timeout=60)
    if newest(ROOT) <= GRAPH.stat().st_mtime:
        return
    # ponytail: runs in the foreground (~20 s, only when files changed); detached children get
    # killed when the hook exits on Windows. Move to a scheduled task if the wait bothers anyone.
    LOCK.touch()
    try:
        with open(ROOT / 'graphify-out' / '.refresh.log', 'w', encoding='utf-8') as log:
            subprocess.run(['graphify', 'update', '.'], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, timeout=110)
    finally:
        LOCK.unlink(missing_ok=True)


if __name__ == '__main__':
    try:
        main()
    except Exception:
        pass  # never block or fail a Claude turn over the graph
