/* HeroQuest QL - ficheros de datos (data/xxx_bin; formatos en docs/ZX_GRAFICOS_QL.md y docs/ZX_DATOS_QL.md) */
#ifndef DATA_H
#define DATA_H
#include "gfx.h"

u16  rd16(const u8 *p);           /* palabra y palabra larga big endian (el formato de los ficheros) */
u32  rd32(const u8 *p);
extern int data_error;            /* por que fallo la ultima data_load: 1 no existe el fichero, 2 sin memoria o lectura incompleta */
u8  *data_load(const char *name, long *size);   /* lee un fichero de datos entero; NULL si no existe */
/* heroquest_dat: escenario, marco, sprites, baldosas, vista, musica, efectos y, el ultimo, retratos en un solo fichero (tools/hq_ql_dat.py). data_init carga todo menos los
   retratos (que no estan en dat_get); despues de las misiones, data_retratos_cargar mira si caben todos y, si no, deja su tabla de desplazamientos y el retrato del ataque
   fallido y lee los demas del disco cuando hacen falta (data_retrato). */
#define DAT_ESCENARIO 0
#define DAT_MARCO     1
#define DAT_SPRITES   2
#define DAT_BALDOSAS  3
#define DAT_VISTA     4
#define DAT_MUSICA    5
#define DAT_EFECTOS   6
#define DAT_RETRATOS  7
#define DAT_NUM       8
#define RETRATO_FALLO 9           /* el retrato del ataque fallido: el unico que se guarda en memoria si no caben todos */
const u8 *dat_get(int id, long *size);          /* parte id de heroquest_dat; NULL si no existe (los retratos no se piden con dat_get) */
void data_retratos_cargar(void);                /* tercera fase de la carga: todos los retratos si caben (dejando libre RESERVA_PARTIDA) y, si no, su tabla y el del ataque fallido */
int  data_retrato(int idx, gimg *g);            /* el retrato idx como grafico (opaco, 128 x 64): de memoria o, si no caben todos, leido del disco; valido hasta la siguiente llamada; 0 si no hay */
/* fonts_dat (mismo formato): las dos fuentes y la baldosa de la marca del personaje activo */
#define FNT_NORMAL     0
#define FNT_MANUSCRITA 1
#define FNT_MARCA      2
#define FNT_NUM        3
const u8 *fonts_get(int id, long *size);        /* parte id de fonts_dat; NULL si no existe */
int  pack_get(const u8 *pack, int idx, gimg *g);   /* entrada idx de un fichero HQGF; 0 si no existe */
int  pack_count(const u8 *pack);

extern u8 *pk_marco;              /* DAT_MARCO: 0 = marco de la pantalla de juego, 1 = pantalla de carga */
extern u8 *fnt_normal, *fnt_manuscrita;   /* fuentes de FONT_BYTES bytes (FNT_NORMAL y FNT_MANUSCRITA de fonts_dat) */
extern u8 *txt;                   /* textos del idioma elegido (fichero textos_es o textos_en: solo hay un idioma en memoria) */
extern u8 *fx_data;               /* DAT_EFECTOS (sin efectos de sonido si no vale) */
extern u8 *mus_data;               /* DAT_MUSICA  */
extern u8 *mis_indice;            /* indice de la aventura activa en el idioma elegido (<aventura>_xx_idx): las entradas de sus misiones; solo hay uno en memoria */
extern int g_idioma;              /* idioma de las misiones: IDIOMA_ES o IDIOMA_EN */
#define IDIOMA_ES 0
#define IDIOMA_EN 1
extern char g_aventura[16];       /* prefijo de los ficheros de la aventura: "heroquest" o la que se ha cargado (witchlord...) */
extern int  g_nmis;               /* numero de misiones de la aventura (lo dice su indice) */
extern const u8 *g_subtitulo;     /* subtitulo de la aventura que sale bajo HEROQUEST: byte de longitud y caracteres (longitud 0 = ninguno) */
const u8 *data_mision_titulo(int n, int *len);   /* nombre de la mision n de la aventura en su indice (caracteres del juego) */
#define RESERVA_PARTIDA 50000L    /* memoria que se deja libre para la partida (bufferes de la vista: 2 x 12000 + 176 x 128 = 46528, mas margen) al precargar las misiones */
u8  *data_mision_obtener(int n, long *size);   /* la mision n de la aventura activa: la precargada o, si no, leida del disco (NULL si falla: data_error) */
void data_mision_soltar(u8 *m);   /* se llama al acabar con lo que dio data_mision_obtener (libera solo lo que no esta precargado) */
const u8 *data_estadisticas(const u8 *idx);       /* tabla de estadisticas de monstruo (n y n entradas de 9 bytes: movimiento, ataque, defensa, monedas (word), a quien, objeto, mensaje, jefe) de un indice; 0 = el de la activa */
const u8 *data_tipos(const u8 *idx);              /* tabla de tipos de monstruo (n y n entradas de 16 bytes: retrato, icono, sprite, longitud, nombre) de un indice de aventura; 0 = el de la activa */
const u8 *data_epilogo(int n);                    /* pagina n (de 0) del epilogo de la aventura activa (operaciones de guion para tx_ops), 0 si no hay */
int  data_aventura_info(const char *nombre, u8 *sub);   /* lee la cabecera de <nombre>_es_idx (o _en_idx): 1 si es una aventura valida y deja en sub (36 bytes) el subtitulo (byte de longitud y caracteres del juego) */
int  data_aventura(const char *nombre);   /* la aventura activa pasa a ser <nombre>: su indice (<nombre>_es_idx o _en_idx) sustituye al actual y sus misiones son <nombre>_NN_es...; 0 si no existe (sigue la anterior) */
void data_mision_nombre(char *name, int n);   /* nombre del fichero de la mision n en el idioma elegido: <aventura>_NN_es o <aventura>_NN_en (name: 24 bytes) */
int  data_idioma(int idioma);     /* elige el idioma: borra los textos y el indice y los vuelve a cargar del disco; 0 si falta el fichero */

int  data_init(void);             /* carga todo lo que usan los menus; 0 si falta algo (el mensaje ya sale por pantalla) */
#endif
