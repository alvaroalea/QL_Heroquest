#!/usr/bin/env python3
"""Empaqueta los graficos en el formato de color del QL (modo 8) en data/ (nombres sin punto).
Origen de cada PNG: asset/graficos_ql/<ruta> si existe (version retocada); si no, asset/graficos/<ruta>.
La mascara sale de la transparencia del PNG (alfa < 128 = transparente). Los colores se asignan a la
paleta del QL (negro, azul, rojo, magenta, verde, cian, amarillo, blanco) por proximidad; se avisa si algun
punto esta lejos de la paleta.

Salidas (en data/; los nombres del QL no llevan punto):
  sprites_bin, escenario_bin, retratos_bin, marco_bin   graficos con color (y mascara si hay transparencia)
  heroquest_scr   la pantalla de carga (marco/pantalla_carga.png) como memoria de pantalla del QL (32768 bytes) para volcarla desde el boot
  baldosas_bin, fuente_normal_bin, fuente_manuscrita_bin   1 bit por punto, 8 bytes por caracter (como el original)
  build/datos/indice_<categoria>.txt   (solo para el PC) nombre, desplazamiento, ancho, alto
Formato de los ficheros de graficos (todo en big endian, el del 68000):
  +0   'HQGF'   +4 version (word = 1)   +6 numero de graficos N (word)   +8 tabla de N entradas de 16 bytes   luego los datos
  entrada: desplazamiento desde el inicio del fichero (long), ancho en puntos (word), alto (word), palabras por fila (word),
           copias desplazadas (word: 1 o 4), banderas (word: bit 0 = lleva mascara), reservado (word)
  datos: por cada copia y cada fila, por cada grupo de 4 puntos (palabra QL):
         [mascara_par, mascara_impar,] color_par, color_impar    (la mascara solo si bit 0 de las banderas)
  mascara: 1 en los dos bits de cada punto opaco (se usa: pantalla = (pantalla AND NOT mascara) OR color)
  color: formato de pantalla del modo 8: byte par = verde (bit 7-2k) y parpadeo (bit 6-2k), byte impar = rojo (bit 7-2k) y azul (bit 6-2k)
  copias desplazadas: si son 4, la copia j (0 a 3) esta desplazada j puntos a la derecha y tiene una palabra mas por fila
Uso: hq_ql_empaquetar.py [--desplazados] [--verificar]
  --desplazados   guarda 4 copias desplazadas de los sprites de personajes y de los muebles (dibujado mas rapido, 4 veces mas datos)
  --verificar     decodifica lo empaquetado y lo compara con los PNG"""
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from hq_png import leer
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QL = [(0, 0, 0), (0, 0, 255), (255, 0, 0), (255, 0, 255), (0, 255, 0), (0, 255, 255), (255, 255, 0), (255, 255, 255)]
def ruta(rel):
    """(ruta, base): base = 'asset/graficos_ql' solo si el PNG existe y es distinto del original (retocado)"""
    o = os.path.join(ROOT, 'asset/graficos', rel); q = os.path.join(ROOT, 'asset/graficos_ql', rel)
    if os.path.exists(q):
        return q, ('asset/graficos_ql' if not os.path.exists(o) or open(q, 'rb').read() != open(o, 'rb').read() else 'asset/graficos')
    if os.path.exists(o): return o, 'asset/graficos'
    raise SystemExit('falta el grafico ' + rel)
def a_indices(px, avisos, nombre):
    """pixel RGBA -> None (transparente) o indice de color QL"""
    r, g, b, a = px
    if a < 128: return None
    best = min(range(8), key=lambda i: (QL[i][0] - r) ** 2 + (QL[i][1] - g) ** 2 + (QL[i][2] - b) ** 2)
    d = (QL[best][0] - r) ** 2 + (QL[best][1] - g) ** 2 + (QL[best][2] - b) ** 2
    if d > 3 * 70 * 70: avisos.add(nombre)
    return best
