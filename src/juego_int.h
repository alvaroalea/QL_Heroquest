/* HeroQuest QL - lo que comparten juego.c (el bucle) y juego_combate.c (ataque y bolsa). */
#ifndef JUEGO_INT_H
#define JUEGO_INT_H
#include "juego.h"
#include "accion.h"

#define UI_FILA(n) (UI_ROW0 + (n))

void jv_esperar(int cuadros);                       /* 0x9D66 */
void jv_soltar(void);                               /* 0x8DAA: espera a que se suelte el boton */
void jv_boton(int cuadros);                         /* espera que el boton salta */
void jv_panel_movimiento(void);                     /* 0x9242 */
void jv_mostrar_salida(int pers, const Salida *o);  /* los mensajes de una accion */
extern int jv_fondo_ok, jv_ventanas;
void jv_mensajes(const Salida *o);                  /* los mensajes sin redibujar antes */
void jv_info_cursor(void);                          /* 0x7733: nombre bajo el cursor y variables de la seleccion */
void jv_guion_cifra(int id, int n, int cifra);      /* linea n de un guion con su primera cifra cambiada */
int  jv_entrada(u16 *antes, int *movido);           /* un fotograma: mueve el cursor y devuelve 1 si se acaba de pulsar el boton, 2 si es ESC; *movido = 1 si el cursor ha cambiado de sitio */

int ui_atacar(int pers);                           /* 0x7319 */
int ui_bolsa(int pers);                            /* 0x7877 */
void ui_hechizo(int pers);                          /* 0x78B4: el hechizo elegido en Pers.hechizo (objetivo, lanzarlo y mensajes) */
void ui_defensa(int heroe, int monstruo);           /* 0x9921: el monstruo ataca al heroe (pantalla de combate) */
void ui_hooks(void);                                /* acciones de misiones: preguntas, ataques de emboscada y salas especiales */
void ui_turno_monstruos(void);                      /* 0x9CEA: cada monstruo anda y ataca (el gancho juego_turno_monstruos) */
void ui_combate_iniciar(void);                      /* carga los retratos */
void ui_combate_liberar(void);
#endif
