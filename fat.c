/**********************************************************************

HBIOS CP/M FAT Utility ("FAT.COM")

Author: Wayne Warthen
Updated: 2-May-2019

LICENSE:
	GNU GPLv3 (see file LICENSE.txt)

**********************************************************************/

#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "bdos.h"
#include "ff.h"

#define MAX_FN 11
#define MAX_PATH 255

#define FALSE 0
#define TRUE (!FALSE)

#define FS_UNK 0
#define FS_FAT 1
#define FS_CPM 2

char * ErrTab[] =
{
	"Successful Completion",              // FR_OK
	"Disk I/O Error",                     // FR_DISK_ERR
	"Internal Error - Assertion Failed",  // FR_INT_ERR
	"Drive Not Ready",                    // FR_NOT_READY
	"File Not Found",                     // FR_NO_FILE
	"Path Not Found",                     // FR_NO_PATH
	"Invalid Path Name",                  // FR_INVALID_NAME
	"Access Denied",                      // FR_DENIED
	"Exists",                             // FR_EXIST
	"Invalid Object",                     // FR_INVALID_OBJECT
	"Write Protected",                    // FR_WRITE_PROTECTED
	"Invalid Drive",                      // FR_INVALID_DRIVE
	"Volume Not Mounted",                 // FR_NOT_ENABLED
	"No Filesystem on Drive",             // FR_NO_FILESYSTEM
	"Make Filesystem Failed",             // FR_MKFS_ABORTED
	"Timeout",                            // FR_TIMEOUT
	"Locked",                             // FR_LOCKED
	"Insuffieient Memory",                // FR_NOT_ENOUGH_CORE
	"Too Many Open Files",                // FR_TOO_MANY_OPEN_FILES
	"Invalid Parameter"                   // FR_INVALID_PARAMETER
};

char * strupr(char * str)
{
	char * s;
	
	for (s = str; *s; s++)
		*s = toupper(*s);
	
	return str;
}

int Error(FRESULT fr)
{
	printf("\n\nError: %s\n", ErrTab[fr]);
	
	return 8;
}

int Usage(void)
{
	printf(
		"\nCP/M FAT Utility v0.9, 2-May-2019 [%s]"
		"\nCopyright (C) 2019, Wayne Warthen, GNU GPL v3"
		"\n"
		"\nUsage: FAT <cmd> <parms>"
		"\n  FAT DIR <path>"
		"\n  FAT COPY <src> <dst>"
		"\n  FAT REN <fn1> <fn2>"
		"\n  FAT DEL <fn>"
		"\n"
		"\nCP/M filespec: <d>:FILENAME.EXT (where <d> is A-P)"
		"\nFAT filespec:  <u>:/DIR/FILENAME.EXT (where <u> is disk unit #)"
		"\n"
		, "HBIOS"
	);
	
	return 4;
}

void DirLine(FILINFO *pfno)
{
	printf("\n%02u/%02u/%04u  ",
		((pfno->fdate >> 5) & 0x0F),
		((pfno->fdate >> 0) & 0x1F),
		(((pfno->fdate >> 9) & 0x7F) + 1980)
		);

	printf("%02u:%02u:%02u  ",
		((pfno->ftime >> 11) & 0x1F),
		((pfno->ftime >> 5) & 0x3F),
		(((pfno->ftime >> 0) & 0x1F) << 1)
		);
		
	// Print attributes???

    if (pfno->fattrib & AM_DIR)                    /* It is a directory */
		printf("  <dir>       ");
	else
		printf("%12lu  ", pfno->fsize);

    printf("%s", pfno->fname);
}

FRESULT SplitPath(char * szPath, char * szFileSpec)
{
	char * p;
		
	// Find terminal separator
	p = szPath + strlen(szPath);
	while ((p > szPath) && (*p != '/') && (*p != '\\') && (*p != ':'))
		p--;
	
	// Bump past the separator character
	if (p > szPath)
		p++;

	// Extract filename
	memset(szFileSpec, 0, MAX_FN + 1);
	strncpy(szFileSpec, p, MAX_FN);

	// Truncate path
	*p = '\0';

	// Debug...
	//printf("\nPath='%s'", szPath);
	//printf("\nFileSpec='%s'", szFileSpec);
	
	return FR_OK;
}

