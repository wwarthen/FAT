;--------------------------------------------------------------------------
;  crt0.s - Generic crt0.s for a Z80
;
;  Copyright (C) 2000, Michael Hope
;
;  This library is free software; you can redistribute it and/or modify it
;  under the terms of the GNU General Public License as published by the
;  Free Software Foundation; either version 2, or (at your option) any
;  later version.
;
;  This library is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License 
;  along with this library; see the file COPYING. If not, write to the
;  Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
;   MA 02110-1301, USA.
;
;  As a special exception, if you link this library with other files,
;  some of which are compiled with SDCC, to produce an executable,
;  this library does not by itself cause the resulting executable to
;  be covered by the GNU General Public License. This exception does
;  not however invalidate any other reasons why the executable file
;   might be covered by the GNU General Public License.
;--------------------------------------------------------------------------

	.module ucrt0
	.globl	_main
	.globl	l__INITIALIZER, s__INITIALIZED, s__INITIALIZER
	.globl	l__DATA, s__DATA
	.globl	___sdcc_external_startup

	;; Ordering of segments for the linker.
	.area	_HEADER (ABS)
	.area	_HOME
	.area	_CODE
	.area	_INITIALIZER
	.area	_GSINIT
	.area	_GSFINAL

	.area	_DATA
	.area	_INITIALIZED
	.area	_BSEG
	.area	_BSS
	.area	_HEAP

	.area	_CODE
init:
	;; Set stack pointer directly below BDOS.
	ld	hl, (#0x0006)	; get BDOS call vector
	ld	l, #0x00	; throw away LSB
	ld	sp, hl		; result is TOS

	call	___sdcc_external_startup

	;; Initialise global variables. Skip if __sdcc_external_startup returned
	;; non-zero value. Note: calling convention version 1 only.
	or	a, a
	call	Z, gsinit

	;; Setup argc, argv
	ld	hl, #1		; assume no parms (just prog name)
	ld	a, (#0x0080)	; get cmd length prefix byte
	or	a		; zero?
	jr	z, noargs	; if so, move ahead
	inc	hl		; bump hl if we have cmd args
noargs:
	ld	de, #argv	; point to argument array

	call	_main		; do main(), return code in de
	jr	__exit		; and go to exit processing

_exit::
	ex	de,hl		; move exit code hl->de, fall thru

__exit:
	; Map return code in de to CP/M 3 conventions:
	;   0x0000-0xFEFF for success
	;   0xFF00-0xFFFE for failure
	ld	a,d
	or	e
	jr	z,1$		; zero is success, use as is
	ld	d,#0xff		; treat as CP/M 3 error ($FFnn)
1$:
	ld	c,#108		; CP/M 3 func to set return code
	call	#0x0005		; call CP/M

	jp	0		; back to CP/M

argv:	.dw	__main		; name of program
	.dw	0x0081		; CP/M command args
	.dw	0		; final null (mandatory per C lang spec)

__main:	.asciz	"main"

	.area	_GSINIT
gsinit::

	; Default-initialized global variables.
	ld	bc, #l__DATA
	ld	a, b
	or	a, c
	jr	Z, zeroed_data
	ld	hl, #s__DATA
	ld	(hl), #0x00
	dec	bc
	ld	a, b
	or	a, c
	jr	Z, zeroed_data
	ld	e, l
	ld	d, h
	inc	de
	ldir

zeroed_data:
	; Explicitly initialized global variables.
	ld	bc, #l__INITIALIZER
	ld	a, b
	or	a, c
	jr	Z, gsinit_next
	ld	de, #s__INITIALIZED
	ld	hl, #s__INITIALIZER
	ldir

gsinit_next:

	.area	_GSFINAL
	ret
