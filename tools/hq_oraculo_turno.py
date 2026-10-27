#!/usr/bin/env python3
"""Oraculo del turno: ejecuta en el simulador Z80 las rutinas ORIGINALES del movimiento de los heroes y vuelca el estado fotograma a fotograma en
build/oraculo_turno.txt, que compara tests/turno_test.c con src/turno.c (y src/vista.c para la animacion).
Cada caso: una mision cargada como en tools/hq_oraculo_partida.py, los heroes en casillas al azar (a menudo junto a puertas, objetos o escaleras), la vista
construida (0x9BE3, 0x8712, 0x8791) y uno de estos guiones:
  PASO  d : uno de los cuatro manejadores de paso (0x82CB, 0x8357, 0x83E3, 0x8478)
  CLIC x y: el boton pulsado con el cursor en (x, y): 0x7201 (zonas de la pantalla y camino hacia el cursor)
  y despues 40 fotogramas del bucle del turno (0x6FDA MOVER_AUTOMATICO, 0x85F2, 0xA4FD, 0xA794 con la animacion; si hace falta repintar, 0x9BE3 + 0x8712 + 0x8791).
De cada paso del guion se vuelcan: el heroe, su registro de animacion, las variables del turno y un resumen de las capas.
Uso: tmp/venv/bin/python -I tools/hq_oraculo_turno.py   (desde la raiz del proyecto)"""
import sys, os, random
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import hq_oraculo_partida as op
from skoolkit.simutils import PC, SP
nuevo, peek, poke, hx, cargar_mision, HEROES = (getattr(op, k) for k in ('nuevo', 'peek', 'poke', 'hx', 'cargar_mision', 'HEROES'))

PARAR_ACCIONES = {0x804E: 'OJO', 0x7710: 'MAPA', 0x7877: 'BOLSA', 0x7319: 'ATACAR', 0x727E: 'LLAVE'}
ACC_NUM = {'OJO': 4, 'MAPA': 6, 'BOLSA': 7, 'ATACAR': 8, 'LLAVE': 9}
GUIONES = [0xC1BE, 0xC1DD, 0xC1FC, 0xC21B]

def correr(sim, pc, paradas=(), limite=3000000):
    """Llama a la rutina y devuelve la direccion de parada (de 'paradas') o None si vuelve."""
    regs = sim.registers; mem = sim.memory; ops = sim.opcodes
    ret = 0x0001
    regs[SP] = (regs[SP] - 2) & 0xFFFF
    mem[regs[SP]] = ret & 0xFF; mem[(regs[SP] + 1) & 0xFFFF] = ret >> 8
    regs[PC] = pc
    n = 0
    while True:
        p = regs[PC]
        if p == ret: return None
        if p in paradas and n > 0:
            regs[SP] = (regs[SP] + 2) & 0xFFFF           # la direccion de retorno que se puso
            return p
        ops[mem[p]]()
        n += 1
        if n > limite: raise RuntimeError('la rutina %04X no termina' % pc)

def h(sim, a, n=1): return peek(sim, a, n)[0] if n == 1 else peek(sim, a, n)
def w16(sim, a): return h(sim, a) | h(sim, a + 1) << 8

def resumen(sim, pers, esc_estado):
    """una linea con todo lo que se compara"""
    hero = HEROES[pers]
    r = peek(sim, hero, 26)
    anim = 0x6A8E + 18 * (13 if pers == 4 else pers)
    ra = peek(sim, anim, 18)
    ptr = ra[8] | ra[9] << 8
    off = ptr - GUIONES[0] if ra[0] == 2 else 0
    guion = 0; paso_off = 0
    if ra[0] == 2:
        guion = max(i for i in range(4) if GUIONES[i] <= ptr); paso_off = ptr - GUIONES[guion]
    cuerpo = lambda a, n: sum((i + 1) * b for i, b in enumerate(peek(sim, a, n))) & 0xFFFFFFFF
    v = lambda a: h(sim, a)
    vars_ = [v(0xA98C), v(0xA98B), v(0xA9B9), (w16(sim, 0xA9BD) if v(0xA9B9) else 0), w16(sim, 0xA9BF), v(0xA9B3), v(0xA987), v(0xA988), v(0xA98A), v(0xA9B7), v(0xA9BB), v(0xA9B1), v(0xA98E)]
    return ' '.join(str(x) for x in (
        r[1], r[2], r[3], r[4] | r[5] << 8, r[8], r[0], r[0x0F], r[0x18], r[0x16], r[0x17],
        ra[0], ra[1], ra[2], ra[3], guion, paso_off,
        *vars_,
        cuerpo(0x6200, 512), cuerpo(0x6400, 512), cuerpo(0x6600, 512), cuerpo(0x6C0C, 250),
        *peek(sim, 0x8FE4, 4)))

