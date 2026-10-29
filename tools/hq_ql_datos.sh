#!/bin/bash
# Genera todos los datos del port a QL en data/ (nombres sin punto) y los indices para el PC en build/datos/.
#   graficos: asset/graficos_ql (retocados) o asset/graficos (originales)  -> sprites_bin, escenario_bin, retratos_bin, marco_bin, baldosas_bin, fuente_*_bin
#   textos:   asset/textos_ql/textos.es.txt o asset/textos/textos.es.txt    -> build/datos/textos_es_bin y textos_en_bin (se pegan a heroquest_xx_idx)   (idioma: variable HQ_VERSION, por defecto es)
#   misiones: asset/misiones_ql o asset/misiones (misionNN.es.txt)          -> heroquest_NN, heroquest_xx_idx
#   todo junto: escenario, marco, retratos, sprites, baldosas, vista, musica y efectos -> heroquest_dat (tools/hq_ql_dat.py)
# Uso: tools/hq_ql_datos.sh [extraer]   ('extraer' regenera antes los originales de asset/ (textos y misiones en espanol e ingles; los graficos
#       son los mismos) y copia los ficheros que falten a los *_ql)
set -e
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"; PY="tmp/venv/bin/python -I"
if [ "$1" = "extraer" ]; then
  $PY tools/hq_gfx_extraer.py
  for v in es en; do HQ_VERSION=$v $PY tools/hq_textos_extraer.py; HQ_VERSION=$v $PY tools/hq_misiones_extraer.py; done
  $PY tools/hq_ql_inicializar.py
fi
$PY tools/hq_ql_empaquetar.py --verificar
$PY tools/hq_textos_empaquetar.py
$PY tools/hq_misiones_empaquetar.py
$PY tools/hq_vista_empaquetar.py
$PY tools/hq_musica_empaquetar.py
$PY tools/hq_efectos_empaquetar.py
$PY tools/hq_ql_dat.py
