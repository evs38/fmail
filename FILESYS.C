//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2015 Wilfred van Velzen
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
//---------------------------------------------------------------------------


#include <fcntl.h>
#include <io.h>
#include <dir.h>
#include <dos.h>
#include <fcntl.h>
#include <string.h>
#ifndef __OS2__
#include <windows.h>
#endif

#include "fmail.h"
#include "config.h"
#include "filesys.h"

int fs_maxfname;
int fs_maxdir;
int fs_flags;


#define seg(x) (int)(((long)x)>>16)
#define off(x) (int)x


#ifndef __FMAILX__
#define fsint86x(a,b,c,d) int86x(a,b,c,d)
#else
#define fsint86x(a,b,c,d) int86xdpmi(a,b,c,d)
#ifndef __32BIT__
static void *intbuf1p, *intbuf1r;
static void *intbuf2p, *intbuf2r;

typedef struct _REGDATA
{
    u32         di;
    u32         si;
    u32         bp;
    u32         reserved;
    u32         bx;
    u32         dx;
    u32         cx;
    u32         ax;
    u16         flags;
    u16         es;
    u16         ds;
    u16         fs;
    u16         gs;
    u16         ip;
    u16         cs;
    u16         sp;
    u16         ss;
} REGDATA;

static REGDATA regdata;

int int86xdpmi(int intno, union REGS *inregs, union REGS *outregs, struct SREGS *segregs)
{  regdata.di    = inregs->x.di;
   regdata.si    = inregs->x.si;
   regdata.bx    = inregs->x.bx;
   regdata.dx    = inregs->x.dx;
   regdata.cx    = inregs->x.cx;
   regdata.ax    = inregs->x.ax;
   regdata.flags = inregs->x.flags;
   regdata.es    = segregs->es;
   regdata.ds    = segregs->ds;
   _ES     = seg(&regdata);
   _DI     = off(&regdata);
   _AX     = 0x0300;
   _BL     = intno;
   _BH     = 0;
   _CX     = 0;
   geninterrupt(0x31);
   outregs->x.di     = (u16)regdata.di;
   outregs->x.si     = (u16)regdata.si;
   outregs->x.bx     = (u16)regdata.bx;
   outregs->x.dx     = (u16)regdata.dx;
   outregs->x.cx     = (u16)regdata.cx;
   outregs->x.ax     = (u16)regdata.ax;
   outregs->x.flags  = (u16)regdata.flags;
   outregs->x.cflag  = (u16)regdata.flags & 1;
   segregs->es       = (u16)regdata.es;
   segregs->ds       = (u16)regdata.ds;
   return (int)regdata.ax;
}
#endif
#endif



#ifndef __FMAILX__
#define segx1(ptr) seg(ptr)
#else
#define segx1(ptr)        \
(  memcpy(intbuf1p, ptr, 320), \
   seg(intbuf1r)         \
)
#endif                    \

#ifndef __FMAILX__
#define offx1(ptr) off(ptr)
#else
#define offx1(ptr)        \
(  memcpy(intbuf1p, ptr, 320), \
   off(intbuf1r)         \
)
#endif                    \

#ifndef __FMAILX__
#define segx2(ptr) seg(ptr)
#else
#define segx2(ptr)        \
(  memcpy(intbuf2p, ptr, 320), \
   seg(intbuf2r)         \
)
#endif                    \

#ifndef __FMAILX__
#define offx2(ptr) off(ptr);
#else
#define offx2(ptr)        \
(  memcpy(intbuf2p, ptr, 320), \
   off(intbuf2r)         \
)
#endif                    \

