#!/bin/bash
# Pantallas del combate en el QL (compilado con c68): compila tests/combate_demo.c con qcc y el juego (sin main.c), lo graba en un disco con los datos y lo arranca en
# sQLux con las teclas de tools/ql_test.sh; deja las capturas en build/uitest/ql/. Uso: tests/run_ql_ui.sh ESCENA "teclas" [opciones -D de qcc]
#   ESCENA = ataque | bolsa | lanzar | defensa ; opciones: -DDEMO_PERS=\"3\" -DDEMO_CELDA=\"2,11\" -DDEMO_MONS=\"4\" -DDEMO_HECHIZO=\"3\"
#   teclas = las de tools/ql_test.sh (U D L R F E, . espera, Sxx captura)
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
ESC=$1; KEYS=$2; shift 2
D=build/uitest; rm -rf $D; mkdir -p $D/ql
make build/gfx_asm.o build/ql_ipc.o build/datos/textos_ids.h > /dev/null || exit 1
SRCS=$(make -pn 2>/dev/null | sed -n 's/^SRC *= *//p' | head -1 | sed 's# src/main.c##; s#src/main.c ##')
qcc -DQL_ASM -DDEMO_QL -DDEMO_ESCENA=\"$ESC\" "$@" -Isrc -Ibuild/datos -O tests/combate_demo.c $SRCS src/plat_ql.c build/ql_ipc.o build/gfx_asm.o -o $D/cdemo || exit 1
sed 's/flp1_heroquest/flp1_cdemo/' data/boot > $D/boot
IMG=$D/cdemo.img
qltools $IMG -fdd CDEMO > /dev/null
qltools $IMG -W $D/cdemo $D/boot data/*_bin data/carga_scr > /dev/null || exit 1
qltools $IMG -x cdemo 40000 > /dev/null
sed "s|build/heroquest.img|$IMG|" tmp/sqlux_hq.ini > $D/sqlux.ini
OUT=$D/ql INI=$D/sqlux.ini BOOT_WAIT=${BOOT_WAIT:-8} tools/ql_test.sh $KEYS
