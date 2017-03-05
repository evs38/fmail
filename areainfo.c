//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2016  Wilfred van Velzen
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

#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmail.h"

#include "areainfo.h"

#include "cfgfile.h"
#include "crc.h"
#include "log.h"
#include "minmax.h"
#include "msgmsg.h"
#include "nodeinfo.h"
#include "time.h"
#include "utils.h"
#include "version.h"

//---------------------------------------------------------------------------
struct orgLineListType
{
  struct orgLineListType *next;
  char   originLine[ORGLINE_LEN];
};

static struct orgLineListType *orgLineListPtr = NULL;

u16 echoCount;
u16 forwNodeCount = 0;

cookedEchoType   *echoAreaList;
echoToNodePtrType echoToNode[MAX_AREAS];
nodeFileType      nodeFileInfo;

extern configType       config;
extern internalMsgType *message;
extern u16              status;

//---------------------------------------------------------------------------
s16 makeNFInfo(nodeFileRecType *nfInfo, s16 srcAka, nodeNumType *destNode)
{
   s16 errorDisplay = 0;

   memset(nfInfo, 0, sizeof(nodeFileRecType));

   nfInfo->destNode     = nfInfo->destNode4d = *destNode;
   nfInfo->nodePtr      = getNodeInfo(destNode);
   nfInfo->requestedAka = srcAka;

   if (  nfInfo->nodePtr->useAka && nfInfo->nodePtr->useAka <= MAX_AKAS
      && config.akaList[nfInfo->nodePtr->useAka - 1].nodeNum.zone
      )
      nfInfo->srcAka = nfInfo->nodePtr->useAka - 1;
   else
     if ((nfInfo->srcAka = srcAka) == -1)
        nfInfo->srcAka = matchAka(&nfInfo->destNode4d, nfInfo->nodePtr->useAka);

   nfInfo->srcNode = config.akaList[nfInfo->srcAka].nodeNum;

   if (nfInfo->nodePtr->node.zone == 0)
   {
      logEntryf(LOG_ALWAYS, 0, "Warning: Node %s is not defined in the node manager", nodeStr(&nfInfo->destNode4d));
      errorDisplay++;
   }

   if (!(nfInfo->nodePtr->capability & PKT_TYPE_2PLUS))
   {
      /* Use fakenet */

      node2d (&nfInfo->srcNode);
      nfInfo->destFake = (node2d (&nfInfo->destNode) != -1);

      if ((nfInfo->srcNode.point != 0) ||
          (nfInfo->destNode.point != 0))
      {
         if (nfInfo->srcAka)
            logEntryf(LOG_ALWAYS, 0, "Warning: Fakenet not defined but required for AKA %u", nfInfo->srcAka);
         else
            logEntry("Warning: Fakenet not defined but required for the main node number", LOG_ALWAYS, 0);

         errorDisplay++;
      }
   }
   return errorDisplay;
}
//---------------------------------------------------------------------------
void initAreaInfo(void)
{
  tempStrType   tempStr;
  char               *helpPtr;
  s16           c
              , count
              , count2;
  u16           autoDisconnectCount = 0;
  u16           error = 0;
  s16           errorDisplay = 0;
  struct orgLineListType *olHelpPtr;
  headerType   *areaHeader;
  rawEchoType  *areaBuf;

  memset(nodeFileInfo, 0, sizeof(nodeFileType));

  echoCount     = 0;
  forwNodeCount = 0;

  if (!openConfig(CFG_ECHOAREAS, &areaHeader, (void**)&areaBuf))
    logEntry("Bad or missing "dARFNAME, LOG_ALWAYS, 1);

  if ((echoAreaList = (cookedEchoType*)malloc(sizeof(cookedEchoType) * areaHeader->totalRecords + 1)) == NULL)
    logEntry("Not enough memory available", LOG_ALWAYS, 2);

  memset(echoAreaList, 0, sizeof(cookedEchoType) * areaHeader->totalRecords);

  for (echoCount = 0; echoCount < areaHeader->totalRecords; echoCount++)
  {
    getRec(CFG_ECHOAREAS, echoCount);
    if (*areaBuf->areaName == 0)
       logEntry("One or more area tags are not defined. Please run FSetup.", LOG_ALWAYS, 4);

    areaBuf->areaName  [ECHONAME_LEN - 1] = 0;
    areaBuf->comment   [COMMENT_LEN  - 1] = 0;
    areaBuf->originLine[ORGLINE_LEN  - 1] = 0;

    areaBuf->address = min(areaBuf->address, MAX_AKAS);
    areaBuf->group  &= 0x03FFFFFFL;
    areaBuf->board   = min(areaBuf->board, MBBOARDS);

    if (echoCount >= MAX_AREAS)
       logEntryf(LOG_ALWAYS, 4, "More than %u areas listed in "dARFNAME, MAX_AREAS);

    if (config.akaList[areaBuf->address].nodeNum.zone == 0)
    {
      error = 1;
      logEntryf(LOG_ALWAYS, 0, "ERROR: Origin address of area %s (AKA %u) is not defined", areaBuf->areaName, areaBuf->address);
      errorDisplay++;
    }

    if (*areaBuf->comment == '^')
    {
      if ((echoAreaList[echoCount].oldAreaName = (char*)malloc(strlen(areaBuf->comment))) == NULL)
        logEntry("Not enough memory available", LOG_ALWAYS, 2);

      strupr(areaBuf->comment);
      strcpy(echoAreaList[echoCount].oldAreaName, areaBuf->comment + 1);
    }
    else
      echoAreaList[echoCount].oldAreaName = (char*)"";

    if (  (echoAreaList[echoCount].areaName = (char*)malloc(strlen(areaBuf->areaName)+1)) == NULL
       || (echoToNode[echoCount] = (echoToNodePtrType)malloc(sizeof(echoToNodeType))    ) == NULL
       )
      logEntry("Not enough memory available", LOG_ALWAYS, 2);

    memset(echoToNode[echoCount], 0, sizeof(echoToNodeType));
    strcpy(echoAreaList[echoCount].areaName, areaBuf->areaName);

    if (*areaBuf->msgBasePath)
    {
      if ((echoAreaList[echoCount].JAMdirPtr = (char*)malloc(strlen(areaBuf->msgBasePath) + 1)) == NULL)
        logEntry("Not enough memory available", LOG_ALWAYS, 2);

      strcpy(echoAreaList[echoCount].JAMdirPtr, areaBuf->msgBasePath);
    }

    echoAreaList[echoCount].writeLevel  = areaBuf->writeLevel;
    echoAreaList[echoCount].board       = areaBuf->board;
    echoAreaList[echoCount].address     = areaBuf->address;
    echoAreaList[echoCount].alsoSeenBy  = areaBuf->alsoSeenBy;
    echoAreaList[echoCount].options     = areaBuf->options;
    echoAreaList[echoCount].areaNameCRC = crc32 (areaBuf->areaName);
    echoAreaList[echoCount].options._reserved = 0;

    olHelpPtr = orgLineListPtr;

    while (olHelpPtr != NULL && strcmp(olHelpPtr->originLine, areaBuf->originLine) != 0)
      olHelpPtr = olHelpPtr->next;

    if (olHelpPtr == NULL)
    {
      if ((olHelpPtr = (struct orgLineListType *)malloc(5 + strlen(areaBuf->originLine))) == NULL)
        logEntry("Not enough memory available", LOG_ALWAYS, 2);

      strcpy(olHelpPtr->originLine, areaBuf->originLine);
      olHelpPtr->next = orgLineListPtr;
      orgLineListPtr = olHelpPtr;
    }
    echoAreaList[echoCount].originLine = olHelpPtr->originLine;

    count = 0;
    while (count < MAX_FORWARD && areaBuf->forwards[count].nodeNum.zone != 0)
    {
      if (areaBuf->forwards[count].flags.locked)
      {
        count++;
        continue;
      }

      c = 0;
      while (  c < MAX_AKAS
            && memcmp(&areaBuf->forwards[count].nodeNum, &config.akaList[c].nodeNum, sizeof(nodeNumType)) != 0
            )
        c++;

      if (c < MAX_AKAS)
      {
        logEntryf(LOG_ALWAYS, 0, "Warning: Can't forward area %s to a local AKA", areaBuf->areaName);
        errorDisplay++;
      }
      else
      {
        c = 0;
        while (  c < forwNodeCount
              && (  areaBuf->address != nodeFileInfo[c]->requestedAka
                 || memcmp(&areaBuf->forwards[count].nodeNum, &nodeFileInfo[c]->destNode4d, sizeof (nodeNumType)) != 0
                 )
              )
          c++;

        if (c >= MAX_OUTPKT)
          logEntry("Can't send mail to more than "MAX_OUTPKT_STR" nodes in one run", LOG_ALWAYS, 4);

        if (c == forwNodeCount)
        {
          if ((nodeFileInfo[forwNodeCount] = (nodeFileRecType*)malloc(sizeof(nodeFileRecType))) == NULL)
            logEntry("Not enough memory available", LOG_ALWAYS, 2);

          errorDisplay |= makeNFInfo(nodeFileInfo[forwNodeCount++], areaBuf->address, &(areaBuf->forwards[count].nodeNum));
        }
        if ( !(  areaBuf->forwards[count].flags.readOnly
              && areaBuf->forwards[count].flags.writeOnly
              )
           )
        {
          if (areaBuf->forwards[count].flags.readOnly)
            echoToNode[echoCount][ETN_INDEX(c)] |= ETN_SETRO(c);
          else
            if (areaBuf->forwards[count].flags.writeOnly)
              echoToNode[echoCount][ETN_INDEX(c)] |= ETN_SETWO(c);
            else
              echoToNode[echoCount][ETN_INDEX(c)] |= ETN_SET(c);

          echoAreaList[echoCount].echoToNodeCount++;
          if ( !areaBuf->forwards[count].flags.writeOnly
             && nodeFileInfo[c]->nodePtr->useAka
             )
            echoAreaList[echoCount].alsoSeenBy |= 1L << (nodeFileInfo[c]->nodePtr->useAka - 1);
        }
      }
      count++;
    }

    if (  config.mgrOptions.autoDiscArea
       && !areaBuf->options.disconnected
       &&  areaBuf->options.active
       && !areaBuf->options.local
       &&  areaBuf->board == 0
       && *areaBuf->msgBasePath == 0
       &&  areaBuf->forwards[0].nodeNum.zone
       && !areaBuf->forwards[1].nodeNum.zone
       )
    {
      count = 0;
      while (count < MAX_UPLREQ && memcmp(&areaBuf->forwards[0].nodeNum, &config.uplinkReq[count].node, sizeof(nodeNumType)) != 0)
        count++;

      if (count < MAX_UPLREQ)
      {
        areaBuf->options.disconnected = 1;
        strcpy(areaBuf->comment, "AutoDisconnected");
        memset(&areaBuf->forwards[0], 0, sizeof(nodeNumXType));
        putRec(CFG_ECHOAREAS, echoCount);

        echoAreaList[echoCount].options._reserved = 1;
        echoAreaList[echoCount].msgCount = count;
        autoDisconnectCount++;
      }
    }
  }

  // delete auto-disconnected areas, TEMPORARILY ALWAYS DONE !
  for (count = echoCount - 1; count >= 0; count--)
    if (echoAreaList[count].options._reserved)
      delRec(CFG_ECHOAREAS, count);

  closeConfig (CFG_ECHOAREAS);

  if (errorDisplay)
    newLine();

  if (error != 0)
    logEntry("One or more origin addresses are not defined", LOG_ALWAYS, 4);

  memset(message, 0, INTMSG_SIZE);

  if (autoDisconnectCount)
  {
    for (count = 0; count < MAX_UPLREQ; count++)
    {
      helpPtr = message->text;
      for (count2 = 0; count2 < echoCount; count2++)
        if (echoAreaList[count2].options._reserved && echoAreaList[count2].msgCount == count)
          helpPtr += sprintf (helpPtr, "-%s\r", echoAreaList[count2].areaName);

      if (helpPtr != message->text)
      {
        strcpy(message->fromUserName, config.sysopName);
        strcpy(message->toUserName,   config.uplinkReq[count].program);
        strcpy(message->subject,      config.uplinkReq[count].password);
        message->srcNode   = config.akaList[config.uplinkReq[count].originAka].nodeNum;
        message->destNode  = config.uplinkReq[count].node;
        message->attribute = PRIVATE|KILLSENT;

        strcpy(helpPtr, TearlineStr());
        writeMsgLocal(message, NETMSG, 1);
      }
    }

    strcpy(message->fromUserName, "FMail");
    strcpy(message->toUserName, config.sysopName);
    strcpy(message->subject, "Areas disconnected from uplink");
    message->destNode  = message->srcNode
                                 = config.akaList[0].nodeNum;
    message->attribute = PRIVATE;

    helpPtr = message->text;
    for (count = 0; count < echoCount; count++)
    {
      if (echoAreaList[count].options._reserved)
      {
        logEntryf(LOG_ALWAYS, 0, "Area %s has been disconnected from %s", echoAreaList[count].areaName, nodeStr(&config.uplinkReq[echoAreaList[count].msgCount].node));
        helpPtr += sprintf (helpPtr, "%s\r", tempStr);

        echoAreaList[count].options._reserved = 0;
        echoAreaList[count].msgCount = 0;
      }
    }
    strcpy(helpPtr, TearlineStr());
    writeMsgLocal(message, NETMSG, 1);

    newLine();
  }
#ifdef _DEBUG0
  for (count = 0; count < echoCount; count++)
  {
    const char *c = echoAreaList[count].JAMdirPtr != NULL ? echoAreaList[count].JAMdirPtr : "";
    logEntryf(LOG_DEBUG, 0, "DEBUG Area: %d '%s' '%s'", count, echoAreaList[count].areaName, c);
  }
#endif
}
//---------------------------------------------------------------------------
void deInitAreaInfo(void)
{
   u16      count, count2;
   struct orgLineListType *helpPtr;
   headerType    *areaHeader;
   rawEchoType   *areaBuf;

   while (orgLineListPtr != NULL)
   {
      helpPtr = orgLineListPtr;
      orgLineListPtr = orgLineListPtr->next;
      free (helpPtr);
   }
   if (!openConfig(CFG_ECHOAREAS, &areaHeader, (void**)&areaBuf))
      logEntry("Bad or missing "dARFNAME, LOG_ALWAYS, 1);

   for (count = 0; count < areaHeader->totalRecords; count++)
   {
      getRec(CFG_ECHOAREAS, count);
      for (count2 = 0; count2 < echoCount; count2++)
      {
         if ( !strcmp(echoAreaList[count2].areaName, areaBuf->areaName) )
         {
            if ( echoAreaList[count2].msgCountV )
            {
               if ( status == 1 )
                  areaBuf->lastMsgScanDat = startTime;
               if ( status == 2 )
               {
                  areaBuf->lastMsgTossDat = startTime;
                  areaBuf->stat.tossedTo = 1;
               }
               if ( status )
                  putRec(CFG_ECHOAREAS, count);
            }
            break;
         }
      }
   }
   closeConfig (CFG_ECHOAREAS);

   for (count = 0; count < echoCount; count++)
   {
      if (echoAreaList[count].JAMdirPtr != NULL)
         free(echoAreaList[count].JAMdirPtr);

      free(echoAreaList[count].areaName);
      free(echoToNode[count]);
   }
   free(echoAreaList);
   for (count = 0; count < forwNodeCount; count++)
      free(nodeFileInfo[count]);
}
//---------------------------------------------------------------------------
