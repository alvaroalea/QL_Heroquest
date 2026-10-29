#!/usr/bin/env python3
"""Compara el volcado de Hero Quest con el de la ampliacion Return of the Witch Lord (docs/ZX_AMPLIACION.md).
Uso: hq_ampliacion_diff.py texto|codigo|bancos [es|en] [bytes_minimos]
  texto   textos del juego (0xA400-0xC1C0) que cambian, se anaden o se quitan
  codigo  bloques de codigo (0x6D81-0xA400) insertados, borrados o cambiados (el codigo de la ampliacion esta desplazado; bytes_minimos filtra los cambios de direcciones, 12 por defecto)
  bancos  bytes distintos por banco de memoria (graficos: bancos 0, 6 y 7)
Necesita asset/extraido[_en] y asset/extraido_wl[_en] (tools/tzx_to_z80.py)."""
import re, sys, os, difflib
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def dirs(v): return ('extraido', 'extraido_wl') if v == 'es' else ('extraido_en', 'extraido_wl_en')
def rd(d, f): return open(os.path.join(ROOT, 'asset', d, f), 'rb').read()
def toks(b, lo, hi):
    out = []
    for m in re.finditer(rb'[\x30-\x5f]{3,}', b[lo - 0x4000:hi - 0x4000]):
        t = m.group(0).decode('latin1').replace('_', ' ')
        if re.search(r'[A-Z]{2}', t): out.append((lo + m.start(), t.strip()))
    return out
def texto(v):
    a, b = dirs(v); ta = toks(rd(a, 'main48.bin'), 0xA400, 0xC1C0); tb = toks(rd(b, 'main48.bin'), 0xA400, 0xC1C0)
    for op, i1, i2, j1, j2 in difflib.SequenceMatcher(None, [t for _, t in ta], [t for _, t in tb], autojunk=False).get_opcodes():
        if op == 'equal': continue
        print(op.upper())
        for k in range(i1, i2): print('   - %04X %s' % ta[k])
        for k in range(j1, j2): print('   + %04X %s' % tb[k])
def codigo(v, minimo):
    a, b = dirs(v); lo, hi = 0x6D81 - 0x4000, 0xA400 - 0x4000
    sm = difflib.SequenceMatcher(None, rd(a, 'main48.bin')[lo:hi], rd(b, 'main48.bin')[lo:hi + 0x400], autojunk=False)
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op != 'equal' and max(i2 - i1, j2 - j1) >= minimo:
            print('%-7s hq %04X-%04X (%d)  ampliacion %04X-%04X (%d)' % (op, 0x6D81 + i1, 0x6D81 + i2, i2 - i1, 0x6D81 + j1, 0x6D81 + j2, j2 - j1))
def bancos(v):
    a, b = dirs(v)
    for i in range(8):
        x, y = rd(a, 'bank%d.bin' % i), rd(b, 'bank%d.bin' % i)
        print('banco %d: %d bytes distintos de %d' % (i, sum(1 for p, q in zip(x, y) if p != q), len(x)))
if __name__ == '__main__':
    modo = sys.argv[1] if len(sys.argv) > 1 else ''; v = sys.argv[2] if len(sys.argv) > 2 else 'es'
    if modo == 'texto': texto(v)
    elif modo == 'codigo': codigo(v, int(sys.argv[3]) if len(sys.argv) > 3 else 12)
    elif modo == 'bancos': bancos(v)
    else: print(__doc__)
