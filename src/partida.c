/* HeroQuest QL - estado de la partida (ver partida.h). Cada funcion lleva la direccion de la rutina del original. */
#include <string.h>
#include "partida.h"

Partida g_part;

/* ---------------------------------------------------------------- dados */
static u32 rng = 0x2545F491UL;

void dado_semilla(u32 s) { rng = (s & 0xFFFFFFFFUL) | 1UL; }

static u32 azar(void)                 /* congruencial de 32 bits (se enmascara: en el PC unsigned long es de 64) */
{
    rng = (rng * 1664525UL + 1013904223UL) & 0xFFFFFFFFUL;
    return rng >> 8;
}

/* DADO (0x9BD3): C = 3; B = R; RR B; repite B veces (256 si es 0) "DEC C; si llega a 0, C = 6"; DEC C. El acarreo de RR es el que dejara
   el codigo anterior: aqui, uno al azar. */
int dado_con_r(u8 r, int acarreo)
{
    u8 b = (u8)((r >> 1) | (acarreo ? 0x80 : 0));
    int c = 3;
    do {
        c--;
        if (c == 0) c = 6;
        b--;
    } while (b != 0);
    return c - 1;
}

static int guion_on, guion_n, guion_i, guion_v[256];
static u8 ultimo_r;                   /* el valor de R de la ultima tirada: DADO deja en A (no solo en C) el R que leyo */

void dado_guion(const int *v, int n)
{
    int i;
    guion_on = 1; guion_n = n > 256 ? 256 : n; guion_i = 0;
    for (i = 0; i < guion_n; i++) guion_v[i] = v[i];
}

int dado(void)
{
    u32 a;
    if (guion_on) {
        int v = guion_i < guion_n ? guion_v[guion_i++] : 0;
        ultimo_r = (u8)((v & 0x100) ? 0 : ((v & 0x200) ? 5 : 0x55));           /* pruebas: el bit 8 del valor pide R = 0 y el 9 R = 5 */
        return v & 0xFF;
    }
    a = azar();
    ultimo_r = (u8)a;
    return dado_con_r((u8)a, (int)((a >> 8) & 1));
}

int dado_r(void) { return ultimo_r; }

int dado_combate(void)                /* DADO_COMBATE (0x9BC8): tabla 0xAA7B */
{
    static const u8 cara[6] = { 0, 1, 2, 0, 1, 0 };
    return cara[dado()];
}

/* CALCULAR_DADOS (0x6ECD): 1 dado; 2 para los heroes salvo con el bit 3 de +0x13 o el bit 6 de +0x0F; se dobla con el bit 5 de +0x17
   (que se borra, y el bit 5 de +0x10) y con el bit 3 de +0x18 (que se borra); si sale 8 se deja en 6. Tambien pone a cero el fin de turno. */
int dados_mov_calcular(Partida *pa, int pers)
{
    Pers *r = &pa->pers[pers];
    int a = 1;
    pa->v.turno_acabado = 0;
    pa->v.estado_turno = 0;
    if (!(r->r13 & 0x08) && !(r->armadura & 0x40) && pers != 4) a = 2;
    if (r->t17 & 0x20) { r->t17 &= (u8)~0x20; r->efectos &= (u8)~0x20; a *= 2; }
    if (r->t18 & 0x08) { r->t18 &= (u8)~0x08; a *= 2; if (a == 8) a = 6; }
    pa->v.dados_mov = (u8)a;
    return a;
}

/* TIRAR_DADOS (0x6F1C), la parte de las tiradas: suma (dado + 1) de cada dado de movimiento */
int dados_mov_tirar(Partida *pa)
{
    int i, e = 0;
    for (i = 0; i < pa->v.dados_mov; i++) e += dado() + 1;
    pa->v.pm = (u8)e;
    return e;
}

/* ---------------------------------------------------------------- personajes */
static const u8 cuerpo_ini[5] = { 8, 7, 6, 4, 2 };      /* tabla 0x7E4E */

