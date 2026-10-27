# Herramientas

## Port a QL

| Herramienta | Uso |
|---|---|
| **XTC68** (`qcc`, `qcpp`, `c68`, `as68`, `qld`) | compilador C89 y ensamblador (sintaxis de `c68 -S`: `.sect .text`, `.extern`, `.data4`, comentarios con `!`, saltos sin sufijo `.s`). Instalado en `~/.local/bin`. `qcc ... -o build/heroquest` produce el ejecutable de QDOS |
| **qltools** | `qltools img -fdd ETIQUETA` formatea un disco de 720 KB; `-W ficheros` escribe; `-x nombre tamaño` marca un fichero como ejecutable con ese espacio de datos (solo 16 bits: `-x f 200000` guarda 3392); `-d` lista. Los nombres del QL **no llevan punto** (qltools convierte `.` en `_`) |
| **sQLux** (emulador) | no está en el sistema: se clona y compila en `tmp/sQLux` (`git clone https://github.com/SinclairQL/sQLux`, `git submodule update --init args inih sds`, `cmake -DSUPPORT_SHADERS=OFF`, `make`). Trae Minerva en `tmp/sQLux/roms` |
| `tools/ql_run.sh` | arranca `build/heroquest.img` en sQLux (`tmp/sqlux_hq.ini`: Minerva, 640 KB, FLP1 = la imagen, arranque desde FLP1) |
| `tools/ql_test.sh` | prueba automática: lanza sQLux, pulsa teclas (`U D L R F E 0 1 2 3`; `Sxx` = captura; `Rh2` = tecla mantenida 2 s) y guarda `build/ql_test/xx.png`. **Requiere un parche local de sQLux** (ya aplicado en `tmp/sQLux/src/SDL2main.c` y `SDL2screen.c`): `SIGUSR1` vuelca la memoria de pantalla al fichero de `SQLUX_DUMP` y `SIGUSR2` pulsa o suelta las teclas del QL que lee de `SQLUX_KEYS` (líneas `d 0xNN` / `u 0xNN`, código de tecla de `include/qlkeys.h`). Las capturas de ventana (`import`, `xwd`) no funcionan en este escritorio |
| `tools/ql2png.py` | volcado de pantalla del QL (32768 bytes, modo 8) a PNG |

## Análisis del Spectrum (`docs/ZX_ANALISIS.md`)

SkoolKit 10.1 en un entorno virtual: `python3 -m venv tmp/venv && tmp/venv/bin/pip install skoolkit`. Las herramientas `tools/hq_*.py` se ejecutan con `tmp/venv/bin/python -I`; `tools/hq_build.sh` regenera los desensamblados y `tools/hq_ql_datos.sh` los datos del QL.

## QSound en sQLux

`tmp/sqlux_hq.ini` carga `QSROM = qsound_real.rom`. Con `SQLUX_QSLOG=1` el sQLux parcheado escribe en `sqlux.log` las líneas `QS <registro> <valor>` (ver `docs/QSOUND.md`).

## Tiempos reales en sQLux

`tmp/sqlux_hq.ini` lleva `SPEED = 1` (68008 a 7,5 MHz con el coste real de cada instrucción, incluida la contención de la memoria de vídeo): con él se miden tiempos (`make bench` y `SQLUX_QSLOG=1`, ver `src/bench.c`) pero el arranque tarda ~20 s (`BOOT_WAIT=25` en `tools/ql_test.sh`). `tmp/sqlux_hq_rapido.ini` es el mismo sin `SPEED` (máxima velocidad) y se usa con `INI=tmp/sqlux_hq_rapido.ini` para comparar dibujos (`tests/ql_asm_vs_c.sh` lo hace). `SNAP_WAIT` es la espera antes de cada captura (`Sxx`).

## Oráculos del Spectrum (`make oraculo`, `make test`)

`tools/hq_oraculo_partida.py`, `tools/hq_oraculo_vista.py`, `tools/hq_oraculo_turno.py`, `tools/hq_oraculo_accion.py`, `tools/hq_oraculo_combate.py`, `tools/hq_oraculo_monstruo.py` y `tools/hq_oraculo_mision.py` ejecutan las rutinas **originales** del Spectrum en el simulador Z80 de SkoolKit (con el snapshot `asset/extraido/hq128.z80` y el overlay del menú) y vuelcan la memoria resultante (`tests/oraculo_partida.txt`, `build/oraculo_vista.txt`, `build/oraculo_turno.txt`, `build/oraculo_accion.txt`, `build/oraculo_combate.txt`, `build/oraculo_monstruo.txt`, `build/oraculo_mision.txt`); `tests/mision_test.c`, `tests/monstruo_test.c`, `tests/partida_test.c`, `tests/vista_test.c`, `tests/turno_test.c`, `tests/accion_test.c` y `tests/combate_test.c` comprueban que la versión en C da lo mismo. `tests/run_ql_partida.sh`, `tests/run_ql_vista.sh` y `tests/run_ql_turno.sh`, `tests/run_ql_accion.sh` y `tests/run_ql_combate.sh` y `tests/run_ql_monstruo.sh` y `tests/run_ql_mision.sh` repiten las pruebas compiladas con c68 y ejecutadas en sQLux (el resultado se lee del disco del emulador con `qltools -n`). La RAM libre del QL es de unos 350 KB: por eso las sondas usan parte del oráculo.

`tools/host_jugar.sh "guion de teclas"` ejecuta la versión de PC con un guion (`U D L R F E 0-3 . Sxx`, `Rx12` mantiene 12 cuadros) y deja las capturas como PNG en `build/host/jugar/`.
- `tools/hq_efectos_empaquetar.py [--zx] [-o salida]`: convierte los volcados `asset/musica/psg/efecto_N.psg` en `data/efectos_bin` (periodos de 750 kHz, diferencias por fotograma); con `--zx` deja los periodos del Spectrum para `tests/run_mus.sh`, que compara `tests/fx_test.c` con los nueve volcados.
