"""Lectura y escritura de PNG sin dependencias (solo zlib).
 escribir_rgba(ruta, ancho, alto, filas)  filas = lista de listas de (r, g, b, a)
 leer(ruta) -> (ancho, alto, filas RGBA)   admite profundidad 1/2/4/8, tipos 0, 2, 3 (con tRNS), 4 y 6; sin entrelazado"""
import zlib, struct
def _chunk(t, d):
    c = struct.pack('>I', len(d)) + t + d
    return c + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)
def escribir_rgba(ruta, w, h, filas):
    raw = b''.join(b'\x00' + bytes(c for p in fila for c in p) for fila in filas)
    open(ruta, 'wb').write(b'\x89PNG\r\n\x1a\n' + _chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) +
                           _chunk(b'IDAT', zlib.compress(raw, 9)) + _chunk(b'IEND', b''))
def leer(ruta):
    d = open(ruta, 'rb').read()
    assert d[:8] == b'\x89PNG\r\n\x1a\n', 'no es un PNG: ' + ruta
    i = 8; idat = b''; pal = None; trns = None
    while i < len(d):
        n, t = struct.unpack('>I4s', d[i:i + 8]); c = d[i + 8:i + 8 + n]; i += 12 + n
        if t == b'IHDR': w, h, bd, ct, _, _, il = struct.unpack('>IIBBBBB', c)
        elif t == b'PLTE': pal = [tuple(c[k:k + 3]) for k in range(0, len(c), 3)]
        elif t == b'tRNS': trns = c
        elif t == b'IDAT': idat += c
    assert il == 0, 'PNG entrelazado no soportado: ' + ruta
    nc = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ct]
    bpp = max(1, nc * bd // 8); stride = (w * nc * bd + 7) // 8
    raw = zlib.decompress(idat); out = []; prev = bytearray(stride); p = 0
    for y in range(h):
        f = raw[p]; line = bytearray(raw[p + 1:p + 1 + stride]); p += 1 + stride
        for x in range(stride):
            a = line[x - bpp] if x >= bpp else 0; b = prev[x]; c = prev[x - bpp] if x >= bpp else 0
            if f == 1: line[x] = (line[x] + a) & 255
            elif f == 2: line[x] = (line[x] + b) & 255
            elif f == 3: line[x] = (line[x] + ((a + b) >> 1)) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if pa <= pb and pa <= pc else b if pb <= pc else c
                line[x] = (line[x] + pr) & 255
        prev = line
        vals = []
        if bd == 8: vals = list(line)
        elif bd == 16: vals = [line[k] for k in range(0, stride, 2)]
        else:
            per = 8 // bd
            for byte in line:
                for k in range(per): vals.append((byte >> (8 - bd * (k + 1))) & ((1 << bd) - 1))
        row = []
        for x in range(w):
            if ct == 6: q = vals[4 * x:4 * x + 4]; row.append(tuple(q))
            elif ct == 2: q = vals[3 * x:3 * x + 3]; row.append((q[0], q[1], q[2], 255))
            elif ct == 0:
                v = vals[x] * 255 // ((1 << bd) - 1) if bd < 8 else vals[x]; row.append((v, v, v, 255))
            elif ct == 4: row.append((vals[2 * x],) * 3 + (vals[2 * x + 1],))
            else:
                k = vals[x]; r, g, bl = pal[k]; a = trns[k] if trns and k < len(trns) else 255
                row.append((r, g, bl, a))
        out.append(row)
    return w, h, out
