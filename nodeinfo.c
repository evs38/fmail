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

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "fmail.h"

#include "nodeinfo.h"

#include "areainfo.h"
#include "cfgfile.h"
#include "config.h"
#include "log.h"
#include "minmax.h"
#include "msgmsg.h"
#include "utils.h"
#include "version.h"

//---------------------------------------------------------------------------
nodeInfoType *nodeInfo[MAX_NODES];
nodeInfoType defaultNodeInfo;

u16 nodeCount;

extern configType config;

//---------------------------------------------------------------------------
void initNodeInfo(void)
{
  u16           count;
  headerType   *nodeHeader;
  nodeInfoType *nodeBuf;

  if (!openConfig(CFG_NODES, &nodeHeader, (void**)&nodeBuf))
    logEntry("Bad or missing "dNODFNAME, LOG_ALWAYS, 2);

  nodeCount = min(MAX_NODES, (u16)(nodeHeader->totalRecords));

  for (count = 0; count < nodeCount; count++)
  {
    if ((nodeInfo[count] = (nodeInfoType*)malloc(sizeof(nodeInfoType))) == NULL)
      logEntry("Not enough memory available", LOG_ALWAYS, 2);

    getRec(CFG_NODES, count);
    memcpy(nodeInfo[count], nodeBuf, sizeof(nodeInfoType));
    nodeInfo[count]->password[16] = 0;
    nodeInfo[count]->packetPwd[8] = 0;
    nodeInfo[count]->sysopName[35] = 0;
  }
  for (; count < MAX_NODES; count++)
    nodeInfo[count] = NULL;

  closeConfig(CFG_NODES);

  memset(&defaultNodeInfo, 0, sizeof(nodeInfoType));
  defaultNodeInfo.options.active = 1;
  defaultNodeInfo.capability     = PKT_TYPE_2PLUS;
  defaultNodeInfo.archiver       = config.defaultArc;
}
//---------------------------------------------------------------------------
nodeInfoType *getNodeInfo(nodeNumType *nodeNum)
{
  u16 count = 0;

  if (nodeNum->zone == 0)
    while (count < nodeCount && memcmp(&nodeNum->net, &(nodeInfo[count]->node.net), sizeof(nodeNumType) - 2) != 0)
      count++;
  else
    while (count < nodeCount && memcmp(nodeNum, &(nodeInfo[count]->node), sizeof(nodeNumType)) != 0)
      count++;

  if (count < nodeCount)
    return nodeInfo[count];

  return &defaultNodeInfo;
}
//---------------------------------------------------------------------------
extern u32 totalBundleSize[MAX_NODES];

void closeNodeInfo(void)
{
  headerType   *nodeHeader;
  nodeInfoType *nodeBuf;
  u16           count
              , count2;

  // verwerk bundle datum via-node in node zelf
  for (count = 0; count < nodeCount; count++)
    for (count2 = 0; count2 < nodeCount; count2++)
      if (memcmp(&(nodeInfo[count]->node), &(nodeInfo[count2]->viaNode), sizeof(nodeNumType)) == 0)
      {
        nodeInfo[count2]->lastNewBundleDat = nodeInfo[count]->lastNewBundleDat;
        nodeInfo[count2]->referenceLNBDat  = nodeInfo[count]->referenceLNBDat;
      }

  if (openConfig(CFG_NODES, &nodeHeader, (void**)&nodeBuf))
  {
    strcpy(message->fromUserName, "FMail AutoPassive");
    strcpy(message->subject     , "Status of system changed to Passive");
    message->srcNode   = config.akaList[0].nodeNum;
    message->attribute = PRIVATE;

    chgNumRec(CFG_NODES, nodeCount);
    for (count = 0; count < nodeCount; count++)
    {
      if (nodeInfo[count] == NULL)  // Fail safe, shouldn't happen.
        continue;

      memcpy(nodeBuf, nodeInfo[count], sizeof(nodeInfoType));
      if (nodeBuf->options.active && nodeBuf->lastNewBundleDat == nodeBuf->referenceLNBDat)
      {
        if (nodeBuf->passiveSize && totalBundleSize[count] / 1024ul > nodeBuf->passiveSize * 100ul)
        {
          message->destNode = config.akaList[0].nodeNum;
          strcpy(message->toUserName, config.sysopName);
          sprintf(message->text, "The status of system %s (SysOp %s) has been changed to Passive because the maximum total bundle size of %u kb was exceed. The current total bundle size is %u kb.\r\r%s"
                , nodeStr(&nodeBuf->node), nodeBuf->sysopName, (u32)nodeBuf->passiveSize * 100, (u32)totalBundleSize[count] / 1024
                , TearlineStr()
                );
          writeMsgLocal(message, NETMSG, 1);
          sprintf(message->text, "The status of your system %s has been changed to Passive because the maximum total bundle size of %u kb was exceed. The current total bundle size is %u kb.\r\r"
                                 "You can reactivate your system by sending a message containing %%ACTIVE to FMail.\r\r%s"
                , nodeStr(&nodeBuf->node), (u32)nodeBuf->passiveSize * 100, (u32)totalBundleSize[count] / 1024
                , TearlineStr()
                );
          message->destNode = nodeBuf->node;
          strcpy(message->toUserName, nodeBuf->sysopName);
          writeMsgLocal (message, NETMSG, 1);
          nodeBuf->options.active = 0;
        }
        else
          if (  nodeBuf->passiveDays && nodeBuf->referenceLNBDat
             && startTime - nodeBuf->referenceLNBDat >= (u32)nodeBuf->passiveDays * 86400L)
          {
            message->destNode = config.akaList[0].nodeNum;
            strcpy(message->toUserName, config.sysopName);
            sprintf(message->text, "The status of system %s (SysOp %s) has been changed to Passive because no mail was received from, or send to that system for at least %u days.\r\r%s"
                   , nodeStr(&nodeBuf->node), nodeBuf->sysopName, nodeBuf->passiveDays
                   , TearlineStr()
                   );
            writeMsgLocal (message, NETMSG, 1);
            sprintf(message->text, "The status of your system %s has been changed to Passive because you didn't receive or send any mail for at least %u days.\r\r"
                                   "You can reactivate your system by sending a message containing %%ACTIVE to FMail.\r\r%s"
                   , nodeStr(&nodeBuf->node), nodeBuf->passiveDays
                   , TearlineStr()
                   );
            message->destNode = nodeBuf->node;
            strcpy(message->toUserName, nodeBuf->sysopName);
            writeMsgLocal(message, NETMSG, 1);
            nodeBuf->options.active = 0;
          }
      }
      putRec(CFG_NODES, count);
    }
    closeConfig(CFG_NODES);
  }
  // Free used memory
  for (count = 0; count < nodeCount; count++)
    freeNull(nodeInfo[count]);
}
//---------------------------------------------------------------------------
