/* HeroQuest QL - combate y bolsa (ver combate.h y docs/ZX_COMBATE.md). Cada funcion lleva la direccion de la rutina del original. */
#include <string.h>
#include "combate.h"

static void msg(Salida *o, int id, int a, int b) { accion_mensaje(o, id, a, b); }

static Mons *monstruo(Partida *p, int num) { return &p->mons[num - 1]; }

int combate_cuenta_calaveras(int dados)                          /* 0x7B1C: cuantas calaveras salen en los dados de combate */
{
    int e = 0;
    while (dados-- > 0) if (dado_combate() == 0) e++;
    return e;
}

/* ---------------------------------------------------------------- dados */
void combate_datos_monstruo(Partida *p)                          /* 0x751D */
{
    const int *t = vista_monstruo(monstruo(p, p->v.objetivo)->estad);
    p->v.ataque_base = (u8)t[1];
    p->v.defensa_base = (u8)t[2];
}

void combate_preparar(Partida *p, int pers)                      /* 0x9B65 */
{
    const Pers *r = &p->pers[pers];
    int k, c = r->r11;
    p->v.dados_ataque = r->ataque;
    for (k = 0; k < 7 && !((c >> k) & 1); k++) { }               /* el primer arma elegida (bit 0 = baston ... bit 6 = lanza) */
    if (k < 7) {
        p->v.bono_arma = (u8)vista_arma(6 - k);
        if (p->v.bono_arma & 3) p->v.dados_ataque = (u8)((p->v.bono_arma & 3) + 1);
        return;
    }
    if (r->r12 & 8) { p->v.bono_arma = 0; p->v.dados_ataque = 2; return; }       /* la vara de orco */
    if (!(r->r12 & 0x10)) return;
    p->v.bono_arma = 0;                                          /* la espada espiritual: 4 dados contra los muertos vivientes */
    {
        int tipo = monstruo(p, p->v.objetivo)->tipo;
        p->v.dados_ataque = (u8)((tipo == 4 || tipo == 5 || tipo == 6) ? 4 : 3);
    }
}

void combate_bonos_ataque(Partida *p, int pers)                  /* 0x7559 */
{
    Pers *r = &p->pers[pers];
    int a = p->v.dados_ataque;
    if (((monstruo(p, p->v.objetivo)->estad + 1) & 0xFE) == 0x0C && !(r->r12 & 0x10)) a = 0;   /* los jefes solo reciben dano de la espada espiritual */
    else {
        if (r->t17 & 1) { a += 2; r->t17 &= (u8)~1; r->efectos &= (u8)~1; }
        if (r->t18 & 0x10) { r->t18 &= (u8)~0x10; a += 2; }
    }
    p->v.dados_ataque = (u8)a;
}

void combate_dados_defensa(Partida *p, int pers)                 /* 0x7596 */
{
    Pers *r = &p->pers[pers];
    int a = 0;
    if (!(r->armadura & 0x20)) {
        a = r->defensa;
        if (r->r12 & 1) a = 4;
        else if (r->r13 & 8) a = 4;
        else if (r->r13 & 4) a = 3;
        if (r->r13 & 1) a++;
        if (r->r13 & 2) a++;
        if (r->t17 & 2) { a += 2; r->t17 &= (u8)~2; r->efectos &= (u8)~2; }
        if (r->t18 & 0x40) { r->t18 &= (u8)~0x40; a += 2; }
    }
    p->v.dados_defensa = (u8)a;
}

/* ---------------------------------------------------------------- linea de vista */
void (*combate_punto)(int x, int y);                           /* gancho: la pantalla muestra cada punto de la linea (el original volcaba el buffer tras cada uno) */

static int pixel(u8 *plano, int x, int y, int quedan)          /* 0x7F2A: 1 si choca; si no, enciende el punto */
{
    int idx = 0x103 + (y & 255) * 32 + ((x & 255) >> 3), m = 0x80 >> (x & 7);
    if (idx >= VISTA_ALTO * 32) return 0;
    if ((plano[idx] & m) && quedan >= 4) return 1;
    plano[idx] = (u8)(plano[idx] | m);
    if (combate_punto) combate_punto(x & 255, y & 255);          /* 0x7F68: CALL 0x8D03 */
    return 0;
}

