# HeroQuest QL - compilacion y pruebas.
#   make data      genera los ficheros de datos (data/, build/datos/) a partir de asset/ (hay que hacerlo antes de compilar)
#   make ql        compila para el QL con XTC68 (qcc): build/heroquest
#   make img       crea el disco de 720 KB build/heroquest.img con qltools (programa, datos y boot)
#   make host      compila una version para PC (pruebas con teclas guionizadas; ver src/plat_host.c)
#   make test      ejecuta la version de PC con un guion y comprueba las capturas
#   make run       arranca la imagen en el emulador sQLux (tmp/sQLux/build/sqlux)
#   make oraculo   regenera tests/oraculo_partida.txt y build/oraculo_vista.txt y build/oraculo_turno.txt y build/oraculo_accion.txt ejecutando el original en un simulador Z80 (tools/hq_oraculo_*.py)
#   make c68_check compila con c68 y enteros de 16 bits (comprobacion de portabilidad)
SRC      = src/main.c src/data.c src/gfx.c src/text.c src/keys.c src/menu.c src/game.c src/cursor.c src/partida.c src/vista.c src/turno.c src/accion.c src/combate.c src/monstruo.c src/mision.c src/juego.c src/juego_combate.c src/qsound.c src/music.c
QLSRC    = $(SRC) src/plat_ql.c
HOSTSRC  = $(SRC) src/plat_host.c
INC      = -Isrc -Ibuild/datos
QLFLAGS  = -DQL_ASM                   # -DQL_ASM usa gfx_asm.asm; vacio = solo C (para comparar)
QCC      = qcc
DISK     = build/heroquest.img
# Los datos de data/ se regeneran (tools/hq_ql_datos.sh) cuando cambia cualquier fichero de asset/ que los alimenta o una herramienta de tools/
ASSETS   = $(shell find asset/graficos asset/graficos_ql asset/textos asset/textos_ql asset/misiones asset/misiones_ql asset/musica -type f 2>/dev/null) $(wildcard tools/hq_*.py tools/hq_ql_datos.sh)
STAMP    = build/datos/.datos
SQLUX    = tmp/sQLux/build/sqlux

.PHONY: oraculo all data ql img host test run clean c68_check
all: img

data: $(STAMP)
$(STAMP): $(ASSETS)
	@mkdir -p build/datos
	tools/hq_ql_datos.sh
	@touch $(STAMP)

build/%.o: src/%.asm
	@mkdir -p build
	as68 $< -o $@

