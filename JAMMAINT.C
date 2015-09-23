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

#include <conio.h>
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

#include "areainfo.h"
#include "config.h"
#include "crc.h"
#include "filesys.h"
#include "ftools.h"
#include "jam.h"
#include "log.h"
#include "nodeinfo.h"
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
// return values:
//   0 = everything ok
//  -1 = filesize == 0
// -10 = fileerror
// -20 = not enough memory
//
int InitFileBufs(fhandle h, char **inbuf, char **outbuf, u32 *size)
{
  struct stat s;

  if (fstat(h, &s) != 0)
  {
    *size = 0;
    // *inbuf  = NULL;
    // *outbuf = NULL;
    return -10;
  }

  *size = s.st_size;
  if (s.st_size <= 0)
    return -1;

  *outbuf = malloc(*size);
  if (*outbuf == NULL)
    return -20;

  memset(*outbuf, 0, *size);

  *inbuf = malloc(*size);
  if (*inbuf == NULL)
    return -20;

  if (read(h, *inbuf, *size) != (int)*size)
    return -10;

  return 0;
}
//---------------------------------------------------------------------------
int InitBuf(u32 **buf, u32 size)
{
  if (size > 0)
  {
    size *= sizeof(u32);

    if ((*buf = (u32*)malloc(size)) == NULL)
      return -1;

    memset(*buf, 0, size);
  }
  return 0;
}
//---------------------------------------------------------------------------
void writedata(const char *fn, const char *data, size_t n)
{
  fhandle h;
  if ((h = fsopen(fn, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE, 1)) != -1)
  {
    if (n > 0)
      write(h, data, n);
    fsclose(h);
  }
}
//---------------------------------------------------------------------------
int writefile(fhandle h, char *obuf, int os, char *buf, int s)
{
  if (s == os && 0 == memcmp(obuf, buf, s))
    return 0;  // If data isn't changed, don't write it.

  if (  0 != lseek(h, 0, SEEK_SET)
     || (s >  0 && s != write(h, buf, s))
     || (s < os && 0 != ftruncate(h, s))
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
#ifdef _DEBUG
#define TXTBUFSIZE 0x7000 // moet 16-voud zijn i.v.m. lastread buf
#define LRSIZE     0x3ff0  // 'record' is u16, max 7ff0
#define MSGIDNUM   0x3ff0  // 'record' is u32

static JAMHDRINFO  headerInfo;
static JAMHDR      headerRec;
static JAMIDXREC   indexRec;
static char       *buf      = NULL;
static u16        *lrBuf    = NULL;
static u32        *msgidBuf = NULL;
static u32        *replyBuf = NULL;

s16 JAMmaintOld(rawEchoType *areaPtr, s32 switches, const char *name);

//---------------------------------------------------------------------------
void logComb(const char *fs, const char *ds)
{
  tempStrType ts;
  sprintf(ts, fs, ds);
  newLine();
  logEntry(ts, LOG_DEBUG, 0);
}
//---------------------------------------------------------------------------
void compareFileToBuf(const char *fname, const char *buf, u32 size)
{
  fhandle h;
  if ((h = fsopen(fname, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE, 1)) != -1)
  {
    struct stat s;

    if (fstat(h, &s) == 0)
    {
      if ((off_t)size == s.st_size)
      {
        if (size > 0)
        {
          char *ibuf = malloc(size);
          if (ibuf != NULL)
          {
            if (read(h, ibuf, size) == (int)size)
            {
              if (memcmp(buf, ibuf, size))
                logComb("*** Differ %s", fname);
            }
            else
              logComb("*** Read error %s", fname);

            free(ibuf);
          }
          else
            logComb("*** Can't alloc buffer %s", fname);
        }
      }
      else
        logComb("*** Size differs %s", fname);
    }
    else
      logComb("*** Can't fstat %s", fname);
  }
  else
    logComb("*** Can't open %s", fname);

  fsclose(h);
}
//---------------------------------------------------------------------------
#endif
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
  u32    *lrBuf
       , *msgidBuf
       , *replyBuf;
};
//---------------------------------------------------------------------------
void CleanUp(struct data *d)
{
  if (d->hJDX != -1) { fsclose(d->hJDX); d->hJDX = -1; }
  if (d->hJLR != -1) { fsclose(d->hJLR); d->hJLR = -1; }
  if (d->hJDT != -1) { fsclose(d->hJDT); d->hJDT = -1; }
  if (d->hJHR != -1) { fsclose(d->hJHR); d->hJHR = -1; } // Close JHR last, because of the lock

  freeNull(d->ibJHR   );
  freeNull(d->ibJDT   );
  freeNull(d->ibJDX   );
  freeNull(d->ibJLR   );
  freeNull(d->obJHR   );
  freeNull(d->obJDT   );
  freeNull(d->obJDX   );
  freeNull(d->obJLR   );
  freeNull(d->lrBuf   );
  freeNull(d->msgidBuf);
  freeNull(d->replyBuf);
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
  struct data   d = { -1, -1, -1, -1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
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
              , woJHRpre
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

#ifdef _DEBUG
  if (JAMmaintOld(areaPtr, switches, name))
    return -1;
#endif

  sprintf(tempStr, "Processing JAM area: %s", areaPtr->areaName);
  logEntry(tempStr, LOG_INBOUND | LOG_NOSCRN, 0);
  printf("%s... ", tempStr);

  if (  (d.hJHR = fsopen(expJAMname(areaPtr->msgBasePath, EXT_BCK_HDR), O_RDWR | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE, 1)) == -1
     || (d.hJDT = fsopen(expJAMname(areaPtr->msgBasePath, EXT_BCK_TXT), O_RDWR | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE, 1)) == -1
     || (d.hJDX = fsopen(expJAMname(areaPtr->msgBasePath, EXT_BCK_IDX), O_RDWR | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE, 1)) == -1
     || (d.hJLR = fsopen(expJAMname(areaPtr->msgBasePath, EXT_BCK_LRD), O_RDWR | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE, 1)) == -1
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
    CleanUp(&d);
    newLine();
    logEntry("SHARE is required when Message Base Sharing is enabled", LOG_ALWAYS, 0);
    return 1;
  }

  if (  InitFileBufs(d.hJHR, &d.ibJHR, &d.obJHR, &sizeJHR) < 0
     || InitFileBufs(d.hJDT, &d.ibJDT, &d.obJDT, &sizeJDT) < 0
     || InitFileBufs(d.hJDX, &d.ibJDX, &d.obJDX, &sizeJDX) < 0
     || InitFileBufs(d.hJLR, &d.ibJLR, &d.obJLR, &sizeJLR) < -1
     )
  {
    CleanUp(&d);
    newLine();
    logEntry("Not enough memory available to pack JAM bases or JAM file error", LOG_ALWAYS, 0);
    return 1;
  }

  // make bufs aan de hand van sizeJDX
  maxMsg = sizeJDX / sizeof(JAMIDXREC);

  if (  InitBuf(&d.lrBuf   , maxMsg)
     || InitBuf(&d.msgidBuf, maxMsg)
     || InitBuf(&d.replyBuf, maxMsg)
     )
  {
    CleanUp(&d);
    newLine();
    logEntry("Not enough memory available to pack JAM bases", LOG_ALWAYS, 0);
    return 1;
  }

#ifdef _DEBUG
  logEntry("Updating", LOG_DEBUG | LOG_NOSCRN, 0);
#endif
  printf("Updating ");

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
  logEntry("Maint", LOG_DEBUG | LOG_NOSCRN, 0);
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
      woJHRpre = woJHR;
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
      if (  (  (switches & SW_D)
            && (  delCount
               || (areaPtr->days && areaPtr->days * 86400L + msgTime < startTime)
               || (  (headerRec->Attribute & MSG_READ)
                  && (areaPtr->daysRcvd && areaPtr->daysRcvd * 86400L + msgTime < startTime)
                  )
               )
            )
         || (switches & SW_K)
         )
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
        indexRec->HdrOffset = woJHRpre;

        headerInfo->ActiveMsgs++;
        d.msgidBuf[msgNumNew] = headerRec->MsgIdCRC;
        d.replyBuf[msgNumNew] = headerRec->ReplyCRC;
        headerRec->MsgNum = ++msgNumNew;
        d.lrBuf[msgNum - 1] = msgNumNew;

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
        if (woJHR == highJHR)
          highJHR = woJHRpre;
        woJHR = woJHRpre;
      }
    }
  }

  // Update reply chains
