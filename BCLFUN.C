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

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"

#include "archive.h"
#include "areainfo.h"
#include "bclfun.h"
#include "cfgfile.h"
#include "config.h"
#include "log.h"
#include "msgpkt.h"
#include "mtask.h"
#include "nodeinfo.h"
#include "output.h"
#include "utils.h"
#include "version.h"

//---------------------------------------------------------------------------
bcl_header_type bcl_header;
bcl_type        bcl;
static fhandle  bclHandle;

//---------------------------------------------------------------------------
udef openBCL(uplinkReqType *uplinkReq)
{
  tempStrType tempStr;
  nodeNumType tempNode;

  sprintf(tempStr, "%s%s", configPath, uplinkReq->fileName);
  if ( (bclHandle = openP(tempStr, O_RDONLY|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE)) == -1
     || read(bclHandle, &bcl_header, sizeof(bcl_header)) != sizeof(bcl_header)
     )
  {
    close(bclHandle);
    return 0;
  }
  tempNode.point = 0;
  if (  memcmp(bcl_header.FingerPrint, "BCL", 4)
     || sscanf(bcl_header.Origin, "%hu:%hu/%hu.%hu", &tempNode.zone, &tempNode.net, &tempNode.node, &tempNode.point) < 3 )
  {
    close(bclHandle);
    return 0;
  }
  return 1;
}
//---------------------------------------------------------------------------
udef readBCL(char **tag, char **descr)
{
  static char buf[256];

  if (eof(bclHandle))
    return 0;
  if (read(bclHandle, &bcl, sizeof(bcl_type)) != sizeof(bcl_type) )
    return 0;
  if (read(bclHandle, buf, bcl.EntryLength - sizeof(bcl_type)) != (int)(bcl.EntryLength - sizeof(bcl_type)))
    return 0;

  *tag = buf;
  *descr = strchr(buf, 0) + 1;

  return 1;
}
//---------------------------------------------------------------------------
udef closeBCL(void)
{
  return close(bclHandle);
}
//---------------------------------------------------------------------------
void LogFileDetails(const char *fname, const char *txt)
{
  struct stat statbuf;
  tempStrType tempStr;

  if (0 == stat(fname, &statbuf))
  {
    struct tm *tm = localtime(&statbuf.st_mtime);
    sprintf(tempStr, "%s %s %lu, %04d-%02d-%02d %02d:%02d:%02d", txt, fname, statbuf.st_size
                   , tm->tm_year + 1900, tm->tm_mon +1, tm->tm_mday
                   , tm->tm_hour, tm->tm_min, tm->tm_sec);
  }
  else
    sprintf(tempStr, "%s %s", txt, fname);

  logEntry(tempStr, LOG_ALWAYS, 0);
}
//---------------------------------------------------------------------------
udef process_bcl(char *fileName)
{
  fhandle     handle;
  tempStrType tempStr
            , tempStr2;
  char        newFileName[13];
  u16         index;
  nodeNumType tempNode;

  sprintf (tempStr, "%s%s", config.inPath, fileName);
  if ( (handle = openP(tempStr, O_RDONLY | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE)) == -1
     || read(handle, &bcl_header, sizeof(bcl_header)) != sizeof(bcl_header)
     )
  {
    close(handle);
    return 0;
  }
  close(handle);
  tempNode.point = 0;
  if (  memcmp(bcl_header.FingerPrint, "BCL", 4)
     || sscanf(bcl_header.Origin, "%hu:%hu/%hu.%hu",
                                 &tempNode.zone, &tempNode.net,
                                 &tempNode.node, &tempNode.point) < 3 )
    return 0;

  index = 0;
  while (index < MAX_UPLREQ)
  {
    if (  config.uplinkReq[index].node.zone && config.uplinkReq[index].fileType == 2
       && !memcmp(&config.uplinkReq[index].node, &tempNode, sizeof(nodeNumType)) )
      break;
    ++index;
  }
  if (index == MAX_UPLREQ)
    return 0;

  sprintf(newFileName, "%08lX.BCL", uniqueID());
  sprintf(tempStr2, "%s%s", configPath, newFileName);
  if (!moveFile(tempStr, tempStr2))
  {
    sprintf(tempStr, "New BCL file received from uplink %s", nodeStr(&tempNode));
    logEntry(tempStr, LOG_ALWAYS, 0);
    sprintf(tempStr, "%s%s", configPath, config.uplinkReq[index].fileName);
    LogFileDetails(tempStr , "Old:");
    unlink(tempStr);
    LogFileDetails(tempStr2, "New:");
    strcpy(config.uplinkReq[index].fileName, newFileName);
  }
  return 1;
}
//---------------------------------------------------------------------------
void ChkAutoBCL(void)
{
  u16 index;

  logEntry("Check AutoBCL", LOG_DEBUG, 0);

  for (index = 0; index < nodeCount; index++)
  {
    nodeInfoType *ni = nodeInfo[index];
    u16 autobcl = ni->autoBCL;

    if (autobcl && startTime - (time_t)ni->lastSentBCL > autobcl * 86400L)
    {
      send_bcl(&config.akaList[matchAka(&ni->node, 0)].nodeNum, &ni->node, ni);
      ni->lastSentBCL = startTime;
    }
  }
}
//---------------------------------------------------------------------------
udef ScanNewBCL(void)
{
  tempStrType  tempStr;
  struct _finddata_t fd;
  long fh;
  udef count = 0;

  logEntry("Scan for received BCL files", LOG_DEBUG, 0);

  sprintf(tempStr, "%s*.BCL", config.inPath);
  fh = _findfirst(tempStr, &fd);
  if (fh != -1)
  {
    do
    {
      count += process_bcl(fd.name);
    }
    while (0 == _findnext(fh, &fd));
    _findclose(fh);
  }
  if (count)
    newLine();

  return count;
}
//---------------------------------------------------------------------------
void send_bcl(nodeNumType *srcNode, nodeNumType *destNode, nodeInfoType *nodeInfoPtr)
{
  u16          count;
  tempStrType  tempStr;
  fhandle      helpHandle;
  headerType  *areaHeader;
  rawEchoType *areaBuf;

  if (!openConfig(CFG_ECHOAREAS, &areaHeader, (void**)&areaBuf))
    return;

  sprintf(tempStr, "%s%08lX.$$$", config.outPath, uniqueID());
  if ((helpHandle = openP(tempStr, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) != -1)
  {
    tempStrType logStr;
    sprintf(logStr, "Creating BCL file for node %s: %s", nodeStr(destNode), tempStr);
    logEntry(logStr, LOG_ALWAYS, 0);

    memset(&bcl_header, 0, sizeof(bcl_header));
    strcpy(bcl_header.FingerPrint, "BCL");
    strncpy(bcl_header.ConfMgrName, VersionStr(), 30);
    strcpy(bcl_header.Origin, nodeStr(srcNode));
    bcl_header.CreationTime = time(NULL);
    bcl_header.flags = BCLH_ISLIST;
    write(helpHandle, &bcl_header, sizeof(bcl_header));
    for (count = 0; count < areaHeader->totalRecords; count++)
    {
      getRec(CFG_ECHOAREAS, count);

      if (!areaBuf->options.active || areaBuf->options.local)
        continue;
      if (!(areaBuf->group & nodeInfoPtr->groups))
        continue;

      bcl.EntryLength = sizeof(bcl) + strlen(areaBuf->areaName) + 1 + strlen(areaBuf->comment) + 1 + 1;
      bcl.flags1 = FLG1_MAILCONF
                 | (areaBuf->options.allowPrivate ? FLG1_PRIVATE : 0)
                 | ((nodeInfoPtr->writeLevel < areaBuf->writeLevel) ? FLG1_READONLY : 0);
      write(helpHandle, &bcl, sizeof(bcl));
      write(helpHandle, areaBuf->areaName, strlen(areaBuf->areaName)+1);
      write(helpHandle, areaBuf->comment, strlen(areaBuf->comment)+1);
      write(helpHandle, "", 1);
    }
    close(helpHandle);
    packArc(tempStr, srcNode, destNode, nodeInfoPtr);
  }
  closeConfig(CFG_ECHOAREAS);
}
//---------------------------------------------------------------------------
