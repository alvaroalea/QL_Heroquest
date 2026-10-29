#!/usr/bin/env python3
"""Empaqueta las misiones en los dos idiomas: un fichero data/heroquest_NN_es y otro heroquest_NN_en por mision y
data/heroquest_es_idx y heroquest_en_idx con el indice.
Lee asset/misiones_ql/misionNN.<idioma>.txt (el nombre antiguo misionNN.txt vale para el espanol) si existe
(mision retocada o personalizada) y, si no, asset/misiones/misionNN.<idioma>.txt.
Formato (big endian):
 heroquest_NN_<idioma>:
  +0 'HQMS'  +4 version (word, 1)  +6 numero de mision (word)
  +8 4 heroes x [columna, fila, grafico]  (12 bytes)
  +20 numero de monstruos (byte), numero de objetos (byte), banderas de eventos (word: bit n = comportamiento de la mision n de Hero Quest, EV_* de src/partida.h)
  +24 desplazamientos desde el inicio del fichero (9 longs): mapa capa A, mapa capa B, indicadores, monstruos, objetos, objetivo, rutinas de muerte, ruta de los errantes, teletransporte
  +60 datos: capa A (26 x 19 = 494 bytes), capa B (494), indicadores (86), monstruos (n x 10), objetos (n x 3: columna, fila, codigo),
      objetivo (formato de guion de textos_bin: nops (word), operaciones 1 attr alin x y long caracteres | 2 caja | 3 modo OR, 0),
      rutinas de muerte (n, y por entrada: estadistica, rutina 0 normal 1 salida 2 epilogo, nb, na, (nb + na) pares columna, fila),
      ruta de los monstruos errantes (pasos por ronda, n y n tramos: eje (bit 0: 1 fila) y sentido (bit 1: 1 suma), coordenada con la que acaba),
      teletransporte (sala que se abandona, n y n destinos de 5 bytes: sala, columna, fila, columna y fila de la puerta)
  cada seccion se alinea a palabra. El mapa es de 26 columnas x 19 filas, fila a fila.
 heroquest_<idioma>_idx (y <aventura>_<idioma>_idx: formato en indice()): cabecera de 48 bytes (version 2, numero de misiones, desplazamiento de los textos, subtitulo),
  N entradas de 40 bytes (numero, tamano, nombre) (los textos del idioma van en data/textos_<idioma>, formato 'HQTX' de hq_textos_empaquetar.py: ya no van detras del indice);
  el long del desplazamiento de los textos (+8) es ahora el de los datos de la aventura (0 = no hay), detras de las entradas: tres longs con los desplazamientos, desde su inicio, del epilogo, de los tipos de monstruo y de sus estadisticas, el epilogo (n paginas (word) y, por pagina, tamano (word) y operaciones de guion), los tipos (n y n entradas de 16 bytes: retrato, icono, sprite, longitud del nombre, clase y nombre) y las estadisticas (n y n entradas de 9 bytes: movimiento, ataque, defensa, monedas (word), a quien, objeto, mensaje, jefe)
Los ficheros se llaman heroquest_00_es ... heroquest_13_en y, para cada aventura adicional (asset/misiones/<aventura>.<idioma>.txt, p. ej. witchlord), witchlord_00_es ... (el QL no admite puntos en los nombres).
Uso: hq_misiones_empaquetar.py"""
import sys, os, re, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hq_txt_lib as T
ROOT = T.ROOT
MAX_MONS = 32                                  # N_MONS de src/partida.h (Hero Quest usa 25: las misiones de Return of the Witch Lord llevan hasta 32)
CLASES = {'N': 0, 'V': 1, 'M': 2}                        # clase de un tipo de monstruo (TIPO_* de src/partida.h)
RUTINAS = {'normal': 0, 'salida': 1, 'epilogo': 2}      # MUERTE_* de src/partida.h
def leer(n, ver, plantilla='mision%02d'):
    q, o = T.fuente('misiones', plantilla % n, ver)
    ruta = q if os.path.exists(q) else o
    d = dict(plantilla=plantilla, nombre='', heroes=[], mapa=[], flags=[], ev=None, muertes=[], errante=None, teleporte=None, monstruos=[], objetos=[], objetivo=[], ruta=ruta, mision=n)
    sec = None
    for k, l in enumerate(open(ruta, encoding='utf-8').read().split('\n'), 1):
        def err(m): raise SystemExit('ERROR: %s:%d: %s' % (os.path.basename(ruta), k, m))
        if not l.strip() or l.startswith('#'): continue
        if l.startswith('mision '): d['num'] = int(l.split()[1]); continue
        if l.startswith('nombre '): d['nombre'] = l[7:].strip(); continue
        if l.startswith('eventos'):                  # banderas de eventos: numeros de bit (0 a 15) separados por espacios; '-' o nada = ninguna
            try: d['ev'] = sum(1 << int(x) for x in l[7:].split() if x != '-')
            except ValueError: err('linea eventos mal formada (numeros de bit de 0 a 15)')
            if d['ev'] > 0xFFFF: err('las banderas de eventos son los bits 0 a 15')
            continue
        if l.startswith('muerte '):                  # muerte <estadistica> <normal|salida|epilogo> [casillas]: rutina al morir un monstruo de esa estadistica
            p = l[7:].split(' ;')[0].split()
            try:
                est = int(p[0]); rut = p[1]; resto = ' '.join(p[2:])
                if rut not in RUTINAS or (rut != 'salida' and resto): raise ValueError
                celdas = []
                if rut == 'salida':
                    g = resto.split('>')
                    if len(g) > 2: raise ValueError
                    g = ['', g[0]] if len(g) == 1 else g                      # sin '>' todo son casillas de la escalera
                    celdas = [[tuple(int(v) for v in c.split(',')) for c in s.split()] for s in g]
                    if any(len(c) != 2 for s in celdas for c in s): raise ValueError
                d['muertes'].append((est, RUTINAS[rut], celdas, k))
            except (ValueError, IndexError): err('linea muerte mal formada (muerte <estadistica> normal|salida|epilogo; salida: casillas a borrar > casillas de la escalera, columna,fila)')
            continue
        if l.startswith('teleporte'):                # teleporte <sala> <destinos>: con la bandera 13, salir de la sala lleva a un destino al azar; destino = sala,columna,fila,columna_puerta,fila_puerta
            p = l[9:].split(' ;')[0].split()
            try:
                if len(p) < 2 or d['teleporte'] is not None: raise ValueError
                dest = [tuple(int(v) for v in t_.split(',')) for t_ in p[1:]]
                if any(len(t_) != 5 for t_ in dest): raise ValueError
                d['teleporte'] = (int(p[0]), dest, k)
            except ValueError: err('linea teleporte mal formada (teleporte <sala> <destinos>, destino = sala,columna,fila,columna_puerta,fila_puerta)')
            continue
        if l.startswith('errante'):                  # errante <pasos> <tramos>: ruta de los monstruos errantes; tramo = eje (x o y), sentido (+ o -) y la coordenada con la que acaba: x-0 y-0 x+15
            p = l[7:].split(' ;')[0].split()
            try:
                if not p or d['errante'] is not None: raise ValueError
                tr = [re.fullmatch(r'([xy])([+-])(\d+)', t_) for t_ in p[1:]]
                if None in tr: raise ValueError
                d['errante'] = (int(p[0]), [((1 if m_.group(1) == 'y' else 0) | (2 if m_.group(2) == '+' else 0), int(m_.group(3))) for m_ in tr], k)
            except ValueError: err('linea errante mal formada (errante <pasos por ronda> <tramos>, tramo = x o y, + o - y la coordenada con que acaba: x-0 y-0 x+15)')
            continue
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
    if len(d['objetos']) > 255: e('demasiados objetos')
    for x, y, c in d['objetos']:
        if not (0 <= x < 26 and 0 <= y < 19): e('objeto fuera del mapa (%d,%d)' % (x, y))
    for h in d['heroes']:
        if not (0 <= h[0] < 26 and 0 <= h[1] < 19): e('heroe fuera del mapa')
    if d['teleporte']:
        sala, dest, k = d['teleporte']
        if not (0 <= sala < 64 and 1 <= len(dest) <= 6): e('linea %d: sala de 0 a 63 y de 1 a 6 destinos' % k)
        for t_ in dest:
            if not (0 <= t_[0] < 64 and 0 <= t_[1] < 26 and 0 <= t_[2] < 19 and 0 <= t_[3] < 26 and 0 <= t_[4] < 19): e('linea %d: destino fuera del mapa o de las salas %s' % (k, t_))
    if d['errante']:
        pasos, tr, k = d['errante']
        if not (1 <= pasos <= 32 and 1 <= len(tr) <= 8): e('linea %d: de 1 a 32 pasos por ronda y de 1 a 8 tramos' % k)
        for t_ in tr:
            if t_[1] >= (19 if t_[0] & 1 else 26): e('linea %d: coordenada fuera del mapa (%d)' % (k, t_[1]))
    total = 0
    for est, rut, celdas, k in d['muertes']:
        if not 0 <= est < 20: e('linea %d: la estadistica de monstruo va de 0 a 19' % k)
        if rut == 1 and (len(celdas[1]) > 4 or not celdas[1]): e('linea %d: la escalera de salida lleva de 1 a 4 casillas' % k)
        for g in celdas:
            for c in g:
                total += 1
                if not (0 <= c[0] < 26 and 0 <= c[1] < 19): e('linea %d: casilla fuera del mapa (%d,%d)' % (k, c[0], c[1]))
    if total > 32: e('las rutinas de muerte usan mas de 32 casillas')
