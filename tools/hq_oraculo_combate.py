#!/usr/bin/env python3
"""Oraculo del combate y la bolsa: ejecuta en el simulador Z80 las rutinas ORIGINALES del ataque (preparar, alcance y linea de vista, bonos, tirada y muerte
del monstruo con su recompensa), de la defensa de un heroe (PANEL_COMBATE 0x9921), de lanzar un hechizo (0x78F6 y los efectos hasta 0x7B15) y de los
manejadores del panel del personaje (0x7C45-0x7E61), y vuelca en build/oraculo_combate.txt el estado de antes y de despues y los guiones que el original
habria mostrado, que compara tests/combate_test.c con src/combate.c.
Las rutinas de dibujar y de esperar se interceptan (se anota lo que se habria mostrado y se vuelve); los dados (0x9BD3) devuelven los valores de una lista
que el caso lleva en su cabecera. Uso: tmp/venv/bin/python -I tools/hq_oraculo_combate.py   (desde la raiz del proyecto)"""
import sys, os, random
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import hq_oraculo_partida as op
import hq_oraculo_accion as oa
from skoolkit.simutils import PC, SP
nuevo, peek, poke, hx, cargar_mision, HEROES = (getattr(op, k) for k in ('nuevo', 'peek', 'poke', 'hx', 'cargar_mision', 'HEROES'))
A_, B_, C_, D_, E_ = 0, 2, 3, 4, 5
def h(sim, a): return peek(sim, a, 1)[0]
def w16(sim, a): return h(sim, a) | h(sim, a + 1) << 8

# variables que comparan el oraculo y el C, en este orden (direccion del original)
VARS = [0xA98B, 0xA98C, 0xA98A, 0xA9A4, 0xA9A8, 0xA9A9, 0xA9AA, 0xA9AB, 0xA9AC, 0xA9C3, 0xA9C4, 0xA999, 0xA9A3, 0xA9B6, 0xA9B7, 0xA9BB,
        'OBJ', 'CELDA', 0xA9D3, 0xA9D4, 0xA99F, 0xA9A0, 0xA98D, 0xA998, 0xA9A1]

class Corredor(oa.Corredor):
    def __init__(self, sim, dados):
        super().__init__(sim, dados)
        ret = self.ret
        for a in (0x8D03, 0x8FDB, 0x9C30, 0x9C2F, 0x9D64, 0x9D66, 0x9D61, 0x7BA2, 0x7BA4, 0x7B9E, 0x8009, 0x99DB, 0x99F8, 0x9A26, 0x9A4D, 0x9C62, 0xA3E1, 0x764A,
                  0x9C71, 0x95BE, 0x9656, 0x966A, 0x9682, 0x9696, 0x96C3, 0xA3C8, 0x7701, 0x7706, 0x9C3F):
            self.salto[a] = ret
        self.salto[0x8BF9] = self.dibuja
        self.salto[0x9D5E] = self.guion2
        self.salto[0x991B] = self.guion2
        del self.salto[0x827A]
    def dibuja(self):                       # el interprete de guiones: solo se anota el del agua santa
        ix = self.ix()
        if ix in (0xBA2F, 0xBA6A): self.guion2()
        else: self.ret()
    def guion2(self):
        ix = self.ix()
        ev = ['S', '%04X' % ix]
        if ix == 0xBAEA: ev.append('%02X' % h(self.sim, 0xBAFE))
        if ix == 0xBA2F: ev += ['%02X' % h(self.sim, 0xBA58), '%02X' % h(self.sim, 0xBA59)]
        self.eventos.append(ev); self.ret()

def ptr_mons(n): return 0x6C0C + 10 * (n - 1)

