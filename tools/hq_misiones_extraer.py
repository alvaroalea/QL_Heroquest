#!/usr/bin/env python3
"""Extrae las 14 misiones a ficheros de texto editables asset/misiones/misionNN.txt (formato en docs/ZX_DATOS_QL.md).
NO escribe en asset/misiones_ql/ (alli van las misiones retocadas). El mapa tiene 26 columnas x 19 filas; cada casilla es
'AA.BB' (hexadecimal): AA = capa A (bits 0-5 numero de habitacion, bit 6 visible en el plano, bit 7 visible en 3D),
BB = capa B (bit 6 pared oeste, bit 7 pared norte, bit 4 puerta oeste, bit 5 puerta norte, bit 2 puerta abierta, bit 3 puerta secreta o bloqueada).
Uso: hq_misiones_extraer.py [directorio_salida]"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
import hq_missions as hm
import hq_txt_lib as T
ROOT = T.ROOT
def nombre_objeto(c):
    idx = m.byte(0xB5C3 + c - 0x30) if 0x30 <= c < 0x5D else 0; a = 0xB5F2 + 10 * idx
    return ''.join(chr(m.byte(a + k)) for k in range(10) if 0x30 <= m.byte(a + k) < 0x5F).strip().lower()
def nombres_menu():
    """los 14 nombres del menu de misiones (guion 0x689A del overlay, una linea por fila 4..17)"""
    seg, _ = T.guion(T.b_ov, 0x689A, None); lin = [l for l in T.guion_a_lineas(seg) if l[0] == 'texto']
    por_fila = {l[3]: T.decodificar(l[4]).strip(' ') for l in lin}
    return [por_fila[4 + i] for i in range(14)]
def main(dest):
    os.makedirs(dest, exist_ok=True); nombres = nombres_menu()
    for n in range(14):
        r = hm.mision(n); A, B = r['capa_a'], r['capa_b']; L = []
        L.append('# Mision %d de Hero Quest (original ZX 128K: banco %d, 0x%04X) - generado por tools/hq_misiones_extraer.py' % (n, r['bank'], r['ptr']))
        L.append('# EDITAR LA COPIA de asset/misiones_ql/ (este fichero se regenera). Formato y significado de los campos en docs/ZX_DATOS_QL.md.')
        L.append('')
        L.append('mision %d' % n)
        L.append('nombre %s' % nombres[n])
        L.append('')
        L.append('# heroes: indice|columna|fila|grafico inicial (se asigna a los 4 heroes en orden: barbaro, enano, elfo, hechicero)')
        for i, h in enumerate(r['heroes']): L.append('heroe %d|%d|%d|%d' % ((i,) + tuple(h)))
        L.append('')
        L.append('[mapa] 26 columnas x 19 filas, cada casilla AA.BB (capa A . capa B) en hexadecimal')
        for y in range(19): L.append(' '.join('%02X.%02X' % (A[y * 26 + x], B[y * 26 + x]) for x in range(26)))
        L.append('')
        L.append('[indicadores] 86 bytes iniciales (se copian a 0x6BB6 del original): marcas de habitaciones, cofres y puertas ya usados')
        fl = list(r['flags'])
        for i in range(0, 86, 26): L.append(' '.join('%02X' % v for v in fl[i:i + 26]))
        L.append('')
        L.append('[monstruos] estado|columna|fila|b3|b4|b5|b6|b7|tipo|estadistica   (10 bytes del original; b3-b5 los pone el juego)')
        for mo in r['monstruos']: L.append('monstruo|' + '|'.join(str(v) for v in mo))
        L.append('')
        L.append('[objetos] columna|fila|codigo   (codigos de la capa de objetos: 0x30-0x5B; el nombre es solo orientativo)')
        for x, y, c in r['objetos']: L.append('objeto|%d|%d|%d ; 0x%02X %s' % (x, y, c, c, nombre_objeto(c)))
        L.append('')
        L.append('[objetivo] texto de presentacion: atributo(hex)|alineacion(L/C/R)|columna|fila|texto')
        tx = r['texto']; seg, _ = T.guion(lambda a: tx[a] if a < len(tx) else 0, 0, len(tx))      # el intérprete del juego se detiene en el primer 00 o FA
        for l in T.guion_a_lineas(seg):
            if l[0] != 'texto': continue
            _, at, x, y, bs = l; t = T.decodificar(bs); lead = len(t) - len(t.lstrip(' ')); core = t.strip(' ')
            if core: x += lead; t = core
            al, col = T.alineacion(x, len(t)); L.append('%02X|%s|%d|%d|%s' % (at, al, col, y, t))
        open(os.path.join(dest, 'mision%02d.txt' % n), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
    print('14 misiones en', dest)
if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'asset/misiones'))
