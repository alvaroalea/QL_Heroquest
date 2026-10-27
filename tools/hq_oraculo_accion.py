#!/usr/bin/env python3
"""Oraculo de las acciones de los iconos: ejecuta en el simulador Z80 las rutinas ORIGINALES de abrir puertas (0x727E), buscar trampas y puertas secretas (0x8079),
buscar tesoros (0x80A9) y los eventos de las casillas con trampa (0xA108) y vuelca en build/oraculo_accion.txt el estado de antes y de despues y las ventanas
que el original habria mostrado, que compara tests/accion_test.c con src/accion.c.
Las rutinas de dibujar y de esperar (ventanas, volcados, HALT) se interceptan: se anota lo que se habria mostrado y se vuelve. Los dados (0x9BD3) devuelven
los valores de una lista que el caso lleva en su cabecera; tambien los usa el C.
Uso: tmp/venv/bin/python -I tools/hq_oraculo_accion.py   (desde la raiz del proyecto)"""
import sys, os, random
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import hq_oraculo_partida as op
from skoolkit.simutils import PC, SP
nuevo, peek, poke, hx, cargar_mision, HEROES = (getattr(op, k) for k in ('nuevo', 'peek', 'poke', 'hx', 'cargar_mision', 'HEROES'))
A_, B_, C_ = 0, 2, 3
def h(sim, a): return peek(sim, a, 1)[0]
def w16(sim, a): return h(sim, a) | h(sim, a + 1) << 8

class Corredor:
    def __init__(self, sim, dados):
        self.sim = sim; self.dados = list(dados); self.eventos = []
        self.salto = {0x8DD1: self.ventana, 0x8D99: self.ventana_repinta, 0x8FCF: self.ventana_repinta, 0x9D5E: self.guion, 0x991B: self.guion,
                      0x9D64: self.ret, 0x9D66: self.ret, 0x9C30: self.ret, 0x9C2F: self.ret, 0x8DB2: self.repinta, 0x8D03: self.ret, 0x8FDB: self.ret,
                      0x8009: self.ret, 0x9921: self.ret, 0x926A: self.ret, 0x91F7: self.ret, 0x9242: self.ret, 0x9BD3: self.dado, 0x7BA2: self.ret, 0x7BA4: self.ret, 0x7B9E: self.ret,
                      0x827A: self.especial}
        self.fin = False
    def ix(self): r = self.sim.registers; return r[8] << 8 | r[9]
    def ret(self):
        r = self.sim.registers; mem = self.sim.memory
        sp = r[SP]; r[PC] = mem[sp] | mem[(sp + 1) & 0xFFFF] << 8; r[SP] = (sp + 2) & 0xFFFF
    def ventana(self):
        ix = self.ix(); ev = ['W', '%04X' % ix]
        if ix == 0xADDE: ev.append(''.join(chr(h(self.sim, 0xADF0 + i)) for i in range(3)))
        if ix == 0xAE03: ev += [hx(peek(self.sim, 0xAE28, 10)).replace(' ', ''), hx(peek(self.sim, 0xAE42, 29)).replace(' ', '')]
        self.eventos.append(ev); self.ret()
    def ventana_repinta(self):
        self.ventana(); poke(self.sim, 0xA987, 1); poke(self.sim, 0xA989, 1); poke(self.sim, 0xA9DB, [0xF0, 0xB4])
    def guion(self):
        self.eventos.append(['S', '%04X' % self.ix()]); self.ret()
    def repinta(self):
        poke(self.sim, 0xA989, 1); self.ret()
    def dado(self):
        v = self.dados.pop(0) if self.dados else 0
        self.sim.registers[C_] = v & 0xFF
        self.sim.registers[A_] = 0 if v & 0x100 else (5 if v & 0x200 else 0x55)          # DADO deja en A el R que ha leido (el bit 8 del valor pide R = 0 y el 9, R = 5)
        self.ret()
    def especial(self):
        r = self.sim.registers
        hl = r[6] << 8 | r[7]; valor = r[A_]
        self.eventos.append(['X', '1' if 0x6BB6 <= hl < 0x6BE1 else '0', str(valor)])
        # termina la rutina: se vuelve al llamante
        self.fin = True; self.ret()
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
            if p in self.salto and (n > 0 or p != pc):
                self.salto[p]()
                if self.fin:
                    # el especial salta con JP: la rutina de entrada termina; saca lo que hubiera en la pila hasta el centinela
                    while regs[PC] != ret:
                        ops[mem[regs[PC]]]()
                    return
                n += 1; continue
            ops[mem[p]]()
            n += 1
            if n > limite: raise RuntimeError('la rutina %04X no termina' % pc)

def iy_ix(sim, pers):
    iy = HEROES[pers]; ix = 0x6A8E + 18 * (13 if pers == 4 else pers)
    r = sim.registers
    r[8] = ix >> 8; r[9] = ix & 0xFF; r[10] = iy >> 8; r[11] = iy & 0xFF

def preparar(sim, pers):
    poke(sim, 0xA998, pers); poke(sim, 0xA9DD, [HEROES[pers] & 0xFF, HEROES[pers] >> 8])
    a = 0x6A8E + 18 * (13 if pers == 4 else pers)
    poke(sim, 0xA9DF, [a & 0xFF, a >> 8])

def construir(sim, pers):
    op.ejecutar(sim, 0x9BE3); poke(sim, 0xA989, 0); poke(sim, 0xA987, 0)
    op.ejecutar(sim, 0x8712); op.ejecutar(sim, 0x8791)

