/* Prueba de src/monstruo.c contra el original: lee build/oraculo_monstruo.txt (lo genera tools/hq_oraculo_monstruo.py ejecutando las rutinas del Spectrum en un
   simulador Z80), pone la partida en el estado de «antes» que vuelca el oraculo, ejecuta el turno de un monstruo o de una ronda de monstruos y comprueba que
   el estado de «despues» (heroes, monstruos, capas, variables) y los guiones que habria mostrado son los mismos. Uso: build/host/monstruo_test [oraculo] [data/] (desde la raiz) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "monstruo.h"
#include "text.h"
#include "textos_ids.h"

static int fallos = 0, pruebas = 0, caso = 0;
#ifdef QLTEST
static const char *ddir = "flp1_";
#define FICHERO_ORACULO "flp1_oraculo_monstruo_txt"
u8 *screen;
const char *plat_data_dir(void) { return "flp1_"; }
#else
static const char *ddir = "data/";
#define FICHERO_ORACULO "build/oraculo_monstruo.txt"
#endif
static FILE *out;
static char cabecera[400];

static int hexv(char c) { return c >= '0' && c <= '9' ? c - '0' : (c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1); }
static const char *leer_hex(const char *s, u8 *b, int n)
{
    int i;
    for (i = 0; i < n; i++) { while (*s == ' ') s++; b[i] = (u8)(hexv(s[0]) * 16 + hexv(s[1])); s += 2; }
    return s;
}

static u8 *leer_fichero(const char *name, long *n)
{
    FILE *f = fopen(name, "rb");
    u8 *b;
    long l;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); l = ftell(f); fseek(f, 0, SEEK_SET);
    b = (u8 *)malloc((size_t)l + 1);
    if (fread(b, 1, (size_t)l, f) != (size_t)l) { fclose(f); free(b); return NULL; }
    b[l] = 0; fclose(f);
    if (n) *n = l;
    return b;
}

static void pers_de_bytes(Pers *r, const u8 *b)
{
    r->estado = b[0]; r->col = b[1]; r->fila = b[2]; r->hab = b[3]; r->casilla = (u16)(b[4] | (b[5] << 8));
    r->cuerpo = b[6]; r->mente = b[7]; r->fotograma = b[8]; r->oro = (u16)(b[9] | (b[10] << 8));
    r->hechizos[0] = b[11]; r->hechizos[1] = b[12]; r->objetos = b[13]; r->armas = b[14]; r->armadura = b[15]; r->efectos = b[16];
    r->r11 = b[17]; r->r12 = b[18]; r->r13 = b[19]; r->ataque = b[20]; r->defensa = b[21]; r->hechizo = b[22]; r->t17 = b[23];
    r->t18 = b[24]; r->indice = b[25];
}

static void mons_de_bytes(Mons *m, const u8 *b)
{
    m->estado = b[0]; m->col = b[1]; m->fila = b[2]; m->hab = b[3]; m->casilla = (u16)((b[4] | (b[5] << 8)) - 0x6200 > 0x7FFF ? 0 : (b[4] | (b[5] << 8)) - 0x6200);
    m->cuerpo = b[6]; m->b7 = b[7]; m->tipo = b[8]; m->estad = b[9];
}

static void error(const char *que, int i, long c, long o)
{
    fprintf(out, "FALLO %s (caso %d) posicion %d: C=%ld original=%ld\n    %s\n", que, caso, i, c, o, cabecera);
    fallos++;
}

static void igual(const char *que, const u8 *a, const u8 *b, int n)
{
    int i;
    pruebas++;
    for (i = 0; i < n; i++) if (a[i] != b[i]) { error(que, i, a[i], b[i]); return; }
}

static char evt[3000];

static void anade(const char *s) { if (evt[0]) strcat(evt, " ; "); strcat(evt, s); }

static void comparar_eventos(const char *linea)
{
    const char *q = linea;
    pruebas++;
    while (*q == ' ') q++;
    if (strcmp(q, evt)) { fprintf(out, "FALLO sucesos (caso %d):\n  C:        %s\n  original: %s\n    %s\n", caso, evt, q, cabecera); fallos++; }
}

static void ataque_de(Partida *p, int num)
{
    static const char *muerte[] = { "BB18", "BB30", "BB46", "BB5D", "BB77" };
    Salida o;
    char t[60];
    int r = monstruo_planear(p, num), k, n, h;
    if (r == MON_NADA) return;
    n = monstruo_pasos();
    for (k = 0; k < n; k++) monstruo_paso(p, num, k);
    h = p->v.atacante;
    sprintf(t, "A %d %d", h, num); anade(t);
    memset(&o, 0, sizeof(o));
    combate_defensa(p, h, num, &o);
    for (k = 0; k < o.n; k++) { sprintf(t, "S %s", muerte[o.m[k].a]); anade(t); }
}

#define NV 13
static void vars_a_partida(Partida *p, const int *v)
{
    Vars *x = &p->v;
    x->turno_acabado = (u8)v[0]; x->ausentes = (u8)v[1]; x->muertos = (u8)v[2]; x->pers_muerto = (u8)v[3]; x->golpes = (u8)v[4]; x->escudos = (u8)v[5];
    x->dados_defensa = (u8)v[6]; x->ataque_base = (u8)v[7]; x->defensa_base = (u8)v[8]; x->atacante = (u8)v[9]; x->resultado = (u8)v[10]; x->camino_n = (u8)v[11];
    x->pers_actual = (u8)v[12];
}

static void partida_a_vars(const Partida *p, int *v)
{
    const Vars *x = &p->v;
    v[0] = x->turno_acabado; v[1] = x->ausentes; v[2] = x->muertos; v[3] = x->pers_muerto; v[4] = x->golpes; v[5] = x->escudos; v[6] = x->dados_defensa;
    v[7] = x->ataque_base; v[8] = x->defensa_base; v[9] = x->atacante; v[10] = x->resultado; v[11] = x->camino_n; v[12] = x->pers_actual;
}

int main(int argc, char **argv)
{
    const char *fn = argc > 1 ? argv[1] : FICHERO_ORACULO;
    static Partida p;
    static u8 buf[1600], b2[1600];
    static int dados[256];
    u8 *tx, *spr, *esc, *vis, *bal, *mar, *dm, pr[4];
    char *cur, *ln, nm[200], tipo[10];
    long n;
    int i, k, m = 0, nmon = 0, en_caso = 0, iv[NV], pv[NV], nd;
    out = stdout;
#ifdef QLTEST
    out = fopen("flp1_mtest_out", "w");
    if (!out) return 3;
    screen = (u8 *)malloc((size_t)SCR_BYTES);
    if (!screen) { fprintf(out, "sin memoria para la pantalla\n"); fclose(out); return 3; }
    memset(screen, 0, (size_t)SCR_BYTES);
#endif
    if (argc > 2) ddir = argv[2];
    tx = leer_fichero(fn, &n);
    if (!tx) { fprintf(out, "no se puede leer %s (hay que ejecutar tools/hq_oraculo_monstruo.py)\n", fn); return 2; }
    {
        char path[200];
        sprintf(path, "%stextos_bin", ddir); txt = leer_fichero(path, 0);
        sprintf(path, "%ssprites_bin", ddir); spr = leer_fichero(path, 0);
        sprintf(path, "%sescenario_bin", ddir); esc = leer_fichero(path, 0);
        sprintf(path, "%svista_bin", ddir); vis = leer_fichero(path, 0);
        sprintf(path, "%sbaldosas_bin", ddir); bal = leer_fichero(path, 0);
        sprintf(path, "%smarca_activo_bin", ddir); mar = leer_fichero(path, 0);
    }
    if (!txt || !vista_datos(spr, esc, vis, bal, mar)) { fprintf(out, "faltan datos (error %d)\n", vista_error); return 2; }
    memset(pr, 1, 4);
    for (cur = (char *)tx; *cur;) {
        ln = cur;
        cur = strchr(cur, '\n');
        if (cur) *cur++ = 0; else cur = ln + strlen(ln);
        if (!strncmp(ln, "CASO ", 5)) {
            char *q = strchr(ln, '|') + 1;
            sscanf(ln, "CASO %d %9s %d", &m, tipo, &nmon);
            for (nd = 0; nd < 256 && *q; nd++) {
                while (*q == ' ') q++;
                if (!*q) break;
                dados[nd] = atoi(q);
                while (*q && *q != ' ') q++;
            }
            sprintf(nm, "%smision_%02d_es_bin", ddir, m);
            dm = leer_fichero(nm, &n);
            if (!dm) { fprintf(out, "no se puede leer %s\n", nm); return 2; }
            pers_boot(&p);
            if (!partida_cargar(&p, m, dm, n, pr)) { fprintf(out, "mision rechazada\n"); return 2; }
            free(dm);
            strncpy(cabecera, ln, 120); cabecera[120] = 0;
            caso++; en_caso = 1;
        } else if (!en_caso) continue;
        else if (ln[0] == 'I' && ln[1] == 'H' && ln[2] >= '0' && ln[2] <= '4') { leer_hex(ln + 4, buf, 26); pers_de_bytes(&p.pers[ln[2] - '0'], buf); }
        else if (!strncmp(ln, "IM ", 3)) { leer_hex(ln + 3, buf, 250); for (i = 0; i < N_MONS; i++) mons_de_bytes(&p.mons[i], buf + 10 * i); }
        else if (!strncmp(ln, "IL ", 3)) {
            static u8 l[3 * MAP_P];
            leer_hex(ln + 3, l, 3 * MAP_P);
            memcpy(p.capa_a, l, MAP_P); memcpy(p.capa_b, l + MAP_P, MAP_P); memcpy(p.capa_c, l + 2 * MAP_P, MAP_P);
        } else if (!strncmp(ln, "IV ", 3)) {
            const char *s = ln + 3;
            for (k = 0; k < NV; k++) { iv[k] = atoi(s); while (*s && *s != ' ') s++; while (*s == ' ') s++; }
            vars_a_partida(&p, iv);
            dado_guion(dados, nd);
            evt[0] = 0;
            if (!strcmp(tipo, "MONSTRUO")) ataque_de(&p, nmon);
            else for (i = 1; i <= 25; i++) ataque_de(&p, i);
        } else if (ln[0] == 'P' && ln[1] == 'H' && ln[2] >= '0' && ln[2] <= '4') {
            leer_hex(ln + 4, buf, 26); pers_bytes(&p.pers[ln[2] - '0'], b2);
            igual("heroe", b2, buf, 26);
        } else if (!strncmp(ln, "PM ", 3)) {
            leer_hex(ln + 3, buf, 250);
            for (i = 0; i < N_MONS; i++) mons_bytes(&p.mons[i], b2 + 10 * i);
            for (i = 0; i < N_MONS; i++) if (!b2[10 * i]) { memset(b2 + 10 * i + 4, 0, 2); memset(buf + 10 * i + 4, 0, 2); }
            igual("monstruos", b2, buf, 250);
        } else if (!strncmp(ln, "PL ", 3)) {
            static u8 l[3 * MAP_P], c[3 * MAP_P];
            leer_hex(ln + 3, l, 3 * MAP_P);
            memcpy(c, p.capa_a, MAP_P); memcpy(c + MAP_P, p.capa_b, MAP_P); memcpy(c + 2 * MAP_P, p.capa_c, MAP_P);
            igual("capas", c, l, 3 * MAP_P);
        } else if (!strncmp(ln, "PV ", 3)) {
            const char *s = ln + 3;
            for (k = 0; k < NV; k++) { pv[k] = atoi(s); while (*s && *s != ' ') s++; while (*s == ' ') s++; }
            partida_a_vars(&p, iv);
            for (k = 0; k < NV; k++) { b2[k] = (u8)iv[k]; buf[k] = (u8)pv[k]; }
            igual("variables", b2, buf, NV);
        } else if (ln[0] == 'E' && ln[1] == ' ') comparar_eventos(ln + 2);
    }
    free(tx);
    fprintf(out, "monstruo_test: %d comprobaciones, %d fallos, %d casos\n", pruebas, fallos, caso);
    if (out != stdout) fclose(out);
    return fallos ? 1 : 0;
}
