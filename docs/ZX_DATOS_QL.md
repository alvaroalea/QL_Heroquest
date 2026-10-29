# Textos y misiones para el port a QL

Igual que los gráficos (`docs/ZX_GRAFICOS_QL.md`), los textos y las misiones se extraen del original a ficheros de texto **editables**, se copian a una carpeta propia del usuario que ninguna herramienta de extracción sobrescribe y se empaquetan en binarios para el QL. El juego del QL leerá los binarios, de modo que **se pueden traducir y personalizar sin modificar el juego**.

**Nombres de fichero del QL: sin puntos.** Todos los ficheros de `data/` terminan en `_bin` (`sprites_bin`, `heroquest_es_idx`, `heroquest_03`...). Lo que solo sirve al PC (índices, constantes) va a `build/datos/`.

**`fonts_dat` y `heroquest_dat`.** `data/fonts_dat` (mismo formato, `FNT_*` de `src/data.h`, `fonts_get()`) junta `fuente_normal_bin`, `fuente_manuscrita_bin` y `marca_activo_bin`.

**`heroquest_dat`.** En el disco, `escenario_bin`, `marco_bin`, `sprites_bin`, `baldosas_bin`, `vista_bin`, `musica_bin`, `efectos_bin` y, **el último**, `retratos_bin` van juntos en un único fichero `data/heroquest_dat` (`tools/hq_ql_dat.py`, última línea de `tools/hq_ql_datos.sh`). Formato (big endian): `'HQDT'`, `u16 n`, `u16 0`, `n × {u32 desplazamiento, u32 tamaño}` y los datos alineados a 4, en el orden de `DAT_*` (`src/data.h`). `data_init()` lo lee una vez **menos el último trozo (los retratos)** y `dat_get(DAT_xxx, &tam)` devuelve un puntero a cada parte; no se libera. Los retratos se cargan después de las misiones, si caben (`data_retratos_cargar`, `data_retrato`; ver `docs/PORT_QL.md`). Los ficheros sueltos se siguen generando en `data/` (los usan las pruebas de `tests/`) pero `make img` ya no los copia al disco. `textos_bin`, `heroquest_scr` y las misiones siguen sueltos.

```
asset/textos/textos.es.txt|.en.txt  <- original extraído (se regenera)     asset/textos_ql/textos.es.txt|.en.txt  <- tuyo (nunca se sobrescribe)
asset/misiones/misionNN.es.txt|.en.txt <- originales extraídos (se regeneran)  asset/misiones_ql/misionNN.es.txt|.en.txt <- tuyos
data/<aventura>_NN_es y _en, data/<aventura>_es_idx y _en_idx (índice de misiones, subtítulo y, en heroquest, textos del idioma)   <- binarios del QL
build/datos/textos_ids.h, indice_textos.txt, indice_misiones_es.txt e indice_misiones_en.txt     <- índices para el PC
```

Todo el flujo (también gráficos): `tools/hq_ql_datos.sh extraer` (extrae los originales, copia a `*_ql` lo que falte y empaqueta) y, después de editar, `tools/hq_ql_datos.sh`. Cada paso se puede lanzar por separado: `hq_textos_extraer.py`, `hq_misiones_extraer.py`, `hq_ql_inicializar.py` (copia solo lo que falte; `--estado` lista lo pendiente), `hq_textos_empaquetar.py`, `hq_misiones_empaquetar.py`. El empaquetado usa el fichero de `*_ql` si existe y, si no, el original; indica cuántos están retocados. `hq_textos_verificar.py` y `hq_misiones_verificar.py` comparan los binarios con la memoria del juego original (solo con los originales sin tocar).

## Alfabeto

