/* HeroQuest QL - la vista isometrica de la sala, las figuras y el plano de la mazmorra.
   Porta de la memoria fija del original (ver docs/ZX_VISTA.md): ENTRAR EN SALA (0x8923), CALCULAR_VISIBLES (0x8712) con sus rutinas
   (0x875F, 0x8B64 suelo, 0x8A7F paredes y puertas, 0x8A40, 0x87F4 objetos, 0x98F6), ACTUALIZAR_HEROES (0x8791, 0x8773), CALCULAR_VISTA
   (0xA2E1), DESCUBRIR_MONSTRUOS (0xA2C1), ORDENAR_FIGURAS (0xA4FD), DIBUJAR_FIGURAS (0xA534), GUARDAR/RESTAURAR_FONDOS (0xA794, 0xA8DE),
   DIBUJAR_MAPA y DIBUJAR_PLANO (0x92A0, 0x931D, 0x94F8).
   Los graficos salen de sprites_bin, escenario_bin y baldosas_bin; la geometria (posiciones de las casillas, salas, desplazamientos) de
   vista_bin (tools/hq_vista_empaquetar.py). Coordenadas de la vista: las del original, 256 x 176 puntos, que van a la pantalla del QL
   desde la linea VY0. */
#ifndef VISTA_H
#define VISTA_H
#include "partida.h"
#include "data.h"
#include "text.h"

#define N_ANIM 14                 /* registros de animacion (0x6A8E): 5 personajes (0 a 3 heroes, 13 el quinto) y las figuras de la sala */
#define VISTA_ANCHO 256
#define VISTA_ALTO 176
#define VY0 (GY0 + 16)            /* linea de la pantalla del QL de la linea 0 de la vista (fila 2 de celdas) */
#define PLANO_COL0 3              /* columna de celdas de la casilla (0, 0) en el plano */
#define PLANO_FIL0 1

typedef struct {
    u8  estado;                   /* IX+0: 0 libre, 1 figura, 2 heroe animado */
    u8  clave;                    /* IX+1: puesto en la tabla de orden de dibujo (casilla * 3 + 0 a 2) */
    u8  tipo;                     /* IX+6: 0 a 5, como dibujan las rutinas de 0xA572; 3 = heroe (sin alinear a byte) */
    int x, y;                     /* IX+2, IX+3: centro en X y linea de los pies en Y */
    int w, h;                     /* IX+4, IX+5 */
    u8  guion, paso, heroe;       /* animacion de un paso (estado 2): guion 0 a 3 (0xC1B6), fotograma en curso y personaje */
    u8  pack;                     /* 0 = sprites_bin, 1 = escenario_bin */
    int idx;                      /* grafico dentro del fichero */
    int fx, fy, fwpr, frows;      /* fondo guardado: rectangulo de pantalla (x, linea, palabras de ancho, lineas) */
    u8 *fondo;
    u8  dib;                      /* dibujada en pantalla; dpack, didx, dx y dy: lo que se dibujo (vista_figuras_refrescar) */
    u8  dpack;
    int didx, dx, dy;
} Anim;

extern Anim vz_anim[N_ANIM];
void vista_figuras_refrescar(void);   /* tras avanzar la animacion: repinta solo lo que cambia, compuesto fuera de la pantalla y volcado de una vez (sin parpadeo) */
extern u8   vz_orden[96];         /* 0xFD00: numero de registro por puesto de dibujo, 0xFF = libre */

/* gfx de los ficheros cargados (los pone vista_datos) */
int  vista_datos(const u8 *sprites, const u8 *escenario, const u8 *vista, const u8 *baldosas, const u8 *marca);   /* 0 si faltan o no valen */
void vista_liberar(void);
int  vista_sprite(int idx, gimg *g);                        /* grafico idx de sprites_bin; 0 si no existe */
extern int vista_error;          /* por que ha fallado vista_datos (1 faltan ficheros, 2 vista_bin no vale, 20 + n la seccion n, 3 sin memoria) */