int combate_linea(const Partida *p, u8 *plano, int ca, int fa, int ct, int ft)       /* 0x7E7E */
{
    int xa = (ca * 8 + 4) & 255, ya = (fa * 8 + 4) & 255, xt = (ct * 8 + 4) & 255, yt = (ft * 8 + 4) & 255;
    int x = xa, y = ya, acc = 0, hx, hy, dx, dy, b, c, e;
    (void)p;
    if (xa == xt) { hx = 0; dx = 0; } else if (xa > xt) { hx = -1; dx = xa - xt; } else { hx = 1; dx = xt - xa; }
    if (ya == yt) { hy = 0; dy = 0; } else if (ya > yt) { hy = -1; dy = ya - yt; } else { hy = 1; dy = yt - ya; }
    if (dy >= dx) {                                              /* mas vertical: un paso por cada punto de y */
        c = dx; b = dy; e = dy;
        do {
            acc = (acc + c) & 255;
            if (acc >= e) { acc -= e; x = (x + hx) & 255; }
            y = (y + hy) & 255;
            if (pixel(plano, x, y, b)) return 1;
        } while ((b = (b - 1) & 255) != 0);
    } else {                                                     /* mas horizontal */
        e = dy; c = dx; b = dx;
        do {
            acc = (acc + e) & 255;
            if (acc >= c) { acc -= c; y = (y + hy) & 255; }
            x = (x + hx) & 255;
            if (pixel(plano, x, y, b)) return 1;
        } while ((b = (b - 1) & 255) != 0);
    }
    return 0;
}

static void arma_se_pierde(Partida *p, int pers)                 /* 0x74E5: la lanza y la hacha se pierden al lanzarlas */
{
    Pers *r = &p->pers[pers];
    if (!(p->v.bono_arma & 0x20)) return;
    r->armas = (u8)(r->r11 ^ r->armas);
    r->r11 = 0;
}

int combate_alcance(Partida *p, int pers, u8 *plano)             /* 0x7462 y 0x74FC */
{
    Pers *r = &p->pers[pers];
    const Mons *m = monstruo(p, p->v.objetivo);
    int i, dx, dy, vis, c, b;
    u8 antes = p->v.marcas;
    p->v.marcas = 0;
    p->v.vis = 0;
    {                                                            /* 0x750B y 0x7E74: el plano con marcas y la linea de vista desde el heroe */
        static u8 atr[MAP_N];
        p->v.marcas = 1;
        vista_plano_buffer(p, plano, atr);
        p->v.marcas = 0;
    }
    p->v.resultado = (u8)combate_linea(p, plano, r->col, r->fila, m->col, m->fila);
    p->v.vis = (u8)(p->v.resultado ^ 1);
    p->v.marcas = antes;
    if (!p->v.vis) { arma_se_pierde(p, pers); return ALC_NO_VE; }
    for (i = 0; i < 8; i++) {                                    /* las ocho casillas de al lado */
        vista_contacto(i, &dx, &dy);
        if (((r->col + dx) & 255) >= MAP_W || ((r->col + dx) & 255) != m->col) continue;
        if (((r->fila + dy) & 255) >= 0x13 || ((r->fila + dy) & 255) != m->fila) continue;
        p->v.vis |= (u8)(i < 4 ? 2 : 4);
        break;
    }
    vis = p->v.vis;
    c = p->v.bono_arma & 0xD0;
    b = vis;
    if (b & 2) return (r->r11 & 0x10) ? ALC_BALLESTA : ALC_OK;
    if ((c & 0x80) && (b & 4)) return (r->r11 & 0x10) ? ALC_BALLESTA : ALC_OK;
    if (!(c & 0x50)) return ALC_LEJOS;
    arma_se_pierde(p, pers);
    return (b & 1) ? ALC_OK : ALC_NO_VE;
}

