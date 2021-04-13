/**********************************************************************

HBIOS CP/M FAT Utility ("FAT.COM")

Author: Wayne Warthen
Updated: 8-Oct-2019

LICENSE:
	GNU GPLv3 (see file LICENSE.txt)

**********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "bios.h"
#include "bdos.h"
#include "ff.h"

#define MAX_FN 12
#define MAX_PATH 255

#define FALSE 0
#define TRUE (!FALSE)

#define FS_UNK 0
#define FS_FAT 1
#define FS_CPM 2

#define RECLEN 128

#define CHUNKSIZE 32

typedef struct
{
	int	fstyp;
	union
	{
		FCB	fcb;
		FIL fil;
	};
} FILE;

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
	"Insufficient Memory",                // FR_NOT_ENOUGH_CORE
	"Too Many Open Files",                // FR_TOO_MANY_OPEN_FILES
	"Invalid Parameter"                   // FR_INVALID_PARAMETER
};

char * BiosName[] =
{
	"Unknown",
	"RomWBW HBIOS",
	"UNA UBIOS"
};

char * strupr(char * str)
{
	char * s;
	
	for (s = str; *s; s++)
		*s = toupper(*s);
	
	return str;
}

int Confirm(void)
{
	char c;
		
	do
		c = getchar();
	while ((c != 'y') && (c != 'Y') && (c != 'N') && (c != 'n'));
	
	return ((c == 'y') || (c == 'Y'));
}

int Error(FRESULT fr)
{
	printf("\n\nError: %s\n", ErrTab[fr]);
	
	return 8;
}

int Usage(void)
{
	printf(
		"\nCP/M FAT Utility v0.9.8 (beta), 12-Apr-2021 [%s]"
		"\nCopyright (C) 2021, Wayne Warthen, GNU GPL v3"
		"\n"
		"\nUsage: FAT <cmd> <parms>"
		"\n  FAT DIR <path>"
		"\n  FAT COPY <src> <dst>"
		"\n  FAT REN <from> <to>"
		"\n  FAT DEL <path>[<file>|<dir>]"
		"\n  FAT MD <path>"
		"\n  FAT FORMAT <drv>"
		"\n"
		"\nCP/M filespec: <d>:FILENAME.EXT (<d> is CP/M drive letter A-P)"
		"\nFAT filespec:  <u>:/DIR/FILENAME.EXT (<u> is disk unit #)"
		"\n",
		BiosName[bios_id]
	);
	
	return 4;
}

FRESULT SplitPath(char * szPath, char * szFileSpec)
{
	char * p;
		
	// Find terminal separator
	p = szPath + strlen(szPath);
	while ((p >= szPath) && (*p != '/') && (*p != '\\') && (*p != ':'))
		p--;
	
	// Extract filename
	memset(szFileSpec, 0, MAX_FN + 1);
	strncpy(szFileSpec, p + 1, MAX_FN);
	
	if (p >= szPath)
	{
		if (*p == ':')
			p++;
	}
	else
		p++;

	*p = '\0';		// truncate path

	// Debug...
	//printf("\nPath='%s'", szPath);
	//printf("\nFileSpec='%s'", szFileSpec);
	
	return FR_OK;
}

int IsWild(char * szPath)
{
	char * p;

	for (p = szPath; *p != '\0'; p++)
		if ((*p == '*') || (*p == '?'))
			return TRUE;
	
	return FALSE;
}

int IsFatPath(const TCHAR * szPath)
{
	const TCHAR * p;
	
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
	
	memset(pfcb, 0, sizeof(FCB));
	
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
	
	// DumpFCB(pfcb);
	
	return FR_OK;
}

void DumpFCB(FCB * pfcb)
{
	int n;
	
	printf("\nFCB: %i, ", pfcb->drv);
	for (n = 0; n < 8; n++)
		printf("%c", pfcb->name[n]);
	printf(".");
	for (n = 0; n < 3; n++)
		printf("%c", pfcb->ext[n]);
}

int Exists(const TCHAR * path)
{
	BYTE rc;
	FRESULT fr;
	FCB fcb;
	BYTE buf[RECLEN];
	
	if (IsFatPath(path))
	{
		FILINFO fno;

		fr = f_stat(path, &fno);
		
		return (fr == FR_OK);
	}
	
	fr = MakeFCB(path, &fcb);
	
	if (fr != FR_OK)
		return FALSE;
	
	// DumpFCB(&fcb);
		
	BDOS_SETDMA((WORD)&buf);
	
	rc = BDOS_FINDFIRST((WORD)&fcb);
	
	return (rc != 0xFF);
}

FRESULT DeleteFile(const TCHAR * path)
{
	FRESULT fr;
	FCB fcb;
	
	if (IsFatPath(path))
		return f_unlink(path);
	
	fr = MakeFCB(path, &fcb);
	
	BDOS_DELETE((WORD)&fcb);	// DELETE function has no return value

	return FR_OK;
}

FRESULT Open(FILE * pfile, const TCHAR * path, BYTE mode)
{
	if (pfile->fstyp == FS_FAT)
		return f_open(&pfile->fil, path, mode);
	
	BYTE rc;
	FRESULT fr;
	
	fr = MakeFCB(path, &pfile->fcb);
	
	if (fr != FR_OK)
		return fr;
	
	if (mode & FA_READ)
	{
		rc = BDOS_OPENFILE((WORD)pfile->fcb);
		return (rc == 0xFF) ? FR_NO_FILE : FR_OK;
	}

	if (mode & FA_WRITE)
	{
		rc = BDOS_MAKEFILE((WORD)pfile->fcb);
		
		// printf("\nBDOS MakeFile(): %i", rc);

		if (rc == 0xFF)
			return FR_DISK_ERR;		// This should really be "out of space"
		
		return FR_OK;
	}
	
	return FR_INVALID_PARAMETER;
}

FRESULT Close(FILE * pfile)
{
	if (pfile->fstyp == FS_FAT)
		f_close(&pfile->fil);

	BYTE rc;
	
	rc = BDOS_CLOSEFILE((WORD)pfile->fcb);
	
	// printf("\nBDOS CloseFile(): %i", rc);
	
	if (rc == 0xFF)
		return FR_INVALID_OBJECT;

	return FR_OK;
}

FRESULT Read(FILE * pfile, void * pbuf, UINT btr, UINT * br)
{
	if (btr != RECLEN)
		return FR_INVALID_PARAMETER;
	
	if (pfile->fstyp == FS_FAT)
		return f_read(&pfile->fil, pbuf, RECLEN, br);

	BYTE rc;

	BDOS_SETDMA((WORD)pbuf);
	
	rc = BDOS_READSEQ((WORD)&pfile->fcb);
	
	//printf("\nBDOS ReadSeq(): %i", rc);

	// Non-zero return indicates EOF	
	if (rc == 0)
		*br = RECLEN;
	else
		*br = 0;

	return FR_OK;
}

FRESULT Write(FILE * pfile, void * pbuf, UINT btw, UINT * bw)
{
	if (btw != RECLEN)
		return FR_INVALID_PARAMETER;
	
	if (pfile->fstyp == FS_FAT)
		return f_write(&pfile->fil, pbuf, RECLEN, bw);

	BYTE rc;

	BDOS_SETDMA((WORD)pbuf);
	
	rc = BDOS_WRITESEQ((WORD)&pfile->fcb);
	
	// printf("\nBDOS WriteSeq(): %i", rc);
	
	if (rc != 0)
		return FR_DISK_ERR; // Actually "out of space"

	*bw = RECLEN;

	return FR_OK;
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
	
	printf("%c%c%c%c  ",
	       (pfno->fattrib & AM_RDO) ? 'R' : '-',
	       (pfno->fattrib & AM_HID) ? 'H' : '-',
	       (pfno->fattrib & AM_SYS) ? 'S' : '-',
	       (pfno->fattrib & AM_ARC) ? 'A' : '-');
	
    printf("%s", pfno->fname);
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
	
	fr = f_stat(szPath, &fno);

	if ((fr == FR_OK) && (fno.fattrib & AM_DIR))
		*szFileSpec = '\0';
	else
		fr = SplitPath(szPath, szFileSpec);

	if (*szFileSpec == '\0')
		strcpy(szFileSpec, "*");
	
	// printf("\nf_findfirst() szPath: '%s', szFileSpec: '%s'", szPath, szFileSpec);

	if (fr == FR_OK)
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

FRESULT CopyFile(char * szSrcFile, char * szDestFile)
{
	FRESULT fr;
	FILE fileSrc, fileDest;
	
	//printf("\n  CopyFile() %s ==> %s", szSrcFile, szDestFile);
	printf("\n%s ==> %s", szSrcFile, szDestFile);
	
	if (strcmp(szSrcFile, szDestFile) == 0)
		return FR_INVALID_PARAMETER;
	
	memset(&fileSrc, 0, sizeof(fileSrc));
	memset(&fileDest, 0, sizeof(fileDest));
	
	fileSrc.fstyp = IsFatPath(szSrcFile) ? FS_FAT : FS_CPM;
	fileDest.fstyp = IsFatPath(szDestFile) ? FS_FAT : FS_CPM;

	//printf("\nSrcFile %s FAT", fileSrc.fstyp == FS_FAT ? "IS" : "NOT");
	//printf("\nDestFile %s FAT", fileDest.fstyp == FS_FAT ? "IS" : "NOT");
	
	if (Exists(szDestFile))
	{
		printf(" Overwrite? (Y/N)");
		
		if (!Confirm())
			return 100;	// special case value to indicate "skip file"
		
		fr = DeleteFile(szDestFile);
		
		if (fr != FR_OK)
			return fr;
	}
	
	fr = Open(&fileSrc, szSrcFile, FA_READ);
	
	if (fr == FR_OK)
	{
		fr = Open(&fileDest, szDestFile, FA_WRITE | FA_CREATE_ALWAYS);
		
		if (fr == FR_OK)
		{
			UINT br, bw;
			BYTE buf[RECLEN];
			
			printf(" ...");
			
			do
			{
				br = 0;
				memset(buf, 0x1A, RECLEN);
			
				fr = Read(&fileSrc, buf, RECLEN, &br);
				
				if (fr != FR_OK)
					break;
				
				if (br > 0)
				{
					bw = 0;

					fr = Write(&fileDest, buf, RECLEN, &bw);

					if (fr != FR_OK)
						break;
					
					if (bw < RECLEN)
					{
						// This is actually an out of space condition!!!
						fr = FR_DISK_ERR;
						break;
					}
				}
			} while (br == RECLEN);

			Close(&fileDest);
		}

		Close(&fileSrc);
	}
	
	return fr;
}

FRESULT FatCopy(char * szSrcPath, char * szDestPath)
{
	FRESULT fr;
	int nFiles;
	DIR dir;
	FILINFO fno;
	char szSrcSpec[MAX_FN];
	char szDestSpec[MAX_FN];

	// printf("\nFatCopy()...");
	
	nFiles = 0;

	fr = SplitPath(szSrcPath, szSrcSpec);
	if (fr != FR_OK)
		return fr;
	
	if (IsFatPath(szDestPath))
	{
		fr = f_stat(szDestPath, &fno);
		//printf("\nf_stat=%i", fr);

		if ((fr == FR_OK) && (fno.fattrib & AM_DIR))
			*szDestSpec = '\0';
		else
			fr = SplitPath(szDestPath, szDestSpec);
		
		//printf("\nSplitPath=%i", fr);
		//printf("\nszDestPath='%s', szDestSpec='%s'", szDestPath, szDestSpec);
	}
	else
		fr = SplitPath(szDestPath, szDestSpec);

	if (fr != FR_OK)
		return fr;
	
	if (*szSrcSpec == '\0')
		return FR_INVALID_PARAMETER;
	
	if (IsWild(szSrcSpec) && (*szDestSpec != '\0'))
		return FR_INVALID_PARAMETER;
	
	//printf("\n  szSrcPath: %s\n  szSrcSpec: %s", szSrcPath, szSrcSpec);
	
	fr = f_findfirst(&dir, &fno, szSrcPath, szSrcSpec);
	
	if (fr == FR_OK)
		printf("\nCopying...\n");
	else
		return FR_NO_FILE;

	while ((fr == FR_OK) && (fno.fname[0]))
	{
		char szSrcFile[MAX_PATH];
		char szDestFile[MAX_PATH];
		
		if (!(fno.fattrib & AM_DIR))
		{
			//printf("\n%s", fno.fname);

			strncpy(szSrcFile, szSrcPath, sizeof(szSrcFile) - 1);
			strncat(szSrcFile, "/", sizeof(szSrcFile) - 1);
			strncat(szSrcFile, fno.fname, sizeof(szSrcFile) - 1);
			
			strncpy(szDestFile, szDestPath, sizeof(szDestFile) - 1);
			if (IsFatPath(szDestPath))
				strncat(szDestFile, "/", sizeof(szDestFile) - 1);
			strncat(szDestFile, (*szDestSpec == '\0') ? fno.fname : szDestSpec, sizeof(szDestFile) - 1);
			
			fr = CopyFile(szSrcFile, szDestFile);
			if (fr == FR_OK)
			{
				printf(" [OK]");
				nFiles++;
			}
			if (fr == 100)
			{
				printf(" [Skipped]");
				fr = FR_OK;
			}
		}
		
		if (fr == FR_OK)
			fr = f_findnext(&dir, &fno);
	}
	
	printf("\n\n    %i File(s) Copied", nFiles);

	return fr;
}

FRESULT CpmCopy(char * szSrcPath, char * szDestPath)
{
	FRESULT fr;
	int rc;
	int nFiles;
	FCB fcbSave, fcbSrch;
	BYTE buf[RECLEN];
	FCB * dirent;
	FILINFO fno;
	char szSrcSpec[MAX_FN];
	char szDestSpec[MAX_FN];
  
  int nEntry, nSkip;

	szDestPath;

	// printf("\nCpmCopy()...");
	
	nFiles = 0;
	
	fr = MakeFCB(szSrcPath, &fcbSrch);
  
  // DumpFCB(&fcbSrch);
  
  memcpy(&fcbSave, &fcbSrch, sizeof(fcbSave));
	
	if (fr != FR_OK)
		return fr;
	
	fr = SplitPath(szSrcPath, szSrcSpec);
	if (fr != FR_OK)
		return fr;
	
	if (IsFatPath(szDestPath))
	{
		fr = f_stat(szDestPath, &fno);
		//printf("\nf_stat=%i", fr);

		if ((fr == FR_OK) && (fno.fattrib & AM_DIR))
			*szDestSpec = '\0';
		else
			fr = SplitPath(szDestPath, szDestSpec);
		
		//printf("\nSplitPath=%i", fr);
		//printf("\nszDestPath='%s', szDestSpec='%s'", szDestPath, szDestSpec);
	}
	else
		fr = SplitPath(szDestPath, szDestSpec);

	if (fr != FR_OK)
		return fr;
	
	if (*szSrcSpec == '\0')
		return FR_INVALID_PARAMETER;
	
	if (IsWild(szSrcSpec) && (*szDestSpec != '\0'))
		return FR_INVALID_PARAMETER;
  
  nSkip = 0;
  rc = 0;
  
  while (rc != 0xFF)
  {
    char FileList[CHUNKSIZE][12];
    
    memcpy(&fcbSrch, &fcbSave, sizeof(fcbSrch));
    
    // DumpFCB(&fcbSrch);
    
    BDOS_SETDMA((WORD)&buf);
    
    rc = BDOS_FINDFIRST((WORD)&fcbSrch);
    
    // printf("\nBDOS FindFirst(): %i", rc);
    
    if (nSkip == 0)
    {
      if (rc == 0xFF)
        return FR_NO_FILE;
      printf("\nCopying...\n");
    }
    
    nEntry = 0;

    while ((rc != 0xFF) && (nEntry < nSkip))
    {
      rc = BDOS_FINDNEXT((WORD)&fcbSrch);
      nEntry++;
    }
    
    nEntry = 0;

    while ((rc != 0xFF) && (nEntry < CHUNKSIZE))
    {
      dirent = (FCB *)(buf + (32 * rc));
      
      // DumpFCB(dirent);
      
      memcpy(FileList[nEntry], dirent, sizeof(FileList[0]));
      
      nEntry++;
      nSkip++;

      rc = BDOS_FINDNEXT((WORD)&fcbSrch);
    }

    for (int iFile = 0; iFile < nEntry; iFile++)
    {
      char szSrcFile[MAX_PATH];
      char szDestFile[MAX_PATH];
      char *p, *p2;
      int n;

      dirent = (FCB *)FileList[iFile];
      p = szSrcFile;
      
      // DumpFCB(dirent);

      if (fcbSrch.drv > 0)
      {
        *(p++) = fcbSrch.drv + 'A' - 1;
        *(p++) = ':';
      }
      
      p2 = p;			// Remember start of filename/ext
      
      for (n = 0; n < 8; n++)
      {
        if (dirent->name[n] == ' ')
          break;
        *(p++) = dirent->name[n] & 0x7F;
      }
      
      *(p++) = '.';
      
      for (n = 0; n < 3; n++)
      {
        if (dirent->ext[n] == ' ')
          break;
        *(p++) = dirent->ext[n] & 0x7F;
      }

      *(p++) = '\0';
      
      strncpy(szDestFile, szDestPath, sizeof(szDestFile) - 1);
      if (IsFatPath(szDestPath))
        strncat(szDestFile, "/", sizeof(szDestFile) - 1);
      strncat(szDestFile, (*szDestSpec == '\0') ? p2 : szDestSpec, sizeof(szDestFile) - 1);
      
      // printf("\nCopy File: %s", szSrcFile);
      
      fr = CopyFile(szSrcFile, szDestFile);
      if (fr == FR_OK)
      {
        printf(" [OK]");
        nFiles++;
      }
      if (fr == 100)
      {
        printf(" [Skipped]");
        fr = FR_OK;
      }
      if (fr != FR_OK)
        return fr;
    }
  }

	printf("\n\n    %i File(s) Copied", nFiles);

  return fr;

/**************************************************
	
	while (rc != 0xFF)
	{

	
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
	
	printf("\n\n    %i File(s) Copied", nFiles);

	return fr;

***********************************************************/
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

	fr = FR_OK;

	if (IsFatPath(szSrcPath))
		fr = f_mount(&fsSrc, szSrcPath, 1);
	
	if ((fr == FR_OK) && (IsFatPath(szDestPath)))
	{
		fr = f_mount(&fsDest, szDestPath, 1);
		if (fr != FR_OK)
			return fr;
	}

	if (fr == FR_OK)
	{
		if (IsFatPath(szSrcPath))
			fr = FatCopy(szSrcPath, szDestPath);
		else
			fr = CpmCopy(szSrcPath, szDestPath);
	}
	
	f_mount(0, szSrcPath, 0);		// unmount ignoring any errors
	f_mount(0, szDestPath, 0);		// unmount ignoring any errors

	return fr;
}