int cuerpo_max(int pers) { return cuerpo_ini[pers]; }

void pers_boot(Partida *p)            /* los registros tal como los trae el original en memoria (0xA9E5 a 0xAA66) */
{
    static const u8 mente[5] = { 2, 3, 4, 6, 6 }, foto[5] = { 0x11, 0x26, 0x29, 0x3E, 0 }, atq[5] = { 3, 2, 2, 1, 1 };
    int i;
    memset(p, 0, sizeof(Partida));
    for (i = 0; i < N_PERS; i++) {
        Pers *r = &p->pers[i];
        r->cuerpo = (u8)(i == 4 ? 0 : cuerpo_ini[i]);
        r->mente = mente[i];
        r->fotograma = foto[i];
        r->ataque = atq[i];
        r->defensa = 2;
        r->indice = (u8)i;
    }
    p->pers[1].armadura = 0x80;
}

void pers_nuevo(Partida *p, int i)    /* 0x60E6 */
{
    Pers *r = &p->pers[i];
    r->cuerpo = cuerpo_ini[i];
    r->armas = r->armadura = r->objetos = 0;
    r->oro = 0;
    r->r11 = r->r12 = r->r13 = 0;
    if (i == 1) r->armadura |= 0x80;
}

void pers_bytes(const Pers *r, u8 *b)
{
    b[0] = r->estado; b[1] = r->col; b[2] = r->fila; b[3] = r->hab;
    b[4] = (u8)r->casilla; b[5] = (u8)(r->casilla >> 8);
    b[6] = r->cuerpo; b[7] = r->mente; b[8] = r->fotograma;
    b[9] = (u8)r->oro; b[10] = (u8)(r->oro >> 8);
    b[11] = r->hechizos[0]; b[12] = r->hechizos[1];
    b[13] = r->objetos; b[14] = r->armas; b[15] = r->armadura; b[16] = r->efectos;
    b[17] = r->r11; b[18] = r->r12; b[19] = r->r13;
    b[20] = r->ataque; b[21] = r->defensa; b[22] = r->hechizo; b[23] = r->t17; b[24] = r->t18; b[25] = r->indice;
}

void mons_bytes(const Mons *m, u8 *b)
{
    u16 c = (u16)(m->estado ? m->casilla + 0x6200 : 0);
    b[0] = m->estado; b[1] = m->col; b[2] = m->fila; b[3] = m->hab;
    b[4] = (u8)c; b[5] = (u8)(c >> 8);
    b[6] = m->cuerpo; b[7] = m->b7; b[8] = m->tipo; b[9] = m->estad;
}

void pers_marcar_muerte(Partida *p, int i)    /* MARCAR_MUERTE (0xA254): pone el bit del personaje en la mascara de muertos y, como la rutina sigue en 0xA25A, tambien en la de ausentes */
{
    if (i < 4) { p->v.muertos |= (u8)(1 << i); p->v.ausentes |= (u8)(1 << i); }
}

/* Equipo que se quita en la mision 5 (laberinto de Melar): copias de 0x5CE7 (se guardan y se ponen a cero) y de 0xA27D-0xA29B (se devuelven).
   Heroes 0 y 1: +0x0D a +0x0F; el elfo: +0x0B a +0x0F; el hechicero: +0x0B a +0x0E. */
static const u8 equipo_ini[4] = { 0x0D, 0x0D, 0x0B, 0x0B }, equipo_n[4] = { 3, 3, 5, 4 }, equipo_off[4] = { 0, 3, 6, 11 };

static u8 *campo(Pers *r, int off)
{
    switch (off) {
    case 0x0B: return &r->hechizos[0];
    case 0x0C: return &r->hechizos[1];
    case 0x0D: return &r->objetos;
    case 0x0E: return &r->armas;
    default:   return &r->armadura;
    }
}

