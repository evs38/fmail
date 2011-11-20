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

#ifdef __OS2__
#define INCL_DOSPROCESS
#include <os2.h>

extern APIRET16 APIENTRY16 WinSetTitle(PSZ16);

#endif

#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>
#include <string.h>
#include <dir.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <conio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#include "fmail.h"
#include "config.h"
#include "areainfo.h"
#include "nodeinfo.h"
#include "msgra.h"
#include "pack.h"
#include "msgpkt.h"
#include "msgmsg.h"
#include "dups.h"
#include "archive.h"
#include "utils.h"
#include "sorthb.h"
#include "areafix.h"
#include "log.h"
#include "output.h"
#include "ftscprod.h"
#include "jamfun.h"
#include "jammaint.h"
#include "cfgfile.h"
#include "bclfun.h"
#include "pp_date.h"
#include "version.h"

#if defined __WIN32__ && !defined __DPMI32__
#include "sendsmtp.h"
#endif

#if 0
#undef close(a)
#undef _close(a)

int closet(int handle)
{
  char tempStr[80];
  if ( handle >= 5 )
    return close(handle);
  sprintf(tempStr, "Trying to close handle %i, ignored", handle);
  logEntry(tempStr, LOG_ALWAYS, 0);
  return -1;
}

int _closet(int handle)
{
  char tempStr[80];
  if ( handle >= 5 )
    return _rtl_close(handle);
  sprintf(tempStr, "Trying to _close handle %i, ignored", handle);
  logEntry(tempStr, LOG_ALWAYS, 0);
  return -1;
}

#define close(a)  closet(a)
#define _close(a) _closet(a)
#endif


#ifndef __FMAILX__
extern unsigned cdecl _stklen = 16384;
#endif

#if 0
JAMAPIREC JamApiRec;

int JamTestNew(JAMAPIRECptr JamApiRecPtr)
{
  if (((JamApiRecPtr->Hdr.Attribute & (MSG_LOCAL|MSG_TYPEECHO)) == (MSG_LOCAL|MSG_TYPEECHO)) &&
      ((JamApiRecPtr->Hdr.Attribute & (MSG_DELETED|MSG_SENT)) == 0))
  {
    return (ScanMsgHdrStop);
  }
  return (ScanMsgHdrNextHdr);
}
#endif

fhandle fmailLockHandle;

extern u16 dayOfWeek;

u16 status;

badEchoType  badEchos[MAX_BAD_ECHOS];
u16     badEchoCount = 0;
#ifdef __FMAILX__
char programPath[128];
#endif
char configPath[128];
s16  _mb_upderr = 0;

#ifndef __32BIT__
#ifdef __BORLANDC__
u16      cdecl _openfd[TABLE_SIZE] = {0x6001, 0x6002, 0x6002, 0xa004,
                                      0xa002, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000, 0x0000,
                                      0x0000, 0x0000, 0x0000
                                     };
#else
u16      cdecl _openfd[TABLE_SIZE] = {0x2001, 0x2002, 0x2002, 0xa004,
                                      0xa002, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff, 0xffff,
                                      0xffff, 0xffff, 0xffff
                                     };
#endif
#endif

#if defined __WIN32__ && !defined __DPMI32__
char *smtpID;
#endif

globVarsType globVars;

extern configType config;

const char *dayName[7]   = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const char *months       = "JanFebMarAprMayJunJulAugSepOctNovDec";
const char *semaphore[6] = { "fdrescan.now",
                             "ierescan.now",
                             "dbridge.rsn",
                             "",
                             "mdrescan.now",
                             "xmrescan.flg"
                           };

extern time_t    startTime;
extern struct tm timeBlock;

extern u16      echoCount;
extern u16      forwNodeCount;

extern cookedEchoType    *echoAreaList;
extern echoToNodePtrType echoToNode[MAX_AREAS];
extern nodeFileType      nodeFileInfo;

extern s16 messagesMoved;

internalMsgType *message;
s16             diskError    = 0;

extern s16 breakPressed;
s16        mailBomb = 0;


typedef struct bt
{
  char         *name;
  struct ffblk blk;
  struct bt    *nextb;
};


//s16 jamout ( JAMAPIREC * pJam, internalMsgType *message, s16 areaType);


u16 foundToUserName(u8 *toUserName)
{
  return *toUserName &&
         (stricmp(toUserName, config.users[0].userName) == 0 ||
          stricmp(toUserName, config.users[1].userName) == 0 ||
          stricmp(toUserName, config.users[2].userName) == 0 ||
          stricmp(toUserName, config.users[3].userName) == 0 ||
          stricmp(toUserName, config.users[4].userName) == 0 ||
          stricmp(toUserName, config.users[5].userName) == 0 ||
          stricmp(toUserName, config.users[6].userName) == 0 ||
          stricmp(toUserName, config.users[7].userName) == 0);
}


u16 getAreaCode (char *msgText)
{
  areaNameType echoName;
  char         *helpPtr;
  u16          low, mid, high;
  tempStrType  tempStr;

  if ((strncmp (msgText, "AREA:", 5) != 0) &&
      (strncmp (msgText, "\1AREA:", 6) != 0))
  {
    return (NETMSG);
  }

  if (*msgText == 1)
  {
    strcpy (msgText, msgText+1);
  }

  helpPtr = msgText+5;
  while (*helpPtr == ' ')
    helpPtr++;

  strncpy (echoName, helpPtr, sizeof(areaNameType)-1);
  echoName[sizeof(areaNameType)-1] = 0;
  if ((helpPtr = strchr (echoName, '\r')) != NULL)
    *helpPtr = 0;
  if ((helpPtr = strchr (echoName, ' ')) != NULL)
    *helpPtr = 0;

  if (*echoName == 0)
  {
    printString ("Message has no valid area tag ");
  }
  else
  {
    low = 0;
    high = echoCount;
    while (low < high)
    {
      mid = (low + high) >> 1;
      if (stricmp (echoName, echoAreaList[mid].areaName) > 0)
        low = mid+1;
      else
        high = mid;
    }

    if ((low >= echoCount) ||
        (stricmp (echoName, echoAreaList[low].areaName) != 0))
    {
      low = 0;
      while ((low < echoCount) &&
             (stricmp (echoName, echoAreaList[low].oldAreaName) != 0))
      {
        low++;
      }
      if (low < echoCount)
      {
        removeLine (msgText);
        sprintf (tempStr, "AREA:%s\r",
                 strcpy(echoName,echoAreaList[low].areaName));
        insertLine (msgText, tempStr);
      }
    }
    if ((low < echoCount) &&
        (stricmp (echoName, echoAreaList[low].areaName) == 0))
    {
      if ( echoAreaList[low].options.active && !echoAreaList[low].options.local )
      {
        return (low);
      }
      if (echoAreaList[low].options.local)
        printString ("Area is local: ");
      else
        printString ("Area is not active: ");
      printString (echoName);
      printChar (' ');
    }
    else
    {
      printString ("Unknown area: ");
      printString (echoName);
      printChar (' ');

      if (badEchoCount < MAX_BAD_ECHOS)
      {
        low = 0;
        while ((low < badEchoCount) &&
               stricmp (badEchos[low].badEchoName, echoName))
        {
          low++;
        }
        if (low == badEchoCount)
        {
          strcpy (badEchos[badEchoCount].badEchoName, echoName);
          badEchos[badEchoCount].srcNode   = globVars.packetSrcNode;
          badEchos[badEchoCount++].destAka = globVars.packetDestAka;
        }
      }
    }
  }
  return (BADMSG);
}



//extern nodeInfoType *nodeInfo;
//extern u16          nodeCount;