def volcar(sim, f, tag):
    for i, a in enumerate(HEROES): f.write('%sH%d %s\n' % (tag, i, hx(peek(sim, a, 26))))
    f.write('%sM %s\n' % (tag, hx(peek(sim, 0x6C0C, 250))))
    f.write('%sL %s\n' % (tag, hx(peek(sim, 0x6200, 512) + peek(sim, 0x6400, 512) + peek(sim, 0x6600, 512))))
    f.write('%sI %s\n' % (tag, hx(peek(sim, 0x6BB6, 86))))
    f.write('%sD %s | %04X\n' % (tag, hx(peek(sim, 0x9593, 26)), w16(sim, 0xA9E1)))
    f.write('%sF %s\n' % (tag, hx([h(sim, 0xC47F + 4 * k) for k in range(43)])))
    v = [h(sim, a) for a in (0xA984, 0xA9D3, 0xA9D4, 0xA98B, 0xA98C, 0xA98A, 0xA9B3, 0xA9B6, 0xA9B7, 0xA9BB, 0xA9BC, 0xA9B8, 0xA98E, 0xA9B1, 0xA987)]
    f.write('%sV %s\n' % (tag, ' '.join(map(str, v))))

def main():
    rnd = random.Random(777)
    out = open(os.path.join(ROOT, 'build/oraculo_accion.txt'), 'w')
    n = 0
    for m in range(14):
        for rep in range(24):
            sim = nuevo()
            cargar_mision(sim, m, 15, [(0, 0, 0, 0, 0)] * 4)
            sim.registers[SP] = (sim.registers[SP] + 2) & 0xFFFF
            poke(sim, 0x6FD6, 1)
            A = peek(sim, 0x6200, 512); B = peek(sim, 0x6400, 512); C = peek(sim, 0x6600, 512)
            celdas = [(c, f) for f in range(19) for c in range(26) if A[f * 26 + c] & 0x3F]
            def cerca(pred):
                l = [(c, f) for (c, f) in celdas if pred(c, f)]
                return l
            puertas = cerca(lambda c, f: (B[f * 26 + c] & 0x38) or (c < 25 and B[f * 26 + c + 1] & 0x18) or (f < 18 and B[(f + 1) * 26 + c] & 0x28))
            trampas = cerca(lambda c, f: any(0 <= c + dx < 26 and 0 <= f + dy < 19 and C[(f + dy) * 26 + c + dx] in (0x32, 0x34, 0x36) for dx in (-1, 0, 1) for dy in (-1, 0, 1)))
            tipo = rnd.choice(['LLAVE', 'TRAMPAS', 'TRAMPAS', 'TESOROS', 'TESOROS', 'TESOROS', 'EVENTO'])
            pers = rnd.randrange(4)
            pos = [rnd.choice(celdas) for _ in range(4)]
            lista = {'LLAVE': puertas, 'TRAMPAS': (puertas + trampas) or celdas, 'TESOROS': celdas, 'EVENTO': celdas}[tipo] or celdas
            if rnd.random() < 0.8: pos[pers] = rnd.choice(lista)
            for i, (c, f) in enumerate(pos): poke(sim, HEROES[i] + 1, [c, f])
            preparar(sim, pers)
            # estado de partida al azar
            ind = [rnd.choice([0] * 6 + [0xFF, 0xFF, 3, 7, 12]) for _ in range(86)]
            poke(sim, 0x6BB6, ind)
            deck = list(range(25)); rnd.shuffle(deck)
            poke(sim, 0x9593, deck + [0xFF]); poke(sim, 0xA9E1, [0xAC, 0x95])
            for i in range(4):
                a = HEROES[i]
                poke(sim, a + 6, rnd.choice([1, 1, 2, 6, 8])); poke(sim, a + 0x0F, rnd.choice([0, 2, 0x0F, 0x80])); poke(sim, a + 0x13, rnd.choice([0, 2, 8]))
                poke(sim, a + 9, [rnd.randrange(256), rnd.randrange(3)])
            for k in range(25):                                              # monstruos visibles o no
                a = 0x6C0C + 10 * k + 6
                if h(sim, a - 6): poke(sim, a, h(sim, a) & 0xFE | (1 if rnd.random() < 0.15 else 0))
            poke(sim, 0xA98B, rnd.choice([0, 1]))
            poke(sim, 0xA98C, rnd.choice([0, 3, 7]))
            poke(sim, 0xA9B6, rnd.choice([0, 0, 1, 7, 14]))
            construir(sim, pers)
            ev = rnd.randrange(1, 5)
            dados = [rnd.randrange(6) for _ in range(8)]
            out.write('CASO %d %d %s | %s %d %s\n' % (m, pers, ' '.join('%d %d' % p for p in pos), tipo, ev, ' '.join(map(str, dados))))
            volcar(sim, out, 'I')
            cor = Corredor(sim, dados)
            iy_ix(sim, pers)
            if tipo == 'LLAVE': cor.correr(0x727E)
            elif tipo == 'TRAMPAS': cor.correr(0x8079)
            elif tipo == 'TESOROS': cor.correr(0x80A9)
            else:
                poke(sim, 0xA9B3, ev); poke(sim, 0xA9B1, 0)
                cor.correr(0xA108)
            volcar(sim, out, 'P')
            out.write('E %s\n' % ' ; '.join(' '.join(e) for e in cor.eventos))
            n += 1
    out.close()
    print('oraculo de las acciones escrito:', n, 'casos')

if __name__ == '__main__':
    main()
