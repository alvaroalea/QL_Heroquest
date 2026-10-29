#define _POSIX_C_SOURCE 200112L
/* Backend de pruebas en PC: el buffer tiene el mismo formato que la pantalla del QL. Las teclas vienen de un guion
   (variable de entorno HQ_KEYS o fichero keys.txt): fichas separadas por espacios, una por pulsacion (se mantiene 2 cuadros
   y se suelta 2; con el sufijo xN se mantiene N cuadros: Rx20): U D L R arriba, abajo, izquierda, derecha; F disparo; E escape; 0 1 2 3 4; '.' = esperar; Sxx = guardar la
   pantalla en screen_xx.bin (sin cabecera: tools/ql2png.py <bin> <png>). Al acabarse el guion el programa termina. */
#include "platform.h"
#include "music.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>

static u8 buf[SCR_BYTES];
u8 *screen = buf;
u8 plat_raton = RAT_CURSORES;
u8 plat_tecla4 = 0;
static char script[4096];
static int spos = 0, hold = 0, rest = 0;
static u16 cur = 0;
static unsigned long frames = 0;

void plat_init(void)
{
    const char *e = getenv("HQ_KEYS");
    FILE *f;
    script[0] = 0;
    if (e) { strncpy(script, e, sizeof(script) - 1); script[sizeof(script) - 1] = 0; }
    else if ((f = fopen("keys.txt", "r")) != NULL) { size_t n = fread(script, 1, sizeof(script) - 1, f); script[n] = 0; fclose(f); }
}
int plat_raton_auto(void) { return RAT_CURSORES; }
int plat_raton_ok(int modo) { (void)modo; return 0; }
int plat_raton_estado(void) { return 0; }
void plat_raton_delta(int *dx, int *dy) { *dx = *dy = 0; }
void plat_shutdown(void) {}
void plat_flush_input(void) {}
void plat_exit(void) { exit(0); }
void plat_wait_key(void) {}
int plat_default_dir(int prog, char *b, int max) { (void)prog; (void)max; b[0] = 0; return 0; }
const char *plat_data_dir(void) { const char *e = getenv("HQ_DATA"); return e ? e : "data/"; }
unsigned long plat_ticks(void) { return frames; }

int plat_listar(const char *suf, char nombres[][16], int max)
{
    DIR *d = opendir(plat_data_dir());
    struct dirent *e;
    int n = 0, ls = (int)strlen(suf), l;
    if (!d) return 0;
    while (n < max && (e = readdir(d)) != NULL) {
        l = (int)strlen(e->d_name);
        if (l <= ls || l > 15 + ls || strcmp(e->d_name + l - ls, suf) != 0) continue;
        memcpy(nombres[n], e->d_name, (size_t)(l - ls)); nombres[n][l - ls] = 0;
        n++;
    }
    closedir(d);
    return n;
}

void plat_snapshot(const char *name)
{
    char path[64];
    FILE *f;
    sprintf(path, "screen_%s.bin", name);
    f = fopen(path, "wb");
    if (f) { fwrite(buf, 1, SCR_BYTES, f); fclose(f); }
}

int plat_present(void) { frames++; mus_mute(0); mus_tick(); return 1; }

u16 plat_keys(void)
{
    if (hold > 0) { hold--; if (hold == 0) rest = 2; return cur; }
    cur = 0;
    if (rest > 0) { rest--; return 0; }
    while (script[spos] == ' ' || script[spos] == '\n' || script[spos] == '\r') spos++;
    if (!script[spos]) { exit(0); }
    switch (script[spos]) {
    case 'U': cur = K_UP; break;     case 'D': cur = K_DOWN; break;
    case 'L': cur = K_LEFT; break;   case 'R': cur = K_RIGHT; break;
    case 'F': cur = K_FIRE; break;   case 'E': cur = K_ESC; break;
    case '0': cur = K_0; break;      case '1': cur = K_1; break;
    case '2': cur = K_2; break;      case '3': cur = K_3; break;
    case '4': cur = K_4; break;
    case 'Y': cur = K_YES; break;
    case 'M': cur = K_M; break;
    case 'S': {
        char nm[16]; int n = 0;
        spos++;
        while (script[spos] && script[spos] != ' ' && n < 15) nm[n++] = script[spos++];
        nm[n] = 0; plat_snapshot(nm);
        return 0;
    }
    default: break;
    }
    spos++;
    hold = 2;
    if (script[spos] == 'x') { spos++; hold = atoi(script + spos); while (script[spos] >= '0' && script[spos] <= '9') spos++; if (hold < 1) hold = 1; }
    return cur;
}