El juego dibuja el carácter de código `c` con el glifo `c − 0x30` de la fuente (8 bytes por glifo, `fuente_normal_bin`), por lo que solo se pueden escribir los glifos que tiene la fuente. Cada fuente tiene `FONT_GLYPHS` = 85 glifos (680 bytes): los 68 del original (`0x30`-`0x73`) y **17 libres, `0x74`-`0x84`**, para las vocales acentuadas, la ñ, etc. Para usarlos, amplía `asset/graficos_ql/fuentes/fuente_normal.png` (y `fuente_manuscrita.png`) de 17×4 a 17×5 celdas de 8×8 (la fila nueva son los códigos `0x74`-`0x84`) y escribe el código con `{74}`...; el empaquetador toma todas las filas del PNG y rellena con blanco hasta 85 glifos. En los ficheros de texto se escribe así:

| Escribes | Código | Glifo |
|---|---|---|
| `A`-`Z`, `0`-`9` | el ASCII | letras y cifras (no hay minúsculas, ñ ni acentos) |
| espacio | `0x5F` | en blanco |
| `.` `,` `'` `!` `=` `?` | `0x3A` `0x3B` `0x3D` `0x3E` `0x40` `0x5E` | los signos del juego (el original los guarda como `:` `;`...) |
| `{XX}` | `0xXX` | cualquier otro glifo, p. ej. `{72}` y `{73}` (bordes de un recuadro), `{3C}` (corona), `{5B}`-`{5D}` (iconos) |

Si necesitas otros caracteres (acentos, ñ), redibuja glifos de la fuente en `asset/graficos_ql/fuentes/fuente_normal.png` (por ejemplo los iconos que no usas) y escribe su código con `{XX}`. El empaquetador avisa si el texto tiene un carácter sin código.

**Los textos van en `textos_es` y `textos_en`** (antes iban dentro de `heroquest_xx_idx`, detrás de las N entradas del índice de misiones (a partir de `8 + 40 N`; `data_idioma()` pone `txt` ahí)). Los dos idiomas tienen los mismos elementos, en el mismo orden y con los mismos `TX_*`: el español lleva los del juego y el inglés su versión inglesa (o la española si no tiene). Al final van los elementos solo ingleses (`TX_EN_*`, ventanas que el español no tiene), en los dos ficheros. `tools/hq_textos_empaquetar.py` deja `build/datos/textos_es_bin` y `textos_en_bin` y `tools/hq_misiones_empaquetar.py` los pega al índice; ver `docs/ZX_VERSION_EN.md`.

## Dos versiones del original: español (`.es.txt`) e inglés (`.en.txt`)
Los extractores leen la versión que indica la variable `HQ_VERSION` (`es` por defecto, `en` = `ref/Hero Quest - 128k.tzx`, volcado en `asset/extraido_en/`) y escriben `textos.<v>.txt` y `misionNN.<v>.txt`. Para sacar la inglesa: `HQ_VERSION=en tmp/venv/bin/python -I tools/hq_textos_extraer.py` (igual con `hq_misiones_extraer.py`); `tools/hq_ql_datos.sh extraer` hace las dos. Los empaquetadores usan `HQ_VERSION` (por defecto `es`) y siguen aceptando los nombres antiguos sin idioma (`textos_ql/textos.txt`, `misiones_ql/misionNN.txt`) como copia retocada del español. Las direcciones de cada versión están en `hq_txt_lib.DIR`; diferencias entre las dos en `docs/ZX_VERSION_EN.md`.

## textos.<v>.txt

Cada elemento empieza por una línea `[ID] tipo atributos`; las líneas hasta el siguiente elemento son su contenido. `#` empieza un comentario. **Las longitudes pueden cambiar**: al empaquetar se guardan los textos sin relleno y con su alineación, y es el juego el que recalcula anchos, columnas y centrados.

