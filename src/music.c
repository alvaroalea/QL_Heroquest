/* HeroQuest QL - motor de musica: traduccion a C de REPRODUCTOR (banco 4 del Spectrum 128K). Se ha respetado el orden de las
   operaciones del Z80 (hay efectos de borde que dependen de el). Las direcciones de los datos son las del original (0xC000+).
   Los efectos de sonido no se calculan: el original los lanza sobre la melodia 0 (silencio) y siempre suenan igual, asi que se reproducen sus escrituras al AY
   fotograma a fotograma (data/efectos_bin, tools/hq_efectos_empaquetar.py), al final de este fichero. */
#include "music.h"
#include "qsound.h"

#define DATA_END   5000
#define BASE       0xC000U

static const u8 *md;                     /* datos originales */
static const u8 *tper, *trui, *tvib;     /* periodos (palabras little endian), periodo de ruido y amplitud de vibrato */
static int vib_escala;

#define BY(a)   (md[(unsigned)(a) - BASE])
#define WD(a)   ((u16)(BY(a) | (BY((a) + 1) << 8)))
#define PER(i)  ((u16)(tper[((i) & 0x7F) * 2] | (tper[((i) & 0x7F) * 2 + 1] << 8)))

typedef struct {
    int  sp;                             /* pila de bucles y subpatrones (IX+0/1: IY), crece hacia abajo desde 32 */
    u8   stk[32];
    u16  pc;                             /* +2  puntero del patron */
    u16  vl_ini;                         /* +4  lista (duracion, nivel) del instrumento */
    u16  ts_cur, ns_cur, vl_cur;         /* +6, +8, +A: secuencia de tono, de ruido y lista de volumen en curso */
    u16  ts_ini, ns_ini;                 /* +C, +E */
    u16  pl_ini, pl_cur;                 /* +10, +12: lista de patrones */
    u8   f14, f15, f16, f17, f18, f19, f1a;
    u8   maxvol, m_ton, m_toff, m_non, m_noff;   /* +1E, +1F, +20, +21, +22 */
    u8   trans, dur, cnt, gdelay;        /* +23 transposicion, +24 duracion, +25 cuenta de la nota, +26 retardo del deslizamiento */
    u16  gstep, per, gtarget;            /* +27, +29, +2B */
    u8   vdelay, vcnt, vsrc, vamp;       /* +2D, +2E, +2F, +30 */
    u8   vper, vc, vval;                 /* +32, +33, +34 */
    u8   lcnt, note, nidx;               /* +36, +37, +38 */
    u8   nb_dur, nb_cnt, nb_per, inst;   /* +39, +3A, +3B, +3C */
    u16  outper;                         /* +3D */
    u8   noiseper, outvol, busy;         /* +3F, +40, +41 */
} Chan;

static Chan ch[3];
static volatile int lock;                /* > 0 mientras el programa principal toca el estado: la interrupcion no avanza la musica */
static int quiet, muted, isr_modo;                 /* quiet: el fotograma avanza el estado pero no escribe en el chip; muted: volumenes a 0 */

static void wr(int reg, int val) { if (!quiet) qs_write(reg, val); }
static u8 mix, stopped, mark, tempo, tcount, fade_n, fade_v, d506;   /* DD88, DD89, DD8D, DD8E, DD8F, DD8B, DD8C, D506 */

static u16 mul16(u16 bc, u16 de, int bits)       /* DBEE */
{
    u16 hl = 0;
    while (bits--) { if (de & 1) hl = (u16)(hl + bc); de >>= 1; bc = (u16)(bc << 1); }
    return hl;
}

static u16 div16(u16 bc, u16 de)                 /* DBCD: BC = BC / DE */
{
    unsigned hl = 0, bcv = bc;
    int i, carry;
    for (i = 0; i < 16; i++) {
        carry = (bcv >> 15) & 1; bcv = (bcv << 1) & 0xFFFF;
        hl = ((hl << 1) | carry) & 0xFFFF;
        if (hl >= de) { hl -= de; bcv |= 1; }
    }
    return (u16)bcv;
}

