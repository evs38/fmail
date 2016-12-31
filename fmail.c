//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2016  Wilfred van Velzen
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

#include <conio.h>
#include <ctype.h>
#include <dir.h>      // getcwd()
#include <dirent.h>   // opendir()
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef __WIN32__
#include <windows.h>
#endif

#include "fmail.h"

#include "archive.h"
#include "areafix.h"
#include "areainfo.h"
#include "bclfun.h"
#include "cfgfile.h"
#include "config.h"
#include "dups.h"
#include "ftscprod.h"
#include "jamfun.h"
#include "jammaint.h"
#include "log.h"
#include "msgmsg.h"
#include "msgpkt.h"
#include "msgra.h"
#include "nodeinfo.h"
#include "pack.h"
#include "ping.h"
#include "pp_date.h"
#include "sorthb.h"
#include "spec.h"
#include "stpcpy.h"
#include "utils.h"
#include "version.h"

//---------------------------------------------------------------------------
#ifdef __WIN32__
#include "sendsmtp.h"
#define dSENDMAIL
#endif // __WIN32__

fhandle fmailLockHandle;

u16 status;

badEchoType  badEchos[MAX_BAD_ECHOS];
u16     badEchoCount = 0;
#ifdef __FMAILX__
char programPath[128];
#endif
char configPath[FILENAME_MAX];
s16  _mb_upderr = 0;

#ifndef __32BIT__
#ifdef __BORLANDC__
u16      cdecl _openfd[TABLE_SIZE] = {0x6001, 0x6002, 0x6002, 0xa004,
                                      0xa002, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000
                                     };
#else
u16      cdecl _openfd[TABLE_SIZE] = {0x2001, 0x2002, 0x2002, 0xa004,
                                      0xa002, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff
                                     };
#endif
#endif

#if defined __WIN32__ && !defined __DPMI32__
const char *smtpID;
#endif

globVarsType globVars;

const char *dayName[7]   = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char *months       = "JanFebMarAprMayJunJulAugSepOctNovDec";
const char *semaphore[6] = {
                             "fdrescan.now"
                           , "ierescan.now"
                           , "dbridge.rsn"
                           , ""
                           , "mdrescan.now"
                           , "xmrescan.flg"
                           };

long       gmtOffset;
time_t     startTime;
struct tm  timeBlock;

internalMsgType *message   = NULL;
s16              diskError = 0;
s16              mailBomb  = 0;

//---------------------------------------------------------------------------
void About(void)
{
  const char *str =
              "About FMail:\n"
              "\n"
              "    Version          : %s\n"
              "    Operating system : "
#if   defined(__WIN32__)
              "Win32\n"
#elif defined(__linux__)
              "Linux\n"
#else
              "Unknown\n"
#endif
#if 0
               "    Processor        : "
#ifdef __PENTIUMPRO__
               "PentiumPro\n"
#elif __PENTIUM__
               "Pentium\n"
#elif defined __486__
               "i486 and up\n"
#elif defined P386
               "80386 and up\n"
#else
               "8088/8086 and up\n"
#endif
#endif // 0
               "    Compiled on      : %04u-%02u-%02u\n"
               "    Message bases    : JAM and "MBNAME"\n"
               "    Max. areas       : %u\n"
               "    Max. nodes       : %u\n";
  printf(str, VersionStr(), YEAR, MONTH + 1, DAY, MAX_AREAS, MAX_NODES);

  exit(0);
}
//---------------------------------------------------------------------------
u16 foundToUserName(const char *toUserName)
{
  return *toUserName &&
         (  stricmp(toUserName, config.users[0].userName) == 0
         || stricmp(toUserName, config.users[1].userName) == 0
         || stricmp(toUserName, config.users[2].userName) == 0
         || stricmp(toUserName, config.users[3].userName) == 0
         || stricmp(toUserName, config.users[4].userName) == 0
         || stricmp(toUserName, config.users[5].userName) == 0
         || stricmp(toUserName, config.users[6].userName) == 0
         || stricmp(toUserName, config.users[7].userName) == 0
         );
}
//---------------------------------------------------------------------------
u16 getAreaCode(char *msgText)
{
  areaNameType echoName;
  char        *helpPtr;
  u16          low
             , mid
             , high;
  int          r;
  tempStrType  tempStr;

  if (strncmp(msgText, "AREA:", 5) != 0 && strncmp(msgText, "\1AREA:", 6) != 0)
    return NETMSG;

  if (*msgText == 1)
    strcpy(msgText, msgText + 1);

  helpPtr = msgText + 5;
  while (*helpPtr == ' ')
    helpPtr++;

  strncpy(echoName, helpPtr, sizeof(areaNameType) - 1);
  echoName[sizeof(areaNameType) - 1] = 0;
  if ((helpPtr = strchr(echoName, '\r')) != NULL)
    *helpPtr = 0;
  if ((helpPtr = strchr(echoName, ' ')) != NULL)
    *helpPtr = 0;

  if (*echoName == 0)
    logEntry("Message has no valid area tag", LOG_ALWAYS, 0);
  else
  {
    low = 0;
    high = echoCount;
    while (low < high)
    {
      mid = (low + high) / 2;
      if (!(r = stricmp(echoName, echoAreaList[mid].areaName)))
      {
        low = mid;
        break;
      }
      if (r > 0)
        low = mid + 1;
      else
        high = mid;
    }

    if (low >= echoCount || stricmp(echoName, echoAreaList[low].areaName) != 0)
    {
      low = 0;
      while (low < echoCount && stricmp(echoName, echoAreaList[low].oldAreaName) != 0)
        low++;

      if (low < echoCount)
      {
        removeLine(msgText);
        sprintf(tempStr, "AREA:%s\r", strcpy(echoName, echoAreaList[low].areaName));
        insertLine(msgText, tempStr);
      }
    }
    if (low < echoCount && stricmp(echoName, echoAreaList[low].areaName) == 0)
    {
      if (echoAreaList[low].options.active && !echoAreaList[low].options.local)
        return low;

      if (echoAreaList[low].options.local)
        logEntryf(LOG_ALWAYS, 0, "Area is local: '%s'", echoName);
      else
        logEntryf(LOG_ALWAYS, 0, "Area is not active: '%s'", echoName);
    }
    else
    {
      logEntryf(LOG_ALWAYS, 0, "Unknown area: '%s'", echoName);

      if (badEchoCount < MAX_BAD_ECHOS)
      {
        low = 0;
        while (low < badEchoCount && stricmp(badEchos[low].badEchoName, echoName))
          low++;

        if (low == badEchoCount)
        {
          strcpy(badEchos[badEchoCount].badEchoName, echoName);
          badEchos[badEchoCount].srcNode   = globVars.packetSrcNode;
          badEchos[badEchoCount++].destAka = globVars.packetDestAka;
        }
      }
    }
  }
  return BADMSG;
}
//---------------------------------------------------------------------------
const char *badTimeStr(void)
{
  static char tStr[32];
#ifdef __WIN32__
  SYSTEMTIME st;

  GetLocalTime(&st);
  sprintf(tStr, "%04u-%02u-%02u %02u:%02u:%02u"
              , st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond
         );
#else // __WIN32__
  time_t      timer;
  struct tm  *tm;

  time(&timer);
  tm = localtime(&timer);
  if (tm != NULL)
    sprintf(tStr, "%04u-%02u-%02u %02u:%02u:%02u"
                , tm->tm_year + 1900 , tm->tm_mon + 1 , tm->tm_mday
                , tm->tm_hour, tm->tm_min, tm->tm_sec
           );
  else
    strcpy(tStr, "1970-01-01 00:00:00");
#endif // __WIN32__

  return tStr;
}
//---------------------------------------------------------------------------
void tossBad(internalMsgType *message, const char *badStr)
{
  tempStrType tempStr;

  sprintf(tempStr, "\1FMAIL SRC: %s\r"
                   "\1FMAIL DEST: %s\r", nodeStr(&globVars.packetSrcNode), nodeStr(&globVars.packetDestNode));
  insertLineN(message->text, tempStr, 1);

  if (NULL != badStr)
  {
    sprintf(tempStr, "\1FMAIL BAD: %s (%s)\r", badStr, badTimeStr());
    insertLineN(message->text, tempStr, 1);
  }

  if (writeBBS(message, config.badBoard, 1))
    diskError = DERR_WRHBAD;

  globVars.badCount++;
}
//---------------------------------------------------------------------------
void unlinkOrBackup(const char *path)
{
  int doUnlink = 0;

  if (path == NULL || *path == 0)
    return;

  if (*config.inBakPath)
  {
    tempStrType bakPath;
    const char *helpPtr;
    char *p;
    int count = 0;

    if ((helpPtr = strrchr(path, '\\')) == NULL)
      helpPtr = path;
    else
      helpPtr++;

    p = stpcpy(stpcpy(bakPath, config.inBakPath), helpPtr);

    while (access(bakPath, 0) == 0 && count < 100)
      sprintf(p, "~%02d", count++);

    if (count < 100 && moveFile(path, bakPath) == 0)
      logEntryf(LOG_INBOUND | LOG_PACK | LOG_PKTINFO, 0, "Moved to backup: %s -> %s", path, bakPath);
    else
    {
      logEntryf(LOG_INBOUND | LOG_PACK | LOG_PKTINFO, 0, "Backup Failed! %s -> %s", path, bakPath);
      // try renaming in the same dir
      strcpy(stpcpy(bakPath, path), ".backup_failed");
      if (rename(path, bakPath) == 0)
        logEntryf(LOG_INBOUND | LOG_PACK | LOG_PKTINFO, 0, "Renamed: %s -> %s", path, bakPath);
      else
      {
        logEntryf(LOG_INBOUND | LOG_PACK | LOG_PKTINFO, 0, "Rename Failed! %s -> %s", path, bakPath);
        doUnlink = 1;
      }
    }
  }
  else
    doUnlink = 1;

  if (doUnlink)
  {
    if (unlink(path) == 0)
      logEntryf(LOG_INBOUND | LOG_PACK | LOG_PKTINFO, 0, "Deleted: %s", path);
    else
      logEntryf(LOG_INBOUND | LOG_PACK | LOG_PKTINFO, 0, "Delete Failed! %s", path);
  }
}
//---------------------------------------------------------------------------
time_t oldMsgTime = 0;

