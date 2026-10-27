# Decisiones de diseño del port

Rutinas «no necesarias», decisiones de diseño y mejoras propias retiradas (y cómo recuperarlas).

## Pantalla: 256×192 del Spectrum sobre 256×256 del QL (decidido con el usuario)

- El original usa 256×192 píxeles (24 filas de 8) y el QL en modo 8 tiene 256×256. Se mantiene el tamaño del original 1:1 y se **dejan 32 píxeles vacíos y negros arriba y 32 abajo** (la imagen del Spectrum ocupa las líneas 32 a 223 del QL; en bytes de pantalla del QL, 128 bytes por línea, empieza en `0x20000 + 32*128`).
- Ventaja: ninguna rutina de dibujo necesita escalar; el ancho coincide (256 píxeles) y las coordenadas del original solo se desplazan 32 líneas en Y.
- Pendiente de decidir más adelante: cómo traducir los atributos del Spectrum (color por celda de 8×8, 2 colores) a los 8 colores por píxel del modo 8. Ver `docs/ZX_ANALISIS.md` (el juego compone la vista en un buffer monocromo en `0xE700` y pone color aparte en `0x5800`).

## Sprites y escenarios en el QL: color y máscara almacenados (decidido con el usuario)

- En el Spectrum los sprites son de 1 bit por píxel (tinta), sin color propio (el color lo da la celda de atributos de 8×8) y sin máscara guardada: el juego deriva el halo con la tabla de `0x6800` (`mascara = ~(b | b<<1 | b<<2 | b>>1 | b>>2)`) y dibuja `buffer = (buffer AND mascara) OR sprite`. Ver `docs/ZX_ANALISIS.md`, apartado 8.
- En el QL **se almacenarán el color y la máscara de todos los sprites y de los gráficos de escenario** (paredes, puertas, escaleras, suelo, muebles, retratos), en el formato del modo 8 (4 píxeles por palabra), en vez de derivarlos en tiempo de ejecución: la conversión se hace una vez, fuera de línea, y el dibujado en el QL es una copia con máscara.
- Consecuencias para las herramientas de extracción (`tools/hq_gfx_extraer.py` y `hq_ql_empaquetar.py`): hay que generar, por cada gráfico, (1) el bitmap de tinta, (2) la máscara (la del halo calculada con la regla anterior, o la guardada en el caso del cursor) y (3) los colores. Los colores del Spectrum son por celda de 8×8 (tinta y papel), así que habrá que decidir qué colores se asignan a cada sprite y a cada escenario (lo hace el usuario retocando los PNG). Flujo implementado: `docs/ZX_GRAFICOS_QL.md` (PNG de 8 colores con transparencia en `asset/graficos_ql/`, máscara deducida del canal alfa, empaquetado a `data/` (`sprites_bin`, etc.)).

## Datos del port a QL (decidido con el usuario)

- **Nombres de fichero del QL sin punto**: `sprites.bin` → `sprites_bin`. Todos los binarios del QL van a `data/`; los índices y constantes solo para el PC, a `build/datos/`.
- **Textos** (`data/textos_bin`) y **misiones** (`data/mision_NN_es_bin` y `mision_NN_en_bin` + `data/misiones_indice_es_bin` y `misiones_indice_en_bin`) en ficheros separados del juego, para poder **traducirlos y personalizarlos sin modificar el juego**: textos con longitud variable (sin relleno; el juego recalcula anchos y centrados), un fichero por misión y un índice. Ver `docs/ZX_DATOS_QL.md`.
- Las versiones retocadas o traducidas viven en `asset/graficos_ql/`, `asset/textos_ql/` y `asset/misiones_ql/`, que ninguna herramienta de extracción sobrescribe.

## Vista isométrica y plano (paso 2)

- **Geometría en `data/vista_bin`**: las tablas que no son gráficos (posiciones de las 30 casillas, salas con su ventana y color, desplazamientos de puertas y muebles, baldosas del plano, tiras de color y de limpieza) se extraen del original con `tools/hq_vista_empaquetar.py` y se leen al empezar la partida; el juego no lleva ninguna dirección del Spectrum. Cada gráfico se identifica por su índice en `sprites_bin` y `escenario_bin`.
- **Color de la sala**: la vista es siempre blanca (marco original, sin recolorear las 17 tiras de `0xC528`; las 22 de `0xA4BB` se rellenan en blanco). El paper del byte de atributo de cada sala (`0xC47C`, bits 3-5) elige el gráfico del suelo: `suelo_0` a `suelo_7` (`asset/graficos_ql/escenario/`, índices 51 a 58 de `escenario_bin`, a color y con máscara). Se retiraron `gfx_recolor` y `gfx_blit_tint` (el color de la sala ya no se aplica por recoloreo). Los códigos de objeto `0x3C` a `0x3F` (suelo normal en `0xC342`) usan también el suelo de la sala. El `suelo` del original (índice 10) sigue empaquetado pero no se dibuja.
- **Figuras**: sprites con máscara sobre la pantalla, con el fondo guardado y devuelto como el cursor (en vez de componer en un buffer y volcarlo). Las posiciones son siempre múltiplos de 4 puntos, así que no hacen falta las copias desplazadas (`--desplazados`) mientras los héroes estén quietos; cuando se porte el movimiento paso a paso habrá que revisarlo (el original los dibuja al punto).
- **Halo de las figuras (retirado)**: ya no se calcula ni se pinta ningún halo (ni el de las puertas como figura al cargar, `hacer_halo`, ni el recoloreo del halo de los sprites): los PNG de `asset/graficos_ql/` llevan su color y su máscara por la transparencia.
- **Plano**: el buffer de 1 bit del original (y sus atributos) se calcula igual que en el original y se pinta celda a celda con los colores de los atributos; así se puede comparar byte a byte.
- **Tiempo**: una sala completa tarda ahora unos 0,7 s en un QL a 7,5 MHz (mayormente C en vez de ensamblador: rellenos de celdas, recortes y mezcla de halos); es un candidato claro para la fase de profiling.

