/*************************************************************************
*  bdos.c
*************************************************************************/

#include "bdos.h"

unsigned int bdoscall(unsigned char fn, unsigned int DE) __naked
{
	fn;
	DE;

	__asm
	
	bdos = #5
	
	ld		hl,#2
	add		hl,sp
	
	ld		c,(hl)
	inc		hl
	ld		e,(hl)
	inc		hl
	ld		d,(hl)
	
	jp		bdos				// returns w/ result in HL
	
	__endasm;
}
