/* Backend QL (XTC68 / qcc).

   Consola: el runtime de c68 abre una ventana (centrada, con marco rojo y cursor). El juego no la usa (dibuja directo en
   $20000 y lee el teclado con KEYROW), pero el IPC sigue metiendo cada pulsacion en la cola de teclado de esa ventana, que es
   la que tiene el foco. Si nadie la lee, al salir SuperBASIC recibe todas las teclas. Por eso: (1) la ventana se redefine a
   pantalla completa, negra y sin cursor, (2) la cola se vacia al arrancar, en cada plat_present() y al salir (atexit). */
#include "platform.h"
#include "music.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qdos.h>

u8 *screen = (u8 *)0x20000L;               /* pantalla 1 del QL estandar */
u8 plat_raton = RAT_CURSORES;              /* tipo de raton; los cursores solo cuentan con RAT_CURSORES */

/* ql_ipc.asm */
extern long ql_trap3(long d0, long d1, long d2, long d3, long a0, long a1);
extern long ql_trap1(long d0, long d1, long d2, long d3);
extern long ql_ipc(char *blk);
extern void ql_tick_on(void);
extern void ql_tick_off(void);
extern long ql_ticks(void);
extern long ql_isr_set(long on);
extern void ql_qimi_on(void);
extern void ql_qimi_off(void);
extern void ql_qimi_take(long *dx, long *dy);
extern long ql_pe_rptr(long chan, long *rec, long *xy);
extern long ql_dir_open(char *nombre);
extern void ql_dir_close(long canal);

/* Operaciones de trap #3 usadas */
#define IO_FBYTE 0x01L                      /* D3 = timeout; D1.B = byte; ERR.NC (-1) si no hay nada */
#define SD_WDEF  0x0DL                      /* D1.B = color borde, D2.W = ancho borde, A1 -> 4 words w,h,x,y */
#define SD_CURE  0x0EL                      /* activa el cursor */
#define SD_CURS  0x0FL                      /* suprime el cursor */
#define IO_FSTRG 0x03L                      /* D2 = bytes, A1 = buffer, D3 = timeout; D0 = 0 si lo leyo entero */
#define SD_CLEAR 0x20L                      /* borra toda la ventana */
#define FLUSH_MAX 512                       /* tope de bytes por vaciado, por si el canal devolviera datos sin fin */

static long con_in = -1L;                   /* canal QDOS de stdin (lectura) */
static long con_out = -1L;                  /* canal QDOS de stdout (ventana) */
static int tick_up = 0;
static long last_tick = 0;
static int  con_up = 0;                     /* plat_init hecho y plat_shutdown pendiente */
static short win_def[4];                    /* ancho, alto, x, y */
static short orig_mode = -1;                /* modo de pantalla con que se arranco (0 = modo 4, 8 = modo 8): se restaura al salir */

/* Vacia la cola de teclado de la consola sin esperar (timeout 0). */
void plat_flush_input(void)
{
    int n;
    if (con_in < 0L) return;
    for (n = 0; n < FLUSH_MAX; n++)
        if (ql_trap3(IO_FBYTE, 0L, 0L, 0L, con_in, 0L) != 0L) break;
}

/* Ventana de consola a pantalla completa, negra y sin cursor (tras arrancar) */
static void con_setup(void)
{
    if (con_out >= 0L) {
        win_def[0] = 512; win_def[1] = 256; win_def[2] = 0; win_def[3] = 0;   /* pantalla completa */
        ql_trap3(SD_WDEF, 0L, 0L, -1L, con_out, (long)win_def);              /* borde negro, ancho 0 */
        ql_trap3(SD_CURS, 0L, 0L, -1L, con_out, 0L);                         /* sin cursor */
    }
}

