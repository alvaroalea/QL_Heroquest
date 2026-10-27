/* Prueba de src/partida.c contra el original: lee tests/oraculo_partida.txt (lo genera tools/hq_oraculo_partida.py ejecutando las
   rutinas del Spectrum en un simulador Z80) y comprueba que la version en C da exactamente los mismos resultados.
   Uso: build/host/partida_test [tests/oraculo_partida.txt] [data/] (desde la raiz del proyecto). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "partida.h"

static int fallos = 0, pruebas = 0;
#ifdef QLTEST                     /* compilado con qcc y ejecutado en el QL (tests/run_ql_partida.sh): sin directorios y el resultado a un fichero */
static const char *ddir = "flp1_";
#define FICHERO_ORACULO "flp1_oraculo_partida_txt"
#else
static const char *ddir = "data/";
#define FICHERO_ORACULO "tests/oraculo_partida.txt"
#endif
static FILE *out;

static int hexv(char c) { return c >= '0' && c <= '9' ? c - '0' : (c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1); }

/* lee n bytes en hexadecimal separados por espacios; devuelve el puntero tras el ultimo */
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

static void igual(const char *que, const u8 *a, const u8 *b, int n, int ctx)
{
    int i;
    pruebas++;
    for (i = 0; i < n; i++)
        if (a[i] != b[i]) { fprintf(out, "FALLO %s (caso %d) byte %d: C=%02X original=%02X\n", que, ctx, i, a[i], b[i]); fallos++; return; }
}

static void pers_de_bytes(Pers *r, const u8 *b)
{
    r->estado = b[0]; r->col = b[1]; r->fila = b[2]; r->hab = b[3]; r->casilla = (u16)(b[4] | (b[5] << 8));
    r->cuerpo = b[6]; r->mente = b[7]; r->fotograma = b[8]; r->oro = (u16)(b[9] | (b[10] << 8));
    r->hechizos[0] = b[11]; r->hechizos[1] = b[12]; r->objetos = b[13]; r->armas = b[14]; r->armadura = b[15]; r->efectos = b[16];
    r->r11 = b[17]; r->r12 = b[18]; r->r13 = b[19]; r->ataque = b[20]; r->defensa = b[21]; r->hechizo = b[22]; r->t17 = b[23];
    r->t18 = b[24]; r->indice = b[25];
}

static const char *rombits;
static long rompos;
static int bit_rom(void) { return rombits[rompos++] == '1'; }

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

static void cargar_caso(Partida *p, int m, int presentes, const u8 eq[4][5])
{
    char nm[256];
    u8 *d, pr[4];
    long n;
    int i;
    pers_boot(p);
    for (i = 0; i < 4; i++) {
        p->pers[i].hechizos[0] = eq[i][0]; p->pers[i].hechizos[1] = eq[i][1]; p->pers[i].objetos = eq[i][2];
        p->pers[i].armas = eq[i][3]; p->pers[i].armadura = eq[i][4];
        pr[i] = (u8)((presentes >> i) & 1);
    }
    sprintf(nm, "%smision_%02d_bin", ddir, m);
    d = leer_fichero(nm, &n);
    if (!d) { fprintf(out, "no se puede leer %s\n", nm); exit(2); }
    if (!partida_cargar(p, m, d, n, pr)) { fprintf(out, "FALLO: partida_cargar rechaza %s\n", nm); fallos++; }
    free(d);
}

