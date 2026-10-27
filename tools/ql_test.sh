#!/bin/bash
# Prueba automatica en sQLux: tmp/ql_test.sh "tokens" ...   (D F U L R E  = K O... teclas del QL; Sxx = captura; Rh2 = R mantenida 2 segundos)
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
OUT=${OUT:-build/ql_test}; mkdir -p $OUT; rm -f $OUT/*.bin $OUT/*.png $OUT/keys.txt; touch $OUT/keys.txt
export SQLUX_DUMP=$R/$OUT/dump.bin SQLUX_KEYS=$R/$OUT/keys.txt
tmp/sQLux/build/sqlux -f ${INI:-tmp/sqlux_hq.ini} -w 1x > $OUT/sqlux.log 2>&1 &
PID=$!
sleep ${BOOT_WAIT:-6}
declare -A CODE=( [U]=0x17 [D]=0x22 [L]=0x29 [R]=0x03 [F]=0x36 [E]=0x33 [0]=0x0d [1]=0x1b [2]=0x09 [3]=0x19 )
for t in $*; do
  case $t in
    S*) sleep ${SNAP_WAIT:-0.5}; kill -USR1 $PID; sleep 0.5; mv $OUT/dump.bin $OUT/${t#S}.bin 2>/dev/null ;;
    .) sleep 0.5 ;;
    ?h*) k=${t:0:1}; echo "d ${CODE[$k]}" > $OUT/keys.txt; kill -USR2 $PID; sleep ${t:2}
       echo "u ${CODE[$k]}" > $OUT/keys.txt; kill -USR2 $PID; sleep 0.4 ;;
    *) echo "d ${CODE[$t]}" > $OUT/keys.txt; kill -USR2 $PID; sleep 0.25
       echo "u ${CODE[$t]}" > $OUT/keys.txt; kill -USR2 $PID; sleep 0.4 ;;
  esac
done
kill $PID 2>/dev/null; wait $PID 2>/dev/null
for f in $OUT/*.bin; do [ -f "$f" ] && tmp/venv/bin/python -I tools/ql2png.py $f ${f%.bin}.png 2; done
ls $OUT