void plat_init(void)
{
    short mode = -1, tv = -1;              /* -1 = leer el modo actual */
    mt_dmode(&mode, &tv);
    if (orig_mode < 0 && (mode == 0 || mode == 8)) orig_mode = mode;
    if (mode != 8) { mode = 8; tv = -1; mt_dmode(&mode, &tv); }   /* modo 8: 256 x 256, 8 colores; si el boot ya lo puso no se toca (borraria la pantalla de carga) */

    con_in  = (long)fgetchid(stdin);
    con_out = (long)fgetchid(stdout);

    con_setup();
    plat_flush_input();
    if (!con_up) { con_up = 1; atexit(plat_shutdown); }
#ifndef NOTICK
    if (!tick_up) {
        ql_tick_on(); tick_up = 1; last_tick = ql_ticks();
#ifndef NOISR
        mus_isr_modo(1); ql_isr_set(1L);     /* la musica corre en la tarea de 50 Hz, no en el bucle principal */
#endif
    }
#endif
}

/* Teclado: matriz del QL por IPC (KEYROW, orden 9 del 8049). Cada fila es una llamada lenta.
   Codigo de tecla de sQLux (include/qlkeys.h): fila = 7 - (codigo >> 3), bit = codigo & 7. */
static long kr_blk[2];                      /* long: direccion par */

static int kr_row(int row)
{
    char *b = (char *)kr_blk;
    long r;
    b[0] = 9; b[1] = 1; b[2] = 0; b[3] = 0; b[4] = 0; b[5] = 0; b[6] = (char)row; b[7] = 2;
    r = ql_ipc(b);
    return (r < 0L) ? 0 : (int)(r & 0xFF);
}

static const struct { u8 code; u16 bit; } kmap[] = {
    { 0x22, K_DOWN },  /* K */          { 0x17, K_UP },    /* O */
    { 0x29, K_LEFT },  /* Z */          { 0x03, K_RIGHT }, /* X */
    { 0x36, K_FIRE },  /* espacio */
    { 0x32, K_UP },    { 0x37, K_DOWN }, { 0x31, K_LEFT }, { 0x34, K_RIGHT },   /* cursores */
    { 0x33, K_ESC },
    { 0x0D, K_0 }, { 0x1B, K_1 }, { 0x09, K_2 }, { 0x19, K_3 }, { 0x2E, K_M },
    { 0x3E, K_4 },     /* las ultimas tres: solo se leen con plat_tecla4 */
    { 0x16, K_YES },   /* Y */
    { 0x23, K_YES }    /* S */
};
#define NKMAP (sizeof(kmap) / sizeof(kmap[0]))

u8 plat_tecla4 = 0;

/* QIMSI (interfaz PS/2 en el puerto de ROM; registros del manual y protocolo del driver de Peter Graf, qimsi_mouse.zip, y de
   qimsi.js de QLRun): solo hay lecturas. $FEDF = estado (bit 0: hay un byte; bit 4: el raton es IntelliMouse, paquetes de 4 bytes
   en vez de 3), $FEDD = byte actual y $FEDE = leerlo lo da por recibido. Paquete: cabecera (bit 3 siempre 1, bits 0 a 2
   botones izq., der., central), X e Y (enteros de 8 bits con signo, Y positiva = arriba) y, solo con rueda, la rueda.
   Un paquete tarda 3,7 ms y el driver original lee un byte por pasada del planificador (50 Hz); aqui, con un paquete a medias,
   se espera el siguiente byte con un tope. */
#define QS_COD  (*(volatile u8 *)0xFEDDL)
#define QS_ACK  (*(volatile u8 *)0xFEDEL)
#define QS_EST  (*(volatile u8 *)0xFEDFL)
#define QS_ESPERA 800                       /* vueltas de espera por un byte (~4 ms) */
#define QS_MAXB   16                        /* bytes por llamada como mucho (un bus que lea siempre 1 en el bit 0 no cuelga el juego) */

static int qs_dx, qs_dy;                    /* cuentas pendientes de entregar a plat_raton_delta */
static u16 qs_boton;                        /* K_FIRE si el boton izquierdo esta pulsado */
static int qs_n, qs_len;                    /* bytes del paquete ya leidos y largo del paquete */
static u8 qs_pk[4];
static volatile u8 qs_basura;

