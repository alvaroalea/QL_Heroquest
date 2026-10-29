/* Prueba de src/mision.c contra el original: lee build/oraculo_mision.txt (lo genera tools/hq_oraculo_mision.py ejecutando las rutinas del Spectrum en un
   simulador Z80), pone la partida en el estado de «antes» que vuelca el oraculo, ejecuta una sala especial, un evento de mision, la salida de un heroe o una busqueda y comprueba que
   el estado de «despues» (heroes, monstruos, capas, variables) y los guiones que habria mostrado son los mismos. Uso: build/host/mision_test [oraculo] [data/] (desde la raiz) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mision.h"
#include "text.h"
#include "textos_ids.h"
#include "vent_addr.h"

static int fallos = 0, pruebas = 0, caso = 0;
#ifdef QLTEST
static const char *ddir = "flp1_";
#define FICHERO_ORACULO "flp1_oraculo_mision_txt"
u8 *screen;
const char *plat_data_dir(void) { return "flp1_"; }
#else
static const char *ddir = "data/";
#define FICHERO_ORACULO "build/oraculo_mision.txt"
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
static Salida *sal;
static const int *resp_l;
static int resp_i;
static Partida *pp;
static char tipo_g[10];

static void anade(const char *s) { if (evt[0]) strcat(evt, " ; "); strcat(evt, s); }

static int dir_ventana(int tx)
{
    static const int tabla[][2] = { VENT_ADDR };
    unsigned i;
    for (i = 0; i < sizeof(tabla) / sizeof(tabla[0]); i++) if (tabla[i][0] == tx) return tabla[i][1];
    return 0;
}

static void vuelca(Salida *o)                                    /* los mensajes que hay en o, como los habria mostrado el original */
{
    static const char *muerte[] = { "BB18", "BB30", "BB46", "BB5D", "BB77" };
    static const char *ventana[] = { "", "AC50", "AC66", "AC90", "ACD9", "AD1D", "AD8A", "ADDE", "AE03", "AF65", "C172", "C19C" };
    char t[300];
    int i, n, k;
    const u8 *s;
    for (i = 0; i < o->n; i++) {
        const Msg *m = &o->m[i];
        t[0] = 0;
        if (m->id == MSG_VENT) sprintf(t, "W %04X", dir_ventana(m->a));
        else if (m->id == MSG_ORO_PANEL) { if (!strcmp(tipo_g, "ACCION")) continue; strcpy(t, "G"); }
        else if (m->id == MSG_MUERTE) sprintf(t, "S %s", muerte[m->a]);
        else if (m->id == MSG_ESPECIAL) continue;
        else if (m->id == MSG_ORO) sprintf(t, "W ADDE %s%d", m->a < 100 ? "_" : "", m->a);
        else if (m->id == MSG_POCION) {
            char tmp[100];
            strcpy(t, "W AE03 ");
            s = tx_str(TX_POCIONES_NOMBRE, m->a, &n);
            tmp[0] = 0;
            for (k = 0; k < 10; k++) sprintf(tmp + strlen(tmp), "%02X", k < n ? s[k] : 0x5F);
            strcat(t, tmp); strcat(t, " ");
            s = tx_str(TX_POCIONES_DESCRIPCION, m->b, &n);
            tmp[0] = 0;
            for (k = 0; k < 29; k++) sprintf(tmp + strlen(tmp), "%02X", k < n ? s[k] : 0x5F);
            strcat(t, tmp);
        } else if (m->id < 12) sprintf(t, "W %s", ventana[m->id]);
        else sprintf(t, "? %d", m->id);
        anade(t);
    }
    o->n = 0;
}

static int pregunta(Salida *o, int tx)
{
    char t[40];
    vuelca(o);
    sprintf(t, "Q %04X", dir_ventana(tx));
    anade(t);
    return resp_l[resp_i++ % 4];
}

static void ataque(Partida *p, int heroe, int mon, Salida *o)
{
    char t[40];
    vuelca(o);
    sprintf(t, "A %d %d", heroe, mon);
    anade(t);
    combate_defensa(p, heroe, mon, o);
}

