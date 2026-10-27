# Las acciones de los iconos en el original (ZX Spectrum) y su port a C

Llave (abrir puertas), ojo (buscar trampas y puertas secretas, buscar tesoros), los eventos que dejan las casillas con trampa y la muerte de un héroe. Código: `src/accion.c` (lógica, sin dibujar ni esperar) y `src/juego.c` (ventanas y flujo). Verificación: `tools/hq_oraculo_accion.py` ejecuta **las rutinas originales del Spectrum** en el simulador Z80 (interceptando el dibujo, las esperas y los dados) y `tests/accion_test.c` comprueba que el estado de después y las ventanas que habría mostrado son los mismos (`make test`: 336 casos, 4.368 comprobaciones; `tests/run_ql_accion.sh`: ocho casos compilados con c68 en sQLux). Las ventanas con zonas de clic (`VENT_AFE2`) salen de `textos_bin` (`tx_zonas`).

## Llave (`0x727E`, `0x72B5`)

Abre las **puertas cerradas** (bit 2 libre y bit 3, secreta o bloqueada, a cero) de tres sitios: la casilla del héroe (su puerta oeste y, si no la tiene, la norte), la puerta oeste de la casilla de al lado y la puerta norte de la de abajo. Cada puerta que se abre (bit 2) deja ver la sala que hay detrás (`ABRIR_PUERTA_CASILLA`, `0x860B`): pasa a ser la sala actual (`0xA984`, origen de la ventana), se marcan sus casillas como vistas en el plano (rutina `0x98FB` por las casillas de la ventana que son de la sala), se recalcula lo que ve el héroe (`0xA2E1`) y se descubren los monstruos de la sala (`0xA2C1`). No gasta puntos ni hay límite de usos; al terminar se repinta (`0xA987`).

## Ojo (`0x804E`)

Si no es el turno de Ragnar/Morcar y el héroe no ha actuado (bit 1 de `0xA98B`): marca «ha actuado» (y si ya se había movido, no podrá moverse más: bit 2 y movimiento 0) y abre la ventana **REVISAS: BUSCAS TRAMPAS, PUERTAS / TESOROS**, cuyas dos líneas son las zonas de clic de la ventana (`VENT_AFE2`: `0x8079` y `0x80A9`). **No se puede cancelar:** hay que elegir una.

### Buscar trampas y puertas secretas (`0x8079`, `0x8647`, `0x8662`, `0x8688`)

Recorre las casillas de la ventana que son de la sala del héroe y, en cada una, `0x8688`: (1) si tiene una **puerta secreta** (bit 3) sin abrir: al norte si no tiene el bit 4 y al oeste si lo tiene, la abre y deja ver lo que hay detrás; (2) las **trampas** de esa casilla —o de la que hay detrás de la puerta que se acaba de abrir, pues la rutina sigue con la casilla vecina—: el código `0x32` pasa a `0x33` (trampa visible), el `0x36` se borra y el `0x34` solo cuenta como «encontrado»; (3) la puerta secreta oeste de la casilla de la derecha y la norte de la de abajo. Si **encuentra algo** no se dice nada (se ve al repintar); si no, `NO VES NADA`. El indicador de la sala (`ind[43 + sala]`, tabla `0x6BE1`): 0 pasa a `0xFF`, `0xFF` no cambia y cualquier otro valor es una sala especial de la misión (cofre, palanca...) que se trata con su propia rutina (`0x827A`, tabla `0x82A2`; aquí el gancho `accion_especial` y el mensaje `MSG_ESPECIAL`).

### Buscar tesoros (`0x80A9`, `0x862A`)

1. Con un **monstruo visible** (bit 0 de `+6`) en alguna casilla de la sala no se puede (y no se dice nada).
2. Indicador de la sala (`ind[sala]`, `0x6BB6`): `0xFF` → `HABITACION YA HA SIDO REGISTRADA`; otro valor distinto de 0 → sala especial; 0 pasa a `0xFF` y se roba una carta del mazo (`docs/ZX_PARTIDA.md`):

| Carta | Qué pasa | ¿Vuelve al mazo? |
|---|---|---|
| 0-8 | monedas (10, 20, 25, 25, 50, 50, 50, 100, 60) y ventana `ENCUENTRAS n MONEDAS ORO` | sí |
| 9-15 | **poción**: se suma el bit de efecto a `+0x10` y sale `HAS ENCONTRADO UNA POCION DE ... CUANDO EMPLEAS ...` | no |
| 16 | `NO VES NADA` | no |
| 17-19 | trampa: `ACTIVAS UNA TRAMPA, PIERDES PUNTOS` y pierde 1 de cuerpo (si llega a 0 muere) | sí |
| 20-24 | misión 9: sale un fantasma (`ACD9`); en las demás, **un monstruo te sigue**: el monstruo 0 de la misión (el primer registro, vacío, con su tipo) aparece en la primera casilla libre de la sala entre las cuatro de al lado (arriba, derecha, abajo, izquierda; libre = sin héroe, sin objeto ni monstruo y de la misma sala), visible y listo para pelear, y sale `MIENTRAS ESTABAS BUSCANDO, UN MONSTRUO TE HA SEGUIDO Y TE ATACA`. Sin casilla libre, el indicador de la sala vuelve a 0 y se **roba otra carta** | sí |

## Eventos de las trampas (`0xA108`)

Al pisar una casilla con objeto, `OBJETO_EN_CASILLA` deja un evento pendiente (`0xA9B3`) que se trata cuando el héroe deja de animarse: **1** foso (`EL SUELO SE ABRE Y TE CAES A UN FOSO`), **2** foso si la tirada del dado de combate no es calavera, **3** lanza (`UNA LANZA SE DISPARA...`) con la misma condición; los tres acaban el movimiento del héroe (`0xA23E`) y le quitan 1 de cuerpo. **4** (código `0x34`): cae el techo (`DE REPENTE EL TECHO SE CAE ENCIMA DE TI`) y, según la misión: en la 4 se cierra con una roca (`0x31`) la casilla (25, 7) —si hay un héroe encima tira tres dados y pierde 1 de cuerpo sin ser más que 1 y, si muere, queda la roca—, en la 11 se espera a que los cuatro héroes hayan pisado la trampa (se cuentan en `0xA9B6`) y se cierra igual la casilla (0, 0), y en las demás hay un dado de 6 caras: con 4 (salvo casco y hechizo) o más pierde 1 punto (`PIERDES 1 PUNTO`). Antes de todo esto el original llama a `0x70A1` (eventos propios de las misiones 5, 9 y 11): no portado.

## Muerte de un héroe (`0x7B31`)

Cuerpo y estado a cero, bit del héroe en las máscaras de **muertos y ausentes** (la rutina `0xA254` sigue en `0xA25A`, así que marca las dos), pantalla limpia con `EL BARBARO HA MUERTO` (`GUION_BB18`...) y 200 interrupciones; si era el que jugaba termina su turno.

## Diferencias con el original

- Las esperas (200 interrupciones tras cada ventana, 150 más tras el foso y la lanza) se pueden saltar con el botón.
- La ventana de la poción pone el nombre en la segunda línea y la descripción (29 letras) en la última, en vez de partirla entre dos líneas de 22 (`tx_ventana_con`).
- Los repintados y los volcados los hace `juego_redibujar()`; el original los hace en cada ventana (`0x8D99`, `0x8DB2`).
- Las salas especiales, `0x70A1`, la bolsa, atacar y el ataque de la emboscada se añadieron después (`docs/ZX_MISIONES.md`, `docs/ZX_COMBATE.md`).
