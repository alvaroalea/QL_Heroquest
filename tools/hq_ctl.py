#!/usr/bin/env python3
"""Genera un .ctl de SkoolKit (bloques c/b) a partir de la cobertura de ejecucion.
Uso: hq_ctl.py IMAGEN ORG INICIO FIN BANCOS [--fill]
  IMAGEN  fichero binario con la memoria (org = direccion del primer byte)
  BANCOS  bancos de la cobertura que cuentan (p.ej. '-1,0' o '4'); -1 = memoria por debajo de 0xC000
  --fill  ademas declara codigo los huecos que se decodifican limpiamente (heuristico)
 - instrucciones ejecutadas + sucesores estaticos (ramas no tomadas) -> codigo
 - el resto -> datos. Las rutinas se parten en los destinos de CALL."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m

def main(img, org, lo, hi, banks, fill):
    data = open(img, 'rb').read()
    mem = lambda a: data[a - org] if org <= a < org + len(data) else 0
    cov = {a for b, a in m.load_cov() if lo <= a < hi and b in banks}

    def ilen(a):
        op = mem(a)
        if op in (0xDD, 0xFD):
            o2 = mem(a + 1)
            if o2 in (0xDD, 0xFD, 0xED): return 1
            if o2 == 0xCB: return 4
            if o2 in (0x21, 0x22, 0x2A, 0x36, 0x26, 0x2E): return 4
            if o2 in (0x23, 0x2B, 0xE1, 0xE5, 0xE9, 0xF9, 0xE3, 0x29, 0x09, 0x19, 0x39, 0x24, 0x25, 0x2C, 0x2D, 0x44, 0x45, 0x4C, 0x4D, 0x54, 0x55, 0x5C, 0x5D, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6F, 0x7C, 0x7D, 0x84, 0x85, 0x8C, 0x8D, 0x94, 0x95, 0x9C, 0x9D, 0xA4, 0xA5, 0xAC, 0xAD, 0xB4, 0xB5, 0xBC, 0xBD): return 2
            return 3
        if op == 0xED:
            return 4 if (mem(a + 1) & 0xC7) == 0x43 else 2
        if op == 0xCB: return 2
        if op in (0xC3, 0xCD, 0x2A, 0x22, 0x3A, 0x32) or (op & 0xCF) == 0x01 or (op & 0xC7) in (0xC2, 0xC4): return 3
        if op in (0x10, 0x18, 0xD3, 0xDB, 0xC6, 0xCE, 0xD6, 0xDE, 0xE6, 0xEE, 0xF6, 0xFE) or (op & 0xE7) == 0x20 or (op & 0xC7) == 0x06: return 2
        return 1

    def succ(a):
        op = mem(a); n = a + ilen(a); r = []
        if op in (0xC9, 0xE9, 0x76) or (op == 0xED and mem(a + 1) in (0x4D, 0x45)) or (op in (0xDD, 0xFD) and mem(a + 1) == 0xE9): return r
        w = mem(a + 1) | mem(a + 2) << 8
        if op == 0xC3: return [w]
        if op == 0x18: d = mem(a + 1); return [(a + 2 + (d - 256 if d > 127 else d)) & 0xFFFF]
        if op == 0xCD or (op & 0xC7) in (0xC4, 0xC2): r.append(w)
        if op == 0x10 or (op & 0xE7) == 0x20: d = mem(a + 1); r.append((a + 2 + (d - 256 if d > 127 else d)) & 0xFFFF)
        if (op & 0xC7) == 0xC7: r.append(op & 0x38)
        r.append(n)
        return r

    ends = {a: a + ilen(a) for a in cov}
    work = list(ends); seen = set(ends)
    while work:
        a = work.pop()
        for t in succ(a):
            if lo <= t < hi and t not in seen:
                seen.add(t); ends[t] = t + ilen(t); work.append(t)
    cb = set()
    for a, e in ends.items(): cb.update(range(a, e))
    if fill:
        flo, fhi = fill
        def plausible(a, e):
            i = a; nops = 0
            while i < e:
                op = mem(i)
                if op in (0xDD, 0xFD) and mem(i + 1) not in (0x21,0x22,0x2A,0x23,0x2B,0xE1,0xE5,0xE9,0x36,0x34,0x35,0x46,0x4E,0x56,0x5E,0x66,0x6E,0x70,0x71,0x72,0x73,0x74,0x75,0x77,0x7E,0x86,0x8E,0x96,0x9E,0xA6,0xAE,0xB6,0xBE,0xCB,0x09,0x19,0x29,0x39,0xF9,0xE3,0x26,0x2E,0x7C,0x7D): return False
                if op == 0xED and mem(i + 1) not in (0x43,0x4B,0x53,0x5B,0x63,0x6B,0x73,0x7B,0x44,0x4A,0x5A,0x6A,0x7A,0x42,0x52,0x62,0x72,0x56,0x5E,0x47,0x4F,0x57,0x5F,0xA0,0xB0,0xA8,0xB8,0xB1,0xA1,0x78,0x40,0x79,0x41,0x67,0x6F,0x4D,0x45,0x50,0x58,0x60,0x68,0x51,0x59,0x61,0x69,0xB3,0xA3,0xAB,0xBB): return False
                if op in (0x00, 0xFF, 0x76): nops += 1
                i += ilen(i)
            return i == e and nops * 4 < (e - a)
        def last_ok(a, e):
            i = a; l = a
            while i < e: l = i; i += ilen(i)
            op = mem(l)
            return op in (0xC9, 0xC3, 0x18, 0xE9) or (op == 0xED and mem(l + 1) in (0x4D, 0x45)) or (op in (0xDD, 0xFD) and mem(l + 1) == 0xE9) or (op & 0xC7) == 0xC0 or op == 0xCD
        def texty(a, e):
            t = sum(1 for k in range(a, e) if 0x41 <= mem(k) <= 0x5F)
            return e - a >= 6 and t > 0.5 * (e - a)
        i = flo
        while i < fhi:
            if i in cb: i += 1; continue
            j = i
            while j < fhi and j not in cb: j += 1
            if j - i >= 2 and plausible(i, j) and last_ok(i, j) and not texty(i, j):
                k = i
                while k < j: ends[k] = k + ilen(k); cb.update(range(k, ends[k])); k = ends[k]
            i = j
    starts = {mem(a + 1) | mem(a + 2) << 8 for a in ends if mem(a) == 0xCD} & set(ends)
    out = []; i = lo
    while i < hi:
        t = 'c' if i in cb else 'b'
        j = i
        while j < hi and ((j in cb) == (t == 'c')):
            if t == 'c' and j != i and j in starts: break
            j += 1
        out.append((t, i)); i = j
    for t, a in out: print('%s $%04X' % (t, a))

if __name__ == '__main__':
    a = sys.argv[1:]
    main(a[0], int(a[1], 0), int(a[2], 0), int(a[3], 0), {int(x) for x in a[4].split(',')}, (int(a[a.index('--fill')+1], 0), int(a[a.index('--fill')+2], 0)) if '--fill' in a else None)
