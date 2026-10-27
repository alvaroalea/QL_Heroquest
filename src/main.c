/* HeroQuest para el Sinclair QL (modo 8, 256 x 256). Punto de entrada. */
#include "game.h"
#include "qsound.h"
#include "music.h"
#include <stdio.h>

int main(void)
{
    plat_init();
    g_musica = qs_detect();                /* con QSound la musica empieza activada */
    gfx_init();
    if (!data_init()) { printf("\nFaltan ficheros de datos.\n"); plat_wait_key(); plat_shutdown(); return 1; }
    gfx_set_font(fnt_normal);
    game_init();
    if (g_musica && !mus_init(mus_data)) g_musica = 0;     /* sin musica_bin tampoco hay musica */
    mus_fx_init(fx_data);                  /* efectos de sonido: solo suenan con la musica desactivada */
    mus_enable(g_musica);
    mus_song(2);                           /* musica del menu */
#ifdef BENCH
    { extern void bench(void); bench(); }
#endif
    menu_principal();
    mus_enable(0);                         /* para la musica (la interrupcion deja de tocar el chip) y lo silencia */
    if (qs_present()) qs_silence();
    plat_shutdown();
    return 0;
}
