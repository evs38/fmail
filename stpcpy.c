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

#include "stpcpy.h"

//---------------------------------------------------------------------------
char *stpmove(char *dst, const char *src)
{
  size_t l = strlen(src);

  return memmove(dst, src, l + 1) + l;
}
//---------------------------------------------------------------------------
