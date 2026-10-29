/* HeroQuest QL - la vista isometrica, las figuras y el plano (ver vista.h y docs/ZX_VISTA.md). Cada funcion lleva la direccion de la rutina del
   original. Las capas del mapa son las de Partida: capa A (sala en los bits 0-5; bit 6 visto en el plano, bit 7 visto en 3D), capa B (paredes
   y puertas) y capa C (objetos y monstruos). Una casilla es el indice fila * 26 + columna, como en el original (el relleno de las capas, de
   494 a 511, vale cero). */
#include <stdlib.h>
#include <string.h>
#include "vista.h"
#include "gfx.h"

Anim vz_anim[N_ANIM];
u8   vz_orden[96];

/* ---------------------------------------------------------------- tablas de vista_bin */
#define N_CASILLAS 52
static int t_y[N_CASILLAS], t_x[N_CASILLAS];             /* 0xC414 */
static int sala_tab[43][3];                              /* 0xC47C: columna, fila, color */
static int puerta_a[16][3], puerta_b[8][3];              /* 0xB72B, 0xB76B: dy, dx, escenario */
static int mueble[17][5];                                /* 0x88A0 */
static int base[17][3];                                  /* 0x9470 */
static int suelos[16];                                   /* 0xC342 */
static int c35a[64], ico_heroe[5], ico_obj[16], cte[4];
static int borra[22][2];
static int vecinas[52], zonas[10][5], guiones[52][4], movcte[9], teleport[11][4], nombre_obj[44], oro_carta[9], pocion[7][3], emboscada[4][2], arma_tab[7], contacto[8][2], panel_zona[10][4], panel_mascara[8], hechizo_blanco[12];
static int win_grid[5][6], win_off[30];                  /* B6E7 y B6AC: casilla de la ventana -> numero de casilla y desplazamiento en el mapa */

static const u8 *pk_spr, *pk_esc, *baldosas, *marca;
static u8 *pool, *pool2, *back;                          /* fondos guardados bajo las figuras; el segundo juego y el buffer de composicion de vista_figuras_refrescar */
#define POOL_MAX 12000L                                  /* la sala inicial mas cargada (Witch Lord, mision 9) guarda 5.384 bytes de fondos */

#define ESC_PARED_OESTE 0
#define ESC_PARED_NORTE 1
#define ESC_SUELO 10                                     /* el suelo normal del original: en la tabla 0xC342 es el de los codigos sin rombo propio */
#define ESC_SUELO_0 51                                   /* suelo_0 a suelo_7 (el numero es el paper del atributo de la sala) */
static int rd16s(const u8 *q) { int v = (q[0] << 8) | q[1]; return v > 32767 ? v - 65536 : v; }

static const u8 *seccion(const u8 *d, int n, int *cnt, int *k)       /* n-esima seccion de vista_bin */
{
    int i;
    const u8 *q = d + 8;
    for (i = 0; i < n; i++) q += 4 + 2 * (rd16s(q) * rd16s(q + 2));
    *cnt = rd16s(q); *k = rd16s(q + 2);
    return q + 4;
}

static void leer_sec(const u8 *d, int n, int cnt, int k, int *dst)
{
    int c, kk, ec, ek;
    const u8 *q = seccion(d, n, &ec, &ek);
    for (c = 0; c < cnt; c++)
        for (kk = 0; kk < k; kk++) dst[c * k + kk] = rd16s(q + 2 * (c * ek + kk));
}

int vista_error;                                 /* por que ha fallado vista_datos: 1 faltan datos, 2 vista_bin no es valido, 3 sin memoria */

