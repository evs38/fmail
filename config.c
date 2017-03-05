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

#ifdef __STDIO__
#include <conio.h>
#else
#include <bios.h>
#endif
#include <ctype.h>
#include <dir.h>
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#if defined _Windows && !defined __DPMI16__
#define _far
#define far
#endif

#ifdef __FMAILX__
unsigned long _far _pascal GlobalDosAlloc(unsigned long);
unsigned int  _far _pascal GlobalDosFree(unsigned long);
#ifdef __DPMI32__
extern unsigned _cdecl _psp;
#define MK_FP(seg,ofs) ((void *)((long)(seg)<<16) + (ofs))
#endif
#endif

#include "config.h"

#include "areainfo.h"
#include "crc.h"
#include "log.h"
//#include "msgpkt.h" // for openP
#include "mtask.h"
#include "stpcpy.h"
#include "utils.h"

extern fhandle fmailLockHandle;

// Borland C number of files
extern unsigned int _nfile;

// Global available datastructures
configType   config;
unsigned int mbSharingInternal = 1;
char         funcStr[32] = "undefined?";

extern s16 diskError;

extern internalMsgType *message;

extern psRecType *seenByArray;
extern psRecType *tinySeenArray;
extern psRecType *pathArray;
extern psRecType *tinyPathArray;

fhandle fmLockHandle;

#ifdef __STDIO__
#define keyWaiting (kbhit() ? getch() : 0)
#endif

