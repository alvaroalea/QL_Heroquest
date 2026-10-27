#!/usr/bin/env python3
"""Divide una region de 'guiones de texto' (los interpreta 0x8BF9) en subbloques ctl.
Codigos: FF x y (posicion) | FB c (atributo) | FC lo hi w h (recuadro) | FE (modo OR) | FA (fin) | 00 (fin) | resto = caracter.
Uso: hq_scripts.py INI FIN [ctl|txt]"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
if os.environ.get('HQ_OVERLAY'): m.use_overlay()
def scripts(a, fin):
    toks = []
    while a < fin:
        b = m.byte(a)
        if b == 0xFF: toks.append(('B', a, 3, 'POS x=%d y=%d' % (m.byte(a+1), m.byte(a+2)))); a += 3
        elif b == 0xFB: toks.append(('B', a, 2, 'ATTR %02X' % m.byte(a+1))); a += 2
        elif b == 0xFC: toks.append(('B', a, 5, 'BOX')); a += 5
        elif b == 0xFE: toks.append(('B', a, 1, 'OR')); a += 1
        elif b == 0xFA: toks.append(('B', a, 1, 'END')); a += 1
        elif b == 0x00: toks.append(('B', a, 1, 'END0')); a += 1
        else:
            j = a
            while j < fin and m.byte(j) not in (0, 0xFA, 0xFB, 0xFC, 0xFE, 0xFF): j += 1
            toks.append(('T', a, j - a, ''.join(chr(m.byte(k)) if m.byte(k) != 0x5F else ' ' for k in range(a, j)))); a = j
    return toks, a
if __name__ == '__main__':
    a0, a1 = int(sys.argv[1], 0), int(sys.argv[2], 0)
    modo = sys.argv[3] if len(sys.argv) > 3 else 'ctl'
    toks, a = scripts(a0, a1)
    for t, s, n, d in toks:
        if modo == 'ctl':
            print('%s $%04X,%d' % (t, s, n))
        else: print('%04X %s %s' % (s, t, d))
    print('# fin %04X' % a, file=sys.stderr)
