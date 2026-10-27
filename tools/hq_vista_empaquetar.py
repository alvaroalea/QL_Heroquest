#!/usr/bin/env python3
"""Empaqueta en data/vista_bin las tablas de geometria de la vista isometrica y del plano del original (ZX Spectrum), que no son graficos:
posiciones de las 30 casillas de la vista, origen y color de cada sala, desplazamientos de paredes, puertas y muebles, que baldosa
va con cada codigo del plano, y las zonas de la pantalla que se borran y se colorean. Los gráficos a que se refieren (paredes, puertas,
muebles, rombos de suelo...) ya estan en escenario_bin y sprites_bin: aqui solo se guarda su INDICE en esos ficheros.

Formato (big endian): 'HQVI', version (word, 1), numero de secciones N (word) y, por cada seccion, n (word), k (word) y n*k palabras
con signo (word de 16 bits). Secciones, en este orden (de donde sale cada una en el original, entre parentesis):
   0 casillas  n=52 k=2  (y, x) de la esquina superior izquierda del rombo de cada casilla de la vista (0xC414); las 30 primeras son la ventana de 6 x 5
   1 salas     n=43 k=3  (columna, fila, color) de la ventana de cada sala (0xC47C); color = byte de atributo del Spectrum
   2 puertas A n=16 k=3  (dy, dx, indice en escenario_bin) de las puertas que se dibujan como figura, indice (B & 0x0C | 0x20) / 4 (0xB72B)
   3 puertas B n=8  k=3  igual, indice (B & 0x1C) / 4 (0xB76B)
   4 muebles   n=17 k=5  (tipo de dibujo, alto, dx, dy, indice en escenario_bin) de los muebles con sprite, codigos 0x4B a 0x5B (0x88A0)
   5 bases     n=17 k=3  (dx en bytes, dy en lineas, indice en escenario_bin) de las piezas base de los muebles en el plano (0x9470)
   6 suelos    n=16 k=1  indice en escenario_bin del rombo de cada codigo de objeto 0x30 a 0x3F (0xC342; los codigos 0x3C a 0x3F usan el suelo normal)
   7 plano     n=64 k=1  indice de baldosa (baldosas_bin) segun el byte de la capa B >> 2 (0xC35A)
   8 heroes    n=5  k=1  baldosa del icono de cada personaje en el plano (0xC3CE)
   9 monstruos n=14 k=1  baldosa del icono de cada tipo de monstruo en el plano (0xC3B2)
  10 objetos   n=16 k=1  baldosa del icono de cada codigo de objeto 0x30 a 0x3F en el plano (0xC39A)
  11 constantes n=4 k=1  baldosa del cofre (0x40) y del trono (0x41) en el plano, baldosa de la marca del personaje activo (la 32 de baldosas: marca_activo_bin
                         esta aparte) y baldosa vacia
  12 borrado   n=22 k=2 (columna, ancho en celdas) de cada fila de celdas de la vista que se limpia antes de dibujarla (0xA4BB)
  13 colores   n=17 k=3 (fila, columna, ancho en celdas) de las zonas de la vista que cogen el color de la sala (0xC528)
  14 vecinas   n=52 k=1 desplazamiento en el mapa (en casillas, con signo) de cada una de las 52 posiciones de la seccion 0 respecto al origen de la ventana (0xB6AC y 0xB6D9)
  15 zonas     n=10 k=5 (columna 1, columna 2, fila 1, fila 2, accion) de las zonas de clic de la pantalla de juego (0xB4F0); fila y columna de CURSOR_A_CASILLA, los
                         limites superiores no cuentan. Accion: 0 casilla x-1, 1 x+1, 2 y-1, 3 y+1, 4 ojo, 5 siguiente personaje, 6 mapa, 7 bolsa, 8 atacar, 9 llave
  16 guiones   n=52 k=4 (fotograma, hay desplazamiento, dx, dy) de los 4 guiones de la animacion de un paso (0xC1B6), 12 fotogramas + 1 fila de fin (fotograma -1) cada uno;
                         guion 0 = casilla y+1, 1 = x+1, 2 = x-1, 3 = y-1. El desplazamiento se aplica despues de dibujar el fotograma
  18 salto     n=11 k=4 (sala, columna, fila, desplazamiento de la puerta) de los 11 destinos de los teletransportes de la mision 9 (0x85AD)
  19 nombres   n=44 k=1 indice en NOMBRES_OBJETO (textos_bin) del nombre de cada codigo de objeto 0x30 a 0x5B (0xB5C3)
  20 oro       n=9  k=1 monedas de cada una de las cartas 0 a 8 del mazo de tesoros (0xB688)
  21 pociones  n=7  k=3 (bit de efecto que se suma a +0x10, indice del nombre, indice de la descripcion) de las cartas 9 a 15 (0x829B y 0xAE5F)
  22 emboscada n=4  k=2 (dx, dy) de las casillas donde puede aparecer el monstruo que te sigue (0x81C0)
  23 armas     n=7  k=1 byte de cada arma (0xAA81, de la lanza al baston): bits 0-1 = dados de mas, 5 = se pierde al lanzarla, 4 y 6 = alcance, 7 = tambien en diagonal
  24 monstruos n=20 k=7 por tipo de monstruo (0xAA8C): movimiento, dados de ataque, de defensa, monedas que da al morir, a quien (0 nadie, 1 el que lo mata, 2 los cuatro heroes), bit de objeto que da (0 ninguno; 3 = baraja de orco) y mensaje (0 ninguno, 1 BANE DE ORC, 2 ESTRELLA DEL OESTE)
  25 contacto  n=8  k=2 (dx, dy) de las ocho casillas de al lado, primero las cuatro de en medio y despues las diagonales (0x81C0)
  26 panel     n=10 k=4 (columna 1, columna 2, fila 1, fila 2) de las zonas de clic del panel del personaje (0x7FAE)
  27 mascaras  n=8  k=1 zonas del panel que se pueden usar: 0 a 3 por personaje al abrir la bolsa, 4 a 7 al elegir arma para atacar (0x7FEA)
  28 blancos   n=12 k=1 a quien se puede lanzar cada hechizo (0x7B70): bit 0 un heroe, bit 1 un monstruo, bit 2 una casilla
  17 pasos     n=9  k=1 fotograma base de cada personaje al terminar un paso (0x8505, 5 valores) y desplazamiento de cada direccion (0x850A, 4 valores)
Los indices de escenario_bin son los de build/datos/indice_escenario.txt: pared_oeste 0, pared_norte 1, puerta_0..7 = 2..9, suelo 10,
escalera_0..3 = 11..14, mueble_4B.. = 15 + i, base_mueble_4B.. = 32 + i, bloqueo 49, trampa 50,
suelo_0..7 = 51..58 (a color y con mascara; el numero es el paper del atributo de la sala; el suelo 10 ya no se dibuja)."""
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
b = m.byte
def w16(a): return b(a) | b(a + 1) << 8
def s8(v): return v - 256 if v > 127 else v
def s16(v): return v - 65536 if v > 32767 else v

