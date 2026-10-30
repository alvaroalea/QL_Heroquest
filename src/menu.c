/* HeroQuest QL - menus del comienzo: menu principal, eleccion de hechizos y de mision, tienda, crear personaje y controles.
   Reproduce las pantallas del original (overlay del banco 6, docs/ZX_ANALISIS.md): los textos y sus posiciones salen de
   data/textos_xx; los numeros TX_* los genera tools/hq_textos_empaquetar.py. */
#include "game.h"
#include "qsound.h"
#include "music.h"
#include "keys.h"
#include "textos_ids.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Heroe heroes[4];
int g_mision = 0, g_manuscrita = 0, g_musica = 0, g_raton = RAT_CURSORES;

#define NEG C_BLACK

/* ---------------------------------------------------------------- ayudas */
static void draw_line_dy(int id, int n, int ink, int invert, int dy)   /* linea n de un guion desplazada dy filas; ink < 0: el atributo del guion */
{
    int x, y, attr, al, len, c;
    const u8 *s;
    if (!tx_guion_linea(id, n, &x, &y, &attr, &al, &s, &len)) return;
    c = (ink >= 0) ? ink : (attr & 7);
    gfx_text(x, y + UI_ROW0 + dy, s, len, (u8)(invert ? NEG : c), (u8)(invert ? c : NEG));
}

static void draw_line(int id, int n, int ink, int invert) { draw_line_dy(id, n, ink, invert, 0); }

static void draw_msg(int row, int centrado, int i, int ink)    /* cadena i de PORT_MENSAJES */
{
    int n;
    const u8 *s = tx_str(TX_PORT_MENSAJES, i, &n);
    (void)centrado;
    tx_cadena_centrada(row, s, n, (u8)ink, NEG);
}

static void draw_title(void)
{
    tx_guion(TX_MENU_6653, -1, 0, 0);
    if (g_subtitulo && g_subtitulo[0])                  /* el subtitulo de la aventura (su indice) va debajo de HEROQUEST */
        tx_cadena_centrada(2 + UI_ROW0, g_subtitulo + 1, g_subtitulo[0], C_YELLOW, NEG);
}

static void cls(void) { gfx_clear(); }

static void draw_num4(int col, int row, unsigned v, u8 ink)
{
    u8 d[4];
    int i;
    for (i = 3; i >= 0; i--) { d[i] = (u8)(0x30 + v % 10); v /= 10; }
    gfx_text(col, row, d, 4, ink, NEG);
}

/* ---------------------------------------------------------------- hechizos */
static const int grp_tit[4] = { TX_MENU_66F5, TX_MENU_670B, TX_MENU_6722, TX_MENU_673A };
static const int grp_lx[4] = { 0, 16, 0, 16 }, grp_ly[4] = { 9, 9, 17, 17 };

#define ESC_CARTA_0 59                       /* cartas de los colegios (aire, fuego, tierra, agua) en escenario_bin */
static int vista_cartas = 1;                 /* la eleccion de hechizos empieza con las cartas; ARRIBA / ABAJO cambian entre cartas y lista de hechizos */

static void draw_cartas(const int *owner, int cur, int todo)
{
    int g, x, y, attr, al, len, n;
    const u8 *s, *pk = dat_get(DAT_ESCENARIO, 0);
    gimg gi;
    for (g = 0; g < 4; g++) {
        if (todo == 1 && pk && pack_get(pk, ESC_CARTA_0 + g, &gi)) gfx_blit(&gi, g * 64, GY0 + 5 * 8);
        tx_guion_linea(grp_tit[g], 0, &x, &y, &attr, &al, &s, &len);
        if (g_idioma != IDIOMA_EN) { s += 9; n = len - 9; }                           /* el nombre del colegio sin "HECHIZOS " ... */
        else n = len - 7;                                                     /* ... o sin " SPELLS" */
        gfx_text(g * 8 + 4 - n / 2, 17 + UI_ROW0, s, n, (u8)(g == cur ? NEG : (attr & 7)), (u8)(g == cur ? (attr & 7) : NEG));
        if (owner[g]) {
            tx_guion_linea(owner[g] == 1 ? TX_MENU_6750 : TX_MENU_6757, 0, &x, &y, &attr, &al, &s, &len);
            gfx_text(g * 8 + 4 - len / 2, 19 + UI_ROW0, s, len, C_CYAN, NEG);
        }
    }
}

