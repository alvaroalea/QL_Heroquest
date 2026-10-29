#!/usr/bin/env python3
"""Extrae las 14 misiones de Hero Quest (ZX 128K) de los bancos 1 y 3.
Formato (deducido de INICIAR_MISION, 0x5B3F del overlay):
  +0x000  0x200 bytes  capa 0x6200: byte por casilla (26x19 usadas, 26 por fila): bits 0-5 = nº de habitacion, bit 6 = visible en el plano, bit 7 = visible en 3D
  +0x200  0x200 bytes  capa 0x6400: paredes y puertas (bit 6 pared oeste, 7 pared norte, 4 puerta oeste, 5 puerta norte, 2 puerta abierta, 3 puerta secreta/bloqueada)
  +0x400  heroes: 4 x (x, y, orientacion); 0x56 bytes de indicadores; y la cabecera de secciones
Uso: hq_missions.py [salida_dir]"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
import hq_txt_lib as T
def tabla():
    """punteros de las misiones: tabla del overlay (palabras little endian); las primeras estan en el banco 1 (0xD000-0xFFFF) y, cuando el puntero
    deja de crecer, el resto en el banco 3 (desde 0xC000)"""
    a = T.D['mis_tabla'] - 0x5B00; o = m.OVERLAY
    return [o[a + 2 * k] | o[a + 2 * k + 1] << 8 for k in range(T.D['n_mis'])]
PTR = tabla()
N_MIS = len(PTR)
NOMBRES = ['El laberinto', 'El rescate de Sir Ragnar', 'La guarida de los orcs', 'El oro del Principe Magnus', 'El laberinto de Melar',
           'El legado orco', 'El cazador de piedra', 'Mago del fuego', 'Carrera contra el tiempo', 'El castillo misterioso',
           'El baluarte de Chaos', 'Barak Tor', 'La espada espiritual', 'Regreso a Barak Tor']
def mision(n):
    nb1 = next((k for k in range(1, N_MIS) if PTR[k] < PTR[k - 1]), N_MIS)       # misiones en el banco 1
    bank = m.BANKS[1 if n < nb1 else 3]
    off = PTR[n] - 0xC000
    d = bank[off:off + 0x900]
    r = dict(n=n, nombre=NOMBRES[n] if m.JUEGO == 'hq' else '', bank=1 if n < nb1 else 3, ptr=PTR[n])
    r['capa_a'] = d[0:0x200]; r['capa_b'] = d[0x200:0x400]
    p2 = d[0x400:]
    r['heroes'] = [tuple(p2[i*3:i*3+3]) for i in range(4)]
    r['flags'] = p2[12:12 + 0x56]
    h = p2[0x62:]
    r['hdr'] = h[:11]
    t_off = h[0] | h[1] << 8; t_len = h[2] | h[3] << 8
    r['texto'] = bytes(p2[0x62 + t_off:0x62 + t_off + t_len])
    mo_off = h[4] | h[5] << 8; mo_len = h[6] | h[7] << 8
    r['monstruos'] = [tuple(p2[0x62 + mo_off + i:0x62 + mo_off + i + 10]) for i in range(0, mo_len, 10)]
    ob_off = h[8] | h[9] << 8; ob_n = h[10]
    r['objetos'] = [tuple(p2[0x62 + ob_off + i*3:0x62 + ob_off + i*3 + 3]) for i in range(ob_n)]
    return r
def ascii_mapa(r):
    A, B = r['capa_a'], r['capa_b']
    obj = {}
    for x, y, c in r['objetos']: obj[(x, y)] = c
    mon = {(mm[1], mm[2]): mm[0] for mm in r['monstruos']}
    her = {(h[0], h[1]): i for i, h in enumerate(r['heroes'])}
    lines = []
    for y in range(19):
        l1 = ''; l2 = ''
        for x in range(26):
            a = A[y*26+x]; b = B[y*26+x]; room = a & 0x3F
            l1 += ('+' if True else ' ') + ('-' if b & 0x80 and not b & 0x20 else '=' if b & 0x20 else ' ')
            ch = '.' if room else ' '
            if (x, y) in her: ch = 'H'
            elif (x, y) in mon: ch = 'M'
            elif (x, y) in obj: ch = 'o'
            l2 += ('|' if b & 0x40 and not b & 0x10 else '!' if b & 0x10 else ' ') + ch
        lines += [l1 + '+', l2 + '|']
    return '\n'.join(lines)
if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else None
    for n in range(N_MIS):
        r = mision(n)
        txt = 'Mision %d: %s (banco %d, 0x%04X)\nheroes %s\nmonstruos %d objetos %d\n%s\n' % (n, r['nombre'], r['bank'], r['ptr'], r['heroes'], len(r['monstruos']), len(r['objetos']), ascii_mapa(r))
        if out:
            os.makedirs(out, exist_ok=True); open(os.path.join(out, 'mision%02d.txt' % n), 'w').write(txt)
        elif n < 1: print(txt)