def preparar(sim, pers):
    poke(sim, 0xA998, pers); poke(sim, 0xA9DD, [HEROES[pers] & 0xFF, HEROES[pers] >> 8])
    a = 0x6A8E + 18 * (13 if pers == 4 else pers)
    poke(sim, 0xA9DF, [a & 0xFF, a >> 8])

def construir(sim, pers):
    """REDIBUJAR_TURNO sin el panel"""
    op.ejecutar(sim, 0x9BE3); poke(sim, 0xA989, 0); poke(sim, 0xA987, 0)
    op.ejecutar(sim, 0x8712); op.ejecutar(sim, 0x8791)

def fotograma(sim, pers, out):
    # 0x6FDA con IY e IX; si el paso llega a una escalera de salida se para en 0x8EA1 (despues viene lo de cada mision, que no es de este paso)
    r = sim.registers
    iy = HEROES[pers]; ix = 0x6A8E + 18 * (13 if pers == 4 else pers)
    r[8] = ix >> 8; r[9] = ix & 0xFF; r[10] = iy >> 8; r[11] = iy & 0xFF
    if correr(sim, 0x6FDA, (0x8EA1,)) == 0x8EA1: return True
    op.ejecutar(sim, 0x85F2)
    op.ejecutar(sim, 0xA4FD)
    op.ejecutar(sim, 0xA794)
    if h(sim, 0xA987):
        construir(sim, pers)
    return False

def ventana_tiles():
    """numero de casilla de la ventana de 6 x 5 para (dx, dy): se numeran por diagonales (tabla 0xB6E7)"""
    g = {}; n = 0
    for s_ in range(10):
        for dy in range(min(s_, 4), max(0, s_ - 5) - 1, -1):
            g[(s_ - dy, dy)] = n; n += 1
    return g

def objetivos(A, B, C):
    """pasos (col, fila, d) de interes: cruzan una puerta, pisan un objeto o una escalera, chocan con una pared o con un monstruo"""
    delta = ((-1, 0), (1, 0), (0, -1), (0, 1))
    r = {'puerta': [], 'objeto': [], 'escalera': [], 'pared': [], 'bloqueo': []}
    for f in range(19):
        for c in range(26):
            if not A[f * 26 + c] & 0x3F: continue
            for d, (dx, dy) in enumerate(delta):
                nc, nf = c + dx, f + dy
                if not (0 <= nc < 26 and 0 <= nf < 19): continue
                dest = nf * 26 + nc; w = f * 26 + c if d in (0, 2) else dest
                b = B[w]; v = C[dest]
                if b & (0x10 if d < 2 else 0x20): r['puerta'].append((c, f, d))
                if 0x32 <= v < 0x38: r['objeto'].append((c, f, d))
                if 0x38 <= v < 0x3C: r['escalera'].append((c, f, d))
                if b & (0x40 if d < 2 else 0x80): r['pared'].append((c, f, d))
                if 0 < v < 0x32 or 0x40 <= v: r['bloqueo'].append((c, f, d))
    return r

