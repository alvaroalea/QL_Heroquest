"""Biblioteca comun de textos de Hero Quest: alfabeto del juego y analisis de guiones, ventanas y tablas.
El juego dibuja el caracter de codigo c con el glifo (c - 0x30) de la fuente (8 bytes por glifo): los codigos utiles
van de 0x30 a 0x77. 0x5F es el espacio (glifo en blanco)."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
ROOT = m.ROOT
VERSION = m.VERSION          # 'es' o 'en' (variable de entorno HQ_VERSION): sufijo de los ficheros extraidos (textos.es.txt, mision00.en.txt...)
# Direcciones de los textos en cada version (la inglesa tiene otra distribucion: los textos desplazan el codigo y los datos).
#   vent: rangos de ventanas (main); gui: rangos de guiones de la partida (main); gui_ov: id. del menu (overlay, org 0x5B00)
#   heroes, monstruos: (direccion, cantidad) de los nombres (10 bytes, 0 final); objetos: direccion de los 15 nombres de objeto; obj_idx: indices de nombre por codigo de objeto
#   panel: tablas de punteros (nombre, direccion, cantidad); pociones: (nombres, cantidad, descripciones, ancho descripcion, pares de indices, cantidad de pares)
#   nombres_menu: guion del overlay con los 14 nombres de mision; handler_max: limite superior de las rutinas de las zonas activas
DIR = {
    'es': dict(
        vent=((0xAC50, 0xAE5F), (0xAF65, 0xB1B0), (0xBB8E, 0xC1B6)),
        gui=((0xA438, 0xA476), (0xA480, 0xA4BB), (0xB1B0, 0xB1C0), (0xB5EF, 0xB5F2), (0xB78E, 0xBB0E), (0xBB18, 0xBB8E)),
        gui_ov=((0x6561, 0x65D5), (0x65E2, 0x65EC), (0x6653, 0x6B1A), (0x6B2A, 0x6C1B)),
        heroes=(0x7837, 5), monstruos=(0xB54B, 12), objetos=0xB5F2, obj_idx=0xB5C3,
        panel=(('PANEL_EFECTOS', 0x9739, 7), ('PANEL_OBJETOS', 0x9781, 5), ('PANEL_ARMAS', 0x97D7, 8), ('PANEL_ARMADURAS', 0x982A, 4), ('PANEL_HECHIZOS_B', 0x9853, 6), ('PANEL_HECHIZOS_C', 0x98A0, 6)),
        pociones=(0xAE7B, 6, 0xAEB7, 29, 0xAE5F, 7), nombres_menu=0x689A, handler_max=0xB500),
    'en': dict(
        vent=((0xAC56, 0xAEE3), (0xAF9B, 0xB1EF), (0xBBF8, 0xC1BC)),
        gui=((0xA44C, 0xA47C), (0xA486, 0xA4C1), (0xB23A, 0xB24A), (0xB684, 0xB687), (0xB823, 0xBB7C), (0xBB86, 0xBBF8)),
        gui_ov=((0x656B, 0x65E0), (0x65ED, 0x65F7), (0x665E, 0x6AD6), (0x6AE6, 0x6BCC)),
        heroes=(0x7834, 5), monstruos=(0xB5D6, 13), objetos=0xB687, obj_idx=0xB658,
        panel=(('PANEL_EFECTOS', 0x9761, 7), ('PANEL_OBJETOS', 0x97A8, 5), ('PANEL_ARMAS', 0x97F7, 7), ('PANEL_ARMADURAS', 0x9846, 4), ('PANEL_HECHIZOS_B', 0x986E, 6), ('PANEL_HECHIZOS_C', 0x98B7, 6)),
        pociones=(0xAEF7, 4, 0xAF1F, 31, 0xAEE3, 5), nombres_menu=0x6870, handler_max=0xB580),
}
# La ampliacion Return of the Witch Lord (HQ_JUEGO=witchlord) tiene el mismo motor con el codigo desplazado; solo se analizan sus misiones: mis_tabla = tabla de punteros de
# las misiones (overlay, org 0x5B00), n_mis = numero de misiones, nombres_menu = guion de los nombres, titulo = guion del titulo del menu (subtitulo en la segunda linea de texto)
DIR['wl_es'] = dict(mis_tabla=0x65CB, n_mis=10, nombres_menu=0x6832, titulo=0x65DF, monstruos=(0xB0D8, 16))
DIR['wl_en'] = dict(mis_tabla=0x65CC, n_mis=10, nombres_menu=0x6802, titulo=0x65E0, monstruos=(0xB166, 16))
DIR['es'].update(mis_tabla=0x6637, n_mis=14); DIR['en'].update(mis_tabla=0x6642, n_mis=14)
JUEGO = m.JUEGO
D = DIR[VERSION if JUEGO == 'hq' else 'wl_' + VERSION]
def distintos(a, b):
    """True si dos ficheros editables difieren (se ignoran las lineas de comentario '#', que cambian al regenerar el original)"""
    def cuerpo(r): return [l for l in open(r, encoding='utf-8').read().split('\n') if not l.startswith('#')]
    return cuerpo(a) != cuerpo(b)
def fuente(carpeta, base, ver=None):
    """(ruta retocada, ruta original) de un fichero editable de la version actual: asset/<carpeta>_ql/<base>.<v>.txt y asset/<carpeta>/<base>.<v>.txt.
    Para la version espanola, el nombre antiguo asset/<carpeta>_ql/<base>.txt (sin idioma) sigue valiendo como copia retocada si no existe la nueva."""
    ver = ver or VERSION
    q = os.path.join(ROOT, 'asset', carpeta + '_ql', '%s.%s.txt' % (base, ver)); o = os.path.join(ROOT, 'asset', carpeta, '%s.%s.txt' % (base, ver))
    viejo = os.path.join(ROOT, 'asset', carpeta + '_ql', base + '.txt')
    if ver == 'es' and not os.path.exists(q) and os.path.exists(viejo): q = viejo
    return q, o
OVERLAY = open(os.path.join(m.EXT, 'overlay_menu_5B00.bin'), 'rb').read()
def b_main(a): return m.byte(a)
def b_ov(a): return OVERLAY[a - 0x5B00]
# ---------------------------------------------------------------- alfabeto
# caracter legible -> codigo del juego. Lo que no esta aqui se escribe {XX} (codigo en hexadecimal)
DEC = {0x5F: ' ', 0x3A: '.', 0x3B: ',', 0x3D: "'", 0x3E: '!', 0x40: '=', 0x5E: '?'}
for c in range(0x30, 0x3A): DEC[c] = chr(c)
for c in range(0x41, 0x5B): DEC[c] = chr(c)
ENC = {v: k for k, v in DEC.items()}
def decodificar(bs):
    """bytes del juego -> texto legible; los codigos sin caracter se escriben {XX}"""
    s = ''
    for c in bs:
        s += DEC.get(c, '{%02X}' % c)
    return s
def codificar(txt, alfabeto=None):
    """texto legible -> bytes del juego. Acepta {XX}. Lanza ValueError si hay un caracter sin codigo"""
    enc = alfabeto or ENC; out = bytearray(); i = 0
    while i < len(txt):
        ch = txt[i]
        if ch == '{':
            j = txt.index('}', i); out.append(int(txt[i + 1:j], 16)); i = j + 1; continue
        if ch not in enc: raise ValueError('caracter sin codigo en la fuente del juego: %r' % ch)
        out.append(enc[ch]); i += 1
    return bytes(out)
# ---------------------------------------------------------------- guiones (interprete 0x8BF9)
def guion(b, a, fin=None, parar_en_final=True):
    """analiza un guion desde a. Devuelve (segmentos, siguiente_direccion).
    segmento: ('attr', c) | ('pos', x, y) | ('texto', bytes) | ('caja', lo, hi, w, h) | ('or',) """
    seg = []
    while fin is None or a < fin:
        c = b(a)
        if c == 0xFF: seg.append(('pos', b(a + 1), b(a + 2))); a += 3
        elif c == 0xFB: seg.append(('attr', b(a + 1))); a += 2
        elif c == 0xFC: seg.append(('caja', b(a + 1), b(a + 2), b(a + 3), b(a + 4))); a += 5
        elif c == 0xFE: seg.append(('or',)); a += 1
        elif c in (0xFA, 0x00):
            if parar_en_final: return seg, a + 1
            a += 1
        else:
            j = a
            while (fin is None or j < fin) and b(j) not in (0, 0xFA, 0xFB, 0xFC, 0xFE, 0xFF): j += 1
            seg.append(('texto', bytes(b(k) for k in range(a, j)))); a = j
    return seg, a
def guion_a_lineas(seg):
    """segmentos -> lista de (attr, x, y, bytes_texto, cajas) en el orden del guion"""
    out = []; attr = 7; x = y = 0
    for s in seg:
        if s[0] == 'attr': attr = s[1]
        elif s[0] == 'pos': x, y = s[1], s[2]
        elif s[0] == 'texto': out.append(('texto', attr, x, y, s[1])); x += len(s[1])
        elif s[0] == 'caja': out.append(('caja', s[1], s[2], s[3], s[4]))
        elif s[0] == 'or': out.append(('or',))
    return out
def alineacion(x, n):
    """texto de n caracteres en la columna x: ('C', centro) si esta centrado en la pantalla de 32 columnas (+-1), si no ('L', x)"""
    if n and abs(x + n / 2.0 - 16) <= 1.0: return 'C', 16
    return 'L', x
# ---------------------------------------------------------------- ventanas (rutina 0x8DD1)
def zona_valida(b, a):
    x1, x2, y1, y2 = (b(a + i) for i in range(4)); h = b(a + 4) | b(a + 5) << 8
    return x1 < x2 < 32 and y1 < y2 < 24 and 0x6400 <= h < D['handler_max']
def ventanas(b, a, fin):
    """lista de ventanas entre a y fin: dict(a, x, y, w, h, lineas=[bytes], zonas=[(x1,x2,y1,y2,accion)])"""
    out = []
    while a < fin:
        x, y, w, h = (b(a + i) for i in range(4))
        if not (0 < w < 32 and 0 < h < 22 and x < 32 and y < 24): break
        end = a + 4 + w * h; z = []; q = end
        if zona_valida(b, q):
            while b(q) != 0xFE and zona_valida(b, q): z.append(q); q += 6
            if b(q) == 0xFE: q += 1
            else: z = []; q = end
        def hdr(p):
            xx, yy, ww, hh = (b(p + i) for i in range(4))
            return 0 < ww < 32 and 0 < hh < 22 and xx < 32 and yy < 24 and all(0x30 <= b(p + 4 + k) < 0x78 for k in range(min(ww, 4)))
        pad = 0
        while b(q + pad) == 0x5F: pad += 1
        if not hdr(q + pad) and not z and q + pad < fin:      # texto mas corto que su cabecera (erratas del original)
            for k in range(1, 9):
                if hdr(end - k): q = end - k; end = q; pad = 0; break
        n = end - a - 4
        lineas = [bytes(b(a + 4 + r * w + c) for c in range(w)) for r in range(h)] if n == w * h else [bytes(b(a + 4 + k) for k in range(j, min(j + w, n))) for j in range(0, n, w)]
        out.append(dict(a=a, x=x, y=y, w=w, h=h, lineas=lineas, zonas=[(b(p), b(p + 1), b(p + 2), b(p + 3), b(p + 4) | b(p + 5) << 8) for p in z]))
        a = q + pad
    return out, a