static void equipo_ocultar(Partida *p)
{
    int i, k;
    for (i = 0; i < 4; i++)
        for (k = 0; k < equipo_n[i]; k++) {
            u8 *q = campo(&p->pers[i], equipo_ini[i] + k);
            p->equipo_ocu[equipo_off[i] + k] = *q;
            *q = 0;
        }
}

void pers_restaurar_equipo(Partida *p, int i)
{
    int k;
    if (i < 0 || i > 3) return;
    for (k = 0; k < equipo_n[i]; k++) *campo(&p->pers[i], equipo_ini[i] + k) = p->equipo_ocu[equipo_off[i] + k];
}

int mision_continua(const Partida *p)         /* MISION_CONTINUA (0x6EB5) */
{
    if (p->v.muertos != 0x0F) return 1;
    if (!(p->ev & EV_RAGNAR)) return 0;
    return p->pers[4].estado != 0;
}

/* ---------------------------------------------------------------- mazo de cartas de tesoro */
u8  mazo[MAZO_N + 1];
int mazo_fin;

static int bit_azar(void) { return (int)(azar() & 1); }

void mazo_barajar(u8 c, int (*bit)(void))
{
    int i, k, pasadas, e;
    for (i = 0; i < MAZO_N; i++) mazo[i] = (u8)i;
    mazo[MAZO_N] = 0xFF;
    pasadas = c;                                 /* C = numero de pasadas (0x80 a 0xBF) */
    do {
        for (k = 0; k < MAZO_N - 1; k++) {       /* 24 pares de vecinos por pasada */
            if (bit()) { e = mazo[k + 1]; mazo[k + 1] = mazo[k]; mazo[k] = (u8)e; }
        }
        pasadas--;
    } while (pasadas & 0xFF);
    mazo_fin = MAZO_N;
}

void mazo_barajar_azar(void)
{
    mazo_barajar((u8)(0x80 | ((azar() >> 3) & 0x3F)), bit_azar);
}

int mazo_robar(void)
{
    int a = mazo[0], i = 0;
    for (;;) {                                   /* 25 veces: el siguiente pasa a la posicion actual; se para al copiar el 0xFF */
        mazo[i] = mazo[i + 1];
        if (mazo[i] == 0xFF) break;
        i++;
        if (i == MAZO_N) { mazo[i] = 0xFF; break; }
    }
    mazo_fin = i;
    return a;
}

void mazo_devolver(int carta)
{
    mazo[mazo_fin++] = (u8)carta;
    mazo[mazo_fin] = 0xFF;
}

/* ---------------------------------------------------------------- cargar la mision */
int casilla_indice(int col, int fila) { return fila * MAP_W + col; }       /* DIRECCION_CASILLA (0xA0D4) con la tabla 0xB705 */

static u16 be16(const u8 *q) { return (u16)((q[0] << 8) | q[1]); }
static u32 be32(const u8 *q) { return ((u32)q[0] << 24) | ((u32)q[1] << 16) | ((u32)q[2] << 8) | q[3]; }

void partida_estadisticas(Partida *p, const u8 *tab)
{
    int i, n = tab ? tab[0] : 0;
    memset(p->estadisticas, 0, sizeof(p->estadisticas));
    if (n > N_ESTAD) n = N_ESTAD;
    for (i = 0; i < n; i++) {
        const u8 *q = tab + 1 + 9 * i;
        Estadistica *e = &p->estadisticas[i];
        e->mov = q[0]; e->ataque = q[1]; e->defensa = q[2]; e->oro = (u16)((q[3] << 8) | q[4]);
        e->quien = q[5]; e->objeto = q[6]; e->mensaje = q[7]; e->jefe = q[8];
    }
}

const Estadistica *partida_estadistica(const Partida *p, int estad) { return &p->estadisticas[estad < N_ESTAD ? estad : N_ESTAD - 1]; }