int vista_datos(const u8 *sprites, const u8 *escenario, const u8 *vista, const u8 *bal, const u8 *mar)
{
    int s, dy, dx, n = 0, ec, ek, i;
    vista_error = 1;
    if (!sprites || !escenario || !vista || !bal || !mar) return 0;
    vista_error = 2;
    if (vista[0] != 'H' || vista[1] != 'Q' || vista[2] != 'V' || vista[3] != 'I' || rd16s(vista + 6) != 29) return 0;
    for (i = 0; i < 29; i++) {       /* todas las secciones con el tamano que espera el codigo */
        static const int cn[29] = { 52, 43, 16, 8, 17, 17, 16, 64, 5, 14, 16, 4, 22, 17, 52, 10, 52, 9, 11, 44, 9, 7, 4, 7, 20, 8, 10, 8, 12 };
        static const int kk[29] = { 2, 3, 3, 3, 5, 3, 1, 1, 1, 1, 1, 1, 2, 3, 1, 5, 4, 1, 4, 1, 1, 3, 2, 1, 7, 2, 4, 1, 1 };
        seccion(vista, i, &ec, &ek);
        if (ec != cn[i] || ek != kk[i]) { vista_error = 20 + i; return 0; }
    }
    {
        int tmp[N_CASILLAS * 2];
        leer_sec(vista, 0, N_CASILLAS, 2, tmp);
        for (i = 0; i < N_CASILLAS; i++) { t_y[i] = tmp[2 * i]; t_x[i] = tmp[2 * i + 1]; }
    }
    leer_sec(vista, 1, 43, 3, &sala_tab[0][0]);
    leer_sec(vista, 2, 16, 3, &puerta_a[0][0]);
    leer_sec(vista, 3, 8, 3, &puerta_b[0][0]);
    leer_sec(vista, 4, 17, 5, &mueble[0][0]);
    leer_sec(vista, 5, 17, 3, &base[0][0]);
    leer_sec(vista, 6, 16, 1, suelos);
    leer_sec(vista, 7, 64, 1, c35a);
    leer_sec(vista, 8, 5, 1, ico_heroe);
    leer_sec(vista, 10, 16, 1, ico_obj);
    leer_sec(vista, 11, 4, 1, cte);
    leer_sec(vista, 12, 22, 2, &borra[0][0]);
    leer_sec(vista, 14, 52, 1, vecinas);
    leer_sec(vista, 15, 10, 5, &zonas[0][0]);
    leer_sec(vista, 16, 52, 4, &guiones[0][0]);
    leer_sec(vista, 17, 9, 1, movcte);
    leer_sec(vista, 18, 11, 4, &teleport[0][0]);
    leer_sec(vista, 19, 44, 1, nombre_obj);
    leer_sec(vista, 20, 9, 1, oro_carta);
    leer_sec(vista, 21, 7, 3, &pocion[0][0]);
    leer_sec(vista, 22, 4, 2, &emboscada[0][0]);
    leer_sec(vista, 23, 7, 1, arma_tab);
    leer_sec(vista, 25, 8, 2, &contacto[0][0]);
    leer_sec(vista, 26, 10, 4, &panel_zona[0][0]);
    leer_sec(vista, 27, 8, 1, panel_mascara);
    leer_sec(vista, 28, 12, 1, hechizo_blanco);
    /* ventana de 6 x 5 casillas: se numeran por diagonales, de la esquina de arriba a la de abajo (tabla 0xB6E7) */
    for (s = 0; s <= 9; s++)
        for (dy = (s < 4 ? s : 4); dy >= (s - 5 > 0 ? s - 5 : 0); dy--) {
            dx = s - dy;
            win_grid[dy][dx] = n;
            win_off[n] = dy * MAP_W + dx;
            n++;
        }
    pk_spr = sprites; pk_esc = escenario; baldosas = bal; marca = mar;
    if (!pool) pool = (u8 *)malloc((size_t)POOL_MAX);
    if (!pool2) pool2 = (u8 *)malloc((size_t)POOL_MAX);
    if (!back) back = (u8 *)malloc((size_t)VISTA_ALTO * SCR_STRIDE);
    vista_error = 3;
    if (!pool || !pool2 || !back) return 0;
    vista_error = 0;
    return 1;
}

void vista_liberar(void)
{
    if (pool) free(pool);
    if (pool2) free(pool2);
    if (back) free(back);
    pool = pool2 = back = 0;
}

/* ---------------------------------------------------------------- dibujo */
int vista_sprite(int idx, gimg *g) { return pack_get(pk_spr, idx, g); }

static int grafico(int pack, int idx, gimg *g) { return pack_get(pack ? pk_esc : pk_spr, idx, g); }

static void dib_esc(int idx, int x, int y)       /* gfx del escenario en coordenadas de la vista */
{
    gimg g;
    if (grafico(1, idx, &g)) gfx_blit_clip(&g, x, VY0 + y);
}

static int paper = 7;                            /* paper del byte de atributo de la sala: elige el grafico del suelo (suelo_0 a suelo_7) */

void vista_borrar_animaciones(void)              /* BORRAR_ANIMACIONES (0x9BE3) */
{
    memset(vz_anim, 0, sizeof(vz_anim));
}

/* Fondo de la vista: el marco original (sus lineas 0xC000 a la pantalla), siempre en blanco. En el original los atributos de las 17 tiras de
   0xC528 cogen el color de la sala (PANEL_ICONOS 0x8BD4) y las 22 tiras de 0xA4BB se ponen a cero (0x9C77); aqui el color de la sala lo da
   el grafico del suelo (suelo_N, N = paper del atributo) y las tiras de borrado se rellenan en blanco. */
void vista_fondo(int ancla)
{
    gimg g, h;
    int i, top;
    paper = (ancla >> 3) & 7;
    if (pk_marco && pack_get(pk_marco, 0, &g)) {
        top = (256 - g.h) / 2;
        if (g.h >= 256) top = 0;
        h = g;
        h.d = g.d + (long)(VY0 - top) * g.wpr * 2;
        h.h = VISTA_ALTO;
        gfx_blit(&h, 0, VY0);
    }
    for (i = 0; i < 22; i++) gfx_cells_fill(borra[i][0], UI_ROW0 + i, borra[i][1], 1, C_WHITE);
}

/* ---------------------------------------------------------------- salas y casillas */
int vista_sala_de(const Partida *p, int col, int fila)           /* 0x89AD */
{
    return p->capa_a[fila * MAP_W + col] & 0x3F;
}