/* todo = 1: pantalla entera; 2: solo el encabezado (de un heroe a otro) y los textos de los grupos; 0: solo los textos de los grupos (cambia la seleccion): asi no parpadea */
static int grp_activa;                       /* la pantalla de grupos ya esta dibujada (se pasa del mago al elfo sin borrarla) */
static void draw_grupos(const int *owner, int cur, int hdr, int todo)
{
    int g, x, y, attr, al, len, id;
    const u8 *s;
    if (todo == 1) cls();
    else if (todo == 2) gfx_cells_fill(0, 0, 32, 5, NEG);       /* borra solo el encabezado (filas 0 a 4) */
    if (todo && hdr) {
        id = hdr == 1 ? TX_MENU_680C : hdr == 2 ? TX_MENU_686A : TX_MENU_683C;
        tx_guion(id, -1, 0, 0);
        if (tx_guion_linea(id, 0, &x, &y, &attr, &al, &s, &len)) {        /* el nombre (ELFO / MAGO) una fila mas arriba */
            gfx_cells_fill(x, y + UI_ROW0, len, 1, NEG);
            gfx_text(x, y + UI_ROW0 - 1, s, len, (u8)(attr & 7), NEG);
        }
    }
    if (vista_cartas) { draw_cartas(owner, cur, todo); return; }
    if (todo == 1) tx_guion(TX_MENU_6761, -1, 0, 0);
    for (g = 0; g < 4; g++) {
        draw_line(grp_tit[g], 0, -1, g == cur);
        if (owner[g]) {
            tx_guion_linea(owner[g] == 1 ? TX_MENU_6750 : TX_MENU_6757, 0, &x, &y, &attr, &al, &s, &len);
            gfx_text(grp_lx[g], grp_ly[g] + UI_ROW0, s, len, C_CYAN, NEG);
        }
    }
}

static int elegir_grupo(int *owner, int who, int hdr)    /* devuelve el grupo elegido (-1 = ESC) */
{
    int cur = 0, i, todo;
    u16 k;
    while (owner[cur]) cur++;
    draw_grupos(owner, cur, hdr, grp_activa ? 2 : 1);
    grp_activa = 1;
    for (;;) {
        k = key_wait();
        todo = 0;
        if (k & (K_UP | K_DOWN)) { vista_cartas = !vista_cartas; todo = 1; }
        if (k & K_RIGHT) { i = cur; do { cur = (cur + 1) & 3; } while (owner[cur] && cur != i); }
        if (k & K_LEFT) { i = cur; do { cur = (cur + 3) & 3; } while (owner[cur] && cur != i); }
        if (k & K_ESC) return -1;
        if (k & K_FIRE) { owner[cur] = who; return cur; }
        draw_grupos(owner, cur, hdr, todo);
    }
}

/* Primero elige el hechicero y despues el duende (elfo): el hechicero elige 3 grupos si juega solo y 1 si tambien juega el elfo
   (los que queden sin dueño pasan al hechicero) */
static int elegir_hechizos(void)                         /* 0 si se pulsa ESC (se vuelve a la pantalla del objetivo) */
{
    int owner[4], g, n, mago = heroes[3].activo, elfo = heroes[2].activo;
    owner[0] = owner[1] = owner[2] = owner[3] = 0;
    grp_activa = 0;
    heroes[2].grupos = heroes[3].grupos = 0;
    if (!mago && !elfo) return 1;
    if (mago) {
        n = elfo ? 1 : 3;
        while (n--) { g = elegir_grupo(owner, 2, elfo ? 3 : 2); if (g < 0) goto atras; heroes[3].grupos |= (u8)(1 << g); }
    }
    if (elfo) { g = elegir_grupo(owner, 1, 1); if (g < 0) goto atras; heroes[2].grupos |= (u8)(1 << g); }
    if (mago && elfo)
        for (g = 0; g < 4; g++) if (!owner[g]) { owner[g] = 2; heroes[3].grupos |= (u8)(1 << g); }
    draw_grupos(owner, -1, 0, 0);                          /* estado final sin seleccion y con todos los dueños, sin borrar la pantalla */
    grp_activa = 0;
    return 1;
atras:
    grp_activa = 0;
    heroes[2].grupos = heroes[3].grupos = 0;
    return 0;
}

/* ---------------------------------------------------------------- mision */
static void draw_misiones(int sel)                   /* los nombres salen del indice de la aventura (<aventura>_xx_idx), centrados */
{
    int i, n;
    const u8 *t;
    for (i = 0; i < g_nmis; i++) {
        t = data_mision_titulo(i, &n);
        tx_cadena_centrada(4 + i + UI_ROW0, t, n, i == sel ? C_CYAN : C_RED, NEG);
    }
}

static int elegir_mision(void)
{
    int sel = g_mision < g_nmis ? g_mision : 0;
    u16 k;
    cls(); draw_title();
    for (;;) {
        draw_misiones(sel);
        k = key_wait();
        if (k & (K_DOWN | K_RIGHT)) sel = (sel + 1) % g_nmis;
        if (k & (K_UP | K_LEFT)) sel = (sel + g_nmis - 1) % g_nmis;
        if (k & K_ESC) return -1;
        if (k & K_FIRE) return sel;
    }
}

