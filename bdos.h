#ifndef _BDOS_H
#define _BDOS_H

#include "ff.h"

unsigned int bdoscall(unsigned char fn, unsigned int DE) __naked;

#define BDOS_SYSRES() (BYTE)bdoscall(0, 0)
#define BDOS_CONIN() (BYTE)bdoscall(1, 0)
#define BDOS_CONOUT(ch) (BYTE)bdoscall(2, ch)
#define BDOS_RDRIN() (BYTE)bdoscall(3, 0)
#define BDOS_DIRIO(ch) (BYTE)bdoscall(6, ch)
#define BDOS_PRTSTR(str) (BYTE)bdoscall(9, (WORD)str)
#define BDOS_GETVER() (WORD)bdoscall(12, 0)
#define BDOS_OPENFILE(fcb) (BYTE)bdoscall(15, fcb)
#define BDOS_CLOSEFILE(fcb) (BYTE)bdoscall(16, fcb)
#define BDOS_FINDFIRST(fcb) (BYTE)bdoscall(17, fcb)
#define BDOS_FINDNEXT(fcb) (BYTE)bdoscall(18, fcb)
#define BDOS_DELETE(fcb) (BYTE)bdoscall(19, fcb)
#define BDOS_READSEQ(fcb) (BYTE)bdoscall(20, fcb)
#define BDOS_WRITESEQ(fcb) (BYTE)bdoscall(21, fcb)
#define BDOS_MAKEFILE(fcb) (BYTE)bdoscall(22, fcb)
#define BDOS_SETDMA(dma) (BYTE)bdoscall(26, dma)
#define BDOS_GETALLOC() (WORD)bdoscall(27, 0)

typedef struct {
	BYTE drv;		// Drive code
	BYTE name[8];	// File name
	BYTE ext [3];	// File typedef
	BYTE ex;		// Extent number
	BYTE s1;		// Reserved
	BYTE s2;		// Reserved (set to zero)
	BYTE rc;		// Record count for extent
	BYTE map[16];	// Disk map
	BYTE cr;		// Current record
	BYTE rn[3];		// Random record number
} FCB;

#endif /* _BDOS_H */
