# El turno de un personaje en el original (ZX Spectrum) y su port a C

Análisis de lo que ocurre desde que le toca a un personaje hasta que termina su turno: tirada de movimiento, clic, movimiento casilla a casilla, camino automático, animación del paso, marcadores y turno de Morcar. Código: `src/turno.c` (lógica), `src/juego.c` (bucle, marcadores y plano), animación en `src/vista.c`. Verificación: `tools/hq_oraculo_turno.py` ejecuta **las rutinas originales del Spectrum** en el simulador Z80 y `tests/turno_test.c` compara el estado fotograma a fotograma (`make test`: 411 casos, 16.332 comprobaciones; `tests/run_ql_turno.sh` repite unos casos compilados con c68 en sQLux).

## La partida (`PARTIDA`, `0x6D89`)

```
PARTIDA: música (si está activa), marco, borrar registros de animación
   repetir:
      TURNO_PERSONAJE para los 4 héroes (A = 0 a 3) y el quinto (A = 4: Sir Ragnar en la misión 1)
      MISION_CONTINUA (0x6EB5): si no, fin
      FIN_DE_RONDA (0x9CE0): turno de Morcar
```

## `TURNO_PERSONAJE` (`0x6DDD`)

1. Si el registro del héroe está vacío (`+0 = 0`) vuelve. Pone a cero `0xA9A4`, `0xA9B9`, `0xA9BF`, `0xA9C0` y `0xA9B5`; dibuja su nombre (`0x9C5B`) y los marcadores (`0x6F13`).
2. **Estados** (`+0x18`): con el bit 2 (envuelto en una tempestad) pierde el turno y se muestra `TE VES ENVUELTO POR UNA TEMPESTAD` (si además duerme tira un dado: con un 5 despierta; si no, antes sale `ESTAS DURMIENDO`); con solo el bit 1 (dormido) tira un dado: con un 5 despierta y juega, si no pierde el turno con `ESTAS DURMIENDO`. Las ventanas esperan 200 interrupciones (`0x8FCF`).
3. `REDIBUJAR_TURNO` (`0x6E9C`): borra los registros de animación y recalcula la vista (`docs/ZX_VISTA.md`).
4. `CALCULAR_DADOS` (`0x6ECD`) y `TIRAR_DADOS` (`0x6F1C`).
5. **Bucle de fotograma** (`0x6E53`), una vuelta por interrupción de 50 Hz:

```
MOVER_CURSOR (0x713E)      no se mueve si hay un evento pendiente (0xA9B3)
PROCESAR_CLIC (0x7201)
MOVER_AUTOMATICO (0x6FDA)
EVENTO_ALEATORIO (0xA108)  (eventos de las misiones y de las trampas: aún no portado)
ANIMAR_FIGURA (0x85F2)
ORDENAR_FIGURAS (0xA4FD), GUARDAR_FONDOS (0xA794: aquí avanza la animación), figuras, cursor, volcado, RESTAURAR_FONDOS
si 0xA987: REDIBUJAR_TURNO
hasta que 0xA98A != 0
```

## La tirada (`TIRAR_DADOS`)

Un contador de 0 a 7 (`0x6FD9`) hace **parpadear** al héroe (el registro de animación está en estado 0 durante cuatro fotogramas y en 1 durante otros cuatro). **En cada fotograma se vuelve a tirar** el número de dados de movimiento (suma de cara + 1) y se muestra: lo que sale es lo que había en pantalla cuando el jugador pulsa el botón. Después se espera a soltarlo, 10 interrupciones, y el registro queda en estado `0xFF`. `CALCULAR_DADOS`: 1 dado; 2 para los héroes (no para Morcar ni con el bit 3 de `+0x13` ni el 6 de `+0x0F`); con el bit 5 de `+0x17` se duplica (y se borra el bit 5 de `+0x10`) y con el bit 3 de `+0x18` otra vez (si sale 8 se deja en 6).

## `PROCESAR_CLIC` (`0x7201`)

Se ignora mientras queda camino por andar (`0xA9B9 != 0`) o el héroe está animado (`0xA988` y registro en estado 2). Con el botón pulsado espera a que se suelte (`0x8DAA`), convierte el cursor en casilla de pantalla (`columna = (X − 12) / 8`, `fila = (Y − 2) / 8`) y recorre la tabla de zonas (`0xB4F0`, 10 zonas de 6 bytes). Zonas de la pantalla de juego (columnas × filas): flechas diagonales (22-23 × 20: x−1; 22-23 × 21: y+1; 24-25 × 20: y−1; 24-25 × 21: x+1), ojo (26-28 × 20-21), siguiente personaje (9-11 × 20-21), mapa (29-31 × 19-21), bolsa (26-27 × 18-19), atacar (28-31 × 17-18) y llave (20-21 × 21). Si no cae en ninguna, `IR_HACIA_CURSOR`.

**`IR_HACIA_CURSOR` (`0x6FF6`)**: busca entre las 52 posiciones de la vista la más cercana al cursor (distancia en 8 bits: `(|x + 0x24 − X| / 2 + |y + 0x0C − Y|) / 2`, gana la última de las empatadas), obtiene la casilla del mapa que le corresponde (origen de la ventana + desplazamiento de la posición; las posiciones 45 a 51 tienen desplazamientos negativos y el original los divide **como número de 16 bits sin signo**) y, si **comparte fila o columna con el héroe**, deja en `0xA9B9` el número de casillas y en `0xA9BD` el manejador del paso. Si no está en línea no hace nada. **`MOVER_AUTOMATICO`** da un paso por cada vez que se llama mientras el héroe no esté animado (cada paso tarda 12 fotogramas en animarse).

