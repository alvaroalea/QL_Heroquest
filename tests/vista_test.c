/* Prueba de src/vista.c contra el original: lee build/oraculo_vista.txt (lo genera tools/hq_oraculo_vista.py ejecutando las rutinas del Spectrum
   en un simulador Z80) y comprueba que la version en C (con los graficos empaquetados de data/) da lo mismo:
   estado de la partida (capas, salas, monstruos, heroes), registros de las figuras, orden de dibujo y los puntos del buffer de la vista
   (suelo y paredes; despues las figuras) y del plano. Los graficos del QL tienen color: se compara si el punto es negro (tinta) o no.
   Uso: build/host/vista_test [oraculo] [data/]   (desde la raiz) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vista.h"
#include "gfx.h"

static int fallos = 0, pruebas = 0, caso = 0;
const u8 *test_pk[2];
#ifdef QLTEST                     /* compilado con qcc y ejecutado en el QL (tests/run_ql_vista.sh): sin directorios, la pantalla en la memoria y el resultado a un fichero */
static const char *ddir = "flp1_";
#define FICHERO_ORACULO "flp1_oraculo_vista_txt"
u8 *screen;                       /* lo que el resto del programa pide a la plataforma (plat_ql.c no se enlaza) */
const char *plat_data_dir(void) { return "flp1_"; }
#else
static const char *ddir = "data/";
#define FICHERO_ORACULO "build/oraculo_vista.txt"
#endif
static FILE *out;
static int hexv(char c) { return c >= '0' && c <= '9' ? c - '0' : (c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1); }

static const char *leer_hex(const char *s, u8 *b, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        while (*s == ' ') s++;
        b[i] = (u8)(hexv(s[0]) * 16 + hexv(s[1]));
        s += 2;
    }
    return s;
}