void partida_tipos(Partida *p, const u8 *tab)
{
    int i, n = tab ? tab[0] : 0;
    memset(p->tipos, 0, sizeof(p->tipos));
    if (n > N_TIPOS) n = N_TIPOS;
    for (i = 0; i < n; i++) {
        const u8 *q = tab + 1 + 16 * i;
        TipoMons *t = &p->tipos[i];
        t->retrato = q[0]; t->icono = q[1]; t->sprite = q[2]; t->n = q[3] > 11 ? 11 : q[3]; t->clase = q[4];
        memcpy(t->nombre, q + 5, 11);
    }
}

const TipoMons *partida_tipo(const Partida *p, int tipo) { return &p->tipos[tipo < N_TIPOS ? tipo : N_TIPOS - 1]; }

static int cargar_muertes(Partida *p, const u8 *d, long len, long o)       /* n, y por entrada: estadistica, rutina, nb, na y (nb + na) pares (columna, fila) */
{
    int n = d[o++], i, k, ini = 0, nb, na, e;
    for (i = 0; i < n; i++) {
        if (o + 4 > len) return 0;
        e = d[o]; nb = d[o + 2]; na = d[o + 3];
        if (e >= N_ESTAD || d[o + 1] > MUERTE_EPILOGO || na > 4 || ini + nb + na > N_MUERTE_CEL || o + 4 + 2L * (nb + na) > len) return 0;
        p->muerte[e].tipo = d[o + 1]; p->muerte[e].nb = (u8)nb; p->muerte[e].na = (u8)na; p->muerte[e].ini = (u8)ini;
        for (k = 0; k < nb + na; k++) { p->muerte_cel[ini + k][0] = d[o + 4 + 2 * k]; p->muerte_cel[ini + k][1] = d[o + 5 + 2 * k]; }
        ini += nb + na; o += 4 + 2L * (nb + na);
    }
    return 1;
}

