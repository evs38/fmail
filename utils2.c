//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2017 Wilfred van Velzen
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

#ifdef __WIN32__
#include <direct.h>  // chdir() _chdrive()
#endif // __WIN32__
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdarg.h>  // va_start()
#include <stdlib.h>  // atoi()
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>  // chdir()

#include "utils.h"

#include "ispathch.h"
#include "minmax.h"
#include "os.h"
#include "os_string.h"
#include "spec.h"
#include "stpcpy.h"

extern configType config;

//---------------------------------------------------------------------------
char *findCLiStr(char *s1, const char *s2)
{
  char *helpPtr;

  if (strnicmp(s1, s2, strlen(s2)) == 0)
    return s1;

  helpPtr = s1;

  while ((helpPtr = stristr(helpPtr + 1, s2)) != NULL)
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
u16  nodeStrIndex = -1;
char nodeNameStr[NO_NodeStrIndex][24];  // 65535:65535/65535.65535  4 * 5 + 3 + 1 = 24

char *tmpNodeStr(void)
{
  if (++nodeStrIndex >= NO_NodeStrIndex)
    nodeStrIndex = 0;

  return nodeNameStr[nodeStrIndex];
}
//---------------------------------------------------------------------------
const char *nodeStr(const nodeNumType *nodeNum)
{
  char *t1 = tmpNodeStr()
     , *t2 ;

  if (nodeNum->zone != 0)
    t2 = t1 + sprintf(t1, "%u:", nodeNum->zone);
  else
    t2 = t1;

  if (nodeNum->point != 0)
    sprintf(t2, "%u/%u.%u", nodeNum->net, nodeNum->node, nodeNum->point);
  else
    sprintf(t2, "%u/%u"   , nodeNum->net, nodeNum->node);

  return t1;
}
//---------------------------------------------------------------------------
s16 node4d(nodeNumType *node)
{
  u16 count = 0;

  while (  count < MAX_AKAS
        && (  config.akaList[count].nodeNum.zone == 0
           || node->net != config.akaList[count].fakeNet
           || node->point != 0
           )
        )
    count++;

  if (count < MAX_AKAS)
  {
    node->point = node->node;
    memcpy(node, &config.akaList[count].nodeNum, 6);

    return count;
  }
  return -1;
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
void MakeJamAreaPath(rawEchoType *echo, const char *(*CallBackGetAreaPath)(u16))
{
  int toLower = 1;
  char *tempPtr
     , *tempPtr2
     ;
  tempPtr = tempPtr2 = strchr(echo->msgBasePath, 0);
  strncpy(tempPtr, echo->areaName, MB_PATH_LEN - strlen(echo->msgBasePath) - 1);
  while (*tempPtr)
  {
    if (!ispathch(*tempPtr))
      strmove(tempPtr, tempPtr + 1);
    else
      if (islower(*tempPtr++))
        toLower = 0;
  }
  if (toLower)
    strlwr(tempPtr2);

  if (NULL != CallBackGetAreaPath)
    for (;;)
    {
      u16 i;
      const char *ap;

      for (i = 0;; i++)
      {
        if (NULL == (ap = CallBackGetAreaPath(i)))
          return;

        if (0 == stricmp(ap, echo->msgBasePath) )
          break;
      }

      tempPtr = strchr(echo->msgBasePath, 0) - 1;
      if (!isdigit(*tempPtr))
      {
        *++tempPtr = '0';
        *++tempPtr = 0;
      }
      else
      {
        int num;
        while (isdigit(*--tempPtr))
          ;
        num = atoi(++tempPtr) + 1;
        sprintf(tempPtr, "%d", num);
      }
    }
}
//---------------------------------------------------------------------------
int ChDir(const char *path)
{
#if defined(__MSDOS__) && !defined(__WIN32__)
  if (path[1] == ':')
  {
    char drive = toupper(path[0]);
    if (drive >= 'A' && drive <= 'Z')
      if (0 != _chdrive(drive - 'A' + 1))
        return -1;
  }
#endif
  return chdir(fixPath(path));
}
//---------------------------------------------------------------------------
int dirExist(const char *dir)
{
  DIR *d = opendir(fixPath(dir));
  if (d)
  {
    closedir(d);
    return 1;
  }

  return 0;
}
//---------------------------------------------------------------------------
int dirIsEmpty(const char *dirname)
{
  DIR *dir = opendir(fixPath(dirname));
  struct dirent *de;

  if (dir == NULL)
    // Directory doesn't exist, gives the same result as not empty
    return 0;

  while ((de = readdir(dir)) != NULL)
  {
    // Ignore "." and ".." directory members.
    const char *dn = de->d_name;
    if (dn[0] == '.' && (dn[1] == 0 || (dn[1] == '.' && dn[2] == 0)))
      continue;

    // Found a directory entry, so it's not empty
    closedir(dir);
    return 0;
  }
  // Directory is empty
  closedir(dir);
  return 1;
}
//---------------------------------------------------------------------------
// Return 0 if pattern doesn't exist in path
//
int existPattern(const char *path, const char *pattern)
{
  DIR           *dir;
  struct dirent *ent = NULL;

  if ((dir = opendir(fixPath(path))) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
      if (match_spec(pattern, ent->d_name))
        break;

    closedir(dir);
  }

  return ent != NULL;
}
//---------------------------------------------------------------------------
int isFile(const char *path)
{
  struct stat st;

  return stat(fixPath(path), &st) == 0 && S_ISREG(st.st_mode);
}
//---------------------------------------------------------------------------
#ifndef __linux__
int dprintf(int fd, const char *format, ...)
{
  char buf[2048];
  va_list argptr;
  int bw;

  va_start(argptr, format);
  bw = vsnprintf(buf, sizeof(buf), format, argptr);
  va_end(argptr);

  if (bw >= 0)
    write(fd, buf, min(bw, sizeof(buf)));

  return bw;
}
#endif  // __linux__
//---------------------------------------------------------------------------
const char *isoFmtTime(const time_t t)
{
  return tm2str(localtime(&t));  // localtime ok
}
//---------------------------------------------------------------------------
const char *tm2str(struct tm *tm)
{
  static char tStr[24];  // 20 should be enough

  if (NULL != tm)
    sprintf( tStr, "%04d-%02d-%02d %02d:%02d:%02d"
           , tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday
           , tm->tm_hour, tm->tm_min, tm->tm_sec
           );
  else
    strcpy(tStr, "*** Illegal time ***");

  return tStr;
}
//---------------------------------------------------------------------------