FRESULT Rename(void)
{
	FRESULT fr;
	int nFiles;
	FATFS fsSrc;
	DIR dir;
	FILINFO fno;
	char * szSrcPath;
	char * szDestPath;
	char szSrcSpec[MAX_FN];
	char szDestSpec[MAX_FN];
	
	nFiles = 0;
	
	szSrcPath = strtok(NULL, " ");
	if (szSrcPath == NULL)
		return FR_INVALID_PARAMETER;

	szDestPath = strtok(NULL, " ");
	if (szDestPath == NULL)
		return FR_INVALID_PARAMETER;
	
	fr = SplitPath(szSrcPath, szSrcSpec);
	if (fr != FR_OK)
		return fr;
	
	if (!IsFatPath(szSrcPath))
		return FR_INVALID_PARAMETER;

	fr = SplitPath(szDestPath, szDestSpec);
	if (fr != FR_OK)
		return fr;
	
	//if (*szDestPath && !IsFatPath(szDestPath))
	//	return FR_INVALID_PARAMETER;
	
	// TODO: if dest drive specified, dest drive must == src drive

	if (*szSrcSpec == '\0')
		return FR_INVALID_PARAMETER;
	
	//if (*szDestSpec == '\0')
	//	return FR_INVALID_PARAMETER;
	
	if (IsWild(szSrcSpec) && (*szDestSpec != '\0'))
		return FR_INVALID_PARAMETER;
	
	fr = f_mount(&fsSrc, szSrcPath, 1);
	if (fr != FR_OK)
		return fr;
	
	//printf("\nSrcPath='%s', SrcSpec='%s'", szSrcPath, szSrcSpec);
	
	fr = f_findfirst(&dir, &fno, szSrcPath, szSrcSpec);
	
	if (fr == FR_OK)
		printf("\nRenaming...\n");

	while ((fr == FR_OK) && (fno.fname[0]))
	{
		char szSrcFile[MAX_PATH];
		char szDestFile[MAX_PATH];
		
		strncpy(szSrcFile, szSrcPath, sizeof(szSrcFile) - 1);
		strncat(szSrcFile, "/", sizeof(szSrcFile) - 1);
		strncat(szSrcFile, fno.fname, sizeof(szSrcFile) - 1);
		
		//printf("\nszDestPath='%s', szDestSpec='%s'", szDestPath, szDestSpec);

		strncpy(szDestFile, *szDestPath ? szDestPath : szSrcPath, sizeof(szDestFile) - 1);
		strncat(szDestFile, "/", sizeof(szDestFile) - 1);
		strncat(szDestFile, (*szDestSpec == '\0') ? fno.fname : szDestSpec, sizeof(szDestFile) - 1);
		
		printf("\n%s ==> %s", szSrcFile, szDestFile);

		fr = f_rename(szSrcFile, szDestFile);
		if (fr == FR_OK)
			nFiles++;
		
		if (fr == FR_OK)
			fr = f_findnext(&dir, &fno);
	}
	
	f_mount(0, szSrcPath, 0);		// unmount ignoring any errors
	
	if (fr != FR_OK)
		return fr;

	if (nFiles)
		printf("\n\n    %i File(s) Renamed", nFiles);
	else
		fr = FR_NO_FILE;

	return fr;
}

