#!/bin/bash
# Sonda de c68 para src/mision.c: compila tests/mision_test.c con qcc (con el ensamblador de gfx_asm.asm, como el juego) y lo ejecuta en
# sQLux sobre un disco con unos casos del oraculo (build/oraculo_mision.txt, de tools/hq_oraculo_mision.py) y los ficheros de datos. Comprueba que el QL da lo mismo que el PC.
# Uso: tests/run_ql_mision.sh   (CASOS="1 2 3" elige los casos; abre una ventana del emulador)
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
D=build/itest; rm -rf $D; mkdir -p $D
[ -f build/oraculo_mision.txt ] || make build/oraculo_mision.txt || exit 1
make build/gfx_asm.o build/datos/vent_addr.h > /dev/null || exit 1
qcc -DQL_ASM -DQLTEST -Isrc -Ibuild/datos -O tests/mision_test.c src/mision.c src/combate.c src/accion.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c build/gfx_asm.o -o $D/itest || exit 1
printf '100 MODE 4\n110 EXEC_W flp1_itest\n' > $D/boot
CASOS=${CASOS:-5 130 331 470} python3 - <<'PY'
import os
quiero = set(int(x) for x in os.environ['CASOS'].split())
n = 0; keep = False; out = []
for l in open('build/oraculo_mision.txt'):
    if l.startswith('CASO '):
        n += 1; keep = n in quiero
    if keep: out.append(l)
open('build/itest/oraculo_mision_txt', 'w').write(''.join(out))
PY
cp data/sprites_bin data/escenario_bin data/vista_bin data/baldosas_bin data/marca_activo_bin data/textos_bin data/mision_*_bin $D/
IMG=$D/itest.img
qltools $IMG -fdd ATEST > /dev/null
qltools $IMG -W $D/boot $D/itest $D/oraculo_mision_txt $D/sprites_bin $D/escenario_bin $D/vista_bin $D/baldosas_bin $D/marca_activo_bin $D/textos_bin $D/mision_*_bin > /dev/null || exit 1
qltools $IMG -x itest 60000 > /dev/null
sed "s|build/heroquest.img|$IMG|" tmp/sqlux_hq_rapido.ini > $D/sqlux.ini
tmp/sQLux/build/sqlux -f $D/sqlux.ini -w 1x > $D/sqlux.log 2>&1 &
PID=$!
for i in $(seq 1 ${ESPERA:-90}); do
  sleep 2
  qltools $IMG -n itest_out > $D/salida.txt 2>/dev/null
  grep -q "mision_test:" $D/salida.txt && break
done
kill $PID 2>/dev/null; wait $PID 2>/dev/null
cat $D/salida.txt
grep -q " 0 fallos" $D/salida.txt && echo "sonda c68 (mision): correcta" || { echo "sonda c68 (mision): FALLA"; exit 1; }
