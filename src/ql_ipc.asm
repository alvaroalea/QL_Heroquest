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

	.sect	.bss
	.align	2
isr_pila:
	.space	2048
