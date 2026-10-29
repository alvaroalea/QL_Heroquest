# HeroQuest QL — inventario del port

Qué rutinas del original (ZX Spectrum 128K, `docs/ZX_INVENTARIO.md`) están portadas a `src/` y de qué manera. Lo portado es la parte del **overlay del menú** (banco 6 `0xDB58` → `0x5B00`) más las rutinas de la memoria fija que esas pantallas necesitan (texto, ventanas, fondo de la pantalla de juego y lectura de controles). El resto del juego (bucle de la partida, mapa, combate, sonido…) **no está portado**.

Categorías:

- **Directa**: la rutina del original tiene su equivalente en C con el mismo comportamiento y el mismo flujo (la estructura de los datos y las llamadas pueden cambiar).
- **Inspirada**: cumple el mismo papel, pero con otra implementación o con cambios de comportamiento deliberados (se indica cuáles).
- **Sustituida / innecesaria**: el QL no la necesita (memoria de atributos, overlays, doble buffer…).
- **No portada**: pendiente o descartada.

Resumen de las 29 rutinas del overlay: **18 directas**, **7 inspiradas**, **3 sustituidas o innecesarias** y **1 no portada**.

## Overlay del menú (`ref/zx/heroquest_overlay_menu.skool`)

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0x5B00` | `MENU_PRINCIPAL` | Directa | `menu_principal()` (`menu.c`) | Cuatro opciones; selección en verde, resto en magenta. Se ha quitado la quinta, «Cargar otras aventuras». ESC sale del programa |
| `0x5B3F` | `INICIAR_MISION` | Directa (sin la vista) | `partida_cargar()` (`partida.c`), `partida_iniciar()` (`game.c`) y `mostrar_objetivo()` | Capas, héroes, indicadores, monstruos, objetos, misiones 1, 5 y 7 y mazo: idéntico al original (oráculo en simulador Z80, `docs/ZX_PARTIDA.md`). Falta la tabla `0xC47F` de la vista y la de máscaras `0x6800` (el QL las trae precalculadas) |
| `0x5CE7` | Intercambia dos tablas | Directa | `equipo_ocultar()` (`partida.c`) | Misión 5: los héroes pierden el equipo. En el original no vuelve (cae en el código siguiente): ver `docs/ZX_PARTIDA.md` |
| `0x5D29` | `OPCION_MENU` | Directa | `menu_principal()` | El reparto por opción es el del bucle |
| `0x5D43` | `PANTALLA_CONTROLES` | Inspirada | `menu_controles()` | La opción 1 ya no es Kempston: activa o desactiva los cursores. La 2 (música) solo guarda la marca, todavía sin sonido; la 3 (manuscrito) sí cambia la fuente; la 0 sale. Los textos son `PORT_CONTROLES` (traducibles) en vez del guion `MENU_6BC0` |
| `0x5DC6` | Actualiza los rótulos de controles | Inspirada | `draw_controles()` | Se vuelve a dibujar ON/OFF; el original parcheaba los bytes del guion |
| `0x5DE5` | `CREAR_PERSONAJE` | Directa | `menu_crear()` | Mismas pantallas, colores y teclas |
| `0x5E77` | Menú de un héroe | Directa | `menu_crear()`, `draw_opciones()` | Nombre / cargar / guardar. Se añade ESC para volver |
| `0x5ECE` | Resalta las opciones del menú de héroe | Directa | `draw_opciones()` | |
| `0x5FF4` | `ESCRIBIR_NOMBRE` | Directa | `leer_nombre()` | Rejilla A-Z + FIN, 6 letras como máximo. El original escribía el nombre dentro del guion del héroe; aquí va a `heroes[].nombre`. Se añade ESC para cancelar |
| `0x60C4` | Resalta la letra elegida | Directa | `leer_nombre()` | |
| `0x60E6` | Inicializa un héroe nuevo | Directa | `pers_nuevo()` y `pers_boot()` (`partida.c`), `game_init()` | Cuerpo inicial de la tabla `0x7E4E`, sin oro ni equipo; los nombres por omisión siguen en `game_init()` |
| `0x611E` | Dibuja los nombres de los héroes | Directa | `draw_crear()`, `draw_nombre()` | |
| `0x6141` | `ELEGIR_MISION` | Directa | `menu_jugar()`, `elegir_mision()` | Mensaje «TODOS LOS JUGADORES MUERTOS» si no hay héroes; después hechizos, lista de las 14 misiones y objetivo |
| `0x61A9` | `ELEGIR_HECHIZOS` | Directa | `elegir_hechizos()` | Primero el hechicero (3 grupos si juega solo, 1 si juega el duende) y después el duende; los grupos sin dueño pasan al hechicero. Corrección del port: el hechicero también ve su cabecera («MAGO / ELIGE TUS PRIMEROS HECHIZOS», guion `MENU_683C`; en el original español estaba partida en `683C`/`684C` y con el nombre cortado, en el inglés es el guion `6813`) cuando juega el duende (el original solo la mostraba al duende). Los grupos se guardan en `heroes[].grupos` (bits 0-3), no en los bytes `+0x0B/+0x0C` del original |
| `0x6251` | Elige un grupo de hechizos | Directa | `elegir_grupo()` | El original solo avanzaba con «derecha»; aquí también con abajo/izquierda/arriba |
| `0x627E` | Dibuja los grupos de hechizos | Directa | `draw_grupos()` | |
| `0x62C4` | `TIENDA` | Directa | `menu_tienda()` | Misma lógica de compra con los 13 artículos y sus precios (`art[]` copia de la tabla `0x6603`): compra si alcanza el oro y no se tiene ya |
| `0x63FD` | Selecciona un artículo de la tienda | Directa | `draw_art()` | |
| `0x6446` | Muestra armas y armadura del héroe | Inspirada | `draw_art()` | Siempre se listan los 11 artículos; los que ya se tienen salen en amarillo |
| `0x6477` | Muestra el equipo de un héroe | Inspirada (parcial) | `draw_oro()` | Solo el oro; el resto de la ficha del héroe no está portada |
| `0x64B9` | Limpia el área de equipo | Sustituida | `cls()` | Se borra toda la pantalla |
| `0x64CE` | Espera a soltar los controles | Sustituida | `key_event()` (`keys.c`) | Las pulsaciones se detectan por flanco (solo cuenta lo recién pulsado) |
| `0x64D7` | Resalta la línea del menú | Inspirada | `draw_line()` | Se vuelve a pintar la línea con tinta negra sobre papel de color |
| `0x64DC` | Invierte atributos de una fila | Sustituida | — | El QL no tiene memoria de atributos: cada punto lleva su color |
| `0x64EF` | Muestra un menú | Directa | `cls()`, `draw_title()`, `tx_guion()` | |
| `0x6509` | `GRABAR_CINTA` | Inspirada | `guardar_heroe()` | Fichero de disco `hq_NOMBRE` (16 bytes) en vez de cinta |
| `0x6513` | `CARGAR_CINTA` | Inspirada | `cargar_heroe()` | Idem; se pide el nombre con la rejilla |
| `0x651F` | `CARGAR_AVENTURAS` | No portada | — | Nunca se publicaron más aventuras |

## Rutinas de la memoria fija usadas por los menús

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0x6D81` | `BUCLE_PRINCIPAL` | Inspirada | `main()`, `menu_principal()` | Vuelve al menú tras la partida, como el original |
| `0x6D89` | `PARTIDA` | Directa | `game_run()` (`game.c`) y `juego_partida()` | `game_run` carga la misión y los ficheros y arranca el bucle de `juego.c`; al terminar muestra la ventana provisional `PORT_FIN_MISION` |
| `0x8BF9` | `INTERPRETE_TEXTO` | Inspirada | `tx_ops()` | Interpreta el formato nuevo de los textos (texto con posición y alineación, sin relleno), no los bytes `FF/FB/FC` del original |
| `0x8CBA` | Dibuja un recuadro del guion | Directa | `tx_caja()` | Mismos glifos `0x60`-`0x67` |
| `0x8DD1` | `VENTANA` | Inspirada | `tx_ventana()` | Ancho y centrado calculados en tiempo de ejecución |
| `0x8E6F` | Dibuja un carácter de 8×8 | Inspirada | `gfx_char()`, `gfx_text()` (ensamblador: `asm_text` con `asm_mkpair`) | Color por punto en vez de buffer + atributos |
| `0x9BF2` | `PINTAR_MARCO` | Directa | `game_draw_background()` | Vuelca el marco de 256×192 de `marco_bin` con sus colores |
| `0x9F89` / `0xFD60` | `LEER_CONTROLES` y las rutinas de teclado | Inspirada | `plat_keys()` (`plat_ql.c`), `key_event()` | Mismas teclas (Z X O K y ESPACIO) más los cursores; lectura por IPC del QL (KEYROW) |
| `0x8FDB`, `0x9C3F`, `0x9C4D` | Limpiar pantalla, bitmap y buffer | Sustituida | `gfx_clear()` | Se borra directamente la pantalla del QL |
| `0x8D03` | `VOLCAR_BUFFER` (doble buffer `0xE700`) | Sustituida | — | Los menús dibujan directamente en la pantalla |
| `0x9CA3`, `0x9CCE`, `0x9CD0`, `0x9CD7` | Carga del overlay y paginación de bancos | Sustituida | — | Todo está residente en memoria |
| `0x9BD3`, `0x9BC8` | Dados | Directa | `dado()`, `dado_con_r()`, `dado_combate()` (`partida.c`) | El original usa el registro R del Z80 como semilla; aquí un generador congruencial (el algoritmo del dado es el mismo, comprobado con los 256 valores de R y los dos acarreos) |

