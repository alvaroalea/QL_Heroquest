# HeroQuest (ZX Spectrum 128K, Dro Soft 1991) — análisis técnico

Origen: `ref/Hero Quest (Dro Soft).tzx`. Es un juego **solo para 128K** (paginación por `0x7FFD`, sonido con el AY-3-8912).
La versión española: todos los textos están en español. Información de juego en `docs/ZX_JUEGO.md`; inventario de rutinas en `docs/ZX_INVENTARIO.md`.

Estado del trabajo: ver el apartado «Qué queda por hacer / incertidumbres» al final. Lo marcado como *(probable)* está deducido de la lectura del código pero no verificado en ejecución.

## 1. Cómo se carga

El TZX tiene 11 bloques estándar (`tools/tzx_to_z80.py` los extrae a `asset/extraido/`):

| Bloque | Contenido |
|---|---|
| 0, 1 | Cabecera y programa BASIC `HERO`: `CLEAR 24575: BORDER 0: PAPER 0: INK 0: CLS: LOAD ""CODE 16384`, `LOAD ""CODE`, `LET mc = USR 28030` |
| 2, 3 | `screen`: pantalla de carga (6912 bytes en 16384; `asset/graficos/pantalla_carga.png`) |
| 4, 5 | `game.bin`: 39.934 bytes en **25600 (0x6400) a 65533**: el código y los datos de la memoria fija y el banco 0 |
| 6 a 10 | Cinco bloques **sin cabecera** (flags 0x0B a 0x0F) que carga el propio juego, no la ROM |

`USR 28030` es `0x6D7E: JP 0x6580`. La rutina `0x6580` (`CARGADOR_BANCOS`) carga con la rutina de la ROM (`0x0562`, tras emular el preámbulo de `LD-BYTES`) los cinco bloques en los bancos de la RAM de 128K y salta a `0x6400`:

| Flag | Banco | Destino | Longitud | Contenido |
|---|---|---|---|---|
| 0x0B | 1 (`0x11`) | 0xD000 | 0x2FFE | Misiones 0 a 6 |
| 0x0C | 3 (`0x13`) | 0xC000 | 0x3FFE | Misiones 7 a 13 |
| 0x0D | 4 (`0x14`) | 0xC000 | 0x3FFE | **Reproductor de sonido** y partituras |
| 0x0E | 6 (`0x16`) | 0xC000 | 0x3FFE | Marco de pantalla, **overlay del menú**, retratos, muebles, textos |
| 0x0F | 7 (`0x17`) | 0xC000 | 0x0D40 | Dos fuentes y gráficos de texto |

`tools/tzx_to_z80.py` reconstruye los 8 bancos (`asset/extraido/bankN.bin`) y un snapshot `.z80` de 128K (`hq128.z80`) con el que se usan las herramientas de SkoolKit.

## 2. Mapa de memoria

Memoria visible durante el juego (banco 0 en 0xC000 salvo cuando se pagina otro):

