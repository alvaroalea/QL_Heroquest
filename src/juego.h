/* HeroQuest QL - el bucle de la partida: rondas, turno de cada personaje (tirada de movimiento, clic, movimiento), panel de marcadores, plano de la mazmorra y
   turno de Morcar. Porta la parte que dibuja y espera teclas de PARTIDA (0x6D89), TURNO_PERSONAJE (0x6DDD), TIRAR_DADOS (0x6F1C), FIN_DE_RONDA (0x9CE0),
   MOSTRAR_MAPA (0x7710) con INFO_BAJO_CURSOR (0x7733), FIN_ACCION (0x8009), los marcadores (0x926A, 0x91F7, 0x9242, 0x9C5B) y 8FCF; la logica esta en turno.c.
   Docs: docs/ZX_TURNO.md. */
#ifndef JUEGO_H
#define JUEGO_H
#include "turno.h"

/* Resultado de la partida */
#define JUEGO_ACABADA 0           /* la mision ha terminado (todos los heroes han salido o muerto) */
#define JUEGO_ABORTADA 1          /* el jugador ha pulsado ESC */

int  juego_partida(void);         /* PARTIDA (0x6D89): rondas hasta que acabe la mision; JUEGO_* */

/* Ganchos para lo que se ira portando: acciones de los iconos (ACC_OJO, ACC_BOLSA, ACC_ATACAR, ACC_LLAVE), turno de los monstruos y lo que pasa en cada
   mision al salir un heroe por la escalera. Si valen 0 no pasa nada. */
extern int (*juego_accion[10])(int pers);       /* 0 = no ha hecho nada, 1 = hay que redibujar, 2 = ya ha dejado la pantalla redibujada */
extern void (*juego_turno_monstruos)(void);
extern void (*juego_salida_mision)(int pers);

/* partes sueltas, por si las usa game.c o las pruebas */
void juego_panel(int pers);                 /* nombre, cuerpo y mente, oro y puntos de movimiento */
void juego_redibujar(int pers);             /* FIN_ACCION: marco, sala, figuras y marcadores */
void juego_plano(int pers);                 /* MOSTRAR_MAPA: el plano con el cursor y el nombre de lo que hay bajo el, hasta pulsar el boton */
#endif
