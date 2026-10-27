/* HeroQuest QL - motor de musica (reproductor del banco 4 del original, 0xD388-0xDC07) sobre la QSound.
   Los datos son los del original (data/musica_bin, tools/hq_musica_empaquetar.py). mus_frame() debe llamarse 50 veces por segundo
   (el original lo hacia dos veces por interrupcion con los comandos 0 y 1). Los registros salen por qs_write(). */
#ifndef MUSIC_H
#define MUSIC_H
#include "platform.h"

int  mus_init(const u8 *data);   /* data = musica_bin (se queda con el puntero); 0 si no sirve */
void mus_start(int song);        /* comando 2: empieza la melodia (1 = partida, 2 = menu; 0 = silencio) */
void mus_fade(void);             /* comando 3: bajada de volumen y parada */
void mus_stop(void);             /* otro comando: silencia */
void mus_frame(void);            /* un fotograma de 50 Hz */
void mus_enable(int on);         /* activa o desactiva la musica (al desactivar silencia el chip) */
void mus_song(int song);         /* empieza la melodia solo si la musica esta activada */
void mus_isr(long ticks);        /* desde la tarea de 50 Hz (ql_ipc.asm): avanza la musica hasta el cuadro ticks */
void mus_isr_modo(int on);       /* 1 = la musica la mueve la interrupcion (plat_present ya no la toca) */
int  mus_en_isr(void);
void mus_tick_n(int n);          /* n fotogramas: los n - 1 primeros sin sonar (para recuperar el tiempo perdido) */
void mus_mute(int on);           /* 1 = volumenes a 0 mientras se redibuja; 0 = vuelven con el siguiente fotograma */
void mus_tick(void);             /* un fotograma si la musica esta activada (lo llama plat_present) */
int  mus_playing(void);          /* 1 si suena una melodia */
int  mus_mark(void);             /* marca de los patrones (0xDD8D del original) */

/* Efectos de sonido (solo con la musica desactivada: como el original, 0x7BA4) */
int  mus_fx_init(const u8 *data);   /* data = efectos_bin; 0 si no sirve */
void mus_efecto(int n);             /* lanza el efecto n (0 paso, 1 hechizo, 2 ataque, 3 herida, 4 sin uso, 5 dados, 6 clic, 7 escalera, 8 puerta) */
void mus_efectos_activar(int on);   /* 0 = los efectos no suenan aunque la musica este desactivada (tecla M) */
int  mus_efectos_activos(void);
int  mus_fx_suena(void);
void mus_fx_fotograma(void);        /* un fotograma del efecto (pruebas) */
#endif
