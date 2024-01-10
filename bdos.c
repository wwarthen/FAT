/*************************************************************************
*  bdos.c
*************************************************************************/

#include "bdos.h"

WORD bdoscall(BYTE fn, WORD parm) __naked
{
	fn;
	parm;

	__asm

	bdos = #5
	
	ld		c,a		// BDOS func to A, parm already in DE
	call	bdos	// call BDOS
	ex		de,hl	// move BDOS return val HL -> DE
	ret

	__endasm;
}
