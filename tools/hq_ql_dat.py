#!/usr/bin/env python3
"""Junta ficheros de data/ en dos contenedores:
  heroquest_dat = escenario, marco, sprites, baldosas, vista, musica, efectos y, al final, retratos (DAT_* de src/data.h; el juego carga todo menos los retratos y despues decide si caben)
  fonts_dat     = fuente_normal, fuente_manuscrita y marca_activo (FNT_* de src/data.h)
Formato (big endian): 'HQDT', u16 n, u16 0, n x { u32 desplazamiento, u32 tamano }, datos (cada uno alineado a 4).
Uso: tools/hq_ql_dat.py [directorio_data]"""
import os, struct, sys

CONTENEDORES = [
    ("heroquest_dat", ["escenario_bin", "marco_bin", "sprites_bin", "baldosas_bin", "vista_bin", "musica_bin", "efectos_bin", "retratos_bin"]),
    ("fonts_dat", ["fuente_normal_bin", "fuente_manuscrita_bin", "marca_activo_bin"]),
]

def juntar(d, salida, nombres):
    datos = [open(os.path.join(d, n), "rb").read() for n in nombres]
    pos = 8 + 8 * len(datos)
    tabla, cuerpo = b"", b""
    for b in datos:
        pos = (pos + 3) & ~3
        cuerpo += b"\0" * (pos - 8 - 8 * len(datos) - len(cuerpo))
        tabla += struct.pack(">II", pos, len(b))
        cuerpo += b
        pos += len(b)
    with open(os.path.join(d, salida), "wb") as f:
        f.write(b"HQDT" + struct.pack(">HH", len(datos), 0) + tabla + cuerpo)
    print("%s: %d ficheros, %d bytes" % (salida, len(datos), 8 + len(tabla) + len(cuerpo)))

def main():
    d = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")
    for salida, nombres in CONTENEDORES: juntar(d, salida, nombres)

main()
