/* HeroQuest QL - textos del juego (fichero data/textos_es o textos_en, formato en docs/ZX_DATOS_QL.md). Los identificadores TX_* los genera
   tools/hq_textos_empaquetar.py en build/datos/textos_ids.h. */
#ifndef TEXT_H
#define TEXT_H
#include "data.h"

#define UI_ROW0 2                 /* fila de pantalla de la fila 0 del buffer del original (la vista del juego empieza en la fila 2) */

extern u8 ui_ink;                 /* color de los recuadros que dibujan los guiones (como el atributo vigente del original) */

const u8 *tx_item(int id, int *tipo, int *cant);
const u8 *tx_str(int id, int i, int *len);       /* cadena i del elemento de tipo CADENAS id */

/* Guiones: dibuja el guion id. ink >= 0 fuerza el color de la tinta (si no, el atributo del guion); invert = 1 dibuja tinta negra
   sobre papel de color (linea resaltada). dy suma filas. */
void tx_guion(int id, int ink, int invert, int dy);
void tx_ops(const u8 *p, int ink, int invert, int dy);   /* operaciones de un guion (p apunta al contador de operaciones) */
int  tx_guion_linea(int id, int n, int *x, int *y, int *attr, int *al, const u8 **s, int *len);   /* linea de texto n (0..) de un guion; 0 si no hay */
void tx_ventana(int id);                         /* ventana con marco centrada segun sus datos; las lineas con '{' no se interpretan */
void tx_ventana_lineas(int y, int centro, int ancho, int n, const u8 **ln, const int *len, const u8 *al);   /* ventana con textos ya preparados */
const u8 *tx_ventana_linea(int id, int n, int *len);   /* texto de la linea n de una ventana */
int  tx_zonas(int id, const u8 **zonas);         /* zonas de clic de una ventana: devuelve cuantas y deja en *zonas n entradas de 6 bytes (x1, x2, y1, y2, accion en un word) */
void tx_ventana_con(int id, int nrep, const int *linea, const u8 *const *s, const int *len);   /* ventana id con algunas lineas sustituidas (las que lleva un dato que cambia) */
void tx_caja(int col, int row, int w, int h, u8 ink);   /* recuadro de glifos 0x60-0x67 */
void tx_cadena_centrada(int row, const u8 *s, int n, u8 ink, u8 paper);   /* centra en 32 columnas */
void tx_cadena_en(int col, int row, int ancho, const u8 *s, int n, int alin, u8 ink, u8 paper);   /* en un campo de ancho celdas ('L', 'C' o 'R'; rellena con papel) */
#endif