static s16 processPkt(u16 secure, s16 noAreaFix)
{
  tempStrType        pktStr
                   , tempStr;
  char              *helpPtr;
  char              *dirStr;
  echoToNodeType     tempEchoToNode;
  s16                echoToNodeCount
                   , areaIndex
                   , areaFixRep;
  int                i
                   , localAkaNum = -1;
  u16                pktMsgCount;
  DIR               *dir;
  struct dirent     *ent;
  char               prodCodeStr[20];

  if (secure && *config.securePath == 0)
    secure = 0;

  do
  {
    dirStr = secure ? config.securePath : config.inPath;
#ifdef _DEBUG
    {
      tempStrType cwd;
      getcwd(cwd, dTEMPSTRLEN);
      logEntryf(LOG_DEBUG, 0, "DEBUG secure:%d, cwd:%s, scanning:%s", secure, cwd, dirStr);
    }
#endif

    if ((dir = opendir(dirStr)) != NULL)
    {
      while ((ent = readdir(dir)) != NULL && !diskError && !mailBomb)
      {
        if (!match_spec("*.pkt", ent->d_name))
          continue;

        strcpy(stpcpy(pktStr, dirStr), ent->d_name);

        if (access(pktStr, 06) != 0)  // Check for read and write access
        {
          logEntryf(LOG_ALWAYS, 2, "Not sufficient access rights on: %s (\"%s\")", pktStr, strError(errno));
          continue;
        }

        newLine();

        if (0 == openPktRd(pktStr, secure))
        {
          helpPtr = NULL;
          i = 0;
          while (ftscprod[i].prodcode != 0xFFFF)
          {
            if (ftscprod[i].prodcode == globVars.remoteProdCode)
            {
              helpPtr = (char*)ftscprod[i].prodname;
              break;
            }
            ++i;
          }
          if (helpPtr == NULL)
          {
            sprintf(prodCodeStr, "Program id %04hX", globVars.remoteProdCode);
            helpPtr = prodCodeStr;
          }
          logEntryf(LOG_PKTINFO, 0, "Processing %s from %s to %s", ent->d_name, nodeStr(&globVars.packetSrcNode), nodeStr(&globVars.packetDestNode));

          if ((globVars.remoteCapability & 1) == 1)
            sprintf( tempStr, "Pkt info: %s %u.%02u, Type 2+, %d, %04u-%02u-%02u %02u:%02u:%02u%s%s"
                   , helpPtr, globVars.versionHi, globVars.versionLo
                   , globVars.packetSize
                   , globVars.year, globVars.month, globVars.day
                   , globVars.hour, globVars.min, globVars.sec
                   , globVars.password ? ", Pwd" : "", (globVars.password == 2) ? ", Sec" : ""
                   );
          else
            sprintf( tempStr, "Pkt info: %s, Type %s, %d, %04u-%02u-%02u %02u:%02u:%02u%s%s"
                   , helpPtr
                   , globVars.remoteCapability == 0xffff ? "2.2" :"2.0"
                   , globVars.packetSize
                   , globVars.year, globVars.month, globVars.day
                   , globVars.hour, globVars.min, globVars.sec
                   , globVars.password ? ", Pwd" : "", (globVars.password == 2) ? ", Sec" : ""
                   );
          logEntry(tempStr, LOG_XPKTINFO, 0);

          pktMsgCount = 0;

          while (!readPkt(message) && !diskError && !mailBomb)
          {
            printf("\rPkt message %d "dARROW" ", ++pktMsgCount);

            areaIndex = getAreaCode(message->text);

            // Personal Mail Check

            if (areaIndex == NETMSG) // for getLocalAka, moved here from case NETMSG statement
            {
              make4d(message);
              localAkaNum = getLocalAka(&message->destNode);
              addVia(message->text, globVars.packetDestAka, 1);  // Add Toss Via here before any writeMsg()
            }

            if (*config.topic1 || *config.topic2)
            {
              strcpy(tempStr, message->subject);
              strupr(tempStr);
            }
            if (  (  *config.pmailPath
                  && (  areaIndex != NETMSG
                     || (  config.mailOptions.persNetmail
                        && localAkaNum >= 0
                        )
                     )
                  )
               && (  stricmp(message->toUserName, config.sysopName) == 0
                  || (  *config.topic1
                     && strstr(tempStr, config.topic1) != NULL
                     )
                  || (  *config.topic2
                     && strstr(tempStr, config.topic2) != NULL
                     )
                  || foundToUserName(message->toUserName)
                  )
               )
            {
              if (areaIndex == NETMSG)
              {
                sprintf(tempStr, "AREA: Netmail  SOURCE: %s\r", nodeStr(&message->srcNode));
                insertLine(message->text, tempStr);
              }
              if (areaIndex == BADMSG)
                insertLine(message->text, "AREA: Bad Messages\r");

              writeMsg(message, PERMSG, 0);
              if (areaIndex == NETMSG || areaIndex == BADMSG)
                removeLine(message->text);
            }

            switch (areaIndex)
            {
              case NETMSG:
                putStr("NETMAIL");

                message->attribute &= PRIVATE     | FILE_ATT |
                                      RET_REC_REQ |
                                      IS_RET_REC  | AUDIT_REQ;

                areaFixRep = 0;
                // Check for messages to AreaFix in pkt
                if (!noAreaFix && localAkaNum >= 0 && toAreaFix(message->toUserName))
                {
                  if (  config.mgrOptions.keepRequest
                     || findCLiStr(message->text, "%NOTE") != NULL
                     )
                  {
                    message->attribute |= RECEIVED;
                    writeMsg(message, NETMSG, 1);
                  }
                  putchar(' ');
                  areaFixRep = !areaFix(message);
                }

                // Check for messages to PING in pkt
                if (  (  (!config.pingOptions.disablePing  && localAkaNum >= 0)
                      || (!config.pingOptions.disableTrace && localAkaNum <  0)
                      )
                   && toPing(message->toUserName)
                   )
                {
                  putchar(' ');
                  areaFixRep = !Ping(message, localAkaNum) && localAkaNum >= 0;
                  if (areaFixRep && !config.pingOptions.deletePingRequests)
                  {
                    message->attribute |= RECEIVED;
                    writeMsg(message, NETMSG, 1);
                  }
                }

                if (!areaFixRep)
                {
                  // re-route to point
                  if (localAkaNum >= 0)
                  {
                    for (i = 0; i < nodeCount; i++)
                    {
                      if (  nodeInfo[i]->options.routeToPoint
                         && isLocalPoint(&(nodeInfo[i]->node))
                         && memcmp(&(nodeInfo[i]->node), &message->destNode, 6) == 0
                         && stricmp(nodeInfo[i]->sysopName, message->toUserName) == 0
                         )
                      {
                        sprintf(tempStr,"\1TOPT %u\r", message->destNode.point = nodeInfo[i]->node.point);
                        insertLine(message->text, tempStr);
                        localAkaNum = getLocalAka(&message->destNode);
                        break;
                      }
                    }
                  }
                  if (localAkaNum >= 0)
                  {
                    if (  stricmp(message->toUserName, "SysOp") == 0
                       || stricmp(message->toUserName, config.sysopName) == 0
                       )
                    {
                      strcpy(message->toUserName, config.sysopName);
                      globVars.perNetCount++;
                    }
                  }
                  else
                  {
                    if (secure && getLocalAka(&message->srcNode) >= 0)
                    {
                      putStr(" (local secure)");
                      message->attribute |= LOCAL | KILLSENT;
                    }
                    else
                    {
                      putStr(" (in transit)");
                      message->attribute |= IN_TRANSIT | KILLSENT;
                    }
                    if (  !(getNodeInfo(&message->destNode)->capability & PKT_TYPE_2PLUS)
                       && isLocalPoint(&message->destNode)
                       )
                      make2d(message);
                  }
                  if (  (message->attribute & FILE_ATT) && (message->attribute & IN_TRANSIT)
                     && (strchr(message->subject, '*') != 0 || strchr(message->subject, '?') != 0))
                  {
                    insertLine(message->text, "\1FLAGS LOK\r");
                    writeMsg(message, NETMSG, 2);
                  }
                  else
                    writeMsg(message, NETMSG, 0);
                }
                break;

              case BADMSG:
                putStr(dARROW" Bad message");
                tossBad(message, "Area not in configuration");
                break;

              default:
                putStr(echoAreaList[areaIndex].areaName);

                // Security checks
                if (!secure)
                {
                  i = 0;
                  while (  i < forwNodeCount
                        && (  !ETN_WRITEACCESS(echoToNode[areaIndex][ETN_INDEX(i)], i)
                           || memcmp(&(nodeFileInfo[i]->destNode4d.net), &(globVars.packetSrcNode.net), sizeof(nodeNumType)-2) != 0
                           )
                        )
                    i++;

                  if ((i == forwNodeCount &&
                       echoAreaList[areaIndex].options.security) ||
                      (i < forwNodeCount &&
                       echoAreaList[areaIndex].writeLevel >
                       nodeFileInfo[i]->nodePtr->writeLevel))
                  {
                    if (i == forwNodeCount)
                      sprintf(tempStr, "%s is not connected to this area", nodeStr(&globVars.packetSrcNode));
                    else
                      sprintf(tempStr, "%s has no write access to area", nodeStr(&nodeFileInfo[i]->destNode4d));

                    for (i = 0; i < forwNodeCount; i++)
                      if (memcmp(&nodeFileInfo[i]->destNode4d.net, &globVars.packetSrcNode.net, 6) == 0)
                      {
                        nodeFileInfo[i]->fromNodeSec++;
                        i = -1;
                        break;
                      }

                    if (i != -1)
                      ++globVars.fromNoExpSec;

                    putStr(" Security violation "dARROW" Bad message");
                    tossBad(message, tempStr);
                    break;
                  }
                }

                if (oldMsgTime)  // Only check for old messages if set
                {
                  struct tm tm;
                  time_t msgTime;

                  // Calculate message time
                  tm.tm_year  = message->year  - 1900;
                  tm.tm_mon   = message->month - 1;
                  tm.tm_mday  = message->day;
                  tm.tm_hour  = message->hours;
                  tm.tm_min   = message->minutes;
                  tm.tm_sec   = message->seconds;
                  tm.tm_isdst = -1;
                  msgTime = mktime(&tm);  // todo: Possibly need to add gmtOffset!

                  if (msgTime < oldMsgTime)
                  {
                    putStr(" "dARROW" Old message");
                    tossBad(message, "Message too old");
                    break;
                  }
                }

                if (checkDup(message, echoAreaList[areaIndex].areaNameCRC))
                {
                  for (i = 0; i < forwNodeCount; i++)
                    if (memcmp(&nodeFileInfo[i]->destNode4d.net, &globVars.packetSrcNode.net, 6) == 0)
                    {
                      nodeFileInfo[i]->fromNodeDup++;
                      i = -1;
                      break;
                    }

                  if (i != -1)
                    ++globVars.fromNoExpDup;

                  putStr(" "dARROW" Duplicate message");
                  addVia(message->text, globVars.packetDestAka, 0);
                  if (writeBBS(message, config.dupBoard, 1))
                    diskError = DERR_WRHDUP;

                  echoAreaList[areaIndex].dupCount++;
                  globVars.dupCount++;
                  break;
                }

                echoAreaList[areaIndex].msgCount++;
                globVars.echoCount++;

                if (echoAreaList[areaIndex].options.allowPrivate)
                  message->attribute &= PRIVATE;
                else
                  message->attribute = 0;

                memcpy(tempEchoToNode, echoToNode[areaIndex], sizeof(echoToNodeType));
                echoToNodeCount = echoAreaList[areaIndex].echoToNodeCount;

                for (i = 0; i < forwNodeCount; i++)
                {
                  if (memcmp(&nodeFileInfo[i]->destNode4d.net, &globVars.packetSrcNode.net, 6) == 0)
                  {
                    if (ETN_ANYACCESS(tempEchoToNode[ETN_INDEX(i)], i))
                    {
                      tempEchoToNode[ETN_INDEX(i)] &= ETN_RESET(i);
                      echoToNodeCount--;
                    }
                  }
                }
                for (i = 0; i < forwNodeCount; i++)
                {
                  if (memcmp(&nodeFileInfo[i]->destNode4d.net, &globVars.packetSrcNode.net, 6) == 0)
                  {
                    nodeFileInfo[i]->fromNodeMsg++;
                    i = -1;
                    break;
                  }
                }
                if (i != -1)
                  ++globVars.fromNoExpMsg;

                if (echoToNodeCount)
                {
                  addPathSeenBy(message, tempEchoToNode, areaIndex, NULL);

                  if (writeEchoPkt(message, echoAreaList[areaIndex].options, tempEchoToNode))
                    diskError = DERR_WRPKTE;
                }
                else
                {
#ifdef _DEBUG
                  newLine();
                  logEntry("DEBUG processPkt: echoToNodeCount == 0 SEEN-BY & PATH processing", LOG_DEBUG, 0);
#endif
                  if (!echoAreaList[areaIndex].options.impSeenBy)
                  {
                    helpPtr = message->text;
                    while ((helpPtr = findCLStr(helpPtr, "SEEN-BY: ")) != NULL)
                      removeLine(helpPtr);

                    helpPtr = message->text;
                    while ((helpPtr = findCLStr(helpPtr, "\1PATH: ")) != NULL)
                      removeLine(helpPtr);
                  }
                  else
                    // If not a JAM area make the seen-by lines kludge lines
                    if (echoAreaList[areaIndex].JAMdirPtr == NULL)
                    {
                      helpPtr = message->text;
                      while ((helpPtr = findCLStr(helpPtr, "SEEN-BY: ")) != NULL)
                      {
                        memmove(helpPtr + 1, helpPtr, strlen(helpPtr) + 1);
                        *helpPtr = 1;
                        helpPtr += 10;
                      }
                    }
                }

                if (echoAreaList[areaIndex].JAMdirPtr == NULL)
                {
                  // Hudson toss
                  if (writeBBS(message, echoAreaList[areaIndex].board, echoAreaList[areaIndex].options.impSeenBy))
                    diskError = DERR_WRHECHO;
                }
                else
                {
                  // JAM toss
                  if (echoAreaList[areaIndex].options.impSeenBy)
                    strcpy(stpcpy(strchr(message->text, 0), message->normSeen), message->normPath);

                  // Skip AREA tag
                  if (strncmp(message->text, "AREA:", 5) == 0)
                    removeLine(message->text);

                  if (config.mbOptions.removeRe)
                    removeRe(message->subject);

                  // write here
                  if (!jam_writemsg(echoAreaList[areaIndex].JAMdirPtr, message, 0))
                  {
                    newLine();
                    logEntry("Can't write JAM message", LOG_ALWAYS, 0);
                    diskError = DERR_WRJECHO;
                    break;
                  }
                  globVars.jamCountV++;
                }
                break;
            }
            if (config.allowedNumNetmail != 0 && globVars.netCount >= config.allowedNumNetmail)
              mailBomb++;
          }
          closePktRd();

          freePktHandles();

          newLine();
          if (mailBomb)
          {
            strcpy(stpcpy(tempStr, pktStr), ".mailbomb");
            rename(pktStr, tempStr);
            unlinkOrBackup(pktStr);
            logEntryf(LOG_ALWAYS, 0, "Max # net msgs per packet exceeded in packet from %s", nodeStr(&globVars.packetSrcNode));
            logEntryf(LOG_ALWAYS, 0, "Packet %s has been renamed to %s", pktStr, tempStr);
          }
          else
          {
            if (!diskError && (validate1BBS() || validateEchoPktWr()) == 0)
            {
              validate2BBS(0);
              validateMsg();
              validateDups();
              unlinkOrBackup(pktStr);
              for (i = 0; i < echoCount; i++)
              {
                echoAreaList[i].msgCountV = echoAreaList[i].msgCount;
                echoAreaList[i].dupCountV = echoAreaList[i].dupCount;
              }
            }
            else
              if (!diskError)
                diskError = DERR_VAL;
          }
        }
      }
      if (!mailBomb)
        newLine();

      closedir(dir);
    }
  }
  while (!diskError && !mailBomb && secure--);  // do .. while

  jam_closeall();

  return diskError;
}  // processPkt()
//---------------------------------------------------------------------------
void Toss(int argc, char *argv[])
{
  s32            switches;
  tempStrType    tempStr;
  fhandle        tempHandle;
  s16            dayNum;
  struct bt     *bundlePtr
              , *bundlePtr2
              , *bundlePtr3
              ;
  DIR           *dir;
  struct dirent *ent;
  s16            count;
  s16            temp;

  if (argc >= 3 && (argv[2][0] == '?' || argv[2][1] == '?'))
  {
    puts("Usage:\n\n"
         "    FMail Toss [/A] [/B]\n\n"
         "Switches:\n\n"
         "    /A   Do not process AreaMgr requests\n"
         "    /B   Also scan bad message directory for valid echomail messages");
    exit(0);
  }

  status = 2;

  switches = getSwitch(&argc, argv, SW_A | SW_B);

  initFMail("Toss", switches);

  initPkt();
  initNodeInfo();
  initAreaInfo();

  initMsg((u16)(switches & SW_A));  // After initNodeInfo & initAreaInfo
  retryArc();                       // After initMsg
  ScanNewBCL();
  _mb_upderr = multiUpdate();

  initBBS();

  openDup();

  strcpy(stpcpy(tempStr, configPath), dBDEFNAME);
  if ((tempHandle = open(tempStr, O_RDONLY | O_BINARY)) != -1)
  {
    badEchoCount = read(tempHandle, badEchos, MAX_BAD_ECHOS * sizeof(badEchoType)) / sizeof(badEchoType);
    close(tempHandle);
  }

  if (switches & SW_B)
  {
    puts("Tossing messages from bad message board...\n");
    openBBSWr(1);
    moveBadBBS();
    closeBBSWr(1);
    validateDups();
  }

  openBBSWr(0);

  if (!diskError)
  {
    puts("Tossing messages...");

    if (config.oldMsgDays)
      // Set after initFMail() and before processPkt()
      oldMsgTime = startTime - config.oldMsgDays * 24 * 60 * 60;  // TODO check oldMsgTime usage

    diskError = processPkt(1, (u16)(switches & SW_A));

    bundlePtr = NULL;
    if ((dir = opendir(config.inPath)) != NULL)
    {
      while ((ent = readdir(dir)) != NULL && !diskError && !mailBomb)
      {
        for (dayNum = 0; dayNum < 7 && !diskError && !mailBomb; dayNum++)
        {
          struct stat st;
          tempStrType pattern;

          sprintf(pattern, "*.%.2s?", dayName[dayNum]);
          if (!match_spec(pattern, ent->d_name))
            continue;

          strcpy(stpcpy(tempStr, config.inPath), ent->d_name);

          if (  stat(tempStr, &st) != 0
             || (st.st_mode & (S_IWRITE | S_IREAD)) != (S_IWRITE | S_IREAD))
          {
            logEntryf(LOG_ALWAYS, 2, "Not sufficient rights on: %s", tempStr);
            continue;
          }

          bundlePtr3 = bundlePtr;
          bundlePtr2 = NULL;
          while (bundlePtr3 != NULL && (bundlePtr3->mtime < st.st_mtime))
          {
            bundlePtr2 = bundlePtr3;
            bundlePtr3 = bundlePtr3->nextb;
          }
          if ((bundlePtr3 = (struct bt*)malloc(sizeof(struct bt))) == NULL)
            logEntry("Not enough memory available", LOG_ALWAYS, 2);

          strcpy(bundlePtr3->fname, ent->d_name);
          bundlePtr3->mtime = st.st_mtime;
          bundlePtr3->size  = st.st_size;
          if (bundlePtr2 == NULL)
          {
            bundlePtr3->nextb = bundlePtr;
            bundlePtr = bundlePtr3;
          }
          else
          {
            bundlePtr3->nextb = bundlePtr2->nextb;
            bundlePtr2->nextb = bundlePtr3;
          }
        }
      }
      closedir(dir);
    }
    if (bundlePtr == NULL)
      newLine();
    else
    {
      do
      {
        newLine();
        unpackArc(bundlePtr);
        ScanNewBCL();
        diskError = processPkt(0, (u16)(switches & SW_A));
        bundlePtr2 = bundlePtr;
        bundlePtr = bundlePtr->nextb;
        free(bundlePtr2);
      }
      while (bundlePtr != NULL);
    }
    if (closeEchoPktWr())
      diskError = DERR_CLPKTE;
  }

  if (badEchoCount)
  {
    strcpy(stpcpy(tempStr, configPath), dBDEFNAME);
    if ((tempHandle = open(tempStr, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) != -1)
    {
      write(tempHandle, badEchos, badEchoCount * sizeof(badEchoType));
      close(tempHandle);
    }
  }

  if (config.mailOptions.warnNewMail && globVars.perCountV)
  {
    logEntry("Send warning for new mail", LOG_DEBUG, 0);

    strcpy(message->toUserName,   config.sysopName);
    strcpy(message->fromUserName, "FMail");
    strcpy(message->subject,      "New messages");
    message->srcNode   = message->destNode = config.akaList[0].nodeNum;
    message->attribute = PRIVATE;

    strcpy(stpcpy(message->text, "New personal netmail and/or echomail messages have arrived on your system!\r\r"), TearlineStr());
    writeMsgLocal(message, NETMSG, 1);
  }

  closeBBSWr(0);
#ifdef dSENDMAIL
  sendmail_smtp();
#endif
  closeNodeInfo();
  closeDup();
  deInitPkt();

  if (globVars.echoCountV || globVars.dupCountV || globVars.badCountV)
  {
    if (*config.tossedAreasList && (tempHandle = open(config.tossedAreasList, O_WRONLY | O_CREAT | O_APPEND | O_TEXT, S_IREAD | S_IWRITE)) != -1)
      for (count = 0; count < echoCount; count++)
        if (echoAreaList[count].msgCountV)
          dprintf(tempHandle, "%s\n", echoAreaList[count].areaName);

    if (*config.summaryLogName && (tempHandle = open( config.summaryLogName, O_WRONLY | O_CREAT | O_APPEND | O_TEXT, S_IREAD | S_IWRITE)) != -1)
    {
      logEntry("Writing toss summary", LOG_DEBUG, 0);
      dprintf(tempHandle, "\n----------  %s %4u-%02u-%02u %02u:%02u:%02u, %s - Toss Summary\n\n"
                        , dayName[timeBlock.tm_wday]
                        , timeBlock.tm_year + 1900
                        , timeBlock.tm_mon + 1
                        , timeBlock.tm_mday
                        , timeBlock.tm_hour
                        , timeBlock.tm_min
                        , timeBlock.tm_sec
                        , VersionStr()
             );
      dprintf(tempHandle, "Board  Area name                                           #Msgs  Dupes\n");
      dprintf(tempHandle, "-----  --------------------------------------------------  -----  -----\n");
      temp = 0;
      if (globVars.badCountV)
      {
        dprintf(tempHandle, "%5u  Bad messages                                        %5u\n", config.badBoard, globVars.badCountV);
        temp++;
      }
      for (count = 0; count < echoCount; count++)
      {
        if (echoAreaList[count].msgCountV || echoAreaList[count].dupCountV)
        {
          if (echoAreaList[count].board)
            dprintf(tempHandle, "%5u  %-50s  %5u  %5u\n", echoAreaList[count].board, echoAreaList[count].areaName, echoAreaList[count].msgCountV, echoAreaList[count].dupCountV);
          else
            dprintf(tempHandle, "%5s  %-50s  %5u  %5u\n", (echoAreaList[count].JAMdirPtr == NULL) ? "None" : "JAM", echoAreaList[count].areaName, echoAreaList[count].msgCountV, echoAreaList[count].dupCountV);
          temp++;
        }
      }
      dprintf(tempHandle, "-----  --------------------------------------------------  -----  -----\n");
      dprintf(tempHandle, "%5u                                                      %5u  %5u\n", temp, globVars.echoCountV+globVars.badCountV, globVars.dupCountV);
      temp = 0;
      for (count = 0; count < forwNodeCount; count++)
      {
        if (  nodeFileInfo[count]->fromNodeSec
           || nodeFileInfo[count]->fromNodeDup
           || nodeFileInfo[count]->fromNodeMsg)
        {
          if (!temp)
          {
            dprintf(tempHandle, "\nReceived from node        #Msgs  Dupes  Sec V\n"
                                  "------------------------  -----  -----  -----\n");
            temp++;
          }
          dprintf(tempHandle, "%-24s  %5u  %5u  %5u\n"
                            , nodeStr(&nodeFileInfo[count]->destNode4d)
                            , nodeFileInfo[count]->fromNodeMsg
                            , nodeFileInfo[count]->fromNodeDup
                            , nodeFileInfo[count]->fromNodeSec);
        }
      }
      if (globVars.fromNoExpMsg || globVars.fromNoExpDup || globVars.fromNoExpSec)
      {
        if (!temp)
          dprintf(tempHandle, "\nReceived from node        #Msgs  Dupes  Sec V\n"
                                "------------------------  -----  -----  -----\n");

        dprintf(tempHandle, "Not in export list        %5u  %5u  %5u\n"
                          , globVars.fromNoExpMsg
                          , globVars.fromNoExpDup
                          , globVars.fromNoExpSec);
      }
      temp = 0;
      for (count = 0; count < forwNodeCount; count++)
      {
        if (nodeFileInfo[count]->totalMsgs)
        {
          if (!temp)
          {
            dprintf(tempHandle, "\nSent to node              #Msgs\n"
                                  "------------------------  -----\n");
            temp++;
          }
          dprintf(tempHandle, "%-24s  %5u\n", nodeStr(&nodeFileInfo[count]->destNode4d), nodeFileInfo[count]->totalMsgs);
        }
      }
      write(tempHandle, "\n", 1);
      close(tempHandle);
    }
  }

  if (config.mbOptions.updateChains && globVars.jamCountV)
  {
    // Update reply chains
    headerType  *areaHeader;
    rawEchoType *areaBuf;
    u16          count2;

    logEntry("Updating reply chains: Start", LOG_DEBUG, 0);

    if (openConfig(CFG_ECHOAREAS, &areaHeader, (void**)&areaBuf))
    {
      for (count = 0; count < areaHeader->totalRecords; count++)
      {
        getRec(CFG_ECHOAREAS, count);
        if (*areaBuf->msgBasePath)
        {
          count2 = 0;
          while (  count2 < echoCount
                && (echoAreaList[count2].JAMdirPtr == NULL || echoAreaList[count2].msgCountV == 0
                   || strcmp(areaBuf->msgBasePath, echoAreaList[count2].JAMdirPtr) != 0
                   )
                )
            count2++;

          if (count2 < echoCount)
          {
            s32 space = 0;
            // SW_R/SW_* as second argument for testing/debuging the function
            if (!JAMmaint(areaBuf, 0, config.sysopName, &space))
            {
              areaBuf->stat.tossedTo = 0;
              putRec(CFG_ECHOAREAS, count);
            }
          }
        }
      }
      closeConfig(CFG_ECHOAREAS);
    }
    logEntry("Updating reply chains: End", LOG_DEBUG, 0);
  }

  deInitAreaInfo();

  // See also end of PACK block

  temp = 0;
  if ((config.mbOptions.sortNew || config.mbOptions.updateChains) && globVars.mbCountV != 0 && !mailBomb)
  {
    if (mbSharingInternal)
    {
      temp = config.mbOptions.updateChains;
      config.mbOptions.updateChains = 0;
    }
    if (config.mbOptions.updateChains || config.mbOptions.sortNew)
      sortBBS(globVars.origTotalMsgBBS, 0);

    newLine();
  }

  if (!(_mb_upderr = multiUpdate()) && temp)
  {
    mbSharingInternal = 0;
    config.mbOptions.updateChains = 1;
    config.mbOptions.sortNew = 0;
    sortBBS(globVars.origTotalMsgBBS, 1);
    newLine();
  }
}
//---------------------------------------------------------------------------
s16 handleScan(internalMsgType *message, u16 boardNum, u16 boardIndex)
{
  u16            count;
  echoToNodeType tempEchoToNode;
  tempStrType    tempStr;
  char          *helpPtr1,
                *helpPtr2;
  char           tearline[80] = "--- \r";

  switch (config.tearType)
  {
    case 1:
      strcpy(stpcpy(tearline + 4, config.tearLine), "\r");
      break;
    case 2:
    case 3:
      break;
    case 4:
    case 5:
      *tearline = 0;
      break;
    case 6:
    default:
      strcpy(tearline, TearlineStr());
      break;
  }

  if (boardNum && isNetmailBoard(boardNum))  // If JAM netmail area
  {
    message->attribute |= LOCAL |
                          (config.mailOptions.keepExpNetmail?0:KILLSENT) |
                          ((getFlags(message->text)&FL_FILE_REQ)?FILE_REQ:0);

    point4d(message);

    // MSGID kludge

    if (findCLStr(message->text, "\1MSGID: ") == NULL)
    {
      sprintf(tempStr, "\1MSGID: %s %08x\r", nodeStr (&message->srcNode), uniqueID());
      insertLine(message->text, tempStr);
    }

    if (message->srcNode.zone != message->destNode.zone)
      addINTL(message);

    if (writeMsg(message, NETMSG, 0) == -1)
      return 1;

    logEntryf(LOG_NETEXP, 0, "Netmail message for %s moved to netmail directory", nodeStr(&message->destNode));

    if (updateCurrHdrBBS(message))
      return 1;

    validateMsg();
    globVars.nmbCountV++;
    return 0;
  }

  if (boardNum)
  { // Hudson based messages
    for (count = 0; count < echoCount && echoAreaList[count].board != boardNum; count++)
      ;

    if (count == echoCount)
    {
      logEntryf(LOG_ALWAYS, 0, "Found message in unknown message base board (#%u)", boardNum);

      if (updateCurrHdrBBS(message))
        return 1;

      return 0;
    }
  }
  else
    // JAM based message
    count = boardIndex;

  if (echoAreaList[count].options.active && !echoAreaList[count].options.local)
  {
    logEntryf(LOG_ECHOEXP, 0, "Echomail message, area %s", echoAreaList[count].areaName);

    if (echoAreaList[count].options.allowPrivate)
      message->attribute &= PRIVATE;
    else
      message->attribute = 0;

    memcpy(tempEchoToNode, echoToNode[count], sizeof(echoToNodeType));

    // Check origin

    if ((helpPtr1 = findCLStr(message->text, " * Origin:")) != NULL)
    {
      // Retear message with FMail tearline
      if (config.mbOptions.reTear)
      {
        helpPtr2 = helpPtr1;
        while (  helpPtr1 > message->text
              && (*--helpPtr1 == '\r' || *helpPtr1 == '\n')
              )
          ;
        while (  helpPtr1 > message->text
              && *(helpPtr1-1) != '\r' && *(helpPtr1-1) != '\n'
              )
          helpPtr1--;
        if ((strncmp(helpPtr1, "---", 3) == 0) && (helpPtr1[3] != '-'))
        {
          removeLine(helpPtr1);
          insertLine(helpPtr1, tearline);
        }
        else
          insertLine(helpPtr2, tearline);
      }
    }
    else
    {
      sprintf( strchr(message->text, 0), "%s * Origin: %s (%s)\r"
             , tearline
             , echoAreaList[count].originLine
             , nodeStr(&config.akaList[echoAreaList[count].address].nodeNum)
             );
    }
    if (config.tearType)
    {
      if ((helpPtr1 = findCLStr(message->text, "\1TID:")) != NULL)
        removeLine(helpPtr1);

      sprintf(tempStr, "\1TID: %s\r", TIDStr());
      insertLine(message->text, tempStr);
    }

    sprintf(tempStr, "AREA:%s\r", echoAreaList[count].areaName);
    insertLine(message->text, tempStr);

    addPathSeenBy(message, tempEchoToNode, count, NULL);

    if (writeEchoPkt(message, echoAreaList[count].options, tempEchoToNode))
      return 1;

    if ((boardNum && updateCurrHdrBBS(message)) || validateEchoPktWr())
      return 1;

    if (*config.sentEchoPath)
      writeMsg(message, SECHOMSG, 1);

    if (*config.topic1 || *config.topic2)
    {
      strcpy(tempStr, message->subject);
      strupr(tempStr);
    }
    if ( config.mbOptions.persSent &&
         *config.pmailPath &&
         (stricmp(message->fromUserName, config.sysopName) == 0 ||
          (*config.topic1 &&
           strstr(tempStr, config.topic1) != NULL) ||
          (*config.topic2 &&
           strstr(tempStr, config.topic2) != NULL) ||
          foundToUserName(message->fromUserName))
       )
      writeMsg(message, PERMSG, 1);

    checkDup(message, echoAreaList[count].areaNameCRC);
    validateDups();

    echoAreaList[count].msgCountV++;
    globVars.echoCountV++;
  }
  return 0;
}
//---------------------------------------------------------------------------
void Scan(int argc, char *argv[])
{
  s32            switches;
  s16            count;
  s16            boardNum;
  s16            diskErrorT;
  s16            infoBad;
  u16            scanCount = 0;
#ifndef GOLDBASE
  u16            index;
#else
  u32            index;
#endif
  tempStrType    tempStr;
  char          *helpPtr;
  fhandle        tempHandle;
  s32            msgNum;

  if (argc >= 3 && (argv[2][0] == '?' || argv[2][1] == '?'))
  {
    puts("Usage:\n\n"
         "    FMail Scan [/A] [/E] [/N] [/S]\n\n"
         "Switches:\n\n"
         "    /A   Do not process AreaMgr requests\n"
         "    /E   Scan for echomail messages only\n"
         "    /N   Scan for netmail messages only\n"
         "    /H   Scan "MBNAME" base only\n"
         "    /J   Scan JAM bases only\n"
         "    /S   Scan the entire message base (ignore EchoMail."MBEXTN"/JAM and NetMail."MBEXTN")");
    exit(0);
  }

  status = 1;

  switches = getSwitch(&argc, argv, SW_A | SW_E | SW_N | SW_H | SW_J | SW_S);

  initFMail("Scan", switches);

  initPkt();
  initNodeInfo();
  initAreaInfo();

  initMsg((u16)(switches & SW_A));  // After initNodeInfo
  retryArc();                       // After initMsg
  ChkAutoBCL();
  _mb_upderr = multiUpdate();

  initBBS();

  openDup();
  openBBSWr(1);

  if (!(switches & (SW_N | SW_H)))
  {
    for (count = 0; count < echoCount; count++)
      echoAreaList[count].options._reserved = 0;

    infoBad = 0;
    count = 0;
    while (count < echoCount && (  echoAreaList[count].JAMdirPtr == NULL
                                || (!echoAreaList[count].options.active) || echoAreaList[count].options.local))
      count++;

    if (count < echoCount)
    {
      puts("Scanning JAM message bases for outgoing messages...");
      if (!config.mbOptions.scanAlways && !(switches & SW_S))
      {
        strcpy(stpcpy(tempStr, config.bbsPath), dECHOMAIL_JAM);
        if ((tempHandle = open(tempStr, O_RDONLY | O_TEXT)) != -1)
        {
          memset(tempStr, 0, sizeof(tempStrType));
          if ((count = read(tempHandle, tempStr, sizeof(tempStrType)-1)) != -1)
          {
            tempStr[count] = 0;
            while (*tempStr)
            {
              if ((helpPtr = strchr(tempStr, ' ')) != NULL)
              {
                msgNum = atol(helpPtr);
                *(helpPtr++) = 0;
                count = 0;
                while (count < echoCount
                      && (  echoAreaList[count].JAMdirPtr == NULL
                         || stricmp(tempStr, echoAreaList[count].JAMdirPtr)
                         )
                      )
                  count++;

                if (count != echoCount && !echoAreaList[count].options._reserved)
                {
                  if ((msgNum = jam_scan(count, msgNum, 1, message)) == 0)
                  {
                    infoBad++;
                    echoAreaList[count].options._reserved = 1;
                  }
                  else
                  {
                    diskErrorT = 0;
                    if (  !echoAreaList[count].options.active
                       ||  echoAreaList[count].options.local
                       || !(diskErrorT = handleScan(message, 0, count))
                       )
                    {
                      removeLine(message->text);
                      jam_update(count, msgNum, message);
                      scanCount++;
                    }
                    if (diskErrorT != 0)
                      diskError = DERR_PRSCN;
                  }
                }
                if ((helpPtr = strchr(helpPtr, '\n')) != NULL)
                {
                  helpPtr = stpcpy(tempStr, helpPtr + 1);
                  count = (u16)(sizeof(tempStrType) + tempStr - helpPtr);
                  if ((count = read(tempHandle, helpPtr, count - 1)) == -1)
                    *tempStr = 0;
                  else
                    helpPtr[count] = 0;
                }
              }
              else
                *tempStr = 0;
            }
            close(tempHandle);
          }
        }
      }

      if (infoBad || config.mbOptions.scanAlways || (switches & SW_S))
      {
        for (count = 0; count < echoCount; count++)
        {
#ifdef _DEBUG0
          logEntryf(LOG_DEBUG, 0, "DEBUG JAM scan count: %d", count);
#endif // _DEBUG
          if (  (   echoAreaList[count].JAMdirPtr != NULL
                &&  echoAreaList[count].options.active
                && !echoAreaList[count].options.local
                )
             && (config.mbOptions.scanAlways || (switches & SW_S) || echoAreaList[count].options._reserved)
             )
          {
            if ((infoBad || config.mbOptions.scanAlways || (switches & SW_S)) && infoBad != -1)
            {
              if (config.mbOptions.scanAlways || (switches & SW_S))
                puts("Rescanning all JAM areas");
              else
                puts("Rescanning selected JAM areas\n");
              infoBad = -1;
            }
            logEntryf(LOG_ALWAYS, 0, "Scanning JAM area %d: %s", count, echoAreaList[count].areaName);
            msgNum = 0;
            while (!diskError && (msgNum = jam_scan(count, ++msgNum, 0, message)) != 0)
            {
              if (!(diskErrorT = handleScan(message, 0, count)))
              {
                removeLine(message->text);
                jam_update(count, msgNum, message);
                scanCount++;
              }
              if (diskErrorT)
                diskError = DERR_PRSCN;
            }
#ifdef _DEBUG0
            logEntryf(LOG_DEBUG, 0, "DEBUG Done scanning JAM area, scan count: %d", scanCount);
#endif // _DEBUG
          }
        }
      }
      if (!diskError)
      {
        strcpy(stpcpy(tempStr, config.bbsPath), dECHOMAIL_JAM);
#ifdef _DEBUG0
        logEntryf(LOG_DEBUG, 0, "DEBUG access: %s", tempStr);
#endif // _DEBUG
        if (0 == access(tempStr, 02))
        {
#ifdef _DEBUG0
          logEntryf(LOG_DEBUG, 0, "DEBUG unlink: %s", tempStr);
#endif // _DEBUG
          if (0 == unlink(tempStr))
          {
#ifdef _DEBUG0
            logEntryf(LOG_DEBUG, 0, "DEBUG unlinked: %s", tempStr);
#endif // _DEBUG
          }
          else
          {
#ifdef _DEBUG
            logEntryf(LOG_DEBUG, 0, "DEBUG unlink failed on: %s [%s]", tempStr, strError(errno));
#endif // _DEBUG
          }
        }
#ifdef _DEBUG
        else
          logEntryf(LOG_DEBUG, 0, "DEBUG no write access on: %s [%s]", tempStr, strError(errno));
#endif // _DEBUG
      }
      globVars.jamCountV = scanCount;
      if (scanCount == 0)
        puts("\rNo new outgoing messages\n");
      else
      {
        puts("\r                        ");
        scanCount = 0;
      }
    }
  }
  if (!(switches & SW_J))
  {
#ifdef _DEBUG
    logEntry("DEBUG Scanning "MBNAME" message base for outgoing messages", LOG_DEBUG, 0);
#else
    puts("Scanning "MBNAME" message base for outgoing messages...");
#endif // _DEBUG
    infoBad = 0;

    if (!config.mbOptions.scanAlways && !(switches & SW_S))
    {
      for (count = 0; count <= 1; count++)
      {
        if (((count == 0) && (!(switches & SW_N))) ||
            ((count == 1) && (!(switches & SW_E))))
        {
          strcpy(tempStr, makeFullPath(config.bbsPath, getenv("QUICK"), !count ? "echomail."MBEXTN : "netmail."MBEXTN));

          if ((tempHandle = open(tempStr, O_RDONLY | O_BINARY)) != -1)
          {
#ifndef GOLDBASE
            while ((read(tempHandle, &index, 2) == 2)
#else
            while ((read(tempHandle, &index, 4) == 4)
#endif
                  && !diskError)
            {
              if ((boardNum = scanBBS(index, message, 0)) != 0)
              {
                if (boardNum != -1)
                {
                  if (  (count == 0 &&  isNetmailBoard(boardNum))
                     || (count == 1 && !isNetmailBoard(boardNum))
                     )
                    infoBad++;
                  else
                  {
                    if (handleScan(message, boardNum, 0))
                      diskError = DERR_PRSCN;
                    scanCount++;
                  }
                }
                else
                  infoBad++;
              }
              else
                infoBad++;
            }
            close(tempHandle);
            if (!diskError)
              unlink (tempStr);
          }
        }
      }
    }

    if (infoBad || config.mbOptions.scanAlways || (switches & SW_S))
    {
#ifdef _DEBUG
      logEntry("DEBUG Scanning all hudson mb messages", LOG_DEBUG, 0);
#endif // _DEBUG
      index = 0;
      while (  (boardNum = scanBBS(index++, message, 0)) != 0
            && !diskError
            )
      {
        if ((boardNum != -1) &&
            (((!isNetmailBoard(boardNum)) && (!(switches & SW_N))) ||
             ((isNetmailBoard(boardNum)) && (!(switches & SW_E)))))
        {
          if (handleScan(message, boardNum, 0))
            diskError = DERR_PRSCN;
          scanCount++;
        }
      }
      if (scanCount)
        newLine();
    }
    if (scanCount == 0)
      puts("\rNo new outgoing messages\n");
    else
      puts("\r                        ");
  }

  if (closeEchoPktWr())
    diskError = DERR_CLPKTE;

  closeBBSWr(1);
#ifdef dSENDMAIL
  sendmail_smtp();
#endif
  closeNodeInfo();
  deInitAreaInfo();
  closeDup();
}
//---------------------------------------------------------------------------
void Import(int argc, char *argv[])
{
  s32            switches;
  s16            count;
  s16            boardNum;
  unsigned int   temp;
  tempStrType    tempStr;
  char          *helpPtr;
  s32            msgNum;
  DIR           *dir;
  struct dirent *ent;

  if (argc >= 3 && (argv[2][0] == '?' || argv[2][1] == '?'))
  {
    puts("Usage:\n\n"
         "    FMail Import [/A]\n\n"
         "Switches:\n\n"
         "    /A   Do not process AreaMgr requests");
    exit(0);
  }

  switches = getSwitch(&argc, argv, SW_A);

  initFMail("Import", switches);

  initPkt();
  initNodeInfo();
  initAreaInfo();                   // for AreaFix

  initMsg((u16)(switches & SW_A));  // After initNodeInfo
  retryArc();                       // After initMsg
  _mb_upderr = multiUpdate();

  initBBS();
  openBBSWr(0);

  puts("Importing netmail messages\n");

  count = 0;

  if ((dir = opendir(config.netPath)) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
    {
      if (!match_spec("*.msg", ent->d_name))
        continue;

      msgNum = strtoul(ent->d_name, &helpPtr, 10);

      if (  msgNum != 0
         && *helpPtr == '.'
//       && (!(fdMsg.attrib & FA_SPEC))  // No special attributes (Laat deze test maar achterwege, readMsg komt er vanzelf achter)
         && !readMsg(message, msgNum)
         )
      {
        if (stricmp(message->toUserName, "SysOp") == 0)
          strcpy(message->toUserName, config.sysopName);

        if ( getLocalAkaNum (&message->destNode) != -1 &&
             strnicmp(message->toUserName,"ALLFIX",6) != 0 &&
             strnicmp(message->toUserName,"FILEFIX",7) != 0 &&
             strnicmp(message->toUserName,"FILEMGR",7) != 0 &&
             strnicmp(message->toUserName,"RAID",4) != 0 &&
             strnicmp(message->toUserName,"FFGATE",6) != 0 &&
             (config.mbOptions.sysopImport ||
              (stricmp(message->toUserName, config.sysopName) != 0 &&
               !foundToUserName(message->toUserName))) &&
             ((boardNum = getNetmailBoard(&message->destNode)) != -1)
           )
        {
          if (  (message->attribute & FILE_ATT)
             &&  strchr(message->subject,'\\') == NULL
             && (strlen(message->subject) + strlen(config.inPath) < 72))
          {
            strcpy(stpcpy(tempStr, config.inPath), message->subject);
            strcpy(message->subject, tempStr);
          }
          if (config.mailOptions.privateImport)
            message->attribute |= PRIVATE;

          if (!diskError)
          {
            if (writeBBS(message, boardNum, 1))
              diskError = DERR_WRHECHO;
            else
            {
              strcpy(stpcpy(tempStr, config.netPath), ent->d_name);
              if (unlink(tempStr) == 0 && !diskError)
              {
                if (validate1BBS())
                  diskError = DERR_VAL;
                else
                {
                  validate2BBS(0);
                  logEntryf( LOG_NETIMP, 0, "Importing msg #%u from %s to %s (board #%u)"
                           , msgNum, nodeStr(&message->srcNode), nodeStr(&message->destNode), boardNum);
                  count++;
                  globVars.nmbCountV++;
                }
              }
            }
          }
        }
      }
    }

    closedir(dir);
  }

  if (count)
    newLine();

  closeBBSWr(0);
  closeNodeInfo();
  deInitPkt();
  deInitAreaInfo();

  _mb_upderr = multiUpdate();

  // 'Copy' from TOSS

  if (config.mbOptions.updateChains && globVars.mbCountV != 0)
  {
    temp = mbSharingInternal;
    mbSharingInternal = 0;
    config.mbOptions.sortNew = 0;
    sortBBS(globVars.origTotalMsgBBS, temp);
    newLine();
  }
}
//---------------------------------------------------------------------------
void Pack(int argc, char *argv[])
{
  s32 switches;

  if (argc >= 3 && (argv[2][0] == '?' || argv[2][1] == '?'))
  {
    puts("Usage:\n\n"
         "    FMail Pack [<node> [<node> ...] [VIA <node>] [/A] [/C] [/H] [/O] [/I] [/L]]\n\n"
         "Switches:\n\n"
         "    /A   Do not process AreaMgr requests\n\n"
         "    /C   Include messages with Crash status\n"
         "    /H   Include messages with Hold status\n"
         "    /O   Include orphaned messages\n\n"
         "    /I   Only pack messages that are in transit\n"
         "    /L   Only pack messages that originated on your system");
    exit(0);
  }

  switches = getSwitch(&argc, argv, SW_A | SW_C | SW_H | SW_I | SW_L | SW_O);

  initFMail("Pack", switches);

  initPkt();
  initNodeInfo();
  initAreaInfo();                   // for AreaFix

  initMsg((u16)(switches & SW_A));  // after initNodeInfo
  retryArc();                       // after initMsg
//ChkAutoBCL();
  _mb_upderr = multiUpdate();

  pack(argc, argv, switches);

#ifdef dSENDMAIL
  sendmail_smtp();
#endif
  closeNodeInfo();
}
//---------------------------------------------------------------------------
void Mgr(int argc, char *argv[])
{
  s32 switches;

  if (argc >= 3 && (argv[2][0] == '?' || argv[2][1] == '?'))
  {
    puts("Usage:\n\n"
         "    FMail Mgr\n\n"
         "Switches:\n\n"
         "    None");
    exit(0);
  }

  switches = getSwitch(&argc, argv, 0);

  initFMail("Mgr", switches);

  initPkt();
  initNodeInfo();
  initAreaInfo(); // for AreaFix

  initMsg(0);  // after initNodeInfo
//ScanNewBCL();

#ifdef dSENDMAIL
  sendmail_smtp();
#endif
  closeNodeInfo();
}
//----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  tempStrType  tempStr;
  char        *helpPtr;

#ifdef __WIN32__
  // Set default open mode:
  _fmode = O_BINARY;
#endif // __WIN32__

#ifdef __BORLANDC__
  putenv("TZ=LOC0");
  tzset();
#endif // __BORLANDC__
#ifdef __MINGW32__
  // Clear the TZ environment variable so the OS is used
  putenv("TZ=");
  tzset();
#endif // __MINGW32__

  startTime = time(NULL);
  timeBlock = *localtime(&startTime);  // localtime -> should be ok
  {
    struct tm *tm = gmtime(&startTime);  // gmt ok!
    tm->tm_isdst = -1;
    gmtOffset = startTime - mktime(tm);
  }

#ifdef dSENDMAIL
  smtpID = TIDStr();
#endif

  printf("%s - The Fast Echomail Processor\n\n", VersionStr());
  printf("Copyright (C) 1991-%s by FMail Developers - All rights reserved\n\n", __DATE__ + 7);

  memset(&globVars, 0, sizeof(globVarsType));

#ifdef _DEBUG
  getcwd(tempStr, dTEMPSTRLEN);
  printf("DEBUG cwd: %s\n", tempStr);

  {
    int i;
    for (i = 0; i < argc; i++)
      printf("DEBUG arg %d: %s\n", i, argv[i]);
  }
#endif

  if ((helpPtr = getenv("FMAIL")) == NULL || *helpPtr == 0 || !dirExist(helpPtr))
  {
    strcpy(configPath, argv[0]);
    if ((helpPtr = strrchr(configPath, '\\')) == NULL)
      strcpy(configPath, ".\\");
    else
      helpPtr[1] = 0;
  }
  else
  {
    strcpy(configPath, helpPtr);
    if (configPath[strlen(configPath) - 1] != '\\')
      strcat(configPath, "\\");
  }
#ifdef _DEBUG
  printf("DEBUG configPath: %s\n", configPath);
#endif

  if      (argc >= 2 && (stricmp(argv[1], "A") == 0 || stricmp(argv[1], "ABOUT" ) == 0))
    About();

  else if (argc >= 2 && (stricmp(argv[1], "T") == 0 || stricmp(argv[1], "TOSS"  ) == 0))
    Toss(argc, argv);

  else if (argc >= 2 && (stricmp(argv[1], "S") == 0 || stricmp(argv[1], "SCAN"  ) == 0))
    Scan(argc, argv);

  else if (argc >= 2 && (stricmp(argv[1], "I") == 0 || stricmp(argv[1], "IMPORT") == 0))
    Import(argc, argv);

  else if (argc >= 2 && (stricmp(argv[1], "P") == 0 || stricmp(argv[1], "PACK"  ) == 0))
    Pack(argc, argv);

  else if (argc >= 2 && (stricmp(argv[1], "M") == 0 || stricmp(argv[1], "MGR"   ) == 0))
    Mgr(argc, argv);

  else
  {
    puts("Usage:\n"
         "\n"
         "   FMail <command> [parameters]\n"
         "\n"
         "Commands:\n"
         "\n"
         "   About    Show some information about the program\n"
         "   Scan     Scan the message base for outgoing messages\n"
         "   Toss     Toss and forward incoming mailbundles\n"
         "   Import   Import netmail messages into the message base\n"
         "   Pack     Pack and compress outgoing netmail found in the netmail directory\n"
         "   Mgr      Only process AreaMgr requests\n"
         "\n"
         "Enter 'FMail <command> ?' for more information about [parameters]");
    return 0;
  }

  logEntryf( LOG_STATS, 0, "Netmail: %u,  Personal: %u,  "MBNAME": %u,  JAMbase: %u"
           , globVars.netCountV, globVars.perCountV, globVars.mbCountV,  globVars.jamCountV);
  logEntryf( LOG_STATS, 0, "Msgbase net: %u, echo: %u, dup: %u, bad: %u"
           , globVars.nmbCountV, globVars.echoCountV, globVars.dupCountV, globVars.badCountV);

#ifdef _DEBUG0
  logEntryf(LOG_DEBUG | LOG_NOSCRN, 0, "DEBUG sizeof(pingOptionsType): %u", sizeof(pingOptionsType));
#endif

  // Semaphore files
  //
  // Create FDRESCAN / FMRESCAN / IMRESCAN / IERESCAN / MDRESCAN

  if ((config.mailer <= 2 || config.mailer >= 4)
     && *config.semaphorePath && (globVars.createSemaphore || messagesMoved))
  {
    strcpy(tempStr, semaphore[config.mailer]);
    touch(config.semaphorePath, tempStr, "");
    if (config.mailer <= 1)
    {
      tempStr[1] = 'M';
      touch(config.semaphorePath, tempStr, "");
    }
  }

  if (config.mailer == 2)  // D'Bridge
  {
    if (globVars.mbCountV)
      touch(config.semaphorePath, "dbridge.emw", "Mail");

    if (globVars.perNetCountV)
      touch(config.semaphorePath, "dbridge.nmw", "");
  }

  if (diskError)
  {
    newLine();
    switch (diskError)
    {
      case DERR_HACKER:
        logEntry("Internal overflow control error", LOG_ALWAYS, 1);
        break;
      case DERR_VAL   :
        logEntry("File validate error", LOG_ALWAYS, 1);
        break;
      case DERR_WRPKTE:
        logEntry("Echo PKT write error", LOG_ALWAYS, 1);
        break;
      case DERR_CLPKTE:
        logEntry("Close PKT write error", LOG_ALWAYS, 1);
        break;
      case DERR_PRSCN :
        logEntry("Scan handling error", LOG_ALWAYS, 1);
        break;
      case DERR_PACK  :
        logEntry("Net message write error", LOG_ALWAYS, 1);
        break;
      case DERR_WRHECHO:
        logEntry(MBNAME" echo write error", LOG_ALWAYS, 1);
        break;
      case DERR_WRHBAD:
        logEntry(MBNAME" bad message write error", LOG_ALWAYS, 1);
        break;
      case DERR_WRHDUP:
        logEntry(MBNAME" dup message write error", LOG_ALWAYS, 1);
        break;
      case DERR_WRJECHO:
        logEntry("JAM echo write error", LOG_ALWAYS, 1);
        break;
      default:
        logEntry("Unknown disk error", LOG_ALWAYS, 1);
        break;
    }
  }
  if (mailBomb)
  {
    newLine();
    logEntry("Mail bomb protection", LOG_ALWAYS, 5);
  }

  logActive();
  deinitFMail();
  close(fmailLockHandle);

  return _mb_upderr ? 50 : 0;
}
//----------------------------------------------------------------------------
#ifdef __BORLANDC__
void myexit(void)
{
#pragma exit myexit
#ifdef _DEBUG0
  putStr("\nDEBUG Press any key to continue... ");
  getch();
  newLine();
#endif
}
#endif  // __BORLANDC__
//----------------------------------------------------------------------------