void vista_entrar_sala(Partida *p, int sala)                      /* 0x8923 */
{
    int i;
    int x0 = 0, y0 = 0, anc = 0;
    u8 *fl = &p->sala_flag[sala & 63];
    if (sala < 43) { x0 = sala_tab[sala][0]; y0 = sala_tab[sala][1]; anc = sala_tab[sala][2]; }
    p->v.origen_x = (u8)x0; p->v.origen_y = (u8)y0; p->v.ancla = (u8)anc;
    if (*fl) { p->v.sala_nueva = (u8)~*fl; return; }
    p->v.sala_nueva = 0xFF;
    if ((p->ev & EV_RAGNAR) && sala == 0x16) {            /* se encuentra a Sir Ragnar: todo el mapa a la vista y los monstruos ya no estan ocultos */
        p->pers[4].estado = 1;
        p->v.heroe_evento = p->v.pers_actual;
        for (i = 0; i < MAP_P; i++) p->capa_a[i] |= 0xC0;
        for (i = 0; i < N_MONS; i++) p->mons[i].estado &= 0x7F;
    }
    if (((p->ev & EV_SIN_EQUIPO) && sala == 0x1D) || ((p->ev & EV_CASTILLO) && sala == 0x18) || ((p->ev & EV_CIERRE) && sala == 0x1F)) {
        p->v.evento_mision = 1;                            /* la sala dispara el evento de la mision (mision_evento) y no se marca como visitada */
        return;
    }
    *fl = 0xFF;
}

static void marcar_vista(Partida *p, int c)                        /* 0x98F6: la casilla a la vista en el plano y en 3D, y las contiguas en 3D */
{
    p->capa_a[c] |= 0xC0;
    p->capa_a[c + 1] |= 0x80;
    if (c + 26 < MAP_P) p->capa_a[c + 26] |= 0x80;
}

/* ---------------------------------------------------------------- registros de las figuras */
static Anim vz_basura;                                             /* el original escribia pasada la tabla cuando no quedaban registros libres */

static Anim *anim_libre(void)                                      /* 0x8A2D: 13 registros desde el 4 (los cuatro primeros son de los heroes) */
{
    int i;
    for (i = 4; i < 4 + 13; i++) {
        if (i >= N_ANIM) break;
        if (!vz_anim[i].estado) return &vz_anim[i];
    }
    memset(&vz_basura, 0, sizeof(vz_basura));
    return &vz_basura;
}

static void anim_grafico(Anim *a, int pack, int idx)
{
    gimg g;
    a->pack = (u8)pack; a->idx = idx;
    if (grafico(pack, idx, &g)) { a->w = g.w; a->h = g.h; }
}

static void anim_posicion(Anim *a, int tile)                       /* 0x89CD */
{
    a->y = t_y[tile] + 0x0F;
    a->x = t_x[tile] + 0x18;
    a->clave = (u8)(tile * 3 + 1);
}

/* 0x8A99 / 0x8A9C: puerta como figura de 24 x 50 con los desplazamientos de la tabla, con la clave casilla * 3 */
static Anim *puerta_figura(int tile, const int *e)
{
    Anim *a = anim_libre();
    a->estado = 1;
    a->y = e[0] + t_y[tile];
    a->x = e[1] + t_x[tile];
    a->w = 24; a->h = 50;
    a->tipo = 2;
    a->pack = 1; a->idx = e[2];
    a->clave = (u8)(tile * 3);
    return a;
}

static void pared_directa(int idx, int tile, int dx)               /* 0x8B2C / 0x8B43: 24 x 50 a 39 lineas por encima de la esquina del rombo */
{
    dib_esc(idx, t_x[tile] + dx, t_y[tile] - 39);
}

/* 0x8A7F: paredes y puertas de una casilla de la sala */
static void paredes(const Partida *p, int c, int tile)
{
    int b = p->capa_b[c];
    if (b & 1) {                                                   /* con el bit 0: solo las puertas (como figura) */
        if (!(b & 0x30)) return;
        if ((b & 0x0C) == 8) return;
        puerta_figura(tile, puerta_b[(b & 0x1C) >> 2]);
        return;
    }
    if (b & 0x80) pared_directa(ESC_PARED_NORTE, tile, 24);
    if (b & 0x40) pared_directa(ESC_PARED_OESTE, tile, 0);
    if (b & 0x20) {
        if ((b & 0x0C) == 8) pared_directa(ESC_PARED_NORTE, tile, 24);   /* puerta secreta: parece una pared */
        else pared_directa(puerta_a[(b & 0x0C) >> 2][2], tile, 24);
        return;
    }
    if (b & 0x10) {
        if ((b & 0x0C) == 8) pared_directa(ESC_PARED_OESTE, tile, 0);
        else pared_directa(puerta_a[4 + ((b & 0x0C) >> 2)][2], tile, 0);
    }
}

/* 0x8A40: las puertas de las casillas contiguas (la pared oeste de la de al lado, la pared norte de la de abajo) se dibujan con esta casilla */
static void puertas_vecinas(const Partida *p, int c, int tile)
{
    int b;
    Anim *a;
    b = p->capa_b[c + 1];
    if (b & 0x10) {
        if ((b & 0x0C) == 8) return;                               /* un RET en el original: tampoco se mira la de abajo */
        a = puerta_figura(tile, puerta_a[((b & 0x1C) | 0x20) >> 2]);
        a->clave = (u8)(a->clave + 2);
    }
    if (c + MAP_W >= MAP_P) return;
    b = p->capa_b[c + MAP_W];
    if (b & 0x20) {
        if ((b & 0x0C) == 8) return;
        a = puerta_figura(tile, puerta_a[((b & 0x0C) | 0x20) >> 2]);
        a->clave = (u8)(a->clave + 2);
    }
}

