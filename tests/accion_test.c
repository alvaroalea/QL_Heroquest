/* Prueba de src/accion.c contra el original: lee build/oraculo_accion.txt (lo genera tools/hq_oraculo_accion.py ejecutando las rutinas del Spectrum en un
   simulador Z80), pone la partida en el estado de «antes» que vuelca el oraculo, ejecuta la accion y comprueba que el estado de «despues» (heroes, monstruos,
   capas, indicadores, mazo, variables) y las ventanas que habria mostrado son los mismos. Uso: build/host/accion_test [oraculo] [data/] (desde la raiz) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "accion.h"
#include "text.h"
#include "textos_ids.h"

static int fallos = 0, pruebas = 0, caso = 0;
#ifdef QLTEST
static const char *ddir = "flp1_";
#define FICHERO_ORACULO "flp1_oraculo_accion_txt"
u8 *screen;
const char *plat_data_dir(void) { return "flp1_"; }
#else
static const char *ddir = "data/";
#define FICHERO_ORACULO "build/oraculo_accion.txt"
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
    static const char *ventana[] = { "", "AC50", "AC66", "AC90", "ACD9", "AD1D", "AD8A", "ADDE", "AE03", "AF65", "C172", "C19C" };
    static const char *muerte[] = { "BB18", "BB30", "BB46", "BB5D", "BB77" };
    int n, k;
    const u8 *t;
    char tmp[200];
    if (m->id == MSG_MUERTE) sprintf(s, "S %s", muerte[m->a]);
    else if (m->id == MSG_ESPECIAL) sprintf(s, "X %d %d", m->a, m->b);
    else if (m->id == MSG_ORO) sprintf(s, "W ADDE %s%d", m->a < 100 ? "_" : "", m->a);
    else if (m->id == MSG_POCION) {
        strcpy(s, "W AE03 ");
        t = tx_str(TX_POCIONES_NOMBRE, m->a, &n);
        tmp[0] = 0;
        for (k = 0; k < 10; k++) sprintf(tmp + strlen(tmp), "%02X", k < n ? t[k] : 0x5F);
        strcat(s, tmp); strcat(s, " ");
        t = tx_str(TX_POCIONES_DESCRIPCION, m->b, &n);
        tmp[0] = 0;
        for (k = 0; k < 29; k++) sprintf(tmp + strlen(tmp), "%02X", k < n ? t[k] : 0x5F);
        strcat(s, tmp);
    } else sprintf(s, "W %s", ventana[m->id]);
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

int main(int argc, char **argv)
{
    const char *fn = argc > 1 ? argv[1] : FICHERO_ORACULO;
    static Partida p;
    static u8 buf[1600], b2[1600];
    u8 *tx, *spr, *esc, *vis, *bal, *mar, *dm, pr[4];
    char *cur, *ln, nm[200], tipo[10];
    long n;
    int i, k, m = 0, pers = 0, pos[8], ev = 0, dados[8], en_caso = 0, mal = 0;
    Salida o;
    out = stdout;
#ifdef QLTEST
    out = fopen("flp1_atest_out", "w");
    if (!out) return 3;
    screen = (u8 *)malloc((size_t)SCR_BYTES);
    if (!screen) { fprintf(out, "sin memoria para la pantalla\n"); fclose(out); return 3; }
    memset(screen, 0, (size_t)SCR_BYTES);
#endif
    if (argc > 2) ddir = argv[2];
    tx = leer_fichero(fn, &n);
    if (!tx) { fprintf(out, "no se puede leer %s (hay que ejecutar tools/hq_oraculo_accion.py)\n", fn); return 2; }
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
            sscanf(ln, "CASO %d %d %d %d %d %d %d %d %d %d", &m, &pers, &pos[0], &pos[1], &pos[2], &pos[3], &pos[4], &pos[5], &pos[6], &pos[7]);
            sscanf(q, "%9s %d %d %d %d %d %d %d %d %d", tipo, &ev, &dados[0], &dados[1], &dados[2], &dados[3], &dados[4], &dados[5], &dados[6], &dados[7]);
            sprintf(nm, "%smision_%02d_bin", ddir, m);
            dm = leer_fichero(nm, &n);
            if (!dm) { fprintf(out, "no se puede leer %s\n", nm); return 2; }
            pers_boot(&p);
            if (!partida_cargar(&p, m, dm, n, pr)) { fprintf(out, "mision rechazada\n"); return 2; }
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
        } else if (!strncmp(ln, "II ", 3)) leer_hex(ln + 3, p.ind, N_IND);
        else if (!strncmp(ln, "IF ", 3)) { leer_hex(ln + 3, buf, 43); memset(p.sala_flag, 0, N_SALAS); memcpy(p.sala_flag, buf, 43); }
        else if (!strncmp(ln, "ID ", 3)) {
            const char *s = leer_hex(ln + 3, buf, 26);
            int ptr;
            memcpy(mazo, buf, 26);
            while (*s == ' ' || *s == '|') s++;
            ptr = hexv(s[0]) * 4096 + hexv(s[1]) * 256 + hexv(s[2]) * 16 + hexv(s[3]);
            mazo_fin = ptr - 0x9593;
        } else if (!strncmp(ln, "IV ", 3)) {
            int v[15];
            sscanf(ln + 3, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14]);
            p.v.hab_actual = (u8)v[0]; p.v.origen_x = (u8)v[1]; p.v.origen_y = (u8)v[2]; p.v.estado_turno = (u8)v[3]; p.v.pm = (u8)v[4]; p.v.turno_acabado = (u8)v[5];
            p.v.evento = 0; p.v.ausentes = (u8)v[7]; p.v.muertos = (u8)v[8]; p.v.pers_muerto = (u8)v[9]; p.v.hallado = (u8)v[10]; p.v.mons_vista = (u8)v[11];
            p.v.sala_nueva = (u8)v[12]; p.v.evento_mision = (u8)v[13]; p.v.redibujar = (u8)v[14];
            p.v.pers_actual = (u8)pers;
            dado_guion(dados, 8);
            memset(&o, 0, sizeof(o));
            if (!strcmp(tipo, "LLAVE")) accion_abrir_puerta(&p, pers);
            else if (!strcmp(tipo, "TRAMPAS")) accion_buscar_trampas(&p, pers, &o);
            else if (!strcmp(tipo, "TESOROS")) accion_buscar_tesoros(&p, pers, &o);
            else { p.v.evento = (u8)ev; p.v.evento_mision = 0; accion_evento(&p, pers, &o); }
        } else if (ln[0] == 'P' && ln[1] == 'H' && ln[2] >= '0' && ln[2] <= '4') {
            if (mal) continue;
            leer_hex(ln + 4, buf, 26); pers_bytes(&p.pers[ln[2] - '0'], b2);
            igual("heroe", b2, buf, 26);
        } else if (!strncmp(ln, "PM ", 3)) {
            leer_hex(ln + 3, buf, 250);
            for (i = 0; i < N_MONS; i++) mons_bytes(&p.mons[i], b2 + 10 * i);
            for (i = 0; i < N_MONS; i++) if (!b2[10 * i]) memset(b2 + 10 * i + 4, 0, 2);
            igual("monstruos", b2, buf, 250);
        } else if (!strncmp(ln, "PL ", 3)) {
            static u8 l[3 * MAP_P], c[3 * MAP_P];
            leer_hex(ln + 3, l, 3 * MAP_P);
            memcpy(c, p.capa_a, MAP_P); memcpy(c + MAP_P, p.capa_b, MAP_P); memcpy(c + 2 * MAP_P, p.capa_c, MAP_P);
            igual("capas", c, l, 3 * MAP_P);
        } else if (!strncmp(ln, "PI ", 3)) { leer_hex(ln + 3, buf, N_IND); igual("indicadores", p.ind, buf, N_IND); }
        else if (!strncmp(ln, "PF ", 3)) { leer_hex(ln + 3, buf, 43); igual("salas visitadas", p.sala_flag, buf, 43); }
        else if (!strncmp(ln, "PD ", 3)) {
            const char *s = leer_hex(ln + 3, buf, 26);
            int ptr;
            while (*s == ' ' || *s == '|') s++;
            ptr = hexv(s[0]) * 4096 + hexv(s[1]) * 256 + hexv(s[2]) * 16 + hexv(s[3]);
            igual("mazo", mazo, buf, 26);
            pruebas++;
            if (mazo_fin != ptr - 0x9593) { error("final del mazo", 0, mazo_fin, ptr - 0x9593); }
        } else if (!strncmp(ln, "PV ", 3)) {
            int v[15];
            sscanf(ln + 3, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14]);
            b2[0] = p.v.hab_actual; b2[1] = p.v.origen_x; b2[2] = p.v.origen_y; b2[3] = p.v.estado_turno; b2[4] = p.v.pm; b2[5] = p.v.turno_acabado; b2[6] = p.v.evento;
            b2[7] = p.v.ausentes; b2[8] = p.v.muertos; b2[9] = p.v.pers_muerto; b2[10] = p.v.hallado; b2[11] = p.v.mons_vista; b2[12] = p.v.sala_nueva; b2[13] = p.v.evento_mision;
            b2[14] = p.v.redibujar;
            for (k = 0; k < 15; k++) buf[k] = (u8)v[k];
            igual("variables", b2, buf, 14);                       /* sin A987: el original lo pone en cada ventana (es del dibujo) */
        } else if (ln[0] == 'E' && ln[1] == ' ') comparar_eventos(&o, ln + 2);
    }
    free(tx);
    fprintf(out, "accion_test: %d comprobaciones, %d fallos, %d casos\n", pruebas, fallos, caso);
    if (out != stdout) fclose(out);
    return fallos ? 1 : 0;
}