int main(int argc, char **argv)
{
    const char *fn = argc > 1 ? argv[1] : FICHERO_ORACULO;
    u8 *txt, eq[4][5], buf[1024], b2[1024];
    static Partida p;
    char *ln, *nx;
    int caso = 0, mz_c = -1, i, mision = 0, presentes = 0;
    long n;
    out = stdout;
#ifdef QLTEST
    out = fopen("flp1_ptest_out", "w");
    if (!out) return 3;
#endif
    if (argc > 2) ddir = argv[2];
    txt = leer_fichero(fn, &n);
    if (!txt) { fprintf(out, "no se puede leer %s (hay que ejecutar tools/hq_oraculo_partida.py)\n", fn); return 2; }
    fprintf(out, "oraculo leido: %ld bytes\n", n); fflush(out);
    {   /* ROMBITS esta al final del fichero pero el barajado lo necesita antes */
        char *q = strstr((char *)txt, "\nROMBITS ");
        if (!q) { fprintf(out, "falta ROMBITS\n"); return 2; }
        rombits = q + 9;
    }
    memset(eq, 0, sizeof(eq));
    for (ln = (char *)txt; ln && *ln; ln = nx) {
        nx = strchr(ln, '\n');
        if (nx) *nx++ = 0;
        if (!strncmp(ln, "ENTRADA ", 8)) {
            const char *s = ln + 8;
            for (i = 0; i < 4; i++) {
                int k;
                for (k = 0; k < 5; k++) { eq[i][k] = (u8)(hexv(s[0]) * 16 + hexv(s[1])); s += 2; }
                while (*s == ' ') s++;
            }
        } else if (!strncmp(ln, "MISION ", 7)) {
            sscanf(ln, "MISION %d PRESENTES %d", &mision, &presentes);
            cargar_caso(&p, mision, presentes, (const u8 (*)[5])eq);
            caso++;
        } else if (!strncmp(ln, "A ", 2)) { leer_hex(ln + 2, buf, MAP_P); igual("capa A", p.capa_a, buf, MAP_P, caso); }
        else if (!strncmp(ln, "B ", 2)) { leer_hex(ln + 2, buf, MAP_P); igual("capa B", p.capa_b, buf, MAP_P, caso); }
        else if (!strncmp(ln, "C ", 2)) { leer_hex(ln + 2, buf, MAP_P); igual("capa C", p.capa_c, buf, MAP_P, caso); }
        else if (!strncmp(ln, "I ", 2)) { leer_hex(ln + 2, buf, N_IND); igual("indicadores", p.ind, buf, N_IND, caso); }
        else if (!strncmp(ln, "M ", 2)) {
            leer_hex(ln + 2, buf, 250);
            for (i = 0; i < N_MONS; i++) mons_bytes(&p.mons[i], b2 + i * 10);
            igual("monstruos", b2, buf, 250, caso);
        } else if (ln[0] == 'H' && ln[1] >= '0' && ln[1] <= '4' && ln[2] == ' ') {
            leer_hex(ln + 3, buf, 26);
            pers_bytes(&p.pers[ln[1] - '0'], b2);
            igual("heroe", b2, buf, 26, caso);
        } else if (!strncmp(ln, "V ", 2)) {
            leer_hex(ln + 2, buf, 0x54);
            memset(b2, 0, 0x54);
            b2[0x33] = p.v.ausentes; b2[0x34] = p.v.muertos;
            igual("variables", b2, buf, 0x54, caso);
        } else if (!strncmp(ln, "S ", 2)) { leer_hex(ln + 2, buf, 15); igual("equipo oculto", p.equipo_ocu, buf, 15, caso); }
        else if (!strncmp(ln, "RESTAURA ", 9)) {
            int h = atoi(ln + 9);
            const char *s = ln + 9;
            while (*s && *s != ' ') s++;
            while (*s == ' ') s++;
            for (i = 0; i < 4; i++) {
                int k;
                for (k = 0; k < 5; k++) { eq[i][k] = (u8)(hexv(s[0]) * 16 + hexv(s[1])); s += 2; }
                while (*s == ' ') s++;
            }
            cargar_caso(&p, 5, 15, (const u8 (*)[5])eq);
            pers_restaurar_equipo(&p, h);
        } else if (ln[0] == 'R' && ln[1] == 'H' && ln[3] == ' ') {
            leer_hex(ln + 4, buf, 26);
            pers_bytes(&p.pers[ln[2] - '0'], b2);
            igual("restaurar equipo", b2, buf, 26, 100 + ln[2] - '0');
        } else if (!strncmp(ln, "INIT ", 5)) {
            const char *s = ln + 5;
            pers_boot(&p);
            for (i = 0; i < 5; i++) {
                s = leer_hex(s, buf, 26);
                while (*s == ' ' || *s == '|') s++;
                pers_bytes(&p.pers[i], b2);
                igual("heroe inicial", b2, buf, 26, i);
            }
        } else if (!strncmp(ln, "NH ", 3)) {
            const char *s = ln + 5;
            int h = ln[3] - '0';
            u8 antes[26], despues[26];
            s = leer_hex(s, antes, 26);
            while (*s == ' ' || *s == '|') s++;
            leer_hex(s, despues, 26);
            pers_de_bytes(&p.pers[h], antes);
            pers_nuevo(&p, h);
            pers_bytes(&p.pers[h], b2);
            igual("heroe nuevo", b2, despues, 26, h);
        } else if (!strncmp(ln, "DM ", 3)) {
            int pers, b13, b0f, b17, b18, r_dm, r17, r10, r18, r_tt;
            Pers *r;
            sscanf(ln, "DM %d %d %d %d %d -> %d %d %d %d %d", &pers, &b13, &b0f, &b17, &b18, &r_dm, &r17, &r10, &r18, &r_tt);
            pers_boot(&p);
            r = &p.pers[pers];
            r->r13 = (u8)b13; r->armadura = (u8)b0f; r->t17 = (u8)b17; r->t18 = (u8)b18; r->efectos = 0x20;
            p.v.turno_acabado = 1; p.v.estado_turno = 1;
            dados_mov_calcular(&p, pers);
            buf[0] = (u8)r_dm; buf[1] = (u8)r17; buf[2] = (u8)r10; buf[3] = (u8)r18; buf[4] = (u8)r_tt;
            b2[0] = p.v.dados_mov; b2[1] = r->t17; b2[2] = r->efectos; b2[3] = r->t18; b2[4] = (u8)(p.v.turno_acabado * 16 + p.v.estado_turno);
            igual("dados de movimiento", b2, buf, 5, pers * 100 + b13 + b0f + b17 + b18);
        } else if (!strncmp(ln, "D ", 2)) {
            int r, c, e;
            sscanf(ln, "D %d %d -> %d", &r, &c, &e);
            buf[0] = (u8)e; b2[0] = (u8)dado_con_r((u8)r, c);
            igual("dado", b2, buf, 1, r * 2 + c);
        } else if (!strncmp(ln, "MZ ", 3)) {
            u8 fin[2];
            const char *s;
            mz_c = atoi(ln + 3);
            s = strchr(ln + 3, ' ') + 1;
            s = leer_hex(s, buf, 26);
            while (*s == ' ' || *s == '|') s++;
            fin[0] = (u8)(hexv(s[2]) * 16 + hexv(s[3]));   /* 0xA9E1 es el byte bajo: viene tras el alto en el volcado */
            rompos = (long)(0x6FC + (0x80 + mz_c)) * 2 + 1 - 0x0EF8;
            mazo_barajar((u8)(0x80 + mz_c), bit_rom);
            igual("mazo", mazo, buf, 26, mz_c);
            pruebas++;
            if (0x9593 + mazo_fin != (hexv(s[0]) * 16 + hexv(s[1])) * 256 + fin[0]) { fprintf(out, "FALLO mazo_fin (caso %d)\n", mz_c); fallos++; }
        } else if (!strncmp(ln, "RC ", 3) || !strncmp(ln, "DC ", 3)) {
            int c, carta = 0;
            const char *s = ln + 3;
            c = atoi(s); s = strchr(s, ' ') + 1;
            if (ln[0] == 'R') { carta = (hexv(s[0]) * 16 + hexv(s[1])); s += 3; }
            s = leer_hex(s, buf, 26);
            while (*s == ' ' || *s == '|') s++;
            if (ln[0] == 'R') {
                b2[0] = (u8)mazo_robar(); buf[26] = (u8)carta;
                igual("carta robada", b2, buf + 26, 1, c);
            } else mazo_devolver(0x15);
            igual("mazo tras robar o devolver", mazo, buf, 26, c);
            pruebas++;
            if (0x9593 + mazo_fin != (hexv(s[0]) * 16 + hexv(s[1])) * 256 + (hexv(s[2]) * 16 + hexv(s[3]))) { fprintf(out, "FALLO mazo_fin tras robar (caso %d)\n", c); fallos++; }
        } else if (!strncmp(ln, "MC ", 3)) {
            int mu, m, st, e;
            sscanf(ln, "MC %d %d %d -> %d", &mu, &m, &st, &e);
            pers_boot(&p);
            p.v.muertos = (u8)mu; p.mision = (u8)m; p.pers[4].estado = (u8)st;
            buf[0] = (u8)e; b2[0] = (u8)mision_continua(&p);
            igual("mision continua", b2, buf, 1, mu * 100 + m * 10 + (st ? 1 : 0));
        }
    }
    free(txt);
    fprintf(out, "partida_test: %d comprobaciones, %d fallos, %d cargas de mision\n", pruebas, fallos, caso);
    if (out != stdout) fclose(out);
    return fallos ? 1 : 0;
}
