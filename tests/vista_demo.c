/* Dibuja la vista de una sala de una mision en el PC y guarda la pantalla (para verla: tools/ql2png.py).
   Uso: build/host/vista_demo mision columna fila [salida.bin]   (el heroe 0 se pone en esa casilla; desde la raiz del proyecto) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vista.h"
#include "gfx.h"

static u8 *cargar(const char *nombre)
{
    u8 *d = data_load(nombre, 0);
    if (!d) { printf("no se puede leer %s\n", nombre); exit(2); }
    return d;
}

int main(int argc, char **argv)
{
    u8 pr[4] = { 1, 1, 1, 1 }, *dm;
    char nm[40];
    long n;
    gimg g;
    u8 *spr, *esc;
    static Partida *p = &g_part;
    if (argc < 4) { printf("uso: vista_demo mision columna fila [salida.bin]\n"); return 1; }
    plat_init();
    gfx_init();
    pk_marco = cargar("marco_bin");
    spr = cargar("sprites_bin"); esc = cargar("escenario_bin");
    if (!vista_datos(spr, esc, cargar("vista_bin"), cargar("baldosas_bin"), cargar("marca_activo_bin"))) { printf("vista_datos falla\n"); return 2; }
    sprintf(nm, "heroquest_%02d_es", atoi(argv[1]));
    dm = cargar(nm); n = 0;
    { FILE *f; char path[80]; sprintf(path, "%s%s", plat_data_dir(), nm); f = fopen(path, "rb"); fseek(f, 0, 2); n = ftell(f); fclose(f); }
    pers_boot(p);
    if (!partida_cargar(p, atoi(argv[1]), dm, n, pr)) { printf("mision rechazada\n"); return 2; }
    { u8 *ix = cargar("heroquest_es_idx"); partida_tipos(p, data_tipos(ix)); partida_estadisticas(p, data_estadisticas(ix)); }
    p->pers[0].col = (u8)atoi(argv[2]); p->pers[0].fila = (u8)atoi(argv[3]);
    gfx_clear();
    if (pack_get(pk_marco, 0, &g)) gfx_blit(&g, 0, (256 - g.h) / 2);
    vista_construir(p, 0);
    vista_figuras_dibujar();
    plat_snapshot(argc > 4 ? argv[4] : "demo");
    return 0;
}
