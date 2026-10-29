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
    u8 pr[4], *m;
    long n;
    int i, ok;
    m = data_mision_obtener(mision, &n);
    if (!m) return 0;
    for (i = 0; i < 4; i++) pr[i] = heroes[i].activo;
    partida_desde_menu(&g_part);
    ok = partida_cargar(&g_part, mision, m, n, pr);
    data_mision_soltar(m);
    if (ok) { partida_tipos(&g_part, data_tipos(0)); partida_estadisticas(&g_part, data_estadisticas(0)); mazo_barajar_azar(); }
    return ok;
}

/* La mision no ha podido empezar: mensaje (sin memoria, falta el fichero o datos no validos) y espera una tecla; despues se vuelve al menu */
static void game_fallo(int sin_memoria, int falta)
{
    static const char *const msg[2][4] = {
        { "NO HAY MEMORIA SUFICIENTE", "PARA CARGAR LA MISION", "NO SE ENCUENTRA EL FICHERO", "DATOS DE LA MISION NO VALIDOS" },
        { "NOT ENOUGH MEMORY", "TO LOAD THE MISSION", "MISSION FILE NOT FOUND", "INVALID MISSION DATA" } };
    static const char *const tecla[2] = { "PULSA UNA TECLA", "PRESS A KEY" };
    const char *const *m = msg[g_idioma == IDIOMA_EN];
    const char *l[2];
    u8 b[32];
    int i, j, n;
    if (sin_memoria) { l[0] = m[0]; l[1] = m[1]; }
    else if (falta) { l[0] = m[2]; l[1] = ""; }
    else { l[0] = m[3]; l[1] = ""; }
    gfx_clear();
    for (i = 0; i < 2; i++) {
        for (n = 0; l[i][n]; n++) b[n] = (u8)(l[i][n] == ' ' ? 0x5F : l[i][n]);
        if (n) tx_cadena_centrada(9 + i + UI_ROW0, b, n, C_RED, C_BLACK);
    }
    for (n = 0; (j = tecla[g_idioma == IDIOMA_EN][n]) != 0; n++) b[n] = (u8)(j == ' ' ? 0x5F : j);
    tx_cadena_centrada(13 + UI_ROW0, b, n, C_YELLOW, C_BLACK);
    key_reset();
    key_wait();
}

void game_run(int mision)
{
    const u8 *sprites = dat_get(DAT_SPRITES, 0), *escenario = dat_get(DAT_ESCENARIO, 0), *vista = dat_get(DAT_VISTA, 0);
    const u8 *baldosas = dat_get(DAT_BALDOSAS, 0);
    const u8 *marca = fonts_get(FNT_MARCA, 0);
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
    if (!partida_iniciar(mision)) game_fallo(data_error == 2, data_error == 1);
    else if (!vista_datos(sprites, escenario, vista, baldosas, marca)) game_fallo(vista_error == 3, 0);
    else r = juego_partida();
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
}
