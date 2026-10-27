# Hero Quest ZX — inventario de rutinas

Generado por `tools/hq_inventario.py` a partir de `ref/zx/notas/*/rutinas.txt`. Los detalles están en el `.skool` correspondiente (`ref/zx/`).


## Memoria fija (GAME.BIN, 0x6400-0xFFFD, banco 0 en 0xC000)

Fichero: `ref/zx/heroquest_main.skool`

| Dirección | Etiqueta | Rutina |
|---|---|---|
| `0x6400` | `ARRANQUE` | Punto de arranque del juego (tras la carga) |
| `0x6580` | `CARGADOR_BANCOS` | Cargador de los bancos de memoria 128K |
| `0x65E1` |  | Rutina de grabación en disco +3 (no usada) |
| `0x6900` |  | Enlace de desarrollo por puerto paralelo (no usado) |
| `0x6D7E` |  | Entrada desde BASIC |
| `0x6D81` | `BUCLE_PRINCIPAL` | Bucle principal del juego |
| `0x6D89` | `PARTIDA` | Una misión completa |
| `0x6DDD` | `TURNO_PERSONAJE` | Turno de un personaje |
| `0x6E9C` | `REDIBUJAR_TURNO` | Redibuja la escena del turno |
| `0x6EB5` | `MISION_CONTINUA` | Comprueba si la misión sigue |
| `0x6ECD` | `CALCULAR_DADOS` | Calcula cuántos dados de movimiento se tiran |
| `0x6F13` | `DIBUJAR_PANEL` | Pinta marcadores del personaje |
| `0x6F1C` | `TIRAR_DADOS` | Tirada de los dados de movimiento |
| `0x6F90` |  | Comprueba un esquema de control |
| `0x6F99` | `INTERRUPCION` | Rutina de interrupción (IM 2, 50 Hz) |
| `0x6FDA` | `MOVER_AUTOMATICO` | Avanza el movimiento automático del personaje |
| `0x6FF6` | `IR_HACIA_CURSOR` | Calcula el movimiento hacia la casilla pulsada |
| `0x70A1` | `EVENTOS_MISION` | Eventos especiales de las misiones |
| `0x713E` | `MOVER_CURSOR` | Mueve el cursor según las teclas |
| `0x7168` |  | Cursor a la derecha (4 píxeles) |
| `0x7199` |  | Cursor a la izquierda (4 píxeles) |
| `0x71C9` |  | Cursor arriba (2 píxeles) |
| `0x71F5` |  | Cursor abajo (2 píxeles) |
| `0x7201` | `PROCESAR_CLIC` | Procesa el botón de disparo |
| `0x7281` | `EN_ZONA_ESPECIAL` | Comprueba si el personaje está en una zona especial |
| `0x7695` | `CURSOR_A_CASILLA` | Convierte la posición del cursor en casilla |
| `0x7869` | `DIRECCION_MONSTRUO` | Dirección del registro de un monstruo |
| `0x7B9E` |  | Sonido: efecto 3 |
| `0x7BA4` | `EFECTO_SONIDO` | Lanza un efecto de sonido |
| `0x7BC5` | `COMANDO_MUSICA` | Envía un comando al reproductor |
| `0x7BCE` | `LLAMAR_SONIDO` | Llama al reproductor de sonido |
| `0x7BFC` | `CLIC_EN_CARTEL` | Procesa el clic en un menú de opciones |
| `0x7F6F` |  | Distancia en casillas al movimiento |
| `0x8009` | `FIN_ACCION` | Cierre de una acción de turno |
| `0x8025` |  | Guarda los atributos |
| `0x802D` |  | Restaura los atributos |
| `0x82C6` | `MOVIMIENTO_CANCELADO` | Anula el movimiento pendiente |
| `0x82CB` | `MOVER_X_MENOS` | Mueve el personaje una casilla hacia x-1 |
| `0x8357` | `MOVER_X_MAS` | Mueve el personaje una casilla hacia x+1 |
| `0x83E3` | `MOVER_Y_MENOS` | Mueve el personaje una casilla hacia y-1 |
| `0x8478` | `MOVER_Y_MAS` | Mueve el personaje una casilla hacia y+1 |
| `0x850F` | `FIN_PASO` | Termina un paso de movimiento |
| `0x85F2` | `ANIMAR_FIGURA` | Animación del héroe activo |
| `0x860B` | `ABRIR_PUERTA_CASILLA` | Revela una casilla |
| `0x8662` |  | Recorre las 30 casillas contiguas a un monstruo |
| `0x8712` | `CALCULAR_VISIBLES` | Calcula las figuras visibles |
| `0x8773` |  | Posición de pantalla de una figura |
| `0x8791` | `ACTUALIZAR_HEROES` | Actualiza las posiciones de los héroes |
| `0x87F4` |  | Prepara el sprite de un monstruo/objeto |
| `0x8A2D` |  | Busca un registro de animación libre |
| `0x8BD4` | `PANEL_ICONOS` | Dibuja el panel inferior |
| `0x8BF9` | `INTERPRETE_TEXTO` | Intérprete de cadenas de ventana/texto |
| `0x8D03` | `VOLCAR_BUFFER` | Vuelca el buffer de vista a la pantalla |
| `0x8D5E` | `VENTANA_SI_NO` | Ventana modal con espera |
| `0x8D99` |  | Muestra una ventana y repinta |
| `0x8DA3` |  | Espera pulsación y suelta del botón |
| `0x8DAA` |  | Espera a soltar el botón |
| `0x8DB2` | `REPINTAR_ESCENA` | Redibuja toda la escena |
| `0x8DD1` | `VENTANA` | Dibuja una ventana de texto |
| `0x8E6F` |  | Dibuja un carácter 8x8 en el buffer |
| `0x8E7B` | `RESOLVER_CASILLA_ESPECIAL` | Efecto de una casilla especial |
| `0x8FDB` | `LIMPIAR_PANTALLA` | Prepara la pantalla de juego |
| `0x91F7` | `MOSTRAR_ORO` | Muestra el oro del héroe |
| `0x9242` | `MOSTRAR_MOVIMIENTO` | Muestra los puntos de movimiento |
| `0x926A` | `MOSTRAR_ATAQUE_DEFENSA` | Muestra ataque y defensa del héroe |
| `0x92A0` | `DIBUJAR_MAPA` | Dibuja la vista isométrica completa |
| `0x931D` | `DIBUJAR_CASILLA` | Dibuja una casilla del mapa |
| `0x93EF` |  | Dibuja un mueble/decorado |
| `0x9449` |  | Copia una baldosa 8x8 a la pantalla |
| `0x945C` |  | Superpone una baldosa 8x8 |
| `0x94F8` | `DIBUJAR_PLANO` | Dibuja el mapa general (plano) |
| `0x9523` | `ROBAR_CARTA` | Roba una carta del mazo de tesoros |
| `0x9542` | `BARAJAR_MAZO` | Baraja el mazo de cartas de tesoro |
| `0x9588` | `DEVOLVER_CARTA` | Devuelve una carta al final del mazo |
| `0x95AD` | `MODO_ATRIBUTO` | Fija el color del intérprete de texto |
| `0x95B7` | `MODO_NORMAL` | Vuelve al modo normal del intérprete de texto |
| `0x95BE` | `PANEL_PERSONAJE` | Pinta el panel del personaje activo |
| `0x9656` |  | Pinta los efectos activos |
| `0x966A` |  | Pinta los objetos |
| `0x9682` |  | Pinta las armas (listas de 8) |
| `0x9696` |  | Pinta las armaduras |
| `0x96AC` |  | Número de hechizo elegido |
| `0x96C3` |  | Pinta los hechizos |
| `0x9703` | `DIBUJAR_LISTA_BITS` | Dibuja una lista según un mapa de bits |
| `0x98F6` | `MARCAR_CASILLA_VISTA` | Marca una casilla como vista |
| `0x990D` | `MOSTRAR_VENTANA_Y_ESPERAR` | Ventana y espera de botón |
| `0x9921` | `PANEL_COMBATE` | Pinta el panel de combate |
| `0x996B` | `TIRADA_DEFENSA` | Tirada de dados de defensa |
| `0x99A2` | `TIRADA_ATAQUE` | Tirada de dados de ataque |
| `0x99DB` |  | Dibuja los dados de ataque |
| `0x9A15` |  | Rellena una cadena de iconos |
| `0x9A26` |  | Retrato del atacante |
| `0x9AF1` |  | Primera búsqueda de la habitación |
| `0x9B04` | `DANO_AL_ATACANTE` | Daño recibido por el atacante |
| `0x9B25` | `DANO_AL_OBJETIVO` | Daño al objetivo |
| `0x9B31` | `MATAR_PERSONAJE` | Muerte de un personaje |
| `0x9B65` | `PREPARAR_ATAQUE` | Calcula las armas y los dados de ataque |
| `0x9BC8` | `DADO_COMBATE` | Dado de combate |
| `0x9BD3` | `DADO` | Dado de 6 caras |
| `0x9BE3` | `BORRAR_ANIMACIONES` | Borra los registros de animación |
| `0x9BF2` | `PINTAR_MARCO` | Pinta el marco del juego |
| `0x9C02` |  | Copia el marco al buffer |
| `0x9C2F` |  | Borra los atributos |
| `0x9C3F` |  | Borra el bitmap |
| `0x9C4D` |  | Borra el buffer de vista |
| `0x9C5B` |  | Muestra el nombre del personaje |
| `0x9C77` |  | Borra las ventanas de la pantalla |
| `0x9CA3` | `CARGAR_MENU` | Carga y ejecuta el overlay del menú |
| `0x9CBD` |  | Recarga parcial del overlay |
| `0x9CCE` | `PAGINAR_BANCO6` | Pagina el banco 6 en 0xC000 |
| `0x9CD0` | `PAGINAR_BANCO` | Pagina el banco indicado |
| `0x9CD7` | `PAGINAR_BANCO0` | Pagina el banco 0 |
| `0x9CE0` | `FIN_DE_RONDA` | Turno de Morcar y fin de ronda |
| `0x9D5E` | `MOSTRAR_Y_ESPERAR` | Muestra un guion y espera |
| `0x9D66` | `ESPERAR` | Espera B interrupciones |
| `0x9D6B` | `TURNO_MONSTRUO` | Turno de un monstruo |
| `0x9DC0` | `CALCULAR_CAMINO` | Camino de un monstruo hacia un héroe |
| `0x9F80` | `LEER_PUERTO` | Lee una fila del teclado |
| `0x9F89` | `LEER_CONTROLES` | Lee los controles |
| `0x9F94` | `CASILLA_LIBRE` | Prueba una casilla en un camino |
| `0x9FF7` | `BUSCAR_TRAMPAS` | Busca trampas junto al héroe |
| `0xA075` | `EJECUTAR_CAMINO` | Mueve a un monstruo por el camino calculado |
| `0xA0D4` | `DIRECCION_CASILLA` | Dirección de una casilla |
| `0xA0E6` | `OBJETO_EN_CASILLA` | Recoge un objeto |
| `0xA108` | `EVENTO_ALEATORIO` | Evento aleatorio de turno |
| `0xA23E` | `FIN_MOVIMIENTO` | Agota el movimiento |
| `0xA254` | `MARCAR_MUERTE` | Marca a un héroe como muerto |
| `0xA271` | `DESPACHAR_HEROE` | Rutina específica del héroe |
| `0xA2C1` | `DESCUBRIR_MONSTRUOS` | Hace visibles los monstruos de la habitación |
| `0xA2E1` | `CALCULAR_VISTA` | Calcula las casillas visibles |
| `0xA39A` |  | Marca una casilla y sus vecinas |
| `0xA3B2` |  | Aparecen monstruos |
| `0xA3C5` | `REDIBUJAR_VISTA` | Redibuja la vista 3D completa |
| `0xA3E1` |  | Retrato del héroe activo |
| `0xA3F8` | `DIBUJAR_RETRATO` | Dibuja un retrato invertido |
| `0xA4FD` | `ORDENAR_FIGURAS` | Prepara el orden de dibujo |
| `0xA534` | `DIBUJAR_FIGURAS` | Dibuja las figuras por orden |
| `0xA57E` | `DIBUJAR_CURSOR` | Dibuja el cursor |
| `0xA5EA` |  | Dibuja un sprite desplazado a nivel de bit |
| `0xA794` | `GUARDAR_FONDOS` | Guarda lo que hay bajo cada sprite |
| `0xA7B6` |  | Guarda el fondo de un registro |
| `0xA8DE` | `RESTAURAR_FONDOS` | Restaura el fondo bajo los sprites |
| `0xA8FA` |  | Restaura el fondo de un registro |
| `0xA978` | `TABLA_PALABRA` | Lee una palabra de una tabla |
| `0xA979` |  | Lee una palabra de una tabla (índice par) |
| `0x725E` | `PASAR_TURNO` | Icono «siguiente personaje» |
| `0x7277` | `TERMINAR_TURNO` | Termina el turno del personaje |
| `0x727E` | `ABRIR_PUERTA` | Icono de la llave: abre una puerta |
| `0x7319` | `ATACAR` | Icono de la espada y el escudo: atacar |
| `0x7462` |  | Comprueba si el objetivo está al alcance |
| `0x7710` | `MOSTRAR_MAPA` | Icono del mapa |
| `0x7733` | `INFO_BAJO_CURSOR` | Nombre de lo que hay bajo el cursor |
| `0x7877` | `USAR_OBJETO` | Icono de la bolsa: inventario |
| `0x7B62` | `REGISTRO_PERSONAJE` | Dirección del registro de un personaje |
| `0x804E` | `OJO` | Icono del ojo: revisar |
| `0x8079` | `BUSCAR_TRAMPAS_PUERTAS` | Busca trampas y puertas secretas |
| `0x80A9` | `BUSCAR_TESOROS` | Busca tesoros en la habitación |

