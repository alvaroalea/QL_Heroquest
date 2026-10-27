#!/usr/bin/env python3
"""Resumen de una region: tramos de ceros y bloques no nulos (con ASCII). Uso: hq_runs.py INI FIN"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
a, e = int(sys.argv[1], 0), int(sys.argv[2], 0)
i = a
while i < e:
    j = i
    z = m.byte(i) == 0
    while j < e and (m.byte(j) == 0) == z: j += 1
    if z and j - i >= 4: print('%04X-%04X zeros (%d)' % (i, j - 1, j - i))
    else:
        k = i
        while k < j:
            n = min(16, j - k)
            bs = [m.byte(k + t) for t in range(n)]
            print('%04X %-48s %s' % (k, ' '.join('%02X' % b for b in bs), ''.join(chr(b) if 32 <= b < 127 else '.' for b in bs)))
            k += n
    i = j
