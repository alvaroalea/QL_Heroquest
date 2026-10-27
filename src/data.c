#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u8 *pk_marco = 0, *fnt_normal = 0, *fnt_manuscrita = 0, *txt = 0, *mis_indice = 0, *mus_data = 0, *fx_data = 0;

u16 rd16(const u8 *p) { return (u16)((p[0] << 8) | p[1]); }
u32 rd32(const u8 *p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3]; }

u8 *data_load(const char *name, long *size)
{
    char path[96];
    FILE *f;
    long n, got = 0, r;
    u8 *b;
    strcpy(path, plat_data_dir());
    strcat(path, name);
    f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0L, 2); n = ftell(f); fseek(f, 0L, 0);
    b = (u8 *)malloc((size_t)n + 4);
    if (!b) { fclose(f); return 0; }
    while (got < n && (r = (long)fread(b + got, 1, (size_t)(n - got > 8192L ? 8192L : n - got), f)) > 0) got += r;
    fclose(f);
    if (got != n) { free(b); return 0; }
    if (size) *size = n;
    return b;
}

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

int data_init(void)
{
    pk_marco = need("marco_bin");
    fnt_normal = need("fuente_normal_bin");
    fnt_manuscrita = need("fuente_manuscrita_bin");
    txt = need("textos_bin");
    mis_indice = need("misiones_indice_bin");
    mus_data = data_load("musica_bin", 0);
    fx_data = data_load("efectos_bin", 0);
    return pk_marco && fnt_normal && fnt_manuscrita && txt && mis_indice;
}
