#!/usr/bin/env python3
"""Oraculo de la vista: ejecuta en el simulador Z80 las rutinas ORIGINALES de la vista isometrica y del plano y vuelca el resultado en
build/oraculo_vista.txt, que compara tests/vista_test.c con src/vista.c.
Para cada caso (mision, posicion de los 4 heroes y heroe activo, tras cargar la mision como en tools/hq_oraculo_partida.py):
  0x9BE3 BORRAR_ANIMACIONES, buffer de vista a cero, 0x8712 CALCULAR_VISIBLES y 0x8791 ACTUALIZAR_HEROES   -> buffer 'estatico' (suelo y paredes)
  0xA4FD ORDENAR_FIGURAS                                                                                      -> tabla de orden 0xFD00
  0xA794 GUARDAR_FONDOS y 0xA534 DIBUJAR_FIGURAS                                                              -> buffer final
  0x92A0 DIBUJAR_MAPA y 0x94F8 DIBUJAR_PLANO (con el personaje activo y con todos)                            -> buffer y atributos del plano
Uso: tmp/venv/bin/python -I tools/hq_oraculo_vista.py   (desde la raiz del proyecto)"""
import sys, os, random
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import hq_oraculo_partida as op
nuevo, ejecutar, peek, poke, hx, cargar_mision, HEROES = (getattr(op, k) for k in ('nuevo', 'ejecutar', 'peek', 'poke', 'hx', 'cargar_mision', 'HEROES'))
SP = op.SP

ESC = {0xC5BF: 0, 0xC655: 1, 0xE4A8: 10, 0xE520: 11, 0xE598: 12, 0xE610: 13, 0xE688: 14, 0xFF79: 49, 0xFF01: 50}
for k in range(8): ESC[0xC6EB + 150 * k] = 2 + k

def gfx_de(sim, ptr):
    for i in range(66):
        a = 0xC23A + 4 * i
        if peek(sim, a + 2, 2) == [ptr & 0xFF, ptr >> 8]: return 0, i
    if ptr in ESC: return 1, ESC[ptr]
    return -1, -1

def volcar_registros(sim, f):
    for i in range(14):
        a = 0x6A8E + 18 * i
        r = peek(sim, a, 18)
        ptr = r[10] | r[11] << 8
        pack, idx = gfx_de(sim, ptr) if r[0] else (-1, -1)
        f.write('R %d %d %d %d %d %d %d %d %d %d\n' % (i, r[0], r[1], r[6], r[2], r[3], r[4], r[5], pack, idx))

def volcar_estado(sim, f):
    f.write('E %s\n' % hx(peek(sim, 0xA984, 2) + peek(sim, 0xA9D3, 2) + peek(sim, 0xA98E, 1) + peek(sim, 0xA9B0, 2) + peek(sim, 0xAA4D, 1)))
    f.write('A %s\n' % hx(peek(sim, 0x6200, 512)))
    f.write('B %s\n' % hx(peek(sim, 0x6400, 512)))
    f.write('M %s\n' % hx(peek(sim, 0x6C0C, 250)))
    f.write('F %s\n' % hx([peek(sim, 0xC47F + 4 * k, 1)[0] for k in range(43)]))
    for i, a in enumerate(HEROES): f.write('H%d %s\n' % (i, hx(peek(sim, a, 26))))

def buffer(sim): return peek(sim, 0xE700, 0x1600)

def main():
    rnd = random.Random(99)
    out = open(os.path.join(ROOT, 'build/oraculo_vista.txt'), 'w')
    casos = 0
    for m in range(14):
        for rep in range(6):
            sim = nuevo()
            cargar_mision(sim, m, 15, [(0, 0, 0, 0, 0)] * 4)
            sim.registers[SP] = (sim.registers[SP] + 2) & 0xFFFF            # saca la direccion de retorno que dejo la carga
            celdas = [(c, f) for f in range(19) for c in range(26) if peek(sim, 0x6200 + f * 26 + c)[0] & 0x3F]
            pos = [rnd.choice(celdas) for _ in range(4)]
            if rep % 2 == 0: pos[1] = (pos[0][0], pos[0][1])                    # a veces dos heroes en la misma casilla
            if rep >= 3: pos[2] = (min(25, pos[0][0] + 1), pos[0][1]) if peek(sim, 0x6200 + pos[0][1] * 26 + min(25, pos[0][0] + 1))[0] & 0x3F else pos[2]
            activo = rnd.randrange(4)
            for i, (c, f) in enumerate(pos): poke(sim, HEROES[i] + 1, [c, f])
            poke(sim, 0xA998, activo); poke(sim, 0xA9DD, [HEROES[activo] & 0xFF, HEROES[activo] >> 8])
            poke(sim, 0xA9DF, [(0x6A8E + 18 * activo) & 0xFF, (0x6A8E + 18 * activo) >> 8])
            poke(sim, 0xA9A1, 0); poke(sim, 0xA989, 0)
            poke(sim, 0xE700, [0] * 0x1600)
            out.write('CASO %d %d %s\n' % (m, activo, ' '.join('%d %d' % p for p in pos)))
            ejecutar(sim, 0x9BE3)
            ejecutar(sim, 0x8712)
            ejecutar(sim, 0x8791)
            out.write('S %s\n' % hx(buffer(sim)))
            volcar_registros(sim, out)
            volcar_estado(sim, out)
            ejecutar(sim, 0xA4FD)
            out.write('O %s\n' % hx(peek(sim, 0xFD00, 96)))
            ejecutar(sim, 0xA794)
            ejecutar(sim, 0xA534)
            out.write('X %s\n' % hx(buffer(sim)))
            # plano: con todos los personajes y solo con el activo
            for solo in (0, 1):
                poke(sim, 0xA9A1, solo)
                poke(sim, 0xE700, [0] * 0x1600)
                ejecutar(sim, 0x92A0)
                ejecutar(sim, 0x94F8)
                at = [peek(sim, 0x5863 + 32 * f + c)[0] for f in range(19) for c in range(26)]
                out.write('P%d %s\n' % (solo, hx(buffer(sim))))
                out.write('T%d %s\n' % (solo, hx(at)))
            casos += 1
    out.close()
    print('oraculo de la vista escrito:', casos, 'casos')

if __name__ == '__main__':
    main()