## Overlay del menú (banco 6 0xDB58 -> 0x5B00)

Fichero: `ref/zx/heroquest_overlay_menu.skool`

| Dirección | Etiqueta | Rutina |
|---|---|---|
| `0x5B00` | `MENU_PRINCIPAL` | Menú principal (overlay del banco 6) |
| `0x5B3F` | `INICIAR_MISION` | Carga los datos de la misión elegida |
| `0x5CE7` |  | Intercambia dos tablas |
| `0x5D29` | `OPCION_MENU` | Ejecuta la opción elegida del menú |
| `0x5D43` | `PANTALLA_CONTROLES` | Pantalla de controles |
| `0x5DC6` |  | Actualiza los rótulos de la pantalla de controles |
| `0x5DE5` | `CREAR_PERSONAJE` | Crear personaje |
| `0x5E77` |  | Menú de un héroe |
| `0x5ECE` |  | Resalta las opciones del menú de héroe |
| `0x5FF4` | `ESCRIBIR_NOMBRE` | Introduce el nombre del héroe |
| `0x60C4` |  | Resalta la letra elegida |
| `0x60E6` |  | Inicializa un héroe nuevo |
| `0x611E` |  | Dibuja los nombres de los héroes |
| `0x6141` | `ELEGIR_MISION` | Elegir la misión (Jugar) |
| `0x61A9` | `ELEGIR_HECHIZOS` | Elección de grupos de hechizos |
| `0x6251` |  | Elige un grupo de hechizos |
| `0x627E` |  | Dibuja los grupos de hechizos |
| `0x62C4` | `TIENDA` | Comprar equipo |
| `0x63FD` |  | Selecciona un artículo de la tienda |
| `0x6446` |  | Muestra armas y armadura del héroe |
| `0x6477` |  | Muestra el equipo de un héroe |
| `0x64B9` |  | Limpia el área de equipo |
| `0x64CE` |  | Espera a soltar los controles |
| `0x64D7` |  | Resalta la línea del menú |
| `0x64DC` |  | Invierte atributos de una fila |
| `0x64EF` |  | Muestra un menú |
| `0x6509` | `GRABAR_CINTA` | Graba un bloque en cinta |
| `0x6513` | `CARGAR_CINTA` | Carga un bloque de cinta |
| `0x651F` | `CARGAR_AVENTURAS` | Cargar otras aventuras (opción sin uso) |

