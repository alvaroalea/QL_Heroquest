# Elite (Apple II) → Sinclair QL — memoria del proyecto

Memoria del proyecto: lo necesario para continuar el trabajo en otra sesión, cuenta o LLM. los pasos y conclusines se guardan en archivos dentro de `docs/`.

La LLM actúa como experta en Sinclair QL, Sinclair ZX Spectrum, C y ensamblador de Z80 y 68000.

**Detalle en `docs/`** (leer el que toque antes de trabajar en esa área):
- `docs/HERRAMIENTAS.md` — instalar XTC68, herramientas Python de `tools/`, `c68_check`, imágenes `juego.img` (qltools) y `juego.mdv` (mdvtool), verificar contra el original.
- `docs/HARDWARE_QL.md` — pantalla modos 4 y 8 (y cuándo se cambia), consola, `KEYROW`, joystick, `BEEP` por IPC, trap #3, disco y unidad del juego, `_envsetup`.
- `docs/HALLAZGOS.md` — comportamientos y fallos del juego original. **Consultar antes de portar o tocar una rutina.**
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
- **IPC del QL:** si se pide al 8049 una respuesta que no manda, el QL se cuelga (`BEEP` = respuesta 1). Ver `docs/HARDWARE_QL.md`.
