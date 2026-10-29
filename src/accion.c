/* HeroQuest QL - acciones de los iconos (ver accion.h y docs/ZX_ACCIONES.md). Cada funcion lleva la direccion de la rutina del original. */
#include <string.h>
#include "accion.h"

void (*accion_especial)(Partida *p, int pers, int idx, int valor, Salida *o) = 0;
void (*accion_ataque)(Partida *p, int heroe, int monstruo, Salida *o) = 0;

void accion_mensaje(Salida *o, int id, int a, int b)
{
    if (o->n < MSG_MAX) { o->m[o->n].id = id; o->m[o->n].a = a; o->m[o->n].b = b; o->n++; }
}

#define msg accion_mensaje

/* ---------------------------------------------------------------- muerte */
void heroe_muere(Partida *p, int pers)                           /* 0x7B31 */
{
    Pers *r = &p->pers[pers];
    r->cuerpo = 0;
    r->estado = 0;
    p->v.pers_muerto = (u8)pers;
    pers_marcar_muerte(p, pers);
    if (p->v.pers_actual == pers) turno_terminar(p);
}

/* 0x9FAF: ¿se puede poner algo en la casilla (col, fila)? (sala = la del monstruo que se pone) */
static int casilla_ocupada(const Partida *p, int col, int fila, int sala, int *heroe)
{
    int b, c = fila * MAP_W + col;
    for (b = 0; b < 4; b++) {
        const Pers *r = &p->pers[b];
        if (b == p->v.atacante || !r->estado) continue;
        if (r->col == col && r->fila == fila) { if (heroe) *heroe = b; return 1; }
    }
    if (p->capa_c[c]) return 2;
    if ((p->capa_a[c] & 0x3F) != sala) return 3;
    return 0;
}

int casilla_libre(const Partida *p, int col, int fila, int sala) { return casilla_ocupada(p, col, fila, sala, (int *)0); }

/* ---------------------------------------------------------------- casillas de la sala */
static void revelar(Partida *p, int pers, int c);

/* 0x8662: recorre las casillas de la ventana (6 x 5) que son de la sala; modo 0 = marcarlas como vistas (0x98FB), 1 = buscar (0x8688), 2 = monstruos (0x86F7) */
static void recorrer(Partida *p, int pers, int sala, int modo);

static void buscar_celda(Partida *p, int pers, int c)           /* 0x8688 */
{
    int b = p->capa_b[c], cc = c, e, b2;
    if (b & 8) {                                                 /* puerta secreta de esta casilla */
        if (!(b & 0x10)) {                                       /* al norte */
            if (!(b & 4)) { p->capa_b[c] |= 4; cc = c - MAP_W; revelar(p, pers, cc); p->v.hallado = 1; }
        } else if (!(b & 4)) {                                   /* al oeste */
            p->capa_b[c] |= 4; cc = c - 1; revelar(p, pers, cc); p->v.hallado = 1;
        }
    }
    if (cc >= 0 && cc < MAP_P) {                                 /* trampas de la casilla (o de la de detras de la puerta que se acaba de abrir) */
        int v = p->capa_c[cc];
        if (v == 0x32) { p->capa_c[cc]++; p->v.hallado = 1; }
        else if (v == 0x36) { p->capa_c[cc] = 0; p->v.hallado = 1; }
        else if (v == 0x34) p->v.hallado = 1;
    }
    e = c + 1;                                                   /* la puerta secreta oeste de la casilla de al lado */
    b2 = e < MAP_P ? p->capa_b[e] : 0;
    if ((b2 & 8) && (b2 & 0x10) && !(b2 & 4)) { p->capa_b[e] |= 4; revelar(p, pers, e); p->v.hallado = 1; }
    e = c + MAP_W;                                               /* y la norte de la de abajo */
    b2 = e < MAP_P ? p->capa_b[e] : 0;
    if ((b2 & 8) && (b2 & 0x20) && !(b2 & 4)) { p->capa_b[e] |= 4; revelar(p, pers, e); p->v.hallado = 1; }
}

