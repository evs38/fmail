/*
 *  Copyright (C) 2007 Folkert J. Wijnstra
 *
 *
 *  This file is part of FMail.
 *
 *  FMail is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  FMail is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <dos.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "fmail.h"
#include "ftools.h"
#include "config.h"
#include "output.h"
#include "crc.h"
#include "log.h"
#include "jam.h"
#include "areainfo.h"
#include "nodeinfo.h"
#include "utils.h"
#include "filesys.h"


extern configType config;
extern s32        startTime;


static s16 _mdays [13] =
            {
/* Jan */   0,
/* Feb */   31,
/* Mar */   31+28,
/* Apr */   31+28+31,
/* May */   31+28+31+30,
/* Jun */   31+28+31+30+31,
/* Jul */   31+28+31+30+31+30,
/* Aug */   31+28+31+30+31+30+31,
/* Sep */   31+28+31+30+31+30+31+31,
/* Oct */   31+28+31+30+31+30+31+31+30,
/* Nov */   31+28+31+30+31+30+31+31+30+31,
/* Dec */   31+28+31+30+31+30+31+31+30+31+30,
/* Jan */   31+28+31+30+31+30+31+31+30+31+30+31
            };

time_t JAMtime(s32 t)
{
    static struct tm  m;
    s16               LeapDay;

    m.tm_sec  = (s16) (t % 60); t /= 60;
    m.tm_min  = (s16) (t % 60); t /= 60;
    m.tm_hour = (s16) (t % 24); t /= 24;
    m.tm_wday = (s16) ((t + 4) % 7);

    m.tm_year = (s16) (t / 365 + 1);
    do
        {
        m.tm_year--;
        m.tm_yday = (s16) (t - m.tm_year * 365 - ((m.tm_year + 1) / 4));
        }
    while(m.tm_yday < 0);
    m.tm_year += 70;

    LeapDay = ((m.tm_year & 3) == 0 && m.tm_yday >= _mdays [2]);

    for(m.tm_mon = m.tm_mday = 0; m.tm_mday == 0; m.tm_mon++)
        if(m.tm_yday < _mdays [m.tm_mon + 1] + LeapDay)
	    m.tm_mday = m.tm_yday + 1 - (_mdays [m.tm_mon] + (m.tm_mon != 1 ? LeapDay : 0));

    m.tm_mon--;

    m.tm_isdst = -1;

    return mktime(&m);
}



s16 JAMremoveRe(char *buf, u32 *bufsize)
{
   u16         size,
               slen,
               totalsize = 0;
   tempStrType tempStr;
   char        *helpPtr;

   if ( *bufsize >= 65536L )
      return 0;

   while ( ((JAMBINSUBFIELD *)buf)->LoID != JAMSFLD_SUBJECT )
   {  size = (u16)sizeof(JAMBINSUBFIELD)+(u16)((JAMBINSUBFIELD *)buf)->DatLen;
      buf += size;
      totalsize += size;
      if (totalsize >= *bufsize)
         return 0;
   }
   size =(u16)((JAMBINSUBFIELD *)buf)->DatLen;
   buf += sizeof(JAMBINSUBFIELD);
   totalsize += (u16)sizeof(JAMBINSUBFIELD);
   memset(tempStr, 0, sizeof(tempStrType));
   memcpy(tempStr, buf, min(size,100));
   helpPtr = tempStr;
   while ( !strnicmp (helpPtr, "RE:", 3) ||
           !strnicmp (helpPtr, "(R)", 3) ||
           !strnicmp (helpPtr, "RE^", 3) )
   {  helpPtr += 3;
      if ( *(helpPtr-1) == '^' )
      {  while ( *helpPtr && *helpPtr != ':' )
            ++helpPtr;
      }
      while ( *helpPtr == ' ' )
         helpPtr++;
   }
   slen = (u16)strlen(helpPtr);
   if ( slen < size )
   {  size -= slen;
      memmove(buf, buf+size, ((u16)*bufsize)-totalsize-size);
      *bufsize -= size;
      buf -= sizeof(JAMBINSUBFIELD);
#ifdef __32BIT__
      ((JAMBINSUBFIELD *)buf)->DatLen -= size;
#else
      (u16)(((JAMBINSUBFIELD *)buf)->DatLen) -= size;
#endif
      return 1;
   }
   return 0;
}