def par(b):
    return b + (b'\x00' if len(b) % 2 else b'')
def empaquetar(d):
    validar(d); n = d['mision']
    if len(d['monstruos']) > MAX_MONS:           # el motor solo trata las MAX_MONS primeras entradas (la primera es la de relleno): las demas no salen en el mapa
        print('AVISO: %s: %d monstruos, el juego solo usa los %d primeros' % (os.path.basename(d['ruta']), len(d['monstruos']), MAX_MONS)); d['monstruos'] = d['monstruos'][:MAX_MONS]
    A = bytes(c[0] for f in d['mapa'] for c in f); B = bytes(c[1] for f in d['mapa'] for c in f)
    mon = b''.join(bytes(m_) for m_ in d['monstruos']); obj = b''.join(bytes(o) for o in d['objetos'])
    ops = bytearray(); nops = 0
    for at, al, x, y, tx in d['objetivo']:
        try: bs = T.codificar(tx)
        except ValueError as ex: raise SystemExit('ERROR: %s: %s' % (os.path.basename(d['ruta']), ex))
        if len(bs) > 32: raise SystemExit('ERROR: %s: linea del objetivo de mas de 32 caracteres' % os.path.basename(d['ruta']))
        ops += bytes([1, at, ord(al), x, y, len(bs)]) + bs; nops += 1
    ob = struct.pack('>H', nops) + bytes(ops) + b'\x00'
    mu = bytearray([len(d['muertes'])])          # rutinas de muerte: n; por entrada estadistica, rutina, casillas a borrar, casillas de escalera y los pares (columna, fila)
    for est, rut, celdas, k in d['muertes']:
        g = celdas if rut == 1 else [[], []]
        mu += bytes([est, rut, len(g[0]), len(g[1])]) + bytes(v for c in g[0] + g[1] for v in c)
    er = bytes([0, 0]) if not d['errante'] else bytes([d['errante'][0], len(d['errante'][1])]) + bytes(v for t_ in d['errante'][1] for v in t_)    # ruta de los errantes: pasos, n y n tramos (eje y sentido, destino)
    tp = bytes([0, 0]) if not d['teleporte'] else bytes([d['teleporte'][0], len(d['teleporte'][1])]) + bytes(v for t_ in d['teleporte'][1] for v in t_)       # teletransporte: sala, n y n destinos de 5 bytes
    secs = [A, B, bytes(d['flags']), mon, obj, ob, bytes(mu), er, tp]; off = 60; offs = []
    for s in secs: offs.append(off); off += len(par(s))
    ev = d['ev'] or 0                            # banderas de la linea 'eventos' del texto de la mision (sin la linea, ninguna)
    hdr = b'HQMS' + struct.pack('>HH', 1, n) + b''.join(bytes(h) for h in d['heroes']) + bytes([len(d['monstruos']), len(d['objetos'])]) + struct.pack('>H', ev) + struct.pack('>9L', *offs)
    return hdr + b''.join(par(s) for s in secs)