static void ver_monstruo(Partida *p, int c)                     /* 0x86F7 */
{
    int v = p->capa_c[c];
    if (v && v < 0x30 && (p->mons[v - 1].cuerpo & 1)) p->v.mons_vista = 1;
}

static void recorrer(Partida *p, int pers, int sala, int modo)
{
    int t, c, origen = p->v.origen_y * MAP_W + p->v.origen_x;
    for (t = 0; t < 30; t++) {
        c = origen + vista_ventana_off(t);
        if (c >= MAP_P || (p->capa_a[c] & 0x3F) != sala || !sala) continue;
        if (modo == 0) vista_marcar(p, c);
        else if (modo == 1) buscar_celda(p, pers, c);
        else ver_monstruo(p, c);
    }
}

static void revelar(Partida *p, int pers, int c)                /* 0x860B: la sala que hay en la casilla c (detras de una puerta que se abre) pasa a verse */
{
    int sala = p->capa_a[c] & 0x3F;
    p->v.hab_actual = (u8)sala;
    vista_entrar_sala(p, sala);
    recorrer(p, pers, sala, 0);
    vista_calcular_vista(p, pers);
    vista_descubrir_monstruos(p);
}

void accion_revelar(Partida *p, int pers, int c) { revelar(p, pers, c); }

/* ---------------------------------------------------------------- llave: abrir puertas */
int accion_abrir_puerta(Partida *p, int pers)                   /* 0x72B5: devuelve cuantas puertas ha abierto */
{
    const Pers *r = &p->pers[pers];
    int c = r->fila * MAP_W + r->col, b, e, n = 0;
    p->v.redibujar = 1;
    b = p->capa_b[c];
    if ((b & 0x30) && !(b & 4) && !(b & 8)) {                    /* una puerta de esta casilla (la oeste si la hay, si no la norte) */
        p->capa_b[c] |= 4;
        p->v.sonido_pend = 9;                                    /* 0x7313 */
        revelar(p, pers, (b & 0x10) ? c - 1 : c - MAP_W);
        n++;
    }
    e = c + 1;                                                   /* la puerta oeste de la casilla de al lado */
    b = e < MAP_P ? p->capa_b[e] : 0;
    if (!(b & 4) && !(b & 8) && (b & 0x10)) { p->capa_b[e] |= 4; p->v.sonido_pend = 9; revelar(p, pers, e); n++; }
    e = c + MAP_W;                                               /* la norte de la de abajo */
    if (e >= MAP_P) return n;
    b = p->capa_b[e];
    if ((b & 4) || (b & 8) || !(b & 0x20)) return n;
    p->capa_b[e] |= 4;
    p->v.sonido_pend = 9;
    revelar(p, pers, e);
    return n + 1;
}

/* ---------------------------------------------------------------- ojo: buscar trampas y puertas secretas */
static int sala_del_heroe(const Partida *p, int pers) { return vista_sala_de(p, p->pers[pers].col, p->pers[pers].fila); }

static void especial(Partida *p, int pers, int tabla, int idx, int valor, Salida *o)
{
    msg(o, MSG_ESPECIAL, tabla, valor);
    if (accion_especial) accion_especial(p, pers, idx, valor, o);
}

void accion_buscar_trampas(Partida *p, int pers, Salida *o)     /* 0x8079 y 0x8647 */
{
    int sala = sala_del_heroe(p, pers), v;
    o->n = 0;
    p->v.hallado = 0;
    p->v.hab_actual = (u8)sala;
    vista_entrar_sala(p, sala);
    recorrer(p, pers, sala, 1);
    sala = sala_del_heroe(p, pers);
    v = p->ind[43 + sala];
    if (v == 0) p->ind[43 + sala] = 0xFF;
    else if (v != 0xFF) { especial(p, pers, 0, 43 + sala, v, o); return; }
    if (!p->v.hallado) msg(o, MSG_NADA, 0, 0);                   /* si ha encontrado algo se ve en la vista y no se dice nada */
}

