#!/usr/bin/env python3
"""Lee data/textos_bin y lo compara con los textos originales de la memoria del juego (solo tiene sentido si
se empaqueto desde asset/textos/textos.txt sin modificar). Informa de las diferencias."""
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hq_txt_lib as T
ROOT = T.ROOT
def leer():
    d = open(os.path.join(ROOT, 'data/textos_bin'), 'rb').read()
    assert d[:4] == b'HQTX'; n = struct.unpack('>H', d[6:8])[0]; out = []
    for i in range(n):
        off, tipo, al, cant = struct.unpack('>LBBH', d[8 + 8 * i:16 + 8 * i]); out.append((off, tipo, al, cant))
    return d, out
def main():
    d, tab = leer(); ids = [l.split()[1] for l in open(os.path.join(ROOT, 'build/datos/indice_textos.txt')).read().split('\n')[1:] if l.strip()]
    ok = dif = 0; detalles = []
    for (off, tipo, al, cant), id_ in zip(tab, ids):
        if tipo == 1:
            y, centro, ancho, nl, nz, _ = d[off:off + 6]; p = off + 6; lines = []
            for _i in range(nl):
                a, ln = d[p], d[p + 1]; lines.append((chr(a), d[p + 2:p + 2 + ln])); p += 2 + ln
            w = max([ancho] + [len(x[1]) for x in lines])
            # reconstruir como el juego
            rec = []
            for a, bs in lines:
                pad = w - len(bs); lp = 0 if a == 'L' else pad if a == 'R' else pad // 2
                rec.append(b'\x5f' * lp + bs + b'\x5f' * (pad - lp))
            addr = int(id_.split('_')[1], 16)
            orig, _fin = T.ventanas(T.b_main, addr, addr + 1 + 4 + 40 * 22)
            o = orig[0]
            comp = [sum(1 for a, b in zip(r, q) if a != b) for r, q in zip(rec, o['lineas'])] if len(rec) == len(o['lineas']) else [99]
            if sum(comp) == 0 and o['x'] == centro - w // 2 and o['y'] == y: ok += 1
            else: dif += 1; detalles.append('%s: caracteres distintos por linea %s, x %d/%d' % (id_, comp, o['x'], centro - w // 2))
        elif tipo == 2:
            addr = int(id_.split('_')[1], 16); b = T.b_ov if id_.startswith('MENU') else T.b_main
            seg, _ = T.guion(b, addr, None); lin = [l for l in T.guion_a_lineas(seg) if l[0] == 'texto']
            nops = struct.unpack('>H', d[off:off + 2])[0]; p = off + 2; textos = []
            while d[p] != 0:
                op = d[p]
                if op == 1: at, a, x, y, ln = d[p + 1], chr(d[p + 2]), d[p + 3], d[p + 4], d[p + 5]; textos.append((at, x, y, d[p + 6:p + 6 + ln])); p += 6 + ln
                elif op == 2: p += 5
                else: p += 1
            if len(textos) == len(lin) and all(t[0] == l[1] and t[2] == l[3] and t[3] == l[4].strip(b'\x5f') and abs((t[1] if True else 0) - l[2]) <= 40 for t, l in zip(textos, lin)): ok += 1
            else: dif += 1; detalles.append('%s: %d textos / %d originales' % (id_, len(textos), len(lin)))
        else: ok += 1
    print('elementos coincidentes con el original: %d, con diferencias: %d' % (ok, dif))
    for x in detalles[:20]: print('  ', x)
main()
