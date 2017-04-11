//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2017  Wilfred van Velzen
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
#include <windows.h>
#endif
#ifdef __linux__
#include <stdarg.h>
#endif // __linux__
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "fmail.h"

#include "areainfo.h"
#include "dups.h"
#include "log.h"
#include "msgpkt.h"
#include "os.h"
#include "os_string.h"
#include "utils.h"
#include "version.h"

//---------------------------------------------------------------------------
extern configType config;
extern char      *dayName[7];
extern char      *months;

//clock_t           at;

u16               mgrLogUsed = 0;
//---------------------------------------------------------------------------
static void writeLogLine(fhandle logHandle, const char *s)
{
#if    defined(__linux__)
  struct timespec ts;
#elif !defined(__WIN32__)
  time_t timer;
#endif
  unsigned int ms;
  struct tm tm;

#if defined(__WIN32__)
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
  clock_gettime(CLOCK_REALTIME, &ts);
  tm = *localtime(&ts.tv_sec);
  ms = ts.tv_nsec / 1000000;
#else
  time(&timer);
  tm = *localtime(&timer);
  ms = 0;
#endif // __WIN32__

  switch (config.logStyle)
  {
    case 1:  // QuickBBS
      dprintf(logHandle, "%02u-%.3s-%02u %02u:%02u  "
             , tm.tm_mday
             , months + (tm.tm_mon * 3)
             , tm.tm_year % 100
             , tm.tm_hour
             , tm.tm_min
             );
      break;
    case 2:  // D'Bridge
      dprintf(logHandle, "%02u/%02u/%02u %02u:%02u  "
             , tm.tm_mon + 1
             , tm.tm_mday
             , tm.tm_year % 100
             , tm.tm_hour
             , tm.tm_min
             );
      break;
    case 3:  // Binkley
      dprintf(logHandle, "> %02u %.3s %02u %02u:%02u:%02u FMAIL  "
             , tm.tm_mday
             , months + (tm.tm_mon * 3)
             , tm.tm_year % 100
             , tm.tm_hour
             , tm.tm_min
             , tm.tm_sec
             );
      break;
    case 4:  // FMail
      dprintf(logHandle, "%02u:%02u:%02u.%03u  ", tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
      break;
    default: // FrontDoor
      dprintf(logHandle, "  %2u:%02u:%02u  ", tm.tm_hour, tm.tm_min, tm.tm_sec);
      break;
  }
  write(logHandle, s, strlen(s));
  write(logHandle, "\n", 1);
}
//---------------------------------------------------------------------------
void initLog(s32 switches)
{
  fhandle     logHandle;
  tempStrType tempStr;
  u16         count;
  s32         select = 1;
  char       *helpPtr;

  //at = clock();

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
      helpPtr = stpcpy(tempStr, funcStr);
    else
      helpPtr = tempStr + sprintf(tempStr, "%s - %s", VersionStr(), funcStr);

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
        writeLogLine(logHandle, tempStr);
        break;
      }
      case 4:
        dprintf(logHandle, "\n------------  %s %04u-%02u-%02u, %s\n"
                         , dayName[timeBlock.tm_wday]
                         , timeBlock.tm_year + 1900
                         , timeBlock.tm_mon + 1
                         , timeBlock.tm_mday
                         , VersionStr()
               );
        writeLogLine(logHandle, tempStr);
        break;
      default:
        dprintf(logHandle, "\n----------  %s %04u-%02u-%02u, %s\n"
                         , dayName[timeBlock.tm_wday]
                         , timeBlock.tm_year + 1900
                         , timeBlock.tm_mon + 1
                         , timeBlock.tm_mday
                         , tempStr
               );
        break;
    }
    close(logHandle);
  }
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
void logEntry(const char *s, u16 entryType, u16 errorLevel)
{
  fhandle     logHandle;
  tempStrType tempStr;

  if (!(entryType & LOG_NOSCRN))
  {
    puts(s);
    fflush(stdout);
  }

  if (  entryType == LOG_NEVER
     || ( ((config.logInfo | LOG_ALWAYS) & entryType) == 0
        && (config.logInfo & LOG_DEBUG) == 0
        )
     )
  {
    if (errorLevel)
    {
      if (errorLevel != 100)
      {
        printf("Exiting with errorlevel %u\n", errorLevel);
        if (entryType != LOG_NEVER)
          closeDup();
      }
      exit(errorLevel == 100 ? 0 : errorLevel);
    }
    return;
  }

  if ((logHandle = open(fixPath(config.logName), O_WRONLY | O_CREAT | O_APPEND | O_TEXT, dDEFOMODE)) != -1)
    writeLogLine(logHandle, s);

  if (errorLevel)
  {
    if (errorLevel != 100)
    {
      sprintf(tempStr, "Exiting with errorlevel %u", errorLevel);
      puts(tempStr);
      if (logHandle != -1)
      {
        writeLogLine(logHandle, tempStr);
        close(logHandle);
      }
      if (entryType != LOG_NEVER)
        closeDup();
    }
    exit(errorLevel == 100 ? 0 : errorLevel);
  }
  if (logHandle != -1)
    close(logHandle);
}
//---------------------------------------------------------------------------
void mgrLogEntry(const char *s)
{
  fhandle logHandle;

  puts(s);

  if ((*config.areaMgrLogName) && (!(mgrLogUsed++)) &&
      stricmp(config.logName, config.areaMgrLogName) &&
      ((logHandle = open(fixPath(config.areaMgrLogName), O_WRONLY | O_CREAT | O_APPEND | O_TEXT, dDEFOMODE)) != -1))
  {
    if (config.logStyle == 0 || config.logStyle == 4)
    {
      dprintf(logHandle, "\n----------%s  %s %04u-%02u-%02u, %s - AreaMgr\n"
                       , config.logStyle ? "--" : ""
                       , dayName[timeBlock.tm_wday]
                       , timeBlock.tm_year + 1900
                       , timeBlock.tm_mon + 1
                       , timeBlock.tm_mday
                       , VersionStr()
             );
    }
    else
    {
      if (config.logStyle == 1)
        writeLogLine(logHandle, "**************************************************");

      if (config.logStyle == 3)
        write(logHandle, "\n", 1);

      writeLogLine(logHandle, s);
    }
    close(logHandle);
  }

  if (((logHandle = open( fixPath(*config.areaMgrLogName ? config.areaMgrLogName : config.logName)
                        , O_WRONLY | O_CREAT | O_APPEND | O_TEXT, dDEFOMODE)) != -1))
  {
    writeLogLine(logHandle, s);
    close(logHandle);
  }
}
//---------------------------------------------------------------------------
void logActive(void)
{
  newLine();
  clock_t t = clock();
  if (t >= 0)
  {
    double d = ((double)t) / CLOCKS_PER_SEC;
    logEntryf(LOG_STATS, 0, "%s Active: %.*f sec.", funcStr, d < .01 ? 4 : 3, d);
  }
}
//---------------------------------------------------------------------------
