#!/usr/bin/env python3
"""Lee un .psg y muestra su duración audible. Uso: hq_psg_info.py f.psg ..."""
import sys
def leer(path):
    d = open(path, 'rb').read(); i = 16; st = [0] * 16; frames = []; cur = False
    while i < len(d):
        c = d[i]; i += 1
        if c == 0xFF: frames.append(list(st))
        elif c == 0xFE: n = d[i] * 4; i += 1; frames += [list(st)] * n
        elif c <= 0x0F: st[c] = d[i]; i += 1
    return frames
def audible(frames):
    ult = 0
    for k, s in enumerate(frames):
        if any(s[8 + c] & 0x1F for c in range(3)): ult = k + 1
    return ult
if __name__ == '__main__':
    for p in sys.argv[1:]:
        f = leer(p); print('%s: %d fotogramas, audible hasta %d (%.2f s)' % (p, len(f), audible(f), audible(f) / 50))
