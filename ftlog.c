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

#include "ftlog.h"

#include "areainfo.h"
#include "dups.h"
#include "msgpkt.h"
#include "msgra.h"
#include "msgradef.h"
#include "stpcpy.h"
#include "version.h"

//---------------------------------------------------------------------------
extern configType config;

time_t  startTime;
clock_t at;
u16     logUsed = 0;

const char *months     = "JanFebMarAprMayJunJulAugSepOctNovDec";
const char *dayName[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

//---------------------------------------------------------------------------
char *expandName(char *fileName)
{
  static tempStrType tempStr[2];
  static int tempIndex = 0;
  char *ts = tempStr[tempIndex = 1 - tempIndex];

  strcpy(stpcpy(ts, config.bbsPath), fileName);

  return ts;
}
//---------------------------------------------------------------------------
void writeLogLine(fhandle logHandle, const char *s)
{
  struct tm   tm;
  tempStrType tempStr;
  int         sl;

#ifdef __WIN32__
  SYSTEMTIME st;
  GetLocalTime(&st);
  if (config.logStyle != 4)
  {
    tm.tm_year = st.wYear  - 1900;
    tm.tm_mon  = st.wMonth - 1;
    tm.tm_mday = st.wDay;
    tm.tm_hour = st.wHour;
    tm.tm_min  = st.wMinute;
    tm.tm_sec  = st.wSecond;
  }
#else
  time_t timer = time(NULL);
  tm = *gmtime(&timer);
#endif

  switch (config.logStyle)
  {
    case 1:  // QuickBBS
      sl = sprintf( tempStr ,"%02u-%.3s-%02u %02u:%02u  %s\n"
                  , tm.tm_mday
                  , months + (tm.tm_mon * 3)
                  , tm.tm_year % 100
                  , tm.tm_hour
                  , tm.tm_min, s
                  );
      break;
    case 2:  // D'Bridge
      sl = sprintf( tempStr ,"%02u/%02u/%02u %02u:%02u  %s\n"
                  , tm.tm_mon + 1
                  , tm.tm_mday
                  , tm.tm_year % 100
                  , tm.tm_hour
                  , tm.tm_min, s
                  );
      break;
    case 3:  // Binkley
      sl = sprintf( tempStr ,"> %02u %.3s %02u %02u:%02u:%02u FMAIL  %s\n"
                  , tm.tm_mday
                  , months + (tm.tm_mon * 3)
                  , tm.tm_year % 100
                  , tm.tm_hour
                  , tm.tm_min
                  , tm.tm_sec, s
                  );
      break;
#ifdef __WIN32__
    case 4:  // FMail
      sl = sprintf(tempStr, "%02u:%02u:%02u.%03u  %s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, s);
      break;
#endif
    default:  // FrontDoor
      sl = sprintf(tempStr, "  %2u:%02u:%02u  %s\n", tm.tm_hour, tm.tm_min , tm.tm_sec , s);
      break;
  }
  write(logHandle, tempStr, sl);
}
//---------------------------------------------------------------------------
void initLog(char *s, s32 switches)
{
  fhandle     logHandle;
  tempStrType tempStr
            , tempStr2;
  struct tm   timeBlock;
  u16         count;
  s32         select = 1;
  char        *helpPtr;

  at = clock();

  time(&startTime);
  timeBlock = *gmtime(&startTime);

  if (!*config.logName)
    config.logInfo = 0;

  if (!config.logInfo)
    return;

  if ((logHandle = open(config.logName, O_WRONLY | O_CREAT | O_APPEND | O_TEXT, S_IREAD | S_IWRITE)) == -1)
  {
    puts("WARNING: Can't open log file\n");
    config.logInfo = 0;
  }
  else
  {
#ifdef __WIN32__
    if (config.logStyle == 4)
      helpPtr = stpcpy(tempStr2, s);
    else
#endif
      helpPtr = tempStr2 + sprintf(tempStr2, "%s - %s", VersionStr(), s);

    for (count = 0; count < 26; count++)
    {
       if (switches & select)
       {
          *(helpPtr++) = ' ';
          *(helpPtr++) = '/';
        *(helpPtr++) = 'A' + count;
       }
       select <<= 1;
    }
    *helpPtr = 0;

    switch (config.logStyle)
    {
      case 1:
      case 2:
      case 3:
      {
        switch (config.logStyle)
        {
          case 1:
            writeLogLine(logHandle, "**************************************************");
            break;
          case 3:
            write(logHandle, "\n", 1);
            break;
        }
        writeLogLine(logHandle, tempStr2);
        break;
      }
#ifdef __WIN32__
      case 4:
        write( logHandle, tempStr
             , sprintf( tempStr, "\n------------  %s %04u-%02u-%02u, %s\n"
                      , dayName[timeBlock.tm_wday]
                      , timeBlock.tm_year + 1900
                      , timeBlock.tm_mon + 1
                      , timeBlock.tm_mday
                      , VersionStr()
                      )
             );
        writeLogLine(logHandle, tempStr2);
        break;
#endif
      default:
        write( logHandle, tempStr
             , sprintf( tempStr, "\n----------  %s %04u-%02u-%02u, %s\n"
                      , dayName[timeBlock.tm_wday]
                      , timeBlock.tm_year + 1900
                      , timeBlock.tm_mon + 1
                      , timeBlock.tm_mday
                      , tempStr2
                      )
             );
        break;
    }
    close(logHandle);
  }
  logUsed++;
}
//---------------------------------------------------------------------------
void logEntry(char *s, u16 entryType, u16 errorLevel)
{
  fhandle     logHandle;
  tempStrType tempStr;

  if (!(entryType & LOG_NOSCRN))
    puts(s);

  if (  (  ((config.logInfo | LOG_ALWAYS) & entryType) == 0
        && ( config.logInfo & LOG_DEBUG              ) == 0
        )
     || (logHandle = open(config.logName, O_WRONLY | O_APPEND | O_TEXT)) == -1
     )
  {
    if (errorLevel)
    {
      printf("Exiting with errorlevel %u\n", errorLevel);

      exit(errorLevel);
    }
    return;
  }

  if (logUsed)
    writeLogLine(logHandle, s);

  if (errorLevel)
  {
    sprintf(tempStr, "Exiting with errorlevel %u\n", errorLevel);
    if (logUsed)
      writeLogLine(logHandle, tempStr);

    close(logHandle);
    putStr(tempStr);

    unlink(expandName(dMSGHDR"."dEXTTMP));
    unlink(expandName(dMSGIDX"."dEXTTMP));
    unlink(expandName(dMSGTOIDX"."dEXTTMP));
    unlink(expandName(dMSGTXT"."dEXTTMP));
    unlink(expandName(dMSGINFO"."dEXTTMP));

    exit(errorLevel);
  }
  close(logHandle);
}
//---------------------------------------------------------------------------
void logActive(void)
{
  tempStrType timeStr;

  newLine();
  sprintf(timeStr, "FTools Active: %.3f sec.", ((double)(clock() - at)) / CLK_TCK);
  logEntry(timeStr, LOG_STATS, 0);
}
//---------------------------------------------------------------------------