static s16 processPkt (u16 secure, s16 noAreaFix)
{

  tempStrType    pktStr,
  tempStr;
  char           *helpPtr;
  echoToNodeType tempEchoToNode;
  s16            count;
  s16            headerStat;
  s16            donePkt;
  s16            echoToNodeCount;
  u16            pktMsgCount;
  s16            areaIndex;
  s16            localAkaNum;
  struct ffblk   ffblkPkt;
  s16            areaFixRep;
  char           prodCodeStr[20];


  if (secure && (*config.securePath == 0))
  {
    secure = 0;
  }
  secure++;

  do
  {
    strcpy (pktStr, (secure==2)?config.securePath:secure?config.inPath:".\\"); /* was pktPath */
    strcat (pktStr, "*.pkt");
    donePkt = findfirst (pktStr, &ffblkPkt, 0);

    if (!donePkt) /* do not remove ! */
    {
      while ((!donePkt) && (!diskError) && (!breakPressed))
      {
        if ((ffblkPkt.ff_attrib & FA_RDONLY) == 0)
        {
          newLine();

          strcpy (pktStr, (secure==2)?config.securePath:secure?config.inPath:".\\"); /* was pktPath */
          strcat (pktStr, ffblkPkt.ff_name);
          headerStat = openPktRd (pktStr, secure==2);

          helpPtr = "";
          count = 0;
          while ( ftscprod[count].prodcode != 0xFFFF )
          {
            if (ftscprod[count].prodcode == globVars.remoteProdCode )
            {
              helpPtr = ftscprod[count].prodname;
              break;
            }
            ++count;
          }
          if (*helpPtr == 0)
          {
            sprintf (prodCodeStr, "Program id %04hX", globVars.remoteProdCode);
            helpPtr = prodCodeStr;
          }
          sprintf (tempStr, "Processing %s from %s to %s",
                   ffblkPkt.ff_name,
                   nodeStr (&globVars.packetSrcNode),
                   nodeStr (&globVars.packetDestNode));
          logEntry (tempStr, LOG_PKTINFO, 0);

          if (headerStat == 0)
          {
            if ((globVars.remoteCapability&1) == 1)
              sprintf (tempStr, "Pkt info: %s %u.%02u, Type 2+, %uk, %u-%.3s-%u %u:%02u%s%s",
                       helpPtr, globVars.versionHi, globVars.versionLo,
                       globVars.packetSize, globVars.day,
                       months+(globVars.month-1)*3, globVars.year,
                       globVars.hour, globVars.min, /* globVars.sec, */
                       globVars.password?", Pwd":"", (globVars.password==2)?", Sec":"");
            else
              sprintf (tempStr, "Pkt info: %s, Type %s, %uk, %u-%.3s-%u %u:%02u%s%s",
                       helpPtr,
                       globVars.remoteCapability == 0xffff ? "2.2" :"2.0",
                       globVars.packetSize, globVars.day,
                       months+(globVars.month-1)*3, globVars.year,
                       globVars.hour, globVars.min, /* globVars.sec, */
                       globVars.password?", Pwd":"", (globVars.password==2)?", Sec":"");
            logEntry (tempStr, LOG_XPKTINFO, 0);

            pktMsgCount = 0;

            while ((!readPkt (message)) && (!diskError) && (!breakPressed))
            {
              gotoTab(0);
              printString("Pkt message ");
              printInt(++pktMsgCount);
              printString(" "dARROW" ");

              areaIndex = getAreaCode(message->text);

              /* Personal Mail Check */

              if (areaIndex == NETMSG) /* for getLocalAka, moved here from case NETMSG statement */
              {
                make4d (message);
                localAkaNum = getLocalAka(&message->destNode);
              }

              if ( *config.topic1 || *config.topic2 )
              {
                strcpy (tempStr, message->subject);
                strupr (tempStr);
              }
              if ((*config.pmailPath &&
                   (areaIndex != NETMSG ||
                    (config.mailOptions.persNetmail &&
                     localAkaNum >= 0))) &&
                  (stricmp(message->toUserName, config.sysopName) == 0 ||
                   (*config.topic1 &&
                    strstr(tempStr, config.topic1) != NULL) ||
                   (*config.topic2 &&
                    strstr(tempStr, config.topic2) != NULL) ||
                   foundToUserName(message->toUserName))
                 )
              {
                if (areaIndex == NETMSG)
                {
                  sprintf (tempStr, "AREA: Netmail  SOURCE: %s\r",
                           nodeStr(&message->srcNode));
                  insertLine (message->text, tempStr);
                }
                if (areaIndex == BADMSG)
                  insertLine (message->text, "AREA: Bad Messages\r");

                writeMsg (message, PERMSG, 0);
                if ((areaIndex == NETMSG) || (areaIndex == BADMSG))
                  removeLine (message->text);
              }

              switch (areaIndex)
              {
                case NETMSG :
                  printString ("NETMAIL");

                  message->attribute &= PRIVATE     | FILE_ATT |
                                        RET_REC_REQ |
                                        IS_RET_REC  | AUDIT_REQ;

                  areaFixRep = 0;

                  if ((localAkaNum >= 0) &&
                      (!noAreaFix) &&
                      (toAreaFix(message->toUserName)))
                  {
                    if ( config.mgrOptions.keepRequest ||
                         findCLiStr(message->text, "%NOTE") != NULL )
                    {
                      message->attribute |= RECEIVED;
                      writeMsg (message, NETMSG, 1);
                    }
                    printChar (' ');
                    areaFixRep = !areaFix(message);
                  }
                  if (!areaFixRep)
                  {
                    /* re-route to point */
                    if (localAkaNum >= 0)
                    {
                      count = 0;
                      while (count < nodeCount)
                      {
                        if ((nodeInfo[count]->options.routeToPoint) &&
                            (isLocalPoint(&(nodeInfo[count]->node))) &&
                            (memcmp(&(nodeInfo[count]->node), &message->destNode, 6) == 0) &&
                            (stricmp(nodeInfo[count]->sysopName, message->toUserName) == 0))
                        {
                          sprintf(tempStr,"\1TOPT %u\r", message->destNode.point = nodeInfo[count]->node.point);
                          insertLine(message->text, tempStr);
                          count = nodeCount;
                          localAkaNum = getLocalAka(&message->destNode);
                        }
                        else
                          count++;
                      }
                    }
                    if (localAkaNum >= 0)
                    {
                      printFill ();
                      if ((stricmp (message->toUserName, "SysOp") == 0) ||
                          (stricmp (message->toUserName, config.sysopName) == 0))
                      {
                        strcpy (message->toUserName, config.sysopName);
                        globVars.perNetCount++;
                      }
                    }
                    else
                    {
                      if ( secure == 2 && getLocalAka(&message->srcNode) >= 0 )
                      {
                        printStringFill (" (local secure)");
                        message->attribute |= LOCAL | KILLSENT;
                      }
                      else
                      {
                        printStringFill (" (in transit)");
                        message->attribute |= IN_TRANSIT | KILLSENT;
                      }
                      if ((!(getNodeInfo (&message->destNode)->capability & PKT_TYPE_2PLUS)) &&
                          (isLocalPoint (&message->destNode)))
                      {
                        make2d (message);
                      }
                    }
                    if ((message->attribute & FILE_ATT) &&
                        (message->attribute & IN_TRANSIT) &&
                        ((strchr(message->subject, '*') != 0) ||
                         (strchr(message->subject, '?') != 0)))
                    {
                      insertLine (message->text, "\1FLAGS LOK\r");
                      writeMsg (message, NETMSG, 2);
                    }
                    else
                    {
                      writeMsg (message, NETMSG, 0);
                    }
                  }
                  break;
                case BADMSG :
                printStringFill (dARROW" Bad message");

tossbad:
                  sprintf (tempStr, "\r\1FMAIL DEST: %s", nodeStr(&globVars.packetDestNode));
                  if ((helpPtr = strchr (message->text, '\r')) != NULL)
                  {
                    insertLine (helpPtr, tempStr);
                  }
                  sprintf (tempStr, "\r\1FMAIL SRC: %s", nodeStr(&globVars.packetSrcNode));
                  if ((helpPtr = strchr (message->text, '\r')) != NULL)
                  {
                    insertLine (helpPtr, tempStr);
                  }
                  if ( writeBBS (message, config.badBoard, 1) )
                    diskError = DERR_WRHBAD;
                  globVars.badCount++;
                  break;
                default     :
                  printString (echoAreaList[areaIndex].areaName);

                  /* Security Check */

                  if (secure != 2)
                  {
                    count = 0;
                    while ((count < forwNodeCount) &&
                           ((!ETN_WRITEACCESS(echoToNode[areaIndex][ETN_INDEX(count)], count)) ||
                            (memcmp (&(nodeFileInfo[count]->destNode4d.net),
                                     &(globVars.packetSrcNode.net),
                                     sizeof(nodeNumType)-2) != 0)))
                    {
                      count++;
                    }

                    if ((count == forwNodeCount &&
                         echoAreaList[areaIndex].options.security) ||
                        (count < forwNodeCount &&
                         echoAreaList[areaIndex].writeLevel >
                         nodeFileInfo[count]->nodePtr->writeLevel))
                    {
                      if (count == forwNodeCount)
                        sprintf(tempStr, "\1FMAIL BAD: %s is not connected to this area\r",
                                nodeStr(&globVars.packetSrcNode));
                      else
                        sprintf(tempStr, "\1FMAIL BAD: %s has no write access to area\r",
                                nodeStr(&nodeFileInfo[count]->destNode4d));
                      insertLineN(message->text, tempStr, 1);

                      for (count = 0; count < forwNodeCount; count++)
                      {
                        if ( /*nodeFileInfo[count]->srcAka == globVars.packetDestAka &&*/
                          memcmp(&nodeFileInfo[count]->destNode4d.net,
                                 &globVars.packetSrcNode.net, 6) == 0 )
                        {
                          nodeFileInfo[count]->fromNodeSec++;
                          count = -1;
                          break;
                        }
                      }
                      if ( count != -1 )
                        ++globVars.fromNoExpSec;
                      printStringFill (" Security violation "dARROW" Bad message");
                      goto tossbad;
                    }
                  }

                  if (checkDup (message,
                                echoAreaList[areaIndex].areaNameCRC))
                  {
                    for (count = 0; count < forwNodeCount; count++)
                    {
                      if ( /*nodeFileInfo[count]->srcAka == globVars.packetDestAka) &&*/
                        memcmp(&nodeFileInfo[count]->destNode4d.net,
                               &globVars.packetSrcNode.net, 6) == 0 )
                      {
                        nodeFileInfo[count]->fromNodeDup++;
                        count = -1;
                        break;
                      }
                    }
                    if ( count != -1 )
                      ++globVars.fromNoExpDup;
                    printStringFill (" "dARROW" Duplicate message");
                    if ( writeBBS (message, config.dupBoard, 1) )
                      diskError = DERR_WRHDUP;
                    echoAreaList[areaIndex].dupCount++;
                    globVars.dupCount++;
                    break;
                  }
                  printFill ();

                  echoAreaList[areaIndex].msgCount++;
                  globVars.echoCount++;

                  if (echoAreaList[areaIndex].options.allowPrivate)
                    message->attribute &= PRIVATE;
                  else
                    message->attribute = 0;

                  memcpy (tempEchoToNode,
                          echoToNode[areaIndex],
                          sizeof(echoToNodeType));
                  echoToNodeCount = echoAreaList[areaIndex].echoToNodeCount;

                  for (count = 0; count < forwNodeCount; count++)
                  {
                    if (memcmp (&nodeFileInfo[count]->destNode4d.net,
                                &globVars.packetSrcNode.net, 6) == 0)
                    {
                      if ( ETN_ANYACCESS(tempEchoToNode[ETN_INDEX(count)], count) )
                      {
                        tempEchoToNode[ETN_INDEX(count)] &= ETN_RESET(count);
                        echoToNodeCount--;
                      }
                    }
                  }
                  for (count = 0; count < forwNodeCount; count++)
                  {
                    if ( /*nodeFileInfo[count]->srcAka == globVars.packetDestAka &&*/
                      memcmp(&nodeFileInfo[count]->destNode4d.net,
                             &globVars.packetSrcNode.net, 6) == 0 )
                    {
                      nodeFileInfo[count]->fromNodeMsg++;
                      count = -1;
                      break;
                    }
                  }
                  if ( count != -1 )
                    ++globVars.fromNoExpMsg;

                  if (echoToNodeCount)
                  {
                    addPathSeenBy (message->text,
                                   message->normSeen,
                                   message->tinySeen,
                                   message->normPath,
                                   tempEchoToNode,
                                   areaIndex);

                    if ( writeEchoPkt (message,
                                       echoAreaList[areaIndex].options.tinySeenBy,
                                       tempEchoToNode) )
                      diskError = DERR_WRPKTE;
                  }
                  else
                  {
                    helpPtr = message->text;
                    while ((helpPtr = findCLStr (helpPtr, "SEEN-BY: ")) != NULL)
                    {
                      if ( echoAreaList[areaIndex].options.impSeenBy )
                      {
                        if ( echoAreaList[areaIndex].JAMdirPtr == NULL )
                        {
                          memmove (helpPtr+1, helpPtr, strlen(helpPtr)+1);
                          *helpPtr = 1;
                        }
                        helpPtr += 8;
                      }
                      else
                        removeLine (helpPtr);
                    }
                  }

                  if (echoAreaList[areaIndex].JAMdirPtr == NULL)
                  {
                    /* Hudson toss */
                    if (writeBBS (message,
                                  echoAreaList[areaIndex].board,
                                  echoAreaList[areaIndex].options.impSeenBy))
                      diskError = DERR_WRHECHO;
                  }
                  else
                  {
                    /* JAM toss */
                    if (echoAreaList[areaIndex].options.impSeenBy)
                    {
                      strcat(message->text, message->normSeen);
                    }
                    strcat(message->text, message->normPath);

                    if (echoAreaList[areaIndex].JAMdirPtr != NULL)
                    {
                      /* Skip AREA tag */

                      if (strncmp (message->text, "AREA:", 5) == 0)
                      {
                        if (*(helpPtr = strchr (message->text, '\r') + 1) == '\n')
                          helpPtr++;
                        memmove(message->text,helpPtr,strlen(helpPtr)+1);
                      }

                      if (config.mbOptions.removeRe)
                      {
                        strcpy (message->subject, removeRe(message->subject));
                      }

                      /* write here */
                      if ( !jam_writemsg(echoAreaList[areaIndex].JAMdirPtr, message, 0) )
                      {
                        newLine();
                        logEntry ("Can't write JAM message", LOG_ALWAYS, 0);
                        diskError = DERR_WRJECHO;
                        break;
                      }
                      globVars.jamCountV++;
                    }
                  }
                  break;
              }
              if ((config.allowedNumNetmail != 0) &&
                  (globVars.netCount >= config.allowedNumNetmail))
              {
                mailBomb++;
                breakPressed++;
              }
            }
            closePktRd ();

            freePktHandles ();

            if (mailBomb)
            {
              strcpy (tempStr, pktStr);
              strcpy (strchr(tempStr, '.'), ".mlb");
              rename (pktStr, tempStr);
              unlink (pktStr);
              newLine ();
              sprintf (tempStr, "Max # net msgs per packet exceeded in packet from %s", nodeStr(&globVars.packetSrcNode));
              logEntry (tempStr, LOG_ALWAYS, 0);
              sprintf (tempStr, "Packet %s has been renamed to .mlb", pktStr);
              logEntry (tempStr, LOG_ALWAYS, 0);
            }
            else
            {
              if ((!diskError) && (!breakPressed) &&
                  ((validate1BBS () || validateEchoPktWr ()) == 0))
              {
                validate2BBS(0);
                validateMsg();
                validateDups();
                unlink(pktStr);
                for (count = 0; count < echoCount; count++)
                {
                  echoAreaList[count].msgCountV = echoAreaList[count].msgCount;
                  echoAreaList[count].dupCountV = echoAreaList[count].dupCount;
                }
              }
              else if ( !diskError && !breakPressed )
                diskError = DERR_VAL;
              newLine ();
            }
          }
          else if (headerStat == 1)
            printString ("Error opening packet\n");
          else if (headerStat == 2)
            logEntry ("Packet is addressed to another node ï¿½ï¿½ packet is renamed to .DST",
                      LOG_ALWAYS, 0);
          else if (headerStat == 3)
            logEntry ("Packet password security violation ï¿½ï¿½ packet is renamed to .SEC",
                      LOG_ALWAYS, 0);
        }
        donePkt = findnext(&ffblkPkt);
      }
      if (!mailBomb) newLine ();
#if 0
      if (diskError)
      {
        if (diskError == xxx)
          logEntry ("JAM-related error", LOG_ALWAYS, 0);
        else
          logEntry ("Disk-related error (probably disk full)", LOG_ALWAYS, 0);
      }
#endif
    }
  }
  while ((!diskError) && (!breakPressed) && (secure--));
  jam_closeall();
  return diskError;
}