| Tipo | Contenido | Qué se recalcula |
|---|---|---|
| `ventana` (`[VENT_AC50] ventana y=10 centro=16 ancho=9`) | una línea de texto por línea de ventana: `alineación|texto` (`L`, `C` o `R`); líneas `zona|x1|x2|y1|y2|0xACCION` (áreas activas, como las de «SI/NO»; **dependen de la maquetación original** y hay que revisarlas al traducir; la acción es la dirección de la rutina del original) | el ancho es el mayor entre `ancho` y la línea más larga; la columna izquierda sale de `centro − ancho/2` |
| `guion` (`[GUION_A438] guion`) | líneas `atributo(hex)|alineación|columna|fila|texto` (el texto se dibuja con ese atributo de color; `C` = centrado en la pantalla de 32 columnas, `columna`=16; `L` = desde `columna`); `caja|lo|hi|ancho|alto` (recuadro del original) y `or` | el centrado se recalcula con la longitud nueva; el relleno con espacios del original (que servía para centrar o para borrar texto anterior) se ha quitado: **el juego debe borrar la línea antes de escribir** |
| `cadenas` (`[NOMBRES_HEROE] cadenas campo=9 alineacion=C`) | una cadena por línea (`-` = vacía) | `campo` es el ancho máximo del hueco en pantalla; el empaquetador **da error** si una cadena es más larga. Con `campo` los textos se centran o se alinean en ese ancho |
| `indices` (`[POCIONES_INDICES]`) | pares `a|b` de números | no son texto (índices de nombre y descripción de cada tipo de poción) |

Límites: ventanas de hasta 30 columnas y 20 líneas; líneas de guion de hasta 32 caracteres. Lo que se puede cambiar de las ventanas de mensaje: `VENT_*` (buscar tesoros, trampas, premios, mensajes de misión), `GUION_*` (turnos, mensajes de combate, muerte de héroes, rótulos del panel y de la tienda), `MENU_*` (menú principal, controles, elección de hechizos y de misión, tienda, nombres de héroe), `NOMBRES_*` y `PANEL_*`/`POCIONES_*` (nombres de héroes, monstruos, muebles, armas, armaduras, hechizos y pociones).

Los nombres propios de los personajes jugadores son `{72}SIGMAR{73}` (bárbaro), `GRUGNI` (enano), `LADRIL` (elfo/«duende») y `ZOLTAN` (hechicero), más `RAGNAR`, en `GUION_A484`...`GUION_A4B0`; en el menú de «Crear personaje» el jugador los puede cambiar.

### Binario de textos (`data/textos_es` y `textos_en`, big endian)

```
+0 'HQTX'  +4 versión (word, 1)  +6 número de elementos N (word)  +8 N entradas de 8 bytes
   entrada: desplazamiento desde el inicio del fichero (long), tipo (byte), alineación de la tabla (byte: 'L','C','R' o 0), cantidad (word)
 tipo 1 VENTANA : y, centro, ancho_mínimo, nlíneas, nzonas, 0 ; nlíneas × [alineación (byte 'L'/'C'/'R'), longitud, caracteres] ;
                  nzonas × [x1, x2, y1, y2, acción (word)]
 tipo 2 GUION   : nops (word) ; operaciones: 1 attr alin x y long caracteres | 2 lo hi ancho alto (caja) | 3 (modo OR) ; 0 = fin
 tipo 3 CADENAS : n, campo, alineación, 0 ; n × word (desplazamiento de cada cadena desde el inicio del elemento) ; cada cadena = [longitud, caracteres]
 tipo 4 INDICES : n, 0 ; n × [a, b]
```
Los caracteres son códigos del juego (espacio = `0x5F`). Cada elemento se alinea a palabra. El número de cada elemento (su posición en la tabla) está en `build/datos/textos_ids.h` como `TX_<ID>` (p. ej. `TX_VENT_AC50`); el código del juego debe usar esas constantes y no direcciones fijas, así se pueden añadir textos sin tocar el juego.

Diferencias con el original comprobadas: 131 de 140 elementos se reconstruyen idénticos; en 9 hay una diferencia de ±1 columna al centrar (el redondeo del original no es el mismo) o se ha corregido una errata (la ventana `VENT_BE41` del original tiene el texto 4 caracteres más corto que su cabecera).

## Misiones