| Zona | Contenido |
|---|---|
| 0x4000-0x57FF | Bitmap de la pantalla (mapa isométrico y panel); `0x5800` atributos; `0x5B00-0x5DFF` copia de los atributos y, después, **área de fondos guardados bajo los sprites** |
| 0x5B00-0x6CEE | **Overlay del menú** (copiado desde el banco 6) mientras se está en el menú. Durante la partida esta zona contiene: el área de fondos y buffers de trabajo (0x5E00 gráficos temporales) |
| 0x6200-0x63FF | Capa del mapa A: por casilla (26 columnas × 19 filas) nº de habitación (bits 0-5), bit 6 = visible en el plano, bit 7 = visible en 3D |
| 0x6400-0x65FF | Capa del mapa B: paredes y puertas: bit 6 pared oeste, bit 7 pared norte, bit 4 puerta oeste, bit 5 puerta norte, bit 2 puerta abierta/visitada, bit 3 *(probable)* puerta secreta o bloqueada |
| 0x6600-0x67FF | Capa del mapa C: objetos: 0x01-0x2F monstruo nº N, 0x30-0x31 mueble que bloquea, 0x32-0x36 objeto recogible o trampa, 0x37-0x3F, 0x40-0x4A muebles, 0x4B-0x5B puertas dibujadas (gráficos de `0x9470`) |
| 0x6800-0x68FF | Tabla de máscaras de sprites (la crea el overlay): `~(b \| b<<1 \| b<<2 \| b>>1 \| b>>2)` |
| 0x6A8E-0x6B8D | 14 registros de animación de 18 bytes (5 personajes + 13 objetos; ver §6) |
| 0x6B8A | Camino que sigue un monstruo |
| 0x6BB6 | 0x56 indicadores de misión (puertas, cofres, palancas ya usados) |
| 0x6C0C | Lista de **25 monstruos** de 10 bytes cada uno |
| 0x6D7D | Pila |
| 0x6D81-0xB4C1 | **Código y datos de la partida** (ver `ref/zx/heroquest_main.skool`) |
| 0xA981-0xAA80 | Variables del juego, hasta el registro de los héroes (`datos.txt`) |
| 0xA9E5, 0xA9FF, 0xAA19, 0xAA33, 0xAA4D | **Registros de los 4 héroes y de un quinto personaje** (26 bytes cada uno) |
| 0xAA8C | Tabla de atributos por tipo de personaje (5 bytes) |
| 0xAAF0 | Tabla de 176 direcciones de línea de la pantalla de la zona de juego |
| 0xAC50-0xC1B6 | Textos del juego (ventanas y guiones) |
| 0xB1C8-0xB4C0 | Baldosas de 8×8 y fuente de 8×8 activa (0xB2C8) |
| 0xB4F0 | Tabla de **zonas activas de la pantalla de juego** (los iconos) |
| 0xB72B, 0xC1B6... | Tablas de dibujo (paredes, baldosas) |
| 0xC000-0xE6FF (banco 0) | Tablas gráficas: baldosas de pared, **65 sprites de 24 px** (tabla 0xC23A), cursor (0xC55B), etc. |
| 0xE700-0xFCFF | **Buffer de vista** de 32 × 176 bytes (doble buffer: se compone ahí y 0x8D03 lo vuelca a la pantalla) |
| 0xFD00-0xFD59 | Orden de dibujo de las figuras |
| 0xFD60-0xFDFD | Rutinas de lectura de controles |
| 0xFE00-0xFF01 | Tabla de vectores IM 2 (todo 0xFD); `0xFDFD` tiene un `JP 0x6F99` |

### Bancos

| Banco | Qué hay |
|---|---|
| 0 (0xC000-0xFFFD) | Parte alta de `game.bin` |
| 1 (0xD000-0xFFFF) y 3 (0xC000-0xFFFF) | Las 14 misiones (§7) |
| 4 | Reproductor de sonido en `0xD388` y las melodías y efectos (§8) |
| 5 | Pantalla normal (0x4000) |
| 6 | 0xC000: marco de pantalla (0x1B00 bytes); 0xDB58: overlay del menú; 0xE6xx: textos; retratos en `0xF000`-`0xFC00`; gráficos de muebles en `0xFD1C-0xFF8B` |
| 7 | Fuentes: `0xC000` (normal) y `0xC220` (manuscrita), 68 caracteres de 8 bytes cada una, más gráficos |

La pantalla alternativa del 128K no se usa para el doble buffer (se usa el buffer de 0xE700).

## 3. Overlays y paginación

`0x9CCE` pagina el banco 6 (`OUT 0x7FFD,0x16`), `0x9CD0` pagina el que se pase en A y `0x9CD7` vuelve al banco 0 (`0x10`). **Ninguno de los dos cambia la ROM** (bit 4 = 1: ROM 1 de 48 BASIC, que el juego no usa).

- `CARGAR_MENU` (`0x9CA3`) copia 0x11EE bytes del banco 6 `0xDB58` a `0x5B00` y llama a `0x5B00`.
- Los **retratos** y los **muebles** se copian a `0x5E00`/`0x5E20` desde el banco que dice la tabla de `0x9ABB` (puntero y banco) y se componen en pantalla.
- La **interrupción** (`0x6F99`, 50 Hz) pagina el banco 4 y llama al reproductor dos veces.

## 4. Flujo del juego

```
ARRANQUE (0x6400) -> BUCLE_PRINCIPAL (0x6D81)
   CARGAR_MENU (0x9CA3)  ->  MENU_PRINCIPAL (0x5B00): Jugar | Comprar equipo | Crear personaje | Controles | Cargar otras aventuras (nunca usada)
                          ->  INICIAR_MISION (0x5B3F): descomprime la misión elegida
   PARTIDA (0x6D89): bucle de rondas
       para cada personaje (4 héroes y el 5.º): TURNO_PERSONAJE (0x6DDD)
            TIRAR_DADOS (0x6F1C) -> bucle de entrada hasta que termina el turno
       FIN_DE_RONDA (0x9CE0): turno de Morcar (monstruos), comprobar fin de misión (0x6EB5)
```