s16 handleScan(internalMsgType *message, u16 boardNum, u16 boardIndex)
{
  u16            count;
  echoToNodeType tempEchoToNode;
  tempStrType    tempStr;
  char          *helpPtr1,
                *helpPtr2;
  char           tearline[80] = "--- \r";

  switch (config.tearType)
  {
    case 1:
      strcpy(stpcpy(tearline + 4, config.tearLine), "\r");
      break;
    case 2:
    case 3:
      break;
    case 4:
    case 5:
      *tearline = 0;
      break;
    case 6:
    default:
      strcpy(tearline, TearlineStr());
      break;
  }

  if ((boardNum && isNetmailBoard(boardNum)) ||
      (0 /* If JAM netmail area */ ))
  {
    message->attribute |= LOCAL |
                          (config.mailOptions.keepExpNetmail?0:KILLSENT) |
                          ((getFlags(message->text)&FL_FILE_REQ)?FILE_REQ:0);

    point4d (message);

    /* MSGID kludge */

    if (findCLStr (message->text, "\1MSGID: ") == NULL)
    {
      sprintf (tempStr, "\1MSGID: %s %08lx\r",
               nodeStr (&message->srcNode), uniqueID());
      insertLine (message->text, tempStr);
    }

    if (message->srcNode.zone != message->destNode.zone)
    {
      addINTL (message);
    }

    if (writeMsg (message, NETMSG, 0) == -1)
    {
      return (1);
    }

    sprintf (tempStr, "Netmail message for %s moved to netmail directory",
             nodeStr (&message->destNode));
    logEntry (tempStr, LOG_NETEXP, 0);

    if (updateCurrHdrBBS (message))
      return (1);

    validateMsg ();
    globVars.nmbCountV++;
    return (0);
  }

  if (boardNum)
  {  /* Hudson based messages */

    for (count = 0; (count < echoCount) &&
         (echoAreaList[count].board != boardNum);
         count++) {}

    if (count == echoCount)
    {
      sprintf (tempStr, "Found message in unknown message base board (#%u)", boardNum);
      logEntry (tempStr, LOG_ALWAYS, 0);

      if (updateCurrHdrBBS (message))
        return (1);

      return (0);
    }
  }
  else
  { // JAM based message
    count = boardIndex;
  }

  if ( echoAreaList[count].options.active && !echoAreaList[count].options.local )
  {
    sprintf (tempStr, "Echomail message, area %s", echoAreaList[count].areaName);
    logEntry (tempStr, LOG_ECHOEXP, 0);

    if (echoAreaList[count].options.allowPrivate)
      message->attribute &= PRIVATE;
    else
      message->attribute = 0;

    memcpy (tempEchoToNode, echoToNode[count], sizeof(echoToNodeType));

    // Check origin

    if ((helpPtr1 = findCLStr(message->text, " * Origin:")) != NULL)
    {
      // Retear message with FMail tearline
      if (config.mbOptions.reTear)
      {
        helpPtr2 = helpPtr1;
        while ((helpPtr1 > message->text)
              && ((*--helpPtr1 == '\r') || (*helpPtr1 == '\n'))
              )
          ;
        while ((helpPtr1 > message->text)
              && (*(helpPtr1-1) != '\r') && (*(helpPtr1-1) != '\n')
              )
          helpPtr1--;
        if ((strncmp(helpPtr1, "---", 3) == 0) && (helpPtr1[3] != '-'))
        {
          removeLine(helpPtr1);
          insertLine(helpPtr1, tearline);
        }
        else
          insertLine(helpPtr2, tearline);
      }
    }
    else
    {
      sprintf( strchr(message->text, 0), "%s * Origin: %s (%s)\r"
             , tearline
             , echoAreaList[count].originLine
             , nodeStr(&config.akaList[echoAreaList[count].address].nodeNum)
             );
    }
    if (config.tearType)
    {
      if ((helpPtr1 = findCLStr(message->text, "\1TID:")) != NULL)
        removeLine(helpPtr1);

      sprintf(tempStr, "\1TID: %s\r", TIDStr());
      insertLine(message->text, tempStr);
    }

    sprintf(tempStr, "AREA:%s\r", echoAreaList[count].areaName);
    insertLine(message->text, tempStr);

    addPathSeenBy(message->text,
                  message->normSeen,
                  message->tinySeen,
                  message->normPath,
                  tempEchoToNode, count);

    if (writeEchoPkt (message,
                      echoAreaList[count].options.tinySeenBy,
                      tempEchoToNode))
    {
      return (1);
    }

    if ((boardNum && updateCurrHdrBBS (message)) ||
        (validateEchoPktWr ()))
    {
      return (1);
    }
    if (*config.sentEchoPath)
      writeMsg(message, SECHOMSG, 1);

    if (*config.topic1 || *config.topic2)
    {
      strcpy (tempStr, message->subject);
      strupr (tempStr);
    }
    if ( config.mbOptions.persSent &&
         *config.pmailPath &&
         (stricmp(message->fromUserName, config.sysopName) == 0 ||
          (*config.topic1 &&
           strstr(tempStr, config.topic1) != NULL) ||
          (*config.topic2 &&
           strstr(tempStr, config.topic2) != NULL) ||
          foundToUserName(message->fromUserName))
       )
      writeMsg (message, PERMSG, 1);

    checkDup (message, echoAreaList[count].areaNameCRC);
    validateDups ();

    echoAreaList[count].msgCountV++;
    globVars.echoCountV++;
  }
  return (0);
}
//----------------------------------------------------------------------------
int cdecl main(int argc, char *argv[])
{
  s16            count;
  s16            doneArc
               , dayNum;
  s16            boardNum;
  s16            temp;
  s16            diskErrorT;
  s16            infoBad;
  u16            scanCount = 0;
#ifndef GOLDBASE
  u16            index;
#else
  u32            index;
#endif
  tempStrType    tempStr;
  char          *helpPtr;
  fhandle        tempHandle;
  s32            switches;
  s16            doneMsg;
  s32            msgNum;
  struct ffblk   ffblkMsg;
  struct bt     *bundlePtr
              , *bundlePtr2
              , *bundlePtr3
              ;

  putenv("TZ=LOC0");
  tzset();
#ifdef __OS2__
  WinSetTitle(VERSION_STRING);
#endif
#if defined __WIN32__ && !defined __DPMI32__
  smtpID = TIDStr();
#endif

  initOutput();

#ifndef __32BIT__
  if ((_osmajor < 3) || ((_osmajor == 3) && (_osminor < 30)))
  {
    printString("FMail requires at least DOS 3.30\n");
    exit (4);
  }
#endif
  cls();

#ifndef STDO
  setAttr(YELLOW, RED, MONO_NORM);
  printString("ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n");
  printString("³                                                                             ³\n");
  printString("³                                                                             ³\n");
  printString("ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
  setAttr (YELLOW, RED, MONO_HIGH);
  gotoPos (3, 1);
#endif
  printString(VersionStr());
  printString(" - The Fast Echomail Processor");
#ifndef STDO
  gotoPos(3, 2);
#else
  newLine();
  newLine();
#endif
  sprintf(tempStr, "Copyright (C) 1991-%s by FMail Developers - All rights reserved", __DATE__ + 7);
  printString(tempStr);
#ifndef STDO
  gotoPos(0, 5);
  setAttr(LIGHTGRAY, BLACK, MONO_NORM);
#else
  newLine();
  newLine();
#endif

  memset(&globVars, 0, sizeof(globVarsType));

#ifdef __FMAILX__
  strcpy(programPath, argv[0]);
  *(strrchr(programPath, '\\') + 1) = 0;
#endif

  if (((helpPtr = getenv("FMAIL")) == NULL) || (*helpPtr == 0))
  {
    strcpy(configPath, argv[0]);
    *(strrchr(configPath, '\\') + 1) = 0;
  }
  else
  {
    strcpy(configPath, helpPtr);
    if (configPath[strlen(configPath)-1] != '\\')
    {
      strcat(configPath, "\\");
    }
  }

  if ((argc >= 2) &&
      ((stricmp(argv[1], "A") == 0) || (stricmp(argv[1], "ABOUT") == 0)))
  {
    char *str = "About FMail:\n"
                "\n"
                "    Version          : %s\n"
                "    Operating system : "
#ifdef __OS2__
                 "OS/2\n"
#elif defined __WIN32__ && !defined __DPMI32__
                 "Win32\n"
#elif defined __FMAILX__
                 "DOS DPMI\n"
#else
                 "DOS standard\n"
#endif
                 "    Processor        : "
#ifdef __PENTIUMPRO__
                 "PentiumPro\n"
#elif __PENTIUM__
                 "Pentium\n"
#elif defined __486__
                 "i486 and up\n"
#elif defined P386
                 "80386 and up\n"
#else
                 "8088/8086 and up\n"
#endif
                 "    Compiled on      : %d-%02d-%02d\n"
                 "    Message bases    : JAM and "MBNAME"\n"
                 "    Max. areas       : %u\n"
                 "    Max. nodes       : %u\n";
    char tStr[1024];
    sprintf(tStr, str, VersionStr(), YEAR, MONTH + 1, DAY, MAX_AREAS, MAX_NODES);
    printString(tStr);
    showCursor();

    return 0;
  }
  else if ((argc >= 2) &&
           ((stricmp(argv[1], "T") == 0) || (stricmp(argv[1], "TOSS") == 0)))
  {
    if ((argc >= 3) && ((argv[2][0] == '?') || (argv[2][1] == '?')))
    {
      printString("Usage:\n\n"
                  "    FMail Toss [/A] [/B]\n\n"
                  "Switches:\n\n"
                  "    /A   Do not process AreaMgr requests\n"
                  "    /B   Also scan bad message directory for valid echomail messages\n");
      showCursor();
      return 0;
    }

    status = 2;

    switches = getSwitch (&argc, argv, SW_A | SW_B);

    initFMail("TOSS", switches);

    initPkt();
    initNodeInfo();
    initAreaInfo();

    initMsg((u16)(switches & SW_A));  /* After initNodeInfo & initAreaInfo */
    retryArc(); /* After initMsg */
    scan_bcl();
    _mb_upderr = multiUpdate ();

    initBBS();

    openDup();

    strcpy (tempStr, configPath);
    strcat (tempStr, "fmail.bde");
    ++no_msg;
    if ((tempHandle = openP(tempStr, O_RDONLY|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) != -1)
    {
      badEchoCount = (read (tempHandle, badEchos,
                            MAX_BAD_ECHOS*sizeof(badEchoType)+1) /
                      sizeof(badEchoType));
      close(tempHandle);
    }

    if (switches & SW_B)
    {
      printString ("Tossing messages from bad message board...\n\n");
      openBBSWr(1); // was: openBBSRd();
      moveBadBBS();
      closeBBSWr(1);
      validateDups();
    }

    openBBSWr(0);

    if ((!diskError) && (!breakPressed))
    {
      printString ("Tossing messages...\n");

      diskError = processPkt(1, (u16)(switches & SW_A));

      dayNum = 0;
      bundlePtr = NULL;
      while ((dayNum < 8) && (!diskError) && (!breakPressed))
      {
        sprintf (tempStr, "%s*.%.2s?", config.inPath, dayName[dayNum++]);

        doneArc = findfirst (tempStr, &ffblkMsg, 0);

        while ((!doneArc) && (!diskError) && (!breakPressed))
        {
          if ((ffblkMsg.ff_attrib & FA_RDONLY) == 0)
          {
            bundlePtr3 = bundlePtr;
            bundlePtr2 = NULL;
            while (bundlePtr3 != NULL &&
                   (bundlePtr3->blk.ff_fdate < ffblkMsg.ff_fdate ||
                    (bundlePtr3->blk.ff_fdate == ffblkMsg.ff_fdate &&
                     bundlePtr3->blk.ff_fdate < ffblkMsg.ff_ftime)))
            {
              bundlePtr2 = bundlePtr3;
              bundlePtr3 = bundlePtr3->nextb;
            }
            if ((bundlePtr3 = malloc(sizeof(struct bt))) == NULL ||
                (bundlePtr3->name = malloc(strlen(ffblkMsg.ff_name)+1)) == NULL)
            {
              logEntry("Not enough memory available", LOG_ALWAYS, 2);
            }
            strcpy(bundlePtr3->name, ffblkMsg.ff_name);
            bundlePtr3->blk = ffblkMsg;
            if (bundlePtr2 == NULL)
            {
              bundlePtr3->nextb = bundlePtr;
              bundlePtr = bundlePtr3;
            }
            else
            {
              bundlePtr3->nextb = bundlePtr2->nextb;
              bundlePtr2->nextb = bundlePtr3;
            }
          }
          doneArc = findnext (&ffblkMsg);
        }
      }
      if ( bundlePtr == NULL )
        newLine();
      else
      {
        do
        {
          newLine ();
          strcpy(tempStr, config.inPath);
          strcat(tempStr, bundlePtr->name);
          unpackArc(tempStr, &bundlePtr->blk);
          scan_bcl();
          diskError = processPkt(0, (u16)(switches & SW_A));
          bundlePtr2 = bundlePtr;
          bundlePtr = bundlePtr->nextb;
          free(bundlePtr2->name);
          free(bundlePtr2);
        }
        while (bundlePtr != NULL);
      }
      if ( closeEchoPktWr() )
        diskError = DERR_CLPKTE;
    }

    if (badEchoCount)
    {
      strcpy (tempStr, configPath);
      strcat (tempStr, "fmail.bde");
      if ((tempHandle = openP(tempStr, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY|O_DENYNONE,
                              S_IREAD|S_IWRITE)) != -1)
      {
        write (tempHandle, badEchos, badEchoCount*sizeof(badEchoType));
        close(tempHandle);
      }
    }

    if (config.mailOptions.warnNewMail && globVars.perCountV)
    {
      strcpy (message->toUserName,   config.sysopName);
      strcpy (message->fromUserName, "FMail");
      strcpy (message->subject,      "New messages");
      message->srcNode   = message->destNode = config.akaList[0].nodeNum;
      message->attribute = PRIVATE;

      sprintf(tempStr, "New personal netmail and/or echomail messages have arrived on your system!\r\r%s", TearlineStr());
      strcpy(message->text, tempStr);
      writeMsgLocal(message, NETMSG, 1);
    }

    closeBBSWr(0);
#ifdef __WINDOWS32__
    sendmail_smtp();
#endif
    closeNodeInfo ();
    closeDup();
    deInitPkt();
    free (message);

    if ( globVars.echoCountV || globVars.dupCountV || globVars.badCountV )
    {
      if ( *config.tossedAreasList &&
           (tempHandle = openP(config.tossedAreasList,
                               O_WRONLY|O_CREAT|O_APPEND|O_TEXT,
                               S_IREAD|S_IWRITE)) != -1)
      {
        for (count = 0; count < echoCount; count++)
        {
          if ( echoAreaList[count].msgCountV )
          {
            write (tempHandle, tempStr,
                   sprintf (tempStr, "%s\n",
                            echoAreaList[count].areaName));
          }
        }
      }
      if ( *config.summaryLogName &&
           (tempHandle = openP(config.summaryLogName,
                               O_WRONLY|O_CREAT|O_APPEND|O_TEXT,
                               S_IREAD|S_IWRITE)) != -1)
      {
        write (tempHandle, tempStr,
               sprintf (tempStr, "\n----------  %s %2u %.3s %2.2u %u:%02u, %s - Toss Summary\n\n\n",
                        dayName[timeBlock.tm_wday],
                        timeBlock.tm_mday,
                        months+(timeBlock.tm_mon*3),
                        timeBlock.tm_year%100,
                        timeBlock.tm_hour,
                        timeBlock.tm_min,
                        VersionStr()));
        write (tempHandle, tempStr,
               sprintf (tempStr, "Board  Area name                                           #Msgs  Dupes\n"));
        write (tempHandle, tempStr,
               sprintf (tempStr, "-----  --------------------------------------------------  -----  -----\n"));
        temp = 0;
        if (globVars.badCountV)
        {
          write (tempHandle, tempStr,
                 sprintf (tempStr, "%5u  Bad messages                                        %5u\n",
                          config.badBoard, globVars.badCountV));
          temp++;
        }
        for (count = 0; count < echoCount; count++)
        {
          if (echoAreaList[count].msgCountV ||
              echoAreaList[count].dupCountV)
          {
            if (echoAreaList[count].board)
            {
              write (tempHandle, tempStr,
                     sprintf (tempStr, "%5u  %-50s  %5u  %5u\n",
                              echoAreaList[count].board,
                              echoAreaList[count].areaName,
                              echoAreaList[count].msgCountV,
                              echoAreaList[count].dupCountV));
            }
            else
            {
              write (tempHandle, tempStr,
                     sprintf (tempStr, "%5s  %-50s  %5u  %5u\n",
                              (echoAreaList[count].JAMdirPtr==NULL)?"None":"JAM",
                              echoAreaList[count].areaName,
                              echoAreaList[count].msgCountV,
                              echoAreaList[count].dupCountV));
            }
            temp++;
          }
        }
        write (tempHandle, tempStr,
               sprintf (tempStr, "-----  --------------------------------------------------  -----  -----\n"));
        write (tempHandle, tempStr,
               sprintf (tempStr, "%5u                                                      %5u  %5u\n",
                        temp, globVars.echoCountV+globVars.badCountV,
                        globVars.dupCountV));

        temp = 0;
        for (count = 0; count < forwNodeCount; count++)
        {
          if (nodeFileInfo[count]->fromNodeSec ||
              nodeFileInfo[count]->fromNodeDup ||
              nodeFileInfo[count]->fromNodeMsg)
          {
            if (!temp)
            {
              write (tempHandle, tempStr,
                     sprintf (tempStr, "\n\nReceived from node        #Msgs  Dupes  Sec V\n"
                              "------------------------  -----  -----  -----\n"));
              temp++;
            }
            write (tempHandle, tempStr,
                   sprintf (tempStr, "%-24s  %5u  %5u  %5u\n",
                            nodeStr(&nodeFileInfo[count]->destNode4d),
                            nodeFileInfo[count]->fromNodeMsg,
                            nodeFileInfo[count]->fromNodeDup,
                            nodeFileInfo[count]->fromNodeSec));
          }
        }
        if ( globVars.fromNoExpMsg || globVars.fromNoExpDup ||
             globVars.fromNoExpSec )
        {
          if (!temp)
          {
            write (tempHandle, tempStr,
                   sprintf (tempStr, "\n\nReceived from node        #Msgs  Dupes  Sec V\n"
                            "------------------------  -----  -----  -----\n"));
          }
          write (tempHandle, tempStr,
                 sprintf (tempStr, "Not in export list        %5u  %5u  %5u\n",
                          globVars.fromNoExpMsg,
                          globVars.fromNoExpDup,
                          globVars.fromNoExpSec));
        }
        temp = 0;
        for (count = 0; count < forwNodeCount; count++)
        {
          if (nodeFileInfo[count]->totalMsgsV)
          {
            if (!temp)
            {
              write (tempHandle, tempStr,
                     sprintf (tempStr, "\n\nSent to node              #Msgs\n"
                              "------------------------  -----\n"));
              temp++;
            }
            write (tempHandle, tempStr,
                   sprintf (tempStr, "%-24s  %5u\n",
                            nodeStr(&nodeFileInfo[count]->destNode4d),
                            nodeFileInfo[count]->totalMsgsV));
          }
        }
        write (tempHandle, "\n\n\n", 3);
        close(tempHandle);
      }
    }

    if (config.mbOptions.updateChains && globVars.jamCountV)
    {
      headerType  *areaHeader;
      rawEchoType *areaBuf;
      u16         count2;

      if (openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf))
      {
        for (count = 0; count < areaHeader->totalRecords; count++)
        {
          getRec(CFG_ECHOAREAS, count);
          if (*areaBuf->msgBasePath)
          {
            count2 = 0;
            while (count2 < echoCount &&
                   (echoAreaList[count2].JAMdirPtr == NULL ||
                    echoAreaList[count2].msgCountV == 0 ||
                    strcmp(areaBuf->msgBasePath, echoAreaList[count2].JAMdirPtr) != 0))
            {
              count2++;
            }
            if (count2 < echoCount)
            {
              s32 space;
              if ( !JAMmaint(areaBuf, 0, config.sysopName, &space) )
              {
                areaBuf->stat.tossedTo = 0;
                putRec(CFG_ECHOAREAS, count);
              }
            }
          }
        }
        closeConfig(CFG_ECHOAREAS);
//            newLine();
      }
    }

    deInitAreaInfo ();

    /* See also end of PACK block */

    temp = 0;
    if ((config.mbOptions.sortNew || config.mbOptions.updateChains) &&
        ((globVars.mbCountV != 0) /* || (globVars.movedBad != 0) */ ) &&
        (!breakPressed))
    {
      if (config.mbOptions.mbSharing)
      {
        temp = config.mbOptions.updateChains;
        config.mbOptions.updateChains = 0;
      }
      if (config.mbOptions.updateChains || config.mbOptions.sortNew)
      {
        sortBBS (globVars.origTotalMsgBBS, 0);
        /* this was the original position of 'newLine' below */
      }
      newLine ();
    }

    if ((!(_mb_upderr = multiUpdate ())) && temp)
    {
      config.mbOptions.mbSharing = 0;
      config.mbOptions.updateChains = 1;
      config.mbOptions.sortNew = 0;
      sortBBS (globVars.origTotalMsgBBS, 1);
      newLine ();
    }
  }
  else if ((argc >= 2) &&
           ((stricmp(argv[1], "S") == 0) || (stricmp(argv[1], "SCAN") == 0)))
  {
    if ((argc >= 3) && ((argv[2][0] == '?') || (argv[2][1] == '?')))
    {
      printString ("Usage:\n\n"
                   "    FMail Scan [/A] [/E] [/N] [/S]\n\n"
                   "Switches:\n\n"
                   "    /A   Do not process AreaMgr requests\n"
                   "    /E   Scan for echomail messages only\n"
                   "    /N   Scan for netmail messages only\n"
                   "    /H   Scan "MBNAME" base only\n"
                   "    /J   Scan JAM bases only\n"
                   "    /S   Scan the entire message base (ignore EchoMail."MBEXTN"/JAM and NetMail."MBEXTN")\n");
      showCursor ();
      return (0);
    }

    status = 1;

    switches = getSwitch (&argc, argv, SW_A|SW_E|SW_N|SW_H|SW_J|SW_S);

    initFMail("SCAN", switches);

    initPkt();
    initNodeInfo();
    initAreaInfo();

    initMsg ((u16)(switches & SW_A));  /* After initNodeInfo */
    retryArc (); /* After initMsg */
    scan_bcl();
    _mb_upderr = multiUpdate ();

    initBBS ();

    openDup ();
    openBBSWr(1); // was: openBBSRd();

    if (switches & (SW_N|SW_H))
      goto skipJAM;

    for (count=0; count<echoCount; count++)
      echoAreaList[count].options._reserved = 0;

    infoBad = 0;
    count = 0;
    while ((count < echoCount) && (echoAreaList[count].JAMdirPtr == NULL ||
                                   (!echoAreaList[count].options.active) || echoAreaList[count].options.local) )
    {
      count++;
    }
    if (count < echoCount)
    {
      printString ("Scanning JAM message bases for outgoing messages...\n");
      if ((!config.mbOptions.scanAlways) && (!(switches & SW_S)))
      {
        strcpy (tempStr, config.bbsPath);
        strcat (tempStr, "echomail.jam");
        ++no_msg;
        if ((tempHandle = openP(tempStr, O_RDONLY|O_TEXT|O_DENYNONE,S_IREAD|S_IWRITE)) != -1)
        {
          memset (tempStr, 0, sizeof(tempStrType));
          if ((count = read(tempHandle, tempStr, sizeof(tempStrType)-1)) != -1)
          {
            tempStr[count] = 0;
            while (*tempStr)
            {
              if ((helpPtr = strchr(tempStr, ' ')) != NULL)
              {
                msgNum = atol(helpPtr);
                *(helpPtr++) = 0;
                count = 0;
                while ((count < echoCount) &&
                       ((echoAreaList[count].JAMdirPtr == NULL) ||
                        stricmp(tempStr, echoAreaList[count].JAMdirPtr)))
                {
                  count++;
                }
                if (count != echoCount &&
                    !echoAreaList[count].options._reserved)
                {
                  if ( (msgNum = jam_scan(count, msgNum, 1, message)) == 0 )
                  {
                    infoBad++;
                    echoAreaList[count].options._reserved = 1;
                  }
                  else
                  {
                    diskErrorT = 0;
                    if ((!echoAreaList[count].options.active) ||
                        echoAreaList[count].options.local ||
                        !(diskErrorT = handleScan(message, 0, count)) )
                    {
                      removeLine(message->text);
                      jam_update(count, msgNum, message);
                      scanCount++;
                    }
                    if ( diskErrorT != 0 )
                      diskError = DERR_PRSCN;
                  }
                }
                if ((helpPtr = strchr (helpPtr, '\n')) != NULL)
                {
                  helpPtr = stpcpy (tempStr, helpPtr+1);
                  count = (u16)(sizeof(tempStrType)+tempStr-helpPtr);
                  if ((count = read(tempHandle, helpPtr, count-1)) == -1)
                  {
                    *tempStr = 0;
                  }
                  else
                  {
                    helpPtr[count] = 0;
                  }
                }
              }
              else
                *tempStr = 0;
            }
            close (tempHandle);
          }
        }
      }

      if ((infoBad) || (config.mbOptions.scanAlways) || (switches & SW_S))
      {
        for (count=0; count<echoCount; count++)
        {
          if ((echoAreaList[count].JAMdirPtr != NULL &&
               echoAreaList[count].options.active &&
               !echoAreaList[count].options.local) &&
              (config.mbOptions.scanAlways || (switches & SW_S) ||
               echoAreaList[count].options._reserved))
          {
            if ((infoBad || config.mbOptions.scanAlways ||
                 (switches & SW_S)) && infoBad != -1)
            {
              if (config.mbOptions.scanAlways || (switches & SW_S))
                printString("Rescanning all JAM areas\n");
              else
                printString("Rescanning selected JAM areas\n");
              infoBad = -1;
            }
            sprintf(tempStr, "Scanning JAM area %s...\n", echoAreaList[count].areaName);
            printString(tempStr);
            msgNum = 0;
            while ( !diskError && (msgNum = jam_scan(count, ++msgNum, 0, message)) != 0 )
            {
              if ( !(diskErrorT = handleScan (message, 0, count)) )
              {
                removeLine(message->text);
                jam_update(count, msgNum, message);
                scanCount++;
              }
              if ( diskErrorT )
                diskError = DERR_PRSCN;
            }
          }
        }
      }
      if ((!diskError) && (!breakPressed))
      {
        strcpy (tempStr, config.bbsPath);
        strcat (tempStr, "echomail.jam");
        unlink (tempStr);
      }
      globVars.jamCountV = scanCount;
      gotoTab (0);
      if (scanCount == 0)
        printString ("No new outgoing messages\n\n");
      else
      {
        printFill();
        newLine();
        scanCount = 0;
      }
    }
skipJAM:
    if (switches & SW_J)
      goto skipHudson;

    printString ("Scanning "MBNAME" message base for outgoing messages...\n");
    infoBad = 0;

    if ((!config.mbOptions.scanAlways) && (!(switches & SW_S)))
    {
      for (count = 0; count <= 1; count++)
      {
        if (((count == 0) && (!(switches & SW_N))) ||
            ((count == 1) && (!(switches & SW_E))))
        {
          strcpy (tempStr, makeFullPath(config.bbsPath, getenv("QUICK"),
                                        !count ? "echomail."MBEXTN : "netmail."MBEXTN));

          ++no_msg;
          if ((tempHandle = openP(tempStr, O_RDONLY|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) != -1)
          {
#ifndef GOLDBASE
            while ((_read (tempHandle, &index, 2) == 2) &&
#else
            while ((_read (tempHandle, &index, 4) == 4) &&
#endif
                   (!diskError) && (!breakPressed))
            {
              if ((boardNum = scanBBS (index, message, 0)) != 0)
              {
                if (boardNum != -1)
                {
                  if (((count == 0) && (isNetmailBoard(boardNum))) ||
                      ((count == 1) && (!isNetmailBoard(boardNum))))
                  {
                    infoBad++;
                  }
                  else
                  {
                    if ( handleScan (message, boardNum, 0) )
                      diskError = DERR_PRSCN;
                    scanCount++;
                  }
                }
                else
                {
                  infoBad++;
                }
              }
              else
                infoBad++;
            }
            close(tempHandle);
            if ((!diskError) && (!breakPressed))
              unlink (tempStr);
          }
        }
      }
    }

    if ((infoBad) || (config.mbOptions.scanAlways) || (switches & SW_S))
    {
      index = 0;
      while (((boardNum = scanBBS (index++, message, 0)) != 0) &&
             (!diskError) && (!breakPressed))
      {
        if ((boardNum != -1) &&
            (((!isNetmailBoard(boardNum)) && (!(switches & SW_N))) ||
             ((isNetmailBoard(boardNum)) && (!(switches & SW_E)))))
        {
          if ( handleScan(message, boardNum, 0) )
            diskError = DERR_PRSCN;
          scanCount++;
        }
      }
      if ( scanCount )
        newLine ();
    }
    gotoTab (0);
    if (scanCount == 0)
      printString ("No new outgoing messages\n\n");
    else
    {
      printFill();
      newLine();
    }

skipHudson:

    if ( closeEchoPktWr() )
      diskError = DERR_CLPKTE;

    closeBBSWr(1); // was: closeBBSRd();
#ifdef __WINDOWS32__
    sendmail_smtp();
#endif
    closeNodeInfo ();
    deInitAreaInfo ();
    closeDup ();

    /* newLine (); */
  }
  else if ((argc >= 2) &&
           ((stricmp(argv[1], "I") == 0) || (stricmp(argv[1], "IMPORT") == 0)))
  {
    if ((argc >= 3) && ((argv[2][0] == '?') || (argv[2][1] == '?')))
    {
      printString ("Usage:\n\n"
                   "    FMail Import [/A]\n\n"
                   "Switches:\n\n"
                   "    /A   Do not process AreaMgr requests\n");
      showCursor ();
      return (0);
    }

    switches = getSwitch (&argc, argv, SW_A);

    initFMail("IMPORT", switches);

    initPkt ();
    initNodeInfo ();
    initAreaInfo (); /* for AreaFix */

    initMsg ((u16)(switches & SW_A));  /* After initNodeInfo */
    retryArc (); /* After initMsg */
    scan_bcl();
    _mb_upderr = multiUpdate ();

    initBBS ();
    openBBSWr(0);

    printString ("Importing netmail messages\n\n");

    strcpy (tempStr, config.netPath);
    strcat (tempStr, "*.msg");

    doneMsg = findfirst (tempStr, &ffblkMsg, 0);

    count = 0;
    while (!doneMsg)
    {
      msgNum = strtoul (ffblkMsg.ff_name, &helpPtr, 10);

      if ((msgNum != 0) && (*helpPtr == '.') &&
          (!(ffblkMsg.ff_attrib & FA_SPEC)) &&
          (!readMsg (message, msgNum)))
      {
        if (stricmp(message->toUserName, "SysOp") == 0)
          strcpy(message->toUserName, config.sysopName);
        if ( getLocalAkaNum (&message->destNode) != -1 &&
             strnicmp(message->toUserName,"ALLFIX",6) != 0 &&
             strnicmp(message->toUserName,"FILEFIX",7) != 0 &&
             strnicmp(message->toUserName,"FILEMGR",7) != 0 &&
             strnicmp(message->toUserName,"RAID",4) != 0 &&
             strnicmp(message->toUserName,"FFGATE",6) != 0 &&
             (config.mbOptions.sysopImport ||
              (stricmp(message->toUserName, config.sysopName) != 0 &&
               !foundToUserName(message->toUserName))) &&
             ((boardNum = getNetmailBoard(&message->destNode)) != -1) 
           )
        {
          if ((message->attribute & FILE_ATT) &&
              (strchr(message->subject,'\\') == NULL) &&
              (strlen(message->subject)+strlen(config.inPath) < 72))
          {
            strcpy (stpcpy (tempStr, config.inPath), message->subject);
            strcpy (message->subject, tempStr);
          }
          if (config.mailOptions.privateImport)
            message->attribute |= PRIVATE;

          if ( !diskError )
          {
            if ( writeBBS(message, boardNum, 1) )
              diskError = DERR_WRHECHO;
            else
            {
              strcpy (tempStr, config.netPath);
              strcat (tempStr, ffblkMsg.ff_name);
              if ((unlink (tempStr) == 0) && (!diskError) )
              {
                if ( validate1BBS() )
                  diskError = DERR_VAL;
                else
                {
                  validate2BBS(0);
                  helpPtr = tempStr +
                            sprintf (tempStr, "Importing msg #%lu from %s to ",
                                     msgNum, nodeStr(&message->srcNode));
                  sprintf (helpPtr, "%s (board #%u)",
                           nodeStr(&message->destNode), boardNum);
                  logEntry (tempStr, LOG_NETIMP, 0);
                  count++;
                  globVars.nmbCountV++;
                }
              }
            }
          }
        }
      }
      doneMsg = findnext (&ffblkMsg);
    }
    if (count)
      newLine ();

    closeBBSWr(0);
    closeNodeInfo();
    deInitPkt();
    deInitAreaInfo ();
    free (message);

    _mb_upderr = multiUpdate ();

    /* 'Copy' from TOSS */

    if (config.mbOptions.updateChains &&
        (globVars.mbCountV != 0) && (!breakPressed))
    {
      temp = config.mbOptions.mbSharing;
      config.mbOptions.mbSharing = 0;
      config.mbOptions.sortNew = 0;
      sortBBS (globVars.origTotalMsgBBS, temp);
      newLine ();
    }
  }
  else if ((argc >= 2) &&
           ((stricmp(argv[1], "P") == 0) || (stricmp(argv[1], "PACK") == 0)))
  {
    if ((argc >= 3) && ((argv[2][0] == '?') || (argv[2][1] == '?')))
    {
      printString ("Usage:\n\n"
                   "    FMail Pack [<node> [<node> ...] [VIA <node>] [/A] [/C] [/H] [/O] [/I] [/L]]\n\n"
                   "Switches:\n\n"
                   "    /A   Do not process AreaMgr requests\n\n"
                   "    /C   Include messages with Crash status\n"
                   "    /H   Include messages with Hold status\n"
                   "    /O   Include orphaned messages\n\n"
                   "    /I   Only pack messages that are in transit\n"
                   "    /L   Only pack messages that originated on your system\n");
      showCursor ();
      return (0);
    }

    switches = getSwitch (&argc, argv, SW_A|SW_C|SW_H|SW_I|SW_L|SW_O);

    initFMail("PACK", switches);

    initPkt ();
    initNodeInfo ();
    initAreaInfo (); /* for AreaFix */

    initMsg ((u16)(switches & SW_A));  /* after initNodeInfo */
    retryArc (); /* after initMsg */
    scan_bcl();
    _mb_upderr = multiUpdate ();

    pack (argc, argv, switches);

#ifdef __WINDOWS32__
    sendmail_smtp();
#endif
    closeNodeInfo ();
  }
  else if ((argc >= 2) &&
           ((stricmp(argv[1], "M") == 0) || (stricmp(argv[1], "MGR") == 0)))
  {
    if ((argc >= 3) && ((argv[2][0] == '?') || (argv[2][1] == '?')))
    {
      printString ("Usage:\n\n"
                   "    FMail Mgr\n\n"
                   "Switches:\n\n"
                   "    None\n");
      showCursor ();
      return (0);
    }

    switches = getSwitch (&argc, argv, 0);

    initFMail("MGR", switches);

    initPkt ();
    initNodeInfo ();
    initAreaInfo (); /* for AreaFix */

    initMsg (0);  /* after initNodeInfo */
    scan_bcl();

#ifdef __WINDOWS32__
    sendmail_smtp();
#endif
    closeNodeInfo ();
  }
  else
  {
    printString ("Usage:\n"
                 "\n"
                 "   FMail <command> [parameters]\n"
                 "\n"
                 "Commands:\n"
                 "\n"
                 "   About    Show some information about the program\n"
                 "   Scan     Scan the message base for outgoing messages\n"
                 "   Toss     Toss and forward incoming mailbundles\n"
                 "   Import   Import netmail messages into the message base\n"
                 "   Pack     Pack and compress outgoing netmail found in the netmail directory\n"
                 "   Mgr      Only process AreaMgr requests\n"
                 "\n"
                 "Enter 'FMail <command> ?' for more information about [parameters]\n");
    showCursor ();
    return (0);
  }

  sprintf (tempStr, "Netmail: %u,  Personal: %u,  "MBNAME": %u,  JAMbase: %u",
           globVars.netCountV, globVars.perCountV,
           globVars.mbCountV,  globVars.jamCountV);
  logEntry (tempStr, LOG_STATS, 0);
  sprintf (tempStr, "Msgbase net: %u, echo: %u, dup: %u, bad: %u",
           globVars.nmbCountV, globVars.echoCountV,
           globVars.dupCountV, globVars.badCountV);
  logEntry (tempStr, LOG_STATS, 0);


  /* Semaphore files */

  /* Create FDRESCAN / FMRESCAN / IMRESCAN / IERESCAN / MDRESCAN */

  if (((config.mailer <= 2) || (config.mailer >= 4)) &&
      (*config.semaphorePath) && (globVars.createSemaphore || messagesMoved))
  {
    strcpy (tempStr, semaphore[config.mailer]);
    touch (config.semaphorePath, tempStr, "");
    if (config.mailer <= 1)
    {
      tempStr[1] = 'M';
      touch (config.semaphorePath, tempStr, "");
    }
  }

  if (config.mailer == 2) /* D'Bridge */
  {
    if (globVars.mbCountV)
      touch (config.semaphorePath, "dbridge.emw", "Mail");

    if (globVars.perNetCountV)
      touch (config.semaphorePath, "dbridge.nmw", "");
  }

  if (diskError)
  {
    newLine ();
    switch ( diskError )
    {
      case DERR_HACKER:
        logEntry("Internal overflow control error", LOG_ALWAYS, 1);
        break;
      case DERR_VAL   :
        logEntry("File validate error", LOG_ALWAYS, 1);
        break;
      case DERR_WRPKTE:
        logEntry("Echo PKT write error", LOG_ALWAYS, 1);
        break;
      case DERR_CLPKTE:
        logEntry("Close PKT write error", LOG_ALWAYS, 1);
        break;
      case DERR_PRSCN :
        logEntry("Scan handling error", LOG_ALWAYS, 1);
        break;
      case DERR_PACK  :
        logEntry("Net message write error", LOG_ALWAYS, 1);
        break;
      case DERR_WRHECHO:
        logEntry(MBNAME" echo write error", LOG_ALWAYS, 1);
        break;
      case DERR_WRHBAD:
        logEntry(MBNAME" bad message write error", LOG_ALWAYS, 1);
        break;
      case DERR_WRHDUP:
        logEntry(MBNAME" dup message write error", LOG_ALWAYS, 1);
        break;
      case DERR_WRJECHO:
        logEntry("JAM echo write error", LOG_ALWAYS, 1);
        break;
      default:
        logEntry("Unknown disk error", LOG_ALWAYS, 1);
        break;
    }
  }
  if (breakPressed)
  {
    newLine ();
    if (mailBomb)
      logEntry ("Mail bomb protection", LOG_ALWAYS, 5);
    else
      logEntry ("Control-break pressed", LOG_ALWAYS, 3);
  }

  logActive();
  showCursor();
  deinitFMail();
  close(fmailLockHandle);

  return (_mb_upderr ? 50 : 0);
}
//----------------------------------------------------------------------------
void myexit(void)
{
#pragma exit myexit
#ifdef _DEBUG
  getch();
#endif
}
//----------------------------------------------------------------------------

