#!/bin/bash
# Comprueba que el .skool reensamblado es identico al original (memoria fija 0x6400-0xFFFD).
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
T=$(mktemp)
tmp/venv/bin/skool2bin.py ref/zx/heroquest_main.skool "$T" >/dev/null 2>&1
if cmp <(tail -c +$((0x6400-0x4000+1)) asset/extraido/main48.bin | head -c $((0xFFFE-0x6400))) "$T"; then echo "heroquest_main.skool: IDENTICO al original"; else echo "DIFERENCIAS"; exit 1; fi
rm -f "$T"
T=$(mktemp)
tmp/venv/bin/skool2bin.py ref/zx/heroquest_overlay_menu.skool "$T" >/dev/null 2>&1
if cmp <(head -c $((0x6CEE-0x5B00)) asset/extraido/overlay_menu_5B00.bin) "$T"; then echo "heroquest_overlay_menu.skool: IDENTICO al original"; else echo "DIFERENCIAS en el overlay"; exit 1; fi
rm -f "$T"
T=$(mktemp)
tmp/venv/bin/skool2bin.py ref/zx/heroquest_banco4_sonido.skool "$T" >/dev/null 2>&1
if cmp <(head -c 16384 asset/extraido/bank4.bin | head -c $((0x10000-0xC000))) "$T"; then echo "heroquest_banco4_sonido.skool: IDENTICO al original"; else echo "DIFERENCIAS en el banco 4"; exit 1; fi
rm -f "$T"
