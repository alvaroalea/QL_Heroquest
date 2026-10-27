/* Pantallas del ataque, de la bolsa y de la defensa en el PC, con las teclas de HQ_KEYS (las capturas se guardan con Sxxx).
   Uso: HQ_KEYS="..." build/host/combate_demo escena mision [dados]   escena = ataque | bolsa | defensa | hechizo   (desde la raiz del proyecto)
   El heroe 0 se pone al lado del primer monstruo, todo el mapa a la vista y el cursor sobre el monstruo. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "vista.h"
#include "juego_int.h"
#include "cursor.h"
#include "keys.h"
#include "monstruo.h"

#ifdef DEMO_QL                      /* en el QL no hay argumentos ni variables de entorno: se fijan al compilar (-DDEMO_ESCENA=\"ataque\" ...) */
#ifndef DEMO_PERS
#define DEMO_PERS "0"
#endif
#ifndef DEMO_CELDA
#define DEMO_CELDA 0
#endif
#ifndef DEMO_MONS
#define DEMO_MONS 0
#endif
#ifndef DEMO_HECHIZO
#define DEMO_HECHIZO "3"
#endif
static const char *opt(const char *n)
{
    if (!strcmp(n, "HQ_PERS")) return DEMO_PERS;
    if (!strcmp(n, "HQ_CELDA")) return DEMO_CELDA;
    if (!strcmp(n, "HQ_MONS")) return DEMO_MONS;
    if (!strcmp(n, "HQ_HECHIZO")) return DEMO_HECHIZO;
    return 0;
}
#else
#define opt getenv
#endif

static u8 *cargar(const char *nombre)
{
    u8 *d = data_load(nombre, 0);
    if (!d) { printf("no se puede leer %s\n", nombre); exit(2); }
    return d;
}

