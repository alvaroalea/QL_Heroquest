# Port a QL: estado y estructura

Objetivo: `build/heroquest.img`, disco de 720 KB (QDOS) con el juego en C89 (XTC68) y las partes críticas en ensamblador 68000.
`make` (o `make img`) lo crea y regenera antes los datos si ha cambiado algún fichero de `asset/` o de `tools/hq_*` (marca `build/datos/.datos`; `make data` los regenera solo); `make run` lo arranca en sQLux; `make host` y `make test` compilan y prueban una versión de PC.

## Qué funciona

- **Primera pantalla: idioma** (`menu_idioma`, `menu.c`): «1 - ESPANOL / SPANISH» y «2 - ENGLISH / INGLES», con O/K, cursores arriba/abajo y ESPACIO, o las teclas 1 y 2 (ESC sale del programa). Suena la **melodía 3** del original, que ningún código del Spectrum usa (169 s; `docs/ZX_MUSICA.md`); al elegir idioma empieza la del menú. Elige el idioma (`g_idioma`, `data.c`) de las **misiones** (`heroquest_NN_es` o `_en_bin`, `heroquest_xx_idx`) y de las **ventanas**: `tx_item` (`text.c`) redirige cada ventana española (`TX_VENT_*`) a su equivalente inglesa (`EN_VENT_*`, tabla `build/datos/vent_en.h`) según `asociación de asset/textos_ql/equivalencia_en_es.txt`. Detalles del inglés: las ventanas `[NEW]` (brebaje heroico `AE3B`, agua santa `AE93`, tormenta y sueño a la vez `B010`) se usan en `juego.c`; los nombres y descripciones de pociones inglesas son `EN_POCIONES_*`. También salen en inglés los guiones de la partida y del menú y las tablas de nombres y del panel: todos los elementos españoles de `textos_bin` tienen su `EN_*` (`build/datos/idioma_en.h`; guiones por `equivalencia_en_es.txt`, tablas por tener el mismo id). Los nombres de héroe ya copiados a `heroes[].nombre` no cambian al cambiar de idioma. La fuente no tiene `-` ni `/`: esa pantalla usa una copia de la fuente con esos dos glifos puestos en los códigos 0x3F y 0x40.
- **Cambiar de idioma dentro del juego**: opción `4.IDIOMA` de la pantalla de controles (tecla 4; `plat_tecla4` activa su lectura solo en esa pantalla, porque cada fila de KEYROW es una llamada lenta).
- **Menú principal** (JUGAR, COMPRAR EQUIPO, CREAR PERSONAJE, CONTROLES), con las pantallas y colores del original. «Cargar otras aventuras» (5.ª opción): pide el nombre de una ampliación y carga su índice y sus misiones (`witchlord`: `docs/ZX_AMPLIACION.md`).
- **Jugar**: lista de misiones, texto del objetivo, elección de hechizos (después de leer la introducción: primero el hechicero y luego el duende; si juegan los dos, el hechicero elige 1 grupo, el duende 1 y los que sobran pasan al hechicero) y la **partida**: se carga la misión (`partida_cargar`), se muestra la sala del personaje con su suelo, paredes, puertas, muebles, monstruos y héroes, y los marcadores (nombre, cuerpo y mente, oro y puntos de movimiento). Cada personaje (bárbaro, enano, elfo, hechicero y, en la misión 1, Ragnar) juega su **turno**: tirada de dados (el héroe parpadea y los puntos cambian hasta pulsar el botón), mover el cursor con las teclas y hacer clic en las flechas diagonales o en una casilla en línea con el héroe (camina hasta ella), las puertas se abren al pasar, las trampas y objetos se pisan, el icono de «siguiente personaje» pasa el turno, el icono del **mapa** muestra el plano de la mazmorra con el nombre de lo que hay bajo el cursor, la **llave** abre las puertas de al lado y deja ver la sala de detrás, el **ojo** (una vez por turno) busca trampas y puertas secretas o **tesoros** (mazo de cartas: oro, pociones, trampas, fantasma o un monstruo que te sigue), las trampas pisadas dejan fosos, lanzas y rocas y los héroes pueden **morir**, y la escalera de salida saca al héroe de la partida. Al final de la ronda sale el plano y «ES EL TURNO DE MORCAR» (los monstruos aún no hacen nada). La partida termina cuando todos los héroes han salido o muerto (ventana provisional «HAS LLEGADO AL FINAL DE LA PARTE JUGABLE»); **ESC** la aborta y vuelve al menú.
- **Comprar equipo**: lista de héroes, 7 armas y 4 armaduras más la caja con sus precios, oro del héroe; compra si alcanza el oro. (Los héroes empiezan con 0 de oro: de momento no hay forma de ganarlo.)
- **Crear personaje**: elegir héroe, cambiar el nombre con la rejilla de letras, guardar y cargar el personaje en un fichero del disco (`hq_NOMBRE`, 16 bytes).
- **Controles**: 1 = tipo de ratón, que rota entre CURSORES (las teclas de cursor también mueven; las del original O/K/Z/X/ESPACIO funcionan siempre), QIMI (interrupciones externas en `ql_ipc.asm`, ver `docs/HARDWARE_QL.md`; sin probar), QIMSI (`plat_keys` lee el puerto de ROM, ver `docs/HARDWARE_QL.md`; el botón izquierdo es ESPACIO) y POINTER ENV (IOP.RPTR sobre la consola, ver `docs/HARDWARE_QL.md`; sin probar); los no disponibles se saltan (`plat_raton_ok`), 2 = música (con QSound: melodía del menú y de la partida), 3 = tipo de letra manuscrita, 0 = salida.
- Teclas: O/K arriba/abajo, Z/X izquierda/derecha, ESPACIO = botón, además de los cursores; ESC vuelve atrás; **en la partida** pide confirmación con la ventana `PORT_ABANDONAR` («ABANDONAR LA MISION», SI / NO con el cursor; ESC o NO sigue jugando) antes de volver al menú (`abandonar`, `juego.c`); **en el menú principal** muestra una pantalla negra «SALIR A QDOS S/N» (`salir_qdos`, `menu.c`; texto en C porque la fuente no tiene `/`): Y o S sale del programa y cualquier otra tecla vuelve al menú (Y y S se leen solo con `plat_tecla4`, bit `K_YES`); ESC en la pantalla de idioma sigue saliendo directamente; 0 a 3 en la pantalla de controles.

