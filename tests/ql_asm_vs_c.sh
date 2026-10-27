#!/bin/bash
# Compara en sQLux las capturas de la version con ensamblador (QL_ASM) y la de solo C: deben ser identicas bit a bit.
# Tarda unos minutos y abre una ventana del emulador. Uso: tests/ql_asm_vs_c.sh
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
A="S01 D F S02 D D F S03 D D D S04 E E D D F S05 F S06 R F S07 F"; B="F F F F S01 F S02 F S03"; C="F F R F F F S01"
export SNAP_WAIT=${SNAP_WAIT:-1} INI=tmp/sqlux_hq_rapido.ini       # a maxima velocidad: lo que se compara es el dibujo, no el tiempo
make -B ql > /dev/null && make img > /dev/null || exit 1
OUT=build/asm_a BOOT_WAIT=${BOOT_WAIT:-8} tools/ql_test.sh $A > /dev/null; OUT=build/asm_b BOOT_WAIT=${BOOT_WAIT:-8} tools/ql_test.sh $B > /dev/null; OUT=build/asm_c BOOT_WAIT=${BOOT_WAIT:-8} tools/ql_test.sh $C > /dev/null
make -B ql QLFLAGS= > /dev/null && make img > /dev/null || exit 1
OUT=build/c_a BOOT_WAIT=${BOOT_WAIT:-8} tools/ql_test.sh $A > /dev/null; OUT=build/c_b BOOT_WAIT=${BOOT_WAIT:-8} tools/ql_test.sh $B > /dev/null; OUT=build/c_c BOOT_WAIT=${BOOT_WAIT:-8} tools/ql_test.sh $C > /dev/null
mal=0; for f in build/asm_a/*.bin build/asm_b/*.bin build/asm_c/*.bin; do cmp -s $f ${f/asm_/c_} || { echo "DIFERENTE: $f"; mal=1; }; done
export SNAP_WAIT=${SNAP_WAIT:-1} INI=tmp/sqlux_hq_rapido.ini       # a maxima velocidad: lo que se compara es el dibujo, no el tiempo
make -B ql > /dev/null && make img > /dev/null          # deja la imagen con ensamblador
[ $mal = 0 ] && echo "ensamblador y C: capturas identicas"; exit $mal