int main(int argc, char **argv)
{
    u8 pr[4] = { 1, 1, 1, 1 }, *dm, *spr;
    char nm[40];
    long n;
    int i, mision, k, dx, dy, cm, cf, ok = 0, dados[16], nd = 0, ph = opt("HQ_PERS") ? atoi(opt("HQ_PERS")) : 0;
    Partida *p = &g_part;
#ifdef DEMO_QL
    static char *av[3];
    av[1] = (char *)DEMO_ESCENA; av[2] = (char *)"0"; argv = av; argc = 3;
#endif
    if (argc < 3) { printf("uso: combate_demo escena mision [dados...]\n"); return 1; }
    for (i = 3; i < argc && nd < 16; i++) dados[nd++] = atoi(argv[i]);
    mision = atoi(argv[2]);
    plat_init();
    gfx_init();
    if (!data_init()) { printf("data_init falla\n"); return 2; }
    gfx_set_font(fnt_normal);
    spr = cargar("sprites_bin");
    if (!vista_datos(spr, cargar("escenario_bin"), cargar("vista_bin"), cargar("baldosas_bin"), cargar("marca_activo_bin"))) { printf("vista_datos falla\n"); return 2; }
    sprintf(nm, "mision_%02d_es_bin", mision);
    dm = data_load(nm, &n);
    if (!dm) { printf("no se puede leer %s\n", nm); return 2; }
    pers_boot(p);
    if (!partida_cargar(p, mision, dm, n, pr)) { printf("mision rechazada\n"); return 2; }
    ui_combate_iniciar();
    cursor_init(spr);
    for (i = 0; i < MAP_N; i++) if (p->capa_a[i] & 0x3F) p->capa_a[i] |= 0x40;
    for (i = 0; i < N_MONS; i++) if (p->mons[i].estado) { p->mons[i].cuerpo |= 1; if (!ok) { cm = p->mons[i].col; cf = p->mons[i].fila; ok = i + 1; } }
    if (!ok) { printf("sin monstruos\n"); return 2; }
    for (k = 0; k < 4 && p->capa_a[(cf + (k == 2 ? 1 : k == 3 ? -1 : 0)) * MAP_W + cm + (k == 0 ? 1 : k == 1 ? -1 : 0)] == 0; k++) { }
    dx = k == 0 ? 1 : k == 1 ? -1 : 0; dy = k == 2 ? 1 : k == 3 ? -1 : 0;
    p->pers[ph].col = (u8)(cm + dx); p->pers[ph].fila = (u8)(cf + dy);
    p->v.pers_actual = (u8)ph;
    if (nd) dado_guion(dados, nd); else dado_semilla(777UL);
    gfx_clear();
    game_draw_background();
    vista_construir(p, ph);
    vista_figuras_dibujar();
    { const char *e = opt("HQ_MONS"); if (e) { int q = atoi(e) - 1; p->mons[q].cuerpo |= 1; cm = p->mons[q].col; cf = p->mons[q].fila; } }
    cur_x = (cm + PLANO_COL0) * 8 + 0x24 - 3 * 8 + 4; cur_y = (cf + PLANO_FIL0) * 8 + 0x0C - 8 + 4 + 2;
    { const char *e = opt("HQ_CELDA"); int c, r; if (e && sscanf(e, "%d,%d", &c, &r) == 2) { cur_x = c * 8 + 12 + 4; cur_y = r * 8 + 2 + 2; } }
    if (!strcmp(argv[1], "ataque")) ui_atacar(ph);
    else if (!strcmp(argv[1], "bolsa") || !strcmp(argv[1], "hechizo")) {
        if (!strcmp(argv[1], "hechizo")) { p->pers[ph].hechizos[0] = 0x3F; p->pers[ph].hechizos[1] = 0x3F; }
        p->pers[ph].efectos = 0x7F; p->pers[ph].objetos = 0x1F; p->pers[ph].armas = 0x7F; p->pers[ph].armadura |= 0x0F;
        ui_bolsa(ph);
    } else if (!strcmp(argv[1], "lanzar")) {
        p->pers[ph].hechizo = (u8)atoi(opt("HQ_HECHIZO"));
        p->pers[ph].hechizos[0] = 0x3F; p->pers[ph].hechizos[1] = 0x3F;
        ui_hechizo(ph);
    } else if (!strcmp(argv[1], "ronda")) {
        for (i = 0; i < N_MONS; i++) if (p->mons[i].estado) { p->mons[i].estado = 1; p->mons[i].hab = (u8)(p->capa_a[p->mons[i].fila * MAP_W + p->mons[i].col] & 0x3F); }
        for (i = 0; i < 4; i++) p->pers[i].hab = (u8)(p->capa_a[p->pers[i].fila * MAP_W + p->pers[i].col] & 0x3F);
        p->pers[ph].hab = p->mons[ok - 1].hab;
        if ((p->capa_a[cf * MAP_W + cm + 2] & 0x3F) == p->mons[ok - 1].hab && (p->capa_a[cf * MAP_W + cm + 1] & 0x3F) == p->mons[ok - 1].hab && p->capa_c[cf * MAP_W + cm + 2] == 0 && !p->capa_c[cf * MAP_W + cm + 1]) { p->pers[ph].col = (u8)(cm + 2); p->pers[ph].fila = (u8)cf; }
        ui_turno_monstruos();
    } else if (!strcmp(argv[1], "defensa")) ui_defensa(ph, ok);
#ifdef DEMO_QL
    plat_wait_key();
#endif
    plat_snapshot("fin");
    { const Pers *r = &p->pers[ph]; printf("cuerpo %d mente %d oro %d armas %02X r11 %02X r12 %02X r13 %02X efectos %02X t17 %02X t18 %02X hech %02X %02X sel %d estado %02X pm %d objetivo mons estado %d\n", r->cuerpo, r->mente, r->oro, r->armas, r->r11, r->r12, r->r13, r->efectos, r->t17, r->t18, r->hechizos[0], r->hechizos[1], r->hechizo, p->v.estado_turno, p->v.pm, p->mons[ok - 1].estado); }
    return 0;
}
