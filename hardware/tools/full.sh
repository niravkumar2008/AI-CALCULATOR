#!/bin/sh
# full board flow: place -> check -> autoroute -> GND pours/vias -> cleanup -> leftovers -> DRC
set -e
W=/home/claude/pcb/work; P=/home/claude/pcb/proj; T=/tmp/claude-0
$W/run.sh $W/build_board.py
cp $P/ai_calc.kicad_pro $T/pro.keep
$W/run.sh $W/placement_check.py $P/ai_calc.kicad_pcb /home/claude/pcb/render/place.png | head -5
cp $P/ai_calc.kicad_pro $T/pro.keep
$W/run.sh $W/prepass.py $P/ai_calc.kicad_pcb
cp $T/pro.keep $P/ai_calc.kicad_pro
WITHGND=1 $W/route.sh $P/ai_calc.kicad_pcb ${1:-30} | tail -1
drc() { cp $T/pro.keep $P/ai_calc.kicad_pro; (cd $P && kicad-cli pcb drc --severity-error -o $T/drc.rpt ai_calc.kicad_pcb >/dev/null); }
$W/run.sh $W/add_gnd.py $P/ai_calc.kicad_pcb | tail -3
drc
for i in 1 2; do $W/run.sh $W/add_gnd.py $P/ai_calc.kicad_pcb $T/drc.rpt | tail -2; drc; done
$W/run.sh $W/cleanup.py $P/ai_calc.kicad_pcb | tail -2
drc
if grep -q unconnected_items $T/drc.rpt; then FR_GRID=0.05 $W/run.sh $W/fixroute.py $P/ai_calc.kicad_pcb $T/drc.rpt; drc; fi
$W/run.sh $W/check_keys.py $P/ai_calc.kicad_pcb
grep -E '^\[' $T/drc.rpt | sed 's/:.*//' | sort | uniq -c
echo FLOW-DONE
