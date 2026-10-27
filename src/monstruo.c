/* HeroQuest QL - turno de los monstruos (ver monstruo.h y docs/ZX_MONSTRUOS.md). Cada funcion lleva la direccion de la rutina del original. */
#include <string.h>
#include "monstruo.h"

#define LIBRE_HEROE 0xFF          /* 0x9F94: el heroe al que se va esta en la casilla */

static signed char cam[64][2];    /* 0x6B8A: pares (dx, dy) del camino */
static int cam_n;                 /* 0xA99A */

int monstruo_pasos(void) { return cam_n; }

static int cuerpo_ocupada(const Partida *p, int col, int fila, int sala, int heroe)   /* 0x9F94 y 0x9FAF */
{
    const Pers *h = &p->pers[heroe];
    if (h->estado && col == h->col && fila == h->fila) return LIBRE_HEROE;
    return casilla_libre(p, col, fila, sala);
}

static int signo(int d) { return d == 0 ? 0 : (d > 0 ? 1 : -1); }

static int camino(Partida *p, const Mons *m, const Pers *h)     /* 0x9DC0: 0 llega (junto al heroe), 2 no hay camino; 3 ya esta junto a el */
{
    int tx = h->col, ty = h->fila, cx = m->col, cy = m->fila, sx, sy, adx, ady, r, x_dom;
    if (!h->estado || h->hab != m->hab || (h->t18 & 1)) return 2;
    if ((p->capa_a[ty * MAP_W + tx] & 0x3F) != m->hab) return 2;  /* el original salta a 0xA999, que son datos: no ocurre en el juego */
    sx = signo(tx - cx); sy = signo(ty - cy);
    adx = tx > cx ? tx - cx : cx - tx; ady = ty > cy ? ty - cy : cy - ty;
    x_dom = adx >= ady;                                          /* 0x9E71 */
    cam_n = 0;                                                   /* 0x9E42: antes de esto A99A conserva el valor de antes */
    for (;;) {
        if (!x_dom) {                                            /* 0x9E75: primero en Y */
            if (ty != cy && sy != 0) {
                r = cuerpo_ocupada(p, cx, cy + sy, m->hab, h->indice);
                if (r == LIBRE_HEROE) break;
                if (r == 0) { cy += sy; cam[cam_n][0] = 0; cam[cam_n][1] = (signed char)sy; cam_n++; continue; }
            }
            if (tx == cx || sx == 0) return 2;                   /* 0x9EB4 */
            r = cuerpo_ocupada(p, cx + sx, cy, m->hab, h->indice);
            if (r == LIBRE_HEROE) break;
            if (r != 0) return 2;
            cx += sx; cam[cam_n][0] = (signed char)sx; cam[cam_n][1] = 0; cam_n++;
        } else {                                                 /* 0x9EF7: primero en X */
            if (tx != cx && sx != 0) {
                r = cuerpo_ocupada(p, cx + sx, cy, m->hab, h->indice);
                if (r == LIBRE_HEROE) break;
                if (r == 0) { cx += sx; cam[cam_n][0] = (signed char)sx; cam[cam_n][1] = 0; cam_n++; continue; }
            }
            if (ty == cy || sy == 0) return 2;                   /* 0x9F35 */
            r = cuerpo_ocupada(p, cx, cy + sy, m->hab, h->indice);
            if (r == LIBRE_HEROE) break;
            if (r != 0) return 2;
            cy += sy; cam[cam_n][0] = 0; cam[cam_n][1] = (signed char)sy; cam_n++;
        }
        if (cam_n >= 60) return 2;
    }
    return cam_n ? 0 : 3;                                        /* 0x9E5F */
}

int monstruo_planear(Partida *p, int num)                        /* 0x9D6B */
{
    Mons *m = &p->mons[num - 1];
    int i, c;
    cam_n = p->v.camino_n;
    if (!m->estado || (m->estado & 0x80) || !(m->cuerpo & 1)) return MON_NADA;
    if (m->cuerpo & 4) {                                         /* dormido: despierta si el R de la tirada vale 5 (no la cara: el original mira A) */
        dado();
        if (dado_r() != 5) return MON_NADA;
        m->cuerpo &= (u8)~4;
    }
    if (m->cuerpo & 2) { m->cuerpo &= (u8)~2; return MON_NADA; }     /* envuelto en una tempestad: pierde el turno */
    for (i = 0; i < 5; i++) {
        p->v.atacante = p->pers[i].indice;                       /* 0x9D9E */
        c = camino(p, m, &p->pers[i]);
        p->v.resultado = (u8)c;
        p->v.camino_n = (u8)cam_n;
        if (c == 0) return MON_MOVER;
        if (c == 3) return MON_ATACAR;
    }
    return MON_NADA;
}

void monstruo_paso(Partida *p, int num, int k)                   /* 0xA086 */
{
    Mons *m = &p->mons[num - 1];
    int c = m->fila * MAP_W + m->col, v = p->capa_c[c];
    p->capa_c[c] = 0;
    m->col = (u8)(m->col + cam[k][0]);
    m->fila = (u8)(m->fila + cam[k][1]);
    c = m->fila * MAP_W + m->col;
    p->capa_c[c] = (u8)v;
    m->casilla = (u16)c;
}
