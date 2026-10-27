/* HeroQuest QL - las pantallas del ataque y de la bolsa (ver docs/ZX_COMBATE.md): elegir objetivo en el plano, la pantalla de combate con retratos y dados, el
   panel del personaje y el lanzamiento de hechizos. La logica esta en combate.c; aqui se dibuja y se esperan las teclas.
   Rutinas del original: ATACAR (0x7319), 0x7677 y 0x76AE (elegir objetivo), 0x7436 (otro ataque), PANEL_COMBATE (0x9921), 0x99DB, 0x99F8, 0x9A26 y 0x9A4D
   (dados y retratos), PANEL_PERSONAJE (0x95BE) con sus listas (0x9656-0x9703), 0x7BFC (clics del panel), USAR_OBJETO (0x7877). */
#include <stdlib.h>
#include <string.h>
#include "juego_int.h"
#include "game.h"
#include "cursor.h"
#include "combate.h"
#include "monstruo.h"
#include "mision.h"
#include "music.h"
#include "vista.h"
#include "textos_ids.h"

static u8 *pk_retratos;
static u8 plano_los[VISTA_ALTO * 32];                /* estatico: la pila de un programa del QL es pequena */

void ui_combate_iniciar(void)
{
    if (!pk_retratos) pk_retratos = data_load("retratos_bin", 0);
}

void ui_combate_liberar(void)
{
    if (pk_retratos) free(pk_retratos);
    pk_retratos = 0;
}

/* ---------------------------------------------------------------- la pantalla de combate */
static const u8 retrato_heroe[5] = { 0, 1, 2, 3, 2 };                                   /* tabla 0x9ABB: barbaro, enano, elfo, hechicero y Ragnar (el del elfo) */
static const u8 retrato_monstruo[13] = { 11, 4, 5, 6, 7, 10, 8, 4, 4, 4, 7, 7, 7 };       /* tabla 0x9ACA, por tipo (nombre) de monstruo */

static void minifigura(int col, int idx)             /* 0xA3E1 y 0xA3F8: la figura sobre una caja blanca (el original la dibuja en negativo) */
{
    gimg g;
    int top, filas;
    if (!vista_sprite(idx, &g)) return;
    top = 120 - g.h;
    filas = (g.h + 7) / 8;
    gfx_cells_fill(col, UI_FILA(top / 8), 3, filas, C_WHITE);
    gfx_blit_clip(&g, col * 8, VY0 + top);
}

static void banda(int col, const u8 *s, int n, int color)       /* un rotulo de 0x72 ... 0x73 en negativo */
{
    gfx_text(col, UI_FILA(16), s, n, C_BLACK, (u8)color);
}

static void rotulo_punto(int col, int row, int id, int color, int valor)    /* «CUERPO 6» / «MENTE  4» */
{
    int n;
    const u8 *s = tx_str(TX_PORT_COMBATE, id, &n);
    tx_cadena_en(col, UI_FILA(row), 7, s, n, 'L', (u8)color, C_BLACK);
    gfx_char(col + 7, UI_FILA(row), (u8)(0x30 + valor), (u8)color, C_BLACK);
}

static void dados_fila(int col, int cifra, int icono, int color)         /* 0x9A15: 10 casillas con tantos iconos como puntos */
{
    int i;
    for (i = 0; i < 10; i++) gfx_char(col + i, UI_FILA(20), (u8)(i < cifra ? icono : 0x5F), (u8)color, C_BLACK);
}