/* ---------------------------------------------------------------- muerte de un monstruo y recompensa */
void monstruo_muere(Partida *p, int num, int pers, Salida *o)    /* 0x9B31 */
{
    Mons *m = monstruo(p, num);
    const int *t = vista_monstruo(m->estad);
    int i;
    m->b7 &= 0x0F;
    m->estado = 0;
    m->cuerpo &= (u8)~1;
    p->capa_c[m->casilla] = 0;
    if (t[3]) {
        if (t[4] == 2) { for (i = 0; i < 4; i++) p->pers[i].oro = (u16)(p->pers[i].oro + t[3]); }
        else p->pers[pers].oro = (u16)(p->pers[pers].oro + t[3]);
    }
    if (t[5]) p->pers[pers].objetos |= (u8)(1 << t[5]);
    if (t[6]) msg(o, t[6] == 1 ? MSG_BANE_ORC : MSG_ESTRELLA, 0, 0);
}

/* ---------------------------------------------------------------- tiradas */
void combate_ataque(Partida *p, int pers, Salida *o)             /* 0x99A2: el heroe ataca al monstruo objetivo */
{
    int e = 0, b;
    p->v.golpes = 0; p->v.escudos = 0;
    if (p->v.dados_ataque) { e = combate_cuenta_calaveras(p->v.dados_ataque); }
    p->v.golpes = (u8)e; p->v.escudos = 0;
    b = p->v.defensa_base ? p->v.defensa_base : 256;
    e = 0;
    while (b-- > 0) if (dado_combate() == 2) e++;                /* los monstruos se defienden con escudos negros */
    p->v.escudos = (u8)e;
    if (p->v.golpes > p->v.escudos) monstruo_muere(p, p->v.objetivo, pers, o);      /* 0x9B25 */
    combate_datos_monstruo(p);
}

int combate_defensa(Partida *p, int heroe, int monstruo_num, Salida *o)    /* 0x9921 (la parte logica) y 0x996B */
{
    Pers *r = &p->pers[heroe];
    int e = 0, b, dano = 0;
    u8 activo = p->v.pers_actual;
    p->v.pers_actual = (u8)heroe;                                /* 0x9921 pone al defensor como personaje activo mientras dura */
    p->v.objetivo = (u8)monstruo_num;
    combate_dados_defensa(p, heroe);
    combate_datos_monstruo(p);
    b = p->v.dados_defensa;
    if (b) { int k = b; while (k-- > 0) if (dado_combate() == 1) e++; }       /* los heroes se defienden con escudos blancos */
    p->v.escudos = (u8)e;
    p->v.golpes = (u8)combate_cuenta_calaveras(p->v.ataque_base);
    combate_dados_defensa(p, heroe);                             /* 0x7596 otra vez, como el original */
    p->v.pers_muerto = (u8)heroe;                                /* 0x9B04 */
    if (p->v.golpes > p->v.escudos) {
        dano = p->v.golpes - p->v.escudos;
        if (r->cuerpo > dano) r->cuerpo = (u8)(r->cuerpo - dano);
        else { r->cuerpo = 0; heroe_muere(p, heroe); msg(o, MSG_MUERTE, heroe, 0); }
    }
    p->v.pers_actual = activo;
    return dano;
}

/* ---------------------------------------------------------------- el panel del personaje */
int panel_item(int mascara, int fila)                            /* 0x7F6F */
{
    int e = 1, c = mascara & 255;
    for (;;) {
        if (c & 1) { if (fila == 0) return e; fila--; }
        c >>= 1; e <<= 1;
        if (!c) return 0;
    }
}

int panel_item16(int mascara, int fila)                          /* 0x7F8D */
{
    long e = 1, c = mascara & 0xFFFF;
    for (;;) {
        if (c & 1) { if (fila == 0) return (int)e; fila--; }
        c >>= 1; e <<= 1;
        if (!c) return 0;
    }
}

int panel_numero_hechizo(int mascara)                            /* 0x96EC */
{
    int b, a;
    for (b = 15; b > 0; b--) { if (mascara & 1) break; mascara >>= 1; }
    if (b == 0) return 0;
    a = 0x10 - b;
    return a < 7 ? a : a - 2;
}

