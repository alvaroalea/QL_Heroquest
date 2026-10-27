/* HeroQuest QL - tarjeta QSound */
#include "qsound.h"

#ifdef HOST      /* PC: no hay bus del QL; HQ_QSOUND=1 simula la tarjeta y las escrituras van al fichero qs_log.txt */
#include <stdio.h>
#include <stdlib.h>

static int host_qs = 0;

int qs_present(void) { return host_qs; }

int qs_detect(void)
{
    host_qs = getenv("HQ_QSOUND") != 0;
    return host_qs;
}

void qs_write(int reg, int val)
{
    FILE *f;
    if (host_qs && (f = fopen("qs_log.txt", "a")) != NULL) { fprintf(f, "%d %d\n", reg, val); fclose(f); }
}

#else

#define QS_INI  0xC0000L
#define QS_FIN  0x100000L
#define QS_BLQ  0x2000L                  /* tamano de bloque y desplazamiento de los registros */

static volatile u8 *qs_base = 0;         /* A5 del codigo de ejemplo: puerto A en +0 y control en +2 */

int qs_present(void) { return qs_base != 0; }

static int es_qsound(const volatile u8 *b)
{
    static const u8 cad[6] = { 'Q', 's', 'o', 'u', 'n', 'd' };
    int i, j;
    for (i = 8; i <= 44; i++) {          /* la ROM V1.94 lo trae en el byte 10, tras la palabra de longitud del nombre */
        for (j = 0; j < 6; j++)
            if ((b[i + j] | 0x20) != (cad[j] | 0x20)) break;     /* sin distinguir mayusculas */
        if (j == 6) return 1;
    }
    return 0;
}

int qs_detect(void)
{
    long a;
    qs_base = 0;
    for (a = QS_INI; a < QS_FIN; a += QS_BLQ)
        if (es_qsound((const volatile u8 *)a)) { qs_base = (volatile u8 *)(a + QS_BLQ); break; }
    if (qs_base) qs_silence();
    return qs_base != 0;
}

void qs_write(int reg, int val)
{
    volatile u8 *p = qs_base;
    if (!p) return;
    p[0] = (u8)reg;                      /* numero de registro al puerto A */
    p[2] = 0x0F;                         /* BC1 = 1, BDIR = 1: fija la direccion */
    p[2] = 0x0A;                         /* inactivo */
    p[0] = (u8)val;                      /* dato al puerto A */
    p[2] = 0x0E;                         /* BC1 = 0, BDIR = 1: escribe el dato */
    p[2] = 0x0A;                         /* inactivo */
}
#endif

void qs_silence(void)
{
    int r;
    for (r = 0; r < 14; r++) qs_write(r, 0);
    qs_write(7, 0x3F);                   /* mezclador: tonos y ruido apagados */
}