static void igual(const char *que, const u8 *a, const u8 *b, int n)
{
    int i;
    pruebas++;
    for (i = 0; i < n; i++)
        if (a[i] != b[i]) { fprintf(out, "FALLO %s (caso %d) byte %d: C=%02X original=%02X\n", que, caso, i, a[i], b[i]); fallos++; return; }
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

static u8 *cargar(const char *nombre)
{
    char path[200];
    u8 *d;
    sprintf(path, "%s%s", ddir, nombre);
    d = leer_fichero(path, 0);
    if (!d) { fprintf(out, "no se puede leer %s\n", path); exit(2); }
    return d;
}

static int color_de(int x, int y)                     /* color QL del punto de la pantalla */
{
    const u8 *p = screen + (long)y * SCR_STRIDE + (x >> 2) * 2;
    int s = 6 - 2 * (x & 3);
    return (((p[0] >> (s + 1)) & 1) << 2) | (((p[1] >> (s + 1)) & 1) << 1) | ((p[1] >> s) & 1);
}

static void bits_pantalla(u8 *dst)                    /* la vista como el buffer del original: 32 bytes por linea, 1 = tinta negra */
{
    int x, y;
    memset(dst, 0, (size_t)VISTA_ALTO * 32);
    for (y = 0; y < VISTA_ALTO; y++)
        for (x = 0; x < VISTA_ANCHO; x++)
            if (color_de(x, VY0 + y) == C_BLACK) dst[y * 32 + (x >> 3)] |= (u8)(0x80 >> (x & 7));
}

/* con figuras: solo se admiten diferencias dentro del rectangulo de cada figura (mas 8 puntos a los lados): ahi la mascara de los PNG
   (con su propio halo) no es la del original (por bytes; los heroes ademas se desplazan 4 puntos antes de aplicar la mascara) */
static int dentro_de_figura(int x, int y)
{
    int i, l;
    for (i = 0; i < N_ANIM; i++) {
        const Anim *a = &vz_anim[i];
        if (!a->estado) continue;
        l = vista_izquierda(a);
        if (x >= l - 8 && x < l + a->w + 8 && y >= a->y - a->h && y < a->y) return 1;
    }
    return 0;
}

static int tolerado, max_tolerado;

static void comparar_pixeles(const char *que, const u8 *c, const u8 *o, int con_figuras)
{
    int i, n = 0, primero = -1, t = 0;
    pruebas++;
    for (i = 0; i < VISTA_ALTO * 32; i++) {
        u8 d = (u8)(c[i] ^ o[i]);
        int k;
        for (k = 0; k < 8; k++)
            if (d & (0x80 >> k)) {
                if (con_figuras && dentro_de_figura((i % 32) * 8 + k, i / 32)) { t++; continue; }
                n++; if (primero < 0) primero = i * 8 + k;
            }
    }
    tolerado += t; if (t > max_tolerado) max_tolerado = t;
    if (n) {
        fprintf(out, "FALLO %s (caso %d): %d puntos distintos, el primero en x=%d y=%d\n", que, caso, n, (primero % 256), primero / 256); fallos++;
        if (getenv("HQ_DIBUJA")) {                      /* volcado para verlo: C y original, 32 bytes por linea */
            char nombre[80];
            FILE *f;
            sprintf(nombre, "build/vista_dbg_%d_c.bin", caso); f = fopen(nombre, "wb"); if (f) { fwrite(c, 1, VISTA_ALTO * 32, f); fclose(f); }
            sprintf(nombre, "build/vista_dbg_%d_o.bin", caso); f = fopen(nombre, "wb"); if (f) { fwrite(o, 1, VISTA_ALTO * 32, f); fclose(f); }
        }
    }
}

static int canon(int pack, int idx)                  /* el oraculo identifica los graficos por su direccion: los sprites 7 a 15 comparten datos, se toma el primero */
{
    gimg a, b;
    int j;
    if (!pack_get(test_pk[pack], idx, &a)) return idx;
    for (j = 0; j < idx; j++)
        if (pack_get(test_pk[pack], j, &b) && b.w == a.w && b.h == a.h && b.wpr == a.wpr && !memcmp(a.d, b.d, (size_t)a.wpr * a.h * 4)) return j;
    return idx;
}

static void blanco(void) { gfx_cells_fill(0, UI_ROW0, GCOLS, 22, C_WHITE); }

#define LINEA_MAX 40000
static char *siguiente(char **p)
{
    char *l = *p, *n;
    if (!*l) return NULL;
    n = strchr(l, '\n');
    if (n) { *n = 0; *p = n + 1; } else *p = l + strlen(l);
    return l;
}

int main(int argc, char **argv)
{
    const char *fn = argc > 1 ? argv[1] : FICHERO_ORACULO;
    u8 *txt, *spr, *esc, *vis, *bal, *mar, *dm, pr[4];
    static u8 buf[VISTA_ALTO * 32], est[VISTA_ALTO * 32], exp[VISTA_ALTO * 32], b2[1024], b3[1024];   /* estaticos: la pila de un programa del QL es pequena */
    static Partida p;
    static u8 attr[MAP_N];
    char *cur, *ln, nm[200];
    long n;
    int i, m = 0, activo = 0, pos[8], en_caso = 0;
    out = stdout;
#ifdef QLTEST
    out = fopen("flp1_vtest_out", "w");
    if (!out) return 3;
    screen = (u8 *)malloc((size_t)SCR_BYTES);
    if (!screen) { fprintf(out, "sin memoria para la pantalla\n"); fclose(out); return 3; }
    memset(screen, 0, (size_t)SCR_BYTES);
#endif
    if (argc > 2) ddir = argv[2];
    txt = leer_fichero(fn, &n);
    if (!txt) { fprintf(out, "no se puede leer %s (hay que ejecutar tools/hq_oraculo_vista.py)\n", fn); return 2; }
    fprintf(out, "oraculo leido: %ld bytes\n", n); fflush(out);
    spr = cargar("sprites_bin"); esc = cargar("escenario_bin"); vis = cargar("vista_bin"); bal = cargar("baldosas_bin"); mar = cargar("marca_activo_bin");
    test_pk[0] = spr; test_pk[1] = esc;
    if (!vista_datos(spr, esc, vis, bal, mar)) { fprintf(out, "vista_datos rechaza los ficheros (error %d)\n", vista_error); return 2; }
    fprintf(out, "datos cargados\n"); fflush(out);
    gfx_init();
    memset(pr, 1, 4);
    for (cur = (char *)txt; (ln = siguiente(&cur)) != NULL;) {
        if (!strncmp(ln, "CASO ", 5)) {
            sscanf(ln, "CASO %d %d %d %d %d %d %d %d %d %d", &m, &activo, &pos[0], &pos[1], &pos[2], &pos[3], &pos[4], &pos[5], &pos[6], &pos[7]);
            sprintf(nm, "%smision_%02d_bin", ddir, m);
            dm = leer_fichero(nm, &n);
            if (!dm) { fprintf(out, "no se puede leer %s\n", nm); return 2; }
            pers_boot(&p);
            if (!partida_cargar(&p, m, dm, n, pr)) { fprintf(out, "mision rechazada\n"); return 2; }
            free(dm);
            for (i = 0; i < 4; i++) {
                p.pers[i].col = (u8)pos[2 * i]; p.pers[i].fila = (u8)pos[2 * i + 1];
                p.pers[i].hechizos[0] = p.pers[i].hechizos[1] = p.pers[i].objetos = p.pers[i].armas = p.pers[i].armadura = 0;   /* el oraculo los carga a cero */
            }
            p.v.pers_actual = (u8)activo;
            vista_borrar_animaciones();
            blanco();
            vista_calcular_visibles(&p, activo);
            vista_actualizar_heroes(&p);
            bits_pantalla(buf);
            memcpy(est, buf, sizeof(est));
            caso++; en_caso = 1;
            fprintf(out, "caso %d\n", caso); fflush(out);
        } else if (!en_caso) continue;
        else if (!strncmp(ln, "S ", 2)) { leer_hex(ln + 2, exp, VISTA_ALTO * 32); comparar_pixeles("suelo y paredes", buf, exp, 0); }
        else if (!strncmp(ln, "R ", 2)) {
            int idx, est, clave, tipo, x, y, w, h, pack, gi;
            const Anim *a;
            sscanf(ln, "R %d %d %d %d %d %d %d %d %d %d", &idx, &est, &clave, &tipo, &x, &y, &w, &h, &pack, &gi);
            a = &vz_anim[idx];
            b2[0] = a->estado; b3[0] = (u8)est;
            if (est) {
                b2[1] = a->clave; b2[2] = a->tipo; b2[3] = (u8)a->x; b2[4] = (u8)a->y; b2[5] = (u8)a->w; b2[6] = (u8)a->h;
                b3[1] = (u8)clave; b3[2] = (u8)tipo; b3[3] = (u8)x; b3[4] = (u8)y; b3[5] = (u8)w; b3[6] = (u8)h;
                i = 7;
                if (pack >= 0) { b2[i] = a->pack; b3[i] = (u8)pack; b2[i + 1] = (u8)canon(a->pack, a->idx); b3[i + 1] = (u8)gi; i += 2; }
            } else i = 1;
            igual("registro de figura", b2, b3, i);
            if (getenv("HQ_DIBUJA") && memcmp(b2, b3, (size_t)i)) fprintf(out, "  registro %d: C estado=%d clave=%d tipo=%d x=%d y=%d w=%d h=%d pack=%d idx=%d | original estado=%d clave=%d tipo=%d x=%d y=%d w=%d h=%d pack=%d idx=%d\n",
                idx, a->estado, a->clave, a->tipo, a->x, a->y, a->w, a->h, a->pack, a->idx, est, clave, tipo, x, y, w, h, pack, gi);
        } else if (!strncmp(ln, "E ", 2)) {
            leer_hex(ln + 2, b3, 8);
            b2[0] = p.v.hab_actual; b2[1] = p.v.ancla; b2[2] = p.v.origen_x; b2[3] = p.v.origen_y; b2[4] = p.v.sala_nueva;
            b2[5] = p.v.heroe_evento; b2[6] = p.v.evento_mision; b2[7] = p.pers[4].estado;
            igual("estado de la vista", b2, b3, 8);
        } else if (!strncmp(ln, "A ", 2)) { leer_hex(ln + 2, exp, MAP_P); igual("capa A", p.capa_a, exp, MAP_P); }
        else if (!strncmp(ln, "B ", 2)) { leer_hex(ln + 2, exp, MAP_P); igual("capa B", p.capa_b, exp, MAP_P); }
        else if (!strncmp(ln, "M ", 2)) {
            leer_hex(ln + 2, exp, 250);
            for (i = 0; i < N_MONS; i++) mons_bytes(&p.mons[i], b2 + i * 10);
            igual("monstruos", b2, exp, 250);
        } else if (!strncmp(ln, "F ", 2)) { leer_hex(ln + 2, exp, 43); igual("salas visitadas", p.sala_flag, exp, 43); }
        else if (ln[0] == 'H' && ln[1] >= '0' && ln[1] <= '4' && ln[2] == ' ') {
            leer_hex(ln + 3, exp, 26);
            pers_bytes(&p.pers[ln[1] - '0'], b2);
            igual("heroe", b2, exp, 26);
        } else if (!strncmp(ln, "O ", 2)) {
            vista_ordenar();
            leer_hex(ln + 2, exp, 96);
            igual("orden de dibujo", vz_orden, exp, 96);
        } else if (!strncmp(ln, "X ", 2)) {
            vista_figuras_dibujar();
            bits_pantalla(buf);
            leer_hex(ln + 2, exp, VISTA_ALTO * 32);
            comparar_pixeles("vista con figuras", buf, exp, 1);
            {   /* vista_figuras_refrescar: mover dos figuras debe dar lo mismo que borrar todo y volver a dibujar */
                u8 *ref = (u8 *)malloc(VISTA_ALTO * 32), *ver = (u8 *)malloc(VISTA_ALTO * 32);
                int k, movidas = 0;
                for (k = 0; k < N_ANIM && movidas < 2; k++)
                    if (vz_anim[k].estado) { vz_anim[k].x += 4 * (movidas + 1); vz_anim[k].y += 2 - 4 * movidas; movidas++; }
                vista_figuras_refrescar();
                bits_pantalla(ver);
                vista_figuras_borrar();
                vista_figuras_dibujar();
                bits_pantalla(ref);
                igual("refrescar = borrar y dibujar", ver, ref, VISTA_ALTO * 32);
                free(ref); free(ver);
            }
            vista_figuras_borrar();
            bits_pantalla(buf);
            comparar_pixeles("fondo restaurado", buf, est, 0);
        } else if (ln[0] == 'P' && (ln[1] == '0' || ln[1] == '1') && ln[2] == ' ') {
            p.v.marcas = (u8)(ln[1] - '0');
            vista_plano_buffer(&p, buf, attr);
            leer_hex(ln + 3, exp, VISTA_ALTO * 32);
            igual("plano (buffer)", buf, exp, VISTA_ALTO * 32);
            if (getenv("HQ_DIBUJA")) for (i = 0; i < VISTA_ALTO * 32; i++) if (buf[i] != exp[i]) {
                int cx = (i % 32), cy = (i / 32) / 8, c = (cy - PLANO_FIL0) * MAP_W + cx - PLANO_COL0;
                fprintf(out, "  plano caso %d solo=%d: celda col %d fila %d: A=%02X B=%02X C=%02X C=%02X orig=%02X (linea %d)\n", caso, p.v.marcas, cx - PLANO_COL0, cy - PLANO_FIL0,
                    p.capa_a[c], p.capa_b[c], p.capa_c[c], buf[i], exp[i], (i / 32) % 8);
                { int h; for (h = 0; h < 5; h++) if (p.pers[h].estado && p.pers[h].col == cx - PLANO_COL0 && p.pers[h].fila == cy - PLANO_FIL0) fprintf(out, "   heroe %d aqui, activo=%d\n", h, p.v.pers_actual); }
                break;
            }
        } else if (ln[0] == 'T' && (ln[1] == '0' || ln[1] == '1') && ln[2] == ' ') {
            leer_hex(ln + 3, exp, MAP_N);
            igual("plano (atributos)", attr, exp, MAP_N);
        }
    }
    free(txt);
    fprintf(out, "vista_test: %d comprobaciones, %d fallos, %d casos (diferencias del halo dentro de las figuras: %d en total, hasta %d por caso)\n", pruebas, fallos, caso, tolerado, max_tolerado);
    if (out != stdout) fclose(out);
    return fallos ? 1 : 0;
}