static int mostrar_objetivo(int n)                       /* 0 si se pulsa ESC (vuelta a la lista de misiones) */
{
    char name[24];
    u8 *m;
    u16 k;
    data_mision_nombre(name, n);
    m = data_load(name, 0);
    cls();
    if (m) { tx_interlinea = 2; tx_ops(m + rd32(m + 44), -1, 0, 0); tx_interlinea = 0; free(m); }   /* 2 puntos mas entre lineas */
    else tx_guion(TX_MENU_66B6, -1, 0, 0);
    key_reset();
    for (;;) {
        k = key_wait();
        if (k & K_ESC) return 0;
        if (k & K_FIRE) return 1;
    }
}

static void menu_jugar(void)
{
    int i, algun = 0, m, ok;
    for (i = 0; i < 4; i++) algun |= heroes[i].activo;
    if (!algun) {                         /* "TODOS LOS JUGADORES MUERTOS / CREA JUGADORES NUEVOS" */
        cls(); tx_guion(TX_MENU_66B6, -1, 0, 0);
        key_reset(); key_wait();
        return;
    }
    for (ok = 0; !ok;) {                  /* ESC vuelve a la pantalla anterior: de hechizos a objetivo y de objetivo a la lista de misiones */
        m = elegir_mision();
        if (m < 0) return;
        g_mision = m;
        while (mostrar_objetivo(m))
            if (elegir_hechizos()) { ok = 1; break; }    /* los grupos de hechizos se eligen despues de leer la introduccion de la mision */
    }
    game_run(m);
    mus_song(2);                           /* de vuelta al menu: su musica */
}

/* ---------------------------------------------------------------- tienda */
typedef struct { u8 arm, wpn; u16 precio; } Articulo;
static const Articulo art[13] = {          /* armaduras (+0x0F), armas (+0x0E), precio: tabla 0x6603 del overlay */
    { 0, 0x01, 100 }, { 0, 0x02, 150 }, { 0, 0x04, 250 }, { 0, 0x08, 400 }, { 0, 0x10, 350 }, { 0, 0x20, 150 }, { 0, 0x40, 150 },
    { 0x01, 0, 100 }, { 0x02, 0, 120 }, { 0x04, 0, 450 }, { 0x08, 0, 850 }, { 0x80, 0, 250 }, { 0, 0, 0 }
};

static int art_poseido(const Heroe *h, int i) { return (h->armas & art[i].wpn) || (h->armadura & art[i].arm); }

static void art_pos(int i, int *col, int *row, int *ancho)
{
    if (i < 7) { *col = 1; *row = 13 + i; *ancho = 11; }
    else if (i < 11) { *col = 17; *row = 13 + (i - 7); *ancho = 9; }
    else if (i == 11) { *col = 17; *row = 17; *ancho = 9; }
    else { *col = 17; *row = 19; *ancho = 9; }
}

static void draw_art(const Heroe *h, int i, int sel)
{
    int col, row, ancho, n;
    const u8 *s;
    u8 ink = C_GREEN, paper = NEG;
    art_pos(i, &col, &row, &ancho);
    if (i == 12) { ink = C_CYAN; s = (const u8 *)"FINAL"; n = 5; { int x, y, attr, al; tx_guion_linea(TX_MENU_69E2, 21, &x, &y, &attr, &al, &s, &n); } }
    else if (i == 11) { int x, y, attr, al; tx_guion_linea(TX_MENU_6A92, 0, &x, &y, &attr, &al, &s, &n); }
    else if (i < 7) s = tx_str(TX_PANEL_ARMAS, i, &n);
    else s = tx_str(TX_PANEL_ARMADURAS, i - 7, &n);
    if (art_poseido(h, i)) ink = C_YELLOW;
    if (sel) { paper = ink; ink = NEG; }
    tx_cadena_en(col, row + UI_ROW0, ancho, s, n, 'L', ink, paper);
}

static void draw_oro(const Heroe *h)
{
    int x, y, attr, al, len;
    const u8 *s;
    tx_guion_linea(TX_MENU_6AA6, 0, &x, &y, &attr, &al, &s, &len);
    gfx_text(x, y + UI_ROW0, s, len, C_YELLOW, NEG);
    draw_num4(x + len, y + UI_ROW0, h->oro, C_YELLOW);
}

static void draw_tienda_heroes(int sel)
{
    int i;
    for (i = 0; i < 5; i++) draw_line_dy(TX_MENU_69E2, i, i == sel ? C_CYAN : C_RED, 0, 2);      /* dos filas mas abajo que en el original: el subtitulo ocupa su sitio */
}

