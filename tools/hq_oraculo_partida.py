#!/usr/bin/env python3
"""Oraculo del estado de la partida: ejecuta en el simulador Z80 (SkoolKit) las rutinas ORIGINALES del Spectrum y vuelca
el resultado en tests/oraculo_partida.txt, que compara tests/partida_test.c con la version en C (src/partida.c).

  INICIAR_MISION (0x5B3F..0x5C39 y 0x5C56..fin): capas A/B/C, indicadores, monstruos, heroes (posicion, ausentes y muertos) y
      el cambio de equipo de la mision 5. Se prueba cada mision con distintas combinaciones de heroes presentes (8FE4-8FE7) y de equipo.
  CALCULAR_DADOS (0x6ECD): dados de movimiento segun los bits de estado del heroe.
  DADO (0x9BD3): resultado para cada valor del registro R (y con acarreo 0 y 1).
  MISION_CONTINUA (0x6EB5).
Uso: tmp/venv/bin/python -I tools/hq_oraculo_partida.py   (desde la raiz del proyecto)
"""
import sys, os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
from skoolkit.snapshot import Snapshot
from skoolkit.simutils import from_snapshot, PC, SP, A, F, B, C, D, E, H, L, R, T
from skoolkit.simulator import Simulator

# indices de registros del simulador de SkoolKit 10.1
sys.path.insert(0, ROOT)

def nuevo():
    snap = Snapshot.get(os.path.join(ROOT, 'asset/extraido/hq128.z80'))
    sim = from_snapshot(Simulator, snap, {}, {}, {'fast_djnz': False, 'fast_ldir': False}, None)
    mem = sim.memory
    mem.out7ffd(0x10)
    class Tr:
        def __init__(s): s.sim = sim
        def write_port(s, registers, port, value, offset):
            if port & 0x8002 == 0: s.sim.memory.out7ffd(value)
        def read_port(s, registers, port): return 0xFF
    sim.set_tracer(Tr())
    ov = open(os.path.join(ROOT, 'asset/extraido/overlay_menu_5B00.bin'), 'rb').read()
    for i, v in enumerate(ov): mem[0x5B00 + i] = v
    return sim

def ejecutar(sim, pc, ret=0x0001, parar=None, limite=20000000):
    """Llama a la rutina en pc; para al volver a 'ret' (o al llegar a la direccion 'parar')."""
    regs = sim.registers; mem = sim.memory; ops = sim.opcodes
    regs[SP] = (regs[SP] - 2) & 0xFFFF
    mem[regs[SP]] = ret & 0xFF; mem[(regs[SP] + 1) & 0xFFFF] = ret >> 8
    regs[PC] = pc
    n = 0
    while True:
        p = regs[PC]
        if p == ret or (parar is not None and p == parar): return
        ops[mem[p]]()
        n += 1
        if n > limite: raise RuntimeError('rutina %04X no termina' % pc)

def peek(sim, a, n=1): return [sim.memory[a + i] for i in range(n)]
def poke(sim, a, vals):
    if isinstance(vals, int): vals = [vals]
    for i, v in enumerate(vals): sim.memory[a + i] = v & 0xFF

HEROES = [0xA9E5, 0xA9FF, 0xAA19, 0xAA33, 0xAA4D]

def hx(l): return ' '.join('%02X' % v for v in l)

def cargar_mision(sim, m, presentes, equipo):
    """equipo = lista de 4 tuplas (grupoB, grupoC, objetos, armas, armadura) para los registros de los heroes"""
    poke(sim, 0xA981, m)
    poke(sim, 0x8FE4, [0xFF if (presentes >> i) & 1 else 0 for i in range(4)])
    for i, e in enumerate(equipo):
        a = HEROES[i]
        poke(sim, a + 0x0B, e[0]); poke(sim, a + 0x0C, e[1]); poke(sim, a + 0x0D, e[2]); poke(sim, a + 0x0E, e[3])
        poke(sim, a + 0x0F, e[4])
    # basura previa en las variables que borra la rutina y en las capas
    poke(sim, 0xA983, [0x55] * 0x54)
    poke(sim, 0x6C0C, [0x77] * 0xFA)
    ejecutar(sim, 0x5B3F, parar=0x5C39)       # hasta justo antes de dibujar el texto del objetivo (todo el estado ya esta hecho)
    # se salta el dibujo y la espera del boton y se sigue en 0x5C56
    sim.registers[SP] = (sim.registers[SP]) & 0xFFFF
    regs = sim.registers
    regs[PC] = 0x5C56
    # la pila quedo con la direccion de retorno colocada por ejecutar(): se vuelve a llamar con 'ret' en la cima
    ejecutar2(sim, 0x9542)              # hasta BARAJAR_MAZO (al que salta el final de la rutina); el mazo se comprueba aparte

