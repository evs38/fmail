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

#include <alloc.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#include "fmail.h"

#include "cfgfile.h"
#include "config.h"
#include "ftools.h"
#include "ispathch.h"
#include "log.h"
#include "output.h"
#include "update.h"
#include "utils.h"

//---------------------------------------------------------------------------
static badEchoType badEchos[MAX_BAD_ECHOS];

typedef u8 *anType[MAX_AREAS];
typedef u8 *mpType[MAX_AREAS];

//---------------------------------------------------------------------------
void addNew(s32 switches)
{
  anType       *areaNames;
  mpType       *mbPaths;
  u8           usedArea[MBBOARDS];
  u8           *tempPtr, *tempPtr2;
  u16          badEchoCount, areaInfoCount, count, index;
  u16          areasExist, areasAdded;
  int          tempHandle;
  tempStrType  tempStr;
  headerType   *adefHeader, *areaHeader;
  rawEchoType  tempInfo, echoDefaultsRec, *adefBuf, *areaBuf;

  if ( (areaNames = malloc(sizeof(anType))) == NULL )
    return;
  if ( (mbPaths = malloc(sizeof(mpType))) == NULL )
  {
    free(areaNames);
    return;
  }
  areasExist = areasAdded = 0;
  memset(&echoDefaultsRec, 0, RAWECHO_SIZE);
  echoDefaultsRec.options.security = 1;
  if ( openConfig(CFG_AREADEF, &adefHeader, (void*)&adefBuf) )
  {
    if ( getRec(CFG_AREADEF, 0) )
      memcpy(&echoDefaultsRec, adefBuf, RAWECHO_SIZE);
    closeConfig(CFG_AREADEF);
  }
  else
  {
    strcpy(stpcpy(tempStr, configPath), dARDFNAME);
    unlink(tempStr);
  }
  if ( !openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf) )
    goto freemem2;
  memset(usedArea, 0, MBBOARDS);
  if ( config.badBoard && config.badBoard < MBBOARDS)
    usedArea[config.badBoard - 1] = 1;
  if ( config.dupBoard && config.dupBoard < MBBOARDS)
    usedArea[config.dupBoard - 1] = 1;
  if ( config.recBoard && config.recBoard < MBBOARDS)
    usedArea[config.recBoard - 1] = 1;
  for (count = 0; count < MAX_NETAKAS; ++count)
  {
    if (config.netmailBoard[count] && config.netmailBoard[count] < MBBOARDS)
      usedArea[config.netmailBoard[count] - 1] = 1;
  }
  areaInfoCount = 0;
  while (getRec(CFG_ECHOAREAS, areaInfoCount))
  {
    if ( areaBuf->board - 1 < MBBOARDS )
	    usedArea[areaBuf->board - 1] = 1;
    if ( areaBuf->boardNumRA - 1 < MBBOARDS )
	    usedArea[areaBuf->boardNumRA - 1] = 1;
    if ( ((*areaNames)[areaInfoCount] = malloc(strlen(areaBuf->areaName) + 1)) == NULL )
   	  goto freemem;
    strcpy((*areaNames)[areaInfoCount], areaBuf->areaName);
    if ( !*areaBuf->msgBasePath )
      (*mbPaths)[areaInfoCount] = 0;
    else
    {
      if ( ((*mbPaths)[areaInfoCount] = malloc(strlen(areaBuf->msgBasePath) + 1)) == NULL )
	      goto freemem;
	    strcpy((*mbPaths)[areaInfoCount], areaBuf->msgBasePath);
    }
    ++areaInfoCount;
  }
  strcpy(stpcpy(tempStr, configPath), dBDEFNAME);
  if ((tempHandle = open(tempStr, O_BINARY | O_RDONLY | O_DENYNONE)) != -1)
  {
    badEchoCount = (read(tempHandle, badEchos, MAX_BAD_ECHOS * sizeof(badEchoType)) + 1) / sizeof(badEchoType);
    close(tempHandle);
    while (badEchoCount--)
    {
      memcpy(&tempInfo, &echoDefaultsRec, RAWECHO_SIZE);
    	if (!tempInfo.group)
	      tempInfo.group = 1;
      tempInfo.options.active = 1;
      strncpy(tempInfo.areaName, badEchos[badEchoCount].badEchoName, ECHONAME_LEN-1);
      strncpy(tempInfo.comment , badEchos[badEchoCount].badEchoName, ECHONAME_LEN-1);
   // strupr(tempInfo.areaName);
      tempInfo.forwards[0].nodeNum = badEchos[badEchoCount].srcNode;
      tempInfo.address = badEchos[badEchoCount].destAka;
      tempInfo.boardNumRA = 0;
      if (tempInfo.board == 1)  // Hudson
      {
        count = 0;
        while (++count <= MBBOARDS)
          if (!usedArea[count - 1])
            break;

        tempInfo.board = (u16)((count <= MBBOARDS) ? count : 0);
        *tempInfo.msgBasePath = 0;
      }
      else
      {
        if (tempInfo.board == 2)  // JAM
        {
          MakeJamAreaPath(&tempInfo);

          for (;;)
          {
            index = 0;
            while ( index < areaInfoCount )
            {
              if ( (*mbPaths)[index] && !stricmp((*mbPaths)[index], tempInfo.msgBasePath) )
                break;
              ++index;
            }
            if ( index == areaInfoCount )
              break;
            tempPtr = strchr(tempInfo.msgBasePath, 0) - 1;
            for (;;)
            {
              if ( !isdigit(*tempPtr) )
                *tempPtr = '0';
              else
              {
                if ( *tempPtr != '9' )
                  ++*tempPtr;
                else
                {
                  *tempPtr = '0';
                  --tempPtr;
                  if ( tempPtr >= tempInfo.msgBasePath && *tempPtr != '\\' )
                    continue;
                }
              }
              break;
            }
          }
          tempInfo.board = 0;
        }
        else // pass-through
          *tempInfo.msgBasePath = 0;
      }
      index = 0;
	    while (index < areaInfoCount && strcmp(tempInfo.areaName, (*areaNames)[index]) > 0)
        index++;
      if (index < areaInfoCount && !strcmp(tempInfo.areaName, (*areaNames)[index]))
      {
        ++areasExist;
        sprintf(tempStr, "Area %s is already present", tempInfo.areaName);
        logEntry(tempStr, LOG_ALWAYS, 0);
        continue;
      }
      if (index < areaInfoCount++)
      {
        memmove(&(*areaNames)[index+1], &(*areaNames)[index], (areaInfoCount-index) * sizeof(char *));
        memmove(&(*mbPaths  )[index+1], &(*mbPaths  )[index], (areaInfoCount-index) * sizeof(char *));
      }
      if (((*areaNames)[index] = malloc(strlen(tempInfo.areaName) + 1)) == NULL)
       goto freemem;
      strcpy((*areaNames)[index], tempInfo.areaName);
      if (!*tempInfo.msgBasePath)
        (*mbPaths)[index] = 0;
      else
      {
        if (((*mbPaths)[index] = malloc(strlen(tempInfo.msgBasePath) + 1)) == NULL)
          goto freemem;
        strcpy((*mbPaths)[index], tempInfo.msgBasePath);
      }
      memcpy(areaBuf, &tempInfo, RAWECHO_SIZE);
      insRec(CFG_ECHOAREAS, index);
      usedArea[areaBuf->board - 1] = 1;
      ++areasAdded;
      sprintf(tempStr, "Adding area %s", tempInfo.areaName);
      logEntry(tempStr, LOG_ALWAYS, 0);
    }
    unlink(tempStr);
  }
freemem:
  closeConfig(CFG_ECHOAREAS);
  while (areaInfoCount--)
  {
    free((*areaNames)[areaInfoCount]);
    if ((*mbPaths)[areaInfoCount])
      free((*mbPaths)[areaInfoCount]);
  }
  if (areasAdded || areasExist)
    newLine();
  sprintf(tempStr, "Areas added: %u, areas already present: %u", areasAdded, areasExist);
  logEntry(tempStr, LOG_ALWAYS, 0);
  strcpy(stpcpy(tempStr, configPath), dBDEFNAME);
  unlink(tempStr);
freemem2:
  free(mbPaths);
  free(areaNames);
  if (switches & SW_A)
    autoUpdate();
}
//---------------------------------------------------------------------------
