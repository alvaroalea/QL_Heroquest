#!/usr/bin/env python3
"""Empaqueta los textos del juego en data/textos_bin (formato para el QL, big endian) y escribe en build/datos/
el indice y las constantes (textos_ids.h). Lee asset/textos_ql/textos.txt si existe (version traducida o retocada)
y, si no, asset/textos/textos.txt (original). Las longitudes pueden cambiar: el binario guarda los textos sin relleno,
con su alineacion; las ventanas guardan el ancho minimo y la columna central, para que el juego las recalcule.
Formato del binario:
  +0 'HQTX'  +4 version (word, 1)  +6 numero de elementos N (word)  +8 N entradas de 8 bytes
     entrada: desplazamiento desde el inicio del fichero (long), tipo (byte), alineacion de la tabla (byte: 'L','C','R' o 0), cantidad (word)
  tipo 1 VENTANA : y, centro, ancho_minimo, nlineas, nzonas, 0 ;  nlineas x [alineacion(byte 'L','C','R'), longitud, caracteres] ;
                   nzonas x [x1, x2, y1, y2, accion(word)]   (las zonas son las del original: dependen de la maquetacion)
  tipo 2 GUION   : nops (word) ; ops: 1 attr alin x y long caracteres | 2 lo hi ancho alto (caja) | 3 (modo OR) ; 0 = fin
  tipo 3 CADENAS : n (byte), campo (byte), alineacion (byte), 0 ; n x palabra con el desplazamiento de cada cadena desde el inicio del elemento ;
                   cada cadena = [longitud, caracteres]
  tipo 4 INDICES : n (byte), 0 ; n x [a, b]
  Los caracteres son codigos del juego (glifo = codigo - 0x30; 0x5F = espacio). Cada elemento se alinea a palabra.
Uso: hq_textos_empaquetar.py [--permitir-excesos]"""
import sys, os, re, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hq_txt_lib as T
ROOT = T.ROOT
ALINS = ('L', 'C', 'R'); RETOCADO = False
def leer_fuente():
    global RETOCADO; RETOCADO = False
    items = []
    for nombre in ('textos.txt', 'textos_port.txt', 'textos_combate.txt'):      # el juego y los textos propios del port a QL
        q = os.path.join(ROOT, 'asset/textos_ql', nombre); o = os.path.join(ROOT, 'asset/textos', nombre)
        ruta = q if os.path.exists(q) else o
        RETOCADO = RETOCADO or (ruta == q and (not os.path.exists(o) or open(q, 'rb').read() != open(o, 'rb').read()))
        cur = None
        for n, l in enumerate(open(ruta, encoding='utf-8').read().split('\n'), 1):
            if l.startswith('#') or not l.strip():
                continue
            mt = re.match(r'\[(\w+)\]\s+(\w+)\s*(.*)$', l)
            if mt:
                attrs = dict(re.findall(r'(\w+)=(\S+)', mt.group(3)))
                cur = dict(id=mt.group(1), tipo=mt.group(2), attrs=attrs, lineas=[], n=n, ruta=ruta); items.append(cur); continue
            if cur is None: raise SystemExit('%s:%d: linea fuera de un elemento' % (ruta, n))
            cur['lineas'].append((n, l))
    return ruta, items
