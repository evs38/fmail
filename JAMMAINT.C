//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2015 Wilfred van Velzen
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

#include "areainfo.h"
#include "config.h"
#include "crc.h"
#include "filesys.h"
#include "ftools.h"
#include "jam.h"
#include "log.h"
#include "nodeinfo.h"
#include "output.h"
#include "utils.h"

#ifdef __BORLANDC__
#define ftruncate(h, s) chsize((h), (s))
#endif

extern configType config;
extern s32        startTime;

//---------------------------------------------------------------------------
s16 JAMremoveRe(char *buf, u32 *bufsize)
{
  u32         size
            , slen
            , totalsize = 0;
  tempStrType tempStr;
  char       *helpPtr;

  while (((JAMBINSUBFIELD *)buf)->LoID != JAMSFLD_SUBJECT)
  {
    size = sizeof(JAMBINSUBFIELD) + ((JAMBINSUBFIELD *)buf)->DatLen;
    buf += size;
    totalsize += size;
    if (totalsize >= *bufsize)
      return 0;
  }
  size = ((JAMBINSUBFIELD *)buf)->DatLen;
  buf += sizeof(JAMBINSUBFIELD);
  totalsize += sizeof(JAMBINSUBFIELD);
  memset(tempStr, 0, sizeof(tempStrType));
  memcpy(tempStr, buf, min(size, sizeof(tempStrType)));
  helpPtr = tempStr;
  while (  !strnicmp(helpPtr, "RE:", 3)
        || !strnicmp(helpPtr, "(R)", 3)
        || !strnicmp(helpPtr, "RE^", 3)
        )
  {
    helpPtr += 3;
    if (*(helpPtr - 1) == '^')
      while (*helpPtr && *helpPtr != ':')
        ++helpPtr;

    while (*helpPtr == ' ')
      helpPtr++;
  }
  slen = strlen(helpPtr);
  if (slen < size)
  {
    size -= slen;
    memmove(buf, buf + size, *bufsize - totalsize - size);
    *bufsize -= size;
    buf -= sizeof(JAMBINSUBFIELD);
    ((JAMBINSUBFIELD *)buf)->DatLen -= size;

    return 1;
  }
  return 0;
}
//---------------------------------------------------------------------------
const char *expJAMname(const char *basename, const char *ext)
{
  static tempStrType tempStr[2];
  static u16 count = 0;

  count = 1 - count;

  strcpy(stpcpy(stpcpy(tempStr[count], basename), "."), ext);

  return tempStr[count];
}
//---------------------------------------------------------------------------
int InitFileBufs(fhandle h, char **inbuf, char **outbuf, u32 *size)
{
  struct stat s;

  if (fstat(h, &s) == 0)
  {
    *size = s.st_size;
    if (*size > 0)
    {
      *outbuf = malloc(*size);
      if (*outbuf != NULL)
      {
        memset(*outbuf, 0, *size);
        *inbuf = malloc(*size);
        if (*inbuf != NULL)
          if (read(h, *inbuf, *size) == (int)*size)
            return 0;
      }
    }
  }
  else
  {
    *size = 0;
    // *inbuf  = NULL;
    // *outbuf = NULL;
  }
  return -1;
}
//---------------------------------------------------------------------------
void writedata(const char *fn, const char *data, size_t n)
{
  fhandle h;
  if ((h = fsopen(fn, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE, 1)) != -1)
  {
    write(h, data, n);
    fsclose(h);
  }
}
//---------------------------------------------------------------------------
int writefile(fhandle h, char *buf, int s)
{
  if (  0 != lseek(h, 0, SEEK_SET)
     || s != write(h, buf, s)
     || 0 != ftruncate(h, s)
     )
  {
    tempStrType tstr;
    sprintf(tstr, "Problem writing JAM base file (your fucked!) [%s]", strerror(errno));
    logEntry(tstr, LOG_ALWAYS, 2);
    return 1;
  }
  return 0;
}
//---------------------------------------------------------------------------
char *memwrite(char *dest, u32 *offset, const char *src, u32 n, u32 *high, u32 max)
{
  if (*offset + n > max)
    logEntry("Buffer error on JAM bases maintenance", LOG_ALWAYS, 2);

  dest += *offset;
  memcpy(dest, src, n);
  *offset += n;
  if (*offset > *high)
    *high = *offset;

  return dest;
}
//---------------------------------------------------------------------------
#define LRSIZE     0x3ff0  // 'record' is u16, max 7ff0
#define MSGIDNUM   0x3ff0  // 'record' is u32

