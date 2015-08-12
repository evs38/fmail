//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2014 Wilfred van Velzen
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

#include "areafix.h"
#include "areainfo.h"
#include "log.h"
#include "msgmsg.h"
#include "msgpkt.h"  // for openP
#include "nodeinfo.h"
#include "output.h"
#include "utils.h"
#include "version.h"

#define MAX_AFIX 256

const char *upcaseMonths = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";


fAttInfoType fAttInfo[MAX_ATTACH];
u16          fAttCount = 0;


extern time_t     startTime;
extern struct tm  timeBlock;
extern configType config;
extern char       *version;
extern char       *months;
extern const char *dayName[7];
extern s16        zero;

extern const char *upcaseMonths;

extern globVarsType    globVars;

extern internalMsgType *message;


s16 messagesMoved = 0;

//---------------------------------------------------------------------------
void moveMsg(char *msgName, char *destDir)
{
  s32          highMsgNum;
  struct ffblk ffblkMsg;
  tempStrType  tempStr, tempStr2;
  s16          doneMsg;

  static s32   highMsgNumSent = 0,
                                highMsgNumRcvd = 0;

  if (*destDir)
  {
    if (destDir == config.sentPath) highMsgNum = highMsgNumSent;
    else if (destDir == config.rcvdPath) highMsgNum = highMsgNumRcvd;

    if (highMsgNum == 0)
    {
      strcpy (tempStr, destDir);
      strcat (tempStr, "*.msg");
      doneMsg = findfirst (tempStr, &ffblkMsg, FA_RDONLY|FA_HIDDEN|FA_SYSTEM|
                           /*FA_LABEL|*/FA_DIREC);
      while (!doneMsg)
      {
        highMsgNum = max (highMsgNum, atol (ffblkMsg.ff_name));
        doneMsg = findnext (&ffblkMsg);
      }
    }
    sprintf (tempStr, "%s%u.msg", destDir, ++highMsgNum);
    if (!moveFile(msgName, tempStr))
    {
      sprintf (tempStr2, "Moving %s to %s", msgName, tempStr);
      logEntry (tempStr2, LOG_SENTRCVD, 0);
      messagesMoved = 1;
    }
    if (destDir == config.sentPath) highMsgNumSent = highMsgNum;
    else if (destDir == config.rcvdPath) highMsgNumRcvd = highMsgNum;
  }
}
//---------------------------------------------------------------------------
static void removeTrunc(char *path)
{
  s16          doneMsg;
  u16          count, count2;
  struct ffblk ffblkMsg;
  tempStrType  fileNameStr;

  for (count = 0; count < 7; count++)
  {
    sprintf (fileNameStr, "%s*.%.2s?", path, dayName[count]);
    doneMsg = findfirst (fileNameStr, &ffblkMsg, 0);

    count2 = 0xffff;
    while (!doneMsg)
    {
      if ((ffblkMsg.ff_attrib & FA_RDONLY) == 0)
      {
        if (config.mailer == 2)
        {
          count2 = 0;
          while ((count2 < fAttCount) &&
                 (stricmp (fAttInfo[count2].fileName, ffblkMsg.ff_name) != 0))
          {
            count2++;
          }
        }
        if (count != timeBlock.tm_wday)
        {
          if ((ffblkMsg.ff_fsize == 0) || (count2 == fAttCount))
          {
            strcpy (fileNameStr, path);
            strcat (fileNameStr, ffblkMsg.ff_name);
            unlink (fileNameStr);
          }
        }
        else
        {
          if (count2 == fAttCount)
          {
            strcpy (fileNameStr, path);
            strcat (fileNameStr, ffblkMsg.ff_name);
            close(openP(fileNameStr, O_BINARY|O_CREAT|O_TRUNC|O_RDWR, S_IREAD|S_IWRITE));
          }
        }
      }
      doneMsg = findnext (&ffblkMsg);
    }
  }
}
//---------------------------------------------------------------------------
static void subRemTrunc(char *path)
{
  tempStrType  tempStr;
  char         *helpPtr, *helpPtr2;
  s16          doneDir;
  struct ffblk ffblkDir;

  removeTrunc(path);

  strcpy(helpPtr = stpcpy(tempStr, path), "*.pnt");

  doneDir = findfirst (tempStr, &ffblkDir, FA_RDONLY|FA_HIDDEN|
                       FA_SYSTEM|/*FA_LABEL|*/FA_DIREC);
  while (!doneDir)
  {
    if (ffblkDir.ff_attrib & FA_DIREC)
    {
      strcpy (helpPtr2 = stpcpy(helpPtr, ffblkDir.ff_name), "\\");
      removeTrunc(tempStr);
      *helpPtr2 = 0;
      rmdir(tempStr);
    }
    doneDir = findnext (&ffblkDir);
  }
}
//---------------------------------------------------------------------------
u32 totalBundleSize[MAX_NODES];

