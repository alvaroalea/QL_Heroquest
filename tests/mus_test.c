/* Prueba del motor de musica en el PC: reproduce una melodia con los periodos del Spectrum y escribe, por cada fotograma, el estado
   de los 14 registros del AY (hexadecimal). tests/mus_verificar.py lo compara con los volcados .psg del original.
   Uso: mus_test musica_zx_bin melodia fotogramas */
#include <stdio.h>
#include <stdlib.h>
#include "music.h"
#include "qsound.h"

static int regs[16];
int plat_present(void) { return 1; }
int qs_detect(void) { return 1; }
int qs_present(void) { return 1; }
void qs_silence(void) {}
static int modo_w;
void qs_write(int reg, int val) { regs[reg & 15] = val; if (modo_w) printf("QS %d %d\n", reg, val); }

int main(int argc, char **argv)

{
    FILE *f;
    f = fopen(argv[1], "rb");
    static u8 buf[8192];
    int song = atoi(argv[2]), n = atoi(argv[3]), i, r;
    if (!f || fread(buf, 1, sizeof(buf), f) < 5642) return 1;
    fclose(f);
    modo_w = argc > 4;                          /* con un cuarto argumento: lista de escrituras en vez de estados */
    mus_init(buf);
    mus_start(song);
    for (i = 0; i < n; i++) {
        mus_frame();
        if (!modo_w) for (r = 0; r < 14; r++) printf("%02X", regs[r] & 255);
        if (!modo_w) printf("\n");
    }
    return 0;
}