static void qimsi_leer(void)
{
    int w = 0, nb = 0;
    while (nb < QS_MAXB) {
        u8 e = QS_EST;
        if (e & 1) {
            u8 b = QS_COD;
            qs_basura = QS_ACK;                           /* libera el byte; c68 -O elimina una lectura volatil cuyo valor se descarta */
            w = 0;
            nb++;
            if (qs_n == 0) {
                if (!(b & 8)) continue;                   /* sin sincronia: descarta hasta una cabecera */
                qs_len = (e & 0x10) ? 4 : 3;
            }
            qs_pk[qs_n++] = b;
            if (qs_n < qs_len) continue;
            qs_n = 0;
            qs_dx += (int)(signed char)qs_pk[1];
            qs_dy -= (int)(signed char)qs_pk[2];          /* Y positiva = arriba en el raton */
            qs_boton = (qs_pk[0] & 1) ? K_FIRE : 0;
        } else if (qs_n == 0 || ++w > QS_ESPERA) {
            if (w > QS_ESPERA) qs_n = 0;                  /* paquete perdido: vuelve a sincronizar */
            break;
        }
    }
}

/* QIMI (interfaz de raton de Dave Westbury, tambien emulada por Aurora y SuperHermes): $1BF9C = botones (bit 4 derecho, bit 5 izquierdo,
   a 0 = pulsado). Los movimientos llegan como interrupciones externas, una por pulso, y los cuenta ql_ipc.asm; por eso se instala
   la rutina la primera vez que se usa y se quita al salir. */
#define QI_BOT  (*(volatile u8 *)0x1BF9CL)
static u8 qi_up;

static void qimi_instala(void)
{
    if (qi_up) return;
    ql_qimi_on();
    qi_up = 1;
}

/* Pointer Environment: IOP.RPTR sobre el canal de la consola, con terminacion $30 (vuelve siempre ya). D1 trae la posicion absoluta del
   puntero en pantalla (x en la palabra alta, y en la baja) y el registro de puntero, en el byte 11, si hay un boton pulsado. El movimiento
   es la diferencia con la lectura anterior; la x del PE va siempre en la rejilla de 512 puntos (en modo 8 el puntero arranca en x = 256, y = 128), por eso se divide entre 2. IOP.PINF ($70) dice si el Pointer Environment esta cargado. */
static long pe_rec[6];
static long pe_xy;
static int pe_x, pe_y;
static int pe_hay;                          /* ya hay una posicion anterior */
static u16 pe_boton;

static void pe_leer(void)
{
    int x, y;
    if (con_out < 0L || ql_pe_rptr(con_out, pe_rec, &pe_xy) != 0L) return;
    x = (int)(short)(pe_xy >> 16) >> 1; y = (int)(short)(pe_xy & 0xFFFFL);
    if (pe_hay) { qs_dx += x - pe_x; qs_dy += y - pe_y; }
    pe_x = x; pe_y = y; pe_hay = 1;
    pe_boton = (((u8 *)pe_rec)[11] != 0) ? K_FIRE : 0;
}

int plat_raton_auto(void)
{
    u8 e = QS_EST;
    return ((e & 0xEE) == 0 && (e & 0x10)) ? RAT_QIMSI : RAT_CURSORES;   /* con raton sin rueda no hay bit 4: se elige a mano en el menu */
}

int plat_raton_ok(int modo)
{
    if (modo == RAT_QIMI) return (QI_BOT & 0x30) == 0x30;       /* sin boton pulsado los bits 4 y 5 valen 1; no hay deteccion mejor */
    if (modo == RAT_PE) return ql_trap3(0x70L, 0L, 0L, -1L, con_out, 0L) == 0L;
    return modo == RAT_QIMSI && (QS_EST & 0xEE) == 0;     /* el registro real solo puede valer $00, $01, $10 o $11 (qimsi.js de QLRun); lo demas es el bus del puerto de ROM (sQLux con QSound da $75). Sin rueda y sin byte vale 0, igual que un bus a 0: ese caso no se distingue */
}