//---------------------------------------------------------------------------
// Check if the volume in path supports long file names
int fileSys(const char *path)
{
#ifndef __32BIT__
   char buf[256];
   union REGS regs, regs2;
   struct SREGS segregs;
   char pathbuf[128];
   char *helpPtr;
#endif
   fs_maxfname = 12;
   fs_maxdir = 255;
   fs_flags = 0;

#ifdef __OS2__
   path = path;
   return 1;
#else
#ifdef __32BIT__
   if (!config.genOptions.lfn)
      return 0;
   GetVolumeInformation(path, NULL, 0, NULL, NULL, (LPDWORD)&fs_flags, NULL, 0);
   return (fs_flags & 0x4000) != 0;
#else
   if ( !config.genOptions.lfn )
      return 0;
#ifdef  __FMAILX__
   if ( !intbuf1p )
   {
      u32 tmp;
      if ( (tmp = GlobalDosAlloc(320)) == 0 )
        return 0;
      intbuf1p = (void *)(tmp << 16);
      intbuf1r = (void *)(tmp & 0xffff0000L);
   }
   if ( !intbuf2p )
   {
      u32 tmp;
      if ( (tmp = GlobalDosAlloc(320)) == 0 )
        return 0;
      intbuf2p = (void *)(tmp << 16);
      intbuf2r = (void *)(tmp & 0xffff0000L);
   }
#endif
   strcpy(pathbuf, path);
   if ( (helpPtr = strchr(pathbuf, '\\')) != NULL )
      *(helpPtr+1) = 0;
   regs.x.ax = 0x71A0;
   segregs.es = segx1(buf);
   regs.x.di = offx1(buf);
   regs.x.cx = 255;
   segregs.ds = segx2(pathbuf);
   regs.x.dx = offx2(pathbuf);
   fsint86x(0x21, &regs, &regs2, &segregs);
   if ( regs2.x.cflag )
   {  errno = _doserrno = regs2.x.ax;
      return 0;
   }
   errno = _doserrno = 0;
   fs_maxfname = regs2.x.cx;
   fs_maxdir = regs2.x.dx;
   fs_flags = regs2.x.bx;
   return (fs_flags & 0x4000) != 0;
#endif
#endif
}
//---------------------------------------------------------------------------
int fsopen(const char *path, int access, unsigned mode, u16 lfn)
{
#ifdef __32BIT__
#ifdef __OS2__
   lfn = lfn;
   return open(path, access, mode);
#else
   int      ret;
   HANDLE   handle;

   if (!lfn || !fileSys(path))
      return open(path, access, mode);

// return OpenFile(path, &ofstruct,
//		   ((access & O_TRUNC) ? OF_CREATE : 0) |
//		   ((access & O_RDONLY) ? OF_READ : OF_READWRITE) |
//		   (((access & O_DENYNONE) ? OF_SHARE_DENY_NONE :
//		    ((access & O_DENYREAD) ? OF_SHARE_DENY_READ :
//		    ((access & O_DENYWRITE) ? OF_SHARE_DENY_WRITE :
//		    ((access & O_DENYALL) ? OF_SHARE_EXCLUSIVE : 0))))));

   handle = CreateFile(path,
		       GENERIC_READ|GENERIC_WRITE,
		       !(access & O_DENYREAD) ? FILE_SHARE_READ :
		       !(access & O_DENYWRITE) ? FILE_SHARE_WRITE : 0,
		       NULL,
		       (access & O_CREAT) ? ((access & O_TRUNC) ? CREATE_ALWAYS : OPEN_ALWAYS) :
		       (access & O_TRUNC) ? TRUNCATE_EXISTING : OPEN_EXISTING,
		       FILE_ATTRIBUTE_NORMAL,
		       NULL);
   if (handle == INVALID_HANDLE_VALUE)
      return -1;
   if ((ret = _open_osfhandle((long)handle, (access & (O_APPEND | O_RDONLY | O_TEXT)))) == -1)
   {
    CloseHandle(handle);
    return -1;
   }
   return ret;
#endif
#else
   union  REGS  regs, regs2;
   struct SREGS segregs;
   int	  ret;

   if ( !lfn || !fileSys(path) )
      return open(path, access, mode);
   regs.x.ax = 0x716C;
   regs.x.bx = 2;
   regs.x.cx = 0;
   regs.x.dx = 0x11;      // CREAT|OPEN
   regs.x.si = offx1(path);
   segregs.ds = segx1(path);
   ret = fsint86x(0x21, &regs, &regs2, &segregs);
   if ( regs2.x.cflag )
   {  errno = _doserrno = regs2.x.ax;
      return -1;
   }
   errno = _doserrno = 0;
   return ret;
#endif
}
//---------------------------------------------------------------------------
int fsclose(int handle)
{
  return close(handle);
}
//---------------------------------------------------------------------------
#ifndef FMAIL
int fsmkdir(const char *path, u16 lfn)
{
#ifdef __32BIT__
#ifdef __OS2__
   lfn = lfn;
   return mkdir(path);
#else
  SECURITY_ATTRIBUTES sa;
   if ( !lfn || !fileSys(path) )
      return mkdir(path);
  memset(&sa, 0, sizeof(sa));
  return CreateDirectory(path, &sa);
#endif
#else
   union  REGS  regs, regs2;
   struct SREGS segregs;

   if ( path[1] == ':' && (strlen(path) < 3 || (strlen(path) == 3 && path[2] == '\\')) )
      return 0;
   if ( !lfn || !fileSys(path) )
      return mkdir(path);
   regs.x.ax = 0x7139;
   regs.x.dx = offx1(path);
   segregs.ds = segx1(path);
   fsint86x(0x21, &regs, &regs2, &segregs);
   if ( regs2.x.cflag )
   {  errno = _doserrno = regs2.x.ax;
      return -1;
   }
   errno = _doserrno = 0;
   return 0;
#endif
}
//---------------------------------------------------------------------------
#ifndef __32BIT__
typedef struct
{
    u32      dwLowDateTime;
    u32      dwHighDateTime;
} FILETIME;

