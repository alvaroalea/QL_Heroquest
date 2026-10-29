# HeroQuest (ZX Spectrum 128K, Dro Soft) — notas de juego y análisis

Información dada por el usuario (jugador) y confirmada en el código:

- Juego **por turnos**: primero los 4 personajes del jugador (bárbaro, hechicero, enano y elfo/«duende»), luego el ordenador.
- Se tira un dado y ese número son los **puntos de movimiento** del personaje; el movimiento se hace con un **cursor tipo ratón** movido con teclas; el «clic» es SPACE (fuego).
  Se pulsa en casillas cercanas para mover, una o varias veces, hasta agotar los puntos.
- Parte inferior de la pantalla: a la izquierda, saltar al **siguiente personaje**; a la derecha, **abrir puertas, buscar trampas, luchar** y ver el **mapa general** de la mazmorra.
- Menú principal (en español): Jugar, Comprar equipo, Crear personaje, Controles y, por último, Cargar otras aventuras (ver el final de este documento).
- Hechizos (pantalla del hechicero): Aire (Genio, Tempestad, Huracán), Fuego (Valor, Cólera de fuego, Bola de fuego), Tierra (Piel de roca, Sanar heridas, Atravesar muros), Agua (Dormir, Velo de niebla, Aguas curativas).
- `ref/HeroQuest_map.png`: mapas de las 14 misiones.

Ver `docs/ZX_ANALISIS.md` para la estructura técnica.

## Interfaz de juego (según el usuario)

- **Arriba, en cian**: los puntos de movimiento restantes, junto a un icono de **huella de zapato**.
- **Abajo a la derecha**: cuatro **flechas diagonales** junto al icono de la **llave**; mueven al personaje **una casilla** (las diagonales de pantalla son los ejes de la rejilla isométrica).
  En el código: los manejadores `0x82CB` (x−1), `0x8357` (x+1), `0x83E3` (y−1) y `0x8478` (y+1) validan puertas/obstáculos, restan 1 a `A98C` y llaman a `0x850F`.
- El **ojo** abre un pergamino (ventana) para confirmar la acción (buscar trampas, buscar tesoros).
- La **bolsa**, justo encima del ojo, muestra el **inventario** (vacío al empezar).
- **Espada y escudo**: solo funcionan si hay monstruos a la vista.
- Debajo, el icono del **mapa**: cada casilla de 8×8 píxeles es una casilla del juego. En el primer nivel los cuatro personajes empiezan, uno en cada esquina.

## Datos de código confirmados

- Teclas (`0xFD60`): SPACE = botón, Z = izquierda, X = derecha, O = arriba, K = abajo (también hay variantes de joystick: `0xFD90`, `0xFDB8`, Kempston en `0xFDE0`). Se guardan en `0xA997` (bit4 botón, bit0 derecha, bit1 izquierda, bit3 arriba, bit2 abajo).
- Cursor: posición en píxeles en `0xAA69` (X) y `0xAA6A` (Y); la casilla es `(X−12)/8`, `(Y−2)/8` (`0x7695`). Se mueve 4 px en X y 2 px en Y por fotograma, con aceleración (`0x713E`).
- Clic: `0x7201` recorre la tabla de zonas activas apuntada por `0xA9DB` (entradas de 6 bytes: x1, x2, y1, y2, manejador) y salta con `JP (HL)`.
- Mapa: 26×19 casillas (`0x1A` × `0x13`); la casilla (x,y) está en `tabla(0xB705)[y] + x`; capas en `0x6200`, `0x6600`, etc.

## Puertas (según el usuario)

- Situado junto a una puerta, el icono de la **llave/cerradura** la **abre** y se muestra el contenido de toda la habitación **en el plano (mapa)**; en la vista 3D principal solo se ve al **entrar** en ella.
  Hipótesis a verificar en el código: los bits 6 y 7 del byte de cada casilla (capa `0x6200`) controlan qué casillas se dibujan en la vista 3D (`DIBUJAR_CASILLA`, `0x931D`) y cuáles en el plano; la rutina `0x860B` revela una casilla/habitación al abrir.

## Combate (según el usuario)

- Si hay enemigos, el icono de **atacar** (espada y escudo) muestra el **mapa general**; allí se elige a quién atacar. Si el enemigo no está al alcance, no se ataca.
- Si está al alcance se tiran **dados de ataque** (el jugador) y **de defensa** (el defensor). Si los de ataque son más (se representan con **espadas** en pantalla) se quita vida. La mayoría de los enemigos tienen **1 punto de vida** y mueren.

