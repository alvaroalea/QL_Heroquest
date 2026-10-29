# HeroQuest para Sinclair QL

[![English](https://img.shields.io/badge/Language-English-blue)](README.en.md)
[![Español](https://img.shields.io/badge/Language-Español-red)](README.md)

Port del videojuego **HeroQuest** (Gremlin Graphics, 1991; versión de ZX Spectrum 128K) al **Sinclair QL**. Escrito en C89 (XTC68/`qcc`) con las partes críticas en ensamblador 68000, en modo 8 (256×256, 8 colores). Incluye las 14 misiones del juego base, en español y en inglés, y puede cargar la ampliación *Return of the Witch Lord*.

![Captura 1](screenshot_1.png)
![Captura 2](screenshot_2.png)

## Qué es y cómo se ha hecho

No es una emulación del Spectrum: es una reescritura que se apoya en el análisis del original.

1. **Análisis del original.** Se desensambló y se comentó el código del Spectrum (`ref/zx/`, `docs/ZX_*.md`): estructuras de datos, misiones, gráficos, sonido y reglas.
2. **Port por capas.** Lo que depende del hardware (pantalla, teclado, ratón, sonido, disco) va en una capa de plataforma (`src/plat_ql.c`, `src/plat_host.c`). El resto de la lógica (partida, vista isométrica, turnos, combate, hechizos, monstruos, misiones) es C portable, y existe también una versión de PC para probarla.
3. **Verificación contra el original.** Las rutinas portadas se comparan con las del Spectrum ejecutadas en un simulador Z80 (los «oráculos» de `tools/hq_oraculo_*.py`): decenas de miles de comprobaciones en `make test`. Las pruebas de la versión de QL se repiten en el emulador sQLux.
4. **Datos propios.** Los gráficos, textos y misiones se extraen de las cintas originales y se reempaquetan en el formato del QL. Las carpetas `asset/graficos_ql/`, `asset/textos_ql/` y `asset/misiones_ql/` contienen las versiones retocadas o traducidas a mano, y la compilación las prefiere a las originales.
5. **Ensamblador y mejoras al final.** Primero que funcione y sea idéntico; después, perfilado y paso a ensamblador de lo lento.
6. **Bugs y secretos.** Se solventaron varios bugs y errores tipográficos (especialmente en la versión española, por ejemplo, ahora se distingue al elfo del goblin) y se encontró algún secreto, la melodía que suena en la pantalla de selección de idioma ¡¡estaba escondida en el juego original de Spectrum!!.
7. **Ampliaciones.** El port se ha diseñado de forma que sea fácil crear y cargar grupos de misiones, el sistema las reconocerá en el menú Cargar Otras Aventuras, quizá en el futuro podamos ver La torre de Kellar, El mago del Espejo y el resto de aventuras que salieron en tablero. 

Notas, hallazgos y decisiones están en `docs/` (empezar por `docs/PORT_QL.md`).

## Características técnicas

- **CPU y pantalla:** 68008 a 7,5 MHz, sin FPU; modo 8 (256×256, 8 colores). El juego pone el modo 8 al arrancar y vuelve al modo 4 al salir, sin el mensaje «Press a key to exit».
- **Memoria:** necesita unos **370 KB** libres como mínimo (código 90 KB, 40 KB de datos del trabajo y unos 236 KB de montón con los gráficos y la partida). Con RAM de sobra (unos 435 KB) precarga todas las misiones y los retratos; con menos, los lee del disco cuando hacen falta. Con 640 KB funciona (probado). No cabe con el *Pointer Environment* cargado en 640 KB.
- **Soporte:** disco de 720 KB (`build/heroquest.img`) o volumen QXL.WIN de 4 MB (`build/heroquest.win`).
- **Ratones** (opción 1 de la pantalla de controles): teclas de cursor, **QIMSI** (se activa solo si la detecta), **QIMI** y el del **Pointer Environment**. Solo la opción de teclado y la QIMSI en emulador están probadas; las demás, **sin probar en hardware**.
- **Sonido:** música y efectos con la tarjeta **QSound** (AY-3-8912); se detecta sola. Sin ella el juego va sin sonido.
- **Idiomas:** español e inglés, a elegir en la primera pantalla y cambiable en la pantalla de controles.
- **Unidad y directorio de los datos:** el fichero `boot` es un programa SuperBASIC que define `DIR$ = "FLP1_"`, y de ahí cargan todos los ficheros (datos, héroes guardados). Para ejecutarlo desde otra unidad o directorio, edita `boot` y cambia esa línea, por ejemplo `DIR$ = "WIN1_HQ_"` o `DIR$ = "FLP2_"`. El código fuente de `data/boot` es texto; hay que volver a escribirlo en el disco con ese nombre (`qltools img -W boot`). Sin `DIR$`, el juego usa el directorio de datos de Toolkit II y, si no, `flp1_`.

## Cómo jugar

Arranca desde el disco (`LRUN flp1_boot`, o con el arranque automático del `boot`). Escoge idioma y usa el menú principal: **Jugar**, **Comprar equipo**, **Crear personaje**, **Controles** y **Cargar otras aventuras** (por ejemplo `witchlord`).

- **Crear personaje:** cambiar el nombre, cargar y guardar el héroe en el disco, y activar o eliminar a cada héroe (para jugar con menos personajes o para revivir a los que murieron en una partida anterior).
- **Comprar equipo:** armas, armaduras y caja de herramientas con el oro del héroe.
- **Jugar:** elige misión, lee el objetivo y escoge los grupos de hechizos (el mago elige 3 grupos si juega solo, o 1 si también juega el elfo). Cada héroe tira los dados, mueve el cursor y hace clic en las casillas y en los iconos (siguiente personaje, mapa, llave, ojo, atacar y bolsa). Después juegan los monstruos de Morcar.
- **Teclas:** `O`/`K` arriba/abajo, `Z`/`X` izquierda/derecha, `ESPACIO` botón; los cursores también, si la opción de teclas de cursor está activa. `ESC` vuelve atrás (en la partida, pide confirmación para abandonar la misión).

## Cómo compilarlo

Necesitas Linux (se ha usado Debian), `make`, `gcc`, Python 3 y:

- **XTC68** (`qcc`, `as68`, `qld`) para compilar para el QL.
- **qltools** (imagen de disco) y **qxltool** de Norman Dunbar (volumen `.win`).
- **SkoolKit** en un entorno virtual: `python3 -m venv tmp/venv && tmp/venv/bin/pip install skoolkit`.
- **sQLux** para probarlo (`make run`). Ver `docs/HERRAMIENTAS.md` para instalarlo y para el parche local de capturas y teclas.

### 1. Extraer los datos de las cintas originales

Los TZX originales son la fuente de todo; deben estar en `ref/` con estos nombres:

- `Hero Quest (Dro Soft).tzx` (versión española) y `Hero Quest - 128k.tzx` (versión inglesa).
- Opcional, para la ampliación: `Return Of The Witch Lord (Dro Soft).tzx` y `Return Of The Witch Lord - Side 1.tzx` / `Side 2.tzx`.

Después:

```sh
tmp/venv/bin/python -I tools/tzx_to_z80.py "ref/Hero Quest (Dro Soft).tzx" asset/extraido
tmp/venv/bin/python -I tools/tzx_to_z80.py "ref/Hero Quest - 128k.tzx" asset/extraido_en
tools/hq_ql_datos.sh extraer
```

La primera línea saca los bloques y bancos de memoria de la cinta española. La segunda hace lo mismo con la inglesa. La tercera extrae gráficos, textos y misiones a `asset/` y copia a las carpetas `*_ql` lo que falte, sin sobrescribir nunca lo que ya esté retocado. Los pasos y las opciones de la ampliación están en `docs/ZX_AMPLIACION.md` y `docs/ZX_VERSION_EN.md`.

### 2. Compilar

```sh
make data     # empaqueta asset/ en data/ (se hace solo si cambia algo)
make ql       # build/heroquest, el ejecutable de QDOS
make img      # build/heroquest.img, disco de 720 KB con juego, datos y boot
make win      # build/heroquest.win, volumen QXL.WIN de 4 MB
make run      # arranca la imagen en sQLux
```

`make` a secas genera `img` y `win`. Otras órdenes: `make host` (versión de PC con capturas), `make test` (pruebas contra el original) y `make c68_check` (comprobación con enteros de 16 bits).

## Licencia

GNU GPL v3 (ver `LICENSE`). El juego original, sus gráficos, textos y música pertenecen a sus autores; este proyecto es un port sin ánimo de lucro.
