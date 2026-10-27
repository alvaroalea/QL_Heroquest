/* HeroQuest QL - el bucle de la partida (ver juego.h y docs/ZX_TURNO.md). La logica del turno esta en turno.c; aqui se dibuja y se esperan las teclas. */
#include <stdlib.h>
#include <string.h>
#include "juego.h"
#include "game.h"
#include "cursor.h"
#include "textos_ids.h"
#include "accion.h"
#include "combate.h"
#include "mision.h"
#include "music.h"
#include "qsound.h"
#include "juego_int.h"

int (*juego_accion[10])(int pers);
void (*juego_turno_monstruos)(void);
void (*juego_salida_mision)(int pers);
int jv_fondo_ok;                                    /* la escena ya esta dibujada para los mensajes de una mision */
int jv_ventanas;                                    /* ventanas de mensaje mostradas (para saber si hay que esperar) */

#define ROW(n) (UI_ROW0 + (n))

void jv_esperar(int cuadros)                    /* 0x9D66 */
{
    while (cuadros-- > 0) plat_present();
}

void jv_soltar(void)                    /* 0x8DAA */
{
    while (plat_keys() & K_FIRE) plat_present();
}

/* ---------------------------------------------------------------- marcadores */
static void digito(int col, int row, int d, u8 ink) { gfx_char(col, ROW(row), (u8)(0x30 + d), ink, C_BLACK); }

static void panel_cuerpo_mente(int pers)            /* 0x926A: corazon rojo y cerebro verde, uno al lado del otro */
{
    const Pers *r = &g_part.pers[pers];
    gfx_char(0, ROW(1), 0x5D, C_RED, C_BLACK);   gfx_char(1, ROW(1), 0x40, C_RED, C_BLACK);   digito(2, 1, r->cuerpo, C_RED);
    gfx_char(5, ROW(1), 0x5E, C_GREEN, C_BLACK); gfx_char(6, ROW(1), 0x40, C_GREEN, C_BLACK); digito(7, 1, r->mente, C_GREEN);
}

static void panel_oro(int pers)                     /* 0x91F7: monedas, signo igual y cuatro cifras */
{
    unsigned v = g_part.pers[pers].oro;
    gfx_char(0, ROW(2), 0x3C, C_YELLOW, C_BLACK); gfx_char(1, ROW(2), 0x40, C_YELLOW, C_BLACK);
    digito(2, 2, (int)(v / 1000), C_YELLOW); v %= 1000;
    digito(3, 2, (int)(v / 100), C_YELLOW);  v %= 100;
    digito(4, 2, (int)(v / 10), C_YELLOW);
    digito(5, 2, (int)(v % 10), C_YELLOW);
}

void jv_panel_movimiento(void)                  /* 0x9242: la huella y los puntos que quedan (sin cero a la izquierda) */
{
    int pm = g_part.v.pm;
    gfx_char(0, ROW(3), 0x5C, C_CYAN, C_BLACK); gfx_char(1, ROW(3), 0x40, C_CYAN, C_BLACK);
    if (pm >= 10) digito(2, 3, pm / 10, C_CYAN); else gfx_char(2, ROW(3), 0x5F, C_CYAN, C_BLACK);
    digito(3, 3, pm % 10, C_CYAN);
}

void juego_panel(int pers)
{
    tx_guion(TX_GUION_A484 + pers, -1, 0, 0);       /* 0x9C5B: el nombre del personaje con su color */
    panel_cuerpo_mente(pers);
    panel_oro(pers);
    jv_panel_movimiento();
}

void juego_redibujar(int pers)                      /* 0x8009 FIN_ACCION y el principio de 0x6E9C */
{
    cursor_erase();
    gfx_clear();
    game_draw_background();
    vista_construir(&g_part, pers);
    vista_figuras_dibujar();
    juego_panel(pers);
}