void initMsg (s16 noAreaFix)
{
  s16          doneMsg;
  u16          count;
  u16          temp, containsNote;
  u16          scanCount;
  u16          aFixCount = 0;
  u16          aFixMsgNum[MAX_AFIX];
  char         textStr[256];
  struct ffblk ffblkMsg, ffbfatt;
  u16          msgNum;
  char         *helpPtr;
  tempStrType  fileNameStr;

  char         drive[MAXDRIVE];
  char         dir[MAXDIR];
  char         name[MAXFILE];
  char         ext[MAXEXT];
  fhandle      msgMsgHandle;
  fhandle      tempHandle;
  s32          arcSize;
  msgMsgType   msgMsg;
  nodeNumType  origNode,
  destNode;

  printString ("Scanning netmail directory...\n\n");

  Delete(config.netPath, "*."MBEXTB);
  if (*config.pmailPath)
    Delete(config.pmailPath, "*."MBEXTB);

  /* Check netmail dir for areafix and file attach messages  */

  memset (fAttInfo, 0, sizeof(fAttInfo));

  strcpy (fileNameStr, config.netPath);
  strcat (fileNameStr, "*.msg");

  doneMsg = findfirst (fileNameStr, &ffblkMsg, FA_RDONLY|FA_HIDDEN|
                       FA_SYSTEM/*|FA_LABEL*/|FA_DIREC);
  while ((!doneMsg) && (!breakPressed))
  {
    msgNum = (u16)strtoul (ffblkMsg.ff_name, &helpPtr, 10);

    if ((msgNum != 0) && (*helpPtr == '.') &&
        (!(ffblkMsg.ff_attrib & FA_SPEC)))
    {
      sprintf (fileNameStr, "%s%u.msg", config.netPath, msgNum);

      if ((msgMsgHandle = openP(fileNameStr,
                                O_RDONLY|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) != -1)
      {
        if (read (msgMsgHandle, &msgMsg, sizeof(msgMsgType)) !=
            sizeof(msgMsgType))
        {
          close(msgMsgHandle);
        }
        else
        {
          memset (textStr, 0, 256);
          count = _read (msgMsgHandle, textStr, 255);
          close(msgMsgHandle);

          if (config.mailOptions.killEmptyNetmail &&
              (!(msgMsg.attribute & (LOCAL|IN_TRANSIT| /* FILE_ATT|FILE_REQ|FILE_UPD_REQ| */
                                     RET_REC_REQ|IS_RET_REC|AUDIT_REQ))) &&
              (count < 255) &&
              emptyText(textStr) &&
              !unlink (fileNameStr))
          {
            sprintf (fileNameStr, "Killing empty netmail message #%u", msgNum);
            logEntry (fileNameStr, LOG_SENTRCVD, 0);
            messagesMoved = 1;
          }
          else
          {
            if ((*config.sentPath) && (msgMsg.attribute & SENT))
              /*              (!(msgMsg.attribute & (ORPHAN|IN_TRANSIT|FILE_ATT|FILE_REQ|
               		          RET_REC_REQ|IS_RET_REC|AUDIT_REQ|
               		          FILE_UPD_REQ))))
              */
            {
              moveMsg (fileNameStr, config.sentPath);
            }
            else
            {
              if (msgMsg.attribute & RECEIVED)
              {
                moveMsg (fileNameStr, config.rcvdPath);
              }
              else
              {
                if (config.mailOptions.killBadFAtt &&
                    (msgMsg.attribute & FILE_ATT) &&
                    (strcmp(msgMsg.fromUserName, "ARCmail") == 0) &&
                    findfirst(msgMsg.subject, &ffbfatt, 0) &&
                    (count < 255) &&
                    emptyText(textStr) &&
                    !unlink (fileNameStr))
                {
                  sprintf (fileNameStr, "Attached file not found, killing ARCmail message #%u", msgNum);
                  logEntry (fileNameStr, LOG_SENTRCVD, 0);
                  messagesMoved = 1;
                }
                else
                {
                  memset (&origNode, 0, sizeof(nodeNumType));
                  memset (&destNode, 0, sizeof(nodeNumType));

                  if ((helpPtr = findCLStr (textStr, "\1INTL")) == NULL)
                  {
                    scanCount = 0;
                  }
                  else
                  {
                    scanCount = sscanf (helpPtr+6, "%hu:%hu/%hu %hu:%hu/%hu",
                                        &destNode.zone, &destNode.net,
                                        &destNode.node,
                                        &origNode.zone, &origNode.net,
                                        &origNode.node);
                  }
                  if ((helpPtr = findCLStr (textStr, "\1FMPT")) != NULL)
                  {
                    origNode.point = atoi(helpPtr+6);
                  }
                  if ((helpPtr = findCLStr (textStr, "\1TOPT")) != NULL)
                  {
                    destNode.point = atoi(helpPtr+6);
                  }
                  if ((stricmp (msgMsg.fromUserName, "ARCmail") == 0) &&
                      (count < 256) &&
                      (scanCount == 6) &&
                      (msgMsg.attribute & FILE_ATT) &&
                      (origNode.net == msgMsg.origNet) &&
                      (destNode.net == msgMsg.destNet) &&
                      (fAttCount < MAX_ATTACH))
                  {
                    arcSize = 0;
                    if ((tempHandle = openP(msgMsg.subject, O_RDONLY|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) != -1)
                    {
                      if ((arcSize = filelength(tempHandle)) == -1)
                        arcSize = 0;
                      else
                      {
                        for ( temp = 0; temp < nodeCount ; temp++ )
                          if ( !memcmp(&nodeInfo[temp]->node, &destNode, sizeof(nodeNumType)) )
                          {
                            totalBundleSize[temp] += arcSize;
                            break;
                          }
                      }
                      close(tempHandle);
                    }
                    fnsplit(msgMsg.subject, drive, dir, name, ext);
                    if ((isdigit(ext[3]) || (isalpha(ext[3]) && config.mailOptions.extNames))
                       && (  config.maxBundleSize == 0
                          || (arcSize >> 10) < config.maxBundleSize
// necessary for prevention of truncation of mailbundles: (config.mailer == 2)
                          || config.mailer == 2
                          || toupper(ext[3]) == (int)(config.mailOptions.extNames ? 'Z' : '9')
                          )
                       )
                    {
                      strcpy (fileNameStr, drive);
                      strcat (fileNameStr, dir);

                      /* are the first two letters of the extension correct? */
                      for (count = 0; (count < 7) &&
                           (strnicmp(dayName[count],ext+1,2) != 0); count++)
                        {}
                      if ( (!config.mailOptions.dailyMail || count == timeBlock.tm_wday)
                           && count != 7 && (stricmp (fileNameStr, config.outPath) == 0))
                      {
                        strcpy (fAttInfo[fAttCount].fileName, name);
                        strcat (fAttInfo[fAttCount].fileName, ext);

                        if ((ffblkMsg.ff_fsize != 0) &&
                            (strlen(fAttInfo[fAttCount].fileName) == 12))
                        {
                          fAttInfo[fAttCount].origNode   = origNode;
                          fAttInfo[fAttCount++].destNode = destNode;
                        }
                      }
                    }
                  }
                  else
                  {
                    /* Check for Areafix message */

                    if ((aFixCount < MAX_AFIX) &&
                        (!(msgMsg.attribute & RECEIVED)) &&
                        (!noAreaFix) &&
                        (toAreaFix(msgMsg.toUserName)))
                    {
                      aFixMsgNum[aFixCount++] = msgNum;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    doneMsg = findnext (&ffblkMsg);
  }
  if (messagesMoved == 1)
  {
    newLine ();
  }

  for (count = 0; count < aFixCount; count++)
  {
    if (readMsg(message, aFixMsgNum[count]) == 0)
    {
      if (getLocalAkaNum(&message->destNode) != -1)
      {
        // Message is for this node

        sprintf(fileNameStr, "%s%u.msg", config.netPath, aFixMsgNum[count]);

        // DENYNONE was DENYALL
        if ((msgMsgHandle = openP(fileNameStr, O_RDWR|O_DENYNONE|O_BINARY,S_IREAD|S_IWRITE)) != -1)
        {
          if (messagesMoved)
          {
            messagesMoved = 2;
            newLine ();
          }
          temp = message->attribute;
          containsNote = (findCLiStr(message->text, "%NOTE") != NULL);
          if (!areaFix (message))
          {
            close (msgMsgHandle);
            if ( config.mgrOptions.keepRequest || containsNote )
              attribMsg (temp|RECEIVED, aFixMsgNum[count]);
            else
              unlink (fileNameStr);

            validateMsg();
          }
          else
            close(msgMsgHandle);

          newLine();
        }
      }
    }
  }

  // Remove old truncated mailbundles (or w/o file attach in D'B mode)

  if (config.mailer == 3 || config.mailer == 5)
  {
    if ((helpPtr = strchr(strrchr(strcpy(fileNameStr, config.outPath), '\\'), '.')) != NULL)
      strcpy(helpPtr, ".*");
    else
      strcpy(fileNameStr+strlen(fileNameStr)-1, ".*");
    if ( (helpPtr = strrchr(fileNameStr, '\\')) == NULL )
      helpPtr = fileNameStr;
    else
      ++helpPtr;

    doneMsg = findfirst (fileNameStr, &ffblkMsg, FA_RDONLY|FA_HIDDEN|
                         FA_SYSTEM|/*FA_LABEL|*/FA_DIREC);
    while (!doneMsg)
    {
      if (ffblkMsg.ff_attrib & FA_DIREC)
      {
        strcpy(stpcpy(helpPtr, ffblkMsg.ff_name), "\\");
        subRemTrunc(fileNameStr);
      }
      doneMsg = findnext (&ffblkMsg);
    }
  }
  else
    removeTrunc(config.outPath);
}
//---------------------------------------------------------------------------
u16 getFlags(char *text)
{
  u16      flags = 0;
  char     *helpPtr1,
  *helpPtr2,
  *helpPtr3;

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
  tempStrType tempStr1
            , tempStr2;
  fhandle     msgMsgHandle;

  sprintf(tempStr1, "%s%lu.msg", config.netPath, msgNum);

  if (((msgMsgHandle = openP(tempStr1, O_RDWR|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) == -1) ||
      (lseek(msgMsgHandle, sizeof(msgMsgType)-4, SEEK_SET) == -1) ||
      (_write(msgMsgHandle, &attribute, 2) != 2))
  {
    close(msgMsgHandle);
    sprintf(tempStr2, "Can't update file %s", tempStr1);
    logEntry(tempStr2, LOG_ALWAYS, 0);
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
extern u16           nodeCount;

s16 readMsg(internalMsgType *message, s32 msgNum)
{
  tempStrType tempStr1,
  tempStr2;
  fhandle     msgMsgHandle;
  char        *helpPtr;
  msgMsgType  msgMsg;
  u16         count;

  memset (message, 0, INTMSG_SIZE);

  sprintf (tempStr1, "%s%lu.msg", config.netPath, msgNum);

  if ((msgMsgHandle = openP(tempStr1, O_RDONLY|O_BINARY|O_DENYALL,S_IREAD|S_IWRITE)) == -1)
  {
    sprintf (tempStr2, "Can't open file %s", tempStr1);
    logEntry (tempStr2, LOG_ALWAYS, 0);
    return (-1);
  }

  if (  (filelength(msgMsgHandle) > (long)(sizeof(msgMsgType) + TEXT_SIZE))
     || _read(msgMsgHandle, &msgMsg, sizeof(msgMsgType)) != sizeof(msgMsgType)
     )
  {
    close(msgMsgHandle);
    return (-1);
  }

  _read (msgMsgHandle, message->text, TEXT_SIZE);
  close(msgMsgHandle);

  strcpy  (message->fromUserName, msgMsg.fromUserName);
  strcpy  (message->toUserName,   msgMsg.toUserName);
  strcpy  (message->subject,      msgMsg.subject);
  strncpy (message->dateStr,      msgMsg.dateTime, 19);

  helpPtr = message->dateStr;
  message->seconds = 0;

  if (!((isdigit(*helpPtr)) || (isdigit(*(helpPtr+1))) ||
        (isdigit(*(helpPtr+2)))))                     /* Skip day-of-week */
    helpPtr += 4;

  if (sscanf (helpPtr, "%hd-%hd-%hd %hd:%hd:%hd", &message->day,
              &message->month,
              &message->year,
              &message->hours,
              &message->minutes,
              &message->seconds) < 5)
  {
    if (sscanf (helpPtr, "%hd %s %hd %hd:%hd:%hd", &message->day,
                tempStr1,
                &message->year,
                &message->hours,
                &message->minutes,
                &message->seconds) < 5)
    {
      printString ("\nError in date: ");
      printString (message->dateStr);
      newLine ();

      message->day     =  1;
      message->month   =  1;
      message->year    = 80;
      message->hours   =  0;
      message->minutes =  0;
    }
    else
    {
      message->month = (((s16)strstr(upcaseMonths,strupr(tempStr1))-
                         (s16)upcaseMonths)/3) + 1;

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

  make4d (message);

  // re-route to point
  if (getLocalAka(&message->destNode) >= 0)
  {
    count = 0;
    while (count < nodeCount)
    {
      if ((nodeInfo[count]->options.routeToPoint) &&
          (isLocalPoint(&(nodeInfo[count]->node))) &&
          (memcmp(&(nodeInfo[count]->node), &message->destNode, 6) == 0) &&
          (stricmp(nodeInfo[count]->sysopName, message->toUserName) == 0))
      {
        sprintf(tempStr1,"\1TOPT %u\r", message->destNode.point = nodeInfo[count]->node.point);
        insertLine(message->text, tempStr1);
        if (! message->attribute & LOCAL)
          message->attribute |= IN_TRANSIT;
        count = nodeCount;
      }
      else
        count++;
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

  fhandle      msgHandle;
  int          len;
  tempStrType  tempStr
             , tempFName;
  char        *helpPtr;
  s32          highMsgNum;
  u16          count;
  msgMsgType   msgMsg;
  s16          doneMsg;
  struct ffblk ffblkMsg;
  char        *fnPtr[MAX_FNPTR];
  u16          xu
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
         , message->day,      months + (message->month - 1) * 3
         , message->year%100, message->hours
         , message->minutes,  message->seconds);

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
  strcpy(helpPtr, "LASTREAD");
  if (valid && ((msgHandle = openP(tempStr, O_RDONLY|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) != -1))
  {
    if (read(msgHandle, &highMsgNum, 2) != 2)
      highMsgNum = 0;
    close(msgHandle);
  }
  strcpy (helpPtr, valid ? "*.msg" : "*."MBEXTB );
  doneMsg = findfirst(tempStr, &ffblkMsg, FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_DIREC);

  while (!doneMsg)
  {
    highMsgNum = max(highMsgNum, atoi(ffblkMsg.ff_name));
    doneMsg = findnext(&ffblkMsg);
  }

  xu = 0;
  do
  {
    if (xu)
    {
      if (strlen(config.inPath) + strlen(fnPtr[xu-1]) < 72)
        strcpy(stpcpy(msgMsg.subject, config.inPath), fnPtr[xu-1]);
      else
        strcpy(msgMsg.subject, fnPtr[xu-1]);
    }

    // Try to open file
    count = 0;
    sprintf(helpPtr, valid ? "%lu.msg" : "%lu."MBEXTB, ++highMsgNum);

    while (  count < 20
          && (msgHandle = openP( tempStr, O_RDWR|O_CREAT|O_EXCL|O_TRUNC|O_BINARY|O_DENYNONE
                               , S_IREAD|S_IWRITE)) == -1)
    {
      highMsgNum += count++ < 10 ? 1 : 10;
      sprintf(helpPtr, valid ? "%lu.msg" : "%lu."MBEXTB, highMsgNum);
    }

    if (msgHandle == -1)
    {
      printString("Can't open output file.\n");
      return -1;
    }

    len = strlen(message->text) + 1;

    if (  _write(msgHandle, &msgMsg, sizeof(msgMsgType)) != (int)sizeof(msgMsgType)
       || _write(msgHandle, message->text, len) != len
       )
    {
      close(msgHandle);
      printString("Can't write to output file.\n");
      return -1;
    }
    close(msgHandle);

    if (valid == 2)
      chmod(tempStr, S_IREAD);

    {
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
  }
  while (++xu <= fnPtrCnt);

  return highMsgNum;
}
//---------------------------------------------------------------------------
s32 writeMsgLocal(internalMsgType *message, s16 msgType, s16 valid)
{
  struct date dateRec;
  struct time timeRec;
  tempStrType tempStr;

  getdate(&dateRec);
  gettime(&timeRec);

  message->hours     = timeRec.ti_hour;
  message->minutes   = timeRec.ti_min;
  message->seconds   = timeRec.ti_sec;
  message->day       = dateRec.da_day;
  message->month     = dateRec.da_mon;
  message->year      = dateRec.da_year;

  message->attribute |= LOCAL;

  // PID kludge

  sprintf(tempStr, "\1PID: %s\r", PIDStr());
  insertLine(message->text, tempStr);

  // MSGID kludge

  sprintf(tempStr, "\1MSGID: %s %08lx\r", nodeStr(&message->srcNode), uniqueID());
  insertLine(message->text, tempStr);

  // FMPT kludge

  if (message->srcNode.point != 0)
  {
    sprintf(tempStr, "\1FMPT %u\r", message->srcNode.point);
    insertLine(message->text, tempStr);
  }

  // TOPT kludge

  if (message->destNode.point != 0)
  {
    sprintf(tempStr, "\1TOPT %u\r", message->destNode.point);
    insertLine(message->text, tempStr);
  }

  // INTL kludge

  sprintf( tempStr, "\1INTL %u:%u/%u %u:%u/%u\r"
         , message->destNode.zone, message->destNode.net
         , message->destNode.node, message->srcNode.zone
         , message->srcNode.net,   message->srcNode.node);
  insertLine(message->text, tempStr);

  return writeMsg(message, msgType, valid);
}
//---------------------------------------------------------------------------
void validateMsg (void)
{
  s16          c, count;
  s16          doneMsg;
  tempStrType  tempStr1,
  tempStr2;
  char         *helpPtr1,
  *helpPtr2;
  s32          highMsgNum;
  struct ffblk ffblkMsg;

  for (c = PERMSG; c <= NETMSG; c++)
  {
    if (((c == NETMSG) && (globVars.netCount)) ||
        ((c == PERMSG) && (globVars.perCount)))
    {
      switch (c)
      {
        case NETMSG :
          strcpy (tempStr1, config.netPath);
          break;
        case PERMSG :
          strcpy (tempStr1, config.pmailPath);
          break;
      }

      strcpy (tempStr2, tempStr1);
      helpPtr1 = strchr (tempStr1, 0);
      helpPtr2 = strchr (tempStr2, 0);

      /* Determine highest message */

      highMsgNum = 0;
      strcpy (helpPtr1, "*.msg");
      doneMsg = findfirst (tempStr1, &ffblkMsg, FA_RDONLY|FA_HIDDEN|FA_SYSTEM|
                           /*FA_LABEL|*/FA_DIREC);

      while (!doneMsg)
      {
        highMsgNum = max (highMsgNum, atol (ffblkMsg.ff_name));
        doneMsg = findnext (&ffblkMsg);
      }

      /* Try to rename file */

      strcpy (helpPtr1, "*."MBEXTB);
      doneMsg = findfirst (tempStr1, &ffblkMsg, 0);

      while (!doneMsg)
      {
        strcpy (helpPtr1, ffblkMsg.ff_name);

        count = 0;
        do
        {
          highMsgNum += count <  10 ? 1 : 10;
          sprintf (helpPtr2, "%lu.msg", highMsgNum);
        }
        while ((count++ < 20) && (rename (tempStr1, tempStr2)));

        doneMsg = findnext (&ffblkMsg);
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
  tempStrType tempStr;
  char       *helpPtr;

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

  sprintf(tempStr, "Created file attach netmail from %s to %s", nodeStr(srcNd), nodeStr(destNd));
  logEntry(tempStr, LOG_OUTBOUND, 0);

  return 0;
}
//---------------------------------------------------------------------------
