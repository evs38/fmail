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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>    // va_*()
#ifdef __WIN32__
#include <windows.h>
#endif

#include "ftlog.h"

#include "areainfo.h"
#include "dups.h"
#include "msgpkt.h"
#include "msgra.h"
#include "msgradef.h"
#include "os.h"
#include "stpcpy.h"
#include "utils.h"
#include "version.h"

//---------------------------------------------------------------------------
extern configType config;

//time_t  startTime;
clock_t at;
u16     logUsed = 0;
char    funcStr[32] = "";

const char *months     = "JanFebMarAprMayJunJulAugSepOctNovDec";
const char *dayName[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

//---------------------------------------------------------------------------
char *expandName(char *fileName)
{
  static tempStrType tempStr[2];
  static int tempIndex = 0;
  char *ts = tempStr[tempIndex = 1 - tempIndex];

  strcpy(stpcpy(ts, fixPath(config.bbsPath)), fileName);

  return ts;
}
//---------------------------------------------------------------------------
void writeLogLine(fhandle logHandle, const char *s)
{
  unsigned int ms;
  struct tm tm;
#ifdef __WIN32__
  SYSTEMTIME st;
  GetLocalTime(&st);
  tm.tm_year = st.wYear  - 1900;
  tm.tm_mon  = st.wMonth - 1;
  tm.tm_mday = st.wDay;
  tm.tm_hour = st.wHour;
  tm.tm_min  = st.wMinute;
  tm.tm_sec  = st.wSecond;
  ms         = st.wMilliseconds;
#elif defined(__linux__)
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  tm = *localtime(&ts.tv_sec);
  ms = ts.tv_nsec / 1000000;
#else
  time_t timer = time(NULL);
  tm = *localtime(&timer);  // localtime ok!
  ms = 0;
#endif // __WIN32__

  switch (config.logStyle)
  {
    case 1:  // QuickBBS
      dprintf(logHandle, "%02u-%.3s-%02u %02u:%02u  %s\n"
              , tm.tm_mday
              , months + (tm.tm_mon * 3)
              , tm.tm_year % 100
              , tm.tm_hour
              , tm.tm_min, s
              );
      break;
    case 2:  // D'Bridge
      dprintf(logHandle, "%02u/%02u/%02u %02u:%02u  %s\n"
              , tm.tm_mon + 1
              , tm.tm_mday
              , tm.tm_year % 100
              , tm.tm_hour
              , tm.tm_min, s
              );
      break;
    case 3:  // Binkley
      dprintf(logHandle, "> %02u %.3s %02u %02u:%02u:%02u FMAIL  %s\n"
              , tm.tm_mday
              , months + (tm.tm_mon * 3)
              , tm.tm_year % 100
              , tm.tm_hour
              , tm.tm_min
              , tm.tm_sec, s
              );
      break;
    case 4:  // FMail
      dprintf(logHandle, "%02u:%02u:%02u.%03u  %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, ms, s);
      break;
    default:  // FrontDoor
      dprintf(logHandle, "  %2u:%02u:%02u  %s\n", tm.tm_hour, tm.tm_min , tm.tm_sec , s);
      break;
  }
}
//---------------------------------------------------------------------------
void initLog(const char *_funcStr, s32 switches)
{
  fhandle     logHandle;
  tempStrType tempStr2;
  struct tm   timeBlock;
  u16         count;
  s32         select = 1;
  char       *helpPtr;

  at = clock();
  timeBlock = *localtime(&startTime);  // localtime ok!

  strcpy(funcStr, _funcStr);

  if (!*config.logName)
    config.logInfo = 0;

  if (!config.logInfo)
    return;

  if ((logHandle = open(fixPath(config.logName), O_WRONLY | O_CREAT | O_APPEND | O_TEXT, dDEFOMODE)) == -1)
  {
    puts("WARNING: Can't open log file\n");
    config.logInfo = 0;
  }
  else
  {
    if (config.logStyle == 4)
      helpPtr = stpcpy(tempStr2, _funcStr);
    else
      helpPtr = tempStr2 + sprintf(tempStr2, "%s - %s", VersionStr(), _funcStr);

    for (count = 0; count < 26; count++)
    {
       if (switches & select)
       {
          *(helpPtr++) = ' ';
          *(helpPtr++) = '-';
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
      case 4:
        dprintf(logHandle
               , "\n------------  %s %04u-%02u-%02u, %s\n"
               , dayName[timeBlock.tm_wday]
               , timeBlock.tm_year + 1900
               , timeBlock.tm_mon + 1
               , timeBlock.tm_mday
               , VersionStr()
               );
        writeLogLine(logHandle, tempStr2);
        break;
      default:
        dprintf(logHandle
               , "\n----------  %s %04u-%02u-%02u, %s\n"
               , dayName[timeBlock.tm_wday]
               , timeBlock.tm_year + 1900
               , timeBlock.tm_mon + 1
               , timeBlock.tm_mday
               , tempStr2
               );
        break;
    }
    close(logHandle);
  }
  logUsed++;
}
//---------------------------------------------------------------------------
void logEntryf(u16 entryType, u16 errorLevel, const char *fmt, ...)
{
  char buf[1024];
  va_list argptr;

  va_start(argptr, fmt);
  vsnprintf(buf, sizeof(buf) - 1, fmt, argptr);
  buf[sizeof(buf) - 1] = 0;
  va_end(argptr);

  logEntry(buf, entryType, errorLevel);
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
     || (logHandle = open(fixPath(config.logName), O_WRONLY | O_APPEND | O_TEXT)) == -1
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

    unlink(expandName(dMSGHDR"."dEXTTMP));    // already fixPath'd
    unlink(expandName(dMSGIDX"."dEXTTMP));    // already fixPath'd
    unlink(expandName(dMSGTOIDX"."dEXTTMP));  // already fixPath'd
    unlink(expandName(dMSGTXT"."dEXTTMP));    // already fixPath'd
    unlink(expandName(dMSGINFO"."dEXTTMP));   // already fixPath'd

    exit(errorLevel);
  }
  close(logHandle);
}
//---------------------------------------------------------------------------
void logActive(void)
{
  newLine();
  clock_t t = clock();
  if (t >= 0)
  {
    double d = ((double)t) / CLOCKS_PER_SEC;
    if (*funcStr)
      logEntryf(LOG_STATS, 0, "%s Active: %.*f sec.", funcStr, d < .01 ? 4 : 3, d);
    else
      logEntryf(LOG_STATS, 0, "Active: %.*f sec.", d < .01 ? 4 : 3, d);
  }
}
//---------------------------------------------------------------------------
