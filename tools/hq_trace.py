#!/usr/bin/env python3
"""Conductor de simulacion para Hero Quest 128K (SkoolKit).
Ejecuta el snapshot desde 0x6400, inyecta teclas segun un guion y registra:
  - cobertura de instrucciones por (banco, pc)  -> <prefijo>_cov.txt (se parte de asset/trace/cov.txt; fusionar con hq_cov_merge.py)
  - capturas PNG de la pantalla en los frames indicados
Uso: hq_trace.py guion.txt [salida_prefijo]
Guion: lineas 'frame accion' ; acciones: key <teclas> <frames> | shot <nombre> | end
  teclas: letras/numeros/ENTER/SPACE/CAPS/SYM separados por '+'.
"""
import sys, os
from skoolkit.snapshot import Snapshot
from skoolkit.simutils import from_snapshot, PC, T
from skoolkit.simulator import Simulator
from skoolkit.pagingtracer import Memory, PagingTracer
from skoolkit.graphics import Frame, scr_udgs
from skoolkit.components import get_image_writer

ROWS = ['CAPS Z X C V', 'A S D F G', 'Q W E R T', '1 2 3 4 5',
        '0 9 8 7 6', 'P O I U Y', 'ENTER L K J H', 'SPACE SYM M N B']
KEYMAP = {}
for r, row in enumerate(ROWS):
    for b, k in enumerate(row.split()):
        KEYMAP[k] = (r, b)

class T_(PagingTracer):
    def __init__(self, sim, out7ffd):
        self.simulator = sim; self.out7ffd = out7ffd; self.outfffd = 0
        self.ay = [0]*16; self.outfe = 0; self.border = 0
        self.keyboard = [0]*8
        self.ports = []
    def write_port(self, registers, port, value, offset):
        if port % 2 == 0:
            self.border = value & 7; self.outfe = value
        if port & 0x8002 == 0 and self.out7ffd & 32 == 0:
            self.simulator.memory.out7ffd(value); self.out7ffd = value
        if port & 0xC002 == 0xC000: self.outfffd = value
        elif port & 0xC002 == 0x8000 and self.outfffd < 16:
            self.ay[self.outfffd] = value
    def read_port(self, registers, port):
        if port & 0xC002 == 0xC000 and self.outfffd < 16: return self.ay[self.outfffd]
        if port % 2 == 0:
            h = (port >> 8) ^ 0xFF; v = 0x40; i = 0
            while h:
                if h & 1: v |= self.keyboard[i]
                h >>= 1; i += 1
            return v ^ 0xFF
        return 0xFF

def main(script, prefix='asset/trace/run'):
    snap = Snapshot.get('asset/extraido/hq128.z80')
    sim = from_snapshot(Simulator, snap, {}, {}, {'fast_djnz': False, 'fast_ldir': False}, None)
    mem = sim.memory; mem.out7ffd(0x10)
    tr = T_(sim, 0x10); sim.set_tracer(tr)
    events = []
    for ln in open(script):
        ln = ln.split('#')[0].split()
        if ln: events.append(ln)
    regs = sim.registers; opcodes = sim.opcodes
    fd = sim.frame_duration; int_active = sim.int_active
    pc = regs[PC] = 0x6400
    covfile = prefix + '_cov.txt'
    cov = set()
    if os.path.exists('asset/trace/cov.txt'):
        for l in open('asset/trace/cov.txt'):
            b, a = l.split(); cov.add((int(b), int(a, 16)))
    tstates = regs[25]
    next_int = ((tstates + fd - int_active)//fd)*fd
    frame = 0; prev = 0
    releases = []
    ev = 0
    writer = get_image_writer()
    held = {}
    while True:
        opcodes[mem[pc]]()
        cov.add((mem.o7ffd & 7 if pc >= 0xC000 else -1, pc))
        tstates = regs[25]
        if tstates >= next_int:
            if tstates < next_int + int_active:
                if regs[26]:
                    sim.accept_interrupt(regs, mem, pc); tstates = regs[25]
                frame = tstates // fd
                if frame > prev:
                    prev = frame
                    for k in [k for k, e in held.items() if e <= frame]:
                        del held[k]
                    tr.keyboard = [0]*8
                    for k in held:
                        r, b = KEYMAP[k]; tr.keyboard[r] |= 1 << b
                    while ev < len(events) and int(events[ev][0]) <= frame:
                        e = events[ev]; ev += 1
                        if e[1] == 'key':
                            for k in e[2].upper().split('+'):
                                held[k] = frame + int(e[3])
                        elif e[1] == 'shot':
                            scr = scr_udgs(mem.memory[1], 0, 0, 32, 24, 0, 6144)
                            with open('%s_%s.png' % (prefix, e[2]), 'wb') as f:
                                writer.write_image([Frame(scr, 2)], f)
                        elif e[1] == 'shadow':
                            scr = scr_udgs(mem.banks[7], 0, 0, 32, 24, 0, 6144)
                            with open('%s_%s.png' % (prefix, e[2]), 'wb') as f:
                                writer.write_image([Frame(scr, 2)], f)
                        elif e[1] == 'end':
                            pc = regs[PC]; ev = -1; break
                    if ev == -1: break
            else:
                next_int += fd
        pc = regs[PC]
    with open(covfile, 'w') as f:
        for b, a in sorted(cov): f.write('%d %04X\n' % (b, a))
    print('frames', frame, 'instrucciones distintas', len(cov), 'banco paginado', mem.o7ffd)

main(*sys.argv[1:])
