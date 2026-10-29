#!/bin/bash
# Sonda de c68 para src/accion.c y la animacion de src/vista.c: compila tests/accion_test.c con qcc (con el ensamblador de gfx_asm.asm, como el juego) y lo ejecuta en
# sQLux sobre un disco con unos casos del oraculo (build/oraculo_accion.txt, de tools/hq_oraculo_accion.py) y los ficheros de datos. Comprueba que el QL da lo mismo que el PC.
# Uso: tests/run_ql_accion.sh   (CASOS="1 2 3" elige los casos; abre una ventana del emulador)
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
D=build/atest; rm -rf $D; mkdir -p $D
[ -f build/oraculo_accion.txt ] || make build/oraculo_accion.txt || exit 1
make build/gfx_asm.o > /dev/null || exit 1
qcc -DQL_ASM -DQLTEST -Isrc -Ibuild/datos -O tests/accion_test.c src/accion.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c build/gfx_asm.o -o $D/atest || exit 1
printf '100 MODE 4\n110 EXEC_W flp1_atest\n' > $D/boot
CASOS=${CASOS:-3 40 85 120 160 200 250 300} python3 - <<'PY'
import os
quiero = set(int(x) for x in os.environ['CASOS'].split())
n = 0; keep = False; out = []
for l in open('build/oraculo_accion.txt'):
    if l.startswith('CASO '):
        n += 1; keep = n in quiero
    if keep: out.append(l)
open('build/atest/oraculo_accion_txt', 'w').write(''.join(out))
PY
cp data/sprites_bin data/escenario_bin data/vista_bin data/baldosas_bin data/marca_activo_bin data/textos_es data/heroquest_[0-9]* $D/
IMG=$D/atest.img
qltools $IMG -fdd ATEST > /dev/null
qltools $IMG -W $D/boot $D/atest $D/oraculo_accion_txt $D/sprites_bin $D/escenario_bin $D/vista_bin $D/baldosas_bin $D/marca_activo_bin $D/textos_es $D/heroquest_[0-9]* > /dev/null || exit 1
qltools $IMG -x atest 60000 > /dev/null
sed "s|build/heroquest.img|$IMG|" tmp/sqlux_hq_rapido.ini > $D/sqlux.ini
tmp/sQLux/build/sqlux -f $D/sqlux.ini -w 1x > $D/sqlux.log 2>&1 &
PID=$!
for i in $(seq 1 ${ESPERA:-90}); do
  sleep 2
  qltools $IMG -n atest_out > $D/salida.txt 2>/dev/null
  grep -q "accion_test:" $D/salida.txt && break
done
kill $PID 2>/dev/null; wait $PID 2>/dev/null
cat $D/salida.txt
grep -q " 0 fallos" $D/salida.txt && echo "sonda c68 (accion): correcta" || { echo "sonda c68 (accion): FALLA"; exit 1; }
