# Hallazgos sobre el juego original

Comportamientos y fallos del HeroQuest de ZX Spectrum que hay que tener en cuenta al portar. Consultar antes de tocar una rutina.

## Carga de la misión (`docs/ZX_PARTIDA.md`)

- **`0x5CE7` no vuelve**: el intercambio de equipo de la misión 5 no tiene `RET`; cae en el código siguiente (tabla de máscaras de sprites y `JP 0x9542`) y el `RET` del barajado vuelve al `CALL` del héroe siguiente. Se ejecuta cuatro veces el código que reconstruye la tabla `0x6800` y se baraja el mazo cuatro veces. Resultado final correcto; el port lo hace una sola vez.
- **El mazo de tesoros sólo tiene 32 barajados posibles** y el barajado es débil: `0x9542` hace entre 128 y 159 pasadas de intercambios de vecinos, decididos por el bit 3 de bytes de la ROM del Spectrum desde una posición elegida con los 5 bits útiles de `R >> 3`. Las cartas quedan cerca de su posición inicial. El port usa el mismo algoritmo con otra fuente de bits (`mazo_barajar_azar`); con los bits de la ROM da los mismos resultados.
- Las marcas de héroes del menú (`0x8FE4-0x8FE7`) **se ponen a cero al empezar una misión**; sólo vuelven a `0xFF` los héroes que llegan a una casilla especial (`0x8E7B`, que pone `0xFF` en la marca del personaje activo; *probable*: la salida de la misión). Un héroe que muere no vuelve a estar disponible. El port lo tendrá que decidir al terminar la misión (pendiente).
- Mision 7 («Mago del fuego»): al empezar se quitan los hechizos de fuego al elfo y al hechicero (`+0x0C &= 0x38`).

## Dados

- `DADO` (`0x9BD3`) usa el registro R del Z80 y el acarreo que deja el código anterior: resultado uniforme sólo si R lo es. El port usa un generador congruencial.

## Vista y figuras (`docs/ZX_VISTA.md`)

- **Registros de animación**: la tabla tiene 14 registros (se borra, se ordena y se dibuja con 14) pero la búsqueda de un registro libre (`0x8A2D`) recorre 13 desde el 4, es decir, hasta el 16: los dos o tres últimos están fuera de lo que se dibuja, y el 13 es el del quinto personaje (Ragnar), que una sala con diez figuras o más pisaría.
- **Orden de dibujo**: la tabla `0xFD00` tiene 96 bytes pero solo se dibujan los 90 primeros; una figura que por colisión de claves acaba en el puesto 90 o 91 no se ve.
- **`0xA9A1`** no es «solo el activo» sino «marcas»: el plano pinta como marca (`0xB1C0`) a los monstruos y a los demás personajes, omite al personaje activo y no pinta objetos (se usa al elegir objetivo del ataque).
- **Rombos con datos fuera del escenario**: los códigos de objeto `0x30`, `0x31` (casilla bloqueada) y `0x33` (trampa) dibujan rombos que están en `0xFF79` y `0xFF01`, en medio de las tablas de interrupciones y datos; no son suelo ni escalera (`docs/ZX_GRAFICOS_QL.md`).
- **Leer pasada la capa A**: con la ventana de la vista en las últimas filas (salas 1 a 20 con la fila 15) y en las columnas 20 a 22, el original lee casillas pasada la 511, es decir, los bytes de la capa B.
- **Puertas con halo**: las puertas que van como figura (`0x8A99`) se dibujan con la misma máscara AND que los sprites, no con OR como las directas.
- **Sala y color**: el color del suelo de cada sala es el paper del byte de atributo de la tabla `0xC47C`; 0x78 (blanco) en los pasillos. Solo se usan 6 papers: negro (sala 0), magenta (3), verde (4), cian (5), amarillo (6) y blanco (7); nunca azul ni rojo. El bit de brillo (bit 6) lo llevan `0x58`, `0x60`, `0x68` y `0x70`; el port lo ignora.
- **Halo y desplazamiento de héroes**: el original aplica la máscara después de desplazar 4 puntos a los héroes, byte a byte; el halo calculado sobre los PNG da hasta unos 80 puntos de diferencia en una vista completa (solo dentro del rectángulo de las figuras).

