#!/bin/bash
# Reconstruye el desensamblado de Hero Quest (ZX 128K) en ref/zx/ a partir del TZX.
# Uso: tools/hq_build.sh
set -e
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
PY=tmp/venv/bin/python; SK=tmp/venv/bin
$PY -I tools/tzx_to_z80.py "ref/Hero Quest (Dro Soft).tzx" asset/extraido
$PY -I - <<'P'
import sys; sys.path.insert(0, 'tools')
import hqmem as m
open('asset/extraido/main48.bin', 'wb').write(bytes(m.byte(a) for a in range(0x4000, 0x10000)))
open('asset/extraido/overlay_menu_5B00.bin', 'wb').write(m.BANKS[6][0xDB58-0xC000:0xDB58-0xC000+0x11EE])
P
$PY tools/hq_subs.py
python3 tools/hq_notes2ctl.py main ref/zx/ctl/auto_main.ctl > ref/zx/ctl/main.ctl
$SK/sna2skool.py -H -c ref/zx/ctl/main.ctl -o 0x4000 -s 0x6400 asset/extraido/main48.bin > ref/zx/heroquest_main.skool 2> ref/zx/build.log
$PY tools/hq_ctl.py asset/extraido/overlay_menu_5B00.bin 0x5B00 0x5B00 0x6CEE -1 --fill 0x5B00 0x6CEE > ref/zx/ctl/auto_overlay.ctl; echo "i \$6CEE" >> ref/zx/ctl/auto_overlay.ctl
HQ_OVERLAY=1 $PY tools/hq_subs.py overlay
python3 tools/hq_notes2ctl.py overlay ref/zx/ctl/auto_overlay.ctl > ref/zx/ctl/overlay.ctl
$SK/sna2skool.py -H -c ref/zx/ctl/overlay.ctl -o 0x5B00 asset/extraido/overlay_menu_5B00.bin > ref/zx/heroquest_overlay_menu.skool 2> ref/zx/build_overlay.log
$PY tools/hq_ctl.py asset/extraido/bank4.bin 0xC000 0xC000 0x10000 4 --fill 0xD388 0xDEA0 > ref/zx/ctl/auto_sonido.ctl
python3 tools/hq_notes2ctl.py sonido ref/zx/ctl/auto_sonido.ctl > ref/zx/ctl/sonido.ctl
$SK/sna2skool.py -H -c ref/zx/ctl/sonido.ctl -o 0xC000 asset/extraido/bank4.bin > ref/zx/heroquest_banco4_sonido.skool 2> ref/zx/build_sonido.log
for n in heroquest_main heroquest_overlay_menu heroquest_banco4_sonido; do $SK/skool2asm.py -q -H ref/zx/$n.skool > ref/zx/$n.asm 2>/dev/null; done
echo "ok: ref/zx/heroquest_main.skool"
tools/hq_verify.sh
