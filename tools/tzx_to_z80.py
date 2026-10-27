#!/usr/bin/env python3
"""Extrae los bloques de un .tzx de Hero Quest (ZX 128K), reconstruye los 8 bancos
de RAM y escribe un snapshot .z80 v3 (128K) para SkoolKit, mas los binarios sueltos.
Uso: tzx_to_z80.py ref/juego.tzx directorio_salida"""
import sys, os, struct

def blocks(d):
    i = 10
    out = []
    while i < len(d):
        t = d[i]; i += 1
        if t == 0x10:
            l = struct.unpack('<H', d[i+2:i+4])[0]
            out.append(d[i+4:i+4+l]); i += 4 + l
        else:
            raise SystemExit('bloque TZX no soportado %02x' % t)
    return out

def lz80(data):
    return data  # sin compresion (flag 0x4000 en longitud = sin comprimir)

def main(tzx, outdir):
    os.makedirs(outdir, exist_ok=True)
    bl = blocks(open(tzx, 'rb').read())
    for n, b in enumerate(bl):
        open(os.path.join(outdir, 'blk%02d.bin' % n), 'wb').write(b[1:-1])
    scr, game = bl[3][1:-1], bl[5][1:-1]
    ram = [bytearray(16384) for _ in range(8)]
    # memoria 48K fija: 0x4000 = banco 5, 0x8000 = banco 2, 0xC000 = banco 0 (al arrancar)
    mem = bytearray(65536)
    mem[0x4000:0x4000+len(scr)] = scr
    mem[0x6400:0x6400+len(game)] = game
    ram[5][:] = mem[0x4000:0x8000]; ram[2][:] = mem[0x8000:0xC000]; ram[0][:] = mem[0xC000:]
    # bancos paginados por el cargador propio en 0x6580
    for blk, bank, off in ((6, 1, 0x1000), (7, 3, 0), (8, 4, 0), (9, 6, 0), (10, 7, 0)):
        d = bl[blk][1:-1]
        ram[bank][off:off+len(d)] = d
    for i in range(8):
        open(os.path.join(outdir, 'bank%d.bin' % i), 'wb').write(ram[i])
    open(os.path.join(outdir, 'game_ram48.bin'), 'wb').write(mem[0x4000:])
    # cabecera .z80 v3
    h = bytearray(30)
    h[6:8] = struct.pack('<H', 0)          # PC=0 -> v2/v3
    h[8:10] = struct.pack('<H', 0x6D7D)    # SP
    h[10] = 0x3F; h[12] = 0x0 << 1 | 0     # I, flags1 (borde 0)
    h[27] = 1; h[29] = 1 << 1               # IFF1, IM1
    ext = bytearray(56)
    ext[0:2] = struct.pack('<H', 54)
    ext[2:4] = struct.pack("<H", 0x6580)    # PC
    ext[4] = 4                               # hardware: 128K
    ext[5] = 0x10                            # ultimo OUT a 0x7FFD
    body = bytearray()
    for i in range(8):
        body += struct.pack('<HB', 0xFFFF if False else 16384, i + 3)  # sin comprimir: len 0xFFFF
    body = bytearray()
    for i in range(8):
        body += struct.pack('<HB', 0xFFFF, i + 3) + bytes(ram[i])
    h[6:8] = b'\0\0'
    open(os.path.join(outdir, 'hq128.z80'), 'wb').write(bytes(h) + bytes(ext) + bytes(body))

main(*sys.argv[1:3])
