#!/bin/sh
# Stage 7: JLCPCB manufacturing files from the KiCad project.
# usage: make_outputs.sh <project_dir> <out_dir>
set -e
PY=${PY:-python3}  # on Windows: PY="/c/Program Files/KiCad/10.0/bin/python.exe", kicad-cli on PATH
P=${1:-/home/claude/pcb/proj}; O=${2:-/home/claude/pcb/out/hardware/fab}
rm -rf "$O"; mkdir -p "$O/gerbers"
PCB="$P/ai_calc.kicad_pcb"; SCH="$P/ai_calc.kicad_sch"

kicad-cli pcb export gerbers --layers F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts \
  --subtract-soldermask --use-drill-file-origin -o "$O/gerbers/" "$PCB" >/dev/null
kicad-cli pcb export drill --format excellon --excellon-units mm --excellon-separate-th \
  --generate-map --map-format gerberx2 -o "$O/gerbers/" "$PCB" >/dev/null
(cd "$O/gerbers" && "$PY" -m zipfile -c ../ai_calc_gerbers_JLCPCB.zip *)

# pick-and-place (JLCPCB wants: Designator, Mid X, Mid Y, Layer, Rotation)
kicad-cli pcb export pos --format csv --units mm --side front --exclude-dnp -o "$O/pos_raw.csv" "$PCB" >/dev/null
"$PY" - "$O" <<'EOF'
import csv, sys
O = sys.argv[1]
rows = list(csv.DictReader(open(f'{O}/pos_raw.csv')))
with open(f'{O}/ai_calc_CPL_JLCPCB.csv', 'w', newline='') as f:
    w = csv.writer(f); w.writerow(['Designator', 'Mid X', 'Mid Y', 'Layer', 'Rotation'])
    for r in rows:
        if r['Ref'].startswith(('SW', 'TP', 'H')):
            continue
        w.writerow([r['Ref'], r['PosX'] + 'mm', r['PosY'] + 'mm', 'Top' if r['Side'] == 'top' else 'Bottom', r['Rot']])
print('CPL rows', len(rows))
EOF
rm -f "$O/pos_raw.csv"

kicad-cli sch export bom --fields 'Value,Reference,Footprint,LCSC' --labels 'Comment,Designator,Footprint,LCSC Part #' \
  --group-by 'LCSC,Value,Footprint' --exclude-dnp -o "$O/ai_calc_BOM_JLCPCB.csv" "$SCH" >/dev/null

kicad-cli sch export pdf -o "$O/ai_calc_schematic.pdf" "$SCH" >/dev/null
kicad-cli pcb export pdf --layers F.Cu,F.Silkscreen,F.Fab,Edge.Cuts --mode-single -o "$O/ai_calc_assembly_top.pdf" "$PCB" >/dev/null || true
kicad-cli pcb export step --subst-models --no-dnp -o "$O/ai_calc_board.step" "$PCB" >/dev/null 2>&1 || true
kicad-cli pcb render --side top --width 1600 --height 2400 -o "$O/render_top.png" "$PCB" >/dev/null 2>&1 || true
kicad-cli pcb render --side bottom --width 1600 --height 2400 -o "$O/render_bottom.png" "$PCB" >/dev/null 2>&1 || true
ls -la "$O"