int panel_mascara_hechizo(int hechizo)                           /* 0x96AC */
{
    if (hechizo == 0) return 0;
    return hechizo < 7 ? 1 << (hechizo - 1) : 0x100 << (hechizo - 7);
}

void panel_efecto(Partida *p, int pers, int fila, Salida *o)     /* 0x7D6F: pociones */
{
    Pers *r = &p->pers[pers];
    int e = panel_item(r->efectos, fila);
    if (!e) return;
    if (e & 0x10) {                                              /* agua santa: acaba con los muertos vivientes de la sala */
        int t, c, sala, n = 0, origen = p->v.origen_y * MAP_W + p->v.origen_x;
        sala = p->capa_a[origen] & 0x3F;
        for (t = 0; t < 30; t++) {
            c = origen + vista_ventana_off(t);
            if (c >= MAP_P || p->capa_a[c] != sala) continue;
            {
                int v = p->capa_c[c];
                if (v && v < 0x30) {
                    int tipo = monstruo(p, v)->tipo;
                    if (tipo == 4 || tipo == 5 || tipo == 6) { n++; monstruo_muere(p, v, pers, o); }
                }
            }
        }
        msg(o, MSG_AGUA_SANTA, n, 0);
        r->t17 &= (u8)~0x10; r->efectos &= (u8)~0x10;
        return;
    }
    if (e & 0x0C) {                                              /* curacion: 4 puntos de cuerpo sin pasar del maximo */
        int a = r->cuerpo + 4;
        if (a >= cuerpo_max(pers)) a = cuerpo_max(pers);
        r->cuerpo = (u8)a;
        if (e & 4) { r->efectos &= (u8)~4; r->t17 &= (u8)~4; }
        else { r->efectos &= (u8)~8; r->t17 &= (u8)~8; }
        return;
    }
    r->t17 |= (u8)e;                                             /* fuerza, resistencia, velocidad y heroico quedan activas */
}

void panel_objeto(Partida *p, int pers, int fila)                /* 0x7CE2 */
{
    Pers *r = &p->pers[pers];
    int e = panel_item(r->objetos, fila);
    if (!e || (e & 6)) return;
    if (e & 1) { r->r12 = (u8)(r->r12 ^ e); r->r13 &= 3; }       /* la armadura de Borin */
    else { r->r12 = (u8)((r->r12 & 7) | e); r->r11 = 0; }        /* la vara de orco o la espada espiritual sustituyen al arma */
}

void panel_arma(Partida *p, int pers, int fila)                  /* 0x7C45 */
{
    Pers *r = &p->pers[pers];
    int e = panel_item(r->armas, fila);
    if (!e) return;
    r->r11 = (u8)e;
    r->r12 &= 7;
    if (e & 8) r->r13 &= (u8)~1;                                 /* la gran hacha se lleva a dos manos: sin escudo */
}

void panel_arma_sola(Partida *p, int pers, int fila)             /* 0x7CC7 */
{
    Pers *r = &p->pers[pers];
    int e = panel_item(r->armas, fila);
    if (!e) return;
    r->r11 = (u8)e;
    r->r12 &= 7;
}

void panel_armadura(Partida *p, int pers, int fila)              /* 0x7C6B */
{
    Pers *r = &p->pers[pers];
    int e = panel_item(r->armadura & 0x0F, fila);
    if (!e) return;
    if (e & 8) { r->r13 = (u8)((r->r13 & 0x0B) ^ e); r->r12 &= 0x1E; return; }       /* armadura completa */
    if (e & 4) { r->r13 = (u8)((r->r13 & 7) ^ e); r->r12 &= 0x1E; return; }          /* cota de malla */
    if ((e & 1) && (r->r11 & 8)) return;                         /* el escudo no va con la gran hacha */
    r->r13 = (u8)(r->r13 ^ e);
}

