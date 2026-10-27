# Efectos de sonido: qué suena y cuándo en el original (ZX Spectrum) y su port al QL

Complementa `docs/ZX_MUSICA.md` (datos y formato del reproductor del banco 4) y `docs/QSOUND.md` (la tarjeta). Aquí: cuándo se lanza cada efecto y cómo se han portado.

## Cómo suenan en el original

`EFECTO_SONIDO` (`0x7BA4`, `E` = número) **solo suena con la música desactivada** (`0x6FD6 = 0`): con música activada sale sin hacer nada. Pagina el banco 4 y llama al reproductor con `A = 4` (`0xDE9E`), que carga el efecto en un canal propio (`0xE2D0`) y bloquea ese canal de música (`+0x41 = 0xFF`). La interrupción de 50 Hz (`0x6F99`) sigue llamando al reproductor (comandos 0 y 1), que mezcla el efecto con la melodía 0 (silencio) que se arranca al desactivar la música en «Controles». Un efecto nuevo sustituye al que suena.

`0x7BA2` = efecto 6 (clic) y `0x7B9E` = efecto 3 (herida) son atajos. El paso y las puertas no se lanzan directamente: dejan un número en `0x727D` (1 paso, 9 puerta) y el bucle del turno (`0x6E7A`) lo toca una vez por fotograma como efecto `0x727D - 1` (0 o 8).

| Efecto | Duración | Dónde se lanza |
|---|---|---|
| 0 | 0,10 s | paso: cada desplazamiento de la animación (`0xA82A`) y el final del paso (`0x8543`) |
| 1 | 0,50 s | lanzar un hechizo (`0x7945`) |
| 2 | 0,20 s | elegir el objetivo de un ataque (`0x7383`) y el fallo en la pantalla de combate (`0x9AA3`) |
| 3 | 0,62 s | herida: la tirada de combate que acierta (`0x9AA3`), trampa (`0x81D5`), foso y lanza (`0xA126`), roca (`0xA1FC`), cofre con trampa de la misión 2 (`0x901B`) |
| 4 | 1,20 s | sin uso |
| 5 | 0,40 s | **en cada fotograma** de la tirada de los dados de movimiento (`0x6F5B`): se vuelve a lanzar cada 20 ms, así que solo suena el principio del efecto, como un zumbido |
| 6 | 0,06 s | clics: pasar turno (`0x7274`), empezar a atacar (`0x7348`), mapa (`0x7710`), bolsa (`0x788D`), ojo (`0x8063`, `0x8079`, `0x80A9`) y las acciones del panel del personaje (`0x7C65`-`0x7E6A`) |
| 7 | 1,20 s | salir por la escalera (`0x8E7D`) |
| 8 | 0,40 s | abrir una puerta (`0x7313`) |

## El port

Los efectos no dependen de nada del juego (solo del número) y suenan siempre sobre la melodía 0, que es silencio; su única salida son las escrituras al AY de cada fotograma, ya volcadas con el reproductor original en `asset/musica/psg/efecto_N.psg`. En vez de portar los 700 bytes de código del canal de efectos, **se reproducen esos volcados**: `tools/hq_efectos_empaquetar.py` los convierte en `data/efectos_bin` (1228 bytes, 234 fotogramas, diferencias fotograma a fotograma, periodos de tono y de ruido de 1,7734 MHz a los 750 kHz de la QSound) y `mus_efecto(n)` (`src/music.c`) los escribe, un fotograma por interrupción de 50 Hz, cuando la música está desactivada y hay QSound; pasado el último fotograma audible deja el chip en silencio. `tests/fx_test.c` y `tests/run_mus.sh` comprueban, con los periodos sin convertir, que los nueve efectos son idénticos al volcado del original fotograma a fotograma.

Los lugares de la tabla anterior llaman a `mus_efecto()` desde `juego.c`, `juego_combate.c` (pantallas) y, para el paso y la puerta, dejan `Vars.sonido_pend` (`0x727D`) en `turno.c`, `accion.c` y `vista.c`; el bucle de `turno_jugar()` lo toca como el original.

## Diferencias deliberadas

- Sin QSound no suena nada (el original usaba el AY del 128K; el QL de serie solo tiene `BEEP`, que se ha descartado para no interrumpir el IPC).
- Con la música activada no hay efectos (como el original). Al activarla se corta el efecto que suene.
- Los manejadores del panel del personaje suenan en cada clic sobre una zona (el original, solo cuando el clic cambia algo).
- Los periodos de ruido de la música no se convierten a 750 kHz (como hasta ahora), pero los de los efectos sí (la nota de ruido es más baja con el reloj del QL si no se convierte).
- **Tecla M durante la partida** (mejora propia, solo con QSound): alterna música → efectos → nada (`sonido_alternar()` en `src/juego.c`, también durante la tirada de dados). «Efectos» es música desactivada con `mus_efectos_activar(1)`; «nada» es música desactivada y efectos silenciados. El menú «Controles» sigue alternando solo música/no música.
