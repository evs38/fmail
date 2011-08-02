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

 
#include <stdio.h>
#include <string.h>
#include <dir.h>
#include <dos.h>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "fmail.h"
#include "ftlog.h"
#include "output.h"
#include "areainfo.h" /* needed for utils.h */
#include "utils.h"
#include "crc.h"
#include "hudson.h"



extern configType config;
extern time_t     startTime;

fhandle msgHdrHandle;
fhandle msgTxtHandle;
fhandle msgToIdxHandle;
fhandle msgIdxHandle;

fhandle lockHandle;


char *expandNameH (char *fileName)
{
   static tempStrType expandStr;

   strcpy (expandStr, config.bbsPath);
   strcat (expandStr, fileName);
   strcat (expandStr, "."MBEXTN);
   return (expandStr);
}



s16 testMBUnlockNow (void)
{
   static s16 time, date;
   tempStrType tempStr;
   struct ffblk ffblkMBU;
   s16 unlock = 0;

   if (config.mbOptions.mbSharing)
   {
      strcpy (tempStr, config.bbsPath);
      strcat (tempStr, "MBUNLOCK.NOW");
      if (findfirst (tempStr, &ffblkMBU, 0))
      {
         time = 0;
         date = 0;
      }
      else
      {
         unlock = ((time != ffblkMBU.ff_ftime) || (date != ffblkMBU.ff_fdate));
         time = ffblkMBU.ff_ftime;
         date = ffblkMBU.ff_fdate;
      }
   }
   return (unlock);
}



void setMBUnlockNow (void)
{
   tempStrType tempStr;

   if (config.mbOptions.mbSharing)
   {
      strcpy (tempStr, config.bbsPath);
      strcat (tempStr, "MBUNLOCK.NOW");
      close (open(tempStr, O_RDWR|O_CREAT|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE));
      testMBUnlockNow ();
   }
}




s16 lockMB (void)
{
   tempStrType tempStr;
   time_t      time1,
               time2;

   strcpy (tempStr, config.bbsPath);
   strcat (tempStr, "MSGINFO."MBEXTN);

   if ((lockHandle = open(tempStr, O_RDWR|O_CREAT|O_BINARY|O_DENYNONE,
                                   S_IREAD|S_IWRITE)) == -1)
   {
      logEntry ("Can't open file MsgInfo."MBEXTN" for output", LOG_ALWAYS, 0);
      return (1);
   }

   testMBUnlockNow ();
   if ((lock (lockHandle, sizeof(infoRecType) + 1, 1) == -1) &&
       (_doserrno == 0x21))
   {
      printString ("Retrying to lock the message base\n\n");
      setMBUnlockNow ();
      time (&time1);

      do
      {
         time (&time2);
         _doserrno = 0;
      }
      while ((lock (lockHandle, sizeof(infoRecType) + 1, 1) == -1) &&
             (_doserrno == 0x21) &&
             (time2-time1 < 15));
      if (_doserrno == 0x21)
      {
         logEntry ("Can't lock the message base for update", LOG_ALWAYS, 0);
         close (lockHandle);
         return (1);
      }
   }
   return (0);
}



void unlockMB (void)
{
   if (config.mbOptions.mbSharing)
   {
      unlock (lockHandle, sizeof(infoRecType) + 1, 1);
   }
   close (lockHandle);
}



void openBBSWr ()
{
   if ((msgHdrHandle = open(expandNameH("MSGHDR"),
                            O_RDWR|O_CREAT|O_BINARY|O_DENYNONE,
                            S_IREAD|S_IWRITE)) == -1)
   {
      logEntry ("Can't open message base files for output", LOG_ALWAYS, 1);
   }
   lseek (msgHdrHandle, 0, SEEK_END);

   if ((msgTxtHandle = open(expandNameH("MSGTXT"),
                            O_RDWR|O_CREAT|O_BINARY|O_DENYNONE,
                            S_IREAD|S_IWRITE)) == -1)
   {
      logEntry ("Can't open message base files for output", LOG_ALWAYS, 1);
   }
   lseek (msgTxtHandle, 0, SEEK_END);

   if ((msgToIdxHandle = open(expandNameH("MSGTOIDX"),
                              O_RDWR|O_CREAT|O_BINARY|O_DENYNONE,
                              S_IREAD|S_IWRITE)) == -1)
   {
      logEntry ("Can't open message base files for output", LOG_ALWAYS, 1);
   }
   lseek (msgToIdxHandle, 0, SEEK_END);

   if ((msgIdxHandle = open(expandNameH("MSGIDX"),
                            O_RDWR|O_CREAT|O_BINARY|O_DENYNONE,
                            S_IREAD|S_IWRITE)) == -1)
   {
      logEntry ("Can't open message base files for output", LOG_ALWAYS, 1);
   }
   lseek (msgIdxHandle, 0, SEEK_END);
}



