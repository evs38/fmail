//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2015  Wilfred van Velzen
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
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef __MINGW32__
#include <windef.h>  // min() max()
#endif // __MINGW32__

#include "areafix.h"

#include "archive.h"
#include "areainfo.h"
#include "bclfun.h"
#include "cfgfile.h"
#include "config.h"
#include "crc.h"
#include "log.h"
#include "msgmsg.h"
#include "msgpkt.h"
#include "msgra.h"
#include "nodeinfo.h"
#include "pack.h"
#include "stpcpy.h"
#include "utils.h"
#include "version.h"

#define ADD_ALL    1
#define DELETE_ALL 2

#define MAX_DISPLAY 128

#if defined(__FMAILX__) || defined(__32BIT__)
#define MAX_AREAFIX 5124
#else
#define MAX_AREAFIX 1024
#endif

#define MAX_BADAREA 128

static const char *arcName[12] = { "none", "arc", "zip", "lzh", "pak", "zoo", "arj", "sqz", "cus", "uc2", "rar", "jar" };
#define ARCCOUNT 12

extern u16             echoCount;
extern cookedEchoType  *echoAreaList;


typedef struct
{
  char        remove;
  signed char uplink ;
  s16         maxRescan;
  const char *areaName;
} areaFixType;

typedef struct
{
  s16  index;
  char groupChar;
} areaSortType;


typedef areaFixType  areaFixListType[MAX_AREAFIX];
typedef areaSortType areaSortListType[MAX_AREAFIX];
typedef areaNameType badAreaListType[MAX_BADAREA];

badAreaListType *badAreaList;
u16             badAreaCount = 0;

extern char configPath[128];
extern configType config;

extern char *version;

//---------------------------------------------------------------------------
int getGroupCode(s32 groupCode)
{
  int count = 0;

  while ((groupCode >>= 1) != 0)
    count++;

  return count;
}
//---------------------------------------------------------------------------
static char descrStr[ECHONAME_LEN];

