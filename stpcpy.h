#ifndef stpcpyH
#define stpcpyH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2016 - 2017  Wilfred van Velzen
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

#ifdef __MINGW32__

static inline char *stpcpy(char *dst, const char *src)
{
  while ((*dst = *src))
    ++dst, ++src;

  return dst;
}

#endif
//---------------------------------------------------------------------------
#ifdef __linux__
#include <string.h>
#else
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
//---------------------------------------------------------------------------
#endif // __linux__

char *stpmove(char *dst, const char *src);

__inline static char *strmove(char *dst, const char *src)
{
  return memmove(dst, src, strlen(src));
}

#endif  // stpcpyH
