//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2016  Wilfred van Velzen
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
#include <dirent.h>    // opendir()
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "fmail.h"

#include "pack.h"

#include "areainfo.h"
#include "log.h"
#include "msgmsg.h"
#include "msgpkt.h"
#include "nodeinfo.h"
#include "spec.h"
#include "utils.h"


#define MAXNETREC  1024

#define DEST_NONE     0
#define DEST_VIA      1
#define DEST_HOST     2

//---------------------------------------------------------------------------
typedef struct
{
  u32           msgNum;
  nodeNumType   destNode;
  nodeNumType   viaNode;
  u16           attribute;
  u16           flags;
} netRecType;

typedef netRecType netListType[MAXNETREC];

extern configType       config;
extern internalMsgType *message;
extern globVarsType     globVars;
extern char             configPath[128];

static s16              errorDisplay=0;
static u16              netIndex;
static u32              msgNum;
static netListType     *netList;

//---------------------------------------------------------------------------
s16 packValid(nodeNumType *node, char *packedNodes)
{
  tempStrType tempStr;
  char       *helpPtr
           , *helpPtr2;
  char        stringNode[32];
  char        nodeTempStr[32];
  size_t      count;

  if (packedNodes == NULL)
    return 0;

  strcpy(tempStr, packedNodes);
  helpPtr = strtok(tempStr, " ");

  *nodeTempStr = 0;

  while (helpPtr != NULL)
  {
    if (*(u16*)helpPtr == '*' || strchr(helpPtr, ':') != NULL)
      strcpy(nodeTempStr, helpPtr);
    else
    {
      if (strchr(helpPtr, '/') != NULL)
      {
        if ((helpPtr2 = strchr(nodeTempStr, ':')) != NULL)
          strcpy(helpPtr2 + 1, helpPtr);
        else
          logEntry("Bad entry in Pack Manager", LOG_ALWAYS, 4);
      }
      else
      {
        if (*helpPtr != '.')
        {
          if ((helpPtr2 = strchr(nodeTempStr, '/')) != NULL)
            strcpy(helpPtr2 + 1, helpPtr);
          else
            logEntry("Bad entry in Pack Manager", LOG_ALWAYS, 4);
        }
        else
        {
          if ((helpPtr2 = strchr(nodeTempStr, '.')) != NULL)
            strcpy(helpPtr2, helpPtr);
          else
            strcat(nodeTempStr, helpPtr);
        }
      }
    }
    strcpy(stringNode, nodeStr(node));

    for (count = 0; count < strlen(nodeTempStr); count++)
    {
      if (nodeTempStr[count] == '?' && isdigit(stringNode[count]))
        stringNode[count] = '?';

      if (nodeTempStr[count] == '*')
      {
        if (nodeTempStr[count + 1] != 0)
          logEntry ("Asterisk only allowed as last character of node number", LOG_ALWAYS, 4);

        nodeTempStr[count] = 0;
        stringNode[count] = 0;
        if (count && nodeTempStr[count - 1] == '.' && stringNode[count - 1] == 0)
          nodeTempStr[count - 1] = 0;

        break;
      }
    }
    if (strcmp(stringNode, nodeTempStr) == 0)
      return 1;

    if ((helpPtr = strchr(stringNode, '.')) != NULL)
    {
      *helpPtr = 0;
      if (strcmp(stringNode, nodeTempStr) == 0)
        return 1;
    }
    helpPtr = strtok(NULL, " ");
  }
  return 0;
}
//---------------------------------------------------------------------------
void packRoute(char *packedNodes, char *exceptNodes, char destType, nodeNumType *destNode, s32 switches)
{
  u16         count;
  u16         oldAttribute;
  nodeNumType currLineDestNode;

  for (count = 0; count < netIndex; count++)
  {
    if ((*netList)[count].viaNode.zone)
      continue;

    switch (destType)
    {
      case DEST_NONE :
        memcpy (&currLineDestNode,
                &(*netList)[count].destNode,
                sizeof(nodeNumType));
        break;
      case DEST_VIA  :
        memcpy (&currLineDestNode, destNode,
                sizeof(nodeNumType));
        break;
      case DEST_HOST :
        memcpy (&currLineDestNode,
                &(*netList)[count].destNode,
                sizeof(nodeNumType));
        currLineDestNode.node  = 0;
        currLineDestNode.point = 0;
        break;
    }
    oldAttribute = (*netList)[count].attribute;

    if ((((destType == DEST_VIA) &&
          ((memcmp(&currLineDestNode, &(*netList)[count].destNode, sizeof(nodeNumType))==0) ||
           ((destNode->point == 0) &&
            (memcmp(&currLineDestNode, &(*netList)[count].destNode, 6)==0)))) || /* points to boss */
         ((packValid(&(*netList)[count].destNode, packedNodes)) &&
          (!packValid(&(*netList)[count].destNode, exceptNodes)))) &&
        (oldAttribute & (IN_TRANSIT | LOCAL)) &&
        ((oldAttribute & IN_TRANSIT)   || !(switches & SW_I)) &&
        ((oldAttribute & LOCAL)        || !(switches & SW_L)) &&
        ((!(oldAttribute & CRASH))      || (switches & SW_C)) &&
        ((!(oldAttribute & HOLDPICKUP)) || (switches & SW_H)) &&
        ((!(oldAttribute & ORPHAN))     || (switches & SW_O)) &&
        (!(oldAttribute & (SENT | FILE_ATT | FILE_REQ | FILE_UPD_REQ))) &&
        ((*netList)[count].flags == 0))
      memcpy(&(*netList)[count].viaNode, &currLineDestNode, sizeof(nodeNumType));
  }
}
//---------------------------------------------------------------------------
static void processPackLine(char *line, s32 switches)
{
  u16         count;
  char        destType;
  nodeNumType destNode;
  tempStrType tempStr;
  tempStrType lineBuf;
  char        *helpPtr;
  char        newWildCard;

  newWildCard = (strchr(line, '#') != NULL);
  for (count = 0; count <= (newWildCard ? 9 : 0); count++)
  {
    strcpy(lineBuf, line);
    if (newWildCard)
    {
      helpPtr = lineBuf;
      while ((helpPtr = strchr(helpPtr, '#')) != NULL)
        *(helpPtr++) = '0' + count;
    }
    destType = DEST_NONE;

    if ((helpPtr = strstr(lineBuf, " /")) != NULL)
    {
      *helpPtr = 0;
      helpPtr = strtok(helpPtr + 1, " ");
      while (helpPtr != NULL)
      {
        helpPtr[1] = toupper(helpPtr[1]);
        if (  helpPtr[0] != '/'
           || (helpPtr[1] != 'I' && helpPtr[1] != 'L' && helpPtr[1] != 'C' && helpPtr[1] != 'H' && helpPtr[1] != 'O')
           || helpPtr[2] != 0
           )
        {
          sprintf(tempStr, "Bad switch in Pack Manager: %s", helpPtr);
          logEntry(tempStr, LOG_ALWAYS, 0);
        }
        else
          switches |= 1L << (helpPtr[1] - 'A');

        helpPtr = strtok(NULL, " ");
      }
    }

    memset(&destNode, 0, sizeof(nodeNumType));
    if ((helpPtr = strstr(strupr(lineBuf), " VIA ")) != NULL)
    {
      *helpPtr = 0;
      if (stricmp(helpPtr + 5, "HOST") == 0)
        destType = DEST_HOST;
      else
      {
        if (((helpPtr = strtok(helpPtr + 5, " ")) == NULL) ||
            ((sscanf(helpPtr, "%hu:%hu/%hu.%hu",
                     &destNode.zone, &destNode.net, &destNode.node, &destNode.point) < 3) ||
             !((strcmp(helpPtr, nodeStr(&destNode)) == 0) ||
               (strcmp(helpPtr, nodeStrZ(&destNode)) == 0))))
          logEntry ("Bad VIA node", LOG_ALWAYS, 4);

        destType = DEST_VIA;
      }
    }

    if ((helpPtr = strstr(strupr(lineBuf), " EXCEPT ")) != NULL)
    {
      *helpPtr = 0;
      helpPtr += 8;
    }
    else
      helpPtr = strchr(lineBuf, 0);

    packRoute(lineBuf, helpPtr, destType, &destNode, switches);
  }
}
//---------------------------------------------------------------------------
s16 pack(s16 argc, char *argv[], s32 switches)
{
  s16             doneMsg;
  u16             low
                , mid
                , high;
  s16             index
                , count
                , count2;
  tempStrType     tempStr;
  char           *helpPtr;
  DIR            *dir;
  struct dirent  *ent;
  fhandle         fileHandle;
  packType       *pack;
  nodeFileRecType nodeFileInfo;
  u16             oldAttribute;
  nodeNumType     tempNode;
  s16             diskError = 0;

  if (((netList = malloc(sizeof(netListType))) == NULL) ||
      ((pack = malloc(sizeof(packType))) == NULL))
    logEntry("Not enough memory to pack netmail messages", LOG_ALWAYS, 2);

  memset(netList, 0, sizeof(netListType));
  memset(pack   , 0, sizeof(packType   ));

  strcpy(stpcpy(tempStr, configPath), dPCKFNAME);

  if ((fileHandle = openP(tempStr, O_RDONLY | O_BINARY | O_DENYNONE, S_IREAD | S_IWRITE)) != -1)
  {
    if ((_read(fileHandle, pack, sizeof(packType)) != sizeof(packType)) ||
        (close(fileHandle) == -1))
      logEntry("Can't read "dPCKFNAME, LOG_ALWAYS, 1);
  }

  puts("Packing messages...\n");

  netIndex = 0;

  if ((dir = opendir(config.netPath)) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
    {
      if (match_spec("*.msg", ent->d_name))
      {
        msgNum = strtoul(ent->d_name, &helpPtr, 10);

        if (  msgNum != 0
           && netIndex < MAXNETREC
           && *helpPtr == '.'
           && readMsg(message, msgNum) == 0
           && (message->attribute & (IN_TRANSIT | LOCAL))
           && !(message->attribute & FILE_ATT)
           )
        {
          count = 0;
          while (count < MAX_AKAS
                && (  (config.akaList[count].nodeNum.zone == 0)
                   || (memcmp(&config.akaList[count].nodeNum, &message->destNode, sizeof(nodeNumType)) != 0)
                   )
                )
            count++;

          if (count == MAX_AKAS)
          {
            low  = 0;
            high = netIndex;
            while (low < high)
            {
              mid = (low + high) >> 1;
              if (msgNum > (*netList)[mid].msgNum)
                low  = mid + 1;
              else
                high = mid;
            }
            memmove(&((*netList)[low + 1]), &((*netList)[low]), (netIndex++ - low) * sizeof(netRecType));

            (*netList)[low].msgNum    = msgNum;
            (*netList)[low].destNode  = message->destNode;
            (*netList)[low].attribute = message->attribute;
            (*netList)[low].flags     = getFlags(message->text);
          }
        }
      }
    }
    closedir(dir);
  }

  if (argc > 2)
  {
    // Processing command line
    tempStr[0] = tempStr[1] = 0;
    for (count = 2; count < argc; count++)
    {
      strcat(tempStr, " ");
      strcat(tempStr, argv[count]);
    }
    processPackLine(tempStr + 1, switches);
  }
  else
  {
    for (count = nodeCount - 1; count >= 0; count--)
      if (nodeInfo[count]->options.packNetmail)
        packRoute(NULL, NULL, DEST_VIA, &(nodeInfo[count]->node), switches);

    for (count = 0; count < MAX_PACK && *(*pack)[count]; count++)
      processPackLine((*pack)[count], 0);
  }

  for (index = 0; index < netIndex; index++)
  {
    if (!(*netList)[index].viaNode.zone)
      continue;

    memcpy(&tempNode, &(*netList)[index].viaNode, sizeof(nodeNumType));
    errorDisplay |= makeNFInfo(&nodeFileInfo, -1, &tempNode);

    for (count = index; count < netIndex; count++)
    {
      if (memcmp(&(*netList)[count].viaNode, &tempNode, sizeof(nodeNumType)) != 0)
        continue;

      oldAttribute = (*netList)[count].attribute;
      msgNum       = (*netList)[count].msgNum;

      if (  readMsg(message, msgNum) == 0
         && oldAttribute == message->attribute
         && memcmp(&message->destNode, &(*netList)[count].destNode, sizeof(nodeNumType)) == 0
         && (  !isLocalPoint(&message->destNode)
            || memcmp(&(*netList)[count].viaNode, &message->destNode, sizeof(nodeNumType)) == 0
            )
         )
      {
        message->attribute &= PRIVATE | FILE_ATT | UNUSED | RET_REC_REQ | IS_RET_REC | AUDIT_REQ;

        if (errorDisplay)
        {
          errorDisplay=0;
          newLine();
        }

        sprintf( tempStr, "Message #%lu : %s "dARROW" %s", msgNum
               , nodeStr(&message->srcNode), nodeStr(&message->destNode));
        if (memcmp(&(*netList)[count].viaNode, &message->destNode, sizeof(nodeNumType)) != 0)
        {
          strcat(tempStr, " via ");
          strcat(tempStr, nodeStr(&(*netList)[count].viaNode));
        }

        logEntry(tempStr, LOG_PACK, 0);

        if (  !(nodeFileInfo.nodePtr->capability & PKT_TYPE_2PLUS)
           && memcmp(&nodeFileInfo.destNode4d, &message->destNode, sizeof(nodeNumType)) == 0
           )
          make2d(message);

        count2 = 0;
        while (  count2 < MAX_AKAS
              && memcmp(&message->srcNode, &config.akaList[count2].nodeNum, sizeof(nodeNumType)) != 0
              )
          count2++;

        addVia(message->text, count2 < MAX_AKAS ? count2 : nodeFileInfo.srcAka, "Pack", 1);

        if (writeNetPktValid(message, &nodeFileInfo))
          diskError = DERR_PACK;
        else
        {
          if (*config.topic1 || *config.topic2)
          {
            strcpy(tempStr, message->subject);
            strupr(tempStr);
          }
          if (config.mbOptions.persSent &&
              config.mailOptions.persNetmail &&
              *config.pmailPath &&
              (stricmp(message->fromUserName, config.sysopName) == 0 ||
               (*config.topic1 &&
                strstr(tempStr, config.topic1) != NULL) ||
               (*config.topic2 &&
                strstr(tempStr, config.topic2) != NULL) ||
               foundToUserName(message->fromUserName)
              )
             )
          {
            insertLine(message->text, "AREA: Netmail  SOURCE: Local\r");
            writeMsg(message, PERMSG, 1);
            removeLine(message->text);
          }

          // Remove message or change status

          globVars.netCountV++;

          fileHandle = -1;
          if (oldAttribute & KILLSENT)
          {
            sprintf(tempStr, "%s%u.msg", config.netPath, msgNum);

            if ((fileHandle = openP(tempStr, O_RDWR | O_DENYALL | O_BINARY, S_IREAD | S_IWRITE)) != -1)
            {
              close(fileHandle);
              fileHandle = unlink(tempStr);
            }
          }
          if (fileHandle == -1)
            attribMsg(oldAttribute|SENT, msgNum);
        }
        memset(&(*netList)[count].viaNode, 0, sizeof(nodeNumType));
      }
    }
    closeNetPktWr(&nodeFileInfo);
  }

  if (errorDisplay)
  {
    errorDisplay = 0;
    newLine();
  }
  free(netList);
  free(pack);

  return diskError;
}
//---------------------------------------------------------------------------