/* DB11: empieza una nota (a = indice sin transponer) */
static void nota(Chan *c, u8 a)
{
    u8 e, v, b, cc;
    c->lcnt = 0;
    a = (u8)(a + c->trans);
    c->note = a;
    e = (u8)(a & 0x7F);
    c->per = c->outper = PER(e);
    if (c->f19) { c->noiseper = trui[e]; c->nidx = e; }
    c->cnt = c->dur;
    if (c->f16) {
        c->vval = 0;
        cc = c->vsrc;
        if (e >= 0x3C) v = 0x42; else v = tvib[e];
        if (v != 0 && (v & 0x0F) != 0) {
            b = (u8)(v & 0x0F);
            if (v & 0x80) { if (cc >= b) cc = b; }
            else if (v & 0x40) cc = (u8)(cc >> b);
            else cc = (u8)(cc << b);
        }
        if (vib_escala != 64 && cc) { cc = (u8)((cc * vib_escala + 32) >> 6); if (!cc) cc = 1; }
        c->vamp = cc;
    }
}

/* D99E: selecciona el instrumento n (0 a 31) */
static void instrumento(Chan *c, u8 n)
{
    u16 ip = WD(WD(0xC004) + 2U * n);
    u16 de;
    int ruido = 0;
    c->inst = n;
    if (WD(ip + 6) == 0 && WD(ip) != 0) {            /* rafaga de ruido: (duracion, indice del periodo) */
        c->f1a = 0xFF; c->nb_dur = c->nb_cnt = BY(ip);
        mix &= c->m_non;
        c->noiseper = c->nb_per = trui[BY(ip + 1)];
        ruido = 1;
    } else c->f1a = 0;
    if (WD(ip + 2) == 0) c->f16 &= (u8)~1;           /* vibrato */
    else {
        c->f16 |= 1; c->f17 |= 1;
        c->vper = c->vc = (u8)(BY(ip + 2) & 0x0F);
        c->vsrc = (u8)(BY(ip + 2) >> 4);
        c->vdelay = c->vcnt = BY(ip + 3);
        c->vval = 0;
    }
    de = WD(ip + 4);                                 /* secuencia de tono */
    if (de) { c->ts_cur = c->ts_ini = de; c->f18 = 0xFF; mix &= c->m_ton; }
    else { mix |= c->m_toff; c->f18 = 0; }
    de = WD(ip + 6);                                 /* secuencia de ruido */
    if (de) { c->ns_cur = c->ns_ini = de; c->f19 = 0xFF; mix &= c->m_non; }
    else { if (!ruido) mix |= c->m_noff; c->f19 = 0; }
    de = WD(ip + 8);                                 /* lista de volumen */
    if (de) { c->vl_cur = c->vl_ini = de; c->f14 = 0xFF; }
    else { c->outvol = 0; c->f14 = 0; }
}

/* D8E0: deslizamiento de tono hacia otra nota y la nota de partida (devuelve el indice de nota a tocar) */
static u8 deslizar(Chan *c, u16 *hl)
{
    u8 b = BY(*hl), p2 = BY(*hl + 1), p3 = BY(*hl + 2), e, a;
    u16 bc, frames, cnt, p3per, p2per, h, d;
    *hl = (u16)(*hl + 3);
    c->f15 |= 1;
    e = (u8)(b & 0x0F);
    bc = mul16(tempo, e, 8);
    c->gdelay = (u8)bc;
    cnt = (u16)((((b >> 4) & 0x0F) ^ 0x0F) + 1);
    a = (u8)(c->dur - e);
    frames = mul16(tempo, a, 8);
    p3per = PER((u8)(p3 + c->trans));
    p2per = PER((u8)(p2 + c->trans));
    c->gtarget = p2per;
    h = p3per; d = p2per;
    c->f15 &= (u8)~2;
    if (h < d) { u16 t = h; h = d; d = t; c->f15 |= 2; }
    bc = (u16)(h - d);
    if (!bc) bc = 1;
    bc = div16(bc, frames);
    if (!bc) bc = 1;
    bc = mul16(bc, cnt, 12);
    if (!(c->f15 & 2)) bc = (u16)(0 - bc);
    c->gstep = bc;
    return p3;
}

