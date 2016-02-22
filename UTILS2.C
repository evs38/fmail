//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2016 Wilfred van Velzen
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "utils.h"

#include "ispathch.h"

extern configType config;

//---------------------------------------------------------------------------
char *findCLiStr(char *s1, const char *s2)
{
  char *helpPtr;

  if (strnicmp(s1, s2, strlen(s2)) == 0)
    return s1;

  helpPtr = s1;

  while ((helpPtr = stristr(helpPtr+1, s2)) != NULL)
    if (*(helpPtr - 1) == '\r' || *(helpPtr - 1) == '\n')
      return helpPtr;

  return NULL;
}
//---------------------------------------------------------------------------
char *insertLine(char *pos, const char *line)
{
  u16 s = strlen(line);

  memmove(pos + s, pos, strlen(pos) + 1);
  memcpy(pos, line, s);

  return pos + s;
}
//---------------------------------------------------------------------------
u16  nodeStrIndex = 0;
char nodeNameStr[2][24];

const char *nodeStr(const nodeNumType *nodeNum)
{
  char *tempPtr = nodeNameStr[nodeStrIndex = 1 - nodeStrIndex];

  if (nodeNum->zone != 0)
    tempPtr += sprintf(tempPtr, "%u:", nodeNum->zone);

  if (nodeNum->point != 0)
    sprintf(tempPtr, "%u/%u.%u", nodeNum->net, nodeNum->node, nodeNum->point);
  else
    sprintf(tempPtr, "%u/%u"   , nodeNum->net, nodeNum->node);

  return nodeNameStr[nodeStrIndex];
}
//---------------------------------------------------------------------------
char *stristr(const char *str1, const char *str2)
{
  unsigned int len1 = strlen(str1);
  unsigned int len2 = strlen(str2);
  unsigned int i,j,k;

  if ( !len2 )
    return (char *)str1;
  if ( !len1 )
    return 0;
  i = 0;
  for (;;)
  {
    while( i < len1 && toupper(str1[i]) != toupper(str2[0]) )
      ++i;
    if ( i == len1 )
      return 0;
    j = 0;
    k = i;
    while (i < len1 && j < len2 && toupper(str1[i]) == toupper(str2[j]))
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
//---------------------------------------------------------------------------
void MakeJamAreaPath(rawEchoType *echo)
{
  char *tempPtr
     , *tempPtr2
     ;
  tempPtr = tempPtr2 = strchr(echo->msgBasePath, 0);
  strncpy(tempPtr, echo->areaName, MB_PATH_LEN - strlen(echo->msgBasePath) - 1);
  if (!config.genOptions.lfn)
  {
    while (*tempPtr)
    {
      if (!isalnum(*tempPtr))
        strcpy(tempPtr, tempPtr + 1);
      else
        ++tempPtr;
    }
    tempPtr2[8] = 0;
    strupr(echo->msgBasePath);
  }
  else
  {
    int toLower = 1;
    while (*tempPtr)
    {
      if (!ispathch(*tempPtr))
        strcpy(tempPtr, tempPtr + 1);
      else
        if (islower(*tempPtr++))
          toLower = 0;
    }
    if (toLower)
      strlwr(tempPtr2);
  }
}
//---------------------------------------------------------------------------

