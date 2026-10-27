#!/usr/bin/env python3
"""Divide una region de datos en subbloques ctl: cadenas (T, acabadas en 0x00 o 0xFF) y palabras (W) que parecen
punteros (valor alto entre dos limites). Uso: hq_strings.py INI FIN PTR_LO PTR_HI  -> lineas ctl en stdout"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
a, e = int(sys.argv[1], 0), int(sys.argv[2], 0)
lo, hi = int(sys.argv[3], 0), int(sys.argv[4], 0)
def isword(i): 
    w = m.byte(i) | m.byte(i + 1) << 8
    return lo <= w < hi
def istext(b): return 0x30 <= b <= 0x5F or b in (0x3A, 0x3B)
i = a
out = []
while i < e:
    if isword(i) and isword(i + 2) :
        j = i
        while j + 1 < e and isword(j): j += 2
        out.append(('W', i, j - i)); i = j
    elif istext(m.byte(i)):
        j = i
        while j < e and istext(m.byte(j)): j += 1
        if j < e and m.byte(j) in (0, 0xFF): j += 1
        out.append(('T', i, j - i)); i = j
    else:
        j = i
        while j < e and not istext(m.byte(j)) and not (isword(j) and isword(j + 2)): j += 1
        if j == i: j = i + 1
        out.append(('B', i, j - i)); i = j
for t, s, n in out: print('%s $%04X,%d' % (t, s, n))