FRESULT Dir(void)
{
	FATFS fs;
	FRESULT fr;
	DIR dir;
	FILINFO fno;
	char * szPath;
	char szFileSpec[MAX_FN];
	
	szPath = strtok(NULL, " ");
	if (szPath == NULL)
		return FR_INVALID_PARAMETER;

	fr = f_mount(&fs, szPath, 0);
	if (fr != FR_OK)
		return fr;
	
	fr = SplitPath(szPath, szFileSpec);
	if (fr != FR_OK)
		return fr;
	
	if (*szFileSpec == '\0')
		strcpy(szFileSpec, "*");

	fr = f_findfirst(&dir, &fno, szPath, szFileSpec);

	if (fr == FR_OK)
		printf("\nDirectory of %s\n", szPath);

	while ((fr == FR_OK) && (fno.fname[0]))
	{
		DirLine(&fno);
		fr = f_findnext(&dir, &fno);
	}

	f_mount(0, szPath, 0);		// unmount ignoring any errors
	
	return fr;
}

int IsWild(char * szPath)
{
	char * p;

	for (p = szPath; *p != '\0'; p++)
		if ((*p == '*') || (*p == '?'))
			return TRUE;
	
	return FALSE;
}

int IsFatPath(char * szPath)
{
	char * p;
	
	p = szPath;

	while (*p != '\0')
	{
		if (*p == ':')
			return TRUE;
		if ((*p < '0') || (*p > '9'))
			return FALSE;
		p++;
	}

	return FALSE;
}

int FatDrive(char * szPath)
{
	int i;
	char * p;
	char szDrive[4];
	
	i = 0;
	p = szPath;
	
	for (;;)
	{
		if ((*p >= '0') && (*p <= '9'))
		{
			szDrive[i++] = *p;
			if (i >= 2)
				return -1;
		}
		else if (*p == ':')
			break;
		else
			return -1;
		p++;
	}
	szDrive[i] = '\0';

	return atoi(szDrive);
}

int IsValidFilenameChar(char c)
{
	int n;
	char sBadCharList[] = "<>.,;:=?*[]_%|()/\\";
	
	if ((c <= ' ') || (c > '~'))
		return FALSE;
	
	for (n = 0; n < sizeof(sBadCharList); n++)
		if (c == sBadCharList[n])
			return FALSE;
		
	return TRUE;
}

FRESULT MakeFCB(const TCHAR * path, FCB * pfcb)
{
	const TCHAR * p;
	int n;
	
	p = path;
	
	if (p[1] == ':')
	{
		if ((*p >= 'A') && (*p <= 'P'))
			pfcb->drv = *(p++) - 'A' + 1;
		else
			return FR_INVALID_NAME;
		p++;
	}
	else
		pfcb->drv = 0;

	if (*p == '\0')
		return FR_INVALID_NAME;

	for (n = 0; n < 8; n++)
	{
		if ((*p == '\0') || (*p == '.'))
			pfcb->name[n] = ' ';
		else if (*p == '*')
		{
			pfcb->name[n] = '?';
		}
		else
		{
			if (!IsValidFilenameChar(*p))
				return FR_INVALID_NAME;
			pfcb->name[n] = *(p++);
		}
	}
	
	while ((*p != '\0') && (*p != '.'))
		p++;
	
	if (*p == '.')
		p++;
	
	if (p[-1] == '*')
		p--;
	
	for (n = 0; n < 3; n++)
	{
		if (*p == '\0')
			pfcb->ext[n] = ' ';
		else if (*p == '*')
			pfcb->ext[n] = '?';
		else
			pfcb->ext[n] = *(p++);
	}
	
	//printf("\nFCB: %i, ", pfcb->drv);
	//for (n = 0; n < 8; n++)
	//	printf("%c", pfcb->name[n]);
	//printf(".");
	//for (n = 0; n < 3; n++)
	//	printf("%c", pfcb->ext[n]);
	
	
	return FR_OK;
}