static void comparar_eventos(const char *linea)
{
    const char *q = linea;
    pruebas++;
    while (*q == ' ') q++;
    if (strcmp(q, evt)) { fprintf(out, "FALLO sucesos (caso %d):\n  C:        %s\n  original: %s\n    %s\n", caso, evt, q, cabecera); fallos++; }
}

#define NV 14
static void vars_a_partida(Partida *p, const int *v)
{
    Vars *x = &p->v;
    x->turno_acabado = (u8)v[0]; x->ausentes = (u8)v[1]; x->muertos = (u8)v[2]; x->pers_muerto = (u8)v[3]; x->cont_evento = (u8)v[4]; x->cont_tesoro = (u8)v[5];
    x->mask_heroes = (u8)v[6]; x->flag_b4 = (u8)v[7]; x->evento_mision = (u8)v[8]; x->golpes = (u8)v[9]; x->escudos = (u8)v[10]; x->atacante = (u8)v[11];
    x->pers_actual = (u8)v[12]; x->heroe_evento = (u8)v[13];
}

static void partida_a_vars(const Partida *p, int *v)
{
    const Vars *x = &p->v;
    v[0] = x->turno_acabado; v[1] = x->ausentes; v[2] = x->muertos; v[3] = x->pers_muerto; v[4] = x->cont_evento; v[5] = x->cont_tesoro; v[6] = x->mask_heroes;
    v[7] = x->flag_b4; v[8] = x->evento_mision; v[9] = x->golpes; v[10] = x->escudos; v[11] = x->atacante; v[12] = x->pers_actual; v[13] = x->heroe_evento;
}