/* heroe 0 a 4; tipo = tipo (nombre) del monstruo; b7 = sus dos cifras; golpes y escudos de la tirada; ataca_heroe = 1 si el heroe es el que ataca */
static void pantalla_combate(int heroe, int cuerpo, int mente, int tipo, int b7, int golpes, int escudos, int ataca_heroe)
{
    gimg g;
    int n, x, y, attr, al, len, retrato, vence;
    const u8 *s;
    cursor_erase();
    gfx_clear();
    if (tx_guion_linea(TX_GUION_A484 + heroe, 0, &x, &y, &attr, &al, &s, &len)) banda(3, s, len, attr & 7);        /* 0x9C62 */
    minifigura(6, 20 + 12 * (heroe == 4 ? 2 : heroe));                                                                /* 0xA3E1 (tabla 0xA3F3) */
    s = tx_str(TX_NOMBRES_MONSTRUO, tipo, &n);                                                                         /* 0x764A */
    gfx_char(20, UI_FILA(16), 0x72, C_BLACK, C_YELLOW);
    tx_cadena_en(21, UI_FILA(16), 9, s, n, 'C', C_BLACK, C_YELLOW);
    gfx_char(30, UI_FILA(16), 0x73, C_BLACK, C_YELLOW);
    minifigura(23, tipo);
    rotulo_punto(4, 18, 0, C_RED, cuerpo);                                                                             /* 0x75ED */
    rotulo_punto(4, 19, 1, C_RED, mente);
    rotulo_punto(22, 18, 0, C_GREEN, (b7 >> 4) & 15);                                                                  /* 0x751D */
    rotulo_punto(22, 19, 1, C_GREEN, b7 & 15);
    if (ataca_heroe) { dados_fila(4, golpes, 0x70, C_CYAN); dados_fila(22, escudos, 0x71, C_MAGENTA); }               /* 0x99DB */
    else { dados_fila(22, golpes, 0x70, C_MAGENTA); dados_fila(4, escudos, 0x71, C_CYAN); }                           /* 0x99F8 */
    vence = golpes > escudos;                                                                                          /* 0x9A26 y 0x9A4D */
    if (!vence) retrato = 9;
    else retrato = ataca_heroe ? retrato_monstruo[tipo % 13] : retrato_heroe[heroe];
    if (pk_retratos && pack_get(pk_retratos, retrato, &g)) gfx_blit(&g, 64, VY0);
    mus_efecto(vence ? 3 : 2);                                                                                         /* 0x9AA3: golpe o fallo */
    if (ataca_heroe) tx_guion(vence ? TX_GUION_B89F : TX_GUION_B8BA, -1, 0, 0);
    else tx_guion(vence ? TX_GUION_B866 : TX_GUION_B87C, -1, 0, 0);
}

void ui_defensa(int heroe, int monstruo)             /* PANEL_COMBATE (0x9921) */
{
    Partida *p = &g_part;
    Pers *r = &p->pers[heroe];
    Mons *m = &p->mons[monstruo - 1];
    Salida o;
    int cuerpo = r->cuerpo, mente = r->mente, tipo = m->tipo, b7 = m->b7;
    memset(&o, 0, sizeof(o));
    combate_defensa(p, heroe, monstruo, &o);
    pantalla_combate(heroe, cuerpo, mente, tipo, b7, p->v.golpes, p->v.escudos, 0);
    jv_boton(150);                                   /* 0x9D61 */
    jv_mensajes(&o);                                 /* la muerte del heroe (sin redibujar la vista: sigue el turno de Morcar) */
}

/* ---------------------------------------------------------------- ventana de si o no */
static int si_no(int id)                             /* 0x7436 y 0x8D5B: 1 = SI (zona 0x745A), 0 = NO */
{
    const u8 *z;
    int nz, i, col, fila;
    u16 antes;
    int r, mov, primera = 1;
    tx_ventana(id);
    nz = tx_zonas(id, &z);
    jv_soltar();
    antes = plat_keys();
    for (;;) {
        r = jv_entrada(&antes, &mov);
        if (mov || primera || !cursor_dibujado()) { cursor_erase(); cursor_draw(); primera = 0; }
        if (r == 2) { cursor_erase(); return 0; }
        if (r == 1) {
            col = (cur_x - 12) >> 3; fila = (cur_y - 2) >> 3;
            for (i = 0; i < nz; i++) {
                const u8 *e = z + 6 * i;
                if (col >= e[0] && col < e[1] && fila >= e[2] && fila < e[3]) {
                    cursor_erase();
                    return (e[4] | (e[5] << 8)) == 0x745A;
                }
            }
        }
    }
}

