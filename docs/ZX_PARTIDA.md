# Estado de la partida en el original (ZX Spectrum) y su port a C

Análisis de lo que hace el original **antes de que empiece el bucle de turnos**: carga de la misión, registros de personajes y monstruos, capas del mapa, mazo de tesoros y dados. Código en `src/partida.c` y `src/partida.h`. Verificación: `tools/hq_oraculo_partida.py` ejecuta las rutinas **originales del Spectrum** en el simulador Z80 de SkoolKit y `tests/partida_test.c` comprueba que la versión en C da los mismos bytes (`make test`, 1584 comprobaciones; la misma prueba compilada con c68 se ejecuta en sQLux con `tests/run_ql_partida.sh`).

## INICIAR_MISION (`0x5B3F`, overlay del menú)

Orden exacto de lo que hace (variables en `docs/ZX_ANALISIS.md`):

1. Pone a cero `0xA983-0xA9D6` (84 bytes de variables del juego: turno, movimiento, dados, mazo de héroes ausentes/muertos...).
2. Copia la misión (`0x400` bytes a `0x6200`: capa A en `0x6200` y capa B en `0x6400`; en el QL son dos capas de 494 bytes) y pone a cero la capa C (`0x6600`).
3. Héroes (los 4 registros de `0xA9E5`): el byte de estado se toma de la tabla `0x8FE4-0x8FE7` (`0xFF` = el héroe existe; son las marcas del menú «Crear personaje»; después **se ponen a cero**: sólo vuelven a `0xFF` cuando un héroe sale vivo por la escalera, `0x8E7B`). Si no existe se pone su bit en `0xA9B6` (ausentes) y `0xA9B7` (muertos). Después: columna, fila y fotograma de la misión, `+0x10 = +0x16 = +0x17 = +0x18 = 0` y `+0x0F &= 0x8F` (se borran los estados 4-6).
4. Indicadores (86 bytes a `0x6BB6`), texto de presentación (a `0xFA00`, sólo para dibujarlo), lista de monstruos (250 bytes a cero y la copia) y objetos: cada objeto escribe su código en la capa C.
5. Se dibuja el objetivo y se espera el botón (lo hace `mostrar_objetivo()` del menú en el QL).
6. Quinto personaje (`0xAA4D`): estado 0; sólo en la misión 1 (rescate de Sir Ragnar) se le pone col 6, fila 1, cuerpo 2, fotograma `0x2C`.
7. Monstruos: para cada uno con estado ≠ 0: habitación = capa A de su casilla `& 0x3F`, dirección de la casilla en `+4/+5` y su número (1 a 25) **en la capa C** (sobrescribe un objeto si coinciden).
8. Se ponen a cero 43 entradas de 4 bytes en `0xC47F` (tabla de la vista: no se porta hasta que se haga la vista).
9. **Misión 5** (laberinto de Melar): los héroes pierden el equipo: se guardan en `0xA2B2` (y se ponen a cero) los bytes `+0x0D..+0x0F` del bárbaro y el enano, `+0x0B..+0x0F` del elfo y `+0x0B..+0x0E` del hechicero. `DESPACHAR_HEROE` (`0xA271`) los devuelve.
10. Se construye la tabla de máscaras de sprites `0x6800` (`~(b | b<<1 | b<<2 | b>>1 | b>>2)`); en el QL las máscaras ya están guardadas en los gráficos y no hace falta.
11. **Misión 7** (mago del fuego): al elfo y al hechicero se les quitan los hechizos de fuego (`+0x0C &= 0x38`).
12. `JP 0x9542` (barajar el mazo de tesoros).

Detalle importante: la rutina de intercambio `0x5CE7` **no vuelve** (no tiene `RET`): cae en el código del paso 10 y termina en `JP 0x9542`, cuyo `RET` vuelve al `CALL` del siguiente héroe. Funciona de casualidad: la tabla de máscaras se reconstruye cuatro veces y el mazo se baraja cuatro veces en la misión 5. En el port sólo se hace una vez.

## Registro de personaje (26 bytes)

