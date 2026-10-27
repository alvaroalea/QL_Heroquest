/* HeroQuest QL - graficos. Modo 8 del QL: una palabra (2 bytes) son 4 puntos; para el punto k (0 a 3, de izquierda a derecha)
   con s = 6 - 2k: byte par = verde en el bit s+1 y parpadeo en el bit s; byte impar = rojo en el bit s+1 y azul en el bit s. */
#include "gfx.h"
#include "music.h"
#include <string.h>

static u8 ink_e[8][16], ink_o[8][16];       /* para cada color y cada grupo de 4 puntos (nibble): byte par e impar */
#ifdef QL_ASM
static u16 wink[8][16], wpap[8][16];        /* lo mismo como palabra (par en el byte alto); wpap es el color de papel para el nibble n (puntos a 0) */
#endif
static const u8 *font = 0;
#ifdef QL_ASM
static u32 pairtab[256];                    /* byte de glifo -> dos palabras de pantalla, para el ultimo par tinta/papel usado */
static int pair_i = -1, pair_p = -1;
#endif
static long rowoff[GROWS];                  /* desplazamiento en pantalla de la primera linea de cada fila de celdas (evita multiplicar) */

void gfx_init(void)
{
    int c, n, k, s;
    for (c = 0; c < GROWS; c++) rowoff[c] = (long)(GY0 + c * 8) * SCR_STRIDE;
    for (c = 0; c < 8; c++)
        for (n = 0; n < 16; n++) {
            u8 e = 0, o = 0;
            for (k = 0; k < 4; k++) {
                if (!(n & (8 >> k))) continue;
                s = 6 - 2 * k;
                if (c & 4) e |= (u8)(2 << s);
                if (c & 2) o |= (u8)(2 << s);
                if (c & 1) o |= (u8)(1 << s);
            }
            ink_e[c][n] = e; ink_o[c][n] = o;
        }
#ifdef QL_ASM
    for (c = 0; c < 8; c++)
        for (n = 0; n < 16; n++) {
            wink[c][n] = (u16)((ink_e[c][n] << 8) | ink_o[c][n]);
            wpap[c][n] = (u16)((ink_e[c][15 - n] << 8) | ink_o[c][15 - n]);
        }
#endif
}

#ifdef QL_ASM
static void usar_par(int ink, int paper)
{
    if (ink != pair_i || paper != pair_p) { asm_mkpair(pairtab, wink[ink], wpap[paper]); pair_i = ink; pair_p = paper; }
}
#endif

void gfx_clear(void)
{
    mus_mute(1);                            /* se redibuja la pantalla: sin sonido hasta el siguiente plat_present */
#ifdef QL_ASM
    asm_clear(screen, SCR_BYTES / 32L);
#else
    memset(screen, 0, (size_t)SCR_BYTES);
#endif
}

void gfx_set_font(const u8 *f) { font = f; }

static u8 *cell_ptr(int col, int row) { return screen + rowoff[row] + col * 4; }

void gfx_cells_fill(int col, int row, int ncols, int nrows, u8 c)
{
    int y, x;
    u8 *p, e = ink_e[c][15], o = ink_o[c][15];
    for (y = 0; y < nrows * 8; y++) {
        p = cell_ptr(col, row) + (long)y * SCR_STRIDE;
        for (x = 0; x < ncols * 2; x++) { *p++ = e; *p++ = o; }
    }
}

void gfx_glifo(int col, int row, const u8 *g8, u8 ink, u8 paper)
{
#ifdef QL_ASM
    asm_char(cell_ptr(col, row), g8, wink[ink], wpap[paper]);
#else
    u8 *p = cell_ptr(col, row);
    int r, n1, n2;
    for (r = 0; r < 8; r++, p += SCR_STRIDE) {
        n1 = g8[r] >> 4; n2 = g8[r] & 15;
        p[0] = (u8)(ink_e[ink][n1] | ink_e[paper][15 - n1]);
        p[1] = (u8)(ink_o[ink][n1] | ink_o[paper][15 - n1]);
        p[2] = (u8)(ink_e[ink][n2] | ink_e[paper][15 - n2]);
        p[3] = (u8)(ink_o[ink][n2] | ink_o[paper][15 - n2]);
    }
#endif
}

