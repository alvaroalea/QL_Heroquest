/* Prueba de src/turno.c (y de la animacion de src/vista.c) contra el original: lee build/oraculo_turno.txt (lo genera tools/hq_oraculo_turno.py ejecutando
   las rutinas del Spectrum en un simulador Z80) y comprueba, fotograma a fotograma, que la version en C da el mismo estado: el heroe, su registro de animacion,
   las variables del turno y un resumen de las capas, los monstruos y las salidas. Uso: build/host/turno_test [oraculo] [data/] (desde la raiz) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "turno.h"
#include "gfx.h"

static int fallos = 0, pruebas = 0, caso = 0, caso_mal = 0;
static char cabecera[300];
#ifdef QLTEST                     /* compilado con qcc y ejecutado en el QL (tests/run_ql_turno.sh): sin directorios, la pantalla en la memoria y el resultado a un fichero */
static const char *ddir = "flp1_";
#define FICHERO_ORACULO "flp1_oraculo_turno_txt"
u8 *screen;                       /* lo que el resto del programa pide a la plataforma (plat_ql.c no se enlaza) */
const char *plat_data_dir(void) { return "flp1_"; }
#else
static const char *ddir = "data/";
#define FICHERO_ORACULO "build/oraculo_turno.txt"
#endif
static FILE *out;

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

static u8 *cargar(const char *nombre)
{
    char path[200];
    u8 *d;
    sprintf(path, "%s%s", ddir, nombre);
    d = leer_fichero(path, 0);
    if (!d) { fprintf(out, "no se puede leer %s\n", path); exit(2); }
    return d;
}

static unsigned long hash(const u8 *b, int n)       /* sum((i + 1) * b[i]) como en el oraculo */
{
    unsigned long h = 0;
    int i;
    for (i = 0; i < n; i++) h = (h + (unsigned long)(i + 1) * b[i]) & 0xFFFFFFFFUL;
    return h;
}

#define NV 38
static void resumen(const Partida *p, int pers, unsigned long *v)
{
    const Pers *r = &p->pers[pers];
    const Anim *a = &vz_anim[pers == 4 ? 13 : pers];
    static const unsigned mover_dir[5] = { 0, 0x82CB, 0x8357, 0x83E3, 0x8478 };
    u8 m[250];
    int i = 0, k;
    v[i++] = r->col; v[i++] = r->fila; v[i++] = r->hab; v[i++] = r->casilla; v[i++] = r->fotograma; v[i++] = r->estado; v[i++] = r->armadura;
    v[i++] = r->t18; v[i++] = r->hechizo; v[i++] = r->t17;
    v[i++] = a->estado; v[i++] = a->clave; v[i++] = (unsigned long)a->x & 0xFF; v[i++] = (unsigned long)a->y & 0xFF;
    v[i++] = a->estado == 2 ? a->guion : 0; v[i++] = a->estado == 2 ? (unsigned long)vista_guion_offset(a->guion, a->paso) : 0;
    v[i++] = p->v.pm; v[i++] = p->v.estado_turno; v[i++] = p->v.pasos; v[i++] = p->v.pasos ? mover_dir[p->v.mover] : 0;
    v[i++] = p->v.ult_casilla; v[i++] = p->v.evento; v[i++] = p->v.redibujar; v[i++] = p->v.anim_fin; v[i++] = p->v.turno_acabado;
    v[i++] = p->v.muertos; v[i++] = p->v.pers_muerto; v[i++] = p->v.evento_mision; v[i++] = p->v.sala_nueva;
    v[i++] = hash(p->capa_a, MAP_P); v[i++] = hash(p->capa_b, MAP_P); v[i++] = hash(p->capa_c, MAP_P);
    for (k = 0; k < N_MONS; k++) mons_bytes(&p->mons[k], m + k * 10);
    v[i++] = hash(m, 250);
    for (k = 0; k < 4; k++) v[i++] = (p->salidos >> k) & 1 ? 0xFF : 0;
}

