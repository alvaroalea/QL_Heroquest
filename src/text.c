#include "text.h"
#include "idioma_en.h"
#include <string.h>

static const short idioma_en[IDIOMA_EN_N] = IDIOMA_EN_MAP;     /* elemento ingles equivalente a cada elemento espanol (-1 = solo hay espanol) */

u8 ui_ink = C_MAGENTA;

const u8 *tx_item(int id, int *tipo, int *cant)
{
    const u8 *e;
    if (g_idioma == IDIOMA_EN && id >= 0 && id < IDIOMA_EN_N && idioma_en[id] >= 0) id = idioma_en[id];     /* los textos salen en el idioma elegido */
    e = txt + 8 + 8 * id;
    if (tipo) *tipo = e[4];
    if (cant) *cant = (int)rd16(e + 6);
    return txt + rd32(e);
}

const u8 *tx_str(int id, int i, int *len)
{
    const u8 *el = tx_item(id, 0, 0);
    const u8 *s = el + rd16(el + 4 + 2 * i);
    *len = s[0];
    return s + 1;
}

void tx_caja(int col, int row, int w, int h, u8 ink)
{
    static const u8 g[9] = { 0x60, 0x61, 0x62, 0x63, 0x5F, 0x64, 0x65, 0x66, 0x67 };
    int x, y, a, b;
    for (y = 0; y < h; y++) {
        a = (y == 0) ? 0 : (y == h - 1) ? 6 : 3;
        for (x = 0; x < w; x++) {
            b = (x == 0) ? 0 : (x == w - 1) ? 2 : 1;
            gfx_char(col + x, row + y, g[a + b], ink, C_BLACK);
        }
    }
}

void tx_cadena_en(int col, int row, int ancho, const u8 *s, int n, int alin, u8 ink, u8 paper)
{
    int lp, i;
    if (n > ancho) n = ancho;
    lp = (alin == 'C') ? (ancho - n) / 2 : (alin == 'R') ? ancho - n : 0;
    for (i = 0; i < ancho; i++) {
        if (i >= lp && i < lp + n) gfx_char(col + i, row, s[i - lp], ink, paper);
        else gfx_char(col + i, row, 0x5F, ink, paper);
    }
}

void tx_cadena_centrada(int row, const u8 *s, int n, u8 ink, u8 paper)
{
    gfx_text(16 - n / 2, row, s, n, ink, paper);
}

void tx_ops(const u8 *p, int ink, int invert, int dy)
{
    int op, attr, al, x, y, n, c, ci, pa;
    p += 2;                                   /* contador de operaciones */
    while ((op = *p++) != 0) {
        if (op == 1) {
            attr = *p++; al = *p++; x = *p++; y = *p++; n = *p++;
            c = (ink >= 0) ? ink : (attr & 7);
            ci = invert ? C_BLACK : c; pa = invert ? c : C_BLACK;
            if (al == 'C') x = 16 - n / 2; else if (al == 'R') x = x - n;
            gfx_text(x, y + UI_ROW0 + dy, p, n, (u8)ci, (u8)pa);
            for (c = 0; c < n; c++)                  /* los bordes {72} y {73} del nombre se ven en negativo */
                if (p[c] == 0x72 || p[c] == 0x73) gfx_char(x + c, y + UI_ROW0 + dy, p[c], (u8)pa, (u8)ci);
            p += n;
        } else if (op == 2) {
            unsigned off = (unsigned)p[0] | ((unsigned)p[1] << 8);
            tx_caja((int)(off & 31), (int)(off / 256) + dy + UI_ROW0 - 0, p[2], p[3], ui_ink);
            p += 4;
        }                                      /* op 3 = modo OR: no hace falta */
    }
}

void tx_guion(int id, int ink, int invert, int dy)
{
    tx_ops(tx_item(id, 0, 0), ink, invert, dy);
}

