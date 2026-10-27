/* HeroQuest QL - ficheros de datos (data/xxx_bin; formatos en docs/ZX_GRAFICOS_QL.md y docs/ZX_DATOS_QL.md) */
#ifndef DATA_H
#define DATA_H
#include "gfx.h"

u16  rd16(const u8 *p);           /* palabra y palabra larga big endian (el formato de los ficheros) */
u32  rd32(const u8 *p);
u8  *data_load(const char *name, long *size);   /* lee un fichero de datos entero; NULL si no existe */
int  pack_get(const u8 *pack, int idx, gimg *g);   /* entrada idx de un fichero HQGF; 0 si no existe */
int  pack_count(const u8 *pack);

extern u8 *pk_marco;              /* marco_bin: 0 = marco de la pantalla de juego, 1 = pantalla de carga */
extern u8 *fnt_normal, *fnt_manuscrita;   /* fuentes de 544 bytes */
extern u8 *txt;                   /* textos_bin */
extern u8 *fx_data;               /* efectos_bin (puede faltar: entonces no hay efectos de sonido) */
extern u8 *mus_data;               /* musica_bin (puede faltar: entonces no hay musica) */
extern u8 *mis_indice;            /* misiones_indice_bin */

int  data_init(void);             /* carga todo lo que usan los menus; 0 si falta algo (el mensaje ya sale por pantalla) */
#endif
