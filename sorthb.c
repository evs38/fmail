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

#include <ctype.h>
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "fmail.h"

#include "areainfo.h"
#include "crc.h"
#include "log.h"
#include "minmax.h"
#include "msgpkt.h"
#include "msgra.h"    // expandName()
#include "msgradef.h"
#include "mtask.h"
#include "sorthb.h"
#include "stpcpy.h"
#include "utils.h"


extern configType config;

#ifdef GOLDBASE
void sortBBS()
{
}
#else
void sortBBS(u16 origTotalMsgBBS, s16 mbSharing)
{
   tempStrType   tempStr1
               , tempStr2
               , tempStr3;
   u16           count
               , temp
               , p
               , q
               , low
               , mid
               , high
               , bufCount
               , newTotalMsgBBS;
   s32           code;
   msgHdrRec    *msgHdrBuf;
   msgToIdxRec  *msgToIdxBuf;
   msgIdxRec    *msgIdxBuf;
   fhandle       oldHdrHandle
               , msgHdrHandle
               , msgToIdxHandle
               , msgIdxHandle;
   u16           areaSubjChain[MBBOARDS + 1];
   u16           year
               , month
               , day
               , hours
               , minutes;

   u16          *sli_recNum
               ,*sli_msgNum
               ,*sli_prevReply = NULL
               ,*sli_nextReply = NULL;
  u8            *sli_areaNum;
  u16           *sli_subjectCrcHi
               ,*sli_subjectCrcLo
               ,*sli_bsTimeStampHi = NULL
               ,*sli_bsTimeStampLo = NULL;

  u16           HDR_BUFSIZE;

  HDR_BUFSIZE = (248>>3) *
#if defined(__FMAILX__) || defined(__32BIT__)
                          8;
#else
                            (8-((config.ftBufSize==0) ? 0 :
                               ((config.ftBufSize==1) ? 3 :
                               ((config.ftBufSize==2) ? 5 : 7))));
#endif

   if ((msgHdrHandle = open(expandNameHudson(dMSGHDR, 0), O_RDONLY | O_BINARY)) == -1)
   {
      puts("Can't open "dMSGHDR"."MBEXTN" for update.");
      return;
   }
   if (config.mbOptions.sortNew && (fileLength(msgHdrHandle) > diskFree(config.bbsPath)))
   {
#ifdef FMAIL
      logEntry("Not enough free diskspace to sort new messages", LOG_ALWAYS, 0);
#else // FTOOLS
      logEntry("Not enough free diskspace to sort messages", LOG_ALWAYS, 0);
#endif
      if (!config.mbOptions.updateChains)
         return;

      config.mbOptions.sortNew = 0;
   }
   newTotalMsgBBS = (u16)(fileLength(msgHdrHandle) / sizeof(msgHdrRec));
   if (newTotalMsgBBS == origTotalMsgBBS)
   {
      close(msgHdrHandle);
#ifdef FMAIL
      puts("There are no new messages to sort and/or link.");
#else // FTOOLS
      puts("There are no messages to sort.");
#endif
      return;
   }

   if (config.mbOptions.sortNew)
   {
      if (((sli_bsTimeStampHi = (u16*)calloc(newTotalMsgBBS, 2)) == NULL) ||
          ((sli_bsTimeStampLo = (u16*)calloc(newTotalMsgBBS, 2)) == NULL))
      {
         close (msgHdrHandle);
         logEntry("Not enough memory to sort the "MBNAME" message base", LOG_ALWAYS, 0);
         return;
      }
   }

   if (config.mbOptions.updateChains)
   {
      if (  (sli_prevReply = (u16*)calloc(newTotalMsgBBS, 2)) == NULL
         || (sli_nextReply = (u16*)calloc(newTotalMsgBBS, 2)) == NULL
         )
      {
         close(msgHdrHandle);
         logEntry("Not enough memory to sort the "MBNAME" message base", LOG_ALWAYS, 0);
         goto exit3;
      }
   }

   if (((sli_recNum       = (u16*)calloc(newTotalMsgBBS, 2)) == NULL) ||
       ((sli_msgNum       = (u16*)calloc(newTotalMsgBBS, 2)) == NULL) ||
       ((sli_areaNum      = (u8 *)calloc(newTotalMsgBBS, 1)) == NULL) ||
       ((sli_subjectCrcHi = (u16*)calloc(newTotalMsgBBS, 2)) == NULL) ||
       ((sli_subjectCrcLo = (u16*)calloc(newTotalMsgBBS, 2)) == NULL) ||
       ((msgHdrBuf        = (msgHdrRec  *)malloc(HDR_BUFSIZE * sizeof(msgHdrRec))) == NULL) ||
       ((msgIdxBuf        = (msgIdxRec  *)malloc(HDR_BUFSIZE * sizeof(msgIdxRec))) == NULL) ||
       ((msgToIdxBuf      = (msgToIdxRec*)malloc(HDR_BUFSIZE * sizeof(msgToIdxRec))) == NULL))
   {
      close(msgHdrHandle);
      logEntry("Not enough memory to sort the "MBNAME" message base", LOG_ALWAYS, 0);
      goto exit2;
   }

   putStr("Scanning messages... ");
   bufCount = HDR_BUFSIZE;
   for (count = 0; count < newTotalMsgBBS; count++)
   {
      returnTimeSlice(0);
      if (bufCount == HDR_BUFSIZE)
      {
//       printf("%3u%%\b\b\b\b", (u16)((s32) 100 * count / newTotalMsgBBS));

         read(msgHdrHandle, msgHdrBuf, HDR_BUFSIZE * sizeof(msgHdrRec));
         bufCount = 0;
      }

      sli_recNum [count] = count;
      sli_msgNum [count] = msgHdrBuf[bufCount].MsgNum;
      sli_areaNum[count] = msgHdrBuf[bufCount].Board;

      if (!(msgHdrBuf[bufCount].MsgAttr & RA_DELETED))
      {
         if (msgHdrBuf[bufCount].checkSum ^ msgHdrBuf[bufCount].subjCrc ^
             msgHdrBuf[bufCount].wrTime   ^ msgHdrBuf[bufCount].recTime ^
             CS_SECURITY)
         {
            strncpy(tempStr1, msgHdrBuf[bufCount].Subj, temp = min(72, msgHdrBuf[bufCount].sjLength));
            tempStr1[temp] = 0;
            removeRe(tempStr1);

            sli_subjectCrcHi[count] = (u16)((code = crc32alpha(tempStr1)) >> 16);
            sli_subjectCrcLo[count] = (u16)code;

            if (config.mbOptions.sortNew)
            {
               if (scanDate ((char*)&msgHdrBuf[bufCount].ptLength, &year, &month, &day, &hours, &minutes) == 0)
               {
                  sli_bsTimeStampHi[count] = (u16)((code = checkDate(year, month, day, hours, minutes, 0)) >> 16);
                  sli_bsTimeStampLo[count] = (u16)code;
               }
               else
               {
                  putchar('\n');
                  flogEntry(LOG_MSGBASE, 0, "Bad date in message base: message #%u in board #%u", msgHdrBuf[bufCount].MsgNum, msgHdrBuf[bufCount].Board);
                  sli_bsTimeStampHi[count] = 0;
                  sli_bsTimeStampLo[count] = 0;
               }
            }
         }
         else
         {
            sli_subjectCrcHi[count]  = (u16)((code = msgHdrBuf[bufCount].subjCrc) >> 16);
            sli_subjectCrcLo[count]  = (u16)code;
            if (config.mbOptions.sortNew)
            {
               sli_bsTimeStampHi[count] = (u16)((code = msgHdrBuf[bufCount].wrTime) >> 16);
               sli_bsTimeStampLo[count] = (u16)code;
            }
         }
      }
      bufCount++;
   }
   close(msgHdrHandle);
// puts("100%");

   if (config.mbOptions.sortNew)
   {
      printf("Sorting %d "
#ifdef FMAIL
             "new "
#endif
             "messages... ", newTotalMsgBBS - origTotalMsgBBS);
      for (count = origTotalMsgBBS + 1; count < newTotalMsgBBS; count++)
      {
         returnTimeSlice(0);
//       printf("%3u%%\b\b\b\b", (u16)((s32)100 * (count - origTotalMsgBBS - 1) / (newTotalMsgBBS - origTotalMsgBBS - 1)));

         temp = sli_recNum[count];
         low = origTotalMsgBBS;
         high = count;

         if (config.mbOptions.sortSubject)
         {
            while (low < high)
            {
               mid = (low + high) >> 1;
               if ((sli_areaNum[temp] > sli_areaNum[sli_recNum[mid]]) ||
                   ((sli_areaNum[temp] == sli_areaNum[sli_recNum[mid]]) &&
                    ((sli_subjectCrcHi[temp] > sli_subjectCrcHi[sli_recNum[mid]]) ||
                     ((sli_subjectCrcHi[temp] == sli_subjectCrcHi[sli_recNum[mid]]) &&
                      ((sli_subjectCrcLo[temp] > sli_subjectCrcLo[sli_recNum[mid]]) ||
                       ((sli_subjectCrcLo[temp] == sli_subjectCrcLo[sli_recNum[mid]]) &&
                        ((sli_bsTimeStampHi[temp] > sli_bsTimeStampHi[sli_recNum[mid]]) ||
                         ((sli_bsTimeStampHi[temp] == sli_bsTimeStampHi[sli_recNum[mid]]) &&
                          ((sli_bsTimeStampLo[temp] > sli_bsTimeStampLo[sli_recNum[mid]]) ||
                           ((sli_bsTimeStampLo[temp] == sli_bsTimeStampLo[sli_recNum[mid]]) &&
                            (temp > sli_recNum[mid])))))))))))
                  low = mid+1;
               else
                  high = mid;
            }
         }
         else
         {
            while (low < high)
            {
               mid = (low + high) >> 1;
               if ((sli_areaNum[temp] > sli_areaNum[sli_recNum[mid]]) ||
                   ((sli_areaNum[temp] == sli_areaNum[sli_recNum[mid]]) &&
                    ((sli_bsTimeStampHi[temp] > sli_bsTimeStampHi[sli_recNum[mid]]) ||
                     ((sli_bsTimeStampHi[temp] == sli_bsTimeStampHi[sli_recNum[mid]]) &&
                      ((sli_bsTimeStampLo[temp] > sli_bsTimeStampLo[sli_recNum[mid]]) ||
                       ((sli_bsTimeStampLo[temp] == sli_bsTimeStampLo[sli_recNum[mid]]) &&
                        (temp > sli_recNum[mid])))))))
                  low = mid+1;
               else
                  high = mid;
            }
         }
         memmove (&sli_recNum[low+1], &sli_recNum[low],
                  (count-low) << 1);
         sli_recNum[low] = temp;
      }
//    puts("100%");

      if (origTotalMsgBBS != 0)
         temp = sli_msgNum[sli_recNum[origTotalMsgBBS-1]] + 1;
      else
         temp = 1;
      for (count = origTotalMsgBBS; count < newTotalMsgBBS; count++)
         sli_msgNum[sli_recNum[count]] = temp++;
   } // End Sort

   if (config.mbOptions.updateChains)
   {
      putStr("Updating reply-chains in memory... ");
      memset(areaSubjChain, 0xff, sizeof (areaSubjChain));
      for (count = 0; count < newTotalMsgBBS; count++)
         if ((sli_areaNum[temp = sli_recNum[count]]) <= MBBOARDS)
         {
            sli_prevReply[temp] = areaSubjChain[(int)sli_areaNum[temp]];
            areaSubjChain[(int)sli_areaNum[temp]] = count;
         }

      for (count = 1; count <= MBBOARDS; count++)
      {
         returnTimeSlice(0);
//       printf("%3u%%\b\b\b\b", count >> 1);

         p = areaSubjChain[count];
         while (p != 0xffff)
         {
            q = sli_prevReply[sli_recNum[p]];
            code = ((s32)sli_subjectCrcHi[sli_recNum[p]] << 16) |
                          sli_subjectCrcLo[sli_recNum[p]];
            while ((q != 0xffff) && (code != (((s32)sli_subjectCrcHi[sli_recNum[q]] << 16) |
                                                     sli_subjectCrcLo[sli_recNum[q]])))
            {
               q = sli_prevReply[sli_recNum[q]];
            }
            temp = sli_recNum[p];
            p = sli_prevReply[sli_recNum[p]];
            if (q != 0xffff)
            {
               sli_prevReply[temp] = sli_msgNum[sli_recNum[q]];
               sli_nextReply[sli_recNum[q]] = sli_msgNum[temp];
            }
            else
               sli_prevReply[temp] = 0;
         }
      }
//    puts("100%");
   } // End Link

   if (config.mbOptions.sortNew)
   {
      putStr("Writing "dMSGHDR"."MBEXTN" and index files... ");

      if (  ((msgIdxHandle   = open(expandNameHudson(dMSGIDX  , 0), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1)
         || ((msgToIdxHandle = open(expandNameHudson(dMSGTOIDX, 0), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1)
         )
      {
         close(msgIdxHandle);
         putchar('\n');
         logEntry("Can't create sorted/linked message files", LOG_ALWAYS, 0);
         goto exit1;
      }
      strcpy(tempStr1, expandNameHudson(dMSGHDR, 0));
      strcpy(stpcpy(tempStr2, config.bbsPath), dMSGHDR".ZZZ");
      strcpy(stpcpy(tempStr3, config.bbsPath), dMSGHDR".!!!");

      if (((unlink(tempStr2) == -1) && (errno != ENOENT)) ||
          ((msgHdrHandle = open(tempStr3, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1) ||
          ((oldHdrHandle = open(tempStr1, O_RDONLY | O_BINARY)) == -1))
      {
         close(msgHdrHandle);
         newLine();
         logEntry("Can't create sorted/linked message files", LOG_ALWAYS, 0);
         goto exit1;
      }
      bufCount = HDR_BUFSIZE;
      for (count = 0; count < newTotalMsgBBS; count++)
      {
         returnTimeSlice(0);
         if (bufCount == HDR_BUFSIZE)
         {
//          printf("%3u%%\b\b\b\b", (u16)((s32)100 * count / newTotalMsgBBS));

            if (count != 0)
            {
               write(msgHdrHandle  , msgHdrBuf  , HDR_BUFSIZE * sizeof(msgHdrRec  ));
               write(msgIdxHandle  , msgIdxBuf  , HDR_BUFSIZE * sizeof(msgIdxRec  ));
               write(msgToIdxHandle, msgToIdxBuf, HDR_BUFSIZE * sizeof(msgToIdxRec));
            }
            bufCount = 0;
            if (count < origTotalMsgBBS)
            {
               lseek(oldHdrHandle, count * (s32)sizeof(msgHdrRec), SEEK_SET);
               if (count + HDR_BUFSIZE <= origTotalMsgBBS)
                  bufCount = HDR_BUFSIZE;
               else
                  bufCount = origTotalMsgBBS - count;
               read(oldHdrHandle, msgHdrBuf, bufCount * sizeof(msgHdrRec));
            }
            while (bufCount < HDR_BUFSIZE && count + bufCount < newTotalMsgBBS)
            {
               lseek(oldHdrHandle, sli_recNum[count+bufCount] * (s32)sizeof(msgHdrRec), SEEK_SET);
               read (oldHdrHandle, &msgHdrBuf[bufCount++], sizeof(msgHdrRec));
            }
            bufCount = 0;
         }
         temp = sli_recNum[count];
         msgHdrBuf[bufCount].MsgNum     = sli_msgNum[temp];
         if (config.mbOptions.updateChains)
         {
            msgHdrBuf[bufCount].ReplyTo    = sli_prevReply[temp];
            msgHdrBuf[bufCount].SeeAlsoNum = sli_nextReply[temp];
         }
         msgIdxBuf[bufCount].MsgNum = sli_msgNum [temp];
         msgIdxBuf[bufCount].Board  = sli_areaNum[temp];

         if (msgHdrBuf[bufCount].MsgAttr & RA_DELETED)
         {
            memset(&(msgToIdxBuf[bufCount]), 0, sizeof(msgToIdxRec));
            memcpy(&(msgToIdxBuf[bufCount]), "\x0b* Deleted *", 12);
            msgIdxBuf[bufCount].MsgNum = 0xffff;
         }
         else
         if (msgHdrBuf[bufCount].MsgAttr & RA_RECEIVED)
            memcpy (&(msgToIdxBuf[bufCount]), "\x0c* Received *", 13);
         else
            memcpy (&(msgToIdxBuf[bufCount]),
                    &(msgHdrBuf[bufCount].wtLength), sizeof(msgToIdxRec));

         bufCount++;
      }

      write(msgHdrHandle  , msgHdrBuf  , bufCount * sizeof(msgHdrRec  ));
      write(msgIdxHandle  , msgIdxBuf  , bufCount * sizeof(msgIdxRec  ));
      write(msgToIdxHandle, msgToIdxBuf, bufCount * sizeof(msgToIdxRec));

      temp = (filelength(oldHdrHandle) == filelength(msgHdrHandle));

      close(oldHdrHandle);
      close(msgHdrHandle);
      close(msgToIdxHandle);
      close(msgIdxHandle);

//    puts("100%");

      if (temp)
      {
         rename(tempStr1, tempStr2);
         rename(tempStr3, tempStr1);
         unlink(tempStr2);
      }
#ifdef FMAIL
      logEntry("New messages have been sorted", LOG_MSGBASE, 0);
#else // FTOOLS
      logEntry("Messages have been sorted", LOG_MSGBASE, 0);
#endif
   }
   else
   {
      putStr("Updating "dMSGHDR"."MBEXTN"...");

      strcpy(tempStr1, expandNameHudson(dMSGHDR, 0));

      if (  ((msgHdrHandle = open(tempStr1, O_WRONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE)) == -1)
         || ((oldHdrHandle = open(tempStr1, O_RDONLY | O_BINARY)) == -1)
         )
      {
         close(msgHdrHandle);
         logEntry("Can't create purged/packed message files", LOG_ALWAYS, 1);
      }

      if (mbSharing)
         config.mbOptions.mbSharing = 1;

#ifndef FTOOLS
      if (lockMB())
      {
         close(msgHdrHandle);
         close(oldHdrHandle);
         goto exit1;
      }
#endif

      count = 0;
      while (
#ifndef FTOOLS
             (testMBUnlockNow() == 0) &&
#endif
             (bufCount = (read(oldHdrHandle, msgHdrBuf, HDR_BUFSIZE * sizeof(msgHdrRec)) + 1) / sizeof(msgHdrRec)) != 0)
      {
         returnTimeSlice(0);
//       printf("%3u%%\b\b\b\b", (u16)((s32)100 * count / newTotalMsgBBS));

         for (p = 0; p < bufCount; p++)
         {
            msgHdrBuf[p].ReplyTo    = sli_prevReply[sli_recNum[count]];
            msgHdrBuf[p].SeeAlsoNum = sli_nextReply[sli_recNum[count++]];
         }
         write(msgHdrHandle, msgHdrBuf, bufCount * sizeof(msgHdrRec));
      }
//    puts("100%");
      close(oldHdrHandle);
      close(msgHdrHandle);
#ifndef FTOOLS
      unlockMB();
#endif
  }
  if (config.mbOptions.updateChains)
    logEntry("Reply chains have been updated", LOG_MSGBASE, 0);

exit1:
  free(msgHdrBuf);
  free(msgIdxBuf);
  free(msgToIdxBuf);
  free(sli_recNum);
  free(sli_msgNum);
  free(sli_areaNum);
  free(sli_subjectCrcHi);
  free(sli_subjectCrcLo);
exit2:
  if (config.mbOptions.updateChains)
  {
    free(sli_prevReply);
    free(sli_nextReply);
  }
exit3:
  if (config.mbOptions.sortNew)
  {
    free(sli_bsTimeStampHi);
    free(sli_bsTimeStampLo);
  }
}
#endif