/* D7C5 y D827: nueva nota del canal. Devuelve 0 si la melodia ha terminado */
static void leer(Chan *c)
{
    u16 hl = c->pc, de;
    u8 a, b, n;
    c->vl_cur = c->vl_ini; c->ts_cur = c->ts_ini; c->ns_cur = c->ns_ini;
    c->vcnt = c->vdelay; c->f17 = c->f16; c->nb_cnt = c->nb_dur;
    if (c->f1a) { mix &= c->m_non; c->noiseper = c->nb_per; }
    c->f15 = 0; c->nidx = 0;
    for (;;) {
        a = BY(hl++);
        if (a < 0x60) { nota(c, a); break; }
        if (a == 0xFF) {
            if (c->sp < 32) { hl = (u16)(c->stk[c->sp] | (c->stk[c->sp + 1] << 8)); c->sp += 2; continue; }
            de = WD(c->pl_cur); c->pl_cur = (u16)(c->pl_cur + 2);
            if (de == 0) { c->pl_cur = (u16)(c->pl_ini + 2); de = WD(c->pl_ini); }
            else if (de == 0xFFFF) { stopped = 0xFF; return; }
            hl = de; continue;
        }
        if (a == 0x65) { mark = 0xFF; continue; }
        if (a > 0x65) {
            if (a < 0xC0) { c->dur = (u8)(a - 0x7F); continue; }
            if (a < 0xE0) { instrumento(c, (u8)(a - 0xC0)); continue; }
            if (a == 0xE0) { n = deslizar(c, &hl); nota(c, n); break; }
            if (a >= 0xF0) { tempo = (u8)(a - 0xEF); continue; }
            c->maxvol = (u8)(a - 0xE0);
            continue;
        }
        if (a == 0x64) { c->trans = BY(hl++); continue; }
        if (a == 0x61) { b = BY(hl++); c->sp -= 3; c->stk[c->sp] = b; c->stk[c->sp + 1] = (u8)hl; c->stk[c->sp + 2] = (u8)(hl >> 8); continue; }
        if (a == 0x60) {
            c->stk[c->sp]--;
            if (c->stk[c->sp] == 0 || c->stk[c->sp] >= 0x80) c->sp += 3;
            else hl = (u16)(c->stk[c->sp + 1] | (c->stk[c->sp + 2] << 8));
            continue;
        }
        if (a == 0x62) { de = WD(hl); hl = (u16)(hl + 2); c->sp -= 2; c->stk[c->sp] = (u8)hl; c->stk[c->sp + 1] = (u8)(hl >> 8); hl = de; continue; }
        hl = WD(hl);                                  /* 0x63: salto */
    }
    c->pc = hl;
}

static void cuenta(Chan *c)                       /* D7BA */
{
    if (c->cnt == 0) leer(c);
    c->cnt--;
}

/* D624: deslizamiento y retardo del vibrato; continua en envolvente() */
static void envolvente(Chan *c);

static void desplazar(Chan *c)
{
    u16 hl;
    if (c->f17 & 1) {
        if (c->vcnt == 0) { c->f17 &= (u8)~1; c->f17 |= 2; }
        else c->vcnt--;
    }
    if (c->f15 & 1) {
        if (c->gdelay) { c->gdelay--; envolvente(c); return; }
        hl = (u16)(c->per + c->gstep);
        if (c->f15 & 2) { if (hl >= c->gtarget) hl = c->gtarget; }
        else if (hl < c->gtarget) hl = c->gtarget;
        c->per = c->outper = hl;
    }
    envolvente(c);
}

/* D689: secuencias de tono y de ruido, vibrato y lista de volumen (dos veces por fotograma) */
static void envolvente(Chan *c)
{
    u16 hl;
    u8 a;
    if (c->f1a) {
        if (c->nb_cnt == 0) mix |= c->m_noff;
        c->nb_cnt--;
    }
    if (c->f18) {
        hl = c->ts_cur;
        for (;;) {
            a = BY(hl++);
            if (a == 0x80) break;
            if (a == 0x81) { hl = (u16)(c->ts_ini + BY(hl)); c->ts_cur = hl; continue; }
            c->ts_cur = hl;
            c->note = (u8)(c->note + a);
            c->per = c->outper = PER((u8)(c->note & 0x7F));
            break;
        }
    }
    if (c->f19) {
        hl = c->ns_cur;
        for (;;) {
            a = BY(hl++);
            if (a == 0x80) break;
            if (a == 0x81) { hl = (u16)(c->ns_ini + BY(hl)); c->ns_cur = hl; continue; }
            c->ns_cur = hl;
            c->nidx = (u8)(c->nidx + a);
            c->noiseper = trui[c->nidx];
            break;
        }
    }
    if (c->f17 & 2) {
        if (c->vc) c->vc--;
        else {
            c->vval ^= c->vamp;
            hl = c->per;
            if (c->f15 && (c->f15 & 4)) hl = (u16)(hl + c->vval);
            else hl = (u16)(hl - c->vval);
            c->outper = hl;
            c->vc = c->vper;
        }
    }
    if (!c->f14) return;
    if (c->lcnt) { c->lcnt--; return; }
    hl = c->vl_cur;
    a = BY(hl++);
    if (a == 0xFF) return;
    c->lcnt = a;
    a = BY(hl++);
    if (a >= c->maxvol) a = c->maxvol;
    c->outvol = a;
    c->vl_cur = hl;
}