## Estado de la partida (memoria fija; `docs/ZX_PARTIDA.md`)

Fichero: `src/partida.c`. Todo comprobado byte a byte con el original ejecutado en un simulador Z80 (`make test`, `tests/run_ql_partida.sh` en sQLux).

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0xA9E5`... | Registro de personaje (26 bytes) y valores iniciales | Directa | `Pers`, `pers_boot()`, `pers_bytes()` | Los campos llevan el desplazamiento del original |
| `0x6C0C` | Lista de monstruos (10 bytes) | Directa | `Mons`, `mons_bytes()` | `+4/+5` es el índice de casilla (el original guarda `0x6200 +` índice) |
| `0x6200`, `0x6400`, `0x6600` | Capas A, B y C del mapa | Directa | `capa_a`, `capa_b`, `capa_c` | 494 bytes cada una (26 × 19) |
| `0xA0D4` | `DIRECCION_CASILLA` | Directa | `casilla_indice()` | `fila * 26 + col` |
| `0x6ECD` | `CALCULAR_DADOS` | Directa | `dados_mov_calcular()` | |
| `0x6F1C` | `TIRAR_DADOS` | Inspirada (parcial) | `dados_mov_tirar()` | Solo la suma; la animación del dado y la espera del botón irán con el bucle del turno |
| `0x6EB5` | `MISION_CONTINUA` | Directa | `mision_continua()` | |
| `0xA254` | `MARCAR_MUERTE` | Directa | `pers_marcar_muerte()` | |
| `0xA271`, `0xA27D`-`0xA29B` | `DESPACHAR_HEROE`: devuelve el equipo de la misión 5 | Directa | `pers_restaurar_equipo()` | |
| `0x9542` | `BARAJAR_MAZO` | Directa (bits sin ROM) | `mazo_barajar()`, `mazo_barajar_azar()` | El original saca los bits de la ROM del Spectrum; con los bits de la ROM da los mismos 32 barajados |
| `0x9523`, `0x9588` | `ROBAR_CARTA`, `DEVOLVER_CARTA` | Directa | `mazo_robar()`, `mazo_devolver()` | |

## Vista isométrica, figuras y plano (memoria fija; `docs/ZX_VISTA.md`)

Fichero: `src/vista.c`. Comprobado punto a punto con el original ejecutado en un simulador Z80 (`make test`, `tests/run_ql_vista.sh` en sQLux). Gráficos: `sprites_bin`, `escenario_bin`, `baldosas_bin`, `marca_activo_bin`; geometría: `vista_bin`.

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0x8923` | Entrar en una sala | Directa | `vista_entrar_sala()` | Origen de la ventana de 6 × 5, color, visita y eventos de las salas de las misiones 1, 5, 9 y 11 |
| `0x8712`, `0x875F` | `CALCULAR_VISIBLES` | Directa | `vista_calcular_visibles()` | Dibuja suelo y paredes directamente en pantalla |
| `0x98F6` | `MARCAR_CASILLA_VISTA` | Directa | `marcar_vista()` | |
| `0x8B64` | Suelo y escaleras | Directa | `suelo()` | Con los rombos `bloqueo` y `trampa` (añadidos al escenario) y el suelo de color `suelo_N` según el paper de la sala |
| `0x8A7F`, `0x8B2C`, `0x8B43` | Paredes y puertas | Directa | `paredes()` | |
| `0x8A40`, `0x8A99` | Puertas de las casillas contiguas | Directa | `puertas_vecinas()`, `puerta_figura()` | Las puertas como figura se dibujan con su máscara del PNG |
| `0x87F4`, `0x8906`, `0x89CD`, `0x8A2D` | Monstruos, cofre, trono, muebles y registros | Directa | `objetos()`, `anim_libre()` | |
| `0x8791`, `0x87CB`, `0x8773` | `ACTUALIZAR_HEROES` | Directa | `vista_actualizar_heroes()` | |
| `0xA2E1`-`0xA3B2` | `CALCULAR_VISTA` | Directa | `vista_calcular_vista()` | |
| `0xA2C1` | `DESCUBRIR_MONSTRUOS` | Directa | `vista_descubrir_monstruos()` | |
| `0xA4FD`, `0xA534` | `ORDENAR_FIGURAS`, `DIBUJAR_FIGURAS` | Directa | `vista_ordenar()`, `vista_figuras_dibujar()` | Sprites con máscara (`gfx_blit_tint`); los héroes al punto y el resto alineado a byte |
| `0xA794`, `0xA8DE` | `GUARDAR_FONDOS`, `RESTAURAR_FONDOS` | Inspirada | `vista_figuras_dibujar()`, `vista_figuras_borrar()` | Sobre la pantalla del QL, no sobre un buffer |
| `0x6E9C` | `REDIBUJAR_TURNO` | Inspirada (parcial) | `vista_construir()` | Sin el panel de marcadores |
| `0x9BE3` | `BORRAR_ANIMACIONES` | Directa | `vista_borrar_animaciones()` | |
| `0x8BD4`, `0x9C77`, `0x9BF2` (vista) | Color de la sala, limpieza y marco | Inspirada | `vista_fondo()` | Marco copiado de `marco_bin`, color de la sala sustituyendo los puntos blancos |
| `0x92A0`, `0x931D`, `0x93EF`, `0x9449`, `0x945C`, `0x94F8` | `DIBUJAR_MAPA`, `DIBUJAR_PLANO` | Directa | `vista_plano_buffer()`, `vista_plano_dibujar()` | El buffer y los atributos del plano salen idénticos; en pantalla cada celda con sus dos colores |
| `0x7710` | `MOSTRAR_MAPA` | Directa | `juego_plano()`, `jv_info_cursor()` (`juego.c`) | El plano con el cursor y el nombre de lo que hay debajo |