`asset/misiones/misionNN.<v>.txt` (14 ficheros por idioma, de `mision00` a `mision13`). Secciones:

| Línea o sección | Significado |
|---|---|
| `mision N` | número (0 = El laberinto, 1 = El rescate de Sir Ragnar, ... 13 = Regreso a Barak Tor) |
| `nombre TEXTO` | nombre que sale en el menú de misiones (máximo 33 caracteres) |
| `eventos N N ...` | **banderas de eventos** (ver abajo), en todas las misiones: números de bit separados por espacios; `-` o sin línea = ninguna. Las misiones de Hero Quest llevan la bandera de su propio número (`eventos 6` en la 6) |
| `muerte E rutina [casillas]` | rutina que se ejecuta al morir un monstruo de la estadística `E` (0 a 19; sin línea, la normal). `normal`: la recompensa de su tipo. `epilogo`: se anota que hay epílogo y, cuando la misión termina (todos los héroes han salido o han muerto), se muestran las páginas del epílogo de la aventura antes de volver al menú. `salida casillas_a_borrar > escalera`: borra las casillas de la capa de objetos (`columna,fila`) y pone la escalera de salida (de 1 a 4 casillas, códigos `0x38` a `0x3B` por orden); sin `>` todo son casillas de la escalera. Hasta 32 casillas por misión. Ejemplos de la ampliación: `muerte 10 salida 13,7 13,8 > 14,7 14,8 15,7 15,8` (misión 4) y `muerte 14 epilogo` (misión 9) |
| `teleporte sala destinos` | con la bandera 13 (`EV_TELEPORTE`): al salir un héroe de la `sala` (andando a una casilla de otra sala o sin sala) va a uno de los destinos (1 a 6) al azar con un dado; cada destino `sala,columna,fila,columna_puerta,fila_puerta` (la casilla de la puerta pasa a ser la última puerta cruzada, no se abre). Ejemplo de la misión 1 de la ampliación: `teleporte 31 7,9,9,10,9 7,9,9,10,9 8,13,6,13,7 13,12,12,12,11 13,12,12,12,11 9,16,9,16,9` |
| `errante pasos tramos` | ruta de los monstruos **errantes** (los que llevan el bit 7 en `b6` y no tienen vida, `docs/ZX_AMPLIACION.md`): `pasos` por ronda de Morcar (1 a 32) y de 1 a 8 tramos que se repiten, cada uno `x` o `y` (columna o fila), `+` o `-` (suma o resta 1 por paso) y la coordenada con la que acaba el tramo; ejemplo de la misión 2: `errante 6 x-0 y-0 x+15 x-0 y+18 x+24`. Cada monstruo errante lleva su propio tramo. Sin la línea, un monstruo errante no se mueve |
| `heroe i|columna|fila|gráfico` | casilla inicial de cada uno de los 4 héroes (bárbaro, enano, elfo, hechicero) y su gráfico/orientación inicial (el tercer número es el índice base de animación) |
| `[mapa]` | 19 líneas de 26 casillas `AA.BB` en hexadecimal. **AA (capa A)**: bits 0-5 = número de habitación (0 = no hay casilla), bit 6 = visible en el plano, bit 7 = visible en la vista 3D. **BB (capa B)**: bit 7 pared al norte, bit 6 pared al oeste, bit 5 puerta al norte, bit 4 puerta al oeste, bit 2 puerta abierta, bit 3 puerta secreta o bloqueada |
| `[indicadores]` | 86 bytes iniciales del original (se copian a `0x6BB6`): marcas de habitaciones registradas, cofres y puertas ya usados. Casi siempre 0. Un valor distinto de 0 y 0xFF en una sala es una sala especial del ojo: 1 a 18 las de Hero Quest, 19 a 29 las de la ampliación (`docs/ZX_MISIONES.md`, `docs/ZX_AMPLIACION.md`) |
| `[monstruos]` | `monstruo|estado|columna|fila|b3|b4|b5|b6|b7|tipo|estadística`: los 10 bytes del registro del original. Estado 0 = hueco vacío, `129` (0x81) = monstruo oculto. `b3`-`b5` los rellena el juego (habitación y dirección de casilla). `b6` = puntos de cuerpo, `b7` = byte con dos nibbles que muestra el panel, `tipo` = nombre (0 duende, 1 orco, 2 fimir, 3 guerrero, 4 esqueleto, 5 zombi, 6 momia, 7 hechicero, 8 Ulag, 9 Grak, 10 Balur, 11 gran brujo) y `estadística` = índice en la tabla de atributos (movimiento, ataque, defensa). Máximo 25 monstruos |
| `[objetos]` | `objeto|columna|fila|código`: código de la capa de objetos: `0x30`-`0x31` casilla bloqueada, `0x33` trampa, `0x38`-`0x3B` escalera de salida (4 casillas), `0x40` cofre, `0x41` trono, `0x42`-`0x4A` mesa, estantería de hechizos, estante, tumba, banco, chimenea, armario, estante y armería (solo como casilla), `0x4B`-`0x5B` muebles con sprite (mesas, estantes, bancos, hechizos, tumba, armarios, armería y chimeneas, ver `asset/graficos/escenario/mueble_*.png`) |
| `[objetivo]` | texto de presentación: `atributo(hex)|alineación|columna|fila|texto` (como los guiones de textos) |

