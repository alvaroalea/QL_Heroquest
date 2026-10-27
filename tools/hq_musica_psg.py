#!/usr/bin/env python3
"""Ejecuta el reproductor original (banco 4, 0xD388) en el simulador de SkoolKit y registra las escrituras al AY
(puertos 0xFFFD / 0xBFFD) fotograma a fotograma. Escribe ficheros .psg (formato estandar de volcado del AY)
y un resumen en texto.
Como en el juego, en cada interrupcion (50 Hz) se llama a 0xD388 con A=0 y luego A=1.
Uso: hq_musica_psg.py salida_dir [segundos]"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from skoolkit.snapshot import Snapshot
from skoolkit.simutils import from_snapshot, PC
from skoolkit.simulator import Simulator
from skoolkit.pagingtracer import PagingTracer

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STUB = 0x6000

class T(PagingTracer):
    def __init__(self, sim):
        self.simulator = sim; self.out7ffd = 0x14; self.outfffd = 0; self.ay = [0] * 16
        self.outfe = 0; self.border = 0; self.frames = []; self.cur = []; self.sel = 0
    def write_port(self, registers, port, value, offset):
        if port & 0xC002 == 0xC000: self.sel = value                 # 0xFFFD: seleccion de registro
        elif port & 0xC002 == 0x8000:                                 # 0xBFFD: dato
            if self.sel < 16: self.cur.append((self.sel, value))
        elif port & 0xFF == 0x0D:                                     # marca de fotograma (puerto ficticio)
            self.frames.append(self.cur); self.cur = []
    def read_port(self, registers, port): return 0xFF

def ejecutar(tipo, num, fotogramas):
    snap = Snapshot.get(os.path.join(ROOT, 'asset/extraido/hq128.z80'))
    sim = from_snapshot(Simulator, snap, {}, {}, {'fast_djnz': False, 'fast_ldir': False}, None)
    mem = sim.memory; mem.out7ffd(0x14)
    tr = T(sim); sim.set_tracer(tr)
    def poke(a, bs):
        for i, b in enumerate(bs): mem[a + i] = b
    # rutina de prueba: inicializa y repite 'tick' por fotograma
    # LD A,2 ; LD D,n ; CALL D388 ; (efecto: LD A,4 ; LD E,n ; CALL D388) ; bucle: XOR A ; CALL D388 ; LD A,1 ; CALL D388 ; LD A,0 ; OUT (0x0D),A ; JR bucle
    code = []
    if tipo == 'melodia': code += [0x3E, 2, 0x16, num, 0xCD, 0x88, 0xD3]
    else: code += [0x3E, 2, 0x16, 0, 0xCD, 0x88, 0xD3, 0x3E, 4, 0x1E, num, 0xCD, 0x88, 0xD3]
    loop = STUB + len(code)
    code += [0xAF, 0xCD, 0x88, 0xD3, 0x3E, 1, 0xCD, 0x88, 0xD3, 0x3E, 0, 0xD3, 0x0D, 0x18, 0]
    code[-1] = (loop - (STUB + len(code))) & 0xFF
    poke(STUB, code)
    regs = sim.registers; regs[PC] = STUB
    from skoolkit.simutils import SP
    regs[SP] = 0x6D00
    opcodes = sim.opcodes; n = 0; lim = fotogramas
    estados = []; vistos = {}; info = {'bucle': None, 'fin': None}
    last = 0
    while len(tr.frames) < lim:
        opcodes[mem[regs[PC]]](); n += 1
        if len(tr.frames) != last:
            last = len(tr.frames)
            est = bytes(mem[a] for a in range(0xDD88, 0xDEA0)) + bytes(mem[a] for a in range(0xE26A, 0xE2D4))
            if mem[0xDD89] & 0x80 and tipo == 'melodia' and info['fin'] is None:
                info['fin'] = last; break
            if est in vistos and info['bucle'] is None:
                info['bucle'] = (vistos[est], last); break
            vistos[est] = last
        if n > 400000 * lim: break
    return tr.frames, info

def psg(frames, path):
    """Escribe un .psg estandar: cabecera 'PSG' 0x1A, version 0, 50 Hz; por fotograma pares (registro, valor) y 0xFF;
    0xFE n = n*4 fotogramas vacios. Solo se escriben los registros que cambian."""
    est = {}; red = []
    for f in frames:
        nf = []
        for r, v in f:
            if est.get(r) != v: nf.append((r, v)); est[r] = v
        red.append(nf)
    out = bytearray(b'PSG\x1a' + bytes(12))
    i = 0
    while i < len(red):
        if not red[i]:
            j = i
            while j < len(red) and not red[j]: j += 1
            n = j - i
            while n >= 4:
                t = min(n // 4, 255); out += bytes([0xFE, t]); n -= 4 * t
            out += b'\xff' * n
            i = j; continue
        for r, v in red[i]: out += bytes([r, v])
        out += b'\xff'; i += 1
    open(path, 'wb').write(out)

def resumen(frames):
    regs = {}
    for f in frames:
        for r, v in f: regs[r] = v
    # busca el punto de bucle: primer fotograma cuyo contenido de escrituras se repite y el estado coincide
    return len([f for f in frames if f])

if __name__ == '__main__':
    out = sys.argv[1]; seg = float(sys.argv[2]) if len(sys.argv) > 2 else 60
    solo_efectos = len(sys.argv) > 3 and sys.argv[3] == 'efectos'
    os.makedirs(out, exist_ok=True)
    res = []
    for tipo, rango, nom in (('melodia', range(0, 4), 'melodia'), ('efecto', range(0, 9), 'efecto')):
        if solo_efectos and tipo == 'melodia': continue
        for n in rango:
            dur = int(seg * 50) if tipo == 'melodia' else 200
            fr, info = ejecutar(tipo, n, dur)
            if info['fin'] is not None or info['bucle'] is not None: fr = fr[:(info['fin'] or info['bucle'][1])]
            if tipo == 'efecto':   # se recorta al ultimo fotograma audible (volumen != 0)
                est = [0] * 16; ult = 0
                for k, f in enumerate(fr):
                    for r_, v_ in f: est[r_] = v_
                    if any(est[8 + c] & 0x1F for c in range(3)): ult = k + 1
                fr = fr[:ult + 1]
            p = os.path.join(out, '%s_%d.psg' % (nom, n))
            psg(fr, p)
            res.append('%s %d: %d fotogramas (%.2f s)%s' % (nom, n, len(fr), len(fr) / 50.0, '; termina al fotograma %d' % info['fin'] if info['fin'] else '; bucle: el estado del reproductor del fotograma %d se repite en el %d (vuelve a empezar tras %d fotogramas)' % (info['bucle'][1], info['bucle'][0], info['bucle'][1] - info['bucle'][0]) if info['bucle'] else '; efecto sin bucle (recortado al ultimo fotograma audible)' if tipo == 'efecto' else '; sin fin ni bucle en %d fotogramas' % dur))
            print(res[-1])
    open(os.path.join(out, 'resumen_psg.txt'), 'w').write('\n'.join(res) + '\n')
