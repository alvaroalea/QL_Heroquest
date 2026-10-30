/* HeroQuest QL - graficos sobre la pantalla del QL (modo 8). Las celdas son las de texto del Spectrum: 32 x 24 de 8 x 8,
   en las lineas GY0 a GY0 + 191 de la pantalla. */
#ifndef GFX_H
#define GFX_H
#include "platform.h"

/* grafico empaquetado (formato HQGF de tools/hq_ql_empaquetar.py): palabras de 4 puntos, con o sin mascara */
typedef struct {
    const u8 *d;          /* datos de la primera copia */
    int w, h;             /* puntos */
    int wpr;              /* palabras por fila (copias desplazadas: una mas) */
    int copias;           /* 1 o 4 */
    int mask;             /* 1 = cada palabra va precedida de su mascara (2 bytes) */
} gimg;

void gfx_init(void);                                   /* tablas de color */
void gfx_clear(void);                                  /* toda la pantalla a negro */
void gfx_cells_fill(int col, int row, int ncols, int nrows, u8 c);   /* rellena celdas de un color */
#define FONT_GLYPHS 85                                      /* glifos de una fuente: codigos 0x30-0x84 (los del original acaban en 0x73; 0x74-0x84 libres para acentos y n con tilde) */
#define FONT_BYTES  (FONT_GLYPHS * 8)
void gfx_set_font(const u8 *font);                     /* FONT_BYTES bytes: glifo = (codigo - 0x30) * 8 */
void gfx_set_dy(int dy);                               /* baja dy lineas de pantalla las celdas que se dibujen a partir de ahora (0 = normal) */
void gfx_char(int col, int row, u8 code, u8 ink, u8 paper);
void gfx_glifo(int col, int row, const u8 *g8, u8 ink, u8 paper);   /* una celda con 8 bytes de bitmap cualquiera (baldosas del plano) */
void gfx_text(int col, int row, const u8 *s, int n, u8 ink, u8 paper);   /* n codigos del juego */
void gfx_blit(const gimg *g, int x, int y);            /* x multiplo de 4; y en lineas de la pantalla (no de la imagen del Spectrum) */
void gfx_blit_clip(const gimg *g, int x, int y);       /* igual, recortando por la derecha (y por la izquierda) a los 256 puntos de la pantalla */
void gfx_rect_get(u8 *buf, int x, int y, int wpr, int rows);   /* copia a buf el rectangulo de la pantalla (x multiplo de 4, wpr palabras de ancho) */
void gfx_rect_put(const u8 *buf, int x, int y, int wpr, int rows);   /* y lo vuelve a poner */

/* ensamblador (gfx_asm.asm), solo en el QL con -DQL_ASM */
#ifdef QL_ASM
extern void asm_clear(u8 *p, long nbloques);                                    /* nbloques * 32 bytes a cero */
extern void asm_blit_opq(const u8 *src, u8 *dst, long wpr, long filas);         /* copia opaca */
extern void asm_blit_mask(const u8 *src, u8 *dst, long wpr, long filas);        /* copia con mascara */
extern void asm_rect(const u8 *src, u8 *dst, long bytes, long filas, long sstride, long dstride);   /* copia un rectangulo de bytes */
extern void asm_mkpair(u32 *tab, const u16 *tinta, const u16 *papel);          /* tabla de 256 entradas (byte de glifo -> 2 palabras) de un color */
extern void asm_text(u8 *dst, const u8 *font, const u8 *s, long n, const u32 *tab);   /* n celdas de texto seguidas */
extern void asm_char(u8 *dst, const u8 *glifo, const u16 *tinta, const u16 *papel);   /* una celda de texto */
#endif
#endif
