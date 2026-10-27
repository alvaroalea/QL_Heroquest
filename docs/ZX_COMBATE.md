# El combate, la bolsa y los hechizos en el original (ZX Spectrum) y su port a C

Análisis de los iconos de **atacar** (`0x7319`) y de la **bolsa** (`0x7877`), de la pantalla de combate (`0x9921`), de la muerte de los monstruos con sus recompensas y del panel del personaje, y su port a `src/combate.c` (lógica) y `src/juego_combate.c` (pantallas). Verificación: `tools/hq_oraculo_combate.py` ejecuta **las rutinas originales del Spectrum** en el simulador Z80 de SkoolKit y `tests/combate_test.c` compara el resultado (héroes, monstruos, capas, variables y guiones mostrados) con el C (`make test`: 3528 comprobaciones en 392 casos; `tests/run_ql_combate.sh` repite 8 casos compilados con c68 en sQLux). Para ver las pantallas del original: `tools/hq_zx_pantalla.py ESCENA SALIDA.png` (ataque, defensa, panel…).

Gráficos: los de los ficheros empaquetados (`retratos_bin`, `sprites_bin`); las tablas (armas, tipos de monstruo, zonas del panel, a quién va cada hechizo, casillas de al lado) están en `data/vista_bin` (secciones 23 a 28, `tools/hq_vista_empaquetar.py`).

## Atacar (`0x7319`)

1. No se puede con el personaje 4 (Ragnar) ni si el héroe lleva el oro de Magnus (bit 5 de `+0x0F`), ni si ya ha actuado (`0xA98B` bit 1). Si ya se había movido (bit 0) no puede moverse más (`0xA98B |= 4` y `0xA98C = 0`) y se marca como actuado (bit 1). Los valores anteriores se guardan para deshacerlo si se cancela.
2. **Elegir objetivo** (`0x7355-0x7381`): el plano de la mazmorra (`0xA3C8`) con el cursor y, en la fila 21, los botones `ARMA` (columnas 14 a 19) y `CANCELAR` (23 a 28). Mientras se mueve el cursor `INFO_BAJO_CURSOR` (`0x7733`) deja en `0xA99D` el número de monstruo (capa C, solo si la casilla está a la vista en el plano) y en `0xA9C5` su registro. Un clic con un monstruo debajo lo elige; `ARMA` abre el panel del personaje con las zonas de `0x7FEE` (pociones, objetos, armas, armadura y caja, sin hechizos).
3. `PREPARAR_ATAQUE` (`0x9B65`): `0xA9A9` = dados de ataque del héroe (`+0x14`); si lleva un arma (bits de `+0x11`; la primera por el bit 0) toma su byte de la tabla `0xAA81` (lanza `E1`, hacha `61`, ballesta `D2`, gran hacha `03`, espada `02`, daga `81`, bastón `81`: bits 0-1 = dados de más, 5 = se pierde al lanzarla, 4 y 6 = alcance, 7 = también en diagonal) en `0xA9A8` y, si los bits 0-1 no son cero, los dados son ese valor + 1. Sin arma: la vara de orco (bit 3 de `+0x12`) da 2 dados; la espada espiritual (bit 4) da 4 contra esqueletos, zombis y momias y 3 contra el resto. Sin nada, `0xA9A8` conserva el valor de antes.
4. **Alcance** (`0x7462`): `0x74FC` dibuja el plano con marcas (`0xA9A1 = 1`: monstruos y demás personajes como una marca, sin objetos ni el personaje activo) y traza con `0x7E74`/`0x7E7E` una **línea de vista** punto a punto entre los centros de las dos casillas, comprobando en cada punto el buffer (si ya hay un punto encendido y quedan 4 o más pasos, choca). Después mira las ocho casillas de al lado (tabla `0x81C0`; las cuatro primeras, en cruz, ponen el bit 1 de `0xA9A3`; las cuatro de las diagonales, el bit 2). Resultado: `0` bien (cuerpo a cuerpo salvo con la ballesta); `0xFD` ballesta demasiado cerca; `0xFE` el arma no llega (necesita el bit 4 o 6 de `0xA9A8`); `0xFF` no se ve. Las armas arrojadizas (bit 5) se pierden al lanzarlas (se quitan de `+0x0E` y de `+0x11`), llegue o no.
5. **Mensajes** si falla (`0x73AC`): `B913` (muy lejos), `B934` (ballesta), `B8D8` (no se ve) y, si el monstruo no está a la vista (bit 0 de su `+6`), solo `B8F7` (la segunda línea de `B8D8`). Todos con una espera de 150 fotogramas y el fin de la acción.
6. **La tirada** (`0x73DB`): pantalla de combate (más abajo); bonos de ataque (`0x7559`: a los jefes, tipos de atributos 11 y 12, no se les hace daño sin la espada espiritual: 0 dados; +2 con la poción de fuerza y +2 con el hechizo de valor, y se gastan); datos del monstruo (`0x751D`: sus dados de ataque y defensa de la tabla `0xAA8C`); `TIRADA_ATAQUE` (`0x99A2`): tantos dados de combate como `0xA9A9` (calavera = golpe) y tantos como defensa del monstruo (`0xA9AC`, **256 si es cero**) cuentan los escudos negros. Si hay más golpes que escudos el monstruo muere (`0x9B25`, `0x9B31`); no hay heridas: un monstruo muere de un golpe.
7. **Segundo ataque**: con la poción heroica (bit 6 de `+0x17`; se gasta) se ofrece atacar otra vez (`0x7436`: ventana `B020`, zonas `0x7453` = no y `0x745A` = sí). Si el monstruo era de tipo 1 (orco), el héroe tiene la vara de orco y es la primera vez en esa sala (`0xA9A4` bit 0, que también marca la búsqueda), igual. Si se acepta se vuelve a elegir objetivo sin gastar otra acción. Si no, se espera 150 fotogramas más y termina.