Bucle de entrada de un turno (`0x6E53`): `HALT` → `MOVER_CURSOR` → `PROCESAR_CLIC` → animaciones → guardar fondos de los sprites (`0xA794`) → dibujar figuras (`0xA534`) y cursor (`0xA57E`) → volcar el buffer (`0x8D03`) → restaurar fondos (`0xA8DE`).

### Entrada

`0x9F89` llama a la rutina de lectura instalada (parche en `0x9F8E`/`0x6F62`) y devuelve en `0xA997`: bit 4 botón, bit 3 arriba, bit 2 abajo, bit 1 izquierda, bit 0 derecha.
- `0xFD60`: teclado Z/X (izquierda/derecha), O/K (arriba/abajo), SPACE (botón).
- `0xFD90`: Sinclair 2 (6, 7, 8, 9, 0). `0xFDB8`: Sinclair 1 (1 a 5). `0xFDE0`: Kempston.
- La pantalla de controles del menú solo alterna teclado/Kempston; las otras variantes parecen no usarse *(probable)*.

### Clic y zonas

`0xA9DB` apunta a la tabla de zonas activas vigente: entradas de 6 bytes (columna inicial, columna final exclusiva, fila inicial, fila final exclusiva, rutina); el final es `0xFF` (si no hay zona, `0x6FF6` mueve el personaje hacia el cursor) o `0xFE` (no hacer nada). Zonas de la pantalla de juego (`0xB4F0`, 10 zonas):

| Zona (columnas, filas) | Rutina | Icono |
|---|---|---|
| 22-24, 20-21 | `0x82CB` | flecha: una casilla en x-1 |
| 22-24, 21-22 | `0x8478` | flecha: una casilla en y+1 |
| 24-26, 20-21 | `0x83E3` | flecha: una casilla en y-1 |
| 24-26, 21-22 | `0x8357` | flecha: una casilla en x+1 |
| 26-29, 20-22 | `0x804E` | ojo (revisar): trampas/puertas o tesoros |
| 9-12, 20-22 | `0x725E` | siguiente personaje (pasar turno) |
| 29-32, 19-22 | `0x7710` | mapa |
| 26-28, 18-20 | `0x7877` | bolsa (inventario) |
| 28-32, 17-19 | `0x7319` | atacar (espada y escudo) |
| 20-22, 21-22 | `0x727E` | llave (abrir puerta) |

Se leen directamente de `ZONAS_JUEGO` (`0xB4F0`) en `ref/zx/heroquest_main.skool`; coinciden con la descripción de la interfaz dada por el usuario (`docs/ZX_JUEGO.md`).

## 5. Dados y combate

- `DADO` (`0x9BD3`): `C` = 0..5 usando el registro R como semilla. Es la rutina `dice` del código fuente original (restos en memoria, §9).
- `DADO_COMBATE` (`0x9BC8`): `C` = símbolo de la cara. Tabla `0xAA7B` = 0 (calavera, 3 caras), 1 (escudo blanco, 2 caras), 2 (escudo negro, 1 cara).
- Búsqueda de tesoros: `BUSCAR_TESOROS` (`0x80A9`) roba una carta de un **mazo de 25 cartas** (`0x9593`, `BARAJAR_MAZO` `0x9542`): 0-8 monedas (tabla `0xB688`), 9-15 pociones o efectos, 16 «no ves nada», el resto trampas, fantasma o monstruo que ataca (ventanas `0xAC50-0xAE5F`).
- Tirada de movimiento: `TIRAR_DADOS` (`0x6F1C`) suma `0xA9B5` dados (1 o 2, a veces el doble) en `0xA98C`.
- Ataque: dados de ataque = `(IY+0x14)` (`0xA9A9`) y de defensa = `(IY+0x15)` (`0xA9AA`), con bonos por hechizos y armas; ver `PREPARAR_ATAQUE`, `TIRADA_ATAQUE`, `TIRADA_DEFENSA`, `DANO_AL_OBJETIVO` y `MATAR_PERSONAJE`.

## 6. Estructuras

### Registro de héroe (26 bytes, `0xA9E5...`) *(probable)*