## El turno y el movimiento (`docs/ZX_TURNO.md`)

- **La tirada cambia en cada fotograma**: `TIRAR_DADOS` vuelve a tirar los dados de movimiento en cada vuelta del bucle, así que el número que se queda es el que había en pantalla al pulsar el botón.
- **Una puerta solo se cruza una vez por turno**: `0xA9BF` guarda la casilla de la última puerta atravesada y el paso se cancela si se vuelve a esa misma puerta (se pone a cero al empezar cada turno).
- **Abrir una puerta es andar hacia ella**: una puerta cerrada y no bloqueada (bit 3) se abre sola al intentar pasar (bit 2) y el héroe pasa en ese mismo paso. En la misión 4 abrir la puerta de la columna 5, filas 3 y 4, hace visible al monstruo 1 (`EN_ZONA_ESPECIAL`).
- **`IR_HACIA_CURSOR` divide un número de 16 bits sin signo**: para las posiciones de la vista fuera de la ventana (45 a 51) el desplazamiento es negativo y la casilla que sale es absurda (fila de 8 bits cualquiera): solo se mueve si, por casualidad, queda en línea con el héroe. El port lo repite tal cual.
- **Cursor y evento pendiente**: mientras hay un evento sin tratar (`0xA9B3 != 0`) el cursor no se mueve.
- **Pisar un objeto no detiene el paso**: `OBJETO_EN_CASILLA` (`0xA0E6`) cambia el código de la casilla de destino, deja el evento y cancela el camino automático (`JP 0x82C6` vuelve al manejador, que sigue y da el paso).
- **Escaleras**: solo los códigos `0x38`-`0x3B` sacan al héroe; el código `0x32`-`0x37` son objetos/trampas.
- **El quinto personaje usa el registro 13** de animación (Ragnar) y los dados de un monstruo (1 dado).
- **Mientras se anima un paso no se atienden clics** (`0xA988` y registro en estado 2) y `MOVER_AUTOMATICO` espera a que termine.
- **Teletransporte en la misión 9**: pisar una casilla sin sala (`FIN_PASO`) lleva al héroe a uno de once destinos al azar (dos dados sumados, distinta sala de la actual) y abre la puerta correspondiente.

## Acciones de los iconos (`docs/ZX_ACCIONES.md`)

- **`MARCAR_MUERTE` (`0xA254`) marca los dos mapas**: no vuelve tras `CALL 0xA25D` y sigue en `0xA25A`, así que un héroe muerto o que sale por la escalera queda también en `0xA9B6` (ausentes). El port repite esto (`pers_marcar_muerte`).
- **El ojo no se puede cancelar**: la ventana de `REVISAS...` solo tiene las dos zonas de las líneas; cualquier otro clic se ignora (la tabla termina en `0xFE`).
- **Abrir una puerta secreta cambia la casilla que se mira después**: al abrirla, `0x8688` sigue con la casilla de detrás (norte u oeste) para las trampas, no con la propia.
- **Registrar una sala dos veces es silencioso para las trampas** (el indicador `0xFF` no impide volver a buscar) pero no para los tesoros (`HABITACION YA HA SIDO REGISTRADA`).
- **Las cartas de oro, trampa, fantasma y monstruo vuelven al fondo del mazo; las pociones y `NO VES NADA` se gastan**. Con 7 pociones y una carta vacía, el mazo se va acortando con el tiempo.
- **El monstruo que te sigue es siempre el primer registro de monstruos de la misión** (vacío en el fichero de la misión, con su tipo), por eso el mismo monstruo cambia de sitio si te sigue otra vez.
- **Las rocas de las misiones 4 y 11 tiran tres dados y no los usan**: el daño es siempre 1 (`LD E,1` sustituye al resultado de `0x7B1C`) y, si el héroe sobrevive, la roca ni siquiera queda (solo se pone la roca cuando muere).
- **En la misión 11 `0xA9B6` cuenta los héroes que han pisado la trampa de la roca** (reutiliza la máscara de ausentes).

## Combate, bolsa y hechizos (`docs/ZX_COMBATE.md`)

