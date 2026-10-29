#!/usr/bin/env python3
"""Lee data/heroquest_NN_es y compara mapa, heroes, indicadores, monstruos y objetos con los datos originales del juego
(solo tiene sentido si se empaqueto desde asset/misiones/ sin modificar)."""
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hq_missions as hm
import hq_txt_lib as T
mal = 0
for n in range(14):
    d = open(os.path.join(T.ROOT, 'data/heroquest_%02d_es' % n), 'rb').read(); r = hm.mision(n)
    assert d[:4] == b'HQMS' and struct.unpack('>H', d[6:8])[0] == n
    nm, no = d[20], d[21]; offs = struct.unpack('>6L', d[24:48])
    A, B, F, M, O = d[offs[0]:offs[0] + 494], d[offs[1]:offs[1] + 494], d[offs[2]:offs[2] + 86], d[offs[3]:offs[3] + 10 * nm], d[offs[4]:offs[4] + 3 * no]
    ok = (A == bytes(r['capa_a'][:494]) and B == bytes(r['capa_b'][:494]) and F == bytes(r['flags']) and
          [tuple(d[8 + 3 * i:11 + 3 * i]) for i in range(4)] == [tuple(h) for h in r['heroes']] and
          [tuple(M[10 * i:10 * i + 10]) for i in range(nm)] == [tuple(x) for x in r['monstruos']] and
          [tuple(O[3 * i:3 * i + 3]) for i in range(no)] == [tuple(x) for x in r['objetos']])
    # texto: mismos textos (sin relleno) y filas
    tx = r['texto']; seg, _ = T.guion(lambda a: tx[a] if a < len(tx) else 0, 0, len(tx))
    orig = [(l[1], l[3], l[4].strip(b'\x5f')) for l in T.guion_a_lineas(seg) if l[0] == 'texto']
    p = offs[5]; nops = struct.unpack('>H', d[p:p + 2])[0]; p += 2; got = []
    while d[p]:
        if d[p] == 1: at, al, x, y, ln = d[p + 1], d[p + 2], d[p + 3], d[p + 4], d[p + 5]; got.append((at, y, d[p + 6:p + 6 + ln])); p += 6 + ln
        else: p += 5 if d[p] == 2 else 1
    ok2 = got == orig
    if not (ok and ok2): mal += 1; print('mision', n, 'DIFERENCIAS datos:', ok, 'texto:', ok2)
print('misiones que coinciden con el original: %d de 14' % (14 - mal))
sys.exit(1 if mal else 0)
