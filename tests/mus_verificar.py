"""Compara la salida de tests/mus_test (estado de los registros del AY por fotograma) con un volcado .psg del original.
Uso: mus_verificar.py salida.txt melodia.psg   (muestra el primer fotograma que difiere y cuantos fotogramas coinciden)"""
import sys
d = open(sys.argv[2], 'rb').read()
assert d[:4] == b'PSG\x1a'
i = 16; regs = [0] * 16; frames = []
while i < len(d):
    b = d[i]; i += 1
    if b == 0xFF: frames.append(tuple(regs[:14]))
    elif b == 0xFE: n = d[i] * 4; i += 1; frames += [tuple(regs[:14])] * n
    elif b == 0xFD: break
    else: regs[b & 15] = d[i]; i += 1
mine = [tuple(int(l[k:k + 2], 16) for k in range(0, 28, 2)) for l in open(sys.argv[1]).read().split()]
off = int(sys.argv[3]) if len(sys.argv) > 3 else 0
n = min(len(frames), len(mine) - off); bad = None
for k in range(n):
    a, b = mine[k + off], frames[k]
    # el registro 13 (forma de envolvente) y los de ruido sin canal activo no cuentan; se compara todo lo audible
    if a != b: bad = k; break
if bad is None: print('IGUALES: %d fotogramas (desfase %d)' % (n, off))
else:
    print('DIFIERE en el fotograma %d de %d' % (bad, n)); print(' mio ', ' '.join('%02X' % x for x in mine[bad + off])); print(' psg ', ' '.join('%02X' % x for x in frames[bad]))