/* ---------------------------------------------------------------- el panel del personaje */
static int lista(int id, int campo, int n, int mascara, int sel, int col, int row)    /* 0x9703: una linea por cada bit a uno; las elegidas, en negativo */
{
    int k, len;
    const u8 *s;
    for (k = 0; k < n; k++, sel >>= 1)
        if (mascara & (1 << k)) {
            s = tx_str(id, k, &len);
            tx_cadena_en(col, UI_FILA(row), campo, s, len, 'L', (u8)((sel & 1) ? C_BLACK : C_WHITE), (u8)((sel & 1) ? C_WHITE : C_BLACK));
            row++;
        }
    return row;
}

static void rotulo(int id, int n)                    /* una linea de un guion con su color */
{
    int x, y, attr, al, len;
    const u8 *s;
    if (!tx_guion_linea(id, n, &x, &y, &attr, &al, &s, &len)) return;
    if (al == 'C') x = 16 - len / 2; else if (al == 'R') x = x - len;
    gfx_text(x, UI_FILA(y), s, len, (u8)(attr & 7), C_BLACK);
}

static void panel_dibujar(int pers)                  /* PANEL_PERSONAJE (0x95BE) */
{
    const Pers *r = &g_part.pers[pers];
    int mascara = vista_panel_mascara(pers), h, sel, row, tinta = ui_ink;
    cursor_erase();
    gfx_clear();
    ui_ink = C_GREEN;
    tx_guion(TX_GUION_B822, -1, 0, 0);                                         /* SALIDA */
    if (r->armadura & 0x40) tx_guion(TX_GUION_B82E, -1, 0, 0);
    if (r->armadura & 0x20) tx_guion(TX_GUION_B83E, -1, 0, 0);
    if (mascara & 1) {                                                          /* pociones (la caja del primer rotulo no esta en textos_bin) */
        tx_caja(0, UI_FILA(0), 13, 9, C_GREEN);
        rotulo(TX_GUION_B78E, 1);
        lista(TX_PANEL_EFECTOS, 11, 7, r->efectos, r->t17, 1, 1);
    }
    if (mascara & 2) {
        tx_guion(TX_GUION_B79E, -1, 0, 0);
        lista(TX_PANEL_OBJETOS, 16, 5, r->objetos, r->r12, 15, 1);
    }
    if (mascara & 4) {
        tx_guion(TX_GUION_B7B8, -1, 0, 0);
        lista(TX_PANEL_ARMAS, 10, 8, r->armas, r->r11, 1, 10);
    }
    if (mascara & 0x40) {
        tx_guion(TX_GUION_B801, -1, 0, 0);
        lista(TX_PANEL_ARMAS, 10, 1, r->armas, r->r11, 1, 10);
    }
    if (mascara & 0x10) {
        tx_guion(TX_GUION_B7DB, -1, 0, 0);
        lista(TX_PANEL_ARMADURAS, 8, 4, r->armadura & 0x0F, r->r13, 15, 13);
    }
    if (mascara & 0x28) {                                                       /* hechizos: la caja pequena o la grande */
        h = panel_mascara_hechizo(r->hechizo);
        tx_guion((mascara & 0x20) ? TX_GUION_B7EE : TX_GUION_B7C8, -1, 0, 0);
        sel = h & 255;
        row = lista(TX_PANEL_HECHIZOS_B, 15, 6, r->hechizos[0], sel, 15, 8);
        lista(TX_PANEL_HECHIZOS_C, 13, 6, r->hechizos[1], h >> 8, 15, row);
    }
    if ((mascara & 0x80) && (r->armadura & 0x80)) tx_guion(TX_GUION_B810, -1, 0, 0);   /* UTILIZA CAJA */
    ui_ink = (u8)tinta;
    cursor_draw();
}

static void panel_clic(int z, int pers, int fila, Salida *o, int *salir)       /* los manejadores de la tabla 0x7FAE */
{
    Partida *p = &g_part;
    switch (z) {
    case 0: panel_efecto(p, pers, fila, o); break;
    case 1: panel_objeto(p, pers, fila); break;
    case 2: panel_arma(p, pers, fila); break;
    case 3: case 5: if (panel_hechizo(p, pers, fila)) *salir = 1; break;
    case 4: panel_armadura(p, pers, fila); break;
    case 6: panel_arma_sola(p, pers, fila); break;
    case 7: if (p->pers[pers].armadura & 0x80) { panel_caja(p, pers, o); *salir = 1; } break;
    case 8: *salir = 1; break;
    case 9: panel_soltar_oro(p, pers, o); break;
    }
}