## Turno, movimiento y bucle de la partida (memoria fija; `docs/ZX_TURNO.md`)

Ficheros: `src/turno.c` (lógica), `src/juego.c` (bucle, marcadores, plano y turno de Morcar) y la animación en `src/vista.c`. La lógica está comprobada fotograma a fotograma con el original ejecutado en un simulador Z80 (`make test`, `tests/run_ql_turno.sh` en sQLux).

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0x6D89` | `PARTIDA` | Directa | `juego_partida()` | Rondas hasta que acaba la misión (`mision_continua`); ESC aborta |
| `0x6DDD` | `TURNO_PERSONAJE` | Directa (sin eventos) | `turno_jugar()`, `turno_empezar()`, `turno_estado()` | Dormido y tormenta con sus ventanas; falta `EVENTO_ALEATORIO` (`0xA108`) |
| `0x6E9C` | `REDIBUJAR_TURNO` | Directa | `vista_construir()`, `juego_panel()` | |
| `0x6ECD`, `0x6F1C` | `CALCULAR_DADOS`, `TIRAR_DADOS` | Directa | `dados_mov_calcular()`, `turno_dado_fotograma()`, `tirar_dados()` | El héroe parpadea y la suma cambia en cada fotograma hasta pulsar el botón |
| `0x6FDA` | `MOVER_AUTOMATICO` | Directa | `turno_automatico()` | |
| `0x6FF6` | `IR_HACIA_CURSOR` | Directa | `ir_hacia_cursor()`, `vista_cursor_a_casilla()` | La división del original es de 16 bits sin signo |
| `0x7201` | `PROCESAR_CLIC` | Inspirada | `turno_clic()` | El clic cuenta al pulsar (no espera a soltar); zonas de `0xB4F0` en `vista_bin` |
| `0x725E`, `0x7277` | `PASAR_TURNO`, `TERMINAR_TURNO` | Directa | `turno_pasar()`, `turno_terminar()` | |
| `0x7281` | `EN_ZONA_ESPECIAL` | Directa | `zona_especial()` | Misión 4 |
| `0x82C6`-`0x8478` | Cancelar y los cuatro pasos | Directa | `turno_paso()`, `cancelar()` | Un solo código para las cuatro direcciones |
| `0x850F` | `FIN_PASO` | Directa | `fin_paso()` | Incluye el teletransporte de la misión 9 (dos dados) |
| `0x85D9`, `0x85F2`, `0xA7C9` | Empezar el paso, `ANIMAR_FIGURA`, animación | Directa | `vista_iniciar_paso()`, `vista_animar_clave()`, `vista_animar()` | Guiones de `0xC1B6` en `vista_bin` |
| `0xA0E6`-`0xA105` | `OBJETO_EN_CASILLA` | Directa | `objeto_en_casilla()` | Deja el evento pendiente (`Vars.evento`); su tratamiento (`0xA108`) no está portado |
| `0x8E7B` | Salida por la escalera | Inspirada (parcial) | `salida()`, `ventana_salida()` | Solo la parte común y la ventana; lo de cada misión (`0x8FE9`) es el gancho `juego_salida_mision` |
| `0xA23E` | `FIN_MOVIMIENTO` | Directa | `turno_fin_movimiento()` | |
| `0x9CE0` | `FIN_DE_RONDA` | Inspirada (sin monstruos) | `turno_morcar()`, `turno_fin_de_ronda()` | Plano, mensajes y esperas; gancho `juego_turno_monstruos` |
| `0x91F7`, `0x9242`, `0x926A`, `0x9C5B` | Marcadores y nombre | Inspirada | `juego_panel()` | Sobre fondo negro |
| `0x7710`, `0x7733`, `0x768F` | `MOSTRAR_MAPA`, `INFO_BAJO_CURSOR` | Directa | `juego_plano()`, `nombre_bajo_cursor()` | Se borran las 9 celdas del nombre antes de escribirlo |
| `0x8009` | `FIN_ACCION` | Inspirada | `juego_redibujar()` | |
| `0x7877`, `0x7319` | Bolsa y atacar | Directas | `ui_bolsa()`, `ui_atacar()` (`juego_combate.c`) | Ver «Combate, bolsa y hechizos» |
| `0x70A1` | Eventos de las misiones 5, 9 y 11 | Directa | `mision_evento()` | Ver «Lo propio de cada misión» |

## Acciones de los iconos (memoria fija; `docs/ZX_ACCIONES.md`)

Ficheros: `src/accion.c` (lógica) y `ui_llave()`, `ui_ojo()` y las ventanas en `src/juego.c`. Comprobado con el original en el simulador Z80 (`make test`, `tests/run_ql_accion.sh`).

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0x727E`, `0x72B5` | `ABRIR_PUERTA` (llave) | Directa | `accion_abrir_puerta()`, `ui_llave()` | |
| `0x860B`, `0x8662`, `0x98FB` | `ABRIR_PUERTA_CASILLA` y el recorrido de la sala | Directa | `revelar()`, `recorrer()` | |
| `0x804E`, `0x8039` | `OJO` | Directa | `ui_ojo()` | Ventana con zonas de los textos (`tx_zonas`) |
| `0x8079`, `0x8647`, `0x8688`, `0x86E3`-`0x86F1` | `BUSCAR_TRAMPAS_PUERTAS` | Directa | `accion_buscar_trampas()`, `buscar_celda()` | |
| `0x80A9`, `0x862A`, `0x86F7` | `BUSCAR_TESOROS` | Directa | `accion_buscar_tesoros()`, `ver_monstruo()` | Mazo, oro, pociones, trampas, fantasma y emboscada |
| `0x8221`, `0x8225` | Sumar oro | Directa | en `accion_buscar_tesoros()` | |
| `0x9F94`, `0x9FAF` | `CASILLA_LIBRE` | Directa (`0x9FAF`) | `casilla_libre()`, `casilla_ocupada()` | `0x9F94` irá con los monstruos |
| `0xA108` | `EVENTO_ALEATORIO` | Directa (con `0x70A1` aparte) | `accion_evento()` | Fosos, lanzas, techo y las rocas de las misiones 4 y 11 |
| `0x7B31` | `MATAR_PERSONAJE` (héroes) | Directa | `heroe_muere()`, `MSG_MUERTE` | |
| `0xAA8C` (Hero Quest), `0xA76D` (ampliación) y las rutinas de recompensa `0x7602`-`0x7647` | Estadísticas de monstruo: movimiento, dados, recompensa, jefe | Sustituida: datos de la aventura | `[estadisticas]` de `<aventura>.<idioma>.txt`, `partida_estadisticas()`, `partida_estadistica()` | Ver `docs/ZX_DATOS_QL.md` |
| `0x764A`, `0x9ACA`, `0xC3B2` (tablas de nombre, retrato e icono por tipo de monstruo) | Tipos de monstruo | Sustituida: datos de la aventura | `[monstruos]` de `<aventura>.<idioma>.txt`, `partida_tipos()`, `partida_tipo()` | Ver `docs/ZX_DATOS_QL.md` |
| `0x97E0`, `0x9813`, `0x9831`, `0x6D87`-`0x6DA7` (ampliación) | Rutinas de muerte por estadística: normal, abrir la salida y epílogo | Directa (sin oráculo), con las casillas como datos de la misión | `monstruo_muere()`, `muerte_propia()`, `fin_mision()`, `data_epilogo()` | Ver `docs/ZX_AMPLIACION.md` |
| `0x732A`, `0x9A44`-`0x9A91`, `0x9C9C`-`0x9D4F` (ampliación) | Arma rota (bit 6) y niebla de la muerte (bit 7) | Directa (sin oráculo) | `combate_arma_rota()`, `monstruo_errante_paso()`, `monstruo_errante_golpe()`, `ui_errante()` | Ver `docs/ZX_AMPLIACION.md` |
| `0x827A`, `0x82A2` | Salas especiales de las misiones | Directa | `mision_especial()` (gancho `accion_especial`) | Ver «Lo propio de cada misión» |
| `0x8D73` (ampliación), `0x8D89`-`0x8E94` | Las 11 salas especiales de Return of the Witch Lord | Directa (sin oráculo: solo el original de Hero Quest está en el simulador) | `especial_wl()` en `mision_especial()`, indicadores 19 a 29 | Ver `docs/ZX_AMPLIACION.md` |
| `0x7B1C`, `0x7B2B` | Cuenta de calaveras y muerte con repintado | Directas | `roca_sobre_casilla()`, `mostrar_salida()` | |

