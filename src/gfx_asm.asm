! HeroQuest QL - rutinas graficas aceleradas en ensamblador (as68 de XTC68). Equivalen a las de gfx.c (que se usan en el PC y
! cuando no se define QL_ASM). Convencion de c68: argumentos en la pila (4(sp) = primero), se conservan D3-D7 y A2-A6.
! Pantalla del QL modo 8: 128 bytes por linea; una palabra (2 bytes) son 4 puntos.
	.sect	.text
	.align	2
	.extern	_asm_clear
	.extern	_asm_blit_opq
	.extern	_asm_blit_mask
	.extern	_asm_char
	.extern	_asm_rect
	.extern	_asm_text
	.extern	_asm_mkpair

! void asm_clear(u8 *p, long nbloques): pone a cero nbloques * 32 bytes (p alineado a palabra)
_asm_clear:
	movem.l	d2-d7,-(sp)
	move.l	28(sp),a0
	move.l	32(sp),d0
	beq	clr_fin
	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d4
	moveq	#0,d5
	moveq	#0,d6
	moveq	#0,d7
	move.l	d1,a1
	move.l	d0,d1
	asl.l	#5,d1
	add.l	d1,a0			! se rellena hacia atras (movem solo admite predecremento)
	moveq	#0,d1
clr_bucle:
	movem.l	d1-d7/a1,-(a0)
	subq.l	#1,d0
	bne	clr_bucle
clr_fin:
	movem.l	(sp)+,d2-d7
	rts

! void asm_blit_opq(const u8 *src, u8 *dst, long palabras_por_fila, long filas): copia opaca; la pantalla tiene 128 bytes por linea
_asm_blit_opq:
	movem.l	d2-d3,-(sp)
	move.l	12(sp),a0
	move.l	16(sp),a1
	move.l	20(sp),d2
	move.l	24(sp),d3
	subq.l	#1,d3
	bmi	opq_fin
	move.l	#128,d1
	move.l	d2,d0
	add.l	d0,d0
	sub.l	d0,d1			! D1 = 128 - 2 * palabras: salto a la linea siguiente
	subq.l	#1,d2
opq_fila:
	move.l	d2,d0
opq_pal:
	move.w	(a0)+,(a1)+
	dbra	d0,opq_pal
	add.l	d1,a1
	dbra	d3,opq_fila
opq_fin:
	movem.l	(sp)+,d2-d3
	rts

! void asm_blit_mask(const u8 *src, u8 *dst, long palabras_por_fila, long filas)
!   cada palabra son 4 bytes: mascara par, mascara impar, color par, color impar; pantalla = (pantalla AND NOT mascara) OR color
_asm_blit_mask:
	movem.l	d2-d5,-(sp)
	move.l	20(sp),a0
	move.l	24(sp),a1
	move.l	28(sp),d2
	move.l	32(sp),d3
	subq.l	#1,d3
	bmi	msk_fin
	move.l	#128,d1
	move.l	d2,d0
	add.l	d0,d0
	sub.l	d0,d1
	subq.l	#1,d2
msk_fila:
	move.l	d2,d0
msk_pal:
	move.b	(a0)+,d4
	move.b	(a0)+,d5
	not.b	d4
	not.b	d5
	and.b	d4,(a1)
	and.b	d5,1(a1)
	move.b	(a0)+,d4
	move.b	(a0)+,d5
	or.b	d4,(a1)+
	or.b	d5,(a1)+
	dbra	d0,msk_pal
	add.l	d1,a1
	dbra	d3,msk_fila
msk_fin:
	movem.l	(sp)+,d2-d5
	rts

! void asm_char(u8 *dst, const u8 *glifo, const u16 *tinta, const u16 *papel): una celda de 8 x 8 (4 bytes por linea).
!   tinta[n] y papel[n]: palabra de 4 puntos para el nibble n (papel ya complementado: se usa el mismo n)
_asm_char:
	movem.l	d2-d4/a2-a3,-(sp)
	move.l	24(sp),a0
	move.l	28(sp),a1
	move.l	32(sp),a2
	move.l	36(sp),a3
	moveq	#7,d4
chr_fila:
	moveq	#0,d0
	move.b	(a1)+,d0
	move.w	d0,d1
	lsr.w	#4,d0
	and.w	#15,d1
	add.w	d0,d0
	add.w	d1,d1
	move.w	0(a2,d0.w),d2
	or.w	0(a3,d0.w),d2
	move.w	d2,(a0)
	move.w	0(a2,d1.w),d2
	or.w	0(a3,d1.w),d2
	move.w	d2,2(a0)
	lea	128(a0),a0
	dbra	d4,chr_fila
	movem.l	(sp)+,d2-d4/a2-a3
	rts