static void draw_tienda(int sel)
{
    int i;
    cls(); draw_title();
    ui_ink = C_MAGENTA;
    tx_caja(0, 12 + UI_ROW0, 32, 9, ui_ink);                  /* el guion de la tienda sin los nombres de los heroes (lineas 0 a 4), que se pintan aparte */
    for (i = 5; i < 22; i++) draw_line(TX_MENU_69E2, i, -1, 0);
    draw_tienda_heroes(sel);
}

static void menu_tienda(void)
{
    int hs = 0, it, i;
    u16 k;
    Heroe *h;
    draw_tienda(hs);
    for (;;) {
        k = key_wait();
        if (k & (K_DOWN | K_RIGHT)) hs = (hs + 1) % 5;
        if (k & (K_UP | K_LEFT)) hs = (hs + 4) % 5;
        if (k & K_ESC) return;
        if (k & K_FIRE) {
            if (hs == 4) return;
            h = &heroes[hs];
            it = 0;
            for (i = 0; i < 13; i++) draw_art(h, i, i == it);
            draw_oro(h);
            for (;;) {
                k = key_wait();
                if (k & (K_DOWN | K_RIGHT)) it = (it + 1) % 13;
                if (k & (K_UP | K_LEFT)) it = (it + 12) % 13;
                if (k & K_ESC) break;
                if (k & K_FIRE) {
                    if (it == 12) break;
                    if (!art_poseido(h, it) && h->oro >= art[it].precio) {
                        h->oro = (u16)(h->oro - art[it].precio);
                        h->armas |= art[it].wpn; h->armadura |= art[it].arm;
                    }
                }
                for (i = 0; i < 13; i++) draw_art(h, i, i == it);
                draw_oro(h);
            }
            draw_tienda(hs);                   /* de vuelta a la lista de heroes */
        }
        draw_tienda_heroes(hs);
    }
}

/* ---------------------------------------------------------------- crear personaje */
static const int cls_id[5] = { TX_MENU_6AC5, TX_MENU_6AD2, TX_MENU_6ADD, TX_MENU_6AE9, TX_MENU_6AF8 };
static const int nom_id[4] = { TX_GUION_A484, TX_GUION_A48F, TX_GUION_A49A, TX_GUION_A4A5 };
static const u8 cls_col[4] = { C_MAGENTA, C_CYAN, C_RED, C_GREEN };

static void draw_nombre(int i)
{
    u8 b[8];
    int k;
    b[0] = 0x72;
    for (k = 0; k < 6; k++) b[1 + k] = (k < heroes[i].nlen) ? heroes[i].nombre[k] : 0x5F;
    b[7] = 0x73;
    gfx_text(16, 4 + 3 * i + UI_ROW0, b, 8, NEG, cls_col[i]);
    {                                                    /* el heroe que no juega (muerto o eliminado): "MUERTO" a su lado */
        int x, y, attr, al, len;
        const u8 *s;
        gfx_cells_fill(25, 4 + 3 * i + UI_ROW0, 7, 1, NEG);
        if (!heroes[i].activo && tx_guion_linea(TX_MENU_6B0F, 0, &x, &y, &attr, &al, &s, &len))
            gfx_text(25, 4 + 3 * i + UI_ROW0, s, len, C_RED, NEG);
    }
}

static void draw_crear(int sel)
{
    int i;
    for (i = 0; i < 5; i++) draw_line(cls_id[i], 0, -1, i == sel);
    for (i = 0; i < 4; i++) draw_nombre(i);
}

static const int op_id[4] = { TX_MENU_6B5D, TX_MENU_6B5D, TX_MENU_6B7A, TX_MENU_6B7A };
static const int op_n[4] = { 0, 1, 0, 1 };        /* la ultima: linea 1 = ACTIVAR, linea 2 = ELIMINAR (segun el heroe) */

static void draw_opciones(int sel, int visible, int hs)
{
    int i;
    gfx_cells_fill(0, 23, GCOLS, 1, NEG);
    if (!visible) return;
    for (i = 0; i < 4; i++) draw_line(op_id[i], op_n[i] + (i == 3 && heroes[hs].activo), -1, i == sel);
}

/* Teclea un nombre de hasta max letras (max <= 12) con la rejilla de letras (abajo): izquierda y derecha mueven, el boton escribe la letra,
   * borra la ultima escrita y OK termina. Se escribe en las 12 columnas que empiezan en col. Devuelve la longitud (-1 con ESC). */
