"""Empaqueta los datos de la musica del original (banco 4) para el motor de src/music.c.
Salida (data/musica_bin; los punteros de los datos son direcciones 0xC000+):
    +0      5000 bytes  datos originales 0xC000-0xD387 (melodias, efectos, instrumentos, patrones; palabras little endian como en el Z80)
    +5000   1 byte      escala del vibrato (x/64: 64 = igual que el original)
    +5001   1 byte      reservado
    +5002   256 bytes   tabla de periodos (128 palabras little endian, la del original 0xDC08 convertida)
    +5258   256 bytes   tabla de periodos de ruido (0xDCC8; pasado el indice 96 siguen la de vibrato y las variables del reproductor 0xDD88)
    +5514   128 bytes   tabla de amplitudes de vibrato (0xDD28)
Con --zx (para verificar contra los volcados .psg) los periodos no se convierten.
Conversion: reloj del AY del Spectrum 128K 1.7734 MHz, de la QSound 750 kHz: periodo_QL = periodo_ZX * 750000 / 1773400."""
import sys, os
R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
O = R + '/asset/musica/original/'
zx = '--zx' in sys.argv
dat = open(O + 'banco4_C000_datos_musicales.bin', 'rb').read()
per = open(O + 'banco4_DC08_tabla_periodos.bin', 'rb').read()
rui = open(O + 'banco4_DCC8_tabla_volumen.bin', 'rb').read()
vib = open(O + 'banco4_DD28_tabla_vibrato.bin', 'rb').read()
var = open(O + 'banco4_DD88_variables.bin', 'rb').read()
assert len(dat) == 5000 and len(per) >= 192 and len(rui) >= 96 and len(vib) >= 96
out = bytearray(dat)
out.append(64 if zx else 27); out.append(0)
for i in range(128):
    # pasado el indice 95 el original sigue leyendo palabras de la tabla de ruido que hay a continuacion (0xDCC8): se copian igual
    b = per if i < 96 else rui
    k = 2 * i if i < 96 else 2 * (i - 96)
    p = b[k] | (b[k + 1] << 8)
    if not zx and p: p = max(1, int(p * 750000 / 1773400 + 0.5))
    out += bytes([p & 255, p >> 8])
out += rui[:96] + vib[:96] + var[:64]          # el original lee 0xDCC8 + indice (hasta 255): siguen la tabla de vibrato y las variables
out += vib[:96] + bytes(32)
dest = sys.argv[sys.argv.index('-o') + 1] if '-o' in sys.argv else R + '/data/musica_bin'
os.makedirs(os.path.dirname(dest), exist_ok=True)
open(dest, 'wb').write(out)
print('%s: %d bytes%s' % (dest, len(out), ' (periodos del Spectrum)' if zx else ' (periodos convertidos a 750 kHz)'))