| Offset | Significado |
|---|---|
| +0 | Estado (0 = no existe) |
| +1, +2 | Columna y fila |
| +3 | Habitación |
| +4, +5 | Dirección de la casilla en la capa B |
| +6 | Puntos de **cuerpo** |
| +7 | Puntos de **mente** |
| +8 | Fotograma/sprite (orientación + 4 por héroe) |
| +9, +10 | Oro (palabra) |
| +11, +12 | Grupos de hechizos |
| +13 | Objetos (bits) |
| +14 | Armas (bits: bastón, daga, espada, gran hacha, ballesta, hacha, lanza) |
| +15 | Armadura (bits 0-3: escudo, casco, malla, armadura) y estados (bits 4-7) |
| +16 | Efectos activos (bits: fuerza, resistencia, curación×2, agua santa, velocidad, heroico) |
| +17, +18 | Estados temporales (bit 0 y 1: +2 dados de ataque / defensa, bit 5: movimiento doble, bit 4: ..., bit 6 ...) |
| +20, +21 | Dados de ataque y de defensa base |
| +22 | Hechizo elegido |
| +25 | Índice del personaje (0..4) |

### Registro de monstruo (10 bytes, desde `0x6C0C`, 25 monstruos)

`+0` estado (0 = vacío, 0x81 = activo oculto), `+1/+2` columna y fila, `+3` habitación, `+4/+5` casilla, `+6` puntos de cuerpo y bit 0 = visible, `+7` byte con dos nibbles que el panel muestra como cuerpo y mente, `+8` tipo (índice de nombre: 0 duende, 1 orco, 2 fimir, 3 guerrero, 4 esqueleto, 5 zombi, 6 momia, 7 hechicero, 8 Ulag, 9 Grak, 10 Balur, 11 gran brujo), `+9` índice de la tabla de atributos (`0xAA8C`, 5 bytes: movimiento, ataque, defensa, rutina al morir o especial).

### Registro de animación (18 bytes, `0x6A8E...`)

`+0` estado (0 libre, 1 estático, 2 héroe animado), `+1` baldosa, `+2/+3` posición en píxeles, `+4/+5` ancho y alto, `+6` tipo de dibujo (índice a la rutina de `0xA876`/`0xA920`/`0xA572`), `+8/+9` guion de animación, `+0A/+0B` puntero a los gráficos, `+0C/+0D` fondo guardado, `+0E/+0F` posición en el buffer, `+10/+11` puntero a la máscara.

### Guiones de texto (intérprete `0x8BF9`) y ventanas (`0x8DD1`)

Guion: `FF x y` posición, `FB c` color, `FC lo hi w h` recuadro, `FE` modo OR, `FA` o `00` fin; cualquier otro byte es un carácter (código-0x30)×8 en la fuente de `0xB2C8`. `_` (0x5F) es el espacio.
Ventana: `x y ancho alto` + `alto×ancho` caracteres + zonas opcionales (6 bytes) + `0xFE`.
Los textos extraídos están en `asset/textos/` (ventanas, guiones y guiones del menú).

## 7. Misiones

Datos en los bancos 1 y 3; la tabla de punteros está en el overlay (`0x6637`, 14 palabras: `D000 D678 DD79 E3F8 EA75 F1D4 F94A` en el banco 1 y `C000 C70F CE4E D51C DD46 E4DC EC91` en el banco 3).
Formato (deducido de `INICIAR_MISION`, `tools/hq_missions.py`):

| Desplazamiento | Contenido |
|---|---|
| +0x000 | Capa A (0x200 bytes) |
| +0x200 | Capa B (0x200 bytes) |
| +0x400 | 4 posiciones de inicio de los héroes (x, y, orientación) |
| +0x40C | 0x56 indicadores |
| +0x462 | Cabecera de 11 bytes: desplazamiento y longitud del texto de presentación, desplazamiento y longitud de la lista de monstruos, desplazamiento y número de objetos |
| a continuación | Texto de presentación (un guion de texto), lista de monstruos (10 bytes cada uno) y lista de objetos (3 bytes: x, y, código) |

`asset/misiones/mision00.txt ... mision13.txt` contiene un mapa ASCII de cada misión (`H` héroes, `M` monstruos, `o` objetos). Los nombres, por orden: El laberinto, El rescate de Sir Ragnar, La guarida de los orcos, El oro del príncipe Magnus, El laberinto de Melar, El legado orco, El cazador de piedra, Mago del fuego, Carrera contra el tiempo, El castillo misterioso, El baluarte de Chaos, Barak Tor, La espada espiritual y Regreso a Barak Tor (coincide con el mapa de `ref/HeroQuest_map.png`).

## 8. Gráficos

Se extraen con `tools/hq_gfx_extraer.py` a `asset/graficos/` (131 PNG de 8 colores con transparencia; flujo de retoque y empaquetado para el QL en `docs/ZX_GRAFICOS_QL.md`).

