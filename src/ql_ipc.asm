! HeroQuest QL - enlace con QDOS (as68 de XTC68). Convencion de c68: argumentos en la pila (de derecha a izquierda),
! resultado en D0; el que llama pierde D0-D2/A0-A1; se conservan D3-D7 y A2-A6.
	.sect	.text
	.align	2
	.extern	_ql_trap1
	.extern	_ql_trap3
	.extern	_ql_ipc
	.extern	_ql_tick_on
	.extern	_ql_tick_off
	.extern	_ql_ticks
	.extern	_ql_isr_set
	.extern	_mus_isr
	.extern	_ql_qimi_on
	.extern	_ql_qimi_off
	.extern	_ql_qimi_take
	.extern	_ql_pe_rptr
	.extern	_ql_dir_open
	.extern	_ql_dir_close

! long ql_trap1(long d0, long d1, long d2, long d3)   -> D0 (error QDOS)
_ql_trap1:
	movem.l	d2-d3,-(sp)
	move.l	12(sp),d0
	move.l	16(sp),d1
	move.l	20(sp),d2
	move.l	24(sp),d3
	trap	#1
	movem.l	(sp)+,d2-d3
	rts

! long ql_trap3(long d0, long d1, long d2, long d3, long a0, long a1)   -> D0
_ql_trap3:
	movem.l	d2-d3/a2,-(sp)
	move.l	16(sp),d0
	move.l	20(sp),d1
	move.l	24(sp),d2
	move.l	28(sp),d3
	move.l	32(sp),a0
	move.l	36(sp),a1
	trap	#3
	movem.l	(sp)+,d2-d3/a2
	rts

! long ql_ipc(char *blk): MT.IPCOM; devuelve D1.B (respuesta del IPC) o un error negativo
_ql_ipc:
	move.l	a3,-(sp)
	move.l	8(sp),a3
	moveq	#0x11,d0
	trap	#1
	tst.l	d0
	bmi	ipc_fin
	moveq	#0,d0
	move.b	d1,d0
ipc_fin:
	move.l	(sp)+,a3
	rts

! Contador de cuadros de 50 Hz: tarea de la lista de interrupciones sincronizadas (MT.LPOLL / MT.RPOLL).
! enlace: +0 siguiente (long), +4 direccion de la rutina (long), +8 contador (long)
_ql_tick_on:
	move.l	a0,-(sp)
	clr.l	tick_cnt
	clr.l	tick_link
	lea	tick_rut,a0
	move.l	a0,tick_link+4
	lea	tick_link,a0
	moveq	#0x1c,d0
	trap	#1
	move.l	(sp)+,a0
	rts

_ql_tick_off:
	move.l	a0,-(sp)
	lea	tick_link,a0
	moveq	#0x1d,d0
	trap	#1
	move.l	(sp)+,a0
	rts

_ql_ticks:
	move.l	tick_cnt,d0
	rts

! long ql_isr_set(long on): 1 = la tarea del reloj llama ademas a mus_isr(ticks) (el motor de musica a 50 Hz, ver music.c)
_ql_isr_set:
	move.l	4(sp),d0
	move.w	d0,isr_flag
	rts

! rutina de la lista: se llama en modo supervisor 50 veces por segundo. Suma uno al contador y, si esta activado, ejecuta la musica en una
! pila propia (la de supervisor es pequena); se conservan todos los registros.
tick_rut:
	addq.l	#1,tick_cnt
	tst.w	isr_flag
	beq	tick_fin
	movem.l	d0-d7/a0-a6,-(sp)
	move.l	sp,isr_sp
	lea	isr_pila,a0
	add.l	#2048,a0
	move.l	a0,sp
	move.l	tick_cnt,-(sp)
	jsr	_mus_isr
	move.l	isr_sp,sp
	movem.l	(sp)+,d0-d7/a0-a6
tick_fin:
	rts

