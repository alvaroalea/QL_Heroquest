/* HeroQuest QL - estado de la partida: mision cargada, personajes, monstruos, mazo de tesoros y dados.
   Porta INICIAR_MISION (0x5B3F), el registro de personaje (26 bytes desde 0xA9E5), la lista de monstruos (10 bytes desde 0x6C0C),
   las capas del mapa (0x6200, 0x6400, 0x6600), el mazo de cartas (0x9542, 0x9523, 0x9588), los dados (0x9BD3, 0x9BC8) y los dados
   de movimiento (0x6ECD, 0x6F1C). Verificado contra el original con tools/hq_oraculo_partida.py y tests/partida_test.c.
   No depende de la pantalla ni de la plataforma. */
#ifndef PARTIDA_H
#define PARTIDA_H

#include "platform.h"

#define MAP_W 26                  /* casillas por fila y filas del mapa */
#define MAP_H 19
#define MAP_N 494                 /* MAP_W * MAP_H */
#define MAP_P 512                 /* cada capa ocupa 0x200 bytes en el original (a partir de 494 es relleno a cero): la vista lee algunas casillas pasada la 494 */
#define N_SALAS 64
#define N_MONS 32
#define N_PERS 5                  /* 4 heroes y el quinto personaje (Sir Ragnar en la mision 1) */
#define N_IND 86
#define MAZO_N 25
#define MIS_N 14

/* Registro de personaje (26 bytes desde 0xA9E5; los desplazamientos del original van entre parentesis). */
typedef struct {
    u8  estado;                   /* (+0) 0 = no existe, 0xFF = en juego */
    u8  col, fila, hab;           /* (+1, +2, +3) casilla y habitacion */
    u16 casilla;                  /* (+4, +5) direccion de la casilla en la capa B: la calcula la vista, 0 al cargar la mision */
    u8  cuerpo, mente;            /* (+6, +7) */
    u8  fotograma;                /* (+8) orientacion y sprite */
    u16 oro;                      /* (+9, +10) */
    u8  hechizos[2];              /* (+0x0B, +0x0C) bits de hechizo: [0] bits 0-2 agua, 3-5 aire; [1] bits 0-2 fuego, 3-5 tierra */
    u8  objetos;                  /* (+0x0D) */
    u8  armas;                    /* (+0x0E) baston, daga, espada, gran hacha, ballesta, hacha, lanza */
    u8  armadura;                 /* (+0x0F) bits 0-3 escudo, casco, malla, armadura; 4-6 estados; 7 caja (enano: busca trampas sin coste) */
    u8  efectos;                  /* (+0x10) */
    u8  r11, r12, r13;            /* (+0x11 a +0x13) bit 3 de r13: un dado de movimiento menos */
    u8  ataque, defensa;          /* (+0x14, +0x15) dados base */
    u8  hechizo;                  /* (+0x16) hechizo elegido */
    u8  t17, t18;                 /* (+0x17, +0x18) estados temporales (bit 5 de t17 y bit 3 de t18: movimiento doble) */
    u8  indice;                   /* (+0x19) 0 a 4 */
} Pers;

/* Registro de monstruo (10 bytes desde 0x6C0C). */
typedef struct {
    u8  estado;                   /* (+0) 0 = hueco vacio, 0x81 = oculto */
    u8  col, fila, hab;           /* (+1, +2, +3) */
    u16 casilla;                  /* (+4, +5) indice de la casilla en las capas (el original guarda la direccion 0x6200 + indice) */
    u8  cuerpo;                   /* (+6) puntos de cuerpo y bit 0 = visible */
    u8  b7;                       /* (+7) dos nibbles que muestra el panel */
    u8  tipo;                     /* (+8) nombre: 0 duende, 1 orco, 2 fimir, 3 guerrero, 4 esqueleto, 5 zombi, 6 momia, 7 hechicero, 8 Ulag, 9 Grak, 10 Balur, 11 gran brujo */
    u8  estad;                    /* (+9) indice en la tabla de atributos (movimiento, ataque, defensa) */
    u8  tramo;                    /* (no esta en el original) tramo de la ruta de la mision (Partida.ruta) por el que va un monstruo errante (0x9D04) */
} Mons;

