/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "ff.h"

#include "bios.h"

#include "diskio.h"		/* FatFs lower layer API */

typedef struct {
	unsigned char year;
	unsigned char month;
	unsigned char date;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
} TIME;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
	pdrv;

	//printf("\ndisk_status()");
	
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	REGS reg;
	
	//printf("\ndisk_initialize()");
	
	reg.b.B = 0xF8;		// HBIOS Sys Get
	reg.b.C = 0x10;		// Disk unit count
	reg.w.DE = 0;
	reg.w.HL = 0;
	bioscall(&reg, &reg);
	
	if (reg.b.A != 0)
		return STA_NOINIT | STA_NODISK;
	
	if (!(pdrv < reg.b.E))
		return STA_NOINIT | STA_NODISK;
	
	reg.b.B = 0x18;		// HBIOS Media Discovery
	reg.b.C = pdrv;
	reg.w.DE = 0x0001;	// Set bit E:0 for media discovery
	reg.w.HL = 0;
	bioscall(&reg, &reg);
	
	// printf("\nHBIOS Media = %u, Type=%u", reg.b.A, reg.b.E);

	return reg.b.A ? STA_NOINIT : 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	REGS reg;
	
	//printf("\ndisk_read(%u, %lu, %u)", pdrv, sector, count);
	
	reg.b.B = 0x12;		// HBIOS Seek
	reg.b.C = pdrv;
	reg.w.DE = (WORD)(sector>>16);
	reg.w.HL = (WORD)sector;
	reg.b.D |= 0x80;		// High bit signifies LBA address
	bioscall(&reg, &reg);
	
	//printf("\nHBIOS Seek = %u", reg.b.A);

	if (reg.b.A == 0)
	{
		reg.b.B = 0x13;		// HBIOS Read
		reg.b.C = pdrv;
		reg.w.DE = count;
		reg.w.HL = (WORD)buff;
		bioscall(&reg, &reg);
		
		//printf("\nHBIOS Read = %u", reg.b.A);
	}

	return reg.b.A ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	REGS reg;
	
	//printf("\ndisk_write(%uc, %ul, %u)", pdrv, sector, count);

	reg.b.B = 0x12;		// HBIOS Seek
	reg.b.C = pdrv;
	reg.w.DE = (WORD)(sector>>16);
	reg.w.HL = (WORD)sector;
	reg.b.D |= 0x80;		// High bit signifies LBA address
	bioscall(&reg, &reg);
	
	//printf("\nHBIOS Seek = %u", reg.b.A);

	if (reg.b.A == 0)
	{
		reg.b.B = 0x14;		// HBIOS Read
		reg.b.C = pdrv;
		reg.w.DE = count;
		reg.w.HL = (WORD)buff;
		bioscall(&reg, &reg);
		
		//printf("\nHBIOS Write = %u", reg.b.A);
	}

	return reg.b.A ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	pdrv;
	cmd;
	buff;
	REGS reg;
	
	//printf("\ndisk_ioctl(%uc, %uc)", pdrv, cmd);

	switch (cmd)
	{
		case CTRL_SYNC:
			return RES_OK;
		
		case GET_SECTOR_COUNT:
			reg.b.B = 0x1A;		// Disk Capacity
			reg.b.C = pdrv;		// Disk Unit
			reg.w.DE = 0;
			reg.w.HL = 0;
			bioscall(&reg, &reg);
			
			if (reg.b.A != 0)
				return RES_PARERR;
			
			*((DWORD *)buff) = ((DWORD)reg.w.DE << 16) + reg.w.HL;
			return RES_OK;
		
		case GET_SECTOR_SIZE:
			*((WORD *)buff) = 512;
			return RES_OK;
		
		case GET_BLOCK_SIZE:
			*((DWORD *)buff) = 1;
			return RES_OK;
		
		case CTRL_TRIM:
			return RES_OK;
	}

	return RES_PARERR;
}

/*-------------------------------------------------------------------*/
/* User Provided RTC Function for FatFs module                       */
/*-------------------------------------------------------------------*/
/* This is a real time clock service to be called from FatFs module. */
/* This function is needed when FF_FS_READONLY == 0 and FF_FS_NORTC == 0 */

BYTE bcd2bin(BYTE val)
{
	return (((val >> 4) * 10) + (val & 0x0F));
}

DWORD get_fattime (void)
{
	REGS reg;
	TIME time;
	DWORD fattime;
	
	reg.b.B = 0x20;		// HBIOS RTC Get Time
	reg.w.HL = (WORD)&time;
	bioscall(&reg, &reg);
	
	/* Pack date and time into a DWORD variable */
	if (reg.b.A == 0)
	{
		fattime = 0;
		fattime |= (((DWORD)bcd2bin(time.year) + 20) << 25);
		fattime |= ((DWORD)bcd2bin(time.month) << 21);
		fattime |= ((DWORD)bcd2bin(time.date) << 16);
		fattime |= ((WORD)bcd2bin(time.hour) << 11);
		fattime |= ((WORD)bcd2bin(time.minute) << 5);
		fattime |= ((WORD)bcd2bin(time.second) >> 1);
	}
	else
	{
		fattime = 0;
		fattime |= ((DWORD)(2017 - 1980) << 25);
		fattime |= ((DWORD)9 << 21);
		fattime |= ((DWORD)21 << 16);
		fattime |= (WORD)(18 << 11);
		fattime |= (WORD)(44 << 5);
		fattime |= (WORD)(15 >> 1);
	}
	
	return fattime;
}
