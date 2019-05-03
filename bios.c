/*************************************************************************
*  bios.c
*************************************************************************/

#include "bios.h"

void bioscall(REGS *out, REGS *in) __naked
{
	in;
	out;

	__asm

	// Set HL to start of in struct
	ld		hl,#4
	add		hl,sp
	ld		a,(hl)
	inc		hl
	ld		h,(hl)
	ld		l,a

	// Save 4 register pairs on stack from in struct
	ld		b,#4
loop1:
	ld		e,(hl)
	inc		hl
	ld		d,(hl)
	inc		hl
	push	de
	djnz	loop1

	// Set registers from stack
	pop		af
	pop		hl
	pop		de
	pop		bc

	// BIOS call!
	rst		8

	// Save registers to stack
	push	af
	push	hl
	push	de
	push	bc

	// Set HL to start of out struct
	ld		hl,#10
	add		hl,sp
	ld		a,(hl)
	inc		hl
	ld		h,(hl)
	ld		l,a

	// Save 4 register pairs from stack to out struct
	ld		b,#4
loop2:
	pop		de
	ld		(hl),e
	inc		hl
	ld		(hl),d
	inc		hl
	djnz	loop2

	ret

	__endasm;
}