static u16        *lrBuf    = NULL;
static u32        *msgidBuf = NULL;
static u32        *replyBuf = NULL;

//---------------------------------------------------------------------------
struct data
{
  fhandle hJHR
        , hJDT
        , hJDX
        , hJLR;
  char   *ibJHR
       , *ibJDT
       , *ibJDX
       , *ibJLR
       , *obJHR
       , *obJDT
       , *obJDX
       , *obJLR;
};
//---------------------------------------------------------------------------
void CleanUp(struct data *d)
{
  if (d->hJDX != -1) fsclose(d->hJDX);
  if (d->hJLR != -1) fsclose(d->hJLR);
  if (d->hJDT != -1) fsclose(d->hJDT);
  if (d->hJHR != -1) fsclose(d->hJHR); // Close JHR last, because of the lock

  if (d->ibJHR != NULL) { free(d->ibJHR); d->ibJHR = NULL; }
  if (d->ibJDT != NULL) { free(d->ibJDT); d->ibJDT = NULL; }
  if (d->ibJDX != NULL) { free(d->ibJDX); d->ibJDX = NULL; }
  if (d->ibJLR != NULL) { free(d->ibJLR); d->ibJLR = NULL; }
  if (d->obJHR != NULL) { free(d->obJHR); d->obJHR = NULL; }
  if (d->obJDT != NULL) { free(d->obJDT); d->obJDT = NULL; }
  if (d->obJDX != NULL) { free(d->obJDX); d->obJDX = NULL; }
  if (d->obJLR != NULL) { free(d->obJLR); d->obJLR = NULL; }
}
//---------------------------------------------------------------------------
// SW_K - Delete all messages in board
// SW_B - Keep backup files   ??? On tossing this is the toss from bad option!?
// SW_D - Delete selected messages
// SW_R - Remove Re:
// SW_P - Pack message base: WERKT NIET WEGENS DOORLOPEN INDEX (0xFFFFFFFF!)
//---------------------------------------------------------------------------
s16 JAMmaint(rawEchoType *areaPtr, s32 switches, const char *name, s32 *spaceSaved)
{
  s16           JAMerror = 0;
  u16           tabPos;
  struct data   d = { -1, -1, -1, -1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
  u32           reply1st
              , replyNext
              , replyTo
              , count
              , msgNum    = 0
              , msgNumNew = 0
              , keepSysOpCRC
              , keepSysOpMsgNum
              , oldBaseMsgNum
              , maxMsg
              , curMsg
              , temp
              , delCount
              , sizeJHR = 0
              , sizeJDX = 0
              , sizeJDT = 0
              , sizeJLR = 0
              , highJHR = 0
              , highJDX = 0
              , highJDT = 0
              , woJHR   = 0
              , woJDX   = 0
              , woJDT   = 0
              , orgSubfieldLen
              ;
  tempStrType   tempStr;
  time_t        msgTime;
  char         *rpJHR
             , *rpJDX
             , *rpJDT
             , *wpSubf;
  JAMHDRINFO   *headerInfo;
  JAMIDXREC    *indexRec;
  JAMHDR       *headerRec;

  sprintf(tempStr, "Processing JAM area: %s", areaPtr->areaName);
  logEntry(tempStr, LOG_INBOUND, 0);
  printString(tempStr);
  printString("... ");
  flush();

  if (  (lrBuf    == NULL && (lrBuf    = malloc(LRSIZE   * 2)) == NULL)
     || (msgidBuf == NULL && (msgidBuf = malloc(MSGIDNUM * 4)) == NULL)
     || (replyBuf == NULL && (replyBuf = malloc(MSGIDNUM * 4)) == NULL)
     )
    logEntry("Not enough memory available to pack JAM bases", LOG_ALWAYS, 2);

  memset(lrBuf   , 0, LRSIZE   * 2);
  memset(msgidBuf, 0, MSGIDNUM * 4);
  memset(replyBuf, 0, MSGIDNUM * 4);

  if (  (d.hJHR = fsopen(expJAMname(areaPtr->msgBasePath, EXT_HDR), O_RDWR | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE, 1)) == -1
     || (d.hJDT = fsopen(expJAMname(areaPtr->msgBasePath, EXT_TXT), O_RDWR | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE, 1)) == -1
     || (d.hJDX = fsopen(expJAMname(areaPtr->msgBasePath, EXT_IDX), O_RDWR | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE, 1)) == -1
     || (d.hJLR = fsopen(expJAMname(areaPtr->msgBasePath, EXT_LRD), O_RDWR | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE, 1)) == -1
     )
  {
    CleanUp(&d);
    newLine();
    sprintf(tempStr, "JAM area %s was not found or was locked", areaPtr->areaName);
    logEntry(tempStr, LOG_ALWAYS, 0);
    return 1;
  }

  if (lock(d.hJHR, 0L, 1L) == -1 && config.mbOptions.mbSharing)
  {
    logEntry("SHARE is required when Message Base Sharing is enabled", LOG_ALWAYS, 0);
    CleanUp(&d);
    return 1;
  }

  if (  0 != InitFileBufs(d.hJHR, &d.ibJHR, &d.obJHR, &sizeJHR)
     || 0 != InitFileBufs(d.hJDT, &d.ibJDT, &d.obJDT, &sizeJDT)
     || 0 != InitFileBufs(d.hJDX, &d.ibJDX, &d.obJDX, &sizeJDX)
     || 0 != InitFileBufs(d.hJLR, &d.ibJLR, &d.obJLR, &sizeJLR)
     )
  {
    CleanUp(&d);
    newLine();
    logEntry("Not enough memory available to pack JAM bases", LOG_ALWAYS, 0);
    return 1;
  }

  tabPos = getTab();
#ifdef _DEBUG
  logEntry("Updating", LOG_DEBUG, 0);
#endif
  printString("Updating ");
  updateCurrLine();

  woJHR = 0;
  rpJHR = d.ibJHR;
  memwrite(d.obJHR, &woJHR, rpJHR, sizeof(JAMHDRINFO), &highJHR, sizeJHR);
  rpJHR += sizeof(JAMHDRINFO);
  headerInfo = (JAMHDRINFO*)d.obJHR;

  if (areaPtr->msgs && headerInfo->ActiveMsgs > (u32)areaPtr->msgs)
    delCount = headerInfo->ActiveMsgs - areaPtr->msgs;
  else
    delCount = 0;
  oldBaseMsgNum = headerInfo->BaseMsgNum;
  headerInfo->BaseMsgNum = 1;
  headerInfo->ModCounter++;
  headerInfo->ActiveMsgs = 0;

  keepSysOpMsgNum = 0xFFFFFFFFL;
  if (areaPtr->options.sysopRead && sizeJLR > 0)
  {
    keepSysOpCRC = crc32jam(name);
    temp = sizeJLR / sizeof(JAMLREAD);
    for (count = 0; count < temp; count++)
      if (((JAMLREAD *)d.ibJLR)[count].UserCRC == keepSysOpCRC)
      {
        keepSysOpMsgNum = ((JAMLREAD *)d.ibJLR)[count].HighReadMsg - oldBaseMsgNum + 1;
        break;
      }
  }

#ifdef _DEBUG
  logEntry("Maint", LOG_DEBUG, 0);
#endif
  rpJDX = d.ibJDX;
  woJDX = 0;
  woJDT = 0;
  while (!JAMerror && rpJDX < d.ibJDX + sizeJDX)
  {
    indexRec = (JAMIDXREC*)rpJDX;
    rpJDX += sizeof(JAMIDXREC);
    msgNum++;

    if (indexRec->UserCRC != 0xffffffffL || indexRec->HdrOffset != 0xffffffffL)
    {
      if (indexRec->HdrOffset > sizeJHR - sizeof(JAMHDR))
      {
        JAMerror = -1;
        break;
      }
      rpJHR = d.ibJHR + indexRec->HdrOffset;

      headerRec = (JAMHDR*)memwrite(d.obJHR, &woJHR, rpJHR, sizeof(JAMHDR), &highJHR, sizeJHR);
      rpJHR += sizeof(JAMHDR);
      orgSubfieldLen = headerRec->SubfieldLen;
      wpSubf    =          memwrite(d.obJHR, &woJHR, rpJHR, orgSubfieldLen, &highJHR, sizeJHR);
      rpJHR += orgSubfieldLen;

      if (areaPtr->options.arrivalDate && headerRec->DateProcessed)
        msgTime = headerRec->DateProcessed;
      else
        msgTime = headerRec->DateWritten;

      // TEST MSG AND MARK DELETED
      if ( ((switches & SW_D) &&
           (delCount ||
            (areaPtr->days &&
             areaPtr->days * 86400L + msgTime < startTime) ||
            ((headerRec->Attribute & MSG_READ) &&
             (areaPtr->daysRcvd &&
              areaPtr->daysRcvd * 86400L + msgTime < startTime)))) ||
          (switches & SW_K) )
      {
        if (headerRec->MsgNum < keepSysOpMsgNum)
        {
          headerRec->Attribute |= MSG_DELETED;
          if (delCount)
            delCount--;
        }
      }

      if (!(headerRec->Attribute & MSG_DELETED))
      {
        if (headerRec->TxtOffset >= sizeJDT)
        {
          JAMerror = -1;
          break;
        }
        rpJDT = d.ibJDT + headerRec->TxtOffset;
        headerRec->TxtOffset = woJDT;

        memwrite(d.obJDT, &woJDT, rpJDT, headerRec->TxtLen, &highJDT, sizeJDT);
        // rpJDT += headerRec->TxtLen;  // Not used after this point

        indexRec = (JAMIDXREC*)memwrite(d.obJDX, &woJDX, (char*)indexRec, sizeof(JAMIDXREC), &highJDX, sizeJDX);

        indexRec->HdrOffset = woJHR;

        if (headerRec->Attribute & MSG_DELETED)
        {
          indexRec->UserCRC   = 0xffffffffL;
          indexRec->HdrOffset = 0xffffffffL;
        }
        else
        {
          headerInfo->ActiveMsgs++;
          if (msgNumNew < MSGIDNUM)
          {
            msgidBuf[(u16)msgNumNew] = headerRec->MsgIdCRC;
            replyBuf[(u16)msgNumNew] = headerRec->ReplyCRC;
          }
        }

        headerRec->MsgNum = ++msgNumNew;

        if (msgNum < LRSIZE)
          lrBuf[(u16)msgNum] = msgNumNew >= 0x0000ffff ? 0xffff : (u16)msgNumNew;

        // Remove Re:
        if (switches & SW_R)
          if (JAMremoveRe(wpSubf, &headerRec->SubfieldLen))    // SubfieldLen can be changed
          {
            long diff = orgSubfieldLen - headerRec->SubfieldLen;
            if (woJHR == highJHR)
              highJHR -= diff;
            woJHR -= diff;
          }
      }
      else
      {
        // headerRec & subfield not writen, reset woJHR en highJHR
        long diff = sizeof(JAMHDR) + orgSubfieldLen;
        if (woJHR == highJHR)
          highJHR -= diff;
        woJHR -= diff;
      }
    }
  }

  // Update reply chains
#ifdef _DEBUG
  logEntry("Update reply chains", LOG_DEBUG, 0);
#endif

  gotoTab(tabPos);
  printString("Reply chains ");
  updateCurrLine();

  if (msgNumNew > MSGIDNUM)
    maxMsg = MSGIDNUM;
  else
    maxMsg = (u16)msgNumNew;

  curMsg = 0;
  woJDX = 0;

  while (!JAMerror && woJDX < sizeJDX && ++curMsg <= maxMsg)
  {
    indexRec = (JAMIDXREC*)(d.obJDX + woJDX);
    woJDX += sizeof(JAMIDXREC);
    if (indexRec->UserCRC != 0xffffffffL || indexRec->HdrOffset != 0xffffffffL)
    {
      woJHR = indexRec->HdrOffset;
      headerRec = (JAMHDR*)(d.obJHR + woJHR);

      reply1st = replyNext = replyTo = 0;

      if (headerRec->MsgIdCRC != 0xffffffffL)
      {
        count = 0;
        while (count < maxMsg && headerRec->MsgIdCRC != replyBuf[count])
          count++;
        if (count < maxMsg)
          reply1st = count + 1;
      }
      if (headerRec->ReplyCRC != 0xffffffffL)
      {
        count = 0;
        while (count < maxMsg && headerRec->ReplyCRC != msgidBuf[count])
          count++;
        if (count < maxMsg)
        {
          replyTo = count + 1;
          count = curMsg;
          while (  count < maxMsg && (headerRec->ReplyCRC != replyBuf[count]
                || headerRec->MsgIdCRC == msgidBuf[count])
                )
            count++;
          if (count < maxMsg)
            replyNext = count + 1;
        }
      }
      headerRec->Reply1st  = reply1st;
      headerRec->ReplyTo   = replyTo;
      headerRec->ReplyNext = replyNext;
    }
  }

  // Update last read info
#ifdef _DEBUG
  logEntry("Update last read info", LOG_DEBUG, 0);
#endif
  if (!JAMerror && sizeJLR > 0)
  {
    gotoTab(tabPos);
    printStringFill("LastRead ");
    updateCurrLine();

    for (count = 1; count < LRSIZE; count++)
      if (!lrBuf[count])
        lrBuf[count] = lrBuf[count - 1];

    temp = sizeJLR / sizeof(JAMLREAD);
    for (count = 0; count < temp; count++)
    {
      msgNum = ((JAMLREAD *)d.ibJLR)[count].LastReadMsg - oldBaseMsgNum;
      ((JAMLREAD *)d.obJLR)[count].LastReadMsg = lrBuf[(msgNum >= LRSIZE - 1) ? LRSIZE - 1 : (u16)msgNum] + headerInfo->BaseMsgNum;
      msgNum = ((JAMLREAD *)d.ibJLR)[count].HighReadMsg - oldBaseMsgNum;
      ((JAMLREAD *)d.obJLR)[count].HighReadMsg = lrBuf[(msgNum >= LRSIZE - 1) ? LRSIZE - 1 : (u16)msgNum] + headerInfo->BaseMsgNum;
    }
  }
  *spaceSaved += sizeJDX - highJDX
               + sizeJDT - highJDT
               + sizeJHR - highJHR;

  gotoTab(tabPos);
  if (JAMerror)
  {
    newLine();
    sprintf(tempStr, "Encountered a problem during JAM base maintenance, area %s", areaPtr->areaName);
    logEntry(tempStr, LOG_ALWAYS, 0);
  }
  else
  {
#ifdef _DEBUG
    // Write (unchanged) input buffers to backup files
    writedata(expJAMname(areaPtr->msgBasePath, EXT_BCK_LRD), d.ibJLR, sizeJLR);
    writedata(expJAMname(areaPtr->msgBasePath, EXT_BCK_IDX), d.ibJDX, sizeJDX);
    writedata(expJAMname(areaPtr->msgBasePath, EXT_BCK_TXT), d.ibJDT, sizeJDT);
    writedata(expJAMname(areaPtr->msgBasePath, EXT_BCK_HDR), d.ibJHR, sizeJHR);
#endif
    // save data.
    if (  0 != writefile(d.hJLR, d.obJLR, sizeJLR)
       || 0 != writefile(d.hJDX, d.obJDX, highJDX)
       || 0 != writefile(d.hJDT, d.obJDT, highJDT)
       || 0 != writefile(d.hJHR, d.obJHR, highJHR)
       )
      JAMerror = -1;

    printStringFill("Ready");
    newLine();
  }
  CleanUp(&d);
  flush();
#ifdef _DEBUG
  logEntry("Ready", LOG_DEBUG, 0);
#endif
  return JAMerror;
}
//---------------------------------------------------------------------------
