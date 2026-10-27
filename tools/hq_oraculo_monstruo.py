#!/usr/bin/env python3
"""Oraculo del turno de los monstruos: ejecuta en el simulador Z80 las rutinas ORIGINALES TURNO_MONSTRUO (0x9D6B), CALCULAR_CAMINO (0x9DC0) y EJECUTAR_CAMINO
(0xA075) con el ataque (0x990D, 0x9921) para un monstruo o para una ronda de los 25, y vuelca en build/oraculo_monstruo.txt el estado de antes y de despues
y los ataques y guiones que el original habria mostrado, que compara tests/monstruo_test.c con src/monstruo.c.
Las rutinas de dibujar y de esperar se interceptan; los dados (0x9BD3) devuelven los valores de una lista (el bit 8 del valor pide R = 0 y el 9, R = 5).
Uso: tmp/venv/bin/python -I tools/hq_oraculo_monstruo.py   (desde la raiz del proyecto)"""
import sys, os, random
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import hq_oraculo_partida as op
import hq_oraculo_combate as oc
from skoolkit.simutils import PC, SP
peek, poke, hx, HEROES = (getattr(op, k) for k in ('peek', 'poke', 'hx', 'HEROES'))
h, w16 = oc.h, oc.w16
VARS = [0xA98A, 0xA9B6, 0xA9B7, 0xA9BB, 0xA9C3, 0xA9C4, 0xA9AA, 0xA9AB, 0xA9AC, 0xA9AD, 0xA999, 0xA99A, 0xA998]

