#!/usr/bin/env python3
"""Genera un guion para hq_trace.py: pulsa teclas al azar. Uso: hq_fuzz_script.py semilla frames keys > guion"""
import random, sys
seed, n, keys = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3].split(',')
random.seed(seed)
f = 60
while f < n:
    k = random.choice(keys)
    if random.random() < 0.2: k += '+' + random.choice(keys)
    print(f, 'key', k, random.choice([2, 3, 5, 8, 20]))
    f += random.choice([6, 9, 15, 30])
    if f % 400 < 12: print(f, 'shot f%d' % f)
print(n, 'end')