int partida_cargar(Partida *p, int mision, const u8 *d, long len, const u8 *presentes)
{
    long oa, ob, oi, om, oo;
    int i, nm, no;
    Pers *r;
    if (len < 48 || d[0] != 'H' || d[1] != 'Q' || d[2] != 'M' || d[3] != 'S' || be16(d + 6) != (u16)mision) return 0;
    nm = d[20]; no = d[21];
    oa = (long)be32(d + 24); ob = (long)be32(d + 28); oi = (long)be32(d + 32); om = (long)be32(d + 36); oo = (long)be32(d + 40);
    if (oa >= 52 && (be32(d + 48) + 1 > (u32)len)) return 0;
    if (oa >= 56 && (be32(d + 52) + 2 > (u32)len)) return 0;
    if (oa >= 60 && (be32(d + 56) + 2 > (u32)len)) return 0;
    if (nm > N_MONS || oa + MAP_N > len || ob + MAP_N > len || oi + N_IND > len || om + nm * 10L > len || oo + no * 3L > len) return 0;

    memset(&p->v, 0, sizeof(Vars));                    /* 0xA983 a 0xA9D6 a cero */
    p->mision = (u8)mision;
    p->ev = be16(d + 22);                              /* banderas de eventos de la mision */
    p->salidos = 0;
    memcpy(p->capa_a, d + oa, MAP_N);                  /* 0x400 bytes a 0x6200: capas A y B */
    memcpy(p->capa_b, d + ob, MAP_N);
    memset(p->capa_a + MAP_N, 0, MAP_P - MAP_N);       /* el relleno de cada capa */
    memset(p->capa_b + MAP_N, 0, MAP_P - MAP_N);
    memset(p->capa_c, 0, MAP_P);                       /* capa C a cero */
    memset(p->sala_flag, 0, N_SALAS);                  /* ninguna sala visitada (0xC47F...) */

    for (i = 0; i < 4; i++) {                          /* heroes: presentes, posicion, fotograma */
        r = &p->pers[i];
        r->estado = (u8)(presentes[i] ? 0xFF : 0);
        if (!presentes[i]) { p->v.ausentes |= (u8)(1 << i); p->v.muertos |= (u8)(1 << i); }
        r->col = d[8 + i * 3]; r->fila = d[9 + i * 3]; r->fotograma = d[10 + i * 3];
        r->efectos = 0; r->hechizo = 0; r->t17 = 0; r->t18 = 0;
        r->armadura &= 0x8F;
    }
    memcpy(p->ind, d + oi, N_IND);                     /* indicadores a 0x6BB6 */
    memset(p->muerte, 0, sizeof(p->muerte));           /* rutinas de muerte: la normal salvo las de la mision (seccion que empieza en el long de +48) */
    if (oa >= 52 && !cargar_muertes(p, d, len, (long)be32(d + 48))) return 0;
    p->ruta_pasos = 0; p->ruta_n = 0;                  /* ruta de los errantes (seccion que empieza en el long de +52): pasos, n y n tramos (eje y sentido, destino) */
    if (oa >= 56) {
        long o = (long)be32(d + 52);
        int n;
        if (o + 2 > len) return 0;
        n = d[o + 1];
        if (n > N_RUTA || o + 2 + 2L * n > len) return 0;
        p->ruta_pasos = d[o]; p->ruta_n = (u8)n;
        for (i = 0; i < n; i++) { p->ruta[i][0] = d[o + 2 + 2 * i]; p->ruta[i][1] = d[o + 3 + 2 * i]; }
    }
    p->tp_sala = 0; p->tp_n = 0;                       /* teletransporte (seccion de +56): sala, n y n destinos de 5 bytes */
    if (oa >= 60) {
        long o = (long)be32(d + 56);
        int n, k;
        n = d[o + 1];
        if (n > N_TELEPORTE || o + 2 + 5L * n > len) return 0;
        p->tp_sala = d[o]; p->tp_n = (u8)n;
        for (i = 0; i < n; i++) for (k = 0; k < 5; k++) p->tp[i][k] = d[o + 2 + 5 * i + k];
    }
    memset(p->mons, 0, sizeof(p->mons));               /* lista de monstruos (250 bytes a cero y la copia) */
    for (i = 0; i < nm; i++) {
        const u8 *q = d + om + i * 10;
        Mons *m = &p->mons[i];
        m->estado = q[0]; m->col = q[1]; m->fila = q[2]; m->hab = q[3];
        m->casilla = (u16)(q[4] | (q[5] << 8));
        m->cuerpo = q[6]; m->b7 = q[7]; m->tipo = q[8]; m->estad = q[9]; m->tramo = 0;
    }
    for (i = 0; i < no; i++) {                         /* objetos sobre la capa C */
        const u8 *q = d + oo + i * 3;
        p->capa_c[casilla_indice(q[0], q[1])] = q[2];
    }

    r = &p->pers[4];                                   /* el quinto personaje: solo existe, y en otro sitio, en la mision 1 */
    r->estado = 0;
    if (p->ev & EV_RAGNAR) { r->col = 6; r->fila = 1; r->cuerpo = 2; r->fotograma = 0x2C; }

    for (i = 0; i < N_MONS; i++) {                     /* monstruos: habitacion, casilla y su numero en la capa C */
        Mons *m = &p->mons[i];
        int c;
        if (m->estado == 0) continue;
        c = casilla_indice(m->col, m->fila);
        m->hab = (u8)(p->capa_a[c] & 0x3F);
        m->casilla = (u16)c;
        p->capa_c[c] = (u8)(i + 1);
    }

    if (p->ev & EV_SIN_EQUIPO) equipo_ocultar(p);                /* laberinto de Melar: sin equipo ni hechizos */
    if (p->ev & EV_SIN_FUEGO) {                        /* el mago del fuego: el elfo y el hechicero sin hechizos de fuego */
        p->pers[2].hechizos[1] &= 0x38;
        p->pers[3].hechizos[1] &= 0x38;
    }
    return 1;
}