/* 0x87F4: monstruos, cofre, trono y muebles de la casilla */
static void objetos(Partida *p, int c, int tile)
{
    int v = p->capa_c[c];
    Anim *a;
    if (!v) return;
    if (v < 0x30) {                                                /* monstruo: el sprite es su tipo */
        a = anim_libre();
        a->estado = 1; a->tipo = 2;
        anim_grafico(a, 0, partida_tipo(p, p->mons[v - 1].tipo)->sprite);
        anim_posicion(a, tile);
        return;
    }
    if (v < 0x40) return;
    if (v < 0x42) {                                                /* 0x40 cofre (tipo 2) y 0x41 trono (tipo 0): sprites 64 y 65 */
        a = anim_libre();
        a->estado = 1; a->tipo = (u8)(v == 0x40 ? 2 : 0);
        anim_grafico(a, 0, v);
        anim_posicion(a, tile);
        return;
    }
    if (v < 0x4B) return;
    {
        const int *m = mueble[v - 0x4B];
        a = anim_libre();
        a->estado = 1;
        a->tipo = (u8)m[0];
        a->pack = 1; a->idx = m[4];
        a->w = (m[0] + 1) * 8; a->h = m[1];
        anim_posicion(a, tile);
        a->x += m[2];
        a->y += m[3];
    }
}

/* 0x8B64: el rombo del suelo, o el del codigo especial de la casilla */
static void suelo(const Partida *p, int c, int tile)
{
    int v = p->capa_c[c], idx = ESC_SUELO;
    if (v >= 0x30 && v < 0x3C) idx = suelos[v - 0x30];
    if (idx == ESC_SUELO) idx = ESC_SUELO_0 + paper;
    dib_esc(idx, t_x[tile], t_y[tile]);
}

/* 0x875F: una casilla de la sala */
static void casilla_visible(Partida *p, int c, int tile)
{
    marcar_vista(p, c);
    suelo(p, c, tile);
    paredes(p, c, tile);
    objetos(p, c, tile);
    puertas_vecinas(p, c, tile);
}

void vista_calcular_visibles(Partida *p, int pers)                 /* 0x8712 */
{
    Pers *r = &p->pers[pers];
    int sala, t, c, origen;
    sala = vista_sala_de(p, r->col, r->fila);
    r->hab = (u8)sala;
    p->v.hab_actual = (u8)sala;
    vista_entrar_sala(p, sala);
    origen = p->v.origen_y * MAP_W + p->v.origen_x;
    for (t = 0; t < 30; t++) {
        c = origen + win_off[t];
        if (c < MAP_P && (p->capa_a[c] & 0x3F) == sala) casilla_visible(p, c, t);
    }
    vista_calcular_vista(p, pers);
    if (p->v.sala_nueva) {
        p->v.sala_nueva = 0;
        vista_descubrir_monstruos(p);
    }
}

/* 0x8773: casilla de la ventana en la que esta una figura, -1 si queda fuera */
int vista_tile_de(const Partida *p, int col, int fila)
{
    int dx = col - p->v.origen_x, dy = fila - p->v.origen_y;
    if (dx < 0 || dx > 5 || dy < 0 || dy > 4) return -1;
    return win_grid[dy][dx];
}

void vista_actualizar_heroes(Partida *p)                           /* 0x8791 y 0x87CB */
{
    int i, n = (p->ev & EV_RAGNAR) ? 5 : 4, tile;
    for (i = 0; i < n; i++) {
        Pers *r = &p->pers[i];
        Anim *a = &vz_anim[i == 4 ? 13 : i];
        if (!r->estado) continue;
        r->hab = (u8)vista_sala_de(p, r->col, r->fila);
        r->casilla = (u16)(0x6400 + r->fila * MAP_W + r->col);
        if (r->hab != p->v.hab_actual) continue;
        tile = vista_tile_de(p, r->col, r->fila);
        if (tile < 0) continue;
        a->estado = 1; a->tipo = 3;
        anim_grafico(a, 0, r->fotograma);
        anim_posicion(a, tile);
    }
}

/* ---------------------------------------------------------------- movimiento de los heroes */
void vista_marcar(Partida *p, int c) { marcar_vista(p, c); }          /* 0x98FB */
int vista_ventana_off(int t) { return win_off[t]; }

const int *vista_zonas(void) { return &zonas[0][0]; }

int vista_arma(int i) { return arma_tab[i]; }
void vista_contacto(int i, int *dx, int *dy) { *dx = contacto[i][0]; *dy = contacto[i][1]; }
const int *vista_panel_zona(int i) { return panel_zona[i]; }
int vista_panel_mascara(int i) { return panel_mascara[i]; }
int vista_hechizo_blanco(int h) { return hechizo_blanco[h - 1]; }

int vista_oro_carta(int c) { return oro_carta[c]; }
void vista_pocion(int c, int *bit, int *nombre, int *desc) { *bit = pocion[c][0]; *nombre = pocion[c][1]; *desc = pocion[c][2]; }
void vista_emboscada(int i, int *dx, int *dy) { *dx = emboscada[i][0]; *dy = emboscada[i][1]; }

int vista_nombre_objeto(int codigo) { return codigo >= 0x30 && codigo < 0x5C ? nombre_obj[codigo - 0x30] : 0; }

void vista_teleport(int i, int *sala, int *col, int *fila, int *extra)
{
    *sala = teleport[i][0]; *col = teleport[i][1]; *fila = teleport[i][2]; *extra = teleport[i][3];
}

int vista_fotograma_base(int pers, int dir)       /* dir: guion del paso (0xC1B6) */
{
    return movcte[5 + dir] + movcte[pers];
}

