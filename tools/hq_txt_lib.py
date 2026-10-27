"""Biblioteca comun de textos de Hero Quest: alfabeto del juego y analisis de guiones, ventanas y tablas.
El juego dibuja el caracter de codigo c con el glifo (c - 0x30) de la fuente (8 bytes por glifo): los codigos utiles
van de 0x30 a 0x77. 0x5F es el espacio (glifo en blanco)."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
ROOT = m.ROOT
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
    return x1 < x2 < 32 and y1 < y2 < 24 and 0x6400 <= h < 0xB500
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
