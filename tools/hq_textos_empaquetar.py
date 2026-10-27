#!/usr/bin/env python3
"""Empaqueta los textos del juego en data/textos_bin (formato para el QL, big endian) y escribe en build/datos/
el indice y las constantes (textos_ids.h). Lee asset/textos_ql/textos.es.txt (o textos.en.txt con HQ_VERSION=en; el nombre antiguo textos.txt vale para el espanol)
si existe (version traducida o retocada) y, si no, asset/textos/textos.<idioma>.txt (original). Las longitudes pueden cambiar: el binario guarda los textos sin relleno,
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
def parsear(ruta, items):
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
def pares_ventanas():
    """asset/textos_ql/equivalencia_en_es.txt: ventana o guion inglesa -> ventana espanola equivalente (None = [NEW]: solo existe en ingles)"""
    ruta = os.path.join(ROOT, 'asset/textos_ql/equivalencia_en_es.txt'); d = {}
    for n, l in enumerate(open(ruta, encoding='utf-8').read().split('\n'), 1):
        m = re.match(r'\[((?:VENT|GUION|MENU)_\w+)\]\s+\[(\w+)\]\s*$', l)
        if m: d[m.group(1)] = None if m.group(2) == 'NEW' else m.group(2)
    return ruta, d
def leer_fuente():
    """elementos del juego: los de la version espanola (sus ids no cambian), los textos propios del port y, al final, las ventanas y los
    textos de las pociones de la version inglesa con el prefijo EN_ (las zonas de clic llevan la accion de la ventana espanola equivalente)"""
    global RETOCADO; RETOCADO = False
    items = []
    for nombre in ('textos', 'textos_port'):                                    # el juego y los textos propios del port a QL
        q, o = T.fuente('textos', nombre, 'es')
        ruta = q if os.path.exists(q) else o
        RETOCADO = RETOCADO or (ruta == q and (not os.path.exists(o) or T.distintos(q, o)))
        parsear(ruta, items)
    q, o = T.fuente('textos', 'textos', 'en'); ruta = q if os.path.exists(q) else o
    RETOCADO = RETOCADO or (ruta == q and (not os.path.exists(o) or T.distintos(q, o)))
    en = []; parsear(ruta, en)
    q, o = T.fuente('textos', 'textos_port', 'en'); rutaq = q if os.path.exists(q) else o
    parsear(rutaq, en)
    rutap, pares = pares_ventanas()
    es = dict((it['id'], it) for it in items)
    def zonas(it): return [l.split('|') for n, l in it['lineas'] if l.startswith('zona|')]
    acc = {}                       # accion de una zona en el codigo ingles -> en el espanol (se deduce de las ventanas emparejadas con las mismas zonas)
    for it in en:
        if it['tipo'] == 'ventana' and pares.get(it['id']) in es:
            ze, zs = zonas(it), zonas(es[pares[it['id']]])
            if len(ze) == len(zs):
                for a, b in zip(ze, zs):
                    if acc.setdefault(a[5], b[5]) != b[5]: raise SystemExit('ERROR: la accion %s del codigo ingles equivale a dos del espanol (%s y %s)' % (a[5], acc[a[5]], b[5]))
    for it in en:
        if it['tipo'] == 'ventana' and not it['id'].startswith('PORT_'):
            if it['id'] not in pares: raise SystemExit('ERROR: %s: falta la ventana inglesa %s' % (os.path.basename(rutap), it['id']))
            eq = pares[it['id']]
            if eq is not None and eq not in es: raise SystemExit('ERROR: %s: la ventana espanola %s no existe' % (os.path.basename(rutap), eq))
            nl = []
            for n, l in it['lineas']:
                if l.startswith('zona|'):
                    p = l.split('|')
                    if p[5] not in acc: raise SystemExit('ERROR: la ventana %s tiene una zona con la accion %s, que no esta en ninguna ventana emparejada' % (it['id'], p[5]))
                    p[5] = acc[p[5]]; l = '|'.join(p)
                nl.append((n, l))
            it['lineas'] = nl
        elif it['tipo'] == 'guion':
            if it['id'] not in pares: raise SystemExit('ERROR: %s: falta el guion ingles %s' % (os.path.basename(rutap), it['id']))
            if pares[it['id']] is not None and pares[it['id']] not in es: raise SystemExit('ERROR: %s: el guion espanol %s no existe' % (os.path.basename(rutap), pares[it['id']]))
        it['id'] = 'EN_' + it['id']                       # tablas de cadenas, indices y textos del port: mismo nombre que el espanol
        items.append(it)
    for v in es:
        if v.startswith(('VENT_', 'GUION_', 'MENU_')) and v not in pares.values(): raise SystemExit('ERROR: %s: la ventana espanola %s no tiene equivalente ingles' % (os.path.basename(rutap), v))
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
    pos = dict((x, i) for i, x in enumerate(ids)); nes = min(i for i, x in enumerate(ids) if x.startswith('EN_'))
    ven = dict((v, e) for e, v in pares_ventanas()[1].items() if v)
    mapa = []
    for x in ids[:nes]:
        en_id = 'EN_' + ven[x] if x in ven else 'EN_' + x
        mapa.append(pos.get(en_id, -1))
    with open(os.path.join(ROOT, 'build/datos/idioma_en.h'), 'w') as f:
        f.write('/* generado por tools/hq_textos_empaquetar.py: elemento de textos_bin con la version inglesa de cada elemento espanol (-1 = no la tiene) */\n')
        f.write('#define IDIOMA_EN_N %d\n#define IDIOMA_EN_MAP { %s }\n' % (len(mapa), ', '.join(str(x) for x in mapa)))
    with open(os.path.join(ROOT, 'build/datos/indice_textos.txt'), 'w') as f:
        f.write('numero id tipo desplazamiento bytes\n')
        for i, (it, b, al, cant) in enumerate(cuerpos):
            f.write('%3d %-28s %-8s %6d %5d\n' % (i, it['id'], it['tipo'], 8 + 8 * N + sum(len(c[1]) for c in cuerpos[:i]), len(b)))
    print('%s: %d elementos, %d bytes (%s)' % ('data/textos_bin', N, len(blob), 'retocado: asset/textos_ql' if RETOCADO else 'original'))
    return ruta, items
if __name__ == '__main__':
    empaquetar('--permitir-excesos' in sys.argv)
