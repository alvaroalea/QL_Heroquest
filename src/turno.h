/* HeroQuest QL - el turno de un personaje: tirada de movimiento, clic, movimiento casilla a casilla y camino automatico.
   Porta de la memoria fija del original (ver docs/ZX_TURNO.md): TURNO_PERSONAJE (0x6DDD) sin los dibujos, CALCULAR_DADOS y TIRAR_DADOS (0x6ECD, 0x6F1C),
   MOVER_AUTOMATICO (0x6FDA), IR_HACIA_CURSOR (0x6FF6), PROCESAR_CLIC (0x7201), PASAR_TURNO y TERMINAR_TURNO (0x725E, 0x7277), EN_ZONA_ESPECIAL (0x7281),
   MOVER_X_MENOS, MOVER_X_MAS, MOVER_Y_MENOS y MOVER_Y_MAS (0x82CB, 0x8357, 0x83E3, 0x8478), FIN_PASO (0x850F), el paso de los pisos con objetos (0xA0E6),
   la salida de la mazmorra (0x8E7B, solo la parte comun), FIN_MOVIMIENTO (0xA23E) y la cuenta del turno de Morcar (0x9CE0, sin los monstruos).
   Todo esto es logica del juego: no dibuja nada salvo lo que hace vista.c al empezar un paso. Los dibujos, los textos y la espera de teclas
   los hace game.c. Verificado contra el original con tools/hq_oraculo_turno.py y tests/turno_test.c. */
#ifndef TURNO_H
#define TURNO_H
#include "vista.h"

/* acciones de las zonas de clic de la pantalla de juego (tabla 0xB4F0) */
#define ACC_X_MENOS 0
#define ACC_X_MAS   1
#define ACC_Y_MENOS 2
#define ACC_Y_MAS   3
#define ACC_OJO     4             /* revisar: buscar trampas, puertas secretas y tesoros */
#define ACC_PASAR   5             /* siguiente personaje */
#define ACC_MAPA    6
#define ACC_BOLSA   7
#define ACC_ATACAR  8
#define ACC_LLAVE   9             /* abrir una puerta */

/* resultado de un paso (turno_paso) */
#define PASO_NO     0             /* no se ha movido */
#define PASO_SI     1
#define PASO_SALIDA 2             /* el heroe ha llegado a una escalera de salida y ha dejado de estar en la mision */

/* estado del personaje al empezar su turno (turno_estado): mascara de las ventanas que hay que mostrar; con cualquiera pierde el turno, salvo 0 */
#define TURNO_JUEGA        0
#define TURNO_VENT_SUENO   1      /* ESTAS DURMIENDO / PIERDES TU TURNO (VENT_AFBE) */
#define TURNO_VENT_TORMENTA 2     /* TE VES ENVUELTO POR UNA TEMPESTAD / PIERDES TU TURNO (VENT_AF87) */
#define TURNO_DESPIERTA    4      /* sin ventanas: se ha despertado y juega */

void turno_empezar(Partida *p, int pers);                  /* 0x6DDD: variables del turno a cero */
int  turno_estado(Partida *p, int pers);                   /* 0x6E04: dormido o envuelto en una tormenta; tira el dado para despertar */
void turno_calcular_dados(Partida *p, int pers);           /* 0x6ECD (dados_mov_calcular) */
void turno_dado_fotograma(Partida *p, int pers, int n);    /* una vuelta del bucle de TIRAR_DADOS: el heroe parpadea y la suma cambia */
void turno_dados_fin(Partida *p, int pers);                /* 0x6F8A: el registro del heroe queda como figura normal */
int  turno_paso(Partida *p, int pers, int dir);            /* 0x82CB, 0x8357, 0x83E3, 0x8478: un paso (ACC_X_MENOS a ACC_Y_MAS); PASO_* */
int  turno_clic(Partida *p, int pers, int cur_x, int cur_y);   /* 0x7201: el boton se ha pulsado con el cursor ahi; -1 no hace nada, si no la accion (ACC_*) */
void turno_automatico(Partida *p, int pers);               /* 0x6FDA: el siguiente paso del camino al cursor */
void turno_fotograma(Partida *p, int pers);                /* lo que hace cada fotograma del bucle del turno despues del clic: 0x85F2 y el avance de la animacion */
void turno_pasar(Partida *p, int pers);                    /* 0x725E: siguiente personaje */
void turno_terminar(Partida *p);                           /* 0x7277 */
void turno_fin_movimiento(Partida *p);                     /* 0xA23E: no puede moverse mas este turno */
int  turno_continua(const Partida *p);                     /* 1 mientras el personaje siga con su turno (A98A = 0) */
void turno_fin_de_ronda(Partida *p);                       /* 0x9D35: al terminar la ronda se quita el bit 0 de +0x18 de los personajes */

extern void (*turno_evento_salida)(Partida *p, int pers);  /* 0x8E7B: lo que pasa en cada mision al llegar a la salida (se rellenara al portar las misiones) */
#endif
