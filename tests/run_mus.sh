#!/bin/bash
# Verifica el motor de musica (src/music.c) contra los volcados del AY del original (asset/musica/psg/melodia_N.psg): con los periodos
# del Spectrum los 14 registros deben ser identicos en todos los fotogramas de cada melodia (incluido el bucle).
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"; mkdir -p build/datos
python3 tools/hq_musica_empaquetar.py --zx -o build/datos/musica_zx_bin > /dev/null || exit 1
gcc -std=c89 -w -Isrc -o build/mus_test tests/mus_test.c src/music.c || exit 1
mal=0
for p in "0 769" "1 34945" "2 16241" "3 12673"; do
  set -- $p
  build/mus_test build/datos/musica_zx_bin $1 $2 > build/mus_$1.txt
  r=$(python3 tests/mus_verificar.py build/mus_$1.txt asset/musica/psg/melodia_$1.psg 0)
  echo "melodia $1: $r"; case "$r" in IGUALES*) ;; *) mal=1;; esac
done
python3 tools/hq_efectos_empaquetar.py --zx -o build/datos/efectos_zx_bin > /dev/null || exit 1
gcc -std=c89 -w -Isrc -o build/fx_test tests/fx_test.c src/music.c || exit 1
for p in "0 5" "1 25" "2 10" "3 31" "4 60" "5 20" "6 3" "7 60" "8 20"; do
  set -- $p
  build/fx_test build/datos/efectos_zx_bin $1 $2 > build/fx_$1.txt
  r=$(python3 tests/mus_verificar.py build/fx_$1.txt asset/musica/psg/efecto_$1.psg 0)
  echo "efecto $1: $r"; case "$r" in IGUALES*) ;; *) mal=1;; esac
done
exit $mal