def aventura(pref, ver):
    """datos de una aventura adicional (asset/misiones[_ql]/<pref>.<idioma>.txt: linea 'subtitulo ...') y numero de misiones (<pref>_NN.<idioma>.txt consecutivas desde 00)"""
    q, o = T.fuente('misiones', pref, ver); ruta = q if os.path.exists(q) else o
    sub = ''
    for l in open(ruta, encoding='utf-8').read().split('\n'):
        if l.startswith('subtitulo '): sub = l[10:].strip()
    n = 0
    while any(os.path.exists(r) for r in T.fuente('misiones', '%s_%02d' % (pref, n), ver)): n += 1
    return sub, n
def epilogo(pref, ver):
    """el epilogo de una aventura (seccion [epilogo] de asset/misiones[_ql]/<pref>.<idioma>.txt: lineas 'pagina' y, en cada pagina, atributo(hex)|alineacion|columna|fila|texto como en [objetivo]) en el
    formato del indice: n paginas (word) y, por pagina, su tamano (word) y sus operaciones de guion (las de [objetivo]); None si no tiene"""
    q, o = T.fuente('misiones', pref, ver); ruta = q if os.path.exists(q) else o
    pag = []; sec = None
    for k, l in enumerate(open(ruta, encoding='utf-8').read().split('\n'), 1):
        if not l.strip() or l.startswith('#'): continue
        mt = re.match(r'\[(\w+)\]', l)
        if mt: sec = mt.group(1); continue
        if sec != 'epilogo': continue
        if l.strip() == 'pagina': pag.append([]); continue
        try:
            at, al, x, y, tx = l.split('|', 4); bs = T.codificar(tx)
            if not pag or al not in ('L', 'C', 'R') or len(bs) > 32: raise ValueError
            pag[-1].append(bytes([1, int(at, 16), ord(al), int(x), int(y), len(bs)]) + bs)
        except ValueError as ex: raise SystemExit('ERROR: %s:%d: linea del epilogo mal formada (%s): %s' % (os.path.basename(ruta), k, ex, l[:40]))
    if not pag: return None
    out = struct.pack('>H', len(pag))
    for ops in pag:
        b = par(struct.pack('>H', len(ops)) + b''.join(ops) + b'\x00'); out += struct.pack('>H', len(b)) + b
    return out
