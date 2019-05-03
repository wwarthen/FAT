RomWBW HBIOS CP/M FAT Utility ("FAT.COM")

Author: Wayne Warthen
Updated: 2-May-2019

Application to manipulate and exchange files with a FAT (DOS)
filesystem.  Runs on any HBIOS hosted CP/M implementation.

USAGE:
  FAT DIR <path>
  FAT COPY <src> <dst>
  FAT REN <fn1> <fn2>
  FAT DEL <fn>

  CP/M filespec: [<d>:]FILENAME.EXT (where <d> is A-P)
  FAT filespec:  <u>:/DIR/FILENAME.EXT (where <u> is disk unit #)
	
LICENSE:
  GNU GPLv3 (see file LICENSE.txt)

NOTES:
 - During a file copy, if the destination file exists, it is not
   handled well.  If destination if FAT filesystem, the file is
   silently overwritten.  If CP/M, the copy aborts with an error.

 - FAT directory references MUST be terminated with a trailing '/' or
   '\'.

 - Wildcard matching in FAT filesystems is a bit unusual as
   implemented by FatFS.  See FatFS documentation.

 - The application infers whether you are attempting to reference
   a FAT or CP/M filesystem via the drive specifier (char before ':').
   A numeric drive character specifies the HBIOS disk unit number
   for FAT access.  An alpha (A-P) character indicates a CP/M
   file system access targeting the specified drive letter.  If there
   is no drive character specified, the current CP/M filesystem and
   current CP/M drive is assumed.  For example:
   
   2:README.TXT refers to FAT file README.TXT on disk unit #2
   C:README.TXT refers to CP/M file README.TXT on CP/M drive C
   README.TXT refers to CP/M file README.TXT on current CP/M drive
   
 - References to a FAT filesystem MUST start with a number followed
   by a colon.  Any path that does not start with a drive followed
   by a colon are assumed to reference the current CP/M drive.

 - Application is generally oblivious to system files.
 
 - It is not currently possible to reference CP/M user areas other
   than the current user.  To copy files to alternate user areas,
   you must switch to the desired user number first or use an
   additional step to copy the file to the desired user area.
 
BUILD NOTES:
 - Application is based on FatFS.  FatFS source is included.

 - SDCC compiler is required to build (v3.9.0 known working).

 - ZX CP/M emulator is required to build (from RomWBW distribution).

 - See Build.cmd for sample build script under Windows.  References
   to SDCC and ZX must be updated for your environment.
   
 - Note that ff.c (core FatFS code) gneerates quite a few compiler
   warnings which all appear benign.

TO DO:
 - Confirm HBIOS is present at startup.

 - Implement DEL and REN functions.
 
 - Handle existing file collisions with user option prompt.
 
HISTORY:
 2-May-2019: v0.9 initial release (beta)