## Código (`src/`)

| Fichero | Contenido |
|---|---|
| `platform.h`, `plat_ql.c`, `plat_host.c` | capa de plataforma: pantalla del QL (modo 8 en `$20000`), consola de c68 a pantalla completa, teclado por IPC (KEYROW), reloj de 50 Hz, directorio de datos (`flp1_` o DATA_USE). La versión de PC lee las teclas de un guion y guarda capturas |
| `ql_ipc.asm` | `trap #1`, `trap #3`, `MT.IPCOM` (teclado) y el contador de cuadros (tarea de la lista de interrupciones sincronizadas; **no** usa A3: QDOS no deja ahí el enlace) |
| `gfx.c`, `gfx_asm.asm` | texto (glifos de 8×8 con color de tinta y papel), celdas, volcado de gráficos HQGF con o sin máscara. En el QL (`-DQL_ASM`) el borrado, el volcado y los glifos van en ensamblador; la versión en C es la de referencia y se comprueba que sale idéntica (`tests/ql_asm_vs_c.sh`) |
| `data.c` | carga de los ficheros `data/*_bin` (formatos en `docs/ZX_GRAFICOS_QL.md`, `docs/ZX_DATOS_QL.md`) |
| `text.c` | guiones, ventanas y cadenas de los textos (fichero `textos_es` / `textos_en`); los números `TX_*` salen de `build/datos/textos_ids.h` |
| `keys.c` | eventos de teclado (pulsaciones nuevas) para los menús |
| `qsound.c`, `music.c` | detección de la QSound y motor de música (`docs/QSOUND.md`) |
| `cursor.c` | cursor de la vista: movimiento, casilla bajo el cursor, dibujo y restauración del fondo |
| `combate.c`, `combate.h` | el combate, el panel del personaje y los hechizos (`docs/ZX_COMBATE.md`): dados, alcance y línea de vista, tiradas, muerte de un monstruo y recompensas, manejadores del panel, efectos de los hechizos |
| `mision.c`, `mision.h` | lo propio de cada misión: eventos de las misiones 5, 9 y 11, salas especiales del ojo y la salida por la escalera (`docs/ZX_MISIONES.md`) |
| `monstruo.c`, `monstruo.h` | el turno de los monstruos: camino hasta un héroe, pasos y ataque (`docs/ZX_MONSTRUOS.md`) |
| `juego_combate.c`, `juego_int.h` | pantallas del combate y del turno de Morcar: elegir objetivo en el plano, pantalla de combate con retratos y dados, panel del personaje, bolsa y lanzar hechizos |
| `accion.c`, `accion.h` | acciones de los iconos de llave y ojo: puertas, búsquedas, mazo de tesoros, oro, eventos de trampas y muerte de héroes (`docs/ZX_ACCIONES.md`) |
| `turno.c`, `turno.h` | lógica del turno de un personaje: dados, clic y zonas, camino al cursor, los cuatro pasos y su animación (`docs/ZX_TURNO.md`) |
| `juego.c`, `juego.h` | bucle de la partida: rondas, turno (tirada, cursor, clic, movimiento), marcadores, plano con nombres y turno de Morcar |
| `vista.c`, `vista.h` | la vista isométrica de la sala del personaje activo (casillas, paredes, puertas, muebles), las figuras ordenadas con su fondo guardado y el plano de la mazmorra (`docs/ZX_VISTA.md`) |
| `partida.c`, `partida.h` | estado de la partida: misión cargada (capas del mapa, héroes, monstruos, indicadores), mazo de tesoros y dados; no depende de la plataforma (`docs/ZX_PARTIDA.md`) |
| `menu.c`, `game.c`, `main.c` | menús, partida (provisional) y arranque |

