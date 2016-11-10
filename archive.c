// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------

#include <dir.h>
#include <dirent.h>    // opendir()
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <process.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"

#include "archive.h"
#include "areainfo.h"
#include "config.h"
#include "copyFile.h"
#include "crc.h"
#include "log.h"
#include "minmax.h"
#include "msgmsg.h"
#include "msgpkt.h"
#include "nodeinfo.h"
#include "spec.h"
#include "stpcpy.h"
#include "utils.h"

#define MAX_PARSIZE 256
// ----------------------------------------------------------------------------

extern s16 ems;
extern configType config;

extern fAttInfoType fAttInfo[MAX_ATTACH];
extern u16 fAttCount;

extern const char *months;

// ----------------------------------------------------------------------------
static s16 checkExist(char *entry, char *prog, char *par)
{
  const char *helpPtr;

  if (  (helpPtr = strtok(entry, " ")) == NULL
     || (access(helpPtr, 4) != 0 && (helpPtr = _searchpath(helpPtr)) == NULL)
     || strnicmp(helpPtr, config.inPath, strlen(config.inPath)) == 0
     )
  {
    logEntryf(LOG_ALWAYS, 0, "%s not found", entry);
    return -1;
  }
  strcpy(prog, helpPtr);
  if ((helpPtr = strtok(NULL, "")) == NULL)
    *par = 0;
  else
    strcpy(par, helpPtr);

  return 0;
}
// ----------------------------------------------------------------------------
static u8 archiveType(char *fullFileName)
{
  fhandle arcHandle;
  s16 byteCount;
  unsigned char data[29];

  if ((arcHandle = _sopen(fullFileName, O_RDONLY | O_BINARY, SH_DENYRW)) == -1)
    return 0xFE;

  byteCount = read(arcHandle, data, 29);
  close(arcHandle);

  if (byteCount < 2)
    return 0xFE;

  if (memcmp(data, "Rar!\x1a\x07", 7) == 0)  // 7h byte is zero !
    return 9;   // RAR

  if (memcmp(data + 14, "\x1aJar\x1b", 6) == 0)  // 19th byte is zero !
    return 10;   // JAR

  if ((data [0] == 'U') && (data [1] == 'C')
      && (data [2] == '2') && (data [3] == 26) && (byteCount >= 4))
    return 8;     // UC2

  if ((data [0] == 80) && (data [1] == 75)
      && (data [2] == 3) && (data [3] == 4) && (byteCount >= 4))  // 3, 4: GUS
    return 1;     // ZIP

  if ((data [0] == 'H') && (data [1] == 'L')
      && (data [2] == 'S') && (data [3] == 'Q') && (byteCount >= 4))
    return 6;     // SQZ

  if ((data [20] == 0xdc) && (data [21] == 0xa7)
      && (data [22] == 0xc4) && (data [23] == 0xfd) && (byteCount >= 25))
    return 4;     // ZOO

  if ((data [2] == '-') && (data [3] == 'l')
      && /* (data [4] == 'h') && #zie GUS# */
      (data [6] == '-') && (byteCount >= 7))
    return 2;   // LZH

  if ((data [0] == 0x60) && (data [1] == 0xEA))
    return 5;     // ARJ

  if ((data [0] == 26) && (data [1] >= 10)
      && (data [1] < 20) && (byteCount == 29))
    return 3;     // PAK

  if ((data [0] == 26) && (data [1] < 10) && (byteCount == 29))
    return 0;     // ARC, Should check second, third etc. file also

  return 0xFF;
}
// ----------------------------------------------------------------------------
static s16 execute(const char *arcType, const char *program, const char *parameters, const char *archiveName, char *pktName, const char *privPath)
{
  s16         dosExitCode = -1;
  char        tempStr[MAX_PARSIZE];
  tempStrType tempPath;
  char       *helpPtr;
  u16         len = 0;
  clock_t     ct = clock();

  while ((helpPtr = strstr(parameters, "\%a")) != NULL)
  {
    len = 1;
    if (strlen(parameters) + strlen(archiveName) >= MAX_PARSIZE)
    {
      logEntryf(LOG_ALWAYS, 0, "Argument list of %s utility is too big", arcType);
      return 1;
    }
    strcpy(tempStr,                      helpPtr + 2);
    strcpy(stpcpy(helpPtr, archiveName), tempStr);
  }
  if ((helpPtr = strstr(parameters, "\%f")) != NULL)
  {
    if ((len = strlen(pktName)) != 0)
      if (pktName[len - 1] == '\\')
        pktName[len - 1] = 0;
    do
    {
      if (strlen(parameters) + strlen(pktName) >= MAX_PARSIZE)
      {
        logEntryf(LOG_ALWAYS, 0, "Argument list of %s utility is too big", arcType);
        return 1;
      }
      strcpy(tempStr,                  helpPtr + 2);
      strcpy(stpcpy(helpPtr, pktName), tempStr);
    }
    while ((helpPtr = strstr(parameters, "\%f")) != NULL);
  }

  if (privPath != NULL && (helpPtr = strstr(parameters, "\%p")) != NULL)
  {
    strcpy(tempPath, privPath);
    if ((len = strlen(tempPath)) != 0)
      if (tempPath[len - 1] == '\\')
        tempPath[len - 1] = 0;
    do
    {
      if (strlen(parameters) + strlen(tempPath) >= MAX_PARSIZE)
      {
        logEntryf(LOG_ALWAYS, 0, "Argument list of %s utility is too big", arcType);
        return 1;
      }
      strcpy(tempStr, helpPtr + 2);
      strcpy(stpcpy(helpPtr, tempPath), tempStr);
    }
    while ((helpPtr = strstr(parameters, "\%p")) != NULL);
  }

  if (len == 0)
    sprintf(strchr(parameters, 0),  " %s %s", archiveName, pktName);

  logEntryf(LOG_EXEC | LOG_NOSCRN, 0, "Executing %s %s", program, parameters);

  printf("<-- %s Start -->\n", arcType);
  fflush(stdout);

#ifndef __NOSPAWNL__
// for Win16 compile
  if (dosExitCode == -1)
  {
    dosExitCode = spawnl(P_WAIT, program, program, parameters, NULL);
    // flush();
  }
#endif
  printf("<-- %s End [%.2f] -->\n", arcType, ((double)(clock() - ct)) / CLK_TCK);
  // flush();
  if (dosExitCode == -1)
  {
    sprintf(tempStr, "Cannot execute %s utility: ", arcType);
    switch (errno)
    {
      case E2BIG:
        strcat(tempStr, "Arg list too big");
        break;

      case EINVAL:
        strcat(tempStr, "Invalid argument");
        break;

      case ENOENT:
        strcat(tempStr, "Path or file name not found");
        break;

      case ENOEXEC:
        strcat(tempStr, "Exec format error");
        break;

      case ENOMEM:
        strcat(tempStr, "Not enough memory");
        break;
    }
    logEntry(tempStr, LOG_ALWAYS, 0);
    return 1;
  }
  if (dosExitCode != 0)
  {
    sprintf(tempStr, "Archiving utility for %s method reported errorlevel %u", arcType, dosExitCode);
    logEntry(tempStr, LOG_ALWAYS, 0);
    return 1;
  }
  return 0;
}
// ----------------------------------------------------------------------------
static void outboundBackup(const char *path)
{
  tempStrType bakPath;
  const char *helpPtr;

  if (path == NULL || *path == 0 || *config.outBakPath == 0)
    return;

  if ((helpPtr = strrchr(path, '\\')) == NULL)
    helpPtr = path;
  else
    helpPtr++;

  strcpy(stpcpy(bakPath, config.outBakPath), helpPtr);
  if (copyFile(path, bakPath, 0))
    logEntryf(LOG_OUTBOUND | LOG_PACK | LOG_PKTINFO, 0, "Backup %s -> %s", path, bakPath);
  else
    logEntryf(LOG_OUTBOUND | LOG_PACK | LOG_PKTINFO, 0, "Backup Failed! %s -> %s", path, bakPath);
}
// ----------------------------------------------------------------------------
void unpackArc(const struct bt *bp)
{
  tempStrType tempStr
            , fullFileName
            , arcPath
            , unpackPathStr
            , dirStr
            , pathStr;
  s16 temp;
  char *extPtr = "";
  char parStr[MAX_PARSIZE];
  char procStr1[64];
  char procStr2[MAX_PARSIZE];
  s16 arcType;

  *arcPath = 0;
  strcpy(stpcpy(fullFileName, config.inPath), bp->fname);

  switch (arcType = archiveType(fullFileName))
  {
  case  0:      // ARC
    strcpy(arcPath, config.unArc32.programName);
    extPtr = (char*)"arc";
    break;

  case  1:      // ZIP
    strcpy(arcPath, config.unZip32.programName);
    extPtr = (char*)"zip";
    break;

  case  2:      // LZH
    strcpy(arcPath, config.unLzh32.programName);
    extPtr = (char*)"lzh";
    break;

  case  3:      // PAK
    strcpy(arcPath, config.unPak32.programName);
    extPtr = (char*)"pak";
    break;

  case  4:      // ZOO
    strcpy(arcPath, config.unZoo32.programName);
    extPtr = (char*)"zoo";
    break;

  case  5:      // ARJ
    strcpy(arcPath, config.unArj32.programName);
    extPtr = (char*)"arj";
    break;

  case  6:      // SQZ
    strcpy(arcPath, config.unSqz32.programName);
    extPtr = (char*)"sqz";
    break;

  case  8:      // UC2
    strcpy(arcPath, config.unUc232.programName);
    extPtr = (char*)"uc2";
    break;

  case  9:      // RAR
    strcpy(arcPath, config.unRar32.programName);
    extPtr = (char*)"rar";
    break;

  case 10:      // JAR
    strcpy(arcPath, config.unJar32.programName);
    extPtr = (char*)"jar";
    break;

  case 0xFE:
    return;

  default:
    break;
  }

  if (*arcPath == 0)
  {
    if (!*config.GUS.programName)
    {
      if (arcType == 0xFF)
        logEntryf(LOG_ALWAYS, 0, "Unknown archiving utility used for %s"                  , fullFileName);
      else
        logEntryf(LOG_ALWAYS, 0, "Archive decompression program for %s method not defined", extPtr);

      return;
    }
    strcpy(arcPath, config.GUS.programName);
    extPtr = (char*)"gus";
  }
  if (checkExist(arcPath, pathStr, parStr))
  {
    logEntryf(LOG_ALWAYS, 0, "Archive decompression program for %s method not found", extPtr);
    return;
  }
  logEntryf(LOG_INBOUND, 0, "Decompressing %s (%s)", fullFileName, extPtr);
  {
    struct tm *tm = localtime(&bp->mtime);  // TODO (Wilfred#1#10/21/16): Check if correct value is logged
    if (tm != NULL)
      sprintf(tempStr, "Archive info: %ld, %04u-%02u-%02u %02u:%02u:%02u"
                     , bp->size
                     , tm->tm_year + 1900, tm->tm_mon +1, tm->tm_mday
                     , tm->tm_hour, tm->tm_min, tm->tm_sec);
    else
      sprintf(tempStr, "Archive info: %ld, 1970-01-01 00:00:00", bp->size);

    logEntry(tempStr, LOG_INBOUND, 0);
  }
  strcpy(unpackPathStr, config.inPath);  // pktPath

  // preprocessor
  if (*config.preUnarc32.programName)
  {
    strcpy(tempStr, config.preUnarc32.programName);
    if (!checkExist(tempStr, procStr1, procStr2))
      execute("Pre-Unpack", procStr1, procStr2, fullFileName, unpackPathStr,  NULL);
  }
  if (arcType == 4)  // ZOO
  {
    if ((temp = strlen(unpackPathStr)) > 3)
      unpackPathStr[temp - 1] = 0;

    getcwd(dirStr, dTEMPSTRLEN);
    ChDir(unpackPathStr);
    *unpackPathStr = 0;
  }
  if (arcType == 8)  // UC2
  {
    memmove(unpackPathStr + 1, unpackPathStr, strlen(unpackPathStr) + 1);
    *unpackPathStr = '#';
  }
  if (execute(extPtr, pathStr, parStr, fullFileName, unpackPathStr, NULL))
  {
    if (arcType == 4)  // ZOO
      ChDir(dirStr);

    Delete(config.inPath, "*.pkt");
    logEntryf(LOG_ALWAYS, 0, "Cannot unpack bundle %s", fullFileName);
    newLine();
    return;
  }
  if (arcType == 4)  // ZOO
  {
    if (strlen(unpackPathStr) > 3)
      strcat(unpackPathStr, "\\");

    ChDir(dirStr);
  }
  if (arcType == 8) // UC2
    strcpy(unpackPathStr, unpackPathStr + 1);

  // postprocessor
  if (*config.postUnarc32.programName)
  {
    strcpy(tempStr, config.postUnarc32.programName);
    if (!checkExist(tempStr, procStr1, procStr2))
      execute("Post-Unpack", procStr1, procStr2, fullFileName, unpackPathStr, NULL);
  }

  if (!existPattern(config.inPath, "*.pkt") && !existPattern(config.inPath, "*.bcl"))
  {
    logEntryf(LOG_ALWAYS, 0, "No PKT or BCL file(s) found after un%sing file %s", extPtr, fullFileName);
    newLine();
    return;
  }
  unlink(fullFileName);
}
// ----------------------------------------------------------------------------
s16 handleNoCompress(const char *pktName, const char *qqqName, nodeNumType *destNode)
{
  logEntryf(LOG_ALWAYS, 0, "Node %s is on line: cannot compress mail", nodeStr(destNode));
  rename(pktName, qqqName);
  newLine();

  return 1;
}
// ----------------------------------------------------------------------------
s16 packArc(char *qqqName, nodeNumType *srcNode, nodeNumType *destNode, nodeInfoType *nodeInfo)
{
  u16           count;
  tempStrType   archiveStr;
  char         *archivePtr
             , *fnPtr
             , *extPtr;
  s16           oldArc = -1;
  u16           maxArc
              , okArc;
  u8            archiver = 0xFF;
  char          nodeName[32];
  DIR           *dir;
  struct dirent *ent;
  tempStrType   arcPath
              , tempStr
              , pktName
              , semaName;
  char         *helpPtr
              ,*helpPtr2
              , pathStr [64]
              , parStr  [MAX_PARSIZE]
              , procStr1[64]
              , procStr2[MAX_PARSIZE];
  fhandle       tempHandle
              , semaHandle = -1;
  long          arcSize = 0;
  const char   *exto
             , *extn
             , *cPtr = "";

#ifdef _DEBUG
  logEntryf(LOG_DEBUG, 0, "DEBUG packArc: file %s  %s -> %s  outStatus:%d", qqqName, nodeStr(srcNode), nodeStr(destNode), nodeInfo->outStatus);
#endif

  if (!stricmp(qqqName + strlen(qqqName) - 3, dEXTTMP))
  {
    exto = dEXTTMP;
    extn = "bcl";
  }
  else
  {
    exto = "qqq";
    extn = "pkt";
  }

  if (nodeInfo->archiver == 0xFF && *nodeInfo->pktOutPath)
  {
    // Write uncompressed pkt files directly to configured 'pkt outputpath'
    strcpy(pktName, nodeInfo->pktOutPath);
    if ((helpPtr = strrchr(qqqName, '\\')) == NULL)
      return 1;

    strcat(pktName, helpPtr + 1);
    strcpy(pktName + strlen(pktName) - 3, extn);
    if (moveFile(qqqName, pktName))
    {
      // If moving didn't succeed, rename to 'exto' extension. Don't care about the result
      helpPtr = stpcpy(pktName, qqqName);
      strcpy(helpPtr - 3, exto);
      rename(qqqName, pktName);

      return 1;
    }
    outboundBackup(pktName);
    return 0;
  }

  // Rename pkt file to correct extension
  helpPtr = stpcpy(pktName, qqqName);
  strcpy(helpPtr - 3, extn);
  if (rename(qqqName, pktName) != 0)
    return 1;

  strcpy(qqqName + strlen(qqqName) - 3,  exto);

  // If via, change src, dst and nodeInfo accordingly
  if (nodeInfo->viaNode.zone)
  {
    destNode = &nodeInfo->viaNode;
    nodeInfo = getNodeInfo(destNode);
    srcNode  = &config.akaList[matchAka(destNode, nodeInfo->useAka)].nodeNum;
  }

  // Check/create semaphore's for different mailers
  if (config.mailer == dMT_FrontDoor)
  {
    helpPtr = stpcpy(semaName, config.semaphorePath);
    count = sprintf(helpPtr, "%08x.`FM", crc32(nodeStr(destNode))) - 2;
    unlink(semaName);
    *(u16 *)(helpPtr + count) = '*';
    if (existPattern(config.semaphorePath, helpPtr))
      return handleNoCompress(pktName, qqqName, destNode);

    if (config.mailOptions.createSema)
    {
      strcpy(helpPtr + count, "FM");
      if ((semaHandle = open(semaName, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0664)) == -1)
        return handleNoCompress(pktName, qqqName, destNode);
    }
  }
  else
  {
    if (config.mailer == dMT_InterMail)
    {
      helpPtr2 = stpcpy(semaName, config.semaphorePath);
      sprintf(semaName, "X%07X.FM", crc32len((char *)destNode, 8) / 16);
      helpPtr = strrchr(semaName, '.');
      *helpPtr = *(helpPtr - 1);
      *(helpPtr - 1) = '.';
      unlink(semaName);
      *(u16 *)(helpPtr + 1) = '*';
      if (existPattern(config.semaphorePath, helpPtr2))
        return handleNoCompress(pktName, qqqName, destNode);

      if (config.mailOptions.createSema)
      {
        strcpy(helpPtr + 1, "FM");
        if ((semaHandle = open(semaName, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0664)) == -1)
          return handleNoCompress(pktName, qqqName, destNode);
      }
    }
    else
      if (config.mailer == dMT_Binkley || config.mailer == dMT_Xenia)
      {
        archivePtr = stpcpy(semaName, config.outPath);
        if (destNode->zone != config.akaList[0].nodeNum.zone)
        {
          archivePtr += sprintf(archivePtr - 1, ".%03hx", destNode->zone);
          mkdir(semaName);
          *(archivePtr - 1) = '\\';
        }
        if (destNode->point)
        {
          archivePtr += sprintf(archivePtr, "%04hx%04hx.pnt", destNode->net, destNode->node);
          mkdir(semaName);
          sprintf(archivePtr, "\\%08hx.bsy", destNode->point);
        }
        else
          sprintf(archivePtr, "%04hx%04hx.bsy", destNode->net, destNode->node);

        // Don't use config.mailOptions.createSema! FTS-5005 specifies it needs to be set
        if ((semaHandle = open(semaName, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0664)) == -1)
          return handleNoCompress(pktName, qqqName, destNode);
      }
  }

  // Make output path
  archivePtr = stpcpy(archiveStr, config.outPath);
  if (config.mailer == dMT_Binkley || config.mailer == dMT_Xenia)
  {
    if (destNode->zone != config.akaList[0].nodeNum.zone)
    {
      // Set specific BINK paths extensions for out of zone destinations
      archivePtr += sprintf(archivePtr - 1, ".%03hx", destNode->zone);
      mkdir(archiveStr);
      strcpy(archivePtr - 1, "\\");
    }
    if (destNode->point)
    {
      // Set specific BINK paths extensions for point destinations
      archivePtr += sprintf(archivePtr, "%04hx%04hx.pnt", destNode->net, destNode->node);
      mkdir(archiveStr);
      strcpy(archivePtr++, "\\");
    }
  }

  // When the .pkt needs to be packed with an archiver
  if (nodeInfo->archiver != 0xFF)
  {
    // Check if there already is an archive created in this session, destined for the same node
    count = 0;
    do
    {
      while (count < fAttCount
            && (  memcmp(&fAttInfo[count].origNode, srcNode , sizeof(nodeNumType)) != 0
               || memcmp(&fAttInfo[count].destNode, destNode, sizeof(nodeNumType)) != 0
               )
            )
        count++;

      if (count < fAttCount)
      {
        strcpy(archivePtr, fAttInfo[count].fileName);
        if ((archiver = archiveType(archiveStr)) == 0xFE || archiver == 0xFF)
          count++;
      }
    } while (count < fAttCount && (archiver == 0xFE || archiver == 0xFF));

    if (count < fAttCount)
      oldArc = count;
    else
    {
      archiver = nodeInfo->archiver;

      // Create archive name
      if ((!config.mailOptions.neverARC060)
          && (((destNode->zone == config.akaList[0].nodeNum.zone)
               || config.mailOptions.ARCmail060
               || !(nodeInfo->capability & PKT_TYPE_2PLUS))
              && (srcNode->point == 0)
              && (destNode->point == 0)))
        extPtr = archivePtr + sprintf( archivePtr, "%04hX%04hX"                // Keep archive name uppercase, because an external archiver might change it to uppercase
                                     , (*srcNode).net  - (*destNode).net
                                     , (*srcNode).node - (*destNode).node);
      else
      {
        sprintf( nodeName, "%hu:%hu/%hu.%hu", destNode->zone
               , config.akaList[0].nodeNum.point + destNode->net
               , destNode->node - config.akaList[0].nodeNum.net
               , config.akaList[0].nodeNum.node  + destNode->point);
        extPtr = archivePtr + sprintf(archivePtr, "%08X", crc32(nodeName));  // idem
      }

      sprintf(extPtr, ".%.2s?", "SUMOTUWETHFRSA" + (timeBlock.tm_wday << 1)); // idem

      // Search for existing archive files
      maxArc = '0' - 1;
      okArc = 0;

      *(archivePtr - 1) = 0;
      dir = opendir(archiveStr);
      *(archivePtr - 1) = '\\';
      if (dir != NULL)
      {
        while ((ent = readdir(dir)) != NULL)
          if (match_spec(archivePtr, ent->d_name))
          {
            char ep3 = extPtr[3] = ent->d_name[11];
            off_t fsize = fileSize(archiveStr);

            maxArc = max(ep3, maxArc);
            if (  (config.mailer == dMT_Binkley || config.mailer == dMT_Xenia)
               && (  fsize > 0
                  && (  config.maxBundleSize == 0
                     || (fsize >> 10) < config.maxBundleSize
                     )
                  )
               )
              okArc = max(ep3, okArc);

            if (fsize == 0
               && ( (!config.mailOptions.extNames && ep3 == '9')
                  || (config.mailOptions.extNames && ep3 == 'Z')
                  )
               )
              unlink(archiveStr);

            extPtr[3] = '?';  // Needed for match_spec()
          }

        closedir(dir);
      }

      fnPtr = archivePtr;
      for (count = 0; count < fAttCount; count++)
        if (strnicmp(fnPtr, fAttInfo[count].fileName, 11) == 0)
          maxArc = max(fAttInfo[count].fileName[11], maxArc);

      if (okArc && (config.mailer == dMT_Binkley || config.mailer == dMT_Xenia))
        extPtr[3] = okArc;
      else
      {
        if (config.mailOptions.extNames)
          switch (maxArc)
          {
            case '9':
              extPtr[3] = 'A';
              break;

            case 'S':
              extPtr[3] = 'U';
              break;

            case 'Z':
              extPtr[3] = maxArc;
              break;

            default:
              extPtr[3] = maxArc + 1;
              break;
          }
        else
          extPtr[3] = min('9', maxArc + 1);
        if (maxArc + 1 == '0')
          nodeInfo->lastNewBundleDat = nodeInfo->referenceLNBDat = startTime;
      }
    }

    switch (archiver)
    {
      case  0:      // ARC
        strcpy(arcPath, config.arc32.programName);
        cPtr = "arc";
        break;

      case  1:      // ZIP
        strcpy(arcPath, config.zip32.programName);
        cPtr = "zip";
        break;

      case  2:      // LZH
        strcpy(arcPath, config.lzh32.programName);
        cPtr = "lzh";
        break;

      case  3:      // PAK
        strcpy(arcPath, config.pak32.programName);
        cPtr = "pak";
        break;

      case  4:      // ZOO
        strcpy(arcPath, config.zoo32.programName);
        cPtr = "zoo";
        break;

      case  5:      // ARJ
        strcpy(arcPath, config.arj32.programName);
        cPtr = "arj";
        break;

      case  6:      // SQZ
        strcpy(arcPath, config.sqz32.programName);
        cPtr = "sqz";
        break;

      case  7:      // User
        strcpy(arcPath, config.customArc32.programName);
        cPtr = "Custom";
        break;

      case  8:      // UC2
        strcpy(arcPath, config.uc232.programName);
        cPtr = "uc2";
        break;

      case  9:      // RAR
        strcpy(arcPath, config.rar32.programName);
        cPtr = "rar";
        break;

      case 10:      // JAR
        strcpy(arcPath, config.jar32.programName);
        cPtr = "jar";
        break;

      default:
        logEntryf(LOG_ALWAYS, 0, "Unknown archiving utility listed for node %s", nodeStr(destNode));
        if (semaHandle >= 0)
        {
          close(semaHandle);
          unlink(semaName);
        }
        rename(pktName, qqqName);
        newLine();
        return 1;
    }

    if (!*arcPath || checkExist(arcPath, pathStr, parStr))
    {
      if (!*arcPath)
        logEntryf(LOG_ALWAYS, 0, "Archive compression program for %s method not defined", cPtr);
      else
        logEntryf(LOG_ALWAYS, 0, "Archive compression program for %s method not found"  , cPtr);

      if (semaHandle >= 0)
      {
        close(semaHandle);
        unlink(semaName);
      }
      rename(pktName, qqqName);
      newLine();
      return 1;
    }
  }
  else  // No archiver used
  {
    if ((helpPtr = strrchr(pktName, '\\')) == NULL)
      return 1;

    strcpy(archivePtr, helpPtr + 1);
  }

  outboundBackup(pktName);

  // preprocessor
  if (*config.preArc32.programName)
  {
    strcpy(tempStr, config.preArc32.programName);
    if (!checkExist(tempStr, procStr1, procStr2))
      execute("Pre-Pack", procStr1, procStr2, archiveStr, pktName, nodeInfo->pktOutPath);
  }

  okArc = 1;
  if (nodeInfo->archiver != 0xFF)
  {
    if (execute(cPtr, pathStr, parStr, archiveStr, pktName, nodeInfo->pktOutPath))
      okArc = 0;
  }
  else
  {
    if (strcmp(archiveStr, pktName) != 0 && moveFile(pktName, archiveStr))
      okArc = 0;
  }
  if (!okArc)
  {
    if (semaHandle >= 0)
    {
      close(semaHandle);
      unlink(semaName);
    }
    rename(pktName, qqqName);
    newLine();
    return 1;
  }

  // postprocessor
  if (*config.postArc32.programName)
  {
    strcpy(tempStr, config.postArc32.programName);
    if (!checkExist(tempStr, procStr1, procStr2))
      execute("Post-Pack",  procStr1, procStr2, archiveStr, pktName,  nodeInfo->pktOutPath);
  }

  if (nodeInfo->archiver != 0xFF)
  {
    arcSize = 0;
    if ((tempHandle = open(archiveStr, O_RDONLY | O_BINARY)) != -1)
    {
      if ((arcSize = filelength(tempHandle)) == -1)
        arcSize = 0;
      close(tempHandle);
    }
  }
  if (oldArc >= 0)  // oldArc == -1 if nodeInfo->archiver == 0xFF
  {
    if (config.maxBundleSize != 0 && (arcSize >> 10) >= config.maxBundleSize)
      memcpy(&fAttInfo[oldArc], &fAttInfo[oldArc + 1], sizeof(fAttInfoType) * (--fAttCount - oldArc));

    logEntryf(LOG_OUTBOUND, 0, "Mail bundle already going from %s to %s", nodeStr(srcNode), nodeStr(destNode));
  }
  else
  {
    if (config.mailer != dMT_Binkley && config.mailer != dMT_Xenia)
    {
      // Het file attach bericht moet sommige flags (direct, immediate, crash) overnemen van de toegevoegde berichten
      if (fileAttach(archiveStr, srcNode, destNode, nodeInfo))
      {
        if (semaHandle >= 0)
        {
          close(semaHandle);
          unlink(semaName);
        }
        rename(pktName, qqqName);
        newLine();

        return 1;
      }
    }
    else
    {
      strcpy(tempStr, archiveStr);
      if (destNode->point)
        extPtr = archivePtr + sprintf(archivePtr, "%08hx", destNode->point);
      else
        extPtr = archivePtr + sprintf(archivePtr, "%04hx%04hx", destNode->net, destNode->node);

      strcpy(extPtr, ".?lo");

      ent = NULL;
      if ((dir = opendir(config.outPath)) != NULL)
      {
        while ((ent = readdir(dir)) != NULL)
          if (match_spec(archivePtr, ent->d_name))
          {
            strcpy(extPtr, strchr(ent->d_name, '.'));
            break;
          }
        closedir(dir);
      }
      if (ent == NULL)
        sprintf(  extPtr, ".%clo"
               ,  nodeInfo->outStatus == 1  ? 'h'
               : (nodeInfo->outStatus == 2) ? 'c'
               : (nodeInfo->outStatus >= 3 && nodeInfo->outStatus <= 5) ? 'c' : 'f');

      if ((tempHandle = open(archiveStr, O_RDWR | O_CREAT | O_APPEND | O_BINARY, S_IREAD | S_IWRITE)) == -1)
      {
        if (semaHandle >= 0)
        {
          close(semaHandle);
          unlink(semaName);
        }
        rename(pktName, qqqName);
        newLine();

        return 1;
      }
      strcpy(archiveStr, tempStr);
      memset(arcPath, 0x20, sizeof(tempStrType) - 2);
      arcPath[sizeof(tempStrType) - 2] = 0;
      do
      {
        memcpy(arcPath , arcPath + sizeof(tempStrType) / 2 - 1, sizeof(tempStrType) / 2 - 1);
        read(tempHandle, arcPath + sizeof(tempStrType) / 2 - 1, sizeof(tempStrType) / 2 - 1);
      }
      while ((helpPtr = strstr(arcPath, archiveStr)) == NULL && !eof(tempHandle));

      if (helpPtr == NULL)
      {
        lseek(tempHandle, 0, SEEK_END);
        sprintf(tempStr, "%c%s\r\n", nodeInfo->archiver != 0xFF ? '#' : '^', archiveStr);  // Truncate or Delete the file after transmission depends on archived or not
        write(tempHandle, tempStr, strlen(tempStr));
        sprintf(tempStr, "Sending new mail from %s to %s", nodeStr(srcNode), nodeStr(destNode));
      }
      else
        sprintf(tempStr, "Mail bundle already going from %s to %s", nodeStr(srcNode), nodeStr(destNode));

      close(tempHandle);
      logEntry(tempStr, LOG_OUTBOUND, 0);
    }

    if (  nodeInfo->archiver != 0xFF
       && fAttCount < MAX_ATTACH
       && (config.maxBundleSize == 0 || (arcSize >> 10) < config.maxBundleSize)
       )
    {
      fAttInfo[fAttCount].origNode = *srcNode;
      fAttInfo[fAttCount].destNode = *destNode;
      strcpy(fAttInfo[fAttCount++].fileName, archiveStr + strlen(config.outPath));
    }
  }
  if (nodeInfo->archiver != 0xFF)
    unlink(pktName);

  // Close and Delete semaphore file
  if (semaHandle >= 0)
  {
    close(semaHandle);
    unlink(semaName);
  }
  // flush();
  return 0;
}
//---------------------------------------------------------------------------
// Retrying to compress outgoing mailpacket(s), with .qqq extension
//
void retryArc(void)
{
  tempStrType    tempStr;
  nodeNumType    srcNode
               , destNode;
  pktHdrType     msgPktHdr;
  fhandle        pktHandle;
  // s16 donePkt = 0;
  // struct _finddata_t fd;
  // long fdHandle;
  DIR           *dir;
  struct dirent *ent;
  nodeNumType    destNode4d;
  int            logged = 0;

  strcpy(tempStr, config.outPath);
  strcat(tempStr, "*.qqq");

  if ((dir = opendir(config.outPath)) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
      if (match_spec("*.qqq", ent->d_name))
      {
        if (!logged)
        {
          logEntry("Retrying to compress outgoing mailpacket(s)", LOG_OUTBOUND, 0);
          logged = 1;
        }

        strcpy(stpcpy(tempStr, config.outPath), ent->d_name);

        if ( (pktHandle = open(tempStr, O_RDONLY | O_BINARY)) != -1
           && read(pktHandle, &msgPktHdr, sizeof(pktHdrType)) == (int)sizeof(pktHdrType)
           && close(pktHandle) != -1
           )
        {
          srcNode.zone   = msgPktHdr.origZone;
          srcNode.net    = msgPktHdr.origNet;
          srcNode.node   = msgPktHdr.origNode;
          srcNode.point  = msgPktHdr.origPoint;
          destNode.zone  = msgPktHdr.destZone;
          destNode.net   = msgPktHdr.destNet;
          destNode.node  = msgPktHdr.destNode;
          destNode.point = msgPktHdr.destPoint;

          destNode4d = destNode;
          node4d(&destNode4d);

          packArc(tempStr, &srcNode, &destNode, getNodeInfo(&destNode4d));
        }
      }

    newLine();
    closedir(dir);
  }
}
// ----------------------------------------------------------------------------