- **`DADO` deja en A el valor de R, y dos rutinas miran A pensando que es la cara**: `BUSCAR_TRAMPAS` (`0xA026`, la caja del enano y de los demás) hace `CALL 0x9BC8; OR A; JR NZ` y por tanto desactiva la trampa **salvo si R vale 0** (1 de cada 128): la cara que sale no cuenta. `0x9D7E` (turno de los monstruos) hace `CALL 0x9BD3; CP 5` con el mismo error. El port lo repite con `dado_r()` (el R de la última tirada).
- **Con la vara de recuerdo, `0x7D1F` guarda en `0xA98B` el valor de `0xA9A4`**: el segundo hechizo del turno toma `0xA9A4` (marca de sala registrada) como estado del turno (con `| 2` y, si el bit 0 vale 1, `| 4` y movimiento a cero).
- **`PANEL_COMBATE` (`0x9921`) cambia el personaje activo** (`0xA998` y `0xA9DD`) por el que se defiende: si ese héroe muere, `0x7B31` ve que es el activo y termina el turno (`0xA98A = 1`) aunque sea el turno de Morcar.
- **Defensa 0 = 256 dados**: la tirada de defensa del monstruo (`0x99BB`, `DJNZ` con B = 0) lanza 256 dados si su valor de defensa es 0; ningún tipo de la tabla lo tiene, pero el port lo repite.
- **`PREPARAR_ATAQUE` no limpia `0xA9A8`** si el héroe no lleva arma ni vara ni espada: el alcance y la pérdida del arma arrojadiza usan el valor de la tirada anterior.
- **El agua santa solo destruye lo que está en casillas de la sala sin marcas**: compara el byte entero de la capa A con el número de sala (los bits 6 y 7, de lo ya visto, hacen que no coincida) y usa la casilla de origen de la ventana de la vista.
- **Un monstruo muere de un golpe**: no tienen puntos de cuerpo; los dos nibbles de su `+7` son solo los números que muestra el panel (y el alto se borra al morir). Los jefes (tipos de atributos 11 y 12) son inmunes a las armas sin la espada espiritual (0 dados) y a los hechizos (`BAAC` sin gastar el hechizo).
- **La segunda tirada de defensa**: `0x996B` vuelve a llamar a `0x7596` después de tirar; como las pociones y hechizos de defensa ya se gastaron en la primera llamada, solo repite los números que se muestran.
- **Genio con un héroe de blanco** daña igual que sobre un monstruo, pero mata a un monstruo de un golpe y muestra «el hechizo inflige 1 punto».
- **El rótulo de la lista de objetos del panel** del original es `BUSQUEDA TESORO` (el de la caja `0xB79E`), no «objetos».
- **Cursor y hechizos**: no hay forma de cancelar la elección del blanco de un hechizo (se queda en el bucle hasta elegir uno válido).
- **`A999 == 1` nunca ocurre**: `ATACAR` (`0x73B2`) tiene una salida para ese valor del alcance (`JP Z,0x8009`) que ninguna rutina devuelve.

## Turno de los monstruos (`docs/ZX_MONSTRUOS.md`)

- **Un monstruo dormido casi nunca despierta**: `0x9D7E` hace `CALL 0x9BD3; CP 5` y compara `A` (el R del Z80), no la cara del dado: despierta solo si R vale 5 (1 de cada 128 turnos).
- **Los monstruos no tienen límite de movimiento**: `0x9EAF` y similares hacen `DEC A; LD (0xA98C),A; JP C,0x9F73` esperando un acarreo que `DEC` no produce, así que el camino sigue hasta llegar al héroe (siempre dentro de su sala). El movimiento de la tabla `0xAA8C` no se usa. El código «1» (se agota el movimiento) no se da nunca.
- **Un monstruo solo actúa si llega a un héroe**: si no hay camino hasta ninguno (otra sala, velo de niebla, obstáculos) se queda quieto.
- **`0xA99A` y `0xA9AD` quedan como estaban en la última prueba**: se comprueban en el oráculo (el último héroe probado, 4 si no llegó a ninguno).
- **`0x9DFC` salta a `0xA999`** (datos) si la casilla del héroe no es de la sala del monstruo; no puede ocurrir porque `+3` se comprueba antes.
- **El buffer de caminos `0x6B8A` mide 44 bytes** y detrás están los indicadores de la misión (`0x6BB6`); un camino de más de 22 pasos los pisaría (dentro de una sala no pasa).

