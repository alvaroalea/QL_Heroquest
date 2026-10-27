# Lo propio de cada misión en el original (ZX Spectrum) y su port a C

Análisis de los eventos de las misiones 5, 9 y 11 (`EVENTOS_MISION`, `0x70A1`), de las **salas especiales** del icono del ojo (`0x827A` y la tabla `0x82A2`, rutinas `0x9005`-`0x91F4`) y de lo que ocurre al **salir un héroe por la escalera** (`0x8E7B` y la tabla `0x8FE9`, rutinas `0x8EBB`-`0x8FD2`), y su port a `src/mision.c`. Verificación: `tools/hq_oraculo_mision.py` ejecuta las rutinas originales en el simulador Z80 y `tests/mision_test.c` compara héroes, monstruos, capas, indicadores, marcas de salida, variables y los mensajes, preguntas y ataques (`make test`: 6160 comprobaciones en 560 casos; `tests/run_ql_mision.sh` repite 4 casos con c68 en sQLux). El oráculo también ejecuta las búsquedas completas del ojo (`0x8079`, `0x80A9`), con lo que se comprueba la emboscada con su ataque.

## Final de la misión

`PARTIDA` repite rondas mientras `MISION_CONTINUA` (`docs/ZX_PARTIDA.md`) y vuelve al menú: no hay pantalla de fin. Los héroes conservan en sus registros el oro (`+9`, `+10`) y el equipo (`+0x0E`, `+0x0F`), que son los que ve la tienda, y **solo siguen disponibles los que han salido vivos por la escalera** (las marcas `0x8FE4`-`0x8FE7` se ponen a cero al empezar y `0x8E7B` las vuelve a poner): un héroe muerto o que no sale se pierde para las siguientes misiones. El port lo repite en `game_run()` (`oro`, `armas` y `armadura` vuelven a `heroes[]` y `activo` = ha salido). Si el jugador aborta con Esc no se copia nada.

## Salir por la escalera (`0x8E7B`)

Sonido, cancela el movimiento, marca al héroe como ausente (`0xA254`), pone su marca de disponible, lo quita de la partida y termina su turno (`turno.c`); dibuja «TU HAS ESCAPADO DE LA MAZMORRA» (`BB8E`) y salta a la rutina de la misión (tabla `0x8FE9`, `mision_salida()`):

| Misión | Rutina | Qué pasa |
|---|---|---|
| 0 | `0x8EBB` | solo el primero que sale cobra 100 monedas (`0xA9AE`) y sale `BBB0` |
| 1 | `0x8ED4` | si sale Ragnar (personaje 4): `¿DESEAS DIVIDIR LA RECOMPENSA?` (`B0B6`). Sí: 200, 100, 66 o 50 monedas a cada héroe vivo o ya salido según cuántos sean (`0x8F29`); no: 200 monedas al héroe que lo encontró (`0xA9B0`) |
| 3 | `0x8F5D` | si el héroe lleva el oro de Magnus (bit 6 de `+0x0F`): 200 monedas, `¿DESEAS QUEDARTE CON EL ORO?` (`B058`; el texto depende de la respuesta: `BF16` si y `BEDC` no) y, si no se han cogido las tres bolsas (`0xA9AF`), `¿DESEAS IR A POR MAS ORO?` (`B084`): si, vuelve a entrar en (3, 11) y **sigue su turno** |
| 6 | `0x8EC4` | cada héroe que sale cobra 100 monedas (`BBB0`) |
| 9 | `0x8F4E` | si ya no queda ningún héroe en la partida (`0xA9B7 = 0x0F`), `BBDE` |
| las demás | `0x8FD2` | solo se espera |

## Eventos al entrar en una sala (`0x70A1`)

`vista.c` (`0x8923`) deja `Vars.evento_mision = 1` al entrar por primera vez en la sala 0x1D de la misión 5, la 0x18 de la 9 y la 0x1F de la 11 (los números de misión son el valor de `0xA981`, de 0 a 13, como en el resto de la documentación). El bucle del turno llama a `mision_evento()` en cada fotograma:

- **Misión 5** (`0x7105`): cada héroe, una sola vez (máscara `0xA9B2`), recupera el equipo que se le quitó al empezar (`0xA271`) y sale `C0BD`.
- **Misión 9** (`0x70C8`): una vez por héroe, `¿DESEAS COGER 5000 MONEDAS DE ORO?` (`B0F1`): si acepta, lleva el oro (bit 5 de `+0x0F`: no puede atacar ni defenderse; ver `docs/ZX_COMBATE.md`); acepte o no, no se le vuelve a preguntar (bit 4).
- **Misión 11** (`0x70B0`): una sola vez sale `C100` (la sala se cierra).

## Salas especiales (`0x827A`)

El indicador de una sala (`Partida.ind`: 0 a 42 tesoros, 43 a 85 trampas), si no vale 0 ni `0xFF`, indica un tratamiento propio al buscar (tabla `0x82A2`, valor 1 a 18). Casi todos marcan el indicador como usado (`0xFF`), dan algo al héroe y muestran una ventana:

| Valor | Rutina | Efecto |
|---|---|---|
| 1 | `0x9005` | `BD76` |
| 2 | `0x9010` | si la trampa de la sala (`+0x2B`, el indicador de trampas) no se ha desactivado: `BD30` y pierde 1 de cuerpo (o muere); `BCA3` |
| 3 | `0x904A` | 30 monedas, una poción de curación (la segunda va al bit 3) y `BC14` |
| 4 | `0x9073` | 100 monedas, igual que el 2 y `BC51` |
| 5 | `0x90BF` | una lanza (no el hechicero: `BDD1`); `BDAD` |
| 6 | `0x903D` | 50 monedas, poción de curación y `BCB9` |
| 7 | `0x90E0` | un tesoro de la misión 3 (oro de Magnus, bit 6 de `+0x0F`): quita el objeto de una de las tres casillas (19, 10), (17, 10) o (17, 12) según `0xA9AF`; `BE93` |
| 8 | `0x9120` | `BF5F`: quita una roca y abre una puerta secreta (casillas 98, 99 y 124) |
| 9 | `0x9142` | objeto 2 (bit 2 de `+0x0D`) y `BFE2` |
| 10 | `0x9155` | objeto 0 (armadura de Borin) y `C002` |
| 11 | `0x9168` | `C041`, 200 monedas y `C01F` |
| 12 | `0x9181` | objeto 1 (vara de recuerdo), 150 monedas y `BCF6` |
| 13 | `0x9197` | 200 monedas y `BC79` |
| 14 | `0x91A5` | un escudo (no el hechicero: `BE1B`); `BDF5` |
| 15 | `0x91C6` | `BE41` |
| 16 | `0x91D1` | aparece el monstruo 2 (`BE6B`) y **ataca** (`PANEL_COMBATE`) |
| 17 | `0x91E4` | objeto 4 (espada espiritual) y `BF99` |
| 18 | `0x9173` | 200 monedas y `C01F` |

## La emboscada ataca

El monstruo que sigue al héroe en la búsqueda de tesoros (`0x8195`) también ataca con `PANEL_COMBATE` nada más aparecer; en el paso 4 solo se mostraba su ventana. Ahora `accion.c` llama al gancho `accion_ataque` (el juego muestra la pantalla de combate de la defensa y `mision.c` lo usa también en la sala 16).

## Rutinas

| Rutina del original | En el QL |
|---|---|
| `0x70A1` | `mision_evento()` |
| `0x827A`, tabla `0x82A2`, `0x9005`-`0x91F4` | `mision_especial()` (gancho `accion_especial`) |
| `0x8EAF`, tabla `0x8FE9`, `0x8EBB`-`0x8FD2` | `mision_salida()`, `ventana_salida()` (`juego.c`) |
| `0x8D5E` ventana de sí o no | gancho `mision_pregunta` (`ui_pregunta()`) |
| `0x91F7` oro en el panel | `MSG_ORO_PANEL` |
| `0x8195` ataque de una emboscada | gancho `accion_ataque` (`ui_ataque()`) |

## Diferencias deliberadas

- Todas las ventanas se dibujan sobre la escena (en el original, según el caso, con o sin repintado previo) y el botón corta la espera.
- No hay pantalla de fin de misión: se vuelve al menú como el original (se quitó la ventana provisional `PORT_FIN_MISION`).
