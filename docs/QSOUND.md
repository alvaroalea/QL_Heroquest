# QSound (tarjeta de sonido del QL)

Chip AY-3-8912 (como el del Spectrum 128K) con reloj de **750 kHz**, detrás de un PIA 6821.

## Detección (`src/qsound.c`, `qs_detect()`)

- La tarjeta ocupa el espacio de expansión `0xC0000`-`0xFFFFF`. Se recorren los bloques de 8 KB y se busca la cadena **«QSound»** (sin distinguir mayúsculas) entre los bytes **8 y 50** del bloque. La ROM V1.94 (`tmp/sQLux/roms/qsound_real.rom`, de `github.com/alvaroalea/QL_QsoundQprint_clone/ROM/Qsound_V1.94.rom`) la trae en el byte **10**, tras la palabra de longitud del nombre (cabecera estándar de ROM de QDOS), no en el 12.
- Dirección base de los registros = **bloque + 0x2000** (A5 del código de ejemplo): `+0` puerto A (registro o dato) y `+2` control (BC1/BDIR).
- Sin tarjeta, `g_musica` queda a 0 y la opción «2.MUSICA» de Controles no cambia. Con tarjeta arranca en ON.

## Escritura de un registro (`qs_write(reg, valor)`)

```
puerto A = reg;   control = 0x0F;  control = 0x0A     ; fija la dirección (BC1=1, BDIR=1) e inactivo
puerto A = valor; control = 0x0E;  control = 0x0A     ; escribe el dato (BC1=0, BDIR=1) e inactivo
```

`qs_silence()` pone los registros 0-13 a 0 y el mezclador (7) a `0x3F`. Al activar la música en Controles suena 0,3 s un La de 440 Hz en el canal A (periodo `750000 / (16 × 440) = 107`) para comprobar la tarjeta.

## Pruebas

- sQLux monta la tarjeta con `QSROM = qsound_real.rom` en `tmp/sqlux_hq.ini` (necesita `RAMTOP` ≤ 768). **En este equipo no hay dispositivo de audio**, y sin él sQLux deja la QSound desactivada (no suena); por eso el parche local de `tmp/sQLux/qsound.c` registra, con `SQLUX_QSLOG=1`, cada escritura de registro como `QS <reg> <valor>` en `sqlux.log`, decodificando el PIA por su cuenta. Es la referencia para comparar el reproductor con los volcados `.psg` de `asset/musica/psg/`.
- PC: `HQ_QSOUND=1` simula la tarjeta y las escrituras van a `qs_log.txt`.

## Música (`src/music.c`)

- Motor del banco 4 del original traducido a C. Datos: `data/musica_bin` (`tools/hq_musica_empaquetar.py`): los 5.000 bytes originales (melodías, instrumentos, patrones) y las tablas con **los periodos convertidos a 750 kHz** (`× 750000/1773400`) y la amplitud del vibrato escalada (27/64).
- `mus_tick()` se llama desde `plat_present()` (50 Hz). Melodía 2 en el menú y 1 en la partida; con «2.MUSICA» a OFF queda en silencio.
- **Verificación** (`tests/run_mus.sh`, en `make test`): con la tabla de periodos del Spectrum (`--zx`) el motor reproduce, registro a registro y fotograma a fotograma, los volcados `asset/musica/psg/melodia_N.psg`. En sQLux (`SQLUX_QSLOG=1`) las 17.704 escrituras de los primeros ~18 s del menú coinciden con las del PC.
- Los periodos convertidos tienen hasta un 1,3 % de error de afinación en las notas usadas (`docs/ZX_MUSICA.md`).

## La música en la interrupción de 50 Hz

Primero se movía la música desde `plat_present()` (con recuperación de los cuadros perdidos sin tocar el chip). Con un bucle de partida más lento que 20 ms eso solo deja oír uno de cada dos o tres fotogramas, y las envolventes cortas (percusión, vibrato, arpegios) suenan como **otra voz distinta**. Ahora el motor corre en la tarea de la lista de interrupciones sincronizadas, la misma que cuenta los cuadros (`tick_rut` de `src/ql_ipc.asm`):

- Cada cuadro llama a `mus_isr(ticks)` (C, `music.c`) con una **pila propia** de 2 KB (`isr_pila`) y guardando todos los registros. Si se perdiera algún cuadro se recuperan (3 como máximo, tocándolos todos).
- El programa principal toca el estado de la música solo con el cerrojo `lock` echado (`mus_start`, `mus_stop`, `mus_enable`); si la interrupción llega entonces, ese cuadro se pierde.
- Como suena mientras se dibuja, `mus_mute` no hace nada en este modo. El código del motor (`mus_frame`) no llama a ninguna rutina de la librería de C.
- `plat_present()` ya no toca la música; solo devuelve los cuadros transcurridos (para el cursor).
- **Vuelta al modo anterior** si diera problemas en un QL real: compilar con `-DNOISR` (`make ql QLFLAGS="-DQL_ASM -DNOISR"`).

Mediciones en sQLux a tiempo real (`make bench`): borrar la pantalla ~52 ms, texto ~0,16 ms por carácter (`asm_text` con una tabla de 256 entradas por pareja tinta/papel, `asm_mkpair`), fondo de la partida ~110 ms.

## Pendiente

Ensamblador del motor si el perfil lo pide, y probar el tiempo por fotograma en un QL real.
