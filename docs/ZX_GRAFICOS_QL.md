# Gráficos del port a QL: extracción, retoque y empaquetado

Flujo de trabajo (decidido con el usuario): los gráficos originales del Spectrum se extraen a PNG de 8 colores con transparencia; el usuario retoca esos PNG con un editor para crear las versiones del QL; un empaquetador los convierte al formato de color del QL (modo 8) con su máscara.

```
asset/graficos/      <- extracción (tools/hq_gfx_extraer.py): se puede regenerar siempre
asset/graficos_ql/   <- versiones retocadas por el usuario: NINGÚN script las sobrescribe
data/                <- binarios para el QL (tools/hq_ql_empaquetar.py): sprites_bin, escenario_bin, retratos_bin, marco_bin, baldosas_bin, fuente_normal_bin y fuente_manuscrita_bin
build/datos/         <- índices de texto para el PC
```

| Paso | Comando | Qué hace |
|---|---|---|
| 1 | `tmp/venv/bin/python tools/hq_gfx_extraer.py` | Escribe los 131 PNG originales en `asset/graficos/` (+ `indice.txt`). No toca `asset/graficos_ql/` (se niega a escribir ahí) |
| 2 | `tmp/venv/bin/python tools/hq_ql_inicializar.py` | Copia a `asset/graficos_ql/` los PNG que **aún no existan** allí; nunca sobrescribe. Con `--estado` solo lista los que faltan. Al final indica cuántos están retocados |
| 3 | (el usuario) | Retoca los PNG de `asset/graficos_ql/` con su editor |
| 4 | `tmp/venv/bin/python tools/hq_ql_empaquetar.py [--desplazados] [--verificar]` | Empaqueta en `data/` (nombres sin punto: `sprites_bin`...). Para cada gráfico usa el PNG de `graficos_ql/` si existe y, si no, el original |

## Reglas para los PNG

- Tamaño: **no cambiar el ancho ni el alto** (el código del juego depende de él). El ancho se redondea por arriba a un múltiplo de 4 puntos al empaquetar.
- Colores: los 8 del QL en modo 8: negro `(0,0,0)`, azul `(0,0,255)`, rojo `(255,0,0)`, magenta `(255,0,255)`, verde `(0,255,0)`, cian `(0,255,255)`, amarillo `(255,255,0)` y blanco `(255,255,255)`. Cualquier otro color se asigna al más próximo; el empaquetador avisa si alguno está lejos de la paleta.
- Transparencia: un punto con **alfa < 128 es transparente**; el resto, opaco. **La máscara se deduce de ahí.** El PNG puede ser RGBA, RGB, paleta (con `tRNS`) o de grises, de 1 a 8 bits, sin entrelazado.
- Qué traen los PNG originales:
  - `sprites/` (66 personajes y monstruos, más el cofre y el trono, y `cursor`): tinta negra, halo blanco de 2 puntos a cada lado (el juego lo deduce de una tabla) y el resto transparente.
  - `escenario/`: paredes, puertas, suelo, escaleras y piezas base: tinta negra y el resto transparente (el juego las compone con OR). Los **muebles** (17) son sprites con halo blanco.
  - `retratos/` (14, de 128×64) y `marco/` (marco de pantalla y pantalla de carga, 256×192): opacos y con los colores de sus celdas de atributos (sin el brillo).
  - `fuentes/`: `baldosas` (32 baldosas de 8×8), `fuente_normal` y `fuente_manuscrita` (68 caracteres de 8×8, en una hoja de 17 columnas): negro sobre transparente; cualquier punto opaco cuenta como «1».
  - `fuentes/baldosa_niebla.png` (8×8, solo en `asset/graficos_ql/`): la baldosa 33 del plano, el icono de la niebla de la muerte de la ampliación; se empaqueta detrás de la marca en `marca_activo_bin` (16 bytes). Es una copia de la baldosa 27 hasta que se dibuje la voluta.
- En el juego solo se usan la **fuente activa** (la normal por defecto; la manuscrita es la que se activa con «3: MANUSCRITO» en Controles) y las **baldosas del mapa** (`0xB1C8`).

## Formato de los binarios (`data/`)

**Los nombres de fichero del QL no llevan punto**: `sprites.bin` del PC es `sprites_bin` en el QL.