def main():
    rnd = random.Random(2024)
    out = open(os.path.join(ROOT, 'build/oraculo_turno.txt'), 'w')
    n = 0
    grid = ventana_tiles()
    from hq_vista_empaquetar import secciones
    tiles = secciones()[0][1]                                              # (y, x) de las casillas de la vista
    for m in range(14):
        for rep in range(30):
            sim = nuevo()
            cargar_mision(sim, m, 15, [(0, 0, 0, 0, 0)] * 4)
            sim.registers[SP] = (sim.registers[SP] + 2) & 0xFFFF
            poke(sim, 0x6FD6, 1)                                          # con musica activada los efectos de sonido no hacen nada
            A = peek(sim, 0x6200, 512); B = peek(sim, 0x6400, 512); C = peek(sim, 0x6600, 512)
            celdas = [(c, f) for f in range(19) for c in range(26) if A[f * 26 + c] & 0x3F]
            obj = objetivos(A, B, C)
            pers = rnd.randrange(4)
            pos = [rnd.choice(celdas) for _ in range(4)]
            d = rnd.randrange(4)
            tipo = rnd.choice(['puerta', 'objeto', 'escalera', 'pared', 'bloqueo', None, None])
            if tipo and obj[tipo]:
                c, f, d = rnd.choice(obj[tipo]); pos[pers] = (c, f)
            guion = rnd.choice(['PASO', 'PASO', 'CLIC', 'CLIC', 'CLIC'])
            if m == 9: guion = 'PASO'                                   # el teletransporte de la mision 9 usa el dado
            for i, (c, f) in enumerate(pos): poke(sim, HEROES[i] + 1, [c, f])
            preparar(sim, pers)
            pm = rnd.choice([0, 1, 2, 3, 5, 8, 12])
            poke(sim, 0xA98C, pm)
            poke(sim, 0xA98B, rnd.choice([0, 0, 1, 4]))
            poke(sim, HEROES[pers] + 0x18, rnd.choice([0, 0, 0x20, 0x80, 0xA0]))
            poke(sim, 0xA9BF, [0, 0])
            construir(sim, pers)
            if guion == 'PASO':
                r = peek(sim, HEROES[pers], 26)
                dx, dy = ((-1, 0), (1, 0), (0, -1), (0, 1))[d]
                if m == 9 and not (0 <= r[1] + dx < 26 and 0 <= r[2] + dy < 19 and A[(r[2] + dy) * 26 + r[1] + dx] & 0x3F): continue
                if rnd.random() < 0.3:                                      # la misma puerta ya cruzada en este turno
                    cell = r[2] * 26 + r[1]; w = cell if d in (0, 2) else cell + (1 if d == 1 else 26)
                    poke(sim, 0xA9BF, [(0x6400 + w) & 0xFF, (0x6400 + w) >> 8])
                head = 'CASO %d %d %s | %d %d %d %s' % (m, pers, ' '.join('%d %d' % p for p in pos), pm, rep, d, 'PASO')
                out.write(head + '\n')
                out.write('I %s\n' % resumen(sim, pers, 0))
                regs = (0x82CB, 0x8357, 0x83E3, 0x8478)
                poke(sim, 0xA997, 0)
                parada = correr_con_iy(sim, regs[d], pers, paradas=(0x8EA1,))
                out.write('A %d\n' % (2 if parada == 0x8EA1 else 0))
                out.write('P %s\n' % resumen(sim, pers, 0))
                if parada is None: marcar_frames(sim, pers, out)
            else:
                cx = rnd.randrange(0x10, 0xFD, 4); cy = rnd.randrange(0x0A, 0xAF, 2)
                r = peek(sim, HEROES[pers], 26); ox, oy = h(sim, 0xA9D3), h(sim, 0xA9D4)
                if rnd.random() < 0.55:                                     # el cursor en una casilla en linea con el heroe
                    dx, dy = ((-1, 0), (1, 0), (0, -1), (0, 1))[d]
                    k = rnd.randrange(1, 6)
                    tc, tf = r[1] + dx * k, r[2] + dy * k
                    if 0 <= tc - ox < 6 and 0 <= tf - oy < 5:
                        t = grid[(tc - ox, tf - oy)]
                        cx = min(0xFC, max(0x10, ((tiles[t][1] + 0x24) & 0xFF) // 4 * 4)); cy = min(0xAE, max(0x0A, ((tiles[t][0] + 0x0C) & 0xFF) // 2 * 2))
                elif rnd.random() < 0.3:                                    # sobre una de las zonas de los iconos
                    cx, cy = rnd.choice([(0xC8, 0xA0), (0xD8, 0xA4), (0xDC, 0x98), (0xB8, 0x9C), (0x70, 0xA6), (0xE8, 0x90), (0xF4, 0x88), (0xA0, 0x9F), (0xAC, 0x9F)])
                head = 'CASO %d %d %s | %d %d %d %d CLIC' % (m, pers, ' '.join('%d %d' % p for p in pos), pm, rep, cx, cy)
                out.write(head + '\n')
                out.write('I %s\n' % resumen(sim, pers, 0))
                poke(sim, 0xAA69, [cx, cy]); poke(sim, 0xA997, 0x10)
                a = correr_con_iy(sim, 0x7201, pers, paradas=tuple(PARAR_ACCIONES) + (0x8EA1,))
                acc = -1 if a is None else (ACC_NUM[PARAR_ACCIONES[a]] if a in PARAR_ACCIONES else 2)
                out.write('K %d\n' % acc)
                out.write('P %s\n' % resumen(sim, pers, 0))
                if a is None or a == 0x8EA1:
                    marcar_frames(sim, pers, out)
            n += 1
    out.close()
    print('oraculo del turno escrito:', n, 'casos')

def correr_con_iy(sim, pc, pers, paradas):
    """llama a un manejador con IY = heroe e IX = registro de animacion, como lo hacen 0x7201 y 0x6FDA"""
    iy = HEROES[pers]; ix = 0x6A8E + 18 * (13 if pers == 4 else pers)
    r = sim.registers
    r[8] = ix >> 8; r[9] = ix & 0xFF; r[10] = iy >> 8; r[11] = iy & 0xFF       # IXh, IXl, IYh, IYl
    return correr(sim, pc, paradas)

def marcar_frames(sim, pers, out):
    for k in range(40):
        salio = fotograma(sim, pers, out)
        out.write('F %s\n' % resumen(sim, pers, 0))
        if salio: break

if __name__ == '__main__':
    main()