| Carpeta | Contenido |
|---|---|
| `sprites/` | 66 sprites de personajes y monstruos (tabla `0xC23A`: ancho, alto, puntero; los índices 64 y 65 son el cofre y el trono) y el cursor (`0xC55B`, con máscara guardada) |
| `escenario/` | paredes oeste y norte (`0xC5BF`, `0xC655`, 24×50), 8 piezas de puerta (24×50 desde `0xC6EB`), suelo y 4 escaleras (rombos de 120 bytes desde `0xE4A8`; objetos `0x38`-`0x3B` = SALIDA), los 17 muebles (tabla `0x88A0`: mesas, estantes, bancos, estantería de hechizos, tumba, armarios, armería y chimeneas) y las 17 piezas base de los muebles (tabla `0x9470`, función exacta sin confirmar) |
| `retratos/` | **10 retratos de combate** de 128×64 con color (16×8 celdas de atributos): bárbaro (banco 4 `0xFA80`), enano (`0xF600`), elfo (`0xF180`), hechicero (banco 7 `0xC440`) y los de monstruos: `0xE880` (duende, orco y los jefes Ulag y Grak), fimir (banco 3 `0xF802`), guerrero del caos (banco 7 `0xC8C0`), esqueleto y otros (banco 3 `0xF382`), zombi y momia (`0xE400`), más `ataque_fallido` (`0xED00`, espada contra escudo). Formato: 1024 bytes de bitmap en filas de 16 y 128 de atributos; los copia `0x9A7D` a `0x5E00` y a la pantalla |
| `marco/` | marco de la pantalla de juego (banco 6 `0xC000`, 256×192 con sus atributos) y pantalla de carga |
| `fuentes/` | baldosas de 8×8 (`0xB1C8`), fuente normal (banco 7 `0xC000`, que es también la que hay en `0xB2C8` al arrancar) y fuente manuscrita (banco 7 `0xC220`) |

Los retratos se muestran cuando un personaje recibe daño (`0x99A2` para monstruos y `0x9996` para héroes); en el código no hay otra rutina que cargue retratos, así que no parece haber imágenes específicas de muerte: el último golpe enseña el mismo retrato y después aparece el mensaje «EL BARBARO HA MUERTO», etc.

Los sprites de personajes y muebles tienen **tres estados por píxel**: tinta (bit 1 del sprite), halo claro (zona de unos 2 píxeles alrededor de la tinta que se fuerza a 0, calculada con la tabla de máscaras de `0x6800`) y transparente (se conserva el fondo): `buffer = (buffer AND mascara[gfx]) OR gfx`. «Blanco» y «negro» son en realidad el paper y el ink de la celda de atributos. Solo el cursor tiene la máscara guardada (a +0x30 del gráfico).

Cómo se dibuja: la vista 3D se compone en el buffer de `0xE700` (32×176 bytes): primero las casillas (`DIBUJAR_CASILLA`, `0x931D`), con baldosas de 8×8 para el suelo y las paredes, las piezas de pared y puerta de 24×50 y los rombos de las escaleras; luego los sprites (héroes, monstruos, cofre, trono y muebles) con fondo guardado y restaurado en cada fotograma.

## 9. Sonido

El reproductor del banco 4 (`0xD388`) usa el AY-3-8912 con tres canales de música y uno de efectos (estructuras de 0x42 bytes en `0xDDD8`, `0xDE1A`, `0xDE5C` y `0xE2D0`). Entrada con A = comando (0 actualizar la música, 1 envolventes, 2 empezar melodía D, 3 bajada de volumen, 4 lanzar el efecto E, otro = silencio). La tabla de periodos está en `0xDC08`, las secuencias de notas en `0xC4xx-0xD387`.
Los efectos de sonido que usa el código fuente son los números 1-7 (`0x7BA4`: E = efecto): 1 ataque, 2 *(probable)* paso, 3 *(probable)* herida, 4, 5, 6, 7.

## 10. Hallazgos curiosos

