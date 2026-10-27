#!/bin/bash
# Sonda de c68: compila tests/partida_test.c y src/partida.c con qcc (c68), los ejecuta en sQLux sobre un disco con el oraculo del
# original (tests/oraculo_partida.txt) y las misiones, y comprueba que el resultado en el QL es el mismo que en el PC.
# Uso: tests/run_ql_partida.sh   (tarda un minuto; abre una ventana del emulador)
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
D=build/ptest; rm -rf $D; mkdir -p $D
qcc -DQLTEST -Isrc -O tests/partida_test.c src/partida.c -o $D/ptest || exit 1
printf '100 MODE 4\n110 EXEC_W flp1_ptest\n' > $D/boot
# la RAM libre del QL no da para los 450 KB del oraculo completo: se dejan las cargas de mision con todos los heroes (15) o solo el enano y el hechicero (10)
python3 - <<'PY'
import re
keep = True
out = []
for l in open('tests/oraculo_partida.txt'):
    if l.startswith('ENTRADA '):
        pend = l; continue
    if l.startswith('MISION '):
        keep = int(l.split()[3]) in (15, 10)
        if keep: out += [pend, l]
        continue
    if re.match(r'(A|B|C|I|M|V|S|H[0-4]) ', l):
        if keep: out.append(l)
        continue
    keep = True
    out.append(l)
open('build/ptest/oraculo_partida_txt', 'w').write(''.join(out))
PY
cp data/mision_*_bin $D/
IMG=$D/ptest.img
qltools $IMG -fdd PTEST > /dev/null
qltools $IMG -W $D/boot $D/ptest $D/oraculo_partida_txt $D/mision_*_bin > /dev/null
qltools $IMG -x ptest 40000 > /dev/null
sed "s|build/heroquest.img|$IMG|" tmp/sqlux_hq_rapido.ini > $D/sqlux.ini
tmp/sQLux/build/sqlux -f $D/sqlux.ini -w 1x > $D/sqlux.log 2>&1 &
PID=$!
for i in $(seq 1 ${ESPERA:-120}); do
  sleep 2
  qltools $IMG -n ptest_out > $D/salida.txt 2>/dev/null
  grep -q "partida_test:" $D/salida.txt && break
done
kill $PID 2>/dev/null; wait $PID 2>/dev/null
cat $D/salida.txt
grep -q " 0 fallos" $D/salida.txt && echo "sonda c68: correcta" || { echo "sonda c68: FALLA"; exit 1; }