FRESULT Delete(void)
{
	FATFS fs;
	FRESULT fr;
	DIR dir;
	FILINFO fno;
	char * szPath;
	char szFileSpec[MAX_FN];
	int nFiles;
	
	nFiles = 0;
	
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
		return FR_INVALID_PARAMETER;
	
	// printf("\nf_findfirst() szPath: '%s', szFileSpec: '%s'", szPath, szFileSpec);

	fr = f_findfirst(&dir, &fno, szPath, szFileSpec);

	if (fr == FR_OK)
		printf("\nDeleting...\n");

	while ((fr == FR_OK) && (fno.fname[0]))
	{
		char szDelFile[MAX_PATH];
		
		strncpy(szDelFile, szPath, sizeof(szDelFile) - 1);
		strncat(szDelFile, "/", sizeof(szDelFile) - 1);
		strncat(szDelFile, fno.fname, sizeof(szDelFile) - 1);
		
		printf("\n%s", szDelFile);
		
		fr = f_unlink(szDelFile);
		if (fr != FR_OK)
			return fr;
		
		nFiles++;
		
		fr = f_findnext(&dir, &fno);
	}

	f_mount(0, szPath, 0);		// unmount ignoring any errors
	
	if (fr != FR_OK)
		return fr;

	if (nFiles)
		printf("\n\n    %i File(s) Deleted", nFiles);
	else
		fr = FR_NO_FILE;

	return fr;
}

