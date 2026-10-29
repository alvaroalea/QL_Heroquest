/* HeroQuest QL - el turno de un personaje (ver turno.h y docs/ZX_TURNO.md). Cada funcion lleva la direccion de la rutina del original. */
#include <string.h>
#include "turno.h"

void (*turno_evento_salida)(Partida *p, int pers) = 0;

static Anim *registro(int pers) { return &vz_anim[pers == 4 ? 13 : pers]; }       /* 0xA9DF: el registro de animacion del personaje (el quinto es el 13) */

/* ---------------------------------------------------------------- empezar el turno */
void turno_empezar(Partida *p, int pers)                         /* 0x6DDD: a cero A9A4, A9B9, A9BF, A9C0 y A9B5 */
{
    (void)pers;
    p->v.hab_registrada = 0;
    p->v.pasos = 0;
    p->v.ult_casilla = 0;
    p->v.dados_mov = 0;
}

int turno_estado(Partida *p, int pers)                           /* 0x6E04 */
{
    Pers *r = &p->pers[pers];
    int res;
    if (r->t18 & 4) {                                            /* envuelto en una tempestad: pierde el turno */
        r->t18 &= (u8)~4;
        res = TURNO_VENT_TORMENTA;
        if (r->t18 & 2) {                                        /* ademas dormido: intenta despertar */
            if (dado() == 5) r->t18 &= (u8)~2;
            else res |= TURNO_VENT_SUENO;
        }
        return res;
    }
    if (r->t18 & 2) {                                            /* dormido: con un 5 en el dado despierta y juega */
        if (dado() == 5) { r->t18 &= (u8)~2; return TURNO_DESPIERTA; }
        return TURNO_VENT_SUENO;
    }
    return TURNO_JUEGA;
}

void turno_calcular_dados(Partida *p, int pers) { dados_mov_calcular(p, pers); }       /* 0x6ECD */

void turno_dado_fotograma(Partida *p, int pers, int n)           /* 0x6F20-0x6F4F */
{
    Anim *a = registro(pers);
    a->estado = (u8)(((n + 1) & 7) >> 2);                        /* el heroe parpadea: 4 fotogramas sin dibujar y 4 dibujado */
    dados_mov_tirar(p);                                          /* la suma cambia en cada vuelta hasta que se pulsa el boton */
}

void turno_dados_fin(Partida *p, int pers)                       /* 0x6F8A */
{
    (void)p;
    registro(pers)->estado = 0xFF;
}

/* ---------------------------------------------------------------- terminar */
void turno_terminar(Partida *p) { p->v.turno_acabado = 1; }      /* 0x7277 */

int turno_continua(const Partida *p) { return !p->v.turno_acabado; }

void turno_fin_movimiento(Partida *p)                            /* 0xA23E */
{
    p->v.estado_turno |= 4;
    p->v.pm = 0;
}

void turno_pasar(Partida *p, int pers)                           /* 0x725E: quita «atravesar muros» salvo que lo siga habiendo */
{
    Pers *r = &p->pers[pers];
    r->t18 &= (u8)~0x20;
    if (r->t18 & 0x80) { r->t18 |= 0x20; r->t18 &= (u8)~0x80; }
    turno_terminar(p);
}

void turno_fin_de_ronda(Partida *p)                              /* 0x9D35: en 0xA9E5, 0xA9FF, 0xAA19, 0xAA33 y 0xAA4D se quita el bit 0 de +0x18 */
{
    int i;
    for (i = 0; i < N_PERS; i++) p->pers[i].t18 &= (u8)~1;
}

/* ---------------------------------------------------------------- el paso */
static void cancelar(Partida *p) { p->v.pasos = 0; }             /* 0x82C6 */

static void zona_especial(Partida *p, int pers)                  /* 0x7281: en la mision 4 abrir la puerta de la columna 5, filas 3 y 4 deja ver al monstruo 1 */
{
    const Pers *r = &p->pers[pers];
    if ((p->ev & (EV_ROCA | EV_DESPIERTA)) && r->col == 5 && r->fila >= 3 && r->fila < 5) p->mons[1].cuerpo |= 1;
}

