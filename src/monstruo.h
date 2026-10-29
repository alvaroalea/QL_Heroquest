/* HeroQuest QL - el turno de los monstruos (turno de Morcar): cada monstruo a la vista busca un heroe de su sala al que llegar, anda hacia el y le ataca.
   Porta de la memoria fija del original (docs/ZX_MONSTRUOS.md): TURNO_MONSTRUO (0x9D6B), CALCULAR_CAMINO (0x9DC0, 0x9E6A-0x9F7C), CASILLA_LIBRE (0x9F94, 0x9FAF)
   y EJECUTAR_CAMINO (0xA075). Es logica: no dibuja ni espera; las pantallas (el plano con el monstruo andando y la de combate) estan en juego_combate.c.
   Verificado contra el original con tools/hq_oraculo_monstruo.py y tests/monstruo_test.c. */
#ifndef MONSTRUO_H
#define MONSTRUO_H
#include "combate.h"

#define MON_NADA    0             /* el monstruo no hace nada */
#define MON_MOVER   1             /* anda hasta quedar junto a un heroe y le ataca */
#define MON_ATACAR  2             /* ya esta junto a un heroe: le ataca */
#define MON_ERRANTE 3             /* Return of the Witch Lord: monstruo con el bit 7 de +6 y sin vida (la niebla de la muerte): recorre su ruta con monstruo_errante_paso() */

#define MSG_NIEBLA  32            /* GUION_B47A: LA NIEBLA DE LA MUERTE ATACA, PIERDES 1 PUNTO CORPORAL (el heroe de la casilla pierde 1 de cuerpo) */

/* Prepara el turno del monstruo num (1 a 25): estados de dormido y tempestad y camino hasta el primer heroe al que llegue. Si hay camino deja el heroe en
   Vars.atacante (0xA9AD) y los pasos en monstruo_pasos(). */
int  monstruo_planear(Partida *p, int num);
int  monstruo_pasos(void);                                  /* 0xA99A: casillas del camino */
void monstruo_paso(Partida *p, int num, int k);             /* 0xA075: el paso k (de 0): mueve al monstruo y su codigo de la capa C */

/* El monstruo errante de la ampliacion: en cada ronda de Morcar da Partida.ruta_pasos pasos de la ruta de la mision (linea errante: tramos con su eje, sentido y
   coordenada final, que se repiten; el original, 0x9C9C-0x9D04, la lleva en el codigo para la mision 2) y tras cada uno golpea al primer heroe que haya en su casilla. */
void monstruo_errante_paso(Partida *p, int num);            /* 0x9A4F-0x9A5F y 0x9CA8-0x9D03: un paso de la ruta; el codigo de la capa C va con el monstruo salvo sobre otro objeto */
int  monstruo_errante_golpe(Partida *p, int num, Salida *o);   /* 0x9D05: el heroe de su casilla pierde 1 de cuerpo (MSG_NIEBLA) o muere; devuelve 1 si ha golpeado */
#endif