FRESULT cf_open(FCB * pfcb, const TCHAR * path, BYTE mode)
{
	mode;
	
	BYTE rc;
	FRESULT fr;
	BYTE bufTemp[128];
	
	fr = MakeFCB(path, pfcb);
	
	if (fr != FR_OK)
		return fr;
	
	if (mode & FA_READ)
	{
		rc = BDOS_OPENFILE((WORD)pfcb);
		return (rc == 0xFF) ? FR_NO_FILE : FR_OK;
	}

	if (mode & FA_WRITE)
	{
		BDOS_SETDMA((WORD)&bufTemp);
		
		rc = BDOS_FINDFIRST((WORD)pfcb);
		
		// printf("\nBDOS FindFirst(): %i", rc);

		if (rc != 0xFF)
			return FR_EXIST;
		
		rc = BDOS_MAKEFILE((WORD)pfcb);
		
		// printf("\nBDOS MakeFile(): %i", rc);

		if (rc == 0xFF)
			return FR_DISK_ERR;		// This should really be "out of space"
		
		return FR_OK;
	}
	
	return FR_INVALID_PARAMETER;
}

FRESULT cf_close(FCB * pfcb)
{
	BYTE rc;
	
	rc = BDOS_CLOSEFILE((WORD)pfcb);
	
	// printf("\nBDOS CloseFile(): %i", rc);
	
	if (rc == 0xFF)
		return FR_INVALID_OBJECT;

	return FR_OK;
}

FRESULT cf_read(FCB * pfcb, void * pbuf, UINT btr, UINT * br)
{
	BYTE rc;

	if (btr != 128)
		return FR_INVALID_PARAMETER;
	
	BDOS_SETDMA((WORD)pbuf);
	
	rc = BDOS_READSEQ((WORD)pfcb);
	
	//printf("\nBDOS ReadSeq(): %i", rc);

	// Non-zero return indicates EOF	
	if (rc == 0)
		*br = 128;
	else
		*br = 0;

	return FR_OK;
}

FRESULT cf_write(FCB * pfcb, void * pbuf, UINT btw, UINT * bw)
{
	BYTE rc;

	if (btw != 128)
		return FR_INVALID_PARAMETER;
	
	BDOS_SETDMA((WORD)pbuf);
	
	rc = BDOS_WRITESEQ((WORD)pfcb);
	
	// printf("\nBDOS WriteSeq(): %i", rc);
	
	if (rc != 0)
		return FR_DISK_ERR; // Actually "out of space"

	*bw = 128;
	return FR_OK;
}

FRESULT CopyFile(char * szSrcFile, char * szDestSpec)
{
	FRESULT fr;
	FIL filSrc, filDest;
	FCB fcbSrc, fcbDest;
	
	//printf("\n  CopyFile() %s ==> %s", szSrcFile, szDestSpec);
	printf("\n%s ==> %s", szSrcFile, szDestSpec);
	
	//printf("\nSrcFile %s FATPATH", IsFatPath(szSrcFile) ? "IS" : "NOT");
	//printf("\nDestSpec %s FATPATH", IsFatPath(szDestSpec) ? "IS" : "NOT");
	
	memset(&fcbSrc, 0, sizeof(fcbSrc));
	memset(&fcbDest, 0, sizeof(fcbDest));

	if (IsFatPath(szSrcFile))
		fr = f_open(&filSrc, szSrcFile, FA_READ);
	else
		fr = cf_open(&fcbSrc, szSrcFile, FA_READ);
	
	if (fr == FR_OK)
	{
		if (IsFatPath(szDestSpec))
			fr = f_open(&filDest, szDestSpec, FA_WRITE | FA_CREATE_ALWAYS);
		else
			fr = cf_open(&fcbDest, szDestSpec, FA_WRITE | FA_CREATE_ALWAYS);
		
		if (fr == FR_OK)
		{
			UINT br, bw;
			BYTE buf[128];
			
			do
			{
				br = 0;
				memset(buf, 0x1A, 128);
			
				if (IsFatPath(szSrcFile))
					fr = f_read(&filSrc, buf, 128, &br);
				else
					fr = cf_read(&fcbSrc, buf, 128, &br);
				
				if (fr != FR_OK)
					break;
				
				// printf(" <%i", br);
				
				if (br > 0)
				{
					bw = 0;

					if (IsFatPath(szDestSpec))
						fr = f_write(&filDest, buf, 128, &bw);
					else
						fr = cf_write(&fcbDest, buf, 128, &bw);

					if (fr != FR_OK)
						break;
					
					if (bw < 128)
					{
						// This is actually an out of space condition!!!
						fr = FR_DISK_ERR;
						break;
					}
					
					// printf(" >%i", bw);
				}
			}	while (br == 128);
		}
		
		if (IsFatPath(szDestSpec))
			f_close(&filDest);
		else
			cf_close(&fcbDest);
	}
	
	if (IsFatPath(szSrcFile))
		f_close(&filSrc);
	else
		cf_close(&fcbSrc);

	return fr;
}