static int leer_texto(u8 *dst, int max, int col)
{
    int pos = 0, n = 0, x, y, attr, al, len, i, gx;
    const u8 *s;
    u16 k;
    u8 b[12];
    tx_guion_linea(TX_MENU_6B9B, 0, &x, &y, &attr, &al, &s, &len);       /* A-Z, un hueco, * (glifo 0x73), un hueco y OK */
    gx = x;
    for (;;) {
        for (i = 0; i < max; i++) b[i] = (u8)(i < n ? dst[i] : 0x5F);
        gfx_text(col, 19 + UI_ROW0, b, max, C_WHITE, NEG);
        gfx_text(gx, y + UI_ROW0, s, len, C_RED, NEG);
        if (pos < 26) gfx_char(gx + pos, y + UI_ROW0, s[pos], NEG, C_RED);
        else if (pos == 26) gfx_char(gx + 27, y + UI_ROW0, s[27], NEG, C_RED);
        else gfx_text(gx + 29, y + UI_ROW0, s + 29, 2, NEG, C_RED);
        k = key_wait();
        if (k & (K_RIGHT | K_DOWN)) pos = (pos + 1) % 28;
        if (k & (K_LEFT | K_UP)) pos = (pos + 27) % 28;
        if (k & K_ESC) return -1;
        if (k & K_FIRE) {
            if (pos == 27) return n;
            if (pos == 26) { if (n > 0) n--; }
            else if (n < max) dst[n++] = s[pos];
        }
    }
}

static int leer_nombre(u8 *dst) { return leer_texto(dst, 6, 13); }          /* nombre de un heroe: 6 letras */

static void fichero_heroe(char *path, const u8 *nom, int n)
{
    int i;
    strcpy(path, plat_data_dir());
    strcat(path, "hq_");
    for (i = 0; i < n && i < 6; i++) { char ch = (char)nom[i]; strncat(path, (nom[i] == 0x5F) ? "_" : &ch, 1); }
}

static int guardar_heroe(const Heroe *h)
{
    char path[96];
    u8 b[16];
    FILE *f;
    int i;
    fichero_heroe(path, h->nombre, h->nlen);
    f = fopen(path, "wb");
    if (!f) return 0;
    b[0] = 'H'; b[1] = 'Q'; b[2] = 'H'; b[3] = 1;
    b[4] = (u8)(h->oro >> 8); b[5] = (u8)h->oro; b[6] = h->armas; b[7] = h->armadura; b[8] = h->grupos; b[9] = h->nlen;
    for (i = 0; i < 6; i++) b[10 + i] = (i < h->nlen) ? h->nombre[i] : 0x5F;
    i = (fwrite(b, 1, 16, f) == 16);
    fclose(f);
    return i;
}

static int cargar_heroe(Heroe *h, const u8 *nom, int n)
{
    char path[96];
    u8 b[16];
    FILE *f;
    int i;
    fichero_heroe(path, nom, n);
    f = fopen(path, "rb");
    if (!f) return 0;
    i = (fread(b, 1, 16, f) == 16 && b[0] == 'H' && b[1] == 'Q' && b[2] == 'H');
    fclose(f);
    if (!i) return 0;
    h->oro = (u16)((b[4] << 8) | b[5]); h->armas = b[6]; h->armadura = b[7]; h->grupos = b[8]; h->nlen = b[9] > 6 ? 6 : b[9];
    for (i = 0; i < 6; i++) h->nombre[i] = b[10 + i];
    return 1;
}

static void aviso(int msg, int ink)
{
    gfx_cells_fill(0, 21, GCOLS, 1, NEG);
    draw_msg(21, 1, msg, ink);
    key_reset(); key_wait();
    gfx_cells_fill(0, 21, GCOLS, 1, NEG);
}

static void menu_crear(void)
{
    int hs = 0, op, r, i;
    u8 tmp[8];
    u16 k;
    cls();
    tx_guion(TX_MENU_6AB2, -1, 0, 0);
    for (;;) {
        draw_crear(hs);
        k = key_wait();
        if (k & (K_DOWN | K_RIGHT)) hs = (hs + 1) % 5;
        if (k & (K_UP | K_LEFT)) hs = (hs + 4) % 5;
        if (k & K_ESC) return;
        if (k & K_FIRE) {
            if (hs == 4) return;
            op = 0;
            for (;;) {
                draw_opciones(op, 1, hs);
                k = key_wait();
                if (k & (K_RIGHT | K_DOWN)) op = (op + 1) % 4;
                if (k & (K_LEFT | K_UP)) op = (op + 3) % 4;
                if (k & K_ESC) break;
                if (k & K_FIRE) {
                    draw_opciones(op, 0, hs);
                    if (op == 0) {                          /* NOMBRE: se teclea de nuevo (abajo) y al acabar sustituye al anterior */
                        r = leer_nombre(tmp);
                        if (r >= 0) { heroes[hs].nlen = (u8)r; for (i = 0; i < r; i++) heroes[hs].nombre[i] = tmp[i]; }
                        gfx_cells_fill(0, 21, GCOLS, 3, NEG);
                    } else if (op == 1) {                   /* CARGAR: se pide el nombre del fichero */
                        r = leer_nombre(tmp);
                        gfx_cells_fill(0, 21, GCOLS, 3, NEG);
                        if (r > 0) { if (!cargar_heroe(&heroes[hs], tmp, r)) aviso(3, C_RED); }
                    } else if (op == 2) {                   /* GUARDAR */
                        aviso(guardar_heroe(&heroes[hs]) ? 2 : 4, C_YELLOW);
                    } else {                                /* ACTIVAR (tambien revive al muerto en una partida anterior) / ELIMINAR: juega o no */
                        heroes[hs].activo = (u8)!heroes[hs].activo;
                    }
                    draw_crear(hs);
                    break;
                }
            }
            draw_opciones(0, 0, hs);
        }
    }
}