/* ---------------------------------------------------------------- plano con el nombre de lo que hay bajo el cursor */
void jv_info_cursor(void)                           /* 0x7733: nombre de lo que hay bajo el cursor; deja en Vars lo que mira el ataque y los hechizos */
{
    Partida *p = &g_part;
    Vars *v = &p->v;
    const u8 *s = (const u8 *)"";
    int n = 0, row, col, cell, c, i, tipo = -1, idx = -1;
    v->obj_mons = 0; v->ataque_posible = 0; v->heroe_cursor = 0xFF;
    row = cur_y < 0x0C ? -1 : (cur_y - 0x0C >= 0x98 ? -1 : (cur_y - 0x0C) >> 3);
    col = cur_x < 0x24 ? -1 : (cur_x - 0x24 >= 0xD0 ? -1 : (cur_x - 0x24) >> 3);
    if (row >= 0 && col >= 0) {
        cell = row * MAP_W + col;
        v->casilla_cursor = (u16)cell; v->casilla_cursor_ok = 1;
        if (p->capa_a[cell] & 0x40) {
            c = p->capa_c[cell];
            if (c && !(c >= 0x32 && c < 0x38)) {
                if (c < 0x30) { tipo = 0; idx = p->mons[c - 1].tipo; v->obj_mons = (u8)c; v->objetivo = (u8)c; }
                else { tipo = 1; idx = vista_nombre_objeto(c); }
            } else
                for (i = 0; i < N_PERS; i++)
                    if (p->pers[i].estado && p->pers[i].col == col && p->pers[i].fila == row) { tipo = 2; idx = i; v->heroe_cursor = p->pers[i].indice; break; }
        }
        if (tipo < 0 && v->arma_alcance) v->ataque_posible = 1;
    }
    if (tipo == 0) s = tx_str(TX_NOMBRES_MONSTRUO, idx, &n);
    else if (tipo == 1) s = tx_str(TX_NOMBRES_OBJETO, idx, &n);
    else if (tipo == 2) s = tx_str(TX_NOMBRES_HEROE, idx, &n);
    tx_cadena_en(3, ROW(21), 9, s, n, 'C', C_WHITE, C_BLACK);
}

void juego_plano(int pers)                          /* 0x7710 */
{
    u16 k, antes;
    int n;
    (void)pers;
    cursor_erase();
    mus_efecto(6);                                  /* 0x7710 */
    g_part.v.marcas = 0;
    vista_plano_dibujar(&g_part);
    jv_soltar();
    antes = plat_keys();
    for (;;) {
        int ox = cur_x, oy = cur_y;
        n = plat_present();
        k = plat_keys();
        while (n-- > 0) cursor_move(k);
        if (cur_x != ox || cur_y != oy || !cursor_dibujado()) {
            cursor_erase();
            jv_info_cursor();
            cursor_draw();
        }
        if ((k & K_FIRE) && !(antes & K_FIRE)) break;
        if (k & K_ESC) break;
        antes = k;
    }
    cursor_erase();
    jv_soltar();
}

int jv_entrada(u16 *antes, int *movido)
{
    int n = plat_present(), ox = cur_x, oy = cur_y, r = 0;
    u16 k = plat_keys();
    while (n-- > 0) cursor_move(k);
    *movido = (cur_x != ox || cur_y != oy);
    if ((k & K_FIRE) && !(*antes & K_FIRE)) r = 1;
    if (k & K_ESC) r = 2;
    *antes = k;
    return r;
}

/* ---------------------------------------------------------------- ventanas de las acciones */
void jv_boton(int cuadros)            /* 0x9D66, aqui el boton se salta la espera */
{
    u16 antes = plat_keys();
    while (cuadros-- > 0) {
        u16 k;
        plat_present();
        k = plat_keys();
        if ((k & K_FIRE) && !(antes & K_FIRE)) break;
        antes = k;
    }
}

void jv_guion_cifra(int id, int n, int cifra)       /* linea n de un guion con la primera cifra cambiada por otra */
{
    int x, y, attr, al, len, i;
    const u8 *s;
    u8 b[40];
    if (!tx_guion_linea(id, n, &x, &y, &attr, &al, &s, &len)) return;
    if (len > 40) len = 40;
    memcpy(b, s, (size_t)len);
    for (i = 0; i < len; i++) if (b[i] >= 0x30 && b[i] <= 0x39) { b[i] = (u8)(0x30 + cifra); break; }
    if (al == 'C') x = 16 - len / 2; else if (al == 'R') x = x - len;
    gfx_text(x, ROW(y), b, len, (u8)(attr & 7), C_BLACK);
}

