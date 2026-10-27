#!/usr/bin/env python3
"""Mapa de un banco: tramos de ceros / datos por bloques de 256 bytes. Uso: hq_bankruns.py N"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
b = m.BANKS[int(sys.argv[1])]
for p in range(0, 16384, 256):
    blk = b[p:p+256]
    z = blk.count(0)
    print('%04X %3d%% zeros  %s' % (0xC000 + p, z * 100 // 256, ' '.join('%02X' % x for x in blk[:12])))