Los desplazamientos están en `Pers` (`src/partida.h`). Valores de los registros tal como están en la memoria al cargar el juego (`INIT` del oráculo): cuerpo/mente 8/2 el bárbaro (fotograma `0x11`, ataque 3, defensa 2), 7/3 el enano (`0x26`, 2/2 y bit 7 de `+0x0F`), 6/4 el elfo (`0x29`, 2/2), 4/6 el hechicero (`0x3E`, 1/2) y el quinto 0/6 (1/2). La tabla `0x7E4E` da el cuerpo inicial/máximo: `08 07 06 04 02` (el quinto vale 2: lo usa la curación de hechizos y de pociones). `0x60E6` (héroe nuevo) pone el cuerpo de la tabla y borra oro, equipo y `+0x11..+0x13`; el enano recibe el bit 7 de `+0x0F`.

Hechizos: `+0x0B` y `+0x0C` son dos bytes de máscaras de 3 bits por grupo: aire = `0x38` en `+0x0B`, fuego = `0x07` en `+0x0C`, tierra = `0x38` en `+0x0C`, agua = `0x07` en `+0x0B` (tabla `0x65F7`).

## Dados

- `DADO` (`0x9BD3`): `C = 3; B = R; RR B; repetir B veces (256 si es 0): DEC C, si es 0 poner 6; DEC C`. Devuelve 0 a 5. Sólo depende de `R` (7 bits del contador de refresco y el bit 7) y del acarreo de entrada; con cada valor de `R` y acarreo el resultado coincide con el original (512 casos del oráculo). En el QL `R` no existe: se usa un generador congruencial (`dado_semilla()`; en el QL se siembra con el reloj de 50 Hz).
- `DADO_COMBATE` (`0x9BC8`): cara = `{0, 1, 2, 0, 1, 0}[dado]`.
- `CALCULAR_DADOS` (`0x6ECD`): 1 dado; 2 para los héroes (no para Morcar), salvo bit 3 de `+0x13` o bit 6 de `+0x0F`; con el bit 5 de `+0x17` (y el 5 de `+0x10`, ambos se borran) se duplica, y con el bit 3 de `+0x18` (se borra) se duplica otra vez (si sale 8 se deja en 6). Pone a cero `0xA98A` y `0xA98B`.
- `TIRAR_DADOS` (`0x6F1C`): puntos de movimiento = suma de (dado + 1) de cada dado. La animación del dado y la espera del botón se harán con el bucle del turno.

## Mazo de tesoros

`MAZO_CARTAS` (`0x9593`): 25 cartas (0 a 24) y `0xFF` al final.

- `BARAJAR_MAZO` (`0x9542`): pone 0..24 y hace `C` pasadas, `C = 0x80 | ((R >> 3) & 0x3F)` (128 a 159: R sólo tiene 5 bits útiles tras el desplazamiento). Cada pasada recorre los 24 pares de vecinos y los intercambia si el **bit 3 del siguiente byte de la ROM del Spectrum 48K** (desde la dirección `(0x06FC + C) * 2 + 1`) vale 1. Es decir: **sólo hay 32 barajados distintos** y son mediocres (128 pasadas de intercambios de vecinos no mezclan por completo: las cartas se quedan cerca de su posición). El port conserva el algoritmo (`mazo_barajar(c, bit)`) pero saca los bits de un generador (`mazo_barajar_azar`) en lugar de la ROM, que no se distribuye; la prueba le pasa los bits reales de la ROM y obtiene los 32 barajados del original.
- `ROBAR_CARTA` (`0x9523`): devuelve la primera y desplaza el resto una posición (el `0xFF` también); `0xA9E1` apunta al final.
- `DEVOLVER_CARTA` (`0x9588`): la deja al final.

## MISION_CONTINUA (`0x6EB5`)

La misión sigue mientras `0xA9B7 != 0x0F` (hay algún héroe vivo). Con los cuatro muertos acaba, salvo en la misión 1, donde sigue si el quinto personaje (Sir Ragnar) tiene estado ≠ 0.

## Lo que queda fuera de este paso

Todo lo que dibuja o espera teclas: la animación del dado, la vista, las zonas de clic... Los campos `Vars` de `partida.h` se irán ampliando con cada rutina que se porte. La tabla `0xC47F` (paso 8) se hará con la vista.