static const char *nombres[NV] = { "col", "fila", "hab", "casilla", "fotograma", "estado", "armadura", "t18", "hechizo", "t17", "anim.estado", "anim.clave", "anim.x", "anim.y",
    "anim.guion", "anim.paso", "pm", "estado_turno", "pasos", "mover", "ult_casilla", "evento", "redibujar", "anim_fin", "turno_acabado", "muertos", "pers_muerto",
    "evento_mision", "sala_nueva", "capa A", "capa B", "capa C", "monstruos", "salida0", "salida1", "salida2", "salida3", "?" };

static int leer_nums(const char *s, unsigned long *v, int n)
{
    int i;
    char *e;
    for (i = 0; i < n; i++) { v[i] = strtoul(s, &e, 10); if (e == s) return 0; s = e; }
    return 1;
}

static void comparar(const char *que, const Partida *p, int pers, const char *linea)
{
    unsigned long c[NV], o[NV];
    int i;
    if (caso_mal) return;
    resumen(p, pers, c);
    pruebas++;
    if (!leer_nums(linea, o, NV - 1)) { fprintf(out, "linea del oraculo mal formada (%s)\n", que); fallos++; return; }
    for (i = 0; i < NV - 1; i++)
        if (c[i] != o[i]) {
            fprintf(out, "FALLO %s (caso %d) %s: C=%lu original=%lu\n    %s\n", que, caso, nombres[i], c[i], o[i], cabecera); fallos++; caso_mal = 1;
            if (getenv("HQ_DIBUJA")) { int j; fprintf(out, "    C:"); for (j = 0; j < NV - 1; j++) fprintf(out, " %lu", c[j]); fprintf(out, "\n    O:"); for (j = 0; j < NV - 1; j++) fprintf(out, " %lu", o[j]); fprintf(out, "\n"); }
            return;
        }
}

static void construir(Partida *p, int pers)          /* REDIBUJAR_TURNO: la parte de la logica (sin el marco ni los marcadores) */
{
    vista_borrar_animaciones();
    p->v.no_dibujar = 0;
    p->v.redibujar = 0;
    vista_calcular_visibles(p, pers);
    vista_actualizar_heroes(p);
}