## Reproductor de sonido (banco 4, 0xC000)

Fichero: `ref/zx/heroquest_banco4_sonido.skool`

| Dirección | Etiqueta | Rutina |
|---|---|---|
| `0xD388` | `REPRODUCTOR` | Punto de entrada del reproductor de música y efectos (banco 4) |
| `0xD3AA` | `EMPEZAR_MELODIA` | Inicia una melodía |
| `0xD47E` |  | Pone a cero 7 bytes |
| `0xD486` |  | Paso de la bajada de volumen |
| `0xD496` | `ACTUALIZAR_MUSICA` | Un paso de la música (50 Hz) |
| `0xD4E6` |  | Música parada |
| `0xD513` | `ACTUALIZAR_ENVOLVENTES` | Avanza las envolventes de los tres canales |
| `0xD52D` | `ESCRIBIR_AY` | Escribe los registros del AY |
| `0xD5BF` |  | Escribe el tono y el volumen de un canal |
| `0xD624` |  | Desplazamiento de tono de un canal |
| `0xD689` |  | Duraciones y notas de un canal |
| `0xD7AD` |  | Escribe un registro del AY |
| `0xD7BA` |  | Cuenta atrás de un canal |
| `0xDC08` |  | Tabla de periodos de las notas |
| `0xDE9E` | `EFECTO_SONIDO_SFX` | Lanza un efecto de sonido |