int plat_raton_estado(void) { return QS_EST; }

void plat_raton_delta(int *dx, int *dy)
{
    *dx = qs_dx; *dy = qs_dy;
    qs_dx = qs_dy = 0;
    if (qi_up) {
        long ax, ay;
        ql_qimi_take(&ax, &ay);
        *dx += (int)ax; *dy += (int)ay;
    }
}

u16 plat_keys(void)
{
    int rows[8], i, r;
    unsigned int need = 0;
    u16 k = 0;
    int n = plat_tecla4 ? (int)NKMAP : (int)NKMAP - 3;
    for (i = 0; i < n; i++) need |= 1u << (7 - (kmap[i].code >> 3));
    for (r = 0; r < 8; r++) rows[r] = (need & (1u << r)) ? kr_row(r) : 0;
    for (i = 0; i < n; i++) {
        if (i >= 5 && i <= 8 && plat_raton != RAT_CURSORES) continue;       /* entradas de los cursores */
        if (rows[7 - (kmap[i].code >> 3)] & (1 << (kmap[i].code & 7))) k |= kmap[i].bit;
    }
    if (plat_raton == RAT_QIMSI) { qimsi_leer(); k |= qs_boton; }
    else if (plat_raton == RAT_PE) { pe_leer(); k |= pe_boton; }
    else if (plat_raton == RAT_QIMI) { qimi_instala(); if (!(QI_BOT & 0x20)) k |= K_FIRE; }
    return k;
}

/* Espera de tecla: lee la matriz con el mismo comando IPC */
static int wk_any(void)                      /* alguna tecla (salvo SHIFT/CTRL/ALT) */
{
    int r, v;
    for (r = 0; r < 8; r++) {
        v = kr_row(r);
        if (r == 7) v &= 0xF8;
        if (v) return 1;
    }
    return 0;
}

/* Sustituye a _envsetup del runtime de c68 4.24f, que Cinit llama antes de main. El original copia los directorios por
   defecto de Toolkit II (punteros a cadenas QDOS en SV+$AC, $B0 y $B4) a PROG_USE, DATA_USE y SPL_USE sin comprobar nada:
   sin Toolkit II los punteros son 0 y lee la cadena de la direccion 0 de la ROM, cuya primera palabra es la longitud.
   En la ROM JS vale 3 (3 bytes de basura), en Minerva $4C02: copia 19458 bytes en un bufer de 50 de la pila y el
   programa se cuelga antes de llegar a main. Este hace lo mismo que el original (lista de entorno del padre o de la
   extension *ENV* de SuperBASIC), pero solo acepta un puntero par dentro de la RAM y una longitud de 1 a 49. */
static void tk2_default(char *name, long off)
{
    char *q = *(char **)(_sys_var + off);
    char *s, *e;
    int n, i;
    e = getenv(name);
    if (e && *e) return;                                   /* el entorno manda, como en el original */
    if (q <= _sys_var || q >= *(char **)(_sys_var + 0x20) || ((long)q & 1L)) return;   /* fuera de la RAM (SV_RAMT) */
    n = *(short *)q;
    if (n < 1 || n > 49) return;
    s = (char *)malloc(strlen(name) + n + 2);
    if (!s) return;
    strcpy(s, name);
    e = s + strlen(s);
    *e++ = '=';
    for (i = 0; i < n; i++) *e++ = q[2 + i];
    *e = 0;
    putenv(s);
}

/* Unidad (o directorio) de los ficheros del juego: la variable DIR$ de SuperBASIC, que pone el boot (DIR$="FLP1_"). No es una variable de
   entorno: se busca en la tabla de nombres de BASIC (entradas de 8 bytes: uso, tipo (1 = cadena), desplazamiento del nombre en la lista de
   nombres y desplazamiento del valor en el area de valores; las cadenas son una palabra de longitud y los caracteres) */