int panel_hechizo(Partida *p, int pers, int fila)                /* 0x7D1F */
{
    Pers *r = &p->pers[pers];
    int a, m, n;
    if (r->objetos & 2) {                                        /* con la vara de recuerdo se puede lanzar un hechizo mas tras actuar */
        if (p->v.estado_turno & 2) {
            if (p->v.hab_registrada & 2) return 0;
            p->v.hab_registrada |= 2;
            a = p->v.hab_registrada;                             /* fallo del original: sigue con el valor de 0xA9A4 en vez del de 0xA98B (HALLAZGOS.md) */
        } else a = p->v.estado_turno;
    } else {
        if (p->v.estado_turno & 2) return 0;
        a = p->v.estado_turno;
    }
    if (a & 1) { a |= 4; p->v.pm = 0; }
    a |= 2;
    p->v.estado_turno = (u8)a;
    m = panel_item16(r->hechizos[0] | (r->hechizos[1] << 8), fila);
    if (!m) return 0;
    n = panel_numero_hechizo(m);
    if (!n) return 0;
    r->hechizo = (u8)n;
    p->v.zona = 1; p->v.elegido = 1;
    return 1;
}

/* ---------------------------------------------------------------- lanzar un hechizo */
static void efecto_cuerpo(Partida *p, int tipo, int heroe, int num, int puntos, Salida *o)     /* dano de un hechizo a un heroe o muerte de un monstruo */
{
    if (tipo == BLANCO_MONSTRUO) { monstruo_muere(p, num, p->v.pers_actual, o); msg(o, MSG_HECHIZO_1, 0, 0); return; }
    {
        Pers *r = &p->pers[heroe];
        if (r->cuerpo <= puntos) { r->cuerpo = 0; heroe_muere(p, heroe); msg(o, MSG_MUERTE, heroe, 0); return; }
        r->cuerpo = (u8)(r->cuerpo - puntos);
        msg(o, puntos == 1 ? MSG_HECHIZO_1 : MSG_HECHIZO_N, puntos, 0);
    }
}

int hechizo_inmune(const Partida *p)                             /* 0x7925 */
{
    return ((monstruo((Partida *)p, p->v.objetivo)->estad + 1) & 0xFE) == 0x0C;
}

void panel_caja(Partida *p, int pers, Salida *o)                 /* 0x9FF7 */
{
    Pers *r = &p->pers[pers];
    int sala, k, dx, dy, c, f, cell;
    if (!(r->armadura & 0x80)) return;
    sala = vista_sala_de(p, r->col, r->fila);
    for (k = 0; k < 5; k++) {                                    /* la casilla del heroe y las cuatro de al lado (0x81BE) */
        if (k == 0) { dx = 0; dy = 0; } else vista_contacto(k - 1, &dx, &dy);
        c = r->col + dx; f = r->fila + dy;
        if (c < 0 || c >= MAP_W || f < 0 || f >= 0x13) continue;
        cell = f * MAP_W + c;
        if (vista_sala_de(p, c, f) != sala) continue;
        if (p->capa_c[cell] < 0x31 || p->capa_c[cell] >= 0x37) continue;
        p->capa_c[cell] = 0;                                     /* la trampa desaparece */
        if (pers != 1) dado_combate();                           /* 0xA026: el original mira A, que es el R de la tirada, y no la cara que sale (HALLAZGOS.md) */
        if (pers == 1 || dado_r() != 0) { accion_mensaje(o, MSG_TRAMPA_OFF, 0, 0); return; }
        accion_mensaje(o, MSG_TRAMPA_ON, 0, 0);
        if (r->cuerpo > 1) r->cuerpo--;
        else { r->cuerpo = 0; heroe_muere(p, pers); accion_mensaje(o, MSG_MUERTE, pers, 0); }
        return;
    }
}

void panel_soltar_oro(Partida *p, int pers, Salida *o)                  /* 0x7E61 */
{
    Pers *r = &p->pers[pers];
    if (!(r->armadura & 0x20)) return;
    r->armadura &= (u8)~0x20;
    accion_mensaje(o, MSG_ORO_MINA, 0, 0);
}