def volcar(sim, f, tag):
    for i, a in enumerate(HEROES): f.write('%sH%d %s\n' % (tag, i, hx(peek(sim, a, 26))))
    f.write('%sM %s\n' % (tag, hx(peek(sim, 0x6C0C, 250))))
    f.write('%sL %s\n' % (tag, hx(peek(sim, 0x6200, 512) + peek(sim, 0x6400, 512) + peek(sim, 0x6600, 512))))
    v = []
    for a in VARS:
        if a == 'OBJ':
            p = w16(sim, 0xA9C5); v.append((p - 0x6C0C) // 10 + 1 if 0x6C0C <= p < 0x6C0C + 250 else 0)
        elif a == 'CELDA':
            p = w16(sim, 0xA9C1); v.append(p - 0x6600 if 0x6600 <= p < 0x6800 else 0)
        else: v.append(h(sim, a))
    f.write('%sV %s\n' % (tag, ' '.join(map(str, v))))

def preparar(sim, pers):
    poke(sim, 0xA998, pers); poke(sim, 0xA9DD, [HEROES[pers] & 0xFF, HEROES[pers] >> 8])
    a = 0x6A8E + 18 * (13 if pers == 4 else pers)
    poke(sim, 0xA9DF, [a & 0xFF, a >> 8])
    r = sim.registers
    r[10] = HEROES[pers] >> 8; r[11] = HEROES[pers] & 0xFF; r[8] = a >> 8; r[9] = a & 0xFF

def azar_heroe(sim, rnd, pers):
    a = HEROES[pers]
    armas = rnd.choice([0, 0x7F, rnd.randrange(128), rnd.randrange(128)])
    sel = [b for b in range(7) if armas >> b & 1]
    poke(sim, a + 6, rnd.randrange(1, 9)); poke(sim, a + 7, rnd.randrange(1, 7))
    poke(sim, a + 0x0B, rnd.randrange(64)); poke(sim, a + 0x0C, rnd.randrange(64))
    poke(sim, a + 0x0D, rnd.choice([0, rnd.randrange(32)])); poke(sim, a + 0x0E, armas)
    poke(sim, a + 0x0F, rnd.choice([rnd.randrange(16), rnd.randrange(256), rnd.randrange(16) | 0x80, rnd.randrange(16) | 0x20]))
    poke(sim, a + 0x10, rnd.randrange(128))
    poke(sim, a + 0x11, 0 if not sel or rnd.random() < 0.15 else 1 << rnd.choice(sel))
    poke(sim, a + 0x12, rnd.choice([0, 0, 8, 0x10, 0x18, 1, 0x11, rnd.randrange(32)]))
    poke(sim, a + 0x13, rnd.choice([0, 1, 2, 3, 4, 8, 9, 10, rnd.randrange(16)]))
    poke(sim, a + 0x14, rnd.randrange(1, 5)); poke(sim, a + 0x15, rnd.randrange(0, 4))
    poke(sim, a + 0x16, 0)
    poke(sim, a + 0x17, rnd.choice([0, 0, rnd.randrange(128)])); poke(sim, a + 0x18, rnd.choice([0, 0, rnd.randrange(256)]))
    poke(sim, a + 9, [rnd.randrange(256), rnd.randrange(3)])

def azar_monstruos(sim, rnd):
    for k in range(25):
        a = 0x6C0C + 10 * k
        if not h(sim, a): continue
        if rnd.random() < 0.85: poke(sim, a + 6, h(sim, a + 6) | 1)
        if rnd.random() < 0.5: poke(sim, a + 9, rnd.randrange(20))
        if rnd.random() < 0.5: poke(sim, a + 8, rnd.randrange(12))
        if rnd.random() < 0.5: poke(sim, a + 7, rnd.randrange(256))

def azar_plano(sim, rnd):
    for c in range(494):
        v = h(sim, 0x6200 + c)
        if rnd.random() < 0.8: poke(sim, 0x6200 + c, v | 0x40)
        else: poke(sim, 0x6200 + c, v & 0xBF)

def poner_cerca(sim, rnd, pers, nm):
    """pone al heroe cerca del monstruo nm, al lado, en la misma sala, a pocas casillas o donde sea"""
    a = 0x6C0C + 10 * (nm - 1)
    mc, mf = h(sim, a + 1), h(sim, a + 2)
    k = rnd.random()
    cand = []
    if k < 0.45: rad = 1
    elif k < 0.8: rad = 6
    else: rad = 30
    for f in range(19):
        for c in range(26):
            if h(sim, 0x6200 + f * 26 + c) & 0x3F and max(abs(c - mc), abs(f - mf)) <= rad and (c, f) != (mc, mf): cand.append((c, f))
    if not cand: cand = [(c, f) for f in range(19) for c in range(26) if h(sim, 0x6200 + f * 26 + c) & 0x3F]
    c, f = rnd.choice(cand)
    poke(sim, HEROES[pers] + 1, [c, f])

def crear(rnd, m, tipo, rep):
    sim = nuevo()
    cargar_mision(sim, m, 15, [(0, 0, 0, 0, 0)] * 4)
    sim.registers[SP] = (sim.registers[SP] + 2) & 0xFFFF
    poke(sim, 0x6FD6, 1)
    celdas = [(c, f) for f in range(19) for c in range(26) if h(sim, 0x6200 + f * 26 + c) & 0x3F]
    for i in range(4): poke(sim, HEROES[i] + 1, list(rnd.choice(celdas)))
    pers = rnd.randrange(4)
    for i in range(4): azar_heroe(sim, rnd, i)
    azar_monstruos(sim, rnd)
    azar_plano(sim, rnd)
    poke(sim, 0xA98B, rnd.choice([0, 1, 2, 3, 0, 1]))                  # estado del turno
    poke(sim, 0xA98C, rnd.randrange(0, 13))
    poke(sim, 0xA9A4, rnd.choice([0, 1, 2, 3]))
    poke(sim, 0xA998, pers)
    poke(sim, 0xA9D3, [rnd.randrange(0, 20), rnd.randrange(0, 14)])
    poke(sim, 0xA9B6, 0); poke(sim, 0xA9B7, 0)
    preparar(sim, pers)
    return sim, pers

def volcar_iv(f, sim): volcar(sim, f, 'I')

def main():
    rnd = random.Random(4242)
    out = open(os.path.join(ROOT, 'build/oraculo_combate.txt'), 'w')
    n = 0
    for m in range(14):
        for rep in range(28):
            tipo = ['ATAQUE', 'ATAQUE', 'ATAQUE', 'DEFENSA', 'DEFENSA', 'HECHIZO', 'HECHIZO', 'HECHIZO', 'PANEL', 'PANEL', 'PANEL', 'PANEL', 'PANEL', 'PANEL'][rep % 14]
            sim, pers = crear(rnd, m, tipo, rep)
            vivos = [k + 1 for k in range(25) if h(sim, 0x6C0C + 10 * k)]
            if not vivos: continue
            dados = [rnd.randrange(6) for _ in range(20)]
            if tipo == 'PANEL': dados = [rnd.randrange(6) | (0x100 if rnd.random() < 0.2 else 0) for _ in range(20)]
            if tipo == 'ATAQUE': dados = [rnd.choice([0, 0, 3, 5, 1, 4, 2]) for _ in range(20)]
            nm = rnd.choice(vivos)
            par = ''
            if tipo == 'ATAQUE':
                if rnd.random() < 0.5: poke(sim, 0x6C0C + 10 * (nm - 1) + 9, rnd.choice([8, 10, 14, 15, 16, 17, 18, 19]))      # tipos con recompensa
                poner_cerca(sim, rnd, pers, nm)
                poke(sim, 0xA9C5, [ptr_mons(nm) & 0xFF, ptr_mons(nm) >> 8])
                par = str(nm)
            elif tipo == 'DEFENSA':
                hd = rnd.randrange(5) if rnd.random() < 0.1 else rnd.randrange(4)
                a = HEROES[hd]
                poke(sim, 0xA9AD, hd); poke(sim, 0xA9C7, [a & 0xFF, a >> 8])
                poke(sim, 0xA9C5, [ptr_mons(nm) & 0xFF, ptr_mons(nm) >> 8])
                par = '%d %d' % (hd, nm)
            elif tipo == 'HECHIZO':
                hz = rnd.randrange(1, 13)
                a = HEROES[pers]
                bl = h(sim, 0x7B70 + hz - 1)
                opciones = []
                if bl & 1: opciones.append('H')
                if bl & 2: opciones.append('M')
                if bl & 4: opciones.append('C')
                tg = rnd.choice(opciones)
                masc = (1 << (hz - 1)) if hz < 7 else (0x100 << (hz - 7))
                poke(sim, a + 0x0B, h(sim, a + 0x0B) | masc & 255); poke(sim, a + 0x0C, h(sim, a + 0x0C) | masc >> 8)
                poke(sim, a + 0x16, hz)
                poke(sim, 0xA9BA, bl); poke(sim, 0xA99F, 1)
                poke(sim, 0xA9A6, 0xFF); poke(sim, 0xA99D, 0); poke(sim, 0xA99E, 0)
                cell = None
                if tg == 'H':
                    hd = rnd.randrange(4); poke(sim, 0xA9A6, hd)
                    cell = h(sim, HEROES[hd] + 2) * 26 + h(sim, HEROES[hd] + 1)
                elif tg == 'M':
                    poke(sim, 0xA99D, nm); poke(sim, 0xA9C5, [ptr_mons(nm) & 0xFF, ptr_mons(nm) >> 8])
                    cell = h(sim, ptr_mons(nm) + 2) * 26 + h(sim, ptr_mons(nm) + 1)
                else:
                    cell = rnd.choice([f * 26 + c for f in range(19) for c in range(26) if h(sim, 0x6200 + f * 26 + c) & 0x3F])
                    poke(sim, 0xA99E, 1)
                poke(sim, 0xA9C1, [(0x6600 + cell) & 0xFF, (0x6600 + cell) >> 8])
                if rnd.random() < 0.7:                                # el que lanza, en la misma sala que el blanco (si no, hace falta linea de vista)
                    sala = h(sim, 0x6200 + cell) & 0x3F
                    mismas = [(c, f) for f in range(19) for c in range(26) if h(sim, 0x6200 + f * 26 + c) & 0x3F == sala]
                    if mismas: poke(sim, HEROES[pers] + 1, list(rnd.choice(mismas)))
                par = '%s %d %d %d' % (tg, hz, cell, hd if tg == 'H' else nm)
            else:
                z = rnd.choice([0, 0, 0, 1, 2, 2, 3, 4, 5, 6, 7, 7, 9])
                ze = 0x7FAE + 6 * z
                fila = rnd.randrange(0, 8)
                a = HEROES[pers]
                if z == 0 and rnd.random() < 0.6:                    # agua santa, con muertos vivientes en la ventana de la vista
                    ef = h(sim, a + 0x10) | 0x10
                    poke(sim, a + 0x10, ef); fila = bin(ef & 0x0F).count('1')
                    k0 = nm
                    mc, mf = h(sim, ptr_mons(k0) + 1), h(sim, ptr_mons(k0) + 2)
                    poke(sim, 0xA9D3, [max(0, min(20, mc - rnd.randrange(0, 5))), max(0, min(14, mf - rnd.randrange(0, 4)))])
                    for q in vivos:
                        if rnd.random() < 0.6: poke(sim, ptr_mons(q) + 8, rnd.choice([4, 5, 6, 1]))
                    poke(sim, 0x6200 + 0, h(sim, 0x6200))
                if z == 7:
                    poke(sim, a + 0x0F, h(sim, a + 0x0F) | 0x80)
                    c, f = h(sim, a + 1), h(sim, a + 2)
                    for dx, dy in ((0, 0), (0, -1), (1, 0), (0, 1), (-1, 0)):
                        if rnd.random() < 0.5 and 0 <= c + dx < 26 and 0 <= f + dy < 19 and h(sim, 0x6200 + (f + dy) * 26 + c + dx) & 0x3F:
                            poke(sim, 0x6600 + (f + dy) * 26 + c + dx, rnd.choice([0x31, 0x33, 0x36]))
                par = '%d %d' % (z, fila)
            out.write('CASO %d %d %s %s | %s\n' % (m, pers, tipo, par, ' '.join(map(str, dados))))
            volcar_iv(out, sim)
            cor = Corredor(sim, dados)
            if tipo == 'ATAQUE':
                preparar(sim, pers)
                cor.correr(0x9B65)
                preparar(sim, pers)
                cor.correr(0x7462)
                poke(sim, 0xA999, sim.registers[A_])
                if sim.registers[A_] == 0:
                    preparar(sim, pers); cor.correr(0x7559)
                    cor.correr(0x751D)
                    preparar(sim, pers); cor.correr(0x99A2)
            elif tipo == 'DEFENSA':
                preparar(sim, pers); cor.correr(0x9921)
            elif tipo == 'HECHIZO':
                preparar(sim, pers)
                # 0x78F6: lo que sigue a elegir el objetivo (el bucle de eleccion no se ejecuta)
                cor.correr(0x78F6)
            else:
                preparar(sim, pers)
                r = sim.registers
                ze = 0x7FAE + 6 * z
                r[8] = ze >> 8; r[9] = ze & 255
                zf = h(sim, ze + 2)
                r[D_] = zf + fila; r[E_] = h(sim, ze) + 1
                cor.correr(w16(sim, ze + 4))
            if os.environ.get('HQ_DEBUG') == str(n + 1):
                print('caso', n + 1, tipo, par, 'pers', pers, 'caster', peek(sim, HEROES[pers] + 1, 2), 'heroes', [peek(sim, a + 1, 2) for a in HEROES], 'A999', h(sim, 0xA999), 'A9A3', h(sim, 0xA9A3), 'origen', peek(sim, 0xA9D3, 2), 'eventos', cor.eventos, 'sala caster', h(sim, 0x6200 + h(sim, HEROES[pers] + 2) * 26 + h(sim, HEROES[pers] + 1)), 'A9C1', hex(w16(sim, 0xA9C1)), 'A9BB', h(sim, 0xA9BB))
            volcar(sim, out, 'P')
            out.write('E %s\n' % ' ; '.join(' '.join(e) for e in cor.eventos))
            n += 1
    out.close()
    print('oraculo del combate escrito:', n, 'casos')

if __name__ == '__main__':
    main()
