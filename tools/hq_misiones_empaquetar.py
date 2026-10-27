#!/usr/bin/env python3
"""Empaqueta las misiones en los dos idiomas: un fichero data/mision_NN_es_bin y otro mision_NN_en_bin por mision y
data/misiones_indice_es_bin y misiones_indice_en_bin con el indice.
Lee asset/misiones_ql/misionNN.<idioma>.txt (el nombre antiguo misionNN.txt vale para el espanol) si existe
(mision retocada o personalizada) y, si no, asset/misiones/misionNN.<idioma>.txt.
Formato (big endian):
 mision_NN_<idioma>_bin:
  +0 'HQMS'  +4 version (word, 1)  +6 numero de mision (word)
  +8 4 heroes x [columna, fila, grafico]  (12 bytes)
  +20 numero de monstruos (byte), numero de objetos (byte), 0, 0
  +24 desplazamientos desde el inicio del fichero (6 longs): mapa capa A, mapa capa B, indicadores, monstruos, objetos, objetivo
  +48 datos: capa A (26 x 19 = 494 bytes), capa B (494), indicadores (86), monstruos (n x 10), objetos (n x 3: columna, fila, codigo),
      objetivo (formato de guion de textos_bin: nops (word), operaciones 1 attr alin x y long caracteres | 2 caja | 3 modo OR, 0)
  cada seccion se alinea a palabra. El mapa es de 26 columnas x 19 filas, fila a fila.
 misiones_indice_<idioma>_bin:
  'HQMI', version (word, 1), numero de misiones N (word) y N entradas de 40 bytes:
  numero (word), tamano del fichero (long), nombre (byte de longitud + 33 bytes con caracteres del juego, rellenos con 0)
Los ficheros se llaman mision_00_es_bin ... mision_13_en_bin (el QL no admite puntos en los nombres).
Uso: hq_misiones_empaquetar.py"""
import sys, os, re, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hq_txt_lib as T
ROOT = T.ROOT
def leer(n, ver):
    q, o = T.fuente('misiones', 'mision%02d' % n, ver)
    ruta = q if os.path.exists(q) else o
    d = dict(nombre='', heroes=[], mapa=[], flags=[], monstruos=[], objetos=[], objetivo=[], ruta=ruta, mision=n)
    sec = None
    for k, l in enumerate(open(ruta, encoding='utf-8').read().split('\n'), 1):
        def err(m): raise SystemExit('ERROR: %s:%d: %s' % (os.path.basename(ruta), k, m))
        if not l.strip() or l.startswith('#'): continue
        if l.startswith('mision '): d['num'] = int(l.split()[1]); continue
        if l.startswith('nombre '): d['nombre'] = l[7:].strip(); continue
        if l.startswith('heroe '):
            p = l[6:].split('|'); d['heroes'].append(tuple(int(x) for x in p[1:4])); continue
        mt = re.match(r'\[(\w+)\]', l)
        if mt: sec = mt.group(1); continue
        l = l.split(' ;')[0]
        try:
            if sec == 'mapa': d['mapa'].append([tuple(int(h, 16) for h in c.split('.')) for c in l.split()])
            elif sec == 'indicadores': d['flags'] += [int(h, 16) for h in l.split()]
            elif sec == 'monstruos': d['monstruos'].append(tuple(int(x) for x in l.split('|')[1:])); assert len(d['monstruos'][-1]) == 10
            elif sec == 'objetos': d['objetos'].append(tuple(int(x) for x in l.split('|')[1:])); assert len(d['objetos'][-1]) == 3
            elif sec == 'objetivo': at, al, x, y, tx = l.split('|', 4); d['objetivo'].append((int(at, 16), al, int(x), int(y), tx))
            else: err('linea fuera de seccion')
        except (ValueError, AssertionError, IndexError): err('linea mal formada: ' + l[:40])
    d['retocada'] = ruta == q and (not os.path.exists(o) or T.distintos(q, o))
    return d
