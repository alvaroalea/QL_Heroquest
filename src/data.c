#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u8 *pk_marco = 0, *fnt_normal = 0, *fnt_manuscrita = 0, *txt = 0, *mis_indice = 0, *mus_data = 0, *fx_data = 0;
static u8 *dat_buf = 0, *fnt_buf = 0;     /* heroquest_dat y fonts_dat enteros */
int g_idioma = IDIOMA_ES;

u16 rd16(const u8 *p) { return (u16)((p[0] << 8) | p[1]); }
u32 rd32(const u8 *p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3]; }

int data_error;                                  /* por que fallo data_load: 1 no existe el fichero, 2 sin memoria o lectura incompleta */

u8 *data_load(const char *name, long *size)
{
    char path[96];
    FILE *f;
    long n, got = 0, r;
    u8 *b;
    strcpy(path, plat_data_dir());
    strcat(path, name);
    data_error = 0;
    f = fopen(path, "rb");
    if (!f) { data_error = 1; return 0; }
    fseek(f, 0L, 2); n = ftell(f); fseek(f, 0L, 0);
    b = (u8 *)malloc((size_t)n + 4);
    if (!b) { fclose(f); data_error = 2; return 0; }
    while (got < n && (r = (long)fread(b + got, 1, (size_t)(n - got > 8192L ? 8192L : n - got), f)) > 0) got += r;
    fclose(f);
    if (got != n) { free(b); data_error = 2; return 0; }
    if (size) *size = n;
    return b;
}

static long ret_off, ret_size;                   /* el ultimo trozo de heroquest_dat (los retratos): desplazamiento en el fichero y tamano */

static const u8 *parte(const u8 *buf, int id, long *size)
{
    const u8 *e;
    if (!buf || id < 0 || id >= (int)rd16(buf + 4)) return 0;
    if (buf == dat_buf && id == DAT_RETRATOS) return 0;      /* no esta en memoria */
    e = buf + 8 + 8 * id;
    if (size) *size = (long)rd32(e + 4);
    return buf + rd32(e);
}

const u8 *dat_get(int id, long *size) { return parte(dat_buf, id, size); }
const u8 *fonts_get(int id, long *size) { return parte(fnt_buf, id, size); }

int pack_count(const u8 *pack) { return (int)rd16(pack + 6); }

int pack_get(const u8 *pack, int idx, gimg *g)
{
    const u8 *e;
    if (idx < 0 || idx >= pack_count(pack)) return 0;
    e = pack + 8 + 16 * idx;
    g->d = pack + rd32(e);
    g->w = (int)rd16(e + 4); g->h = (int)rd16(e + 6);
    g->wpr = (int)rd16(e + 8); g->copias = (int)rd16(e + 10); g->mask = (int)(rd16(e + 12) & 1);
    return 1;
}

static u8 *need(const char *name)
{
    u8 *b = data_load(name, 0);
    if (!b) { printf("No se encuentra %s%s\n", plat_data_dir(), name); }
    return b;
}

/* ---------------------------------------------------------------- indices de las aventuras (<aventura>_xx_idx: formato en tools/hq_misiones_empaquetar.py) */
char g_aventura[16] = "heroquest";               /* prefijo de los ficheros de la aventura elegida */
int g_nmis = 14;                                 /* numero de misiones de la aventura */
const u8 *g_subtitulo = 0;                       /* subtitulo (byte de longitud y caracteres) que sale bajo HEROQUEST; longitud 0 = ninguno */

static int idx_valido(const u8 *b, long n)
{
    return b && n >= 48 && b[0] == 'H' && b[1] == 'Q' && b[2] == 'M' && b[3] == 'I' && rd16(b + 4) == 2 && n >= 48 + 40L * rd16(b + 6) && (rd32(b + 8) == 0 || (long)rd32(b + 8) + 12 <= n);
}

static u8 *idx_cargar(const char *aventura, int idioma)
{
    char name[40];
    u8 *b;
    long n;
    sprintf(name, "%s_%s_idx", aventura, idioma == IDIOMA_EN ? "en" : "es");
    b = data_load(name, &n);
    if (b && !idx_valido(b, n)) { free(b); b = 0; }
    return b;
}

static void idx_activar(void)                   /* pone al dia lo que sale del indice de la aventura */
{
    g_nmis = rd16(mis_indice + 6);
    g_subtitulo = mis_indice + 12;
}