void vista_borrar_animaciones(void);                       /* 0x9BE3 */
void vista_fondo(int ancla);                               /* marco de la vista con el color de la sala (0xC528) */
int  vista_sala_de(const Partida *p, int col, int fila);   /* 0x89AD: sala de una casilla */
void vista_entrar_sala(Partida *p, int sala);              /* 0x8923: origen de la ventana, color y primera visita */
void vista_calcular_visibles(Partida *p, int pers);        /* 0x8712: suelo y paredes de la sala del personaje y registros de las figuras */
void vista_actualizar_heroes(Partida *p);                  /* 0x8791 */
void vista_calcular_vista(Partida *p, int pers);           /* 0xA2E1: marca las casillas que ve el personaje */
void vista_descubrir_monstruos(Partida *p);                /* 0xA2C1 */
void vista_construir(Partida *p, int pers);                /* 0x6E9C: fondo, borrar figuras, visibles y heroes */
int  vista_izquierda(const Anim *a);                       /* punto de la izquierda con que se dibuja una figura */
void vista_ordenar(void);                                  /* 0xA4FD */
void vista_figuras_dibujar(void);                          /* 0xA794 + 0xA534: guarda el fondo bajo cada figura y las dibuja por orden */
void vista_figuras_borrar(void);                           /* 0xA8DE: devuelve los fondos */

/* movimiento de los heroes */
int  vista_tile_de(const Partida *p, int col, int fila);   /* 0x8773: casilla de la ventana (0 a 29) en la que esta una casilla del mapa, -1 si queda fuera */
void vista_iniciar_paso(Partida *p, int pers, int guion);  /* 0x85D9: empieza a animar el paso del personaje en la direccion guion */
void vista_animar(Partida *p);                             /* un fotograma de la animacion de los registros en estado 2 (0xA7C9) */
void vista_animar_clave(Partida *p, int pers);             /* 0x85F2: puesto en el orden de dibujo del heroe que se mueve */
int  vista_animando(void);
int  vista_guion_offset(int guion, int paso);              /* desplazamiento en bytes del fotograma dentro del guion del original (pruebas) */                                 /* 1 si algun registro esta en estado 2 */
void vista_cursor_a_casilla(const Partida *p, int x, int y, int *col, int *fila);   /* 0x6FF6 (primera parte): casilla del mapa mas cercana al cursor */
int  vista_fotograma_base(int pers, int dir);              /* fotograma del personaje parado tras un paso (0x850F) */
void vista_marcar(Partida *p, int c);                      /* 0x98FB: una casilla vista en el plano y en 3D, y las contiguas en 3D */
int  vista_ventana_off(int t);                             /* 0xB6AC: desplazamiento en el mapa de la casilla t (0 a 29) de la ventana */
int  vista_arma(int i);                                    /* 0xAA81: byte del arma i (0 lanza ... 6 baston) */
void vista_contacto(int i, int *dx, int *dy);              /* 0x81C0: las ocho casillas de al lado */
const int *vista_panel_zona(int i);                        /* 0x7FAE: columna 1, columna 2, fila 1 y fila 2 de la zona i del panel del personaje */
int  vista_panel_mascara(int i);                           /* 0x7FEA */
int  vista_hechizo_blanco(int h);                          /* 0x7B70: a quien se puede lanzar el hechizo h (1 a 12): bit 0 un heroe, 1 un monstruo, 2 una casilla */
int  vista_oro_carta(int c);                               /* 0xB688: monedas de la carta c (0 a 8) */
void vista_pocion(int c, int *bit, int *nombre, int *desc);   /* 0x829B, 0xAE5F: pocion de la carta 9 + c (0 a 6) */
void vista_emboscada(int i, int *dx, int *dy);             /* 0x81C0 */
int  vista_nombre_objeto(int codigo);                      /* 0xB5C3: indice en NOMBRES_OBJETO del nombre de un codigo de objeto (0x30 a 0x5B) */
const int *vista_zonas(void);                              /* 0xB4F0: 10 zonas de clic (columna 1, columna 2, fila 1, fila 2, accion) */
void vista_teleport(int i, int *sala, int *col, int *fila, int *extra);   /* 0x85AD: destino i (0 a 10) de los teletransportes de la mision 9 */

/* plano de la mazmorra: el buffer de 256 x 176 puntos de 1 bit (32 bytes por linea, como el del original) y los atributos de las
   26 x 19 casillas (0x06 = amarillo sobre negro, 0x30 = negro sobre amarillo) */
void vista_plano_buffer(const Partida *p, u8 *buf, u8 *attr);
void vista_plano_dibujar(const Partida *p);                /* limpia la pantalla y pinta el plano (0xA3C8) */
#endif