static int panel(int pers, int modo)                 /* 0x7FF5 y el bucle de 0x7BFC: 1 si se ha elegido un hechizo */
{
    Partida *p = &g_part;
    int mascara = vista_panel_mascara(modo * 4 + pers), z, col, fila, salir = 0, mov, r, ocupado;
    u16 antes;
    Salida o;
    p->v.zona = 0; p->v.elegido = 0;
    panel_dibujar(pers);
    jv_soltar();
    antes = plat_keys();
    while (!salir) {
        r = jv_entrada(&antes, &mov);
        if (mov || !cursor_dibujado()) { cursor_erase(); cursor_draw(); }
        if (r == 2) break;
        if (r != 1) continue;
        col = (cur_x - 12) >> 3; fila = (cur_y - 2) >> 3;
        for (z = 0; z < 10; z++) {
            const int *e = vista_panel_zona(z);
            if (z < 8 && !((mascara >> z) & 1)) continue;
            if (col < e[0] || col >= e[1] || fila < e[2] || fila >= e[3]) continue;
            memset(&o, 0, sizeof(o));
            panel_clic(z, pers, fila - e[2], &o, &salir);
            if (z != 8) mus_efecto(6);                                       /* los manejadores del panel (0x7BA2) */
            ocupado = o.n;
            if (ocupado) { cursor_erase(); jv_mensajes(&o); if (p->pers[pers].estado == 0 || p->v.turno_acabado) salir = 1; }
            if (!salir) { panel_dibujar(pers); jv_soltar(); antes = plat_keys(); }
            break;
        }
    }
    cursor_erase();
    return p->v.elegido;
}

/* ---------------------------------------------------------------- la linea de vista en el plano */
static int linea_puntos;

static void linea_punto(int x, int y)                /* 0x7F68 + 0x8D03: el original volcaba todo el buffer a la pantalla en cada punto, y por eso la linea "crece" */
{
    Partida *p = &g_part;
    u8 t[8];
    int col = x >> 3, fila = y >> 3, l, a;
    if (col >= MAP_W || fila >= MAP_H) return;
    if (!linea_puntos++) {                           /* el primer volcado muestra el plano con marcas */
        p->v.marcas = 1;
        vista_plano_dibujar(p);
        p->v.marcas = 0;
    }
    for (l = 0; l < 8; l++) t[l] = plano_los[((fila + PLANO_FIL0) * 8 + l) * 32 + col + PLANO_COL0];
    a = p->capa_a[fila * MAP_W + col];
    if (!(a & 0x3F) || !(a & 0x40)) a = 0x06; else a = 0x30;
    gfx_glifo(col + PLANO_COL0, UI_ROW0 + fila + PLANO_FIL0, t, (u8)(a & 7), (u8)((a >> 3) & 7));
    plat_present(); plat_present();                  /* dos cuadros por punto (el volcado del original tardaba unos 34 ms) */
}

static void linea_mostrar(void)                      /* a partir de aqui cada punto de la linea de vista se pinta */
{
    linea_puntos = 0;
    combate_punto = linea_punto;
}

static void linea_fin(void)
{
    combate_punto = 0;
    if (linea_puntos) jv_esperar(50);                /* se deja ver un momento */
}

static int alcance_visible(Partida *p, int pers)     /* combate_alcance() mostrando la linea */
{
    int alc;
    linea_mostrar();
    alc = combate_alcance(p, pers, plano_los);
    linea_fin();
    return alc;
}

/* ---------------------------------------------------------------- elegir objetivo en el plano */
static void plano_base(int botones)                  /* 0xA3C8 y 0x7709 */
{
    cursor_erase();
    g_part.v.marcas = 0;
    vista_plano_dibujar(&g_part);
    if (botones) tx_guion(TX_GUION_B851, -1, 0, 0);
}

