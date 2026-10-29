/* HeroQuest QL - lo propio de cada mision (ver mision.h y docs/ZX_MISIONES.md). Cada funcion lleva la direccion de la rutina del original. */
#include <string.h>
#include "mision.h"
#include "textos_ids.h"

int (*mision_pregunta)(Salida *o, int ventana) = 0;

static void msg(Salida *o, int id, int a, int b) { accion_mensaje(o, id, a, b); }
static void vent(Salida *o, int tx) { msg(o, MSG_VENT, tx, 0); }

static int pregunta(Salida *o, int ventana) { return mision_pregunta ? mision_pregunta(o, ventana) : 0; }

static void oro(Partida *p, int heroe, int cantidad)             /* 0x8221 y 0x8225 */
{
    p->pers[heroe].oro = (u16)(p->pers[heroe].oro + cantidad);
}

static int ind_mas43(const Partida *p, int idx)                  /* (IX + 0x2B): el indicador de la otra tabla y, pasado el final, bytes de los monstruos */
{
    u8 b[10];
    int k = idx + 43;
    if (k < N_IND) return p->ind[k];
    k -= N_IND;
    mons_bytes(&p->mons[k / 10], b);
    return b[k % 10];
}

static void muere_o_pierde(Partida *p, int pers, Salida *o)     /* 0x9025-0x9033 y 0x90A6: pierde un punto de cuerpo; si es el ultimo, muere; devuelve 1 si muere */
{
    Pers *r = &p->pers[pers];
    if (r->cuerpo == 1) {
        p->v.pers_muerto = (u8)pers;
        r->cuerpo = 0;
        heroe_muere(p, pers);
        msg(o, MSG_MUERTE, pers, 0);
        return;
    }
    r->cuerpo--;
}

/* ---------------------------------------------------------------- salas especiales (0x827A y la tabla 0x82A2) */
static void con_oro(Partida *p, int pers, Salida *o, int cantidad, int tx)       /* 0x9067: el oro, la ventana y el oro nuevo */
{
    oro(p, pers, cantidad);
    vent(o, tx);
    msg(o, MSG_ORO_PANEL, pers, 0);
}

static void esp_trampa(Partida *p, int pers, int idx, Salida *o, int oro_antes, int tx_ok)   /* 0x9010 y 0x9073: si la trampa de la sala sigue armada, el cofre hiere */
{
    Pers *r = &p->pers[pers];
    int t = ind_mas43(p, idx);
    p->ind[idx] = 0xFF;
    if (oro_antes) oro(p, pers, oro_antes);
    if (t != 0xFF) {
        vent(o, TX_VENT_BD30);
        if (r->cuerpo == 1) { muere_o_pierde(p, pers, o); return; }
        r->cuerpo--;
    }
    if (oro_antes) msg(o, MSG_ORO_PANEL, pers, 0);
    vent(o, tx_ok);
}

/* Salas especiales de Return of the Witch Lord (tabla 0x8D73 de la ampliacion, rutinas 0x8D89-0x8E94): en las misiones de cualquier aventura son los valores 19 a 29 del
   indicador, para que convivan con los 1 a 18 de Hero Quest. n = 1 a 11 es el numero de la tabla de la ampliacion. */
static void especial_wl(Partida *p, int pers, int idx, int n, Salida *o)
{
    static const u16 cofre[11] = { 200, 300, 100, 0, 0, 100, 400, 350, 250, 500, 0 };
    static const int ventana[11] = { TX_VENT_B6EF, TX_VENT_B73F, TX_VENT_B805, TX_VENT_B86E, TX_VENT_B8B4, TX_VENT_B8EB, TX_VENT_B78F, TX_VENT_B767, TX_VENT_B717,
                                     TX_VENT_B7B9, TX_VENT_B842 };
    Pers *r = &p->pers[pers];
    switch (n) {
    case 4:                                                      /* 0x8E1A: cofre con trampa explosiva: se desarma, pero la ventana ya hiere */
        p->ind[idx] = 0xFF;
        if (idx + 43 < N_IND) p->ind[idx + 43] = 0xFF;
        vent(o, ventana[3]);
        muere_o_pierde(p, pers, o);
        break;
    case 5:                                                      /* 0x8E56: indicador de trampa: se desactiva y el cofre de la sala queda usado */
        p->ind[idx] = 0xFF;
        if (idx >= 43) p->ind[idx - 43] = 0xFF;
        vent(o, ventana[4]);
        break;
    case 6:                                                      /* 0x8E68: la llave de Agrain (no se marca hasta la cuarta) */
        vent(o, ventana[5]);
        oro(p, pers, 100);
        msg(o, MSG_ORO_PANEL, pers, 0);
        r->armadura |= 0x40;
        if (++p->v.cont_llave == 4) p->ind[idx] = 0xFF;
        break;
    case 11:                                                     /* 0x8DE4: dos pociones curativas */
        p->ind[idx] = 0xFF;
        r->efectos |= 12;
        vent(o, ventana[10]);
        break;
    default:                                                     /* 0x8D89-0x8DCA: cofres de monedas; el 3 (0x8DD7) lleva ademas una pocion (0x8DFE) */
        p->ind[idx] = 0xFF;
        if (n == 3) { if (r->efectos & 4) r->efectos |= 8; else r->efectos |= 4; }
        con_oro(p, pers, o, cofre[n - 1], ventana[n - 1]);
        break;
    }
}

