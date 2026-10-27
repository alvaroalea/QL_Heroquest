/* Prueba de los efectos de sonido (src/music.c): reproduce un efecto con los periodos del Spectrum (efectos_zx_bin) y escribe, por cada fotograma, el estado de
   los 14 registros del AY (hexadecimal), que tests/mus_verificar.py compara con asset/musica/psg/efecto_N.psg del original.
   Uso: fx_test efectos_zx_bin efecto fotogramas */
#include <stdio.h>
#include <stdlib.h>
#include "music.h"
#include "qsound.h"

static int regs[16];
int plat_present(void) { return 1; }
int qs_detect(void) { return 1; }
int qs_present(void) { return 1; }
void qs_silence(void) {}
void qs_write(int reg, int val) { regs[reg & 15] = val; }

int main(int argc, char **argv)
{
    FILE *f = fopen(argv[1], "rb");
    static u8 buf[4096];
    int n = atoi(argv[3]), i, r;
    if (!f || fread(buf, 1, sizeof(buf), f) < 100) return 1;
    fclose(f);
    mus_fx_init(buf);
    mus_efecto(atoi(argv[2]));
    for (i = 0; i < n; i++) {
        mus_fx_fotograma();
        for (r = 0; r < 14; r++) printf("%02X", regs[r] & 255);
        printf("\n");
    }
    return 0;
}
