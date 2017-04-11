// ----------------------------------------------------------------------------
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2017 Wilfred van Velzen
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

#ifdef __WIN32__
#include <dir.h>
//#include <dos.h>
#include <process.h>
#include <share.h>
#endif // __WIN32__
#ifdef __linux__
#include <sys/types.h>  // wait()
#include <sys/wait.h>   // wait()
#endif // __linux__
#include <dirent.h>     // opendir()
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>     // access() fork()

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
#include "os.h"
#include "spec.h"
#include "os_string.h"
#include "utils.h"

#define MAX_PARSIZE 256
// ----------------------------------------------------------------------------

extern s16 ems;
extern configType config;

extern fAttInfoType fAttInfo[MAX_ATTACH];
extern u16 fAttCount;

extern const char *months;

// ----------------------------------------------------------------------------
static u8 archiveType(char *fullFileName)
{
  fhandle arcHandle;
  s16 byteCount;
  unsigned char data[29];

  if ((arcHandle = _sopen(fixPath(fullFileName), O_RDONLY | O_BINARY, SH_DENYRW)) == -1)
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
//----------------------------------------------------------------------------
void argReplace(char **tok, const char *arg, const char *replStr, char **buf, int *addArgs)
{
  char *p;

  if (replStr == NULL)
    replStr = "";

  while ((p = strstr(*tok, arg)) != NULL)
  {
    p = stpcpy(stpcpy(stpncpy(*buf, *tok, p - *tok), replStr), p + strlen(arg)) + 1;
    *tok = *buf;
    *buf = p;

    if (addArgs != NULL)
      *addArgs = 0;
  }
}
//----------------------------------------------------------------------------
#define dMAXARGS  20
static int execute(const char *arcType, const char *command, const char *archiveName, const char *pktName, const char *privPath)
{
  int         dosExitCode;
  tempStrType cmdBuf
            , parBuf
            , aName
            , pName
            , rName
            ;
  int         addArgs = 1;
  int         argc = 0;
  char       *argv[dMAXARGS];
  char       *tok;
  char       *pBufp = parBuf;
  u16         len = 0;
  clock_t     ct = clock();

  strcpy(cmdBuf, command);
  strcpy(aName, fixPath(archiveName));
  strcpy(pName, fixPath(pktName));
  if (pName != NULL && (len = strlen(pName)) > 0 && isDirSep(pName[len - 1]))
    pName[len - 1] = 0;
  strcpy(rName, fixPath(privPath));
  if (rName != NULL && (len = strlen(rName)) > 0 && isDirSep(rName[len - 1]))
    rName[len - 1] = 0;

  tok = strtok(cmdBuf, " ");
  while (tok != NULL)
  {
    argReplace(&tok, "%a", aName, &pBufp, &addArgs);
    argReplace(&tok, "%f", pName, &pBufp, &addArgs);
    argReplace(&tok, "%p", rName, &pBufp, NULL);

    argv[argc++] = tok;

    tok = strtok(NULL, " ");
  }
  if (argc == 0)
  {
    logEntryf(LOG_ALWAYS, 0, "Command for %s not specified!", arcType);
    return 1;
  }
  //argv[0] = (char*)fixPath(argv[0]);
  if (addArgs)
  {
    argv[argc++] = aName;
    argv[argc++] = pName;
  }
  argv[argc] = NULL;

#ifdef _DEBUG
  {
    int i;
    for (i = 0; i < argc; i++)
      logEntryf(LOG_DEBUG, 0, "Exec argv[%d]: \"%s\"", i, argv[i]);
  }
#else
  {
    tempStrType cStr;
    char *p = cStr;
    int i;
    for (i = 0; i < argc; i++)
    {
      p = stpcpy(p, argv[i]);
      *p++ = ' ';
    }
    *--p = 0;
    logEntryf(LOG_EXEC, 0, "Executing: \"%s\"", cStr);
  }
#endif // _DEBUG

  printf("<-- %s Start -->\n", arcType);
  fflush(stdout);

#if   defined(__WIN32__)
  dosExitCode = spawnvp(P_WAIT, argv[0], argv);
#elif defined(__linux__)
  dosExitCode = fork();

  switch (dosExitCode)
  {
    case -1:
      break;
    case 0:  // this is the code the child runs
    {
#ifdef _DEBUG0
      char *path = getenv("PATH");
      logEntry("DEBUG execute child", LOG_DEBUG, 0);
      if (path)
        logEntryf(LOG_DEBUG, 0, "DEBUG $PATH=\"%s\"", path);
#endif // _DEBUG
      execvp(argv[0], argv);
#ifdef _DEBUG0
      logEntry("DEBUG execute child: execvp return?", LOG_DEBUG, 0);
#endif // _DEBUG
      // execvp returns, so there must be an error!
      logEntryf(LOG_ALWAYS, 0, "Cannot execute %s utility: [%d] %s", arcType, errno, strError(errno));
      exit(EXIT_FAILURE);
    }
    default:  // this is the code the parent runs
    {
      int status;
      if (wait(&status) != -1 && WIFEXITED(status))
        dosExitCode = WEXITSTATUS(status);
      else
        dosExitCode = -1;
      break;
    }
  }
#else
  dosExitCode = -1;
  errno = ENOEXEC;
#endif
  // flush();

  {
    double d = ((double)(clock() - ct)) / CLOCKS_PER_SEC;
    printf("<-- %s End [%.*f] -->\n", arcType, d < .01 ? 4 : 3, d);
  }

  // flush();
  if (dosExitCode == -1)
  {
    logEntryf(LOG_ALWAYS, 0, "Cannot execute %s utility: [%d] %s", arcType, errno, strError(errno));
    return 1;
  }
  else if (dosExitCode != 0)
  {
    logEntryf(LOG_ALWAYS, 0, "Archiving utility for %s method exited with status: %u", arcType, dosExitCode);
    return 1;
  }
  return 0;
}
// ----------------------------------------------------------------------------
static void outboundBackup(const char *path)
{
  tempStrType bakPath;
  const char *p1
           , *p2;

  if (path == NULL || *path == 0 || *config.outBakPath == 0)
    return;

  p1 = fixPath(path);

  if ((p2 = strrchr(p1, dDIRSEPC)) == NULL)
    p2 = p1;
  else
    p2++;

  strcpy(stpcpy(bakPath, fixPath(config.outBakPath)), p2);
  if (copyFile(p1, bakPath, 0))
    logEntryf(LOG_OUTBOUND | LOG_PACK | LOG_PKTINFO, 0, "Backup %s -> %s", p1, bakPath);
  else
    logEntryf(LOG_OUTBOUND | LOG_PACK | LOG_PKTINFO, 0, "Backup Failed! %s -> %s", p1, bakPath);
}
// ----------------------------------------------------------------------------
void unpackArc(const struct bt *bp)
{
  tempStrType tempStr
            , fullFileName
            , arcPath
            , unpackPathStr
            , dirStr
//          , pathStr
            ;
  s16 temp;
  char *extPtr = "Unknown";
//  char parStr[MAX_PARSIZE];
//  char procStr1[64];
//  char procStr2[MAX_PARSIZE];
  s16 arcType;

  *arcPath = 0;
  strcpy(stpcpy(fullFileName, fixPath(config.inPath)), bp->fname);

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

  logEntryf(LOG_INBOUND, 0, "Decompressing %s (%s)", fullFileName, extPtr);
  {
    struct tm *tm = localtime(&bp->mtime);
    if (tm != NULL)
      sprintf(tempStr, "Archive info: %ld, %04u-%02u-%02u %02u:%02u:%02u"
                     , bp->size
                     , tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday
                     , tm->tm_hour, tm->tm_min, tm->tm_sec);
    else
      sprintf(tempStr, "Archive info: %ld, * Illegal date/time", bp->size);

    logEntry(tempStr, LOG_INBOUND, 0);
  }
  if (*arcPath == 0)
  {
    if (!*config.GUS32.programName)
    {
      if (arcType == 0xFF)
      {
        logEntry("Unknown archiving utility used", LOG_ALWAYS, 0);
        addExtension(fullFileName, ".unknown_archiver");
      }
      else
      {
        logEntryf(LOG_ALWAYS, 0, "Archive decompression program for %s method not defined", extPtr);
        addExtension(fullFileName, ".undefined_archiver");
      }
      return;
    }
    strcpy(arcPath, config.GUS32.programName);
    extPtr = (char*)"gus";
  }
  if (*arcPath == 0)
  {
    logEntryf(LOG_ALWAYS, 0, "Archive decompression program for %s not configured", extPtr);
    return;
  }
  strcpy(unpackPathStr, config.inPath);  // pktPath

  // preprocessor
  if (*config.preUnarc32.programName)
    execute("Pre-Unpack", config.preUnarc32.programName, fullFileName, unpackPathStr,  NULL);

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
  if (execute(extPtr, arcPath, fullFileName, unpackPathStr, NULL))
  {
    if (arcType == 4)  // ZOO
      ChDir(dirStr);

    //Delete(config.inPath, "*.pkt");
    logEntryf(LOG_ALWAYS, 0, "Cannot unpack bundle %s", fullFileName);
    addExtension(fullFileName, ".unpack_error");
    newLine();
    return;
  }
  if (arcType == 4)  // ZOO
  {
    if (strlen(unpackPathStr) > 3)
      strcat(unpackPathStr, dDIRSEPS);

    ChDir(dirStr);
  }
  if (arcType == 8) // UC2
    strcpy(unpackPathStr, unpackPathStr + 1);

  // postprocessor
  if (*config.postUnarc32.programName)
    execute("Post-Unpack", config.postUnarc32.programName, fullFileName, unpackPathStr, NULL);

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
  rename(fixPath(pktName), fixPath(qqqName));
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
  DIR          *dir;
  struct dirent *ent;
  tempStrType   arcPath
              , tempStr
              , pktName
              , semaName;
  char         *helpPtr
              ,*helpPtr2
//            , pathStr [64]
//            , parStr  [MAX_PARSIZE]
//            , procStr1[64]
//            , procStr2[MAX_PARSIZE]
              ;
  fhandle       tempHandle
              , semaHandle = -1;
  long          arcSize = 0;
  const char   *exto
             , *extn
             , *cPtr = "";

#ifdef _DEBUG
  logEntryf(LOG_DEBUG, 0, "DEBUG packArc: file %s  %s -> %s  outStatus:%d", fixPath(qqqName), nodeStr(srcNode), nodeStr(destNode), nodeInfo->outStatus);
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

  if (*nodeInfo->pktOutPath)
  {
    // Write uncompressed pkt files directly to configured 'pkt outputpath'
    strcpy(pktName, fixPath(nodeInfo->pktOutPath));
    if (!lastSep(helpPtr, qqqName))
      return 1;

    strcat(pktName, helpPtr + 1);
    strcpy(pktName + strlen(pktName) - 3, extn);
    if (moveFile(qqqName, pktName))
    {
      // If moving didn't succeed, rename to 'exto' extension. Don't care about the result
      helpPtr = stpcpy(pktName, fixPath(qqqName));
      strcpy(helpPtr - 3, exto);
      rename(fixPath(qqqName), pktName);  // pktName already fixed

      return 1;
    }
    outboundBackup(pktName);
    return 0;
  }

  // Rename pkt file to correct extension
  helpPtr = stpcpy(pktName, fixPath(qqqName));
  strcpy(helpPtr - 3, extn);
  if (rename(fixPath(qqqName), pktName) != 0)  // pktName already fixed
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
    unlink(fixPath(semaName));
    *(u16 *)(helpPtr + count) = '*';
    if (existPattern(config.semaphorePath, helpPtr))
      return handleNoCompress(pktName, qqqName, destNode);

    if (config.mailOptions.createSema)
    {
      strcpy(helpPtr + count, "FM");
      if ((semaHandle = open(fixPath(semaName), O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0664)) == -1)
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
      unlink(fixPath(semaName));
      *(u16 *)(helpPtr + 1) = '*';
      if (existPattern(config.semaphorePath, helpPtr2))
        return handleNoCompress(pktName, qqqName, destNode);

      if (config.mailOptions.createSema)
      {
        strcpy(helpPtr + 1, "FM");
        if ((semaHandle = open(fixPath(semaName), O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0664)) == -1)
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
          MKDIR(semaName);
          *(archivePtr - 1) = dDIRSEPC;
        }
        if (destNode->point)
        {
          archivePtr += sprintf(archivePtr, "%04hx%04hx.pnt", destNode->net, destNode->node);
          MKDIR(semaName);
          sprintf(archivePtr, dDIRSEPS"%08hx.bsy", destNode->point);
        }
        else
          sprintf(archivePtr, "%04hx%04hx.bsy", destNode->net, destNode->node);

        // Don't use config.mailOptions.createSema! FTS-5005 specifies it needs to be set
        if ((semaHandle = open(fixPath(semaName), O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0664)) == -1)
          return handleNoCompress(pktName, qqqName, destNode);
      }
  }

  // Make output path
  archivePtr = stpcpy(archiveStr, fixPath(config.outPath));
  if (config.mailer == dMT_Binkley || config.mailer == dMT_Xenia)
  {
    if (destNode->zone != config.akaList[0].nodeNum.zone)
    {
      // Set specific BINK paths extensions for out of zone destinations
      archivePtr += sprintf(archivePtr - 1, ".%03hx", destNode->zone);
      MKDIR(archiveStr);
      strcpy(archivePtr - 1, dDIRSEPS);
    }
    if (destNode->point)
    {
      // Set specific BINK paths extensions for point destinations
      archivePtr += sprintf(archivePtr, "%04hx%04hx.pnt", destNode->net, destNode->node);
      MKDIR(archiveStr);
      strcpy(archivePtr++, dDIRSEPS);
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
      if (  !config.mailOptions.neverARC060
         && (  (  destNode->zone == config.akaList[0].nodeNum.zone
               || config.mailOptions.ARCmail060                               // "╚═> Out-of-zone"
               || !(nodeInfo->capability & PKT_TYPE_2PLUS)
               )
            && srcNode ->point == 0
            && destNode->point == 0
            )
         )
        // ArcMail 0.60 format
        extPtr = archivePtr + sprintf( archivePtr, "%04hX%04hX"               // Keep archive name uppercase, because an external archiver might change it to uppercase
                                     , (*srcNode).net  - (*destNode).net
                                     , (*srcNode).node - (*destNode).node);
      else
      {
        // Crc32 format
        sprintf( nodeName, "%hu:%hu/%hu.%hu", destNode->zone
               , config.akaList[0].nodeNum.point + destNode->net
               , destNode->node - config.akaList[0].nodeNum.net
               , config.akaList[0].nodeNum.node  + destNode->point);
        extPtr = archivePtr + sprintf(archivePtr, "%08X", crc32(nodeName));   // idem
      }

      sprintf(extPtr, ".%.2s?", "SUMOTUWETHFRSA" + (timeBlock.tm_wday << 1)); // idem

      // Search for existing archive files
      maxArc = '0' - 1;
      okArc = 0;

      *(archivePtr - 1) = 0;
      dir = opendir(archiveStr);
      *(archivePtr - 1) = dDIRSEPC;
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
          unlink(fixPath(semaName));
        }
        rename(pktName, fixPath(qqqName));  // pktName already fixed
        newLine();
        return 1;
    }

    if (*arcPath == 0)
    {
      logEntryf(LOG_ALWAYS, 0, "Archive compression program for %s method not defined", cPtr);

      if (semaHandle >= 0)
      {
        close(semaHandle);
        unlink(fixPath(semaName));
      }
      rename(pktName, fixPath(qqqName));  // pktName already fixed
      newLine();
      return 1;
    }
  }
  else  // No archiver used
  {
    if (!lastSep(helpPtr, pktName))
      return 1;

    strcpy(archivePtr, helpPtr + 1);
  }

  outboundBackup(pktName);

  // preprocessor
  if (*config.preArc32.programName)
    execute("Pre-Pack", config.preArc32.programName, archiveStr, pktName, nodeInfo->pktOutPath);

  okArc = 1;
  if (nodeInfo->archiver != 0xFF)
  {
    if (execute(cPtr, arcPath, archiveStr, pktName, nodeInfo->pktOutPath))
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
      unlink(fixPath(semaName));
    }
    rename(pktName, fixPath(qqqName));  // pktName already fixed
    newLine();
    return 1;
  }

  // postprocessor
  if (*config.postArc32.programName)
    execute("Post-Pack", config.postArc32.programName, archiveStr, pktName,  nodeInfo->pktOutPath);

  if (nodeInfo->archiver != 0xFF)
  {
    arcSize = 0;
    if ((tempHandle = open(archiveStr, O_RDONLY | O_BINARY)) != -1)  // archiveStr is fixPath'd
    {
      arcSize = fileLength(tempHandle);
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
          unlink(fixPath(semaName));
        }
        rename(pktName, fixPath(qqqName));  // pktName already fixed
        newLine();

        return 1;
      }
    }
    else
    {
      int create;

      strcpy(tempStr, archiveStr);
      if (destNode->point)
        extPtr = archivePtr + sprintf(archivePtr, "%08hx", destNode->point);
      else
        extPtr = archivePtr + sprintf(archivePtr, "%04hx%04hx", destNode->net, destNode->node);

      strcpy(extPtr, ".?lo");

      ent = NULL;
      if ((dir = opendir(fixPath(config.outPath))) != NULL)
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
      {
        if (nodeInfo->outStatus > 5)
          nodeInfo->outStatus = 0;

        sprintf(extPtr, ".%clo", "fhchdd"[nodeInfo->outStatus]);
      }

      create = access(archiveStr, 0);  // archiveStr already fixed

      if ((tempHandle = open(archiveStr, O_RDWR | O_CREAT | O_APPEND | O_BINARY, dDEFOMODE)) == -1)  // archiveStr already fixed
      {
        if (semaHandle >= 0)
        {
          close(semaHandle);
          unlink(fixPath(semaName));
        }
        rename(pktName, fixPath(qqqName));  // pktName already fixed
        newLine();

        return 1;
      }
      logEntryf(LOG_OUTBOUND, 0, "%s %s", create ? "Create" : "Update", archiveStr);
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
        dprintf(tempHandle, "%c%s\r\n", nodeInfo->archiver != 0xFF ? '#' : '^', archiveStr);  // Truncate or Delete the file after transmission depends on archived or not
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
      strcpy(fAttInfo[fAttCount++].fileName, archiveStr + strlen(fixPath(config.outPath)));
    }
  }
  if (nodeInfo->archiver != 0xFF)
    unlink(pktName);  // pktName already fixed

  // Close and Delete semaphore file
  if (semaHandle >= 0)
  {
    close(semaHandle);
    unlink(fixPath(semaName));
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
  DIR           *dir;
  struct dirent *ent;
  nodeNumType    destNode4d;
  int            logged = 0;

  if ((dir = opendir(fixPath(config.outPath))) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
      if (match_spec("*.qqq", ent->d_name))
      {
        if (!logged)
        {
          logEntry("Retrying to compress outgoing mailpacket(s)", LOG_OUTBOUND, 0);
          logged = 1;
        }

        strcpy(stpcpy(tempStr, fixPath(config.outPath)), ent->d_name);

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