void data_mision_nombre(char *name, int n)
{
    sprintf(name, "%s_%02d_%s", g_aventura, n, g_idioma == IDIOMA_EN ? "en" : "es");
}

const u8 *data_mision_titulo(int n, int *len)    /* nombre de la mision n en el menu (caracteres del juego) */
{
    const u8 *e = mis_indice + 48 + 40 * n;
    *len = e[6];
    return e + 7;
}

/* Datos de la aventura que van detras de las entradas del indice (el long de +8 es su desplazamiento, 0 = no hay): tres longs con los desplazamientos, desde su
   inicio, del epilogo, de la tabla de tipos de monstruo y de la de estadisticas (0 = no hay) y esos tres bloques. */
const u8 *data_epilogo(int n)                    /* pagina n del epilogo de la aventura: un guion en el formato de las operaciones de textos_bin (tx_ops) */
{
    const u8 *blk, *q;
    int np, i;
    if (!mis_indice || rd32(mis_indice + 8) == 0) return 0;
    blk = mis_indice + rd32(mis_indice + 8);
    if (rd32(blk) == 0) return 0;
    q = blk + rd32(blk);
    np = rd16(q); q += 2;
    for (i = 0; i < np; i++) {
        if (i == n) return q + 2;
        q += 2 + rd16(q);
    }
    return 0;
}

const u8 *data_estadisticas(const u8 *idx)       /* tabla de estadisticas de monstruo de un indice (idx = 0: el de la aventura activa): n (byte) y n entradas de 9 bytes, 0 si no hay */
{
    const u8 *blk;
    if (!idx) idx = mis_indice;
    if (!idx || rd32(idx + 8) == 0) return 0;
    blk = idx + rd32(idx + 8);
    return rd32(blk + 8) ? blk + rd32(blk + 8) : 0;
}

const u8 *data_tipos(const u8 *idx)              /* tabla de tipos de monstruo de un indice (idx = 0: el de la aventura activa): n (byte) y n entradas de 16 bytes, 0 si no hay */
{
    const u8 *blk;
    if (!idx) idx = mis_indice;
    if (!idx || rd32(idx + 8) == 0) return 0;
    blk = idx + rd32(idx + 8);
    return rd32(blk + 4) ? blk + rd32(blk + 4) : 0;
}


/* ---------------------------------------------------------------- misiones precargadas
   Al elegir idioma o aventura se intenta tener todas las misiones en memoria (cargar una desde el disco al empezar tarda); se deja libre antes la memoria que
   necesita la partida (RESERVA_PARTIDA) y, si no cabe todo, no se precarga ninguna y cada mision se lee del disco al iniciarla. */
#define MIS_MAX 16
static u8 *mis_buf[MIS_MAX];
static long mis_len[MIS_MAX];

static void misiones_liberar(void)
{
    int i;
    for (i = 0; i < MIS_MAX; i++) { if (mis_buf[i]) free(mis_buf[i]); mis_buf[i] = 0; mis_len[i] = 0; }
}

static void misiones_precargar(void)
{
    char name[24];
    u8 *reserva;
    int i;
    misiones_liberar();
    if (g_nmis > MIS_MAX) return;
    reserva = (u8 *)malloc((size_t)RESERVA_PARTIDA);
    if (!reserva) return;
    for (i = 0; i < g_nmis; i++) {
        data_mision_nombre(name, i);
        mis_buf[i] = data_load(name, &mis_len[i]);
        if (!mis_buf[i]) { misiones_liberar(); break; }
    }
    free(reserva);
}


/* La mision n: la precargada o, si no, la lee del disco (NULL si no puede: data_error dice por que). Se devuelve con data_mision_soltar */
u8 *data_mision_obtener(int n, long *size)
{
    char name[24];
    if (n >= 0 && n < MIS_MAX && mis_buf[n]) { *size = mis_len[n]; return mis_buf[n]; }
    data_mision_nombre(name, n);
    return data_load(name, size);
}

void data_mision_soltar(u8 *m)
{
    int i;
    for (i = 0; i < MIS_MAX; i++) if (mis_buf[i] == m) return;       /* precargada: sigue en memoria */
    free(m);
}

/* Elige el idioma: se borran los textos y el indice de la aventura y se vuelven a leer del disco (textos_<idioma> y <aventura>_<idioma>_idx); solo hay un idioma y
   una aventura en memoria. Si no hay indice de la aventura elegida en ese idioma, se vuelve a Hero Quest. 0 si faltan los textos o el indice de Hero Quest. */