#ifdef _DEBUG
  logEntry("Update reply chains", LOG_DEBUG | LOG_NOSCRN, 0);
#endif

  printf("Reply chains ");

  curMsg = 0;
  woJDX  = 0;

  while (!JAMerror && woJDX < sizeJDX && ++curMsg <= msgNumNew)
  {
    indexRec = (JAMIDXREC*)(d.obJDX + woJDX);
    woJDX += sizeof(JAMIDXREC);
    if (indexRec->UserCRC != 0xffffffffL || indexRec->HdrOffset != 0xffffffffL)
    {
      u32 MsgIdCRC
        , ReplyCRC;

      woJHR = indexRec->HdrOffset;
      headerRec = (JAMHDR*)(d.obJHR + woJHR);

      reply1st = replyNext = replyTo = 0;

      MsgIdCRC = headerRec->MsgIdCRC;
      ReplyCRC = headerRec->ReplyCRC;

      if (MsgIdCRC != 0xffffffffL)
      {
        count = 0;
        while (count < msgNumNew && MsgIdCRC != d.replyBuf[count])
          count++;
        if (count < msgNumNew)
          reply1st = count + 1;
      }
      if (ReplyCRC != 0xffffffffL)
      {
        count = 0;
        while (count < msgNumNew && ReplyCRC != d.msgidBuf[count])
          count++;
        if (count < msgNumNew)
        {
          replyTo = count + 1;
          count = curMsg;
          while (  count < msgNumNew
                && (ReplyCRC != d.replyBuf[count] || MsgIdCRC == d.msgidBuf[count])
                )
            count++;
          if (count < msgNumNew)
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
  logEntry("Update last read info", LOG_DEBUG | LOG_NOSCRN, 0);
#endif
  if (!JAMerror && sizeJLR > 0)
  {
    JAMLREAD *jamlrr = (JAMLREAD *)d.ibJLR
           , *jamlrw = (JAMLREAD *)d.obJLR;
    u32 msgNumHigh = maxMsg > 0 ? maxMsg - 1 : 0;

    printf("LastRead ");

    for (count = 1; count < maxMsg; count++)
      if (!d.lrBuf[count])
        d.lrBuf[count] = d.lrBuf[count - 1];

    temp = sizeJLR / sizeof(JAMLREAD);
    for (count = 0; count < temp; count++)
    {
      jamlrw->UserCRC = jamlrr->UserCRC;
      jamlrw->UserID  = jamlrr->UserID;

      msgNum = jamlrr->LastReadMsg - oldBaseMsgNum;
      jamlrw->LastReadMsg = d.lrBuf[min(msgNum, msgNumHigh)]; // + headerInfo->BaseMsgNum; == always 1
      msgNum = jamlrr->HighReadMsg - oldBaseMsgNum;
      jamlrw->HighReadMsg = d.lrBuf[min(msgNum, msgNumHigh)]; // + headerInfo->BaseMsgNum; == always 1

      jamlrr++;
      jamlrw++;
    }
  }
  {
    s32 ss = sizeJDX - highJDX
           + sizeJDT - highJDT
           + sizeJHR - highJHR;
#ifdef _DEBUG
    if (ss != 0)
    {
      sprintf(tempStr, "Space saved: %d", ss);
      logEntry(tempStr, LOG_DEBUG | LOG_NOSCRN, 0);
    }
#endif
    *spaceSaved += ss;
  }
  if (JAMerror)
  {
    newLine();
    sprintf(tempStr, "Encountered a problem during JAM base maintenance, area %s", areaPtr->areaName);
    logEntry(tempStr, LOG_ALWAYS, 0);
  }
  else
  {
#ifdef _DEBUG
#ifdef MAKEBACKUP
    logEntry("Write backup data", LOG_DEBUG | LOG_NOSCRN, 0);
    // Write (unchanged) input buffers to backup files
    writedata(expJAMname(areaPtr->msgBasePath, "#"BASE_EXT_LRD), d.ibJLR, sizeJLR);
    writedata(expJAMname(areaPtr->msgBasePath, "#"BASE_EXT_IDX), d.ibJDX, sizeJDX);
    writedata(expJAMname(areaPtr->msgBasePath, "#"BASE_EXT_TXT), d.ibJDT, sizeJDT);
    writedata(expJAMname(areaPtr->msgBasePath, "#"BASE_EXT_HDR), d.ibJHR, sizeJHR);
#endif
    compareFileToBuf(expJAMname(areaPtr->msgBasePath, EXT_HDR), d.obJHR, highJHR);
    compareFileToBuf(expJAMname(areaPtr->msgBasePath, EXT_IDX), d.obJDX, highJDX);
    compareFileToBuf(expJAMname(areaPtr->msgBasePath, EXT_TXT), d.obJDT, highJDT);
    compareFileToBuf(expJAMname(areaPtr->msgBasePath, EXT_LRD), d.obJLR, sizeJLR);

    logEntry("Save data", LOG_DEBUG | LOG_NOSCRN, 0);
#endif
    // save data.
    if (  0 != writefile(d.hJLR, d.ibJLR, sizeJLR, d.obJLR, sizeJLR)
       || 0 != writefile(d.hJDX, d.ibJDX, sizeJDX, d.obJDX, highJDX)
       || 0 != writefile(d.hJDT, d.ibJDT, sizeJDT, d.obJDT, highJDT)
       || 0 != writefile(d.hJHR, d.ibJHR, sizeJHR, d.obJHR, highJHR)
       )
      JAMerror = -1;

    puts("Ready");
  }
  CleanUp(&d);
#ifdef _DEBUG
  logEntry("Ready", LOG_DEBUG | LOG_NOSCRN, 0);

#if 0
  printf("\nPress any key to continue... ");
  flush();
  getch();
  newLine();
#endif
#endif

  return JAMerror;
}
#ifdef _DEBUG
//---------------------------------------------------------------------------
s16 JAMmaintOld(rawEchoType *areaPtr, s32 switches, const char *name)
{
  s16           JAMerror = 0;
  fhandle       JHRhandle
              , JDThandle
              , JDXhandle
              , JLRhandle
              , JHRhandleNew
              , JDThandleNew
              , JDXhandleNew
              , JLRhandleNew;
  u32           reply1st
              , replyNext
              , replyTo;
  u16           count;
  u32           msgNum = 0
              , msgNumNew = 0;
  u32           keepSysOpCRC;
  u32           keepSysOpMsgNum;
  u32           oldBaseMsgNum;
  u16           maxMsg, curMsg;
  tempStrType   tempStr;
  s16           temp;
  s16           stat = 0;
  u32           bufCount
              , delCount;
  time_t        msgTime;
  s32           spaceSaved;

#ifdef _DEBUG
  sprintf(tempStr, "O Processing JAM area: %s", areaPtr->areaName);
  logEntry(tempStr, LOG_DEBUG | LOG_NOSCRN, 0);
#endif

  if (  (buf      == NULL && (buf      = malloc(TXTBUFSIZE  )) == NULL)
     || (lrBuf    == NULL && (lrBuf    = malloc(LRSIZE   * 2)) == NULL)
     || (msgidBuf == NULL && (msgidBuf = malloc(MSGIDNUM * 4)) == NULL)
     || (replyBuf == NULL && (replyBuf = malloc(MSGIDNUM * 4)) == NULL)
     )
    logEntry("O Not enough memory available to pack JAM bases", LOG_ALWAYS, 2);

  memset(lrBuf   , 0, LRSIZE   * 2);
  memset(msgidBuf, 0, MSGIDNUM * 4);
  memset(replyBuf, 0, MSGIDNUM * 4);

   if ( (JHRhandle = fsopen(expJAMname(areaPtr->msgBasePath, EXT_HDR),
                            O_RDONLY|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1)) == -1 )
   {
     goto jamx;
   }
   if ( (JDThandle = fsopen(expJAMname(areaPtr->msgBasePath, EXT_TXT),
                            O_RDONLY|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1)) == -1 )
   {
     fsclose(JHRhandle);
     goto jamx;
   }
   if ( (JDXhandle = fsopen(expJAMname(areaPtr->msgBasePath, EXT_IDX),
                            O_RDONLY|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE, 1)) == -1 )
   {
      fsclose(JDThandle);
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
jamx: sprintf(tempStr, "O JAM area %s was not found or was locked", areaPtr->areaName);
      logEntry(tempStr, LOG_ALWAYS, 0);
      return 1;
   }

  stat = lock(JHRhandle, 0L, 1L);
  if (stat == -1 && config.mbOptions.mbSharing)
  {
    newLine();
    logEntry("O SHARE is required when Message Base Sharing is enabled", LOG_ALWAYS, 0);
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

  printf("%s... ", tempStr);
#ifdef _DEBUG
  logEntry("O Updating", LOG_DEBUG | LOG_NOSCRN, 0);
#endif
  printf("Updating ");

  read(JHRhandle, &headerInfo, sizeof(JAMHDRINFO));

  if (areaPtr->msgs && headerInfo.ActiveMsgs > (u32)areaPtr->msgs)
    delCount = headerInfo.ActiveMsgs - areaPtr->msgs;
  else
    delCount = 0;
  oldBaseMsgNum = headerInfo.BaseMsgNum;
  headerInfo.BaseMsgNum = 1;
  headerInfo.ModCounter++;
  headerInfo.ActiveMsgs = 0;
  if (write(JHRhandleNew, &headerInfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO))
  {
    newLine();
    logEntry("O JAMmaint: Can't write new header", LOG_ALWAYS, 0);
    JAMerror = -1;
  }

  keepSysOpMsgNum = 0xFFFFFFFFL;
  if (areaPtr->options.sysopRead && JLRhandle != -1)
  {
    keepSysOpCRC = crc32jam(name);
    while (  keepSysOpMsgNum == 0xFFFFFFFFL && !eof(JLRhandle)
          && (temp = read(JLRhandle, buf, TXTBUFSIZE)) != -1
          )
    {
      temp >>= 4;
      for (count = 0; count < temp; count++)
      {
        if (((JAMLREAD *)buf)[count].UserCRC == keepSysOpCRC)
        {
          keepSysOpMsgNum = ((JAMLREAD *)buf)[count].HighReadMsg - oldBaseMsgNum + 1;
          break;
        }
      }
    }
  }

#ifdef _DEBUG
  logEntry("O Maint", LOG_DEBUG | LOG_NOSCRN, 0);
#endif
  while (!JAMerror && !eof(JDXhandle))
  {
    if (read(JDXhandle, &indexRec, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC))
    {
      newLine();
      logEntry("O JAMmaint: Can't read old index", LOG_ALWAYS, 0);
      JAMerror = -1;
      break; // skip rest of loop
    }
    msgNum++;

    if (indexRec.UserCRC != 0xffffffffL || indexRec.HdrOffset != 0xffffffffL)
    {
      if (fmseek(JHRhandle, indexRec.HdrOffset, SEEK_SET, 5) < 0)
      {
        JAMerror = -1;
        break;
      }
      if (read(JHRhandle, &headerRec, sizeof(JAMHDR)) != sizeof(JAMHDR))
      {
        newLine();
        logEntry("O JAMmaint: Can't read old header", LOG_ALWAYS, 0);
        JAMerror = -1;
        break; /* skip rest of loop */
      }

      if (areaPtr->options.arrivalDate && headerRec.DateProcessed)
        msgTime = headerRec.DateProcessed;
      else
        msgTime = headerRec.DateWritten;

      // TEST MSG AND MARK DELETED
      if ( ((switches & SW_D) &&
           (delCount ||
            (areaPtr->days &&
             areaPtr->days*86400L + msgTime < startTime) ||
            ((headerRec.Attribute & MSG_READ) &&
             (areaPtr->daysRcvd &&
              areaPtr->daysRcvd*86400L + msgTime < startTime)))) ||
          (switches & SW_K) )
      {
        if (headerRec.MsgNum < keepSysOpMsgNum)
        {
          headerRec.Attribute |= MSG_DELETED;
          if ( delCount )
            delCount--;
        }
      }

      if (!(headerRec.Attribute & MSG_DELETED))
      {
        if (fmseek(JDThandle, headerRec.TxtOffset, SEEK_SET, 6) < 0)
        {
          JAMerror = -1;
          break;
        }
        bufCount = headerRec.TxtLen;
        headerRec.TxtOffset = tell(JDThandleNew);

        while (!JAMerror && bufCount)
        {
          if ((temp = read(JDThandle, buf, (u16)min(bufCount, TXTBUFSIZE))) != (u16)min(bufCount, TXTBUFSIZE))
          {
            newLine();
            logEntry("O JAMmaint: Can't read old message text", LOG_ALWAYS, 0);
            JAMerror = -1;
            break;
          }
          if (write(JDThandleNew, buf, temp) != temp)
          {
            newLine();
            logEntry("O JAMmaint: Can't write new message text", LOG_ALWAYS, 0);
            JAMerror = -1;
            break;
          }
          bufCount -= bufCount < TXTBUFSIZE ? bufCount : TXTBUFSIZE;
        }
        if (JAMerror)
          break;

        indexRec.HdrOffset = tell(JHRhandleNew);

        if (headerRec.Attribute & MSG_DELETED)
        {
          indexRec.UserCRC   = 0xffffffffL;
          indexRec.HdrOffset = 0xffffffffL;
        }
        else
        {
          headerInfo.ActiveMsgs++;
          if (msgNumNew < MSGIDNUM)
          {
            msgidBuf[(u16)msgNumNew] = headerRec.MsgIdCRC;
            replyBuf[(u16)msgNumNew] = headerRec.ReplyCRC;
          }
        }

        headerRec.MsgNum = ++msgNumNew;
        if (write(JDXhandleNew, &indexRec, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC))
        {
          newLine();
          logEntry("O JAMmaint: Can't write new index", LOG_ALWAYS, 0);
          JAMerror = -1;
          break;
        }
        if (write(JHRhandleNew, &headerRec, sizeof(JAMHDR)) != sizeof(JAMHDR))
        {
          newLine();
          logEntry("O JAMmaint: Can't write new header", LOG_ALWAYS, 0);
          JAMerror = -1;
          break;
        }
        if (msgNum < LRSIZE)
          lrBuf[(u16)msgNum] = msgNumNew >= 0x0000ffff ? 0xffff : (u16)msgNumNew;

        bufCount = headerRec.SubfieldLen;
        while (bufCount)
        {
          if ( (temp = read(JHRhandle, buf, (u16)min(bufCount, TXTBUFSIZE)))
             != (u16)min(bufCount, TXTBUFSIZE)
             )
          {
            newLine();
            logEntry("O JAMmaint: Can't read old header", LOG_ALWAYS, 0);
            JAMerror = -1;
            break;
          }

          // Remove Re:
          if ((switches & SW_R) && headerRec.SubfieldLen < TXTBUFSIZE)
          {
            if (JAMremoveRe(buf, &headerRec.SubfieldLen))
            {
              if (lseek(JHRhandleNew, -(s32)sizeof(JAMHDR), SEEK_CUR) < 0)
              {
                JAMerror = -1;
                break;
              }
              if (write(JHRhandleNew, &headerRec, sizeof(JAMHDR)) != sizeof(JAMHDR))
              {
                newLine();
                logEntry("O JAMmaint: Can't write new header", LOG_ALWAYS, 0);
                JAMerror = -1;
                break;
              }
              temp = (u16)(bufCount = headerRec.SubfieldLen);
            }
          }
          if (write(JHRhandleNew, buf, temp) != temp)
          {
            newLine();
            logEntry("O JAMmaint: Can't write new header", LOG_ALWAYS, 0);
            JAMerror = -1;
            break;
          }
          bufCount -= bufCount < TXTBUFSIZE ? bufCount : TXTBUFSIZE;
        }
      }
    }
  }
  lseek(JHRhandleNew, 0, SEEK_SET);
  if (write(JHRhandleNew, &headerInfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO))
  {
    newLine();
    logEntry("O JAMmaint: Can't write new header", LOG_ALWAYS, 0);
    JAMerror = -1;
  }

  // Update reply chains
#ifdef _DEBUG
  logEntry("O Update reply chains", LOG_DEBUG | LOG_NOSCRN, 0);
#endif

  printf("Reply chains ");

  if ( msgNumNew > MSGIDNUM )
    maxMsg = MSGIDNUM;
  else
    maxMsg = (u16)msgNumNew;

  curMsg = 0;
  lseek(JDXhandleNew, 0, SEEK_SET);
  while (!JAMerror && !eof(JDXhandleNew) && ++curMsg <= maxMsg)
  {
    if ( read(JDXhandleNew, &indexRec, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC) )
    {
      newLine();
      logEntry("O JAMmaint: Can't read new index", LOG_ALWAYS, 0);
      JAMerror = -1;
      break;
    }
    if (indexRec.UserCRC != 0xffffffffL || indexRec.HdrOffset != 0xffffffffL)
    {
      if (fmseek(JHRhandleNew, indexRec.HdrOffset, SEEK_SET, 7) < 0)
      {
        JAMerror = -1;
        break;
      }
      if (read(JHRhandleNew, &headerRec, sizeof(JAMHDR)) != sizeof(JAMHDR))
      {
        newLine();
        logEntry("O JAMmaint: Can't read new header", LOG_ALWAYS, 0);
        JAMerror = -1;
        break;
      }

      reply1st = replyNext = replyTo = 0;

      if (headerRec.MsgIdCRC != 0xffffffffL)
      {
        count = 0;
        while (count < maxMsg && headerRec.MsgIdCRC != replyBuf[count])
          count++;
        if (count < maxMsg)
          reply1st = count + 1;
      }
      if (headerRec.ReplyCRC != 0xffffffffL)
      {
        count = 0;
        while (count < maxMsg && headerRec.ReplyCRC != msgidBuf[count])
          count++;
        if (count < maxMsg)
        {
          replyTo = count + 1;
          count = curMsg;
          while (  count < maxMsg && (headerRec.ReplyCRC != replyBuf[count]
                || headerRec.MsgIdCRC == msgidBuf[count])
                )
            count++;
          if (count < maxMsg)
            replyNext = count + 1;
        }
      }
      if (  headerRec.Reply1st  != reply1st
         || headerRec.ReplyTo   != replyTo
         || headerRec.ReplyNext != replyNext
         )
      {
        headerRec.Reply1st  = reply1st;
        headerRec.ReplyTo   = replyTo;
        headerRec.ReplyNext = replyNext;

        if (fmseek(JHRhandleNew, indexRec.HdrOffset, SEEK_SET, 8) < 0)
        {
          JAMerror = -1;
          break;
        }
        if (write(JHRhandleNew, &headerRec, sizeof(JAMHDR)) != sizeof(JAMHDR))
        {
          newLine();
          logEntry("O JAMmaint: Can't write new header", LOG_ALWAYS, 0);
          JAMerror = -1;
          break;
        }
      }
    }
  }

  // Update last read info
#ifdef _DEBUG
  logEntry("O Update last read info", LOG_DEBUG | LOG_NOSCRN, 0);
#endif
  if ( !JAMerror && JLRhandle != -1 )
  {
    printf("LastRead ");

    if (!JAMerror)
      for (count = 1; count < LRSIZE; count++)
        if (!lrBuf[count])
          lrBuf[count] = lrBuf[count - 1];

    lseek(JLRhandle, 0, SEEK_SET);
    while (!JAMerror && !eof(JLRhandle) && (temp = read(JLRhandle, buf, TXTBUFSIZE)) != -1)
    {
      temp >>= 4;
      for (count = 0; count < temp; count++)
      {
        msgNum = ((JAMLREAD *)buf)[count].LastReadMsg-oldBaseMsgNum;
        ((JAMLREAD *)buf)[count].LastReadMsg = lrBuf[(msgNum >= LRSIZE-1) ? LRSIZE-1 : (u16)msgNum] + headerInfo.BaseMsgNum;
        msgNum = ((JAMLREAD *)buf)[count].HighReadMsg-oldBaseMsgNum;
        ((JAMLREAD *)buf)[count].HighReadMsg = lrBuf[(msgNum >= LRSIZE-1) ? LRSIZE-1 : (u16)msgNum] + headerInfo.BaseMsgNum;
      }
      if (write(JLRhandleNew, buf, temp<<4) != (temp<<4))
      {
        newLine();
        logEntry("O JAMmaint: Can't write new last read file", LOG_ALWAYS, 0);
        JAMerror = -1;
        break;
      }
    }
  }
  spaceSaved = filelength(JDXhandle) - filelength(JDXhandleNew)
             + filelength(JDThandle) - filelength(JDThandleNew)
             + filelength(JHRhandle) - filelength(JHRhandleNew);

  if (spaceSaved != 0)
  {
    sprintf(tempStr, "o Space saved: %d", spaceSaved);
    logEntry(tempStr, LOG_DEBUG | LOG_NOSCRN, 0);
  }

  fsclose(JLRhandleNew);
  fsclose(JDXhandleNew);
  fsclose(JDThandleNew);
  fsclose(JHRhandleNew);
  fsclose(JLRhandle);
  fsclose(JDXhandle);
  fsclose(JDThandle);
  fsclose(JHRhandle);

  if (JAMerror)
  {
    fsunlink(expJAMname(areaPtr->msgBasePath, EXT_NEW_HDR), 1);
    fsunlink(expJAMname(areaPtr->msgBasePath, EXT_NEW_TXT), 1);
    fsunlink(expJAMname(areaPtr->msgBasePath, EXT_NEW_IDX), 1);
    fsunlink(expJAMname(areaPtr->msgBasePath, EXT_NEW_LRD), 1);
    newLine();
    sprintf(tempStr, "O Disk problems during JAM base maintenance, area %s", areaPtr->areaName);
    logEntry(tempStr, LOG_ALWAYS, 0);
  }
  else
  {
    fsunlink(expJAMname(areaPtr->msgBasePath, EXT_BCK_HDR), 1);
    fsunlink(expJAMname(areaPtr->msgBasePath, EXT_BCK_TXT), 1);
    fsunlink(expJAMname(areaPtr->msgBasePath, EXT_BCK_IDX), 1);
    fsunlink(expJAMname(areaPtr->msgBasePath, EXT_BCK_LRD), 1);

    fsrename(expJAMname(areaPtr->msgBasePath, EXT_HDR), expJAMname(areaPtr->msgBasePath, EXT_BCK_HDR), 1);
    fsrename(expJAMname(areaPtr->msgBasePath, EXT_TXT), expJAMname(areaPtr->msgBasePath, EXT_BCK_TXT), 1);
    fsrename(expJAMname(areaPtr->msgBasePath, EXT_IDX), expJAMname(areaPtr->msgBasePath, EXT_BCK_IDX), 1);
    fsrename(expJAMname(areaPtr->msgBasePath, EXT_LRD), expJAMname(areaPtr->msgBasePath, EXT_BCK_LRD), 1);

    fsrename(expJAMname(areaPtr->msgBasePath, EXT_NEW_HDR), expJAMname(areaPtr->msgBasePath, EXT_HDR), 1);
    fsrename(expJAMname(areaPtr->msgBasePath, EXT_NEW_TXT), expJAMname(areaPtr->msgBasePath, EXT_TXT), 1);
    fsrename(expJAMname(areaPtr->msgBasePath, EXT_NEW_IDX), expJAMname(areaPtr->msgBasePath, EXT_IDX), 1);
    fsrename(expJAMname(areaPtr->msgBasePath, EXT_NEW_LRD), expJAMname(areaPtr->msgBasePath, EXT_LRD), 1);

    puts("Ready");
  }
#ifdef _DEBUG
  logEntry("O Ready", LOG_DEBUG | LOG_NOSCRN, 0);
#endif
  return JAMerror;
}
//---------------------------------------------------------------------------
#endif