static void salida(Chan *c, int vreg, int treg)    /* D5BF */
{
    if (c->busy) return;
    wr(vreg, muted ? 0 : c->outvol);
    wr(treg, c->outper & 0xFF);
    wr(treg + 1, c->outper >> 8);
    if (c->f19) wr(6, c->noiseper);
    else if (c->f1a && c->nb_cnt) wr(6, c->noiseper);
}

static void escribir(void)                         /* D52D */
{
    int i;
    if (fade_n) {
        for (i = 0; i < 3; i++) if (ch[i].outvol >= fade_v) ch[i].outvol = fade_v;
    }
    wr(7, mix);
    salida(&ch[0], 8, 0);
    salida(&ch[1], 9, 2);
    salida(&ch[2], 10, 4);
}

int mus_init(const u8 *data)
{
    static const u8 mt[3] = { 0xFE, 0xFD, 0xFB }, mo[3] = { 0x01, 0x02, 0x04 }, nn[3] = { 0xF7, 0xEF, 0xDF }, nf[3] = { 0x08, 0x10, 0x20 };
    int i;
    md = data;
    if (!md) return 0;
    vib_escala = md[DATA_END];
    tper = md + DATA_END + 2; trui = tper + 256; tvib = trui + 256;
    for (i = 0; i < 3; i++) {
        Chan *c = &ch[i];
        unsigned char *p = (unsigned char *)c;
        unsigned k;
        for (k = 0; k < sizeof(Chan); k++) p[k] = 0;
        c->sp = 32;
        c->maxvol = 0x0F; c->m_ton = mt[i]; c->m_toff = mo[i]; c->m_non = nn[i]; c->m_noff = nf[i];
    }
    mix = 0x3F; stopped = 0xFF; mark = 0; tempo = 6; tcount = 1; fade_n = 0; fade_v = 0; d506 = 0xFF;
    return 1;
}

static void start_(int song);
void mus_start(int song)                           /* D3AA */
{
    lock++; start_(song); lock--;
}

static void start_(int song)
{
    u16 mp = WD(WD(0xC000) + 2U * (unsigned)song);
    int i;
    for (i = 0; i < 3; i++) {
        Chan *c = &ch[i];
        u16 lp = WD(mp + 2U * i);
        c->pl_ini = lp;
        c->pc = WD(lp);
        c->pl_cur = (u16)(lp + 2);
        c->f14 = c->f15 = c->f16 = c->f17 = c->f18 = c->f19 = c->f1a = 0;
        c->cnt = 0; c->busy = 0; c->trans = 0;
        c->sp = 32;
    }
    d506 = 0; stopped = 0; fade_n = 0; fade_v = 0; mark = 0; tcount = 0;
    qs_write(7, 0x3F); qs_write(8, 0); qs_write(9, 0); qs_write(10, 0);
    mix = 0x3F;
}

void mus_fade(void) { fade_n = 2; fade_v = 0x0F; }   /* comando 3: DD8B = 2 (cuenta), DD8C = 0x0F (volumen maximo) */

static void stop_(void);
void mus_stop(void) { lock++; stop_(); lock--; }

static void stop_(void)                            /* D4ED */
{
    stopped = 0xFF;
    mix = 0x3F;
    qs_write(7, 0x3F); qs_write(8, 0); qs_write(9, 0); qs_write(10, 0);
}

static int habilitada;
static const u8 *fxd;                    /* efectos_bin */
static const u8 *fx_p;                   /* siguiente fotograma del efecto que suena (0 = ninguno) */
static int fx_n;                         /* fotogramas que quedan */
static int fx_on = 1;                    /* 0 = efectos silenciados */
void mus_enable(int on) { lock++; if (!on && habilitada && md) stop_(); habilitada = on && md; fx_p = 0; lock--; }
void mus_song(int song) { if (habilitada) mus_start(song); }

void mus_isr_modo(int on) { isr_modo = on; }
int mus_en_isr(void) { return isr_modo; }

/* Tarea de 50 Hz: ticks = cuadros desde el arranque. Si el programa principal esta tocando el estado se pierde ese cuadro; si se han perdido
   cuadros (la interrupcion se retraso) se recuperan, como mucho 3 seguidos. */
