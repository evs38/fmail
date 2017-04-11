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

#ifdef __WIN32__
#include <dir.h>
//#include <dos.h>
#include <share.h>
#endif // __WIN32__
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "config.h"

#include "areainfo.h"
#include "crc.h"
#include "log.h"
#include "mtask.h"
#include "os.h"
#include "os_string.h"
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
  fhandle     configHandle;
  time_t      time1
            , time2
            , time2a;
  tempStrType tempStr
            , tempStr2;
  char       *helpPtr;

  strcpy(funcStr, _funcStr);
  strcpy(stpcpy(tempStr, configPath), dCFGFNAME);

  if (  (configHandle = open(fixPath(tempStr), O_RDONLY | O_BINARY)) == -1
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
  strcpy(tempStr2, fixPath(config.bbsPath));
  if ((helpPtr = strrchr(tempStr2, dDIRSEPC)) != NULL)  // path already fixed for linux
    *helpPtr = 0;

  if (  !access(tempStr2, 0)  // path already fixed for linux
     && (fmailLockHandle = _sopen(fixPath(tempStr), O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, dDEFOMODE)) == -1
     && errno != ENOENT  // path does not exist
     )
  {
    puts("Waiting for another copy of FMail, FTools or FSetup to finish...");

    time(&time1);
    time2 = time1;

    while (  (fmailLockHandle = _sopen(fixPath(tempStr), O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, dDEFOMODE)) == -1
          && (!config.activTimeOut || time2 - time1 < config.activTimeOut)
          )
    {
      time2a = time2 + 1;
      while (time(&time2) <= time2a)
        returnTimeSlice(1);
    }
    if (fmailLockHandle == -1)
    {
      puts("\nAnother copy of FMail, FTools or FSetup did not finish in time...\n\nExiting...");
      exit(4);
    }
  }

  initLog(switches);

  if (config.akaList[0].nodeNum.zone == 0)
    logEntry("Main nodenumber not defined in FSetup!", LOG_ALWAYS, 4);

  if (  *config.netPath == 0
     || *config.bbsPath == 0
     || *config.inPath  == 0
     || *config.outPath == 0
     )
    logEntry("Not all required subdirectories are defined in FSetup!", LOG_ALWAYS, 4);

  if (  (existDir(config.netPath, "netmail"     ) == 0)
     || (existDir(config.bbsPath, "message base") == 0)
     || (existDir(config.inPath,  "inbound"     ) == 0)
     || (existDir(config.outPath, "outbound"    ) == 0)
     || (*config.pmailPath           && existDir(config.pmailPath          , "personal mail") == 0)
     || (*config.sentEchoPath        && existDir(config.sentEchoPath       , "sent echomail") == 0)
     || (*config.autoGoldEdAreasPath && existDir(config.autoGoldEdAreasPath, "AREAS.GLD"    ) == 0)
     )
    logEntry("Please enter the required subdirectories first!", LOG_ALWAYS, 4);

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
#ifdef _DEBUG
#if   defined(__WIN32__)
  logEntryf(LOG_DEBUG, 0, "DEBUG gmtOffset=%ld daylight=%d timezone=%ld tzname=%s-%s", gmtOffset, _daylight, _timezone, _tzname[0], _tzname[1]);
#elif defined(__linux__)
  logEntryf(LOG_DEBUG, 0, "DEBUG gmtOffset=%ld daylight=%d timezone=%ld tzname=%s-%s", gmtOffset, daylight, timezone, tzname[0], tzname[1]);
#endif
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

  strcpy(stpcpy(configFileStr, fixPath(configPath)), dCFGFNAME);

  if (  (configHandle = open(configFileStr, O_WRONLY | O_BINARY)) == -1  // configFileStr already fixed
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