## Cursor (memoria fija, bucle de la partida)

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0x713E` | `MOVER_CURSOR` | Directa | `cursor_move()` (`cursor.c`) | Mismos límites (X `0x10`-`0xFC`, Y `0x0A`-`0xAE`), pasos de 4 y 2 puntos y aceleración (doble paso tras 6 fotogramas con la tecla pulsada, contadores independientes en horizontal y vertical). No mueve si hay un evento pendiente: esa comprobación llegará con el bucle de la partida |
| `0x7168`, `0x7199`, `0x71C9`, `0x71F5` | Cursor derecha / izquierda / arriba / abajo | Directa | `paso_x()`, `paso_y()` | |
| `0x7695` | `CURSOR_A_CASILLA` | No necesaria | - | El juego calcula la casilla donde la usa; la fórmula era columna `(X-12)/8`, fila `(Y-2)/8` |
| `0xA57E` | `DIBUJAR_CURSOR` | Inspirada | `cursor_draw()`, `gfx_blit_clip()` | El original dibuja en el buffer de la vista una de dos copias del sprite (desplazada 4 puntos) según `(X - Y/2) & 4`; en el QL el sprite va con máscara directamente a pantalla, ya alineado a 4 puntos, y se recorta en el borde derecho |
| `0xA7B6` / `0xA8DE` | Guardar y restaurar el fondo bajo el cursor | Inspirada | `cursor_draw()` / `cursor_erase()` (`gfx_rect_get/put`) | Se guarda el rectángulo de pantalla bajo el cursor (no el buffer de la vista). Como se dibuja directamente en pantalla (el original componía en un buffer y lo volcaba entero), el cursor solo se borra y se redibuja cuando se mueve, para que no parpadee |
| `0x7201` | `PROCESAR_CLIC` | Inspirada | `turno_clic()` (`turno.c`) | Ver el apartado del turno |

## Reproductor de sonido (banco 4)

Fichero: `src/music.c` (traducción a C del Z80, respetando el orden de las operaciones). Verificado: **los 14 registros del AY coinciden en todos los fotogramas de las 4 melodías** con los volcados `.psg` del original (`tests/run_mus.sh`, incluye el bucle de cada una).

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0xD388` | `REPRODUCTOR` | Directa (`mus_fade`, `mus_stop` no compiladas: NO_COMP_UNUSED) | `mus_start()`, `mus_fade()`, `mus_stop()`, `mus_frame()` | Los comandos 0 y 1 son los dos pasos de `mus_frame()` |
| `0xD3AA` | `EMPEZAR_MELODIA` | Directa | `mus_start()` | |
| `0xD47E`, `0xD486` | Pone a cero 7 bytes; bajada de volumen | Directa | `mus_start()` / `mus_frame()` | |
| `0xD496` | `ACTUALIZAR_MUSICA` | Directa | `mus_frame()` | |
| `0xD4E6`, `0xD507` | Música parada | Directa (sin efectos) | `mus_frame()` | |
| `0xD513` | `ACTUALIZAR_ENVOLVENTES` | Directa | `mus_frame()` | |
| `0xD52D`, `0xD5BF` | `ESCRIBIR_AY`, tono y volumen de un canal | Directa (sin efectos) | `escribir()`, `salida()` | |
| `0xD624` | Desplazamiento de tono (deslizamiento y retardo de vibrato) | Directa | `desplazar()` | |
| `0xD689` | Envolventes: secuencias de tono y ruido, vibrato, lista de volumen | Directa | `envolvente()` | |
| `0xD7BA`, `0xD7C5` | Cuenta atrás de un canal y lectura del patrón | Directa | `cuenta()`, `leer()` | El código automodificable del Z80 (`0xD8E0`, `0xD99E`) pasa a variables locales |
| `0xD8E0`, `0xD99E`, `0xDB11` | Deslizamiento de tono, instrumento, nota | Directa | `deslizar()`, `instrumento()`, `nota()` | Con la tabla de periodos convertida a 750 kHz y el vibrato escalado (`27/64`) |
| `0xDBCD`, `0xDBEE` | División y multiplicación de 16 bits | Directa | `div16()`, `mul16()` | |
| `0xD7AD` | Escribe un registro del AY | Inspirada | `qs_write()` (`qsound.c`) | Por la PIA de la QSound |
| `0xDC08`, `0xDCC8`, `0xDD28` | Tablas de periodos, de ruido y de vibrato | Datos | `data/musica_bin` | Periodos × 750000/1773400; el original lee más allá de las tablas (índices 96-255) y suena: se copian los bytes que siguen en su memoria |
| `0xDF58`-`0xE2xx`, `0xDE9E` | Efectos de sonido (canal 4) | Inspirada | `mus_efecto()` | Reproducción de los volcados del original (`docs/ZX_SONIDO.md`) |
| `0x6F99` | Interrupción de 50 Hz | Inspirada | `plat_present()` → `mus_tick()` | La música avanza un fotograma cada vez que se espera el 50 Hz (menús y bucle de la partida) |

