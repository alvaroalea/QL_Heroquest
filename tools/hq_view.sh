#!/bin/bash
# Uso: hq_view.sh INICIO FIN  -> desensamblado lineal con marca 'x' en las instrucciones ejecutadas (cobertura)
R=$(cd "$(dirname "$0")/.." && pwd)
"$R/tmp/venv/bin/sna2skool.py" -H -c 0 -o 0x4000 -s $1 -e $2 "$R/asset/extraido/main48.bin" 2>/dev/null | grep -E '^[c* ]' | sed 's/ *;.*$//' | \
awk -v cov="$R/asset/trace/cov.txt" 'BEGIN{while((getline l < cov)>0){split(l,q," ");if(q[1]<=0)c[toupper(q[2])]=1}} {ad=(substr($1,1,1)=="c")?substr($1,3,4):substr($1,2,4); printf "%s %s\n",(ad in c)?"x":" ",$0}'