int vista_guion_offset(int guion, int paso)       /* posicion en bytes del fotograma paso dentro del guion del original (pruebas) */
{
    int i, n = 0;
    for (i = 0; i < paso; i++) n += 1 + (guiones[guion * 13 + i][1] ? 3 : 0);
    return n;
}

static Anim *anim_de(int pers) { return &vz_anim[pers == 4 ? 13 : pers]; }

void vista_iniciar_paso(Partida *p, int pers, int guion)          /* 0x85D9 */
{
    Anim *a = anim_de(pers);
    a->guion = (u8)guion; a->paso = 0; a->heroe = (u8)pers;
    a->estado = 2;
    p->v.anim_fin = 2;
    vista_calcular_vista(p, pers);
}

void vista_animar_clave(Partida *p, int pers)                      /* 0x85F2 */
{
    Anim *a = anim_de(pers);
    int t;
    if (a->estado != 2) return;
    t = vista_tile_de(p, p->pers[pers].col, p->pers[pers].fila);
    if (t >= 0) a->clave = (u8)(t * 3 + 1);
}

int vista_animando(void)
{
    int i;
    for (i = 0; i < N_ANIM; i++) if (vz_anim[i].estado == 2) return 1;
    return 0;
}

void vista_animar(Partida *p)                                      /* 0xA7C9: un fotograma de cada registro en estado 2 */
{
    int i, sprite, base;
    for (i = 0; i < N_ANIM; i++) {
        Anim *a = &vz_anim[i];
        const int *f, *sig;
        if (a->estado != 2) continue;
        f = guiones[a->guion * 13 + a->paso];
        sig = guiones[a->guion * 13 + a->paso + 1];
        base = p->v.pers_actual == 4 ? 0x18 : 12 * p->v.pers_actual;
        sprite = f[0] + base;
        p->pers[a->heroe].fotograma = (u8)sprite;
        anim_grafico(a, 0, sprite);
        if (f[1]) { a->x += f[2]; a->y += f[3]; a->paso++; p->v.sonido_pend = 1; }       /* 0xA82A: cada desplazamiento suena a paso */
        else if (sig[0] < 0) a->estado = 1;
        else a->paso++;
    }
}

/* 0x6FF6, primera parte: de las 52 posiciones de la vista la mas cercana al cursor y la casilla del mapa que le corresponde */
void vista_cursor_a_casilla(const Partida *p, int x, int y, int *col, int *fila)
{
    int k, best = 0, bd = 255, c, d, cell;
    for (k = 0; k < N_CASILLAS; k++) {                              /* todo en 8 bits, como el original */
        c = ((t_x[k] & 0xFF) + 0x24) & 0xFF;
        d = (c >= x ? c - x : x - c) >> 1;
        c = ((t_y[k] & 0xFF) + 0x0C) & 0xFF;
        c = c >= y ? c - y : y - c;
        d = ((c + d) & 0xFF) >> 1;
        if (d <= bd) { bd = d; best = k; }
    }
    cell = (p->v.origen_y * MAP_W + p->v.origen_x + vecinas[best]) & 0xFFFF;     /* la division del original es de 16 bits sin signo */
    *fila = (cell / MAP_W) & 0xFF; *col = cell % MAP_W;
}

/* ---------------------------------------------------------------- casillas que ve el personaje */
static void aparece_monstruo(Partida *p, int c)                    /* 0xA3B2 */
{
    int v = p->capa_c[c];
    if (v && v < 0x30) p->mons[v - 1].estado = 1;
}

static void marcar_y_vecinas(Partida *p, int c, int col, int fila) /* 0xA39A */
{
    p->capa_a[c] |= 0x40;
    if (col != 0x19) p->capa_a[c + 1] |= 0x80;
    if (fila != 0x12) p->capa_a[c + MAP_W] |= 0x80;
}

static void ver_norte(Partida *p, int col, int fila)               /* 0xA326 */
{
    int c = fila * MAP_W + col, b;
    for (;;) {
        marcar_y_vecinas(p, c, col, fila);
        if (fila == 0) return;
        b = p->capa_b[c];
        if (b & 0x80) return;
        if ((b & 0x20) && !(b & 4)) return;
        c -= MAP_W; fila--;
        aparece_monstruo(p, c);
    }
}

static void ver_sur(Partida *p, int col, int fila)                 /* 0xA344 */
{
    int c = fila * MAP_W + col, b;
    for (;;) {
        marcar_y_vecinas(p, c, col, fila);
        if (fila == 0x12) return;
        c += MAP_W; fila++;
        b = p->capa_b[c];
        if (b & 0x80) return;
        if ((b & 0x20) && !(b & 4)) return;
        aparece_monstruo(p, c);
    }
}

static void ver_oeste(Partida *p, int col, int fila)               /* 0xA363 */
{
    int c = fila * MAP_W + col, b;
    for (;;) {
        marcar_y_vecinas(p, c, col, fila);
        if (col == 0) return;
        b = p->capa_b[c];
        if (b & 0x40) return;
        if ((b & 0x10) && !(b & 4)) return;
        c--; col--;
        aparece_monstruo(p, c);
    }
}