char *expJAMname(char *basename, char *ext)
{
   static tempStrType tempStr[2];
   static count = 0;
   if ( count )
      count = 0;
   else
      count = 1;
   strcpy(stpcpy(stpcpy(tempStr[count], basename), "."), ext);
   return tempStr[count];
}


#define TXTBUFSIZE 0x7000 /* moet 16-voud zijn i.v.m. lastread buf */
#define LRSIZE     0x3ff0 /* 'record' is u16, max 7ff0 */
#define MSGIDNUM   0x3ff0 /* 'record' is u32 */


static JAMHDRINFO  headerInfo;
static JAMHDR      headerRec;
static JAMIDXREC   indexRec;
static char        *buf = NULL;
static u16         *lrBuf = NULL;
static u32         *msgidBuf = NULL;
static u32         *replyBuf = NULL;

/*
   SW_K - Delete all messages in board
   SW_B - Keep backup files
   SW_D - Delete selected messages
   SW_R - Remove Re:
   SW_P - Pack message base: WERKT NIET WEGENS DOORLOPEN INDEX (0xFFFFFFFF!)
*/

s16 JAMmaint(rawEchoType *areaPtr, s32 switches, uchar *name, s32 *spaceSaved)
{
   s16           JAMerror = 0;
   u16           tabPos;
   fhandle       JHRhandle,
		 JDThandle,
		 JDXhandle,
		 JLRhandle,
		 JHRhandleNew,
		 JDThandleNew,
		 JDXhandleNew,
		 JLRhandleNew;
   u32           reply1st,
		 replyNext,
		 replyTo;
   u16           count;
   u32           msgNum = 0,
		 msgNumNew = 0;
   u32           keepSysOpCRC;
   u32           keepSysOpMsgNum;
   u32           oldBaseMsgNum;
   u16           maxMsg, curMsg;
   tempStrType   tempStr;
   s16           temp;
   static s16    useLocks = -1;
   s16           stat = 0;
   u32           bufCount,
		 delCount;
   time_t        msgTime;

   if ( buf == NULL &&
       (buf = malloc(TXTBUFSIZE)) == NULL )
      goto NoMem;

   if ( lrBuf == NULL &&
        (lrBuf = malloc(LRSIZE*2)) == NULL )
      goto NoMem;

   if ( msgidBuf == NULL &&
        (msgidBuf = malloc(MSGIDNUM*4)) == NULL )
      goto NoMem;

   if ( replyBuf == NULL &&
        (replyBuf = malloc(MSGIDNUM*4)) == NULL )
   {
NoMem:
      logEntry("No enough memory available to pack JAM bases", LOG_ALWAYS, 2);
   }

   memset(lrBuf, 0, LRSIZE*2);
   memset(msgidBuf, 0, MSGIDNUM*4);
   memset(replyBuf, 0, MSGIDNUM*4);

   if ( (JHRhandle = fsopen(expJAMname(areaPtr->msgBasePath, EXT_HDR),
                            O_RDONLY|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1)) == -1 )
   {  goto jamx;
   }
   if ( (JDThandle = fsopen(expJAMname(areaPtr->msgBasePath, EXT_TXT),
                            O_RDONLY|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1)) == -1 )
   {  fsclose(JHRhandle);
      goto jamx;
   }
   if ( (JDXhandle = fsopen(expJAMname(areaPtr->msgBasePath, EXT_IDX),
                            O_RDONLY|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1)) == -1 )
   {  fsclose(JDThandle);
      fsclose(JHRhandle);
      goto jamx;
   }

   JLRhandle = fsopen(expJAMname(areaPtr->msgBasePath, EXT_LRD),
                      O_RDONLY|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1);

   if ( (JHRhandleNew = fsopen(expJAMname(areaPtr->msgBasePath, EXT_NEW_HDR),
                               O_CREAT|O_TRUNC|O_RDWR|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1)) == -1 )
   {  fsclose(JLRhandle);
      fsclose(JDXhandle);
      fsclose(JDThandle);
      fsclose(JHRhandle);
      goto jamx;
   }
   if ( (JDThandleNew = fsopen(expJAMname(areaPtr->msgBasePath, EXT_NEW_TXT),
                               O_CREAT|O_TRUNC|O_RDWR|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1)) == -1 )
   {  fsclose(JHRhandleNew);
      fsclose(JLRhandle);
      fsclose(JDXhandle);
      fsclose(JDThandle);
      fsclose(JHRhandle);
      goto jamx;
   }
   if ( (JDXhandleNew = fsopen(expJAMname(areaPtr->msgBasePath, EXT_NEW_IDX),
                               O_CREAT|O_TRUNC|O_RDWR|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1)) == -1 )
   {  fsclose(JDThandleNew);
      fsclose(JHRhandleNew);
      fsclose(JLRhandle);
      fsclose(JDXhandle);
      fsclose(JDThandle);
      fsclose(JHRhandle);
      goto jamx;
   }
   if ( (JLRhandleNew = fsopen(expJAMname(areaPtr->msgBasePath, EXT_NEW_LRD),
                               O_CREAT|O_TRUNC|O_RDWR|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1)) == -1 )
   {  fsclose(JDXhandleNew);
      fsclose(JDThandleNew);
      fsclose(JHRhandleNew);
      fsclose(JLRhandle);
      fsclose(JDXhandle);
      fsclose(JDThandle);
      fsclose(JHRhandle);
jamx: sprintf(tempStr, "JAM area %s was not found or was locked", areaPtr->areaName);
      logEntry(tempStr, LOG_ALWAYS, 0);
      return 1;
   }

   if ( useLocks )
   {  stat = lock(JHRhandle, 0L, 1L);
      if ( useLocks == -1 )
      {  useLocks = 1;
         if ( stat == -1 && errno == EINVAL )
         {  if ( !config.mbOptions.mbSharing )
               useLocks = 0;
            else
            {  newLine();
               logEntry("SHARE is required when Message Base Sharing is enabled", LOG_ALWAYS, 0);
               fsclose(JLRhandleNew);
               fsclose(JDXhandleNew);
               fsclose(JDThandleNew);
               fsclose(JHRhandleNew);
               fsclose(JDXhandle);
               fsclose(JLRhandle);
               fsclose(JDThandle);
               fsclose(JHRhandle);
               return 1;
            }
         }
      }
   }
   sprintf(tempStr, "Processing JAM area %s... ", areaPtr->areaName);
   printString(tempStr);
   tabPos = getTab();

   printString("Updating");
   updateCurrLine();

   read(JHRhandle, &headerInfo, sizeof(JAMHDRINFO));

   if ( areaPtr->msgs && headerInfo.ActiveMsgs > areaPtr->msgs )
      delCount = headerInfo.ActiveMsgs - areaPtr->msgs;
   else
      delCount = 0;
   oldBaseMsgNum = headerInfo.BaseMsgNum;
   headerInfo.BaseMsgNum = 1;
   headerInfo.ModCounter++;
   headerInfo.ActiveMsgs = 0;
   if ( write(JHRhandleNew, &headerInfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO) )
   {  newLine();
      logEntry("JAMmaint: Can't write new header", LOG_ALWAYS, 0);
      JAMerror = -1;
   }

   keepSysOpMsgNum = 0xFFFFFFFFL;
   if ( areaPtr->options.sysopRead && JLRhandle != -1 )
   {  keepSysOpCRC = crc32jam(strlwr(strcpy(tempStr, name)));
      while ( keepSysOpMsgNum == 0xFFFFFFFFL && !eof(JLRhandle) &&
              ((temp = read(JLRhandle, buf, TXTBUFSIZE)) != -1) )
      {  temp >>= 4;
         for ( count = 0; count < temp; count++ )
         {  if ( ((JAMLREAD *)buf)[count].UserCRC == keepSysOpCRC )
            {  keepSysOpMsgNum = ((JAMLREAD *)buf)[count].HighReadMsg-oldBaseMsgNum+1;
               break;
            }
	 }
      }
   }

   while ( !JAMerror && !eof(JDXhandle) )
   {  if ( read(JDXhandle, &indexRec, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC) )
      {  newLine();
         logEntry("JAMmaint: Can't read old index", LOG_ALWAYS, 0);
         JAMerror = -1;
         break; /* skip rest of loop */
      }
      msgNum++;

      if ( indexRec.UserCRC != 0xffffffffL || indexRec.HdrOffset != 0xffffffffL )
      {  if ( fmseek(JHRhandle, indexRec.HdrOffset, SEEK_SET, 5) < 0 )
         {  JAMerror = -1;
            break;
         }
         if ( read(JHRhandle, &headerRec, sizeof(JAMHDR)) != sizeof(JAMHDR) )
         {  newLine();
            logEntry("JAMmaint: Can't read old header", LOG_ALWAYS, 0);
            JAMerror = -1;
            break; /* skip rest of loop */
         }

         if ( areaPtr->options.arrivalDate && headerRec.DateProcessed )
            msgTime = JAMtime(headerRec.DateProcessed);
         else
            msgTime = JAMtime(headerRec.DateWritten);

      /* TEST MSG AND MARK DELETED */

         if ( ((switches & SW_D) &&
               (delCount ||
                (areaPtr->days &&
                 areaPtr->days*86400L + msgTime < startTime) ||
                ((headerRec.Attribute & MSG_READ) &&
                 (areaPtr->daysRcvd &&
                  areaPtr->daysRcvd*86400L + msgTime < startTime)))) ||
              (switches & SW_K) )
         {
            if ( headerRec.MsgNum < keepSysOpMsgNum )
            {  headerRec.Attribute |= MSG_DELETED;
               if ( delCount )
                  delCount--;
            }
         }

         if ( !(headerRec.Attribute & MSG_DELETED) )
         {  if ( fmseek(JDThandle, headerRec.TxtOffset, SEEK_SET, 6) < 0 )
            {  JAMerror = -1;
               break;
            }
            bufCount = headerRec.TxtLen;
            headerRec.TxtOffset = tell(JDThandleNew);

            while ( !JAMerror && bufCount )
            {  if ( (temp = read(JDThandle, buf, (u16)min(bufCount, TXTBUFSIZE))) !=
                                                 (u16)min(bufCount, TXTBUFSIZE) )
               {  newLine();
                  logEntry("JAMmaint: Can't read old message text", LOG_ALWAYS, 0);
                  JAMerror = -1;
                  break;
               }
               if ( write(JDThandleNew, buf, temp) != temp )
               {  newLine();
                  logEntry("JAMmaint: Can't write new message text", LOG_ALWAYS, 0);
                  JAMerror = -1;
                  break;
               }
               if ( bufCount < TXTBUFSIZE )
                  bufCount = 0;
               else
                  bufCount -= TXTBUFSIZE;
            }
            if ( JAMerror )
               break;
            indexRec.HdrOffset = tell(JHRhandleNew);

            if ( headerRec.Attribute & MSG_DELETED )
            {  indexRec.UserCRC   = 0xffffffffL;
               indexRec.HdrOffset = 0xffffffffL;
            }
            else
            {  headerInfo.ActiveMsgs++;
               if (msgNumNew < MSGIDNUM)
               {  msgidBuf[(u16)msgNumNew] = headerRec.MsgIdCRC;
                  replyBuf[(u16)msgNumNew] = headerRec.ReplyCRC;
               }
            }

            headerRec.MsgNum = ++msgNumNew;
            if ( write(JDXhandleNew, &indexRec, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC) )
            {  newLine();
               logEntry("JAMmaint: Can't write new index", LOG_ALWAYS, 0);
               JAMerror = -1;
               break;
            }
            if ( write(JHRhandleNew, &headerRec, sizeof(JAMHDR)) != sizeof(JAMHDR) )
            {  newLine();
               logEntry("JAMmaint: Can't write new header", LOG_ALWAYS, 0);
               JAMerror = -1;
               break;
            }
            if ( msgNum < LRSIZE )
               lrBuf[(u16)msgNum] = msgNumNew >= 0x0000ffff ? 0xffff : (u16)msgNumNew;

            bufCount = headerRec.SubfieldLen;
            while ( bufCount )
            {
               if ( (temp = read(JHRhandle, buf, (u16)min(bufCount, TXTBUFSIZE))) !=
                                                 (u16)min(bufCount, TXTBUFSIZE) )
               {  newLine();
                  logEntry("JAMmaint: Can't read old header", LOG_ALWAYS, 0);
                  JAMerror = -1;
                  break;
               }

               /* Remove Re: */
               if ( (switches & SW_R) && headerRec.SubfieldLen < TXTBUFSIZE )
               {  if ( JAMremoveRe(buf, &headerRec.SubfieldLen) )
                  {  if ( lseek(JHRhandleNew, -(s32)sizeof(JAMHDR), SEEK_CUR) < 0 )
                     {  JAMerror = -1;
                        break;
                     }
                     if ( write(JHRhandleNew, &headerRec, sizeof(JAMHDR)) != sizeof(JAMHDR) )
                     {  newLine();
                        logEntry("JAMmaint: Can't write new header", LOG_ALWAYS, 0);
                        JAMerror = -1;
                        break;
                     }
                     temp = (u16)(bufCount = headerRec.SubfieldLen);
                  }
               }
               if ( write(JHRhandleNew, buf, temp) != temp )
               {  newLine();
                  logEntry("JAMmaint: Can't write new header", LOG_ALWAYS, 0);
                  JAMerror = -1;
                  break;
               }
               if ( bufCount < TXTBUFSIZE )
                  bufCount = 0;
               else
                  bufCount -= TXTBUFSIZE;
            }
         }
      }
   }
   lseek(JHRhandleNew, 0, SEEK_SET);
   if ( write(JHRhandleNew, &headerInfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO) )
   {  newLine();
      logEntry("JAMmaint: Can't write new header", LOG_ALWAYS, 0);
      JAMerror = -1;
   }

   /* Update reply chains */

   gotoTab(tabPos);
   printString("Reply chains");
   updateCurrLine();

   if ( msgNumNew > MSGIDNUM )
      maxMsg = MSGIDNUM;
   else
      maxMsg = (u16)msgNumNew;

   curMsg = 0;
   lseek(JDXhandleNew, 0, SEEK_SET);
   while ( !JAMerror && !eof(JDXhandleNew) && ++curMsg <= maxMsg )
   {  if ( read(JDXhandleNew, &indexRec, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC) )
      {  newLine();
         logEntry("JAMmaint: Can't read new index", LOG_ALWAYS, 0);
         JAMerror = -1;
         break;
      }
      if ( indexRec.UserCRC != 0xffffffffL || indexRec.HdrOffset != 0xffffffffL)
      {  if ( fmseek(JHRhandleNew, indexRec.HdrOffset, SEEK_SET, 7) < 0 )
         {  JAMerror = -1;
            break;
         }
         if ( read(JHRhandleNew, &headerRec, sizeof(JAMHDR)) != sizeof(JAMHDR) )
         {  newLine();
            logEntry("JAMmaint: Can't read new header", LOG_ALWAYS, 0);
            JAMerror = -1;
            break;
         }

         reply1st = replyNext = replyTo = 0;

         if ( headerRec.MsgIdCRC != 0xffffffffL )
         {  count = 0;
            while ( count < maxMsg && headerRec.MsgIdCRC != replyBuf[count] )
               count++;
            if ( count < maxMsg )
               reply1st = count + 1;
         }
         if ( headerRec.ReplyCRC != 0xffffffffL )
         {  count = 0;
            while ( count < maxMsg && headerRec.ReplyCRC != msgidBuf[count] )
               count++;
            if ( count < maxMsg )
            {  replyTo = count+1/*headerInfo.BaseMsgNum*/;
               count = curMsg;
               while ( count < maxMsg && (headerRec.ReplyCRC != replyBuf[count] ||
                                          headerRec.MsgIdCRC == msgidBuf[count]) )
                  count++;
               if ( count < maxMsg )
                  replyNext = count+1/*headerInfo.BaseMsgNum*/;
            }
         }
         if ( headerRec.Reply1st  != reply1st ||
              headerRec.ReplyTo   != replyTo  ||
              headerRec.ReplyNext != replyNext )
         {  headerRec.Reply1st  = reply1st;
            headerRec.ReplyTo   = replyTo;
            headerRec.ReplyNext = replyNext;

            if ( fmseek(JHRhandleNew, indexRec.HdrOffset, SEEK_SET, 8) < 0 )
            {  JAMerror = -1;
               break;
            }
            if ( write(JHRhandleNew, &headerRec, sizeof(JAMHDR)) != sizeof(JAMHDR) )
            {  newLine();
               logEntry("JAMmaint: Can't write new header", LOG_ALWAYS, 0);
               JAMerror = -1;
               break;
            }
         }
      }
   }

   /* Update last read info */
   if ( !JAMerror && JLRhandle != -1 )
   {
      gotoTab(tabPos);
      printStringFill("LastRead");
      updateCurrLine();

      if ( !JAMerror )
         for ( count = 1; count < LRSIZE; count++ )
            if ( !lrBuf[count] )
               lrBuf[count] = lrBuf[count-1];

      lseek(JLRhandle, 0, SEEK_SET);
      while ( !JAMerror && !eof(JLRhandle) &&
              (temp = read(JLRhandle, buf, TXTBUFSIZE)) != -1 )
      {  temp >>= 4;
         for ( count = 0; count < temp; count++ )
         {  msgNum = ((JAMLREAD *)buf)[count].LastReadMsg-oldBaseMsgNum;
            ((JAMLREAD *)buf)[count].LastReadMsg = lrBuf[(msgNum >= LRSIZE-1) ? LRSIZE-1 : (u16)msgNum] + headerInfo.BaseMsgNum;
            msgNum = ((JAMLREAD *)buf)[count].HighReadMsg-oldBaseMsgNum;
            ((JAMLREAD *)buf)[count].HighReadMsg = lrBuf[(msgNum >= LRSIZE-1) ? LRSIZE-1 : (u16)msgNum] + headerInfo.BaseMsgNum;
         }
         if ( write(JLRhandleNew, buf, temp<<4) != (temp<<4) )
         {  newLine();
            logEntry("JAMmaint: Can't write new last read file", LOG_ALWAYS, 0);
            JAMerror = -1;
            break;
         }
      }
   }
   *spaceSaved += filelength(JDXhandle) - filelength(JDXhandleNew) +
                  filelength(JDThandle) - filelength(JDThandleNew) +
                  filelength(JHRhandle) - filelength(JHRhandleNew);
   fsclose(JLRhandleNew);
   fsclose(JDXhandleNew);
   fsclose(JDThandleNew);
   fsclose(JHRhandleNew);
   fsclose(JLRhandle);
   fsclose(JDXhandle);
   fsclose(JDThandle);
   fsclose(JHRhandle);

   gotoTab(tabPos);
   if ( JAMerror )
   {
     fsunlink(expJAMname(areaPtr->msgBasePath, EXT_NEW_HDR), 1);
     fsunlink(expJAMname(areaPtr->msgBasePath, EXT_NEW_TXT), 1);
     fsunlink(expJAMname(areaPtr->msgBasePath, EXT_NEW_IDX), 1);
     fsunlink(expJAMname(areaPtr->msgBasePath, EXT_NEW_LRD), 1);
     newLine();
     sprintf(tempStr, "Disk problems during JAM base maintenance, area %s", areaPtr->areaName);
     logEntry(tempStr, LOG_ALWAYS, 0);
   }
   else
   {
     fsunlink(expJAMname(areaPtr->msgBasePath, EXT_BCK_HDR), 1);
     fsunlink(expJAMname(areaPtr->msgBasePath, EXT_BCK_TXT), 1);
     fsunlink(expJAMname(areaPtr->msgBasePath, EXT_BCK_IDX), 1);
     fsunlink(expJAMname(areaPtr->msgBasePath, EXT_BCK_LRD), 1);

     if ( switches & SW_B )
     {
       fsrename(expJAMname(areaPtr->msgBasePath, EXT_HDR), expJAMname(areaPtr->msgBasePath, EXT_BCK_HDR), 1);
       fsrename(expJAMname(areaPtr->msgBasePath, EXT_TXT), expJAMname(areaPtr->msgBasePath, EXT_BCK_TXT), 1);
       fsrename(expJAMname(areaPtr->msgBasePath, EXT_IDX), expJAMname(areaPtr->msgBasePath, EXT_BCK_IDX), 1);
       fsrename(expJAMname(areaPtr->msgBasePath, EXT_LRD), expJAMname(areaPtr->msgBasePath, EXT_BCK_LRD), 1);
     }
     else
     {
       fsunlink(expJAMname(areaPtr->msgBasePath, EXT_HDR), 1);
       fsunlink(expJAMname(areaPtr->msgBasePath, EXT_TXT), 1);
       fsunlink(expJAMname(areaPtr->msgBasePath, EXT_IDX), 1);
       fsunlink(expJAMname(areaPtr->msgBasePath, EXT_LRD), 1);
     }
     fsrename(expJAMname(areaPtr->msgBasePath, EXT_NEW_HDR), expJAMname(areaPtr->msgBasePath, EXT_HDR), 1);
     fsrename(expJAMname(areaPtr->msgBasePath, EXT_NEW_TXT), expJAMname(areaPtr->msgBasePath, EXT_TXT), 1);
     fsrename(expJAMname(areaPtr->msgBasePath, EXT_NEW_IDX), expJAMname(areaPtr->msgBasePath, EXT_IDX), 1);
     fsrename(expJAMname(areaPtr->msgBasePath, EXT_NEW_LRD), expJAMname(areaPtr->msgBasePath, EXT_LRD), 1);

     printStringFill("Ready");
     newLine();
   }
   return JAMerror;
}



