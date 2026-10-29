/* HeroQuest QL - estado del juego que comparten los menus y la partida */
#ifndef GAME_H
#define GAME_H
#include "text.h"
#include "partida.h"

typedef struct {
    u8  activo;           /* 1 = el heroe juega */
    u8  nombre[8];        /* nombre de 6 letras como mucho (codigos del juego) */
    u8  nlen;
    u16 oro;
    u8  armas;            /* bits como el byte +0x0E del registro del original: baston, daga, espada, gran hacha, ballesta, hacha, lanza */
    u8  armadura;         /* byte +0x0F: escudo, casco, malla, armadura (bits 0 a 3) y caja (bit 7) */
    u8  grupos;           /* grupos de hechizos elegidos (bit 0 aire, 1 fuego, 2 tierra, 3 agua) */
} Heroe;

extern Heroe heroes[4];           /* 0 barbaro, 1 enano, 2 elfo ("duende"), 3 hechicero */
extern int g_mision;              /* 0 a 13 */
extern int g_manuscrita;          /* 1 = fuente manuscrita */
extern int g_musica;              /* 1 = musica activada (de momento solo la marca) */
extern int g_raton;               /* tipo de raton (RAT_* de platform.h); RAT_CURSORES = las teclas de cursor tambien mueven */

void game_init(void);             /* heroes por omisión */
void game_run(int mision);        /* la partida (de momento un aviso y vuelta al menu) */
void game_draw_background(void);  /* marco de la pantalla de juego */

int  menu_idioma(void);           /* primera pantalla: idioma de las misiones; 0 si se sale del programa */
void menu_principal(void);        /* bucle del menu inicial; vuelve solo si se sale del programa */
#endif
