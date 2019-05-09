void putchar(char ch) __naked
{
	ch;		// Avoid compiler warnings

	// LF --> CR LF
	if (ch == '\n') putchar('\r');

	__asm
	ld	hl, #2			; point to parm
	add hl, sp			; ... at stack offset
	ld	e, (hl)			; load output char to e
	ld	c, #2			; console out function
	jp	5				; do it, ret is implicit
	__endasm;
}

int getchar(void) __naked
{
	__asm
loop:
	ld	c, #6			; direct console I/O
	ld	e, #0xFF		; read
	call	5			; call BDOS
	or	a				; set flags
	jr	z, loop			; if zero, no char, loop
gotit:
	cp	#26				; is it ctrl-Z?
	jr	nz, not_EOF
	ld	hl, #-1			; signal EOF
	ret
not_EOF:
	ld	l, a			; character to L register
	rla					; bit 7 to Carry
	sbc	a, a			; C==0 -> 00,   C==1 -> FF
	ld	h, a			; sign-extend character input
	ret
	__endasm;
}
