#include "keys.h"
static u16 prev = 0;
u16 key_event(void)
{
    u16 now, ev;
    plat_present();
    now = plat_keys();
    ev = (u16)(now & ~prev);
    prev = now;
    return ev;
}
u16 key_wait(void)
{
    u16 k;
    do { k = key_event(); } while (!k);
    return k;
}
void key_reset(void) { prev = 0xFFFF; }