class Corredor(oc.Corredor):
    def __init__(self, sim, dados):
        super().__init__(sim, dados)
        for a in (0x92A0, 0x926A, 0x91F7, 0x9242): self.salto[a] = self.ret
        del self.salto[0x9921]
        self.log = {0x9921: self.ataque}
    def guion2(self):
        if self.ix() == 0xA438: self.ret(); return
        super().guion2()
    def ataque(self):
        p = w16(self.sim, 0xA9C5)
        self.eventos.append(['A', str(h(self.sim, 0xA9AD)), str((p - 0x6C0C) // 10 + 1)])
    def correr(self, pc, limite=3000000):
        regs = self.sim.registers; mem = self.sim.memory; ops = self.sim.opcodes
        ret = 0x0001
        regs[SP] = (regs[SP] - 2) & 0xFFFF
        mem[regs[SP]] = ret & 0xFF; mem[(regs[SP] + 1) & 0xFFFF] = ret >> 8
        regs[PC] = pc
        n = 0
        while True:
            p = regs[PC]
            if p == ret: return
            if p in self.log: self.log[p]()
            if p in self.salto and (n > 0 or p != pc):
                self.salto[p](); n += 1; continue
            ops[mem[p]](); n += 1
            if n > limite: raise RuntimeError('la rutina %04X no termina' % pc)

def volcar(sim, f, tag):
    for i, a in enumerate(HEROES): f.write('%sH%d %s\n' % (tag, i, hx(peek(sim, a, 26))))
    f.write('%sM %s\n' % (tag, hx(peek(sim, 0x6C0C, 250))))
    f.write('%sL %s\n' % (tag, hx(peek(sim, 0x6200, 512) + peek(sim, 0x6400, 512) + peek(sim, 0x6600, 512))))
    f.write('%sV %s\n' % (tag, ' '.join(str(h(sim, a)) for a in VARS)))

def sala(sim, c, f): return h(sim, 0x6200 + f * 26 + c) & 0x3F

def preparar_caso(rnd, m):
    sim, pers = oc.crear(rnd, m, 'ATAQUE', 0)
    vivos = [k + 1 for k in range(25) if h(sim, 0x6C0C + 10 * k)]
    # los monstruos: sala propia, visibles casi siempre, a veces ocultos, dormidos o con tempestad
    for k in vivos:
        a = 0x6C0C + 10 * (k - 1)
        poke(sim, a + 3, sala(sim, h(sim, a + 1), h(sim, a + 2)))
        c = h(sim, a + 6) & 0xF9
        c |= 1 if rnd.random() < 0.9 else 0
        if rnd.random() < 0.1: c |= 2
        if rnd.random() < 0.1: c |= 4
        poke(sim, a + 6, c)
        poke(sim, a, 0x81 if rnd.random() < 0.06 else 1)
    # los heroes: cerca de algun monstruo y en su sala casi siempre
    for i in range(5):
        a = HEROES[i]
        if i == 4 and rnd.random() < 0.7: poke(sim, a, 0); continue
        if rnd.random() < 0.75 and vivos:
            k = rnd.choice(vivos); mc, mf = h(sim, 0x6C0C + 10 * (k - 1) + 1), h(sim, 0x6C0C + 10 * (k - 1) + 2)
            s = sala(sim, mc, mf)
            cand = [(c, f) for f in range(19) for c in range(26) if sala(sim, c, f) == s and max(abs(c - mc), abs(f - mf)) <= 7]
            c, f = rnd.choice(cand) if cand else (mc, mf)
        else:
            c, f = rnd.choice([(c, f) for f in range(19) for c in range(26) if sala(sim, c, f)])
        poke(sim, a + 1, [c, f]); poke(sim, a + 3, sala(sim, c, f))
        poke(sim, a, 0xFF if rnd.random() < 0.95 else 0)
        if rnd.random() < 0.12: poke(sim, a + 0x18, h(sim, a + 0x18) | 1)
    # lo que ya hay en la capa C: los heroes y monstruos no se pisan
    return sim, vivos

def main():
    rnd = random.Random(31337)
    out = open(os.path.join(ROOT, 'build/oraculo_monstruo.txt'), 'w')
    n = 0
    for m in range(14):
        for rep in range(20):
            tipo = 'RONDA' if rep % 4 == 3 else 'MONSTRUO'
            sim, vivos = preparar_caso(rnd, m)
            if not vivos: continue
            dados = [rnd.randrange(6) | (0x100 if rnd.random() < 0.05 else (0x200 if rnd.random() < 0.15 else 0)) for _ in range(200)]
            nm = rnd.choice(vivos)
            if rnd.random() < 0.85:                                # un heroe cerca del monstruo del caso, sin estados que lo frenen
                a = 0x6C0C + 10 * (nm - 1)
                if h(sim, a) == 0x81: poke(sim, a, 1)
                if rnd.random() < 0.8: poke(sim, a + 6, h(sim, a + 6) & 0xF8 | 1)
                mc, mf = h(sim, a + 1), h(sim, a + 2)
                s = sala(sim, mc, mf)
                cand = [(c, f) for f in range(19) for c in range(26) if sala(sim, c, f) == s and max(abs(c - mc), abs(f - mf)) <= 8 and (c, f) != (mc, mf)]
                if cand:
                    i = rnd.randrange(4); c, f = rnd.choice(cand)
                    ha = HEROES[i]
                    poke(sim, ha + 1, [c, f]); poke(sim, ha + 3, s); poke(sim, ha, 0xFF); poke(sim, ha + 0x18, h(sim, ha + 0x18) & 0xFE)
            out.write('CASO %d %s %d | %s\n' % (m, tipo, nm, ' '.join(map(str, dados))))
            volcar(sim, out, 'I')
            cor = Corredor(sim, dados)
            lista = [nm] if tipo == 'MONSTRUO' else list(range(1, 26))
            for k in lista:
                a = 0x6C0C + 10 * (k - 1)
                r = sim.registers; r[10] = a >> 8; r[11] = a & 255
                cor.correr(0x9D6B)
            volcar(sim, out, 'P')
            out.write('E %s\n' % ' ; '.join(' '.join(e) for e in cor.eventos))
            n += 1
    out.close()
    print('oraculo de los monstruos escrito:', n, 'casos')

if __name__ == '__main__':
    main()
