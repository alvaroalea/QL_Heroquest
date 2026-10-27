#!/usr/bin/env python3
"""Decodifica los datos musicales del reproductor de Hero Quest (banco 4, org 0xC000) y los vuelca a texto.
Formato (deducido de 0xD3AA, 0xD7C5-0xDB10, 0xD689 y 0xDE9E):
  0xC000: palabra = tabla de melodias (4 palabras); 0xC002 = tabla de efectos; 0xC004 = tabla de instrumentos (10 bytes cada uno)
  melodia: 3 palabras = lista de patrones de cada canal (A, B, C). Lista de patrones: palabras (direccion de patron);
           0x0000 = volver al principio de la lista; 0xFFFF = fin de la melodia
  patron (bytecode): 0x00-0x5F nota (indice en la tabla de periodos 0xDC08, mas la transposicion); duran (duracion actual) tics
           0x60 fin de bucle; 0x61 n inicio de bucle (n veces); 0x62 dir llamada a subpatron; 0x63 dir salto; 0x64 n transposicion;
           0x65 marca; 0x80-0xBF duracion (valor-0x7F tics); 0xC0-0xDF n instrumento; 0xE0 a b c deslizamiento;
           0xE1-0xEF volumen maximo; 0xF0-0xFE tempo (fotogramas por tic = valor-0xEF); 0xFF fin de patron o retorno
  efecto: lo mismo que un patron pero un solo canal con su propia estructura (0xE2D0)
Uso: hq_musica.py salida_dir"""
import sys, os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
B = open(os.path.join(ROOT, 'asset/extraido/bank4.bin'), 'rb').read()
def b(a): return B[a - 0xC000]
def w(a): return b(a) | b(a + 1) << 8
NOM = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
def periodo(i): return w(0xDC08 + 2 * i)
def nota(i):
    p = periodo(i) if 0xDC08 + 2 * i < 0xDCC8 else 0
    if not p: return '?%d' % i
    f = 1773400 / (16.0 * p)
    import math
    m = round(69 + 12 * math.log2(f / 440.0))
    return '%s%d' % (NOM[m % 12], m // 12 - 1)
def patron(a, visto, ind=''):
    """decodifica un patron desde a hasta 0xFF; devuelve lineas"""
    out = []
    while True:
        if a in visto and ind == '': pass
        c = b(a); s = '%04X  ' % a
        if c < 0x60: out.append(s + 'nota %s (indice %d)' % (nota(c), c)); a += 1
        elif c == 0x60: out.append(s + 'FIN DE BUCLE'); a += 1
        elif c == 0x61: out.append(s + 'BUCLE x%d' % b(a + 1)); a += 2
        elif c == 0x62: out.append(s + 'LLAMA %04X' % w(a + 1)); a += 3
        elif c == 0x63: out.append(s + 'SALTA %04X' % w(a + 1)); a += 3; break
        elif c == 0x64: out.append(s + 'transposicion %+d' % (b(a + 1) if b(a + 1) < 128 else b(a + 1) - 256)); a += 2
        elif c == 0x65: out.append(s + 'marca (0xDD8D=FF)'); a += 1
        elif 0x66 <= c < 0x80: out.append(s + 'duracion?? %02X' % c); a += 1
        elif c < 0xC0: out.append(s + 'duracion %d' % (c - 0x7F)); a += 1
        elif c < 0xE0: out.append(s + 'instrumento %d' % (c - 0xC0)); a += 1
        elif c == 0xE0: out.append(s + 'deslizamiento %02X %02X %02X' % (b(a + 1), b(a + 2), b(a + 3))); a += 4
        elif c < 0xF0: out.append(s + 'volumen maximo %d' % (c - 0xE0)); a += 1
        elif c < 0xFF: out.append(s + 'tempo %d fotogramas/tic' % (c - 0xEF)); a += 1
        else: out.append(s + 'FIN'); a += 1; break
    return out, a
def instrumento(a):
    return ('instrumento en %04X: ruido=%02X,%02X  vibrato=%02X,%02X  secuencia de tono=%04X  secuencia de volumen=%04X  puertas=%04X' %
            (a, b(a), b(a + 1), b(a + 2), b(a + 3), w(a + 4), w(a + 6), w(a + 8)))
def lista_patrones(a):
    r = []
    while True:
        v = w(a); a += 2
        if v == 0 or v == 0xFFFF: r.append(v); break
        r.append(v)
        if len(r) > 400: break
    return r
def dump():
    L = []
    L.append('Datos musicales de Hero Quest (ZX Spectrum 128K, banco 4)\n')
    L.append('tabla de melodias %04X, de efectos %04X, de instrumentos %04X' % (w(0xC000), w(0xC002), w(0xC004)))
    L.append('\nTabla de periodos del AY (0xDC08), indice: periodo, nota (reloj 1,7734 MHz)')
    for i in range(96): L.append('  %2d: %4d  %s' % (i, periodo(i), nota(i)))
    L.append('\nInstrumentos (10 bytes cada uno)')
    for i in range(0x20):
        a = w(w(0xC004) + 2 * i)
        if not 0xC000 <= a < 0xD400: break
        L.append('  %2d  %s' % (i, instrumento(a)))
    L.append('\nMelodias')
    pats = set()
    for n in range(4):
        t = w(w(0xC000) + 2 * n)
        L.append('\nmelodia %d (descriptor en %04X): listas de patrones A=%04X B=%04X C=%04X' % (n, t, w(t), w(t + 2), w(t + 4)))
        for c in range(3):
            lp = w(t + 2 * c)
            lst = lista_patrones(lp)
            L.append('  canal %s: lista %04X: %s' % ('ABC'[c], lp, ' '.join('%04X' % v for v in lst)))
            for v in lst:
                if v not in (0, 0xFFFF): pats.add(v)
    L.append('\nEfectos')
    efs = []
    for n in range(0, 12):
        a = w(w(0xC002) + 2 * n)
        if not 0xC000 <= a < 0xD400: break
        efs.append((n, a))
        L.append('  efecto %d: %04X' % (n, a))
    L.append('\nPatrones y subpatrones (bytecode)')
    visto = set(); cola = sorted(pats) + [a for n, a in efs]
    while cola:
        a = cola.pop(0)
        if a in visto: continue
        visto.add(a)
        L.append('\npatron %04X' % a)
        ls, fin = patron(a, visto)
        L += ['   ' + x for x in ls]
        for x in ls:
            if ' LLAMA ' in x or ' SALTA ' in x:
                t = int(x.split()[-1], 16)
                if t not in visto: cola.append(t)
    return L
if __name__ == '__main__':
    out = sys.argv[1]; os.makedirs(out, exist_ok=True)
    open(os.path.join(out, 'partituras.txt'), 'w', encoding='utf-8').write('\n'.join(dump()) + '\n')
    raw = os.path.join(out, 'original'); os.makedirs(raw, exist_ok=True)
    def sav(nom, a, e): open(os.path.join(raw, nom), 'wb').write(B[a - 0xC000:e - 0xC000])
    sav('banco4_C000_datos_musicales.bin', 0xC000, 0xD388)       # tablas, instrumentos, efectos y patrones
    sav('banco4_D388_reproductor.bin', 0xD388, 0xDC08)           # codigo del reproductor (Z80)
    sav('banco4_DC08_tabla_periodos.bin', 0xDC08, 0xDCC8)        # 96 periodos del AY (palabras)
    sav('banco4_DCC8_tabla_volumen.bin', 0xDCC8, 0xDD28)         # 96 bytes: indice de nota/paso -> volumen (secuencias de volumen)
    sav('banco4_DD28_tabla_vibrato.bin', 0xDD28, 0xDD88)         # 96 bytes: indice de nota -> amplitud del vibrato
    sav('banco4_DD88_variables.bin', 0xDD88, 0xDEA0)             # estado inicial de las variables y de las 3 estructuras de canal
    sav('banco4_DE9E_efectos_codigo.bin', 0xDE9E, 0xE000)        # rutina de efectos y variables
    sav('banco4_completo.bin', 0xC000, 0x10000)                  # el banco 4 entero (incluye los retratos desde 0xE400)
    print(len(dump()), 'lineas')

def duracion_lista(lp, tempo=12):
    """Cuenta los tics de un canal desde el principio de su lista hasta 0x0000/0xFFFF (una pasada). Devuelve (tics, tempo)."""
    tics = 0; dur = 1; trans = 0
    pila = []; pats = lista_patrones(lp)
    for pa in pats:
        if pa in (0, 0xFFFF): break
        a = pa; pila = []
        guard = 0
        while guard < 100000:
            guard += 1
            c = b(a)
            if c < 0x60: tics += dur; a += 1
            elif c == 0x60:
                cnt, ret = pila[-1]; cnt -= 1
                if cnt <= 0: pila.pop(); a += 1
                else: pila[-1] = (cnt, ret); a = ret
            elif c == 0x61: pila.append((b(a + 1), a + 2)); a += 2
            elif c == 0x62: pila.append((-1, a + 3)); a = w(a + 1)
            elif c == 0x63: a = w(a + 1)
            elif c == 0x64: a += 2
            elif c == 0x65: a += 1
            elif 0x80 <= c < 0xC0: dur = c - 0x7F; a += 1
            elif c < 0xE0: a += 1 if c >= 0xC0 else 1
            elif c == 0xE0: a += 4
            elif c < 0xF0: a += 1
            elif c < 0xFF: tempo = c - 0xEF; a += 1
            else:
                if pila and pila[-1][0] == -1: a = pila.pop()[1]
                else: break
    return tics, tempo