ESC = {0xC5BF: 0, 0xC655: 1, 0xE4A8: 10, 0xE520: 11, 0xE598: 12, 0xE610: 13, 0xE688: 14, 0xFF79: 49, 0xFF01: 50}
for k in range(8): ESC[0xC6EB + 150 * k] = 2 + k

def esc(ptr):
    if ptr == 0: return -1
    if ptr not in ESC: raise SystemExit('direccion de grafico desconocida: %04X' % ptr)
    return ESC[ptr]

def baldosa(addr):
    """direccion de una baldosa de 8 bytes -> indice en baldosas_bin (32 = marca_activo, 0xB1C0)"""
    if addr == 0xB1C0: return 32
    t = (addr - 0xB1C8) // 8
    if (addr - 0xB1C8) % 8 or not 0 <= t < 32: raise SystemExit('baldosa fuera de la tabla: %04X' % addr)
    return t

def secciones():
    S = []
    S.append((2, [[b(0xC414 + 2 * i), (b(0xC414 + 2 * i + 1) - 256 if b(0xC414 + 2 * i + 1) >= 0xF0 else b(0xC414 + 2 * i + 1))] for i in range(52)]))
    S.append((3, [[b(0xC47C + 4 * i), b(0xC47C + 4 * i + 1), b(0xC47C + 4 * i + 2)] for i in range(43)]))
    for base, n in ((0xB72B, 16), (0xB76B, 8)):
        S.append((3, [[b(base + 4 * i), b(base + 4 * i + 1), esc(w16(base + 4 * i + 2))] for i in range(n)]))
    S.append((5, [[b(0x88A0 + 6 * i), b(0x88A0 + 6 * i + 1), s8(b(0x88A0 + 6 * i + 2)), s8(b(0x88A0 + 6 * i + 3)), 15 + i] for i in range(17)]))
    bases = []
    for i in range(17):
        a = 0x9470 + 8 * i; off = s16(w16(a)); dy = round(off / 32); dx = off - dy * 32
        if not -3 <= dx <= 3: raise SystemExit('desplazamiento raro de la base %d: %d' % (i, off))
        bases.append([dx, dy, 32 + i])
    S.append((3, bases))
    S.append((1, [[esc(w16(0xC342 + 2 * i))] for i in range(12)] + [[10]] * 4))
    S.append((1, [[(b(0xC35A + i) // 8)] for i in range(64)]))
    S.append((1, [[baldosa(w16(0xC3CE + 2 * i))] for i in range(5)]))
    S.append((1, [[baldosa(w16(0xC3B2 + 2 * i))] for i in range(14)]))
    S.append((1, [[baldosa(w16(0xC39A + 2 * i))] for i in range(16)]))
    S.append((1, [[baldosa(0xB260)], [baldosa(0xB258)], [32], [0]]))
    strips = []
    for i in range(22):
        a = 0xA4BB + 3 * i; addr = w16(a) - 0xE700; strips.append([addr % 32, b(a + 2) * 2])
        if addr // 256 != i: raise SystemExit('fila inesperada en la tabla 0xA4BB')
    S.append((2, strips))
    att = []
    for i in range(17):
        a = 0xC528 + 3 * i; addr = w16(a) - 0x5800; att.append([addr // 32 - 2, addr % 32, b(a + 2) + 1])
    S.append((3, att))
    off = [b(0xB6AC + i) for i in range(45)] + [s16(w16(0xB6D9 + 2 * i)) for i in range(7)]
    S.append((1, [[o] for o in off]))
    acc = {0x82CB: 0, 0x8357: 1, 0x83E3: 2, 0x8478: 3, 0x804E: 4, 0x725E: 5, 0x7710: 6, 0x7877: 7, 0x7319: 8, 0x727E: 9}
    zonas = []
    for i in range(10):
        a = 0xB4F0 + 6 * i
        zonas.append([b(a), b(a + 1), b(a + 2), b(a + 3), acc[w16(a + 4)]])
    if b(0xB4F0 + 60) != 0xFF: raise SystemExit('la tabla de zonas no termina donde se esperaba')
    S.append((5, zonas))
    guiones = []
    for d in range(4):
        a = w16(0xC1B6 + 2 * d); n = 0
        while True:
            v = b(a); a += 1
            if v == 0xFC: break
            if v >= 0xFD:
                dx, dy = s8(b(a)), s8(b(a + 1)); a += 2
                guiones[-1][1:] = [1, dx, dy]
            else:
                guiones.append([v, 0, 0, 0]); n += 1
        if n != 12: raise SystemExit('el guion %d no tiene 12 fotogramas' % d)
        guiones.append([-1, 0, 0, 0])
    S.append((4, guiones))
    S.append((1, [[b(0x8505 + i)] for i in range(9)]))
    S.append((4, [[b(0x85AD + 4 * i + k) for k in range(4)] for i in range(11)]))
    S.append((1, [[b(0xB5C3 + i)] for i in range(44)]))
    S.append((1, [[b(0xB688 + 4 * i + 3)] for i in range(9)]))
    pociones = []
    for i in range(7):
        n, d = w16(0xAE5F + 4 * i), w16(0xAE5F + 4 * i + 2)
        pociones.append([b(0x829B + i), (n - 0xAE7B) // 10, (d - 0xAEB7) // 29])
    S.append((3, pociones))
    S.append((2, [[s8(b(0x81C0 + 2 * i)), s8(b(0x81C0 + 2 * i + 1))] for i in range(4)]))
    S.append((1, [[b(0xAA81 + i)] for i in range(7)]))
    rec = {0x9A25: (0, 0, 0, 0), 0x7602: (100, 1, 0, 0), 0x7608: (150, 2, 0, 0), 0x761E: (10, 1, 0, 0), 0x7624: (20, 1, 0, 0), 0x7637: (30, 1, 0, 0),
           0x762A: (30, 1, 3, 1), 0x763D: (200, 1, 0, 2)}
    mon = []
    for i in range(20):
        a = 0xAA8C + 5 * i
        mon.append([b(a), b(a + 1), b(a + 2)] + list(rec[w16(a + 3)]))
    S.append((7, mon))
    S.append((2, [[s8(b(0x81C0 + 2 * i)), s8(b(0x81C0 + 2 * i + 1))] for i in range(8)]))
    S.append((4, [[b(0x7FAE + 6 * i + k) for k in range(4)] for i in range(10)]))
    S.append((1, [[b(0x7FEA + i)] for i in range(8)]))
    S.append((1, [[b(0x7B70 + i)] for i in range(12)]))
    return S

def main():
    S = secciones()
    out = bytearray(b'HQVI' + struct.pack('>HH', 1, len(S)))
    for k, datos in S:
        out += struct.pack('>HH', len(datos), k)
        for fila in datos:
            assert len(fila) == k
            out += struct.pack('>%dh' % k, *fila)
    open(os.path.join(ROOT, 'data', 'vista_bin'), 'wb').write(out)
    print('vista_bin                %6d bytes, %d secciones' % (len(out), len(S)))

if __name__ == '__main__':
    main()
