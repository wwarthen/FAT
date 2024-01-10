#include "bdos.h"
#include "stdio.h"

int putchar(int ch)
{
	if (ch == 10)			// if LF
		BDOS_CONOUT(13);	//... insert CR
	BDOS_CONOUT(ch);

	return(ch);
}

int getchar(void)
{
	int ch;

	while (!(ch = BDOS_DIRIO(0xFF)));
	if (ch == 13) return 10;
	if (ch == 10) return 13;
	if (ch == 26) return -1;
	return(ch);
}
