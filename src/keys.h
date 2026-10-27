/* HeroQuest QL - eventos de teclado para los menus */
#ifndef KEYS_H
#define KEYS_H
#include "platform.h"
u16 key_event(void);              /* espera un fotograma y devuelve las teclas recien pulsadas (bits K_*) */
u16 key_wait(void);               /* espera hasta que se pulse algo */
void key_reset(void);             /* olvida el estado anterior (para que lo que sigue pulsado cuente como nuevo) */
#endif
