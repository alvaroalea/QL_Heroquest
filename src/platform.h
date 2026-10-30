/* HeroQuest QL - capa de plataforma (C89, apto para XTC68/qcc, donde int puede ser de 32 o 16 bits).
   Pantalla = formato nativo QL modo 8 (32 KB, 256 x 256, 128 bytes por linea). La imagen del Spectrum (256 x 192) se coloca
   centrada: 32 lineas negras arriba y 32 abajo (docs/DECISIONES.md). */
#ifndef PLATFORM_H
#define PLATFORM_H

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

/* Rutinas portadas que el juego no llama (mus_fade, mus_stop...): con NO_COMP_UNUSED definido no se compilan.
   Quitar esta linea para volver a compilarlas. */
#define NO_COMP_UNUSED

#define SCR_W 256
#define SCR_H 256
#define SCR_STRIDE 128
#define SCR_BYTES 32768L          /* no cabe en un int de 16 bits: siempre long */
#define GY0 32                    /* primera linea de la imagen de 192 lineas del Spectrum */
#define GCOLS 32                  /* celdas de texto de 8 x 8 en horizontal y en vertical (imagen del Spectrum) */
#define GROWS 24

/* Colores QL: bit2 = verde, bit1 = rojo, bit0 = azul (igual que la numeracion de los atributos del Spectrum) */
#define C_BLACK   0
#define C_BLUE    1
#define C_RED     2
#define C_MAGENTA 3
#define C_GREEN   4
#define C_CYAN    5
#define C_YELLOW  6
#define C_WHITE   7

/* Controles: estado de las teclas (varios bits a la vez) */
#define K_UP    0x0001
#define K_DOWN  0x0002
#define K_LEFT  0x0004
#define K_RIGHT 0x0008
#define K_FIRE  0x0010
#define K_ESC   0x0020
#define K_0     0x0040
#define K_1     0x0080
#define K_2     0x0100
#define K_3     0x0200
#define K_M     0x0400            /* alterna musica, efectos y silencio durante la partida */
#define K_YES   0x1000            /* Y o S (confirmar en ingles o en espanol); como K_4, solo se lee con plat_tecla4 */
#define K_4     0x0800            /* solo se lee si plat_tecla4 (el menu de controles): cada fila de KEYROW es una llamada lenta */

/* Tipo de raton (menu de controles). Las teclas O K Z X ESPACIO funcionan siempre; los cursores solo con RAT_CURSORES */
#define RAT_CURSORES 0
#define RAT_QIMI     1            /* interfaz QIMI: botones en $1BF9C y movimiento por interrupciones externas */
#define RAT_QIMSI    2            /* interfaz PS/2 del puerto de ROM (registros $FEDD a $FEDF) */
#define RAT_PE       3            /* Pointer Environment: IOP.RPTR sobre la consola */
extern u8 plat_raton;
int  plat_raton_auto(void);       /* tipo de raton con el que empezar: RAT_QIMSI si se detecta la placa, si no RAT_CURSORES */
int  plat_raton_ok(int modo);     /* 1 si el modo esta implementado y su hardware detectado */
int  plat_raton_estado(void);      /* diagnostico: byte crudo del registro de estado de la QIMSI ($FEDF); PC: 0 */
void plat_raton_delta(int *dx, int *dy);   /* movimiento acumulado desde la llamada anterior (+x derecha, +y abajo, en cuentas del raton); lo pone a 0 */

extern u8 plat_tecla4;            /* 1 = plat_keys lee tambien la tecla 4 (en el QL cuesta una lectura mas de la matriz) */
extern u8 *screen;                /* pantalla en formato QL modo 8 */

void plat_init(void);
void plat_pantalla_carga(void);   /* QL: vuelve a poner la pantalla de carga (heroquest_scr) que plat_init borra; PC: nada */
void plat_shutdown(void);         /* idempotente; en el QL se registra con atexit */
void plat_exit(void);             /* QL: termina el programa sin el mensaje "Press a key to exit" (MT.FRJOB); PC: exit(0). Es la salida normal de main */
unsigned long plat_ticks(void);   /* cuadros de 50 Hz desde el arranque */
int  plat_present(void);          /* una vez por fotograma: espera al siguiente cuadro de 50 Hz, vacia la cola de teclado de la consola y avanza la musica; devuelve los cuadros transcurridos (1 si el bucle va a 50 Hz, mas si tarda) */
u16  plat_keys(void);             /* teclas pulsadas ahora (bits K_*) */
void plat_flush_input(void);
void plat_wait_key(void);         /* espera una pulsacion (suelta, pulsa, suelta) */
int  plat_default_dir(int prog, char *buf, int max);   /* QL: directorio por defecto de Toolkit II (prog = 0: DATA_USE, 1: PROG_USE); 1 si hay */
int  plat_listar(const char *sufijo, char nombres[][16], int max);   /* nombres de los ficheros de la unidad de datos que acaban en sufijo (sin el sufijo, en minusculas, 15 caracteres como mucho); como mucho max; devuelve cuantos */
const char *plat_data_dir(void);  /* prefijo de los ficheros de datos: QL "flp1_", PC "data/" */
void plat_snapshot(const char *name);   /* PC: guarda la pantalla en un fichero (pruebas); QL: no hace nada */

#endif