static void mostrar_msg(const Msg *m)               /* las ventanas y guiones de 0x8D99 */
{
    static const int ventana[12] = { 0, TX_VENT_AC50, TX_VENT_AC66, TX_VENT_AC90, TX_VENT_ACD9, TX_VENT_AD1D, TX_VENT_AD8A, TX_VENT_ADDE, TX_VENT_AE03,
                                     TX_VENT_AF65, TX_VENT_C172, TX_VENT_C19C };
    int n, i, l;
    u8 num[8], l1[40], l2[40];
    const u8 *s[2], *t, *d;
    int lin[2], len[2];
    switch (m->id) {
    case MSG_ORO:                                   /* el numero va en la linea del medio */
        n = m->a; i = 0;
        if (n >= 100) num[i++] = (u8)(0x30 + n / 100);
        if (n >= 10) num[i++] = (u8)(0x30 + n / 10 % 10);
        num[i++] = (u8)(0x30 + n % 10);
        s[0] = num; lin[0] = 1; len[0] = i;
        tx_ventana_con(TX_VENT_ADDE, 1, lin, s, len);
        jv_boton(200);
        break;
    case MSG_POCION:                                /* «POCION DE <nombre>» y «CUANDO EMPLEAS» seguido de lo que hace */
        t = tx_str(TX_POCIONES_NOMBRE, m->a, &n);
        l = 0;
        d = tx_ventana_linea(TX_VENT_AE03, 1, &i);
        memcpy(l1, d, (size_t)i); l = i; l1[l++] = 0x5F;
        while (n > 0 && t[n - 1] == 0x5F) n--;
        memcpy(l1 + l, t, (size_t)n); l += n;
        d = tx_str(TX_POCIONES_DESCRIPCION, m->b, &i);
        memcpy(l2, d, (size_t)i);
        while (i > 0 && l2[i - 1] == 0x5F) i--;
        s[0] = l1; lin[0] = 1; len[0] = l;
        s[1] = l2; lin[1] = 3; len[1] = i;
        tx_ventana_con(TX_VENT_AE03, 2, lin, s, len);
        jv_boton(200);
        break;
    case MSG_MUERTE:                                /* 0x7B31: pantalla limpia con el mensaje */
        cursor_erase();
        gfx_clear();
        tx_guion(TX_GUION_BB18 + m->a, -1, 0, 0);
        jv_boton(200);
        break;
    case MSG_ESPECIAL:
    case MSG_ORO_PANEL:
        break;
    case MSG_VENT:
        if (m->a == TX_VENT_BD30) mus_efecto(3);                 /* 0x901B */
        tx_ventana(m->a); jv_ventanas++; jv_boton(200);
        break;
    case MSG_BANE_ORC: tx_guion(TX_GUION_B9E3, -1, 0, 0); jv_boton(150); break;       /* 0x9D5E: dibuja y espera */
    case MSG_ESTRELLA: tx_guion(TX_GUION_BA09, -1, 0, 0); jv_boton(150); break;
    case MSG_TRAMPA_OFF: tx_guion(TX_GUION_B968, -1, 0, 0); jv_boton(150); break;
    case MSG_TRAMPA_ON: tx_guion(TX_GUION_B982, -1, 0, 0); jv_boton(150); break;
    case MSG_ORO_MINA: tx_guion(TX_GUION_B9B7, -1, 0, 0); jv_boton(150); break;
    case MSG_HECHIZO_OK: tx_guion(TX_GUION_BA8F, -1, 0, 0); jv_boton(200); break;     /* 0x991B y 0x7B5D */
    case MSG_HECHIZO_NO: tx_guion(TX_GUION_BAAC, -1, 0, 0); jv_boton(200); break;
    case MSG_HECHIZO_1: tx_guion(TX_GUION_BAC6, -1, 0, 0); jv_boton(200); break;
    case MSG_HECHIZO_N:                             /* la cifra de los puntos va en el texto (0xBAFE) */
        jv_guion_cifra(TX_GUION_BAEA, 0, m->a);
        jv_boton(200);
        break;
    case MSG_AGUA_SANTA:                            /* 0x7DF0: cuantos monstruos (dos cifras en 0xBA58 y 0xBA59) */
        if (m->a == 0) tx_guion(TX_GUION_BA6A, -1, 0, 0);
        else {
            tx_guion(TX_GUION_BA2F, -1, 0, 0);
            gfx_char(1, ROW(21), m->a >= 10 ? 0x31 : 0x5F, C_YELLOW, C_BLACK);
            gfx_char(2, ROW(21), (u8)(0x30 + m->a % 10), C_MAGENTA, C_BLACK);
        }
        jv_boton(150);
        break;
    default:
        if (m->id == MSG_TRAMPA || m->id == MSG_FOSO || m->id == MSG_LANZA || m->id == MSG_ROCA_PUNTO) mus_efecto(3);      /* 0x81D5, 0xA126, 0xA1FC */
        tx_ventana(ventana[m->id]);
        jv_boton(200);
        break;
    }
}

