#!/usr/bin/env python3
"""Empaqueta los 9 efectos de sonido del original para src/music.c (mus_efecto).
Los efectos del Spectrum son siempre iguales (no dependen del estado del juego: suenan sobre la melodia 0, que esta en silencio) y la unica salida del
reproductor original son las escrituras al AY de cada fotograma, que ya estan volcadas en asset/musica/psg/efecto_N.psg (tools/hq_musica_psg.py ejecuta el
reproductor original en el simulador). Aqui esos volcados se convierten (periodos de tono y de ruido de 1,7734 MHz a los 750 kHz de la QSound) y se guardan
como diferencias fotograma a fotograma.
Formato de data/efectos_bin (big endian):
    +0   'HQFX', +4 word numero de efectos (9), +6 word 0
    +8   9 palabras largas: desplazamiento de cada efecto
    efecto: word fotogramas; por cada fotograma: byte n de registros escritos y n pares (registro, valor). El primer fotograma lleva los 14 registros.
Con --zx no se convierten los periodos (para verificar contra los .psg). Uso: hq_efectos_empaquetar.py [--zx] [-o salida]"""
import sys, os, struct
R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
zx = '--zx' in sys.argv

def leer_psg(ruta):
    d = open(ruta, 'rb').read()
    assert d[:4] == b'PSG\x1a'
    i = 16; regs = [0] * 16; frames = []
    while i < len(d):
        b = d[i]; i += 1
        if b == 0xFF: frames.append(tuple(regs[:14]))
        elif b == 0xFE: n = d[i] * 4; i += 1; frames += [tuple(regs[:14])] * n
        elif b == 0xFD: break
        else: regs[b & 15] = d[i]; i += 1
    return frames

def conv(p):
    return 0 if p == 0 else max(1, int(p * 750000 / 1773400 + 0.5))

def convertir(f):
    r = list(f)
    if not zx:
        for c in range(3):
            p = conv(r[2 * c] | (r[2 * c + 1] & 15) << 8)
            r[2 * c] = p & 255; r[2 * c + 1] = (r[2 * c + 1] & 0xF0) | (p >> 8)
        r[6] = conv(r[6] & 31) | (r[6] & 0xE0)
    return r

def efecto(n):
    fr = [convertir(f) for f in leer_psg('%s/asset/musica/psg/efecto_%d.psg' % (R, n))]
    out = struct.pack('>H', len(fr)); prev = None
    for f in fr:
        pares = [(k, f[k]) for k in range(14) if prev is None or f[k] != prev[k]]
        out += bytes([len(pares)]) + b''.join(bytes(p) for p in pares)
        prev = f
    return out

datos = [efecto(n) for n in range(9)]
off = 8 + 4 * 9
cab = b'HQFX' + struct.pack('>HH', 9, 0)
tab = b''
for d in datos:
    tab += struct.pack('>L', off); off += len(d)
dest = sys.argv[sys.argv.index('-o') + 1] if '-o' in sys.argv else R + '/data/efectos_bin'
os.makedirs(os.path.dirname(dest), exist_ok=True)
open(dest, 'wb').write(cab + tab + b''.join(datos))
print('%s: %d bytes, %d fotogramas%s' % (dest, off, sum(struct.unpack('>H', d[:2])[0] for d in datos), ' (periodos del Spectrum)' if zx else ''))
