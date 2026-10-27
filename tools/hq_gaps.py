#!/usr/bin/env python3
"""Lista los bloques de datos del .ctl automatico con una pista de que contienen (texto, ceros, codigo probable).
Uso: hq_gaps.py ctl IMAGEN ORG FIN"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
ctl, img, org, fin = sys.argv[1], sys.argv[2], int(sys.argv[3], 0), int(sys.argv[4], 0)
data = open(img, 'rb').read()
bl = [(l.split()[0], int(l.split()[1][1:], 16)) for l in open(ctl) if l[0] in 'cbi' and l[1] == ' ']
for k, (t, a) in enumerate(bl):
    e = bl[k + 1][1] if k + 1 < len(bl) else fin
    if t != 'b': continue
    d = data[a - org:e - org]
    z = d.count(0); txt = sum(1 for x in d if 0x41 <= x <= 0x5F or x == 0x5F)
    kind = 'ceros' if z == len(d) else 'texto' if txt > 0.5 * len(d) else 'mixto'
    print('%04X-%04X %5d %-6s %s' % (a, e - 1, e - a, kind, d[:10].hex()))
