# Elite (Apple II) → Sinclair QL — memoria del proyecto

Memoria del proyecto: lo necesario para continuar el trabajo en otra sesión, cuenta o LLM. los pasos y conclusines se guardan en archivos dentro de `docs/`.

La LLM actúa como experta en Sinclair QL, Sinclair ZX Spectrum, C y ensamblador de Z80 y 68000.

**Detalle en `docs/`** (leer el que toque antes de trabajar en esa área):
- `docs/HERRAMIENTAS.md` — XTC68, qltools, sQLux (compilado en `tmp/` con un parche local para capturas y teclas), herramientas Python de `tools/`.
- `docs/QL_INVENTARIO.md` — qué rutinas del original están portadas (directas / inspiradas / sustituidas / no portadas).
- `docs/PORT_QL.md` — **estado del port a QL**, estructura de `src/` y pruebas. Comandos: `make data` (datos), `make ql` (ejecutable), `make img` (`build/heroquest.img`, 720 KB), `make win` (`build/heroquest.win`, QXL.WIN de 4 MB con qxltool), `make run` (sQLux), `make host` / `make test` (versión de PC con capturas), `tools/ql_test.sh` (pruebas en sQLux), `make c68_check`.
- `docs/QSOUND.md` — detección de la tarjeta QSound, escritura de registros del AY y cómo probarla (sQLux sin audio: `SQLUX_QSLOG=1`).
- `docs/HARDWARE_QL.md` — pantalla modos 4 y 8 (y cuándo se cambia), consola, `KEYROW`, joystick, `BEEP` por IPC, trap #3, disco y unidad del juego, `_envsetup`.
- `docs/HALLAZGOS.md` — comportamientos y fallos del juego original. **Consultar antes de portar o tocar una rutina.**
- `docs/ZX_ANALISIS.md`, `docs/ZX_JUEGO.md`, `docs/ZX_INVENTARIO.md`, `docs/ZX_MUSICA.md` (música y efectos: datos originales, formato y notas para QSound), `docs/ZX_GRAFICOS_QL.md` y `docs/ZX_DATOS_QL.md` (extracción, retoque y empaquetado de gráficos, textos y misiones para el QL con `tools/hq_ql_datos.sh`; **`asset/graficos_ql/`, `asset/textos_ql/` y `asset/misiones_ql/` son las versiones retocadas/traducidas por el usuario: nunca sobrescribirlas**) — análisis del original de ZX Spectrum 128K (TZX de `ref/`): carga, memoria, estructuras, misiones, gráficos, sonido, herramientas (`tools/hq_*`) y desensamblado comentado en `ref/zx/` (`*.skool`, `*.asm`; se regenera con `tools/hq_build.sh`).
- `docs/ZX_PARTIDA.md` — estado de la partida en el original (carga de misión, registros, dados, mazo de tesoros) y su port a `src/partida.c`, verificado con un simulador Z80 (`make oraculo`, `make test`, `tests/run_ql_partida.sh`).
- `docs/ZX_VISTA.md` — la vista isométrica, las figuras y el plano en el original y su port a `src/vista.c` (geometría en `data/vista_bin`), verificado con el simulador Z80 (`tests/vista_test.c`).
- `docs/ZX_TURNO.md` — el turno de un personaje (dados, clic, movimiento, camino automático, animación, marcadores, plano y turno de Morcar) y su port a `src/turno.c` y `src/juego.c`, verificado con el simulador Z80 (`tests/turno_test.c`).
- `docs/ZX_ACCIONES.md` — los iconos de llave y ojo (puertas, búsqueda de trampas y tesoros, mazo, oro, eventos de trampas, muerte de héroes) y su port a `src/accion.c`, verificado con el simulador Z80 (`tests/accion_test.c`).
- `docs/ZX_COMBATE.md` — el icono de atacar, la pantalla de combate, la muerte de los monstruos y la bolsa (panel del personaje y hechizos) y su port a `src/combate.c` y `src/juego_combate.c`, verificado con el simulador Z80 (`tests/combate_test.c`; `tools/hq_zx_pantalla.py` pinta las pantallas del original).
- `docs/ZX_MONSTRUOS.md` — el turno de Morcar: camino de los monstruos hacia un héroe, movimiento y ataque, y su port a `src/monstruo.c` (`tests/monstruo_test.c`).
- `docs/ZX_MISIONES.md` — lo propio de cada misión: eventos, salas especiales del ojo, salida por la escalera y vuelta al menú, y su port a `src/mision.c` (`tests/mision_test.c`).
- `docs/ZX_SONIDO.md` — los efectos de sonido del juego (qué suena y cuándo, por qué solo con la música desactivada) y su port a `src/music.c` (`mus_efecto`, `data/efectos_bin`).
- `docs/ZX_AMPLIACION.md` — la ampliación Return of the Witch Lord (`ref/Return Of The Witch Lord*.tzx`): qué cambia, extracción de sus 10 misiones (`HQ_JUEGO=witchlord`), formato del índice `<aventura>_xx_idx` y la opción «Cargar otras aventuras».
- `docs/ZX_VERSION_EN.md` — la versión inglesa (`ref/Hero Quest - 128k.tzx`): cómo se extrae (`HQ_VERSION=en`, `asset/extraido_en/`), qué es igual y qué cambia respecto a la española; los textos y misiones salen como `*.es.txt` / `*.en.txt`.
- `docs/DECISIONES.md` — rutinas «no necesarias», decisiones de diseño y mejoras propias retiradas (y cómo recuperarlas).