def ejecutar2(sim, parar, limite=20000000):
    """Ejecuta hasta 'parar'. INICIAR_MISION en la mision 5 no vuelve de 0x5CE7 (cae en 0x5CEF y termina con JP 0x9542): cada vez que se
    llega a 0x9542 con una direccion de retorno distinta de la del llamante (0x0001) se vuelve a ella sin barajar, para que sigan los
    intercambios de equipo de los otros heroes."""
    regs = sim.registers; mem = sim.memory; ops = sim.opcodes
    n = 0
    while True:
        if regs[PC] == parar:
            sp = regs[SP]
            ret = mem[sp] | (mem[(sp + 1) & 0xFFFF] << 8)
            if ret == 0x0001: break
            regs[SP] = (sp + 2) & 0xFFFF
            regs[PC] = ret
        ops[mem[regs[PC]]]()
        n += 1
        if n > limite: raise RuntimeError('no termina')

def volcar(sim, m, presentes, f):
    f.write('MISION %d PRESENTES %d\n' % (m, presentes))
    f.write('A ' + hx(peek(sim, 0x6200, 512)) + '\n')
    f.write('B ' + hx(peek(sim, 0x6400, 512)) + '\n')
    f.write('C ' + hx(peek(sim, 0x6600, 512)) + '\n')
    f.write('I ' + hx(peek(sim, 0x6BB6, 0x56)) + '\n')
    f.write('M ' + hx(peek(sim, 0x6C0C, 250)) + '\n')
    for i, a in enumerate(HEROES):
        f.write('H%d ' % i + hx(peek(sim, a, 26)) + '\n')
    f.write('V ' + hx(peek(sim, 0xA983, 0x54)) + '\n')
    f.write('S ' + hx(peek(sim, 0xA2B2, 15)) + '\n')

