//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
#include "os.h"

//----------------------------------------------------------------------------
#ifdef __linux__

#include "stpcpy.h"

#include <sys/file.h>   // flock()
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>      // isalpha
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>      // FILENAME_MAX
#include <unistd.h>

//----------------------------------------------------------------------------
char replaceDrive[FILENAME_MAX];

//----------------------------------------------------------------------------
int eof(int fd)
{
  off_t endPosn
      , curPosn;

  // Save current file position.
  if ((curPosn = lseek(fd, 0, SEEK_CUR)) == -1)
    return -1;

  // Get ending file position.
  if ((endPosn = lseek(fd, 0, SEEK_END)) == -1)
    return -1;

  // Restore current file position.
  if (lseek(fd, curPosn, SEEK_SET) == -1)
    return -1;

  return curPosn >= endPosn;
}
//----------------------------------------------------------------------------
int _sopen(const char *pathname, int flags, int shflags, ... /* mode_t mode */)
{
  int fd;

  if (flags & O_CREAT)
  {
    va_list ap;

    va_start(ap, shflags);
    fd = open(pathname, flags, va_arg(ap, mode_t));
    va_end(ap);
  }
  else
    fd = open(pathname, flags);

  if (fd >= 0 && (shflags == SH_DENYRW || shflags == SH_DENYWR || shflags == SH_DENYRD))
    if (flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
      close(fd);

      return -1;
    }

  return fd;
}
//---------------------------------------------------------------------------
#define dFIXPATHBUFFERS  8
const char *fixPath(const char *path)
{
  static char nPath[dFIXPATHBUFFERS][FILENAME_MAX];
  static int  c = -1;
  char *p;

  if (path == NULL)
    return "";

  if (++c >= dFIXPATHBUFFERS)
    c = 0;

  p = nPath[c];

  if (isalpha(*path) && path[1] == ':' && *replaceDrive)
  {
    path += 2;
    p = stpcpy(p, replaceDrive);
    if (isDirSep(*path))
      path++;
  }

  do
  {
    if (*path == dDIRSEPCa)
      *p++ = dDIRSEPC;
    else
      *p++ = *path;
  }
  while (*path++ != 0);

  return nPath[c];
}
//----------------------------------------------------------------------------
#endif // __linux__
