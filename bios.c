/*************************************************************************
*  bios.c
*************************************************************************/

#include "bios.h"

unsigned char bios_id = 0;

unsigned char chkbios(void) __naked
{
	__asm
	
	// Check for UNA (UBIOS)
	ld		a,(#0xfffd)		; fixed location of UNA API vector
	cp		#0xc3			; jp instruction?
	jr		nz,idbio1		; if not, not UNA
	ld		hl,(#0xfffe)	; get jp address
	ld		a,(hl)			; get byte at target address
	cp		#0xfd			; first byte of UNA push ix instruction
	jr		nz,idbio1		; if not, not UNA
	inc		hl				; point to next byte
	ld		a,(hl)			; get next byte
	cp		#0xe5			; second byte of UNA push ix instruction
	jr		nz,idbio1		; if not, not UNA, check others
	ld		a,#2			; UNA BIOS id = 2
	jr		idbio3			; and done

idbio1:
	// Check for RomWBW (HBIOS)
	ld		hl,(0xfffe)		; HL := HBIOS ident location
	ld		a,#'W'			; First byte of ident
	cp		(hl)			; Compare
	jr		nz,idbio2		; Not HBIOS
	inc		hl				; Next byte of ident
	ld		a,#~'W'			; Second byte of ident
	cp		(hl)			; Compare
	jr		nz,idbio2		; Not HBIOS
	ld		a,#1			; HBIOS BIOS id = 1
	jr		idbio3			; and done

idbio2:
	// Unknown
	xor		a				; Setup return value of 0

idbio3:
	ld	(_bios_id),a		; Update global
	ret						; and done
	
	__endasm;
}

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