static void objeto_en_casilla(Partida *p, int c, int v)         /* 0xA0E6: pisar un objeto lo convierte en otra cosa y deja un evento pendiente; el paso sigue */
{
    static const u8 tab[5][2] = { { 0x33, 1 }, { 0x33, 2 }, { 0x34, 4 }, { 0x35, 0 }, { 0x00, 3 } };    /* tabla 0xA24A */
    if (v >= 0x37) return;
    p->capa_c[c] = tab[v - 0x32][0];
    p->v.evento = tab[v - 0x32][1];
    cancelar(p);
}

static void salida(Partida *p, int pers)                         /* 0x8E7B: llega a la escalera de salida */
{
    cancelar(p);
    p->v.pers_muerto = (u8)pers;
    pers_marcar_muerte(p, pers);                                 /* sale de la partida como los muertos */
    if (pers < 4) p->salidos |= (u8)(1 << pers);                 /* y el menu lo vuelve a tener disponible */
    p->pers[pers].estado = 0;
    turno_terminar(p);
    if (turno_evento_salida) turno_evento_salida(p, pers);
}

static void fin_paso(Partida *p, int pers, int guion, int dest)  /* 0x850F */
{
    Pers *r = &p->pers[pers];
    int sala, i, e, c, hab, col, fila, extra, cell;
    r->casilla = (u16)(0x6400 + dest);
    r->fotograma = (u8)vista_fotograma_base(pers, guion);
    p->v.estado_turno |= 1;
    p->v.pm--;
    if (p->v.sonido_pend != 9) p->v.sonido_pend = 1;             /* 0x8543: el del paso, salvo que haya una puerta pendiente */
    sala = vista_sala_de(p, r->col, r->fila);
    if (sala == r->hab) { vista_iniciar_paso(p, pers, guion); return; }       /* sigue en la sala: se anima */
    if ((p->ev & EV_TELEPORTE) && p->tp_n && r->hab == p->tp_sala) {                    /* ampliacion (0x848C): salir de la sala lleva a un destino al azar */
        const u8 *t = p->tp[dado() % p->tp_n];
        r->hab = t[0]; r->col = t[1]; r->fila = t[2];
        r->casilla = (u16)(0x6400 + t[2] * MAP_W + t[1]);
        p->v.ult_casilla = (u16)(0x6400 + t[4] * MAP_W + t[3]);
        p->v.redibujar = 1;
        return;
    }
    if (sala != 0 || !(p->ev & EV_CASTILLO)) { p->v.redibujar = 1; return; }          /* otra sala: se repinta la vista */
    for (;;) {                                                   /* mision 9: una casilla sin sala es un teletransporte al azar */
        e = dado(); c = dado();
        i = c + e;
        vista_teleport(i, &hab, &col, &fila, &extra);
        if (hab != r->hab) break;
    }
    r->armadura &= (u8)~0x10;
    r->hab = (u8)hab; r->col = (u8)col; r->fila = (u8)fila;
    cell = fila * MAP_W + col;
    r->casilla = (u16)(0x6400 + cell);
    p->capa_b[cell + extra] |= 4;
    p->v.ult_casilla = (u16)(0x6400 + cell + extra);
    p->v.redibujar = 1;
}

