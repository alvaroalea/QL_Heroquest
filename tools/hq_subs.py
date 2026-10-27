#!/usr/bin/env python3
"""Genera ref/zx/notas/main/subbloques.txt (subbloques de datos tipados) y los volcados de texto de asset/textos/.
Usa los analizadores de guiones, ventanas y cadenas. Ejecutar tras cambiar bloques.txt."""
import sys, os, subprocess
R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PY = os.path.join(R, 'tmp/venv/bin/python')
def run(*a):
    r = subprocess.run([PY, os.path.join(R, 'tools', a[0])] + list(a[1:]), capture_output=True, text=True)
    return r.stdout
mode = sys.argv[1] if len(sys.argv) > 1 else 'main'
out = []
def blk(t, a, comment=''):
    out.append('%s $%04X%s' % (t, a, (' ' + comment) if comment else ''))
if mode == 'overlay':
    blk('b', 0x6561); out += run('hq_scripts.py', '0x6561', '0x65D5', 'ctl').split('\n')
    out += run('hq_scripts.py', '0x65E2', '0x65EC', 'ctl').split('\n')
    blk('b', 0x6653); out += run('hq_scripts.py', '0x6653', '0x6B1A', 'ctl').split('\n')
    out.append('W $6B1A,16')
    out += run('hq_scripts.py', '0x6B2A', '0x6C1B', 'ctl').split('\n')
    open(os.path.join(R, 'ref/zx/notas/overlay/subbloques.txt'), 'w').write('\n'.join(l for l in out if l.strip() and not l.startswith('#')) + '\n')
    g = run('hq_scripts.py', '0x6561', '0x65D5', 'txt') + run('hq_scripts.py', '0x6653', '0x6B1A', 'txt') + run('hq_scripts.py', '0x6B2A', '0x6C1B', 'txt')
    open(os.path.join(R, 'asset/textos/guiones_menu.txt'), 'w').write('Guiones de texto del overlay del menu (0x5B00)\n\n' + g)
    sys.exit(0)
# --- 0x9739: rotulos del panel de estado (punteros + cadenas acabadas en 0)
blk('b', 0x9739); out += run('hq_strings.py', '0x9739', '0x98F6', '0x9700', '0x9900').split('\n')
# --- 0xA438: guiones de texto del panel y de turnos
blk('b', 0xA438)
out += run('hq_scripts.py', '0xA438', '0xA476', 'ctl').split('\n')
out.append('W $A476,10')
out += run('hq_scripts.py', '0xA480', '0xA4BB', 'ctl').split('\n')
out.append('B $A4BB,66,3')
# --- 0xAA8C...: tablas de datos de personajes (a mano, ver bloques.txt) y ventanas de texto
blk('b', 0xAC50)
out += run('hq_textos.py', 'ctl', '0xAC50', '0xAE5F').split('\n')
out.append('W $AE5F,28')
out.append('T $AE7B,60,10')
out.append('T $AEB7,174,29')
out += run('hq_textos.py', 'ctl', '0xAF65', '0xB1B0').split('\n')
blk('b', 0xB1B0); out += run('hq_scripts.py', '0xB1B0', '0xB1C0', 'ctl').split('\n')
# --- 0xB78E: guiones (menus de tienda/cajas, mensajes de combate, muerte de personajes)
blk('b', 0xB78E); out += run('hq_scripts.py', '0xB78E', '0xBB0E', 'ctl').split('\n')
out.append('W $BB0E,10')
out += run('hq_scripts.py', '0xBB18', '0xBB8E', 'ctl').split('\n')
out += run('hq_textos.py', 'ctl', '0xBB8E', '0xC1B6').split('\n')
open(os.path.join(R, 'ref/zx/notas/main/subbloques.txt'), 'w').write('\n'.join(l for l in out if l.strip() and not l.startswith('#')) + '\n')

# --- volcados de texto para docs/asset
os.makedirs(os.path.join(R, 'asset/textos'), exist_ok=True)
t = ''
for a, b in (('0xAC50', '0xAE5F'), ('0xAF65', '0xB1B0'), ('0xBB8E', '0xC1B6')):
    t += run('hq_textos.py', 'txt', a, b)
open(os.path.join(R, 'asset/textos/ventanas.txt'), 'w').write('Ventanas de texto (X, Y, ancho x alto; ' + "'zona' = area activa con su rutina)\n\n" + t)
g = ''
for a, b in (('0xA438', '0xA476'), ('0xA480', '0xA4BB'), ('0xB78E', '0xBB0E'), ('0xBB18', '0xBB8E')):
    g += run('hq_scripts.py', a, b, 'txt')
open(os.path.join(R, 'asset/textos/guiones.txt'), 'w').write('Guiones de texto (interpretados por 0x8BF9)\n\n' + g)
