#ifndef osH
#define osH
// ----------------------------------------------------------------------------
//  Copyright (C) 2017  Wilfred van Velzen
//
//
//  This file is part of FMail.
//
//  FMail is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  FMail is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------
#if defined(__linux__)

#define O_BINARY   0
#define O_TEXT     0
#define	SH_DENYRW  0x10	/* Deny read/write */
#define	SH_DENYWR  0x20	/* Deny write */
#define	SH_DENYRD  0x30	/* Deny read */
#define MKDIR(d)   mkdir(fixPath(d), S_IRWXU | S_IRWXG)
#define dDIRSEPS   "/"
#define dDIRSEPSa  "\\"
#define dDIRSEPC   '/'
#define dDIRSEPCa  '\\'

//  S_IRWXU  00700 user (file owner) has read, write, and execute permission
//  S_IRUSR  00400 user has read permission
//  S_IWUSR  00200 user has write permission
//  S_IXUSR  00100 user has execute permission
//  S_IRWXG  00070 group has read, write, and execute permission
//  S_IRGRP  00040 group has read permission
//  S_IWGRP  00020 group has write permission
//  S_IXGRP  00010 group has execute permission
//  S_IRWXO  00007 others have read, write, and execute permission
//  S_IROTH  00004 others have read permission
//  S_IWOTH  00002 others have write permission
//  S_IXOTH  00001 others have execute permission
//
//  According to POSIX, the effect when other bits are set in mode is unspecified.  On Linux, the following bits are also honored in mode:
//
//  S_ISUID  0004000 set-user-ID bit
//  S_ISGID  0002000 set-group-ID bit (see stat(2))
//  S_ISVTX  0001000 sticky bit (see stat(2))

#define dDEFOMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

extern char replaceDrive[];

int eof(int fd);
int _sopen(const char *pathname, int flags, int shflags, ... /* mode_t mode */);
const char *fixPath(const char *path);

//----------------------------------------------------------------------------
#elif defined(__WIN32__)

#define fixPath(x)  (x)
#define MKDIR(d)    mkdir(d)  // windows: no fixPath
#define dDIRSEPS    "\\"
#define dDIRSEPSa   "/"
#define dDIRSEPC    '\\'
#define dDIRSEPCa   '/'
#define dDEFOMODE   (S_IREAD | S_IWRITE)

//----------------------------------------------------------------------------
#else
  #error "Defines for your platform are missing!"
#endif
//----------------------------------------------------------------------------
#define isDirSep(c)    ((c) == dDIRSEPC || (c) == dDIRSEPCa)
#define lastSep(p, pa) (((p) = strrchr((pa), dDIRSEPC)) != NULL || ((p) = strrchr((pa), dDIRSEPCa)) != NULL)
#define contSep(pa)    (strpbrk((pa), dDIRSEPS dDIRSEPSa) != NULL)
//----------------------------------------------------------------------------
#endif // osH