### La pantalla de combate (`0x9921`, `0x73DB`)

Pantalla limpia con: retrato de 128 × 64 en la columna 8 (de `retratos_bin`: 0 bárbaro, 1 enano, 2 elfo, 3 hechicero —Ragnar usa el del elfo—, 4 duende/orco/Ulag/Grak, 5 fimir, 6 guerrero, 7 esqueleto, 8 momia, 9 ataque fallido, 10 zombi, 11 goblin (retocados en el QL; en el original 4 era duende/orco/Ulag/Grak y 8 zombi/momia); tablas `0x9ABB` y `0x9ACA`), el mensaje debajo (`B89F` has matado, `B8BA` tu ataque no tiene éxito, `B866` el enemigo te hiere, `B87C` el ataque enemigo sin éxito), las figuras de los dos combatientes (`0xA3E1`, `0x764A`: sprite 20 + 12 × héroe y el del tipo del monstruo, en negativo), los rótulos con el nombre (fila 16; el del héroe en la columna 3 y el del monstruo en la 20), `CUERPO` y `MENTE` de cada uno (filas 18 y 19; el héroe en rojo, el monstruo en verde) y las dos filas de iconos (fila 20; `0x70` espadas = golpes y `0x71` escudos): atacando el héroe, golpes a la izquierda (cian) y escudos del monstruo a la derecha (magenta); defendiéndose, al revés. Las cifras del monstruo son los dos nibbles de su `+7`, tal como estaban **antes** de la tirada.

## Defensa (`0x9921`, `0x996B`, `0x9B04`)

`PANEL_COMBATE` pone al defensor como personaje activo mientras dura (`0xA998 = 0xA9AD`, `0xA9DD = 0xA9C7`), calcula sus dados de defensa (`0x7596`: nada si lleva el oro de Magnus; si no `+0x15`, 4 con el bit 0 de `+0x12`, 4 con el bit 3 o 3 con el bit 2 de `+0x13` —armadura completa y cota de malla—, +1 el escudo y +1 el casco, +2 con la poción de resistencia y +2 con la piel de roca; las dos últimas se gastan), tira esos dados contando los escudos blancos y los dados de ataque del monstruo contando las calaveras, repite `0x7596` y resta la diferencia al cuerpo del héroe (`0x9B04`): si llega a 0 muere (`0x7B31`).

