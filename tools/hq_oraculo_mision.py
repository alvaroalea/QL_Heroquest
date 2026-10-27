#!/usr/bin/env python3
"""Oraculo de lo propio de cada mision: ejecuta en el simulador Z80 las rutinas ORIGINALES de las salas especiales (0x827A y la tabla 0x82A2), de los eventos de
las misiones 5, 9 y 11 (0x70A1), de la salida por la escalera (0x8E7B y la tabla 0x8FE9) y de las busquedas del ojo con la emboscada (0x80A9, 0x8079), y vuelca
en build/oraculo_mision.txt el estado de antes y de despues y las ventanas, preguntas y ataques que el original habria mostrado, que compara tests/mision_test.c
con src/mision.c. Las preguntas de si o no (0x8D5E) devuelven los valores de una lista del caso. Uso: tmp/venv/bin/python -I tools/hq_oraculo_mision.py"""
import sys, os, random
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import hq_oraculo_partida as op
import hq_oraculo_combate as oc
import hq_oraculo_monstruo as om
from skoolkit.simutils import PC, SP
peek, poke, hx, HEROES = (getattr(op, k) for k in ('peek', 'poke', 'hx', 'HEROES'))
h, w16 = oc.h, oc.w16
VARS = [0xA98A, 0xA9B6, 0xA9B7, 0xA9BB, 0xA9AE, 0xA9AF, 0xA9B2, 0xA9B4, 0xA9B1, 0xA9C3, 0xA9C4, 0xA9AD, 0xA998, 0xA9B0]
MARCAS = 0x8FE4

class Corredor(om.Corredor):
    def __init__(self, sim, dados, respuestas):
        super().__init__(sim, dados)
        self.resp = list(respuestas)
        self.salto[0x8D5E] = self.pregunta
        self.salto[0x9BF2] = self.ret
        self.salto[0x9C5B] = self.ret
        self.log[0x91F7] = lambda: self.eventos.append(['G'])
        self.salto.pop(0x91F7, None)
    def pregunta(self):
        r = self.sim.registers
        self.eventos.append(['Q', '%04X' % self.ix()])
        v = self.resp.pop(0) if self.resp else 0
        r[0] = v; self.ret()

def volcar(sim, f, tag):
    for i, a in enumerate(HEROES): f.write('%sH%d %s\n' % (tag, i, hx(peek(sim, a, 26))))
    f.write('%sM %s\n' % (tag, hx(peek(sim, 0x6C0C, 250))))
    f.write('%sL %s\n' % (tag, hx(peek(sim, 0x6200, 512) + peek(sim, 0x6400, 512) + peek(sim, 0x6600, 512))))
    f.write('%sI %s\n' % (tag, hx(peek(sim, 0x6BB6, 86))))
    f.write('%sS %s\n' % (tag, hx(peek(sim, MARCAS, 4))))
    f.write('%sQ %s\n' % (tag, hx(peek(sim, 0xA2B2, 15))))
    f.write('%sV %s\n' % (tag, ' '.join(str(h(sim, a)) for a in VARS)))

def sala(sim, c, f): return h(sim, 0x6200 + f * 26 + c) & 0x3F

def main():
    rnd = random.Random(2026)
    out = open(os.path.join(ROOT, 'build/oraculo_mision.txt'), 'w')
    n = 0
    for m in range(14):
        for rep in range(40):
            tipo = ['ESPECIAL'] * 4 + ['SALIDA'] * 2 + ['EVENTO'] + ['ACCION']
            tipo = tipo[rep % len(tipo)]
            if m in (5, 9, 11) and rep % 2 == 1: tipo = 'EVENTO'
            sim, pers = oc.crear(rnd, m, 'ATAQUE', 0)
            if tipo == 'SALIDA' and m not in (0, 1, 3, 6, 9) and rnd.random() < 0.7: tipo = 'ESPECIAL'
            if tipo == 'EVENTO' and m not in (5, 9, 11) and rnd.random() < 0.8: tipo = 'ESPECIAL'
            vivos = [k + 1 for k in range(25) if h(sim, 0x6C0C + 10 * k)]
            dados = [rnd.randrange(6) | (0x100 if rnd.random() < 0.1 else 0) for _ in range(60)]
            resp = [rnd.randrange(2) for _ in range(4)]
            for i in range(4): poke(sim, HEROES[i] + 3, sala(sim, h(sim, HEROES[i] + 1), h(sim, HEROES[i] + 2)))
            if rnd.random() < 0.4:
                poke(sim, HEROES[pers] + 6, rnd.choice([1, 1, 2]))
            ind = [rnd.choice([0, 0, 0xFF, 3]) for _ in range(86)]
            poke(sim, 0x6BB6, ind)
            for k in vivos: poke(sim, 0x6C0C + 10 * (k - 1), 1)
            poke(sim, 0xA9AF, rnd.choice([0, 1, 2, 3])); poke(sim, 0xA9AE, rnd.choice([0, 0, 1]))
            poke(sim, 0xA9B2, rnd.choice([0, 0, 1, 2, 5])); poke(sim, 0xA9B4, rnd.choice([0, 0, 1]))
            poke(sim, 0xA9B0, rnd.randrange(4)); poke(sim, 0xA9B7, rnd.choice([0, 0, 15, 7, 3])); poke(sim, 0xA9B6, h(sim, 0xA9B7))
            for i in range(4): poke(sim, MARCAS + i, rnd.choice([0, 0, 0xFF]))
            a = HEROES[pers]
            if rnd.random() < 0.3: poke(sim, a + 0x0F, h(sim, a + 0x0F) | 0x40)
            if rnd.random() < 0.2: poke(sim, a + 0x0F, h(sim, a + 0x0F) | 0x10)
            par = ''
            if tipo == 'ESPECIAL':
                v = rnd.randrange(1, 19)
                idx = rnd.randrange(0, 43) if rnd.random() < 0.9 else rnd.randrange(43, 86)
                ind[idx] = v; poke(sim, 0x6BB6 + idx, v)
                if idx < 43: poke(sim, 0x6BB6 + idx + 43, rnd.choice([0, 0xFF, 5]))
                par = '%d %d' % (idx, v)
            elif tipo == 'SALIDA':
                if m == 1 and rnd.random() < 0.8: pers = 4
                par = ''
            elif tipo == 'EVENTO':
                poke(sim, 0xA9B1, 1)
            else:
                par = str(rnd.choice([0, 1]))
            poke(sim, 0xA998, pers)
            oc.preparar(sim, pers)
            out.write('CASO %d %d %s %s | %s | %s\n' % (m, pers, tipo, par, ' '.join(map(str, resp)), ' '.join(map(str, dados))))
            volcar(sim, out, 'I')
            cor = Corredor(sim, dados, resp)
            if tipo == 'ACCION': cor.log.pop(0x91F7, None)
            oc.preparar(sim, pers)
            r = sim.registers
            if tipo == 'ESPECIAL':
                idx, v = map(int, par.split())
                r[6] = (0x6BB6 + idx) >> 8; r[7] = (0x6BB6 + idx) & 255; r[0] = v
                cor.correr(0x827A)
            elif tipo == 'SALIDA':
                cor.correr(0x8E7B)
            elif tipo == 'EVENTO':
                cor.correr(0x70A1)
            else:
                if par == '0': cor.correr(0x8079)
                else: cor.correr(0x80A9)
            volcar(sim, out, 'P')
            out.write('E %s\n' % ' ; '.join(' '.join(e) for e in cor.eventos))
            n += 1
    out.close()
    print('oraculo de las misiones escrito:', n, 'casos')

if __name__ == '__main__':
    main()
