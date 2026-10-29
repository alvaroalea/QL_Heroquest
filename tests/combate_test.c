/* Prueba de src/combate.c contra el original: lee build/oraculo_combate.txt (lo genera tools/hq_oraculo_combate.py ejecutando las rutinas del Spectrum en un
   simulador Z80), pone la partida en el estado de «antes» que vuelca el oraculo, ejecuta el ataque, la defensa, el hechizo o el manejador del panel y comprueba que
   el estado de «despues» (heroes, monstruos, capas, variables) y los guiones que habria mostrado son los mismos. Uso: build/host/combate_test [oraculo] [data/] (desde la raiz) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "combate.h"
#include "text.h"
#include "textos_ids.h"

static int fallos = 0, pruebas = 0, caso = 0;
#ifdef QLTEST
static const char *ddir = "flp1_";
#define FICHERO_ORACULO "flp1_oraculo_combate_txt"
u8 *screen;
const char *plat_data_dir(void) { return "flp1_"; }
#else
static const char *ddir = "data/";
#define FICHERO_ORACULO "build/oraculo_combate.txt"
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

/* Lo que dice el oraculo: Msg -> texto del suceso */
static void texto_msg(const Msg *m, char *s)
{
    static const char *muerte[] = { "BB18", "BB30", "BB46", "BB5D", "BB77" };
    switch (m->id) {
    case MSG_MUERTE: sprintf(s, "S %s", muerte[m->a]); break;
    case MSG_BANE_ORC: strcpy(s, "S B9E3"); break;
    case MSG_ESTRELLA: strcpy(s, "S BA09"); break;
    case MSG_HECHIZO_OK: strcpy(s, "S BA8F"); break;
    case MSG_HECHIZO_NO: strcpy(s, "S BAAC"); break;
    case MSG_HECHIZO_1: strcpy(s, "S BAC6"); break;
    case MSG_HECHIZO_N: sprintf(s, "S BAEA %02X", 0x30 + m->a); break;
    case MSG_AGUA_SANTA:
        if (m->a == 0) strcpy(s, "S BA6A");
        else sprintf(s, "S BA2F %02X %02X", m->a >= 10 ? 0x31 : 0x5F, 0x30 + m->a % 10);
        break;
    case MSG_TRAMPA_OFF: strcpy(s, "S B968"); break;
    case MSG_TRAMPA_ON: strcpy(s, "S B982"); break;
    case MSG_ORO_MINA: strcpy(s, "S B9B7"); break;
    default: sprintf(s, "? %d", m->id); break;
    }
}

static void comparar_eventos(const Salida *o, const char *linea)
{
    char esp[900], c[900], t[300];
    int i;
    const char *q = linea;
    pruebas++;
    esp[0] = 0;
    while (*q == ' ') q++;
    strcpy(esp, q);
    c[0] = 0;
    for (i = 0; i < o->n; i++) { texto_msg(&o->m[i], t); if (i) strcat(c, " ; "); strcat(c, t); }
    if (strcmp(esp, c)) { fprintf(out, "FALLO ventanas (caso %d):\n  C:        %s\n  original: %s\n    %s\n", caso, c, esp, cabecera); fallos++; }
}

#define NV 25
static void vars_a_partida(Partida *p, const int *v)
{
    Vars *x = &p->v;
    x->estado_turno = (u8)v[0]; x->pm = (u8)v[1]; x->turno_acabado = (u8)v[2]; x->hab_registrada = (u8)v[3]; x->bono_arma = (u8)v[4]; x->dados_ataque = (u8)v[5];
    x->dados_defensa = (u8)v[6]; x->ataque_base = (u8)v[7]; x->defensa_base = (u8)v[8]; x->golpes = (u8)v[9]; x->escudos = (u8)v[10]; x->resultado = (u8)v[11];
    x->vis = (u8)v[12]; x->ausentes = (u8)v[13]; x->muertos = (u8)v[14]; x->pers_muerto = (u8)v[15]; x->objetivo = (u8)v[16]; x->casilla_cursor = (u16)v[17];
    x->origen_x = (u8)v[18]; x->origen_y = (u8)v[19]; x->arma_alcance = (u8)v[20]; x->elegido = (u8)v[21]; x->zona = (u8)v[22]; x->pers_actual = (u8)v[23]; x->marcas = (u8)v[24];
}