static void fx_frame(void);
void mus_isr(long ticks)
{
    static long hecho = -1;
    long n;
    if (hecho < 0) hecho = ticks - 1;
    n = ticks - hecho;
    hecho = ticks;
    if (lock) return;
    if (n > 3) n = 3;
    if (!habilitada) { while (n-- > 0) fx_frame(); return; }
    while (n-- > 0) mus_frame();
}
void mus_tick(void)
{
    if (isr_modo) return;
    if (habilitada) mus_frame(); else fx_frame();
}

/* n fotogramas seguidos (el bucle ha tardado mas de 20 ms): los n - 1 primeros solo avanzan el estado, sin tocar el chip, para
   que la melodia siga en su sitio sin oirse una rafaga */
void mus_tick_n(int n)
{
    if (isr_modo) return;
    if (!habilitada) { while (n-- > 0) fx_frame(); return; }
    quiet = 1;
    while (n-- > 1) mus_frame();
    quiet = 0;
    mus_frame();
}

/* Silencia (o devuelve) el volumen mientras se redibuja la pantalla: la musica sigue contando */
void mus_mute(int on)
{
    if (!habilitada || isr_modo || muted == on) return;     /* con la musica en la interrupcion no hace falta: suena mientras se dibuja */
    muted = on;
    if (on) { qs_write(8, 0); qs_write(9, 0); qs_write(10, 0); }
}

int mus_playing(void) { return md && !stopped; }
int mus_mark(void) { return mark; }

void mus_frame(void)
{
    int i;
    if (!md) return;
    /* comando 0: ACTUALIZAR_MUSICA */
    if (stopped & 0x80) {
        if (d506 < 0x80) qs_write(7, 0x3F);        /* D507: mezclador en silencio (sin efectos) */
        else { mix = 0x3F; qs_write(7, 0x3F); qs_write(8, 0); qs_write(9, 0); qs_write(10, 0); }
        return;                                    /* el comando 1 no hace nada con la musica parada */
    }
    if (fade_n | fade_v) {                         /* D486: cada 5 fotogramas baja el volumen maximo; a 0 para la musica */
        if (--fade_n == 0) {
            fade_n = 5;
            if (--fade_v == 0) stopped = 0xFF;
        }
    }
    if (tcount == 0) {
        for (i = 0; i < 3; i++) cuenta(&ch[i]);
        tcount = tempo;
    }
    for (i = 0; i < 3; i++) desplazar(&ch[i]);
    tcount--;
    escribir();
    /* comando 1: ACTUALIZAR_ENVOLVENTES */
    if (stopped & 0x80) return;
    for (i = 0; i < 3; i++) envolvente(&ch[i]);
    escribir();
}

/* ---------------------------------------------------------------- efectos de sonido (7BA4 y 0xDE9E del original) */
int mus_fx_init(const u8 *data)
{
    fxd = (data && data[0] == 'H' && data[1] == 'Q' && data[2] == 'F' && data[3] == 'X') ? data : 0;
    fx_p = 0; fx_n = 0;
    return fxd != 0;
}

void mus_efecto(int n)                   /* 7BA4: solo suenan con la musica desactivada (y con QSound) */
{
    const u8 *e;
    if (!fxd || habilitada || !fx_on || !qs_present() || n < 0 || n > 8) return;
    e = fxd + ((unsigned long)fxd[8 + 4 * n] << 24 | (unsigned long)fxd[9 + 4 * n] << 16 | (unsigned long)fxd[10 + 4 * n] << 8 | fxd[11 + 4 * n]);
    lock++;
    fx_n = (e[0] << 8) | e[1];
    fx_p = e + 2;
    lock--;
}

static void fx_silencio(void)
{
    qs_write(7, 0x3F); qs_write(8, 0); qs_write(9, 0); qs_write(10, 0);
}

static void fx_frame(void)
{
    int k, n;
    if (!fx_p) return;
    if (fx_n <= 0) { fx_p = 0; fx_silencio(); return; }       /* pasado el ultimo fotograma audible el original sigue en silencio */
    n = *fx_p++;
    for (k = 0; k < n; k++, fx_p += 2) qs_write(fx_p[0], fx_p[1]);
    fx_n--;
}

void mus_efectos_activar(int on) { lock++; fx_on = on; if (!on && fx_p) { fx_p = 0; fx_silencio(); } lock--; }
int mus_efectos_activos(void) { return fx_on; }
int mus_fx_suena(void) { return fx_p != 0; }
void mus_fx_fotograma(void) { fx_frame(); }       /* pruebas */