int data_idioma(int idioma)
{
    char previa[16];
    misiones_liberar();
    if (mis_indice) free(mis_indice);
    if (txt) free(txt);
    mis_indice = 0; txt = 0;
    g_idioma = idioma;
    txt = need(idioma == IDIOMA_EN ? "textos_en" : "textos_es");
    if (!txt) return 0;
    strcpy(previa, g_aventura);
    mis_indice = idx_cargar(previa, idioma);
    if (!mis_indice) {
        strcpy(g_aventura, "heroquest");
        mis_indice = idx_cargar(g_aventura, idioma);
        if (!mis_indice) { printf("heroquest_xx_idx no vale\n"); return 0; }
    }
    idx_activar();
    misiones_precargar();
    return 1;
}

int data_aventura_info(const char *nombre, u8 *sub)
{
    char path[96];
    u8 h[48];
    FILE *f;
    int ok;
    sprintf(path, "%s%s_%s_idx", plat_data_dir(), nombre, g_idioma == IDIOMA_EN ? "en" : "es");
    f = fopen(path, "rb");
    if (!f) return 0;
    ok = (fread(h, 1, 48, f) == 48 && h[0] == 'H' && h[1] == 'Q' && h[2] == 'M' && h[3] == 'I' && rd16(h + 4) == 2 && h[12] <= 35);
    fclose(f);
    if (ok) memcpy(sub, h + 12, 36);
    return ok;
}

/* Elige otra aventura (<nombre>_xx_idx en la unidad de los datos): su indice sustituye al de la actual (solo hay uno en memoria); 0 si no existe en el idioma elegido,
   y entonces sigue la anterior. El nombre se pasa en minusculas. */
int data_aventura(const char *nombre)
{
    u8 *adv = idx_cargar(nombre, g_idioma);
    if (!adv) return 0;
    misiones_liberar();
    free(mis_indice);
    mis_indice = adv;
    strcpy(g_aventura, nombre);
    idx_activar();
    misiones_precargar();
    return 1;
}

/* ---------------------------------------------------------------- heroquest_dat por fases y los retratos */
static u8 *ret_all;                              /* todos los retratos (el trozo entero, con su tabla HQGF) si cupieron */
static u8 *ret_tab;                              /* si no: la tabla (cabecera HQGF con los desplazamientos de cada retrato dentro del trozo) ... */
static u8 *ret_fallo;                            /* ... el retrato RETRATO_FALLO ... */
static u8 *ret_buf;                              /* ... y un bufer donde se lee del disco el que haga falta */
static long ret_buf_n;

static FILE *abrir_dat(void)
{
    char path[96];
    strcpy(path, plat_data_dir());
    strcat(path, "heroquest_dat");
    return fopen(path, "rb");
}

static int leer_en(FILE *f, long pos, u8 *b, long n)       /* lee n bytes desde pos */
{
    long got = 0, r;
    if (fseek(f, pos, 0) != 0) return 0;
    while (got < n && (r = (long)fread(b + got, 1, (size_t)(n - got > 8192L ? 8192L : n - got), f)) > 0) got += r;
    return got == n;
}

/* Primera fase: todo heroquest_dat menos el ultimo trozo (los retratos): se lee desde el principio hasta su desplazamiento, con la tabla de trozos dentro */
static u8 *dat_cargar(void)
{
    u8 h[8 + 8 * 16], *b = 0;
    long total, n;
    FILE *f = abrir_dat();
    data_error = 1;
    if (!f) return 0;
    fseek(f, 0L, 2); total = ftell(f);
    data_error = 2;
    if (!leer_en(f, 0L, h, 8) || memcmp(h, "HQDT", 4) != 0 || rd16(h + 4) != DAT_NUM || !leer_en(f, 0L, h, 8 + 8 * DAT_NUM)) { fclose(f); return 0; }
    ret_off = (long)rd32(h + 8 + 8 * DAT_RETRATOS); ret_size = (long)rd32(h + 8 + 8 * DAT_RETRATOS + 4);
    n = ret_off;
    if (n < 8 + 8 * DAT_NUM || ret_off + ret_size != total) { fclose(f); return 0; }          /* los retratos tienen que ser el ultimo trozo */
    b = (u8 *)malloc((size_t)n + 4);
    if (b && !leer_en(f, 0L, b, n)) { free(b); b = 0; }
    fclose(f);
    if (b) data_error = 0;
    return b;
}

