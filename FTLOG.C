//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2014 Wilfred van Velzen
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

#include "ftlog.h"

#include "areainfo.h"
#include "dups.h"
#include "msgpkt.h"
#include "output.h"
#include "version.h"

//---------------------------------------------------------------------------
extern configType config;

time_t startTime;
u16    logUsed = 0;

const char *months     = "JanFebMarAprMayJunJulAugSepOctNovDec";
const char *dayName[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

#ifdef __BORLANDC__
#ifdef __WIN32__
#ifndef __DPMI32__
struct  COUNTRY
{
  short co_date;
  char  co_curr[5];
  char  co_thsep[2];
  char  co_desep[2];
  char  co_dtsep[2];
  char  co_tmsep[2];
  char  co_currstyle;
  char  co_digits;
  char  co_time;
  long  co_case;
  char  co_dasep[2];
  char  co_fill[10];
};
#endif
#endif
   struct COUNTRY countryInfo;
#else
   struct country countryInfo;
#endif

//---------------------------------------------------------------------------
char *expandName(char *fileName)
{
   static tempStrType tempStr[2];
   static tempIndex = 0;

   strcpy (tempStr[tempIndex = 1-tempIndex], config.bbsPath);
   strcat (tempStr[tempIndex], fileName);
   return (tempStr[tempIndex]);
}
//---------------------------------------------------------------------------
void writeLogLine (fhandle logHandle, char *s)
{
	time_t      timer;
	struct tm   timeBlockL;
	tempStrType tempStr;

	time (&timer);
        timeBlockL = *gmtime (&timer);

   switch (config.logStyle)
   {
      case 1  : /* QuickBBS */
                sprintf (tempStr ,"%02u-%.3s-%02u %02u:%02u  %s\n",
                                  timeBlockL.tm_mday,
                                  months+(timeBlockL.tm_mon*3),
                                  timeBlockL.tm_year%100,
                                  timeBlockL.tm_hour,
                                  timeBlockL.tm_min, s);
					 break;
      case 2  : /* D'Bridge */
                sprintf (tempStr ,"%02u/%02u/%02u %02u:%02u  %s\n",
                                  timeBlockL.tm_mon+1,
                                  timeBlockL.tm_mday,
                                  timeBlockL.tm_year%100,
                                  timeBlockL.tm_hour,
                                  timeBlockL.tm_min, s);
                break;
      case 3  : /* Binkley */
                sprintf (tempStr ,"> %02u %.3s %02u %02u:%02u:%02u FTOOLS %s\n",
                                  timeBlockL.tm_mday,
                                  months+(timeBlockL.tm_mon*3),
                                  timeBlockL.tm_year%100,
                                  timeBlockL.tm_hour,
                                  timeBlockL.tm_min,
                                  timeBlockL.tm_sec, s);
                break;
      default : /* FrontDoor */
                sprintf (tempStr, "  %2u%c%02u%c%02u  %s\n",
                                  timeBlockL.tm_hour, countryInfo.co_tmsep[0],
                                  timeBlockL.tm_min,  countryInfo.co_tmsep[0],
											 timeBlockL.tm_sec,  s);
                break;
   }
   write (logHandle, tempStr, strlen(tempStr));
}
//---------------------------------------------------------------------------
void initLog(char *s, s32 switches)
{
   fhandle     logHandle;
   tempStrType tempStr,
               tempStr2;
   struct tm   timeBlock;
   u16         count;
   s32         select = 1;
   char        *helpPtr;

   time(&startTime);
   timeBlock = *gmtime (&startTime);

   if (!*config.logName)
   {
      config.logInfo = 0;
   }
   if (!config.logInfo)
   {
      return;
   }

#ifndef __WIN32__
   country(0, &countryInfo);
#else
   countryInfo.co_tmsep[0] = ':';
#endif

   if ((logHandle = open(config.logName, O_RDWR|O_CREAT|O_APPEND|O_TEXT|O_DENYNONE,
                                         S_IREAD|S_IWRITE)) == -1)
   {
      printString ("WARNING: Can't open log file\n\n");
      config.logInfo = 0;
   }
   else
   {
      sprintf (tempStr2, "%s - %s", VersionStr(), s);
      helpPtr = strchr (tempStr2, 0);

      for (count = 0; count < 26; count++)
      {
         if (switches & select)
         {
            *(helpPtr++) = ' ';
            *(helpPtr++) = '/';
            *(helpPtr++) = 'A'+count;
         }
         select <<= 1;
      }
      *helpPtr = 0;

      if (config.logStyle == 0)
      {
        write( logHandle, tempStr
             , sprintf( tempStr, "\n----------  %s %4u-%02u-%02u, %s\n"
                      , dayName[timeBlock.tm_wday]
                      , timeBlock.tm_year + 1900
                      , timeBlock.tm_mon + 1
                      , timeBlock.tm_mday
                      , tempStr2
                      )
             );
      }
      else
      {
         if (config.logStyle == 1)
         {
            writeLogLine (logHandle, "**************************************************");
         }
         if (config.logStyle == 3)
         {
      	    write (logHandle, "\n", 1);
         }
         writeLogLine (logHandle, tempStr2);
      }
      close (logHandle);
   }
   logUsed++;
}
//---------------------------------------------------------------------------
void logEntry(char *s, u16 entryType, u16 errorLevel)
{
   fhandle     logHandle;
   tempStrType tempStr;

   if (!(entryType & LOG_NOSCRN))
   {
      printString (s);
      newLine ();
   }

   if (((((config.logInfo | LOG_ALWAYS) & entryType) == 0) &&
        ((config.logInfo & LOG_DEBUG) == 0)) ||
       ((logHandle = open(config.logName, O_RDWR|O_APPEND|O_TEXT|O_DENYNONE, 0)) == -1))
   {
      if (errorLevel)
      {
         sprintf (tempStr, "Exiting with errorlevel %u\n", errorLevel);
         printString (tempStr);
         showCursor ();
         exit (errorLevel);
      }
      return;
   }

   if (logUsed) writeLogLine (logHandle, s);

   if (errorLevel)
   {
      sprintf (tempStr, "Exiting with errorlevel %u\n", errorLevel);
      if (logUsed) writeLogLine (logHandle, tempStr);
      close (logHandle);
      printString (tempStr);
      showCursor ();
      unlink(expandName("MSGHDR.$$$"));
      unlink(expandName("MSGIDX.$$$"));
      unlink(expandName("MSGTOIDX.$$$"));
      unlink(expandName("MSGTXT.$$$"));
      unlink(expandName("MSGINFO.$$$"));
      exit (errorLevel);
   }
   close(logHandle);
}
//---------------------------------------------------------------------------
void logActive (void)
{
  time_t endTime;
  char   timeStr[32];

  newLine();
  time(&endTime);
  endTime -= startTime;
  sprintf(timeStr, "Active: %2u:%02u", (u16)(endTime / 60), (u16)(endTime % 60));
  logEntry(timeStr, LOG_STATS, 0);
}
//---------------------------------------------------------------------------
