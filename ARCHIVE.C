// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------

#ifdef __BORLANDC__
#include <alloc.h>
#endif  // __BORLANDC__
#include <dir.h>
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef __MINGW32__
#include <windef.h>  // min() max()
#endif // __MINGW32__

#include "archive.h"

#include "areainfo.h"
#include "config.h"
#include "crc.h"
#include "log.h"
#include "msgmsg.h"
#include "msgpkt.h"
#include "nodeinfo.h"
#include "stpcpy.h"
#include "utils.h"

#ifndef __FMAILX__
#ifndef __32BIT__
#include "spawno.h"
#endif
#endif

#ifdef __32BIT__
#define MAX_PARSIZE 256
#else
#define MAX_PARSIZE 128
#endif
// ----------------------------------------------------------------------------

extern s16 ems;
extern time_t startTime;
extern struct tm timeBlock;
extern configType config;

extern fAttInfoType fAttInfo[MAX_ATTACH];
extern u16 fAttCount;

extern const char *months;

// ----------------------------------------------------------------------------
static s16 checkExist(char *entry, char *prog, char *par)
{
  tempStrType tempStr;
  const char *helpPtr;

  if (  (helpPtr = strtok(entry, " ")) == NULL
     || (access(helpPtr, 4) != 0 && (helpPtr = _searchpath(helpPtr)) == NULL)
     || strnicmp(helpPtr, config.inPath, strlen(config.inPath)) == 0
     )
  {
    sprintf(tempStr, "%s not found", entry);
    logEntry(tempStr, LOG_ALWAYS, 0);
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

  if ((arcHandle = openP(fullFileName, O_RDONLY | O_BINARY | O_DENYALL, S_IREAD | S_IWRITE)) == -1)
    return 0xFE;

  byteCount = _read(arcHandle, data, 29);
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
#if !defined __WIN32__ && !defined __OS2__
static s16 _Cdecl spawnlo(const char *overlay_path, const char *prog_name, ...)
{
  return __spawnv(overlay_path, prog_name, _va_ptr, 0);
}
#endif
// ----------------------------------------------------------------------------
static s16 execute(const char *arcType, const char *program, const char *parameters, const char *archiveName, char *pktName, const char *privPath
#if !defined __FMAILX__ && !defined __32BIT__
                  , u16 memReq
#endif
                  )
{
  s16         dosExitCode = -1;
  char        tempStr[MAX_PARSIZE];
  tempStrType tempPath;
  char       *helpPtr;
  u16         len = 0;
  clock_t     ct = clock();
#if !defined __FMAILX__ && !defined __32BIT__
  fhandle     EMSHandle;
#endif

  while ((helpPtr = strstr(parameters, "\%a")) != NULL)
  {
    len = 1;
    if (strlen(parameters) + strlen(archiveName) >= MAX_PARSIZE)
    {
      sprintf(tempStr, "Argument list of %s utility is too big", arcType);
      logEntry(tempStr, LOG_ALWAYS, 0);
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
        sprintf(tempStr, "Argument list of %s utility is too big", arcType);
        logEntry(tempStr, LOG_ALWAYS, 0);
        return 1;
      }
      strcpy( tempStr,                  helpPtr + 2);
      strcpy( stpcpy(helpPtr, pktName), tempStr);
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
        sprintf(tempStr, "Argument list of %s utility is too big", arcType);
        logEntry(tempStr, LOG_ALWAYS, 0);
        return 1;
      }
      strcpy(tempStr, helpPtr + 2);
      strcpy(stpcpy(helpPtr, tempPath), tempStr);
    }
    while ((helpPtr = strstr(parameters, "\%p")) != NULL);
  }

  if (len == 0)
    sprintf(strchr(parameters, 0),  " %s %s", archiveName, pktName);

  sprintf(tempStr, "Executing %s %s", program, parameters);
  logEntry(tempStr, LOG_EXEC | LOG_NOSCRN, 0);

  printf("<-- %s Start -->\n", arcType);

#if !defined __FMAILX__ && !defined __32BIT__
  if ((config.genOptions.swap)
      && ((memReq == 0) || ((coreleft() >> 10) < memReq)))
  {
#ifdef DEBUG
    puts("SPAWNLO");
#endif
    if (ems)
    {
      _AH = 0x43;
      _BX = 1;                    /* 1 dummy page */
      geninterrupt(0x67);       /* Open handle */
      if (_AH == 0)
      {
        EMSHandle = _DX;
        _AH = 0x47;
        geninterrupt(0x67);
      }
      else
      {
        logEntry("Cannot save EMS pageframe before swapping", LOG_ALWAYS, 0);
        return 1;
      }
    }
    __spawn_xms = config.genOptions.swapXMS;

    handles20();
    dosExitCode = spawnlo(config.swapPath, program, program, parameters, NULL);
    handlesReset40();

    if (ems) /* Restore page frame */
    {
      _DX = EMSHandle;
      _AH = 0x48;
      geninterrupt(0x67);

      _DX = EMSHandle;
      _AH = 0x45;
      geninterrupt(0x67);
    }
  }
#endif
#ifndef __OS2__
#ifndef __NOSPAWNL__
// for Win16 compile
  if (dosExitCode == -1)
  {
    dosExitCode = spawnl(P_WAIT, program, program, parameters, NULL);
    // flush();
  }

#endif
#else
  printf("Running %s utility in an OS/2 shell...", arcType);
  if (  (dosExitCode = spawnl(P_WAIT, program, program, parameters, NULL)) == -1
     && errno == ENOEXEC)
  {
    static char *cmdPtr = NULL;

    putchar('\r');
    if (cmdPtr == NULL && (cmdPtr = getenv("OS2_SHELL")) == NULL)
    {
      logEntry("Can't find command shell (OS2_SHELL environment var)", LOG_ALWAYS, 0);
      return 1;
    }
    printf("Running %s utility in a DOS shell...\n", arcType);
    dosExitCode = spawnl(P_WAIT, cmdPtr, cmdPtr, "/C", program, parameters, NULL);
  }
  else
    puts("\n");
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
void unpackArc(char *fullFileName, struct _finddata_t *fdArc)
{
  tempStrType tempStr;
  s16 temp;
  char *extPtr;
  tempStrType arcPath;
  char pathStr[64];
  char parStr[MAX_PARSIZE];
  char procStr1[64];
  char procStr2[MAX_PARSIZE];
  tempStrType unpackPathStr;
  char dirStr[128];
  struct _finddata_t fdPkt;
  s16 arcType;
#if (!defined(__FMAILX__) && !defined(__32BIT__))
  u16 memReq;
#endif
  *arcPath = 0;
  switch (arcType = archiveType(fullFileName))
  {
  case  0:      // ARC
#ifdef __32BIT__
    strcpy(arcPath, config.unArc32.programName);
#else
    strcpy(arcPath, config.unArc.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.unArc.memRequired;
#endif
    extPtr = (char*)"arc";
    break;

  case  1:      // ZIP
#ifdef __32BIT__
    strcpy(arcPath, config.unZip32.programName);
#else
    strcpy(arcPath, config.unZip.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.unZip.memRequired;
#endif
    extPtr = (char*)"zip";
    break;

  case  2:      // LZH
#ifdef __32BIT__
    strcpy(arcPath, config.unLzh32.programName);
#else
    strcpy(arcPath, config.unLzh.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.unLzh.memRequired;
#endif
    extPtr = (char*)"lzh";
    break;

  case  3:      // PAK
#ifdef __32BIT__
    strcpy(arcPath, config.unPak32.programName);
#else
    strcpy(arcPath, config.unPak.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.unPak.memRequired;
#endif
    extPtr = (char*)"pak";
    break;

  case  4:      // ZOO
#ifdef __32BIT__
    strcpy(arcPath, config.unZoo32.programName);
#else
    strcpy(arcPath, config.unZoo.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.unZoo.memRequired;
#endif
    extPtr = (char*)"zoo";
    break;

  case  5:      // ARJ
#ifdef __32BIT__
    strcpy(arcPath, config.unArj32.programName);
#else
    strcpy(arcPath, config.unArj.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.unArj.memRequired;
#endif
    extPtr = (char*)"arj";
    break;

  case  6:      // SQZ
#ifdef __32BIT__
    strcpy(arcPath, config.unSqz32.programName);
#else
    strcpy(arcPath, config.unSqz.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.unSqz.memRequired;
#endif
    extPtr = (char*)"sqz";
    break;

  case  8:      // UC2
#ifdef __32BIT__
    strcpy(arcPath, config.unUc232.programName);
#else
    strcpy(arcPath, config.unUc2.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.unUc2.memRequired;
#endif
    extPtr = (char*)"uc2";
    break;

  case  9:      // RAR
#ifdef __32BIT__
    strcpy(arcPath, config.unRar32.programName);
#else
    strcpy(arcPath, config.unRar.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.unRar.memRequired;
#endif
    extPtr = (char*)"rar";
    break;

  case 10:      // JAR
#ifdef __32BIT__
    strcpy(arcPath, config.unJar32.programName);
#else
    strcpy(arcPath, config.unJar.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.unJar.memRequired;
#endif
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
        sprintf(tempStr, "Unknown archiving utility used for %s"                  , fullFileName);
      else
        sprintf(tempStr, "Archive decompression program for %s method not defined", extPtr);

      logEntry(tempStr, LOG_ALWAYS, 0);

      return;
    }
    strcpy(arcPath, config.GUS.programName);
#if (!defined(__FMAILX__) && !defined(__32BIT__))
    memReq = config.GUS.memRequired;
#endif
    extPtr = (char*)"gus";
  }
  if (checkExist(arcPath, pathStr, parStr))
  {
    sprintf(tempStr, "Archive decompression program for %s method not found", extPtr);
    logEntry(tempStr, LOG_ALWAYS, 0);
    return;
  }
  sprintf(tempStr, "Decompressing %s (%s)", fullFileName, extPtr);
  logEntry(tempStr, LOG_INBOUND, 0);
  {
    struct tm *tm = localtime(&fdArc->time_write);
    sprintf(tempStr, "Archive info: %ld, %04u-%02u-%02u %02u:%02u:%02u"
                   , fdArc->size
                   , tm->tm_year + 1900, tm->tm_mon +1, tm->tm_mday
                   , tm->tm_hour, tm->tm_min, tm->tm_sec);
    logEntry(tempStr, LOG_INBOUND, 0);
  }
  strcpy(unpackPathStr, config.inPath);  // pktPath

  // preprocessor
#ifdef __32BIT__
  if (*config.preUnarc32.programName)
  {
    strcpy(tempStr, config.preUnarc32.programName);
#else
  if (*config.preUnarc.programName)
  {
    strcpy(tempStr, config.preUnarc.programName);
#endif
    if (!checkExist(tempStr, procStr1, procStr2))
    {
#if (defined __FMAILX__ || defined __32BIT__)
      execute("Pre-Unpack", procStr1, procStr2, fullFileName, unpackPathStr,  NULL);
#else
      execute("Pre-Unpack", procStr1, procStr2, fullFileName, unpackPathStr,  NULL, config.preUnarc.memRequired);
#endif
    }
  }
  if (arcType == 4)  // ZOO
  {
    if ((temp = strlen(unpackPathStr)) > 3)
      unpackPathStr[temp - 1] = 0;

    getcwd(dirStr, 128);
    ChDir(unpackPathStr);
    *unpackPathStr = 0;
  }
  if (arcType == 8)  // UC2
  {
    memmove(unpackPathStr + 1, unpackPathStr, strlen(unpackPathStr) + 1);
    *unpackPathStr = '#';
  }
#if (defined __FMAILX__ || defined __32BIT__)
  if (execute(extPtr, pathStr, parStr, fullFileName, unpackPathStr, NULL))
#else
  if (execute(extPtr, pathStr, parStr, fullFileName, unpackPathStr, NULL, memReq))
#endif
  {
    if (arcType == 4)  // ZOO
      ChDir(dirStr);

    Delete(config.inPath, "*.pkt");
    sprintf(tempStr, "Cannot unpack bundle %s", fullFileName);
    logEntry(tempStr, LOG_ALWAYS, 0);
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
#ifdef __32BIT__
  if (*config.postUnarc32.programName)
  {
    strcpy(tempStr, config.postUnarc32.programName);
#else
  if (*config.postUnarc.programName)
  {
    strcpy(tempStr, config.postUnarc.programName);
#endif
    if (!checkExist(tempStr, procStr1, procStr2))
    {
#if (defined __FMAILX__ || defined __32BIT__)
      execute("Post-Unpack", procStr1, procStr2, fullFileName, unpackPathStr, NULL);
#else
      execute("Post-Unpack", procStr1, procStr2, fullFileName, unpackPathStr, NULL, config.postUnarc.memRequired);
#endif
    }
  }

  strcpy(stpcpy(tempStr, config.inPath), "*.pkt");
  strcpy(stpcpy(dirStr , config.inPath), "*.bcl");
  if (_findfirst(tempStr, &fdPkt) == -1 && _findfirst(dirStr, &fdPkt) == -1)
  {
    sprintf(tempStr, "No PKT or BCL file(s) found after un%sing file %s", extPtr, fullFileName);
    logEntry(tempStr, LOG_ALWAYS, 0);
    newLine();
    return;
  }
  unlink(fullFileName);
}
// ----------------------------------------------------------------------------
s16 handleNoCompress(const char *pktName, const char *qqqName, nodeNumType *destNode)
{
  tempStrType tempStr;

  sprintf(tempStr, "Node %s is on line: cannot compress mail", nodeStr(destNode));
  logEntry(tempStr, LOG_ALWAYS, 0);
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
             , *fnPtr;
  char         *extPtr;
  s16           oldArc = -1;
  u16           maxArc
              , okArc;
  u8            archiver = 0xFF;
  char          nodeName[32];
  struct _finddata_t fd;
  long          fdHandle;
  tempStrType   arcPath
              , tempStr
              , pktName
              , semaName;
  char         *helpPtr
              , pathStr [64]
              , parStr  [MAX_PARSIZE]
              , procStr1[64]
              , procStr2[MAX_PARSIZE];
//s16           doneArc;
  fhandle       tempHandle
              , semaHandle = -1;
  s32           arcSize;
  const char   *exto
             , *extn
             , *cPtr;
#if (!defined(__FMAILX__) && !defined(__32BIT__))
  u16           memReq;
#endif

#ifdef _DEBUG
  sprintf(tempStr, "[packArc Debug] file %s  %s -> %s  outStatus:%d", qqqName, nodeStr(srcNode), nodeStr(destNode), nodeInfo->outStatus);
  logEntry(tempStr, LOG_DEBUG, 0);
#endif

  if (!stricmp(qqqName + strlen(qqqName) - 3, "$$$"))
  {
    exto = "$$$";
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
    count = sprintf(semaName, "%s%08lx.`FM", config.semaphorePath, crc32(nodeStr(destNode))) - 2;
    unlink(semaName);
    *(u16 *)(semaName + count) = '*';
    if ((fdHandle = _findfirst(semaName, &fd)) != -1)
      return handleNoCompress(pktName, qqqName, destNode);

    _findclose(fdHandle);

    if (config.mailOptions.createSema)
    {
      strcpy(semaName + count, "FM");
      if ((semaHandle = openP(semaName, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0664)) == -1)
        return handleNoCompress(pktName, qqqName, destNode);
    }
  }
  else
  {
    if (config.mailer == dMT_InterMail)
    {
      sprintf(semaName, "%sX%07lX.FM", config.semaphorePath, crc32len((char *)destNode, 8) / 16);
      helpPtr = strrchr(semaName, '.');
      *helpPtr = *(helpPtr - 1);
      *(helpPtr - 1) = '.';
      unlink(semaName);
      *(u16 *)(helpPtr + 1) = '*';
      if ((fdHandle = _findfirst(semaName, &fd)) != -1)
        return handleNoCompress(pktName, qqqName, destNode);

      _findclose(fdHandle);

      if (config.mailOptions.createSema)
      {
        strcpy(helpPtr + 1, "FM");
        if ((semaHandle = openP(semaName, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0664)) == -1)
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
        if ((semaHandle = openP(semaName, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0664)) == -1)
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
        extPtr = archivePtr + sprintf(archivePtr, "%04hX%04hX"                // Keep archive name uppercase, because an external archiver might change it to uppercase
                                     , (*srcNode).net  - (*destNode).net
                                     , (*srcNode).node - (*destNode).node);
      else
      {
        sprintf(nodeName, "%hu:%hu/%hu.%hu", destNode->zone
               , config.akaList[0].nodeNum.point + destNode->net
               , destNode->node - config.akaList[0].nodeNum.net
               , config.akaList[0].nodeNum.node  + destNode->point);
        extPtr = archivePtr + sprintf(archivePtr, "%08lX", crc32(nodeName));  // idem
      }

      sprintf(extPtr, ".%.2s?", "SUMOTUWETHFRSA" + (timeBlock.tm_wday << 1)); // idem

      // Search for existing archive files
      maxArc = '0' - 1;
      okArc = 0;
      fdHandle = _findfirst(archiveStr, &fd);

      if (fdHandle != -1)
      {
        do
        {
          maxArc = max(fd.name[11], maxArc);
          if ((config.mailer == dMT_Binkley || config.mailer == dMT_Xenia)
              && ((fd.size > 0)
                  && ((config.maxBundleSize == 0)
                      || ((fd.size >> 10) < config.maxBundleSize))))
            okArc = max(fd.name[11], okArc);
          if (fd.size == 0
             && ( (!config.mailOptions.extNames && fd.name[11] == '9')
                || (config.mailOptions.extNames && fd.name[11] == 'Z')
                )
             )
          {
            extPtr[3] = fd.name[11];
            unlink(archiveStr);
          }
        } while (_findnext(fdHandle, &fd) == 0);

        _findclose(fdHandle);
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
#ifdef __32BIT__
        strcpy(arcPath, config.arc32.programName);
#else
        strcpy(arcPath, config.arc.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.arc.memRequired;
#endif
        cPtr = "arc";
        break;

      case  1:      // ZIP
#ifdef __32BIT__
        strcpy(arcPath, config.zip32.programName);
#else
        strcpy(arcPath, config.zip.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.zip.memRequired;
#endif
        cPtr = "zip";
        break;

      case  2:      // LZH
#ifdef __32BIT__
        strcpy(arcPath, config.lzh32.programName);
#else
        strcpy(arcPath, config.lzh.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.lzh.memRequired;
#endif
        cPtr = "lzh";
        break;

      case  3:      // PAK
#ifdef __32BIT__
        strcpy(arcPath, config.pak32.programName);
#else
        strcpy(arcPath, config.pak.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.pak.memRequired;
#endif
        cPtr = "pak";
        break;

      case  4:      // ZOO
#ifdef __32BIT__
        strcpy(arcPath, config.zoo32.programName);
#else
        strcpy(arcPath, config.zoo.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.zoo.memRequired;
#endif
        cPtr = "zoo";
        break;

      case  5:      // ARJ
#ifdef __32BIT__
        strcpy(arcPath, config.arj32.programName);
#else
        strcpy(arcPath, config.arj.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.arj.memRequired;
#endif
        cPtr = "arj";
        break;

      case  6:      // SQZ
#ifdef __32BIT__
        strcpy(arcPath, config.sqz32.programName);
#else
        strcpy(arcPath, config.sqz.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.sqz.memRequired;
#endif
        cPtr = "sqz";
        break;

      case  7:      // User
#ifdef __32BIT__
        strcpy(arcPath, config.customArc32.programName);
#else
        strcpy(arcPath, config.customArc.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.customArc.memRequired;
#endif
        cPtr = "Custom";
        break;

      case  8:      // UC2
#ifdef __32BIT__
        strcpy(arcPath, config.uc232.programName);
#else
        strcpy(arcPath, config.uc2.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.uc2.memRequired;
#endif
        cPtr = "uc2";
        break;

      case  9:      // RAR
#ifdef __32BIT__
        strcpy(arcPath, config.rar32.programName);
#else
        strcpy(arcPath, config.rar.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.rar.memRequired;
#endif
        cPtr = "rar";
        break;

      case 10:      // JAR
#ifdef __32BIT__
        strcpy(arcPath, config.jar32.programName);
#else
        strcpy(arcPath, config.jar.programName);
#endif
#if (!defined(__FMAILX__) && !defined(__32BIT__))
        memReq = config.jar.memRequired;
#endif
        cPtr = "jar";
        break;

      default:
        sprintf(tempStr, "Unknown archiving utility listed for node %s", nodeStr(destNode));
        logEntry(tempStr, LOG_ALWAYS, 0);
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
        sprintf(tempStr, "Archive compression program for %s method not defined", cPtr);
      else
        sprintf(tempStr, "Archive compression program for %s method not found"  , cPtr);

      logEntry(tempStr, LOG_ALWAYS, 0);
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

  // preprocessor
#ifdef __32BIT__
  if (*config.preArc32.programName)
  {
    strcpy(tempStr, config.preArc32.programName);
#else
  if (*config.preArc.programName)
  {
    strcpy(tempStr, config.preArc.programName);
#endif
    if (!checkExist(tempStr, procStr1, procStr2))
    {
#if (defined __FMAILX__ || defined __32BIT__)
      execute("Pre-Pack", procStr1, procStr2, archiveStr, pktName, nodeInfo->pktOutPath);
#else
      execute("Pre-Pack", procStr1, procStr2, archiveStr, pktName, nodeInfo->pktOutPath, config.preArc.memRequired);
#endif
    }
  }

  okArc = 1;
  if (nodeInfo->archiver != 0xFF)
  {
#if (defined __FMAILX__ || defined __32BIT__)
    if (execute(cPtr, pathStr, parStr, archiveStr, pktName, nodeInfo->pktOutPath))
#else
    if (execute(extPtr, pathStr, parStr, archiveStr, pktName, nodeInfo->pktOutPath, memReq))
#endif
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
#ifdef __32BIT__
  if (*config.postArc32.programName)
  {
    strcpy(tempStr, config.postArc32.programName);
#else
  if (*config.postArc.programName)
  {
    strcpy(tempStr, config.postArc.programName);
#endif
    if (!checkExist(tempStr, procStr1, procStr2))
    {
#if (defined __FMAILX__ || defined __32BIT__)
      execute("Post-Pack",  procStr1, procStr2, archiveStr, pktName,  nodeInfo->pktOutPath);
#else
      execute("Post-Pack",  procStr1, procStr2, archiveStr, pktName,  nodeInfo->pktOutPath, config.postArc.memRequired);
#endif
    }
  }

  if (nodeInfo->archiver != 0xFF)
  {
    arcSize = 0;
    if ((tempHandle = openP(archiveStr, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) != -1)
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

    sprintf(tempStr, "Mail bundle already going from %s to %s", nodeStr(srcNode), nodeStr(destNode));
    logEntry(tempStr, LOG_OUTBOUND, 0);
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
        archivePtr += sprintf(archivePtr, "%08hx", destNode->point);
      else
        archivePtr += sprintf(archivePtr, "%04hx%04hx", destNode->net, destNode->node);

      strcpy(archivePtr, ".?lo");
      if ((fdHandle = _findfirst(archiveStr, &fd)) == -1)
        sprintf(archivePtr, ".%clo"
               ,  nodeInfo->outStatus == 1  ? 'h'
               : (nodeInfo->outStatus == 2) ? 'c'
               : (nodeInfo->outStatus >= 3 && nodeInfo->outStatus <= 5) ? 'c' : 'f');
      else
      {
        _findclose(fdHandle);
        strcpy(archivePtr, strchr(fd.name, '.'));
      }

      if ((tempHandle = openP(archiveStr, O_RDWR | O_CREAT | O_APPEND | O_BINARY | O_DENYNONE, S_IREAD | S_IWRITE)) == -1)
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
#ifdef __WINDOWS32__
      strcpy(archiveStr, tempStr);
#else
      strcpy(archiveStr, strupr(tempStr));             // for 4DOS
#endif
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
  tempStrType tempStr;
  nodeNumType srcNode
            , destNode;
  pktHdrType msgPktHdr;
  fhandle pktHandle;
  s16 donePkt = 0;
  struct _finddata_t fd;
  long fdHandle;
  nodeNumType destNode4d;

  strcpy(tempStr, config.outPath);
  strcat(tempStr, "*.qqq");

  fdHandle = _findfirst(tempStr, &fd);

  if (fdHandle != -1)
  {
    logEntry("Retrying to compress outgoing mailpacket(s)", LOG_OUTBOUND, 0);

    while (!donePkt && !breakPressed)
    {
      if (!(fd.attrib & FA_SPEC))
      {
        strcpy(stpcpy(tempStr, config.outPath), fd.name);

        if ( (pktHandle = openP(tempStr, O_RDONLY | O_BINARY | O_DENYNONE, S_IREAD | S_IWRITE)) != -1
           && _read(pktHandle, &msgPktHdr, sizeof(pktHdrType)) == sizeof(pktHdrType)
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
      donePkt = _findnext(fdHandle, &fd);
    }
    newLine();
  }
}
// ----------------------------------------------------------------------------