def desplazar(mat, k):
    """k puntos a la derecha; una palabra (4 puntos) mas de ancho"""
    W = len(mat[0]) + 4
    return [[None] * k + fila + [None] * (W - len(fila) - k) for fila in mat]
def empaquetar_img(mat, mascara):
    """mat: filas de indice de color o None; devuelve (bytes, palabras_por_fila)"""
    W = len(mat[0]); pw = (W + 3) // 4; out = bytearray()
    for fila in mat:
        fila = fila + [None] * (pw * 4 - W)
        for g in range(pw):
            me = mo = ce = co = 0
            for k in range(4):
                c = fila[g * 4 + k]
                if c is None: continue
                s = 6 - 2 * k
                me |= 3 << s; mo |= 3 << s
                if c & 4: ce |= 2 << s            # verde
                if c & 2: co |= 2 << s            # rojo
                if c & 1: co |= 1 << s            # azul
            if mascara: out += bytes([me, mo])
            out += bytes([ce, co])
    return bytes(out), pw
def desempaquetar(datos, pw, h, mascara, W):
    """inversa, para --verificar: devuelve filas de indice o None"""
    p = 0; res = []
    for y in range(h):
        fila = []
        for g in range(pw):
            if mascara: me, mo = datos[p], datos[p + 1]; p += 2
            else: me = mo = 0xFF
            ce, co = datos[p], datos[p + 1]; p += 2
            for k in range(4):
                s = 6 - 2 * k
                if not (me >> s) & 3: fila.append(None); continue
                fila.append(((ce >> (s + 1)) & 1) * 4 + ((co >> (s + 1)) & 1) * 2 + ((co >> s) & 1))
        res.append(fila[:W])
    return res
