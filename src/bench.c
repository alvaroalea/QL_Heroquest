/* Banco de pruebas de velocidad (make bench; hay que arrancar sQLux con SPEED = 1 y SQLUX_QSLOG=1): mide en ticks de 50 Hz cuanto tardan
   borrar la pantalla (1), 5 pantallas de caracteres sueltos (2), 5 veces la lista de misiones (3), 5 ventanas (4) y 5 fondos (5), y lo
   saca por la traza de la QSound como "QS 15 etiqueta" y "QS 14 ticks". */
#include "game.h"
#include "qsound.h"
#include "textos_ids.h"

static void medir(int tag, unsigned long t0)
{
    qs_write(15, tag);
    qs_write(14, (int)(plat_ticks() - t0));
}

void bench(void)
{
    unsigned long t;
    int i, c, r;
    plat_present();
    t = plat_ticks(); for (i = 0; i < 10; i++) gfx_clear(); medir(1, t);
    t = plat_ticks(); for (i = 0; i < 5; i++) for (r = 0; r < 24; r++) for (c = 0; c < 32; c++) gfx_char(c, r, (u8)0x41, C_WHITE, C_BLACK); medir(2, t);
    t = plat_ticks(); for (i = 0; i < 5; i++) tx_guion(TX_MENU_689A, -1, 0, 0); medir(3, t);
    t = plat_ticks(); for (i = 0; i < 5; i++) tx_ventana(TX_PORT_FIN_MISION); medir(4, t);
    t = plat_ticks(); for (i = 0; i < 5; i++) game_draw_background(); medir(5, t);
}
