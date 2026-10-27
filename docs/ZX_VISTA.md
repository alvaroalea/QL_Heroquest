# La vista isométrica, las figuras y el plano en el original (ZX Spectrum) y su port a C

Análisis del código que dibuja la sala del personaje activo, las figuras y el plano de la mazmorra, y el port a `src/vista.c`. Verificación: `tools/hq_oraculo_vista.py` ejecuta **las rutinas originales del Spectrum** en el simulador Z80 de SkoolKit y `tests/vista_test.c` compara punto a punto la versión en C con los gráficos empaquetados de `data/` (`make test`, 2688 comprobaciones sobre 84 casos: 14 misiones × 6 posiciones de héroes; `tests/run_ql_vista.sh` repite una parte compilada con c68 y con el ensamblador, en sQLux).

Gráficos: los de los ficheros empaquetados (`sprites_bin`, `escenario_bin`, `baldosas_bin`, `marca_activo_bin`), nunca los del Spectrum. La geometría (que no son gráficos) está en `data/vista_bin` (`tools/hq_vista_empaquetar.py`, formato en su cabecera). Para que el escenario del QL tenga todo lo que el original dibuja se han añadido al extractor tres gráficos que no estaban: `escenario/bloqueo` (rombo de las casillas `0x30` y `0x31`, datos `0xFF79`), `escenario/trampa` (casilla `0x33`, datos `0xFF01`) y `fuentes/marca_activo` (baldosa `0xB1C0`, la marca del plano). Están al final de sus ficheros para no cambiar los índices existentes.

## Qué se ve y cómo se compone

La vista del juego es un rectángulo de 256 × 176 puntos (filas 2 a 23 de celdas; en el QL desde la línea `VY0 = 48`). Se compone en un buffer monocromo (`0xE700`, 32 bytes por línea) y el color lo ponen los atributos. En el QL cada gráfico lleva su color y su máscara.

- **Una sala a la vez.** La vista muestra solo la sala (número de la capa A, bits 0-5) en la que está el personaje activo, dentro de una **ventana de 6 × 5 casillas** cuya esquina superior izquierda sale de la tabla `0xC47C` (43 salas × 4 bytes: columna, fila, byte de atributo y marca de «sala ya visitada»). Las salas 1 a 20 son las 20 ventanas en las que se divide el mapa para los pasillos, con color blanco.
- **Las 30 casillas de la ventana** se numeran por diagonales (de la esquina de arriba a la de abajo): es el orden de dibujo, de atrás hacia delante, y el número de casilla que usan todas las tablas. `0xC414` da la posición (y, x) de la esquina del rombo de cada una (x del casillero 10 es −8: se recorta por la izquierda; la tabla tiene 52 entradas, las 22 últimas son posiciones fuera de la ventana para los movimientos).
- **Rombo del suelo 48 × 24** (`0xE4A8`), o el de la escalera de la salida (códigos de objeto `0x38`-`0x3B`), o el de casilla bloqueada (`0x30`, `0x31`) o de trampa (`0x33`).
- **Paredes y puertas de 24 × 50** a 39 líneas por encima del rombo: la pared norte a +24 puntos en X, la oeste en X. Las puertas «directas» (sin el bit 0 de la capa B) se dibujan igual, con OR; las **puertas como figura** (bit 0 de la capa B, y las de las casillas contiguas) son registros que se ordenan con las demás figuras.
- **Figuras**: heroes (tipo 3, sprite = byte de fotograma), monstruos (sprite = tipo del monstruo; los sprites 7 a 15 tienen el mismo dibujo), cofre y trono (sprites 64 y 65) y los 17 muebles (tabla `0x88A0`: tipo de dibujo, alto, desplazamiento y gráfico). Cada una es un **registro de animación** de 18 bytes (14 en total): centro X y línea de los pies Y (`posición del rombo + (0x18, 0x0F)`), tamaño y gráfico. Se dibujan con su halo (la máscara quita los puntos de alrededor).
- **Orden de dibujo**: cada registro tiene una clave (`casilla × 3 + 1`; las puertas, `casilla × 3` y `+2`) y se coloca en la tabla `0xFD00`; si el puesto está ocupado se intenta el anterior y después el siguiente. Se dibujan en orden de puesto.
- **Posición en pantalla** de una figura: arriba = `Y − alto`, izquierda = `X − ancho/2`; los héroes se dibujan en el punto exacto (rutina desplazada de `0xA5EA`) y el resto alineado al byte (`(X − ancho/2)` redondeado a múltiplo de 8). En el QL todas las posiciones son múltiplos de 4 (así hace falta una sola copia de cada gráfico).
- **Fondo guardado** (`GUARDAR_FONDOS`, `RESTAURAR_FONDOS`): antes de dibujar las figuras se guarda lo que hay debajo de cada una (y después el del cursor) y se devuelve al siguiente fotograma. Aquí igual, sobre la pantalla del QL, sin buffer de vista.