static int seleccionar(int pers, int hechizo)        /* 0x7355-0x7381 (ataque) y 0x78CF-0x7940 (hechizo): 1 si se ha elegido, 0 si se cancela */
{
    Partida *p = &g_part;
    Vars *v = &p->v;
    int r, mov, primera, col, fila, blancos = 0;
    u16 antes;
    if (hechizo) blancos = vista_hechizo_blanco(p->pers[pers].hechizo);
    v->arma_alcance = (u8)hechizo;
    for (;;) {
        plano_base(!hechizo);
        v->obj_mons = 0; v->ataque_posible = 0; v->heroe_cursor = 0xFF; v->casilla_cursor_ok = 0;
        jv_soltar();
        antes = plat_keys();
        primera = 1;
        for (;;) {
            r = jv_entrada(&antes, &mov);
            if (mov || primera || !cursor_dibujado()) { cursor_erase(); jv_info_cursor(); cursor_draw(); primera = 0; }
            if (r == 2) { cursor_erase(); v->arma_alcance = 0; return 0; }
            if (r != 1) continue;
            col = (cur_x - 12) >> 3; fila = (cur_y - 2) >> 3;
            if (hechizo) {                                                     /* 0x78F6: primero un heroe, despues un monstruo y despues una casilla */
                if ((blancos & 1) && v->heroe_cursor != 0xFF) { v->pers_muerto = v->heroe_cursor; cursor_erase(); return 1; }
                if (!(blancos & 2)) continue;
                if (v->obj_mons) { v->pers_muerto = BLANCO_MONSTRUO; v->objetivo = v->obj_mons; cursor_erase(); return 1; }
                if ((blancos & 4) && v->ataque_posible) { v->pers_muerto = BLANCO_CASILLA; cursor_erase(); return 1; }
                continue;
            }
            if (fila == 0x15) {                                                /* 0x76AE: la fila de botones */
                if (col >= 0x0E && col < 0x14) { cursor_erase(); panel(pers, 1); break; }
                if (col >= 0x17 && col < 0x1D) { cursor_erase(); return 0; }
            } else if (v->obj_mons) { cursor_erase(); v->objetivo = v->obj_mons; return 1; }
        }
    }
}

/* ---------------------------------------------------------------- ATACAR (0x7319) */
int ui_atacar(int pers)
{
    Partida *p = &g_part;
    Vars *v = &p->v;
    Pers *r = &p->pers[pers];
    int pm0, est0, est, alc, otra, tipo, b7, cuerpo, mente, hab;
    Salida o;
    if (pers == 4 || (r->armadura & 0x20)) return 0;
    pm0 = v->pm; est0 = v->estado_turno;
    if (est0 & 2) return 0;
    est = est0;
    if (est & 1) { est |= 4; v->pm = 0; }
    v->estado_turno = (u8)(est | 2);
    mus_efecto(6);                                                                              /* 0x7348 */
    for (;;) {
        if (!seleccionar(pers, 0)) { v->estado_turno = (u8)est0; v->pm = (u8)pm0; return 1; }      /* 0x7374: se deshace */
        mus_efecto(2);                                                                          /* 0x7383 */
        combate_preparar(p, pers);                                                              /* 0x7386 y 0x7389 */
        alc = alcance_visible(p, pers);
        cursor_erase();
        gfx_clear();
        if (!(p->mons[v->objetivo - 1].cuerpo & 1) || alc) {                                    /* 0x73A6 */
            if (!(p->mons[v->objetivo - 1].cuerpo & 1)) rotulo(TX_GUION_B8D8, 1);
            else if (alc == ALC_BALLESTA) tx_guion(TX_GUION_B934, -1, 0, 0);
            else if (alc == ALC_LEJOS) tx_guion(TX_GUION_B913, -1, 0, 0);
            else tx_guion(TX_GUION_B8D8, -1, 0, 0);
            jv_boton(150);
            return 1;
        }
        tipo = p->mons[v->objetivo - 1].tipo; b7 = p->mons[v->objetivo - 1].b7;                 /* 0x73DB */
        cuerpo = r->cuerpo; mente = r->mente;
        combate_bonos_ataque(p, pers);
        combate_datos_monstruo(p);
        memset(&o, 0, sizeof(o));
        combate_ataque(p, pers, &o);
        pantalla_combate(pers, cuerpo, mente, tipo, b7, v->golpes, v->escudos, 1);
        jv_boton(150);
        if (o.n) jv_mensajes(&o);                                                               /* la recompensa del monstruo */
        otra = 0;
        if (r->t17 & 0x40) {                                                                    /* 0x73F4: heroico, otro ataque */
            r->t17 &= (u8)~0x40; r->efectos &= (u8)~0x40;
            otra = 1;
        } else if (tipo == 1) {                                                                 /* 0x741E y 0x9AF1: la vara de orco, la primera vez en la sala */
            hab = 0;
            if (r->r12 & 8) if (!(v->hab_registrada & 1)) { v->hab_registrada |= 1; hab = 1; }
            if (!hab) { jv_boton(150); return 1; }
            otra = 1;
        } else { jv_boton(150); return 1; }
        if (otra && !si_no(TX_VENT_B020)) return 1;                                               /* 0x7436 */
        v->obj_mons = 0;
    }
}

