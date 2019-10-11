#ifndef _BIOS_H
#define _BIOS_H

#define BIOS_UNK 0
#define	BIOS_WBW 1
#define	BIOS_UNA 2

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

extern unsigned char bios_id;

//unsigned char idbios(void) __naked;
unsigned char chkbios(void);
void bioscall(REGS *out, REGS *in) __naked;

#endif /* _BIOS_H */
