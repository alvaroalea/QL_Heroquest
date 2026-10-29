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
    if (!m->estado || (m->estado & 0x80)) return MON_NADA;
    if (!(m->cuerpo & 1)) return ((m->cuerpo & 0x80) && p->ruta_n) ? MON_ERRANTE : MON_NADA;   /* 0x9A44: sin vida, solo el errante con ruta actua */
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

/* ---------------------------------------------------------------- el monstruo errante (ampliacion) */
void monstruo_errante_paso(Partida *p, int num)                  /* 0x9CA8-0x9D03 con la ruta como dato de la mision */
{
    Mons *m = &p->mons[num - 1];
    int c = m->fila * MAP_W + m->col, eje, sig, v, dest;
    if (!p->ruta_n) return;
    if (m->tramo >= p->ruta_n) m->tramo = 0;
    eje = p->ruta[m->tramo][0] & 1; sig = (p->ruta[m->tramo][0] & 2) ? 1 : -1; dest = p->ruta[m->tramo][1];
    if (p->capa_c[c] == num) p->capa_c[c] = 0;
    v = (eje ? m->fila : m->col) + sig;
    if (v < 0) v = 0;
    if (v >= (eje ? MAP_H : MAP_W)) v = (eje ? MAP_H : MAP_W) - 1;
    if (eje) m->fila = (u8)v; else m->col = (u8)v;
    if (sig > 0 ? v >= dest : v <= dest) m->tramo = (u8)((m->tramo + 1) % p->ruta_n);
    c = m->fila * MAP_W + m->col;
    m->casilla = (u16)c;
    if (!p->capa_c[c]) p->capa_c[c] = (u8)num;
}

int monstruo_errante_golpe(Partida *p, int num, Salida *o)       /* 0x9D05-0x9D4F */
{
    const Mons *m = &p->mons[num - 1];
    int i;
    for (i = 0; i < 4; i++) {
        Pers *r = &p->pers[i];
        if (!r->estado || r->col != m->col || r->fila != m->fila) continue;
        if (r->cuerpo <= 1) {
            p->v.pers_muerto = (u8)i;
            r->cuerpo = 0;
            heroe_muere(p, i);
            accion_mensaje(o, MSG_MUERTE, i, 0);
        } else {
            r->cuerpo--;
            accion_mensaje(o, MSG_NIEBLA, 0, 0);
        }
        return 1;
    }
    return 0;
}
