//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2017  Wilfred van Velzen
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

#ifdef __WIN32__
#include <share.h>
#endif // __WIN32__
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>   // calloc()
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "fmail.h"

#include "archive.h"
#include "areainfo.h"
#include "bclfun.h"
#include "cfgfile.h"
#include "config.h"
#include "log.h"
#include "msgmsg.h"    // writeMsg()
#include "msgpkt.h"
#include "mtask.h"
#include "nodeinfo.h"
#include "os.h"
#include "os_string.h"
#include "spec.h"
#include "utils.h"
#include "version.h"

//---------------------------------------------------------------------------
bcl_header_type bcl_header;
static bcl_type bcl;
static fhandle  bclHandle;
static char    *bclBuf = NULL;

typedef struct
{
  void     *next;
  bcl_type *bcl;
  char     *tag;
  char     *descr;
//char     *admin;  // not used

} bclListNode;
//---------------------------------------------------------------------------
int openBcl(const char *fname, bcl_header_type *bh, nodeNumType *nodeNum)
{
  int hndl;

  if ( (hndl = _sopen(fixPath(fname), O_RDONLY | O_BINARY, SH_DENYRW)) == -1
     || read(hndl, bh, sizeof(bcl_header_type)) != sizeof(bcl_header_type)
     )
  {
    close(hndl);

    return -1;
  }
  nodeNum->point = 0;
  if (  memcmp(bh->FingerPrint, "BCL", 4)
     || sscanf(bh->Origin, "%hu:%hu/%hu.%hu", &nodeNum->zone, &nodeNum->net, &nodeNum->node, &nodeNum->point) < 3
     )
  {
    close(hndl);

    return -1;
  }
  return hndl;
}
//---------------------------------------------------------------------------
int openBCL(uplinkReqType *uplinkReq)
{
  tempStrType tempStr;
  nodeNumType tmpNode;

  strcpy(stpcpy(tempStr, configPath), uplinkReq->fileName);

  return (bclHandle = openBcl(tempStr, &bcl_header, &tmpNode)) >= 0;
}
//---------------------------------------------------------------------------
char *readBcl(fhandle hndl)
{
  int l;
  char *buf;

  if (eof(hndl))
    return NULL;

  if ((buf = malloc(sizeof(bcl_type))) == NULL)
    return NULL;

  if (read(hndl, buf, sizeof(bcl_type)) != sizeof(bcl_type))
  {
    free(buf);

    return NULL;
  }

  if ((buf = realloc(buf, ((bcl_type*)buf)->EntryLength)) == NULL)
    return NULL;

  l = ((bcl_type*)buf)->EntryLength - sizeof(bcl_type);
  if (read(hndl, buf + sizeof(bcl_type), l) != l)
  {
    free(buf);

    return NULL;
  }

  return buf;
}
//---------------------------------------------------------------------------
int readBCL(char **tag, char **descr)
{
  free(bclBuf);

  if ((bclBuf = readBcl(bclHandle)) == NULL);
    return 0;

  *tag   = bclBuf + sizeof(bcl_type);
  *descr = strchr(*tag, 0) + 1;

  return 1;
}
//---------------------------------------------------------------------------
int closeBCL(void)
{
  free(bclBuf);

  return close(bclHandle);
}
//---------------------------------------------------------------------------
void LogFileDetails(const char *fname, const char *txt)
{
  struct stat statbuf;
  tempStrType tempStr;
  struct tm *tm;

  if (  0 == stat(fixPath(fname), &statbuf)
     && (tm = localtime(&statbuf.st_mtime)) != NULL
     )
    sprintf(tempStr, "%s %s %lu, %04d-%02d-%02d %02d:%02d:%02d", txt, fname, statbuf.st_size
                   , tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday
                   , tm->tm_hour, tm->tm_min, tm->tm_sec);
  else
    sprintf(tempStr, "%s %s", txt, fname);

  logEntry(tempStr, LOG_ALWAYS, 0);
}
//---------------------------------------------------------------------------
bclListNode *readBclList(fhandle hndl, int *maxLen, int *no)
{
  bclListNode *bln = NULL
            , *tln
            , *nln = NULL;
  void        *buf;
  size_t       sl;
  int          n = 0;

  while ((buf = readBcl(hndl)) != NULL)
  {
    n++;
    if ((tln = malloc(sizeof(bclListNode))) == NULL)
      break;

    if (nln != NULL)
      nln->next = tln;

    nln = tln;
    nln->next  = NULL;
    nln->bcl   = buf;
    nln->tag   = buf + sizeof(bcl_type);
    sl = strlen(nln->tag);
    if (sl > *maxLen)
      *maxLen = sl;
    nln->descr = nln->tag + sl + 1;
  //nln->admin = strchr(nln->descr, 0) + 1;  // not used

    if (bln == NULL)
      bln = nln;
  }
  if (no)
    *no = n;

  return bln;
}
//---------------------------------------------------------------------------
void freeBclList(bclListNode *bln)
{
  bclListNode *tln;

  while (bln != NULL)
  {
    tln = bln->next;
    free(bln->bcl);
    free(bln);
    bln = tln;
  }
}
//---------------------------------------------------------------------------
int process_bcl(char *fileName)
{
  fhandle         hndl;
  tempStrType     recFile
                , newFile
                , oldFile;
  char            newFileName[13];
  u16             index;
  uplinkReqType  *uplink;
  nodeNumType     tmpNode;
  bcl_header_type bclhdr;

  strcpy(stpcpy(recFile, config.inPath), fileName);
  if ((hndl = openBcl(recFile, &bclhdr, &tmpNode)) < 0)
    return 0;

  logEntryf(LOG_ALWAYS, 0, "New BCL file [%s] received from: %s", fileName, nodeStr(&tmpNode));

  index = 0;
  while (index < MAX_UPLREQ)
  {
    if (  config.uplinkReq[index].node.zone && config.uplinkReq[index].fileType == 2
       && !memcmp(&config.uplinkReq[index].node, &tmpNode, sizeof(nodeNumType))
       )
      break;
    ++index;
  }
  if (index >= MAX_UPLREQ)
  {
    close(hndl);
    logEntryf(LOG_ALWAYS, 0, "Not processed, %s is not a configured uplink of this system", nodeStr(&tmpNode));
    addExtension(recFile, ".not_an_uplink");

    return 0;
  }
  uplink = &config.uplinkReq[index];

  sprintf(newFileName, "%08x.bcl", uniqueID());
  strcpy(stpcpy(newFile, configPath), newFileName);
  if (*uplink->fileName)
    strcpy(stpcpy(oldFile, configPath), uplink->fileName);

  if (uplink->bclReport)
  {
    // Do report and/or compare.
    char           *helpPtr;
    bclListNode    *bln
                 , *nln
                 , *bln2
                 , *nln2;
    int             aka;
    fhandle         hndl2;
    bcl_header_type bclhdr2;
    int             maxLen = 0;
    int             epilog = 0;
    int             no     = 0;
    // Make new message
    internalMsgType *msg;
    int msgLen = 1000000; // sizeof bcl file * 3 + msgHeader

    if (NULL == (msg = (internalMsgType *)calloc(msgLen, 1)))
      logEntry("Not enough memory to create bcl report message", LOG_ALWAYS, 2);

    logEntry("Generating bcl report message", LOG_ALWAYS, 0);

    nln  = bln  = readBclList(hndl , &maxLen, &no);

    // Set nodenumber, name, and other items for destination
    snprintf(msg->fromUserName, 36, "FMail %s", funcStr);
    strncpy (msg->toUserName  , config.sysopName, 35);
    strcpy  (msg->subject     , "BCL receive report");
    aka = matchAka(&tmpNode, 0);
    msg->destNode  =
    msg->srcNode   = *getAkaNodeNum(aka, 1);
    msg->attribute = PRIVATE;
    setCurDateMsg(msg);
    // Add kludges
    helpPtr = addINTLKludge  (msg, msg->text);
    helpPtr = addPointKludges(msg, helpPtr);
    helpPtr = addMSGIDKludge (msg, helpPtr);
    helpPtr = addTZUTCKludge (helpPtr);
    helpPtr = addPIDKludge   (helpPtr);

    // Add some text
    helpPtr += sprintf( helpPtr
                      , "\r"
                        "From node            : %s\r"
                        "Received file        : %s\r"
                      , nodeStr(&tmpNode)
                      , fileName
                      );

    if (*uplink->fileName)
      helpPtr += sprintf( helpPtr
                        , "Replaces             : %s\r"
                        , uplink->fileName
                        );

    helpPtr += sprintf( helpPtr
                      , "Saved as             : %s\r"
                        "Sending software     : %s\r"
                        "Creation time        : %s\r"
                        "Type                 : %s\r"
                        "Number of conferences: %d\r"
                        "\r"
                      , newFileName
                      , bclhdr.ConfMgrName
                      , isoFmtTime(bclhdr.CreationTime)
                      , bclhdr.flags == BCLH_ISLIST ? "Full list" : bclhdr.flags == BCLH_ISUPDATE ? "Update" : "Unknown"
                      , no
                      );

    if (  bclhdr.flags == BCLH_ISLIST && uplink->bclReport == 1 && *uplink->fileName
       && (hndl2 = openBcl(oldFile, &bclhdr2, &tmpNode)) >= 0
       )
    {
      nln2 = bln2 = readBclList(hndl2, &maxLen, NULL);
      epilog = 1;

      while (nln != NULL || nln2 != NULL)
      {
        int c;
        if      (nln  == NULL)
          c = -1;
        else if (nln2 == NULL)
          c =  1;
        else
          c = stricmp(nln2->tag, nln->tag);

        if (c < 0)
        {
          helpPtr += sprintf(helpPtr, "- %-*s %s\r", maxLen, nln2->tag, nln2->descr);
          nln2 = nln2->next;
          epilog = 2;
        }
        else if (c > 0)
        {
          helpPtr += sprintf(helpPtr, "+ %-*s %s\r", maxLen, nln->tag, nln->descr);
          nln = nln->next;
          epilog = 2;
        }
        else
        {
          if (strcmp(nln2->descr, nln->descr))
          {
            helpPtr += sprintf(helpPtr, "~ %-*s %s\r", maxLen, nln->tag, nln->descr);
            epilog = 2;
          }

          nln  = nln ->next;
          nln2 = nln2->next;
        }
      }
      freeBclList(bln2);
      close(hndl2);
    }
    else
      if (bclhdr.flags == BCLH_ISUPDATE)
        for (; nln != NULL; nln = nln->next)
          helpPtr += sprintf(helpPtr, "%c %-*s %s\r", (nln->bcl->flags1 & FLG1_REMOVE) ? '-' : '+', maxLen, nln->tag, nln->descr);
      else
        // Full list
        for (; nln != NULL; nln = nln->next)
          helpPtr += sprintf(helpPtr, "  %-*s %s\r", maxLen, nln->tag, nln->descr);

    freeBclList(bln);

    if (epilog)
      helpPtr = stpcpy(helpPtr, epilog == 1 ? "\rThere are no changes in the listed conferences, compared to the previous list.\r"
                                            : "\r"
                                              "- = Conference was removed.\r"
                                              "+ = Conference was added.\r"
                                              "~ = Conference description has changed.\r"
                      );

    *helpPtr++ = '\r';
    addVia(helpPtr, aka, 1);

    // Plaats het in netmail.
    writeMsg(msg, NETMSG, 1);
    free(msg);
  }
#ifdef _DEBUG
  else
    logEntryf(LOG_DEBUG, 0, "DEBUG Not generating bcl report message: %u", uplink->bclReport);
#endif // _DEBUG

  // File needs to be closed before moving it.
  close(hndl);
  if (!moveFile(recFile, newFile))
  {
    if (*uplink->fileName)
    {
      LogFileDetails(oldFile , "Replaces:");
      unlink(fixPath(oldFile));
    }
    LogFileDetails(newFile, "Saved as:");
    strcpy(uplink->fileName, newFileName);
  }
  else
    logEntryf(LOG_ALWAYS, 0, "* Error moving %s -> %s", recFile, newFile);

  return 1;
}
//---------------------------------------------------------------------------
int ScanNewBCL(void)
{
  int            count = 0;
  DIR           *dir;
  struct dirent *ent;

#ifdef _DEBUG
  logEntry("DEBUG Scan for received BCL files", LOG_DEBUG, 0);
#endif

  if ((dir = opendir(fixPath(config.inPath))) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
      if (match_spec("*.bcl", ent->d_name))
        count += process_bcl(ent->d_name);

    closedir(dir);
  }
  if (count)
    newLine();

  return count;
}
//---------------------------------------------------------------------------
void ChkAutoBCL(void)
{
  u16 index;

#ifdef _DEBUG
  logEntry("DEBUG Check AutoBCL", LOG_DEBUG, 0);
#endif

  for (index = 0; index < nodeCount; index++)
  {
    nodeInfoType *ni = nodeInfo[index];
    if (ni->options.active)
    {
      u16 autobcl = ni->autoBCL;

      if (autobcl && startTime - (time_t)ni->lastSentBCL > autobcl * 86400L)
      {
        send_bcl(&config.akaList[matchAka(&ni->node, 0)].nodeNum, &ni->node, ni);
        ni->lastSentBCL = startTime;
      }
    }
  }
}
//---------------------------------------------------------------------------
void send_bcl(nodeNumType *srcNode, nodeNumType *destNode, nodeInfoType *nodeInfoPtr)
{
  u16           count;
  tempStrType   tempStr;
  fhandle       helpHandle;
  headerType  *areaHeader;
  rawEchoType *areaBuf;

  if (!openConfig(CFG_ECHOAREAS, &areaHeader, (void**)&areaBuf))
    return;

  sprintf(tempStr, "%s%08x."dEXTTMP, config.outPath, uniqueID());
  if ((helpHandle = open(fixPath(tempStr), O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) != -1)
  {
    logEntryf(LOG_ALWAYS, 0, "Creating BCL file for node %s: %s", nodeStr(destNode), tempStr);

    memset(&bcl_header, 0, sizeof(bcl_header));
    strcpy (bcl_header.FingerPrint, "BCL");
    strncpy(bcl_header.ConfMgrName, VersionStr(), 30);
    strcpy (bcl_header.Origin, nodeStr(srcNode));
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
      write(helpHandle, areaBuf->areaName, strlen(areaBuf->areaName) + 1);
      write(helpHandle, areaBuf->comment , strlen(areaBuf->comment ) + 1);
      write(helpHandle, "", 1);
    }
    close(helpHandle);
    packArc(tempStr, srcNode, destNode, nodeInfoPtr);
  }
  closeConfig(CFG_ECHOAREAS);
}
//---------------------------------------------------------------------------