Uso en el juego: melodía 2 en el menú (desde el arranque y al volver de la partida) y melodía 1 al empezar la partida (`game_run`). «2.MUSICA» de Controles la activa o desactiva (solo con QSound).

## Datos portados

| Datos del original | En el QL |
|---|---|
| Tabla de artículos de la tienda `0x6603` | `art[]` en `menu.c`, tal cual |
| Textos de los menús (scripts del overlay) | `textos_es` / `textos_en` (`MENU_*`), con identificadores `TX_*` |
| Marco de la pantalla de juego (banco 6 `0xC000`) | `data/marco_bin` |
| Fuentes del banco 7 y baldosas `0xB1C8` | `data/fuente_normal_bin`, `fuente_manuscrita_bin`, `baldosas_bin` |
| Misiones (bancos 1 y 3) | `data/heroquest_NN_es`, `heroquest_NN_en` y `heroquest_es_idx`, `heroquest_en_idx` |
| Sprites, escenario y retratos | `data/sprites_bin`, `escenario_bin`, `retratos_bin` (empaquetados, aún sin usar en el código) |

## No portado

Todo el código residente de la partida del original (`0x6DDD` a `0xA978` salvo lo indicado arriba): turnos y animación del dado, clic y zonas de la pantalla, animación y movimiento de los héroes, puertas y búsqueda, combate y retratos, movimiento de los monstruos, eventos de las misiones, panel de marcadores y ventanas de la partida. Véase `docs/ZX_INVENTARIO.md` para la lista completa.

