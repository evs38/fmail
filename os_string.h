#ifndef os_stringH
#define os_stringH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2016 Wilfred van Velzen
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

#ifdef __linux__
#include <ctype.h>  // toupper() tolower()
#endif // __linux__

#ifdef __MINGW32__
static inline char *stpcpy(char *dst, const char *src)
{
  while ((*dst = *src))
    ++dst, ++src;

  return dst;
}
#endif
//---------------------------------------------------------------------------
#ifndef __linux__
#ifdef __BORLANDC__
#pragma warn -8060
#pragma warn -8027
#endif // __BORLANDC__

__inline static char *stpncpy(char *dst, const char *src, int n)
{
  while (n-- > 0  && (*dst = *src))
    ++dst, ++src;

  return dst;
}
#endif // __linux__
//---------------------------------------------------------------------------
#ifdef __linux__
__inline static char *strupr(char *s)
{
  char *p;

  for (p = s; *p; ++p)
    *p = toupper(*p);

  return s;
}
//---------------------------------------------------------------------------
__inline static char *strlwr(char *s)
{
  char *p;

  for (p = s; *p; ++p)
    *p = tolower(*p);

  return s;
}
#endif // __linux__
//---------------------------------------------------------------------------
#endif  // os_stringH
