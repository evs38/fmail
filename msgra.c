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

#include <dir.h>
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"

#include "msgra.h"

#include "areainfo.h"
#include "config.h"
#include "crc.h"
#include "dups.h"
#include "jamfun.h"
#include "log.h"
#include "msgmsg.h"
#include "msgpkt.h"
#include "mtask.h"
#include "utils.h"

u16 HDR_BUFSIZE = 104;
u16 TXT_BUFSIZE = 200;

extern time_t            startTime;
extern cookedEchoType   *echoAreaList;
extern echoToNodePtrType echoToNode[MAX_AREAS];

extern internalMsgType  *message;       // rescan
extern cookedEchoType   *echoAreaList;  // rescan
extern u16               echoCount;     // rescan

u16             crcCount;
#ifdef GOLDBASE
u32             msgTxtRecNum;
#else
u16             msgTxtRecNum;
#endif
fhandle         msgHdrHandle;
fhandle         msgTxtHandle;
fhandle         msgToIdxHandle;
fhandle         msgIdxHandle;
static fhandle  lockHandle;
infoRecType	infoRec;
infoRecType     infoRecValid;
u16             hdrBufCount;
u16             txtBufCount;
msgHdrRec       *msgHdrBuf;
msgIdxRec       *msgIdxBuf;
msgToIdxRec     *msgToIdxBuf;
msgTxtRec       *msgTxtBuf;
#ifdef GOLDBASE
u32             raHdrRecValid;
u32             raTxtRecValid;
#else
u16             raHdrRecValid;
u16             raTxtRecValid;
#endif
u16             secondsInc = 0;

extern globVarsType globVars;
extern configType   config;

extern char    *months;
extern u16      echoCount;
extern char    *version;

static void readMsgInfo (u16);
static void writeMsgInfo(u16);