## Entrar en una sala (`0x8923`)

Fija el origen de la ventana, el color (`0xA985`) y mira la marca de visita (`+3` de la entrada): si ya estaba visitada deja `0xA98E = 0`; si no, `0xA98E = 0xFF` (y se descubren los monstruos de la sala) y la marca se pone a `0xFF`. Hay cuatro salas con eventos: misión 1 sala `0x16` (se encuentra a Sir Ragnar: `0xAA4D = 1`, todo el mapa a la vista y los monstruos ya no están ocultos), misión 5 sala `0x1D`, misión 9 sala `0x18` y misión 11 sala `0x1F` (ponen `0xA9B1 = 1`: evento de misión pendiente; la sala no se marca visitada hasta que se resuelva).

## Casillas que ve el personaje (`0xA2E1`)

Desde su casilla recorre las cuatro direcciones (norte, sur, oeste y este, ese orden) marcando cada casilla (bit 6 de la capa A y bit 7 de la de al lado y de la de abajo: aparecen en el plano) hasta una pared o una puerta cerrada, y hace aparecer (estado 1) a los monstruos que hay en ellas. En las columnas 12 y 13 recorre las dos columnas. En la misión 9 no lo hace. No es la visibilidad de la sala (esa ya la marca `0x98F6` al dibujar cada casilla de la sala).

## El plano (`0x92A0`, `0x94F8`)

Cada casilla del mapa es una baldosa de 8 × 8 (del buffer; casilla (0, 0) en la celda (3, 1)): si está a la vista en el plano (bit 6), la baldosa de paredes y puertas que da la tabla `0xC35A` según los bits altos de la capa B más el icono del monstruo, objeto, cofre, trono o la pieza base del mueble (tabla `0x9470`); si solo está a la vista en 3D (bit 7), solo las paredes que lindan con casillas ya vistas. Los personajes se pintan encima con su icono. Colores (`0x94F8`): casilla sin sala o aún no vista, amarillo sobre negro; vista, negro sobre amarillo. Con `0xA9A1 = 1` (se usa al elegir objetivo) los monstruos y los demás personajes salen como una marca (`marca_activo`), el personaje activo no sale y no se pintan objetos.

## Movimiento de los héroes (`docs/ZX_TURNO.md`)

`vista_iniciar_paso()` (`0x85D9`) pone el registro del héroe en estado 2 con el guion de la dirección; `vista_animar()` (la parte de `0xA7C9` de `GUARDAR_FONDOS`) muestra un fotograma por llamada (`fotograma del guion + 12 × héroe`, escrito en el fotograma del héroe) y le suma el desplazamiento (±4, ±2) cuando lo lleva; `vista_animar_clave()` (`0x85F2`) pone la clave de orden de dibujo de la casilla nueva. Las tablas (guiones, zonas de clic, desplazamientos de las casillas vecinas, nombres de objeto, destinos de teletransporte) están en `vista_bin`.

## Diferencias deliberadas con el original