FRESULT FatCopy(char * szSrcPath, char * szDestPath)
{
  FRESULT fr;
	int nFiles;
  DIR dir;
  FILINFO fno;
	char szFileSpec[MAX_FN];
	char szDestFileSpec[MAX_FN];

	// printf("\nFatCopy()...");
	
	nFiles = 0;

	fr = SplitPath(szSrcPath, szFileSpec);
	if (fr != FR_OK)
		return fr;
	
	fr = SplitPath(szDestPath, szDestFileSpec);
	if (fr != FR_OK)
		return fr;
	
	if (*szFileSpec == '\0')
		return FR_INVALID_PARAMETER;
	
	if (IsWild(szFileSpec) && (*szDestFileSpec != '\0'))
		return FR_INVALID_PARAMETER;
	
	fr = f_findfirst(&dir, &fno, szSrcPath, szFileSpec);
	
	while ((fr == FR_OK) && (fno.fname[0]))
	{
		char szSrcFile[MAX_PATH];
		char szDestFile[MAX_PATH];
		
		if (!(fno.fattrib & AM_DIR))
		{
			//printf("\n%s", fno.fname);

			strncpy(szSrcFile, szSrcPath, sizeof(szSrcFile) - 1);
			//strncat(szSrcFile, "/", sizeof(szSrcFile) - 1);
			strncat(szSrcFile, fno.fname, sizeof(szSrcFile) - 1);
			
			strncpy(szDestFile, szDestPath, sizeof(szDestFile) - 1);
			//if (IsFatPath(szDestPath))
			//	strncat(szDestFile, "/", sizeof(szDestFile) - 1);
			strncat(szDestFile, (*szDestFileSpec == '\0') ? fno.fname : szDestFileSpec, sizeof(szDestFile) - 1);
			
			fr = CopyFile(szSrcFile, szDestFile);
			if (fr != FR_OK)
				return fr;
			
			nFiles++;
		}
		
		fr = f_findnext(&dir, &fno);
	}
	
	if (nFiles)
		printf("\n\n    %i File(s) Copied", nFiles);
	else
		fr = FR_NO_FILE;

	return fr;
}