Cada casilla se identifica por columna (0-25) y fila (0-18). Para personalizar una misión basta con editar el fichero de `asset/misiones_ql/` y volver a empaquetar; añadir o quitar monstruos u objetos no requiere cambios en el juego (los recuentos van en la cabecera).

### Binarios

`data/heroquest_NN_es` y `heroquest_NN_en` (big endian; el empaquetador genera siempre los dos idiomas, y el juego carga el que se elige en la primera pantalla):
```
+0 'HQMS'  +4 versión (word, 1)  +6 número de misión (word)
+8  4 héroes × [columna, fila, gráfico]  (12 bytes)
+20 nº de monstruos (byte), nº de objetos (byte), banderas de eventos (word)
+24 9 longs con los desplazamientos (desde el inicio del fichero) de: capa A, capa B, indicadores, monstruos, objetos, objetivo, rutinas de muerte, ruta de los errantes, teletransporte
+60 datos: capa A (494 bytes = 26×19, fila a fila), capa B (494), indicadores (86), monstruos (n × 10), objetos (n × 3: columna, fila, código),
    objetivo (nops (word), operaciones como en un guion de textos_bin, 0), rutinas de muerte (n (byte) y, por entrada: estadística, rutina (0 normal, 1 salida, 2 epilogo),
    nb, na y (nb + na) pares columna, fila), ruta de los errantes (pasos, n y n tramos: eje (bit 0: fila) y sentido (bit 1: suma), coordenada final), teletransporte (sala, n y n destinos de 5 bytes). Cada sección se alinea a palabra.
    Los ficheros de antes (datos desde +48, +52 o +56, sin las últimas secciones) también se cargan.
```
`data/<aventura>_es_idx` y `<aventura>_en_idx` (`heroquest` y, para las ampliaciones, `witchlord`...): cabecera de 48 bytes: `'HQMI'`, versión (word, 2), N = número de misiones (word; 14 en `heroquest`, 10 en `witchlord`), desplazamiento de los **datos de la aventura** (long, 0 = no hay; los textos del juego van aparte, en `textos_es` y `textos_en`) y **subtítulo** (byte de longitud + 35 caracteres del juego: sale bajo HEROQUEST en los menús; el de `heroquest` sale de `asset/misiones_ql/heroquest.<idioma>.txt` (línea `subtitulo ...`, opcional; sin el fichero queda vacío), «RETORNO DEL GRAN BRUJO» / «RETURN OF THE WITCH LORD» en `witchlord`); después N entradas de 40 bytes: número (word), tamaño del fichero (long) y nombre (byte de longitud + 33 bytes con caracteres del juego rellenos con 0). La misión *N* está en `<aventura>_NN_es` o `_en` (dos cifras). Los datos de la aventura van detrás de las entradas: tres longs con los desplazamientos (desde su inicio) del epílogo, de los tipos de monstruo y de las estadísticas (0 = no hay), el epílogo (n páginas (word) y, por página, su tamaño (word) y sus operaciones de guion), los tipos (n (byte) y n entradas de 16 bytes: retrato, icono, sprite, longitud del nombre, clase y nombre en 11 bytes) y las estadísticas (n y n entradas de 9 bytes: movimiento, ataque, defensa, monedas (word), a quién, objeto, mensaje, jefe). Salen de `asset/misiones_ql/<aventura>.<idioma>.txt`. **`[monstruos]`** (obligatoria, también en `heroquest`): una línea `nombre|retrato|icono|sprite|clase` por tipo de monstruo, la primera es el tipo 0 y hasta 16: el nombre (hasta 9 caracteres) que sale en el panel y en la pantalla de combate, el retrato de la pantalla de combate (índice de `retratos`), la baldosa del icono en el plano (índice de `baldosas_bin`), el sprite de la vista (índice de `sprites_bin`) y la **clase**: `N` normal, `V` orco (si el héroe lleva la vara de orco, segundo ataque la primera vez en cada sala) o `M` muerto viviente (la espada espiritual le tira 4 dados en lugar de 3 y el agua santa lo mata); el tipo de un monstruo es el que lleva en la columna `tipo` de `[monstruos]` de su misión. Los valores de Hero Quest son los de las tablas del original (`0x9ACA`, `0xC3B2`, sprite = tipo) y los tipos 12 a 15 repiten esas tablas (`tipo % 13`, `tipo % 14`) hasta que se personalicen. **`[estadisticas]`** (obligatoria, también en `heroquest`): una línea `movimiento|ataque|defensa|monedas|a_quien|objeto|mensaje|jefe` por estadística de monstruo (la columna `estadística` de `[monstruos]` de las misiones; la primera línea es la 0 y se admiten hasta 20). `ataque` y `defensa` son los dados del monstruo; `monedas` lo que da al morir y `a_quien` quién lo recibe (0 nadie, 1 el que lo mata, 2 los cuatro héroes); `objeto` el bit de `+0x0D` que da (0 ninguno; 3 = la baraja de orco); `mensaje` 0 ninguno, 1 BANE DE ORC, 2 ESTRELLA DEL OESTE; `jefe` 1 = solo la espada espiritual le hace daño. `movimiento` no tiene efecto, como en el original (el `DEC A` que lo gasta no deja acarreo: `docs/HALLAZGOS.md`). La rutina de muerte propia de una estadística (salida, epílogo) sigue siendo la línea `muerte` de la misión, y sustituye a la recompensa. Hero Quest lleva sus 20 estadísticas con las recompensas del original; la ampliación, sus 16 sin recompensas. `[epilogo]` sale de la sección `[epilogo]` de `asset/misiones_ql/<aventura>.<idioma>.txt`: una línea `pagina` por página y, debajo, las líneas `atributo(hex)|alineación|columna|fila|texto` como en `[objetivo]`; `data_epilogo()` da cada página. Las genera `tools/hq_misiones_empaquetar.py`: `heroquest` siempre y cada ampliación que tenga `asset/misiones[_ql]/<aventura>.<idioma>.txt` (línea `subtitulo ...`) con sus `<aventura>_NN.<idioma>.txt` consecutivos desde 00 (el número de misiones sale de cuántos hay).