## Combate, bolsa y hechizos (memoria fija; `docs/ZX_COMBATE.md`)

Ficheros: `src/combate.c` (lógica) y `src/juego_combate.c` (pantallas). Comprobado con el original en el simulador Z80 (`make test`, `tests/run_ql_combate.sh`).

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0x7319`, `0x7355`-`0x7381`, `0x76AE`, `0x7436` | `ATACAR`, elegir objetivo, otro ataque | Directa | `ui_atacar()`, `seleccionar()`, `si_no()` | Sin sonidos |
| `0x7462`, `0x74FC`, `0x74E5`, `0x750B` | Alcance | Directa | `combate_alcance()`, `arma_se_pierde()` | |
| `0x7E74`, `0x7E7E`, `0x7F2A` | Línea de vista | Directa | `combate_linea()` | |
| `0x751D`, `0x7559`, `0x7596`, `0x75ED`, `0x9B65` | Datos y bonos | Directa | `combate_datos_monstruo()`, `combate_bonos_ataque()`, `combate_dados_defensa()`, `combate_preparar()` | |
| `0x99A2`, `0x996B`, `0x9B04`, `0x9B25`, `0x7B1C` | Tiradas y daño | Directa | `combate_ataque()`, `combate_defensa()`, `combate_cuenta_calaveras()` | |
| `0x9B31`, `0x7602`-`0x7647` | Muerte de un monstruo y recompensa | Directa | `monstruo_muere()` | |
| `0x9921`, `0x99DB`-`0x9A7B`, `0xA3E1`, `0x764A` | `PANEL_COMBATE`, dados y retratos | Inspirada | `ui_defensa()`, `pantalla_combate()` | Figuras en color sobre fondo blanco |
| `0x95BE`, `0x9656`-`0x9738` | `PANEL_PERSONAJE` y listas | Directa | `panel_dibujar()`, `lista()` | |
| `0x7BFC`, `0x7C45`-`0x7E61`, `0x7F6F`, `0x7F8D`, `0x96AC`, `0x96EC`, `0x9FF7` | Zonas y manejadores del panel | Directa | `panel()`, `panel_clic()`, `panel_*()` | |
| `0x7877`, `0x78B4`, `0x7943`-`0x7B15`, `0x7925` | Bolsa y hechizos | Directa | `ui_bolsa()`, `ui_hechizo()`, `hechizo_lanzar()`, `hechizo_inmune()` | Esc cancela la elección del blanco |
| `0x7BA2`, `0x7BA4`, `0x7B9E` | Efectos de sonido | Inspirada | `mus_efecto()` (`music.c`) | Reproduce los volcados del AY del original (`docs/ZX_SONIDO.md`) |

## Turno de los monstruos (memoria fija; `docs/ZX_MONSTRUOS.md`)

Ficheros: `src/monstruo.c` (lógica) y `ui_turno_monstruos()` en `src/juego_combate.c`. Comprobado con el original en el simulador Z80 (`make test`, `tests/run_ql_monstruo.sh`).

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0x9CE0`-`0x9D04` | Bucle de monstruos de `FIN_DE_RONDA` | Directa | `ui_turno_monstruos()`, `turno_morcar()` | |
| `0x9D6B` | `TURNO_MONSTRUO` | Directa | `monstruo_planear()` | |
| `0x9DC0`, `0x9E6A`-`0x9F7C` | `CALCULAR_CAMINO` | Directa | `camino()` | |
| `0x9F94`, `0x9FAF` | `CASILLA_LIBRE` | Directa | `cuerpo_ocupada()`, `casilla_libre()` | |
| `0xA075` | `EJECUTAR_CAMINO` | Directa | `monstruo_paso()` | El plano se redibuja entero en cada paso |
| `0x990D` | Ataque del monstruo | Directa | `ui_defensa()` | |