void jv_mensajes(const Salida *o)                   /* los mensajes tal cual, sin redibujar la vista antes */
{
    int i;
    for (i = 0; i < o->n; i++) if (o->m[i].id != MSG_ESPECIAL) mostrar_msg(&o->m[i]);
}

void jv_mostrar_salida(int pers, const Salida *o)
{
    int i;
    for (i = 0; i < o->n; i++) {
        if (o->m[i].id == MSG_ESPECIAL) continue;
        if (o->m[i].id != MSG_MUERTE) juego_redibujar(pers);
        mostrar_msg(&o->m[i]);
        if (o->m[i].id == MSG_FOSO || o->m[i].id == MSG_LANZA) jv_boton(150);
    }
    if (o->n) juego_redibujar(pers);
}

/* llave: abrir las puertas de la casilla y las contiguas (0x727E) */
static int ui_llave(int pers)
{
    if (accion_abrir_puerta(&g_part, pers)) return 1;
    g_part.v.redibujar = 0;                         /* no hay puertas que abrir: nada cambia */
    return 0;
}

/* ojo: la ventana de «REVISAR Y BUSCAR: TRAMPAS, PUERTAS, TESOROS» (0x804E) y la busqueda elegida */
static int ui_ojo(int pers)
{
    Partida *p = &g_part;
    const u8 *z;
    int nz, i, col, fila, elegida = -1;
    u16 k, antes;
    Salida o;
    if (pers == 4 || (p->v.estado_turno & 2)) return 0;
    mus_efecto(6);                                  /* 0x8063 */
    p->v.estado_turno |= 2;                         /* 0x8039: ya ha actuado, y si se habia movido no puede moverse mas */
    if (p->v.estado_turno & 1) { p->v.estado_turno |= 4; p->v.pm = 0; }
    jv_panel_movimiento();
    tx_ventana(TX_VENT_AFE2);
    nz = tx_zonas(TX_VENT_AFE2, &z);
    jv_soltar();
    antes = plat_keys();
    while (elegida < 0) {
        int n = plat_present(), ox = cur_x, oy = cur_y;
        k = plat_keys();
        while (n-- > 0) cursor_move(k);
        if (cur_x != ox || cur_y != oy || !cursor_dibujado()) { cursor_erase(); cursor_draw(); }
        if ((k & K_FIRE) && !(antes & K_FIRE)) {
            col = (cur_x - 12) >> 3; fila = (cur_y - 2) >> 3;
            for (i = 0; i < nz; i++) {
                const u8 *e = z + 6 * i;
                if (col >= e[0] && col < e[1] && fila >= e[2] && fila < e[3]) { elegida = (e[4] << 8 | e[5]) == 0x80A9; break; }
            }
        }
        antes = k;
    }
    cursor_erase();
    memset(&o, 0, sizeof(o));
    mus_efecto(6);                                  /* 0x8079 y 0x80A9 */
    if (elegida == 0) accion_buscar_trampas(p, pers, &o); else accion_buscar_tesoros(p, pers, &o);
    for (i = 0; i < o.n && o.m[i].id == MSG_ESPECIAL; i++) ;
    if (i == o.n || o.m[i].id == MSG_MUERTE) juego_redibujar(pers);      /* borra la ventana de eleccion (jv_mostrar_salida la borra antes de cada mensaje) */
    jv_mostrar_salida(pers, &o);
    return 2;
}

/* ---------------------------------------------------------------- el turno */
static void sonido_alternar(void)                   /* tecla M: musica -> efectos -> nada (como el original, los efectos solo suenan sin musica) */
{
    if (!qs_present()) return;
    if (g_musica) {
        g_musica = 0; mus_enable(0); mus_efectos_activar(1);
    } else if (mus_efectos_activos()) {
        mus_efectos_activar(0);
    } else {
        g_musica = 1; mus_efectos_activar(1); mus_enable(1); mus_song(1);
    }
}