def main():
    sim0 = nuevo()
    estado0 = None
    out = open(os.path.join(ROOT, 'tests/oraculo_partida.txt'), 'w')
    import random
    rnd = random.Random(1234)
    # --- cargar misiones
    casos = 0
    for m in range(14):
        for presentes in (15, 7, 10, 1, 0):
            sim = nuevo()
            equipo = [tuple(rnd.randrange(256) for _ in range(5)) for _ in range(4)]
            # semilla de estado de entrada en el fichero para que el C parta de lo mismo
            out.write('ENTRADA %s\n' % ' '.join(hx(e).replace(' ', '') for e in equipo))
            cargar_mision(sim, m, presentes, equipo)
            volcar(sim, m, presentes, out)
            casos += 1
    # --- restaurar equipo de la mision 5 (DESPACHAR_HEROE, 0xA271)
    for h in range(4):
        sim = nuevo()
        equipo = [tuple(rnd.randrange(256) for _ in range(5)) for _ in range(4)]
        cargar_mision(sim, 5, 15, equipo)
        poke(sim, 0xA998, h)
        ejecutar(sim, 0xA271)
        out.write('RESTAURA %d %s\n' % (h, ' '.join(hx(e).replace(' ', '') for e in equipo)))
        for i, a in enumerate(HEROES[:4]):
            out.write('RH%d ' % i + hx(peek(sim, a, 26)) + '\n')
    # --- dados de movimiento
    sim = nuevo()
    for pers in range(5):
        for b13 in (0, 8):
            for b0f in (0, 0x40):
                for b17 in (0, 0x20):
                    for b18 in (0, 8):
                        poke(sim, 0xA998, pers)
                        a = HEROES[min(pers, 4)]
                        poke(sim, 0xA9DD, [a & 0xFF, a >> 8])
                        poke(sim, a + 0x13, b13); poke(sim, a + 0x0F, b0f); poke(sim, a + 0x17, b17); poke(sim, a + 0x18, b18)
                        poke(sim, a + 0x10, 0x20)
                        poke(sim, 0xA98A, 1); poke(sim, 0xA98B, 1)
                        ejecutar(sim, 0x6ECD)
                        out.write('DM %d %d %d %d %d -> %d %d %d %d %d\n' % (
                            pers, b13, b0f, b17, b18, peek(sim, 0xA9B5)[0], peek(sim, a + 0x17)[0], peek(sim, a + 0x10)[0],
                            peek(sim, a + 0x18)[0], peek(sim, 0xA98A)[0] * 16 + peek(sim, 0xA98B)[0]))
    # --- dado: para cada R y acarreo
    sim = nuevo()
    regs = sim.registers
    for r in range(256):
        sim.registers[R] = r
        ejecutar_dado(sim, r, out)
    # --- mazo de tesoros: BARAJAR_MAZO para cada valor posible de R>>3 (32), ROBAR_CARTA y DEVOLVER_CARTA
    for c in range(32):
        sim = nuevo()
        ejecutar(sim, 0x9542, parar=0x954F)
        sim.registers[A] = c * 8
        sim.registers[PC] = 0x9551
        ejecutar_hasta_ret(sim)
        out.write('MZ %d %s | %02X%02X\n' % (c, hx(peek(sim, 0x9593, 26)), peek(sim, 0xA9E2)[0], peek(sim, 0xA9E1)[0]))
        if c % 9 == 0:
            for k in range(3):
                ejecutar(sim, 0x9523)
                out.write('RC %d %02X %s | %02X%02X\n' % (c, sim.registers[A], hx(peek(sim, 0x9593, 26)), peek(sim, 0xA9E2)[0], peek(sim, 0xA9E1)[0]))
            sim.registers[A] = 0x15
            ejecutar(sim, 0x9588)
            out.write('DC %d %s | %02X%02X\n' % (c, hx(peek(sim, 0x9593, 26)), peek(sim, 0xA9E2)[0], peek(sim, 0xA9E1)[0]))
    # bits 3 de la ROM que lee el barajado (0x0EF8 a 0x2200)
    rom = sim.memory
    out.write('ROMBITS ' + ''.join('1' if rom[a] & 8 else '0' for a in range(0x0EF8, 0x2200)) + '\n')
    # --- heroes iniciales (memoria tal como se carga) y tabla de cuerpo maximo; INICIALIZA HEROE NUEVO (0x60E6)
    sim = nuevo()
    out.write('INIT ' + ' | '.join(hx(peek(sim, a, 26)) for a in HEROES) + '\n')
    out.write('TCUERPO ' + hx(peek(sim, 0x7E4E, 5)) + '\n')
    for i in range(5):
        sim = nuevo()
        poke(sim, HEROES[i], [rnd.randrange(256) for _ in range(26)])
        poke(sim, 0x65F0, i)
        antes = peek(sim, HEROES[i], 26)
        ejecutar(sim, 0x60E6)
        out.write('NH %d %s | %s\n' % (i, hx(antes), hx(peek(sim, HEROES[i], 26))))
    # --- mision continua
    for muertos in (0, 7, 15):
        for m in (0, 1, 2):
            for st in (0, 0xFF):
                sim = nuevo()
                poke(sim, 0xA9B7, muertos); poke(sim, 0xA981, m); poke(sim, 0xAA4D, st)
                ejecutar(sim, 0x6EB5)
                out.write('MC %d %d %d -> %d\n' % (muertos, m, st, sim.registers[A]))
    out.close()
    print('oraculo escrito:', casos, 'cargas de mision')

def ejecutar_hasta_ret(sim):
    # BARAJAR_MAZO termina con RET a la direccion colocada por la llamada anterior (0x0001)
    regs = sim.registers; mem = sim.memory; ops = sim.opcodes
    while regs[PC] != 0x0001: ops[mem[regs[PC]]]()

def ejecutar_dado(sim, r, out):
    """9BD3 lee R con LD A,R: se sustituye por el valor deseado ejecutando el cuerpo desde 0x9BD7 con A=r (el bit 7 de R no cambia)"""
    regs = sim.registers
    for carry in (0, 1):
        regs[A] = r
        regs[F] = carry
        regs[C] = 3
        ejecutar(sim, 0x9BD7)
        out.write('D %d %d -> %d\n' % (r, carry, regs[C]))

if __name__ == '__main__':
    main()
