#!/usr/bin/env python3
"""Genera docs/ZX_INVENTARIO.md con las rutinas documentadas (ref/zx/notas/*/rutinas.txt)."""
import os
R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sec = (('main', 'Memoria fija (GAME.BIN, 0x6400-0xFFFD, banco 0 en 0xC000)', 'heroquest_main.skool'),
       ('overlay', 'Overlay del menú (banco 6 0xDB58 -> 0x5B00)', 'heroquest_overlay_menu.skool'),
       ('sonido', 'Reproductor de sonido (banco 4, 0xC000)', 'heroquest_banco4_sonido.skool'))
o = ['# Hero Quest ZX — inventario de rutinas\n', 'Generado por `tools/hq_inventario.py` a partir de `ref/zx/notas/*/rutinas.txt`. Los detalles están en el `.skool` correspondiente (`ref/zx/`).\n']
for d, t, f in sec:
    o.append('\n## %s\n\nFichero: `ref/zx/%s`\n\n| Dirección | Etiqueta | Rutina |\n|---|---|---|' % (t, f))
    for l in open(os.path.join(R, 'ref/zx/notas', d, 'rutinas.txt'), encoding='utf-8'):
        if not l.strip() or l.startswith('#'): continue
        p = l.rstrip('\n').split('|')
        a = p[0].split(':')[-1]
        o.append('| `0x%s` | %s | %s |' % (a, ('`%s`' % p[1]) if p[1] else '', p[2]))
open(os.path.join(R, 'docs/ZX_INVENTARIO.md'), 'w', encoding='utf-8').write('\n'.join(o) + '\n')
