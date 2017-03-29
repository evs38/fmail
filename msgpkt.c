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
#include <dir.h>
//#include <dos.h>
#include <share.h>
#endif // __WIN32__
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "fmail.h"

#include "archive.h"
#include "areainfo.h"
#include "log.h"
#include "minmax.h"
#include "msgpkt.h"
#include "mtask.h"
#include "nodeinfo.h"
#include "os.h"
#include "os_string.h"
#include "utils.h"
#include "version.h"

//---------------------------------------------------------------------------
const u16 PKT_BUFSIZE = 32000;

#include "pshpack1.h"

typedef struct
{
  u16 two;
  u16 srcNode;
  u16 destNode;
  u16 srcNet;
  u16 destNet;
  u16 attribute;
  u16 cost;
} pmHdrType;

#include "poppack.h"

u16 inBuf
  , startBuf
  , endBuf
  , oldStart;

char      *pktRdBuf;
fhandle    pktHandle;
pmHdrType  pmHdr;
s16        zero = 0;

extern char *months,
            *upcaseMonths;

//extern globVarsType globVars;
//extern configType   config;

//---------------------------------------------------------------------------
void initPkt(void)
{
  if ((pktRdBuf = (char*)malloc(PKT_BUFSIZE)) == NULL)
    logEntry("Error allocating memory for packet read buffer", LOG_ALWAYS, 2);

  pmHdr.two = 2;


}
//---------------------------------------------------------------------------
void deInitPkt(void)
{
  free(pktRdBuf);
}
//---------------------------------------------------------------------------
s16 openPktRd(char *pktName, s16 secure)
{
   u16           srcCapability;
   nodeInfoType *nodeInfoPtr;
   char          password[9];
   pktHdrType    msgPktHdr;

   startBuf = PKT_BUFSIZE;
   endBuf   = PKT_BUFSIZE;
   pktRdBuf[PKT_BUFSIZE - 1] = 0;
   globVars.remoteCapability = 0;
   globVars.packetDestAka = 0;
   globVars.remoteProdCode = 0x0104;

   if ((pktHandle = _sopen(pktName, O_RDONLY | O_BINARY, SH_DENYRW)) == -1)
   {
      logEntryf(LOG_ALWAYS, 0, "Error opening packet file: %s", pktName);

      return 1;
   }

   if (read(pktHandle, &msgPktHdr, 58) < 58)
   {
      close(pktHandle);
      addExtension(pktName, ".error");
      logEntryf(LOG_ALWAYS, 0, "Error reading packet header in file: %s, renamed with extension: '.error'", pktName);

      return 1;
   }

   globVars.remoteProdCode = (u16)msgPktHdr.prodCodeLo;

   // Capability word

   srcCapability = msgPktHdr.cwValid;
   srcCapability = ((srcCapability >> 8) | ((char)srcCapability) << 8) & 0x7fff;
   if (srcCapability != (msgPktHdr.capWord & 0x7fff))
      srcCapability = 0;

   if ((srcCapability & 1) || msgPktHdr.prodCodeLo == 0x0c
                           || msgPktHdr.prodCodeLo == 0x1a
                           || msgPktHdr.prodCodeLo == 0x3f
                           || msgPktHdr.prodCodeLo == 0x45
      )
   {
      // 4-d info

      globVars.remoteCapability = srcCapability;

      while (  MAX_AKAS > globVars.packetDestAka
            && (  config.akaList[globVars.packetDestAka].nodeNum.zone == 0
               || msgPktHdr.destZone != config.akaList[globVars.packetDestAka].nodeNum.zone
               || (  (  msgPktHdr.destNet  != config.akaList[globVars.packetDestAka].nodeNum.net
                     || msgPktHdr.destNode != config.akaList[globVars.packetDestAka].nodeNum.node
                     || (  msgPktHdr.destPoint != config.akaList[globVars.packetDestAka].nodeNum.point
                        && config.akaList[globVars.packetDestAka].nodeNum.point != 0
                        )
                     )
                  && (  msgPktHdr.destNet   != config.akaList[globVars.packetDestAka].fakeNet
                     || msgPktHdr.destNode  != config.akaList[globVars.packetDestAka].nodeNum.point
                     || msgPktHdr.destPoint != 0
                     )
                  )
               )
            )
         globVars.packetDestAka++;

#ifdef _DEBUG0
      logEntryf(LOG_DEBUG, 0, "DEBUG Type 2+ dest zone: %d", msgPktHdr.destZone);
#endif
      // FSC-0048 rev. 2

      if (msgPktHdr.origPoint && msgPktHdr.origNet == 0xffff)
         msgPktHdr.origNet = msgPktHdr.auxNet;

      globVars.packetSrcNode.zone   = msgPktHdr.origZone;
      globVars.packetSrcNode.net    = msgPktHdr.origNet;
      globVars.packetSrcNode.node   = msgPktHdr.origNode;
      globVars.packetSrcNode.point  = msgPktHdr.origPoint;
      globVars.packetDestNode.zone  = msgPktHdr.destZone;
      globVars.packetDestNode.net   = msgPktHdr.destNet;
      globVars.packetDestNode.node  = msgPktHdr.destNode;
      globVars.packetDestNode.point = msgPktHdr.destPoint;

      globVars.versionHi = msgPktHdr.serialNoMajor;
      globVars.versionLo = msgPktHdr.serialNoMinor;

      globVars.remoteProdCode |= (u16)msgPktHdr.prodCodeHi<<8;
   }
   else
   {
      // Detect FSC-0045

      if (  ((FSC45pktHdrType*)&msgPktHdr)->packetType == 2
         && ((FSC45pktHdrType*)&msgPktHdr)->subVersion == 2
         )
      {
         globVars.remoteCapability = -1;

         while ((globVars.packetDestAka < MAX_AKAS) &&
                ((config.akaList[globVars.packetDestAka].nodeNum.zone == 0) ||
                 (((FSC45pktHdrType*)&msgPktHdr)->destZone  != config.akaList[globVars.packetDestAka].nodeNum.zone) ||
                 (((FSC45pktHdrType*)&msgPktHdr)->destNet   != config.akaList[globVars.packetDestAka].nodeNum.net)  ||
                 (((FSC45pktHdrType*)&msgPktHdr)->destNode  != config.akaList[globVars.packetDestAka].nodeNum.node) ||
                 ((((FSC45pktHdrType*)&msgPktHdr)->destPoint != config.akaList[globVars.packetDestAka].nodeNum.point) &&
                  (config.akaList[globVars.packetDestAka].nodeNum.point != 0))))
            globVars.packetDestAka++;

#ifdef _DEBUG0
         logEntryf(LOG_DEBUG, 0, "DEBUG FSC-0045 dest AKA: %d", globVars.packetDestAka);
#endif
         globVars.packetSrcNode .net   = ((FSC45pktHdrType*)&msgPktHdr)->origNet;
         globVars.packetSrcNode .zone  = ((FSC45pktHdrType*)&msgPktHdr)->origZone;
         globVars.packetSrcNode .node  = ((FSC45pktHdrType*)&msgPktHdr)->origNode;
         globVars.packetSrcNode .point = ((FSC45pktHdrType*)&msgPktHdr)->origPoint;
         globVars.packetDestNode.zone  = ((FSC45pktHdrType*)&msgPktHdr)->destZone;
         globVars.packetDestNode.net   = ((FSC45pktHdrType*)&msgPktHdr)->destNet;
         globVars.packetDestNode.node  = ((FSC45pktHdrType*)&msgPktHdr)->destNode;
         globVars.packetDestNode.point = ((FSC45pktHdrType*)&msgPktHdr)->destPoint;
      }
      else
      {
         // 2-d / fake-net info

         while (  globVars.packetDestAka < MAX_AKAS
               && (  config.akaList[globVars.packetDestAka].nodeNum.zone == 0
                  || (  (  msgPktHdr.destNet  != config.akaList[globVars.packetDestAka].nodeNum.net
                        || msgPktHdr.destNode != config.akaList[globVars.packetDestAka].nodeNum.node
                        )
                     && (  msgPktHdr.destNet  != config.akaList[globVars.packetDestAka].fakeNet
                        || msgPktHdr.destNode != config.akaList[globVars.packetDestAka].nodeNum.point
                        )
                     )
                  )
               )
            globVars.packetDestAka++;

#ifdef _DEBUG0
         logEntryf(LOG_DEBUG, 0, "DEBUG Type 2 dest AKA: %d", globVars.packetDestAka);
#endif
         globVars.packetSrcNode .zone  = 0;
         globVars.packetSrcNode .net   = msgPktHdr.origNet;
         globVars.packetSrcNode .node  = msgPktHdr.origNode;
         globVars.packetSrcNode .point = 0;
         globVars.packetDestNode.zone  = 0;
         globVars.packetDestNode.net   = msgPktHdr.destNet;
         globVars.packetDestNode.node  = msgPktHdr.destNode;
         globVars.packetDestNode.point = 0;
      }
   }

   node4d(&globVars.packetSrcNode);
   node4d(&globVars.packetDestNode);

   if (globVars.packetDestAka == MAX_AKAS)
   {
      if (config.mailOptions.checkPktDest && !secure)
      {
         close(pktHandle);
         addExtension(pktName, ".wrong_destination");
         logEntryf(LOG_ALWAYS, 0, "Packet is addressed to another node (%s) --> packet file: %s is renamed with extension: '.wrong_destination'", nodeStr(&globVars.packetDestNode), pktName);

         return 2;  // Destination address not found
      }
      logEntryf(LOG_ALWAYS, 0, "Packet is addressed to another node (%s)!", nodeStr(&globVars.packetDestNode));
      globVars.packetDestAka = 0;
   }

   nodeInfoPtr = getNodeInfo(&globVars.packetSrcNode);

   globVars.password = 0;
   if (*msgPktHdr.password)
      globVars.password++;

   if (!nodeInfoPtr->options.ignorePwd && !secure)
   {
      strncpy(password, msgPktHdr.password, 8);
      password[8] = 0;
      strupr(password);
      if (*nodeInfoPtr->packetPwd != 0)
      {
         if (strcmp(nodeInfoPtr->packetPwd, password) != 0)
         {
            close(pktHandle);
            logEntryf(LOG_ALWAYS, 0, "Received password \"%s\" from node %s, expected \"%s\"", password, nodeStr(&globVars.packetSrcNode), nodeInfoPtr->packetPwd);
            addExtension(pktName, ".wrong_password");
            logEntryf(LOG_ALWAYS, 0, "Packet password security violation --> packet file: %s is renamed with extension: '.wrong_password'", pktName);

            return 3;  // Password security violation
         }
         else
            globVars.password++;
      }
      else
      {
        if (*password)
          logEntryf(LOG_UNEXPPWD, 0, "Unexpected packet password \"%s\" from node %s", password, nodeStr(&globVars.packetSrcNode));
      }
   }

  {
    struct stat statbuf = { 0 };
    struct tm *tm;
    if (  0 == fstat(pktHandle, &statbuf)
       && (tm = localtime(&statbuf.st_mtime)) != NULL  // localtime OK: pkt file time is logged correctly
       )
    {
      globVars.hour  = tm->tm_hour;
      globVars.min   = tm->tm_min;
      globVars.sec   = tm->tm_sec;
      globVars.day   = tm->tm_mday;
      globVars.month = tm->tm_mon + 1;
      globVars.year  = tm->tm_year + 1900;

      globVars.packetSize = statbuf.st_size;
    }
    else
    {
      globVars.hour  =
      globVars.min   =
      globVars.sec   = 0;
      globVars.day   =
      globVars.month = 1;
      globVars.year  = 1970;

      globVars.packetSize = 0;
    }
  }
  nodeInfoPtr->lastMsgRcvdDat = startTime;

  return 0;
}
//---------------------------------------------------------------------------
s16 bscanstart(void)
{
  u16 offset;

  do
  {
    if (endBuf - startBuf < 2)
    {
      offset = 0;
      if (endBuf - startBuf == 1)
      {
        pktRdBuf[0] = pktRdBuf[startBuf];
        offset = 1;
      }
      startBuf = 0;
      oldStart = 0;
      if ((endBuf = read(pktHandle, pktRdBuf + offset, PKT_BUFSIZE - offset) + offset) < 2)
        return EOF;
    }
  }
  while (*(u16*)(pktRdBuf + startBuf++) != 2);

  startBuf++;

  return 0;
}
//---------------------------------------------------------------------------
s16 bgetw(u16 *w)
{
  u16 offset;

  if (endBuf - startBuf < 2)
  {
    offset = 0;
    if (endBuf - startBuf == 1)
    {
      pktRdBuf[0] = pktRdBuf[startBuf];
      offset = 1;
    }
    startBuf = 0;
    oldStart = 0;
    if ((endBuf = read(pktHandle, pktRdBuf + offset, PKT_BUFSIZE - offset) + offset) < 2)
      return EOF;
  }
  *w = *(u16*)(pktRdBuf + startBuf);
  startBuf += 2;

  return 0;
}
//---------------------------------------------------------------------------
static s16 bgets(char *s, size_t n) // !MSGSIZE
{
  size_t sLen = 0;
  size_t m;
  char *helpPtr;

  while ((helpPtr = (char*)memccpy(s + sLen, pktRdBuf + startBuf, 0, m = min(n - sLen, (size_t)endBuf - (size_t)startBuf))) == NULL)
  {
    sLen += m;
    if (sLen == n)
    {
      if (n)
        s[n - 1] = 0;
      else
        *s = 0;

      return EOF;
    }
    startBuf = 0;
    oldStart = 0;
    endBuf   = read(pktHandle, pktRdBuf, PKT_BUFSIZE);
    if (endBuf == 0)
    {
      /*--- put extra zero at end of PKT file */
      pktRdBuf[0] = 0;
      ++endBuf;
    }
  }
  startBuf += helpPtr - s - sLen;

  return 0;
}
//---------------------------------------------------------------------------
int getPktDate(char *dateStr, u16 *year, u16 *month, u16 *day, u16 *hours, u16 *minutes, u16 *seconds)
{
  if (strlen(dateStr) < 15)
    return -1;

  *seconds = 0;

  if (!(isdigit(dateStr[0]) || isdigit(dateStr[1]) || isdigit(dateStr[2])))
    dateStr += 4;  // Skip day-of-week

  if (sscanf(dateStr, "%hd-%hd-%hd %hd:%hd:%hd", day, month, year, hours, minutes, seconds) < 5)
  {
    char monthStr[23];

    if (sscanf(dateStr, "%hd %s %hd %hd:%hd:%hd", day, monthStr, year, hours, minutes, seconds) < 5)
    {
      logEntryf(LOG_ALWAYS, 0, "Error in pkt date: %s", dateStr);

      *day     =    1;
      *month   =    1;
      *year    = 1980;
      *hours   =    0;
      *minutes =    0;
      *seconds =    0;

      return 1;
    }
    else
    {
      char *ts = strstr(upcaseMonths, strupr(monthStr));
      if (NULL != ts)
        *month = ((ts - upcaseMonths) / 3) + 1;
      else
        *month = 1;
    }
  }

  if (*year < 1980)
  {
    if (*year >= 200)
      *year = 1980;
    else
    {
      if (*year >= 100)
        *year %= 100;
      if (*year >= 80)
        *year += 1900;
      else
        *year += 2000;
    }
  }

  if (*month == 0 || *month > 12)
    *month = 1;

  if (*day == 0 || *day > 31)
    *day = 1;

  if (*hours >= 24)
    *hours = 0;

  if (*minutes >= 60)
    *minutes = 0;

  if (*seconds >= 60)
    *seconds = 0;

  return 0;
}
//---------------------------------------------------------------------------
s16 bgetdate( char *dateStr
            , u16 *year,  u16 *month,   u16 *day
            , u16 *hours, u16 *minutes, u16 *seconds
            )
{
  if (bgets(dateStr, 23) || getPktDate(dateStr, year, month, day, hours, minutes, seconds) < 0)
    return EOF;

  if (endBuf - startBuf < 1)
  {
    startBuf = 0;
    oldStart = 0;
    endBuf = read(pktHandle, pktRdBuf, PKT_BUFSIZE);
  }
  if (  (strlen(dateStr) < 19)
     && (endBuf - startBuf > 0)
     && (((*(pktRdBuf + startBuf) > 0) && (*(pktRdBuf + startBuf) < 32))
     || (*(pktRdBuf + startBuf) == (char)255))
     )
    startBuf++;

  return 0;
}
//---------------------------------------------------------------------------
s16 readPkt(internalMsgType *message)
{
  u16 check = 0;

  *message->okDateStr = 0;
  *message->tinySeen  = 0;
  *message->normSeen  = 0;
  *message->tinyPath  = 0;
  *message->normPath  = 0;
  memset(&message->srcNode, 0, 2 * sizeof(nodeNumType) + PKT_INFO_SIZE);

  do
  {
    if (check++)
    {
      startBuf = oldStart;
      if (check == 2)
      {
        newLine();
        logEntry("Skipping garbage in PKT file...", LOG_ALWAYS, 0);
      }
    }
    if (bscanstart())
      return EOF;
    oldStart = startBuf;
  }
  while (  bgetw(&message->srcNode.node )
        || bgetw(&message->destNode.node)
        || bgetw(&message->srcNode.net  )
        || bgetw(&message->destNode.net )
        || bgetw(&message->attribute    )
        || bgetw(&message->cost         )
        || bgetdate( message->dateStr
                   , &(message->year   ), &(message->month  )
                   , &(message->day    ), &(message->hours  )
                   , &(message->minutes), &(message->seconds)
                   )
        || bgets(message->toUserName  , 36)
        || bgets(message->fromUserName, 36)
        || bgets(message->subject     , 72)
        );

  bgets(message->text, TEXT_SIZE - 0x0800);

  return 0;
}
//---------------------------------------------------------------------------
void closePktRd(void)
{
  close(pktHandle);
}
//---------------------------------------------------------------------------
s16 openPktWr(nodeFileRecType *nfInfoRec)
{
   pktHdrType   msgPktHdr;
   struct tm   *tm;
   time_t       ti;
   fhandle      pktHandle;

   nfInfoRec->bytesValid = 0;

   sprintf (nfInfoRec->pktFileName, "%s%08x.tmp", config.outPath, uniqueID());

   if ((pktHandle = _sopen(nfInfoRec->pktFileName, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, SH_DENYRW, S_IREAD | S_IWRITE)) == -1)
   {
      nfInfoRec->pktHandle    = 0;
      *nfInfoRec->pktFileName = 0;
      return -1;
   }
   nfInfoRec->pktHandle = pktHandle;

   memset(&msgPktHdr, 0, sizeof (pktHdrType));

   time(&ti);
   tm = localtime(&ti);  // localtime -> ok, used for current time in pkt header

   msgPktHdr.year          = tm->tm_year + 1900;
   msgPktHdr.month         = tm->tm_mon;
   msgPktHdr.day           = tm->tm_mday;
   msgPktHdr.hour          = tm->tm_hour;
   msgPktHdr.minute        = tm->tm_min;
   msgPktHdr.second        = tm->tm_sec;
   msgPktHdr.baud          = 0;
   msgPktHdr.packetType    = 2;
   msgPktHdr.origZoneQ     = nfInfoRec->srcNode.zone;
   msgPktHdr.destZoneQ     = nfInfoRec->destNode.zone;
   msgPktHdr.origZone      = nfInfoRec->srcNode.zone;
   msgPktHdr.destZone      = nfInfoRec->destNode.zone;
   msgPktHdr.origNet       = nfInfoRec->srcNode.net;
   msgPktHdr.destNet       = nfInfoRec->destNode.net;
   msgPktHdr.origNode      = nfInfoRec->srcNode.node;
   msgPktHdr.destNode      = nfInfoRec->destNode.node;
   msgPktHdr.origPoint     = nfInfoRec->srcNode.point;
   msgPktHdr.destPoint     = nfInfoRec->destNode.point;
   msgPktHdr.prodCodeLo    = PRODUCTCODE_LOW;
   msgPktHdr.prodCodeHi    = PRODUCTCODE_HIGH;
   msgPktHdr.serialNoMinor = SERIAL_MINOR;
   msgPktHdr.serialNoMajor = SERIAL_MAJOR;

   strncpy(msgPktHdr.password, nfInfoRec->nodePtr->packetPwd, 8);

   if (nfInfoRec->nodePtr->capability & PKT_TYPE_2PLUS)
   {
      msgPktHdr.capWord       = CAPABILITY;
      msgPktHdr.cwValid       = (CAPABILITY >> 8) | (((char)CAPABILITY) << 8);
   }

   if (write(pktHandle, &msgPktHdr, 58) != 58)
   {
      close(pktHandle);
      unlink(nfInfoRec->pktFileName);
      nfInfoRec->pktHandle    = 0;
      *nfInfoRec->pktFileName = 0;
      return -1;
   }
   nfInfoRec->nodePtr->lastMsgSentDat = startTime;
   return 0;
}
//---------------------------------------------------------------------------
void RemoveNetKludge(char *text, const char *kludge)
{
  char *helpPtr;

  if ((helpPtr = findCLStr(text, kludge)) != NULL)
  {
    tempStrType tempStr = "Warning: Found netmail kludge in echomail: ";
    char *p = helpPtr + 1
       , *pt = tempStr + strlen(tempStr);
    int l = 0;

    while (l++ < 60 && *p != '\r' && *p != 0)
      *pt++ = *p++;

    *pt = 0;

    if (config.mailOptions.removeNetKludges)
    {
      strcpy(pt, " -> Removed it.");
      removeLine(helpPtr);
    }
    logEntry(tempStr, LOG_INBOUND | LOG_OUTBOUND, 0);
  }
}
//---------------------------------------------------------------------------
char *setSeenByPath(internalMsgType *msg, char *txtEnd, areaOptionsType areaOptions, nodeOptionsType nodeOptions)
{
#ifdef _DEBUG0
  logEntryf(LOG_DEBUG, 0, "DEBUG setSeenByPath areaOptions.tinySeenBy:%u nodeOptions.tinySeenBy:%u areaOptions.tinyPath:%u", areaOptions.tinySeenBy, nodeOptions.tinySeenBy, areaOptions.tinyPath);
#endif

  if (NULL == txtEnd)
    txtEnd = strchr(msg->text, 0);

  txtEnd = stpcpy(txtEnd, areaOptions.tinySeenBy || nodeOptions.tinySeenBy
                                                 ? msg->tinySeen : msg->normSeen);
  txtEnd = stpcpy(txtEnd, areaOptions.tinyPath   ? msg->tinyPath : msg->normPath);

  return txtEnd;
}
//---------------------------------------------------------------------------
s16 writeEchoPkt(internalMsgType *message, areaOptionsType areaOptions, echoToNodeType echoToNode)
{
  s16        count;
  fhandle    pktHandle;
  fnRecType *fnPtr;
  char       dateStr[24];
  char      *ftsPtr;
  char      *helpPtr;
#if 0
#define ADDPATH
  char      *helpPtr2;
#endif
  char      *pktBufStart;
  char      *psbStart;
  int        pktBufLen;

  RemoveNetKludge(message->text, "\1INTL");
  RemoveNetKludge(message->text, "\1FMPT");
  RemoveNetKludge(message->text, "\1TOPT");

  strcpy(ftsPtr = message->pktInfo + PKT_INFO_SIZE - 1 - strlen(message->subject), message->subject);
  strcpy(ftsPtr -= strlen(message->fromUserName) + 1, message->fromUserName);
  strcpy(ftsPtr -= strlen(message->toUserName  ) + 1, message->toUserName  );

  psbStart = strchr(message->text, 0);

  // Remove multiple trailing cr/lfs

  while (*(psbStart - 1) == '\r' || *(psbStart - 1) == '\n')
    psbStart--;

  *(psbStart++) = '\r';

  for (count = 0; count < forwNodeCount; count++)
  {
    if (  ETN_READACCESS(echoToNode[ETN_INDEX(count)], count)
       && (  nodeFileInfo[count]->nodePtr->options.active
          || (  !nodeFileInfo[count]->nodePtr->options.nosysopmail
             && stricmp(nodeFileInfo[count]->nodePtr->sysopName, message->toUserName) == 0
             )
          )
       )
    {
      if (nodeFileInfo[count]->pktHandle == 0)
      {
        if (*nodeFileInfo[count]->pktFileName == 0)
           openPktWr(nodeFileInfo[count]);
        else
        {
          if ((nodeFileInfo[count]->pktHandle = _sopen(nodeFileInfo[count]->pktFileName, O_WRONLY | O_BINARY, SH_DENYRW)) != -1)
            lseek(nodeFileInfo[count]->pktHandle, 0, SEEK_END);
          else
            nodeFileInfo[count]->pktHandle = 0;
        }

        if (nodeFileInfo[count]->pktHandle == 0)
           return 1;
      }

      pktHandle = nodeFileInfo[count]->pktHandle;

      if (nodeFileInfo[count]->nodePtr->options.fixDate || *message->dateStr == 0)
      {
        sprintf( dateStr, "%02d %.3s %02d  %02d:%02d:%02d"
               , message->day, months + (message->month - 1) * 3
               , message->year % 100, message->hours
               , message->minutes   , message->seconds
               );
        strcpy(helpPtr = ftsPtr - strlen(dateStr) - 1, dateStr);
      }
      else
        strcpy(helpPtr = ftsPtr - strlen(message->dateStr) - 1, message->dateStr);

#ifdef ADDPATH
      helpPtr2 =
#endif
      setSeenByPath(message, psbStart, areaOptions, nodeFileInfo[count]->nodePtr->options);

#ifdef ADDPATH
#ifdef _DEBUG0
      logEntryf(LOG_DEBUG, 0, "DEBUG srcAka: %s reqAka: %s", getAkaStr(nodeFileInfo[count]->srcAka, 0), getAkaStr(nodeFileInfo[count]->requestedAka, 0));
#endif // _DEBUG

      if (  config.akaList[nodeFileInfo[count]->srcAka].nodeNum.zone
         && nodeFileInfo[count]->srcAka != nodeFileInfo[count]->requestedAka
         )
      {
        // Add an extra PATH line if the "Use AKA" for the destination node (srcAka)
        // isn't the same as the "Origin AKA" for the echomail area the message is received in (requestedAka).
        // 2017-01-03 Don't think this is necessary, so turned off the code.
        sprintf( helpPtr2 , "\1PATH: %u/%u\r"
               , config.akaList[nodeFileInfo[count]->srcAka].nodeNum.net
               , config.akaList[nodeFileInfo[count]->srcAka].nodeNum.node
               );
#ifdef _DEBUG
        logEntryf(LOG_DEBUG, 0, "DEBUG added extra PATH: %u/%u", config.akaList[nodeFileInfo[count]->srcAka].nodeNum.net
                                                               , config.akaList[nodeFileInfo[count]->srcAka].nodeNum.node);
#endif // _DEBUG
      }
#endif // ADDPATH

      pktBufStart = helpPtr - sizeof(pmHdrType);
      pktBufLen   = (int)(strchr(message->text, 0) - pktBufStart + 1);

      ((pmHdrType*)pktBufStart)->two       = 2;
      ((pmHdrType*)pktBufStart)->srcNode   = nodeFileInfo[count]->srcNode.node;
      ((pmHdrType*)pktBufStart)->destNode  = nodeFileInfo[count]->destNode.node;
      ((pmHdrType*)pktBufStart)->srcNet    = nodeFileInfo[count]->srcNode.net;
      ((pmHdrType*)pktBufStart)->destNet   = nodeFileInfo[count]->destNode.net;

      ((pmHdrType*)pktBufStart)->attribute = message->attribute;
      ((pmHdrType*)pktBufStart)->cost      = message->cost;

      if (write(pktHandle, pktBufStart, pktBufLen) != pktBufLen )
      {
        puts("Cannot write to PKT file.");
        return 1;
      }

      nodeFileInfo[count]->totalMsgs++;

      if (config.maxPktSize != 0 && fileLength(pktHandle) >= config.maxPktSize * (s32)1000)
      {
        if (write(pktHandle, &zero, 2) != 2)
        {
          puts("Cannot write to PKT file.");
          close(pktHandle);
          return (1);
        }
        close(pktHandle);
        nodeFileInfo[count]->pktHandle = 0;

        if ((fnPtr = (fnRecType*)malloc(sizeof(fnRecType))) == NULL)
          return 1;

        memset(fnPtr, 0, sizeof(fnRecType));

        strcpy(fnPtr->fileName, nodeFileInfo[count]->pktFileName);

        fnPtr->nextRec = nodeFileInfo[count]->fnList;
        nodeFileInfo[count]->fnList = fnPtr;

        *nodeFileInfo[count]->pktFileName = 0;
        nodeFileInfo[count]->bytesValid   = 0;
      }
    }
  }
  *psbStart = 0;

  return 0;
}
//---------------------------------------------------------------------------
void freePktHandles(void)
{
   u16 count;

   for (count = 0; count < forwNodeCount; count++)
   {
      if ((*nodeFileInfo[count]->pktFileName != 0) &&
          (nodeFileInfo[count]->pktHandle != 0))
      {
         close(nodeFileInfo[count]->pktHandle);
         nodeFileInfo[count]->pktHandle = 0;
      }
   }
}
//---------------------------------------------------------------------------
s16 validateEchoPktWr(void)
{
   u16           count;
   fnRecType    *fnPtr;
   tempStrType   tempStr;
   fhandle       tempHandle;
   long          tempLen[MAX_OUTPKT];
   u32           totalPktSize = 0;

   for (count = 0; count < forwNodeCount; count++)
   {
      tempLen[count] = -1;
      if ( *nodeFileInfo[count]->pktFileName != 0
         && nodeFileInfo[count]->pktHandle   != 0
         )
      {
         if (  (tempLen[count] = fileLength(nodeFileInfo[count]->pktHandle)) == 0
            || close(nodeFileInfo[count]->pktHandle) == -1
            )
         {
            logEntry("ERROR: Cannot determine length of file", LOG_ALWAYS, 0);
            return 1;
         }
         nodeFileInfo[count]->pktHandle = 0;
      }
   }

   for (count = 0; count < forwNodeCount; count++)
   {
      if (*nodeFileInfo[count]->pktFileName != 0)
      {
               if (tempLen[count] == -1)
         {
            if (((tempHandle = open(nodeFileInfo[count]->pktFileName, O_RDONLY | O_BINARY)) == -1) ||
                ((tempLen[count] = fileLength(tempHandle)) == 0) ||
                (close(tempHandle) == -1))
            {
               logEntry("ERROR: Cannot determine length of file", LOG_ALWAYS, 0);
               return 1;
            }
         }
         nodeFileInfo[count]->bytesValid = tempLen[count];
         totalPktSize += tempLen[count];
      }
   }

   for (count = 0; count < forwNodeCount; count++)
   {
      fnPtr = nodeFileInfo[count]->fnList;
      while (fnPtr != NULL)
      {
         strcpy(tempStr, fnPtr->fileName);
         strcpy(fnPtr->fileName + strlen(tempStr) - 3, "QQQ");

         rename(tempStr, fnPtr->fileName);
         fnPtr->valid = 1;
         fnPtr = (fnRecType*)fnPtr->nextRec;
      }
   }

   if (*config.outPath == *config.inPath) // Same drive ?
   {
      if (totalPktSize > diskFree(config.outPath))
      {
         puts("\nFreeing up diskspace...");

         return closeEchoPktWr();
      }
   }

   return 0;
}
//---------------------------------------------------------------------------
s16 closeEchoPktWr(void)
{
   u16         count;
   fhandle     pktHandle;
   fnRecType   *fnPtr, *fnPtr2;

   for (count = 0; count < forwNodeCount; count++)
   {
      if (*nodeFileInfo[count]->pktFileName != 0)
      {
         if (nodeFileInfo[count]->pktHandle != 0)
         {
            if (close(nodeFileInfo[count]->pktHandle) == -1)
            {
               logEntry("ERROR: Cannot close file", LOG_ALWAYS, 0);
               return 1;
            }
            nodeFileInfo[count]->pktHandle = 0;
         }
         if (nodeFileInfo[count]->bytesValid == 0)
         {
            unlink (nodeFileInfo[count]->pktFileName);
            *nodeFileInfo[count]->pktFileName = 0;
         }
      }
   }

   for (count = 0; count < forwNodeCount; count++)
   {
      if (*nodeFileInfo[count]->pktFileName != 0)
      {
         if (((pktHandle = _sopen(nodeFileInfo[count]->pktFileName, O_WRONLY | O_BINARY, SH_DENYRW)) == -1) ||
             (lseek(pktHandle, 0, SEEK_SET) == -1) ||
             (ftruncate(pktHandle, nodeFileInfo[count]->bytesValid) == -1) ||
             (lseek(pktHandle, 0, SEEK_END) == -1) ||
             (write(pktHandle, &zero, 2) != 2) ||
             (close(pktHandle) == -1))
         {
            logEntry("ERROR: Cannot adjust length of file", LOG_ALWAYS, 0);
            return 1;
         }
      }
   }

   for (count = 0; count < forwNodeCount; count++)
   {
      // pack 'oude' bundles
      while ((fnPtr = nodeFileInfo[count]->fnList) != NULL)
      {
         // zoek laatste = eerst gemaakte
         fnPtr2 = NULL;
         while (fnPtr->nextRec != NULL)
         {
            fnPtr2 = fnPtr;
            fnPtr = (fnRecType*)fnPtr->nextRec;
         }
         if (fnPtr2 == NULL)
            nodeFileInfo[count]->fnList = NULL;
         else
            fnPtr2->nextRec = NULL;

         if (fnPtr->valid)
         {
            packArc(fnPtr->fileName, &nodeFileInfo[count]->srcNode, &nodeFileInfo[count]->destNode, nodeFileInfo[count]->nodePtr);
            newLine();
         }
         else
            unlink(fnPtr->fileName);

         free(fnPtr);
      }

      // pack 'nieuwste' bundle
      if (*nodeFileInfo[count]->pktFileName != 0)
      {
         if (nodeFileInfo[count]->bytesValid)
         {
            packArc(nodeFileInfo[count]->pktFileName, &nodeFileInfo[count]->srcNode, &nodeFileInfo[count]->destNode, nodeFileInfo[count]->nodePtr);
            newLine();
         }
         else
            unlink(nodeFileInfo[count]->pktFileName);

         *nodeFileInfo[count]->pktFileName = 0;
      }
   }
   return 0;
}
//---------------------------------------------------------------------------
s16 writeNetPktValid(internalMsgType *message, nodeFileRecType *nfInfo)
{
   u16         len;
   fhandle     pktHandle;
   s32         tempLen;
   s16         error = 0;
   fnRecType  *fnPtr;

   if (nfInfo->pktHandle == 0)
   {
      if (*nfInfo->pktFileName == 0)
      {
         if (openPktWr (nfInfo) == -1)
         {
            logEntry("Cannot create new netmail PKT file", LOG_ALWAYS, 0);
            return 1;
         }
      }
      else
      {
         if ((nfInfo->pktHandle = _sopen(nfInfo->pktFileName, O_WRONLY | O_BINARY, SH_DENYRW)) == -1)
         {
            nfInfo->pktHandle = 0;
            logEntry("Cannot open netmail PKT file", LOG_ALWAYS, 0);
            return 1;
         }
         lseek(nfInfo->pktHandle, 0, SEEK_END);
      }
   }

   pktHandle = nfInfo->pktHandle;

   pmHdr.srcNode   = message->srcNode.node;
   pmHdr.destNode  = message->destNode.node;
   pmHdr.srcNet    = message->srcNode.net;
   pmHdr.destNet   = message->destNode.net;

   pmHdr.attribute = message->attribute;
   pmHdr.cost      = message->cost;

   error |= (write(pktHandle, &pmHdr, sizeof(pmHdrType)) != sizeof(pmHdrType));

   if ((nfInfo->nodePtr->options.fixDate) || (*message->dateStr == 0))
   {
      if (*message->okDateStr == 0)
         sprintf(message->okDateStr, "%02d %.3s %02d  %02d:%02d:%02d",
                           message->day, months + (message->month - 1) * 3,
                           message->year % 100, message->hours,
                           message->minutes,  message->seconds);
      len = strlen(message->okDateStr) + 1;
      error |= (write(pktHandle, message->okDateStr, len) != len);
   }
   else
   {
      len = strlen(message->dateStr) + 1;
      error |= (write(pktHandle, message->dateStr, len) != len);
   }

   len = strlen(message->toUserName) + 1;
   error |= (write(pktHandle, message->toUserName, len) != len);

   len = strlen(message->fromUserName) + 1;
   error |= (write(pktHandle, message->fromUserName, len) != len);

   len = strlen(message->subject) + 1;
   error |= (write(pktHandle, message->subject, len) != len);

   len = strlen(message->text) + 1;
   error |= (write(pktHandle, message->text, len) != len);

   if ((tempLen = fileLength(pktHandle)) == 0)
   {
      close(pktHandle);
      logEntry("ERROR: Cannot determine length of file", LOG_ALWAYS, 0);
      nfInfo->pktHandle = 0;

      return 1;
   }

   if (config.maxPktSize != 0 && tempLen >= config.maxPktSize * (s32)1000)
   {
      if (write(pktHandle, &zero, 2) != 2)
      {
         puts("Cannot write to PKT file.");
         close(pktHandle);
         nfInfo->pktHandle = 0;

         return 1;
      }
      close(pktHandle);
      nfInfo->pktHandle = 0;
      if ((fnPtr = (fnRecType*)malloc(sizeof(fnRecType))) == NULL)
        return 1;
      memset(fnPtr, 0, sizeof(fnRecType));
      strcpy(fnPtr->fileName, nfInfo->pktFileName);
      fnPtr->nextRec = nfInfo->fnList;
      nfInfo->fnList = fnPtr;
      *nfInfo->pktFileName = 0;
      nfInfo->bytesValid   = 0;
   }
   else
      error |= close(pktHandle);

   nfInfo->pktHandle = 0;

   if (error)
   {
      logEntry("ERROR: Cannot write to PKT file", LOG_ALWAYS, 0);

      return 1;
   }
   nfInfo->bytesValid = tempLen;

   return 0;
}
//---------------------------------------------------------------------------
s16 closeNetPktWr(nodeFileRecType *nfInfo)
{
   fhandle    pktHandle;
   fnRecType *fnPtr
           , *fnPtr2;

   if (*nfInfo->pktFileName != 0)
   {
      if (nfInfo->bytesValid == 0)
      {
         unlink(nfInfo->pktFileName);
         *nfInfo->pktFileName = 0;
         return (0);
      }

      if (((pktHandle = _sopen(nfInfo->pktFileName, O_WRONLY | O_BINARY, SH_DENYRW)) == -1) ||
            (lseek(pktHandle, 0, SEEK_SET) == -1) ||
            (ftruncate(pktHandle, nfInfo->bytesValid) == -1) ||
            (lseek(pktHandle, 0, SEEK_END) == -1) ||
            (write(pktHandle, &zero, 2) != 2) ||
            (close(pktHandle) == -1))
      {
         logEntry("ERROR: Cannot adjust length of file", LOG_ALWAYS, 0);
         return 1;
      }

      // pack oude bundle
      while ((fnPtr = nfInfo->fnList) != NULL)
      {
         // zoek laatste = eerst gemaakte
         fnPtr2 = NULL;
         while(fnPtr->nextRec != NULL)
         {
            fnPtr2 = fnPtr;
            fnPtr = (fnRecType*)fnPtr->nextRec;
         }
         if (fnPtr2 == NULL)
            nfInfo->fnList = NULL;
         else
            fnPtr2->nextRec = NULL;

         packArc(fnPtr->fileName, &nfInfo->srcNode, &nfInfo->destNode, nfInfo->nodePtr);
         newLine();
         free(fnPtr);
      }

      packArc(nfInfo->pktFileName, &nfInfo->srcNode, &nfInfo->destNode, nfInfo->nodePtr);
      newLine();
      *nfInfo->pktFileName = 0;
   }

   return 0;
}
//---------------------------------------------------------------------------
