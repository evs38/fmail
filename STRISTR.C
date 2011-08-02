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


#include <string.h>
#include <ctype.h>


char * stristr(const char *str1, const char *str2)
{
	 unsigned int len1 = strlen(str1);
	 unsigned int len2 = strlen(str2);
	 unsigned int i,j,k;

	 if ( !len2 )
		  return (char *)str1;
	 if ( !len1 )
		  return 0;
	 i = 0;
	 for( ; ; )
	 {
		  while( i < len1 && toupper(str1[i]) != toupper(str2[0]) )
				 ++i;
		  if ( i == len1 )
				return 0;
		  j = 0;
		  k = i;
		  while ( i < len1 && j < len2 && toupper(str1[i]) == toupper(str2[j]) )
		  {
				++i;
				++j;
		  }
		  if ( j == len2 )
				return (char *)str1 + k;
		  if ( i == len1 )
				return 0;
		  i = k + 1;
	 }
}