/* ---------------------------------------------------------------- ojo: buscar tesoros */
void accion_buscar_tesoros(Partida *p, int pers, Salida *o)     /* 0x80A9 */
{
    Pers *r = &p->pers[pers];
    o->n = 0;
    for (;;) {
        int sala = sala_del_heroe(p, pers), v, c, k, dx, dy, col, fila, cell;
        p->v.mons_vista = 0;                                     /* 0x862A: con monstruos a la vista no se puede buscar */
        p->v.hab_actual = (u8)sala;
        vista_entrar_sala(p, sala);
        recorrer(p, pers, sala, 2);
        if (p->v.mons_vista) return;
        sala = p->v.hab_actual;
        v = p->ind[sala];
        if (v == 0xFF) { msg(o, MSG_REGISTRADA, 0, 0); return; }
        if (v) { especial(p, pers, 1, sala, v, o); return; }
        p->ind[sala] = 0xFF;
        c = mazo_robar();
        if (c < 9) {                                             /* monedas de oro */
            mazo_devolver(c);
            r->oro = (u16)(r->oro + vista_oro_carta(c));
            msg(o, MSG_ORO, vista_oro_carta(c), 0);
            return;
        }
        if (c < 16) {                                            /* pocion: se queda con el efecto */
            int bit, nombre, desc;
            vista_pocion(c - 9, &bit, &nombre, &desc);
            r->efectos |= (u8)bit;
            msg(o, MSG_POCION, nombre, desc);
            return;
        }
        if (c == 16) { msg(o, MSG_NADA, 0, 0); return; }
        mazo_devolver(c);
        if (c < 20) {                                            /* trampa: pierde un punto de cuerpo */
            msg(o, MSG_TRAMPA, 0, 0);
            if (--r->cuerpo == 0) { heroe_muere(p, pers); msg(o, MSG_MUERTE, pers, 0); }
            return;
        }
        if (p->ev & EV_CASTILLO) { msg(o, MSG_FANTASMA, 0, 0); return; }
        p->v.atacante = (u8)pers;                                /* un monstruo te sigue: aparece en una casilla libre de al lado */
        p->mons[0].hab = r->hab;
        for (k = 0; k < 4; k++) {
            vista_emboscada(k, &dx, &dy);
            col = r->col + dx; fila = r->fila + dy;
            if (col >= MAP_W || col < 0 || fila >= 0x13 || fila < 0) continue;
            if (!casilla_ocupada(p, col, fila, p->mons[0].hab, (int *)0)) break;
        }
        if (k == 4) { p->ind[sala] = 0; continue; }              /* sin sitio: otra carta */
        cell = fila * MAP_W + col;
        if (p->mons[0].estado) p->capa_c[p->mons[0].fila * MAP_W + p->mons[0].col] = 0;
        p->mons[0].estado = 1;
        p->mons[0].col = (u8)col; p->mons[0].fila = (u8)fila;
        p->mons[0].cuerpo |= 1;
        p->mons[0].b7 = (u8)((p->mons[0].b7 & 0x0F) | 0x10);
        p->capa_c[cell] = 1;
        p->mons[0].casilla = (u16)cell;
        p->v.redibujar = 1;
        msg(o, MSG_EMBOSCADA, 0, 0);
        if (accion_ataque) accion_ataque(p, pers, 1, o);          /* 0x81B1: el monstruo ataca nada mas aparecer */
        return;
    }
}

/* ---------------------------------------------------------------- eventos de las casillas con trampa */
static void perder_un_punto(Partida *p, int heroe, Salida *o, int marca)       /* 0xA13B - 0xA14C: se acaba el movimiento y pierde un punto (marca: deja el personaje en 0xA9BB como el foso y la lanza) */
{
    Pers *r = &p->pers[heroe];
    turno_fin_movimiento(p);
    if (marca) p->v.pers_muerto = (u8)p->v.pers_actual;
    if (--r->cuerpo == 0) { heroe_muere(p, heroe); msg(o, MSG_MUERTE, heroe, 0); }
}

