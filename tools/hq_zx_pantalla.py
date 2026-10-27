#!/usr/bin/env python3
"""Pinta en un PNG la pantalla del Spectrum despues de ejecutar rutinas ORIGINALES de interfaz (simulador Z80 de SkoolKit), para ver como son las pantallas
que hay que portar (combate, panel del personaje...). Las esperas (HALT) se anulan.
Uso: tmp/venv/bin/python -I tools/hq_zx_pantalla.py ESCENA SALIDA.png   ESCENA = ataque | defensa | panel | panel0 | ataque_fallo | ojo
(desde la raiz del proyecto)"""
import sys, os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import hq_oraculo_partida as op
import hq_png
from skoolkit.simutils import PC, SP
nuevo, ejecutar, peek, poke, cargar_mision, HEROES = (getattr(op, k) for k in ('nuevo', 'ejecutar', 'peek', 'poke', 'cargar_mision', 'HEROES'))
A_, B_, C_, D_, E_, H_, L_ = 0, 2, 3, 4, 5, 6, 7
COL = [(0, 0, 0), (0, 0, 205), (205, 0, 0), (205, 0, 205), (0, 205, 0), (0, 205, 205), (205, 205, 0), (205, 205, 205)]
BRI = [(0, 0, 0), (0, 0, 255), (255, 0, 0), (255, 0, 255), (0, 255, 0), (0, 255, 255), (255, 255, 0), (255, 255, 255)]

def pantalla(sim, ruta):
    m = sim.memory; filas = []
    for y in range(192):
        base = 0x4000 + ((y & 7) << 8) + (((y >> 3) & 7) << 5) + ((y >> 6) << 11)
        fila = []
        for x in range(256):
            at = m[0x5800 + (y >> 3) * 32 + (x >> 3)]
            pal = BRI if at & 0x40 else COL
            bit = (m[base + (x >> 3)] >> (7 - (x & 7))) & 1
            fila.append(pal[(at & 7) if bit else ((at >> 3) & 7)] + (255,))
        filas.append(fila)
    hq_png.escribir_rgba(ruta, 256, 192, [[p for p in f for _ in (0, 1)] for f in filas for _ in (0, 1)] if False else filas)

def llamar(sim, pc, **r):
    for k, v in r.items(): sim.registers[{'a': A_, 'b': B_, 'c': C_, 'd': D_, 'e': E_, 'h': H_, 'l': L_}[k]] = v
    ejecutar(sim, pc)

def ix(sim, v): sim.registers[8] = v >> 8; sim.registers[9] = v & 255

def main():
    escena, ruta = sys.argv[1], sys.argv[2]
    sim = nuevo()
    cargar_mision(sim, 0, 15, [(0, 0, 0, 0, 0)] * 4)
    sim.registers[SP] = (sim.registers[SP] + 2) & 0xFFFF
    poke(sim, 0x9C31, 0); poke(sim, 0x9D67, 0)                        # sin HALT
    poke(sim, 0x6FD6, 1)
    pers = 2 if escena in ('panel', 'defensa') else 0
    iy = HEROES[pers]
    poke(sim, 0xA998, pers); poke(sim, 0xA9DD, [iy & 255, iy >> 8])
    a = 0x6A8E + 18 * pers; poke(sim, 0xA9DF, [a & 255, a >> 8])
    mon = 0x6C0C + 10 * 0
    poke(sim, 0xA9C5, [mon & 255, mon >> 8])
    if escena in ('panel', 'panel0'):
        poke(sim, iy + 0x0B, 0x2B); poke(sim, iy + 0x0C, 0x15); poke(sim, iy + 0x0D, 0x0F); poke(sim, iy + 0x0E, 0x7F); poke(sim, iy + 0x0F, 0xEF)
        poke(sim, iy + 0x10, 0x7F); poke(sim, iy + 0x11, 0x02); poke(sim, iy + 0x12, 0x02); poke(sim, iy + 0x13, 0x03); poke(sim, iy + 0x17, 0x05); poke(sim, 0xA99B, 0xFF)
        ejecutar(sim, 0x8FDB); llamar(sim, 0x9C30, a=0x44); llamar(sim, 0x95BE); ejecutar(sim, 0x8D03)
    elif escena in ('ataque', 'ataque_fallo'):
        poke(sim, 0xA9A9, 4 if escena == 'ataque' else 0); poke(sim, 0xA9AC, 1 if escena == 'ataque' else 6)
        ejecutar(sim, 0x8FDB); llamar(sim, 0x9C30, a=7); ix(sim, 0xAA88); ejecutar(sim, 0x8BF9); ejecutar(sim, 0x9C62); ejecutar(sim, 0xA3E1); ejecutar(sim, 0x764A)
        ejecutar(sim, 0x751D); ejecutar(sim, 0x99A2)
    elif escena == 'ojo':
        ejecutar(sim, 0x8FDB); ix(sim, 0xAFE2); ejecutar(sim, 0x8DD1)
    elif escena == 'defensa':
        poke(sim, 0xA9AD, pers); poke(sim, 0xA9C7, [iy & 255, iy >> 8]); poke(sim, 0xA9AB, 3)
        ejecutar(sim, 0x9921)
    pantalla(sim, ruta)

main()
