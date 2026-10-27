# Textos y misiones para el port a QL

Igual que los gráficos (`docs/ZX_GRAFICOS_QL.md`), los textos y las misiones se extraen del original a ficheros de texto **editables**, se copian a una carpeta propia del usuario que ninguna herramienta de extracción sobrescribe y se empaquetan en binarios para el QL. El juego del QL leerá los binarios, de modo que **se pueden traducir y personalizar sin modificar el juego**.

**Nombres de fichero del QL: sin puntos.** Todos los ficheros de `data/` terminan en `_bin` (`sprites_bin`, `textos_bin`, `mision_03_bin`...). Lo que solo sirve al PC (índices, constantes) va a `build/datos/`.

```
asset/textos/textos.es.txt|.en.txt  <- original extraído (se regenera)     asset/textos_ql/textos.es.txt|.en.txt  <- tuyo (nunca se sobrescribe)
asset/misiones/misionNN.es.txt|.en.txt <- originales extraídos (se regeneran)  asset/misiones_ql/misionNN.es.txt|.en.txt <- tuyos
data/textos_bin, data/mision_NN_es_bin y _en_bin, data/misiones_indice_es_bin y _en_bin   <- binarios del QL (misiones en los dos idiomas)
build/datos/textos_ids.h, indice_textos.txt, indice_misiones_es.txt e indice_misiones_en.txt     <- índices para el PC
```

Todo el flujo (también gráficos): `tools/hq_ql_datos.sh extraer` (extrae los originales, copia a `*_ql` lo que falte y empaqueta) y, después de editar, `tools/hq_ql_datos.sh`. Cada paso se puede lanzar por separado: `hq_textos_extraer.py`, `hq_misiones_extraer.py`, `hq_ql_inicializar.py` (copia solo lo que falte; `--estado` lista lo pendiente), `hq_textos_empaquetar.py`, `hq_misiones_empaquetar.py`. El empaquetado usa el fichero de `*_ql` si existe y, si no, el original; indica cuántos están retocados. `hq_textos_verificar.py` y `hq_misiones_verificar.py` comparan los binarios con la memoria del juego original (solo con los originales sin tocar).

## Alfabeto

El juego dibuja el carácter de código `c` con el glifo `c − 0x30` de la fuente (8 bytes por glifo, 68 glifos, `fuente_normal_bin`), por lo que solo se pueden escribir los glifos que tiene la fuente. En los ficheros de texto se escribe así:

| Escribes | Código | Glifo |
|---|---|---|
| `A`-`Z`, `0`-`9` | el ASCII | letras y cifras (no hay minúsculas, ñ ni acentos) |
| espacio | `0x5F` | en blanco |
| `.` `,` `'` `!` `=` `?` | `0x3A` `0x3B` `0x3D` `0x3E` `0x40` `0x5E` | los signos del juego (el original los guarda como `:` `;`...) |
| `{XX}` | `0xXX` | cualquier otro glifo, p. ej. `{72}` y `{73}` (bordes de un recuadro), `{3C}` (corona), `{5B}`-`{5D}` (iconos) |

Si necesitas otros caracteres (acentos, ñ), redibuja glifos de la fuente en `asset/graficos_ql/fuentes/fuente_normal.png` (por ejemplo los iconos que no usas) y escribe su código con `{XX}`. El empaquetador avisa si el texto tiene un carácter sin código.

`textos_bin` lleva, además de los textos españoles, las ventanas inglesas y las pociones inglesas (elementos `EN_*` al final) y `build/datos/vent_en.h` con la correspondencia; ver `docs/ZX_VERSION_EN.md`.

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