int turno_paso(Partida *p, int pers, int dir)
{
    Pers *r = &p->pers[pers];
    int cell = r->fila * MAP_W + r->col, dest, w, b, v, abrir = 0, muro, puerta;
    static const int delta[4] = { -1, 1, -MAP_W, MAP_W };
    static const int guion[4] = { 2, 1, 3, 0 };                  /* direccion de cada paso en la tabla de animaciones (A = 2, 1, 3, 0 en el original) */
    if ((p->v.estado_turno & 4) || !p->v.pm) { cancelar(p); return PASO_NO; }
    if ((dir == ACC_X_MENOS && r->col == 0) || (dir == ACC_X_MAS && r->col == 0x19) ||
        (dir == ACC_Y_MENOS && r->fila == 0) || (dir == ACC_Y_MAS && r->fila == 0x12)) { cancelar(p); return PASO_NO; }
    dest = cell + delta[dir];
    w = (dir == ACC_X_MENOS || dir == ACC_Y_MENOS) ? cell : dest;      /* la casilla que lleva la pared y la puerta de este lado */
    muro = dir < 2 ? 0x40 : 0x80;
    puerta = dir < 2 ? 0x10 : 0x20;
    b = p->capa_b[w];
    if (b & muro) {                                              /* pared: solo se atraviesa con «atravesar muros» y hacia una sala */
        if (!(r->t18 & 0x20)) { cancelar(p); return PASO_NO; }
        if (!(p->capa_a[dest] & 0x3F)) { cancelar(p); return PASO_NO; }
    }
    if (b & puerta) {
        if (!(b & 4)) {                                          /* cerrada: si no esta bloqueada se abre al pasar */
            if (b & 8) { cancelar(p); return PASO_NO; }
            zona_especial(p, pers);
            abrir = 4;
        }
        if (p->v.ult_casilla == (u16)(0x6400 + w)) { cancelar(p); return PASO_NO; }   /* no se cruza dos veces la misma puerta en el turno */
        p->v.ult_casilla = (u16)(0x6400 + w);
    }
    v = p->capa_c[dest];
    if (v) {
        if (v < 0x32 || v >= 0x40) { cancelar(p); return PASO_NO; }       /* monstruo o mueble que bloquea */
        objeto_en_casilla(p, dest, v);
    }
    p->capa_b[w] |= (u8)abrir;
    if (abrir) p->v.sonido_pend = 9;                             /* 0x7313: el sonido de la puerta */
    if (dir == ACC_X_MENOS) r->col--; else if (dir == ACC_X_MAS) r->col++;
    else if (dir == ACC_Y_MENOS) r->fila--; else r->fila++;
    if (v >= 0x38 && v < 0x3C) { salida(p, pers); return PASO_SALIDA; }
    fin_paso(p, pers, guion[dir], dest);
    return PASO_SI;
}

/* ---------------------------------------------------------------- clic y camino */
static void ir_hacia_cursor(Partida *p, int pers, int cx, int cy)       /* 0x6FF6 */
{
    const Pers *r = &p->pers[pers];
    int col, fila, d;
    vista_cursor_a_casilla(p, cx, cy, &col, &fila);
    if (r->col != col) {
        if (r->fila != fila) return;
        d = r->col - col;
        if (d >= 0) p->v.mover = ACC_X_MENOS + 1; else { p->v.mover = ACC_X_MAS + 1; d = -d; }
    } else {
        if (r->fila == fila) return;
        d = r->fila - fila;
        if (d >= 0) p->v.mover = ACC_Y_MENOS + 1; else { p->v.mover = ACC_Y_MAS + 1; d = -d; }
    }
    p->v.pasos = (u8)d;
}

int turno_clic(Partida *p, int pers, int cur_x, int cur_y)
{
    int col = (cur_x - 12) >> 3, fila = (cur_y - 2) >> 3, i;
    const int *z = vista_zonas();
    if (p->v.pasos) return -1;
    if (cur_y < 0x0A || cur_y > 0xAE) return -1;             /* el raton llega a todas las lineas, mas alla de la vista: no es un clic en el tablero */
    if (p->v.anim_fin) {
        if (registro(pers)->estado == 2) return -1;
        p->v.anim_fin = 0;
    }
    for (i = 0; i < 10; i++, z += 5) {
        if (col < z[0] || col >= z[1] || fila < z[2] || fila >= z[3]) continue;
        if (z[4] < 4) { turno_paso(p, pers, z[4]); return z[4]; }
        if (z[4] == ACC_PASAR) turno_pasar(p, pers);
        return z[4];
    }
    ir_hacia_cursor(p, pers, cur_x, cur_y);
    return -1;
}

void turno_automatico(Partida *p, int pers)                      /* 0x6FDA */
{
    if (registro(pers)->estado == 2) return;
    if (!p->v.pasos) return;
    p->v.pasos--;
    turno_paso(p, pers, p->v.mover - 1);
}

void turno_fotograma(Partida *p, int pers)                       /* 0x85F2 y, en 0xA794, el avance de la animacion */
{
    vista_animar_clave(p, pers);
    vista_ordenar();
    vista_animar(p);
}
