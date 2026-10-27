/* HeroQuest QL - tarjeta QSound (AY-3-8912 a 750 kHz). Se busca en los 32 bloques de 8 KB de 0xC0000 a 0xFFFFF: el bloque de la tarjeta
   tiene la cadena "QSound" entre los bytes 8 y 50 (la ROM V1.94 la trae en el byte 10) y su PIA esta en la segunda parte, bloque + 0x2000. */
#ifndef QSOUND_H
#define QSOUND_H
#include "platform.h"

int  qs_detect(void);                    /* 1 si hay QSound (y la deja lista y en silencio); 0 si no */
int  qs_present(void);                   /* resultado de la ultima deteccion */
void qs_write(int reg, int val);         /* escribe un registro del AY (0 a 13); no hace nada si no hay tarjeta */
void qs_silence(void);                   /* volumenes a 0 y tonos y ruido desactivados */
#endif