def err(ruta, n, msg): raise ValueError('%s:%d: %s' % (os.path.basename(ruta), n, msg))
def empaquetar(permitir):
    ruta, items = leer_fuente(); cuerpos = []; ids = []; avisos = []
    for it in items:
        t = it['tipo']; a = it['attrs']; b = bytearray(); alin_t = 0; cant = 0
        try:
            if t == 'ventana':
                lines = []; zonas = []
                for n, l in it['lineas']:
                    if l.startswith('zona|'):
                        p = l.split('|')[1:]; zonas.append((int(p[0]), int(p[1]), int(p[2]), int(p[3]), int(p[4], 0)))
                    else:
                        al, tx = l.split('|', 1)
                        if al not in ALINS: err(ruta, n, 'alineacion invalida (L, C o R)')
                        lines.append((al, T.codificar(tx)))
                ancho = max([int(a['ancho'])] + [len(x[1]) for x in lines])
                if ancho > 30 and not permitir: err(ruta, it['n'], 'ventana %s de %d columnas (maximo 30)' % (it['id'], ancho))
                if len(lines) > 20: err(ruta, it['n'], 'demasiadas lineas')
                b += bytes([int(a['y']), int(a['centro']), int(a['ancho']), len(lines), len(zonas), 0])
                for al, bs in lines: b += bytes([ord(al), len(bs)]) + bs
                for z in zonas: b += struct.pack('>BBBBH', *z)
                cant = len(lines)
            elif t == 'guion':
                ops = bytearray(); nops = 0
                for n, l in it['lineas']:
                    if l == 'or': ops += b'\x03'; nops += 1
                    elif l.startswith('caja|'): ops += bytes([2] + [int(x) for x in l.split('|')[1:]]); nops += 1
                    else:
                        at, al, x, y, tx = l.split('|', 4); bs = T.codificar(tx)
                        if al not in ALINS: err(ruta, n, 'alineacion invalida')
                        if len(bs) > 32: err(ruta, n, 'linea de mas de 32 caracteres (ancho de la pantalla)')
                        ops += bytes([1, int(at, 16), ord(al), int(x), int(y), len(bs)]) + bs; nops += 1
                b += struct.pack('>H', nops) + ops + b'\x00'; cant = nops
            elif t == 'cadenas':
                campo = int(a['campo']); alin_t = ord(a.get('alineacion', 'L')); ss = []
                for n, l in it['lineas']:
                    bs = b'' if l == '-' else T.codificar(l)
                    if campo and len(bs) > campo and not permitir: err(ruta, n, 'cadena de %d caracteres en un campo de %d (%s)' % (len(bs), campo, it['id']))
                    ss.append(bs)
                tab = 4 + 2 * len(ss); offs = []; datos = bytearray()
                for bs in ss: offs.append(tab + len(datos)); datos += bytes([len(bs)]) + bs
                b += bytes([len(ss), campo, alin_t, 0]) + b''.join(struct.pack('>H', o) for o in offs) + datos; cant = len(ss)
            elif t == 'indices':
                pares = [tuple(int(x) for x in l.split('|')) for n, l in it['lineas']]
                b += bytes([len(pares), 0]) + b''.join(bytes(p) for p in pares); cant = len(pares)
            else: err(ruta, it['n'], 'tipo desconocido ' + t)
        except ValueError as e:
            raise SystemExit('ERROR: %s' % e)
        if len(b) % 2: b += b'\x00'
        cuerpos.append((it, bytes(b), alin_t, cant)); ids.append(it['id'])
    N = len(cuerpos); off = 8 + 8 * N; tabla = b''; datos = b''; tipos = {'ventana': 1, 'guion': 2, 'cadenas': 3, 'indices': 4}
    for it, b, al, cant in cuerpos:
        tabla += struct.pack('>LBBH', off + len(datos), tipos[it['tipo']], al, cant); datos += b
    blob = b'HQTX' + struct.pack('>HH', 1, N) + tabla + datos
    os.makedirs(os.path.join(ROOT, 'data'), exist_ok=True); os.makedirs(os.path.join(ROOT, 'build/datos'), exist_ok=True)
    open(os.path.join(ROOT, 'data/textos_bin'), 'wb').write(blob)
    with open(os.path.join(ROOT, 'build/datos/textos_ids.h'), 'w') as f:
        f.write('/* generado por tools/hq_textos_empaquetar.py: numero de cada elemento de data/textos_bin */\n#ifndef TEXTOS_IDS_H\n#define TEXTOS_IDS_H\n')
        for i, x in enumerate(ids): f.write('#define TX_%s %d\n' % (x, i))
        f.write('#define TX_NUM %d\n#endif\n' % N)
    with open(os.path.join(ROOT, 'build/datos/indice_textos.txt'), 'w') as f:
        f.write('numero id tipo desplazamiento bytes\n')
        for i, (it, b, al, cant) in enumerate(cuerpos):
            f.write('%3d %-28s %-8s %6d %5d\n' % (i, it['id'], it['tipo'], 8 + 8 * N + sum(len(c[1]) for c in cuerpos[:i]), len(b)))
    print('%s: %d elementos, %d bytes (%s)' % ('data/textos_bin', N, len(blob), 'retocado: asset/textos_ql/textos.txt' if RETOCADO else 'original'))
    return ruta, items
if __name__ == '__main__':
    empaquetar('--permitir-excesos' in sys.argv)