FRESULT CpmCopy(char * szSrcPath, char * szDestPath)
{
	FRESULT fr;
	int rc;
	int nFiles;
	FCB fcb, fcbSave;
	BYTE buf[128];
	FCB * dirent;
	char szFileSpec[MAX_FN];
	char szDestFileSpec[MAX_FN];

	szDestPath;

	//printf("\nCpmCopy()...");
	
	nFiles = 0;
	
	memset(&fcb, 0, sizeof(fcb));
	fr = MakeFCB(szSrcPath, &fcb);
	memcpy(&fcbSave, &fcb, sizeof(fcb));
	
	if (fr != FR_OK)
		return fr;
	
	fr = SplitPath(szSrcPath, szFileSpec);
	if (fr != FR_OK)
		return fr;
	
	fr = SplitPath(szDestPath, szDestFileSpec);
	if (fr != FR_OK)
		return fr;
	
	if (*szFileSpec == '\0')
		return FR_INVALID_PARAMETER;
	
	if (IsWild(szFileSpec) && (*szDestFileSpec != '\0'))
		return FR_INVALID_PARAMETER;
	
	BDOS_SETDMA((WORD)&buf);
	
	rc = BDOS_FINDFIRST((WORD)&fcb);
	
	// printf("\nBDOS FindFirst(): %i", rc);
	
	while (rc != 0xFF)
	{
		char szSrcFile[MAX_PATH];
		char szDestFile[MAX_PATH];
		char *p, *p2;
		int n;

		dirent = (FCB *)(buf + (32 * rc));
		p = szSrcFile;

		if (fcb.drv > 0)
		{
			*(p++) = fcb.drv + 'A' - 1;
			*(p++) = ':';
		}
		
		p2 = p;			// Remember start of filename/ext
		
		for (n = 0; n < 8; n++)
		{
			if (dirent->name[n] == ' ')
				break;
			*(p++) = dirent->name[n];
		}
		
		*(p++) = '.';
		
		for (n = 0; n < 3; n++)
		{
			if (dirent->ext[n] == ' ')
				break;
			*(p++) = dirent->ext[n];
		}

		*(p++) = '\0';
		
		strncpy(szDestFile, szDestPath, sizeof(szDestFile) - 1);
		//if (IsFatPath(szDestPath))
		//	strncat(szDestFile, "/", sizeof(szDestFile) - 1);
		strncat(szDestFile, (*szDestFileSpec == '\0') ? p2 : szDestFileSpec, sizeof(szDestFile) - 1);
		
		fr = CopyFile(szSrcFile, szDestFile);
		if (fr != FR_OK)
			return fr;
	
		//printf("\nCopy File: %s", szSrcFile);
		
		nFiles++;
		
		BDOS_SETDMA((WORD)&buf);

		// Restart search from last file found		
		memcpy(fcb.name, dirent->name, sizeof(fcb.name));
		memcpy(fcb.ext, dirent->ext, sizeof(fcb.ext));
		memset(&fcb + 12, 0, sizeof(fcb) - 12);
		rc = BDOS_FINDFIRST((WORD)&fcb);
		if (rc == 0xFF)
			return FR_INVALID_OBJECT;
		
		//printf("\nBDOS FindFirst(): %i", rc);
	
		// Restore the search spec and find next
		memcpy(&fcb.name, &fcbSave.name, sizeof(fcb.name));
		memcpy(&fcb.ext, &fcbSave.ext, sizeof(fcb.ext));
		rc = BDOS_FINDNEXT((WORD)&fcb);

		//printf("\nBDOS FindNext(): %i", rc);
	}
	
	if (nFiles)
		printf("\n\n    %i File(s) Copied", nFiles);
	else
		fr = FR_NO_FILE;

	return fr;
}

FRESULT Copy(void)
{
	FRESULT fr;
	FATFS fsSrc;
	FATFS fsDest;
	char * szSrcPath;
	char * szDestPath;
	
	szSrcPath = strtok(NULL, " ");
	if (szSrcPath == NULL)
		return FR_INVALID_PARAMETER;

	szDestPath = strtok(NULL, " ");
	if (szDestPath == NULL)
		return FR_INVALID_PARAMETER;
	
	if (IsFatPath(szSrcPath))
	{
		fr = f_mount(&fsSrc, szSrcPath, 1);
		if (fr != FR_OK)
			return fr;
	}
	
	if (IsFatPath(szDestPath))
	{
		fr = f_mount(&fsDest, szDestPath, 1);
		if (fr != FR_OK)
			return fr;
	}
	
	if (IsFatPath(szSrcPath))
		fr = FatCopy(szSrcPath, szDestPath);
	else
		fr = CpmCopy(szSrcPath, szDestPath);
	
	f_mount(0, szSrcPath, 0);		// unmount ignoring any errors
	f_mount(0, szDestPath, 0);		// unmount ignoring any errors

	return fr;
}

FRESULT Rename(void)
{
	printf("\nREN function not yet implemented!!!");
	return FR_INVALID_PARAMETER;
}

FRESULT Delete(void)
{
	printf("\nDEL function not yet implemented!!!");
	return FR_INVALID_PARAMETER;
}

int main(int argc, char * argv[])
{
	char * tok;
	FRESULT fr;

	if (argc != 2)
		return Usage();
	
	tok = strtok(argv[1], " ");
	
	if (tok == NULL)
		return Usage();
	
	strupr(tok);
	
	if (!strcmp(tok, "DIR"))
		fr = Dir();
	else if (!strcmp(tok, "COPY"))
		fr = Copy();
	else if (!strcmp(tok, "REN"))
		fr = Rename();
	else if (!strcmp(tok, "DEL"))
		fr = Delete();
	else
		fr = FR_INVALID_PARAMETER;
	
	if (fr != FR_OK)
		return Error(fr);
	
	printf("\n");
	
	return 0;
}