FRESULT MakeDir(void)
{
	FATFS fs;
	FRESULT fr;
	char * szPath;
	
	szPath = strtok(NULL, " ");
	if (szPath == NULL)
		return FR_INVALID_PARAMETER;

	fr = f_mount(&fs, szPath, 0);
	if (fr != FR_OK)
		return fr;
	
	fr = f_mkdir(szPath);
	
	f_mount(0, szPath, 0);		// unmount ignoring any errors
	
	return fr;
}

FRESULT Format(void)
{
	FRESULT fr;
	BYTE opt;
	int drv;
	char * szPath;
	REGS reg;
	BYTE buf[FF_MAX_SS];
	
	szPath = strtok(NULL, " ");
	if (szPath == NULL)
		return FR_INVALID_PARAMETER;
	
	drv = FatDrive(szPath);
	if (drv == -1)
		return FR_INVALID_DRIVE;
	
	reg.b.B = 0x17;		// HBIOS Device Function
	reg.b.C = drv;		// HBIOS Disk Unit Number
	reg.w.DE = 0;
	reg.w.HL = 0;
	bioscall(&reg, &reg);
	
	if (reg.b.A != 0)
		return FR_INVALID_DRIVE;
	
	if ((reg.b.C & 0x80) || 		// Floppy
	    ((reg.b.C >> 3) == 4) ||	// ROM
		((reg.b.C >> 3) == 5) || 	// RAM
		((reg.b.C >> 3) == 6))		// RAMF
		opt = FM_ANY | FM_SFD;
	else
		opt = FM_ANY;				// Hard Disk

	if (opt & FM_SFD)
		printf("\nAbout to format Disk Unit #%i."
			   "\nAll existing data will be destroyed!!!",
			   FatDrive(szPath));
	else
		printf("\nAbout to format FAT Filesystem on Disk Unit #%i."
			   "\nAll existing FAT partition data will be destroyed!!!",
			   FatDrive(szPath));
	
	printf("\n\nContinue (y/n)?");
	
	if (!Confirm()) {
		printf("\n\nFormat operation aborted.");
		return 0;
	}
	
	printf("\n\nFormatting...");
	
	fr = f_mkfs(szPath, opt, 0, buf, sizeof(buf));
	
	printf("%s", fr == FR_OK ? " Done" : " Failed!");
	
	return fr;
}

int main(int argc, char * argv[])
{
	char * tok;
	FRESULT fr;
	
	if (chkbios() != BIOS_WBW)
	{
		printf("\nUnsupported BIOS!");
		return Error(FR_INT_ERR);
	}

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
	else if (!strcmp(tok, "MD"))
		fr = MakeDir();
	else if (!strcmp(tok, "FORMAT"))
		fr = Format();
	else
		fr = FR_INVALID_PARAMETER;
	
	if (fr != FR_OK)
		return Error(fr);
	
	printf("\n");
	
	return 0;
}
