#ifndef _BIOS_H
#define _BIOS_H

#include "type.h"

#define BIOS_UNK 0
#define	BIOS_WBW 1
#define	BIOS_UNA 2

struct BYTEREGS
{
	BYTE	C;
	BYTE	B;
	BYTE	E;
	BYTE	D;
	BYTE	L;
	BYTE	H;
	BYTE	F;
	BYTE	A;
};

struct WORDREGS
{
	WORD BC;
	WORD DE;
	WORD HL;
	WORD AF;
};

typedef union
{
	struct BYTEREGS b;
	struct WORDREGS w;
} REGS;

BYTE chkbios(void) __naked;
void bioscall(REGS *out, REGS *in) __sdcccall(0) __naked;

#endif /* _BIOS_H */
