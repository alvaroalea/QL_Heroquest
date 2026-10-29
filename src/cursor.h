/* HeroQuest QL - el cursor de la vista del juego (MOVER_CURSOR, CURSOR_A_CASILLA y DIBUJAR_CURSOR del original).
   X e Y son las coordenadas del original: X centro del sprite (0x10 a 0xFC, de 4 en 4) e Y la linea de sus pies en el buffer de la
   vista (0x0A a 0xAE, de 2 en 2); el sprite de 24 x 8 ocupa X-12..X+11 e Y-8..Y-1. */
#ifndef CURSOR_H
#define CURSOR_H
#include "data.h"

extern int cur_x, cur_y;                 /* posicion del cursor (AA69 y AA6A del original) */

void cursor_init(const u8 *sprites);     /* centro de la vista; sprites = sprites_bin (el cursor es el grafico 66) */
void cursor_move(u16 keys);              /* un fotograma de movimiento con las teclas pulsadas (K_*); acelera tras 6 fotogramas */
int  cursor_dibujado(void);              /* 1 si el cursor esta dibujado ahora mismo en pantalla */
void cursor_draw(void);                  /* guarda el fondo y dibuja el cursor */
void cursor_erase(void);                 /* devuelve el fondo guardado (hacerlo antes de dibujar otra cosa debajo) */
#endif
