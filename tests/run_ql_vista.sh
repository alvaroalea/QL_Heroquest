#!/bin/bash
# Sonda de c68 para src/vista.c: compila tests/vista_test.c con qcc (con el ensamblador de gfx_asm.asm, como el juego) y lo ejecuta en sQLux sobre un disco
# con un caso del oraculo (build/oraculo_vista.txt, de tools/hq_oraculo_vista.py: la RAM del QL no da para los 6 MB completos) y los ficheros de datos.
# Comprueba que el resultado en el QL es el mismo que en el PC. Uso: tests/run_ql_vista.sh   (abre una ventana del emulador)
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
D=build/vtest; rm -rf $D; mkdir -p $D
[ -f build/oraculo_vista.txt ] || make build/oraculo_vista.txt || exit 1
make build/gfx_asm.o > /dev/null || exit 1
qcc -DQL_ASM -DQLTEST -Isrc -Ibuild/datos -O tests/vista_test.c src/vista.c src/partida.c src/gfx.c src/data.c src/music.c src/qsound.c build/gfx_asm.o -o $D/vtest || exit 1
printf '100 MODE 4\n110 EXEC_W flp1_vtest\n' > $D/boot
CASOS=${CASOS:-31} python3 - <<'PY'
import os
quiero = set(int(x) for x in os.environ['CASOS'].split())
n = 0; keep = False; out = []
for l in open('build/oraculo_vista.txt'):
    if l.startswith('CASO '):
        n += 1; keep = n in quiero
    if keep: out.append(l)
open('build/vtest/oraculo_vista_txt', 'w').write(''.join(out))
PY
cp data/sprites_bin data/escenario_bin data/vista_bin data/baldosas_bin data/marca_activo_bin data/heroquest_[0-9]* $D/
IMG=$D/vtest.img
qltools $IMG -fdd VTEST > /dev/null
qltools $IMG -W $D/boot $D/vtest $D/oraculo_vista_txt $D/sprites_bin $D/escenario_bin $D/vista_bin $D/baldosas_bin $D/marca_activo_bin $D/heroquest_[0-9]* > /dev/null || exit 1
qltools $IMG -x vtest 60000 > /dev/null
sed "s|build/heroquest.img|$IMG|" tmp/sqlux_hq_rapido.ini > $D/sqlux.ini
tmp/sQLux/build/sqlux -f $D/sqlux.ini -w 1x > $D/sqlux.log 2>&1 &
PID=$!
for i in $(seq 1 ${ESPERA:-90}); do
  sleep 2
  qltools $IMG -n vtest_out > $D/salida.txt 2>/dev/null
  grep -q "vista_test:" $D/salida.txt && break
done
kill $PID 2>/dev/null; wait $PID 2>/dev/null
cat $D/salida.txt
grep -q " 0 fallos" $D/salida.txt && echo "sonda c68 (vista): correcta" || { echo "sonda c68 (vista): FALLA"; exit 1; }
