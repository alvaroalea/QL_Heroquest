#!/usr/bin/env python3
"""Extrae los graficos originales de Hero Quest (ZX 128K) a PNG RGBA de 8 colores del QL con transparencia.
Escribe en asset/graficos/ (se puede ejecutar siempre: NO toca asset/graficos_ql/, donde van las versiones retocadas).
  sprites/     66 sprites de personajes y monstruos (+ cofre y trono) y el cursor
               tinta = negro, halo = blanco (se deduce con la regla del juego), resto transparente
  escenario/   paredes, puertas, suelo y escaleras, muebles (sprites), piezas base de muebles, rombos de casilla bloqueada y de trampa
               tinta = negro, resto transparente (el juego las compone con OR sobre el fondo)
  retratos/    retratos de combate 128x64 con sus colores (opacos)
  marco/       marco de la pantalla de juego y pantalla de carga (256x192, con sus colores)
  fuentes/     baldosas de 8x8, fuente normal y fuente manuscrita (negro sobre transparente; se empaquetan a 1 bit por punto)
  indice.txt   orden y tamano de todos los graficos (lo usa el empaquetador)
Paleta del QL (modo 8): 0 negro, 1 azul, 2 rojo, 3 magenta, 4 verde, 5 cian, 6 amarillo, 7 blanco (bit 2 = verde, bit 1 = rojo, bit 0 = azul)."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
from hq_png import escribir_rgba
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QL = [(0, 0, 0), (0, 0, 255), (255, 0, 0), (255, 0, 255), (0, 255, 0), (0, 255, 255), (255, 255, 0), (255, 255, 255)]
NEGRO = QL[0] + (255,); BLANCO = QL[7] + (255,); TRANSP = (0, 0, 0, 0)
ANCHOS_ROMBO = [4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4]
def w16(a, f=m.byte): return f(a) | f(a + 1) << 8
def bits(get, wb, h):
    """matriz h x (wb*8) de bits (1 = tinta)"""
    return [[(get(y * wb + x) >> (7 - k)) & 1 for x in range(wb) for k in range(8)] for y in range(h)]
def con_halo(mat, halo=True, mascara=None):
    """RGBA: tinta negra, halo blanco (2 puntos a cada lado en horizontal) y transparente"""
    out = []
    for y, fila in enumerate(mat):
        W = len(fila); r = []
        for x in range(W):
            if fila[x]: r.append(NEGRO)
            elif halo and any(0 <= x + d < W and fila[x + d] for d in (-2, -1, 1, 2)): r.append(BLANCO)
            else: r.append(TRANSP)
        out.append(r)
    return out
def solo_tinta(mat):
    return [[NEGRO if v else TRANSP for v in fila] for fila in mat]
def pantalla_zx(bmp, att):
    """256x192 RGBA opaco desde un bitmap de pantalla del Spectrum (6144 bytes) y sus atributos (768)"""
    out = []
    for y in range(192):
        a = ((y >> 6) << 11) | ((y & 7) << 8) | (((y >> 3) & 7) << 5)
        r = []
        for cx in range(32):
            at = att[(y >> 3) * 32 + cx]; ink, pap = QL[at & 7], QL[(at >> 3) & 7]
            b = bmp[a + cx]
            for k in range(8): r.append((ink if b & (0x80 >> k) else pap) + (255,))
        out.append(r)
    return out
def retrato(banco, ptr):
    d = m.BANKS[banco]; o = ptr - 0xC000; bmp, att = d[o:o + 0x400], d[o + 0x400:o + 0x480]
    out = []
    for y in range(64):
        r = []
        for x in range(16):
            at = att[(y // 8) * 16 + x]; ink, pap = QL[at & 7], QL[(at >> 3) & 7]; b = bmp[y * 16 + x]
            for k in range(8): r.append((ink if b & (0x80 >> k) else pap) + (255,))
        out.append(r)
    return out
def nombre_objeto(c):
    idx = m.byte(0xB5C3 + c - 0x30); a = 0xB5F2 + 10 * idx
    return ''.join(chr(m.byte(a + k)) for k in range(10) if 0x30 <= m.byte(a + k) < 0x5F).strip().lower() or 'sin_nombre'
def catalogo():
    """lista de (categoria, nombre, matriz RGBA o ('bin1', bytes, ancho_celda, alto_celda, n, columnas))"""
    L = []
    # --- sprites de personajes y monstruos (tabla 0xC23A)
    for i in range(66):
        a = 0xC23A + 4 * i; wd, h, p = m.byte(a), m.byte(a + 1), w16(a + 2)
        wb = wd // 8
        L.append(('sprites', 'sprite%02d' % i, con_halo(bits(lambda k, p=p: m.byte(p + k), wb, h))))
    # cursor (0xC55B: 24x8, mascara en +0x30; bit de mascara 1 = se conserva el fondo)
    g = bits(lambda k: m.byte(0xC55B + k), 3, 8); mk = bits(lambda k: m.byte(0xC55B + 0x30 + k), 3, 8)
    L.append(('sprites', 'cursor', [[NEGRO if g[y][x] else (TRANSP if mk[y][x] else BLANCO) for x in range(24)] for y in range(8)]))
    # --- escenario
    for n, a in (('pared_oeste', 0xC5BF), ('pared_norte', 0xC655)):
        L.append(('escenario', n, solo_tinta(bits(lambda k, a=a: m.byte(a + k), 3, 50))))
    for k in range(8):
        a = 0xC6EB + 150 * k
        L.append(('escenario', 'puerta_%d' % k, solo_tinta(bits(lambda j, a=a: m.byte(a + j), 3, 50))))
    for n, a in (('suelo', 0xE4A8), ('escalera_0', 0xE520), ('escalera_1', 0xE598), ('escalera_2', 0xE610), ('escalera_3', 0xE688)):
        mat = []; p = a
        for wd in ANCHOS_ROMBO:
            fila = []
            for x in range(6):
                if x < wd:
                    b = m.byte(p + x); fila += [(b >> (7 - kk)) & 1 for kk in range(8)]
                else: fila += [0] * 8
            mat.append(solo_tinta([fila])[0]); p += wd
        # fuera del ancho de cada fila: transparente (ya lo es: tinta 0)
        L.append(('escenario', n, mat))
    b6 = m.BANKS[6]
    for i in range(17):
        a = 0x88A0 + 6 * i; ty, h, src = m.byte(a), m.byte(a + 1), w16(a + 4); wb = ty + 1
        L.append(('escenario', 'mueble_%02X_%s' % (0x4B + i, nombre_objeto(0x4B + i)), con_halo(bits(lambda k, s=src: b6[s - 0xC000 + k] if s - 0xC000 + k < len(b6) else 0, wb, h))))
    for i in range(17):
        a = 0x9470 + 8 * i; src, wd, h = w16(a + 2), m.byte(a + 6), m.byte(a + 7)
        L.append(('escenario', 'base_mueble_%02X' % (0x4B + i), solo_tinta(bits(lambda k, s=src: b6[s - 0xC000 + k], wd, h))))
    # rombos que el juego dibuja con los mismos codigos de objeto que las escaleras (0x30 y 0x31 casilla bloqueada: 0xFF79; 0x33 trampa: 0xFF01),
    # al final del escenario para que no cambien los indices de los demas
    for n, a in (('bloqueo', 0xFF79), ('trampa', 0xFF01)):
        mat = []; p = a
        for wd in ANCHOS_ROMBO:
            fila = []
            for x in range(6):
                if x < wd:
                    b = m.byte(p + x); fila += [(b >> (7 - kk)) & 1 for kk in range(8)]
                else: fila += [0] * 8
            mat.append(solo_tinta([fila])[0]); p += wd
        L.append(('escenario', n, mat))
    # --- retratos de combate
    for n, b, p in (('barbaro', 4, 0xFA80), ('enano', 4, 0xF600), ('elfo', 4, 0xF180), ('hechicero', 7, 0xC440),
                    ('monstruo_orco_ulag_grak', 4, 0xE880), ('monstruo_fimir', 3, 0xF802), ('monstruo_guerrero', 7, 0xC8C0),
                    ('monstruo_esqueleto', 3, 0xF382), ('monstruo_momia', 4, 0xE400), ('ataque_fallido', 4, 0xED00)):
        L.append(('retratos', 'retrato_' + n, retrato(b, p)))
    # --- marco de pantalla y pantalla de carga
    L.append(('marco', 'marco', pantalla_zx(b6[0:0x1800], b6[0x1800:0x1B00])))
    scr = open(os.path.join(ROOT, 'asset/extraido/blk03.bin'), 'rb').read()
    L.append(('marco', 'pantalla_carga', pantalla_zx(scr[0:0x1800], scr[0x1800:0x1B00])))
    # --- fuentes y baldosas (1 bit por punto)
    b7 = m.BANKS[7]
    L.append(('fuentes', 'baldosas', ('bin1', bytes(m.byte(0xB1C8 + i) for i in range(256)), 8, 8, 32, 8)))
    L.append(('fuentes', 'marca_activo', ('bin1', bytes(m.byte(0xB1C0 + i) for i in range(8)), 8, 8, 1, 1)))      # baldosa 0xB1C0: marca del personaje activo en el plano
    L.append(('fuentes', 'fuente_normal', ('bin1', bytes(b7[0:0x220]), 8, 8, 68, 17)))
    L.append(('fuentes', 'fuente_manuscrita', ('bin1', bytes(b7[0x220:0x440]), 8, 8, 68, 17)))
    return L
def hoja_bin1(datos, cw, ch, n, cols):
    filas = (n + cols - 1) // cols; out = [[TRANSP] * (cols * cw) for _ in range(filas * ch)]
    for c in range(n):
        for y in range(ch):
            b = datos[c * ch + y]
            for k in range(cw):
                if b & (0x80 >> k): out[(c // cols) * ch + y][(c % cols) * cw + k] = NEGRO
    return out
def main(dest):
    if os.path.abspath(dest).startswith(os.path.join(ROOT, 'asset/graficos_ql')): raise SystemExit('asset/graficos_ql es la carpeta de las versiones retocadas: no se escribe aqui')
    idx = []
    for cat, nom, img in catalogo():
        d = os.path.join(dest, cat); os.makedirs(d, exist_ok=True)
        if isinstance(img, tuple): _, datos, cw, ch, n, cols = img; mat = hoja_bin1(datos, cw, ch, n, cols); extra = 'hoja %dx%d celdas de %dx%d, %d caracteres' % (cols, (n + cols - 1) // cols, cw, ch, n)
        else: mat = img; extra = ''
        escribir_rgba(os.path.join(d, nom + '.png'), len(mat[0]), len(mat), mat)
        idx.append('%s/%s.png %d %d %s' % (cat, nom, len(mat[0]), len(mat), ('bin1:%d:%d:%d:%d:%d ' % (cw, ch, n, cols, 0) if isinstance(img, tuple) else '') + extra))
    open(os.path.join(dest, 'indice.txt'), 'w').write('\n'.join(idx) + '\n')
    print(len(idx), 'graficos en', dest)
if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'asset/graficos'))
