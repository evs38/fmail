/*
 *  Copyright (C) 2007 Folkert J. Wijnstra
 *
 *
 *  This file is part of FMail.
 *
 *  FMail is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  FMail is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fmail.h"

s32 getGroupCode(char *groupString)
{
  s32 code;

  if (strchr(groupString, '*') != NULL)
    return 0x03ffffffL;

  code = 0;
  while (*groupString)
  {
    if (isalpha(*groupString))
      code |= (1L << (toupper(*(groupString++)) - 'A'));
    else
      groupString++;
  }
  return code;
}



char getGroupChar(s32 groupCode)
{
   char count = 0;

   while (groupCode != 0)
   {
      groupCode >>= 1;
      count++;
   }
   count += 'A'-1;
   return (count);
}



char *getGroupString(s32 groupCode, char *groupString)
{
   u16 count;
   u16 pos = 0;

   for (count = 0; count <= 26; count++)
      if (groupCode & (1L << count))
         groupString[pos++] = 'A' + count;
   groupString[pos] = 0;
   return groupString;
}