def tipos(pref, ver):
    """los tipos de monstruo de una aventura (seccion [monstruos] de asset/misiones[_ql]/<pref>.<idioma>.txt: una linea nombre|retrato|icono|sprite|clase por tipo, la primera es el tipo 0):
    n (byte) y n entradas de 16 bytes: retrato de combate, baldosa del plano, sprite de la vista, longitud del nombre, clase (0 normal, 1 orco, 2 muerto) y el nombre (11 bytes); None si no tiene"""
    q, o = T.fuente('misiones', pref, ver); ruta = q if os.path.exists(q) else o
    if not os.path.exists(ruta): return None
    out = bytearray(); sec = None; n = 0
    for k, l in enumerate(open(ruta, encoding='utf-8').read().split('\n'), 1):
        if not l.strip() or l.startswith('#'): continue
        mt = re.match(r'\[(\w+)\]', l)
        if mt: sec = mt.group(1); continue
        if sec != 'monstruos': continue
        try:
            nom, ret, ico, spr, cl = l.split(' ;')[0].split('|'); bs = T.codificar(nom.strip())
            if not nom.strip() or len(bs) > 9 or any(not 0 <= int(v) < 256 for v in (ret, ico, spr)): raise ValueError('nombre de 1 a 9 caracteres y numeros de 0 a 255')
            if cl.strip() not in CLASES: raise ValueError('clase N (normal), V (orco) o M (muerto)')
            out += bytes([int(ret), int(ico), int(spr), len(bs), CLASES[cl.strip()]]) + bs + bytes(11 - len(bs)); n += 1
        except ValueError as ex: raise SystemExit('ERROR: %s:%d: linea de [monstruos] mal formada (%s): %s' % (os.path.basename(ruta), k, ex, l[:40]))
    if n > 16: raise SystemExit('ERROR: %s: el juego admite 16 tipos de monstruo (hay %d)' % (os.path.basename(ruta), n))
    return bytes([n]) + bytes(out) if n else None