s16 writeBBS (internalMsgType *message, u16 boardNum, s16 isNetmail)
{
   msgHdrRec   msgRa;
   msgIdxRec   msgIdx;
   msgTxtRec   msgTxt;
   u16         msgTxtRecNum;
   u16         msgHdrRecNum;
   infoRecType infoRec;
   char *textPtr,
        *helpPtr;
   fhandle msgInfoHandle;
   fhandle mailBBSHandle;
   struct date dateRec;
   struct time timeRec;

   memset (&msgRa, 0, sizeof(msgHdrRec));

   if (boardNum)
   {
      if (lockMB()) return (1);

      if (((msgInfoHandle = open(expandNameH("MSGINFO"),
                                 O_RDWR|O_CREAT|O_BINARY|O_DENYNONE,
											S_IREAD|S_IWRITE)) == -1) ||
			 (_read (msgInfoHandle, &infoRec, sizeof(infoRecType)) != sizeof(infoRecType)))
		{
			 memset (&infoRec, 0, sizeof(infoRecType));
		}

		msgRa.ReplyTo    = 0;
		msgRa.SeeAlsoNum = 0;
		msgRa.TRead      = 0;
		msgRa.NetAttr    = 0;
                msgRa.StartRec   = msgTxtRecNum = (u16)(filelength(msgTxtHandle) >> 8);
		msgRa.Board      = boardNum;
                msgHdrRecNum = (u16)(filelength(msgHdrHandle)/sizeof(msgHdrRec));

                if ((s16)(msgRa.MsgNum = ++infoRec.HighMsg) < 0)
		{
			logEntry ("Highest allowed message number has been reached", LOG_ALWAYS, 0);
			unlockMB();
			return (1);
		}

		textPtr = message->text;
		while (*textPtr)
		{
			if ((helpPtr = memccpy (msgTxt.txtStr, textPtr, 0, 255)) == NULL)
			{
				msgTxt.txtLength = 255;
				textPtr += 255;
			}
			else
			{
                                textPtr += (msgTxt.txtLength = (u16)(helpPtr - 1 - msgTxt.txtStr));
         }
         if (!++msgTxtRecNum)
         {
            logEntry ("Maximum message base size has been reached", LOG_ALWAYS, 0);
            newLine ();
            unlockMB();
            return (1);
         }
         if (_write (msgTxtHandle, &msgTxt, 256) != 256)
         {
            unlockMB();
            return (1);
         }
         msgRa.NumRecs++;
      }

      gettime (&timeRec);
      getdate (&dateRec);

      sprintf (&msgRa.ptLength, "\x05%02d:%02d\x08%02d-%02d-%02d",
                                timeRec.ti_hour, timeRec.ti_min,
                                dateRec.da_mon,  dateRec.da_day,
                                dateRec.da_year%100);

      msgRa.MsgAttr = RA_LOCAL | ((message->attribute & PRIVATE) ? RA_PRIVATE : 0);

      if (isNetmail)
      {
         msgRa.MsgAttr |= RA_IS_NETMAIL|RA_NET_OUT;

         msgRa.NetAttr = ((message->attribute & KILLSENT) ? RA_KILLSENT : 0) |
                         ((message->attribute &    CRASH) ? RA_CRASH    : 0) |
                         ((message->attribute & FILE_ATT) ? RA_FILE_ATT : 0);

         msgRa.OrigZone = message->srcNode.zone;
         msgRa.OrigNet  = message->srcNode.net;
         msgRa.OrigNode = message->srcNode.node;
         msgRa.DestZone = message->destNode.zone;
         msgRa.DestNet  = message->destNode.net;
         msgRa.DestNode = message->destNode.node;
         msgRa.Cost     = message->cost;

         mailBBSHandle = open(expandNameH("NETMAIL"),
                              O_WRONLY|O_CREAT|O_APPEND|O_BINARY|O_DENYWRITE,
                              S_IREAD|S_IWRITE);
      }
      else
      {
         msgRa.MsgAttr |= RA_ECHO_OUT;

         mailBBSHandle = open(expandNameH("ECHOMAIL"),
                              O_WRONLY|O_CREAT|O_APPEND|O_BINARY|O_DENYWRITE,
                              S_IREAD|S_IWRITE);
      }
      if (mailBBSHandle != -1)
      {
         write (mailBBSHandle, &msgHdrRecNum, 2);
         close (mailBBSHandle);
      }

      msgRa.wtLength = strlen(message->toUserName);
      strncpy (msgRa.WhoTo, message->toUserName, 35);
      msgRa.wfLength = strlen(message->fromUserName);
      strncpy (msgRa.WhoFrom, message->fromUserName, 35);
      msgRa.sjLength = strlen(message->subject);
      strncpy (msgRa.Subj, message->subject, 72);

      if ((msgRa.sjLength) <= 56)
      {
         msgRa.subjCrc    = crc32alpha(config.mbOptions.removeRe?
                                       message->subject:removeRe(message->subject));

         msgRa.wrTime     = msgRa.recTime = startTime;

         time(&msgRa.recTime);

         msgRa.checkSum = CS_SECURITY ^ msgRa.subjCrc
                                      ^ msgRa.wrTime
                                      ^ msgRa.recTime;
      }

      msgIdx.MsgNum = msgRa.MsgNum;
      msgIdx.Board  = msgRa.Board;

      if ((_write (msgHdrHandle, &msgRa,
                                 sizeof(msgHdrRec)) != sizeof(msgHdrRec)) ||
          (_write (msgIdxHandle, &msgIdx,
                                 sizeof(msgIdxRec)) != sizeof(msgIdxRec)) ||
          (_write (msgToIdxHandle, &msgRa.wtLength,
                                   sizeof(msgToIdxRec)) != sizeof(msgToIdxRec)))
      {
         unlockMB();
         return (1);
      }

      infoRec.ActiveMsgs[msgRa.Board-1]++;
      infoRec.TotalActive++;

      if ((lseek(msgInfoHandle, 0, SEEK_SET) == -1) ||
          (write (msgInfoHandle, &infoRec, sizeof(infoRecType)) == -1))
      {
         logEntry ("Can't open file MsgInfo."MBEXTN" for output", LOG_ALWAYS, 1);
      }
      close (msgInfoHandle);

      unlockMB();
  }
    return (0);
}



void closeBBS (void)
{
   close (msgHdrHandle);
   close (msgTxtHandle);
   close (msgIdxHandle);
   close (msgToIdxHandle);
}
