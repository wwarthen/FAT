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
  
  // IX and IY are borked by CP/NET client!!! Preserve for SDCC!!!
  push  ix
  push  iy
	
	ld		c,(hl)
	inc		hl
	ld		e,(hl)
	inc		hl
	ld		d,(hl)
	
	call		bdos				// returns w/ result in HL
  
  pop iy
  pop ix
  
  ret
	
	__endasm;
}