Arranque: `data/boot` pone el modo 8, vuelca la pantalla de carga (`heroquest_scr`, ver `docs/ZX_GRAFICOS_QL.md`), lanza el juego y al salir vuelve al modo 4.

Pantalla: el Spectrum es de 256×192; se centra en los 256×256 del QL con 32 líneas negras arriba y abajo (`GY0 = 32`). Las filas de los guiones del original (buffer de la vista del juego) empiezan en la fila 2 de celdas (`UI_ROW0`).

## Textos propios del port

`asset/textos_ql/textos_port.es.txt` y `textos_port.en.txt` (solo existen ahí; no salen del original): pantalla de controles, mensajes, la ventana final provisional y los rótulos del combate (`PORT_COMBATE`, antes `textos_combate.txt`). Los ingleses se empaquetan como `EN_PORT_*` y `tx_item` los elige según el idioma, igual que las ventanas (`build/datos/idioma_en.h`). En `asset/textos_ql/textos.txt` se ha corregido la errata «ENAMO» del menú de la tienda.

## Pruebas

- `make test`: primero `tests/partida_test.c` (1584 comprobaciones de `partida.c` contra el original: `tests/oraculo_partida.txt`, que genera `make oraculo` ejecutando las rutinas del Spectrum en un simulador Z80) y después la versión de PC con teclas guionizadas; compara el CRC de 66 capturas (cada guion empieza pasando la pantalla de idioma con F; `idioma` y `jugar_en` prueban la elección y las misiones en inglés) con `tests/esperado_host.txt` (`tests/run_host.sh --actualizar` tras revisar `build/host/t/*.png`).
- `tests/combate_test.c` (dentro de `make test`): 3528 comprobaciones de `combate.c` contra el original (`build/oraculo_combate.txt`, de `tools/hq_oraculo_combate.py`): ataque (preparar, alcance y línea de vista, bonos, tirada, muerte y recompensa), defensa de un héroe, hechizos y manejadores del panel; `tests/run_ql_combate.sh` repite 8 casos con c68 en sQLux.
- `tests/monstruo_test.c` (dentro de `make test`): 2520 comprobaciones de `monstruo.c` contra el original (`build/oraculo_monstruo.txt`, de `tools/hq_oraculo_monstruo.py`); `tests/run_ql_monstruo.sh` repite 6 casos con c68.
- `tests/mision_test.c` (dentro de `make test`): 6160 comprobaciones de `mision.c` contra el original (`build/oraculo_mision.txt`, de `tools/hq_oraculo_mision.py`), incluidas las búsquedas del ojo con la emboscada; `tests/run_ql_mision.sh` repite 4 casos con c68.
- `tests/run_ql_partida.sh`: la misma prueba compilada con c68 y ejecutada en sQLux (sonda de c68 para `partida.c`).
- `tests/vista_test.c` (dentro de `make test`): 2688 comprobaciones de `vista.c` contra el original (`build/oraculo_vista.txt`, generado por `tools/hq_oraculo_vista.py`, 6 MB): estado de las capas, registros de figuras, orden de dibujo y los puntos de la vista y del plano. `tests/run_ql_vista.sh` repite un caso compilado con c68 y con el ensamblador en sQLux (la RAM libre del QL no da para más). `tests/vista_demo.c` dibuja una sala en el PC para verla.
- Un programa del QL tiene poca pila: los arrays grandes han de ser `static` (el test de la vista no arrancaba con 20 KB en la pila).
- `tools/ql_test.sh`: la misma prueba en sQLux con capturas (ver `docs/HERRAMIENTAS.md`).
- `make c68_check`: compila con enteros de 16 bits (solo avisos).