**Los nombres del menú de misiones salen siempre del índice** (`<aventura>_xx_idx`, también en Hero Quest, centrados en la fila 4 + n): se editan en la línea `nombre ...` de `asset/misiones_ql/misionNN.<idioma>.txt` (o `<aventura>_NN.<idioma>.txt`). El guion `MENU_689A` del original (y su equivalente inglés `MENU_6870`) ya no existe en `textos.xx.txt`.

**Banderas de eventos** (`eventos` en el texto de la misión, word en `+22` del fichero, `Partida.ev`, constantes `EV_*` de `src/partida.h`). Cada bit repite lo especial de una misión de Hero Quest, sea cual sea el número de la misión nueva (`docs/ZX_MISIONES.md`):

| Bit | Constante | Efecto |
|---|---|---|
| 0 | `EV_SALE_PRIMERO` | solo el primer héroe que sale por la escalera cobra 100 monedas |
| 1 | `EV_RAGNAR` | quinto personaje (Sir Ragnar) en la casilla (6, 1); se encuentra en la sala `0x16` (mapa a la vista); al salir, se pregunta cómo repartir la recompensa; la misión sigue mientras viva |
| 3 | `EV_ORO_MAGNUS` | el oro de Magnus (bit 6 de `+0x0F`): 200 monedas al salir con él y se puede volver a por más |
| 4 | `EV_ROCA` | cae una roca que cierra la casilla (25, 7); abrir la puerta de la columna 5 (filas 3 y 4) deja ver al monstruo 1 |
| 5 | `EV_SIN_EQUIPO` | los héroes empiezan sin equipo ni hechizos y lo recuperan al entrar en la sala `0x1D` |
| 6 | `EV_SALE_COBRA` | cada héroe que sale por la escalera cobra 100 monedas |
| 7 | `EV_SIN_FUEGO` | el elfo y el hechicero pierden los hechizos de fuego |
| 9 | `EV_CASTILLO` | 5000 monedas en la sala `0x18`, teletransportes al azar, sin ver el mapa y fantasma al buscar tesoros |
| 12 | `EV_DESPIERTA` | pisar la columna 5 (filas 3 y 4) hace que el monstruo 1 cobre vida (lo de `EV_ROCA` sin la roca). **No lo usa ninguna misión**: el código equivalente de la ampliación (`0x71EF`) nunca se llama |
| 14 | `EV_ROCA_DER` | (ampliación, misión 7) la roca de la trampa de código `0x34` cae en la casilla de la derecha del héroe: si hay un héroe pierde un punto (y muere si era el último, dejando la roca), si no la casilla queda cerrada |
| 13 | `EV_TELEPORTE` | (ampliación, misión 1) salir de la sala de la línea `teleporte` lleva a un destino al azar (destinos en la misión) |
| 11 | `EV_CIERRE` | la sala `0x1F` se cierra al entrar y la trampa cierra la salida cuando los cuatro la han pisado |

