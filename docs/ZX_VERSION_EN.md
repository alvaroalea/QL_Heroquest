# Versión inglesa (`ref/Hero Quest - 128k.tzx`, Gremlin Graphics) frente a la española (Dro Soft)

Extracción: `tools/tzx_to_z80.py "ref/Hero Quest - 128k.tzx" asset/extraido_en` (el TZX inglés lleva un bloque 0x32 de información del archivo, que el extractor ahora salta; el resto es igual: 11 bloques con la misma carga). Genera también `main48.bin` y `overlay_menu_5B00.bin`. Las herramientas eligen versión con `HQ_VERSION=en` (`tools/hqmem.py`).

## Qué es igual
- **Bancos 4 (sonido) y 7 (fuentes)**: idénticos byte a byte. Los gráficos y la música no hay que extraerlos de nuevo.
- **Misiones**: mismos punteros (`0xD000`, `0xD678`...), mismos mapas, héroes, monstruos, objetos e indicadores (se comprobó campo a campo: solo cambian el nombre y el texto de presentación).
- **Overlay del menú**: mismo sitio (banco 6, `0xDB58`, `0x11EE` bytes) y mismo tamaño de carga.

## Qué cambia
El código es el mismo programa, pero los textos tienen otra longitud y desplazan todo lo que hay detrás (de −3 a +150 bytes según la zona), así que **ninguna dirección fija del español vale**. Las rutinas de las zonas activas de las ventanas también se mueven (p. ej. `0x7453`→`0x7450`, `0x8079`→`0x8084`). La tabla `DIR` de `tools/hq_txt_lib.py` guarda las direcciones de cada versión (ventanas, guiones, tablas de nombres, tablas del panel, pociones, guion de nombres de misión).

Diferencias de contenido que afectan a un empaquetado bilingüe:
- **Ventanas**: 50 en inglés frente a 47 en español. Los ids `[VENT_xxxx]` llevan la dirección de cada versión, así que **no coinciden** con las constantes `TX_VENT_*` del port; habrá que emparejarlas por orden o por contenido.
  Entre otras: tres variantes de «ENVUELTO POR UNA TEMPESTAD / DORMIDO», y dos ventanas de pociones fijas (*Heroic Brew*, *Holy Water*) en lugar de componerlas con las tablas.
- **Guiones**: 80 en inglés frente a 81 en español (el menú de elegir hechizos es distinto: *CHOOSE YOUR FIRST SET OF SPELLS* / *THREE SETS*).
- **Pociones**: 4 nombres (campo 10) y 4 descripciones (campo 31) con 5 pares de índices, frente a 6 nombres, 6 descripciones (campo 29) y 7 pares en español.
- **Nombres de monstruo**: 13 (el último, *WITCHLORD*, está repetido) frente a 12.
- **Armas del panel**: 7 punteros (el español lee un octavo que es basura y sale como `-`).
- Erratas del original inglés que se conservan tal cual: `RESILIANCE`, `EQIPMENT`, `ENAMO` (español).

Empaquetado: `tools/hq_misiones_empaquetar.py` genera ya `data/mision_NN_en_bin` (y `misiones_indice_en_bin`) además de los `_es_`; el juego los carga según el idioma elegido en la primera pantalla. `tools/hq_textos_empaquetar.py` añade además a `textos_bin` las **50 ventanas inglesas** y los nombres y descripciones de las pociones (ids `EN_VENT_xxxx`, `EN_POCIONES_NOMBRE`, `EN_POCIONES_DESCRIPCION`, al final, para no cambiar los ids españoles). La equivalencia ventana inglesa → española la da `asset/textos_ql/equivalencia_en_es.txt` (`[NEW]` en el lado español = solo existe en inglés). El mismo fichero lleva al final las equivalencias de los 43 guiones de la partida (`GUION_*`) y de los 38 guiones del menú (`MENU_*`) por orden; el único español sin equivalente inglés es `MENU_684C` («ELIGE TUS PRIMEROS HECHIZOS»), marcado `[NEW]` en el lado inglés: en inglés su texto va dentro del guion `MENU_6813`, que corresponde a `MENU_683C` («MAGO»); las acciones de las zonas de clic se traducen del código inglés al español (`0x7450`→`0x7453`, `0x7457`→`0x745A`, `0x8084`→`0x8079`, `0x80B4`→`0x80A9`) deduciéndolas de las ventanas emparejadas. También los textos del port (`textos_port.en.txt`, como `EN_PORT_*`). La tabla general `build/datos/idioma_en.h` da, para cada elemento español, su versión inglesa (o -1), y `tx_item` la aplica. Y todo lo demás de `textos_bin` (guiones, tablas): los guiones por la equivalencia y las tablas por el mismo id. Las tablas pueden tener distinto tamaño en cada idioma (pociones 6/4 nombres y 7/5 pares, armas 8/7): el código usa solo las entradas que existen.

Notas: la ventana española `B058` («¿DESEAS QUEDARTE CON EL ORO?») no tiene zonas de clic en el original español, mientras que la inglesa equivalente (`B0C5`) sí; el texto inglés de la poción de resistencia/defensa trae `INCREASESYOUR` sin espacio (errata del original). Las descripciones inglesas de pociones están partidas en dos con huecos: el port las junta en una línea con un solo espacio.

Los ficheros resultantes son `asset/textos/textos.en.txt` y `asset/misiones/mision00..13.en.txt`.
