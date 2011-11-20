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
#include <string.h>
#include <dos.h>
#include <dir.h>
#include <mem.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fmail.h"
#include "config.h"
#include "window.h"
#include "uplink.h"
#include "version.h"

#define NETMSG   -1
#define PERMSG   -2
#define DUPMSG   -3
#define BADMSG   -4
#define SECHOMSG -5


static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";

typedef struct
{
  unsigned day      : 5;
  unsigned month    : 4;
  unsigned year     : 7;
  unsigned dSeconds : 5;
  unsigned minutes  : 6;
  unsigned hours    : 5;
} packedTime;


typedef struct
{
  uchar       fromUserName [36];
  uchar       toUserName [36];
  uchar       subject [72];
  uchar       dateTime [20];
  u16         timesRead;
  u16         destNode;
  u16         origNode;
  u16         cost;
  u16         origNet;
  u16         destNet;
  packedTime  wrTime;
  packedTime  recTime;
  u16         replyTo;
  u16         attribute;
  u16         nextReply;
} msgMsgType;


static internalMsgType *message;

static char *insertLine (char *pos, char *line)
{
  u16 s;
  memmove (pos+(s=strlen(line)), pos, strlen(pos)+1);
  memcpy (pos, line, s);
  return (pos+s);
}


static u32 lastID = 0;

static u32 uniqueID(void)
{
  if ( lastID == 0 )
  {
    time((time_t *)&lastID);
    lastID <<= 4;
  }
  else
    lastID++;
  return lastID;
}