void mision_especial(Partida *p, int pers, int idx, int valor, Salida *o)
{
    Pers *r = &p->pers[pers];
    int k, c;
    switch (valor) {
    case 1:  p->ind[idx] = 0xFF; vent(o, TX_VENT_BD76); break;                                  /* 0x9005 */
    case 2:  esp_trampa(p, pers, idx, o, 0, TX_VENT_BCA3); break;                                /* 0x9010 */
    case 3:                                                                                      /* 0x904A: monedas y una pocion de curacion */
        p->ind[idx] = 0xFF;
        if (r->efectos & 4) r->efectos |= 8; else r->efectos |= 4;
        con_oro(p, pers, o, 30, TX_VENT_BC14);
        break;
    case 4:  p->ind[idx] = 0xFF; esp_trampa(p, pers, idx, o, 100, TX_VENT_BC51); break;          /* 0x9073 */
    case 5:                                                                                      /* 0x90BF */
        if (pers == 3) { vent(o, TX_VENT_BDD1); break; }
        p->ind[idx] = 0xFF; r->armas |= 0x40; vent(o, TX_VENT_BDAD);
        break;
    case 6:                                                                                      /* 0x903D */
        p->ind[idx] = 0xFF;
        if (r->efectos & 4) r->efectos |= 8; else r->efectos |= 4;
        con_oro(p, pers, o, 50, TX_VENT_BCB9);
        break;
    case 7:                                                                                      /* 0x90E0: un tesoro que se reparte */
        if (r->armadura & 0x40) break;
        k = p->v.cont_tesoro;
        if (k == 2) p->ind[idx] = 0xFF;
        p->v.cont_tesoro = (u8)(k + 1);
        {
            static const u8 celda[3][2] = { { 0x13, 0x0A }, { 0x11, 0x0A }, { 0x11, 0x0C } };    /* (columna, fila) */
            c = celda[k % 3][1] * MAP_W + celda[k % 3][0];
            p->capa_c[c] = 0;
        }
        r->armadura |= 0x40;
        vent(o, TX_VENT_BE93);
        break;
    case 8:                                                                                      /* 0x9120 */
        p->ind[idx] = 0xFF;
        vent(o, TX_VENT_BF5F);
        p->capa_c[98] = 0; p->capa_c[124] = 0x41; p->capa_b[99] = 0x1C;
        accion_revelar(p, pers, 99);
        break;
    case 9:  p->ind[idx] = 0xFF; r->objetos |= 4; vent(o, TX_VENT_BFE2); break;                  /* 0x9142 */
    case 10: p->ind[idx] = 0xFF; r->objetos |= 1; vent(o, TX_VENT_C002); break;                  /* 0x9155 */
    case 11:                                                                                     /* 0x9168 */
        vent(o, TX_VENT_C041);
        p->ind[idx] = 0xFF;
        con_oro(p, pers, o, 200, TX_VENT_C01F);
        break;
    case 12: p->ind[idx] = 0xFF; r->objetos |= 2; con_oro(p, pers, o, 150, TX_VENT_BCF6); break; /* 0x9181 */
    case 13: p->ind[idx] = 0xFF; con_oro(p, pers, o, 200, TX_VENT_BC79); break;                  /* 0x9197 */
    case 14:                                                                                     /* 0x91A5 */
        if (pers == 3) { vent(o, TX_VENT_BE1B); break; }
        p->ind[idx] = 0xFF; r->armadura |= 1; vent(o, TX_VENT_BDF5);
        break;
    case 15: p->ind[idx] = 0xFF; vent(o, TX_VENT_BE41); break;                                   /* 0x91C6 */
    case 16:                                                                                     /* 0x91D1: un monstruo aparece y ataca */
        p->ind[idx] = 0xFF;
        p->mons[1].cuerpo |= 1;
        p->v.atacante = (u8)pers;
        vent(o, TX_VENT_BE6B);
        p->v.redibujar = 1;
        if (accion_ataque) accion_ataque(p, pers, 2, o);
        break;
    case 17: p->ind[idx] = 0xFF; r->objetos |= 0x10; vent(o, TX_VENT_BF99); break;              /* 0x91E4 */
    case 18: p->ind[idx] = 0xFF; con_oro(p, pers, o, 200, TX_VENT_C01F); break;                  /* 0x9173 */
    default:
        if (valor >= ESP_WL_1 && valor <= ESP_WL_11) especial_wl(p, pers, idx, valor - ESP_WL_1 + 1, o);
        break;
    }
}

