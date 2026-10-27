/* HeroQuest QL - las acciones de los iconos de la pantalla de juego: abrir puertas (llave), buscar trampas y puertas secretas, buscar tesoros
   (mazo de cartas, oro, pociones, trampas, emboscadas), los eventos que dejan las casillas con trampa y la muerte de un heroe.
   Porta de la memoria fija del original (docs/ZX_ACCIONES.md): ABRIR_PUERTA (0x727E, 0x72B5), ABRIR_PUERTA_CASILLA (0x860B), la busqueda en las casillas
   de la sala (0x8662, 0x8688, 0x86F7, 0x8647, 0x862A), BUSCAR_TRAMPAS_PUERTAS (0x8079), BUSCAR_TESOROS (0x80A9), la comprobacion de casillas libres (0x9F94,
   0x9FAF), EVENTO_ALEATORIO (0xA108) y MATAR_PERSONAJE (0x7B31).
   Es logica: no dibuja ni espera. Lo que el jugador debe ver (las ventanas del original) se devuelve en una Salida; juego.c lo muestra. Verificado contra el
   original con tools/hq_oraculo_accion.py y tests/accion_test.c. */
#ifndef ACCION_H
#define ACCION_H
#include "turno.h"

/* mensajes (ventanas y guiones del original) */
#define MSG_NADA        1         /* VENT_AC50: NO VES NADA */
#define MSG_TRAMPA      2         /* VENT_AC66: ACTIVAS UNA TRAMPA, PIERDES PUNTOS */
#define MSG_EMBOSCADA   3         /* VENT_AC90: un monstruo te ha seguido y te ataca */
#define MSG_FANTASMA    4         /* VENT_ACD9 */
#define MSG_FOSO        5         /* VENT_AD1D */
#define MSG_LANZA       6         /* VENT_AD8A */
#define MSG_ORO         7         /* VENT_ADDE: a = monedas */
#define MSG_POCION      8         /* VENT_AE03: a = indice del nombre, b = indice de la descripcion (POCIONES_*) */
#define MSG_REGISTRADA  9         /* VENT_AF65: HABITACION YA HA SIDO REGISTRADA */
#define MSG_ROCA        10        /* VENT_C172: EL TECHO SE CAE ENCIMA DE TI */
#define MSG_ROCA_PUNTO  11        /* VENT_C19C: PIERDES 1 PUNTO */
#define MSG_MUERTE      12        /* GUION_BB18 + a: EL BARBARO HA MUERTO... */
#define MSG_ESPECIAL    13        /* sala con tratamiento propio de su mision: a = tabla (0 trampas, 1 tesoros), b = valor del indicador */
#define MSG_MAX 6

typedef struct { int id, a, b; } Msg;
typedef struct { Msg m[MSG_MAX]; int n; } Salida;

void accion_mensaje(Salida *o, int id, int a, int b);          /* anade un mensaje a la salida */

/* Gancho para las salas especiales de las misiones (indicadores distintos de 0 y 0xFF: cofres, palancas...): lo rellenara el port de las misiones. Si es 0 no pasa nada. */
extern void (*accion_especial)(Partida *p, int pers, int idx, int valor, Salida *o);   /* idx = posicion del indicador en Partida.ind (0 a 42 tesoros, 43 a 85 trampas) */

/* El ataque de un monstruo a un heroe (PANEL_COMBATE 0x9921) cuando lo lanza una accion: la emboscada de la busqueda de tesoros y las de las misiones. Si es 0 no pasa nada. */
extern void (*accion_ataque)(Partida *p, int heroe, int monstruo, Salida *o);

int accion_abrir_puerta(Partida *p, int pers);                 /* 0x72B5: abre las puertas cerradas de la casilla del heroe y las contiguas y deja ver lo que hay detras */
void accion_buscar_trampas(Partida *p, int pers, Salida *o);    /* 0x8079 */
void accion_buscar_tesoros(Partida *p, int pers, Salida *o);    /* 0x80A9 */
void accion_evento(Partida *p, int pers, Salida *o);            /* 0xA108 sin los eventos de las misiones (0x70A1): lo que ha dejado pendiente una casilla (Vars.evento) */
void heroe_muere(Partida *p, int pers);                         /* 0x7B31: el personaje deja la partida y, si es el que juega, termina su turno */
void accion_revelar(Partida *p, int pers, int c);               /* 0x860B: la sala de la casilla c pasa a verse */
int  casilla_libre(const Partida *p, int col, int fila, int sala);   /* 0x9FAF: 0 si esta libre y en la sala; 1 hay un heroe, 2 un objeto o monstruo, 3 otra sala */
#endif