static int tirar_dados(int pers)                    /* 0x6F1C: el heroe parpadea y los puntos cambian hasta que se pulsa el boton; 1 si se pulsa ESC */
{
    int n = 0;
    u16 k, m_antes = plat_keys();
    jv_soltar();
    for (;;) {
        plat_present();
        vista_figuras_borrar();
        turno_dado_fotograma(&g_part, pers, n++);
        mus_efecto(5);                              /* 0x6F5B: los dados suenan en cada fotograma */
        vista_figuras_dibujar();
        jv_panel_movimiento();
        k = plat_keys();
        if (k & K_ESC) return 1;
        if ((k & K_M) && !(m_antes & K_M)) sonido_alternar();
        m_antes = k;
        if (k & K_FIRE) break;
    }
    vista_figuras_borrar();
    turno_dados_fin(&g_part, pers);
    vista_figuras_dibujar();
    jv_soltar();
    jv_esperar(10);
    return 0;
}

static void ventana_salida(int pers)                /* 0x8E7B: «TU HAS ESCAPADO DE LA MAZMORRA» y lo de cada mision */
{
    Salida o;
    mus_efecto(7);                                    /* 0x8E7D */
    tx_ventana(TX_VENT_BB8E);
    memset(&o, 0, sizeof(o));
    jv_fondo_ok = 1; jv_ventanas = 0;
    mision_salida(&g_part, pers, &o);
    jv_mensajes(&o);
    if (!jv_ventanas) jv_boton(200);                  /* 0x8FD2 */
    if (juego_salida_mision) juego_salida_mision(pers);
}

#define ANIM_RITMO 4                                /* cuadros de 50 Hz por fotograma de la animacion del paso (12,5 por segundo) */

static int turno_jugar(int pers)                    /* TURNO_PERSONAJE (0x6DDD): 1 si el jugador aborta */
{
    Partida *p = &g_part;
    int st, n, i, boton, moviendo, rebuilt, pm, accion, ritmo = 0, avanza, cambio, borradas;
    u16 k, antes;
    if (!p->pers[pers].estado) return 0;
    p->v.pers_actual = (u8)pers;
    turno_empezar(p, pers);
    st = turno_estado(p, pers);
    juego_redibujar(pers);
    if (st & (TURNO_VENT_SUENO | TURNO_VENT_TORMENTA)) {      /* pierde el turno */
        if (st & TURNO_VENT_SUENO) tx_ventana(TX_VENT_AFBE);
        if (st & TURNO_VENT_TORMENTA) tx_ventana(TX_VENT_AF87);
        jv_esperar(200);
        return 0;
    }
    turno_calcular_dados(p, pers);
    if (tirar_dados(pers)) return 1;
    antes = plat_keys();
    while (turno_continua(p)) {
        int ox = cur_x, oy = cur_y;
        n = plat_present();
        k = plat_keys();
        if (k & K_ESC) return 1;
        if ((k & K_M) && !(antes & K_M)) sonido_alternar();
        boton = (k & K_FIRE) && !(antes & K_FIRE);
        antes = k;
        moviendo = vista_animando() || p->v.pasos;
        ritmo += n;                                    /* la animacion avanza a su ritmo, sincronizada con el cuadro; los cuadros perdidos cuentan */
        avanza = !moviendo || ritmo >= ANIM_RITMO;
        if (moviendo && avanza) ritmo = 0;
        moviendo = moviendo && avanza;                 /* solo se borra y redibuja cuando la figura cambia */
        cambio = moviendo;                             /* solo se borran las figuras (y se redibujan) si algo va a cambiar */
        borradas = 0;
        if (moviendo) cursor_erase();                  /* las figuras no se borran: vista_figuras_refrescar compone el cambio fuera de la pantalla */
        pm = p->v.pm;
        rebuilt = 0;
        accion = -1;
        for (i = 0; i < n; i++) {
            if (!p->v.evento) cursor_move(k);                  /* 0x713E: con un evento pendiente no se mueve */
            if (i == 0 && boton) {
                accion = turno_clic(p, pers, cur_x, cur_y);
                if (accion == ACC_OJO && (pers == 4 || (p->v.estado_turno & 2))) accion = -1;      /* el ojo ya no se puede usar: no hace nada */
                if (!cambio && (p->v.pasos || p->v.redibujar || p->v.evento || pm != p->v.pm || vista_animando())) {
                    cursor_erase(); vista_figuras_borrar(); cambio = 1; borradas = 1;      /* antes de reconstruir la vista */
                }
            }
            if (avanza) { turno_automatico(p, pers); turno_fotograma(p, pers); }
            if (!p->pers[pers].estado) break;                  /* ha llegado a la escalera */
            if (p->v.redibujar) {
                if (!borradas) { cursor_erase(); vista_figuras_borrar(); cambio = 1; borradas = 1; }
                vista_construir(p, pers); rebuilt = 1;
            }
        }
        if (cambio && !borradas && !rebuilt) vista_figuras_refrescar();
        if (!p->pers[pers].estado && (p->salidos & (1 << pers))) {      /* salida de la mazmorra */
            cursor_erase();
            juego_redibujar(pers);
            ventana_salida(pers);
            if (!p->pers[pers].estado) return 0;       /* mision 4: vuelve a entrar a por mas oro y sigue su turno */
            juego_redibujar(pers);
            antes = plat_keys();
            continue;
        }
        if (accion == ACC_PASAR) mus_efecto(6);                  /* 0x7274 */
        if (p->v.sonido_pend) { mus_efecto(p->v.sonido_pend - 1); p->v.sonido_pend = 0; }      /* 0x6E7A: el paso o la puerta */
        if (accion == ACC_MAPA) {
            juego_plano(pers);
            juego_redibujar(pers);
            antes = plat_keys();
            continue;
        }
        if (accion == ACC_OJO || accion == ACC_BOLSA || accion == ACC_ATACAR || accion == ACC_LLAVE) {
            if (juego_accion[accion]) {
                int r;
                cursor_erase();
                r = juego_accion[accion](pers);
                if (r == 1) juego_redibujar(pers);
                if (r) { antes = plat_keys(); continue; }
            }
        }
        if (p->v.evento_mision) {                                  /* 0x70A1: eventos de las misiones 5, 9 y 11 */
            Salida o;
            memset(&o, 0, sizeof(o));
            jv_fondo_ok = 0; jv_ventanas = 0;
            mision_evento(p, pers, &o);
            if (o.n || jv_fondo_ok) {
                cursor_erase();
                if (!jv_fondo_ok) juego_redibujar(pers);
                jv_mensajes(&o);
                juego_redibujar(pers);
                antes = plat_keys();
                continue;
            }
        }
        if (p->v.evento && !vista_animando()) {                  /* 0xA108: lo que ha dejado pendiente una casilla */
            Salida o;
            cursor_erase();
            memset(&o, 0, sizeof(o));
            accion_evento(p, pers, &o);
            jv_mostrar_salida(pers, &o);
            antes = plat_keys();
            continue;
        }
        if (rebuilt) juego_panel(pers);
        else if (pm != p->v.pm) jv_panel_movimiento();
        if (borradas || rebuilt) vista_figuras_dibujar();
        if (cur_x != ox || cur_y != oy || !cursor_dibujado() || cambio) {
            cursor_erase();
            cursor_draw();
        }
    }
    cursor_erase();
    return 0;
}