/* Tercera fase, despues de las misiones: si caben todos los retratos (y sigue libre RESERVA_PARTIDA para la partida) se leen enteros; si no, solo la tabla de
   desplazamientos, el retrato del ataque fallido y un bufer para leer los demas del disco cuando haga falta (y si ni eso cabe, no hay retratos) */
void data_retratos_cargar(void)
{
    u8 *reserva, cab[8];
    FILE *f;
    long tn, i, mx = 0, sz;
    gimg g;
    if (ret_all || ret_tab || !dat_buf) return;
    f = abrir_dat();
    if (!f) return;
    reserva = (u8 *)malloc((size_t)RESERVA_PARTIDA);
    if (reserva) {
        ret_all = (u8 *)malloc((size_t)ret_size + 4);
        if (ret_all && !leer_en(f, ret_off, ret_all, ret_size)) { free(ret_all); ret_all = 0; }
        free(reserva);
    }
    if (!ret_all && leer_en(f, ret_off, cab, 8) && memcmp(cab, "HQGF", 4) == 0) {
        tn = 8 + 16L * rd16(cab + 6);
        ret_tab = (u8 *)malloc((size_t)tn);
        if (ret_tab && !leer_en(f, ret_off, ret_tab, tn)) { free(ret_tab); ret_tab = 0; }
        for (i = 0; ret_tab && i < (long)rd16(cab + 6); i++) {
            pack_get(ret_tab, (int)i, &g);
            sz = (long)g.h * g.wpr * (g.mask ? 4 : 2) * g.copias;
            if (sz > mx) mx = sz;
        }
        if (ret_tab) {
            ret_buf = (u8 *)malloc((size_t)mx);
            ret_fallo = (u8 *)malloc((size_t)mx);
            ret_buf_n = mx;
            if (ret_buf && ret_fallo && pack_get(ret_tab, RETRATO_FALLO, &g)) {
                sz = (long)g.h * g.wpr * (g.mask ? 4 : 2) * g.copias;
                if (!leer_en(f, ret_off + (long)(g.d - ret_tab), ret_fallo, sz)) { free(ret_fallo); ret_fallo = 0; }
            }
            if (!ret_buf) { if (ret_fallo) free(ret_fallo); free(ret_tab); ret_tab = 0; ret_fallo = 0; }
        }
    }
    fclose(f);
}


int data_retrato(int idx, gimg *g)
{
    long sz, pos;
    FILE *f;
    if (ret_all) return pack_get(ret_all, idx, g);
    if (!ret_tab || !pack_get(ret_tab, idx, g)) return 0;
    pos = ret_off + (long)(g->d - ret_tab);                  /* desplazamiento del retrato dentro del fichero */
    sz = (long)g->h * g->wpr * (g->mask ? 4 : 2) * g->copias;
    if (idx == RETRATO_FALLO && ret_fallo) { g->d = ret_fallo; return 1; }
    if (sz > ret_buf_n) return 0;
    f = abrir_dat();
    if (!f) return 0;
    sz = leer_en(f, pos, ret_buf, sz);
    fclose(f);
    if (!sz) return 0;
    g->d = ret_buf;
    return 1;
}

int data_init(void)
{
    dat_buf = dat_cargar();
    if (!dat_buf) { printf("No se puede cargar %sheroquest_dat\n", plat_data_dir()); return 0; }
    pk_marco = (u8 *)dat_get(DAT_MARCO, 0);
    fnt_buf = need("fonts_dat");
    if (!fnt_buf || memcmp(fnt_buf, "HQDT", 4) != 0 || rd16(fnt_buf + 4) < FNT_NUM) { printf("fonts_dat no vale\n"); return 0; }
    fnt_normal = (u8 *)fonts_get(FNT_NORMAL, 0);
    fnt_manuscrita = (u8 *)fonts_get(FNT_MANUSCRITA, 0);
    mus_data = (u8 *)dat_get(DAT_MUSICA, 0);
    fx_data = (u8 *)dat_get(DAT_EFECTOS, 0);
    if (!(pk_marco && fnt_normal && fnt_manuscrita && data_idioma(IDIOMA_ES))) return 0;      /* ... y las misiones (2.ª fase) */
    data_retratos_cargar();                                                                       /* 3.ª fase: los retratos */
    return 1;
}
