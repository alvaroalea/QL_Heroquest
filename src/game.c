/* HeroQuest QL - del menu a la partida: el fondo de la pantalla de juego, los registros de los heroes a partir de los datos del menu, la carga de la mision
   (partida.c) y el arranque del bucle de juego (juego.c). */
#include <stdlib.h>
#include <stdio.h>
#include "game.h"
#include "partida.h"
#include "vista.h"
#include "juego.h"
#include "juego_int.h"
#include "cursor.h"
#include "keys.h"
#include "music.h"
#include "textos_ids.h"

void game_draw_background(void)
{
    gimg g;
    if (pack_get(pk_marco, 0, &g)) gfx_blit(&g, 0, (256 - g.h) / 2);   /* marco de la pantalla de juego: 256 x 256 (las 32 lineas de arriba y de abajo no se usan); si fuera de 256 x 192 va centrado */
}

/* Del menu a los registros de la partida: oro, equipo y los hechizos de los grupos elegidos (masks de las tablas 0x65F7 del overlay) */
static void partida_desde_menu(Partida *p)
{
    static const u8 grupo_masc[4][2] = { { 0x38, 0 }, { 0, 0x07 }, { 0, 0x38 }, { 0x07, 0 } };   /* aire, fuego, tierra, agua */
    int i, g;
    for (i = 0; i < 4; i++) {
        Pers *r = &p->pers[i];
        r->oro = heroes[i].oro;
        r->armas = heroes[i].armas;
        r->armadura = heroes[i].armadura;
        r->hechizos[0] = r->hechizos[1] = 0;
        for (g = 0; g < 4; g++)
            if (heroes[i].grupos & (1 << g)) { r->hechizos[0] |= grupo_masc[g][0]; r->hechizos[1] |= grupo_masc[g][1]; }
    }
}

/* Carga la mision elegida (INICIAR_MISION): 1 si ha ido bien */
static int partida_iniciar(int mision)
{
    char name[24];
    u8 pr[4], *m;
    long n;
    int i, ok;
    data_mision_nombre(name, mision);
    m = data_load(name, &n);
    if (!m) return 0;
    for (i = 0; i < 4; i++) pr[i] = heroes[i].activo;
    partida_desde_menu(&g_part);
    ok = partida_cargar(&g_part, mision, m, n, pr);
    free(m);
    if (ok) mazo_barajar_azar();
    return ok;
}

void game_run(int mision)
{
    u8 *sprites = data_load("sprites_bin", 0), *escenario = data_load("escenario_bin", 0), *vista = data_load("vista_bin", 0);
    u8 *baldosas = data_load("baldosas_bin", 0), *marca = data_load("marca_activo_bin", 0);
    int r = JUEGO_ABORTADA;
#ifdef HOST
    dado_semilla(12345UL);                            /* las pruebas de PC tienen que salir siempre igual */
#else
    dado_semilla(plat_ticks());
#endif
    gfx_clear();
    game_draw_background();
    cursor_init(sprites);
    mus_song(1);                                      /* musica de la partida */
    while (plat_keys() & K_FIRE) plat_present();      /* el ESPACIO con el que se llego aqui no cuenta */
    ui_combate_iniciar();
    if (partida_iniciar(mision) && vista_datos(sprites, escenario, vista, baldosas, marca)) r = juego_partida();
    if (r == JUEGO_ACABADA) {                         /* de vuelta al menu: el oro y el equipo se quedan con el heroe y solo siguen los que han salido vivos (0x8FE4) */
        int i;
        for (i = 0; i < 4; i++) {
            Pers *q = &g_part.pers[i];
            heroes[i].activo = (u8)((g_part.salidos >> i) & 1);
            heroes[i].oro = q->oro;
            heroes[i].armas = q->armas;
            heroes[i].armadura = q->armadura;
        }
        cursor_erase();
    }
    plat_snapshot("fin");
    vista_liberar();
    ui_combate_liberar();
    if (sprites) free(sprites);
    if (escenario) free(escenario);
    if (vista) free(vista);
    if (baldosas) free(baldosas);
    if (marca) free(marca);
}