## Muerte de un monstruo (`0x9B31`)

Quita el nibble alto de `+7`, el registro (`+0 = 0`), el bit de visible y su casilla de la capa C, y salta a la rutina de su tipo (tabla `0xAA8C`: movimiento, ataque, defensa y rutina), que da la recompensa **al héroe activo**:

| Tipos | Rutina | Recompensa |
|---|---|---|
| 8 | `0x7602` | 100 monedas |
| 10 | `0x7608` | 150 monedas a cada uno de los cuatro héroes |
| 14 | `0x761E` | 10 monedas |
| 15 | `0x7624` | 20 monedas |
| 16 y 17 | `0x7637` | 30 monedas |
| 18 | `0x762A` | 30 monedas, el objeto «bane de orco» (bit 3 de `+0x0D`) y el guion `B9E3` |
| 19 | `0x763D` | 200 monedas y el guion `BA09` (la Estrella del Oeste) |
| los demás | `0x9A25` | nada |

## La bolsa y el panel del personaje (`0x7877`, `0x95BE`, `0x7BFC`)

`PANEL_PERSONAJE` pinta (pantalla limpia, caja verde) según la máscara de cada héroe (`0x9626`, igual a `0x7FEA`: `97 97 9F E3`): **pociones** (`+0x10`, 7 nombres; en negativo las activas de `+0x17`), «objetos» (`+0x0D`; el rótulo original dice `BUSQUEDA TESORO`), **armas** (`+0x0E`; en negativo la de `+0x11`), **armadura** (`+0x0F & 0x0F`; en negativo `+0x13`), el **arma** única del hechicero, los **hechizos** (`+0x0B` y `+0x0C`, 6 y 6 nombres; el elegido, en negativo), `UTILIZA CAJA` (bit 7 de `+0x0F`), `SALIDA` y `ORO MAGNUS`/`DEJA CAER ORO`. Cada lista dibuja una línea por bit a uno (las posiciones están en `0x9703`). Los clics (`0x7BFC`) se comparan con la tabla de zonas `0x7FAE` (10 de 6 bytes: columnas, filas y manejador) y con la máscara de zonas permitidas (`0xA99B`, de `0x7FEA` o `0x7FEE`):

| Zona | Manejador | Efecto |
|---|---|---|
| 0 pociones | `0x7D6F` | cura 4 puntos (hasta el máximo de `0x7E4E`: 8, 7, 6, 4 y 2) o activa el efecto o, con el agua santa, destruye a los esqueletos, zombis y momias de la sala (sala de la casilla de origen de la vista; compara el byte entero de la capa A) |
| 1 objetos | `0x7CE2` | la armadura de Borin (bit 0) se alterna y quita las armaduras; la vara de orco y la espada espiritual sustituyen al arma |
| 2 armas | `0x7C45` | elige arma; la gran hacha quita el escudo |
| 3 y 5 hechizos | `0x7D1F` | gasta la acción del turno (con la vara de recuerdo, una vez más) y elige el hechizo |
| 4 armadura | `0x7C6B` | la armadura completa y la cota de malla se excluyen, el escudo no va con la gran hacha |
| 6 arma única | `0x7CC7` | |
| 7 caja | `0x7E53` | el enano (y quien tenga la caja) desactiva las trampas de al lado (`0x9FF7`, tabla `0x81BE`) |
| 8 salida | `0x768F` | cierra el panel |
| 9 oro | `0x7E61` | deja caer el oro de Magnus (`B9B7`) |

### Lanzar un hechizo (`0x7877`, `0x78B4`, `0x7943`)