void hechizo_lanzar(Partida *p, int pers, u8 *plano, Salida *o)  /* 0x7943 */
{
    Pers *r = &p->pers[pers];
    int h = r->hechizo, tipo = p->v.pers_muerto, heroe = tipo < 5 ? tipo : 0, num = p->v.objetivo, k, e, m;
    Pers *t = &p->pers[heroe];
    Mons *mo = tipo == BLANCO_MONSTRUO ? monstruo(p, num) : 0;
    int m16 = panel_mascara_hechizo(h);
    if (tipo == BLANCO_MONSTRUO && hechizo_inmune(p)) {          /* 0x7925: no se gasta */
        p->v.arma_alcance = 0; r->hechizo = 0; msg(o, MSG_HECHIZO_NO, 0, 0); return;
    }
    p->v.arma_alcance = 0;
    r->hechizos[0] = (u8)(r->hechizos[0] ^ (m16 & 255));         /* el hechizo se gasta */
    r->hechizos[1] = (u8)(r->hechizos[1] ^ (m16 >> 8));
    r->hechizo = 0;
    if (h != 6 && h != 9 && tipo != pers) {                      /* alcance: la misma sala o una linea de vista libre */
        int sala = vista_sala_de(p, r->col, r->fila), ok = 0;
        if (sala >= 0x15 && (p->capa_a[p->v.casilla_cursor] & 0x3F) == sala) ok = 1;
        if (!ok) {
            static u8 atr[MAP_N];
            int c2, f2;
            p->v.marcas = 1; vista_plano_buffer(p, plano, atr); p->v.marcas = 0;
            if (tipo < 5) { c2 = t->col; f2 = t->fila; } else { c2 = mo->col; f2 = mo->fila; }
            p->v.resultado = (u8)combate_linea(p, plano, r->col, r->fila, c2, f2);
            if (p->v.resultado) { msg(o, MSG_HECHIZO_NO, 0, 0); return; }
        }
    }
    switch (h) {
    case 2: case 12: {                                           /* curar 4 puntos */
        int a = t->cuerpo + 4;
        t->cuerpo = (u8)(a >= cuerpo_max(heroe) ? cuerpo_max(heroe) : a);
        break;
    }
    case 5:  t->t18 |= 8; break;                                 /* huracan: movimiento doble */
    case 7:  t->t18 |= 0x10; break;                              /* valor: dos dados de ataque mas */
    case 11: t->t18 |= 0x40; break;                              /* piel de roca */
    case 1:  t->t18 |= 1; break;                                 /* velo de niebla */
    case 4:                                                      /* tempestad */
        if (tipo == BLANCO_MONSTRUO) mo->cuerpo |= 2; else t->t18 |= 4;
        break;
    case 3:                                                      /* dormir: falla con un escudo blanco */
        if (dado_combate() == 1) { msg(o, MSG_HECHIZO_NO, 0, 0); return; }
        if (tipo == BLANCO_MONSTRUO) mo->cuerpo |= 4; else t->t18 |= 2;
        break;
    case 8:                                                      /* bola de fuego: dos dados, cada escudo blanco la detiene */
        e = 0;
        if (dado_combate() == 1) e++;
        if (dado_combate() == 1) e++;
        e = 2 - e;
        if (!e) { msg(o, MSG_HECHIZO_NO, 0, 0); return; }
        efecto_cuerpo(p, tipo, heroe, num, e, o);
        return;
    case 9:                                                      /* fuego de ira: un dado */
        if (dado_combate() == 1) { msg(o, MSG_HECHIZO_NO, 0, 0); return; }
        efecto_cuerpo(p, tipo, heroe, num, 1, o);
        return;
    case 6:                                                      /* genio: abre la sala de una casilla o ataca con cinco dados */
        if (tipo == BLANCO_CASILLA) { accion_revelar(p, pers, p->v.casilla_cursor); break; }
        e = combate_cuenta_calaveras(5);
        if (!e) { msg(o, MSG_HECHIZO_NO, 0, 0); return; }
        efecto_cuerpo(p, tipo, heroe, num, e, o);
        return;
    case 10:                                                     /* atravesar muros: el hechicero que ya se ha movido lo tiene el turno siguiente */
        if (pers == 3 && (p->v.estado_turno & 1)) r->t18 |= 0x80; else t->t18 |= 0x20;
        break;
    default: break;
    }
    (void)k; (void)m;
    msg(o, MSG_HECHIZO_OK, 0, 0);
}
