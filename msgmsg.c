//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2017 Wilfred van Velzen
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
#include <dir.h>
#include <dirent.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"

#include "areafix.h"
#include "areainfo.h"
#include "log.h"
#include "minmax.h"
#include "msgmsg.h"
//#include "msgpkt.h"  // for openP
#include "nodeinfo.h"
#include "ping.h"
#include "spec.h"
#include "stpcpy.h"
#include "utils.h"
#include "version.h"

#define MAX_AFIX 256

const char *upcaseMonths = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";


fAttInfoType fAttInfo[MAX_ATTACH];
u16          fAttCount = 0;

extern configType  config;
extern char       *version;
extern char       *months;
extern const char *dayName[7];
extern internalMsgType *message;

s16 messagesMoved = 0;

//---------------------------------------------------------------------------
void moveMsg(char *msgName, char *destDir)
{
  s32            highMsgNum = 0;
  tempStrType    tempStr;
  DIR           *dir;
  struct dirent *ent;
  static s32     highMsgNumSent = 0
               , highMsgNumRcvd = 0;

  if (*destDir)
  {
    if (destDir == config.sentPath)
      highMsgNum = highMsgNumSent;
    else
      if (destDir == config.rcvdPath)
        highMsgNum = highMsgNumRcvd;

    if (highMsgNum == 0)
    {
      if ((dir = opendir(destDir)) != NULL)
      {
        while ((ent = readdir(dir)) != NULL)
          if (match_spec("*.msg", ent->d_name))
          {
            long mNum = atol(ent->d_name);
            highMsgNum = max(highMsgNum, mNum);
          }

        closedir(dir);
      }
    }
    sprintf(tempStr, "%s%u.msg", destDir, (u32)++highMsgNum);
    if (!moveFile(msgName, tempStr))
    {
      logEntryf(LOG_SENTRCVD, 0, "Moving %s to %s", msgName, tempStr);
      messagesMoved = 1;
    }
    if (destDir == config.sentPath)
      highMsgNumSent = highMsgNum;
    else
      if (destDir == config.rcvdPath)
        highMsgNumRcvd = highMsgNum;
  }
}
//---------------------------------------------------------------------------
static void removeTrunc(char *path)
{
  u16            count
               , count2;
  DIR           *dir;
  struct dirent *ent;
  char          *fileNamePtr;
  tempStrType    fileNameStr
               , pattern;

#ifdef _DEBUG0
  logEntryf(LOG_DEBUG | LOG_NOSCRN, 0, "DEBUG removeTrunc: %s", path);
#endif

  if ((dir = opendir(path)) != NULL)
  {
    fileNamePtr = stpcpy(fileNameStr, path);

    for (count = 0; count < 7; count++)
    {
      sprintf(pattern, "*.%.2s?", dayName[count]);

      count2 = 0xffff;
      while ((ent = readdir(dir)) != NULL)
      {
        if (!match_spec(pattern, ent->d_name))
          continue;

        strcpy(fileNamePtr, ent->d_name);

        if (access(fileNameStr, 6) == 0)  // File is writable
        {
          if (config.mailer == dMT_DBridge)
          {
            count2 = 0;
            while (  count2 < fAttCount
                  && stricmp(fAttInfo[count2].fileName, ent->d_name) != 0
                  )
              count2++;
          }
          if (count != timeBlock.tm_wday)
          {
            if (fileSize(fileNameStr) == 0 || count2 == fAttCount)
            {
              logEntryf(LOG_DEBUG, 0, "Removing: %s", fileNameStr);
              unlink(fileNameStr);
            }
          }
          else
          {
            if (count2 == fAttCount)
              close(open(fileNameStr, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE));
          }
        }
      }
      rewinddir(dir);
    }
    closedir(dir);
  }
}
//---------------------------------------------------------------------------
static void subRemTrunc(char *path)
{
  tempStrType    fStr;
  char          *fPtr
              , *helpPtr;
  DIR           *dir;
  struct dirent *ent;

  removeTrunc(path);

  if ((dir = opendir(path)) != NULL)
  {
    fPtr = stpcpy(fStr, path);
    while ((ent = readdir(dir)) != NULL)
    {
      if (!match_spec("*.pnt", ent->d_name))
        continue;

      helpPtr = stpcpy(fPtr, ent->d_name);
      if (dirExist(fStr))
      {
        *helpPtr++ = '\\';
        *helpPtr-- = 0;
        removeTrunc(fStr);
        *helpPtr = 0;
        rmdir(fStr);
      }
    }
    closedir(dir);
  }
}
//---------------------------------------------------------------------------
u32 totalBundleSize[MAX_NODES];