static void partida_a_vars(const Partida *p, int *v)
{
    const Vars *x = &p->v;
    v[0] = x->estado_turno; v[1] = x->pm; v[2] = x->turno_acabado; v[3] = x->hab_registrada; v[4] = x->bono_arma; v[5] = x->dados_ataque; v[6] = x->dados_defensa;
    v[7] = x->ataque_base; v[8] = x->defensa_base; v[9] = x->golpes; v[10] = x->escudos; v[11] = x->resultado; v[12] = x->vis; v[13] = x->ausentes; v[14] = x->muertos;
    v[15] = x->pers_muerto; v[16] = x->objetivo; v[17] = x->casilla_cursor; v[18] = x->origen_x; v[19] = x->origen_y; v[20] = x->arma_alcance; v[21] = x->elegido;
    v[22] = x->zona; v[23] = x->pers_actual; v[24] = x->marcas;
}

int main(int argc, char **argv)
{
    const char *fn = argc > 1 ? argv[1] : FICHERO_ORACULO;
    static Partida p;
    static u8 buf[1600], b2[1600], plano[VISTA_ALTO * 32];
    u8 *tx, *spr, *esc, *vis, *bal, *mar, *dm, pr[4];
    char *cur, *ln, nm[200], tipo[10], tg[4];
    long n;
    int i, k, m = 0, pers = 0, dados[20], en_caso = 0, mal = 0, hz = 0, cell = 0, who = 0, z = 0, fila = 0, nmon = 0, hd = 0, iv[NV], pv[NV];
    Salida o;
    out = stdout;
#ifdef QLTEST
    out = fopen("flp1_ctest_out", "w");
    if (!out) return 3;
    screen = (u8 *)malloc((size_t)SCR_BYTES);
    if (!screen) { fprintf(out, "sin memoria para la pantalla\n"); fclose(out); return 3; }
    memset(screen, 0, (size_t)SCR_BYTES);
#endif
    if (argc > 2) ddir = argv[2];
    tx = leer_fichero(fn, &n);
    if (!tx) { fprintf(out, "no se puede leer %s (hay que ejecutar tools/hq_oraculo_combate.py)\n", fn); return 2; }
    {
        char path[200];
        sprintf(path, "%stextos_es", ddir); txt = leer_fichero(path, 0);
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
            tg[0] = 0; hd = 0; nmon = 0; hz = 0; cell = 0; who = 0; z = 0; fila = 0;
            sscanf(ln, "CASO %d %d %9s", &m, &pers, tipo);
            if (!strcmp(tipo, "ATAQUE")) sscanf(ln, "CASO %d %d %9s %d", &m, &pers, tipo, &nmon);
            else if (!strcmp(tipo, "DEFENSA")) sscanf(ln, "CASO %d %d %9s %d %d", &m, &pers, tipo, &hd, &nmon);
            else if (!strcmp(tipo, "HECHIZO")) sscanf(ln, "CASO %d %d %9s %3s %d %d %d", &m, &pers, tipo, tg, &hz, &cell, &who);
            else sscanf(ln, "CASO %d %d %9s %d %d", &m, &pers, tipo, &z, &fila);
            for (i = 0; i < 20; i++) dados[i] = 0;
            sscanf(q, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &dados[0], &dados[1], &dados[2], &dados[3], &dados[4], &dados[5], &dados[6], &dados[7],
                   &dados[8], &dados[9], &dados[10], &dados[11], &dados[12], &dados[13], &dados[14], &dados[15], &dados[16], &dados[17], &dados[18], &dados[19]);
            sprintf(nm, "%sheroquest_%02d_es", ddir, m);
            dm = leer_fichero(nm, &n);
            if (!dm) { fprintf(out, "no se puede leer %s\n", nm); return 2; }
            pers_boot(&p);
            if (!partida_cargar(&p, m, dm, n, pr)) { fprintf(out, "mision rechazada\n"); return 2; }
            { static u8 *ix_hq; char pp_hq[80]; if (!ix_hq) { sprintf(pp_hq, "%sheroquest_es_idx", ddir); ix_hq = leer_fichero(pp_hq, 0); } partida_estadisticas(&p, data_estadisticas(ix_hq)); partida_tipos(&p, data_tipos(ix_hq)); }
            free(dm);
            strncpy(cabecera, ln, sizeof(cabecera) - 1); cabecera[sizeof(cabecera) - 1] = 0;
            caso++; en_caso = 1; mal = 0;
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
            dado_guion(dados, 20);
            memset(&o, 0, sizeof(o));
            if (!strcmp(tipo, "ATAQUE")) {
                int alc;
                p.v.objetivo = (u8)nmon;
                combate_preparar(&p, pers);
                alc = combate_alcance(&p, pers, plano);
                p.v.resultado = (u8)alc;
                if (alc == 0) {
                    combate_bonos_ataque(&p, pers);
                    combate_datos_monstruo(&p);
                    combate_ataque(&p, pers, &o);
                }
            } else if (!strcmp(tipo, "DEFENSA")) {
                combate_defensa(&p, hd, nmon, &o);
            } else if (!strcmp(tipo, "HECHIZO")) {
                p.pers[pers].hechizo = (u8)hz;
                p.v.casilla_cursor = (u16)cell;
                if (tg[0] == 'H') p.v.pers_muerto = (u8)who;
                else if (tg[0] == 'M') { p.v.pers_muerto = BLANCO_MONSTRUO; p.v.objetivo = (u8)who; }
                else p.v.pers_muerto = BLANCO_CASILLA;
                hechizo_lanzar(&p, pers, plano, &o);
            } else {
                switch (z) {
                case 0: panel_efecto(&p, pers, fila, &o); break;
                case 1: panel_objeto(&p, pers, fila); break;
                case 2: panel_arma(&p, pers, fila); break;
                case 3: case 5: panel_hechizo(&p, pers, fila); break;
                case 4: panel_armadura(&p, pers, fila); break;
                case 6: panel_arma_sola(&p, pers, fila); break;
                case 7: if (p.pers[pers].armadura & 0x80) { panel_caja(&p, pers, &o); p.v.zona = 1; } break;
                case 9: panel_soltar_oro(&p, pers, &o); break;
                default: break;
                }
            }
        } else if (ln[0] == 'P' && ln[1] == 'H' && ln[2] >= '0' && ln[2] <= '4') {
            if (mal) continue;
            leer_hex(ln + 4, buf, 26); pers_bytes(&p.pers[ln[2] - '0'], b2);
            igual("heroe", b2, buf, 26);
        } else if (!strncmp(ln, "PM ", 3)) {
            leer_hex(ln + 3, buf, 250);
            for (i = 0; i < N_MONS; i++) mons_bytes(&p.mons[i], b2 + 10 * i);
            for (i = 0; i < N_MONS; i++) if (!b2[10 * i]) { memset(b2 + 10 * i + 4, 0, 2); memset(buf + 10 * i + 4, 0, 2); }       /* un monstruo muerto conserva su direccion */
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
        } else if (ln[0] == 'E' && ln[1] == ' ') comparar_eventos(&o, ln + 2);
    }
    free(tx);
    fprintf(out, "combate_test: %d comprobaciones, %d fallos, %d casos\n", pruebas, fallos, caso);
    if (out != stdout) fclose(out);
    return fallos ? 1 : 0;
}