/* Variables del juego que borra INICIAR_MISION (0xA983 a 0xA9D6). Se anaden aqui segun se vayan portando. */
typedef struct {
    u8 hab_actual;                /* 0xA984 sala del personaje activo */
    u8 ancla;                     /* 0xA985 byte de atributo del Spectrum con el color de la sala (el paper es el color del suelo) */
    u8 origen_x, origen_y;        /* 0xA9D3, 0xA9D4 casilla del mapa de la esquina de la ventana de 6 x 5 de la vista */
    u8 redibujar;                 /* 0xA987 */
    u8 anim_fin;                  /* 0xA988 2 = un paso del heroe se esta animando */
    u8 hab_registrada;            /* 0xA9A4 bit 0 = la sala ya se ha registrado en busca de trampas o tesoros */
    u8 pasos;                     /* 0xA9B9 casillas que le quedan al movimiento automatico */
    u8 mover;                     /* 0xA9BD paso del movimiento automatico: 0 ninguno, 1 a 4 = x-1, x+1, y-1, y+1 */
    u16 ult_casilla;              /* 0xA9BF direccion (capa B) de la ultima puerta que ha cruzado el personaje en este turno */
    u8 zona;                      /* 0xA98D resultado de una pantalla de eleccion: 0 nada, 1 aceptar, 0xFF cancelar */
    u8 obj_mons;                  /* 0xA99D numero de monstruo (1 a 25) bajo el cursor */
    u8 ataque_posible;            /* 0xA99E */
    u8 arma_alcance;              /* 0xA99F 1 = se elige objetivo de un hechizo */
    u8 elegido;                   /* 0xA9A0 se ha elegido un hechizo o un arma al cerrar el panel */
    u8 flag_ataque;               /* 0xA9A2 */
    u8 vis;                       /* 0xA9A3 bit 0 = a la vista, bits 1 y 2 = al lado (en cruz o en diagonal) */
    u8 heroe_cursor;              /* 0xA9A6 heroe bajo el cursor (0xFF ninguno) */
    u8 bono_arma, dados_ataque, dados_defensa, ataque_base, defensa_base;   /* 0xA9A8 a 0xA9AC */
    u8 golpes, escudos;           /* 0xA9C3, 0xA9C4 calaveras del que ataca y escudos del que se defiende */
    u8 objetivo;                  /* 0xA9C5 monstruo (1 a 25, como en la capa C) o 0x80 + heroe */
    u8 resultado;                 /* 0xA999 */
    u8 hechizo_flags;             /* 0xA9BA a quien se puede lanzar el hechizo elegido: bit 0 heroe, 1 monstruo, 2 casilla */
    u8 casilla_cursor_ok;         /* 1 si casilla_cursor vale (0xA9C1 tiene la casilla bajo el cursor) */
    u16 casilla_cursor;           /* 0xA9C1 casilla del mapa bajo el cursor (capa C) */
    u8 cont_evento;               /* 0xA9AE heroes que han salido en la mision 1 (el primero cobra) */
    u8 cont_tesoro;               /* 0xA9AF cuenta de los tesoros especiales de la mision 3 (y de los viajes por oro) */
    u8 cont_llave;                /* llaves de Agrain encontradas (sala especial 24 de la ampliacion; el original lo guarda en su codigo, 0x8E96) */
    u8 epilogo;                   /* 0x9837 una muerte con la rutina MUERTE_EPILOGO pide mostrar el epilogo al acabar el turno */
    u8 mask_heroes;               /* 0xA9B2 heroes que ya han recuperado el equipo en la mision 5 */
    u8 flag_b4;                   /* 0xA9B4 el aviso de la mision 11 ya se ha dado */
    u8 sonido_pend;               /* 0x727D efecto de sonido pendiente (efecto + 1): 1 paso, 9 puerta; lo toca el bucle del turno */
    u8 camino_n;                  /* 0xA99A casillas del camino de un monstruo */
    u8 hallado;                   /* 0xA9BC la busqueda ha encontrado algo (trampas, puertas secretas) */
    u8 mons_vista;                /* 0xA9B8 hay monstruos a la vista en la sala (no se puede buscar tesoros) */
    u8 atacante;                  /* 0xA9AD personaje que no cuenta al mirar si una casilla esta libre */
    u8 evento;                    /* 0xA9B3 evento pendiente por pisar un objeto (1 a 4) */
    u8 no_dibujar;                /* 0xA989 distinto de 0 = no guardar ni dibujar las figuras (se esta repintando) */
    u8 sala_nueva;                /* 0xA98E 0xFF = se acaba de entrar por primera vez en la sala */
    u8 marcas;               /* 0xA9A1 1 = el plano solo dibuja al personaje activo */
    u8 heroe_evento;              /* 0xA9B0 heroe que activo el evento de la sala */
    u8 evento_mision;             /* 0xA9B1 1 = evento especial de la mision pendiente */
    u8 turno_acabado;             /* 0xA98A distinto de 0 = el turno del personaje ha terminado */
    u8 estado_turno;              /* 0xA98B bit 0 ya se ha movido, 1 ya ha actuado, 2 no puede hacer mas */
    u8 pm;                        /* 0xA98C puntos de movimiento que le quedan */
    u8 pers_actual;               /* 0xA998 0 a 3 heroes, 4 turno de Morcar */
    u8 dados_mov;                 /* 0xA9B5 numero de dados de movimiento */
    u8 ausentes;                  /* 0xA9B6 mascara de heroes que no participan */
    u8 muertos;                   /* 0xA9B7 mascara de heroes muertos o ausentes (0x0F = todos) */
    u8 pers_muerto;               /* 0xA9BB */
} Vars;

