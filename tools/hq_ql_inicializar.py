#!/usr/bin/env python3
"""Copia a las carpetas de versiones retocadas los ficheros originales que todavia no existan alli (NUNCA sobrescribe):
  asset/graficos/*.png      -> asset/graficos_ql/
  asset/textos/textos.txt, textos_port.txt y textos_combate.txt -> asset/textos_ql/
  asset/misiones/mision*.txt -> asset/misiones_ql/
Esas carpetas son las de las versiones retocadas, traducidas o personalizadas: ninguna herramienta de extraccion las toca.
Uso: hq_ql_inicializar.py [--estado]"""
import sys, os, shutil
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOTS = (('asset/graficos', 'asset/graficos_ql', '.png'), ('asset/textos', 'asset/textos_ql', '.txt'), ('asset/misiones', 'asset/misiones_ql', '.txt'))
nuevos = 0; igual = 0; ya = 0
for orig, dest, ext in ROOTS:
    ORIG = os.path.join(ROOT, orig); DEST = os.path.join(ROOT, dest)
    for ruta, dirs, ficheros in os.walk(ORIG):
        for f in sorted(ficheros):
            if not f.endswith(ext) or (orig == 'asset/misiones' and not f.startswith('mision')) or (orig == 'asset/textos' and f not in ('textos.txt', 'textos_port.txt', 'textos_combate.txt')): continue
            rel = os.path.relpath(os.path.join(ruta, f), ORIG); d = os.path.join(DEST, rel)
            if os.path.exists(d):
                ya += 1; igual += open(d, 'rb').read() == open(os.path.join(ORIG, rel), 'rb').read()
                continue
            if '--estado' in sys.argv: print('falta:', os.path.join(dest, rel)); nuevos += 1; continue
            os.makedirs(os.path.dirname(d), exist_ok=True); shutil.copyfile(os.path.join(ORIG, rel), d); nuevos += 1
print('%s: %d nuevos, %d ya existian (%d sin retocar, %d retocados)' % ('pendientes' if '--estado' in sys.argv else 'copiados', nuevos, ya, igual, ya - igual))
