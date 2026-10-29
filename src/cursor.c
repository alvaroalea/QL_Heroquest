/* HeroQuest QL - cursor de la vista del juego */
#include "cursor.h"

#define CUR_IDX   66                     /* el cursor es el ultimo grafico de sprites_bin */
#define CUR_WPR   6                      /* 24 puntos = 6 palabras */
#define CUR_H     8
#define VISTA_Y   (GY0 + 16)             /* linea de pantalla de la fila 0 del buffer de la vista (UI_ROW0 = 2 filas de 8) */

int cur_x, cur_y;
static gimg img;
static int have;
static u8 fondo[CUR_WPR * 2 * CUR_H];    /* lo que habia bajo el cursor */
static int fx, fy, fw, fok;              /* donde se guardo, ancho en palabras (recortado por la derecha) y si hay algo guardado */
static int cnt_h, cnt_v;                 /* fotogramas con la tecla pulsada (7200 y 71FF del original) */

void cursor_init(const u8 *sprites)
{
    cur_x = 0x80; cur_y = 0x78;          /* valores iniciales del registro CURSOR (0xAA67) */
    cnt_h = cnt_v = 0;
    fok = 0;
    have = (sprites != 0) && pack_get(sprites, CUR_IDX, &img);
}

static void paso_x(int dx)               /* 7168 y 7199: 4 puntos con los limites de la vista */
{
    if (dx > 0) { if (cur_x < 0xFC) cur_x += 4; }
    else if (cur_x >= 0x10) cur_x -= 4;
}

static void paso_y(int dy)               /* 71C9 y 71F5: 2 puntos */
{
    if (dy > 0) { if (cur_y < 0xAE) cur_y += 2; }
    else if (cur_y >= 0x0A) cur_y -= 2;
}

/* Tecla mantenida: se mueve en cada fotograma y, desde el septimo, el doble (el contador llega a 6) */
static int repetir(int *cnt)
{
    int n = 1;
    if (*cnt >= 6) n = 2;
    else (*cnt)++;
    return n;
}

/* Raton: posicion absoluta en puntos de pantalla (una cuenta = un punto), sin aceleracion; el cursor es esa posicion redondeada a su
   rejilla (4 puntos en X, 2 en Y). Si las teclas movieron el cursor, el raton sigue desde donde quedo. */
static int m_x, m_y, m_cx = -1, m_cy;
/* En vertical el raton recorre las 256 lineas (el sprite entero dentro de la pantalla: su linea superior es VISTA_Y + cur_y - CUR_H);
   las teclas siguen con los limites del original (0x0A a 0xAE) */
#define MY_MIN  (CUR_H - VISTA_Y)
#define MY_MAX  (SCR_H - VISTA_Y)

static void mover_raton(void)
{
    int dx, dy;
    plat_raton_delta(&dx, &dy);
    if (m_cx != cur_x || m_cy != cur_y) { m_x = cur_x; m_y = cur_y; }
    m_x += dx; m_y += dy;
    if (m_x < 0x10) m_x = 0x10;
    if (m_x > 0xFC) m_x = 0xFC;
    if (m_y < MY_MIN) m_y = MY_MIN;
    if (m_y > MY_MAX) m_y = MY_MAX;
    cur_x = 0x10 + ((m_x - 0x10 + 2) & ~3);
    cur_y = 0x0A + ((m_y - 0x0A + 1) & ~1);
    if (cur_x > 0xFC) cur_x = 0xFC;
    if (cur_y > MY_MAX) cur_y = MY_MAX;
    m_cx = cur_x; m_cy = cur_y;
}

void cursor_move(u16 keys)
{
    int n;
    if (plat_raton != RAT_CURSORES) mover_raton();
    if (keys & K_RIGHT) { for (n = repetir(&cnt_h); n > 0; n--) paso_x(1); }
    else if (keys & K_LEFT) { for (n = repetir(&cnt_h); n > 0; n--) paso_x(-1); }
    else cnt_h = 0;
    if (keys & K_UP) { for (n = repetir(&cnt_v); n > 0; n--) paso_y(-1); }
    else if (keys & K_DOWN) { for (n = repetir(&cnt_v); n > 0; n--) paso_y(1); }
    else cnt_v = 0;
}

int cursor_dibujado(void) { return fok; }

void cursor_draw(void)
{
    int x = cur_x - 12, y = VISTA_Y + cur_y - CUR_H, w = CUR_WPR;
    if (!have) return;
    if (x + w * 4 > SCR_W) w = (SCR_W - x) / 4;
    fx = x; fy = y; fw = w; fok = 1;
    gfx_rect_get(fondo, x, y, w, CUR_H);
    gfx_blit_clip(&img, x, y);
}

void cursor_erase(void)
{
    if (fok) gfx_rect_put(fondo, fx, fy, fw, CUR_H);
    fok = 0;
}