void gfx_char(int col, int row, u8 code, u8 ink, u8 paper)
{
#ifdef QL_ASM
    u8 cc;
    if (!font) return;
    usar_par(ink, paper);
    cc = code;                              /* un u8 pasado a c68 ocupa un long: &code seria el byte alto */
    asm_text(cell_ptr(col, row), font, &cc, 1L, pairtab);
    return;
#else
    const u8 *g;
    u8 *p = cell_ptr(col, row);
    int r, n1, n2;
    static const u8 blank[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    g = (font && code >= 0x30 && code < 0x30 + 68) ? font + (code - 0x30) * 8 : blank;
    for (r = 0; r < 8; r++, p += SCR_STRIDE) {
        n1 = g[r] >> 4; n2 = g[r] & 15;
        p[0] = (u8)(ink_e[ink][n1] | ink_e[paper][15 - n1]);
        p[1] = (u8)(ink_o[ink][n1] | ink_o[paper][15 - n1]);
        p[2] = (u8)(ink_e[ink][n2] | ink_e[paper][15 - n2]);
        p[3] = (u8)(ink_o[ink][n2] | ink_o[paper][15 - n2]);
    }
#endif
}

void gfx_text(int col, int row, const u8 *s, int n, u8 ink, u8 paper)
{
    int i;
#ifdef QL_ASM
    if (col < 0) { s -= col; n += col; col = 0; }        /* recorte por los dos lados de las 32 columnas */
    if (col + n > GCOLS) n = GCOLS - col;
    if (n > 0 && font) { usar_par(ink, paper); asm_text(cell_ptr(col, row), font, s, (long)n, pairtab); }
    (void)i;
#else
    for (i = 0; i < n && col + i < GCOLS; i++)
        if (col + i >= 0) gfx_char(col + i, row, s[i], ink, paper);
#endif
}

/* Vuelca un grafico en la pantalla. Con mascara: pantalla = (pantalla AND NOT mascara) OR color */
void gfx_blit(const gimg *g, int x, int y)
{
    int r, w;
    const u8 *s = g->d;
    u8 *p;
#ifdef QL_ASM
    p = screen + (long)y * SCR_STRIDE + (x >> 2) * 2;
    if (g->mask) asm_blit_mask(s, p, (long)g->wpr, (long)g->h);
    else asm_blit_opq(s, p, (long)g->wpr, (long)g->h);
    (void)r; (void)w;
    return;
#endif
    for (r = 0; r < g->h; r++) {
        p = screen + (long)(y + r) * SCR_STRIDE + (x >> 2) * 2;
        if (g->mask) {
            for (w = 0; w < g->wpr; w++, s += 4, p += 2) {
                p[0] = (u8)((p[0] & ~s[0]) | s[2]);
                p[1] = (u8)((p[1] & ~s[1]) | s[3]);
            }
        } else {
            for (w = 0; w < g->wpr; w++, s += 2, p += 2) { p[0] = s[0]; p[1] = s[1]; }
        }
    }
}

/* Igual que gfx_blit pero recortando lo que se salga de los 256 puntos: el cursor llega al borde derecho de la vista */
void gfx_blit_clip(const gimg *g, int x, int y)
{
    int r, w, w0 = 0, w1 = g->wpr, sw = g->mask ? 4 : 2;
    const u8 *s;
    u8 *p;
    if (x < 0) w0 = (-x + 3) / 4;
    if (x + w1 * 4 > SCR_W) w1 = (SCR_W - x) / 4;
    if (w1 <= w0) return;
#ifdef QL_ASM
    if (w0 == 0 && w1 == g->wpr) { gfx_blit(g, x, y); return; }
#endif
    for (r = 0; r < g->h; r++) {
        s = g->d + (long)r * g->wpr * sw + w0 * sw;
        p = screen + (long)(y + r) * SCR_STRIDE + ((x >> 2) + w0) * 2;
        if (g->mask)
            for (w = w0; w < w1; w++, s += 4, p += 2) { p[0] = (u8)((p[0] & ~s[0]) | s[2]); p[1] = (u8)((p[1] & ~s[1]) | s[3]); }
        else
            for (w = w0; w < w1; w++, s += 2, p += 2) { p[0] = s[0]; p[1] = s[1]; }
    }
}

void gfx_rect_get(u8 *buf, int x, int y, int wpr, int rows)
{
    const u8 *src = screen + (long)y * SCR_STRIDE + (x >> 2) * 2;
#ifdef QL_ASM
    asm_rect(src, buf, (long)wpr * 2, (long)rows, SCR_STRIDE, (long)wpr * 2);
#else
    int r;
    for (r = 0; r < rows; r++) memcpy(buf + (long)r * wpr * 2, src + (long)r * SCR_STRIDE, (size_t)wpr * 2);
#endif
}

void gfx_rect_put(const u8 *buf, int x, int y, int wpr, int rows)
{
    u8 *dst = screen + (long)y * SCR_STRIDE + (x >> 2) * 2;
#ifdef QL_ASM
    asm_rect(buf, dst, (long)wpr * 2, (long)rows, (long)wpr * 2, SCR_STRIDE);
#else
    int r;
    for (r = 0; r < rows; r++) memcpy(dst + (long)r * SCR_STRIDE, buf + (long)r * wpr * 2, (size_t)wpr * 2);
#endif
}