## Pendiente

«Cargar otras aventuras» (no necesario), profiling y paso a ensamblador.

## Pendiente de la ampliación (Return of the Witch Lord)
Ordenado por importancia (análisis en `docs/ZX_AMPLIACION.md`): 
    1) *(hecho: salas especiales 19 a 29, ver `docs/ZX_AMPLIACION.md`)*; 
    2) *(hecho: rutinas de muerte por estadística, salida y epílogo)*; 
    3) *(hecho: monstruo errante y arma rota)*; 
    4) *(hecho: tabla `[monstruos]` por aventura; falta ajustar los retratos, iconos y sprites de los tipos nuevos)*. 
Hoy sus misiones cargan, se ven y se juegan sin eventos (`eventos -`); la idea es que cada comportamiento nuevo sea una bandera más de la línea `eventos` de la misión.

- **Fallo al cargar una misión** (`game_fallo`, `game.c`): si `data_load` no consigue memoria (`data_error` = 2, p. ej. con el Pointer Environment cargado y `RAMTOP` bajo), falta el fichero (1) o `vista_datos` no consigue memoria (`vista_error` = 3) o no vale, se muestra el mensaje en el idioma elegido, se espera una tecla y se vuelve al menú.

## Memoria (medida el 2026-10-29, `POOL_MAX` 12.000, retratos al final, misiones precargadas)

| Dónde | Qué | Caso más restrictivo | Todo en memoria |
|---|---|---:|---:|
| Código | `build/heroquest` | 90.472 | 90.472 |
| Job (dataspace) | estáticos (c68 informa ≈ 28.900: `Partida` 2.224, `plano_buf` 5.632, `plano_los` 5.632, tablas de la vista, fuentes de los menús…) y pila, hasta los 40.000 de `qltools -x` / `ql_win.sh` | 40.000 | 40.000 |
| Montón, fase 1 | `heroquest_dat` sin los retratos (escenario 67.704, marco 32.792, sprites 60.256, baldosas 256, vista 2.770, música 5.642, efectos 1.228, cabecera) | 170.724 | 170.724 |
| Montón, siempre | `fonts_dat` 1.400 + textos del idioma 8.392 (es; en 8.172) + índice de la aventura 608 | 10.400 | 10.400 |
| Montón, fase 2 | las 14 misiones de Hero Quest (es; en 25.886; Witch Lord 19.124 / 18.580) | 0 (del disco una a una, ≤ 2.088 al cargar, se libera) | 25.638 |
| Montón, fase 3 | retratos: tabla 200 + ataque fallido 4.096 + búfer 4.096, o enteros | 8.392 | 49.352 |
| Montón, en partida | `pool` y `pool2` de la vista (12.000 cada uno) + `back` (176 × 128) | 46.528 | 46.528 |
| **Montón total** | | **236.044** | **302.642** |
| **Total con código y job** | | **≈ 366.516** | **≈ 433.114** |
| Pantalla | `$20000`, la del sistema, no es nuestra | 32.768 | 32.768 |

