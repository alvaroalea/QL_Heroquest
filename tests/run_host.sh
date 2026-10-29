#!/bin/bash
# Pruebas de la version de PC: ejecuta los menus con teclas guionizadas, convierte las capturas a PNG y compara su CRC32 con
# tests/esperado_host.txt (cada guion empieza pasando la pantalla de idioma con F = espanol). Con --actualizar se regenera ese fichero (hacerlo solo tras revisar las capturas: build/host/t/*.png).
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
H=build/host/t; mkdir -p $H; rm -f $H/*.bin $H/*.png
rm -rf build/host/data; mkdir -p build/host/data; cp data/*_es_idx data/*_en_idx data/textos_es data/textos_en data/*_[0-9][0-9]_es data/*_[0-9][0-9]_en data/heroquest_dat data/fonts_dat build/host/data/      # los ficheros de heroes guardados no ensucian data/
export HQ_QSOUND=1       # simula la tarjeta QSound (las escrituras quedan en qs_log.txt)
export HQ_DATA=$R/build/host/data/
run() { (cd $H && rm -f qs_log.txt; HQ_KEYS="${PRE-. F }${2//S/S$1_}" ../hq > /dev/null; for f in screen_$1_*.bin; do mv "$f" "${f#screen_}"; done); }
PRE=". " run idioma "S0 D S1 D S2 U S3 2 S4 E"           # primera pantalla: elegir idioma (S0 espanol, S1 ingles, S2 espanol, S3 ingles, S4 menu principal)
PRE=". 2 " run jugar_en "F S0 F S1 ."       # misiones en ingles: S0 = lista, S1 = objetivo de la mision 0
run menu  "S0 D S1 D S2 D S3 E"
PRE=". 2 " run menu_en "S0 D S1 D S2 D S3 E"
run tienda "D F S0 D D S1 F S2 D D D D D D D D D D D D S3 F S4 E E"
PRE=". 2 " run tienda_en "D F S0 D D S1 F S2 D D D D D D D D D D D D S3 F S4 E E"
run crear "D D F S0 D F S1 F S2 F F F S3 D F S4 F S5 E E E E"
PRE=". 2 " run crear_en "D D F S0 D F S1 F S2 F F F S3 D F S4 F S5 E E E E"
run controles "D D D F S0 1 S1 3 S2 2 S3 0 E"
run jugar "F S0 F S1 F S2 F S3 R F . . S4 F S5 Rx12 Dx15 S6 F .x14 S7 F .x14 F .x14 S8 Lx30 Ux40 S9 F .x20 S10 E"
run ojo "F F F F R F . . S0 F Rx15 Dx14 F . S1 Lx18 Ux17 Dx1 F .x5 S2 .x20 S3 E"
PRE=". 2 " run ojo_en "F F F F R F . . S0 F Rx15 Dx14 F . S1 Lx18 Ux17 Dx1 F .x5 S2 .x20 S3 E"                                    # las mismas ventanas en ingles
run ctl_idioma "D D D F S0 4 S1 4 S2 0 E"        # 4.IDIOMA: espanol, ingles, espanol
OUT=$(for f in $(ls $H/*.bin | sort); do printf "%s %s\n" "$(basename $f)" "$(cksum < $f | cut -d' ' -f1)"; done)
for f in $H/*.bin; do tmp/venv/bin/python -I tools/ql2png.py $f ${f%.bin}.png 2; done
if [ "$1" = "--actualizar" ]; then echo "$OUT" > tests/esperado_host.txt; echo "actualizado tests/esperado_host.txt"; exit 0; fi
if [ "$OUT" = "$(cat tests/esperado_host.txt 2>/dev/null)" ]; then echo "pruebas de PC: correctas ($(echo "$OUT" | wc -l) capturas)"; else echo "pruebas de PC: DIFERENCIAS"; diff <(echo "$OUT") tests/esperado_host.txt; exit 1; fi