static void turno_morcar(void)                      /* FIN_DE_RONDA (0x9CE0): el plano, sus mensajes y los monstruos (todavia no se mueven) */
{
    cursor_erase();
    g_part.v.marcas = 0;
    vista_plano_dibujar(&g_part);
    tx_guion(TX_GUION_A438, -1, 0, 0);
    jv_esperar(150);
    if (juego_turno_monstruos) juego_turno_monstruos();
    tx_guion(TX_GUION_A453, -1, 0, 0);
    jv_esperar(100);
    turno_fin_de_ronda(&g_part);
}

int juego_partida(void)
{
    int pers;
    if (!juego_accion[ACC_LLAVE]) juego_accion[ACC_LLAVE] = ui_llave;
    if (!juego_accion[ACC_OJO]) juego_accion[ACC_OJO] = ui_ojo;
    ui_hooks();
    if (!juego_turno_monstruos) juego_turno_monstruos = ui_turno_monstruos;
    if (!juego_accion[ACC_ATACAR]) juego_accion[ACC_ATACAR] = ui_atacar;
    if (!juego_accion[ACC_BOLSA]) juego_accion[ACC_BOLSA] = ui_bolsa;
    for (;;) {
        for (pers = 0; pers < N_PERS; pers++)
            if (turno_jugar(pers)) return JUEGO_ABORTADA;
        if (!mision_continua(&g_part)) return JUEGO_ACABADA;
        turno_morcar();
        if (!mision_continua(&g_part)) return JUEGO_ACABADA;
    }
}
