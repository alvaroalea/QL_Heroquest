/* HeroQuest QL - el combate y la bolsa: ataque (dados, alcance y linea de vista, dano, muerte de un monstruo y su recompensa), panel del personaje (armas,
   armaduras, objetos, pociones y hechizos) y lanzar hechizos.
   Porta de la memoria fija del original (docs/ZX_COMBATE.md): ATACAR (0x7319, la logica), 7462 y 74FC (alcance), 7E74 y 7E7E (linea de vista), 751D, 7559, 7596,
   9B65 (dados), 99A2 y 996B (tiradas), 9B04, 9B25 y 9B31 (dano y muerte), 7602-7647 (recompensas), USAR_OBJETO (0x7877, lanzar el hechizo y sus efectos
   0x79CC-0x7B15) y los manejadores del panel (0x7C45-0x7E61, 0x7F6F, 0x7F8D, 0x96EC).
   Es logica: no dibuja ni espera; lo que el jugador debe ver sale en una Salida (accion.h). Verificado contra el original con tools/hq_oraculo_combate.py y
   tests/combate_test.c. */
#ifndef COMBATE_H
#define COMBATE_H
#include "accion.h"

/* resultados de combate_alcance() */
#define ALC_OK       0
#define ALC_BALLESTA 0xFD         /* no se puede usar la ballesta tan cerca */
#define ALC_LEJOS    0xFE         /* el arma no llega */
#define ALC_NO_VE    0xFF         /* no se ve al objetivo */

/* tipos de blanco de un hechizo (Vars.pers_muerto = A9BB) */
#define BLANCO_MONSTRUO 5
#define BLANCO_CASILLA  6

/* mensajes de combate (ademas de los de accion.h) */
#define MSG_BANE_ORC    14        /* GUION_B9E3: ENCUENTRAS LA ESPADA BANE DE ORC */
#define MSG_ESTRELLA    15        /* GUION_BA09: ENCUENTRAS LA ESTRELLA DEL OESTE */
#define MSG_HECHIZO_OK  16        /* GUION_BA8F: EL HECHIZO TIENE EFECTO */
#define MSG_HECHIZO_NO  17        /* GUION_BAAC: EL HECHIZO SE DESVIO */
#define MSG_HECHIZO_1   18        /* GUION_BAC6: EL HECHIZO INFLIGE 1 PUNTO */
#define MSG_HECHIZO_N   19        /* GUION_BAEA: HECHIZO INFLIGE n PUNTOS (a = n) */
#define MSG_AGUA_SANTA  20        /* GUION_BA2F (a = monstruos destruidos) o BA6A si a = 0 */
#define MSG_TRAMPA_OFF  21        /* GUION_B968: DESACTIVAS LA TRAMPA */
#define MSG_TRAMPA_ON   22        /* GUION_B982: LA TRAMPA SE ACTIVA, PIERDES 1 DE TUS PUNTOS */
#define MSG_ORO_MINA    23        /* GUION_B9B7: EL ORO DESAPARECE Y VUELVE A LA MINA */

/* ---- datos y dados */
void combate_datos_monstruo(Partida *p);                    /* 0x751D: dados de ataque y defensa del tipo del monstruo objetivo */
void combate_preparar(Partida *p, int pers);                /* 0x9B65: dados de ataque segun el arma elegida */
void combate_bonos_ataque(Partida *p, int pers);            /* 0x7559: +2 dados con la fuerza y el valor; 0 contra los jefes sin la espada espiritual */
void combate_dados_defensa(Partida *p, int pers);           /* 0x7596 */
extern void (*combate_punto)(int x, int y);               /* si no es NULL, se llama tras encender cada punto de la linea de vista (x, y en puntos del plano) */
int  combate_alcance(Partida *p, int pers, u8 *plano);      /* 0x7462: ALC_*; plano = buffer de 256 x 176 puntos de 1 bit donde se dibuja el plano con marcas y la linea de vista */
int  combate_linea(const Partida *p, u8 *plano, int col1, int fila1, int col2, int fila2);   /* 0x7E7E: 1 si la linea entre dos casillas del plano choca con algo */

/* ---- la ampliacion Return of the Witch Lord */
int  combate_arma_rota(Partida *p, int pers);               /* 0x732A: atacar a un monstruo con el bit 6 de +6 rompe el arma y el objeto en uso; devuelve 1 si se ha roto algo */

/* ---- tiradas */
void combate_ataque(Partida *p, int pers, Salida *o);       /* 0x99A2: el heroe ataca al monstruo objetivo: golpes, escudos y, si hay mas golpes, muere */
int  combate_defensa(Partida *p, int heroe, int monstruo, Salida *o);   /* 0x996B y 0x9B04: el monstruo (1 a 25) ataca al heroe; devuelve los puntos de cuerpo que pierde */
void monstruo_muere(Partida *p, int num, int pers, Salida *o);          /* 0x9B31 y la rutina de su tipo (0x7602-0x7647) */
int  combate_cuenta_calaveras(int dados);                   /* 0x7B1C */

/* ---- el panel del personaje (la bolsa): cada una recibe la fila (desde la primera de la zona) en que se ha pulsado */
int  panel_item(int mascara, int fila);                     /* 0x7F6F: el bit de la fila-esima de las que estan a uno, 0 si no hay */
int  panel_item16(int mascara, int fila);                   /* 0x7F8D */
int  panel_numero_hechizo(int mascara);                     /* 0x96EC: hechizo (1 a 12) de un bit de +0x0B/+0x0C */
int  panel_mascara_hechizo(int hechizo);                    /* 0x96AC: el bit de un hechizo en +0x0B y +0x0C (word) */
void panel_efecto(Partida *p, int pers, int fila, Salida *o);      /* 0x7D6F: usar una pocion */
void panel_objeto(Partida *p, int pers, int fila);          /* 0x7CE2 */
void panel_arma(Partida *p, int pers, int fila);            /* 0x7C45 */
void panel_arma_sola(Partida *p, int pers, int fila);       /* 0x7CC7 */
void panel_armadura(Partida *p, int pers, int fila);        /* 0x7C6B */
int  panel_hechizo(Partida *p, int pers, int fila);         /* 0x7D1F: elegir un hechizo (1 si se ha elegido); gasta la accion del turno */
void panel_caja(Partida *p, int pers, Salida *o);           /* 0x7E53 y 0x9FF7: la caja del enano desactiva las trampas de al lado (el enano sin tirar dado) */
void panel_soltar_oro(Partida *p, int pers, Salida *o);            /* 0x7E61: el que lleva el oro de Magnus lo deja caer */
int  hechizo_inmune(const Partida *p);                      /* 0x7925: los jefes no sufren hechizos (1 si el monstruo objetivo es inmune) */

/* ---- lanzar un hechizo: antes hay que haber puesto Vars.pers_muerto (tipo de blanco), Vars.objetivo y, si el blanco es una casilla, Vars.casilla_cursor */
void hechizo_lanzar(Partida *p, int pers, u8 *plano, Salida *o);          /* 0x7943 y los efectos */
#endif