def validar(d):
    def e(m): raise SystemExit('ERROR: %s: %s' % (os.path.basename(d['ruta']), m))
    if len(d['heroes']) != 4: e('hacen falta 4 lineas heroe')
    if len(d['mapa']) != 19 or any(len(f) != 26 for f in d['mapa']): e('el mapa debe tener 19 filas de 26 casillas AA.BB')
    if len(d['flags']) != 86: e('los indicadores deben ser 86 bytes (hay %d)' % len(d['flags']))
    if len(d['monstruos']) > 25: e('maximo 25 monstruos (hay %d)' % len(d['monstruos']))
    if len(d['objetos']) > 255: e('demasiados objetos')
    for x, y, c in d['objetos']:
        if not (0 <= x < 26 and 0 <= y < 19): e('objeto fuera del mapa (%d,%d)' % (x, y))
    for h in d['heroes']:
        if not (0 <= h[0] < 26 and 0 <= h[1] < 19): e('heroe fuera del mapa')
def par(b):
    return b + (b'\x00' if len(b) % 2 else b'')
def empaquetar(d):
    validar(d); n = d['mision']
    A = bytes(c[0] for f in d['mapa'] for c in f); B = bytes(c[1] for f in d['mapa'] for c in f)
    mon = b''.join(bytes(m_) for m_ in d['monstruos']); obj = b''.join(bytes(o) for o in d['objetos'])
    ops = bytearray(); nops = 0
    for at, al, x, y, tx in d['objetivo']:
        try: bs = T.codificar(tx)
        except ValueError as ex: raise SystemExit('ERROR: %s: %s' % (os.path.basename(d['ruta']), ex))
        if len(bs) > 32: raise SystemExit('ERROR: %s: linea del objetivo de mas de 32 caracteres' % os.path.basename(d['ruta']))
        ops += bytes([1, at, ord(al), x, y, len(bs)]) + bs; nops += 1
    ob = struct.pack('>H', nops) + bytes(ops) + b'\x00'
    secs = [A, B, bytes(d['flags']), mon, obj, ob]; off = 48; offs = []
    for s in secs: offs.append(off); off += len(par(s))
    hdr = b'HQMS' + struct.pack('>HH', 1, n) + b''.join(bytes(h) for h in d['heroes']) + bytes([len(d['monstruos']), len(d['objetos']), 0, 0]) + struct.pack('>6L', *offs)
    return hdr + b''.join(par(s) for s in secs)
def main():
    os.makedirs(os.path.join(ROOT, 'data'), exist_ok=True); os.makedirs(os.path.join(ROOT, 'build/datos'), exist_ok=True)
    for ver in ('es', 'en'):
        idx = bytearray(b'HQMI' + struct.pack('>HH', 1, 14)); txt = ['numero fichero bytes nombre']; usadas = 0
        for n in range(14):
            d = leer(n, ver); blob = empaquetar(d); usadas += d['retocada']
            fich = 'mision_%02d_%s_bin' % (n, ver)
            open(os.path.join(ROOT, 'data', fich), 'wb').write(blob)
            nom = T.codificar(d['nombre'])
            if len(nom) > 33: raise SystemExit('ERROR: nombre de mision demasiado largo (maximo 33)')
            idx += struct.pack('>HL', n, len(blob)) + bytes([len(nom)]) + nom + bytes(33 - len(nom))
            txt.append('%2d %s %5d %s%s' % (n, fich, len(blob), d['nombre'], '  (retocada)' if d['retocada'] else ''))
        open(os.path.join(ROOT, 'data/misiones_indice_%s_bin' % ver), 'wb').write(bytes(idx))
        open(os.path.join(ROOT, 'build/datos/indice_misiones_%s.txt' % ver), 'w').write('\n'.join(txt) + '\n')
        print('\n'.join(txt)); print('data/misiones_indice_%s_bin: %d bytes; misiones retocadas: %d' % (ver, len(idx), usadas))
if __name__ == '__main__':
    main()