FONT_GLYPHS = 85                                  # glifos de cada fuente: 17 x 5 (los codigos 0x30-0x73 del original y 0x74-0x84 libres); igual que FONT_GLYPHS de src/gfx.h
def main(desplazados, verificar):
    ent = [l.split() for l in open(os.path.join(ROOT, 'asset/graficos/indice.txt'))]
    cats = {}
    for e in ent: cats.setdefault(e[0].split('/')[0], []).append(e)
    # los 8 suelos de color (suelo_0 a suelo_7: el numero es el paper del atributo de la sala) van al final del escenario, indices 51 a 58;
    # solo existen en asset/graficos_ql/ y el juego los elige segun la sala (el 'suelo' del original, indice 10, ya no se usa)
    for k in range(8): cats['escenario'].append(['escenario/suelo_%d.png' % k])
    os.makedirs(os.path.join(ROOT, 'data'), exist_ok=True); os.makedirs(os.path.join(ROOT, 'build/datos'), exist_ok=True)
    avisos = set(); n_ret = 0; err = 0
    for cat, lista in cats.items():
        if cat == 'fuentes':
            for e in lista:
                nom = os.path.basename(e[0])[:-4]; p, base = ruta(e[0]); w, h, filas = leer(p)
                cw, ch, n, cols = (int(x) for x in e[3].split(':')[1:5])
                if nom.startswith('fuente_'): n = max(n, cols * (h // ch))     # si el PNG tiene mas filas (acentos, n con tilde) se empaquetan tambien
                b = bytearray()
                for c in range(n):
                    for y in range(ch):
                        v = 0
                        for k in range(cw):
                            if filas[(c // cols) * ch + y][(c % cols) * cw + k][3] >= 128: v |= 0x80 >> k
                        b.append(v)
                if nom.startswith('fuente_'): b += bytes(max(0, FONT_GLYPHS * 8 - len(b)))      # hueco hasta FONT_GLYPHS (src/gfx.h): glifos 0x74-0x84 libres
                if nom == 'marca_activo':                    # baldosas que no estan en la hoja: la 33 (voluta de la niebla) va detras de la marca (la 32), de fuentes/baldosa_niebla.png
                    pe, _ = ruta('fuentes/baldosa_niebla.png'); we, he, fe = leer(pe)
                    for y in range(8): b.append(sum(0x80 >> k for k in range(8) if fe[y][k][3] >= 128))
                open(os.path.join(ROOT, 'data', nom + '_bin'), 'wb').write(b)
                print('%-24s %5d bytes  (%s)' % (nom + '_bin', len(b), base)); n_ret += base == 'asset/graficos_ql'
            continue
        cuerpos = []; tabla = []; indice = []
        lista_all = lista; lista = [e for e in lista_all if os.path.basename(e[0])[:-4] != 'pantalla_carga']      # la pantalla de carga no va en marco_bin: solo en heroquest_scr
        N = len(lista); off = 8 + 16 * N
        for e in lista_all:
            rel = e[0]; nom = os.path.basename(rel)[:-4]; p, base = ruta(rel); n_ret += base == 'asset/graficos_ql'
            w, h, filas = leer(p)
            mat = [[a_indices(px, avisos, rel) for px in fila] for fila in filas]
            mascara = any(c is None for fila in mat for c in fila)
            copias = 4 if desplazados and (cat == 'sprites' and nom.startswith('sprite') or nom.startswith('mueble_')) else 1
            datos = bytearray(); pw0 = (w + 3) // 4
            for j in range(copias):
                m2 = mat if copias == 1 else desplazar(mat, j)
                d, pw = empaquetar_img(m2, mascara); datos += d
            if verificar:
                d0 = datos[:len(datos) // copias]
                back = desempaquetar(d0, (len(mat[0]) + 3) // 4 if copias == 1 else (len(mat[0]) + 4 + 3) // 4, h, mascara, len(mat[0]) + (0 if copias == 1 else 4))
                ref = mat if copias == 1 else desplazar(mat, 0)
                if back != ref: err += 1; print('ERROR de verificacion en', rel)
            if nom == 'pantalla_carga':
                # pantalla de carga para el arranque (data/boot la vuelca con LBYTES a 0x20000): la memoria de pantalla entera, 128 bytes por linea,
                # centrada en vertical (los 256 x 192 del Spectrum van entre las lineas 32 y 223) y el resto en negro
                scr = bytearray(32768); y0 = (256 - h) // 2; d0, pw0c = empaquetar_img(mat, False)
                if w == 256 and h <= 256: scr[y0 * 128:(y0 + h) * 128] = d0
                open(os.path.join(ROOT, 'data', 'heroquest_scr'), 'wb').write(scr)
                print('%-24s %6d bytes  (pantalla de carga, %s)' % ('heroquest_scr', len(scr), base))
                continue
            tabla.append(struct.pack('>LHHHHHH', off, w, h, pw0 if copias == 1 else pw0 + 1, copias, 1 if mascara else 0, 0))
            cuerpos.append(bytes(datos)); indice.append('%3d %-40s %8d %4d %3d %s%s' % (len(indice), nom, off, w, h, 'M' if mascara else '-', ' x%d' % copias if copias > 1 else ''))
            off += len(datos)
        blob = b'HQGF' + struct.pack('>HH', 1, N) + b''.join(tabla) + b''.join(cuerpos)
        open(os.path.join(ROOT, 'data', cat + '_bin'), 'wb').write(blob)
        open(os.path.join(ROOT, 'build/datos', 'indice_%s.txt' % cat), 'w').write('indice nombre desplazamiento ancho alto (M = con mascara)\n' + '\n'.join(indice) + '\n')
        print('%-24s %6d bytes, %d graficos' % (cat + '_bin', len(blob), N))
    print('graficos retocados usados: %d' % n_ret)
    if avisos: print('AVISO: puntos lejos de la paleta del QL en:', ', '.join(sorted(avisos)))
    if verificar: print('verificacion:', 'correcta' if not err else '%d errores' % err); sys.exit(1 if err else 0)
if __name__ == '__main__':
    main('--desplazados' in sys.argv, '--verificar' in sys.argv)
