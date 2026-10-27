#!/usr/bin/env python3
"""Convierte un volcado de la pantalla del QL (modo 8, 32768 bytes, 128 bytes por linea) a PNG.
Uso: ql2png.py screen.bin salida.png [escala]"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from hq_png import escribir_rgba
QL = [(0, 0, 0), (0, 0, 255), (255, 0, 0), (255, 0, 255), (0, 255, 0), (0, 255, 255), (255, 255, 0), (255, 255, 255)]
d = open(sys.argv[1], 'rb').read(); esc = int(sys.argv[3]) if len(sys.argv) > 3 else 2
filas = []
for y in range(256):
    fila = []
    for x in range(256):
        w = x // 4; k = x % 4; e = d[y * 128 + w * 2]; o = d[y * 128 + w * 2 + 1]; s = 6 - 2 * k
        c = (((e >> (s + 1)) & 1) << 2) | (((o >> (s + 1)) & 1) << 1) | ((o >> s) & 1)
        fila += [QL[c] + (255,)] * esc
    filas += [fila] * esc
escribir_rgba(sys.argv[2], 256 * esc, 256 * esc, filas)
