#ifndef _BIOS_H
#define _BIOS_H

struct BYTEREGS
{
	unsigned char	C;
	unsigned char	B;
	unsigned char	E;
	unsigned char	D;
	unsigned char	L;
	unsigned char	H;
	unsigned char	F;
	unsigned char	A;
};

struct WORDREGS
{
	unsigned int BC;
	unsigned int DE;
	unsigned int HL;
	unsigned int AF;
};

typedef union
{
	struct BYTEREGS b;
	struct WORDREGS w;
} REGS;

void bioscall(REGS *out, REGS *in) __naked;

#endif /* _BIOS_H */