static char dir_basic[48];

static void leer_dir(void)
{
    char *bv, *nt, *ntb, *nl, *vv, *q;
    int n, i;
    dir_basic[0] = 0;
    _super();
    bv  = *(char **)(_sys_var + 0x10) + 0x68;
    ntb = bv + *(long *)(bv + 0x18);                       /* BV_NTBAS */
    nt  = bv + *(long *)(bv + 0x1C);                       /* BV_NTP */
    nl  = bv + *(long *)(bv + 0x20);                       /* BV_NLBAS */
    vv  = bv + *(long *)(bv + 0x28);                       /* BV_VVBAS */
    for (; nt > ntb; ) {
        nt -= 8;
        if (nt[1] != 1 || nt[0] != 2) continue;
        q = nl + *(short *)(nt + 2);
        if (q[0] != 4 || (q[1] & 0xDF) != 'D' || (q[2] & 0xDF) != 'I' || (q[3] & 0xDF) != 'R' || q[4] != '$') continue;
        if (*(long *)(nt + 4) < 0) break;                  /* DIR$ sin valor */
        q = vv + *(long *)(nt + 4);
        if ((long)q & 1L) break;
        n = *(short *)q;
        if (n < 1 || n > 40) break;
        for (i = 0; i < n; i++) dir_basic[i] = q[2 + i];
        dir_basic[n] = 0;
        break;
    }
    _user();
}
void _envsetup(char *env)
{
    char *p = 0, *bv, *nt, *ntb, *nl;
    leer_dir();                                            /* lo primero: la unidad de los ficheros */
    if (env && !((long)env & 1L) && *(short *)env == 0x4AFC)
        p = *(char **)(env + 2);                           /* entorno pasado por el trabajo padre */
    else {
        _super();                                          /* que SuperBASIC no mueva sus tablas mientras se recorren */
        bv  = *(char **)(_sys_var + 0x10) + 0x68;          /* SV_BASIC: variables de BASIC */
        ntb = bv + *(long *)(bv + 0x18);                   /* BV_NTBAS */
        nt  = bv + *(long *)(bv + 0x1C) - 8;               /* BV_NTP: ultima entrada */
        nl  = bv + *(long *)(bv + 0x20);                   /* BV_NLBAS */
        for (; nt >= ntb; nt -= 8)
            if (nt[0] == 8 && nt[1] == 0 && !strncmp("*ENV*", nl + *(short *)(nt + 2) + 1, 5)) {
                p = *(char **)(*(char **)(nt + 4) + 2);  /* funcion *ENV* de la extension ENV */
                break;
            }
        _user();
    }
    putenv("");
    if (p)
        for (; *p; p += strlen(p) + 1) putenv(p);
    tk2_default("PROG_USE", 0xACL);
    tk2_default("DATA_USE", 0xB0L);
    tk2_default("SPL_USE", 0xB4L);
}

/* directorios por defecto de Toolkit II (DATA_USE / PROG_USE), que _envsetup deja en el entorno al arrancar;
   sin Toolkit II quedan vacios. */
int plat_default_dir(int prog, char *buf, int max)
{
    char *e = getenv(prog ? "PROG_USE" : "DATA_USE");
    int n = 0;
    buf[0] = 0;
    if (!e) return 0;
    while (e[n] && n < max - 1) { buf[n] = e[n]; n++; }
    buf[n] = 0;
    return buf[0] != 0;
}

/* Prefijo de todos los ficheros (datos, misiones, heroes guardados): DIR$ de SuperBASIC; si no esta definida, el directorio de datos de
   Toolkit II si lo hay y, si no, la unidad de disco 1 */
const char *plat_data_dir(void)
{
    static char dir[64];
    if (dir_basic[0]) return dir_basic;
    if (!plat_default_dir(0, dir, sizeof(dir) - 1)) strcpy(dir, "flp1_");
    return dir;
}