#include "hudson_shared.c"
//---------------------------------------------------------------------------
void initBBS(void)
{
  off_t fsize = fileSize(expandNameHudson("MSGHDR", 0));

  if (fsize >= 0)
#ifdef GOLDBASE
    globVars.origTotalMsgBBS = fsize / sizeof(msgHdrRec);
#else
    globVars.origTotalMsgBBS = (u16)(fsize / sizeof(msgHdrRec));
#endif
  else
    globVars.origTotalMsgBBS = 0;

  if (  config.mbOptions.mbSharing
     && (fsize = fileSize(expandNameHudson("MSGTXT", 1))) >= 0
     )
#ifdef GOLDBASE
    globVars.baseTotalTxtRecs = fsize / 256;
#else
    globVars.baseTotalTxtRecs = (u16)(fsize / 256);
#endif
  else
    globVars.baseTotalTxtRecs = 0;

  HDR_BUFSIZE = (104>>3) *
#if defined(__FMAILX__) || defined(__32BIT__)
	 		    8;
#else
			    (8-((config.bufSize==0) ? 0 :
			       ((config.bufSize==1) ? 3 :
			       ((config.bufSize==2) ? 5 :
			       ((config.bufSize==3) ? 6 : 7)))));
#endif
  TXT_BUFSIZE = (200>>3) *
#if defined(__FMAILX__) || defined(__32BIT__)
			     8;
#else
			    (8-((config.bufSize==0) ? 0 :
			       ((config.bufSize==1) ? 3 :
			       ((config.bufSize==2) ? 5 :
			       ((config.bufSize==3) ? 6 : 7)))));
#endif
}
//---------------------------------------------------------------------------
s16 multiUpdate(void)
{
   fhandle  srcTxtHandle;
   fhandle  destTxtHandle;
   fhandle  srcHdrHandle;
   fhandle  destHdrHandle;
   fhandle  destIdxHandle;
   fhandle  destToIdxHandle;
   int      recsRead;
   u16      count;
#ifdef GOLDBASE
   u32      msgHdrOffset;
   u32      msgTxtOffset;
   u32      msgNumOffset;
#else
   u16      msgHdrOffset;
   u16      msgTxtOffset;
   u16      msgNumOffset;
#endif
   tempStrType tempStr;
   char        *helpPtr;
   msgTxtRec   *txtBuf;
   msgHdrRec   *hdrBuf;
   msgIdxRec   *idxBuf;
   msgToIdxRec *toIdxBuf;
   infoRecType newInfoRec;

   if (config.mbOptions.mbSharing && access(expandNameHudson("MSGHDR", 0), 0) == 0)
   {
      logEntry("Updating actual message base files...", LOG_MSGBASE, 0);
      newLine();
//    flush();

      if (lockMB())
         return 1;

      lseek (lockHandle, 0, SEEK_SET);
      if (read(lockHandle, &newInfoRec, sizeof(infoRecType)) != sizeof(infoRecType))
         memset (&newInfoRec, 0, sizeof(infoRecType));

      msgNumOffset = newInfoRec.HighMsg;

      if ((srcHdrHandle = open(expandNameHudson("MSGHDR", 0), O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      {
         unlockMB();
         logEntry("Can't update the message base files", LOG_ALWAYS, 0);
         newLine();
         return 1;
      }

      if ((srcTxtHandle = open(expandNameHudson("MSGTXT", 0), O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      {
         close(srcHdrHandle);
         unlockMB();
         logEntry("Can't update the message base files", LOG_ALWAYS, 0);
         newLine();
         return 1;
      }

      helpPtr = stpcpy(tempStr, config.bbsPath);

      strcpy(helpPtr, "MSGHDR."MBEXTN);
      if ((destHdrHandle = open(tempStr, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      {
         close(srcTxtHandle);
         close(srcHdrHandle);
         unlockMB();
         logEntry("Can't update the message base files", LOG_ALWAYS, 0);
         newLine();
         return 1;
      }

      strcpy(helpPtr, "MSGIDX."MBEXTN);
      if ((destIdxHandle = open(tempStr, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      {
         close(destHdrHandle);
         close(srcTxtHandle);
         close(srcHdrHandle);
         unlockMB();
         logEntry("Can't update the message base files", LOG_ALWAYS, 0);
         newLine();
         return 1;
      }
      strcpy(helpPtr, "MSGTOIDX."MBEXTN);
      if ((destToIdxHandle = open(tempStr, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      {
         close(destIdxHandle);
         close(destHdrHandle);
         close(srcTxtHandle);
         close(srcHdrHandle);
         unlockMB();
         logEntry("Can't update the message base files", LOG_ALWAYS, 0);
         newLine();
         return 1;
      }

      strcpy(helpPtr, "MSGTXT."MBEXTN);
      if ((destTxtHandle = open(tempStr, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      {
         close(destToIdxHandle);
         close(destIdxHandle);
         close(destHdrHandle);
         close(srcTxtHandle);
      	 close(srcHdrHandle);
         unlockMB();
         logEntry("Can't update the message base files", LOG_ALWAYS, 0);
         newLine();
         return 1;
      }

#ifndef GOLDBASE
      if (filelength(srcTxtHandle) + filelength(destTxtHandle) >= 0x1000000L)
      {
         logEntry("Maximum message base size has been reached (text size)", LOG_ALWAYS, 0);
         sprintf(tempStr, "- new txt size: %lu, orig txt size: %lu", filelength(srcTxtHandle), filelength(destTxtHandle));
         logEntry(tempStr, LOG_ALWAYS, 0);
         close(destTxtHandle);
         close(destToIdxHandle);
         close(destIdxHandle);
         close(destHdrHandle);
         close(srcTxtHandle);
         close(srcHdrHandle);
         unlockMB();
         logEntry("Can't update the message base files", LOG_ALWAYS, 0);
         newLine();
         return 1;
      }
      msgHdrOffset = (u16)(filelength(destHdrHandle) / sizeof(msgHdrRec));
      msgTxtOffset = (u16)(filelength(destTxtHandle) >> 8);
#else
      msgHdrOffset = filelength(destHdrHandle) / sizeof(msgHdrRec);
      msgTxtOffset = filelength(destTxtHandle) >> 8;
#endif
      if ((txtBuf = (msgTxtRec*)malloc(TXT_BUFSIZE * 256)) == NULL)
      {
         close(destTxtHandle);
         close(destToIdxHandle);
         close(destIdxHandle);
         close(destHdrHandle);
         close(srcTxtHandle);
         close(srcHdrHandle);
         unlockMB();
         logEntry("Not enough memory to copy message base files", LOG_ALWAYS, 0);
         newLine();
         return 1;
      }

      lseek(srcTxtHandle, 0, SEEK_SET);
      lseek(destTxtHandle, ((u32)msgTxtOffset) << 8, SEEK_SET);

      while ((recsRead = (read(srcTxtHandle, txtBuf, TXT_BUFSIZE*256)+1)>>8) != 0)
      {
         if ((testMBUnlockNow()) ||
             (write (destTxtHandle, txtBuf, recsRead << 8) != (recsRead << 8)))
         {
            free(txtBuf);
            chsize(destTxtHandle, ((u32)msgTxtOffset) << 8);
            close(destTxtHandle);
            close(destToIdxHandle);
            close(destIdxHandle);
            close(destHdrHandle);
            close(srcTxtHandle);
            close(srcHdrHandle);
            unlockMB();
            logEntry("Can't update the message base files", LOG_ALWAYS, 0);
            newLine();
            return 1;
         }
      }
      free(txtBuf);

      hdrBuf   = (msgHdrRec  *)malloc(HDR_BUFSIZE * sizeof(msgHdrRec));
      idxBuf   = (msgIdxRec  *)malloc(HDR_BUFSIZE * sizeof(msgIdxRec));
      toIdxBuf = (msgToIdxRec*)malloc(HDR_BUFSIZE * sizeof(msgToIdxRec));

      if ((hdrBuf == NULL) || (idxBuf == NULL) || (toIdxBuf == NULL))
      {
         if (hdrBuf != NULL)
            free (hdrBuf);
         if (idxBuf != NULL)
            free (idxBuf);
         if (toIdxBuf != NULL)
            free (toIdxBuf);

         chsize (destTxtHandle, ((u32)msgTxtOffset) << 8);

         close(destTxtHandle);
         close(destToIdxHandle);
         close(destIdxHandle);
         close(destHdrHandle);
         close(srcTxtHandle);
         close(srcHdrHandle);
         unlockMB();
         logEntry("Not enough memory to copy message base files", LOG_ALWAYS, 0);
         newLine();
         return 1;
      }

      lseek(srcHdrHandle, 0, SEEK_SET);
      lseek(destHdrHandle,   msgHdrOffset*(u32)sizeof(msgHdrRec),   SEEK_SET);
      lseek(destIdxHandle,   msgHdrOffset*(u32)sizeof(msgIdxRec),   SEEK_SET);
      lseek(destToIdxHandle, msgHdrOffset*(u32)sizeof(msgToIdxRec), SEEK_SET);

      while ((recsRead = (read(srcHdrHandle, hdrBuf, HDR_BUFSIZE*sizeof(msgHdrRec))+1)/sizeof(msgHdrRec)) != 0)
      {
         for (count = 0; count < recsRead; count++)
         {
            hdrBuf[count].MsgNum   += msgNumOffset;
            hdrBuf[count].StartRec += msgTxtOffset;
            memcpy(&toIdxBuf[count], &hdrBuf[count].wtLength, 36);
            idxBuf[count].MsgNum = hdrBuf[count].MsgNum;
            idxBuf[count].Board  = hdrBuf[count].Board;
         }
#ifdef GOLDBASE
         if ((s32)hdrBuf[count - 1].MsgNum < 0)
#else
         if ((s16)hdrBuf[count - 1].MsgNum < 0)
#endif
         {
            logEntry("Highest allowed message number has been reached", LOG_ALWAYS, 0);
            free(hdrBuf);
            free(idxBuf);
            free(toIdxBuf);
            chsize(destIdxHandle,   msgHdrOffset*(u32)sizeof(msgIdxRec));
            chsize(destToIdxHandle, msgHdrOffset*(u32)sizeof(msgToIdxRec));
            chsize(destHdrHandle,   msgHdrOffset*(u32)sizeof(msgHdrRec));
            chsize(destTxtHandle,   ((u32)msgTxtOffset) << 8);
            close(destTxtHandle);
            close(destToIdxHandle);
            close(destIdxHandle);
            close(destHdrHandle);
            close(srcTxtHandle);
            close(srcHdrHandle);
            unlockMB();
            logEntry("Can't update the message base files", LOG_ALWAYS, 0);
            newLine();
            return 1;
         }
         if (  testMBUnlockNow()
            || (write(destHdrHandle  , hdrBuf  , recsRead * sizeof(msgHdrRec  )) != (int)(recsRead * sizeof(msgHdrRec  )))
            || (write(destIdxHandle  , idxBuf  , recsRead * sizeof(msgIdxRec  )) != (int)(recsRead * sizeof(msgIdxRec  )))
            || (write(destToIdxHandle, toIdxBuf, recsRead * sizeof(msgToIdxRec)) != (int)(recsRead * sizeof(msgToIdxRec)))
            )
         {
            free(hdrBuf);
            free(idxBuf);
            free(toIdxBuf);
            chsize(destIdxHandle,   msgHdrOffset*(u32)sizeof(msgIdxRec));
            chsize(destToIdxHandle, msgHdrOffset*(u32)sizeof(msgToIdxRec));
            chsize(destHdrHandle,   msgHdrOffset*(u32)sizeof(msgHdrRec));
            chsize(destTxtHandle,   ((u32)msgTxtOffset) << 8);
            close(destTxtHandle);
            close(destToIdxHandle);
            close(destIdxHandle);
            close(destHdrHandle);
            close(srcTxtHandle);
            close(srcHdrHandle);
            unlockMB();
            logEntry("Can't update the message base files", LOG_ALWAYS, 0);
            newLine();
            return 1;
         }
      }

      close(destTxtHandle);
      close(destToIdxHandle);
      close(destIdxHandle);
      close(destHdrHandle);
      close(srcTxtHandle);
      close(srcHdrHandle);

      free(hdrBuf);
      free(idxBuf);
      free(toIdxBuf);

      readMsgInfo(0);

      newInfoRec.LowMsg  = newInfoRec.LowMsg ? newInfoRec.LowMsg:infoRec.LowMsg+msgNumOffset;
      newInfoRec.HighMsg = infoRec.HighMsg+msgNumOffset;
      newInfoRec.TotalActive += infoRec.TotalActive;

      for (count = 0; count < MBBOARDS; count++)
         newInfoRec.ActiveMsgs[count] += infoRec.ActiveMsgs[count];

      lseek(lockHandle, 0, SEEK_SET);
      write(lockHandle, &newInfoRec, sizeof(infoRecType));

      unlockMB();

      Delete(config.bbsPath, "MSG*."MBEXTB);
   }
   return 0;
}
//---------------------------------------------------------------------------
static void readMsgInfo(u16 orgName)
{
   fhandle msgInfoHandle;

   if (((msgInfoHandle = open(expandNameHudson("MSGINFO", orgName), O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1) ||
        read (msgInfoHandle, &infoRec, sizeof(infoRecType)) != sizeof(infoRecType) )
      memset (&infoRec, 0, sizeof(infoRecType));

   close(msgInfoHandle);

   memcpy(&infoRecValid, &infoRec, sizeof(infoRecType));
}
//---------------------------------------------------------------------------
static void writeMsgInfo(u16 orgName)
{
   fhandle msgInfoHandle;

   if ( ((msgInfoHandle = open(expandNameHudson("MSGINFO", orgName), O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1) ||
        write (msgInfoHandle, &infoRecValid, sizeof(infoRecType)) == -1 )
      logEntry ("Can't open file MsgInfo."MBEXTN" for output", LOG_ALWAYS, 1);

   close(msgInfoHandle);
}
//---------------------------------------------------------------------------
void openBBSWr(u16 orgName)
{
   readMsgInfo(orgName);

   if (((msgHdrBuf   = (msgHdrRec  *)malloc(HDR_BUFSIZE * sizeof(msgHdrRec))) == NULL) ||
       ((msgIdxBuf   = (msgIdxRec  *)malloc(HDR_BUFSIZE * sizeof(msgIdxRec))) == NULL) ||
       ((msgToIdxBuf = (msgToIdxRec*)malloc(HDR_BUFSIZE * sizeof(msgToIdxRec))) == NULL) ||
       ((msgTxtBuf   = (msgTxtRec  *)malloc(TXT_BUFSIZE * 256)) == NULL))
      logEntry ("Not enough memory to allocate message base file buffers", LOG_ALWAYS, 2);

   if ((msgHdrHandle = open(expandNameHudson("MSGHDR", orgName), O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      logEntry("Can't open message base files for output", LOG_ALWAYS, 1);

   lseek(msgHdrHandle, 0, SEEK_END);

   if ((msgTxtHandle = open(expandNameHudson("MSGTXT", orgName), O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      logEntry ("Can't open message base files for output", LOG_ALWAYS, 1);

   lseek(msgTxtHandle, 0, SEEK_END);

   if ((msgToIdxHandle = open(expandNameHudson("MSGTOIDX", orgName), O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      logEntry ("Can't open message base files for output", LOG_ALWAYS, 1);

   lseek(msgToIdxHandle, 0, SEEK_END);

   if ((msgIdxHandle = open(expandNameHudson("MSGIDX", orgName), O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      logEntry ("Can't open message base files for output", LOG_ALWAYS, 1);

   lseek(msgIdxHandle, 0, SEEK_END);

#ifdef GOLDBASE
   raHdrRecValid = filelength (msgHdrHandle) / sizeof(msgHdrRec);
   raTxtRecValid = msgTxtRecNum = filelength(msgTxtHandle) >> 8;
#else
   raHdrRecValid = (u16)(filelength (msgHdrHandle) / sizeof(msgHdrRec));
   raTxtRecValid = msgTxtRecNum = (u16)(filelength(msgTxtHandle) >> 8);
#endif

   hdrBufCount = 0;
   txtBufCount = 0;
}
//---------------------------------------------------------------------------
#ifdef GOLDBASE
static s16 writeText (char *msgText, char *seenBy, char *path, u16 skip,
                      u32 *startRec, u16 *numRecs)
#else
static s16 writeText (char *msgText, char *seenBy, char *path, u16 skip,
                      u16 *startRec, u16 *numRecs)
#endif
{
   char *textPtr,
        *helpPtr;
   tempStrType tempStr;

   // Skip AREA tag
   if (skip && (strncmp (msgText, "AREA:", 5) == 0))
      if (*(msgText = strchr (msgText, '\r') + 1) == '\n')
         msgText++;

   // SEEN-BY handling

   if (*seenBy)
   {
      // Convert SEEN-BYs to kludge line

      helpPtr = seenBy;
      while ((helpPtr = findCLStr(helpPtr, "SEEN-BY: ")) != NULL)
      {
         memmove(helpPtr + 1, helpPtr, strlen(helpPtr) + 1);
         *helpPtr = 1;
         helpPtr += 8;
      }
      strcat(msgText, seenBy);
   }

   // Add PATH

   strcat(msgText, path);

   *startRec = msgTxtRecNum;
   *numRecs  = 0;
   textPtr   = msgText;

   while (*textPtr)
   {
      if (txtBufCount == TXT_BUFSIZE)
      {
         lseek(msgTxtHandle, 0, SEEK_END);
         if (write(msgTxtHandle, msgTxtBuf, TXT_BUFSIZE * 256) != TXT_BUFSIZE * 256)
            return 1;

         txtBufCount = 0;
      }

      if ((helpPtr = (char*)memccpy(msgTxtBuf[txtBufCount].txtStr, textPtr, 0, 255)) == NULL)
      {
         msgTxtBuf[txtBufCount++].txtLength = 255;
         textPtr += 255;
      }
      else
      {
         textPtr += (msgTxtBuf[txtBufCount].txtLength = (u16)(helpPtr - 1 - msgTxtBuf[txtBufCount].txtStr));
         txtBufCount++;
      }
#ifdef GOLDBASE
      if ( !(++msgTxtRecNum+globVars.baseTotalTxtRecs) )
#else
      if ( !(u16)(++msgTxtRecNum+globVars.baseTotalTxtRecs) )
#endif
      {
         newLine();
         logEntry("Maximum message base size has been reached (# records)", LOG_ALWAYS, 0);
         sprintf(tempStr, "- new txt recs: %hu, orig txt recs: %hu", msgTxtRecNum, globVars.baseTotalTxtRecs);
         logEntry(tempStr, LOG_ALWAYS, 0);
         return 1;
      }
      (*numRecs)++;
   }
   return 0;
}
//---------------------------------------------------------------------------
s16 writeBBS (internalMsgType *message, u16 boardNum, u16 impSeenBy)
{
   msgHdrRec msgRa;

// returnTimeSlice(0);

   if (boardNum)
   {
      msgRa.ReplyTo    = 0;
      msgRa.SeeAlsoNum = 0;
      msgRa.TRead      = 0;
      msgRa.NetAttr    = 0;

      msgRa.Board  = boardNum;
#ifdef GOLDBASE
      if ( !(msgRa.MsgNum = ++infoRec.HighMsg) )
#else
      if ( (s16)(msgRa.MsgNum = ++infoRec.HighMsg) < 0 )
#endif
      {
         logEntry ("Highest allowed message number has been reached", LOG_ALWAYS, 0);
         return 1;
      }
      if ( !infoRec.LowMsg || msgRa.MsgNum < infoRec.LowMsg )
         infoRec.LowMsg = msgRa.MsgNum;

      if (!impSeenBy)
      {
        *message->normSeen = 0;
        *message->normPath = 0;
      }

      if (writeText(message->text, message->normSeen, message->normPath,
                     (boardNum != config.badBoard) && (boardNum != config.dupBoard),
                     &msgRa.StartRec,
                     &msgRa.NumRecs))
         return 1;

      sprintf((char*)&msgRa.ptLength, "\x05%02d:%02d\x08%02d-%02d-%02d"
                                    , message->hours, message->minutes, message->month, message->day, message->year % 100);

      msgRa.MsgAttr = (message->attribute & PRIVATE) ? RA_PRIVATE : 0;

      if (isNetmailBoard(boardNum)) /* Netmail board ? */
      {
         msgRa.MsgAttr |= RA_IS_NETMAIL;

         msgRa.NetAttr = ((message->attribute & KILLSENT)    ? RA_KILLSENT    : 0) |
                         ((message->attribute & CRASH)       ? RA_CRASH       : 0) |
                         ((message->attribute & FILE_ATT)    ? RA_FILE_ATT    : 0) |
                         ((message->attribute & RET_REC_REQ) ? QBBS_REQ_REC   : 0) |
                         ((message->attribute & AUDIT_REQ)   ? QBBS_AUDIT_REQ : 0) |
                         ((message->attribute & IS_RET_REC)  ? QBBS_RET_REC   : 0) |
                         ((message->attribute & FILE_REQ)    ? QBBS_FREQ      : 0) |
                         RA_SENT;

         msgRa.OrigZone = message->srcNode.zone;
         msgRa.OrigNet  = message->srcNode.net;
         msgRa.OrigNode = message->srcNode.node;
         msgRa.DestZone = message->destNode.zone;
         msgRa.DestNet  = message->destNode.net;
         msgRa.DestNode = message->destNode.node;
         msgRa.Cost     = message->cost;
      }

      if (config.mbOptions.removeRe)
         removeRe(message->subject);

      msgRa.wtLength = strlen(message->toUserName);
      strncpy (msgRa.WhoTo, message->toUserName, 35);
      msgRa.wfLength = strlen(message->fromUserName);
      strncpy (msgRa.WhoFrom, message->fromUserName, 35);
      msgRa.sjLength = strlen(message->subject);
      strncpy (msgRa.Subj, message->subject, 72);

      if ((msgRa.sjLength) <= 56)
      {
         msgRa.subjCrc    = crc32alpha(message->subject);

         msgRa.wrTime     = checkDate (message->year,  message->month,   message->day,
                                       message->hours, message->minutes, message->seconds);
         msgRa.recTime    = startTime;

         msgRa.checkSum = CS_SECURITY ^ msgRa.subjCrc
                                      ^ msgRa.wrTime
                                      ^ msgRa.recTime;
      }

      if (hdrBufCount == HDR_BUFSIZE)
      {
         lseek(msgHdrHandle, 0, SEEK_END);
         lseek(msgIdxHandle, 0, SEEK_END);
         lseek(msgToIdxHandle, 0, SEEK_END);
         if (  write(msgHdrHandle  , msgHdrBuf  , HDR_BUFSIZE * sizeof(msgHdrRec  )) != (int)(HDR_BUFSIZE * sizeof(msgHdrRec  ))
            || write(msgIdxHandle  , msgIdxBuf  , HDR_BUFSIZE * sizeof(msgIdxRec  )) != (int)(HDR_BUFSIZE * sizeof(msgIdxRec  ))
            || write(msgToIdxHandle, msgToIdxBuf, HDR_BUFSIZE * sizeof(msgToIdxRec)) != (int)(HDR_BUFSIZE * sizeof(msgToIdxRec))
            )
            return 1;

         hdrBufCount = 0;
      }

      msgIdxBuf[hdrBufCount].MsgNum = msgRa.MsgNum;
      msgIdxBuf[hdrBufCount].Board  = msgRa.Board;

      memcpy (&(msgToIdxBuf[hdrBufCount]), &(msgRa.wtLength), 36);

      msgHdrBuf[hdrBufCount++] = msgRa;

      infoRec.ActiveMsgs[msgRa.Board-1]++;
      infoRec.TotalActive++;

      globVars.mbCount++;
   }
   return 0;
}
//---------------------------------------------------------------------------
s16 validate1BBS(void)
{
   s16 error = 0;

   if (hdrBufCount != 0)
   {
      lseek(msgHdrHandle, 0, SEEK_END);
      lseek(msgIdxHandle, 0, SEEK_END);
      lseek(msgToIdxHandle, 0, SEEK_END);
      error =  write(msgHdrHandle  , msgHdrBuf  , hdrBufCount * sizeof(msgHdrRec  )) != (int)(hdrBufCount * sizeof(msgHdrRec  ))
            || write(msgIdxHandle  , msgIdxBuf  , hdrBufCount * sizeof(msgIdxRec  )) != (int)(hdrBufCount * sizeof(msgIdxRec  ))
            || write(msgToIdxHandle, msgToIdxBuf, hdrBufCount * sizeof(msgToIdxRec)) != (int)(hdrBufCount * sizeof(msgToIdxRec));
      hdrBufCount = 0;
   }

   if ((!error) && (txtBufCount != 0))
   {
      lseek(msgTxtHandle, 0, SEEK_END);
      error = (write(msgTxtHandle, msgTxtBuf, txtBufCount << 8) != (txtBufCount << 8));
      txtBufCount = 0;
   }
   return error;
}
//---------------------------------------------------------------------------
void validate2BBS(u16 orgName)
{
   fhandle tempHandle;

   memcpy(&infoRecValid, &infoRec, sizeof(infoRecType));

   if (!config.mbOptions.quickToss)
   {
      tempHandle = dup(msgHdrHandle);
      close(tempHandle);
      tempHandle = dup(msgIdxHandle);
      close(tempHandle);
      tempHandle = dup(msgToIdxHandle);
      close(tempHandle);
      tempHandle = dup(msgTxtHandle);
      close(tempHandle);

      writeMsgInfo(orgName);
   }

#ifdef GOLDBASE
   raTxtRecValid = filelength(msgTxtHandle) >> 8;
   raHdrRecValid = filelength(msgHdrHandle) / sizeof(msgHdrRec);
#else
   raTxtRecValid = (u16)(filelength(msgTxtHandle) >> 8);
   raHdrRecValid = (u16)(filelength(msgHdrHandle) / sizeof(msgHdrRec));
#endif

   if ( globVars.mbCount > globVars.mbCountV )
      globVars.mbCountV = globVars.mbCount;
   if ( globVars.echoCount > globVars.echoCountV )
      globVars.echoCountV = globVars.echoCount;
   if ( globVars.badCount > globVars.badCountV )
      globVars.badCountV = globVars.badCount;
   if ( globVars.dupCount > globVars.dupCountV )
      globVars.dupCountV = globVars.dupCount;
}
//---------------------------------------------------------------------------
extern  u16             forwNodeCount;
extern  nodeFileType    nodeFileInfo;

static s16 processMsg(u16 areaIndex)
{
   u16            count = 0;
   u16            diskError = 0;
   echoToNodeType tempEchoToNode;
   s16            echoToNodeCount;
   char          *helpPtr;

   while ((count < forwNodeCount) &&
          ((!ETN_WRITEACCESS(echoToNode[areaIndex][ETN_INDEX(count)], count)) ||
           (memcmp(&(nodeFileInfo[count]->destNode4d.net),
                   &(globVars.packetSrcNode.net), sizeof(nodeNumType)-2) != 0)))
     count++;

   if ((count == forwNodeCount &&
        echoAreaList[areaIndex].options.security) ||
       (count < forwNodeCount &&
        echoAreaList[areaIndex].writeLevel >
        nodeFileInfo[count]->nodePtr->writeLevel))
   {
     printf("Security violation for area %s\n", echoAreaList[areaIndex].areaName);
     globVars.badCount++;

     return 0;
   }

   if (checkDup(message, echoAreaList[areaIndex].areaNameCRC))
   {
      for (count = 0; count < forwNodeCount; count++)
         if ((nodeFileInfo[count]->srcAka == globVars.packetDestAka) &&
             (memcmp (&nodeFileInfo[count]->destNode4d.net,
	     	      &globVars.packetSrcNode.net, 6) == 0))
            nodeFileInfo[count]->fromNodeDup++;

      puts(dARROW" Duplicate message");
      addVia(message->text, globVars.packetDestAka, 0);
      writeBBS(message, config.dupBoard, 1);
      echoAreaList[areaIndex].dupCount++;
      globVars.dupCount++;

      return -1;
   }

   echoAreaList[areaIndex].msgCount++;
   globVars.echoCount++;

   if (echoAreaList[areaIndex].options.allowPrivate)
      message->attribute &= PRIVATE;
   else
      message->attribute = 0;

   memcpy(tempEchoToNode, echoToNode[areaIndex], sizeof(echoToNodeType));
   echoToNodeCount = echoAreaList[areaIndex].echoToNodeCount;

   for (count = 0; count < forwNodeCount; count++)
   {
      if (memcmp(&nodeFileInfo[count]->destNode4d.net,
      	          &globVars.packetSrcNode.net, 6) == 0)
      {
         if ( ETN_ANYACCESS(tempEchoToNode[ETN_INDEX(count)], count) )
         {
           tempEchoToNode[ETN_INDEX(count)] &= ETN_RESET(count);
           echoToNodeCount--;
         }
         if (nodeFileInfo[count]->srcAka == globVars.packetDestAka)
           nodeFileInfo[count]->fromNodeMsg++;
      }
   }
   while ( (helpPtr = findCLStr(message->text, "\1FMAIL BAD")) != NULL )
      removeLine(helpPtr);
   while ( (helpPtr = findCLStr(message->text, "\1FMAIL SRC:")) != NULL )
      removeLine(helpPtr);
   while ( (helpPtr = findCLStr(message->text, "\1FMAIL DEST:")) != NULL )
      removeLine(helpPtr);

   if (echoToNodeCount)
   {
      addPathSeenBy(message, tempEchoToNode, areaIndex, NULL);

      if (writeEchoPkt(message, echoAreaList[areaIndex].options, tempEchoToNode))
         diskError = DERR_WRPKTE;
  }
  else
  {
#ifdef _DEBUG
    logEntry("DEBUG processMsg: echoToNodeCount == 0 SEEN-BY & PATH processing", LOG_DEBUG, 0);
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
    lseek(msgIdxHandle, 0, SEEK_END);
    lseek(msgToIdxHandle, 0, SEEK_END);
    lseek(msgHdrHandle, 0, SEEK_END);
    lseek(msgTxtHandle, 0, SEEK_END);
    if (writeBBS(message, echoAreaList[areaIndex].board, echoAreaList[areaIndex].options.impSeenBy))
      diskError = DERR_WRHECHO;
  }
  else
  {
  // JAM toss
      if (echoAreaList[areaIndex].options.impSeenBy)
        strcpy(stpcpy(strchr(message->text, 0), message->normSeen), message->normPath);

      if (echoAreaList[areaIndex].JAMdirPtr != NULL)
      {
      // Skip AREA tag
         if (strncmp (message->text, "AREA:", 5) == 0)
         {
            if (*(helpPtr = strchr (message->text, '\r') + 1) == '\n')
              helpPtr++;

            memmove(message->text,helpPtr,strlen(helpPtr)+1);
         }
         if (config.mbOptions.removeRe)
            removeRe(message->subject);
         // write here
         if (!jam_writemsg(echoAreaList[areaIndex].JAMdirPtr, message, 0))
         {
           newLine();
           logEntry ("Can't write JAM message", LOG_ALWAYS, 0);
           diskError = DERR_WRJECHO;
         }
         globVars.jamCountV++;
      }
   }
   if (diskError)
      return 2;

   return 1;
}
//---------------------------------------------------------------------------
void moveBadBBS(void)
{
   msgHdrRec   msgRa;
   msgIdxRec   msgIndex;
   s16         areaIndex;
   s16         move = 0;
   s16         result;
//   char        *helpPtr;
#ifndef GOLDBASE
   s16         minusOne = -1;
   u16         offset = 0;
#else
   s32         minusOne = -1;
   u32         offset = 0;
#endif

   while (  lseek(msgIdxHandle, offset++ * sizeof(msgIdxRec), SEEK_SET) != -1
         && read (msgIdxHandle, &msgIndex, sizeof(msgIdxRec)) == (int)sizeof(msgIdxRec)
         )
   {
      if ((s32)msgIndex.MsgNum == -1 || msgIndex.Board != config.badBoard)
         continue;

      if ((result = scanBBS(offset - 1, message, 2)) < 0)
         continue;

      if (!result)
         break;

      printf("(%d) ", msgIndex.MsgNum);
      getKludgeNode(message->text, "\1FMAIL SRC: ", &globVars.packetSrcNode);
      getKludgeNode(message->text, "\1FMAIL DEST: ", &globVars.packetDestNode);
      globVars.packetDestAka = getLocalAkaNum(&globVars.packetDestNode);

      if ((areaIndex = getAreaCode(message->text)) < 0)
      {
         if ( areaIndex == BADMSG )
            newLine();
         else
            putchar('\r');
         continue;
      }

      move++;
      if ( !(result = processMsg(areaIndex)) )
         continue;
      if ( result == 2 )
         break;
      if ( result < 0 )
         goto deleteMsg;

      if (echoAreaList[areaIndex].JAMdirPtr == NULL && echoAreaList[areaIndex].board)
         printf("Moving message #%u to "MBNAME" area %s\n", msgIndex.MsgNum, echoAreaList[areaIndex].areaName);
      else if (echoAreaList[areaIndex].JAMdirPtr == NULL)
         printf("Deleting message #%u for pass through area %s\n", msgIndex.MsgNum, echoAreaList[areaIndex].areaName);
      else
         printf("Moving message #%u to JAM area %s\n", msgIndex.MsgNum, echoAreaList[areaIndex].areaName);
deleteMsg:
      lseek(msgToIdxHandle, (offset - 1) * (u32)sizeof(msgToIdxRec), SEEK_SET);
      write(msgToIdxHandle, "\x0b* Deleted *", 12);

      lseek(msgIdxHandle, (offset - 1) * (u32)sizeof(msgIdxRec), SEEK_SET);
      write(msgIdxHandle, &minusOne, sizeof(minusOne));

      lseek(msgHdrHandle, (offset - 1)*(u32)sizeof(msgHdrRec), SEEK_SET);
      read(msgHdrHandle, &msgRa, sizeof(msgHdrRec));
      msgRa.MsgAttr |= RA_DELETED;
      lseek(msgHdrHandle, (offset - 1)*(u32)sizeof(msgHdrRec), SEEK_SET);
      write(msgHdrHandle, &msgRa, sizeof(msgHdrRec));

      if ( msgRa.MsgNum == infoRec.LowMsg )
         infoRec.LowMsg++;
      if ( msgRa.MsgNum == infoRec.HighMsg )
         infoRec.HighMsg--;
      infoRec.TotalActive--;
      infoRec.ActiveMsgs[config.badBoard-1]--;
      globVars.movedBad++;
      if ( validate1BBS() || validateEchoPktWr() == 0 )
         validate2BBS(1);
   }
   if ( move )
      newLine();
   else
      putchar('\r');

//#pragma message ("Moet deze regel echt weg?")
// memcpy (&infoRecValid, &infoRec, sizeof(infoRecType));
   jam_closeall();
   lseek(msgIdxHandle, 0, SEEK_END);
   lseek(msgToIdxHandle, 0, SEEK_END);
   lseek(msgHdrHandle, 0, SEEK_END);
   lseek(msgTxtHandle, 0, SEEK_END);
}
//---------------------------------------------------------------------------
void closeBBSWr(u16 orgName)
{

   lseek (msgHdrHandle, 0, SEEK_SET);
   chsize(msgHdrHandle, raHdrRecValid*(u32)sizeof(msgHdrRec));
   close (msgHdrHandle);
   free  (msgHdrBuf);

   lseek (msgToIdxHandle, 0, SEEK_SET);
   chsize(msgToIdxHandle, raHdrRecValid*(u32)sizeof(msgToIdxRec));
   close (msgToIdxHandle);
   free  (msgToIdxBuf);

   lseek (msgIdxHandle, 0, SEEK_SET);
   chsize(msgIdxHandle, raHdrRecValid*(u32)sizeof(msgIdxRec));
   close (msgIdxHandle);
   free  (msgIdxBuf);

   lseek (msgTxtHandle, 0, SEEK_SET);
   chsize(msgTxtHandle, ((u32)raTxtRecValid) << 8);
   close (msgTxtHandle);
   free  (msgTxtBuf);

   writeMsgInfo(orgName);
}
//---------------------------------------------------------------------------
#if 0
void openBBSRd(void)
{
   tempStrType tempStr;

   readMsgInfo (0);

   if (((msgHdrBuf = malloc (HDR_BUFSIZE*sizeof(msgHdrRec))) == NULL) ||
       ((msgTxtBuf = malloc (TXT_BUFSIZE*256)) == NULL))
   {
      logEntry ("Not enough memory to allocate message base file buffers", LOG_ALWAYS, 2);
   }

   strcpy (tempStr, config.bbsPath);
   strcat (tempStr, "MSGHDR."MBEXTN);

   if ((msgHdrHandle = open(tempStr, O_RDWR | O_BINARY)) == -1)
      logEntry ("Can't open message base files for input", LOG_ALWAYS, 1);

   strcpy (tempStr, config.bbsPath);
   strcat (tempStr, "MSGTXT."MBEXTN);

   if ((msgTxtHandle = open(tempStr, O_RDWR | O_BINARY)) == -1)
      logEntry ("Can't open message base files for input", LOG_ALWAYS, 1);

#ifdef GOLDBASE
   msgTxtRecNum = (u16)(filelength (msgTxtHandle) >> 8);
#else
   msgTxtRecNum = (u16)(filelength (msgTxtHandle) >> 8);
#endif
   hdrBufCount = 0;
   txtBufCount = 0;
}
#endif
//---------------------------------------------------------------------------
#ifndef GOLDBASE
s16 scanBBS(u16 index, internalMsgType *message, u16 rescan)
#else
s16 scanBBS(u32 index, internalMsgType *message, u16 rescan)
#endif
{
   msgHdrRec   msgRa;
   msgTxtRec   txtRa;
   s16         count;
   char       *helpPtr;
   tempStrType tempStr;
   u16         temp;

// returnTimeSlice(0);

   if (lseek(msgHdrHandle, index * (u32)sizeof(msgHdrRec), SEEK_SET) == -1)
      return 0;

   if ((temp = read(msgHdrHandle, &msgRa, sizeof(msgHdrRec))) != sizeof(msgHdrRec))
   {
      if (eof(msgHdrHandle) != 1)
         logEntry("Can't read MsgHdr."MBEXTN, LOG_ALWAYS, 0);
      return 0;
   }

   if (
         (
            (
               (
                  ((msgRa.MsgAttr & RA_ECHO_OUT) && !isNetmailBoard(msgRa.Board))
               || ((msgRa.MsgAttr & RA_NET_OUT ) &&  isNetmailBoard(msgRa.Board))
               )
            && (msgRa.MsgAttr & RA_LOCAL)
            )
         || rescan
         )
      && !(msgRa.MsgAttr & RA_DELETED)
      )
   {
      if ((u32)msgRa.NumRecs > ((u32)((u32)TEXT_SIZE - 2048) >> 8))
      {
         putchar('\r');
         sprintf(tempStr, "Message too big: message #%u in board #%u "dARROW" Skipped",
                           msgRa.MsgNum, (u16)msgRa.Board);
         logEntry(tempStr, LOG_ALWAYS, 0);
         return -1;
      }

      memset(message, 0, INTMSG_SIZE);
      strncpy(message->fromUserName, msgRa.WhoFrom, msgRa.wfLength);
      strncpy(message->toUserName,   msgRa.WhoTo,   msgRa.wtLength);
      strncpy(message->subject,      msgRa.Subj,    msgRa.sjLength);

      if (scanDate((char*)&msgRa.ptLength,
                   &message->year, &message->month, &message->day,
                   &message->hours, &message->minutes) != 0)
      {
         putchar('\r');
         sprintf(tempStr, "Bad date in message base: message #%u in board #%u "dARROW" Skipped",
                           msgRa.MsgNum, msgRa.Board);
         logEntry(tempStr, LOG_MSGBASE, 0);
         return -1;
      }

      if (message->year >= 100)
         message->year = 1980;
      else
      {
         if (message->year >= 80)
            message->year += 1900;
         else
            message->year += 2000;
      }

      if ((message->month == 0) || (message->month > 12))
         message->month = 1;

      if ((message->day == 0) || (message->day > 31))
         message->day = 1;

      if (message->hours >= 24)
         message->hours = 0;

      if (message->minutes >= 60)
         message->minutes = 0;

      message->seconds   = secondsInc++;

      if (secondsInc == 60)
         secondsInc = 0;

      if (msgRa.MsgAttr & RA_PRIVATE)
         message->attribute |= PRIVATE;

      helpPtr = message->text;

      lseek(msgTxtHandle, ((u32)msgRa.StartRec) << 8, SEEK_SET);

      for (count = 0; count < msgRa.NumRecs; count++)
      {
         if (read(msgTxtHandle, &txtRa, 256) != 256)
         {
            puts("\nError reading MsgTxt."MBEXTN".");
            return 0;
         }
         strncpy(helpPtr, txtRa.txtStr, txtRa.txtLength);
         helpPtr += txtRa.txtLength;
      }

      removeLfSr(message->text);

      if (getFlags(message->text) & FL_LOK)
         return -1;

      if (isNetmailBoard(msgRa.Board))
      {
         if (msgRa.NetAttr & RA_KILLSENT)
            message->attribute |= KILLSENT;
         if (msgRa.NetAttr & RA_FILE_ATT)
            message->attribute |= FILE_ATT;
         if (msgRa.NetAttr & RA_CRASH)
            message->attribute |= CRASH;
         if (msgRa.NetAttr & QBBS_REQ_REC)
            message->attribute |= RET_REC_REQ;
         if (msgRa.NetAttr & QBBS_AUDIT_REQ)
            message->attribute |= AUDIT_REQ;
         if (msgRa.NetAttr & QBBS_RET_REC)
            message->attribute |= IS_RET_REC;
         if (msgRa.NetAttr & QBBS_FREQ)
            message->attribute |= FILE_REQ;

         message->cost          = msgRa.Cost;

         message->srcNode.zone  = msgRa.OrigZone;
         message->srcNode.net   = msgRa.OrigNet;
         message->srcNode.node  = msgRa.OrigNode;

         message->destNode.zone = msgRa.DestZone;
         message->destNode.net  = msgRa.DestNet;
         message->destNode.node = msgRa.DestNode;

         if ((helpPtr = findCLStr (message->text, "\1MSGID:")) != NULL)
         {
            temp = atoi (helpPtr+7);
            if (temp % 256 == msgRa.OrigZone)
               message->srcNode.zone = temp;
            if (temp % 256 == msgRa.DestZone)
               message->destNode.zone = temp;
         }
         point4d (message);

         for (count = 0; count < MAX_NETAKAS; count++)
            if (config.netmailBoard[count] == msgRa.Board)
            {
               if (!memcmp(&message->destNode, &config.akaList[count].nodeNum, sizeof(nodeNumType)))
                  return -1;
               break;
            }
      }
      else
      {
         // Convert SEEN-BY kludge lines to normal SEEN-BY lines
         helpPtr = message->text;
         while ((helpPtr = findCLStr(helpPtr, "\1SEEN-BY: ")) != NULL)
         {
            memmove(helpPtr, helpPtr + 1, strlen(helpPtr));
            helpPtr += 9;
         }
      }
      return msgRa.Board;
   }
   return -1;
}
//---------------------------------------------------------------------------
s16 updateCurrHdrBBS(internalMsgType *message)
{
   msgHdrRec     msgRa;
   infoRecType   msgInfo;
   fhandle       tempHandle;
   u32           recNum;
   u16           deletedFlag = -1;
   tempStrType   tempStr;

   if (lockMB())
      return 1;

   recNum = lseek(msgHdrHandle, -(long)(sizeof(msgHdrRec)), SEEK_CUR) / (u32)sizeof(msgHdrRec);
   if (read(msgHdrHandle, &msgRa, sizeof(msgHdrRec)) != (int)sizeof(msgHdrRec))
   {
      unlockMB();
      return 1;
   }
   /* Local bit is not removed since 0.36a, RA_NET_OUT bit is now removed */
   msgRa.MsgAttr &= ~(RA_ECHO_OUT|RA_NET_OUT);

   if (msgRa.MsgAttr & RA_IS_NETMAIL)
   {
      msgRa.NetAttr |= RA_SENT;
      if (msgRa.NetAttr & RA_KILLSENT)
      {
         msgRa.MsgAttr |= RA_DELETED;

         strcpy (tempStr, config.bbsPath);
         strcat (tempStr, "MSGIDX."MBEXTN);

         if ((tempHandle = open(tempStr, O_RDWR | O_BINARY)) == -1)
            logEntry ("Can't open message base files for update", LOG_ALWAYS, 1);

         lseek(tempHandle, recNum*(u32)sizeof(msgIdxRec), SEEK_SET);
         write(tempHandle, &deletedFlag, 2);
         close(tempHandle);

         strcpy(tempStr, config.bbsPath);
         strcat(tempStr, "MSGTOIDX."MBEXTN);

         if ((tempHandle = open(tempStr, O_RDWR | O_BINARY)) == -1)
            logEntry ("Can't open message base files for update", LOG_ALWAYS, 1);

         lseek(tempHandle, recNum*(u32)sizeof(msgToIdxRec), SEEK_SET);
         write(tempHandle, "\x0b* Deleted *", 12);
         close(tempHandle);

         if (msgRa.Board >= 1 && msgRa.Board <= MBBOARDS)
         {
            read(lockHandle, &msgInfo, sizeof(infoRecType));
            msgInfo.ActiveMsgs[msgRa.Board-1]--;
            msgInfo.TotalActive--;
            lseek(lockHandle, 0, SEEK_SET);
            write(lockHandle, &msgInfo, sizeof(infoRecType));
         }
      }
   }
   if (msgRa.sjLength <= 56)
   {
      msgRa.subjCrc = crc32alpha(msgRa.Subj);

      msgRa.wrTime     = checkDate (message->year,  message->month,   message->day,
                                    message->hours, message->minutes, message->seconds);
      msgRa.recTime    = startTime;

      msgRa.checkSum = CS_SECURITY ^ msgRa.subjCrc
                                   ^ msgRa.wrTime
                                   ^ msgRa.recTime;
   }

   if (config.mbOptions.scanUpdate)
   {
      lseek (msgTxtHandle, 0, SEEK_END);
      if ( (writeText (message->text, message->normSeen, message->normPath, 1,
                      &msgRa.StartRec, &msgRa.NumRecs)) ||
           validate1BBS() )
//          (write (msgTxtHandle, msgTxtBuf, txtBufCount << 8) != (txtBufCount << 8)))
      {
         unlockMB();
         return 1;
      }
      validate2BBS(1);
   }

   lseek(msgHdrHandle, -(long)sizeof(msgHdrRec), SEEK_CUR);
   if (write(msgHdrHandle, &msgRa, sizeof(msgHdrRec)) != sizeof(msgHdrRec))
   {
      lseek(msgHdrHandle, 0, SEEK_CUR);
      unlockMB();
      return 1;
   }

   lseek(msgHdrHandle, 0, SEEK_CUR);

   globVars.mbCountV++;

   unlockMB();
   return 0;
}
//---------------------------------------------------------------------------
s16 rescan(nodeInfoType *nodeInfo, const char *areaName, u16 maxRescan, fhandle msgHandle1, fhandle msgHandle2)
{
  fhandle         origMsgHdrHandle
                , origMsgTxtHandle;
  fhandle         tempHandle;
  u16             count;
  tempStrType     tempStr
                , tempStr2;
  msgIdxRec       msgIdxBuf[256];
  u16             msgIdxBufCount;
  nodeFileRecType nfInfo;
#ifndef GOLDBASE
  u16             index;
#else
  u32             index;
#endif
  u16             echoIndex = 0;
  u16             msgsFound = 0;
  s16             msgCount;

#ifdef _DEBUG0
  logEntry("DEBUG Rescan start", LOG_DEBUG, 0);
#endif

  while (echoIndex < echoCount && strcmp(echoAreaList[echoIndex].areaName, areaName) != 0)
    echoIndex++;

  if (echoIndex == echoCount)
    return -1;

  if (echoAreaList[echoIndex].board == 0 && echoAreaList[echoIndex].JAMdirPtr == NULL)
  {
    sprintf(tempStr, "Can't rescan pass-through area %s", echoAreaList[echoIndex].areaName);
    mgrLogEntry(tempStr);
    strcat(tempStr, "\r");
    write(msgHandle1, tempStr, strlen(tempStr));
    write(msgHandle2, tempStr, strlen(tempStr));

    return 0;
  }

  if (echoAreaList[echoIndex].JAMdirPtr != NULL)
    msgsFound = (u16)jam_rescan(echoIndex, maxRescan, nodeInfo, message);
  else
  {
#ifdef _DEBUG0
    logEntry("DEBUG Rescan hudson", LOG_DEBUG, 0);
#endif
    origMsgHdrHandle = msgHdrHandle;
    origMsgTxtHandle = msgTxtHandle;

    strcpy(tempStr, config.bbsPath);
    strcat(tempStr, "MSGINFO."MBEXTN);
    if (((tempHandle = open(tempStr, O_RDONLY | O_BINARY)) == -1) ||
       (lseek(tempHandle, 4 + (echoAreaList[echoIndex].board * 2), SEEK_SET) == -1) ||
       (read(tempHandle, &msgCount, 2) != 2)  ||
       (close(tempHandle) == -1))
      return -1;

    strcpy(tempStr, config.bbsPath);
    strcat(tempStr, "MSGHDR."MBEXTN);
    if ((msgHdrHandle = open(tempStr, O_RDONLY | O_BINARY)) == -1)
      return -1;

    strcpy(tempStr, config.bbsPath);
    strcat(tempStr, "MSGTXT."MBEXTN);
    if ((msgTxtHandle = open(tempStr, O_RDONLY | O_BINARY)) == -1)
      return (-1);

    strcpy(tempStr, config.bbsPath);
    strcat(tempStr, "MSGIDX."MBEXTN);
    if ((tempHandle = open(tempStr, O_RDONLY | O_BINARY)) == -1)
      return (-1);

    sprintf(tempStr, "Scanning for messages in HUDSON area: %s", echoAreaList[echoIndex].areaName);
    logEntry(tempStr, LOG_ALWAYS, 0);

    sprintf(tempStr2, "AREA:%s\r\1RESCANNED", echoAreaList[echoIndex].areaName);
    setViaStr(tempStr, tempStr2, echoAreaList[echoIndex].address);

    makeNFInfo(&nfInfo, echoAreaList[echoIndex].address, &nodeInfo->node);

    index = 0;
    while ((msgIdxBufCount = (read(tempHandle, msgIdxBuf, 256 * sizeof(msgIdxRec)) / 3)) != 0)
    {
      // returnTimeSlice(0);

      for (count = 0; count < msgIdxBufCount; count++)
      {
         if (  msgIdxBuf[count].Board == echoAreaList[echoIndex].board
#ifndef GOLDBASE
            && msgIdxBuf[count].MsgNum != UINT16_MAX
#else
            && msgIdxBuf[count].MsgNum != UINT32_MAX
#endif
            )
         {
            if (  msgCount <= maxRescan
               && scanBBS(index, message, 1) == echoAreaList[echoIndex].board
               )
            {
#ifdef _DEBUG0
              {
                char fname[64];
                sprintf(fname, "%08lx_rescanned_hudson.msg", uniqueID());
                logEntry(fname, LOG_DEBUG, 0);
                if ((tempHandle = open(fname, O_WRONLY | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) != -1)
                {
                  write(tempHandle, message->text, strlen(message->text));
                  close(tempHandle);
                }
              }
#endif
              addPathSeenBy(message, echoToNode[echoIndex], echoIndex, &nodeInfo->node);
              setSeenByPath(message, NULL, echoAreaList[echoIndex].options, nodeInfo->options);
              message->srcNode  = *getAkaNodeNum(echoAreaList[echoIndex].address, 1);
              message->destNode = nodeInfo->node;
              insertLine(message->text, tempStr);
              writeNetPktValid(message, &nfInfo);
              msgsFound++;
            }
            msgCount--;
        }
        index++;
      }
    }
    newLine();
    closeNetPktWr(&nfInfo);

    close(msgHdrHandle);
    close(msgTxtHandle);
    close(tempHandle);

    msgHdrHandle = origMsgHdrHandle;
    msgTxtHandle = origMsgTxtHandle;
  }

  putchar('\r');
  sprintf(tempStr, "Rescanned %u messages in area %s", msgsFound, echoAreaList[echoIndex].areaName);
  mgrLogEntry(tempStr);
#ifdef _DEBUG
  logEntry(tempStr, LOG_DEBUG, 0);
#endif
  strcat(tempStr, "\r");
  write(msgHandle1, tempStr, strlen(tempStr));
  write(msgHandle2, tempStr, strlen(tempStr));

  return msgsFound;
}
//---------------------------------------------------------------------------