static void roca_sobre_casilla(Partida *p, int pers, int cell, Salida *o)    /* 0xA219 y siguientes: tira tres dados (el resultado no cuenta) y el heroe de esa casilla pierde un punto */
{
    int heroe = 0, col = cell % MAP_W, fila = cell / MAP_W, k;
    (void)pers;
    casilla_ocupada(p, col, fila, 0, &heroe);
    for (k = 0; k < 3; k++) dado_combate();
    if (--p->pers[heroe].cuerpo > 0) return;
    p->capa_c[cell] = 0x31;
    p->v.pers_muerto = (u8)heroe;
    heroe_muere(p, heroe);
    msg(o, MSG_MUERTE, heroe, 0);
}

void accion_evento(Partida *p, int pers, Salida *o)             /* 0xA108 (sin 0x70A1) */
{
    Pers *r = &p->pers[pers];
    int ev = p->v.evento, a;
    o->n = 0;
    if (!ev) return;
    p->v.evento = 0;
    if (ev == 2 || ev == 3) {                                    /* con una calavera no pasa nada */
        if (dado_combate() == 0) return;
    }
    if (ev < 4) {                                                /* foso o lanza */
        msg(o, ev == 3 ? MSG_LANZA : MSG_FOSO, 0, 0);
        perder_un_punto(p, pers, o, 1);
        return;
    }
    if (p->ev & EV_CIERRE) {                                     /* mision 11: cuando los cuatro han pisado la trampa se cierra la salida */
        p->v.pers_muerto = (u8)pers;
        if (pers < 4) p->v.ausentes |= (u8)(1 << pers);        /* 0xA25A: aqui el mapa de ausentes cuenta los heroes que han pisado la trampa */
        if (p->v.ausentes != 0x0F) return;
        turno_fin_movimiento(p);
        msg(o, MSG_ROCA, 0, 0);
        p->v.redibujar = 1;
        p->capa_c[1] = 0;
        p->v.atacante = (u8)pers;
        a = casilla_ocupada(p, 0, 0, r->hab, (int *)0);
        if (a == 1) roca_sobre_casilla(p, pers, 0, o); else p->capa_c[0] = 0x31;
        return;
    }
    msg(o, MSG_ROCA, 0, 0);
    p->v.redibujar = 1;
    if (p->ev & EV_ROCA_DER) {                                   /* ampliacion (0x9F46): la roca cae en la casilla de la derecha del heroe */
        int col = r->col + 1, cell = r->fila * MAP_W + col;
        if (col >= MAP_W) return;
        p->v.atacante = (u8)pers;
        a = casilla_ocupada(p, col, r->fila, r->hab, (int *)0);
        if (a == 1) roca_sobre_casilla(p, pers, cell, o); else p->capa_c[cell] = 0x31;
        return;
    }
    if (p->ev & EV_ROCA) {                                       /* mision 4: cae una roca que cierra la casilla (25, 7) */
        p->capa_c[6 * MAP_W + 25] = 0;
        p->v.atacante = (u8)pers;
        a = casilla_ocupada(p, 25, 7, r->hab, (int *)0);
        if (a == 1) roca_sobre_casilla(p, pers, 7 * MAP_W + 25, o); else p->capa_c[7 * MAP_W + 25] = 0x31;
        return;
    }
    a = dado();                                                  /* otras misiones: con 4 o 5 le da (con 4 no si lleva casco y el hechizo) */
    if (a < 4) return;
    if (a == 4 && (r->armadura & 2) && (r->r13 & 2)) return;
    msg(o, MSG_ROCA_PUNTO, 0, 0);
    perder_un_punto(p, pers, o, 0);
}