## Turno y movimiento (paso 3)

- **Lógica separada del dibujo**: `turno.c` no dibuja (salvo lo que ya hace `vista.c`); `juego.c` dibuja, espera y lee teclas. Así la lógica se comprueba contra el original fotograma a fotograma.
- **Clic por flanco**: el original, al detectar el botón, espera a que se suelte antes de procesarlo (bloquea el bucle). Aquí el clic cuenta al pulsar. Cambia lo que ocurre si se mantiene pulsado mientras anda (en el original, cuando acaba el camino un botón aún pulsado se vuelve a procesar).
- **Redibujado por zonas**: cada fotograma solo se borran y dibujan las figuras (y el cursor) si algo se ha movido; la sala entera se redibuja al cambiar de sala o tras una ventana.
- **Marcadores**: se escriben con `gfx_char` sobre fondo negro; las constantes de posiciones y colores son las de los guiones `0x9236`, `0x9285` y `0x928F` del original.
- **Ganchos** para lo que falta (`juego_accion[]`, `juego_turno_monstruos`, `juego_salida_mision`, `turno_evento_salida`): los siguientes pasos solo tienen que rellenarlos.
- **Mapa y cursor**: el cursor se comparte entre la sala y el plano; al cambiar de pantalla se borra y se vuelve a dibujar.

## Acciones de los iconos (paso 4)

- **Lógica con resultado, interfaz aparte**: las acciones no dibujan; devuelven una lista de mensajes (`Salida`, `Msg`) con lo que el original habría mostrado (ventanas y guiones). `juego.c` los muestra con `tx_ventana` o `tx_ventana_con` (ventanas con datos que cambian: oro y poción).
- **Pruebas con dados guionizados**: `dado_guion()` hace que las tiradas valgan una lista; el oráculo hace lo mismo con la rutina `0x9BD3`, así se comparan los resultados con sucesos al azar.
- **Esperas saltables**: el botón corta las esperas de las ventanas (el original no deja).
- **Ganchos**: `accion_especial` (salas especiales de las misiones) y `juego_accion[ACC_BOLSA/ACC_ATACAR]`.
- **Tablas del original** (oro de las cartas, pociones, casillas de la emboscada, nombres de objeto) en `vista_bin`.

## Combate y bolsa (paso 5)

- **Lógica y pantallas separadas**: `combate.c` no dibuja ni espera (devuelve `Salida`, como `accion.c`); `juego_combate.c` tiene el plano con el cursor, la pantalla de combate, el panel del personaje y la elección del blanco. Así toda la lógica se comprueba contra el original.
- **`retratos_bin` se carga al empezar la partida** (`ui_combate_iniciar()`, 41 KB; se libera al salir).
- **Figuras de la pantalla de combate**: en color sobre una caja blanca en vez de en negativo.
- **Panel del personaje**: se redibuja entero tras cada clic (el original solo redibuja la lista afectada).
- **Esc** cancela la elección del blanco de un hechizo y cierra el panel y la ventana de sí/no.
- **Rótulos nuevos** (`CUERPO`, `MENTE`) en `textos_port.es.txt`/`textos_port.en.txt` (`PORT_COMBATE`) (el original los lleva fijos en el guion `0xB161`). Se copia a `asset/textos_ql/` solo si falta.
- **`ui_defensa(heroe, monstruo)`** (la defensa con su pantalla) queda lista para el turno de los monstruos del paso 6.

## Misiones (paso 7)

- **Preguntas por gancho**: la lógica de `mision.c` pregunta con `mision_pregunta()` (sí o no) y el juego la responde con `ui_pregunta()`; así las pruebas con el original pueden dar las respuestas.
- **Sin pantalla de fin de misión**: se quitó la ventana provisional `PORT_FIN_MISION` (queda el texto en `textos_port.txt`, sin usar); se vuelve al menú como el original y el menú ya avisa si no queda ningún héroe.
- **Ataque de la emboscada** por el gancho `accion_ataque`: en las pruebas de `accion_test` no está puesto (el oráculo de ese paso lo interceptaba); lo comprueba `mision_test` (búsquedas del ojo completas).
- **Eventos de misión en cada fotograma**: como en el original (`EVENTO_ALEATORIO` llama a `0x70A1`), con una comprobación rápida de `Vars.evento_mision`.

## Efectos de sonido (paso 8)

- **Reproducción de volcados en vez de portar el canal de efectos**: ver `docs/ZX_SONIDO.md`. Equivale al original (se comprueba fotograma a fotograma con los `.psg`), ocupa 1,2 KB y no añade código de motor. Si algún día hiciera falta portar el canal de efectos (para mezclarlo con la música, que el original no hace), el código está en `0xDFC8`-`0xE267`.
- **El ruido de los efectos sí se convierte a 750 kHz**; el de la música no (queda como estaba): revisar si la música suena más grave de lo debido en los golpes de ruido.
- **`Vars.sonido_pend`** es la variable `0x727D`: el paso y la puerta dejan el efecto pendiente y el bucle del turno lo toca.
