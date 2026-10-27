#!/usr/bin/env python3
"""Fusiona los ficheros *_cov.txt de asset/trace en asset/trace/cov.txt."""
import glob, os
d = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'asset', 'trace')
s = set()
for f in glob.glob(d + '/*_cov.txt') + [d + '/cov.txt']:
    if os.path.exists(f):
        s |= {tuple(l.split()) for l in open(f)}
with open(d + '/cov.txt', 'w') as o:
    for b, a in sorted(s, key=lambda x: (int(x[0]), int(x[1], 16))): o.write('%s %s\n' % (b, a))
print(len(s))
