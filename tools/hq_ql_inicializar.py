#!/usr/bin/env python3
"""Copia a las carpetas de versiones retocadas los ficheros originales que todavia no existan alli (NUNCA sobrescribe):
  asset/graficos/*.png          -> asset/graficos_ql/
  asset/textos/textos.es.txt y textos.en.txt -> asset/textos_ql/  (los textos del port, textos_port.es.txt y .en.txt, viven solo en asset/textos_ql/)
  asset/misiones/misionNN.es.txt y misionNN.en.txt -> asset/misiones_ql/
Esas carpetas son las de las versiones retocadas, traducidas o personalizadas: ninguna herramienta de extraccion las toca.
Los textos y misiones de la version espanola se llamaban antes textos.txt y misionNN.txt: si existe la copia con el nombre antiguo
cuenta como existente (no se crea otra con el original, que taparia los cambios).
Uso: hq_ql_inicializar.py [--estado]"""
import sys, os, re, shutil
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOTS = (('asset/graficos', 'asset/graficos_ql', '.png'), ('asset/textos', 'asset/textos_ql', '.txt'), ('asset/misiones', 'asset/misiones_ql', '.txt'))
nuevos = 0; igual = 0; ya = 0
def cuerpo(r):
    """contenido sin las lineas de comentario de los .txt (cambian al regenerar el original)"""
    d = open(r, 'rb').read()
    return d if r.endswith('.png') else [l for l in d.split(b'\n') if not l.startswith(b'#')]
for orig, dest, ext in ROOTS:
    ORIG = os.path.join(ROOT, orig); DEST = os.path.join(ROOT, dest)
    for ruta, dirs, ficheros in os.walk(ORIG):
        for f in sorted(ficheros):
            if not f.endswith(ext) or (orig == 'asset/misiones' and not f.startswith('mision')) or (orig == 'asset/textos' and not re.match(r'textos(\.(es|en))?\.txt$', f)): continue
            rel = os.path.relpath(os.path.join(ruta, f), ORIG); d = os.path.join(DEST, rel)
            if not os.path.exists(d) and f.endswith('.es.txt'): d = os.path.join(DEST, os.path.dirname(rel), f[:-7] + '.txt')      # nombre antiguo (sin idioma) de la version espanola
            if os.path.exists(d):
                ya += 1; igual += cuerpo(d) == cuerpo(os.path.join(ORIG, rel))
                continue
            d = os.path.join(DEST, rel)
            if '--estado' in sys.argv: print('falta:', os.path.join(dest, rel)); nuevos += 1; continue
            os.makedirs(os.path.dirname(d), exist_ok=True); shutil.copyfile(os.path.join(ORIG, rel), d); nuevos += 1
print('%s: %d nuevos, %d ya existian (%d sin retocar, %d retocados)' % ('pendientes' if '--estado' in sys.argv else 'copiados', nuevos, ya, igual, ya - igual))
