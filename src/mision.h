/* HeroQuest QL - lo que es propio de cada mision: eventos al entrar en ciertas salas (misiones 5, 9 y 11), las salas especiales del ojo (cofres, palancas,
   tesoros: indicadores de 1 a 18) y lo que pasa al salir un heroe por la escalera (misiones 1, 2, 4, 7 y 10).
   Porta de la memoria fija del original (docs/ZX_MISIONES.md): EVENTOS_MISION (0x70A1), las rutinas de la tabla 0x82A2 (0x9005-0x91F4) y las de la tabla
   0x8FE9 (0x8EBB-0x8FD2). Es logica: no dibuja ni espera; los mensajes salen en una Salida (MSG_VENT: una ventana de textos_bin) y las preguntas de si o no
   se hacen con mision_pregunta. Verificado contra el original con tools/hq_oraculo_mision.py y tests/mision_test.c. */
#ifndef MISION_H
#define MISION_H
#include "combate.h"

#define MSG_VENT        30        /* una ventana de mensaje: a = su numero en textos_bin (TX_VENT_xxxx) */
#define MSG_ORO_PANEL   31        /* se ha cambiado el oro del heroe (0x91F7): hay que pintarlo */

/* Pregunta de si o no (0x8D5E): ventana a (TX_VENT_xxxx). Antes de mostrarla hay que mostrar los mensajes que ya hay en o (la rutina del juego los vacia). 1 = si. */
extern int (*mision_pregunta)(Salida *o, int ventana);

void mision_evento(Partida *p, int pers, Salida *o);                    /* 0x70A1: eventos de las misiones 5, 9 y 11 (con Vars.evento_mision a 1 al entrar en la sala) */
void mision_especial(Partida *p, int pers, int idx, int valor, Salida *o);   /* 0x827A: la sala especial de un indicador (el gancho accion_especial) */
void mision_salida(Partida *p, int pers, Salida *o);                    /* 0x8EAF: lo que pasa al salir un heroe por la escalera, segun la mision */
#endif