Las salas, casillas y números de objeto de cada bandera son los del original (no son parámetros); los bits 5, 9 y 11 comparten el aviso de entrada en sala: no los combines en una misma misión (manda 5, luego 9 y luego 11). Ejemplo: `eventos 6` en cualquier misión hace que cada héroe que salga cobre 100 monedas.

**Cargar otras aventuras** (5.ª opción del menú principal): pide el nombre (la rejilla de letras, hasta 12) y carga `<nombre>_es_idx` (o `_en_idx`, según el idioma) de la unidad `DIR$`: el número de misiones, los nombres del menú de misiones y el subtítulo salen de ese índice, y las misiones de `<nombre>_NN_es`. Los textos, gráficos y fuentes siguen siendo los de Hero Quest. Sin el fichero sale «NO SE ENCUENTRA»; escribir `HEROQUEST` vuelve al juego original. Los eventos de cada misión salen de sus banderas (`eventos`): las misiones de Return of the Witch Lord llevan `eventos -` (ninguna) porque sus eventos propios no se han analizado. La lista de monstruos admite `N_MONS` = 32 entradas (Hero Quest usa 25; el motor de la ampliación trata 32).

Las 14 misiones empaquetadas coinciden byte a byte (mapa, héroes, indicadores, monstruos, objetos y textos) con las del original (`tools/hq_misiones_verificar.py`).