static s32 writeMsg (internalMsgType *message, s16 msgType, s16 valid)
{
  /* valid = 0 : write .FML file
     valid = 1 : write .MSG file
     valid = 2 : write READ-ONLY .MSG file
  */
  fhandle     msgHandle;
  u16         len;
  tempStrType tempStr;
  char        *helpPtr;
  s32         highMsgNum;
  u16         count;
  msgMsgType  msgMsg;
  s16         doneMsg;
  struct ffblk ffblkMsg;

  memset (&msgMsg, 0, sizeof(msgMsgType));

  strcpy (msgMsg.fromUserName, message->fromUserName);
  strcpy (msgMsg.toUserName, message->toUserName);

  if ((message->attribute & FILE_ATT) &&
      (strchr(message->subject,'\\') == NULL) &&
      (strlen(message->subject)+strlen(config.inPath) < 72))
  {
    strcpy (stpcpy (msgMsg.subject, config.inPath), message->subject);
  }
  else
  {
    strcpy (msgMsg.subject, message->subject);
  }

  sprintf (msgMsg.dateTime, "%02u %.3s %02u  %02u:%02u:%02u",
           message->day,      months+(message->month-1)*3,
           message->year%100, message->hours,
           message->minutes,  message->seconds);

  msgMsg.wrTime.hours    = message->hours;
  msgMsg.wrTime.minutes  = message->minutes;
  msgMsg.wrTime.dSeconds = message->seconds >> 1;
  msgMsg.wrTime.day      = message->day;
  msgMsg.wrTime.month    = message->month;
  msgMsg.wrTime.year     = message->year-1980;

  msgMsg.origNet   = message->srcNode.net;
  msgMsg.origNode  = message->srcNode.node;
  msgMsg.destNet   = message->destNode.net;
  msgMsg.destNode  = message->destNode.node;

  msgMsg.attribute = message->attribute;

  switch (msgType)
  {
    case NETMSG:
      strcpy (tempStr, config.netPath);
      break;
    case PERMSG:
      strcpy (tempStr, config.pmailPath);
      break;
    case SECHOMSG:
      strcpy (tempStr, config.sentEchoPath);
      break;
    default      :
      return (-1);
  }
  helpPtr = strchr (tempStr, 0);

  /* Determine highest message */

  highMsgNum = 0;
  strcpy(helpPtr, "lastread");
  if (valid && ((msgHandle = open(tempStr, O_RDONLY|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) != -1))
  {
    if ( read(msgHandle, &highMsgNum, 2) != 2 )
      highMsgNum = 0;
    close(msgHandle);
  }
  strcpy (helpPtr, valid ? "*.msg" : "*."MBEXTB);
  doneMsg = findfirst(tempStr, &ffblkMsg, FA_RDONLY|FA_HIDDEN|FA_SYSTEM|
                      /*FA_LABEL|*/FA_DIREC);

  while (!doneMsg)
  {
    highMsgNum = max (highMsgNum, atoi(ffblkMsg.ff_name));
    doneMsg = findnext (&ffblkMsg);
  }

  /* Try to open file */

  count = 0;
  sprintf (helpPtr, valid?"%lu.msg":"%lu."MBEXTB, ++highMsgNum);

  while ((count < 20) &&
         ((msgHandle = open(tempStr, O_RDWR|O_CREAT|O_EXCL|
                            O_TRUNC|O_BINARY|O_DENYNONE,
                            S_IREAD|S_IWRITE)) == -1))
  {
    highMsgNum += count++ < 10 ? 1 : 10;
    sprintf (helpPtr, valid?"%lu.msg":"%lu."MBEXTB, highMsgNum);
  }

  if (msgHandle == -1)
  {
    displayMessage("Can't open output file");
    return (-1);
  }

  len = strlen(message->text)+1;

  if ((_write (msgHandle, &msgMsg, sizeof(msgMsgType)) !=
       sizeof(msgMsgType)) ||
      (_write (msgHandle, message->text, len) != len))
  {
    close(msgHandle);
    displayMessage("Can't write to output file");
    return (-1);
  }
  close(msgHandle);

  if ( valid == 2 )
    chmod(tempStr, S_IREAD);

  return (highMsgNum);
}




static s32 writeMsgLocal (internalMsgType *message, s16 msgType, s16 valid)
{
  struct date dateRec;
  struct time timeRec;
  tempStrType tempStr;

  getdate (&dateRec);
  gettime (&timeRec);

  message->hours     = timeRec.ti_hour;
  message->minutes   = timeRec.ti_min;
  message->seconds   = timeRec.ti_sec;
  message->day       = dateRec.da_day;
  message->month     = dateRec.da_mon;
  message->year      = dateRec.da_year;

  message->attribute |= LOCAL;

  /* PID kludge */

  sprintf(tempStr, "\1PID: %s\r", PIDStr());
  insertLine(message->text, tempStr);

  /* MSGID kludge */

  sprintf (tempStr, "\1MSGID: %s %08lx\r",
           nodeStr (&message->srcNode), uniqueID());
  insertLine (message->text, tempStr);

  /* FMPT kludge */

  if (message->srcNode.point != 0)
  {
    sprintf (tempStr, "\1FMPT %u\r", message->srcNode.point);
    insertLine (message->text, tempStr);
  }

  /* TOPT kludge */

  if (message->destNode.point != 0)
  {
    sprintf (tempStr, "\1TOPT %u\r", message->destNode.point);
    insertLine (message->text, tempStr);
  }

  /* INTL kludge */

  sprintf (tempStr, "\1INTL %u:%u/%u %u:%u/%u\r",
           message->destNode.zone, message->destNode.net,
           message->destNode.node, message->srcNode.zone,
           message->srcNode.net,   message->srcNode.node);
  insertLine (message->text, tempStr);

  return (writeMsg (message, msgType, valid));
}





static u16 uplinkMsgActive, uplinkMsgUsed;
static nodeNumType currentNode;

#define FS_MSGSIZE (sizeof(internalMsgType) - DEF_TEXT_SIZE + 16384)

u16 openUplinkMsg(nodeNumType *node)
{
  u16 count;
  tempStrType tempStr;

  if ( uplinkMsgActive )
    return 1;

  if ( !memcmp(node, &currentNode, sizeof(nodeNumType)) )
    return 0;

  currentNode = *node;

  if ( message == NULL )
  {
    if ( (message = malloc(FS_MSGSIZE)) == NULL )
    {
      displayMessage("Not enough memory");
      return 1;
    }
  }

  for ( count = 0; count < MAX_UPLREQ; count++ )
  {
    if ( !memcmp(node, &config.uplinkReq[count].node, sizeof(nodeNumType)) )
      break;
  }
  if ( count == MAX_UPLREQ )
    return 1;

  sprintf(tempStr, "Send uplink request to node %s?", nodeStr(node));
  if ( askBoolean(tempStr, 'Y') != 'Y' )
    return 1;

  uplinkMsgActive = count + 1;
  uplinkMsgUsed = 0;
  memset(message, 0, FS_MSGSIZE);

  message->srcNode = config.akaList[config.uplinkReq[count].originAka].nodeNum;
  message->destNode = *node;
  strcpy (message->fromUserName, config.sysopName);
  strcpy (message->toUserName,   config.uplinkReq[count].program);
  strcpy (message->subject,      config.uplinkReq[count].password);
  message->srcNode  = config.akaList[config.uplinkReq[count].originAka].nodeNum;
  message->destNode = config.uplinkReq[count].node;
  message->attribute= PRIVATE|KILLSENT;
  return 0;
}

u16 addUplinkMsg(u8 *tag, u16 remove)
{
  u8 *helpPtr;

  if ( !uplinkMsgActive )
    return 1;
  helpPtr = strchr(message->text, 0);
  sprintf(helpPtr, "%s%s\r", remove ? "-" : config.uplinkReq[uplinkMsgActive - 1].options.addPlusPrefix ? "+" : "", tag);
  ++uplinkMsgUsed;
  return 0;
}

u16 closeUplinkMsg(void)
{
  memset(&currentNode, 0, sizeof(nodeNumType));

  if ( !uplinkMsgActive )
    return 1;
  if ( !uplinkMsgUsed )
    return 0;

  strcat(message->text, "---\r");

  writeMsgLocal(message, NETMSG, 1);

  uplinkMsgActive = 0;
  return 0;
}