//---------------------------------------------------------------------------
s16 getNetmailBoard(nodeNumType *akaNode)
{
  s16 count = MAX_NETAKAS;

  while (  count >= 0
        && (memcmp(&config.akaList[count].nodeNum, akaNode, sizeof(nodeNumType)) != 0)
        )
    count--;

  if (count < 0 || config.netmailBoard[count] == 0)
    return -1;

  return config.netmailBoard[count];
}
//---------------------------------------------------------------------------
s16 isNetmailBoard(u16 board)
{
  s16 count = MAX_NETAKAS;

  while (count >= 0 && config.netmailBoard[count] != board)
    count--;

  return count != -1;
}
//---------------------------------------------------------------------------
void initFMail(const char *_funcStr, s32 switches)
{
  s16         ch;
  fhandle     configHandle;
  time_t      time1
            , time2
            , time2a;
  tempStrType tempStr
            , tempStr2;
  char       *helpPtr;

  strcpy(funcStr, _funcStr);
  strcpy(stpcpy(tempStr, configPath), dCFGFNAME);

  if (  (configHandle = open(tempStr, O_RDONLY | O_BINARY)) == -1
     || read(configHandle, &config, sizeof(configType)) < (int)sizeof(configType)
     || close(configHandle) == -1
     )
  {
    printf("Can't read: %s\n", tempStr);
    exit(4);
  }
  if (config.versionMajor != CONFIG_MAJOR || config.versionMinor != CONFIG_MINOR)
  {
    printf("ERROR: "dCFGFNAME" is not created by FSetup %d.%d or later.\n", CONFIG_MAJOR, CONFIG_MINOR);
    if ((config.versionMajor == 0) && (config.versionMinor == 36))
      puts("Running FSetup will update the config files.");

    exit(4);
  }

  strcpy(stpcpy(tempStr, config.bbsPath), dFMAIL_LOC);
  strcpy(tempStr2, config.bbsPath);
  if ((helpPtr = strrchr(tempStr2, '\\')) != NULL)
    *helpPtr = 0;

  if (  !access(tempStr2, 0)
     && (fmailLockHandle = _sopen(tempStr, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, S_IREAD | S_IWRITE)) == -1
     && errno != ENOFILE
     )
  {
    puts("Waiting for another copy of FMail, FTools or FSetup to finish...");

    time(&time1);
    time2 = time1;

    while (  (fmailLockHandle = _sopen(tempStr, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, S_IREAD | S_IWRITE)) == -1
          && (!config.activTimeOut || time2 - time1 < config.activTimeOut)
          && (ch = (keyWaiting & 0xff)) != 27
          )
    {
      if (ch == 0 || ch == -1)
      {
        time2a = time2 + 1;
        while (time(&time2) <= time2a)
          returnTimeSlice(1);
      }
#ifndef __STDIO__
      else
        keyRead;
#endif
    }
#ifndef __STDIO__
    if (ch != 0 && ch != -1)
      keyRead;
#endif
    if (fmailLockHandle == -1)
    {
      puts("\nAnother copy of FMail, FTools or FSetup did not finish in time...\n\nExiting...");
      exit(4);
    }
  }

  if (config.akaList[0].nodeNum.zone == 0)
  {
    puts("Main nodenumber not defined in FSetup!");
    exit(4);
  }

  if ((*config.netPath == 0) ||
    (*config.bbsPath == 0) ||
    (*config.inPath  == 0) ||
    (*config.outPath == 0))
  {
    puts("Not all required subdirectories are defined in FSetup!");
    exit(4);
  }

  if (  (existDir(config.netPath, "netmail"     ) == 0)
     || (existDir(config.bbsPath, "message base") == 0)
     || (existDir(config.inPath,  "inbound"     ) == 0)
     || (existDir(config.outPath, "outbound"    ) == 0)
     || (*config.pmailPath           && existDir(config.pmailPath          , "personal mail") == 0)
     || (*config.sentEchoPath        && existDir(config.sentEchoPath       , "sent echomail") == 0)
     || (*config.autoGoldEdAreasPath && existDir(config.autoGoldEdAreasPath, "AREAS.GLD"    ) == 0)
     )
  {
    puts("Please enter the required subdirectories first!");
    exit(4);
  }

  if (config.maxForward < 64)
    config.maxForward = 64;
  if (config.maxMsgSize < 45)
    config.maxMsgSize = 45;
  if (config.recBoard > MBBOARDS)
    config.recBoard = 0;
  if (config.badBoard > MBBOARDS)
    config.badBoard = 0;
  if (config.dupBoard > MBBOARDS)
    config.dupBoard = 0;

#ifdef GOLDBASE
  config.bbsProgram = BBS_QBBS;
#endif

  initLog(switches);

#ifdef _DEBUG
  logEntryf(LOG_DEBUG, 0, "DEBUG gmtOffset=%ld daylight=%d timezone=%ld tzname=%s-%s", gmtOffset, _daylight, _timezone, _tzname[0], _tzname[1]);
#endif

  if (  NULL == (message       = (internalMsgType *)malloc(INTMSG_SIZE                          ))
     || NULL == (seenByArray   = (psRecType       *)malloc(sizeof(psRecType) * MAX_MSGSEENBY    ))
     || NULL == (tinySeenArray = (psRecType       *)malloc(sizeof(psRecType) * MAX_MSGTINYSEENBY))
     || NULL == (pathArray     = (psRecType       *)malloc(sizeof(psRecType) * MAX_MSGPATH      ))
     || NULL == (tinyPathArray = (psRecType       *)malloc(sizeof(psRecType) * MAX_MSGTINYPATH  ))
     )
    logEntry("Not enough memory to initialize FMail", LOG_ALWAYS, 2);

  strupr(config.topic1);
  strupr(config.topic2);

  puts("\n");
}
//---------------------------------------------------------------------------
void deinitFMail(void)
{
  fhandle     configHandle;
  tempStrType configFileStr;

  strcpy(stpcpy(configFileStr, configPath), dCFGFNAME);

  if (  (configHandle = open(configFileStr, O_WRONLY | O_BINARY)) == -1
     || lseek(configHandle, offsetof(configType, uplinkReq), SEEK_SET) == -1L
     || write(configHandle, &config.uplinkReq, sizeof(uplinkReqType)*MAX_UPLREQ) < (int)sizeof(uplinkReqType)*MAX_UPLREQ
     || lseek(configHandle, offsetof(configType, lastUniqueID), SEEK_SET) == -1L
     || write(configHandle, &config.lastUniqueID, sizeof(config.lastUniqueID)) < (int)sizeof(config.lastUniqueID)
     || close(configHandle) == -1
     )
  {
    if (configHandle != -1)
      close(configHandle);

    logEntryf(LOG_ALWAYS, 0, "Can't write: %s [%s]", configFileStr, strError(errno));
  }
  freeNull(message      );
  freeNull(seenByArray  );
  freeNull(tinySeenArray);
  freeNull(pathArray    );
  freeNull(tinyPathArray);
}
//---------------------------------------------------------------------------