## Lo propio de cada misión (memoria fija; `docs/ZX_MISIONES.md`)

Ficheros: `src/mision.c` (lógica), `ventana_salida()` y el bucle de eventos en `src/juego.c`, ganchos en `src/juego_combate.c`. Comprobado con el original en el simulador Z80 (`make test`, `tests/run_ql_mision.sh`).

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0x70A1` | `EVENTOS_MISION` (misiones 5, 9 y 11) | Directa | `mision_evento()` | La pregunta de las 5000 monedas con `ui_pregunta()` |
| `0x827A`, `0x82A2`, `0x9005`-`0x91F4` | Salas especiales (indicadores 1 a 18) | Directa | `mision_especial()` | |
| `0x8E7B` (tras la marca de salida), `0x8FE9`, `0x8EBB`-`0x8FD2` | Salida por la escalera según la misión | Directa | `mision_salida()`, `ventana_salida()` | La salida con «ir a por más oro» (misión 3) sigue el turno |
| `0x8D5E` | Ventana de sí o no | Directa | gancho `mision_pregunta`, `ui_pregunta()` | |
| `0x8195`, `0x81B1` | Ataque de una emboscada | Directa | gancho `accion_ataque`, `ui_ataque()` | Antes solo se mostraba la ventana |
| `0x6D81`-`0x6D87` | Vuelta al menú tras la misión | Inspirada | `game_run()` | Oro, equipo y héroes que han salido vivos vuelven a `heroes[]` |

## Efectos de sonido (`docs/ZX_SONIDO.md`)

| Dirección | Rutina del original | Estado | En el QL | Notas |
|---|---|---|---|---|
| `0x7BA4` `EFECTO_SONIDO`, `0xDE9E`, `0xDF58`-`0xE267` | Lanzar un efecto y el canal de efectos del reproductor | Inspirada | `mus_efecto()`, `fx_frame()` | Se reproducen los volcados `.psg` del original (`data/efectos_bin`), solo con la música desactivada |
| `0x727D`, `0x6E7A` | Efecto pendiente de paso y puerta | Directa | `Vars.sonido_pend` y el bucle de `turno_jugar()` | |
| `0x6F5B`, `0x7274`, `0x7348`, `0x7383`, `0x7710`, `0x788D`, `0x7945`, `0x7C65`-`0x7E6A`, `0x8063`-`0x80A9`, `0x81D5`, `0x8E7D`, `0x901B`, `0x9AA3`, `0xA126`, `0xA1FC` | Los lugares donde suena | Directa | llamadas a `mus_efecto()` en `juego.c` y `juego_combate.c` | |

## Rutinas auxiliares que no tienen función propia

| Dirección | Rutina del original | Estado | En el QL |
|---|---|---|---|
| `0x6580` | `CARGADOR_BANCOS` | Sustituida | no hay bancos: los datos se leen de `data/` |
| `0x6F13` | `DIBUJAR_PANEL` | Directa | `juego_panel()` |
| `0x7869`, `0x7B62` | dirección de un monstruo / personaje | Sustituida | `p->mons[]`, `p->pers[]` |
| `0x7BC5`, `0x7BCE` | comandos y llamada al reproductor | Sustituida | `mus_start()`, `mus_efecto()` |
| `0x82CB`, `0x8357`, `0x83E3`, `0x8478` | `MOVER_X/Y_MENOS/MAS` | Directa | `turno_paso()` |
| `0x8DB2`, `0xA3C5` | repintar la escena / la vista | Directa | `juego_redibujar()` |
| `0x95AD`, `0x95B7`, `0x9703` | modos del intérprete de texto y listas de bits | Directa | `lista()` y `rotulo()` (`juego_combate.c`) |
| `0x9D5E`, `0x9D66` | mostrar y esperar | Directa | `jv_boton()`, `jv_esperar()` |
| `0x9F80` | leer una fila del teclado | Sustituida | `plat_keys()` |
| `0xA3F8` | retrato invertido | Inspirada | `minifigura()` |

**Quedan sin portar**, por decisión: `CARGAR_AVENTURAS` (`0x651F`, nunca hubo más aventuras).