/* Rutina que se ejecuta al morir un monstruo segun su estadistica (Mons.estad; tabla 0xA76D del original, un puntero por entrada). La mision elige cual
   para cada estadistica con lineas 'muerte' (docs/ZX_DATOS_QL.md); las que no nombra usan la normal. */
#define N_ESTAD        20
#define N_MUERTE_CEL   32
#define MUERTE_NORMAL  0          /* 0x96CD: la recompensa de su tipo */
#define MUERTE_SALIDA  1          /* 0x9813: borra casillas (nb) y pone la escalera de salida (na, codigos 0x38 a 0x3B) en las celdas de muerte_cel[ini..] */
#define MUERTE_EPILOGO 2          /* 0x9831: pide el epilogo de la aventura (Vars.epilogo) */
typedef struct { u8 tipo, nb, na, ini; } Muerte;

/* Estadisticas de monstruo (Mons.estad; tabla 0xAA8C de Hero Quest y 0xA76D de la ampliacion), de la tabla [estadisticas] de <aventura>.<idioma>.txt */
typedef struct { u8 mov, ataque, defensa; u16 oro; u8 quien, objeto, mensaje, jefe; } Estadistica;
/* mov: casillas que anda (el camino hasta un heroe no puede ser mas largo); ataque y defensa: dados; oro: monedas al morir y quien las recibe (0 nadie, 1 el que lo mata, 2 los cuatro);
   objeto: bit de +0x0D que da (0 ninguno; 3 = baraja de orco); mensaje: 0 ninguno, 1 BANE DE ORC, 2 ESTRELLA DEL OESTE; jefe: 1 = solo la espada espiritual le hace dano */

/* Tipos de monstruo (Mons.tipo) de la aventura: su nombre y los graficos que lo representan, de la tabla [monstruos] de <aventura>.<idioma>.txt */
#define N_TIPOS        16
#define N_TELEPORTE    6                          /* destinos de la sala de teletransporte (EV_TELEPORTE) */
#define N_RUTA         8                          /* tramos de la ruta de los monstruos errantes */
#define TIPO_NORMAL    0                          /* clase de un tipo de monstruo: sin comportamiento propio */
#define TIPO_ORCO      1                          /* da el segundo ataque de la vara de orco (letra V en [monstruos]) */
#define TIPO_MUERTO    2                          /* muerto viviente: 4 dados de la espada espiritual y muere con el agua santa (letra M) */
typedef struct { u8 retrato, icono, sprite, n, clase; u8 nombre[12]; } TipoMons;     /* retrato de combate, baldosa del plano, sprite de la vista, nombre (n caracteres del juego) y clase */