/* ---------------------------------------------------------------- controles */
/* Convierte un texto ASCII a codigos del juego (solo letras, cifras y espacio) */
static int a_codigos(const char *a, u8 *b)
{
    int n;
    for (n = 0; a[n]; n++) b[n] = (u8)(a[n] == ' ' ? 0x5F : a[n]);
    return n;
}

static void draw_controles(void)
{
    int n, on;
    const u8 *s;
    u8 b[16];
    static const u8 col[5] = { C_MAGENTA, C_GREEN, C_RED, C_WHITE, C_CYAN };
    static const int row[5] = { 5, 6, 7, 8, 10 };
    static const int txt_i[5] = { 1, 2, 3, 7, 4 };          /* cadenas de PORT_CONTROLES: 1.CURSORES 2.MUSICA 3.MANUSCRITO 4.IDIOMA 0.SALIDA */
    int i;
    cls();
    s = tx_str(TX_PORT_CONTROLES, 0, &n);
    tx_cadena_centrada(2 + UI_ROW0, s, n, C_YELLOW, NEG);
    for (i = 0; i < 5; i++) {
        s = tx_str(TX_PORT_CONTROLES, txt_i[i], &n);
        gfx_text(6, row[i] + UI_ROW0, s, n, col[i], NEG);
        if (i == 0) {                                         /* tipo de raton */
            static const char *const nom[4] = { "CURSORES", "QIMI", "QIMSI", "POINTER ENV" };
            int c = 6 + n + 1;                                    /* detras del rotulo (cambia de largo con el idioma) */
            if (c < 6 + 11) c = 6 + 11;
            n = a_codigos(nom[g_raton], b);
            gfx_text(c, row[i] + UI_ROW0, b, n, col[i], NEG);
        } else if (i == 1) {
            s = tx_str(TX_PORT_CONTROLES, g_musica ? 5 : 6, &n);
            gfx_text(6 + 11 + 0, row[i] + UI_ROW0, s, n, col[i], NEG);
        } else if (i == 3) {                                  /* idioma actual */
            on = 6 + n + 1;                                       /* detras del rotulo (que cambia de largo con el idioma) */
            if (on < 6 + 11) on = 6 + 11;
            n = a_codigos(g_idioma == IDIOMA_EN ? "ENGLISH" : "ESPANOL", b);
            gfx_text(on, row[i] + UI_ROW0, b, n, col[i], NEG);
        }
    }
    {                                                         /* diagnostico temporal: registro de estado de la QIMSI */
        static const char hx[] = "0123456789ABCDEF";
        char t[16];
        int e = plat_raton_estado() & 0xFF;
        strcpy(t, "QIMSI FEDF XX");
        t[11] = hx[e >> 4]; t[12] = hx[e & 15];
        n = a_codigos(t, b);
        gfx_text(6, 13 + UI_ROW0, b, n, C_BLUE, NEG);
    }
}

static void menu_controles(void)
{
    u16 k;
    plat_tecla4 = 1;
    for (;;) {
        draw_controles();
        k = key_wait();
        if (k & K_1) {                                            /* rota entre los tipos de raton disponibles */
            int m = g_raton, j;
            for (j = 0; j < 4; j++) {
                m = (m + 1) & 3;
                if (m == RAT_CURSORES || plat_raton_ok(m)) break;
            }
            g_raton = m;
            plat_raton = (u8)m;
        }
        if ((k & K_2) && qs_present()) {
            g_musica = !g_musica;
            mus_enable(g_musica);
            mus_song(2);
        }
        if (k & K_3) { g_manuscrita = !g_manuscrita; gfx_set_font(g_manuscrita ? fnt_manuscrita : fnt_normal); }
        if (k & K_4) {                                            /* cambia el idioma de las misiones y las ventanas */
            int antes = g_idioma;
            if (!data_idioma(antes == IDIOMA_EN ? IDIOMA_ES : IDIOMA_EN)) data_idioma(antes);
        }
        if (k & (K_0 | K_ESC | K_FIRE)) break;
    }
    plat_tecla4 = 0;
}

/* ---------------------------------------------------------------- menu principal */
static void draw_principal(int sel)
{
    int i;
    for (i = 0; i < 5; i++) draw_line_dy(TX_MENU_6662, i, i == sel ? C_GREEN : C_MAGENTA, 0, 1);      /* una fila mas abajo (el subtitulo de la aventura baja una) */
}