## Misiones, salas especiales y salidas (`docs/ZX_MISIONES.md`)

- **Los héroes que no salen vivos se pierden**: las marcas `0x8FE4`-`0x8FE7` se ponen a cero al empezar la misión y solo `0x8E7B` las vuelve a poner; el menú no vuelve a ofrecer al héroe muerto (o que no llegó a la escalera).
- **Misión 1: el primero que sale cobra 100 monedas y los demás no** (contador `0xA9AE`); en la 6 cobran todos.
- **Misión 3, «ir a por más oro»**: el héroe vuelve a entrar en (3, 11) con el turno sin terminar, pero conserva los bits de ausente y muerto de `0xA9B6`/`0xA9B7` que le puso la salida, y la posición se pone sin actualizar la casilla (`+4`, `+5`).
- **Sala especial 2 y 4**: miran `(IX + 0x2B)`, el indicador de trampas de la misma sala; si el indicador está en la tabla de trampas, leen bytes de los monstruos (el original no lo usa así).
- **Sala especial 7**: la casilla que se limpia depende de un contador (`0xA9AF`) que no se comprueba: con más de tres usos leería una tabla inexistente.
- **La emboscada del ojo ataca**: el monstruo que sale al buscar tesoros (`0x81B1`) ataca con la pantalla de combate nada más aparecer. El port de los pasos anteriores solo mostraba la ventana.
- **Evento de la misión 11**: `0xA9B1` no se borra (se evita repetir con `0xA9B4`), así que `EVENTOS_MISION` se evalúa en todos los fotogramas.

## Efectos de sonido (`docs/ZX_SONIDO.md`)

- **Con la música activada no hay efectos**: `0x7BA4` sale si `0x6FD6 != 0`. Solo se oyen con la música desactivada en «Controles».
- **Los dados suenan una vez por fotograma** (`0x6F5B`): cada fotograma relanza el efecto 5 y solo se oye su principio.
- **El efecto 4 no lo lanza ningún código** del juego.
- **El sonido del paso y el de la puerta comparten una variable** (`0x727D`): cada desplazamiento de la animación la pone a 1 incluso si había una puerta pendiente (9), así que la puerta suena poco o nada si hay animación el mismo fotograma.

## Línea de vista del ataque (0x7F68 / 0x8D03)
En el original cada punto de la línea de vista va seguido de `VOLCAR_BUFFER` (0x8D03: 5632 `LDI`, unos 34 ms), así que la línea «crece» sobre el plano con marcas. El port la calculaba en un buffer que nunca se mostraba; ahora `combate_punto` (gancho de `combate.c`) pinta cada punto con un cuadro de espera y la línea se deja ver 50 cuadros antes de la pantalla de combate (`alcance_visible` en `juego_combate.c`). También en los hechizos a distancia (`ui_hechizo`). Dos cuadros por punto y 50 al final.

## La ampliación Return of the Witch Lord (resumen; detalle en `docs/ZX_AMPLIACION.md`)
- Es otra imagen completa del juego: mismo motor desplazado, mismos gráficos, 10 misiones y 32 monstruos por misión (Hero Quest: 14 y 25).
- Quita todo lo que dependía del número de misión y lo sustituye por datos de las misiones: 11 salas especiales por indicador (tabla `0x8D73`), rutinas de muerte por estadística de monstruo (`0xA76D`: la 10 abre la salida, la 14 muestra el epílogo) y banderas en el byte +6 del monstruo (bit 6: arma rota; bit 7: niebla errante).
- La misión 9 trae 34 monstruos pero el motor solo usa 32; los textos nuevos (arma rota, niebla de la muerte, llave de Agrain, epílogo) y los nombres de monstruo nuevos no están todavía en el port.
- Cuidado al leer sus ficheros: el nombre de una variable de SuperBASIC se guarda con el `$` (`DIR$`), `qltools -W` escribe con el nombre del fichero local y `make img` no se rehace si solo cambia el `Makefile`.
