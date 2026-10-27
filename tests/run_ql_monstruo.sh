#!/bin/bash
# Sonda de c68 para src/monstruo.c: compila tests/monstruo_test.c con qcc (con el ensamblador de gfx_asm.asm, como el juego) y lo ejecuta en
# sQLux sobre un disco con unos casos del oraculo (build/oraculo_monstruo.txt, de tools/hq_oraculo_monstruo.py) y los ficheros de datos. Comprueba que el QL da lo mismo que el PC.
# Uso: tests/run_ql_monstruo.sh   (CASOS="1 2 3" elige los casos; abre una ventana del emulador)
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
D=build/mtest; rm -rf $D; mkdir -p $D
[ -f build/oraculo_monstruo.txt ] || make build/oraculo_monstruo.txt || exit 1
make build/gfx_asm.o > /dev/null || exit 1
qcc -DQL_ASM -DQLTEST -Isrc -Ibuild/datos -O tests/monstruo_test.c src/monstruo.c src/combate.c src/accion.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c build/gfx_asm.o -o $D/mtest || exit 1
printf '100 MODE 4\n110 EXEC_W flp1_mtest\n' > $D/boot
CASOS=${CASOS:-2 8 40 100 150 200} python3 - <<'PY'
import os
quiero = set(int(x) for x in os.environ['CASOS'].split())
n = 0; keep = False; out = []
for l in open('build/oraculo_monstruo.txt'):
    if l.startswith('CASO '):
        n += 1; keep = n in quiero
    if keep: out.append(l)
open('build/mtest/oraculo_monstruo_txt', 'w').write(''.join(out))
PY
cp data/sprites_bin data/escenario_bin data/vista_bin data/baldosas_bin data/marca_activo_bin data/textos_bin data/mision_*_bin $D/
IMG=$D/mtest.img
qltools $IMG -fdd ATEST > /dev/null
qltools $IMG -W $D/boot $D/mtest $D/oraculo_monstruo_txt $D/sprites_bin $D/escenario_bin $D/vista_bin $D/baldosas_bin $D/marca_activo_bin $D/textos_bin $D/mision_*_bin > /dev/null || exit 1
qltools $IMG -x mtest 60000 > /dev/null
sed "s|build/heroquest.img|$IMG|" tmp/sqlux_hq_rapido.ini > $D/sqlux.ini
tmp/sQLux/build/sqlux -f $D/sqlux.ini -w 1x > $D/sqlux.log 2>&1 &
PID=$!
for i in $(seq 1 ${ESPERA:-90}); do
  sleep 2
  qltools $IMG -n mtest_out > $D/salida.txt 2>/dev/null
  grep -q "monstruo_test:" $D/salida.txt && break
done
kill $PID 2>/dev/null; wait $PID 2>/dev/null
cat $D/salida.txt
grep -q " 0 fallos" $D/salida.txt && echo "sonda c68 (monstruo): correcta" || { echo "sonda c68 (monstruo): FALLA"; exit 1; }