static void ver_este(Partida *p, int col, int fila)                /* 0xA37E */
{
    int c = fila * MAP_W + col, b;
    for (;;) {
        marcar_y_vecinas(p, c, col, fila);
        if (col == 0x19) return;
        c++; col++;
        b = p->capa_b[c];
        if (b & 0x40) return;
        if ((b & 0x10) && !(b & 4)) return;
        aparece_monstruo(p, c);
    }
}

void vista_calcular_vista(Partida *p, int pers)                    /* 0xA2E1 (y 0xA300: en las columnas 12 y 13 se recorren las dos) */
{
    Pers *r = &p->pers[pers];
    int col = r->col, k, cols[2], n = 1;
    void (*dir[4])(Partida *, int, int);
    if (p->ev & EV_CASTILLO) return;
    dir[0] = ver_norte; dir[1] = ver_sur; dir[2] = ver_oeste; dir[3] = ver_este;    /* el orden del original: 0xA326, 0xA344, 0xA363 y 0xA37E */
    cols[0] = col;
    if (col == 0x0C) { cols[0] = 0x0C; cols[1] = 0x0D; n = 2; }
    else if (col == 0x0D) { cols[0] = 0x0D; cols[1] = 0x0C; n = 2; }
    for (k = 0; k < 4; k++) {
        int j;
        for (j = 0; j < n; j++) dir[k](p, cols[j], r->fila);
    }
}

void vista_descubrir_monstruos(Partida *p)                         /* 0xA2C1 */
{
    int i;
    for (i = 0; i < N_MONS; i++)
        if (p->mons[i].estado && p->mons[i].hab == p->v.hab_actual) p->mons[i].estado &= 0x7F;
}

void vista_construir(Partida *p, int pers)                         /* REDIBUJAR_TURNO (0x6E9C) sin el panel */
{
    vista_borrar_animaciones();
    p->v.no_dibujar = 0;
    p->v.redibujar = 0;
    {
        int sala = vista_sala_de(p, p->pers[pers].col, p->pers[pers].fila);
        vista_fondo(sala < 43 ? sala_tab[sala][2] : 0x78);
    }
    vista_calcular_visibles(p, pers);
    vista_actualizar_heroes(p);
}

/* ---------------------------------------------------------------- figuras */
void vista_ordenar(void)                                           /* 0xA4FD */
{
    int i, l;
    memset(vz_orden, 0xFF, sizeof(vz_orden));
    for (i = 0; i < N_ANIM; i++) {
        if (!vz_anim[i].estado) continue;
        l = vz_anim[i].clave;
        if (vz_orden[l] != 0xFF) {
            l--;
            if (vz_orden[l] != 0xFF) l += 2;
        }
        vz_orden[l] = (u8)i;
    }
}

int vista_izquierda(const Anim *a)                                /* punto de la izquierda del grafico (0xA833): los heroes (tipo 3) van al punto, el resto alineado a byte */
{
    int l = a->x - a->w / 2;
    if (a->tipo != 3) l -= ((l % 8) + 8) % 8;
    return l - (((l % 4) + 4) % 4);
}

static void anim_dibujar(const Anim *a)
{
    gimg g;
    if (!grafico(a->pack, a->idx, &g)) return;
    gfx_blit_clip(&g, vista_izquierda(a), VY0 + a->y - a->h);
}

static int rect_figura(const Anim *a, int *x0, int *y, int *wpr, int *rows)      /* el rectangulo de pantalla que cubre la figura (0 si no se dibuja) */
{
    gimg g;
    int x1;
    if (!a->estado || !grafico(a->pack, a->idx, &g)) return 0;
    *x0 = vista_izquierda(a); x1 = *x0 + g.wpr * 4;
    if (*x0 < 0) *x0 = 0;
    if (x1 > SCR_W) x1 = SCR_W;
    if (x1 <= *x0) return 0;
    *y = VY0 + a->y - a->h; *wpr = (x1 - *x0) / 4; *rows = g.h;
    return 1;
}

static void anim_marcar(Anim *a)
{
    a->dib = 1; a->dpack = a->pack; a->didx = a->idx; a->dx = a->x; a->dy = a->y;
}

void vista_figuras_dibujar(void)                                   /* 0xA794 + 0xA534 */
{
    int i, x0, y;
    long n = 0;
    Anim *a;
    if (!pool) return;
    for (i = 0; i < N_ANIM; i++) {                                 /* GUARDAR_FONDOS: todos antes de dibujar ninguno */
        a = &vz_anim[i];
        a->fondo = 0;
        a->dib = 0;
        if (!rect_figura(a, &x0, &y, &a->fwpr, &a->frows)) continue;
        a->fx = x0; a->fy = y;
        if (n + (long)a->fwpr * 2 * a->frows > POOL_MAX) continue;
        a->fondo = pool + n;
        gfx_rect_get(a->fondo, a->fx, a->fy, a->fwpr, a->frows);
        n += (long)a->fwpr * 2 * a->frows;
    }
    vista_ordenar();
    for (i = 0; i < 90; i++)
        if (vz_orden[i] != 0xFF) { a = &vz_anim[vz_orden[i]]; anim_dibujar(a); if (a->estado) anim_marcar(a); }
}

void vista_figuras_borrar(void)                                    /* 0xA8DE */
{
    int i;
    for (i = N_ANIM - 1; i >= 0; i--) {
        Anim *a = &vz_anim[i];
        if (a->fondo) { gfx_rect_put(a->fondo, a->fx, a->fy, a->fwpr, a->frows); a->fondo = 0; }
        a->dib = 0;
    }
}

