#!/usr/bin/env python3
"""Vuelca bytes como bitmap 1bpp en un PNG (sin dependencias). 
Uso: hq_bits.py fichero salida.png [ancho_en_bytes=32] [offset=0] [longitud] [escala=1]"""
import sys, zlib, struct
def png(path, w, h, rows):
    raw = b''.join(b'\x00' + bytes(r) for r in rows)
    def ch(t, d): 
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + ch(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 0, 0, 0, 0)) + ch(b'IDAT', zlib.compress(raw)) + ch(b'IEND', b''))
def render(data, wb, scale=1):
    rows = []
    for y in range(0, len(data) // wb):
        row = []
        for x in range(wb):
            b = data[y*wb + x]
            row += [0 if b & (0x80 >> k) else 255 for k in range(8)]
        r = []
        for v in row: r += [v] * scale
        rows += [r] * scale
    return len(rows[0]), len(rows), rows
if __name__ == '__main__':
    f, out = sys.argv[1:3]
    wb = int(sys.argv[3]) if len(sys.argv) > 3 else 32
    off = int(sys.argv[4], 0) if len(sys.argv) > 4 else 0
    d = open(f, 'rb').read()
    ln = int(sys.argv[5], 0) if len(sys.argv) > 5 else len(d) - off
    sc = int(sys.argv[6]) if len(sys.argv) > 6 else 1
    w, h, rows = render(d[off:off+ln], wb, sc)
    png(out, w, h, rows)