- **Caso más restrictivo** (aventura inicial de 14 misiones; ni misiones ni retratos caben): 90.472 + 40.000 + 236.044 ≈ **367 K**. La partida solo necesita los 46.528 de la vista, más lo fijo (181.124 + 8.392 de retratos mínimos = 189.516 + 46.528 = 236.044).
- **Caso que carga todo** (misiones y retratos enteros): 90.472 + 40.000 + 302.642 ≈ **433 K**. Los ≈ 66,6 K de diferencia son las misiones (25,6 K) y los retratos enteros (49,4 − 8,4 = 41 K).
- **Reserva al cargar:** `misiones_precargar` y `data_retratos_cargar` piden `RESERVA_PARTIDA` (50.000) para comprobar que queda sitio para la partida y la liberan; durante la fase 3 el montón llega a ≈ 306 K (con todo cargado y la reserva a la vez), unos 3,5 K más que en partida.
- Las cifras no incluyen la sobrecarga de `malloc` (cabeceras de cada bloque) ni lo que ocupe la pila del job por encima de los estáticos.
- Con RAMTOP 640 en sQLux no se precargan misiones y los retratos se leen del disco (caso restrictivo); con 2048, todo en memoria. Con el Pointer Environment (`ptr_gen` + `wman` + sus búferes) no cabe ni lo mínimo: ver `docs/HARDWARE_QL.md`.
- Lo grande son los 170,7 K de gráficos de la fase 1 (escenario, sprites y marco), que se cargan siempre enteros.

- **Índice y textos en memoria:** solo hay **un idioma y una aventura** cargados. `mis_indice` es el índice de la aventura activa (`<aventura>_<idioma>_idx`, ≈ 600 bytes: ya no lleva los textos) y `txt` los textos del idioma (`textos_es` / `textos_en`, ≈ 8 K, generados por `hq_misiones_empaquetar.py`). `data_aventura` carga el índice de la elegida y libera el anterior; `data_idioma` libera textos e índice y los vuelve a leer del disco (si no hay índice de la aventura en ese idioma, vuelve a Hero Quest). Ahorro: el índice de Hero Quest ya no se queda en memoria (≈ 9 K) cuando se juega otra aventura.
- **Misiones precargadas** (`data.c`, `misiones_precargar`): al cargar el juego y al cambiar de idioma o de aventura se intenta tener **todas las misiones de la aventura en memoria** (≈ 26 K las 14 de Hero Quest). Antes se reservan `RESERVA_PARTIDA` (50.000 bytes: los bufferes de la vista de la partida más margen) y se liberan al acabar, para que la partida siempre tenga sitio; si algo no cabe se liberan todas y cada misión se lee del disco al iniciarla, como antes. `game.c` usa `data_mision_obtener` / `data_mision_soltar`. Comprobado en sQLux: con `RAMTOP = 640` no hay sitio (no se precarga, el juego funciona igual) y con 2048 se precargan.
- **Carga en tres fases** (`data.c`): (1) `heroquest_dat` menos los retratos (que son el **último trozo** del fichero: `DAT_RETRATOS` = 7), (2) las misiones (`misiones_precargar`), (3) `data_retratos_cargar`: si caben todos los retratos (49.352 bytes) dejando libre `RESERVA_PARTIDA` (50.000), se leen enteros; si no, solo se guarda **su tabla de desplazamientos** (la cabecera HQGF del trozo, 200 bytes), el retrato del ataque fallido (`RETRATO_FALLO` = 9) y un bufer de un retrato (4.096 bytes). En combate `data_retrato(idx, &g)` da el gráfico de memoria o, si no están todos, lo lee del disco con `fseek` a `desplazamiento del trozo + desplazamiento de la tabla` (el del ataque fallido ya está en memoria). Si ni eso cabe, no hay retratos. Comprobado en sQLux: con `RAMTOP = 640` se leen del disco y con 2048 están todos en memoria; los gráficos salen idénticos. El fichero de datos tiene que tener los retratos al final (`tools/hq_ql_dat.py`).