ql: build/heroquest
build/heroquest: $(QLSRC) src/*.h build/ql_ipc.o build/gfx_asm.o build/datos/textos_ids.h
	@mkdir -p build
	$(QCC) $(QLFLAGS) $(INC) -O $(QLSRC) build/ql_ipc.o build/gfx_asm.o -o build/heroquest

build/datos/textos_ids.h: $(STAMP)
	@touch $@

host: build/host/hq
build/host/hq: $(HOSTSRC) src/*.h build/datos/textos_ids.h
	@mkdir -p build/host
	gcc -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -DHOST $(INC) -o build/host/hq $(HOSTSRC)

img: $(DISK)
$(DISK): build/heroquest $(STAMP) data/boot
	@rm -f $(DISK)
	qltools $(DISK) -fdd HEROQUEST
	qltools $(DISK) -W build/heroquest data/boot data/*_bin data/*_scr > /dev/null
	qltools $(DISK) -x heroquest 40000 > /dev/null
	@qltools $(DISK) -s

bench: build/ql_ipc.o build/gfx_asm.o build/datos/textos_ids.h
	$(QCC) $(QLFLAGS) -DBENCH $(INC) -O $(QLSRC) src/bench.c build/ql_ipc.o build/gfx_asm.o -o build/heroquest

build/host/partida_test: tests/partida_test.c src/partida.c src/partida.h
	@mkdir -p build/host
	gcc -std=c89 -pedantic -Wall -Wextra -Isrc -o build/host/partida_test tests/partida_test.c src/partida.c

build/host/vista_test: tests/vista_test.c src/vista.c src/vista.h src/partida.c src/gfx.c src/data.c src/music.c src/qsound.c src/plat_host.c
	@mkdir -p build/host
	gcc -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -DHOST -Isrc -o build/host/vista_test tests/vista_test.c src/vista.c src/partida.c src/gfx.c src/data.c src/music.c src/qsound.c src/plat_host.c

build/host/turno_test: tests/turno_test.c src/turno.c src/turno.h src/vista.c src/vista.h src/partida.c src/gfx.c src/data.c src/music.c src/qsound.c src/plat_host.c
	@mkdir -p build/host
	gcc -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -DHOST -Isrc -o build/host/turno_test tests/turno_test.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/music.c src/qsound.c src/plat_host.c

build/oraculo_turno.txt: tools/hq_oraculo_turno.py tools/hq_oraculo_partida.py tools/hq_vista_empaquetar.py
	tmp/venv/bin/python -I tools/hq_oraculo_turno.py

build/host/accion_test: tests/accion_test.c src/accion.c src/accion.h src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c src/plat_host.c
	@mkdir -p build/host
	gcc -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -DHOST -Isrc -Ibuild/datos -o build/host/accion_test tests/accion_test.c src/accion.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c src/plat_host.c

build/oraculo_accion.txt: tools/hq_oraculo_accion.py tools/hq_oraculo_partida.py
	tmp/venv/bin/python -I tools/hq_oraculo_accion.py

build/host/combate_demo: tests/combate_demo.c $(HOSTSRC) src/*.h build/datos/textos_ids.h
	@mkdir -p build/host
	gcc -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -DHOST $(INC) -o build/host/combate_demo tests/combate_demo.c $(filter-out src/main.c,$(HOSTSRC))

build/host/combate_test: tests/combate_test.c src/combate.c src/combate.h src/accion.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c src/plat_host.c
	@mkdir -p build/host
	gcc -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -DHOST -Isrc -Ibuild/datos -o build/host/combate_test tests/combate_test.c src/combate.c src/accion.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c src/plat_host.c

build/oraculo_combate.txt: tools/hq_oraculo_combate.py tools/hq_oraculo_accion.py tools/hq_oraculo_partida.py
	tmp/venv/bin/python -I tools/hq_oraculo_combate.py

build/host/monstruo_test: tests/monstruo_test.c src/monstruo.c src/monstruo.h src/combate.c src/accion.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c src/plat_host.c
	@mkdir -p build/host
	gcc -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -DHOST -Isrc -Ibuild/datos -o build/host/monstruo_test tests/monstruo_test.c src/monstruo.c src/combate.c src/accion.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c src/plat_host.c

build/oraculo_monstruo.txt: tools/hq_oraculo_monstruo.py tools/hq_oraculo_combate.py tools/hq_oraculo_accion.py tools/hq_oraculo_partida.py
	tmp/venv/bin/python -I tools/hq_oraculo_monstruo.py

build/datos/vent_addr.h: build/datos/textos_ids.h
	sed -n 's/#define TX_VENT_\([0-9A-F]*\) \([0-9]*\)$$/{ \2, 0x\1 },/p' build/datos/textos_ids.h | tr '\n' ' ' | sed 's/^/#define VENT_ADDR /; s/, $$//' > build/datos/vent_addr.h; echo >> build/datos/vent_addr.h

build/host/mision_test: tests/mision_test.c src/mision.c src/mision.h src/combate.c src/accion.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c src/plat_host.c build/datos/vent_addr.h
	@mkdir -p build/host
	gcc -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -DHOST -Isrc -Ibuild/datos -o build/host/mision_test tests/mision_test.c src/mision.c src/combate.c src/accion.c src/turno.c src/vista.c src/partida.c src/gfx.c src/data.c src/text.c src/music.c src/qsound.c src/plat_host.c

build/oraculo_mision.txt: tools/hq_oraculo_mision.py tools/hq_oraculo_monstruo.py tools/hq_oraculo_combate.py tools/hq_oraculo_accion.py tools/hq_oraculo_partida.py
	tmp/venv/bin/python -I tools/hq_oraculo_mision.py

build/host/vista_demo: tests/vista_demo.c src/vista.c src/vista.h src/partida.c src/gfx.c src/data.c src/music.c src/qsound.c src/plat_host.c
	@mkdir -p build/host
	gcc -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -DHOST -Isrc -o build/host/vista_demo tests/vista_demo.c src/vista.c src/partida.c src/gfx.c src/data.c src/music.c src/qsound.c src/plat_host.c

build/oraculo_vista.txt: tools/hq_oraculo_vista.py tools/hq_oraculo_partida.py
	tmp/venv/bin/python -I tools/hq_oraculo_vista.py

oraculo:
	tmp/venv/bin/python -I tools/hq_oraculo_partida.py
	tmp/venv/bin/python -I tools/hq_oraculo_vista.py
	tmp/venv/bin/python -I tools/hq_oraculo_turno.py
	tmp/venv/bin/python -I tools/hq_oraculo_accion.py
	tmp/venv/bin/python -I tools/hq_oraculo_combate.py
	tmp/venv/bin/python -I tools/hq_oraculo_monstruo.py
	tmp/venv/bin/python -I tools/hq_oraculo_mision.py

test: host build/host/partida_test build/host/vista_test build/host/turno_test build/host/accion_test build/host/combate_test build/host/monstruo_test build/host/mision_test build/oraculo_vista.txt build/oraculo_turno.txt build/oraculo_accion.txt build/oraculo_combate.txt build/oraculo_monstruo.txt build/oraculo_mision.txt
	build/host/partida_test
	build/host/vista_test
	build/host/turno_test
	build/host/accion_test
	build/host/combate_test
	build/host/monstruo_test
	build/host/mision_test
	tests/run_host.sh
	tests/run_mus.sh

run: $(DISK)
	tools/ql_run.sh

clean:
	rm -rf build/host build/heroquest build/*.o $(DISK)

# Comprobacion de portabilidad: c68 con enteros de 16 bits (el qcc normal usa 32). Solo informa de avisos.
c68_check: build/datos/textos_ids.h
	@mkdir -p build/c68
	@for f in main data gfx text keys menu game cursor partida vista turno accion combate monstruo mision juego juego_combate qsound music plat_ql; do \
	  qcpp $(INC) src/$$f.c build/c68/$$f.i 2>&1 | grep -v "^$$" ; \
	  c68 -int=16 build/c68/$$f.i build/c68/$$f.s 2>&1 | grep -v "^$$" ; done; echo "c68 -int=16: terminado"