typedef struct _WIN32_FIND_DATA
{
    u32      dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    u32      nFileSizeHigh;
    u32      nFileSizeLow;
    u32      dwReserved0;
    u32      dwReserved1;
    u8       cFileName[260];
    u8       cAlternateFileName[14];
} WIN32_FIND_DATA;
#endif
//---------------------------------------------------------------------------
int fsfindfirst(const char *pathname, struct ffblk *ffblk, int attrib, u16 lfn)
{
#ifdef __32BIT__
#ifdef __OS2__
   lfn = lfn;
   return findfirst(pathname, ffblk, attrib);
#else
   WIN32_FIND_DATA wfd;
   HANDLE          handle;

   if ( !lfn || !fileSys(pathname) )
      return findfirst(pathname, ffblk, attrib);
   if ( (handle = FindFirstFile(pathname, &wfd)) == INVALID_HANDLE_VALUE )
      return -1;
   FindClose(handle);
   ffblk->ff_attrib = (u16)wfd.dwFileAttributes;
   ffblk->ff_fdate = (u16)(wfd.ftLastWriteTime.dwLowDateTime>>16) & 0x0000FFFF;
   ffblk->ff_ftime = (u16)wfd.ftLastWriteTime.dwLowDateTime & 0x0000FFFF;
   ffblk->ff_fsize = wfd.nFileSizeLow;
   strncpy(ffblk->ff_name, wfd.cAlternateFileName, 13);
   return 0;
#endif
#else
   union  REGS  regs, regs2;
   struct SREGS segregs;
   WIN32_FIND_DATA wfd;

   if ( !lfn || !fileSys(pathname) )
      return findfirst(pathname, ffblk, attrib);
   regs.x.ax = 0x714E;
   regs.h.ch = 0;
   regs.h.cl = attrib;
   segregs.ds = segx1(pathname);
   regs.x.dx = offx1(pathname);
   segregs.es = segx2(&wfd);
   regs.x.di = offx2(&wfd);
   regs.x.si = 1;
   fsint86x(0x21, &regs, &regs2, &segregs);
   if ( regs2.x.cflag )
   {  errno = _doserrno = regs2.x.ax;
      return -1;
   }
   errno = _doserrno = 0;
#ifndef __FMAILX__
   ffblk->ff_attrib = (u16)wfd.dwFileAttributes;
   ffblk->ff_fdate = (u16)(wfd.ftLastWriteTime.dwLowDateTime>>16) & 0x0000FFFF;
   ffblk->ff_ftime = (u16)wfd.ftLastWriteTime.dwLowDateTime & 0x0000FFFF;
   ffblk->ff_fsize = wfd.nFileSizeLow;
   strncpy(ffblk->ff_name, wfd.cAlternateFileName, 13);
#else
   ffblk->ff_attrib = (u16)((WIN32_FIND_DATA *)intbuf2p)->dwFileAttributes;
   ffblk->ff_fdate = (u16)(((WIN32_FIND_DATA *)intbuf2p)->ftLastWriteTime.dwLowDateTime>>16) & 0x0000FFFF;
   ffblk->ff_ftime = (u16)((WIN32_FIND_DATA *)intbuf2p)->ftLastWriteTime.dwLowDateTime & 0x0000FFFF;
   ffblk->ff_fsize = ((WIN32_FIND_DATA *)intbuf2p)->nFileSizeLow;
   strncpy(ffblk->ff_name, ((WIN32_FIND_DATA *)intbuf2p)->cAlternateFileName, 13);
#endif
// FindClose
   regs.x.ax = 0x71A1;
   regs.x.bx = regs2.x.ax;
   fsint86x(0x21, &regs, &regs2, &segregs);
   return 0;
#endif
}
//---------------------------------------------------------------------------
int fsunlink(const char *path, u16 lfn)
{
#ifdef __32BIT__
#ifdef __OS2__
   lfn = lfn;
   return unlink(path);
#else
   if ( !lfn || !fileSys(path) )
      return unlink(path);
   return DeleteFile(path);
#endif
#else
   union  REGS  regs, regs2;
   struct SREGS segregs;

   if ( path[1] == ':' && (strlen(path) < 3 || (strlen(path) == 3 && path[2] == '\\')) )
      return 0;
   if ( !lfn || !fileSys(path) )
      return unlink(path);
   regs.x.ax = 0x7141;
   regs.x.dx = offx1(path);
   segregs.ds = segx1(path);
   regs.x.si = 0;
   regs.x.cx = 0;
   fsint86x(0x21, &regs, &regs2, &segregs);
   if ( regs2.x.cflag )
   {  errno = _doserrno = regs2.x.ax;
      return -1;
   }
   errno = _doserrno = 0;
   return 0;
#endif
}
//---------------------------------------------------------------------------
int fsrename(const char *oldname, const char *newname, u16 lfn)
{
#ifdef __32BIT__
#ifdef __OS2__
   lfn = lfn;
   return rename(oldname, newname);
#else
   if ( !lfn || !fileSys(oldname) || !fileSys(newname) )
      return rename(oldname, newname);
   return MoveFile(oldname, newname);
#endif
#else
   union  REGS  regs, regs2;
   struct SREGS segregs;

   if ( oldname[1] == ':' && (strlen(oldname) < 3 || (strlen(oldname) == 3 && oldname[2] == '\\')) )
      return 0;
   if ( newname[1] == ':' && (strlen(newname) < 3 || (strlen(newname) == 3 && newname[2] == '\\')) )
      return 0;
   if ( !lfn || !fileSys(oldname) || !fileSys(newname) )
      return rename(oldname, newname);
   regs.x.ax = 0x7156;
   regs.x.dx = offx1(oldname);
   segregs.ds = segx1(oldname);
   regs.x.di = offx1(newname);
   segregs.es = segx1(newname);
   fsint86x(0x21, &regs, &regs2, &segregs);
   if ( regs2.x.cflag )
   {  errno = _doserrno = regs2.x.ax;
      return -1;
   }
   errno = _doserrno = 0;
   return 0;
#endif
}
#endif
//---------------------------------------------------------------------------