! void asm_rect(const u8 *src, u8 *dst, long bytes, long filas, long sstride, long dstride): copia un rectangulo (bytes pares por fila)
_asm_rect:
	movem.l	d2-d5/a2-a3,-(sp)
	move.l	28(sp),a0
	move.l	32(sp),a1
	move.l	36(sp),d2
	move.l	40(sp),d3
	move.l	44(sp),d4
	move.l	48(sp),d5
	subq.l	#1,d3
	bmi	rec_fin
	lsr.l	#1,d2			! palabras por fila
	subq.l	#1,d2
rec_fila:
	move.l	d2,d0
	move.l	a0,a2
	move.l	a1,a3
rec_pal:
	move.w	(a2)+,(a3)+
	dbra	d0,rec_pal
	add.l	d4,a0
	add.l	d5,a1
	dbra	d3,rec_fila
rec_fin:
	movem.l	(sp)+,d2-d5/a2-a3
	rts

! void asm_mkpair(u32 *tab, const u16 *tinta, const u16 *papel): tab[b] = las dos palabras (4 puntos cada una) de una fila de glifo con el
!   byte b (puntos a 1 = tinta, a 0 = papel); el nibble alto va en la palabra alta (a la izquierda)
_asm_mkpair:
	movem.l	d2/a2,-(sp)
	move.l	12(sp),a0
	move.l	16(sp),a1
	move.l	20(sp),a2
	add.l	#1024,a0
	move.w	#255,d2
mkp_bucle:
	move.w	d2,d0
	lsr.w	#4,d0
	add.w	d0,d0
	move.w	0(a1,d0.w),d1
	or.w	0(a2,d0.w),d1
	swap	d1
	move.w	d2,d0
	and.w	#15,d0
	add.w	d0,d0
	move.w	0(a1,d0.w),d1
	or.w	0(a2,d0.w),d1
	move.l	d1,-(a0)
	dbra	d2,mkp_bucle
	movem.l	(sp)+,d2/a2
	rts

! void asm_text(u8 *dst, const u8 *font, const u8 *s, long n, const u32 *tab): n celdas de texto seguidas (8 x 8: 4 bytes por linea,
!   128 bytes entre lineas). Codigos 0x30 a 0x84 (FONT_GLYPHS = 85): glifo (codigo - 0x30) * 8 de la fuente; el resto, vacio. tab: asm_mkpair del color.
_asm_text:
	movem.l	d2-d3/a2-a5,-(sp)
	move.l	28(sp),a0
	move.l	32(sp),a1
	move.l	36(sp),a2
	move.l	40(sp),d2
	move.l	44(sp),a3
	subq.l	#1,d2
	bmi	txt_fin
txt_cel:
	moveq	#0,d0
	move.b	(a2)+,d0
	sub.w	#48,d0
	bcs	txt_vacio
	cmp.w	#85,d0
	bcc	txt_vacio
	lsl.w	#3,d0
	lea	0(a1,d0.w),a5
	bra	txt_filas
txt_vacio:
	move.l	#blank_g,a5
txt_filas:
	moveq	#0,d0
	move.b	(a5)+,d0
	add.w	d0,d0
	add.w	d0,d0
	move.l	0(a3,d0.w),0(a0)
	moveq	#0,d0
	move.b	(a5)+,d0
	add.w	d0,d0
	add.w	d0,d0
	move.l	0(a3,d0.w),128(a0)
	moveq	#0,d0
	move.b	(a5)+,d0
	add.w	d0,d0
	add.w	d0,d0
	move.l	0(a3,d0.w),256(a0)
	moveq	#0,d0
	move.b	(a5)+,d0
	add.w	d0,d0
	add.w	d0,d0
	move.l	0(a3,d0.w),384(a0)
	moveq	#0,d0
	move.b	(a5)+,d0
	add.w	d0,d0
	add.w	d0,d0
	move.l	0(a3,d0.w),512(a0)
	moveq	#0,d0
	move.b	(a5)+,d0
	add.w	d0,d0
	add.w	d0,d0
	move.l	0(a3,d0.w),640(a0)
	moveq	#0,d0
	move.b	(a5)+,d0
	add.w	d0,d0
	add.w	d0,d0
	move.l	0(a3,d0.w),768(a0)
	moveq	#0,d0
	move.b	(a5)+,d0
	add.w	d0,d0
	add.w	d0,d0
	move.l	0(a3,d0.w),896(a0)
	addq.l	#4,a0
	dbra	d2,txt_cel
txt_fin:
	movem.l	(sp)+,d2-d3/a2-a5
	rts

	.sect	.data
blank_g:
	.data4	0
	.data4	0