Con el hechizo elegido se vuelve al plano (sin botones) y se elige el blanco; la tabla `0x7B70` dice a quién se puede lanzar cada uno (bit 0 un héroe, bit 1 un monstruo, bit 2 una casilla vacía). Un jefe es inmune: no se gasta y el mensaje es `BAAC` (se desvió). Al lanzarlo se gasta (`XOR` de la máscara de `+0x0B/+0x0C`); salvo el genio y el fuego de ira o si el blanco es el mismo que lanza, hace falta que estén en la misma sala (por encima de `0x15`) o que haya línea de vista (`BAAC` si no). Efectos (tabla `0x7B7C`): velo de niebla, huracán, valor y piel de roca ponen un bit de `+0x18`; aguas curativas y sanar cuerpo suman 4 puntos; dormir (un escudo blanco lo evita) y tempestad ponen el bit 1 o 2 del monstruo o del héroe; bola de fuego (2 dados, cada escudo blanco resta un punto), fuego de ira (1 dado) y genio (5 dados; sobre una casilla abre su sala, `0x860B`) hacen daño y matan a un monstruo de un golpe; cruzar muros va al héroe (el hechicero que ya se ha movido lo tendrá el turno siguiente). Mensajes `BA8F`, `BAC6` y `BAEA` (con la cifra en `0xBAFE`).

## Rutinas

| Rutina del original | En el QL |
|---|---|
| `0x7319` `ATACAR`, `0x7355`-`0x7381`, `0x76AE` | `ui_atacar()`, `seleccionar()` (`juego_combate.c`) |
| `0x7436` | `si_no()` |
| `0x7462`, `0x74FC`, `0x74E5` | `combate_alcance()`, `arma_se_pierde()` |
| `0x7E74`, `0x7E7E`, `0x7F2A` | `combate_linea()`, `pixel()` |
| `0x751D`, `0x7559`, `0x7596` | `combate_datos_monstruo()`, `combate_bonos_ataque()`, `combate_dados_defensa()` |
| `0x9B65` | `combate_preparar()` |
| `0x99A2`, `0x9B25` | `combate_ataque()` |
| `0x9921`, `0x996B`, `0x9B04` | `combate_defensa()`, `ui_defensa()` |
| `0x99DB`-`0x9A7B`, `0xA3E1`, `0x764A` | `pantalla_combate()`, `minifigura()`, `banda()` |
| `0x9B31`, `0x7602`-`0x7647` | `monstruo_muere()` |
| `0x7B1C` | `combate_cuenta_calaveras()` |
| `0x95BE`, `0x9656`-`0x9703` | `panel_dibujar()`, `lista()` |
| `0x7BFC`, `0x7F6F`, `0x7F8D`, `0x96AC`, `0x96EC` | `panel()`, `panel_clic()`, `panel_item()`, `panel_item16()`, `panel_mascara_hechizo()`, `panel_numero_hechizo()` |
| `0x7C45`-`0x7D6F`, `0x7DC4`, `0x7E53`, `0x7E61`, `0x9FF7` | `panel_arma()`, `panel_armadura()`, `panel_arma_sola()`, `panel_objeto()`, `panel_hechizo()`, `panel_efecto()`, `panel_caja()`, `panel_soltar_oro()` |
| `0x7877`, `0x78B4`, `0x7943`-`0x7B15` | `ui_bolsa()`, `ui_hechizo()`, `hechizo_lanzar()`, `hechizo_inmune()` |

## Diferencias deliberadas con el original

- **Las figuras de la pantalla de combate** se dibujan con sus colores sobre una caja blanca (el original las pone en negativo con la tinta del atributo).
- **Esc cancela** la elección del objetivo de un hechizo (el original no tiene cancelar; el hechizo no se gasta) y cierra el panel.
- **El botón corta las esperas** de los mensajes.
- **Rótulos de la pantalla de combate** (`CUERPO`, `MENTE`): son del port (`textos_combate.txt`, traducibles) porque en el original son datos fijos del guion `0xB161`.
- Los **sonidos** (`0x7BA2`, `0x7BA4`) irán con el paso de efectos de sonido.