typedef struct { int x0, y0, x1, y1; } Caja;                       /* x en puntos, y en lineas; fin excluido */

static int caja_toca(const Caja *a, const Caja *b) { return a->x0 < b->x1 && b->x0 < a->x1 && a->y0 < b->y1 && b->y0 < a->y1; }
static void caja_une(Caja *a, const Caja *b)
{
    if (b->x0 < a->x0) a->x0 = b->x0;
    if (b->y0 < a->y0) a->y0 = b->y0;
    if (b->x1 > a->x1) a->x1 = b->x1;
    if (b->y1 > a->y1) a->y1 = b->y1;
}

void vista_figuras_refrescar(void)
{
    Caja ant[N_ANIM], nue[N_ANIM], r;
    u8 hay_ant[N_ANIM], hay_nue[N_ANIM], sucia[N_ANIM];
    int i, x, y, w, h, alguna = 0, cambio, wb;
    long n = 0;
    u8 *real, *t;
    Anim *a;
    if (!pool) return;
    for (i = 0; i < N_ANIM; i++) {
        a = &vz_anim[i];
        hay_ant[i] = a->dib;
        if (a->dib) { ant[i].x0 = a->fx; ant[i].y0 = a->fy; ant[i].x1 = a->fx + a->fwpr * 4; ant[i].y1 = a->fy + a->frows; }
        hay_nue[i] = (u8)rect_figura(a, &x, &y, &w, &h);
        if (hay_nue[i]) { nue[i].x0 = x; nue[i].y0 = y; nue[i].x1 = x + w * 4; nue[i].y1 = y + h; }
        sucia[i] = (u8)(hay_ant[i] != hay_nue[i] ||
            (hay_nue[i] && (a->dpack != a->pack || a->didx != a->idx || a->dx != a->x || a->dy != a->y)));
        if (sucia[i]) {
            if (!alguna) { r.x0 = 256; r.y0 = 1000; r.x1 = 0; r.y1 = 0; }
            if (hay_ant[i]) caja_une(&r, &ant[i]);
            if (hay_nue[i]) caja_une(&r, &nue[i]);
            alguna = 1;
        }
    }
    if (!alguna) return;
    do {                                                           /* las que se solapan con la zona tambien se repintan, para respetar el orden */
        cambio = 0;
        for (i = 0; i < N_ANIM; i++) {
            if (sucia[i]) continue;
            if ((hay_ant[i] && caja_toca(&r, &ant[i])) || (hay_nue[i] && caja_toca(&r, &nue[i]))) {
                if (hay_ant[i]) caja_une(&r, &ant[i]);
                if (hay_nue[i]) caja_une(&r, &nue[i]);
                sucia[i] = 1; cambio = 1;
            }
        }
    } while (cambio);
    wb = ((r.x1 - r.x0) >> 2) * 2;
    real = screen;
    for (y = 0; y < r.y1 - r.y0; y++) memcpy(back + (long)y * SCR_STRIDE, real + (long)(r.y0 + y) * SCR_STRIDE + (r.x0 >> 2) * 2, (size_t)wb);
    screen = back - (long)r.y0 * SCR_STRIDE - (long)(r.x0 >> 2) * 2;       /* las rutinas de gfx pintan ahora en el buffer */
    for (i = N_ANIM - 1; i >= 0; i--) {
        a = &vz_anim[i];
        if (sucia[i] && a->fondo) gfx_rect_put(a->fondo, a->fx, a->fy, a->fwpr, a->frows);
    }
    for (i = 0; i < N_ANIM; i++) {                                 /* los fondos van al otro juego: los de las que no cambian se copian */
        a = &vz_anim[i];
        if (sucia[i]) {
            a->fondo = 0;
            if (!hay_nue[i]) { a->dib = 0; continue; }
            a->fx = nue[i].x0; a->fy = nue[i].y0; a->fwpr = (nue[i].x1 - nue[i].x0) >> 2; a->frows = nue[i].y1 - nue[i].y0;
            if (n + (long)a->fwpr * 2 * a->frows > POOL_MAX) continue;
            a->fondo = pool2 + n;
            gfx_rect_get(a->fondo, a->fx, a->fy, a->fwpr, a->frows);
        } else if (a->fondo) {
            memcpy(pool2 + n, a->fondo, (size_t)a->fwpr * 2 * a->frows);
            a->fondo = pool2 + n;
        } else continue;
        n += (long)a->fwpr * 2 * a->frows;
    }
    t = pool; pool = pool2; pool2 = t;
    for (i = 0; i < 90; i++) {
        if (vz_orden[i] == 0xFF) continue;
        a = &vz_anim[vz_orden[i]];
        if (!sucia[vz_orden[i]] || !hay_nue[vz_orden[i]]) continue;
        anim_dibujar(a);
        anim_marcar(a);
    }
    screen = real;
    for (y = 0; y < r.y1 - r.y0; y++) memcpy(real + (long)(r.y0 + y) * SCR_STRIDE + (r.x0 >> 2) * 2, back + (long)y * SCR_STRIDE, (size_t)wb);
}

/* ---------------------------------------------------------------- plano */
static void celda_copia(u8 *buf, int cx, int cy, const u8 *t)     /* 0x9449: 8 bytes con paso 32 */
{
    int l;
    for (l = 0; l < 8; l++) buf[(cy * 8 + l) * 32 + cx] = t[l];
}