/* CARGAR OTRAS AVENTURAS: se leen los <nombre>_es_idx (o _en_idx) de la unidad de datos y se listan por su subtitulo (hasta AV_MAX); se elige una con arriba/abajo
   y el boton, o se vuelve a escanear (por si se cambia de disco) o se vuelve sin cargar nada (tambien con ESC) */
#define AV_MAX 6
#define AV_FILA (11)                                        /* fila de pantalla de la primera aventura */

static void av_texto(int fila, const u8 *cod, int n, int sel)
{
    gfx_cells_fill(0, fila, GCOLS, 1, NEG);
    tx_cadena_centrada(fila, cod, n, sel ? C_GREEN : C_MAGENTA, NEG);
}

static void menu_cargar_aventura(void)
{
    static const char *const t_cab[2] = { "ELIGE LA AVENTURA", "CHOOSE THE ADVENTURE" };
    static const char *const t_esc[2] = { "VOLVER A ESCANEAR", "SCAN AGAIN" };
    static const char *const t_vol[2] = { "VOLVER", "BACK" };
    static const char *const t_nin[2] = { "NO HAY AVENTURAS", "NO ADVENTURES FOUND" };
    char nom[AV_MAX + 6][16];
    char elegido[AV_MAX][16];
    u8 sub[AV_MAX][36], tmp[36], b[40];
    int en = (g_idioma == IDIOMA_EN), n, c, i, sel = 0, nsel, t;
    u16 k;
    for (;;) {
        cls(); draw_title();
        n = 0;
        c = plat_listar(en ? "_en_idx" : "_es_idx", nom, AV_MAX + 6);
        for (i = 0; i < c && n < AV_MAX; i++) {                      /* la de Hero Quest va primero; las que no tienen una cabecera valida no se listan */
            if (!data_aventura_info(nom[i], tmp)) continue;
            if (tmp[0] == 0) {                                       /* sin subtitulo: el nombre */
                for (t = 0; nom[i][t] && t < 35; t++) tmp[1 + t] = (u8)((nom[i][t] >= 'a' && nom[i][t] <= 'z') ? nom[i][t] - 32 : nom[i][t]);
                tmp[0] = (u8)t;
            }
            strcpy(elegido[n], nom[i]);
            memcpy(sub[n], tmp, 36);
            if (strcmp(nom[i], "heroquest") == 0 && n > 0) {         /* a la cabeza */
                for (t = n; t > 0; t--) { strcpy(elegido[t], elegido[t - 1]); memcpy(sub[t], sub[t - 1], 36); }
                strcpy(elegido[0], "heroquest"); memcpy(sub[0], tmp, 36);
            }
            n++;
        }
        nsel = n + 2;                                                /* aventuras, VOLVER A ESCANEAR y VOLVER */
        if (sel >= nsel) sel = 0;
        t = a_codigos(t_cab[en], b);
        tx_cadena_centrada(AV_FILA - 2, b, t, C_YELLOW, NEG);
        if (n == 0) {
            t = a_codigos(t_nin[en], b);
            tx_cadena_centrada(AV_FILA, b, t, C_RED, NEG);
        }
        key_reset();
        for (;;) {
            for (i = 0; i < n; i++) {                                /* "1 SUBTITULO" */
                b[0] = (u8)('1' + i); b[1] = 0x5F;
                memcpy(b + 2, sub[i] + 1, sub[i][0]);
                av_texto(AV_FILA + i, b, sub[i][0] + 2, sel == i);
            }
            t = a_codigos(t_esc[en], b);
            av_texto(AV_FILA + AV_MAX + 1, b, t, sel == n);
            t = a_codigos(t_vol[en], b);
            av_texto(AV_FILA + AV_MAX + 2, b, t, sel == n + 1);
            k = key_wait();
            if (k & (K_DOWN | K_RIGHT)) sel = (sel + 1) % nsel;
            if (k & (K_UP | K_LEFT)) sel = (sel + nsel - 1) % nsel;
            if (k & K_ESC) return;
            if (k & K_FIRE) break;
        }
        if (sel == n + 1) return;                                    /* volver sin cargar nada */
        if (sel == n) continue;                                      /* volver a escanear */
        if (data_aventura(elegido[sel])) { g_mision = 0; return; }
        aviso(3, C_RED);                                             /* NO SE ENCUENTRA */
    }
}

static const u8 glifo_guion[8] = { 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00 };
static const u8 glifo_barra[8] = { 0x00, 0x02, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40 };

