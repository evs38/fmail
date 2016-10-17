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
#include <dir.h>     // getcwd()
#ifdef __DOS
#include <direct.h>  // _chdrive()
#endif
#include <dirent.h>
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>   // getenv()
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <utime.h>
#if defined(__WIN32__)
#include <windows.h>
#endif

#include "utils.h"

#include "areainfo.h"
#include "log.h"
#include "msgpkt.h"  // for openP
#include "spec.h"
#include "stpcpy.h"
#include "version.h"

extern configType config;
psRecType *seenByArray   = NULL;
psRecType *tinySeenArray = NULL;
psRecType *pathArray     = NULL;
psRecType *tinyPathArray = NULL;

extern u16 echoCount;
extern u16 forwNodeCount;

extern cookedEchoType *echoAreaList;
extern nodeFileType    nodeFileInfo;

extern const char *months;
extern const char *dayName[7];

extern char *version;

//---------------------------------------------------------------------------
int moveFile(const char *oldName, const char *newName)
{
  fhandle h1
        , h2;
  char   *buf;
  size_t  count
        , size;

  if (rename(oldName, newName))
  {
#ifdef __BORLANDC__
    if (errno != ENOTSAM)
      return -1;
#endif
    if ((h1 = _sopen(oldName, O_RDONLY | O_BINARY, SH_DENYRW, 0)) == -1)
      return -1;

    if ((h2 = _sopen(newName, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, S_IREAD | S_IWRITE)) == -1)
    {
      close(h1);
      return -1;
    }
    size = min(32767, (size_t)filelength(h1));
    if ((buf = (char*)malloc(size)) == NULL)
    {
      close(h1);
      close(h2);
      return -1;
    }
    while ((count = read(h1, buf, size)) > 0)
      write(h2, buf, count);

    {
      struct stat statbuf;
      if (0 == fstat(h1, &statbuf))
      {
#ifdef __MINGW32__
#define utimbuf _utimbuf
#endif // __MINGW32__
        struct utimbuf utbuf;
        utbuf.actime  = statbuf.st_atime;
        utbuf.modtime = statbuf.st_mtime;
        _futime(h2, &utbuf);
      }
    }
    close(h1);
    close(h2);
    free(buf);
    unlink(oldName);
  }
  return 0;
}
//---------------------------------------------------------------------------
int dirExist(const char *dir)
{
  DIR *d = opendir(dir);
  if (d)
  {
    closedir(d);
    return 1;
  }

  return 0;
}
//---------------------------------------------------------------------------
s16 existDir(const char *dir, const char *descr)
{
  tempStrType tempStr;

  if (dirExist(dir))
    return 1;

  sprintf(tempStr, "The %s [%s] directory does not exist", descr, dir);
  logEntry(tempStr, LOG_ALWAYS, 0);

  return 0;
}
//---------------------------------------------------------------------------
// Usage           char *searchpath(const char *filename);
//
// Description     searchpath searches for the file 'filename' in the
//                 current directory, and then in the list of semicolon-
//                 separated directories specifed by the "PATH" environment variable.
//
// Return value    A pointer to the filename string if the file is successfully
//                 found; this string is stored in a static array that is
//                 overwritten with each call.  NULL is returned if the
//                 file is not found.
//
static char pathbuf[FILENAME_MAX];

