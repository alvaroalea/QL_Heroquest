# HeroQuest ZX 128K — música y efectos de sonido

Todo lo extraído está en `asset/musica/`. Este documento describe lo que hay, cómo funciona el reproductor original y qué hay que tener en cuenta para llevarlo al QL (tarjeta QSound).

## 1. Qué hay

| Fichero | Contenido |
|---|---|
| `asset/musica/original/banco4_C000_datos_musicales.bin` | **Datos originales**: 5.000 bytes (0xC000-0xD387 del banco 4): cabecera, melodías, efectos, instrumentos y patrones |
| `asset/musica/original/banco4_D388_reproductor.bin` | Código Z80 del reproductor (0xD388-0xDC07, 2.176 bytes) |
| `asset/musica/original/banco4_DC08_tabla_periodos.bin` | Tabla de 96 periodos del AY (palabras): el índice de nota da el periodo |
| `asset/musica/original/banco4_DCC8_tabla_volumen.bin` | 96 bytes: índice → volumen, para las secuencias de volumen |
| `asset/musica/original/banco4_DD28_tabla_vibrato.bin` | 96 bytes: índice de nota → amplitud del vibrato |
| `asset/musica/original/banco4_DD88_variables.bin` | Variables y estructuras de canal del reproductor (estado inicial) |
| `asset/musica/original/banco4_DE9E_efectos_codigo.bin` | Rutina de efectos y sus variables |
| `asset/musica/original/banco4_completo.bin` | El banco 4 entero (el resto, desde 0xE400, son retratos, no música) |
| `asset/musica/partituras.txt` | Decodificación legible: tablas, instrumentos, listas de patrones y bytecode de cada patrón |
| `asset/musica/psg/*.psg` | **Volcado de los registros del AY** fotograma a fotograma de cada melodía y de cada efecto, obtenido ejecutando el reproductor original en el simulador. Formato PSG estándar (50 Hz) |
| `asset/musica/psg/resumen_psg.txt` | Duración y punto de bucle de cada volcado |

Los volcados PSG son la referencia exacta de lo que sonaba en el Spectrum; sirven para verificar un reproductor nuevo comparando registro a registro. Se regeneran con `tools/hq_musica_psg.py` y los datos y la decodificación con `tools/hq_musica.py`.

## 2. Qué se toca en el juego

- Chip: **AY-3-8912** del Spectrum 128K, puertos `0xFFFD` (seleccionar registro) y `0xBFFD` (escribir). Reloj 1,7734 MHz.
- La interrupción de 50 Hz (`0x6F99`) pagina el banco 4 en `0xC000` y llama **dos veces** a `0xD388`: con A=0 (`ACTUALIZAR_MUSICA`) y con A=1 (`ACTUALIZAR_ENVOLVENTES`).
- Entradas de `0xD388` (A = comando): 0 y 1 = los dos pasos por interrupción; 2 = empezar la melodía D; 3 = programar una bajada de volumen; 4 = lanzar el efecto E; cualquier otro = silenciar.
- El juego tiene dos formas de sonar: **música** (si se activa en «Controles», variable `0x6FD6`) y **efectos** (cuando la música está desactivada; con la música activada no suenan efectos, `0x7BA4` sale si `0x6FD6` no es 0).

### Melodías (D = número)

| Melodía | Uso | Duración de una pasada | Notas |
|---|---|---|---|
| 0 | silencio (se usa cuando la música está desactivada, y como base de los efectos) | 768 fotogramas (15,4 s) | todos los volúmenes a 0 |
| 1 | **música de la partida** (`0x6D8F`, `LD D,1`) | 20.928 fotogramas (418,6 s) | 3 canales; es la más larga |
| 2 | **menú** (`0x9CB2` y pantalla de controles, `LD D,2`) | 10.864 fotogramas (217,3 s) | 3 canales |
| 3 | **no la usa ningún código del juego** (hay datos completos) | 8.448 fotogramas (169 s) | 3 canales |

(Una «pasada» es lo que tarda en volver a repetirse el estado completo del reproductor. Las longitudes salen igual con la decodificación estática: 1744 tics × 12 fotogramas, 1552 × 7 y 1408 × 6.)

### Efectos (E = número, en `0xC00E`)

| E | Dónde se lanza | Duración audible |
|---|---|---|
| 0 | paso del personaje (pendiente en `0x727D`) | 0,10 s |
| 1 | usar un objeto o hechizo (`0x7943`) | 0,50 s |
| 2 | `0x7381` (inicio de un ataque) | 0,20 s |
| 3 | golpe o herida (`0x7B9E`) | 0,62 s |
| 4 | *(sin localizar: no hay ninguna llamada con E=4 fijo; puede salir de una tabla)* | 1,20 s |
| 5 | tirada de los dados de movimiento (`0x6F5B`) | 0,40 s |
| 6 | clic o selección en un menú (`0x7BA2`, la mayoría de las llamadas) | 0,06 s |
| 7 | evento especial, escaleras (`0x8E7B`) | 1,20 s |
| 8 | abrir puerta o trampa (`0x727D` = 9) | 0,40 s |

(Los usos se han deducido de las llamadas; los nombres de las acciones son probables.)

## 3. Formato de los datos

Todas las direcciones son del banco 4 (org `0xC000`); las palabras están en little-endian.