static void celda_or(u8 *buf, int cx, int cy, const u8 *t)        /* 0x945C */
{
    int l;
    for (l = 0; l < 8; l++) buf[(cy * 8 + l) * 32 + cx] |= t[l];
}

static const u8 *baldosa(int i) { return i == 32 ? marca : (i == 33 ? marca + 8 : baldosas + i * 8); }      /* 32 = marca del personaje activo, 33 = voluta de la niebla (marca_activo_bin) */

static void pieza_mueble(u8 *buf, int cx, int cy, int i)          /* 0x93EF: la pieza base de un mueble, con OR, desplazada */
{
    gimg g;
    int bx, l, w, y, linea, byte;
    const int *b = base[i];
    u8 bits[3 * 24];
    if (!pack_get(pk_esc, b[2], &g) || g.w > 24 || g.h > 24) return;
    w = (g.w + 7) / 8;                                             /* el grafico del QL (con mascara) vuelve a ser un bitmap: los puntos opacos */
    memset(bits, 0, sizeof(bits));
    for (y = 0; y < g.h; y++)
        for (l = 0; l < g.w; l++) {
            const u8 *q = g.d + ((long)y * g.wpr + (l >> 2)) * 4;
            if (q[0] & (3 << (6 - 2 * (l & 3)))) bits[y * w + (l >> 3)] |= (u8)(0x80 >> (l & 7));
        }
    for (y = 0; y < g.h; y++)
        for (bx = 0; bx < w; bx++) {
            linea = cy * 8 + y + b[1]; byte = cx + bx + b[0];
            if (linea < 0 || linea >= VISTA_ALTO || byte < 0 || byte >= 32) continue;
            buf[linea * 32 + byte] |= bits[y * w + bx];
        }
}

void vista_plano_buffer(const Partida *p, u8 *buf, u8 *attr)       /* DIBUJAR_MAPA (0x92A0) y DIBUJAR_PLANO (0x94F8) */
{
    int fila, col, c, a, e, v, cx, cy, i;
    memset(buf, 0, (size_t)VISTA_ALTO * 32);
    for (fila = 0; fila < MAP_H; fila++)
        for (col = 0; col < MAP_W; col++) {
            c = fila * MAP_W + col;
            cx = col + PLANO_COL0; cy = fila + PLANO_FIL0;
            a = p->capa_a[c];
            if (a & 0x40) {                                        /* 0x9377 */
                celda_copia(buf, cx, cy, baldosa(c35a[p->capa_b[c] >> 2]));
                v = p->capa_c[c];
                if (!v) continue;
                if (v < 0x30) celda_or(buf, cx, cy, baldosa(p->v.marcas ? 32 : partida_tipo(p, p->mons[v - 1].tipo)->icono));
                else if (v < 0x40) { if (v >= 0x32 && p->v.marcas) continue; celda_or(buf, cx, cy, baldosa(ico_obj[v - 0x30])); }
                else if (v < 0x42) celda_or(buf, cx, cy, baldosa(v == 0x41 ? cte[1] : cte[0]));
                else if (v >= 0x4B) pieza_mueble(buf, cx, cy, v - 0x4B);
                continue;
            }
            if (!(a & 0x80) || col == 0) continue;                 /* 0x9321: no vista en 3D, o la primera columna */
            e = p->capa_b[c];
            if ((e & 0xA0) && fila != 0 && !(p->capa_a[c - MAP_W] & 0x40)) e &= ~0xA0;
            if ((p->capa_b[c] & 0x50) && !(p->capa_a[c - 1] & 0x40)) e &= ~0x50;
            celda_copia(buf, cx, cy, baldosa(c35a[e >> 2]));
        }
    for (i = 0; i < 5; i++) {                                      /* los personajes */
        const Pers *r = &p->pers[i];
        if (p->v.marcas && p->v.pers_actual == r->indice) continue;
        if (!r->estado) continue;
        celda_or(buf, r->col + PLANO_COL0, r->fila + PLANO_FIL0, baldosa(p->v.marcas ? 32 : ico_heroe[r->indice]));
    }
    for (fila = 0; fila < MAP_H; fila++)
        for (col = 0; col < MAP_W; col++) {
            c = fila * MAP_W + col;
            if (!(p->capa_a[c] & 0x3F) || !(p->capa_a[c] & 0x40)) attr[c] = 0x06; else attr[c] = 0x30;
        }
}

static u8 plano_buf[VISTA_ALTO * 32], plano_attr[MAP_N];             /* estaticos: la pila de un programa del QL es pequena */

void vista_plano_dibujar(const Partida *p)                         /* A3C8: pantalla limpia y el plano con los colores de los atributos */
{
    u8 *buf = plano_buf, *attr = plano_attr, t[8];
    int fila, col, l;
    vista_plano_buffer(p, buf, attr);
    gfx_clear();
    for (fila = 0; fila < MAP_H; fila++)
        for (col = 0; col < MAP_W; col++) {
            int a = attr[fila * MAP_W + col];
            for (l = 0; l < 8; l++) t[l] = buf[((fila + PLANO_FIL0) * 8 + l) * 32 + col + PLANO_COL0];
            gfx_glifo(col + PLANO_COL0, UI_ROW0 + fila + PLANO_FIL0, t, (u8)(a & 7), (u8)((a >> 3) & 7));
        }
}