const char *_searchpath(const char *filename)
{
  const char *ipath;
  char c;
  int  len;
  char *temp;

  // If the environment variable isn't defined, at least try
  // the current directory.
  if ((ipath = getenv("PATH")) == NULL)
    ipath = "";

  // Try the current directory, then all directories in the
  // string ipath.
  if (getcwd(pathbuf, FILENAME_MAX) == NULL)
    len = 0;
  else
    len = strlen(pathbuf);

  for (;;)
  {
    // The next directory to try is already in pathname, and its
    // length is in len.  If it doesn't end in a slash, and isn't
    // blank, append a slash.

    //pathbuf[len] = '\0';

    if (len != 0 && (c = pathbuf[len - 1]) != '\\' && c != '/')
      pathbuf[len++] = '\\';

    // Append the filename to the directory name, then break
    // if the file exists.
    strcpy(pathbuf + len, filename);
    if (access(pathbuf, 0) == 0)
      break;

    // Try the next directory in the ipath string.

    if (*ipath == '\0')           // end of the variable
      return NULL;

    for (len = 0; *ipath != ';' && *ipath != '\0'; ipath++)
    {
      // Strip off any quotes around this individual path dir
      if (*ipath != '\"')
        pathbuf[len++] = *ipath;  // copy next directory
    }
    if (*ipath != '\0')
      ipath++;                    // skip over semicolon
  }

  // Pathname contains the relative path of the found file.  Convert
  // it to an absolute path.
  if ((temp = _fullpath(NULL, pathbuf, FILENAME_MAX)) != NULL)
  {
    strcpy(pathbuf, temp);
    free(temp);
  }

  return (pathbuf[0] == '\0' ? NULL : pathbuf);
}
//---------------------------------------------------------------------------
// Return 0 if pattern doesn't exist in path
//
int existPattern(const char *path, const char *pattern)
{
  DIR           *dir;
  struct dirent *ent = NULL;

  if ((dir = opendir(path)) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
      if (match_spec(pattern, ent->d_name))
        break;

    closedir(dir);
  }

  return ent != NULL;
}
//---------------------------------------------------------------------------
#ifdef __WIN32__
const char *fmtU64(u64 u)
{
  static tempStrType tempStr;
#if defined(__BORLANDC__)
#define dKBC (100ui64 * 1024)
#define dMBC (100ui64 * 1024 * 1024)
#define dGBC (100ui64 * 1024 * 1024 * 1024)
#define dLLFMT "%Lu"
#elif defined(__MINGW32__)
#define dKBC (100ull * 1024)
#define dMBC (100ull * 1024 * 1024)
#define dGBC (100ull * 1024 * 1024 * 1024)
#define dLLFMT "%I64u"
#endif

  if (u >= dKBC)
  {
    if (u >= dMBC)
    {
      if (u >= dGBC)
        sprintf(tempStr, dLLFMT"GB", u / (1024 * 1024 * 1024));
      else
        sprintf(tempStr, dLLFMT"MB", u / (1024 * 1024));
    }
    else
      sprintf(tempStr, dLLFMT"KB", u / 1024);
  }
  else
    sprintf(tempStr, dLLFMT, u);

  return tempStr;
}
//---------------------------------------------------------------------------
u64 diskFree64(const char *path)
{
  ULARGE_INTEGER ul;

  if (GetDiskFreeSpaceEx(path, &ul, NULL, NULL))
    return ul.QuadPart;

  return 0;
}
#endif
//---------------------------------------------------------------------------
u32 diskFree(const char *path)
{
#ifdef __WIN32__
  u64 ul = diskFree64(path);

  if (ul > (uint64_t)UINT32_MAX)
    return UINT32_MAX;

  return ul;
#else
  tempStrType  tempStr;
  struct dfree dtable;
  char        *helpPtr;
#ifdef __CANUSE64BIT
  u64          dfs;
#endif

  helpPtr = (strchr(path, 0) - 1);
  if (*helpPtr == '\\')
    *helpPtr = 0;
  else
    helpPtr = NULL;

  if (isalpha(path[0]) && path[1] == ':')
    getdfree(toupper(path[0]) - 'A' + 1, &dtable);
  else
  {
    getcwd(tempStr, sizeof(tempStrType));
    chdir(path);
    getdfree(0, &dtable);
    chdir(tempStr);
  }

  if (helpPtr != NULL)
    *helpPtr = '\\';

  if (dtable.df_sclus == (unsigned)-1)
    return UINT32_MAX;
#ifdef __CANUSE64BIT
  dfs = (u64)dtable.df_avail * (u64)dtable.df_bsec * (u64)dtable.df_sclus;

#ifdef _DEBUG
  sprintf(tempStr, "DEBUG Disk %s free: %s", path, fmtU64(dfs));
  logEntry(tempStr, LOG_DEBUG, 0);
#endif

  if (dfs > (uint64_t)UINT32_MAX)
    return UINT32_MAX;

  return (uint32_t)dfs;
#else
  return dtable.df_avail * (u32)dtable.df_bsec * dtable.df_sclus;
#endif
#endif
}
//---------------------------------------------------------------------------
u32 fileLength(int handle)
{
  long fl = filelength(handle);
  if (fl < 0)
    return 0;

  return (u32)fl;
}
//---------------------------------------------------------------------------
off_t fileSize(const char *filename)
{
  struct stat st;

  if (stat(filename, &st) == 0)
    return st.st_size;

  return -1;
}
//---------------------------------------------------------------------------
void touch(const char *path, const char *filename, const char *t)
{
  tempStrType tempStr;
  fhandle     tempHandle;

  strcpy(stpcpy(tempStr, path), filename);
  if ((tempHandle = open(tempStr, O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) != -1)
  {
    write(tempHandle, t, strlen(t));
    close(tempHandle);
  }
}
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
const char *strError(int errn)
{
  const static char *unknownStr = "Unknown error";

  if (errn >= 0 && errn < _sys_nerr)
    return _sys_errlist[errn];

  return unknownStr;
}
#endif
//---------------------------------------------------------------------------
// retMain: set to none 0 if you want the main aka returned in case of
// out of bounds request
//
const nodeNumType *getAkaNodeNum(int aka, int retmain)
{
  if (aka < 0 || aka >= MAX_AKAS)
    return retmain ? &config.akaList[0].nodeNum : NULL;

  return &config.akaList[aka].nodeNum;
}
//---------------------------------------------------------------------------
// retMain: set to none 0 if you want the main aka returned in case of
// out of bounds request
//
const char *getAkaStr(int aka, int retMain)
{
  static const char *nullnode = "0:0/0.0";
  const nodeNumType *node = getAkaNodeNum(aka, retMain);

  return NULL == node ? nullnode : nodeStr(node);
}
//---------------------------------------------------------------------------
int matchAka(nodeNumType *node, int useAka)
{
  int i
    , srcAka = -1
    , valid  = 6;

  if (useAka > 0 && useAka <= MAX_AKAS && config.akaList[useAka - 1].nodeNum.zone)
    return useAka - 1;

  do
  {
    for (i = 0; i < MAX_AKAS; i++)
      if (memcmp(node, &config.akaList[i].nodeNum, valid) == 0)
      {
        srcAka = i;
        break;
      }

    valid -= 2;
  }
  while (valid > 0 && srcAka < 0);

  return srcAka >=0 ? srcAka : 0;
}
//---------------------------------------------------------------------------
s16 emptyText(char *text)
{
  s16 code;

  code = (*text == '\r') || (*text == '\n') || (*text == 1) || (*text == 0);

  while (code && *text)
  {
    if (*text == '\r' || *text == '\n')
    {
      text++;
      code = (*text == '\r') || (*text == '\n') || (*text == 1) || (*text == 0);
    }
    else
      text++;
  }
  return code;
}
//---------------------------------------------------------------------------
void Delete(const char *path, const char *wildCard)
{
  DIR           *dir;
  struct dirent *ent;
  tempStrType    tempStr;
  char          *helpPtr;

  helpPtr = stpcpy(tempStr, path);

  if ((dir = opendir(path)) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
      if (match_spec(wildCard, ent->d_name))
      {
        strcpy(helpPtr, ent->d_name);
        unlink(tempStr);
      }

    closedir(dir);
  }
}
//---------------------------------------------------------------------------
s32 getSwitch(int *argc, char *argv[], s32 mask)
{
  s32         tempMask;
  s16         error = 0;
  s32         result = 0;
  int         count = *argc;
  tempStrType tempStr;

  while ( count && --count >= 1 )
  {
    if ( argv[count][0] == '/' )
    {
      if ( count != --(*argc) )
      {
         puts("Switches should be last on command line");
         exit(4);
      }
      if ((strlen(argv[count]) != 2) ||
          (!(isalpha(argv[count][1]))))
      {
         printf("Illegal switch: %s\n", argv[count]);
         error++;
      }
      else
      {
         tempMask = 1L << (toupper(argv[count][1]) - 'A');
         if (mask & tempMask)
            result |= tempMask;
         else
         {
            sprintf(tempStr, "Illegal switch: %s", argv[count]);
            logEntry(tempStr, LOG_ALWAYS, 0);
            error++;
         }
      }
    }
  }
  if (error)
    logEntry ("Bad parameters", LOG_ALWAYS, 4);

  return result;
}
//---------------------------------------------------------------------------
//
// SI     = Saved ID
// NI     = New ID    (Calculated from start time)
// MAXINT = 0xFFFFFFFF (2 ^ 32 - 1)
//
//
// NI > SI :
//
//    0              SI NI             MAXINT
// 1: +--------------+--+--------------+       =>  NI = NI
//
// and  NI - SI > MAXINT / 2 :
//
//    0  SI                         NI MAXINT
// 2: +--+--------------------------+--+       =>  NI = SI + 1
//
//
//
// NI <= SI :
//
//    0              NI SI             MAXINT
// 3: +--------------+--+--------------+       =>  NI = SI + 1
//
// and  SI - NI > MAXINT / 2 :
//
//    0  NI                         SI MAXINT
// 4: +--+--------------------------+--+       =>  NI = NI
//
//
//
// 1: Normal situation: new ID is a little bit bigger than the stored ID.
//
// 2: The stored ID is a lot smaller than the new ID: The stored ID probably
//    overflowed past the MAXINT value in the previous session.
//
// 3: The new ID is a little bit smaller than the stored ID or eqaul to: This
//    session is run very shortly after the previous one possibly within the
//    same second, and or the previous session used a lot of ID's.
//
// 4: The new ID is a log smaller than the stored ID: The new ID probably
//    overflowed past the MAXINT value.
//
extern time_t startTime;
u32 lastID = 0;

u32 uniqueID(void)
{
  if (lastID == 0)
  {
    lastID = (u32)startTime << 4;
    if (  (lastID >  config.lastUniqueID && (lastID - config.lastUniqueID) > 0x80000000UL)
       || (lastID <= config.lastUniqueID && (config.lastUniqueID - lastID) < 0x80000000UL)
       )
      lastID = config.lastUniqueID + 1;
#ifdef _DEBUG
    {
      tempStrType tempStr;
      sprintf(tempStr, "UID: Saved:%08X New:%08X", config.lastUniqueID, lastID);
      logEntry(tempStr, LOG_DEBUG, 0);
    }
#endif
  }
  else
    lastID++;

  return config.lastUniqueID = lastID;
}
//---------------------------------------------------------------------------
char *removeRe(char *str)
{
  s16 update;
  char *tstr = str;

  do
  {
    update = 0;
    if (!strnicmp(tstr, "RE:", 3) || !strnicmp(tstr, "(R)", 3))
    {
      tstr += 3;
      update++;
    }
    while (*tstr == ' ')
    {
      tstr++;
      update++;
    }
  } while (update);

  return (char*)memmove(str, tstr, strlen(tstr) + 1);
}
//---------------------------------------------------------------------------
void removeLfSr(char *msgText)
{
  if (config.mbOptions.removeLf)
    removeLf(msgText);
  if (config.mbOptions.removeSr)
    removeSr(msgText);
}
#ifdef _DEBUG0
#define _DEBUG_LOGREMOVELFSR
#endif
//---------------------------------------------------------------------------
void removeLf(char *msgText)
{
  char *oldStart
     , *newEnd
     , *helpPtr
     , *wPtr = NULL
     ;
#ifdef _DEBUG_LOGREMOVELFSR
  int   n = 0;
#endif
  oldStart = newEnd = helpPtr = msgText;
  while ((helpPtr = strchr(helpPtr, '\n')) != NULL)
  {
#ifdef _DEBUG_LOGREMOVELFSR
    n++;
#endif
    // Remove linefeeds (only if preceeded by a (soft) carriage return)
    if ((wPtr != helpPtr - 1 && *(helpPtr - 1) == '\r') || *(helpPtr - 1) == (char)0x8d)
    {
      *helpPtr = 0;
      if (newEnd != oldStart)
        newEnd = stpcpy(newEnd, oldStart);
      else
        newEnd = helpPtr;
      oldStart = ++helpPtr;
    }
    // Otherwise replace it with cr
    else
    {
      wPtr = helpPtr;
      *helpPtr++ = '\r';
    }
  }
  if (newEnd != oldStart)
    strcpy(newEnd, oldStart);
#ifdef _DEBUG_LOGREMOVELFSR
  if (n > 0)
  {
    tempStrType tempStr;
    sprintf(tempStr, "Removed/replaced %d line feed characters", n);
    logEntry(tempStr, LOG_DEBUG, 0);
  }
#endif
}
//---------------------------------------------------------------------------
void removeSr(char *msgText)
{
  char *oldStart,
       *newEnd,
       *helpPtr;
#ifdef _DEBUG_LOGREMOVELFSR
  int   n = 0;
#endif

  oldStart = newEnd = helpPtr = msgText;
  while ((helpPtr = strchr(helpPtr, 0x8d)) != NULL)
  {
#ifdef _DEBUG_LOGREMOVELFSR
    n++;
#endif
    // Replace soft CR's by a cr if followed by a CR
    if (*(helpPtr+1) == '\r')
      *helpPtr = '\r';
    // Remove soft CR's if not followed by a CR
    else
    {
      *helpPtr = 0;
      strcpy(newEnd, oldStart);
      oldStart = ++helpPtr;
      newEnd   = strchr (newEnd, 0);
    }
  }
  strcpy(newEnd, oldStart);
#ifdef _DEBUG_LOGREMOVELFSR
  if (n > 0)
  {
    tempStrType tempStr;
    sprintf(tempStr, "Removed/replaced %d soft carriage return characters", n);
    logEntry(tempStr, LOG_DEBUG, 0);
  }
#endif
}
//---------------------------------------------------------------------------
char *srchar(char *string, s16 t, s16 c)
{
   char *helpPtr1
      , *helpPtr2;
   s16  temp;

   if ((helpPtr1 = strchr (string, t)) == NULL)
      helpPtr1 = strchr (string, 0);
   temp = *helpPtr1;
   *helpPtr1 = 0;
   helpPtr2 = strrchr (string, c);
   *helpPtr1 = temp;
   return (helpPtr2);
}
//---------------------------------------------------------------------------
extern u16  nodeStrIndex;
extern char nodeNameStr[NO_NodeStrIndex][24];

const char *nodeStrZ(const nodeNumType *nodeNum)
{
  char *t1 = tmpNodeStr()
     , *t2;

  if (nodeNum->zone != 0)
    t2 = t1 + sprintf(t1, "%u:", nodeNum->zone);
  else
    t2 = t1;

  sprintf(t2, "%u/%u.%u", nodeNum->net, nodeNum->node, nodeNum->point);

  return t1;
}
//---------------------------------------------------------------------------
char *findCLStr(char *s1, const char *s2)
{
  char *helpPtr;

  if (strncmp(s1, s2, strlen(s2)) == 0)
    return s1;

  helpPtr = s1;

  while ((helpPtr = strstr(helpPtr + 1, s2)) != NULL)
    if (*(helpPtr - 1) == '\r' || *(helpPtr - 1) == '\n')
      return helpPtr;

  return NULL;
}
//---------------------------------------------------------------------------
s16 scanDate(const char *dtPtr, u16 *year , u16 *month, u16 *day, u16 *hours, u16 *minutes)
{
  char timeStr[6];
  char dateStr[9];

  if (dtPtr[0] > 5 || dtPtr[6] > 8)
    return -1;

  strncpy(timeStr, dtPtr + 1, 5);
  timeStr[(int)dtPtr[0]] = 0;
  strncpy(dateStr, dtPtr + 7, 8);
  dateStr[(int)dtPtr[6]] = 0;

  if (  sscanf(timeStr, "%hu%*c%hu", hours, minutes) != 2
     || sscanf(dateStr, "%hu%*c%hu%*c%hu", month, day, year) != 3
     )
    return -1;

  return 0;
}
//---------------------------------------------------------------------------
s32 checkDate(u16 year, u16 month, u16 day, u16 hours, u16 minutes, u16 seconds)
{
    struct tm tms;

    if (((year >= 100) && (year  < 1980)) || (year  > 2099) ||
        (month < 1)    || (month > 12)   ||
        (day   < 1)    || (day   > 31))
    {
       year  = 1980;
       month = 1;
       day   = 1;
    }
    if (year < 100)
    {
       if (year >= 80)
          year += 1900;
       else
          year += 2000;
    }
    if ((hours >= 24) || (minutes >= 60) || (seconds >= 60))
    {
       hours   = 0;
       minutes = 0;
       seconds = 0;
    }

    tms.tm_year = year - 1900;
    tms.tm_mon  = month - 1;
    tms.tm_mday = day;
    tms.tm_hour = hours;
    tms.tm_min  = minutes;
    tms.tm_sec  = seconds;

    return mktime(&tms);
}
//---------------------------------------------------------------------------
void removeLine(char *s)
{
  char *helpPtr;

  if ((helpPtr = strchr(s, 0x0d)) == NULL)
    *s = 0;
  else
  {
    if (*(++helpPtr) == 0x0a)
      helpPtr++;

    memmove(s, helpPtr, strlen(helpPtr) + 1);
  }
}
//---------------------------------------------------------------------------
char *insertLineN(char *pos, char *line, u16 num)
{
  char *helpPtr;

  while (*pos && num--)
  {
    if ((helpPtr = strchr(pos, '\r')) == NULL)
      pos = strchr(pos, 0);
    else
    {
      pos = helpPtr;
      while (*pos == '\r' || *pos == '\n')
        pos++;
    }
  }
  return insertLine(pos, line);
}
//---------------------------------------------------------------------------
static void readPathSeenBy(u16 type, char *msgText, psRecType *psArray, u16 *arrayCount)
{
   const char *searchStr;
   u16   searchLength;
   char *helpPtr1
      , *helpPtr2;
   u16   psNet = 0;
   s16   skip  = 0;
   u16   helpInt;
   u16   maxLength;

   if (type == ECHO_SEENBY)
   {
      searchStr    = "SEEN-BY: ";
      searchLength = 9;
      maxLength    = MAX_MSGSEENBY;
   }
   else
   {
      searchStr    = "\1PATH: ";
      searchLength = 7;
      maxLength    = MAX_MSGPATH;
   }

   *arrayCount = 0;

   helpPtr2 = msgText;
   if ((helpPtr1 = findCLStr(helpPtr2, " * Origin: ")) != NULL)
      helpPtr2 = helpPtr1;

   if ((helpPtr1 = (helpPtr2 = findCLStr(helpPtr2, searchStr))) != NULL)
   {
      while (memcmp(searchStr, helpPtr2, searchLength) == 0)
      {
        helpPtr2 += searchLength;
        while (isdigit(*helpPtr2))
        {
            helpInt = 0;
            do
            {
               helpInt = (helpInt << 3) + (helpInt << 1) + *(helpPtr2++) - '0';
            }
            while (isdigit(*helpPtr2));

            switch (*helpPtr2)
            {
               case '.'       : skip = 1;
                                break;
               case '/'       : psNet = helpInt;
                                break;
               default        : if ((*arrayCount < maxLength) &&
                                    (psNet != 0) &&
                                    (!skip))
                                {
                                   psArray[ *arrayCount   ].net  = psNet;
                                   psArray[(*arrayCount)++].node = helpInt;
                                }
                                skip = 0;
            }
            if (*helpPtr2)
               helpPtr2++;

            while (*helpPtr2 == ' ')
               helpPtr2++;
         }

         while ( (*helpPtr2 == '\r' + (char)0x80)
               || *helpPtr2 == '\r'
               || *helpPtr2 == '\n'
               )
            helpPtr2++;
      }
      strcpy(helpPtr1, helpPtr2);
   }
}
//---------------------------------------------------------------------------
void logPathSeenBy(const char *pre, u16 type, psRecType *psArray, int arrayCount)
{
  char tmpStr[0x4000]
    , *p;
  int  i;

  p = stpcpy(stpcpy(tmpStr, pre), type == ECHO_SEENBY ? "SEEN-BY:" : "PATH:");
  for (i = 0; i < arrayCount; i++)
    p += sprintf(p, " %u/%u", psArray[i].net, psArray[i].node);

  logEntry(tmpStr, LOG_ALWAYS, 0);
}
//---------------------------------------------------------------------------
static void writePathSeenBy(u16 type, char *pathSeen, psRecType *psArray, u16 arrayCount)
{
   char     *helpPtr,
            *helpPtr2,
            *startPtr;
   const char *name;
   u16      num,
	    count;
   char     tempStr[6];

   if (type == ECHO_SEENBY)
      name = "SEEN-BY: ";
   else
      name = "\1PATH: ";

   helpPtr = startPtr = pathSeen;
   tempStr[5] = 0;

   for (count = 0; count < arrayCount; count++)
   {
      if (helpPtr - startPtr >= 68 || helpPtr == pathSeen)
      {
        startPtr = helpPtr;

         if (helpPtr != pathSeen)
            *helpPtr++ = '\r';

         helpPtr = stpcpy (helpPtr, name);
         num = psArray[count].net;
         helpPtr2 = tempStr+5;
         do
         {
#ifndef __32BIT__
	   _AX = num;
	   _DX = 0;
	   _CX = 10;
	   __emit__ (0xf7, 0xf1);
	   num = _AX;
	   *(--helpPtr2) = _DX + '0';
#else
	   div_t divRes;

	   divRes = div (num, 10);
	   *(--helpPtr2) = divRes.rem + '0';
	   num = divRes.quot;
#endif /* __32BIT__ */
	 }
	 while (num != 0);
	 helpPtr = stpcpy (helpPtr, helpPtr2);

	 *(helpPtr++) = '/';
      }
      else
      {
         *(helpPtr++) = ' ';
	       if (psArray[count].net != psArray[count - 1].net)
         {
            num = psArray[count].net;
            helpPtr2 = tempStr+5;
            do
	          {
#ifndef __32BIT__
	       _AX = num;
	       _DX = 0;
	       _CX = 10;
	       __emit__ (0xf7, 0xf1);
	       num = _AX;
	       *(--helpPtr2) = _DX + '0';
#else
	       div_t divRes;

	       divRes = div (num, 10);
	       *(--helpPtr2) = divRes.rem + '0';
	       num = divRes.quot;
#endif /* __32BIT__ */
	    }
	    while (num);
	    helpPtr = stpcpy (helpPtr, helpPtr2);

	    *(helpPtr++) = '/';
	 }
      }
      num = psArray[count].node;
      helpPtr2 = tempStr+5;
      do
      {
#ifndef __32BIT__
	 _AX = num;
	 _DX = 0;
	 _CX = 10;
	 __emit__ (0xf7, 0xf1);
	 num = _AX;
	 *(--helpPtr2) = _DX + '0';
#else
	 div_t divRes;

	 divRes = div (num, 10);
	 *(--helpPtr2) = divRes.rem + '0';
	 num = divRes.quot;
#endif /* __32BIT__ */
      }
      while (num);
      helpPtr = stpcpy (helpPtr, helpPtr2);
   }
   *(helpPtr++) = '\r';
   *helpPtr = 0;
}
//---------------------------------------------------------------------------
static s16 addSeenByNode(u16 net, u16 node, psRecType *array, u16 *seenByCount)
{
  u16 count;

  for (count = 0; (count < *seenByCount) &&
                 ((array[count].net < net) ||
                 ((array[count].net == net) &&
                  (array[count].node < node)))
      ; count++)
  {}

  if (count == *seenByCount)
  {
    array[ *seenByCount   ].net  = net;
    array[(*seenByCount)++].node = node;
  }
  else
    if (array[count].net != net || array[count].node != node)
    {
      memmove ( &(array[count + 1]), &(array[count])
              , (((*seenByCount)++) - count) * sizeof(psRecType));
      array[count].net  = net;
      array[count].node = node;
    }
    else
      return 0;

  return 1;
}
//---------------------------------------------------------------------------
int getLocalAkaNum(const nodeNumType *node)
{
  int i = 0;

  while (  i < MAX_AKAS
        && memcmp(node, &config.akaList[i].nodeNum, sizeof(nodeNumType)) != 0
        )
    i++;

  return i < MAX_AKAS ? i : -1;
}
//---------------------------------------------------------------------------
int getLocalAka(const nodeNumType *node)
{
  int i = 0;

  while (  i < MAX_AKAS
        && (  config.akaList[i].nodeNum.zone == 0
           || memcmp(&(node->net), &(config.akaList[i].nodeNum.net), sizeof(nodeNumType) - sizeof(u16)) != 0
           )
        )
    i++;

  return i < MAX_AKAS ? i : -1;
}
//---------------------------------------------------------------------------
s16 isLocalPoint(nodeNumType *node)
{
   u16 count;

   if (node->point == 0)
      return 0;
   count = 0;
   while ((count < MAX_AKAS) &&
          ((config.akaList[count].nodeNum.point != 0) ||
           (memcmp (node, &config.akaList[count].nodeNum, sizeof(nodeNumType)-2) != 0)))
   {
      count++;
   }
   return count == MAX_AKAS ? 0 : 1;
}


s16 node2d(nodeNumType *node)
{
   u16 count;

   count = 0;
   while ((count < MAX_AKAS) &&
          ((config.akaList[count].nodeNum.zone == 0) ||
           (config.akaList[count].fakeNet == 0) ||
           (node->net   != config.akaList[count].nodeNum.net) ||
           (node->node  != config.akaList[count].nodeNum.node) ||
           (node->point == 0)))
   {
      count++;
   }
   if (count < MAX_AKAS)
   {
      node->zone  = config.akaList[count].nodeNum.zone;
      node->net   = config.akaList[count].fakeNet;
      node->node  = node->point;
      node->point = 0;
      return count;
   }
   return -1;
}
//---------------------------------------------------------------------------
void make2d(internalMsgType *message)
{
  char *helpPtr;

  // 2d source
  if ((helpPtr = findCLStr(message->text, "\1FMPT")) != NULL)
    message->srcNode.point = atoi(helpPtr+5);

  if ((node2d(&message->srcNode) != -1) && helpPtr != NULL)
    removeLine(helpPtr);

  // 2d destination
  if ((helpPtr = findCLStr(message->text, "\1TOPT")) != NULL)
    message->destNode.point = atoi(helpPtr + 5);

  if ((node2d(&message->destNode) != -1) && helpPtr != NULL)
    removeLine(helpPtr);
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
void point4d(internalMsgType *message)
{
   char       *helpPtr;
   tempStrType tempStr;

   // 4d source
   if ((helpPtr = findCLStr(message->text, "\1FMPT")) != NULL)
   {
      message->srcNode.point = atoi(helpPtr + 5);
      removeLine(helpPtr);
   }
   node4d(&message->srcNode);
   if (message->srcNode.point != 0)
   {
      sprintf(tempStr, "\1FMPT %u\r", message->srcNode.point);
      insertLine(message->text, tempStr);
   }

   // 4d destination
   if ((helpPtr = findCLStr(message->text, "\1TOPT")) != NULL)
   {
      message->destNode.point = atoi(helpPtr + 5);
      removeLine(helpPtr);
   }
   node4d(&message->destNode);
   if (message->destNode.point != 0)
   {
      sprintf(tempStr, "\1TOPT %u\r", message->destNode.point);
      insertLine(message->text, tempStr);
   }
}
//---------------------------------------------------------------------------
void make4d(internalMsgType *message)
{
   char       *helpPtr;
   nodeNumType tempNode;

   message->srcNode.zone  = config.akaList[0].nodeNum.zone;
   message->destNode.zone = config.akaList[0].nodeNum.zone;

   if ((helpPtr = findCLStr(message->text, "\x1MSGID: ")) != NULL)
   {
      memset(&tempNode, 0, sizeof(nodeNumType));
      if (sscanf(helpPtr += 8, "%hu:%hu/%hu", &tempNode.zone, &tempNode.net, &tempNode.node) == 3)
      {
         message->srcNode.zone  = tempNode.zone;
         message->destNode.zone = tempNode.zone;
      }
   }

   if ((helpPtr = findCLStr(message->text, "\x1INTL ")) != NULL)
   {
      memset(&tempNode, 0, sizeof(nodeNumType));
      if (sscanf(helpPtr += 6, "%hu:%hu/%hu", &tempNode.zone, &tempNode.net, &tempNode.node) == 3)
      {
         if (tempNode.zone && (*(s32*)&tempNode.net == *(s32*)&message->destNode.net))
            message->destNode.zone = tempNode.zone;

         memset(&tempNode, 0, sizeof(nodeNumType));
         if (sscanf(strchr(helpPtr, ' '), "%hu:%hu/%hu", &tempNode.zone, &tempNode.net, &tempNode.node) == 3)
            if (tempNode.zone && (*(s32*)&tempNode.net == *(s32*)&message->srcNode.net))
               message->srcNode.zone = tempNode.zone;
      }
   }
   point4d(message);
}
//---------------------------------------------------------------------------
int comparSeenBy(const void *p1, const void *p2)
{
  if (((psRecType*)p1)->net == ((psRecType*)p2)->net)
    return ((psRecType*)p1)->node - ((psRecType*)p2)->node;

  return ((psRecType*)p1)->net - ((psRecType*)p2)->net;
}
//---------------------------------------------------------------------------
#ifdef _DEBUG0
#define _DEBUG_LOGPATHSEENBY
#endif

void addPathSeenBy(internalMsgType *msg, echoToNodeType echoToNode, u16 areaIndex, nodeNumType *rescanNode)
{
  u16          seenByCount   = 0
             , tinySeenCount = 0
             , pathCount     = 0
             , tinyPathCount = 0
             , count
             , count2;
  u32          bitshift;
  nodeFakeType mainNode;

  mainNode = config.akaList[echoAreaList[areaIndex].address];

  if (mainNode.nodeNum.point != 0 && mainNode.fakeNet != 0)
  {
    count2 = 0;
    for (count = 0; count < forwNodeCount; count++)
      if (ETN_ANYACCESS(echoToNode[ETN_INDEX(count)], count))
        count2 |= nodeFileInfo[count]->destFake;

    if (count2 != 0)
    {
      mainNode.nodeNum.net   = mainNode.fakeNet;
      mainNode.nodeNum.node  = mainNode.nodeNum.point;
      mainNode.nodeNum.point = 0;
    }
    else
      mainNode.fakeNet = 0;
  }
  else
    mainNode.fakeNet = 0;

  // SEEN-BY
  readPathSeenBy(ECHO_SEENBY, msg->text, seenByArray, &seenByCount);
#ifdef _DEBUG_LOGPATHSEENBY
  logPathSeenBy("DEBUG 1r ", ECHO_SEENBY, *seenByArray, seenByCount);
#endif

  // Remove fake-net from seen-by
  if (seenByCount != 0)
    for (count = 0; count < MAX_AKAS; count++)
      if (config.akaList[count].nodeNum.zone != 0)
        for (count2 = 0; count2 < seenByCount; count2++)
          if (config.akaList[count].fakeNet == seenByArray[count2].net)
            memcpy( &(seenByArray[count2])
                  , &(seenByArray[count2 + 1])
                  , ((--seenByCount) - count2) * sizeof(psRecType)
                  );

  // Sort seen-by list
  qsort(seenByArray, seenByCount, sizeof(psRecType), comparSeenBy);

#ifdef _DEBUG_LOGPATHSEENBY
  logPathSeenBy("DEBUG 1s ", ECHO_SEENBY, seenByArray, seenByCount);
#endif

  // Add other nodes to SEENBY, or asking node in case of rescan
  if (NULL != rescanNode)
  {
    // Rescan
    if (rescanNode->point == 0)
    {
      addSeenByNode(rescanNode->net, rescanNode->node, seenByArray  , &seenByCount  );
      addSeenByNode(rescanNode->net, rescanNode->node, tinySeenArray, &tinySeenCount);
    }
  }
  else
    for (count = 0; count < forwNodeCount; count++)
    {
      // Add only nodes to the seen-by's that get the message forwarded == Read Access
      if (   ETN_READACCESS(echoToNode[ETN_INDEX(count)], count)
         && !nodeFileInfo[count]->destFake
         &&  nodeFileInfo[count]->destNode.point == 0
         )
      {
        // Assign return value of addSeenByNode to
        // echoToNode[ETN_INDEX(count)] & ETN_SET(count) when SEEN-BY usage is required
        count2 = addSeenByNode( nodeFileInfo[count]->destNode.net
                              , nodeFileInfo[count]->destNode.node
                              , seenByArray
                              , &seenByCount
                              );

        if (echoAreaList[areaIndex].options.checkSeenBy && count2 == 0)
          echoToNode[ETN_INDEX(count)] &= ETN_RESET(count);

        addSeenByNode( nodeFileInfo[count]->destNode.net
                     , nodeFileInfo[count]->destNode.node
                     , tinySeenArray
                     , &tinySeenCount
                     );
#ifdef _DEBUG0
        {
          tempStrType ts;
          sprintf(ts, "DEBUG Add other node to SEENBY: %s", nodeStr(&nodeFileInfo[count]->destNode));
          logEntry(ts, LOG_DEBUG, 0);
        }
#endif
      }
    }

  // Add other AKAs to SEENBY
  if (echoAreaList[areaIndex].alsoSeenBy != 0)
  {
    bitshift = 1;
    for (count = 0; count < MAX_AKAS; count++)
    {
      if (  (echoAreaList[areaIndex].alsoSeenBy & bitshift)
         && config.akaList[count].nodeNum.zone  != 0
         && config.akaList[count].nodeNum.net   != 0
         && config.akaList[count].nodeNum.point == 0
         )
      {
        addSeenByNode(config.akaList[count].nodeNum.net, config.akaList[count].nodeNum.node, seenByArray  , &seenByCount  );
        addSeenByNode(config.akaList[count].nodeNum.net, config.akaList[count].nodeNum.node, tinySeenArray, &tinySeenCount);
#ifdef _DEBUG0
        {
          tempStrType ts;
          sprintf(ts, "DEBUG Add other AKA to SEENBY: %s", nodeStr(&config.akaList[count].nodeNum));
          logEntry(ts, LOG_DEBUG, 0);
        }
#endif
      }
      bitshift <<= 1;
    }
  }

  // Conditionally add main area address to SEENBY (PATH?)
  if (seenByCount == 0 || !mainNode.fakeNet)
  {
    addSeenByNode(mainNode.nodeNum.net, mainNode.nodeNum.node, seenByArray  , &seenByCount);
    addSeenByNode(mainNode.nodeNum.net, mainNode.nodeNum.node, tinySeenArray, &tinySeenCount);
  }

  writePathSeenBy(ECHO_SEENBY, msg->normSeen,   seenByArray,   seenByCount);
  writePathSeenBy(ECHO_SEENBY, msg->tinySeen, tinySeenArray, tinySeenCount);

#ifdef _DEBUG_LOGPATHSEENBY
  logPathSeenBy("DEBUG 2F ", ECHO_SEENBY, seenByArray  , seenByCount  );
  logPathSeenBy("DEBUG 2T ", ECHO_SEENBY, tinySeenArray, tinySeenCount);
#endif

  // PATH
  readPathSeenBy(ECHO_PATH, msg->text, pathArray, &pathCount);
#ifdef _DEBUG_LOGPATHSEENBY
  logPathSeenBy("DEBUG 1  ", ECHO_PATH, pathArray, pathCount);
#endif

  if (!mainNode.nodeNum.point || config.mailOptions.addPointToPath)             // If not a point or if option "Add point to path'
  {
    if (  pathCount == 0                                                            // Er is nog geen path
       || (  !mainNode.fakeNet                                                      // Geen fakenet
          && memcmp(&pathArray[pathCount - 1], &mainNode.nodeNum.net, 4) != 0       // En hij staat er nog niet in als laatste
          )
       )
      memcpy(&pathArray[pathCount++].net, &mainNode.nodeNum.net, 4);

    memcpy(&tinyPathArray[tinyPathCount++].net, &mainNode.nodeNum.net, 4);
  }

  if (pathCount)
    writePathSeenBy(ECHO_PATH, msg->normPath, pathArray    , pathCount    );
  if (tinyPathCount)
    writePathSeenBy(ECHO_PATH, msg->tinyPath, tinyPathArray, tinyPathCount);
#ifdef _DEBUG_LOGPATHSEENBY
  logPathSeenBy("DEBUG 2F ", ECHO_PATH, pathArray   , pathCount    );
  logPathSeenBy("DEBUG 2T ", ECHO_PATH, inyPathArray, tinyPathCount);
#endif
}
//---------------------------------------------------------------------------
#ifdef FMAIL
extern char funcStr[];

void setViaStr(char *buf, const char *preStr, u16 aka)
{
  if (NULL != buf)
  {
#if defined(__WIN32__)
    SYSTEMTIME st;
    GetSystemTime(&st);
    sprintf(buf, "%s %s @%04u%02u%02u.%02u%02u%02u.%03u.UTC %s(%s) %s\r"
               , preStr, getAkaStr(aka, 1)
               , st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds
               , TOOLSTR, funcStr, Version()
           );
#ifdef _DEBUG
    {
      tempStrType tStr;
      sprintf( tStr, "DEBUG setViaStr: %04u%02u%02u.%02u%02u%02u.%03u.UTC"
             , st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
      logEntry(tStr, LOG_DEBUG, 0);
    }
#endif
#elif defined(linux)
    struct timeval tv;
    struct tm *tm;

    if (  0 != gettimeofday(&tv, NULL)
       || NULL == (tm = gmtime(&tv.tv_sec))
       )
      sprintf(buf, "%s %s @00000000.000000 %s(%s) %s\r"
                 , preStr, getAkaStr(aka, 1)
                 , TOOLSTR, funcStr, Version()
             );
    else
      sprintf(buf, "%s %s @%04u%02u%02u.%02u%02u%02u.%03u.UTC %s(%s) %s\r"
                 , preStr, getAkaStr(aka, 1)
                 , tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday
                 , tm->tm_hour, tm->tm_min, tm->tm_sec
                 , tv.tv_usec / 1000
                 , TOOLSTR, funcStr, Version()
             );
#else
    struct tm *tmPtr;
    tmPtr = gmtime(&startTime);
    sprintf(buf, "%s %s @%04u%02u%02u.%02u%02u%02u %s(%s) %s\r"
               , preStr, getAkaStr(aka, 1)
               , tmPtr->tm_year + 1900, tmPtr->tm_mon + 1, tmPtr->tm_mday
               , tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec
               , TOOLSTR, funcStr, Version()
           );
#endif
  }
}
//---------------------------------------------------------------------------
void addVia(char *msgText, u16 aka, int isNetmail)
{
  char *helpPtr;
  tempStrType tStr;

  if (isNetmail)
  {
    if ((helpPtr = strchr(msgText, 0)) != NULL)
      if (*(helpPtr - 1) != '\r' && (*(helpPtr - 1) != '\n' || *(helpPtr - 2) != '\r'))
        *(helpPtr++) = '\r';
  }
  else
    helpPtr = tStr;

  if (NULL != helpPtr)
  {
#ifdef _DEBUG
    {
      tempStrType tStrDB;
      sprintf(tStrDB, "DEBUG addVia: %s", isNetmail ? "netmail" : "echomail");
      logEntry(tStrDB, LOG_DEBUG, 0);
    }
#endif
    setViaStr(helpPtr, "\x1Via", aka);

    if (!isNetmail)
      insertLineN(msgText, tStr, 1);
  }
}
#endif
//---------------------------------------------------------------------------
// const char *makeName(char *path, char *name)
// {
//   static tempStrType tempStr;
//   char  *helpPtr;
//   udef   len;
//   u16    count;

//   helpPtr = stpcpy(tempStr, path);
//   strcpy(helpPtr, name);

//   if (fileSys(path))
//     return tempStr;

//   len = strlen(name);
//   name = helpPtr;
//   if (len > 8)
//   {
//     helpPtr[8] = 0;
//     len = 8;
//   }
//   while ((helpPtr = strchr(helpPtr, '.')) != NULL)
//   {
//     strcpy(helpPtr, helpPtr + 1);
//     len--;
//   }

//   helpPtr = strchr(tempStr, 0);
//   count = 0;
//   while (  count < echoCount
//         && (  echoAreaList[count].JAMdirPtr == NULL
//            || stricmp(tempStr, echoAreaList[count].JAMdirPtr) != 0
//            )
//         )
//     count++;

//   if (count < echoCount)
//   {
//     if ( len >= 8 )
//       --helpPtr;
//     if ( len >= 7 )
//       --helpPtr;

//     if (*helpPtr == '\\' || *(helpPtr + 1) == '\\')
//       return "";

//     *helpPtr = '`';
//     *(helpPtr + 1) = '0';
//     strcpy(helpPtr + 2, ".*");
//     do
//     {
//       if ((*(helpPtr + 1))++ == '9')
//         return "";
//     }
//     while (existPattern(path, name));
//     helpPtr += 2;
//   }
//   *helpPtr = 0;

//   return tempStr;
// }
//---------------------------------------------------------------------------
long fmseek(int handle, long offset, int fromwhere, int code)
{
  if (fromwhere == SEEK_SET && offset < 0)
  {
    tempStrType tempStr;

    sprintf(tempStr, "Illegal Seek operation, code %u", code);
    logEntry(tempStr, LOG_ALWAYS, 0);

    return -1;
  }
  return lseek(handle, offset, fromwhere);
}
//---------------------------------------------------------------------------
const char *makeFullPath(const char *deflt, const char *override, const char *name)
{
  static tempStrType tempStr;
  char  *helpPtr;

  helpPtr = stpcpy(tempStr, (override && *override) ? override : deflt);
  if (*(helpPtr - 1) != '\\')
    *helpPtr++ = '\\';
  strcpy(helpPtr, name);

  return tempStr;
}
//---------------------------------------------------------------------------
char *normalize_nd(char *name)
{
  static u16 index;
  static     tempStrType tempStr[2];
  char      *helpPtr;

  strcpy(tempStr[index], name);
  while ((helpPtr = strchr(tempStr[index], '/')) != NULL)
    *helpPtr = '\\';

  if (memcmp(tempStr[index], "\\\\", 2) == 0)
  {
    if (  (helpPtr = strchr(tempStr[index] + 2, '\\')) != NULL
       && (helpPtr = strchr(helpPtr + 1, '\\')) != NULL
       )
      strcpy(tempStr[index], helpPtr);
  }
  else
    if (tempStr[index][1] == ':')
      strcpy(tempStr[index], tempStr[index] + 2);

  return tempStr[index++];
}
//---------------------------------------------------------------------------
u16 getKludge(char *txt, const char *kludge, char *subfield, u16 bufsize)
{
  char *helpPtr1
     , *helpPtr2
     , *helpPtr3;

  if (!bufsize)
    return 0;

  if ((helpPtr1 = helpPtr3 = findCLStr(txt, kludge)) == NULL)
    return 0;

  helpPtr1 += strlen(kludge);
  helpPtr2 = subfield;
  while (--bufsize && *helpPtr1 && *helpPtr1 != '\r' && *helpPtr1 != '\n')
    *(helpPtr2++) = *(helpPtr1++);

  *helpPtr2 = 0;
  removeLine(helpPtr3);

  return 1;
}
//---------------------------------------------------------------------------
void getKludgeNode(char *txt, const char *kludge, nodeNumType *nodeNum)
{
  char *helpPtr;

  memset(nodeNum, 0, sizeof(nodeNumType));
  if ((helpPtr = findCLStr(txt, kludge)) != NULL)
  {
    helpPtr += strlen(kludge);
    if (sscanf(helpPtr, "%hu:%hu/%hu.%hu", &nodeNum->zone, &nodeNum->net, &nodeNum->node, &nodeNum->point) < 3)
      memset(nodeNum, 0, sizeof(nodeNumType));
  }
}
//---------------------------------------------------------------------------
char *addKludge(char *txt, const char *kludge, const char *kludgeValue)
{
  tempStrType tempStr;

  sprintf(tempStr, "\1%s %s\r", kludge, kludgeValue);

  return insertLine(txt, tempStr);
}
//---------------------------------------------------------------------------
void addINTL(internalMsgType *message)
{
  if (findCLStr(message->text, "\1INTL") == NULL)
    addINTLKludge(message, NULL);
}
//---------------------------------------------------------------------------
char *addINTLKludge(internalMsgType *message, char *insertPoint)
{
  tempStrType tempStr;

  sprintf(tempStr, "%u:%u/%u %u:%u/%u"
                 , message->destNode.zone, message->destNode.net, message->destNode.node
                 , message->srcNode.zone, message->srcNode.net, message->srcNode.node
         );
  return addKludge(insertPoint ? insertPoint : message->text, "INTL", tempStr);
}
//---------------------------------------------------------------------------
char *addMSGIDKludge(internalMsgType *message, char *insertPoint)
{
  tempStrType tempStr;

  sprintf(tempStr, "%s %08x", nodeStr(&message->srcNode), uniqueID());

  return addKludge(insertPoint ? insertPoint : message->text, "MSGID:", tempStr);
}
//---------------------------------------------------------------------------
char *addPIDKludge(char *txt)
{
  return addKludge(txt, "PID:", PIDStr());
}
//---------------------------------------------------------------------------
char *addPointKludges(internalMsgType *message, char *insertPoint)
{
  tempStrType tempStr;

  if (NULL == insertPoint)
    insertPoint = message->text;

  // TOPT kludge
  if (message->destNode.point != 0)
  {
    sprintf(tempStr, "%u", message->destNode.point);
    insertPoint = addKludge(insertPoint, "TOPT", tempStr);
  }

  // FMPT kludge
  if (message->srcNode.point != 0)
  {
    sprintf(tempStr, "%u", message->srcNode.point);
    insertPoint = addKludge(insertPoint, "FMPT", tempStr);
  }

  return insertPoint;
}
//---------------------------------------------------------------------------
const char *TZUTCStr(void)
{
#if defined(__WIN32__)
  LONG bias;
  TIME_ZONE_INFORMATION tzi;
  unsigned long ubias;
  static char s[10];

  // Get the timezone info.
  switch (GetTimeZoneInformation(&tzi))
  {
    case TIME_ZONE_ID_UNKNOWN:
      bias = tzi.Bias;
      break;
    case TIME_ZONE_ID_STANDARD:
      bias = tzi.Bias + tzi.StandardBias;
      break;
    case TIME_ZONE_ID_DAYLIGHT:
      bias = tzi.Bias + tzi.DaylightBias;
      break;
    default:
      *s = 0;
      return NULL;
  }

  ubias = abs(bias);
  sprintf(s, "%s%02lu%02lu", bias <= 0 ? "" : "-", ubias / 60, ubias % 60);

  return s;
#else
  return NULL;
#endif
}
//---------------------------------------------------------------------------
char *addTZUTCKludge(char *txt)
{
  const char *tzutcStr;

  if (NULL != (tzutcStr = TZUTCStr()))
    return addKludge(txt, "TZUTC:", tzutcStr);

  return txt;
}
//---------------------------------------------------------------------------
void setCurDateMsg(internalMsgType *msg)
{
  time_t     ti;
  struct tm *tm;

  *msg->dateStr = 0;

  time(&ti);
  tm = localtime(&ti);

  msg->year    = tm->tm_year + 1900;
  msg->month   = tm->tm_mon + 1;
  msg->day     = tm->tm_mday;
  msg->hours   = tm->tm_hour;
  msg->minutes = tm->tm_min;
  msg->seconds = tm->tm_sec;
}
//---------------------------------------------------------------------------
static tempStrType searchString;
static char       *searchPos;

u16 nextFile(char **filename)
{
  char *ep;

  if (searchPos == NULL)
    return 0;
  do
  {
    if (*searchPos == 0)
       return 0;
  } while (*searchPos == ' ');
  if (*searchPos == '\"')
  {
    ++searchPos;
    if ((ep = strchr(searchPos, '\"')) == NULL)
    {
      *filename = searchPos;
      searchPos = NULL;
      return 1;
    }
    *ep = 0;
    *filename = searchPos;
    searchPos = ep + 1;
    return 1;
  }
  if ((ep = strchr(searchPos, ' ')) == NULL)
  {
    *filename = searchPos;
    searchPos = NULL;
    return 1;
  }
  *ep = 0;
  *filename = searchPos;
  searchPos = ep + 1;

  return 1;
}
//---------------------------------------------------------------------------
u16 firstFile(char *string, char **filename)
{
  if (string == NULL)
  {
    searchPos = NULL;
    return 0;
  }
  strncpy(searchString, string, sizeof(searchString) - 1);
  searchPos = searchString;

  return nextFile(filename);
}
//---------------------------------------------------------------------------
