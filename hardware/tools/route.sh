#!/bin/sh
# Autoroute with Freerouting 1.9 (run under xvfb) and import the result.
# usage: [WITHGND=1] route.sh <pcb> [max_passes]
#   WITHGND=1 routes GND too (needed for fine-pitch GND pins); otherwise GND is left to the pours.
set -e
W=/home/claude/pcb/work; T=/tmp/claude-0
PCB=$(readlink -f "${1:-/home/claude/pcb/proj/ai_calc.kicad_pcb}"); MP=${2:-30}
PRO="${PCB%.kicad_pcb}.kicad_pro"
$W/run.sh $W/export_dsn.py "$PCB" $T/ai.dsn
python3 - <<'EOF'
s=open('/tmp/claude-0/ai.dsn').read()
i=s.find('(net GND'); d=0; j=i
while True:
    c=s[j]
    if c=='(': d+=1
    elif c==')':
        d-=1
        if d==0: break
    j+=1
open('/tmp/claude-0/ai_nognd.dsn','w').write((s[:i]+s[j+1:]).replace('(class Ground GND','(class Ground'))
EOF
DSN=ai_nognd.dsn; [ "$WITHGND" = 1 ] && DSN=ai.dsn
rm -f $T/ai.ses
(cd $T && timeout ${TMO:-2400} xvfb-run -a java -jar $W/fr19.jar -de $DSN -do ai.ses -mp $MP -mt 1 -oit ${OIT:-0.05} > $T/fr_route.log 2>&1) || true
ls -la $T/ai.ses
cp "$PRO" $T/pro.bak
$W/run.sh $W/import_ses.py "$PCB" $T/ai.ses
cp $T/pro.bak "$PRO"