## 1. Resumen

- **Objetivo:** port al **Sinclair QL** (68008 a 7,5 MHz, bus de 8 bits, sin FPU, ≤ 640 KB; **modo 8: 256×256, 8 colores, en **C89** con **qcc (XTC68/c68)** y ensamblador `as68`. Versión de disco.
- **Referencias** (solo para verificar, no son parte del proyecto): en `ref`.
- **Estrategia:** HAL para lo que depende del hardware (no se compara con el original); el resto se porta a C sin verificar cada rutina, anotándola en `tools/ported.txt` (de ahí sale `docs/INVENTARIO.md`); una vez que funcione, se comprueban que sean ideanticas a las del original, finalmente profiling, paso a ensamblador y mejoras.


## 2. Estructura del repositorio

```
Makefile        compilación y pruebas
docs/           archivos .md de informacion
asset/          ficheros externos para generar datos
data/           boot y otros binarios que forman parte de la imagen final (README.txt no se distribuye)
build/          ejecutables e intermedios
src/            código del juego
tests/          archivos internos para verificar el port
tools/          herramientas Python para reconstruir la imagen
ref/            fuentes de referencia
tmp/            utilidades de terceros compiladas aquí si no están en local
```

## 4. Convenciones y trampas que afectan a casi todo

- **C89 puro:** sin `stdint.h`, sin declaraciones mezcladas ni en `for`, sin `float`; `u8 = unsigned char`; ensamblador en ficheros `.asm` (sintaxis Sozobon: `.globl`, `.text`, comentarios `;`).
- **`int`:** el `qcc` del `Makefile` lo compila de 32 bits, `make c68_check` de 16 (`-int=16`). No depender del tamaño: `long` para valores > 32767 (`SCR_BYTES = 32768L`) y no confiar en promociones. **El ensamblador no lee por símbolo variables `int`** (solo `u8` y punteros): los `int` se pasan como argumentos `long`.
- `qcc` busca los `#include "..."` en el directorio actual: siempre `-Isrc`.
- **Fallo del optimizador `-O` de c68:** en `x = (u8)d; cc = (d >= 0);` usa los flags del `move.b` y no los de `d` (naves deformadas, números mal en `dt_bprnt`). **Regla: calcular siempre el signo/acarreo ANTES de guardar el byte** (`cc = (d >= 0); x = (u8)d;`). Una sonda de c68 por cada módulo nuevo.
- **Sondas c68:** en el PC `unsigned long` es de 64 bits; enmascarar los generadores congruenciales (`& 0xFFFFFFFFUL`) o la sonda sale distinta sin que c68 falle.
- **Nombres de fichero del QL: sin puntos** (`sprites_bin`, `heroquest_03`...); lo que es solo del PC va a `build/`.
- **Trampas de QDOS/as68 ya vistas:** la rutina de la lista de interrupciones sincronizadas NO recibe el enlace en A3 (corrompe la pantalla si se usa `8(a3)`); as68 usa la sintaxis de `c68 -S` (`.sect .text`, `.extern`, `.data4`, `!` para comentarios, `bra`/`bmi` sin `.s`); `qltools -x` solo guarda 16 bits de espacio de datos.
- **IPC del QL:** si se pide al 8049 una respuesta que no manda, el QL se cuelga (`BEEP` = respuesta 1). Ver `docs/HARDWARE_QL.md`.
