# El turno de los monstruos en el original (ZX Spectrum) y su port a C

Análisis de `TURNO_MONSTRUO` (`0x9D6B`), `CALCULAR_CAMINO` (`0x9DC0`, `0x9E6A`-`0x9F7C`), `CASILLA_LIBRE` (`0x9F94`, `0x9FAF`) y `EJECUTAR_CAMINO` (`0xA075`), y su port a `src/monstruo.c` (lógica) y `ui_turno_monstruos()` (`src/juego_combate.c`). Verificación: `tools/hq_oraculo_monstruo.py` ejecuta las rutinas originales en el simulador Z80 (un monstruo o una ronda de los 25, con el ataque `0x990D`/`0x9921` incluido) y `tests/monstruo_test.c` compara héroes, monstruos, capas, variables y ataques (`make test`: 2520 comprobaciones en 280 casos; `tests/run_ql_monstruo.sh` repite 6 casos con c68 en sQLux).

## La ronda de Morcar (`0x9CE0`)

Plano de la mazmorra y «ES EL TURNO DE MORCAR» (150 fotogramas); `TURNO_MONSTRUO` para cada uno de los 25 registros de monstruos por orden; «EL TURNO DE MORCAR SE TERMINO»; si la misión sigue, vuelve la vista y se quita el bit 0 de `+0x18` (velo de niebla) de los héroes.

## Un monstruo (`0x9D6B`)

1. No hace nada si el registro está vacío, oculto (`+0` con el bit 7, `0x81`) o no está a la vista (bit 0 de `+6`).
2. **Dormido** (bit 2 de `+6`): despierta si el registro R del Z80 vale 5 (ver `HALLAZGOS.md`: el original compara `A`, que es el R que leyó `DADO`, no la cara).
3. **Tempestad** (bit 1 de `+6`): se quita y pierde el turno.
4. Recorre los cinco registros de personajes (`0xA9AD` = el que prueba) y calcula el camino hasta cada uno. Con el primero que da **0** (llega junto a él) o **3** (ya está junto a él) actúa: anda por el camino y le ataca. Si ninguno, no hace nada (no se acerca a un héroe al que no llega).

## El camino (`0x9DC0`)

No hay camino (2) si el héroe no existe, no está en la sala del monstruo (`+3`; ni la casilla del héroe en la capa A) o tiene el velo de niebla (bit 0 de `+0x18`). El camino es un recorrido voraz casilla a casilla: si la diferencia en X es mayor o igual que la de Y avanza primero en X y, si la casilla no está libre, en Y (y al revés), repitiendo hasta que la casilla siguiente sea la del héroe (llega) o no se pueda avanzar (2). Cada paso se anota en `0x6B8A` como (dx, dy). `CASILLA_LIBRE` (`0x9F94`) da `0xFF` si el héroe objetivo está en la casilla, 1 si hay otro héroe, 2 si hay un objeto o un monstruo, 3 si es de otra sala y 0 si está libre. Si llega sin dar ningún paso, el código es 3.

## Moverse y atacar (`0xA075`, `0x990D`)

Cada paso borra el código del monstruo de su casilla (capa C), mueve el registro (`+1`, `+2`, `+4`, `+5`) y pone el código en la nueva (después se pinta el plano); al final espera 50 fotogramas y llama a `PANEL_COMBATE` (`docs/ZX_COMBATE.md`): el héroe se defiende, el plano vuelve y se repite «ES EL TURNO DE MORCAR».

## Rutinas

| Rutina del original | En el QL |
|---|---|
| `0x9CE0` bucle de monstruos | `ui_turno_monstruos()` |
| `0x9D6B` `TURNO_MONSTRUO` | `monstruo_planear()` |
| `0x9DC0`, `0x9E6A`-`0x9F7C` `CALCULAR_CAMINO` | `camino()` |
| `0x9F94`, `0x9FAF` `CASILLA_LIBRE` | `cuerpo_ocupada()`, `casilla_libre()` |
| `0xA075` `EJECUTAR_CAMINO` | `monstruo_paso()` |
| `0x990D`, `0x9921` | `ui_defensa()`, `combate_defensa()` |

## Diferencias deliberadas

- El plano se redibuja entero en cada paso (el original solo vuelca el buffer). El botón corta las esperas.
- Si la casilla del héroe no es de la sala del monstruo el original salta a `0xA999` (datos); no ocurre porque antes se comprueba `+3`: aquí devuelve 2.
