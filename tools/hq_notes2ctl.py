#!/usr/bin/env python3
"""Convierte las notas manuales de ref/zx/notas/*.txt en un .ctl de SkoolKit.
 bloques.txt      : 'T DIR [,long,elem]'  (T = c b w t s) tipo de bloque. Linea con '#' = comentario.
 rutinas.txt      : 'DIR|ETIQUETA|Titulo|Descripcion' (usar ' // ' para separar parrafos; etiqueta puede ir vacia)
 variables.txt    : 'DIR|ETIQUETA|Titulo|Descripcion' para datos (el bloque debe estar en bloques.txt)
 datos.txt        : 'DIR|ETIQUETA|LONG|ELEM|comentario' subbloque de datos con comentario (prefijo 'W:' o 'T:' en DIR para el tipo)
 comentarios.txt  : 'DIR,LONG|comentario' comentario de subbloque de instrucciones
Uso: hq_notes2ctl.py SUBDIR [auto.ctl]   (SUBDIR dentro de ref/zx/notas)
Salida: stdout."""
import sys, os, re
d = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'ref', 'zx', 'notas', sys.argv[1])
def lines(n):
    p = os.path.join(d, n)
    if not os.path.exists(p): return []
    return [l.rstrip('\n') for l in open(p, encoding='utf-8') if l.strip() and not l.lstrip().startswith('#')]
auto = sys.argv[2] if len(sys.argv) > 2 else None
excl = {int(l.split()[0], 16) for l in lines('excluir.txt')} if os.path.exists(os.path.join(d, 'excluir.txt')) else set()
out = []
man = []   # (direccion, tipo) de bloques manuales
for n in ('bloques.txt', 'subbloques.txt'):
    for l in lines(n):
        f = l.split()
        if len(f) >= 2 and f[0] in 'cbwtsi' and f[1].startswith('$') and ',' not in f[1]:
            man.append((int(f[1][1:], 16), f[0]))
for n in ('rutinas.txt', 'variables.txt'):
    for l in lines(n):
        a = l.split('|')[0].strip()
        t = 'c' if n == 'rutinas.txt' else 'b'
        if ':' in a: t, a = a.split(':', 1)
        man.append((int(a, 16), t))
man.sort()
if auto:
    for l in open(auto):
        f = l.split()
        if len(f) >= 2 and f[0] in 'cbwtsi' and f[1].startswith('$'):
            a = int(f[1][1:], 16)
            if f[0] == 'i': out.append(l.rstrip('\n')); continue
            if a in excl: continue
            # bloque manual que contiene a: ultimo inicio manual < a
            prev = [m for m in man if m[0] < a]
            if prev and prev[-1][1] in 'bwts' : continue          # dentro de datos manuales: se descarta
            if prev and prev[-1][1] == 'c' and f[0] != 'c': continue
            if a in [m[0] for m in man]: continue                    # el manual manda
        out.append(l.rstrip('\n'))
for l in lines('bloques.txt'):
    out.append(l)
for l in lines('subbloques.txt'):
    out.append(l)
def emit(kind, l):
    f = l.split('|')
    a = f[0].strip(); lab = f[1].strip(); t = f[2].strip(); ds = f[3].strip() if len(f) > 3 else ''
    # cabecera: el tipo de bloque lo pone bloques.txt o la regla por defecto
    if ':' in a: kind, a = a.split(':', 1)
    out.append('%s $%s %s' % (kind, a, t))
    if lab: out.append('@ $%s label=%s' % (a, lab))
    for p in [x.strip() for x in ds.split(' // ') if x.strip()]:
        out.append('D $%s %s' % (a, p))
for l in lines('rutinas.txt'): emit('c', l)
for l in lines('variables.txt'): emit('b', l)
for l in lines('datos.txt'):
    f = l.split('|')
    a, lab, ln, el = f[0].strip(), f[1].strip(), f[2].strip(), f[3].strip()
    t = 'B'
    if ':' in a: t, a = a.split(':', 1)
    out.append('%s $%s,%s%s %s' % (t, a, ln, (',' + el) if el else '', f[4].strip()))
    if lab: out.append('@ $%s label=%s' % (a, lab))
for l in lines('comentarios.txt'):
    a, c = l.split('|', 1)
    out.append('C $%s %s' % (a.strip().replace(',', ',', 1), c.strip()))
print('\n'.join(out))