**Marco y pantalla de carga** (`marco_bin`): entrada 0 = `marco.png`, ahora de **256 × 256** (la versión retocada en `asset/graficos_ql/marco/`; las 32 líneas de arriba y de abajo no se usan y quedan en negro) y se vuelca en la línea 0; si volviera a ser de 256 × 192 el juego la centra (`(256 - alto) / 2`). `marco_bin` lleva solo esa entrada: la pantalla de carga (`pantalla_carga.png`, 256 × 192) ya no va en el paquete (el juego no la usaba y ocupaba 24.576 bytes de montón). El empaquetador la escribe solo como **`heroquest_scr`**: la pantalla de carga como memoria de pantalla del QL (32.768 bytes, 128 por línea, centrada: líneas 32 a 223), que el `boot` vuelca a `0x20000` antes de lanzar el juego:

```
110 MODE 8
120 LBYTES flp1_heroquest_scr,131072
130 EXEC_W flp1_heroquest
140 MODE 4
```

El juego, al arrancar, no cambia el modo si ya es el 8 ni borra la ventana (se vería desaparecer la pantalla de carga): se queda hasta que el menú dibuja la primera pantalla. Al salir, el `boot` vuelve al modo 4.

Gráficos con color: `sprites_bin`, `escenario_bin`, `retratos_bin` y `marco_bin`, en big endian (el del 68000). Los índices con nombre, desplazamiento y tamaño de cada gráfico están en `build/datos/indice_<categoría>.txt`.

```
+0   'HQGF'        +4  versión (word, 1)         +6  número de gráficos N (word)
+8   N entradas de 16 bytes:
       desplazamiento desde el inicio del fichero (long)
       ancho en puntos (word)   alto (word)   palabras por fila (word)
       copias (word: 1, o 4 con --desplazados)   banderas (word: bit 0 = lleva máscara)   reservado (word)
...  datos de cada gráfico
```

Datos: por cada copia y cada fila, por cada grupo de 4 puntos (una palabra de pantalla del QL): `máscara_par, máscara_impar` (solo si la bandera lo indica) y `color_par, color_impar`.
- Color: formato de pantalla del modo 8 (`docs/HARDWARE_QL.md`): para el punto k (0 a 3, de izquierda a derecha), byte par: verde en el bit `7-2k` y parpadeo (siempre 0) en `6-2k`; byte impar: rojo en el bit `7-2k` y azul en el `6-2k`.
- Máscara: los dos bits de cada punto opaco a 1 en los dos bytes. Dibujo: `pantalla = (pantalla AND NOT máscara) OR color`. Los gráficos totalmente opacos (retratos y marco) no llevan máscara.
- `--desplazados` guarda 4 copias (desplazadas 0, 1, 2 y 3 puntos a la derecha, con una palabra más por fila) de los sprites de personajes y de los muebles, para no tener que desplazar al dibujar. Cuesta 4 veces más datos: sprites 60 KB → 277 KB y escenario 56 KB → 165 KB.

Fuentes y baldosas: `baldosas_bin` (256 bytes), `fuente_normal_bin` y `fuente_manuscrita_bin` (544 bytes): **1 bit por punto, 8 bytes por carácter**, como en el original; el color se pone al dibujar.

Tamaños actuales (sin desplazar): sprites 60.256 bytes, escenario 56.024, retratos 41.128 y marco 32.792 (antes 49.192 con la pantalla de carga).

## Gráficos añadidos para la vista

Al final de `escenario_bin`, índices 51 a 58: `suelo_0` a `suelo_7` (48×24, a color y con máscara; **solo existen en `asset/graficos_ql/escenario/`**, los añade el empaquetador y el número es el paper del atributo de la sala; se usan 0, 3, 4, 5, 6 y 7). Los índices 49 y 50 son `bloqueo` (rombo de las casillas bloqueadas, códigos de objeto `0x30` y `0x31`, datos `0xFF79` del Spectrum) y `trampa` (código `0x33`, datos `0xFF01`); el original los dibuja con las rutinas del suelo. `marca_activo_bin` (8 bytes) es la baldosa `0xB1C0` que marca a los personajes en el plano al elegir objetivo. La geometría de la vista (posiciones, salas, desplazamientos de puertas y muebles) no son gráficos: está en `vista_bin` (`tools/hq_vista_empaquetar.py`).

## Notas

- Las rutinas del original dibujan los sprites con corrimiento de 1 punto en horizontal; en el QL un punto es de 2 bits, por lo que el desplazamiento horizontal de un sprite no es un simple `SHIFT` de bytes: de ahí `--desplazados`.
- El halo blanco de los PNG originales se calcula sobre toda la fila; el original lo calcula byte a byte (su máscara se pierde en los bordes de cada byte), por lo que los PNG son ligeramente «mejores» que el original en esos bordes.
- El cursor del original lleva la máscara guardada (a +0x30 del gráfico) y trae 2 copias (una desplazada 4 puntos); en el PNG solo está la primera.
- Pantallas del QL: ver `docs/DECISIONES.md` (256×192 centrado con 32 líneas negras arriba y abajo).