## Los cuatro pasos (`0x82CB`, `0x8357`, `0x83E3`, `0x8478`)

Comparten la lógica (ver `turno_paso()`): se cancelan (`0xA9B9 = 0`, `0x82C6`) si el héroe no puede mover (bit 2 de `0xA98B`, sin puntos de movimiento o en el borde del mapa); la **pared** (bit 6 de la casilla para x, bit 7 para y; la del propio héroe al ir hacia x−1 o y−1, la de la casilla de destino al ir hacia x+1 o y+1) solo se atraviesa con «atravesar muros» (`+0x18` bit 5) y hacia una casilla de una sala; la **puerta** (bit 4 o 5): si está cerrada y no bloqueada (bit 3) **se abre al pasar** (bit 2), pero una puerta no se cruza dos veces en el mismo turno (`0xA9BF`); en la casilla de destino un monstruo o un mueble (`< 0x32` o `>= 0x40`) bloquea, y los códigos `0x32`-`0x36` (objetos y trampas) se convierten en otra cosa y dejan un **evento pendiente** (`0xA9B3`, tabla `0xA24A`) y cancelan el camino automático (el paso se da). Una escalera de salida (`0x38`-`0x3B`) lleva a `0x8E7B`.

**`FIN_PASO` (`0x850F`)**: guarda la casilla nueva, pone el fotograma del héroe parado (`tabla 0x850A[dirección] + 0x8505[personaje]`), marca «ha movido» (bit 0 de `0xA98B`), resta un punto de movimiento y, si la casilla nueva sigue en la sala del héroe, **empieza la animación** (`0x85D9`: registro en estado 2 y el guion de esa dirección de `0xC1B6`) y recalcula lo que ve (`0xA2E1`); si es de otra sala pide repintar la vista (`0xA987`). En la **misión 9** pisar una casilla sin sala es un teletransporte al azar (tabla `0x85AD`, dos dados).

**El guion de animación** (4 guiones, uno por dirección): doce fotogramas (los fotogramas `0x10`-`0x1A` del héroe, `+ 12 × héroe`, `+ 0x18` para el quinto) con seis desplazamientos de (±4, ±2) puntos, que suman los (±24, ±12) de una casilla a la siguiente. Cada fotograma del bucle muestra uno y, si lleva desplazamiento, mueve la figura. Todas las posiciones son múltiplos de 4 puntos.

## La salida (`0x8E7B`)

Anula el camino, marca al personaje como si hubiera muerto (bit en `0xA9B7`), vuelve a ponerle la marca de «disponible» del menú (`0x8FE4 + personaje = 0xFF`, aquí `Partida.salidos`), vacía su registro (`+0 = 0`), termina el turno, muestra «TU HAS ESCAPADO DE LA MAZMORRA» (`VENT_BB8E`) y salta por la tabla `0x8FE9` a lo que pasa en cada misión (recompensa en oro, texto de victoria...). Esto último no está portado: `turno_evento_salida` y `juego_salida_mision` son ganchos.

## `FIN_DE_RONDA` (`0x9CE0`)

Muestra el plano (como `MOSTRAR_MAPA`) y «ES EL TURNO DE MORCAR», espera 150 interrupciones, hace actuar a cada monstruo (`TURNO_MONSTRUO`, `0x9D6B`: aún no portado, es el gancho `juego_turno_monstruos`), muestra «EL TURNO DE MORCAR SE TERMINO», espera 100 y quita el bit 0 de `+0x18` de los cinco personajes.

## Marcadores (`0x926A`, `0x91F7`, `0x9242`, `0x9C5B`)

En las filas 0 a 3 de la vista: el nombre del personaje con su color; en la fila 1 el cuerpo (corazón rojo `0x5D`) y la mente (`0x5E`, verde, columna 5); en la 2 el oro (`0x3C`, cuatro cifras, amarillo) y en la 3 los puntos de movimiento (`0x5C`, cian, sin el cero a la izquierda).

## `MOSTRAR_MAPA` (`0x7710`) e `INFO_BAJO_CURSOR` (`0x7733`)

Limpia la pantalla, dibuja el plano y deja mover el cursor. Debajo del cursor, si la casilla está a la vista en el plano, escribe en la fila 21 el nombre del monstruo (tabla de tipos), del objeto o mueble (`0xB5C3` indica la cadena de cada código) o del personaje que hay. Cualquier pulsación del botón termina y `FIN_ACCION` (`0x8009`) repinta todo.

## Diferencias con el original

- **Pulsación**: el original espera a que se suelte el botón dentro del propio manejador del clic; aquí el clic cuenta al pulsar (flanco), sin esperar.
- **Fondo de los marcadores**: se dibujan sobre fondo negro (el original deja la imagen del marco en los puntos apagados).
- **Nombre bajo el cursor**: se borran las nueve celdas antes de escribirlo (el original solo escribe tres espacios cuando no hay nada y deja restos del nombre anterior).
- **Final**: al acabar la misión se muestra la ventana provisional `PORT_FIN_MISION` y se vuelve al menú; ESC aborta la partida.
- **Faltan** (siguientes pasos): los iconos ojo, bolsa, atacar y llave (ganchos `juego_accion`), el turno de los monstruos, los eventos (`0xA108`, `0x70A1`) y lo que ocurre en cada misión al salir.