int main(int argc, char **argv)
{
    const char *fn = argc > 1 ? argv[1] : FICHERO_ORACULO;
    static Partida p;
    u8 *txt, *spr, *esc, *vis, *bal, *mar, *dm, pr[4];
    char *cur, *ln, nm[200];
    long n;
    int i, m = 0, pers = 0, pos[8], en_caso = 0, esclic = 0, fin_frames = 0, d = 0, cx = 0, cy = 0, pm = 0, acc = 0, salida = 0, hecho;
    unsigned long ini[NV];
    out = stdout;
#ifdef QLTEST
    out = fopen("flp1_ttest_out", "w");
    if (!out) return 3;
    screen = (u8 *)malloc((size_t)SCR_BYTES);
    if (!screen) { fprintf(out, "sin memoria para la pantalla\n"); fclose(out); return 3; }
    memset(screen, 0, (size_t)SCR_BYTES);
#endif
    if (argc > 2) ddir = argv[2];
    txt = leer_fichero(fn, &n);
    if (!txt) { fprintf(out, "no se puede leer %s (hay que ejecutar tools/hq_oraculo_turno.py)\n", fn); return 2; }
    spr = cargar("sprites_bin"); esc = cargar("escenario_bin"); vis = cargar("vista_bin"); bal = cargar("baldosas_bin"); mar = cargar("marca_activo_bin");
    if (!vista_datos(spr, esc, vis, bal, mar)) { fprintf(out, "vista_datos rechaza los ficheros (error %d)\n", vista_error); return 2; }
    gfx_init();
    memset(pr, 1, 4);
    for (cur = (char *)txt; *cur;) {
        ln = cur;
        cur = strchr(cur, '\n');
        if (cur) *cur++ = 0; else cur = ln + strlen(ln);
        if (!strncmp(ln, "CASO ", 5)) {
            char tipo[8];
            int rep;
            sscanf(ln, "CASO %d %d %d %d %d %d %d %d %d %d | %d %d", &m, &pers, &pos[0], &pos[1], &pos[2], &pos[3], &pos[4], &pos[5], &pos[6], &pos[7], &pm, &rep);
            esclic = strstr(ln, "CLIC") != NULL;
            if (esclic) sscanf(strchr(ln, '|') + 1, "%d %d %d %d %7s", &pm, &rep, &cx, &cy, tipo);
            else sscanf(strchr(ln, '|') + 1, "%d %d %d %7s", &pm, &rep, &d, tipo);
            sprintf(nm, "%smision_%02d_es_bin", ddir, m);
            dm = leer_fichero(nm, &n);
            if (!dm) { fprintf(out, "no se puede leer %s\n", nm); return 2; }
            pers_boot(&p);
            if (!partida_cargar(&p, m, dm, n, pr)) { fprintf(out, "mision rechazada\n"); return 2; }
            free(dm);
            for (i = 0; i < 4; i++) {
                p.pers[i].col = (u8)pos[2 * i]; p.pers[i].fila = (u8)pos[2 * i + 1];
                p.pers[i].hechizos[0] = p.pers[i].hechizos[1] = p.pers[i].objetos = p.pers[i].armas = p.pers[i].armadura = 0;
            }
            p.v.pers_actual = (u8)pers;
            p.v.pm = (u8)pm;
            caso++; en_caso = 1; salida = 0; hecho = 0; caso_mal = 0; fin_frames = 0; strncpy(cabecera, ln, sizeof(cabecera) - 1); cabecera[sizeof(cabecera) - 1] = 0;
        } else if (!en_caso) continue;
        else if (!strncmp(ln, "I ", 2)) {
            leer_nums(ln + 2, ini, NV - 1);
            p.v.estado_turno = (u8)ini[17];                  /* los valores que el oraculo eligio al azar */
            p.pers[pers].t18 = (u8)ini[7];
            p.v.ult_casilla = (u16)ini[20];
            construir(&p, pers);
            comparar("estado inicial", &p, pers, ln + 2);
        } else if (!strncmp(ln, "K ", 2)) {
            int k = atoi(ln + 2);
            acc = turno_clic(&p, pers, cx, cy);
            if (p.pers[pers].estado == 0) salida = 1;
            pruebas++;
            if (acc >= 4 && acc != 5) { if (acc != k) { fprintf(out, "FALLO accion (caso %d): C=%d original=%d\n", caso, acc, k); fallos++; } hecho = 1; }
            else if (k != -1 && k != 2) { fprintf(out, "FALLO accion (caso %d): C=%d original=%d\n", caso, acc, k); fallos++; }
            else if (k == 2 && !salida) { fprintf(out, "FALLO accion (caso %d): el original llega a la salida y C no\n", caso); fallos++; }
        } else if (!strncmp(ln, "A ", 2)) {
            int r = turno_paso(&p, pers, d), k = atoi(ln + 2);
            pruebas++;
            if ((r == PASO_SALIDA ? 2 : 0) != k) { fprintf(out, "FALLO salida (caso %d): C=%d original=%d\n", caso, r, k); fallos++; }
        } else if (!strncmp(ln, "P ", 2)) {
            if (esclic && !hecho) {                       /* un clic: el camino al cursor y los pasos del primer fotograma los hace el bucle */
            }
            comparar("tras la accion", &p, pers, ln + 2);
        } else if (!strncmp(ln, "F ", 2)) {
            if (fin_frames) continue;
            turno_automatico(&p, pers);
            if (p.pers[pers].estado == 0 && salida == 0) { salida = 1; fin_frames = 1; comparar("fotograma", &p, pers, ln + 2); continue; }   /* llega a la escalera: el original sigue con lo de la mision */
            turno_fotograma(&p, pers);
            if (p.v.redibujar) construir(&p, pers);
            comparar("fotograma", &p, pers, ln + 2);
        }
    }
    free(txt);
    fprintf(out, "turno_test: %d comprobaciones, %d fallos, %d casos\n", pruebas, fallos, caso);
    if (out != stdout) fclose(out);
    return fallos ? 1 : 0;
}