typedef struct {
    u8   mision;                  /* 0xA981 numero de la mision en su aventura (solo informativo: el comportamiento lo dan las banderas de ev) */
    u16  ev;                      /* banderas de eventos de la mision (EV_*), de la cabecera del fichero de la mision */
    u8   capa_a[MAP_P];           /* habitacion (bits 0-5), bit 6 visible en el plano, bit 7 visible en 3D */
    u8   capa_b[MAP_P];           /* paredes y puertas */
    u8   capa_c[MAP_P];           /* objetos (0x30...) y numero de monstruo + 1 */
    u8   sala_flag[N_SALAS];      /* 0xC47F + 4 * sala: 0xFF = ya se ha entrado en la sala (se borra al cargar la mision) */
    u8   ind[N_IND];              /* indicadores de la mision (0x6BB6) */
    Mons mons[N_MONS];
    Pers pers[N_PERS];
    Vars v;
    Estadistica estadisticas[N_ESTAD];   /* las de la aventura (partida_estadisticas) */
    TipoMons tipos[N_TIPOS];      /* los de la aventura (partida_tipos), no los borra la carga de la mision */
    u8   ruta_pasos, ruta_n;      /* ruta de los monstruos errantes (linea errante de la mision): pasos por ronda y numero de tramos (0 = no hay) */
    u8   ruta[N_RUTA][2];         /* tramos: eje y sentido (bit 0: 0 columna, 1 fila; bit 1: 1 = suma 1, 0 = resta 1) y la coordenada con la que acaba */
    u8   tp_sala, tp_n;           /* EV_TELEPORTE (linea teleporte de la mision): sala que se abandona y numero de destinos */
    u8   tp[N_TELEPORTE][5];      /* destinos: sala, columna, fila y columna y fila de la casilla de la puerta (ultima puerta cruzada) */
    Muerte muerte[N_ESTAD];       /* rutina de muerte de cada estadistica de monstruo */
    u8   muerte_cel[N_MUERTE_CEL][2];   /* (columna, fila) de las casillas de las rutinas MUERTE_SALIDA */
    u8   salidos;                 /* bit i = el heroe i ha salido por una escalera (el original vuelve a poner a 0xFF su marca 0x8FE4 + i) */
    u8   equipo_ocu[15];          /* 0xA2B2: equipo que se quita a los heroes en la mision 5 (0: +D,+E,+F; 1: +D,+E,+F; 2: +B a +F; 3: +B a +E) */
} Partida;

extern Partida g_part;

/* ---- dados */
void dado_semilla(u32 s);         /* el original usa el registro R del Z80 (el contador de refresco): aqui un generador congruencial */
void dado_guion(const int *v, int n);   /* pruebas: las siguientes n tiradas valen v[0]...; despues (hasta otra llamada) siempre 0 */
int  dado_con_r(u8 r, int acarreo);   /* 0x9BD3 con el valor de R y el acarreo dados (pruebas): 0 a 5 */
int  dado(void);                  /* 0x9BD3: 0 a 5 */
int  dado_r(void);                /* el R que leyo la ultima tirada (DADO lo deja en A: 0xA026 y 0x9D7E lo usan sin querer, ver HALLAZGOS.md) */
int  dado_combate(void);          /* 0x9BC8: cara de un dado de combate: 0 calavera, 1 escudo blanco, 2 escudo negro */
int  dados_mov_calcular(Partida *p, int pers);   /* 0x6ECD: deja en p->v.dados_mov los dados de movimiento de pers y quita los efectos temporales */
int  dados_mov_tirar(Partida *p);   /* 0x6F1C (la suma): tira dados_mov dados, suma (cara + 1) y lo deja en p->v.pm */

/* ---- personajes */
void pers_boot(Partida *p);       /* registros tal como los deja la carga del original; todo lo demas a cero */
void pers_nuevo(Partida *p, int i);   /* 0x60E6: inicializa un heroe nuevo (cuerpo inicial de la tabla 0x7E4E, sin oro ni equipo) */
void pers_bytes(const Pers *r, u8 *b);   /* los 26 bytes del registro del original (pruebas) */
void pers_marcar_muerte(Partida *p, int i);   /* 0xA254 */
void pers_restaurar_equipo(Partida *p, int i);   /* 0xA271: devuelve al heroe i el equipo que se le quito en la mision 5 */
int  cuerpo_max(int pers);                /* 0x7E4E: puntos de cuerpo con los que empieza cada personaje */
int  mision_continua(const Partida *p);   /* 0x6EB5: 1 si la mision sigue, 0 si ha terminado */
void mons_bytes(const Mons *m, u8 *b);   /* los 10 bytes del original (con la direccion 0x6200 + casilla en +4, +5) */