Correspondencia con el código (por verificar):
- `0x9BC8` tira un dado de combate: lee la cara de `0xAA7B[d6]`; los valores observados son 0, 1 y 2 (3 caras con 0, 2 con 1 y 1 con 2: equivalen a calaveras, escudos blancos y escudo negro del juego de mesa). El resultado queda en C.
- `0x9BD3` es el dado normal de 6 caras (mismo algoritmo que la rutina `dice` del código fuente original que quedó en memoria en `0x6C0C`: usa el registro R).
- `0x9B04` y `0x9B25` aplican el daño (restan a `IX+6`, vida del objetivo) y, si llega a 0, llaman a `0x7B31`/`0x9B31` (muerte/retirada del personaje).

## Menús del overlay (según el usuario)

**Comprar equipo.** Arriba aparece la lista de héroes: se cambia de héroe con la tecla K y se elige con SPACE. Elegido el héroe, el menú de abajo ofrece diversos objetos (armas a la izquierda, armaduras y objetos a la derecha, con su precio); la última línea, en amarillo, es el dinero que tiene el héroe (se consigue buscando tesoros, como premio por acabar una mazmorra y de otros modos). Si hay dinero suficiente, SPACE (el botón de disparo, `0x6339`) lo pasa al inventario de ese personaje. Se sube y baja por la lista y se acaba eligiendo «FINAL». Los objetos tienen características que ayudan en el juego.
En el código: `TIENDA` (`0x62C4`), tabla de artículos `0x6603` (13 entradas de 4 bytes: máscara para `IY+0x0E`, máscara para `IY+0x0F` y precio), precios en el guion `0x69E2`.

**Crear personaje.** Pide elegir un héroe (con K y SPACE); una vez elegido se puede *cambiar el nombre* del héroe, *cargar* o *guardar* los datos de ese personaje (en cinta: `GRABAR_CINTA`/`CARGAR_CINTA` llaman a las rutinas de la ROM). Al elegir cambiar el nombre, se borra el actual y en la parte de abajo aparece el abecedario para ir seleccionando las letras que forman el nombre; «FIN» termina. En el código: `CREAR_PERSONAJE` (`0x5DE5`), `ESCRIBIR_NOMBRE` (`0x5FF4`, rejilla del guion `0x6B9B`, hasta 6 letras).

## Elección de hechizos al empezar (según el usuario)

Al pulsar «Jugar», antes de elegir la misión, se eligen los hechizos entre **4 grupos: Aire, Fuego, Tierra y Agua** (Aire: Genio, Tempestad, Huracán; Fuego: Valor, Cólera de fuego, Bola de fuego; Tierra: Piel de roca, Sanar heridas, Atravesar muros; Agua: Dormir, Velo de niebla, Aguas curativas). **Primero elige el hechicero y después el duende (elfo).**
Código (`ELEGIR_HECHIZOS`, `0x61A9`): si juega el hechicero, `0x61F9` muestra los grupos y le deja elegir primero; el guion cambia según si también juega el elfo («ELIGE TUS 3 GRUPOS DE HECHIZOS» si está solo, «ELIGE TUS PRIMEROS HECHIZOS» si hay elfo, que elige después con «ELIGE TU GRUPO DE HECHIZOS»). Cada elección activa un bit en los bytes `+0x0B`/`+0x0C` del registro del héroe.

## Secuencia de «Jugar» (según el usuario)

1. Elección de hechizos (hechicero y luego duende).
2. **Elección de la misión** (lista de las 14).
3. El juego muestra el **texto con el objetivo de la misión** (el guion de presentación de cada misión, que `INICIAR_MISION` descomprime en `0xFA00` y dibuja con `0x8BF9`; se espera el botón con `0x8DA3`).
4. Pasa al juego en sí (`PARTIDA`, `0x6D89`).

Coincide con el código: `ELEGIR_MISION` (`0x6141`) → `0x9CBA: JP 0x5B3F` (`INICIAR_MISION`: texto en `0x5C3C-0x5C53`) → `BUCLE_PRINCIPAL` llama a `PARTIDA`.

## Opción «Cargar otras aventuras»

Existe en el menú y en el código (`CARGAR_AVENTURAS`, `0x651F`: carga de cinta un bloque de 0x400 bytes en `0x4000` y salta a `0x4015`), pero en el original no se usa: la ampliación *Return of the Witch Lord* es otra imagen completa de la cinta (`docs/ZX_AMPLIACION.md`). En el port la opción carga las misiones de una ampliación desde el disco.
