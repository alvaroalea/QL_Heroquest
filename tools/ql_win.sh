#!/bin/bash
# Crea build/heroquest.win (volumen QXL.WIN) con qxltool (Norman Dunbar, tmp/qxltools) y los mismos ficheros que build/heroquest.img.
# Uso: tools/ql_win.sh [tamano_en_MB]     (el ejecutable lleva su espacio de datos en el bloque XTcc del final: se pone el mismo que en la imagen)
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
MB=${1:-4}; DS=${DATASPACE:-40000}
Q=tmp/qxltools/qxltool; WIN=build/heroquest.win; T=build/win_tmp
if [ ! -x $Q ]; then
  [ -d tmp/qxltools ] || git clone -q https://github.com/NormanDunbar/qxltools.git tmp/qxltools || exit 1
  # el codigo es de 32 bits: en x86_64 u_long y time_t miden 8 y QLDIR no mide 64 bytes (el directorio se corrompe con mas de 8 ficheros);
  # stpcpy ya existe en libc y faltan cabeceras. Se compila una copia parcheada en tmp/qxltools/linux64
  L=tmp/qxltools/linux64; mkdir -p $L
  sed -e 's/\bu_long\b/qx_u32/g' -e 's/^#define QLPATH_MAX 36/#define QLPATH_MAX 36\ntypedef unsigned int qx_u32;/' \
      -e 's/^    time_t date PACKED;/    qx_u32 date PACKED;/' -e 's/^typedef struct$/#pragma pack(push,1)\ntypedef struct/;t' tmp/qxltools/qxltool.h > $L/qxltool.h
  python3 - $L/qxltool.h <<'PY'
import sys, re
p = sys.argv[1]; s = open(p).read()
s = s.replace("#pragma pack(push,1)\n", "")                      # un solo bloque empaquetado
s = s.replace("typedef struct\n{\n    char id[4];", "#pragma pack(push,1)\ntypedef struct\n{\n    char id[4];", 1)
s = s.replace("} XTCC;", "} XTCC;\n#pragma pack(pop)", 1)
s = s.replace("        long x;", "        int x;")
open(p, 'w').write(s)
PY
  sed -e 's/^#ifndef HAVE_STPCPY/#if 0/' -e 's/\bu_long\b/qx_u32/g' -e 's/(u_long)/(qx_u32)/g' \
      -e 's/tm = localtime (&d->date);/{ time_t tt = (time_t) d->date; tm = localtime (\&tt); }/' tmp/qxltools/qxltool.c > $L/qxltool.c
  cp tmp/qxltools/version.h $L/
  gcc -std=gnu89 -w -include time.h -include unistd.h -include alloca.h -o $Q $L/qxltool.c || exit 1
fi
rm -rf $T $WIN; mkdir -p $T
cp build/heroquest $T/heroquest
python3 - "$T/heroquest" $DS <<'PY'
import sys, struct
p, ds = sys.argv[1], int(sys.argv[2])
d = bytearray(open(p, 'rb').read())
assert d[-8:-4] == b'XTcc'
d[-4:] = struct.pack('>I', ds)
open(p, 'wb').write(d)
PY
sed "s/FLP1_/WIN1_/" data/boot > $T/boot      # en el volumen la unidad es WIN1_ (la imagen de disco usa FLP1_)
for f in data/heroquest_dat data/fonts_dat data/*_[0-9][0-9]_es data/*_[0-9][0-9]_en data/*_es_idx data/*_en_idx data/textos_es data/textos_en data/*_scr; do cp $f $T/; done
( echo "wr $T/heroquest"; echo "wr $T/boot"
  for f in $T/*; do case $f in */heroquest|*/boot) ;; *) echo "wr $f" ;; esac; done
  echo ls ) | $Q -w $WIN $MB HEROQUEST > build/win.log 2>&1
rm -rf $T
tail -n 5 build/win.log; ls -l $WIN