/* ---- mazo de cartas de tesoro */
extern u8  mazo[MAZO_N + 1];      /* 0x9593: cartas 0 a 24 y 0xFF al final */
extern int mazo_fin;              /* 0xA9E1 (indice del 0xFF final) */
void mazo_barajar(u8 c, int (*bit)(void));   /* 0x9542 con C = 0x80 | ((R >> 3) & 0x3F); el original saca 'bit' del bit 3 de los bytes de la ROM del Spectrum desde (0x06FC + C) * 2 + 1 */
void mazo_barajar_azar(void);     /* el mismo barajado con el generador de dados en vez de la ROM */
int  mazo_robar(void);            /* 0x9523: primera carta (se desplazan las demas) */
void mazo_devolver(int carta);    /* 0x9588: la deja al final */

/* ---- banderas de eventos de las misiones (docs/ZX_MISIONES.md). El numero de bit es el de la mision de Hero Quest cuyo comportamiento repiten; en el fichero de
   la mision (linea 'eventos' del texto) cualquier aventura puede activarlas, sea cual sea su numero. */
#define EV_SALE_PRIMERO (1 << 0)   /* mision 0: solo el primer heroe que sale por la escalera cobra 100 monedas */
#define EV_RAGNAR       (1 << 1)   /* mision 1: quinto personaje (Sir Ragnar) en la casilla (6, 1), se encuentra en la sala 0x16, recompensa al salir y la mision sigue mientras viva */
#define EV_ORO_MAGNUS   (1 << 3)   /* mision 3: el oro de Magnus (200 monedas al salir con el, y se puede volver a por mas) */
#define EV_ROCA         (1 << 4)   /* mision 4: cae una roca que cierra la casilla (25, 7) y abrir la puerta de la columna 5 (filas 3 y 4) deja ver al monstruo 1 */
#define EV_SIN_EQUIPO   (1 << 5)   /* mision 5: los heroes empiezan sin equipo ni hechizos y lo recuperan al entrar en la sala 0x1D */
#define EV_SALE_COBRA   (1 << 6)   /* mision 6: cada heroe que sale por la escalera cobra 100 monedas */
#define EV_SIN_FUEGO    (1 << 7)   /* mision 7: el elfo y el hechicero pierden los hechizos de fuego */
#define EV_CASTILLO     (1 << 9)   /* mision 9: 5000 monedas en la sala 0x18, teletransportes al azar, sin ver el mapa y fantasma al buscar tesoros */
#define EV_DESPIERTA    (1 << 12)  /* ampliacion, mision 1: pisar la columna 5 (filas 3 y 4) hace que el monstruo 1 cobre vida (como EV_ROCA, sin la roca) */
#define EV_TELEPORTE    (1 << 13)  /* ampliacion, mision 1: al salir de la sala de la linea teleporte el heroe va a uno de sus destinos al azar */
#define EV_ROCA_DER     (1 << 14)  /* ampliacion, mision 7: la roca de la trampa (codigo 0x34) cae en la casilla de la derecha del heroe: si hay un heroe pierde un punto, si no queda cerrada */
#define EV_CIERRE       (1 << 11)  /* mision 11: la sala 0x1F se cierra al entrar y la trampa cierra la salida cuando los cuatro la han pisado */

/* ---- cargar la mision */
/* datos = contenido de heroquest_NN (formato HQMS de docs/ZX_DATOS_QL.md); presentes[i] != 0 si el heroe i juega. Hace todo lo de
   INICIAR_MISION salvo mostrar el objetivo y barajar el mazo (game.c lo baraja con mazo_barajar_azar). 0 si el fichero no es valido. */
int  partida_cargar(Partida *p, int mision, const u8 *datos, long len, const u8 *presentes);
void partida_estadisticas(Partida *p, const u8 *tab);   /* copia la tabla de estadisticas de monstruo (data_estadisticas(): n y n entradas de 9 bytes); sin tabla, todo a cero */
const Estadistica *partida_estadistica(const Partida *p, int estad);   /* la estadistica, o la ultima si se sale de la tabla */
void partida_tipos(Partida *p, const u8 *tab);    /* copia la tabla de tipos de monstruo (data_tipos(): n y n entradas de 16 bytes); sin tabla, todo a cero */
const TipoMons *partida_tipo(const Partida *p, int tipo);   /* el tipo de monstruo, o el ultimo si se sale de la tabla */
int  casilla_indice(int col, int fila);   /* 0xA0D4: indice de una casilla en las capas */
#endif