/* Lista la unidad de datos: se abre el dispositivo (p. ej. "flp1_") como directorio y se leen entradas de 64 bytes: +14 longitud del nombre (palabra) y +16 el nombre */
int plat_listar(const char *suf, char nombres[][16], int max)
{
    const char *d = plat_data_dir();
    long qa[16];
    long ch;
    u8 e[64];
    char *qn = (char *)qa;
    int n = 0, ls = (int)strlen(suf), ld = (int)strlen(d), l, i, c;
    if (ld > 40) return 0;
    qn[0] = 0; qn[1] = (char)ld;
    memcpy(qn + 2, d, (size_t)ld);
    ch = ql_dir_open(qn);
    if (ch < 0L) return 0;
    while (n < max && ql_trap3(IO_FSTRG, 0L, 64L, -1L, ch, (long)e) == 0L) {
        l = (e[14] << 8) | e[15];
        if (l <= ls || l > 15 + ls || l > 36) continue;
        for (i = 0; i < ls; i++) {
            c = e[16 + l - ls + i];
            if (c >= 'A' && c <= 'Z') c += 32;
            if (c != suf[i]) break;
        }
        if (i < ls) continue;
        for (i = 0; i < l - ls; i++) {
            c = e[16 + i];
            nombres[n][i] = (char)((c >= 'A' && c <= 'Z') ? c + 32 : c);
        }
        nombres[n][l - ls] = 0;
        n++;
    }
    ql_dir_close(ch);
    return n;
}

void plat_snapshot(const char *name) { (void)name; }

void plat_wait_key(void)
{
    while (wk_any()) plat_flush_input();    /* primero que suelte la tecla con la que lanzo el programa */
    while (!wk_any()) plat_flush_input();
    while (wk_any()) plat_flush_input();    /* y que la suelte: asi SuperBASIC no la recibe */
    plat_flush_input();
}

unsigned long plat_ticks(void) { return tick_up ? (unsigned long)ql_ticks() : 0UL; }

/* Se llama una vez por fotograma desde el bucle: vacia la cola de teclado y espera al siguiente cuadro de 50 Hz */
int plat_present(void)
{
    unsigned long now;
    long n = 1;
    plat_flush_input();
    if (tick_up) {
        while (ql_ticks() == last_tick) { }
        now = ql_ticks();
        n = (long)(now - last_tick);        /* cuadros de 50 Hz desde la ultima vez: si el bucle tarda mas de 20 ms hay que recuperarlos */
        last_tick = now;
    }
    if (n > 150) n = 150;                   /* tras una pausa muy larga (carga de ficheros) no se recupera mas de 3 s */
    if (!mus_en_isr()) { mus_mute(0); mus_tick_n((int)n); }                     /* la musica avanza un fotograma de 50 Hz por cuadro; los perdidos, sin sonar */
    return (int)n;
}

void plat_shutdown(void)
{
    if (qi_up) { ql_qimi_off(); qi_up = 0; }
    if (tick_up) { ql_isr_set(0L); mus_isr_modo(0); ql_tick_off(); tick_up = 0; }      /* antes de terminar: la tarea del reloj esta en nuestro codigo */
    if (!con_up) return;                    /* idempotente: tambien lo llama atexit */
    con_up = 0;
    plat_flush_input();
    if (orig_mode >= 0 && orig_mode != 8) {      /* el modo con que arranco el usuario */
        short mode = orig_mode, tv = -1;
        mt_dmode(&mode, &tv);
        con_setup();
    }
    if (con_out >= 0L) {
        ql_trap3(SD_CURE, 0L, 0L, -1L, con_out, 0L);                         /* devuelve el cursor */
    }
}

/* Salida sin el "Press a key to exit" del runtime de c68: se limpia la consola y el trabajo se elimina a si mismo
   con MT.FRJOB (trap #1, D0=5, D1=-1 = este trabajo, D3 = codigo de error 0). No vuelve. */
void plat_exit(void)
{
    plat_shutdown();
    ql_trap1(5L, -1L, 0L, 0L);
}