/* ---------------------------------------------------------------- eventos al entrar en una sala (0x70A1) */
void mision_evento(Partida *p, int pers, Salida *o)
{
    Pers *r = &p->pers[pers];
    int e;
    if (p->ev & EV_SIN_EQUIPO) {                                 /* 0x7105: el heroe recupera su equipo */
        if (!p->v.evento_mision) return;
        p->v.evento_mision = 0;
        e = 1 << pers;
        if (p->v.mask_heroes & e) return;
        p->v.mask_heroes |= (u8)e;
        if (pers < 4) pers_restaurar_equipo(p, pers);
        vent(o, TX_VENT_C0BD);
    } else if (p->ev & EV_CASTILLO) {                            /* 0x70C8: cargar con 5000 monedas */
        if (!p->v.evento_mision) return;
        p->v.evento_mision = 0;
        if ((r->armadura & 0x20) || (r->armadura & 0x10)) return;
        if (pregunta(o, TX_VENT_B0F1)) r->armadura |= 0x20;
        r->armadura |= 0x10;
    } else if (p->ev & EV_CIERRE) {                              /* 0x70B0: la sala se cierra */
        if (!p->v.evento_mision || p->v.flag_b4) return;
        p->v.flag_b4 = 1;
        vent(o, TX_VENT_C100);
    }
}

/* ---------------------------------------------------------------- salida por la escalera (0x8EAF) */
static void salida_oro100(Partida *p, int pers, Salida *o)       /* 0x8EC4 */
{
    oro(p, pers, 100);
    msg(o, MSG_ORO_PANEL, pers, 0);
    vent(o, TX_VENT_BBB0);
}

void mision_salida(Partida *p, int pers, Salida *o)
{
    static const u16 reparto[5] = { 0, 200, 100, 66, 50 };
    Pers *r = &p->pers[pers];
    int i, n;
    if ((p->ev & EV_SALE_PRIMERO) && p->v.cont_evento++ == 0)    /* 0x8EBB: solo el primero que sale cobra */
        salida_oro100(p, pers, o);
    if (p->ev & EV_SALE_COBRA) salida_oro100(p, pers, o);        /* 0x8EC4 */
    if ((p->ev & EV_RAGNAR) && pers == 4) {                      /* 0x8ED4: sale Ragnar */
        if (pregunta(o, TX_VENT_B0B6)) {                         /* dividir la recompensa entre los que se salvan */
            n = 0;
            for (i = 0; i < 4; i++) if (p->pers[i].estado || (p->salidos & (1 << i))) n++;
            for (i = 3; i >= 0; i--) if (p->pers[i].estado || (p->salidos & (1 << i))) oro(p, i, reparto[n]);
        } else oro(p, p->v.heroe_evento, 200);
    }
    if ((p->ev & EV_ORO_MAGNUS) && (r->armadura & 0x40)) {       /* 0x8F5D: el oro de Magnus */
        oro(p, pers, 200);
        n = pregunta(o, TX_VENT_B058);
        msg(o, MSG_ORO_PANEL, pers, 0);
        vent(o, n ? TX_VENT_BF16 : TX_VENT_BEDC);
        if (p->v.cont_tesoro != 3 && pregunta(o, TX_VENT_B084)) {   /* vuelve a la mazmorra por mas oro */
            p->salidos &= (u8)~(1 << pers);
            r->estado = 0xFF;
            p->v.turno_acabado = 0;
            r->col = 3; r->fila = 0x0B;
            r->armadura &= (u8)~0x40;
            p->v.redibujar = 1;
        }
    }
    if ((p->ev & EV_CASTILLO) && p->v.muertos == 0x0F) vent(o, TX_VENT_BBDE);   /* 0x8F4E: salen todos */
}
