#!/usr/bin/env python3
"""Ventanas de texto de Hero Quest (memoria fija). Formato (rutina VENTANA, 0x8DD1):
  X, Y, ancho, alto, alto*ancho caracteres ('_' = espacio) y opcionalmente zonas de 6 bytes
  (x1, x2, y1, y2, rutina) terminadas en 0xFE.
Uso: hq_textos.py txt INI FIN   -> texto legible
     hq_textos.py ctl INI FIN   -> subbloques ctl"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
def txt(bs): return ''.join(' ' if b == 0x5F else chr(b) if 0x20 <= b < 0x7F else '<%02X>' % b for b in bs)
def zona(a):
    x1, x2, y1, y2 = (m.byte(a + i) for i in range(4)); h = m.byte(a + 4) | m.byte(a + 5) << 8
    return x1 < x2 < 32 and y1 < y2 < 24 and 0x6400 <= h < 0xB500
def parse(a, fin):
    out = []
    while a < fin:
        x, y, w, h = (m.byte(a + i) for i in range(4))
        if not (0 < w < 32 and 0 < h < 22 and x < 32 and y < 24): return out, a
        end = a + 4 + w * h
        z = []
        b = end
        if zona(b):
            while m.byte(b) != 0xFE and zona(b): z.append(b); b += 6
            if m.byte(b) == 0xFE: b += 1
            else: z = []; b = end
        pad = 0
        while m.byte(b + pad) == 0x5F: pad += 1
        def hdr(q):
            xx, yy, ww, hh = (m.byte(q + i) for i in range(4))
            return 0 < ww < 32 and 0 < hh < 22 and xx < 32 and yy < 24 and all(0x20 <= m.byte(q + 4 + k) < 0x7F for k in range(min(ww, 4)))
        if not hdr(b + pad) and not z and b + pad < fin:
            for k in range(1, 9):
                if hdr(end - k): b = end - k; pad = 0; end = b; break
        out.append(dict(a=a, x=x, y=y, w=w, h=h, end=end, z=z, fin=b, pad=pad))
        a = b + pad
    return out, a
if __name__ == '__main__':
    modo, a0, a1 = sys.argv[1], int(sys.argv[2], 0), int(sys.argv[3], 0)
    ws, fin = parse(a0, a1)
    for v in ws:
        if modo == 'txt':
            print('%04X  x=%2d y=%2d  %dx%d' % (v['a'], v['x'], v['y'], v['w'], v['h']))
            for r in range(v['h']): print('        |%s|' % txt(bytes(m.byte(v['a'] + 4 + r * v['w'] + c) for c in range(v['w']))))
            for z in v['z']: print('        zona x=%d..%d y=%d..%d -> %04X' % (m.byte(z), m.byte(z+1), m.byte(z+2), m.byte(z+3), m.byte(z+4) | m.byte(z+5) << 8))
        else:
            print('B $%04X,4' % v['a'])
            n = v['end'] - v['a'] - 4
            print('T $%04X,%d%s' % (v['a'] + 4, n, (',%d' % v['w']) if n == v['w'] * v['h'] else ''))
            for z in v['z']: print('B $%04X,6' % z)
            if v['fin'] > v['end'] + 6 * len(v['z']): print('B $%04X,1' % (v['fin'] - 1))
            if v['pad']: print('B $%04X,%d' % (v['fin'], v['pad']))
    print('# fin %04X' % fin, file=sys.stderr)