def estadisticas(pref, ver):
    """las estadisticas de monstruo de una aventura (seccion [estadisticas] de asset/misiones[_ql]/<pref>.<idioma>.txt: una linea movimiento|ataque|defensa|monedas|a_quien|objeto|mensaje|jefe
    por estadistica, la primera es la 0): n (byte) y n entradas de 9 bytes (movimiento, ataque, defensa, monedas (word), a quien, objeto, mensaje, jefe); None si no tiene"""
    q, o = T.fuente('misiones', pref, ver); ruta = q if os.path.exists(q) else o
    if not os.path.exists(ruta): return None
    out = bytearray(); sec = None; n = 0
    for k, l in enumerate(open(ruta, encoding='utf-8').read().split('\n'), 1):
        if not l.strip() or l.startswith('#'): continue
        mt = re.match(r'\[(\w+)\]', l)
        if mt: sec = mt.group(1); continue
        if sec != 'estadisticas': continue
        try:
            v = [int(x) for x in l.split(' ;')[0].split('|')]
            if len(v) != 8 or any(x < 0 for x in v) or v[3] > 65535 or any(x > 255 for x in v[:3] + v[5:]): raise ValueError('8 numeros: movimiento, ataque, defensa, monedas, a quien, objeto, mensaje y jefe')
            out += bytes(v[:3]) + struct.pack('>H', v[3]) + bytes(v[4:]); n += 1
        except ValueError as ex: raise SystemExit('ERROR: %s:%d: linea de [estadisticas] mal formada (%s): %s' % (os.path.basename(ruta), k, ex, l[:40]))
    if n > 20: raise SystemExit('ERROR: %s: el juego admite 20 estadisticas de monstruo (hay %d)' % (os.path.basename(ruta), n))
    return bytes([n]) + bytes(out) if n else None
def datos_aventura(pref, ver):
    """el bloque que va detras de las entradas del indice: tres longs con los desplazamientos (desde su inicio) del epilogo, de los tipos de monstruo y de sus estadisticas (0 = no hay)
    y esos tres bloques"""
    ep = epilogo(pref, ver); tp = tipos(pref, ver); es = estadisticas(pref, ver)
    if tp is None: raise SystemExit('ERROR: la aventura %s (%s) no tiene la seccion [monstruos] de sus tipos de monstruo (nombre|retrato|icono|sprite|clase)' % (pref, ver))
    if es is None: raise SystemExit('ERROR: la aventura %s (%s) no tiene la seccion [estadisticas] de sus estadisticas de monstruo (movimiento|ataque|defensa|monedas|a_quien|objeto|mensaje|jefe)' % (pref, ver))
    ep = par(ep) if ep else b''; tp = par(tp)
    return struct.pack('>LLL', 12 if ep else 0, 12 + len(ep), 12 + len(ep) + len(tp)) + ep + tp + es