### Binario `data/textos_bin` (big endian)

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
| `nombre TEXTO` | nombre que sale en el menú de misiones (máximo 28 caracteres) |
| `heroe i|columna|fila|gráfico` | casilla inicial de cada uno de los 4 héroes (bárbaro, enano, elfo, hechicero) y su gráfico/orientación inicial (el tercer número es el índice base de animación) |
| `[mapa]` | 19 líneas de 26 casillas `AA.BB` en hexadecimal. **AA (capa A)**: bits 0-5 = número de habitación (0 = no hay casilla), bit 6 = visible en el plano, bit 7 = visible en la vista 3D. **BB (capa B)**: bit 7 pared al norte, bit 6 pared al oeste, bit 5 puerta al norte, bit 4 puerta al oeste, bit 2 puerta abierta, bit 3 puerta secreta o bloqueada |
| `[indicadores]` | 86 bytes iniciales del original (se copian a `0x6BB6`): marcas de habitaciones registradas, cofres y puertas ya usados. Casi siempre 0 |
| `[monstruos]` | `monstruo|estado|columna|fila|b3|b4|b5|b6|b7|tipo|estadística`: los 10 bytes del registro del original. Estado 0 = hueco vacío, `129` (0x81) = monstruo oculto. `b3`-`b5` los rellena el juego (habitación y dirección de casilla). `b6` = puntos de cuerpo, `b7` = byte con dos nibbles que muestra el panel, `tipo` = nombre (0 duende, 1 orco, 2 fimir, 3 guerrero, 4 esqueleto, 5 zombi, 6 momia, 7 hechicero, 8 Ulag, 9 Grak, 10 Balur, 11 gran brujo) y `estadística` = índice en la tabla de atributos (movimiento, ataque, defensa). Máximo 25 monstruos |
| `[objetos]` | `objeto|columna|fila|código`: código de la capa de objetos: `0x30`-`0x31` casilla bloqueada, `0x33` trampa, `0x38`-`0x3B` escalera de salida (4 casillas), `0x40` cofre, `0x41` trono, `0x42`-`0x4A` mesa, estantería de hechizos, estante, tumba, banco, chimenea, armario, estante y armería (solo como casilla), `0x4B`-`0x5B` muebles con sprite (mesas, estantes, bancos, hechizos, tumba, armarios, armería y chimeneas, ver `asset/graficos/escenario/mueble_*.png`) |
| `[objetivo]` | texto de presentación: `atributo(hex)|alineación|columna|fila|texto` (como los guiones de textos) |

Cada casilla se identifica por columna (0-25) y fila (0-18). Para personalizar una misión basta con editar el fichero de `asset/misiones_ql/` y volver a empaquetar; añadir o quitar monstruos u objetos no requiere cambios en el juego (los recuentos van en la cabecera).

### Binarios

`data/mision_NN_es_bin` y `mision_NN_en_bin` (big endian; el empaquetador genera siempre los dos idiomas, y el juego carga el que se elige en la primera pantalla):
```
+0 'HQMS'  +4 versión (word, 1)  +6 número de misión (word)
+8  4 héroes × [columna, fila, gráfico]  (12 bytes)
+20 nº de monstruos (byte), nº de objetos (byte), 0, 0
+24 6 longs con los desplazamientos (desde el inicio del fichero) de: capa A, capa B, indicadores, monstruos, objetos, objetivo
+48 datos: capa A (494 bytes = 26×19, fila a fila), capa B (494), indicadores (86), monstruos (n × 10), objetos (n × 3: columna, fila, código),
    objetivo (nops (word), operaciones como en un guion de textos_bin, 0). Cada sección se alinea a palabra.
```
`data/misiones_indice_es_bin` y `misiones_indice_en_bin`: `'HQMI'`, versión (word, 1), N = 14 (word) y N entradas de 40 bytes: número (word), tamaño del fichero (long) y nombre (byte de longitud + 33 bytes con caracteres del juego rellenos con 0). La misión *N* está en `mision_NN_bin` (dos cifras).

Las 14 misiones empaquetadas coinciden byte a byte (mapa, héroes, indicadores, monstruos, objetos y textos) con las del original (`tools/hq_misiones_verificar.py`).