1. **Restos del código fuente original** del programador en `0x6A8E`, `0x6B9E`, `0x6C0C` y `0x7BEC` (mientras se cargaba el juego la memoria conservaba el búfer del ensamblador). Etiquetas: `mons_addr`, `w_stat`, `eb_graf`, `skeleton`, `zombie`, `mummy`, `play_attack`, `combat_dice`, `combat_list`, `dice`, `clear_sprite`, `get_de`. La versión de `dice`: `ld c,3 / ld a,r / ld b,a / rr b / loop: dec c / jr nz,over / ld c,6 / over: djnz loop / dec c / ret`. El comentario `;spirit blade` indica que el ataque con la espada espiritual hace 3 dados, o 4 contra no muertos.
2. **Código de grabación para +3 DOS** sin usar en `0x65E1-0x6672` y los nombres de ficheros de la versión de disco: `GAME.BIN`, `LEV1.BIN`, `LEV8.BIN`, `MUSIC.BIN`, `SHOP.BIN`, `PICS.BIN`, `SCREEN.BIN`.
3. **Enlace de desarrollo** por puertos `0x3F`, `0x5F`, `0x1F` en `0x6900-0x6A39`, nunca ejecutado en el juego.
5. Errata en el menú: «ENAMO» por «ENANO» (`0x69F6` del overlay); «ENEIGO» en un mensaje de ataque; «HECHICE» en otro (`0x6841`).
6. **Dos mensajes con ventana más corta que su cabecera** (p. ej. `0xBE41`): el texto invade las 4 primeras posiciones de la ventana siguiente.

## 11. Herramientas

Todas en `tools/` (Python 3 y SkoolKit 10.1, instalado en `tmp/venv` con `python3 -m venv tmp/venv && tmp/venv/bin/pip install skoolkit`).

| Herramienta | Uso |
|---|---|
| `tzx_to_z80.py` | extrae bloques y bancos y escribe el snapshot de 128K |
| `hq_trace.py` + `hq_fuzz_script.py` + `hq_cov_merge.py` | simulan el juego con teclas inyectadas y registran la **cobertura de ejecución** (`asset/trace/cov.txt`) |
| `hq_ctl.py` | genera el `.ctl` de código/datos a partir de la cobertura |
| `hq_subs.py`, `hq_scripts.py`, `hq_textos.py`, `hq_strings.py` | tipan los datos de texto (guiones, ventanas, cadenas) |
| `hq_notes2ctl.py` | une el `.ctl` automático con las notas manuales de `ref/zx/notas/` |
| `hq_build.sh` | reconstruye los tres `.skool` de `ref/zx/` y los **verifica** reensamblando con `skool2bin` (`hq_verify.sh`) |
| `hq_missions.py` | extracción de las misiones |
| `hq_gfx_extraer.py`, `hq_ql_inicializar.py`, `hq_ql_empaquetar.py`, `hq_png.py` | gráficos: extracción a PNG, carpeta de retocados y empaquetado al formato del QL (`docs/ZX_GRAFICOS_QL.md`) |
| `hq_tiles.py`, `hq_bits.py` | volcados de depuración de bitmaps |
| `hq_musica.py`, `hq_musica_psg.py`, `hq_psg_info.py` | música y efectos (`docs/ZX_MUSICA.md`) |
| `hq_inventario.py` | genera `docs/ZX_INVENTARIO.md` |

Para convertir los `.skool` en ensamblador: `tmp/venv/bin/skool2asm.py ref/zx/heroquest_main.skool > heroquest_main.asm`.

## 12. Qué queda por hacer / incertidumbres

- La cobertura de ejecución se obtuvo con teclas aleatorias: hay código no ejecutado que se ha clasificado por heurística (decodifica limpiamente hasta un `RET`/`JP`) y revisado a mano en los bloques de `ref/zx/notas/main/bloques.txt`; puede haber errores en bloques pequeños.
- Los comentarios a nivel de instrucción son escasos: casi todo está comentado a nivel de rutina.
- Las rutinas de dibujo de sprites con desplazamiento (`0xA5EA`...), el cálculo de camino de los monstruos (`0x9DC0`) y los eventos de cada misión (`0x8EBB-0x91F6`) necesitan más estudio.
- Los gráficos de las 8×8 de pared y suelo de la vista 3D (`0xB1C8`, tablas `0xC35A`, `0xC39A`, `0xC3B2` y `0xC3D8`) están identificados pero su composición exacta no está documentada.
- La música (partituras del banco 4) no está decodificada ni exportada.

## 13. Opciones previstas y no usadas

- **Cargar otras aventuras** (`0x651F`): el original no la usa (la ampliación Return of the Witch Lord es otra imagen completa de la cinta, `docs/ZX_AMPLIACION.md`). El código carga un bloque de cinta en `0x4000` y lo ejecuta.
- **Grabación en disco +3** (`0x65E1`) y enlace de desarrollo por puerto paralelo (`0x6900`).