def aventuras():
    """prefijos de las aventuras adicionales: los ficheros asset/misiones[_ql]/<prefijo>.<idioma>.txt (con el subtitulo)"""
    pref = set()
    for d in ('asset/misiones', 'asset/misiones_ql'):
        for f in os.listdir(os.path.join(ROOT, d)):
            mt = re.match(r'(\w+)\.(es|en)\.txt$', f)
            if mt and not mt.group(1).startswith('mision') and not re.search(r'_\d\d$', mt.group(1)): pref.add(mt.group(1))
    return sorted(pref)
def indice(pref, plantilla, n_mis, ver, sub, texto):
    """fichero data/<pref>_<ver>_idx. Formato (big endian): 'HQMI', version (word, 2), numero de misiones N (word), desplazamiento de los textos (long, 0 = no hay),
    subtitulo (byte de longitud + 35 caracteres del juego: sale bajo HEROQUEST en los menus) y N entradas de 40 bytes: numero (word), tamano del fichero de la mision (long),
    nombre (byte de longitud + 33 caracteres). Los ficheros de las misiones son <pref>_NN_<ver>. Detras de las entradas, si hay, los datos de la aventura (datos_aventura(): el long de +8 es su desplazamiento)."""
    idx = bytearray(); txt = ['numero fichero bytes nombre']; usadas = 0
    for n in range(n_mis):
        d = leer(n, ver, plantilla); blob = empaquetar(d); usadas += d['retocada']
        fich = '%s_%02d_%s' % (pref, n, ver)
        open(os.path.join(ROOT, 'data', fich), 'wb').write(blob)
        nom = T.codificar(d['nombre'])
        if len(nom) > 33: raise SystemExit('ERROR: nombre de mision demasiado largo (maximo 33)')
        idx += struct.pack('>HL', n, len(blob)) + bytes([len(nom)]) + nom + bytes(33 - len(nom))
        txt.append('%2d %s %5d %s%s' % (n, fich, len(blob), d['nombre'], '  (retocada)' if d['retocada'] else ''))
    st = T.codificar(sub)
    if len(st) > 35: raise SystemExit('ERROR: subtitulo demasiado largo (maximo 35)')
    cab = b'HQMI' + struct.pack('>HHL', 2, n_mis, 48 + len(idx) if texto else 0) + bytes([len(st)]) + st + bytes(35 - len(st))
    out = bytes(cab) + bytes(idx) + (texto or b'')
    open(os.path.join(ROOT, 'data/%s_%s_idx' % (pref, ver)), 'wb').write(out)
    open(os.path.join(ROOT, 'build/datos/indice_%s_%s.txt' % (pref, ver)), 'w').write('\n'.join(txt) + '\n')
    print('\n'.join(txt)); print('data/%s_%s_idx: %d bytes, %d misiones, subtitulo "%s"; misiones retocadas: %d' % (pref, ver, len(out), n_mis, sub, usadas))
def main():
    os.makedirs(os.path.join(ROOT, 'data'), exist_ok=True); os.makedirs(os.path.join(ROOT, 'build/datos'), exist_ok=True)
    for ver in ('es', 'en'):
        tx = os.path.join(ROOT, 'build/datos/textos_%s_bin' % ver)           # los textos del idioma van detras de las entradas de Hero Quest (hq_textos_empaquetar.py)
        if not os.path.exists(tx): raise SystemExit('ERROR: falta %s (ejecutar antes tools/hq_textos_empaquetar.py)' % tx)
        q, o = T.fuente('misiones', 'heroquest', ver)                         # el subtitulo de la aventura principal (opcional): asset/misiones[_ql]/heroquest.<idioma>.txt
        sub = aventura('heroquest', ver)[0] if os.path.exists(q) or os.path.exists(o) else ''
        open(os.path.join(ROOT, 'data/textos_%s' % ver), 'wb').write(open(tx, 'rb').read())      # los textos van aparte: solo hay un idioma en memoria, y el indice de una sola aventura
        indice('heroquest', 'mision%02d', 14, ver, sub, datos_aventura('heroquest', ver))
        for pref in aventuras():
            sub, n = aventura(pref, ver)
            if n: indice(pref, pref + '_%02d', n, ver, sub, datos_aventura(pref, ver))
if __name__ == '__main__':
    main()
