#!/usr/bin/env python3
"""Hoja de baldosas 1bpp -> PNG. Uso: hq_tiles.py fichero salida.png offset cuantas ancho_bytes alto_filas columnas [escala]
Los datos de cada baldosa son filas consecutivas (ancho_bytes por fila)."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from hq_bits import png
f, out = sys.argv[1:3]
off, n, tw, th, cols = (int(x, 0) for x in sys.argv[3:8])
sc = int(sys.argv[8]) if len(sys.argv) > 8 else 2
d = open(f, 'rb').read()
rows_t = (n + cols - 1) // cols
W = cols * (tw * 8 + 1); H = rows_t * (th + 1)
img = [[200] * W for _ in range(H)]
for t in range(n):
    cx, cy = (t % cols) * (tw * 8 + 1), (t // cols) * (th + 1)
    for y in range(th):
        for x in range(tw):
            i = off + t * tw * th + y * tw + x
            b = d[i] if i < len(d) else 0
            for k in range(8):
                img[cy + y][cx + x * 8 + k] = 0 if b & (0x80 >> k) else 255
rows = []
for r in img:
    rr = []
    for v in r: rr += [v] * sc
    rows += [rr] * sc
png(out, len(rows[0]), len(rows), rows)