static s16 checkForward(const char *areaName, nodeInfoType *nodeInfoPtr)
{
  s16      found = -1;
  u16      count;
  fhandle  textHandle;
  char     fileStr[129];
  char     fileStr2[129];
  char     *helpPtr
         , *copyPtr
         , *tag
         , *descr;
  tempStrType tempStr;

  if (nodeInfoPtr->options.forwardReq)
  {
    count = 0;
    do
    {
      if (config.uplinkReq[count].originAka >= MAX_AKAS)
      {
        sprintf(tempStr, "Bad origin AKA defined for Uplink #%u", count);
        logEntry(tempStr, LOG_ALWAYS, 0);
      }
      else if (config.uplinkReq[count].node.zone &&
               ((nodeInfoPtr->groups & config.uplinkReq[count].groups) != 0))
      {
        if (config.uplinkReq[count].options.unconditional)
          found = count;
        else if (!*config.uplinkReq[count].fileName)
          continue;
        else if (config.uplinkReq[count].fileType == 2)
        {
          if (openBCL(&config.uplinkReq[count]))
          {
            while (found == -1 && readBCL(&tag, &descr))
              if (!strcmp(areaName, tag))
                found = count;

            closeBCL();
          }
        }
        else
        {
          strcpy(stpcpy(fileStr, configPath), config.uplinkReq[count].fileName);
          if ((textHandle = openP(fileStr, O_RDONLY | O_BINARY | O_DENYNONE, S_IREAD | S_IWRITE)) == -1)
          {
            sprintf(fileStr, "Uplink areas file %s not found in FMail system dir",
                    config.uplinkReq[count].fileName);
            mgrLogEntry(fileStr);
          }
          else
          {
            memset (fileStr+64, 0x0d, 64);
            memset (fileStr2+64, 0x0d, 64);
            fileStr[128] = 0;
            fileStr2[128] = 0;
            do
            {
              memcpy (fileStr, fileStr+64, 64);
              memcpy (fileStr2, fileStr+64, 64);
              memset (fileStr+64, 0, 64);
              memset (fileStr2+64, 0, 64);
              read (textHandle, fileStr+64, 64);
              memcpy (fileStr2+64, fileStr+64, 64);
              strupr (fileStr+64);
              helpPtr = fileStr;
              while ((found == -1) &&
                     ((helpPtr = strstr(helpPtr+2, areaName)) != NULL))
              {
                if ((isspace(*(helpPtr+strlen(areaName)))) &&
                    (isspace(*(--helpPtr))) &&
                    ((config.uplinkReq[count].fileType != 1) ||
                     (*helpPtr == 0x00) ||
                     (*helpPtr == 0x0a) ||
                     (*helpPtr == 0x0d)))
                {
                  found = count;
                  memcpy(fileStr, fileStr2, 128);

                  /* copy the description of the area */

                  if (config.uplinkReq[count].fileType == 1)
                  {
                    helpPtr += strlen(areaName)+1;
                    while (*helpPtr == ' ')
                    {
                      helpPtr++;
                      if (helpPtr == fileStr+128)
                      {
                        read (textHandle, fileStr, 128);
                        helpPtr = fileStr;
                      }
                    }
                    copyPtr = descrStr;
                    while ((*helpPtr >= ' ') && (copyPtr < descrStr+ECHONAME_LEN-1))
                    {
                      *copyPtr++ = *helpPtr++;
                      if (helpPtr == fileStr+128)
                      {
                        read (textHandle, fileStr, 128);
                        helpPtr = fileStr;
                      }
                    }
                    *copyPtr = 0;
                  }
                }
              }
            }
            while ((found == -1) && (!eof(textHandle)));
            close (textHandle);
          }
        }
      }
    }
    while ((++count < MAX_UPLREQ) && (found == -1));
  }
  return(found);
}
//---------------------------------------------------------------------------
int toAreaFix(const char *toName)
{
  const char *helpPtr = toName;
  int l;

  while (isspace(*helpPtr))
    helpPtr++;

  l = strlen(helpPtr);

  return (  (strnicmp(helpPtr, "AREAFIX",  7) == 0 && l <= 8)
         || (strnicmp(helpPtr, "AREAMGR",  7) == 0 && l <= 8)
         || (strnicmp(helpPtr, "AREALINK", 8) == 0 && l <= 9)
         || (strnicmp(helpPtr, "ECHOMGR",  7) == 0 && l <= 8)
         || (strnicmp(helpPtr, "FMAIL",    5) == 0 && l <= 6)
         );
}
//---------------------------------------------------------------------------
static const char *getAreaNamePtr(const char *echoName)
{
  u16         count;
  char       *helpPtr;
  tempStrType tempStr;

  strncpy(tempStr, echoName, ECHONAME_LEN - 1);
  tempStr[ECHONAME_LEN] = 0;
  if ((helpPtr = strchr(strupr(tempStr), ' ')) != NULL)
    *helpPtr = 0;

  if ((helpPtr = strchr(strupr(tempStr), ',')) != NULL)
    *helpPtr = 0;

  if (*tempStr == 0 || strchr(tempStr, '?') != NULL
     ||  strchr(tempStr, '*') != NULL)
    return NULL;

  count = 0;
  while (count < echoCount && stricmp(tempStr, echoAreaList[count].areaName) != 0)
    count++;

  if (count < echoCount)
    return echoAreaList[count].areaName;

  count = 0;
  while (count < badAreaCount && stricmp((*badAreaList)[count], tempStr) != 0)
    count++;

  if (count < badAreaCount)
    return (*badAreaList)[count];

  if (badAreaCount < MAX_BADAREA)
    return strcpy((*badAreaList)[badAreaCount++], tempStr);

  return "[unknown area]";
}
//---------------------------------------------------------------------------
// send message that doesn't contain any kludges in the text body yet
//
static void sendMsg( internalMsgType *message, char *replyStr
                   , nodeInfoType *nodeInfoPtr , s32 *msgNum1
                   , nodeInfoType *nodeInfoPtr2, s32 *msgNum2
                   )
{
  tempStrType tempStr;
  char       *helpPtr;
  u16         count;
  nodeNumType tempNode;

  setCurDateMsg(message);

  helpPtr = addTZUTCKludge(message->text);
  addPIDKludge(helpPtr);

  *msgNum1 = 0;
  *msgNum2 = 0;
  tempNode = message->srcNode;
  helpPtr  = message->text;

  for (count = 0; count < 2; count++)
  {
    if ((count == 0 && nodeInfoPtr != NULL) || (count == 1 && nodeInfoPtr2 != NULL))
    {
      if (count)
      {
        nodeInfoPtr = nodeInfoPtr2;
        strcpy(message->text, helpPtr);
      }
      if (nodeInfoPtr->node.zone)
        message->destNode = nodeInfoPtr->node;

      strcpy(message->toUserName, *nodeInfoPtr->sysopName ? nodeInfoPtr->sysopName : "SysOp");

      helpPtr = message->text;

      if (!(nodeInfoPtr->capability & PKT_TYPE_2PLUS) && isLocalPoint(&message->destNode))
      {
        node2d(&message->srcNode);
        node2d(&message->destNode);
      }

      message->attribute = LOCAL | PRIVATE;
      if (!config.mgrOptions.keepReceipt)
        message->attribute |= KILLSENT;

      helpPtr = addINTLKludge  (message, helpPtr);
      helpPtr = addPointKludges(message, helpPtr);

      // FLAGS
      switch (nodeInfoPtr->outStatus)
      {
        case 1 :
          message->attribute |= HOLDPICKUP;
          break;
        case 2 :
          message->attribute |= CRASH;
          break;
        case 3 :
          message->attribute |= HOLDPICKUP;
          helpPtr = insertLine (helpPtr, "\1FLAGS DIR\r");
          break;
        case 4 :
          message->attribute |= CRASH;
        case 5 :
          helpPtr = insertLine (helpPtr, "\1FLAGS DIR\r");
          break;
      }

      helpPtr = addMSGIDKludge(message, helpPtr);

      if (*replyStr != 0)
        helpPtr = addKludge(helpPtr, "REPLY:", replyStr);

      if (count)
        *msgNum2 = writeMsg(message, NETMSG, 1);
      else
        *msgNum1 = writeMsg(message, NETMSG, 1);

      message->srcNode = tempNode;
    }
  }
}
//---------------------------------------------------------------------------
int areaFix(internalMsgType *message)
{
  nodeInfoType   *nodeInfoPtr;
  nodeInfoType   *remMaintPtr;
  char           *helpPtr;
  const char     *helpPtr2;
  s16             allType;
  u16             removeArea, updateArea;
  s16             bytesRead;
  u16             c, count, count2;
  s16             temp;
  s32             msgNum1, msgNum2;
  rawEchoType     rawEchoInfo2;
  u16             areaCount;
  int             areaFixCount = 0;
  u16             activeAreasCount = 0;
  areaFixListType *areaFixList;
  areaFixType     areaFixRec;
  u8              archiver;
  s16             compare;
  fhandle         helpHandle;
  fhandle         msgHandle1, msgHandle2;
  tempStrType     replyStr;
  u16             availCount;
  s16             bufCount;
  s16             replyHelp = 0;
  s16             replyList = 0;
  s16             replyBCL = 0;
  s16             replyQuery = 0;
  s16             replyUnlinked = 0;
  s16             activePassive = 0;
  s16             passwordChange = 0;
  s16             autoBCLchange = 0;
  s16             pktPasswordChange = 0;
  s16             notifyChange = 0;
  s16             rescanRequest = 0;
  s16             rescanAll = 0; /*int*/
  nodeNumType     tempNode;
  tempStrType     tempStr;
  u16             areaSortListIndex;
  int             oldGroupCode;
  u32             tempGroups;
  areaSortListType *areaSortList;
  headerType	   *areaHeader
               , *adefHeader;
  rawEchoType	   *areaBuf
               , *adefBuf;
  udef            komma;
  char           *tag
               , *descr;

  if ((areaSortList = (areaSortListType*)malloc(sizeof(areaSortListType))) == NULL )
  {
    mgrLogEntry("Not enough memory available for AreaFix");
    return -1;
  }

#if defined(__FMAILX__) || defined(__32BIT__)
  if ((areaFixList = (areaFixListType*)malloc(sizeof(areaFixListType))) == NULL )
  {
    mgrLogEntry("Not enough memory available for AreaFix");
    free(areaSortList);
    return -1;
  }
  if ((badAreaList = (badAreaListType*)malloc(sizeof(badAreaListType))) == NULL )
  {
    mgrLogEntry("Not enough memory available for AreaFix");
    free(areaSortList);
    free(areaFixList);
    return -1;
  }
#else
  areaFixList = (areaFixListType*)(message->text + TEXT_SIZE - sizeof(areaFixListType));
  badAreaList = (badAreaListType*)(message->text + TEXT_SIZE - sizeof(areaFixListType) - sizeof(badAreaListType));
#endif

  if (!openConfig(CFG_ECHOAREAS, &areaHeader, (void**)&areaBuf))
  {
    free(areaSortList);
#if defined(__FMAILX__) || defined(__32BIT__)
    free(areaFixList);
    free(badAreaList);
#endif
    mgrLogEntry("AreaMgr: Can't open file "dARFNAME" for input");
    return -1;
  }

  remMaintPtr = NULL;
  memset(replyStr, 0, sizeof(tempStrType));

  if ((helpPtr = findCLStr(message->text, "\x1MSGID: ")) != NULL)
  {
    memcpy(replyStr, helpPtr + 8, sizeof(tempStrType) - 1);
    if ((helpPtr = strchr(replyStr, 0x0d)) != NULL)
      *helpPtr = 0;
  }

  nodeInfoPtr = getNodeInfo(&message->srcNode);

  strcpy(message->fromUserName, "FMail AreaMgr");

  tempNode          = message->srcNode;
  message->srcNode  = message->destNode;
  message->destNode = nodeInfoPtr->node;

  if (nodeInfoPtr->node.zone == 0)
  {
    message->destNode = tempNode;
    sprintf(tempStr, "Unauthorized AreaMgr request from node %s", nodeStr(&tempNode));
    mgrLogEntry(tempStr);

    sprintf(message->text, "%s is not authorized to make AreaMgr requests.\r"
                           "Please contact %s.\r\r", nodeStr(&tempNode), config.sysopName);
    helpPtr = strchr(message->text, 0);
    strcpy(message->subject, "AreaMgr error report");
  }
  else
  {
    message->cost = 0;

    helpPtr = strtok(message->subject, " ");

    if (  helpPtr == NULL
       || strlen(nodeInfoPtr->password) == 0
       || stricmp(helpPtr, nodeInfoPtr->password) != 0 )
    {
      sprintf(tempStr, "Bad AreaMgr request from %s", nodeStr(&nodeInfoPtr->node));
      mgrLogEntry(tempStr);

      helpPtr = message->text + sprintf(message->text, "Your AreaMgr request contained a wrong password: '%s'\r", message->subject);
      strcpy(message->subject, "AreaMgr error report");
    }
    else
    {
      strupr(message->text);
      helpPtr = message->text;
      if ((helpPtr = findCLStr(++helpPtr, "%FROM ")) != NULL)
      {
        memset (&tempNode, 0, sizeof(nodeNumType));
        if ((temp = sscanf(helpPtr+6, "%hu:%hu/%hu.%hu", &tempNode.zone, &tempNode.net, &tempNode.node, &tempNode.point)) < 3)
        {
          strcpy(message->text, "Bad FROM node used");
          mgrLogEntry(message->text);
          strcat(message->text, "\r");
          strcpy(message->subject, "AreaMgr error report");
          goto Send;
        }
        remMaintPtr = nodeInfoPtr;
        nodeInfoPtr = getNodeInfo(&tempNode);
        if (!remMaintPtr->options.remMaint)
        {
          sprintf(message->text, "Illegal attempt to perform remote maintenance by %s", nodeStr(&remMaintPtr->node));
          mgrLogEntry(message->text);
          strcat(message->text, "\r");
          strcpy(message->subject, "AreaMgr error report");
          nodeInfoPtr = NULL;
          goto Send;
        }
        sprintf(tempStr, "Accepted AreaMgr request from %s", nodeStr(&remMaintPtr->node));
        mgrLogEntry(tempStr);
        newLine();
        if (nodeInfoPtr->node.zone == 0)
        {
          sprintf(message->text, "Invalid AreaMgr request: %s is not listed in Node Manager", nodeStr(&tempNode));
          mgrLogEntry(message->text);
          strcat(message->text, "\r");
          strcpy(message->subject, "AreaMgr error report");
          nodeInfoPtr = NULL;
          goto Send;
        }
        sprintf(tempStr, "- doing remote maintenance for %s", nodeStr(&nodeInfoPtr->node));
        mgrLogEntry(tempStr);
      }
      else
      {
        sprintf(tempStr, "Accepted AreaMgr request from %s", nodeStr(&nodeInfoPtr->node));
        mgrLogEntry(tempStr);
        newLine();
      }

      archiver = nodeInfoPtr->archiver;

      if ((helpPtr = strtok(NULL, " ")) != NULL && (*helpPtr == '-' || *helpPtr == '/'))
      {
        helpPtr++;
        count = 0;
        while (count < ARCCOUNT && strnicmp(arcName[count], helpPtr, 3) != 0)  // Only first 3 chars are significant
          count++;

        if (count < ARCCOUNT)
          archiver = count - 1;
        else
        {
          if (*(helpPtr+1) == 0)
          {
            switch (toupper(*helpPtr))
            {
              case 'H' :
                replyHelp = 1;
                break;
              case 'L' :
                replyList = 1;
                break;
              case 'Q' :
                replyQuery = 1;
                break;
              case 'U' :
                replyUnlinked = 1;
            }
          }
        }
      }

      memset(areaFixList, 0, sizeof(areaFixListType));
      for (count = 0; count < MAX_AREAFIX; ++count)
        (*areaFixList)[count].maxRescan = -1;

      helpPtr = strtok(message->text, "\x0a\x0d\x8d");

      allType = 0;
      while (  helpPtr != NULL
            && strncmp (helpPtr, "---"  , 3) != 0
            && strncmp (helpPtr, "..."  , 3) != 0
            && strnicmp(helpPtr, "%NOTE", 5) != 0
            )
      {
        while (*helpPtr == ' ')
          helpPtr++;  // Skip spaces

        if ((*helpPtr != 0)   && (*helpPtr != 1) &&
            (*helpPtr != ';') && (*helpPtr != '>'))
          // Skip kludge lines and comment lines
        {
          if (*helpPtr == '%')  // % command handling
          {
            helpPtr++;
            if (  config.mgrOptions.allowCompr
               && (!strnicmp(helpPtr, "COMPRESS ", 9) || !strnicmp(helpPtr, "COMPRESSION ", 12))
               )
            {
              if ( *(helpPtr+8) == 'I' )
                helpPtr += 3;
              helpPtr += 9;

              count = 0;
              while (count < ARCCOUNT && strnicmp(arcName[count], helpPtr, 3) != 0)  // Only care about first 3 chars of archiver name
                count++;

              if (count < ARCCOUNT)
                archiver = count - 1;
            }
            else if (config.mgrOptions.allowPassword &&
                     (!strnicmp(helpPtr, "PWD ", 4) ||
                      !strnicmp(helpPtr, "PASSWORD ", 9)) )
            {
              if ( *(helpPtr + 3) == 'S' )
                helpPtr += 5;

              strncpy (tempStr, helpPtr+4, 16);
              count = 0;
              while (count < 16 && isalnum(tempStr[count]))
                count++;
              tempStr[count] = 0;
              strupr(tempStr);
              if (strlen(tempStr) <= 4)
                passwordChange = 1;
              else
              {
                strcpy(nodeInfoPtr->password, tempStr);
                passwordChange = 2;
              }
            }
            else if (config.mgrOptions.allowPktPwd && strnicmp(helpPtr, "PKTPWD ", 7) == 0)
            {
              strncpy (tempStr, helpPtr+7, 8);
              count = 0;
              while ((count < 8) && isalnum(tempStr[count])) count++;
              tempStr[count] = 0;
              strupr(tempStr);
              strcpy (nodeInfoPtr->packetPwd, tempStr);
              pktPasswordChange = 2;
            }
            else if (config.mgrOptions.allowBCL &&
                     strnicmp(helpPtr, "AUTOBCL ", 8) == 0)
            {
              nodeInfoPtr->autoBCL = atoi(helpPtr + 8);
              autoBCLchange = 1;
            }
            else if ((strnicmp(helpPtr, "HELP", 4) == 0) && !isalnum(*(helpPtr+4)))
            {
              replyHelp = 1;
            }
            else if ( (strnicmp(helpPtr, "LIST", 4) == 0) &&
                      (*(helpPtr+4) == ',' || *(helpPtr+4) == ' ' || !isalnum(*(helpPtr+4))) )
            {
              helpPtr += 4;
              while (*helpPtr == ' ')
                helpPtr++;
              if ( (komma = (*helpPtr == ',')) != 0 )
                helpPtr++;
              while (*helpPtr == ' ') helpPtr++;

              if ( komma && toupper(*helpPtr) == 'B' )
                replyBCL = 1;
              else
                replyList = 1;
            }
            else if ((strnicmp(helpPtr, "BLIST", 5) == 0) && !isalnum(*(helpPtr+5)))
            {
              replyBCL = 1;
            }
            else if ((strnicmp(helpPtr, "QUERY", 5) == 0) && !isalnum(*(helpPtr+5)))
            {
              replyQuery = 1;
            }
            else if ((strnicmp(helpPtr, "UNLINKED", 8) == 0) && !isalnum(*(helpPtr+8)))
            {
              replyUnlinked = 1;
            }
            else if ((strnicmp(helpPtr, "RESCAN", 6) == 0) && !isalnum(*(helpPtr+6)))
            {
              rescanRequest++;
              if ( *(helpPtr+6) == '=' || *(helpPtr+6) == ':' )
              {
                rescanAll = atoi(helpPtr+7);
              }
              else
              {
                rescanAll = config.defMaxRescan ?
                            config.defMaxRescan : 0x7fff;
              }
            }
            else if (config.mgrOptions.allowActive &&
                     ((strnicmp(helpPtr, "ACTIVE", 6) == 0) ||
                      (strnicmp(helpPtr, "RESUME", 6) == 0)) &&
                     !isalnum(*(helpPtr+6)))
            {
              activePassive = 1;
            }
            else if (config.mgrOptions.allowActive &&
                     ((strnicmp(helpPtr, "PASSIVE", 7) == 0) ||
                      (strnicmp(helpPtr, "PAUSE", 5) == 0)) &&
                     !isalnum(*(helpPtr+8)))
            {
              activePassive = 2;
            }
            else if (config.mgrOptions.allowNotify &&
                     (strnicmp(helpPtr, "NOTIFY ", 7) == 0))
            {
              if ((strnicmp (helpPtr+7, "ON", 2) == 0) && !isalnum(*(helpPtr+9)))
                notifyChange = 1;
              else if ((strnicmp (helpPtr+7, "OFF", 3) == 0) && !isalnum(*(helpPtr+10)))
                notifyChange = 2;
            }
            else if (config.mgrOptions.allowAddAll &&
                     (((strnicmp(helpPtr, "ALL", 3) == 0) && !isalnum(*(helpPtr+3))) ||
                      ((strnicmp(helpPtr, "+ALL", 4) == 0) && !isalnum(*(helpPtr+4)))))
            {
              if (*helpPtr == '+')
                helpPtr++;
              helpPtr += 3;
              while (*helpPtr == ' ')
                helpPtr++;
              if ( (komma = (*helpPtr == ',')) != 0 )
                helpPtr++;
              while (*helpPtr == ' ')
                helpPtr++;

              if ( strnicmp (helpPtr, "/R", 2) == 0 ||
                   (komma && toupper(*helpPtr) == 'R') )
              {
                if ( !komma )
                  ++helpPtr;

                rescanRequest++;

                ++helpPtr;
                if ( *helpPtr == '=' || *helpPtr == ':' )
                  rescanAll = atoi(helpPtr+1);
                else
                  rescanAll = config.defMaxRescan ?
                              config.defMaxRescan : 0x7fff;
              }
              areaFixCount = 0;
              allType = ADD_ALL;
            }
            else if ((strnicmp(helpPtr, "-ALL", 4) == 0) && !isalnum(*(helpPtr+4)))
            {
              areaFixCount = 0;
              allType = DELETE_ALL;
            }
            else if (strnicmp(helpPtr, "FROM", 4) != 0)
            {
              sprintf (tempStr, "Bad command: \"%.10s\"", helpPtr);
              mgrLogEntry (tempStr);
            }
          }
          else
          {
            if ((*helpPtr == '+') || (*helpPtr == '-') || (*helpPtr == '='))
            {
              removeArea = (*(helpPtr) == '-');
              updateArea = (*(helpPtr++) == '=');
            }
            else
              removeArea = updateArea = 0;
            if ( updateArea )
              rescanRequest++;

            if ((helpPtr2 = getAreaNamePtr(helpPtr)) != NULL)
            {
              compare = 1;
              count = 0;
              while ((count < areaFixCount) &&
                     ((compare = stricmp(helpPtr2, (*areaFixList)[count].areaName)) > 0))
              {
                count++;
              }
              if ((compare == 0) || (areaFixCount < MAX_AREAFIX))
              {
                if (compare != 0)
                {
                  memmove (&(*areaFixList)[count + 1],
                           &(*areaFixList)[count],
                           ((areaFixCount++) - count) * sizeof(areaFixType));
                  (*areaFixList)[count].areaName = helpPtr2;
                }
                (*areaFixList)[count].uplink = -1;

                if ( updateArea )
                  (*areaFixList)[count].remove = 8;
                else
                  (*areaFixList)[count].remove = removeArea;

                if ( !removeArea )
                {
                  helpPtr += strlen(helpPtr2);

                  while (*helpPtr == ' ') helpPtr++;
                  if ( (komma = (*helpPtr == ',')) != 0 )
                    helpPtr++;
                  while (*helpPtr == ' ') helpPtr++;

                  if ( strnicmp (helpPtr, "/R", 2) == 0 ||
                       (komma && toupper(*helpPtr) == 'R') )
                  {
                    if ( !komma )
                      ++helpPtr;

                    rescanRequest++;

                    ++helpPtr;
                    if ( *helpPtr == '=' || *helpPtr == ':' )
                      (*areaFixList)[count].maxRescan = atoi(++helpPtr);
                  }
                }
              }
            }
          }
        }
        helpPtr = strtok(NULL, "\x0a\x0d\x8d");
      }

      strcpy(message->subject, "FMail AreaMgr status report");

      helpPtr = message->text+
                sprintf (message->text, "FMail AreaMgr status report for %s on %s\r",
                         nodeStr(&nodeInfoPtr->node),
                         nodeStr(&message->srcNode));

      if (remMaintPtr != NULL)
      {
        helpPtr = helpPtr + sprintf (helpPtr,
                                     "Remote maintenance performed by %s\r",
                                     nodeStr(&remMaintPtr->node));
      }

      for (areaCount = 0; areaCount<areaHeader->totalRecords; areaCount++)
      {
        getRec(CFG_ECHOAREAS, areaCount);
        areaBuf->areaName[ECHONAME_LEN-1] = 0;
        areaBuf->comment[COMMENT_LEN-1] = 0;
        areaBuf->originLine[ORGLINE_LEN-1] = 0;

        if ((areaBuf->options.active) &&
            (!areaBuf->options.local) &&
            (areaBuf->options.allowAreafix) &&
            (areaBuf->group & nodeInfoPtr->groups) &&
            (areaFixCount < MAX_AREAFIX) && allType)
        {
          compare = 1;
          c = 0;
          while ((c < areaFixCount) &&
                 ((compare = stricmp (areaBuf->areaName,
                                      (*areaFixList)[c].areaName)) > 0))
          {
            c++;
          }
          if (compare != 0)
          {
            memmove (&(*areaFixList)[c+1],
                     &(*areaFixList)[c],
                     (areaFixCount - c) * sizeof(areaFixType));
            (*areaFixList)[c].areaName  = getAreaNamePtr(areaBuf->areaName);
            (*areaFixList)[c].uplink    = -1;
            (*areaFixList)[c].maxRescan = -1;
            (*areaFixList)[c].remove    = allType-1;
            areaFixCount++;
          }
        }
        c = 0;
        while ((c < MAX_AREAFIX) && (c < areaCount) &&
               (getGroupCode(areaBuf->group) >= (*areaSortList)[c].groupChar))
        {
          c++;
        }
        if (c < MAX_AREAFIX)
        {
          memmove(&(*areaSortList)[c+1], &(*areaSortList)[c], sizeof(areaSortType)*(min(areaCount, MAX_AREAFIX) - c));
          (*areaSortList)[c].index = areaCount;
          (*areaSortList)[c].groupChar = getGroupCode(areaBuf->group);
        }
      }

      areaSortListIndex = 0;
      bufCount = MAX_DISPLAY;

      while ((areaSortListIndex < min(areaCount,MAX_AREAFIX)) &&
             getRec(CFG_ECHOAREAS,(*areaSortList)[areaSortListIndex++].index))
      {
        areaBuf->areaName[ECHONAME_LEN-1] = 0;
        areaBuf->comment[COMMENT_LEN-1] = 0;
        areaBuf->originLine[ORGLINE_LEN-1] = 0;

        count = 0;
        while (count < areaFixCount && strcmp(areaBuf->areaName, (*areaFixList)[count].areaName) != 0)
          count++;

        if (count < areaFixCount && (*areaFixList)[count].remove != 8 )
        {
          if ((areaBuf->options.active) &&
              (!areaBuf->options.local) &&
              (areaBuf->options.allowAreafix) /* &&
                   (!areaBuf->options.disconnected)*/)
          {
            /* found */
            if ( (*areaFixList)[count].remove )
            {
              c = 0;
              while ((c < MAX_FORWARD) &&
                     (memcmp (&(areaBuf->forwards[c].nodeNum),
                              &nodeInfoPtr->node,
                              sizeof(nodeNumType)) != 0))
              {
                c++;
              }
              if (c < MAX_FORWARD)
              {
                if ( areaBuf->forwards[c].flags.readOnly ||
                     areaBuf->forwards[c].flags.writeOnly )
                {
                  (*areaFixList)[count].remove = 7; // new in 1.41
                }
                else
                {
                  memcpy (&(areaBuf->forwards[c]),
                          &(areaBuf->forwards[c+1]),
                          (MAX_FORWARD-1 - c ) * sizeof(nodeNumXType));
                  memset (&(areaBuf->forwards[MAX_FORWARD-1]), 0,
                          sizeof(nodeNumXType));
                  (*areaFixList)[count].remove = 4;
                }
              }
              else
                (*areaFixList)[count].remove =
                  (areaBuf->group & nodeInfoPtr->groups) ? 5 : 6;
            }
            else
            {
              c = 0;
              while ((c < MAX_FORWARD) &&
                     (areaBuf->forwards[c].nodeNum.zone != 0) &&
                     ((nodeInfoPtr->node.zone > areaBuf->forwards[c].nodeNum.zone) ||
                      ((nodeInfoPtr->node.zone == areaBuf->forwards[c].nodeNum.zone) &&
                       (nodeInfoPtr->node.net > areaBuf->forwards[c].nodeNum.net)) ||
                      ((nodeInfoPtr->node.zone == areaBuf->forwards[c].nodeNum.zone) &&
                       (nodeInfoPtr->node.net == areaBuf->forwards[c].nodeNum.net) &&
                       (nodeInfoPtr->node.node > areaBuf->forwards[c].nodeNum.node)) ||
                      ((nodeInfoPtr->node.zone == areaBuf->forwards[c].nodeNum.zone) &&
                       (nodeInfoPtr->node.net == areaBuf->forwards[c].nodeNum.net) &&
                       (nodeInfoPtr->node.node == areaBuf->forwards[c].nodeNum.node) &&
                       (nodeInfoPtr->node.point > areaBuf->forwards[c].nodeNum.point))))
              {
                c++;
              }
              if ((c < MAX_FORWARD) &&
                  (memcmp (&nodeInfoPtr->node,
                           &(areaBuf->forwards[c].nodeNum),
                           sizeof(nodeNumType)) != 0))
              {
                if (areaBuf->group & nodeInfoPtr->groups)
                {
                  memmove (&(areaBuf->forwards[c+1]),
                           &(areaBuf->forwards[c]),
                           (MAX_FORWARD-1 - c) * sizeof(nodeNumXType));
                  memset(&(areaBuf->forwards[c]), 0, sizeof(nodeNumXType));
                  areaBuf->forwards[c].nodeNum = nodeInfoPtr->node;
                  (*areaFixList)[count].remove = 2;
                }
                else
                  (*areaFixList)[count].remove = 6;
              }
              else
                (*areaFixList)[count].remove = 3;
            }
            if (((*areaFixList)[count].remove == 2) ||
                ((*areaFixList)[count].remove == 4))
            {
              putRec(CFG_ECHOAREAS,(*areaSortList)[areaSortListIndex-1].index);
            }
          }
          else
          {
            (*areaFixList)[count].remove =
              areaBuf->options.allowAreafix ? 6 : 7;
          }
        }
        if (areaBuf->options.active && !areaBuf->options.local)
        {
          c = 0;
          while ((c < MAX_FORWARD) &&
                 (memcmp (&(areaBuf->forwards[c].nodeNum), &nodeInfoPtr->node,
                          sizeof(nodeNumType)) != 0))
          {
            c++;
          }
          if (c < MAX_FORWARD)
          {
            activeAreasCount++;

            if ( (*areaFixList)[count].remove == 8
               && ( !nodeInfoPtr->options.allowRescan
                  || areaBuf->options.noRescan
                  || areaBuf->forwards[c].flags.writeOnly
                  )
               )
              (*areaFixList)[count].remove = 9;
          }
        }
      }

      if (areaFixCount != 0)
        helpPtr += sprintf (helpPtr, "\rResult of requested mutations:\r");

      for (count = 0; count < areaFixCount; count++)
      {
        if (bufCount-- <= 0)
        {
          sprintf(helpPtr, "\r*** Listing is continued in the next message ***\r");
          sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);
          sprintf(message->subject, "FMail AreaMgr confirmation report for %s (continued)", nodeStr(&nodeInfoPtr->node));
          helpPtr = message->text + sprintf(message->text, "*** Following list is a continuation of the previous message ***\r");
          *(helpPtr++) = '\r';
          bufCount = MAX_DISPLAY;
        }
        helpPtr2 = helpPtr;
        sprintf(helpPtr, "- %.40s .......................................", (*areaFixList)[count].areaName);

        switch ((*areaFixList)[count].remove)
        {
          case 2 :
            helpPtr += sprintf(helpPtr + 43, " added") + 43;
            break;
          case 3 :
            helpPtr += sprintf(helpPtr + 43, " already active") + 43;
            break;
          case 4 :
            helpPtr += sprintf(helpPtr + 43, " removed") + 43;
            break;
          case 5 :
            helpPtr += sprintf(helpPtr + 43, " not active") + 43;
            break;
          case 1 :
          case 6 :
            helpPtr += sprintf(helpPtr + 43, " not found") + 43;
            break;
          case 7 :
            helpPtr += sprintf(helpPtr + 43, " not allowed") + 43;
            break;
          case 8 :
            helpPtr += sprintf(helpPtr + 43, " rescan requested") + 43;
            break;
          case 9 :
            helpPtr += sprintf(helpPtr + 43, " rescan not allowed") + 43;
            break;
          default:
            if (((*areaFixList)[count].uplink = checkForward((*areaFixList)[count].areaName, nodeInfoPtr)) != -1)
              helpPtr += sprintf(helpPtr + 43, " requested") + 43;
            else
              helpPtr += sprintf(helpPtr + 43, " not available from uplink") + 43;
            break;
        }
        mgrLogEntry (helpPtr2);
        *(helpPtr++) = '\r';
      }
      if (areaFixCount)
        newLine();

      if (activeAreasCount)
        helpPtr += sprintf(helpPtr, "\rYou are connected to %u area%s.\r", activeAreasCount, activeAreasCount != 1 ? "s" : "");
      else
        helpPtr += sprintf(helpPtr, "\rYou are not connected to any areas.\r");

      if (activePassive)
      {
        nodeInfoPtr->options.active = 2-activePassive;
        if ( activePassive == 1 )
          nodeInfoPtr->referenceLNBDat = 0;
      }

      if (notifyChange)
        nodeInfoPtr->options.notify = 2-notifyChange;

#ifdef __32BIT__
      if ((archiver ==  0 && *config.arc32.programName == 0) ||
          (archiver ==  1 && *config.zip32.programName == 0) ||
          (archiver ==  2 && *config.lzh32.programName == 0) ||
          (archiver ==  3 && *config.pak32.programName == 0) ||
          (archiver ==  4 && *config.zoo32.programName == 0) ||
          (archiver ==  5 && *config.arj32.programName == 0) ||
          (archiver ==  6 && *config.sqz32.programName == 0) ||
          (archiver ==  8 && *config.uc232.programName == 0) ||
          (archiver ==  9 && *config.rar32.programName == 0) ||
          (archiver == 10 && *config.jar32.programName == 0))
#else
      if ((archiver ==  0 && *config.arc.programName == 0) ||
          (archiver ==  1 && *config.zip.programName == 0) ||
          (archiver ==  2 && *config.lzh.programName == 0) ||
          (archiver ==  3 && *config.pak.programName == 0) ||
          (archiver ==  4 && *config.zoo.programName == 0) ||
          (archiver ==  5 && *config.arj.programName == 0) ||
          (archiver ==  6 && *config.sqz.programName == 0) ||
          (archiver ==  8 && *config.uc2.programName == 0) ||
          (archiver ==  9 && *config.rar.programName == 0) ||
          (archiver == 10 && *config.jar.programName == 0))
#endif
      {
        archiver = 0xFF;
      }
      else
      {
        if (archiver != nodeInfoPtr->archiver)
        {
          nodeInfoPtr->archiver = archiver;
          archiver = 1;
        }
        else
          archiver = 0;
      }

      if (config.mgrOptions.allowActive)
      {
        *(helpPtr++) = '\r';
        helpPtr2 = helpPtr;
        helpPtr += sprintf( helpPtr, "Node status : %s%s", nodeInfoPtr->options.active ? "ACTIVE" : "PASSIVE"
                          , activePassive ? " (new setting)" : "" );
        mgrLogEntry (helpPtr2);
      }

      if (config.mgrOptions.allowNotify)
      {
        *(helpPtr++) = '\r';
        helpPtr2 = helpPtr;
        helpPtr += sprintf( helpPtr, "Notify      : %s%s", nodeInfoPtr->options.notify ? "ON" : "OFF"
                          , notifyChange ? " (new setting)" : "" );
        mgrLogEntry (helpPtr2);
      }
      switch (nodeInfoPtr->archiver)
#ifdef __32BIT__
      {
        case 0:
          strcpy(tempStr, config.arc32.programName);
          break;
        case 1:
          strcpy(tempStr, config.zip32.programName);
          break;
        case 2:
          strcpy(tempStr, config.lzh32.programName);
          break;
        case 3:
          strcpy(tempStr, config.pak32.programName);
          break;
        case 4:
          strcpy(tempStr, config.zoo32.programName);
          break;
        case 5:
          strcpy(tempStr, config.arj32.programName);
          break;
        case 6:
          strcpy(tempStr, config.sqz32.programName);
          break;
        case 7:
          strcpy(tempStr, config.customArc32.programName);
          break;
        case 8:
          strcpy(tempStr, config.uc232.programName);
          break;
        case 9:
          strcpy(tempStr, config.rar32.programName);
          break;
        case 10:
          strcpy(tempStr, config.jar32.programName);
          break;
#else
      {
        case 0:
          strcpy(tempStr, config.arc.programName);
          break;
        case 1:
          strcpy(tempStr, config.zip.programName);
          break;
        case 2:
          strcpy(tempStr, config.lzh.programName);
          break;
        case 3:
          strcpy(tempStr, config.pak.programName);
          break;
        case 4:
          strcpy(tempStr, config.zoo.programName);
          break;
        case 5:
          strcpy(tempStr, config.arj.programName);
          break;
        case 6:
          strcpy(tempStr, config.sqz.programName);
          break;
        case 7:
          strcpy(tempStr, config.customArc.programName);
          break;
        case 8:
          strcpy(tempStr, config.uc2.programName);
          break;
        case 9:
          strcpy(tempStr, config.rar.programName);
          break;
        case 10:
          strcpy(tempStr, config.jar.programName);
          break;
#endif
        case 0xFF:
          strcpy(tempStr, "None");
          break;
        default:
          strcpy(tempStr, "UNKNOWN");
          break;
      }
      if ( (helpPtr2 = strchr(tempStr, ' ')) != NULL )
      {
        while ( helpPtr2 > tempStr && *(helpPtr2-1) != '\\' )
          --helpPtr2;
        strcpy(tempStr, helpPtr2);
      }
      else if ( (helpPtr2 = strrchr(tempStr, '\\')) != NULL )
        strcpy(tempStr, helpPtr2);
      *(helpPtr++) = '\r';
      helpPtr2 = helpPtr;
      helpPtr += sprintf(helpPtr, "Compression : %s%s", tempStr, archiver == 1 ? " (new setting)" : "");
      mgrLogEntry (helpPtr2);
      helpPtr += sprintf (helpPtr, "\r- available : NONE");
#ifdef __32BIT__
      if (*config.arc32.programName != 0)
        helpPtr += sprintf (helpPtr, " ARC");
      if (*config.zip32.programName != 0)
        helpPtr += sprintf (helpPtr, " ZIP");
      if (*config.lzh32.programName != 0)
        helpPtr += sprintf (helpPtr, " LZH");
      if (*config.pak32.programName != 0)
        helpPtr += sprintf (helpPtr, " PAK");
      if (*config.zoo32.programName != 0)
        helpPtr += sprintf (helpPtr, " ZOO");
      if (*config.arj32.programName != 0)
        helpPtr += sprintf (helpPtr, " ARJ");
      if (*config.sqz32.programName != 0)
        helpPtr += sprintf (helpPtr, " SQZ");
      if (*config.uc232.programName != 0)
        helpPtr += sprintf (helpPtr, " UC2");
      if (*config.rar32.programName != 0)
        helpPtr += sprintf (helpPtr, " RAR");
      if (*config.jar32.programName != 0)
        helpPtr += sprintf (helpPtr, " JAR");
#else
      if (*config.arc.programName != 0)
        helpPtr += sprintf (helpPtr, " ARC");
      if (*config.zip.programName != 0)
        helpPtr += sprintf (helpPtr, " ZIP");
      if (*config.lzh.programName != 0)
        helpPtr += sprintf (helpPtr, " LZH");
      if (*config.pak.programName != 0)
        helpPtr += sprintf (helpPtr, " PAK");
      if (*config.zoo.programName != 0)
        helpPtr += sprintf (helpPtr, " ZOO");
      if (*config.arj.programName != 0)
        helpPtr += sprintf (helpPtr, " ARJ");
      if (*config.sqz.programName != 0)
        helpPtr += sprintf (helpPtr, " SQZ");
      if (*config.uc2.programName != 0)
        helpPtr += sprintf (helpPtr, " UC2");
      if (*config.rar.programName != 0)
        helpPtr += sprintf (helpPtr, " RAR");
      if (*config.jar.programName != 0)
        helpPtr += sprintf (helpPtr, " JAR");
#endif
      *(helpPtr++) = '\r';

      if (passwordChange)
      {
        helpPtr2 = helpPtr;
        if (passwordChange == 1)
          helpPtr += sprintf (helpPtr, "AreaMgr pwd : new password ignored (should be at least 5 characters long)");
        else
          helpPtr += sprintf (helpPtr, "AreaMgr pwd : new password accepted");
        mgrLogEntry (helpPtr2);
        *((helpPtr)++) = '\r';
      }
      if (pktPasswordChange)
      {
        helpPtr2 = helpPtr;
        helpPtr += sprintf (helpPtr, "Packet pwd  : new password accepted");
        mgrLogEntry (helpPtr2);
        *((helpPtr)++) = '\r';
      }
      if ( autoBCLchange )
      {
        helpPtr2 = helpPtr;
        if ( nodeInfoPtr->autoBCL )
          helpPtr += sprintf (helpPtr, "Auto BCL    : %u days", nodeInfoPtr->autoBCL);
        else
          helpPtr += sprintf (helpPtr, "Auto BCL    : OFF");
        mgrLogEntry (helpPtr2);
        *((helpPtr)++) = '\r';
      }
    }
    helpPtr = stpcpy(helpPtr, "\rUse %HELP in a message to FMail for more information.\r\r");
  }
Send:
  strcpy(stpcpy(tempStr, configPath), "areamgr.txt");

  ++no_msg;
  if ((helpHandle = openP(tempStr, O_RDONLY | O_BINARY, 0)) != -1)
  {
    bytesRead = read(helpHandle, helpPtr, 0x7FFF);
    close(helpHandle);
    if (bytesRead >= 0)
      *(helpPtr + bytesRead) = 0;
  }
  sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);

  // Send uplink requests

  for (count = 0; count < MAX_UPLREQ; count++)
  {
    if (config.uplinkReq[count].node.zone)
    {
      areaCount = 0;
      helpPtr = message->text;

      for (c = 0; c < areaFixCount; c++)
      {
        if ((*areaFixList)[c].uplink == count)
        {
          helpPtr += sprintf (helpPtr, "%s%s\r",
                              config.uplinkReq[count].options.addPlusPrefix?"+":"",
                              (*areaFixList)[c].areaName);

          memset(&rawEchoInfo2, 0, RAWECHO_SIZE);

          if (openConfig(CFG_AREADEF, &adefHeader, (void**)&adefBuf))
          {
            if (getRec(CFG_AREADEF, 0))
              memcpy(&rawEchoInfo2, adefBuf, RAWECHO_SIZE);

            closeConfig(CFG_AREADEF);
          }
          strcpy(rawEchoInfo2.areaName, (*areaFixList)[c].areaName);

          *descrStr = 0;
          if (checkForward((*areaFixList)[c].areaName, nodeInfoPtr) != -1 &&
              *descrStr)
            strcpy (rawEchoInfo2.comment, descrStr);
          else
            sprintf (rawEchoInfo2.comment, "Requested from %s",
                     nodeStr(&config.uplinkReq[count].node));
          rawEchoInfo2.address = config.uplinkReq[count].originAka;
          rawEchoInfo2.options.active = 1;
          rawEchoInfo2.options.allowAreafix = 1;

          if ( !rawEchoInfo2.board )
            *rawEchoInfo2.msgBasePath = 0;
          else if ( rawEchoInfo2.board == 1 )
          {
            u16  xu;
            char boardtest[MBBOARDS+1];

            memset(boardtest, 0, sizeof(boardtest));
            for ( xu = 0; xu < echoCount; xu++ )
              if ( echoAreaList[xu].board &&
                   echoAreaList[xu].board <= MBBOARDS )
                boardtest[echoAreaList[xu].board] = 1;
            count = 0;
            while ( ++xu <= MBBOARDS )
              if ( !boardtest[xu] )
                break;
            if ( xu <= MBBOARDS )
              rawEchoInfo2.board = xu;
            else
              rawEchoInfo2.board = 0;
            *rawEchoInfo2.msgBasePath = 0;
          }
          else if ( rawEchoInfo2.board == 2 )
          {
            rawEchoInfo2.board = 0;
            strcpy(rawEchoInfo2.msgBasePath, makeName(rawEchoInfo2.msgBasePath, rawEchoInfo2.areaName));
          }

// originele code (hieronder) weer teruggezet 30-1-96
//             if (!rawEchoInfo2.group)
          rawEchoInfo2.group = 1;

          tempGroups = 1;
          while (tempGroups &&
                 !(tempGroups & (config.uplinkReq[count].groups & nodeInfoPtr->groups)))
          {
            tempGroups <<= 1;
          }
          if (tempGroups)
            rawEchoInfo2.group = tempGroups;

          if (( config.uplinkReq[count].node.zone  > nodeInfoPtr->node.zone) ||
              ((config.uplinkReq[count].node.zone == nodeInfoPtr->node.zone) &&
               (config.uplinkReq[count].node.net   > nodeInfoPtr->node.net)) ||
              ((config.uplinkReq[count].node.zone == nodeInfoPtr->node.zone) &&
               (config.uplinkReq[count].node.net  == nodeInfoPtr->node.net)  &&
               (config.uplinkReq[count].node.node  > nodeInfoPtr->node.node))||
              ((config.uplinkReq[count].node.zone == nodeInfoPtr->node.zone) &&
               (config.uplinkReq[count].node.net  == nodeInfoPtr->node.net)  &&
               (config.uplinkReq[count].node.node == nodeInfoPtr->node.node) &&
               (config.uplinkReq[count].node.point > nodeInfoPtr->node.point)))
          {
            rawEchoInfo2.forwards[0].nodeNum = nodeInfoPtr->node;
            rawEchoInfo2.forwards[1].nodeNum = config.uplinkReq[count].node;
          }
          else
          {
            rawEchoInfo2.forwards[0].nodeNum = config.uplinkReq[count].node;
            rawEchoInfo2.forwards[1].nodeNum = nodeInfoPtr->node;
          }

          temp = 0;
          while (getRec (CFG_ECHOAREAS, temp) &&
                 (strcmp(areaBuf->areaName, rawEchoInfo2.areaName) < 0))
          {
            temp++;
          }
          memcpy (areaBuf, &rawEchoInfo2, RAWECHO_SIZE);
          insRec(CFG_ECHOAREAS, temp);
          for (count2 = 0; count2 < min(MAX_AREAFIX,areaHeader->totalRecords); count2++)
          {
            if ((*areaSortList)[count2].index >= temp)
            {
              (*areaSortList)[count2].index++;
            }
          }
          (*areaSortList)[areaHeader->totalRecords-1].index = -1;
          (*areaSortList)[areaHeader->totalRecords-1].groupChar = 127;
          areaCount++;
        }
      }
      if (areaCount)
      {
        strcpy (helpPtr, "---\r");
        strcpy (message->fromUserName, config.sysopName);
        strcpy (message->toUserName,   config.uplinkReq[count].program);
        strcpy (message->subject,      config.uplinkReq[count].password);
        message->srcNode  = config.akaList[config.uplinkReq[count].originAka].nodeNum;
        message->destNode = config.uplinkReq[count].node;
        message->attribute= PRIVATE|KILLSENT;

        writeMsgLocal (message, NETMSG, 1);

        helpPtr = message->text + sprintf(message->text,
                                          "The following area(s) have been requested from %s for %s:\r\r",
                                          nodeStr(&config.uplinkReq[count].node),
                                          nodeStr(&nodeInfoPtr->node));
        for (c = 0; c < areaFixCount; c++)
        {
          if ((*areaFixList)[c].uplink == count)
            helpPtr += sprintf (helpPtr, "%s\r", (*areaFixList)[c].areaName);
        }
        strcpy (message->fromUserName, "FMail AreaMgr");
        strcpy (message->toUserName, config.sysopName);
        strcpy (message->subject,    "Area(s) requested from uplink");
        message->destNode  = config.akaList[config.uplinkReq[count].originAka].nodeNum;
        message->attribute = PRIVATE;

        writeMsgLocal (message, NETMSG, 1);
      }
    }
  }

  if (rescanRequest)
  {
    sprintf(tempStr, "%s%lu.msg", config.netPath, msgNum1);
    msgHandle1 = openP(tempStr, O_WRONLY|O_BINARY|O_APPEND|O_DENYNONE, 0);
    sprintf(tempStr, "%s%lu.msg", config.netPath, msgNum2);
    msgHandle2 = openP(tempStr, O_WRONLY|O_BINARY|O_APPEND|O_DENYNONE, 0);

    strcpy(stpcpy (tempStr, config.bbsPath), "areamgr.$$$");
    if (nodeInfoPtr->options.allowRescan)
    {
      if ((helpHandle = openP(tempStr, O_RDWR|O_BINARY|O_CREAT|O_TRUNC|O_DENYNONE, S_IREAD|S_IWRITE)) != -1)
      {
        if (  write(helpHandle, areaFixList, areaFixCount * sizeof(areaFixType))
           == (int)(areaFixCount * sizeof(areaFixType))
           )
        {
          lseek (helpHandle, 0, SEEK_SET);
          while (read(helpHandle, &areaFixRec, sizeof(areaFixType)) > 0)
          {
            if ((areaFixRec.remove == 2 || areaFixRec.remove == 8) && areaFixRec.maxRescan)
            {
              rescan(nodeInfoPtr, areaFixRec.areaName,
                      areaFixRec.maxRescan != -1 ? areaFixRec.maxRescan :
                      (rescanAll ? rescanAll :
                       config.defMaxRescan ? config.defMaxRescan : 0x7FFF),
                      msgHandle1, msgHandle2);
            }
          }
        }
        close(helpHandle);
      }
      unlink(tempStr);
    }
    else
    {
      unlink(tempStr);
      sprintf(tempStr, "Node %s is not allowed to rescan the message base.", nodeStr(&nodeInfoPtr->node));
      mgrLogEntry(tempStr);
      strcat(tempStr, "\r");
      write(msgHandle1, tempStr, strlen(tempStr));
      write(msgHandle2, tempStr, strlen(tempStr));
    }
    close(msgHandle1);
    close(msgHandle2);
  }

  strcpy(message->fromUserName, "FMail AreaMgr");

  if (replyHelp)
  {
    strcpy(stpcpy(tempStr, configPath), "areamgr.hlp");

    if ((helpHandle = openP(tempStr, O_RDONLY | O_BINARY, 0)) != -1)
    {
      bytesRead = read(helpHandle, message->text, DEF_TEXT_SIZE - 0x1000);  // Er moeten nog wat kludges aan kunnen worden toegevoegd
      close (helpHandle);
      if (bytesRead > 0)
        *(message->text + bytesRead) = 0;
      else
        *message->text = 0;
      removeLf(message->text);  // Voor het geval het een linux line terminated file is
      strcpy(message->subject, "FMail AreaMgr information");
      sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);
    }
  }

  if (replyList)
  {
    char semaFlag = 0;

    bufCount = MAX_DISPLAY;
    availCount = 0;
    oldGroupCode = -1;
    strcpy(message->subject, "Available areas");
    helpPtr = message->text + sprintf(message->text, "Available areas for %s:\r", nodeStr(&nodeInfoPtr->node));

    for (count = 0; count<areaHeader->totalRecords && (*areaSortList)[count].index != -1; count++)
    {
      getRec(CFG_ECHOAREAS,(*areaSortList)[count].index);

      areaBuf->areaName[ECHONAME_LEN-1] = 0;
      areaBuf->comment[COMMENT_LEN-1] = 0;
      areaBuf->originLine[ORGLINE_LEN-1] = 0;

      c = 0;
      while (c < MAX_FORWARD && memcmp (&(areaBuf->forwards[c].nodeNum), &nodeInfoPtr->node, sizeof(nodeNumType)) != 0)
        c++;

      if ((areaBuf->options.active) &&
          (!areaBuf->options.local) &&
          ((areaBuf->group & nodeInfoPtr->groups) || (c < MAX_FORWARD)))
      {
        if ((strlen(areaBuf->areaName) >= ECHONAME_LEN_OLD) && *areaBuf->comment)
          bufCount--;

        if (bufCount-- <= 0)
        {
          sprintf(helpPtr, "\r*** Listing is continued in the next message ***\r");
          sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);
          sprintf(message->subject, "Available areas (continued)");
          helpPtr = message->text + sprintf(message->text, "*** Following list is a continuation of the previous message ***\r");
          *(helpPtr++) = '\r';
          bufCount = MAX_DISPLAY;
          oldGroupCode = -1;
        }
        if (oldGroupCode != getGroupCode(areaBuf->group))
        {
          oldGroupCode = getGroupCode(areaBuf->group);
          if (*config.groupDescr[oldGroupCode])
            helpPtr += sprintf(helpPtr, "\r%s\r\r", config.groupDescr[oldGroupCode]);
          else
            helpPtr += sprintf(helpPtr, "\rGroup %c\r\r", 'A' + oldGroupCode);
        }

        if (c < MAX_FORWARD && !areaBuf->forwards[c].flags.locked)
        {
          if (areaBuf->forwards[c].flags.readOnly)
          {
            c = 'R';
            semaFlag |= BIT3;
          }
          if (areaBuf->forwards[c].flags.writeOnly)
          {
            c = 'W';
            semaFlag |= BIT4;
          }
          else if (areaBuf->options.allowAreafix)
          {
            c = '*';
            semaFlag |= BIT0;
          }
          else
          {
            c = '+';
            semaFlag |= BIT1;
          }
        }
        else
        {
          if (areaBuf->options.allowAreafix)
            c = ' ';
          else
          {
            c = '-';
            semaFlag |= BIT2;
          }
        }

        helpPtr += sprintf(helpPtr, ((strlen(areaBuf->areaName) >= ECHONAME_LEN_OLD) && *areaBuf->comment)
                          ? "%c %s\r                            %s\r" : "%c %-24s  %s\r"
                          , c, areaBuf->areaName, areaBuf->comment );
        availCount++;
      }
    }
    if (availCount == 0)
      strcpy(helpPtr, "\r=== There are no areas available to you ===\r");
    else
      sprintf( helpPtr, "\r%s%s%s%s%sYou are connected to %u of %u available area%s.\r"
             , semaFlag&BIT0 ? "* indicates that you are connected to this area.\r" : ""
             , semaFlag&BIT1 ? "+ indicates that you are connected, but cannot disconnect this area yourself.\r" : ""
             , semaFlag&BIT2 ? "- indicates that you are not connected and cannot connect this area yourself.\r" : ""
             , semaFlag&BIT3 ? "R indicates that you are read-only connected and cannot disconnect this area yourself.\r" : ""
             , semaFlag&BIT4 ? "W indicates that you are write-only connected and cannot connect this area yourself.\r" : ""
             , activeAreasCount, availCount
             , availCount != 1 ? "s" : ""
             );

    sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);

    // --- Available from Uplink ---
    if (config.mgrOptions.sendUplArList)
    {
      availCount = 0;
      bufCount = MAX_DISPLAY;
      strcpy(message->subject, "Areas available from uplinks");
      helpPtr = message->text + sprintf(message->text, "Available areas from uplinks for %s:\r\r", nodeStr(&nodeInfoPtr->node));
      for (count = 0; count < MAX_UPLREQ; count++)
      {
        if (nodeInfoPtr->groups & config.uplinkReq[count].groups)
        {
          if (!openBCL(&config.uplinkReq[count]))
            continue;

          while (readBCL(&tag, &descr))
          {
            if (bufCount-- <= 0)
            {
              sprintf(helpPtr, "\r*** Listing is continued in the next message ***\r\r");
              sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);
              sprintf(message->subject, "Available areas from uplinks (continued)");
              helpPtr = message->text + sprintf(message->text, "*** Following list is a continuation of the previous message ***\r");
              *(helpPtr++) = '\r';
              bufCount = MAX_DISPLAY;
            }
            count2 = 0;
            while (count2 < echoCount && strcmp(tag, echoAreaList[count2].areaName))
              ++count2;
            if (count2 == echoCount)
            {
              helpPtr += sprintf( helpPtr, ((strlen(tag) >= ECHONAME_LEN_OLD) && *descr)
                                ? "  %s\r                            %s\r" : "  %-24s  %s\r"
                                , tag, descr);
              availCount++;
            }
          }
          closeBCL();
        }
      }
      if (availCount)
        sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);
    }
  }

  if (replyQuery)
  {
    bufCount = MAX_DISPLAY;
    availCount = 0;
    sprintf(message->subject, "Active areas for %s", nodeStr(&nodeInfoPtr->node));
    helpPtr = message->text + sprintf(message->text, "You are active for the following areas:\r\r");

    for (count = 0; count < areaHeader->totalRecords; count++)
    {
      getRec(CFG_ECHOAREAS,count);
      areaBuf->areaName[ECHONAME_LEN-1] = 0;
      areaBuf->comment[COMMENT_LEN-1] = 0;
      areaBuf->originLine[ORGLINE_LEN-1] = 0;

      if (areaBuf->options.active && !areaBuf->options.local)
      {
        c = 0;
        while (c < MAX_FORWARD && memcmp (&(areaBuf->forwards[c].nodeNum), &nodeInfoPtr->node, sizeof(nodeNumType)) != 0)
          c++;

        if (c < MAX_FORWARD && !areaBuf->forwards[c].flags.locked)
        {
          if (strlen(areaBuf->areaName) >= ECHONAME_LEN_OLD && *areaBuf->comment)
            bufCount--;

          if (bufCount-- <= 0)
          {
            sprintf(helpPtr, "\r*** Listing is continued in the next message ***\r");
            sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);
            sprintf(message->subject, "Active areas for %s (continued)", nodeStr(&nodeInfoPtr->node));
            helpPtr = message->text + sprintf (message->text, "*** Following list is a continuation of the previous message ***\r");
            *(helpPtr++) = '\r';
            bufCount = MAX_DISPLAY;
          }
          helpPtr += sprintf( helpPtr, ((strlen(areaBuf->areaName) >= ECHONAME_LEN_OLD) && *areaBuf->comment)
                            ? "%s\r                          %s\r" : "%-24s  %s\r"
                            , areaBuf->areaName, areaBuf->comment );
          if ( areaBuf->forwards[c].flags.readOnly )
            helpPtr += sprintf(helpPtr, "                          The area is ReadOnly\r");
          if ( areaBuf->forwards[c].flags.writeOnly )
            helpPtr += sprintf(helpPtr, "                          The area is WriteOnly\r");

          availCount++;
        }
      }
    }
    if (!availCount)
      strcpy(helpPtr, "\r--- You are not active for any areas ---\r");

    sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);
  }

  if (replyUnlinked)
  {
    bufCount = MAX_DISPLAY;
    availCount = 0;
    sprintf(message->subject, "Unlinked areas for %s", nodeStr(&nodeInfoPtr->node));
    helpPtr = message->text + sprintf(message->text, "You are not active for the following areas:\r\r");

    for (count=0; count<areaHeader->totalRecords; count++)
    {
      getRec(CFG_ECHOAREAS,count);
      areaBuf->areaName[ECHONAME_LEN-1] = 0;
      areaBuf->comment[COMMENT_LEN-1] = 0;
      areaBuf->originLine[ORGLINE_LEN-1] = 0;

      if ((areaBuf->options.active) &&
          (!areaBuf->options.local) &&
          (areaBuf->options.allowAreafix) &&
          (areaBuf->group & nodeInfoPtr->groups))
      {
        c = 0;
        while (c < MAX_FORWARD && memcmp(&(areaBuf->forwards[c].nodeNum), &nodeInfoPtr->node, sizeof(nodeNumType)) != 0)
          c++;

        if (c == MAX_FORWARD)
        {
          if ((strlen(areaBuf->areaName) >= ECHONAME_LEN_OLD) && *areaBuf->comment)
            bufCount--;

          if (bufCount-- <= 0)
          {
            sprintf(helpPtr, "\r*** Listing is continued in the next message ***\r");
            sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);
            sprintf(message->subject, "Unlinked areas for %s (continued)", nodeStr(&nodeInfoPtr->node));
            helpPtr = message->text + sprintf (message->text, "*** Following list is a continuation of the previous message ***\r");
            *(helpPtr++) = '\r';
            bufCount = MAX_DISPLAY;
          }
          helpPtr += sprintf( helpPtr, ((strlen(areaBuf->areaName) >= ECHONAME_LEN_OLD) && *areaBuf->comment)
                            ? "%s\r                          %s\r" : "%-24s  %s\r"
                            , areaBuf->areaName, areaBuf->comment );
          availCount++;
        }
      }
    }
    if (!availCount)
      strcpy(helpPtr, "\r--- You are active for all areas ---\r");

    sendMsg(message, replyStr, nodeInfoPtr, &msgNum1, remMaintPtr, &msgNum2);
  }
  closeConfig(CFG_ECHOAREAS);

  // Binary Conference Listing
  if (replyBCL)
    send_bcl(&message->srcNode, &message->destNode, nodeInfoPtr);

  free(areaSortList);
#if defined(__FMAILX__) || defined(__32BIT__)
  free(areaFixList);
  free(badAreaList);
#endif
  return 0;
}
//---------------------------------------------------------------------------