void initMsg(s16 noAreaFix)
{
  u16            count;
  u16            temp
               , containsNote;
  u16            scanCount;
  u16            aFixCount = 0
               , pingCount = 0;
  u32            aFixMsgNum[MAX_AFIX];
  u32            pingMsgNum[MAX_AFIX];
  DIR           *dir;
  struct dirent *ent;
  u32            msgNum;
  char          *helpPtr
              , *fPtr;
  tempStrType    fileNameStr
               , tempStr
               , textStr;
  char           drive[_MAX_DRIVE];
  char           dirStr[_MAX_DIR];
  char           name[_MAX_FNAME];
  char           ext[_MAX_EXT];
  fhandle        msgMsgHandle;
  fhandle        tempHandle;
  s32            arcSize;
  msgMsgType     msgMsg;
  nodeNumType    origNode
               , destNode;

  puts("Scanning netmail directory...\n");

  Delete(config.netPath, "*."MBEXTB);
  if (*config.pmailPath)
    Delete(config.pmailPath, "*."MBEXTB);

  // Check netmail dir for areafix and file attach messages

  memset(fAttInfo, 0, sizeof(fAttInfo));

  fPtr = stpcpy(fileNameStr, config.netPath);

  if ((dir = opendir(config.netPath)) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
    {
      if (!match_spec("*.msg", ent->d_name))
        continue;

      msgNum = strtoul(ent->d_name, &helpPtr, 10);

      if (msgNum != 0 && *helpPtr == '.')
      {
        sprintf(fPtr, "%u.msg", msgNum);

        if ((msgMsgHandle = open(fileNameStr, O_RDONLY | O_BINARY)) != -1)
        {
          if (read(msgMsgHandle, &msgMsg, sizeof(msgMsgType)) != sizeof(msgMsgType))
            close(msgMsgHandle);
          else
          {
            memset(textStr, 0, 256);
            count = read(msgMsgHandle, textStr, 255);
            close(msgMsgHandle);

            if (  config.mailOptions.killEmptyNetmail
               && !(msgMsg.attribute & (LOCAL | IN_TRANSIT | RET_REC_REQ | IS_RET_REC | AUDIT_REQ))
               && count < 255
               && emptyText(textStr)
               && !unlink(fileNameStr)
               )
            {
              logEntryf(LOG_SENTRCVD, 0, "Killing empty netmail message #%u, from: %s to: %s", msgNum, msgMsg.fromUserName, msgMsg.toUserName);
              messagesMoved = 1;
            }
            else
            {
              if (*config.sentPath && (msgMsg.attribute & SENT))
                moveMsg(fileNameStr, config.sentPath);
              else
              {
                if (msgMsg.attribute & RECEIVED)
                  moveMsg(fileNameStr, config.rcvdPath);
                else
                {
                  if (config.mailOptions.killBadFAtt &&
                      (msgMsg.attribute & FILE_ATT) &&
                      (strcmp(msgMsg.fromUserName, "ARCmail") == 0) &&
                      access(msgMsg.subject, 0) &&
                      (count < 255) &&
                      emptyText(textStr) &&
                      !unlink(fileNameStr))
                  {
                    logEntryf(LOG_SENTRCVD, 0, "Attached file not found, killing ARCmail message #%u", msgNum);
                    messagesMoved = 1;
                  }
                  else
                  {
                    memset(&origNode, 0, sizeof(nodeNumType));
                    memset(&destNode, 0, sizeof(nodeNumType));

                    if ((helpPtr = findCLStr(textStr, "\1INTL")) == NULL)
                      scanCount = 0;
                    else
                    {
                      scanCount = sscanf(helpPtr+6, "%hu:%hu/%hu %hu:%hu/%hu",
                                          &destNode.zone, &destNode.net,
                                          &destNode.node,
                                          &origNode.zone, &origNode.net,
                                          &origNode.node);
                    }
                    if ((helpPtr = findCLStr(textStr, "\1FMPT")) != NULL)
                      origNode.point = atoi(helpPtr+6);

                    if ((helpPtr = findCLStr(textStr, "\1TOPT")) != NULL)
                      destNode.point = atoi(helpPtr+6);

                    if (  stricmp(msgMsg.fromUserName, "ARCmail") == 0
                       && count < 256
                       && scanCount == 6
                       && (msgMsg.attribute & FILE_ATT)
                       && origNode.net == msgMsg.origNet
                       && destNode.net == msgMsg.destNet
                       && fAttCount < MAX_ATTACH
                       )
                    {
                      arcSize = 0;
                      if ((tempHandle = open(msgMsg.subject, O_RDONLY | O_BINARY)) != -1)
                      {
                        if ((arcSize = filelength(tempHandle)) == -1)
                          arcSize = 0;
                        else
                        {
                          for (temp = 0; temp < nodeCount ; temp++)
                            if (!memcmp(&nodeInfo[temp]->node, &destNode, sizeof(nodeNumType)))
                            {
                              totalBundleSize[temp] += arcSize;
                              break;
                            }
                        }
                        close(tempHandle);
                      }
                      _splitpath(msgMsg.subject, drive, dirStr, name, ext);
                      if (isdigit(ext[3] || isalpha(ext[3] && config.mailOptions.extNames))
                         && (  config.maxBundleSize == 0
                            || (arcSize >> 10) < config.maxBundleSize
  // necessary for prevention of truncation of mailbundles: (config.mailer == dMT_DBridge)
                            || config.mailer == dMT_DBridge
                            || toupper(ext[3]) == (int)(config.mailOptions.extNames ? 'Z' : '9')
                            )
                         )
                      {
                        strcpy(stpcpy(tempStr, drive), dirStr);

                        // are the first two letters of the extension correct?
                        for (count = 0; (count < 7) && strnicmp(dayName[count], ext + 1, 2) != 0; count++)
                        {}
                        if ( (!config.mailOptions.dailyMail || count == timeBlock.tm_wday)
                             && count != 7 && (stricmp(tempStr, config.outPath) == 0))
                        {
                          strcpy(stpcpy(fAttInfo[fAttCount].fileName, name), ext);

                          if (  fileSize(fileNameStr) != 0
                             && strlen(fAttInfo[fAttCount].fileName) == 12
                             )
                          {
                            fAttInfo[fAttCount].origNode   = origNode;
                            fAttInfo[fAttCount++].destNode = destNode;
                          }
                        }
                      }
                    }
                    else
                    {
                      if (  !(msgMsg.attribute & RECEIVED)
                         && !noAreaFix
                         )
                      {
                        // Check for Areafix & Ping messages
                        if (toAreaFix(msgMsg.toUserName))
                        {
                          if (aFixCount < MAX_AFIX)
                            aFixMsgNum[aFixCount++] = msgNum;
                        }
                        else
                          if (!config.pingOptions.disablePing && toPing(msgMsg.toUserName))
                          {
                            if (pingCount < MAX_AFIX)
                              pingMsgNum[pingCount++] = msgNum;
                          }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    closedir(dir);
  }
  if (messagesMoved == 1)
    newLine();

  // Process AreaFix messages.
  for (count = 0; count < aFixCount; count++)
  {
    if (readMsg(message, aFixMsgNum[count]) == 0)
    {
      if (getLocalAkaNum(&message->destNode) != -1)
      {
        // Message is for this node
        if (messagesMoved)
        {
          messagesMoved = 2;
          newLine();
        }
        temp = message->attribute;
        containsNote = findCLiStr(message->text, "%NOTE") != NULL;
        if (!areaFix(message))
        {
          if (config.mgrOptions.keepRequest || containsNote)
            attribMsg(temp | RECEIVED, aFixMsgNum[count]);
          else
          {
            sprintf(fPtr, "%u.msg", aFixMsgNum[count]);
            unlink(fileNameStr);
          }
          validateMsg();
        }
        newLine();
      }
    }
  }

  // Process Ping messages.
  if (!config.pingOptions.disablePing)
    for (count = 0; count < pingCount; count++)
    {
      if (readMsg(message, pingMsgNum[count]) == 0)
      {
        int localAkaNum = getLocalAkaNum(&message->destNode);
        if (localAkaNum >= 0)
        {
          // Message is for this node
          if (messagesMoved)
          {
            messagesMoved = 2;
            newLine();
          }
          if (!Ping(message, localAkaNum))
          {
            if (config.pingOptions.deletePingRequests)
            {
              sprintf(fPtr, "%u.msg", pingMsgNum[count]);
              unlink(fileNameStr);
              logEntryf(LOG_ALWAYS, 0, "Delete PING request message: %s", fileNameStr);
            }
            else
              attribMsg(message->attribute | RECEIVED, pingMsgNum[count]);
            validateMsg();
          }
          newLine();
        }
      }
    }

  // Remove old truncated mailbundles (or w/o file attach in D'B mode)
  if (config.mailer == dMT_Binkley || config.mailer == dMT_Xenia)
  {
    tempStrType dirStr;
    int bl;

    helpPtr = stpcpy(dirStr, config.outPath);
    *--helpPtr = 0;  // remove trailing '\'

    // Split dir and file
    if ((fPtr = strrchr(dirStr, '\\')) == NULL)
    {
      strcpy(tempStr, dirStr);
      strcpy(dirStr, ".\\");
    }
    else
    {
      strcpy(tempStr, ++fPtr);
      *fPtr = 0;
    }

    if ((helpPtr = strrchr(tempStr, '.')) != NULL)
      *helpPtr = 0;

    bl = strlen(tempStr);

#ifdef _DEBUG0
    logEntryf(LOG_DEBUG | LOG_NOSCRN, 0, "DEBUG initMsg: %s, %s", dirStr, tempStr);
#endif

    subRemTrunc(config.outPath);

    if ((dir = opendir(dirStr)) != 0)
    {
      while ((ent = readdir(dir)) != NULL)
      {
        char *dn = ent->d_name;
        int l = strlen(dn);

        // Check if hex extension of 3 or 4 chars.
        if (  l < bl + 4
           || l > bl + 5
           || dn[bl] != '.'
           || !isxdigit(dn[bl + 1])
           || !isxdigit(dn[bl + 2])
           || !isxdigit(dn[bl + 3])
           || (l == bl + 5 && !isxdigit(dn[bl + 4]))
           || strnicmp(dn, tempStr, bl) != 0
           )
          continue;

        sprintf(fileNameStr, "%s%s\\", dirStr, dn);
        subRemTrunc(fileNameStr);
      }
      closedir(dir);
    }
    else
      logEntryf(LOG_ALWAYS, 0, "*** Error opendir on: %s [%s]", dirStr, strError(errno));
  }
  else
    removeTrunc(config.outPath);
}
//---------------------------------------------------------------------------
u16 getFlags(char *text)
{
  u16      flags = 0;
  char     *helpPtr1
         , *helpPtr2
         , *helpPtr3;

  helpPtr1 = text;

  while ((helpPtr1 = findCLStr(helpPtr1, "\1FLAGS ")) != NULL)
  {
    helpPtr2 = strchr(helpPtr1, 0x0d);

    if (((helpPtr3 = strstr(helpPtr1, "IMM")) != NULL) &&
        (helpPtr3 < helpPtr2))
      flags |= FL_IMMEDIATE;

    if (((helpPtr3 = strstr(helpPtr1, "DIR")) != NULL) &&
        (helpPtr3 < helpPtr2))
      flags |= FL_DIRECT;

    if (((helpPtr3 = strstr(helpPtr1, "FRQ")) != NULL) &&
        (helpPtr3 < helpPtr2))
      flags |= FL_FILE_REQ;

    if (((helpPtr3 = strstr(helpPtr1, "LOK")) != NULL) &&
        (helpPtr3 < helpPtr2))
      flags |= FL_LOK;

    helpPtr1++;
  }

  return flags;
}
//---------------------------------------------------------------------------
s16 attribMsg(u16 attribute, s32 msgNum)
{
  tempStrType tempStr1;
  fhandle     msgMsgHandle;

  sprintf(tempStr1, "%s%u.msg", config.netPath, msgNum);

  if (((msgMsgHandle = open(tempStr1, O_WRONLY | O_BINARY)) == -1) ||
      (lseek(msgMsgHandle, sizeof(msgMsgType)-4, SEEK_SET) == -1) ||
      (write(msgMsgHandle, &attribute, 2) != 2))
  {
    close(msgMsgHandle);
    logEntryf(LOG_ALWAYS, 0, "Can't update file %s", tempStr1);
    return -1;
  }
  close(msgMsgHandle);

  if (attribute & RECEIVED)
    moveMsg(tempStr1, config.rcvdPath);
  else
    if (attribute & SENT)
      moveMsg(tempStr1, config.sentPath);

  globVars.createSemaphore++;
  return 0;
}
//---------------------------------------------------------------------------

s16 readMsg(internalMsgType *message, s32 msgNum)
{
  tempStrType tempStr;
  fhandle     msgMsgHandle;
  char       *helpPtr;
  msgMsgType  msgMsg;
  u16         count;

  memset(message, 0, INTMSG_SIZE);

  sprintf(tempStr, "%s%u.msg", config.netPath, msgNum);

  if ((msgMsgHandle = _sopen(tempStr, O_RDONLY | O_BINARY, SH_DENYRW)) == -1)
  {
    logEntryf(LOG_ALWAYS, 0, "Can't open file %s", tempStr);
    return -1;
  }

  if (  (filelength(msgMsgHandle) > (long)(sizeof(msgMsgType) + TEXT_SIZE))
     || read(msgMsgHandle, &msgMsg, sizeof(msgMsgType)) != (int)sizeof(msgMsgType)
     )
  {
    close(msgMsgHandle);
    return (-1);
  }

  read(msgMsgHandle, message->text, TEXT_SIZE);
  close(msgMsgHandle);

  strcpy (message->fromUserName, msgMsg.fromUserName);
  strcpy (message->toUserName,   msgMsg.toUserName);
  strcpy (message->subject,      msgMsg.subject);
  strncpy(message->dateStr,      msgMsg.dateTime, 19);

  helpPtr = message->dateStr;
  message->seconds = 0;

  if (!(isdigit(*helpPtr) || isdigit(*(helpPtr + 1)) || isdigit(*(helpPtr + 2))))  // Skip day-of-week
    helpPtr += 4;

  if (sscanf(helpPtr, "%hd-%hd-%hd %hd:%hd:%hd", &message->day,
              &message->month,
              &message->year,
              &message->hours,
              &message->minutes,
              &message->seconds) < 5)
  {
    if (sscanf(helpPtr, "%hd %s %hd %hd:%hd:%hd", &message->day,
                tempStr,
                &message->year,
                &message->hours,
                &message->minutes,
                &message->seconds) < 5)
    {
      printf("\nError in date: %s\n", message->dateStr);

      message->day     =  1;
      message->month   =  1;
      message->year    = 80;
      message->hours   =  0;
      message->minutes =  0;
    }
    else
    {
      message->month = ((strstr(upcaseMonths, strupr(tempStr)) - upcaseMonths) / 3) + 1;

      if (message->year >= 80)
        message->year += 1900;
      else
        message->year += 2000;
    }
  }

  if ((message->year < 1980) || (message->year > 2099))
    message->year = 1980;

  if ((message->month == 0) || (message->month > 12))
    message->month = 1;

  if ((message->day == 0) || (message->day > 31))
    message->day = 1;

  if (message->hours >= 24)
    message->hours = 0;

  if (message->minutes >= 60)
    message->minutes = 0;

  if (message->seconds >= 60)
    message->seconds = 0;

  message->attribute     = msgMsg.attribute;
  message->srcNode.net   = msgMsg.origNet;
  message->srcNode.node  = msgMsg.origNode;
  message->destNode.net  = msgMsg.destNet;
  message->destNode.node = msgMsg.destNode;

  make4d(message);

  // re-route to point
  if (getLocalAka(&message->destNode) >= 0)
  {
    for (count = 0; count < nodeCount; count++)
    {
      if (  nodeInfo[count]->options.routeToPoint
         && isLocalPoint(&(nodeInfo[count]->node))
         && memcmp(&(nodeInfo[count]->node), &message->destNode, 6) == 0
         && stricmp(nodeInfo[count]->sysopName, message->toUserName) == 0
         )
      {
        sprintf(tempStr,"\1TOPT %u\r", message->destNode.point = nodeInfo[count]->node.point);
        insertLine(message->text, tempStr);
        if (!(message->attribute & LOCAL))
          message->attribute |= IN_TRANSIT;
        break;
      }
    }
  }

  return 0;
}
//---------------------------------------------------------------------------
#define MAX_FNPTR 8

s32 writeMsg(internalMsgType *message, s16 msgType, s16 valid)
{
// valid = 0 : write .FML file
// valid = 1 : write .MSG file
// valid = 2 : write READ-ONLY .MSG file

  fhandle        msgHandle;
  int            len;
  tempStrType    tempStr
               , tempFName
               ;
  char          *helpPtr;
  s32            highMsgNum;
  u16            count;
  msgMsgType     msgMsg;
  DIR           *dir;
  struct dirent *ent;
  char          *fnPtr[MAX_FNPTR];
  u16            xu
               , fnPtrCnt = 0;

  memset(&msgMsg, 0, sizeof(msgMsgType));

  strcpy(msgMsg.fromUserName, message->fromUserName);
  strcpy(msgMsg.toUserName, message->toUserName);

  if ((message->attribute & FILE_ATT) && strchr(message->subject, '\\') == NULL)
  {
    helpPtr = strchr(strcpy(tempFName, message->subject), ' ');
    if (helpPtr != NULL)
      *(helpPtr++) = 0;
    if (strlen(config.inPath) + strlen(message->subject) < 72)
      strcpy(stpcpy(msgMsg.subject, config.inPath), tempFName);
    else
      strcpy(msgMsg.subject, tempFName);

    while (helpPtr != NULL && fnPtrCnt < MAX_FNPTR)
    {
      while (*helpPtr == ' ')
        ++helpPtr;
      if (!*helpPtr)
        break;
      fnPtr[fnPtrCnt++] = helpPtr;
      if ((helpPtr = strchr(helpPtr, ' ')) != NULL)
        *(helpPtr++) = 0;
    }
  }
  else
    strcpy(msgMsg.subject, message->subject);

  sprintf( msgMsg.dateTime, "%02u %.3s %02u  %02u:%02u:%02u"
         , message->day       , months + (message->month - 1) * 3
         , message->year % 100, message->hours
         , message->minutes   , message->seconds);

  msgMsg.wrTime.hours    = message->hours;
  msgMsg.wrTime.minutes  = message->minutes;
  msgMsg.wrTime.dSeconds = message->seconds >> 1;
  msgMsg.wrTime.day      = message->day;
  msgMsg.wrTime.month    = message->month;
  msgMsg.wrTime.year     = message->year-1980;

  msgMsg.origNet  = message->srcNode.net;
  msgMsg.origNode = message->srcNode.node;
  msgMsg.destNet  = message->destNode.net;
  msgMsg.destNode = message->destNode.node;

  msgMsg.attribute = message->attribute;

  switch (msgType)
  {
    case NETMSG:
      helpPtr = stpcpy(tempStr, config.netPath);
      break;

    case PERMSG:
      helpPtr = stpcpy(tempStr, config.pmailPath);
      break;

    case SECHOMSG:
      helpPtr = stpcpy(tempStr, config.sentEchoPath);
      break;

    default:
      return -1;
  }

  // Determine highest message

  highMsgNum = 0;
  strcpy(helpPtr, dLASTREAD);
  if (valid && ((msgHandle = open(tempStr, O_RDONLY | O_BINARY)) != -1))
  {
    if (read(msgHandle, &highMsgNum, 2) != 2)
      highMsgNum = 0;

    close(msgHandle);
  }
  *helpPtr = 0;
  if ((dir = opendir(tempStr)) != NULL)
  {
    strcpy(helpPtr, valid ? "*.msg" : "*."MBEXTB );

    while ((ent = readdir(dir)) != NULL)
      if (match_spec(helpPtr, ent->d_name))
      {
        long mn = atol(ent->d_name);
        highMsgNum = max(highMsgNum, mn);
      }

    closedir(dir);
  }

  xu = 0;
  do
  {
    if (xu)
    {
      if (strlen(config.inPath) + strlen(fnPtr[xu - 1]) < 72)
        strcpy(stpcpy(msgMsg.subject, config.inPath), fnPtr[xu - 1]);
      else
        strcpy(msgMsg.subject, fnPtr[xu - 1]);
    }

    // Try to open file
    count = 0;
    sprintf(helpPtr, valid ? "%u.msg" : "%u."MBEXTB, ++highMsgNum);

    while (  count < 20
          && -1 == (msgHandle = open(tempStr, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE))
          )
    {
      highMsgNum += count++ < 10 ? 1 : 10;
      sprintf(helpPtr, valid ? "%u.msg" : "%u."MBEXTB, highMsgNum);
    }

    if (msgHandle == -1)
    {
      puts("Can't open output file.");
      return -1;
    }

    len = strlen(message->text) + 1;

    if (  write(msgHandle, &msgMsg, sizeof(msgMsgType)) != (int)sizeof(msgMsgType)
       || write(msgHandle, message->text, len) != len
       )
    {
      close(msgHandle);
      puts("Can't write to output file.");

      return -1;
    }
    close(msgHandle);

#ifdef _DEBUG
    logEntryf(LOG_DEBUG, 0, "DEBUG Message file writen: %s", tempStr);
#endif

    if (valid == 2)
      chmod(tempStr, S_IREAD);

    switch (msgType)
    {
      case NETMSG:
        globVars.netCount++;
        globVars.createSemaphore++;
        break;

      case PERMSG:
        globVars.perCount++;
        break;

      default:
        return -1;
    }
  }
  while (++xu <= fnPtrCnt);

  return highMsgNum;
}
//---------------------------------------------------------------------------
s32 writeMsgLocal(internalMsgType *message, s16 msgType, s16 valid)
{
  char *p;
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);  // localtime -> should be ok

  message->hours   = tm->tm_hour;
  message->minutes = tm->tm_min;
  message->seconds = tm->tm_sec;
  message->day     = tm->tm_mday;
  message->month   = tm->tm_mon + 1;
  message->year    = tm->tm_year + 1900;

  message->attribute |= LOCAL;

  p = addINTLKludge  (message, NULL);
  p = addPointKludges(message, p);
  p = addMSGIDKludge (message, p);
  p = addTZUTCKludge (p);
      addPIDKludge   (p);

  return writeMsg(message, msgType, valid);
}
//---------------------------------------------------------------------------
void validateMsg(void)
{
  s16            c
               , count;
  tempStrType    tempStr1
              ,  tempStr2;
  char          *helpPtr1
              , *helpPtr2;
  s32            highMsgNum;
  DIR           *dir;
  struct dirent *ent;

  for (c = PERMSG; c <= NETMSG; c++)
  {
    if ((c == NETMSG && globVars.netCount) || (c == PERMSG && globVars.perCount))
    {
      switch (c)
      {
        case NETMSG:
          helpPtr1 = stpcpy(tempStr1, config.netPath);
          break;
        case PERMSG:
          helpPtr1 = stpcpy(tempStr1, config.pmailPath);
          break;
      }

      helpPtr2 = stpcpy(tempStr2, tempStr1);

      // Determine highest message
      highMsgNum = 0;
      if ((dir = opendir(tempStr1)) != NULL)
      {
        while ((ent = readdir(dir)) != NULL)
          if (match_spec("*.msg", ent->d_name))
          {
            long mn = atol(ent->d_name);
            highMsgNum = max(highMsgNum, mn);
          }

        rewinddir(dir);

        // Try to rename file
        while ((ent = readdir(dir)) != NULL)
          if (match_spec("*."MBEXTB, ent->d_name))
          {
            strcpy(helpPtr1, ent->d_name);

            count = 0;
            do
            {
              highMsgNum += count < 10 ? 1 : 10;
              sprintf(helpPtr2, "%u.msg", highMsgNum);
            }
            while (count++ < 20 && rename(tempStr1, tempStr2));
          }

        closedir(dir);
      }
    }
  }
  globVars.netCountV    += globVars.netCount;
  globVars.perCountV    += globVars.perCount;
  globVars.perNetCountV += globVars.perNetCount;
  globVars.netCount     = 0;
  globVars.perCount     = 0;
  globVars.perNetCount  = 0;
}
//---------------------------------------------------------------------------
s16 fileAttach(char *fileName, nodeNumType *srcNd, nodeNumType *destNd, nodeInfoType *nodeInfo)
{
  char *helpPtr;

  memset(message, 0, INTMSG_SIZE);

  strcpy(message->fromUserName, "ARCmail");
  strcpy(message->toUserName, *nodeInfo->sysopName ? nodeInfo->sysopName : "SysOp");
  strcpy(message->subject, fileName);

  message->srcNode  = *srcNd;
  message->destNode = *destNd;

  helpPtr = stpcpy(message->text, nodeInfo->archiver != 0xFF ? "\1FLAGS TFS" : "\1FLAGS KFS");

  message->attribute = LOCAL | PRIVATE | FILE_ATT | KILLSENT;

  switch (nodeInfo->outStatus)
  {
    case 1:
      message->attribute |= HOLDPICKUP;
      break;
    case 2:
      message->attribute |= CRASH;
      break;
    case 3:
      message->attribute |= HOLDPICKUP;
      helpPtr = stpcpy(helpPtr, " DIR");
      break;
    case 4:
      message->attribute |= CRASH;
    case 5:
      helpPtr = stpcpy(helpPtr, " DIR");
      break;
  }

  strcpy(helpPtr, "\r");

  if (writeMsgLocal(message, NETMSG, 1) == -1)
    return 1;

  logEntryf(LOG_OUTBOUND, 0, "Created file attach netmail from %s to %s", nodeStr(srcNd), nodeStr(destNd));

  return 0;
}
//---------------------------------------------------------------------------