/* ESC en el menu principal: pantalla negra con la pregunta; Y o S sale, cualquier otra tecla vuelve al menu */
static int salir_qdos(void)
{
    static u8 fuente[FONT_BYTES];
    static const char *const txt[2] = { "SALIR A QDOS S/N", "EXIT TO QDOS Y/N" };
    const char *t = txt[g_idioma == IDIOMA_EN];
    u8 b[20];
    int n;
    u16 k;
    memcpy(fuente, g_manuscrita ? fnt_manuscrita : fnt_normal, FONT_BYTES);
    memcpy(fuente + (0x40 - 0x30) * 8, glifo_barra, 8);       /* la fuente no tiene '/' */
    for (n = 0; t[n]; n++) b[n] = (u8)(t[n] == ' ' ? 0x5F : t[n] == '/' ? 0x40 : t[n]);
    cls();
    gfx_set_font(fuente);
    tx_cadena_centrada(12 + UI_ROW0, b, n, C_WHITE, NEG);
    gfx_set_font(g_manuscrita ? fnt_manuscrita : fnt_normal);
    plat_tecla4 = 1;
    key_reset();
    k = key_wait();
    plat_tecla4 = 0;
    return (k & K_YES) != 0;
}

void menu_principal(void)
{
    int sel = 0;
    u16 k;
    for (;;) {
        cls(); draw_title();
        for (;;) {
            draw_principal(sel);
            k = key_wait();
            if (k & (K_DOWN | K_RIGHT)) sel = (sel + 1) % 5;
            if (k & (K_UP | K_LEFT)) sel = (sel + 4) % 5;
            if (k & K_ESC) {
                if (salir_qdos()) return;           /* salir del programa */
                cls(); draw_title();
            }
            if (k & K_FIRE) break;
        }
        if (sel == 0) menu_jugar();
        else if (sel == 1) menu_tienda();
        else if (sel == 2) menu_crear();
        else if (sel == 3) menu_controles();
        else menu_cargar_aventura();
    }
}

/* ---------------------------------------------------------------- idioma */
/* Primera pantalla: elige el idioma de las misiones (de momento solo ellas; ventanas, pociones, monstruos y guiones siguen en espanol).
   La fuente no tiene '-' ni '/': se dibujan con una copia de la fuente donde se les ha puesto glifo (codigos 0x3F y 0x40). */
static const char *const idioma_txt[2] = { "1 - ESPANOL / SPANISH", "2 - ENGLISH / INGLES" };
#define IDIOMA_FILA 12

static void draw_idioma(int sel)
{
    int i, j, n;
    u8 b[24];
    for (i = 0; i < 2; i++) {
        for (n = 0; idioma_txt[i][n]; n++) {
            j = idioma_txt[i][n];
            b[n] = (u8)(j == ' ' ? 0x5F : j == '-' ? 0x3F : j == '/' ? 0x40 : j);
        }
        tx_cadena_centrada(IDIOMA_FILA + 3 * i, b, n, i == sel ? C_GREEN : C_MAGENTA, NEG);
    }
}

/* 1 si se ha elegido idioma, 0 si se pulso ESC (salir del programa) */
int menu_idioma(void)
{
    static u8 fuente[FONT_BYTES];
    int sel = g_idioma, ok = 1;
    u16 k;
    memcpy(fuente, fnt_normal, FONT_BYTES);
    memcpy(fuente + (0x3F - 0x30) * 8, glifo_guion, 8);
    memcpy(fuente + (0x40 - 0x30) * 8, glifo_barra, 8);
    gfx_set_font(fuente);
    cls(); draw_title();
    key_reset();
    for (;;) {
        draw_idioma(sel);
        k = key_wait();
        if (k & (K_DOWN | K_RIGHT)) sel = (sel + 1) % 2;
        if (k & (K_UP | K_LEFT)) sel = (sel + 1) % 2;
        if (k & K_1) { sel = IDIOMA_ES; break; }
        if (k & K_2) { sel = IDIOMA_EN; break; }
        if (k & K_ESC) { ok = 0; break; }
        if (k & K_FIRE) break;
    }
    gfx_set_font(g_manuscrita ? fnt_manuscrita : fnt_normal);
    if (!ok) return 0;
    if (!data_idioma(sel)) { printf("\nFaltan ficheros de datos.\n"); return 0; }
    mus_song(2);                           /* musica del menu */
    return 1;
}

/* ---------------------------------------------------------------- datos iniciales */
void game_init(void)
{
    int i, j, x, y, attr, al, len;
    const u8 *s;
    pers_boot(&g_part);
    for (i = 0; i < 4; i++) {
        memset(&heroes[i], 0, sizeof(Heroe));
        heroes[i].activo = 1;
        tx_guion_linea(nom_id[i], 0, &x, &y, &attr, &al, &s, &len);
        for (j = 0; j < len && heroes[i].nlen < 6; j++)
            if (s[j] != 0x72 && s[j] != 0x73) heroes[i].nombre[heroes[i].nlen++] = s[j];
    }
}