void tx_ventana_lineas(int y, int centro, int ancho, int n, const u8 **ln, const int *len, const u8 *al)
{
    int i, x0, w = ancho, row, col;
    for (i = 0; i < n; i++) if (len[i] > w) w = len[i];
    x0 = centro - w / 2;
    row = y + UI_ROW0;
    gfx_char(x0, row, 0x68, C_BLACK, C_WHITE);
    for (col = 0; col < w; col++) gfx_char(x0 + 1 + col, row, 0x69, C_BLACK, C_WHITE);
    gfx_char(x0 + w + 1, row, 0x6A, C_BLACK, C_WHITE);
    for (i = 0; i < n; i++) {
        gfx_char(x0, row + 1 + i, 0x6B, C_BLACK, C_WHITE);
        tx_cadena_en(x0 + 1, row + 1 + i, w, ln[i], len[i], al[i], C_BLACK, C_WHITE);
        gfx_char(x0 + w + 1, row + 1 + i, 0x6C, C_BLACK, C_WHITE);
    }
    gfx_char(x0, row + 1 + n, 0x6D, C_BLACK, C_WHITE);
    for (col = 0; col < w; col++) gfx_char(x0 + 1 + col, row + 1 + n, 0x6E, C_BLACK, C_WHITE);
    gfx_char(x0 + w + 1, row + 1 + n, 0x6F, C_BLACK, C_WHITE);
}

void tx_ventana(int id)
{
    const u8 *p = tx_item(id, 0, 0);
    const u8 *ln[24];
    int len[24], i;
    u8 al[24];
    int y = p[0], centro = p[1], ancho = p[2], n = p[3], nz = p[4];
    p += 6;
    for (i = 0; i < n && i < 24; i++) { al[i] = p[0]; len[i] = p[1]; ln[i] = p + 2; p += 2 + len[i]; }
    (void)nz;
    tx_ventana_lineas(y, centro, ancho, n, ln, len, al);
}

const u8 *tx_ventana_linea(int id, int n, int *len)
{
    const u8 *p = tx_item(id, 0, 0);
    int i, nl = p[3];
    p += 6;
    for (i = 0; i < nl; i++) { if (i == n) { *len = p[1]; return p + 2; } p += 2 + p[1]; }
    *len = 0;
    return p;
}

int tx_zonas(int id, const u8 **zonas)
{
    const u8 *p = tx_item(id, 0, 0);
    int n = p[3], nz = p[4], i;
    p += 6;
    for (i = 0; i < n; i++) p += 2 + p[1];
    *zonas = p;
    return nz;
}

void tx_ventana_con(int id, int nrep, const int *linea, const u8 *const *s, const int *len)
{
    const u8 *p = tx_item(id, 0, 0);
    const u8 *ln[24];
    int ll[24], i, k;
    u8 al[24];
    int y = p[0], centro = p[1], ancho = p[2], n = p[3];
    p += 6;
    for (i = 0; i < n && i < 24; i++) { al[i] = p[0]; ll[i] = p[1]; ln[i] = p + 2; p += 2 + ll[i]; }
    for (k = 0; k < nrep; k++) if (linea[k] < n) { ln[linea[k]] = s[k]; ll[linea[k]] = len[k]; if (al[linea[k]] != 'C') al[linea[k]] = 'C'; }
    tx_ventana_lineas(y, centro, ancho, n, ln, ll, al);
}

/* Linea de texto n de un guion (las cajas y demas operaciones no cuentan) */
int tx_guion_linea(int id, int n, int *x, int *y, int *attr, int *al, const u8 **s, int *len)
{
    const u8 *p = tx_item(id, 0, 0) + 2;
    int op, k = 0, nn;
    while ((op = *p++) != 0) {
        if (op == 1) {
            nn = p[4];
            if (k == n) {
                *attr = p[0]; *al = p[1]; *x = p[2]; *y = p[3]; *len = nn; *s = p + 5;
                if (*al == 'C') *x = 16 - nn / 2; else if (*al == 'R') *x = *x - nn;
                return 1;
            }
            k++; p += 5 + nn;
        } else if (op == 2) p += 4;
    }
    return 0;
}