/* ---------------------------------------------------------------- bolsa: USAR_OBJETO (0x7877) */
int ui_bolsa(int pers)
{
    if (pers == 4) return 0;
    mus_efecto(6);                                                                  /* 0x788D */
    if (!panel(pers, 0)) return 1;                                                    /* sin hechizo elegido: 0x8009 */
    ui_hechizo(pers);
    return 1;
}

void ui_hechizo(int pers)                            /* 0x78B4: elegir a quien se lanza el hechizo elegido y lanzarlo */
{
    Partida *p = &g_part;
    Vars *v = &p->v;
    Pers *r = &p->pers[pers];
    Salida o;
    if (!seleccionar(pers, 1)) { r->hechizo = 0; v->arma_alcance = 0; return; }
    memset(&o, 0, sizeof(o));
    mus_efecto(1);                                                                  /* 0x7945 */
    linea_mostrar();
    hechizo_lanzar(p, pers, plano_los, &o);
    linea_fin();
    v->arma_alcance = 0;
    jv_mensajes(&o);
}

/* ---------------------------------------------------------------- el turno de Morcar (0x9CEA) */
void ui_turno_monstruos(void)
{
    Partida *p = &g_part;
    int num, k, n, r;
    for (num = 1; num <= N_MONS; num++) {
        r = monstruo_planear(p, num);
        if (r == MON_NADA) continue;
        n = monstruo_pasos();
        for (k = 0; k < n; k++) {                                                   /* 0xA086: un paso y el plano */
            monstruo_paso(p, num, k);
            p->v.marcas = 0;
            vista_plano_dibujar(p);
        }
        jv_esperar(50);                                                             /* 0xA0CC */
        ui_defensa(p->v.atacante, num);                                             /* 0x990D */
        p->v.marcas = 0;
        vista_plano_dibujar(p);                                                     /* 0xA3C8 */
        tx_guion(TX_GUION_A438, -1, 0, 0);
        jv_boton(50);
    }
}

/* ---------------------------------------------------------------- ganchos de las misiones */
static int ui_pregunta(Salida *o, int ventana)                    /* 0x8D5E */
{
    if (!jv_fondo_ok) { cursor_erase(); juego_redibujar(g_part.v.pers_actual); jv_fondo_ok = 1; }
    jv_mensajes(o);
    o->n = 0;
    jv_ventanas++;
    return si_no(ventana);
}

static void ui_ataque(Partida *p, int heroe, int monstruo, Salida *o)       /* 0x9921 tras la ventana de una emboscada */
{
    jv_mostrar_salida(p->v.pers_actual, o);
    o->n = 0;
    ui_defensa(heroe, monstruo);
    juego_redibujar(p->v.pers_actual);
}

void ui_hooks(void)
{
    mision_pregunta = ui_pregunta;
    accion_ataque = ui_ataque;
    accion_especial = mision_especial;
}