**Cabecera** (`0xC000`): tres palabras: tabla de melodías (`0xC006`, 4 palabras), tabla de efectos (`0xC00E`, 9 palabras a partir de `0xC020`) y tabla de instrumentos (`0xC1C1`, 10 bytes cada uno).

**Melodía**: tres palabras (canales A, B y C), cada una apunta a una **lista de patrones**: palabras con la dirección de cada patrón; `0x0000` = volver al principio de la lista (la melodía se repite) y `0xFFFF` = fin. Las listas de la melodía 1 tienen unos 30 patrones por canal.

**Patrón** (bytecode que interpreta `0xD7C5`):

| Byte | Significado |
|---|---|
| `0x00`-`0x5F` | nota: índice en la tabla de periodos `0xDC08` más la transposición; dura la duración actual |
| `0x60` | fin de bucle |
| `0x61 n` | inicio de bucle (n veces) |
| `0x62 dir` | llamada a un subpatrón (se vuelve con `0xFF`) |
| `0x63 dir` | salto |
| `0x64 n` | transposición (en semitonos, con signo) |
| `0x65` | marca (pone `0xDD8D` a `0xFF`) |
| `0x80`-`0xBF` | duración = valor − 0x7F tics |
| `0xC0`-`0xDF` | seleccionar el instrumento n − 0xC0 |
| `0xE0 a b c` | deslizamiento de tono (glissando) |
| `0xE1`-`0xEF` | volumen máximo del canal |
| `0xF0`-`0xFE` | tempo: valor − 0xEF fotogramas por tic |
| `0xFF` | fin del patrón (o retorno de un subpatrón) |

**Instrumento** (10 bytes, `0xD99E`): `+0,+1` ruido (duración e índice de periodo de ruido), `+2,+3` vibrato (velocidad y amplitud en el nibble bajo/alto de `+2`, retardo en `+3`), `+4,+5` secuencia de tono (arpegio: valores que se suman al índice de nota; `0x80` fin, `0x81 n` bucle), `+6,+7` secuencia de **ruido** (valores que se suman al índice y dan el periodo de ruido de la tabla `0xDCC8`, que va al registro 6 del AY; mismas marcas), `+8,+9` lista de pares (duración, nivel) que fija el volumen del canal hasta `0xFF`. (Corrección posterior: `+6,+7` no es una secuencia de volumen; el volumen sale solo de la lista `+8,+9`. Motor portado y verificado en `src/music.c`.)

No se usan los registros de envolvente hardware del AY (11 a 13 siempre a 0): toda la dinámica es software, un valor de volumen por canal y fotograma.

**Efecto**: mismo bytecode de un solo canal con su propia estructura (`0xE2D0`); se mezcla con la música en el mezclador (registro 7).

El estado de cada canal (estructura de 0x42 bytes en `0xDDD8`, `0xDE1A`, `0xDE5C`) está descrito en las notas del `.skool` (`ref/zx/heroquest_banco4_sonido.skool`).

## 4. Notas para el QL (QSound)

- Si la tarjeta **QSound** está presente tiene un AY como el del 128K (el usuario indica que con reloj de **750 kHz** en lugar de 1,7734 MHz): hay que detectar la tarjeta y, si no está, el juego debe seguir sin música (o usar `BEEP` solo para efectos).
- **Periodos de tono**: frecuencia = reloj / (16 × periodo). Para conservar las notas hay que convertir cada periodo: `periodo_QL = periodo_ZX × 750000 / 1773400 ≈ periodo_ZX × 0,423`. Los periodos del Spectrum van de 3823 a 16, los del QL de 1617 a 7: todo cabe en los 12 bits del registro. El error de afinación máximo es del 0,16 % hasta el índice 36 (A#3), del 0,3 % hasta el 48 (A#4), del 0,7 % hasta el 60 (A#5), del 1,5 % hasta el 72 (A#6), del 3,3 % hasta el 84 y del 5,7 % en los índices más agudos (90-95, a partir de unos 5 kHz). Según el recorrido estático de los patrones, las melodías usan **índices de nota entre 12 y 67** (melodía 0: solo 36; 1: 12-67; 2: 14-55; 3: 12-64; sin contar arpegios, deslizamientos ni vibrato), así que el error de afinación de la música real es de **≈1,3 % como máximo** y no merece la pena compensarlo. Se recomienda convertir **la tabla `0xDC08`** (96 valores) una sola vez y dejar el bytecode intacto.
- **Ruido**: el periodo de ruido (registro 6, valores 0-27 en los volcados) se convierte con el mismo factor, redondeando (resultado 0-12).
- Los volúmenes (registros 8 a 10, 0-15) y el mezclador (registro 7) no cambian.
- El reproductor necesita **50 interrupciones por segundo** (dos llamadas por interrupción); el QL PAL tiene interrupción de fotograma a 50 Hz.
- Caminos posibles para la conversión: (a) **portar el motor** (≈2 KB de código Z80, sin envolventes hardware) a 68000 y usar los datos originales con la tabla de periodos convertida; (b) reproducir los volcados PSG (referencia exacta, pero 286 KB la melodía 1 y la conversión de periodos ha de hacerse registro a registro). La opción (a) es más compacta y se puede verificar contra los PSG.