! Raton QIMI: rutina de la lista de interrupciones externas (MT.LXINT = $1A, MT.RXINT = $1B). Cada pulso del raton levanta una
! interrupcion: $1BFBC da el estado (bit 2 = pulso en X, bit 4 = X a la derecha, bit 5 = pulso en Y, bit 0 = Y hacia arriba) y leer
! $1BFBE lo da por atendido y rearma la interrupcion. Suma una cuenta por pulso en qimi_dx / qimi_dy (+x derecha, +y abajo).
! enlace: +0 siguiente (long), +4 direccion de la rutina (long)
_ql_qimi_on:
	move.l	a0,-(sp)
	clr.l	qimi_dx
	clr.l	qimi_dy
	clr.l	qimi_link
	lea	qimi_rut,a0
	move.l	a0,qimi_link+4
	lea	qimi_link,a0
	moveq	#0x1a,d0
	trap	#1
	move.b	0x1bfbe,d1
	move.l	(sp)+,a0
	rts

_ql_qimi_off:
	move.l	a0,-(sp)
	lea	qimi_link,a0
	moveq	#0x1b,d0
	trap	#1
	move.l	(sp)+,a0
	rts

! void ql_qimi_take(long *dx, long *dy): entrega las cuentas acumuladas y las resta (sub.l a memoria no se parte con la interrupcion)
_ql_qimi_take:
	move.l	4(sp),a0
	move.l	qimi_dx,d0
	sub.l	d0,qimi_dx
	move.l	d0,(a0)
	move.l	8(sp),a0
	move.l	qimi_dy,d0
	sub.l	d0,qimi_dy
	move.l	d0,(a0)
	rts

! long ql_pe_rptr(long chan, long *rec, long *xy): Pointer Environment, IOP.RPTR ($71) con terminacion $30 (bits 4 y 5: vuelve ya, este
! o no el puntero en la ventana). rec = registro de puntero de 24 bytes; *xy = D1 (x en la palabra alta, y en la baja, en coordenadas
! de pantalla): entra la posicion anterior y sale la actual. Devuelve D0 (error QDOS)
_ql_pe_rptr:
	movem.l	d2-d3/a2,-(sp)
	move.l	16(sp),a0
	move.l	20(sp),a1
	move.l	24(sp),a2
	move.l	(a2),d1
	moveq	#0x30,d2
	moveq	#-1,d3
	moveq	#0x71,d0
	trap	#3
	move.l	d1,(a2)
	movem.l	(sp)+,d2-d3/a2
	rts

! long ql_dir_open(char *nombre): IO.OPEN (trap #2) en modo directorio (modo 4 = IO.DIR) del nombre QDOS (palabra de longitud y caracteres,
! p. ej. "flp1_"). Devuelve el canal o un error negativo
_ql_dir_open:
	movem.l	d2-d3,-(sp)
	move.l	12(sp),a0
	moveq	#1,d0
	moveq	#-1,d1
	moveq	#0,d2
	moveq	#4,d3
	trap	#2
	tst.l	d0
	bne	dir_open_fin
	move.l	a0,d0
dir_open_fin:
	movem.l	(sp)+,d2-d3
	rts

! void ql_dir_close(long canal): IO.CLOSE
_ql_dir_close:
	move.l	4(sp),a0
	moveq	#2,d0
	trap	#2
	rts

qimi_rut:
	movem.l	d0/a0,-(sp)
	move.l	#0x1bfbc,a0
	move.b	(a0),d0
	btst	#2,d0
	beq	qimi_y
	btst	#4,d0
	beq	qimi_xl
	addq.l	#1,qimi_dx
	bra	qimi_y
qimi_xl:
	subq.l	#1,qimi_dx
qimi_y:
	btst	#5,d0
	beq	qimi_fin
	btst	#0,d0
	beq	qimi_yd
	subq.l	#1,qimi_dy
	bra	qimi_fin
qimi_yd:
	addq.l	#1,qimi_dy
qimi_fin:
	move.b	2(a0),d0
	movem.l	(sp)+,d0/a0
	rts

	.sect	.data
	.align	2
tick_link:
	.data4	0
	.data4	0
tick_cnt:
	.data4	0
isr_sp:
	.data4	0
isr_flag:
	.data2	0
	.data2	0
qimi_link:
	.data4	0
	.data4	0
qimi_dx:
	.data4	0
qimi_dy:
	.data4	0

	.sect	.bss
	.align	2
isr_pila:
	.space	2048
