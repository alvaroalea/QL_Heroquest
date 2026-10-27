#!/bin/bash
# Ejecuta la version de PC con un guion de teclas y deja las capturas como PNG en build/host/jugar/ (para verlas). Uso: tools/host_jugar.sh "F F R ... S0 ... E"
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
D=build/host/jugar; rm -rf $D; mkdir -p $D
export HQ_QSOUND=1 HQ_DATA=$R/data/
(cd $D && HQ_KEYS="$1" $R/build/host/hq > /dev/null 2>&1)
for f in $D/screen_*.bin; do [ -f "$f" ] && tmp/venv/bin/python -I tools/ql2png.py $f ${f%.bin}.png 1; done
ls $D/*.png