int main(int argc, char **argv)
{
    const char *fn = argc > 1 ? argv[1] : FICHERO_ORACULO;
    static Partida p;
    static u8 buf[1600], b2[1600];
    static int dados[256], resp[4];
    u8 *tx, *spr, *esc, *vis, *bal, *mar, *dm, pr[4];
    char *cur, *ln, nm[200], tipo[10];
    long n;
    int i, k, m = 0, pers = 0, idx = 0, val = 0, op = 0, en_caso = 0, iv[NV], pv[NV], nd;
    Salida o;
    out = stdout;
#ifdef QLTEST
    out = fopen("flp1_itest_out", "w");
    if (!out) return 3;
    screen = (u8 *)malloc((size_t)SCR_BYTES);
    if (!screen) { fprintf(out, "sin memoria para la pantalla\n"); fclose(out); return 3; }
    memset(screen, 0, (size_t)SCR_BYTES);
#endif
    if (argc > 2) ddir = argv[2];
    tx = leer_fichero(fn, &n);
    if (!tx) { fprintf(out, "no se puede leer %s (hay que ejecutar tools/hq_oraculo_mision.py)\n", fn); return 2; }
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
    mision_pregunta = pregunta; accion_especial = mision_especial; accion_ataque = ataque;
    for (cur = (char *)tx; *cur;) {
        ln = cur;
        cur = strchr(cur, '\n');
        if (cur) *cur++ = 0; else cur = ln + strlen(ln);
        if (!strncmp(ln, "CASO ", 5)) {
            char *q = strchr(ln, '|') + 1;
            idx = 0; val = 0; op = 0;
            sscanf(ln, "CASO %d %d %9s", &m, &pers, tipo);
            if (!strcmp(tipo, "ESPECIAL")) sscanf(ln, "CASO %d %d %9s %d %d", &m, &pers, tipo, &idx, &val);
            else if (!strcmp(tipo, "ACCION")) sscanf(ln, "CASO %d %d %9s %d", &m, &pers, tipo, &op);
            for (k = 0; k < 4; k++) { resp[k] = atoi(q); while (*q == ' ') q++; while (*q && *q != ' ') q++; while (*q == ' ') q++; }
            q = strchr(q, '|') + 1;
            for (nd = 0; nd < 256 && *q; nd++) {
                while (*q == ' ') q++;
                if (!*q) break;
                dados[nd] = atoi(q);
                while (*q && *q != ' ') q++;
            }
            sprintf(nm, "%sheroquest_%02d_es", ddir, m);
            dm = leer_fichero(nm, &n);
            if (!dm) { fprintf(out, "no se puede leer %s\n", nm); return 2; }
            pers_boot(&p);
            if (!partida_cargar(&p, m, dm, n, pr)) { fprintf(out, "mision rechazada\n"); return 2; }
            { static u8 *ix_hq; char pp_hq[80]; if (!ix_hq) { sprintf(pp_hq, "%sheroquest_es_idx", ddir); ix_hq = leer_fichero(pp_hq, 0); } partida_estadisticas(&p, data_estadisticas(ix_hq)); partida_tipos(&p, data_tipos(ix_hq)); }
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
        } else if (!strncmp(ln, "II ", 3)) leer_hex(ln + 3, p.ind, N_IND);
        else if (!strncmp(ln, "IQ ", 3)) leer_hex(ln + 3, p.equipo_ocu, 15);
        else if (!strncmp(ln, "IS ", 3)) { leer_hex(ln + 3, buf, 4); p.salidos = 0; for (k = 0; k < 4; k++) if (buf[k]) p.salidos |= (u8)(1 << k); }
        else if (!strncmp(ln, "IV ", 3)) {
            const char *s = ln + 3;
            for (k = 0; k < NV; k++) { iv[k] = atoi(s); while (*s && *s != ' ') s++; while (*s == ' ') s++; }
            vars_a_partida(&p, iv);
            dado_guion(dados, nd);
            strcpy(tipo_g, tipo);
            evt[0] = 0; resp_l = resp; resp_i = 0; sal = &o; pp = &p;
            memset(&o, 0, sizeof(o));
            if (!strcmp(tipo, "ESPECIAL")) mision_especial(&p, pers, idx, val, &o);
            else if (!strcmp(tipo, "EVENTO")) mision_evento(&p, pers, &o);
            else if (!strcmp(tipo, "SALIDA")) {
                accion_mensaje(&o, MSG_VENT, TX_VENT_BB8E, 0);
                p.v.pers_muerto = (u8)pers;
                pers_marcar_muerte(&p, pers);
                if (pers < 4) p.salidos |= (u8)(1 << pers);
                p.pers[pers].estado = 0;
                turno_terminar(&p);
                mision_salida(&p, pers, &o);
            } else if (op == 0) accion_buscar_trampas(&p, pers, &o);
            else accion_buscar_tesoros(&p, pers, &o);
            vuelca(&o);
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
        } else if (!strncmp(ln, "PI ", 3)) { leer_hex(ln + 3, buf, N_IND); igual("indicadores", p.ind, buf, N_IND); }
        else if (!strncmp(ln, "PS ", 3)) {
            leer_hex(ln + 3, buf, 4);
            for (k = 0; k < 4; k++) { b2[k] = (u8)((p.salidos >> k) & 1); buf[k] = (u8)(buf[k] != 0); }
            igual("heroes que han salido", b2, buf, 4);
        } else if (!strncmp(ln, "PV ", 3)) {
            const char *s = ln + 3;
            for (k = 0; k < NV; k++) { pv[k] = atoi(s); while (*s && *s != ' ') s++; while (*s == ' ') s++; }
            partida_a_vars(&p, iv);
            for (k = 0; k < NV; k++) { b2[k] = (u8)iv[k]; buf[k] = (u8)pv[k]; }
            igual("variables", b2, buf, NV);
        } else if (ln[0] == 'E' && ln[1] == ' ') comparar_eventos(ln + 2);
    }
    free(tx);
    fprintf(out, "mision_test: %d comprobaciones, %d fallos, %d casos\n", pruebas, fallos, caso);
    if (out != stdout) fclose(out);
    return fallos ? 1 : 0;
}