- **Color de la sala.** En el original el color del suelo es el atributo de 17 tiras de celdas (`0xC528`) y la vista se limpia con 22 tiras (`0xA4BB`). Aquí la vista es siempre blanca (`vista_fondo` copia la zona de la vista del marco y rellena en blanco las 22 tiras) y el color lo da el rombo del suelo: `suelo()` dibuja `suelo_N` (índice 51 + N de `escenario_bin`), con N = paper del atributo de la sala (`sala_tab[sala][2] >> 3 & 7`). No hay recoloreo ni halos.
- **Halo de las figuras.** Los PNG llevan su propia máscara (el halo del original ya no se calcula), así que dentro del rectángulo de una figura puede haber diferencias con el original (`vista_test` las tolera). Fuera de las figuras la vista es idéntica punto a punto en cuanto a tinta negra.
- **Puertas como figura.** Se dibujan como cualquier figura, con el color y la máscara de su PNG (ya no se calcula halo al cargar).
- **Registros de animación.** Igual que el original: 14 registros (los 4 primeros, de los héroes; el 13, de Ragnar) y las figuras de la sala buscan hueco entre el 4 y el 16 (13 registros), por lo que los tres últimos no existen y una sala con más de 10 figuras perdería alguna (y podría pisar el registro de Ragnar).
- **Puntos por encima de `0xFD59`.** La tabla de orden tiene 90 puestos; una clave desplazada al puesto 90 o 91 no se dibuja (como en el original).
- Hay casillas del final del mapa (cuando el origen de la ventana está en las últimas filas) que leen más allá de la capa A; el original leía los bytes de la capa B. Aquí se leen ceros (el relleno).

## Rutinas

| Rutina del original | En el QL |
|---|---|
| `0x8923` entrar en la sala | `vista_entrar_sala()` |
| `0x8712` `CALCULAR_VISIBLES`, `0x875F` casilla de la sala | `vista_calcular_visibles()`, `casilla_visible()` |
| `0x98F6` `MARCAR_CASILLA_VISTA` | `marcar_vista()` |
| `0x8B64` suelo y escaleras | `suelo()` |
| `0x8A7F`, `0x8B2C`, `0x8B43` paredes y puertas | `paredes()`, `pared_directa()` |
| `0x8A40`, `0x8A99` puertas de las casillas contiguas | `puertas_vecinas()`, `puerta_figura()` |
| `0x87F4`, `0x8906`, `0x89CD` monstruos, cofre, trono y muebles | `objetos()`, `anim_grafico()`, `anim_posicion()` |
| `0x8791`, `0x87CB`, `0x8773` `ACTUALIZAR_HEROES` | `vista_actualizar_heroes()`, `tile_de()` |
| `0xA2E1`, `0xA300`, `0xA326`-`0xA3B2` `CALCULAR_VISTA` | `vista_calcular_vista()`, `ver_norte()`, `ver_sur()`, `ver_oeste()`, `ver_este()`, `marcar_y_vecinas()`, `aparece_monstruo()` |
| `0xA2C1` `DESCUBRIR_MONSTRUOS` | `vista_descubrir_monstruos()` |
| `0xA4FD` `ORDENAR_FIGURAS`, `0xA534` `DIBUJAR_FIGURAS` | `vista_ordenar()`, `vista_figuras_dibujar()` |
| `0xA794`, `0xA8DE` guardar y restaurar fondos | `vista_figuras_dibujar()`, `vista_figuras_borrar()` |
| `0x6E9C` `REDIBUJAR_TURNO` (sin el panel) | `vista_construir()` |
| `0x9BE3` `BORRAR_ANIMACIONES` | `vista_borrar_animaciones()` |
| `0x8BD4`, `0x9C77` color y limpieza de la vista | `vista_fondo()` |
| `0x92A0`, `0x931D`, `0x93EF`, `0x9449`, `0x945C`, `0x94F8` el plano | `vista_plano_buffer()`, `vista_plano_dibujar()` |
