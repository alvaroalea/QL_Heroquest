#!/usr/bin/env python3
"""Extrae todos los textos del juego a asset/textos/textos.txt, en un formato editable que permite traducirlos
(las longitudes pueden cambiar: ventanas, centrados y campos se recalculan al empaquetar).
Formato: ver docs/ZX_DATOS_QL.md. NO escribe en asset/textos_ql/ (alli van los textos retocados/traducidos).
Uso: hq_textos_extraer.py [salida.txt]"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
import hq_txt_lib as T
ROOT = T.ROOT
def w16(b, a): return b(a) | b(a + 1) << 8
def alin_linea(bs, w):
    """linea de una ventana (bytes del juego, rellena con espacios a w) -> (alineacion, texto sin relleno)"""
    s = T.decodificar(bs); n = len(s.rstrip(' ')); lp = len(s) - len(s.lstrip(' ')); core = s.strip(' ')
    if not core: return 'C', ''
    rp = len(s) - lp - len(core)
    if lp == 0 and rp == 0: return 'L', core
    if abs(lp - rp) <= 1: return 'C', core
    if lp == 0: return 'L', core
    if rp == 0: return 'R', core
    return 'C', core
def main(salida):
    L = []
    L.append('# Textos de Hero Quest (ZX Spectrum 128K, version en espanol) - generado por tools/hq_textos_extraer.py')
    L.append('# EDITAR LA COPIA de asset/textos_ql/textos.txt (este fichero se regenera). Formato en docs/ZX_DATOS_QL.md.')
    L.append('# Alfabeto: letras mayusculas, cifras, espacio, . , \' ! = ? y los codigos {XX} (glifo XX de la fuente; ver alfabeto del documento).')
    L.append('# Las longitudes pueden cambiar: ventanas (ancho y columna), centrados y campos se recalculan al empaquetar.')
    L.append('')
    n_ventana = n_guion = 0
    # ---- ventanas
    L.append('# ===================== VENTANAS (mensajes con marco)')
    for a0, a1 in ((0xAC50, 0xAE5F), (0xAF65, 0xB1B0), (0xBB8E, 0xC1B6)):
        vs, fin = T.ventanas(T.b_main, a0, a1)
        for v in vs:
            L.append('')
            L.append('[VENT_%04X] ventana y=%d centro=%d ancho=%d original=0x%04X' % (v['a'], v['y'], v['x'] + v['w'] // 2, v['w'], v['a']))
            for ln in v['lineas']:
                al, tx = alin_linea(ln, v['w']); L.append('%s|%s' % (al, tx))
            for z in v['zonas']: L.append('zona|%d|%d|%d|%d|0x%04X' % z)
            n_ventana += 1
    # ---- guiones
    def guiones(b, a0, a1, pref, cabecera):
        nonlocal n_guion
        L.append(''); L.append('# ===================== ' + cabecera)
        a = a0
        while a < a1:
            seg, sig = T.guion(b, a, a1)
            lin = T.guion_a_lineas(seg)
            if any(l[0] in ('texto', 'caja') for l in lin):
                L.append(''); L.append('[%s_%04X] guion original=0x%04X' % (pref, a, a))
                for l in lin:
                    if l[0] == 'texto':
                        _, at, x, y, bs = l; tx = T.decodificar(bs)
                        # se quitan los espacios de relleno (el original los usa para centrar o para borrar texto anterior)
                        lead = len(tx) - len(tx.lstrip(' ')); core = tx.strip(' ')
                        if core: x += lead; tx = core
                        al, col = T.alineacion(x, len(tx))
                        L.append('%02X|%s|%d|%d|%s' % (at, al, col, y, tx))
                    elif l[0] == 'caja': L.append('caja|%d|%d|%d|%d' % l[1:])
                    else: L.append('or')
                n_guion += 1
            a = sig
    for a0, a1 in ((0xA438, 0xA476), (0xA480, 0xA4BB), (0xB1B0, 0xB1C0), (0xB5EF, 0xB5F2), (0xB78E, 0xBB0E), (0xBB18, 0xBB8E)):
        guiones(T.b_main, a0, a1, 'GUION', 'GUIONES de la partida (0x%04X-0x%04X)' % (a0, a1))
    for a0, a1 in ((0x6561, 0x65D5), (0x65E2, 0x65EC), (0x6653, 0x6B1A), (0x6B2A, 0x6C1B)):
        guiones(T.b_ov, a0, a1, 'MENU', 'GUIONES del menu (overlay, 0x%04X-0x%04X)' % (a0, a1))
    # ---- tablas de cadenas
    L.append(''); L.append('# ===================== TABLAS DE CADENAS')
    def tabla(nombre, strs, campo, alin, orig, nota=''):
        L.append(''); L.append('[%s] cadenas campo=%d alineacion=%s original=%s %s' % (nombre, campo, alin, orig, nota))
        for s in strs: L.append(s if s else '-')   # '-' = cadena vacia
    def fijas(a, n, paso, term):
        out = []
        for i in range(n):
            bs = bytes(T.b_main(a + i * paso + k) for k in range(paso - (1 if term else 0)))
            out.append(T.decodificar(bs).strip(' '))
        return out
    tabla('NOMBRES_HEROE', fijas(0x7837, 5, 10, True), 9, 'C', '0x7837', '; se muestran en el panel y en las ventanas')
    tabla('NOMBRES_MONSTRUO', fijas(0xB54B, 12, 10, True), 9, 'C', '0xB54B')
    tabla('NOMBRES_OBJETO', fijas(0xB5F2, 15, 10, True), 9, 'C', '0xB5F2', '; muebles, trampas, salida, etc.')
    pn = fijas(0xAE7B, 6, 10, False); pd = fijas(0xAEB7, 6, 29, False)
    tabla('POCIONES_NOMBRE', pn, 10, 'L', '0xAE7B', '; 6 nombres de campo fijo')
    tabla('POCIONES_DESCRIPCION', pd, 29, 'L', '0xAEB7', '; 6 descripciones de campo fijo')
    def lista(nombre, tab, n, orig, nota=''):
        ss = []
        for i in range(n):
            p = w16(T.b_main, tab + 2 * i); bs = bytearray()
            while T.b_main(p) != 0: bs.append(T.b_main(p)); p += 1
            ss.append(T.decodificar(bs).rstrip(' '))
        campo = max(len(x) for x in ss)
        tabla(nombre, ss, campo, 'L', orig, nota)
    lista('PANEL_EFECTOS', 0x9739, 7, '0x9739', '; textos del panel de estado (se dibujan a partir de una posicion fija)')
    lista('PANEL_OBJETOS', 0x9781, 5, '0x9781')
    lista('PANEL_ARMAS', 0x97D7, 8, '0x97D7', '; la ultima entrada esta vacia en el original')
    lista('PANEL_ARMADURAS', 0x982A, 4, '0x982A')
    lista('PANEL_HECHIZOS_B', 0x9853, 6, '0x9853')
    lista('PANEL_HECHIZOS_C', 0x98A0, 6, '0x98A0')
    # ---- indices de las pociones
    L.append(''); L.append('[POCIONES_INDICES] indices original=0xAE5F ; por cada pocion: indice de nombre e indice de descripcion (no son texto)')
    for i in range(7):
        pn_, pd_ = w16(T.b_main, 0xAE5F + 4 * i), w16(T.b_main, 0xAE5F + 4 * i + 2)
        L.append('%d|%d' % ((pn_ - 0xAE7B) // 10, (pd_ - 0xAEB7) // 29))
    open(salida, 'w', encoding='utf-8').write('\n'.join(L) + '\n')
    print('%d ventanas, %d guiones, 8+ tablas en %s' % (n_ventana, n_guion, salida))
if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'asset/textos/textos.txt'))
