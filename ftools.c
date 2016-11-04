//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2016 Wilfred van Velzen
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

#ifdef __STDIO__
#include <conio.h>
#else
#include <bios.h>
#endif
#include <ctype.h>
#include <dir.h>
#include <dirent.h>
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stddef.h>    // offsetof()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"

#include "ftools.h"

#include "areainfo.h"
#include "cfgfile.h"
#include "crc.h"
#include "ftlog.h"
#include "ftnotify.h"
#include "ftr.h"
#include "hudson.h"
#include "jam.h"
#include "jamfun.h"
#include "jammaint.h"
#include "lock.h"
#include "minmax.h"
#include "msgmsg.h"
#include "msgradef.h"
#include "mtask.h"
#include "pp_date.h"
#include "sorthb.h"
#include "spec.h"
#include "stpcpy.h"
#include "utils.h"
#include "version.h"

// linkfout FTools voorkomen!
u16 echoCount;

#ifdef __STDIO__
#define keyWaiting (kbhit() ? getch() : 0)
#else
#define keyWaiting bioskey(1)
#define keyRead    bioskey(0)
#endif

#define RENUM_BUFSIZE 32760 // MAX 32760
#define DELRECNUM      8192

#ifdef __WIN32__
const char *smtpID;
#endif

time_t startTime;
long   gmtOffset;

const char *semaphore[6] =
{
  "fdrescan.now"
, "ierescan.now"
, "dbridge.rsn"
, ""
, "mdrescan.now"
, "xmrescan.flg"
};
//----------------------------------------------------------------------------
typedef struct
{
  u8  used;
  u16 usedCount;
} keepTxtRecType;
//----------------------------------------------------------------------------
u16 forwNodeCount;
nodeFileType nodeFileInfo;
cookedEchoType *echoAreaList;

typedef u16 lastReadType[256];

lastReadType lastReadRec;

boardInfoType boardInfo[MBBOARDS == 200 ? 256 : 512];

fhandle msgInfoHandle;

char configPath[FILENAME_MAX];
configType config;

u32 lastSavedUniqID = 0;

u16 recovered  = 0;
u16 alrDeleted = 0;
u16 unDeleted  = 0;
u16 nowDeleted = 0;

const uchar setBitTab[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

const uchar bitMaskTab[8] = { 0, 1, 3, 7, 15, 31, 63, 127 };

const uchar bitCountTab[256] =
{ 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5
, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5
, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6
, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5
, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6
, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6
, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7
, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5
, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6
, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6
, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7
, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6
, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7
, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7
, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};
//----------------------------------------------------------------------------
void About(void)
{
  char *str = "About FTools:\n\n"
              "    Version          : %s\n"
              "    Operating system : "
#if   defined(__WIN32__)
              "Win32\n"
#elif defined(__linux__)
              "Linux\n"
#else
              "Unknown\n"
#endif
#if 0
              "    Target processor : "
#if   defined(__PENTIUMPRO__)
              "PentiumPro\n"
#elif defined(__PENTIUM__)
              "Pentium\n"
#elif defined(__486__)
              "i486 and up\n"
#elif defined(P386)
              "80386 and up\n"
#else
              "8088/8086 and up\n"
#endif
#endif
              "    Compiled on      : %04u-%02u-%02u\n";

  printf(str, VersionStr(), YEAR, MONTH + 1, DAY);
}
//----------------------------------------------------------------------------
void Usage(void)
{
  char *str = "Usage:   FTools <command> [parameters]\n\n"
              "Commands:\n\n"
              "   About     Print program info\n"
#ifndef GOLDBASE
              "   Maint     Message base maintenance (also updates reply chains)\n"
              "   Delete    Delete all messages in one board\n"
              "   Undelete  Undelete all messages in one board           ("MBNAME" only)\n"
              "   Export    Export messages in a board to a text file    ("MBNAME" only)\n"
              "   Move      Move messages from one board to another      ("MBNAME" only)\n"
              "   Sort      Sorts messages in the message base           ("MBNAME" only)\n"
              "   Stat      Generate message base statistics             ("MBNAME" only)\n"
#endif
              "   Post      Post a file as a netmail or echomail message\n"
              "   Notify    Send a status message to selected nodes\n"
              "   MsgM      *.MSG maintenance\n"
              "   AddNew    Add new areas found during FMail Toss\n"
              "\nEnter 'FTools <command> ?' for more information about [parameters]";
  puts(str);
}
//----------------------------------------------------------------------------
s16 readAreaInfo(void)
{
  u16 count;
  headerType  *header;
  rawEchoType *areaBuf;

  memset(&boardInfo, 0, sizeof(boardInfo));

  if (!openConfig(CFG_ECHOAREAS, &header, (void *)&areaBuf))
    return 1;
  else
  {
    for (count = 0; count < header->totalRecords; count++)
    {
      returnTimeSlice(0);
      getRec(CFG_ECHOAREAS, count);
      if ((areaBuf->board > 0) && (areaBuf->board <= MBBOARDS)
          && (!*areaBuf->msgBasePath))
      {
        boardInfo[areaBuf->board].defined++;
        if ((boardInfo[areaBuf->board].name = malloc(strlen(areaBuf->areaName) + 1)) != NULL)
          strcpy(boardInfo[areaBuf->board].name, areaBuf->areaName);

        boardInfo[areaBuf->board].aka         = areaBuf->address;
        boardInfo[areaBuf->board].options     = areaBuf->options;
        boardInfo[areaBuf->board].numDays     = areaBuf->days;
        boardInfo[areaBuf->board].numDaysRcvd = areaBuf->daysRcvd;
        boardInfo[areaBuf->board].numMsgs     = areaBuf->msgs;
      }
    }
    closeConfig(CFG_ECHOAREAS);
  }
  return 0;
}
//----------------------------------------------------------------------------
s32 getSwitchFT(int *argc, char *argv[], s32 mask)
{
  s32 tempMask;
  s16 error = 0;
  s32 result = 0;
  int count = *argc;

  while (count && --count >= 1)
    if (argv[count][0] == '/')
    {
      if (count != --(*argc))
      {
        puts("Switches should be last on command line");
        exit(4);
      }
      if (strlen(argv[count]) != 2 || !(isalpha(argv[count][1])))
      {
        printf("Illegal switch: %s\n", argv[count]);
        error++;
      }
      else
      {
        tempMask = 1L << (toupper(argv[count][1]) - 'A');
        if (mask & tempMask)
          result |= tempMask;
        else
        {
          printf("Illegal switch: %s\n", argv[count]);
          error++;
        }
      }
    }
  if (error)
    exit(4);

  return result;
}
//----------------------------------------------------------------------------
void addNew(s32);
void myexit(void);

//----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  s16 ch;
  tempStrType tempStr, tempStr2, tempStr3;
  u16 count
    , c
    , ddd
    , temp
    , bufCount
    , newBufCount;
  fhandle configHandle
        , lastReadHandle
        , semaHandle;
  s16 postBoard;
  s32 switches;
  rawEchoType *areaPtr;

  char        *helpPtr, *helpPtr2, *helpPtr3;

  u16 areaSubjChain[256];

  u16         *msgRenumBuf;
  u16 msgNum;
  u16 low, mid, high;
  s16 isNetmail;
  internalMsgType *message;
  u16 srcAka;
  time_t time1, time2, time2a;
  DIR           *dir;
  struct dirent *ent;

#ifndef GOLDBASE
  u16 e;
  u32 temp4;
  msgHdrRec      *msgHdrBuf;
  msgToIdxRec    *msgToIdxBuf;
  msgIdxRec      *msgIdxBuf;
  msgTxtRec      *msgTxtBuf;
  u16            *lruBuf;
  u16            *linkArraySubjectCrcHigh;
  u16            *linkArraySubjectCrcLow;
  u16            *linkArrayPrevReply;
  u16            *linkArrayNextReply;
  u16            *linkArrayMsgNum;
  u16 linkArrIndex;
  fhandle usersBBSHandle
        , oldTxtHandle
        , oldHdrHandle
        , msgHdrHandle
        , msgToIdxHandle
        , msgIdxHandle
        , msgTxtHandle;
  infoRecType infoRec
    , oldInfoRec;
  s16 oldBoard
    , newBoard
    , undeleteBoard = 0; // FTools Maint Undelete
  uchar *keepHdr;
  u16 keepIdx;
  uchar keepBit
      , mask;
  keepTxtRecType *keepTxt;
  s32 spaceSavedJAM;
  time_t msgTime;
  boardInfoType currBoardInfo;
  u16 *renumArray;
  s16 newMsgNum = 0;
  u16 msgCount = 0;
  u16 badBoardCount = 0;
  u16 badDateCount = 0;
  u16 badTxtlenCount = 0;
  u16 crossLinked = 0;
  s32 code;
  u16 codeHigh, codeLow;
  s32 fileSize;
  u16 oldHdrSize
    , newHdrSize = 0
    , oldTxtSize
    , newTxtSize = 0;

  u16 lowMsgNum
    , highMsgNum;
  u32 totalMsgs
    , totalDeleted
    , totalTxtHdr;
  u16 totalTxtBBS;
  u32 totalLocal;
  u16 hours, minutes, day, month, year;
  u16 maxRead, index;
  u16 msgIdxBufCount = 0;
  u16 TXT_BUFSIZE, LRU_BUFSIZE;
  u16 HDR_BUFSIZE;
#endif

  atexit(myexit);

#ifdef __BORLANDC__
  putenv("TZ=LOC0");
  tzset();
#endif // __BORLANDC__
#ifdef __MINGW32__
  // Clear the TZ environment variable so the OS is used
  putenv("TZ=");
  tzset();
#endif // __MINGW32__

  time(&startTime);
  {
    struct tm *tm = gmtime(&startTime);  // gmt ok!
    tm->tm_isdst = -1;
    gmtOffset = startTime - mktime(tm);
  }
#ifdef __WIN32__
  // Set default open mode:
  _fmode = O_BINARY;

  smtpID = TIDStr();
#endif // __WIN32__

  printf("%s - The Fast Message Base Utility\n\n", VersionStr());
  printf("Copyright (C) 1991-%s by FMail Developers - All rights reserved\n\n", __DATE__ + 7);

  // Array for linking
  memset(areaSubjChain, 0xff, sizeof(areaSubjChain));

  if ((helpPtr = getenv("FMAIL")) == NULL || *helpPtr == 0)
  {
    strcpy(configPath, argv[0]);
    if ((helpPtr = strrchr(configPath, '\\')) == NULL)
      strcpy(configPath, ".\\");
    else
      helpPtr[1] = 0;
  }
  else
  {
    strcpy(configPath, helpPtr);
    if (configPath[strlen(configPath) - 1] != '\\')
      strcat(configPath, "\\");
  }
#ifdef _DEBUG
  printf("DEBUG configPath: %s\n", configPath);
#endif

  strcpy(stpcpy(tempStr, configPath), dCFGFNAME);

  if (  (configHandle = _sopen(tempStr, O_RDONLY | O_BINARY, SH_DENYWR)) == -1
     || read(configHandle, &config, sizeof(configType)) < sizeof(configType)
     || close(configHandle) != 0
     )
  {
    puts("Can't read "dCFGFNAME);
    exit(1);
  }
  lastSavedUniqID = config.lastUniqueID;
  if (config.maxForward < 64)
    config.maxForward = 64;
  if (config.maxMsgSize < 45)
    config.maxMsgSize = 45;
  if (config.recBoard > MBBOARDS)
    config.recBoard = 0;
  if (config.badBoard > MBBOARDS)
    config.badBoard = 0;
  if (config.dupBoard > MBBOARDS)
    config.dupBoard = 0;

#ifdef GOLDBASE
  config.bbsProgram = BBS_QBBS;
#endif

  strcpy(stpcpy(tempStr, config.bbsPath), dFMAIL_LOC);
  strcpy(tempStr2, config.bbsPath);
  if ((helpPtr = strrchr(tempStr2, '\\')) != NULL)
    *helpPtr = 0;

  if  (  !access(tempStr2, 0)
      && (semaHandle = _sopen(tempStr, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYWR, S_IREAD | S_IWRITE)) == -1
      && errno != ENOFILE
      )
  {
    puts("Waiting for another copy of FMail, FTools or FSetup to finish...");

    time(&time1);
    time2 = time1;

    ch = 0;
    while (((semaHandle = _sopen(tempStr, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, S_IREAD | S_IWRITE)) == -1)
           && (!config.activTimeOut || time2 - time1 < config.activTimeOut) && ((ch = (keyWaiting & 0xff)) != 27))
    {
      if (ch == 0 || ch == -1)
      {
        time2a = time2 + 1;
        while (time(&time2) <= time2a)
          returnTimeSlice(1);
      }
#ifndef __STDIO__
      else
        keyRead;
#endif
    }
#ifndef __STDIO__
    if (ch != 0 && ch != -1)
      keyRead;
#endif

    if (semaHandle == -1)
    {
      puts("\nAnother copy of FMail, FTools or FSetup did not finish in time...\n\nExiting...");

      exit(4);
    }
  }

  // FIX bad RA2 update !!!!!!!!
  if ((config.bbsProgram == BBS_RA1X) && config.genOptions._RA2)
  {
    config.bbsProgram = BBS_RA20;
    config.genOptions._RA2 = 0;
  }

  if (config.akaList[0].nodeNum.zone == 0)
  {
    puts("Main nodenumber not defined in FConfig");
    exit(4);
  }

  if ((*config.netPath == 0)
      || (*config.bbsPath == 0)
      || (*config.inPath  == 0)
      || (*config.outPath == 0))
  {
    puts("Not all subdirectories are defined in FConfig\n");
    exit(4);
  }

  if (  existDir(config.netPath, "netmail")      == 0
     || existDir(config.bbsPath, "message base") == 0
     || existDir(config.inPath,  "inbound")      == 0
     || existDir(config.outPath, "outbound")     == 0
     )
  {
    puts("Please enter the required subdirectories first!");
    exit(4);
  }

  // 200x187 >= 140x256 !!!

#ifndef GOLDBASE
  HDR_BUFSIZE = (248 >> 3) * 8;
  TXT_BUFSIZE = (HDR_BUFSIZE << 1) / 3;
  LRU_BUFSIZE = HDR_BUFSIZE * 100;

  if (  ((msgHdrBuf   = malloc(HDR_BUFSIZE * sizeof(msgHdrRec))) == NULL)
     || ((msgIdxBuf   = malloc(HDR_BUFSIZE * sizeof(msgIdxRec))) == NULL)
     || ((msgToIdxBuf = malloc(HDR_BUFSIZE * sizeof(msgToIdxRec))) == NULL)
     || ((keepHdr     = malloc(DELRECNUM)) == NULL)
     || ((keepTxt     = malloc(DELRECNUM * sizeof(keepTxtRecType))) == NULL)
     || ((renumArray  = malloc(RENUM_BUFSIZE * 2)) == NULL)
     )
  {
    puts("Not enough free memory");
    exit(2);
  }
#endif

  if (readAreaInfo())
    logEntry("Can't read file "dARFNAME, LOG_ALWAYS, 1);

  for (count = MAX_NETAKAS - 1; count != 0xffff; count--)
    if (config.netmailBoard[count])
    {
      boardInfo[config.netmailBoard[count]].defined++;
      if ((boardInfo[config.netmailBoard[count]].name = malloc(7)) != NULL)
      {
        if (count)
          sprintf(boardInfo[config.netmailBoard[count]].name, "#net%u", count);
        else
          strcpy(boardInfo[config.netmailBoard[count]].name, "#netm");
      }
      boardInfo[config.netmailBoard[count]].aka         = count;
      boardInfo[config.netmailBoard[count]].options     = config.optionsAKA[count];
      boardInfo[config.netmailBoard[count]].numDays     = config.daysAKA[count];
      boardInfo[config.netmailBoard[count]].numDaysRcvd = config.daysRcvdAKA[count];
      boardInfo[config.netmailBoard[count]].numMsgs     = config.msgsAKA[count];
    }
  if (config.badBoard)
  {
    boardInfo[config.badBoard].defined++;
    boardInfo[config.badBoard].name = "#bad";
  }
  if (config.dupBoard)
  {
    boardInfo[config.dupBoard].defined++;
    boardInfo[config.dupBoard].name = "#dup";
  }
  if (config.recBoard)
  {
    boardInfo[config.recBoard].defined++;
    boardInfo[config.recBoard].name = "#rec";
  }

#ifndef GOLDBASE
  switches = 0;

  if ((argc >= 2)
      && ((stricmp(argv[1], "D") == 0) || (stricmp(argv[1], "DELETE") == 0)
          || (stricmp(argv[1], "M") == 0) || (stricmp(argv[1], "MAINT") == 0)
          || (stricmp(argv[1], "U") == 0) || (stricmp(argv[1], "UNDELETE") == 0)
          || (stricmp(argv[1], "V") == 0) || (stricmp(argv[1], "MOVE") == 0)))
  {
    if ((stricmp(argv[1], "D") == 0) || (stricmp(argv[1], "DELETE") == 0))
      switches = SW_K;
    if ((stricmp(argv[1], "U") == 0) || (stricmp(argv[1], "UNDELETE") == 0))
      switches = SW_U;
    if ((stricmp(argv[1], "V") == 0) || (stricmp(argv[1], "MOVE") == 0))
      switches = SW_M;
    if ((argc >= 3) && ((argv[2][0] == '?') || (argv[2][1] == '?')))
    {
      if (switches & SW_K)
        puts("Usage:\n\n"
             "    FTools Delete <board>\n");
      else
        if (switches & SW_U)
          puts("Usage:\n\n"
               "    FTools Undelete <board>\n");
        else
          if (switches & SW_M)
            puts("Usage:\n\n"
                 "    FTools Move <old board> <new board>\n");
          else
            puts("Usage:   FTools Maint [JAM areas][/H][/J][/Q][/C][/D][/N][/P][/R][/T][/U][/X]\n"
                 "Switches:                                                        [/B][/F][/O]\n"
                 "    /H   Process "MBNAME " base only\n"
                 "    /J   Process JAM message bases only\n"
                 "    /Q   Process only modified JAM areas\n"
                 "    /C   reCover messages in undefined boards ("MBNAME" only)\n"
                 "    /D   Delete messages using the information in the Area Manager\n"
                 "    /N   reNumber the message base ("MBNAME" only)\n"
                 "    /P   Pack (remove deleted messages) ("MBNAME" only)\n"
                 "    /R   Remove \"Re:\" from subject lines\n"
                 "    /T   Fix bad text length info in MsgHdr.BBS ("MBNAME" only)\n"
                 "    /U   Undelete all deleted messages in all boards ("MBNAME" only)\n"
                 "    /X   Delete messages with bad dates or bad board numbers ("MBNAME" only)\n\n"

                 "    /B   keep .Bak files      (only in combination with /P)\n"
                 "    /F   Force overwrite mode (only in combination with /P, "MBNAME" only)\n"
                 "    /O   Overwrite existing message base only if short of disk space\n"
                 "                              (only in combination with /P, "MBNAME" only)");
      return 0;
    }

    if (switches & SW_K)
    {
      initLog("DELETE", 0);
      if ((((oldBoard = atoi(argv[2])) == 0)
           || (oldBoard > MBBOARDS))
          && ((oldBoard = getBoardNum(argv[2], argc - 2, &temp, &areaPtr)) == -1))
      {
        flogEntry(LOG_ALWAYS, 0, "Deleting all messages in area %s (JAM base %s)", areaPtr->areaName, areaPtr->msgBasePath);
        spaceSavedJAM = 0;
        JAMmaint(areaPtr, switches, config.sysopName, &spaceSavedJAM);

        return 0;
      }
      flogEntry(LOG_ALWAYS, 0, "Deleting all messages in "MBNAME" board %u", oldBoard);
    }
    else
      if (switches & SW_M)
      {
        initLog("MOVE", 0);
        if ((((oldBoard = atoi(argv[2])) == 0)
             || (oldBoard > MBBOARDS))
            && ((oldBoard = getBoardNum(argv[2], argc - 2, &temp, &areaPtr)) == -1))
          logEntry("This function does not yet support the JAM base", LOG_ALWAYS, 1000);

        if ((((newBoard = atoi(argv[3])) == 0)
             || (newBoard > MBBOARDS))
            && ((newBoard = getBoardNum(argv[3], argc - 3, &temp, &areaPtr)) == -1))
          logEntry("This function does not yet support the JAM base", LOG_ALWAYS, 1000);

        flogEntry(LOG_ALWAYS, 0, "Moving all messages in board %u to board %u", oldBoard, newBoard);
      }
      else
        if (switches & SW_U)
        {
          initLog("UNDELETE", 0);
          if ((((undeleteBoard = atoi(argv[2])) == 0)
               || (undeleteBoard > MBBOARDS))
              && ((undeleteBoard = getBoardNum(argv[2], argc - 2, &temp, &areaPtr)) == -1))
            logEntry("This function does not yet support the JAM base", LOG_ALWAYS, 1000);

          flogEntry(LOG_ALWAYS, 0, "Undeleting all deleted messages in board %u", undeleteBoard);
        }
        else
        {
          switches = getSwitchFT(&argc, argv, SW_H | SW_J | SW_C | SW_D | SW_N | SW_P | SW_Q | SW_R | SW_T | SW_U | SW_X | SW_B | SW_F | SW_O);
          initLog("MAINT", switches);
        }

    if (argv[2] && *argv[2] != '/')
      switches &= ~SW_H;
    if (switches & SW_J)
      goto JAMonly;

    memset(&lastReadRec, 0, sizeof(lastReadType));

    if ((lastReadHandle = open(expandName("LASTREAD."MBEXTN), O_RDONLY | O_BINARY)) == -1)
      logEntry("Can't read file LastRead."MBEXTN, LOG_ALWAYS, 1);

    if (read(lastReadHandle, &(lastReadRec[1]), 400) != 400)
    {
      close(lastReadHandle);
      logEntry("Can't read file LastRead."MBEXTN, LOG_ALWAYS, 1);
    }
    close(lastReadHandle);

    if (((msgTxtHandle = _sopen(expandName(dMSGTXT"."MBEXTN), O_RDONLY | O_BINARY, SH_DENYRW)) == -1)
        || ((fileSize = filelength(msgTxtHandle)) == -1)
        || (close(msgTxtHandle) == -1))
      logEntry("File status request error", LOG_ALWAYS, 1);
    totalTxtBBS = (u16)(fileSize >> 8);

    memset(renumArray, 0, RENUM_BUFSIZE * 2);
    memset(&infoRec,   0, sizeof(infoRecType));

    memset(keepHdr,    0, DELRECNUM);
    memset(keepTxt,    0, DELRECNUM * sizeof(keepTxtRecType));

    keepIdx = 0;
    keepBit = 1;

    // Read MSGINFO.BBS

    if (((msgInfoHandle = _sopen(expandName(dMSGINFO"."MBEXTN), O_RDONLY | O_BINARY, SH_DENYRW)) == -1)
        || (read(msgInfoHandle, &oldInfoRec, sizeof(infoRecType)) == -1))
      logEntry("Can't open file MsgInfo."MBEXTN " for input", LOG_ALWAYS, 1);
    close(msgInfoHandle);

    if ((oldInfoRec.HighMsg >= config.autoRenumber) && !(switches & SW_N))
    {
      switches |= SW_N;
      logEntry("Message base AutoRenumbering activated", LOG_ALWAYS, 0);
    }

    puts("Analyzing the message base...");

    if ((msgHdrHandle = _sopen(expandName(dMSGHDR"."MBEXTN), O_RDWR | O_BINARY, SH_DENYRW)) == -1)
      logEntry("Can't open MsgHdr."MBEXTN " for update", LOG_ALWAYS, 1);
    if ((switches & SW_T))
      if ((msgTxtHandle = _sopen(expandName(dMSGTXT"."MBEXTN), O_RDONLY | O_BINARY, SH_DENYRW)) == -1)
        logEntry("Can't open MsgTxt."MBEXTN " for read-only",  LOG_ALWAYS, 1);

    oldHdrSize = (u16)(filelength(msgHdrHandle) / sizeof(msgHdrRec));

    if (((linkArraySubjectCrcHigh = malloc((oldHdrSize << 1) + 1)) == NULL)
        || ((linkArraySubjectCrcLow  = malloc((oldHdrSize << 1) + 1)) == NULL)
        || ((linkArrayMsgNum    = malloc((oldHdrSize << 1) + 1)) == NULL)
        || ((linkArrayNextReply = malloc((oldHdrSize << 1) + 1)) == NULL)
        || ((linkArrayPrevReply = malloc((oldHdrSize << 1) + 1)) == NULL))
    {
      newLine();
      logEntry("Not enough free memory", LOG_ALWAYS, 2);
    }

    while ((bufCount = read(msgHdrHandle, msgHdrBuf, HDR_BUFSIZE
                             * sizeof(msgHdrRec)) / sizeof(msgHdrRec)) > 0)
    {
      for (count = 0; count < bufCount; count++)
      {
        returnTimeSlice(0);
        if (msgHdrBuf[count].MsgAttr & RA_DELETED)
          alrDeleted++;

        if ((switches & SW_M)
            && (msgHdrBuf[count].Board == oldBoard))
          msgHdrBuf[count].Board = newBoard;

        currBoardInfo = boardInfo[msgHdrBuf[count].Board];

        // Undelete

        if ((switches & SW_U) && (msgHdrBuf[count].MsgAttr & RA_DELETED)
            && ((undeleteBoard == 0) || (undeleteBoard == msgHdrBuf[count].Board)))
        {
          msgHdrBuf[count].MsgAttr &= ~RA_DELETED;
          unDeleted++;
        }

        // Fix bad messages

        if ((msgHdrBuf[count].Board == 0)
            || (msgHdrBuf[count].Board > MBBOARDS))
          if ((switches & SW_X)
              && (!(msgHdrBuf[count].MsgAttr & RA_DELETED)))
          {
            msgHdrBuf[count].MsgAttr |= RA_DELETED;
            badBoardCount++;
          }

        if ((switches & SW_X)
            && (!(msgHdrBuf[count].MsgAttr & RA_DELETED)))
          if (scanDate((char*)&msgHdrBuf[count].ptLength, &year, &month, &day, &hours, &minutes) != 0)
          {
            msgHdrBuf[count].MsgAttr |= RA_DELETED;
            badDateCount++;
          }

        // Delete

        if ((!(msgHdrBuf[count].MsgAttr & RA_DELETED))
            && (msgHdrBuf[count].Board > 0)
            && (msgHdrBuf[count].Board <= MBBOARDS))
        {
          if ((switches & SW_K)
              && (msgHdrBuf[count].Board == oldBoard))
          {
            msgHdrBuf[count].MsgAttr |= RA_DELETED;
            nowDeleted++;
          }

          if ((switches & SW_D)
              && ((!currBoardInfo.options.sysopRead)
                  || (msgHdrBuf[count].MsgNum < lastReadRec[msgHdrBuf[count].Board])))
          {
            if (msgHdrBuf[count].checkSum ^ msgHdrBuf[count].subjCrc
                ^ msgHdrBuf[count].wrTime ^ msgHdrBuf[count].recTime
                ^ CS_SECURITY)
            {
              if (scanDate((char*)&msgHdrBuf[count].ptLength, &year, &month, &day, &hours, &minutes) != 0)
              {
                newLine();
                flogEntry(LOG_ALWAYS, 0, "Bad date in message base: message #%u in board #%u", msgHdrBuf[count].MsgNum, msgHdrBuf[count].Board);
                time(&msgTime);
              }
              else
                msgTime = checkDate(year, month, day, hours, minutes, 0);
            }
            else
            {
              msgTime = msgHdrBuf[count].wrTime;

              if ((currBoardInfo.options.arrivalDate)
                  || (msgTime - 86400L > startTime))
                msgTime = msgHdrBuf[count].recTime;
            }

            if ((currBoardInfo.numMsgs != 0)
                && (currBoardInfo.numMsgs
                    < oldInfoRec.ActiveMsgs[msgHdrBuf[count].Board - 1]))
            {
              oldInfoRec.ActiveMsgs[msgHdrBuf[count].Board - 1]--;
              msgHdrBuf[count].MsgAttr |= RA_DELETED;
              nowDeleted++;
            }
            else
              if (((currBoardInfo.numDays != 0)
                   && (currBoardInfo.numDays * 86400L + msgTime < startTime))
                  || ((currBoardInfo.numDaysRcvd != 0)
                      && (msgHdrBuf[count].MsgAttr & RA_RECEIVED)
                      && (currBoardInfo.numDaysRcvd * 86400L + msgTime < startTime)))
              {
                oldInfoRec.ActiveMsgs[msgHdrBuf[count].Board - 1]--;
                msgHdrBuf[count].MsgAttr |= RA_DELETED;
                nowDeleted++;
              }
          }
        }

        // Recovery Board

        if ((switches & SW_C)    // config.recBoard &&
            && (!currBoardInfo.defined)
            && (!(msgHdrBuf[count].MsgAttr & RA_DELETED)))
        {
          if ((msgHdrBuf[count].Board > 0)
              && (msgHdrBuf[count].Board <= MBBOARDS))
            oldInfoRec.ActiveMsgs[msgHdrBuf[count].Board - 1]--;
          if (config.recBoard)
            msgHdrBuf[count].Board = config.recBoard;
          else
            msgHdrBuf[count].MsgAttr |= RA_DELETED;
          recovered++;
        }

        // Renumber message base

        if ((switches & SW_N)
            && ((!(msgHdrBuf[count].MsgAttr & RA_DELETED))
                || (!(switches & SW_P))))
          msgHdrBuf[count].MsgNum = ++newMsgNum;

        // Update MSGINFO data

        if (!(msgHdrBuf[count].MsgAttr & RA_DELETED))
        {
          if ((msgHdrBuf[count].Board > 0)
              && (msgHdrBuf[count].Board <= MBBOARDS))
            infoRec.ActiveMsgs[msgHdrBuf[count].Board - 1]++;
          if ((infoRec.LowMsg > msgHdrBuf[count].MsgNum)
              || (infoRec.LowMsg == 0))
            infoRec.LowMsg = msgHdrBuf[count].MsgNum;
          if (infoRec.HighMsg < msgHdrBuf[count].MsgNum)
            infoRec.HighMsg = msgHdrBuf[count].MsgNum;
          infoRec.TotalActive++;
        }

        // Link info

        linkArrayMsgNum[msgCount] = 0;
        linkArrayNextReply[msgCount] = 0;
        linkArrayPrevReply[msgCount] = 0;
        linkArraySubjectCrcHigh[msgCount] = 0;
        linkArraySubjectCrcLow[msgCount] = 0;

        if (!(msgHdrBuf[count].MsgAttr & RA_DELETED))
        {
          if (msgHdrBuf[count].checkSum ^ msgHdrBuf[count].subjCrc
              ^ msgHdrBuf[count].wrTime ^ msgHdrBuf[count].recTime
              ^ CS_SECURITY)
          {
            helpPtr = strncpy(tempStr, msgHdrBuf[count].Subj, temp = min(72, msgHdrBuf[count].sjLength));
            tempStr[temp] = 0;
            while (!strnicmp(helpPtr, "RE:", 3) || !strnicmp(helpPtr, "(R)", 3))
            {
              helpPtr += 3;

              while (*helpPtr == ' ')
                helpPtr++;
            }
            linkArraySubjectCrcHigh[msgCount] = (u16)((code = crc32alpha(helpPtr)) >> 16);
            linkArraySubjectCrcLow[msgCount] = (u16)(code & 0x0000ffff);
          }
          else
          {
            linkArraySubjectCrcHigh[msgCount] = (u16)(msgHdrBuf[count].subjCrc >> 16);
            linkArraySubjectCrcLow[msgCount]  = (u16)(msgHdrBuf[count].subjCrc & 0x0000ffff);
          }

          if ((ddd = msgHdrBuf[count].Board) <= MBBOARDS) // for linking
          {
            linkArrayMsgNum[msgCount]    = msgHdrBuf[count].MsgNum;
            linkArrayPrevReply[msgCount] = areaSubjChain[ddd];
            areaSubjChain[ddd]           = msgCount;
          }
        }

        // Pack
        if ((!(msgHdrBuf[count].MsgAttr & RA_DELETED))
            && (msgHdrBuf[count].StartRec < totalTxtBBS)
            && ((msgHdrBuf[count].NumRecs >> 8) == 0)
            && (msgHdrBuf[count].StartRec
                + msgHdrBuf[count].NumRecs  <= totalTxtBBS))
        {
          keepHdr[keepIdx] |= keepBit;
          newHdrSize++;

          ddd = msgHdrBuf[count].StartRec;
          mask = setBitTab[ddd & 7];
          ddd >>= 3;

          if ((switches & SW_T))
            lseek(msgTxtHandle, msgHdrBuf[count].StartRec * (u32)sizeof(msgTxtRec), SEEK_SET);
          for (c = 0; c < msgHdrBuf[count].NumRecs; c++)
          {
            if ((switches & SW_T))
            {
              msgTxtRec txtRec;

              if (read(msgTxtHandle, &txtRec, sizeof(msgTxtRec)) == sizeof(msgTxtRec)
                  && c + 1 < msgHdrBuf[count].NumRecs
                  && txtRec.txtLength < 255)
              {
                msgHdrBuf[count].NumRecs = c + 1;
                badTxtlenCount++;
                break;
              }
            }
            if (keepTxt[ddd].used & mask)
            {
              if (keepHdr[keepIdx] & keepBit)
              {
                crossLinked++;
                keepHdr[keepIdx] &= ~keepBit;
                newHdrSize--;
              }
            }
            else
            {
              keepTxt[ddd].used |= mask;
              newTxtSize++;
            }

            if ((mask <<= 1) == 0)
            {
              mask = 1;
              ddd++;
            }
          }
        }
        if ((keepBit <<= 1) == 0)
        {
          keepBit = 1;
          keepIdx++;
        }
        msgCount++;
      }
    }
    close(msgHdrHandle);

    if (switches & SW_U)
      flogEntry(LOG_STATS, 0, "Undeleted   : %u msgs", unDeleted);
    else
      if ((alrDeleted) && !(switches & SW_K))
        flogEntry(LOG_STATS, 0, "Already del.: %u msgs", alrDeleted);

    if (crossLinked)
      flogEntry(LOG_STATS, 0, "Crosslinked : %u msgs (fixed)", crossLinked);

    if (badBoardCount)
      flogEntry(LOG_STATS, 0, "Bad board # : %u msgs%s", badBoardCount, (switches & SW_X) ? " (fixed)" : "");

    if (badDateCount)
      flogEntry(LOG_STATS, 0, "Bad dates   : %u msgs (fixed)", badDateCount);

    if (badTxtlenCount)
      flogEntry(LOG_STATS, 0, "Bad txt len : %u msgs (fixed)", badTxtlenCount);

    if (switches & SW_C)
    {
      if (config.recBoard)
        flogEntry(LOG_STATS, 0, "Recovered   : %u msgs", recovered);
      else
        flogEntry(LOG_STATS, 0, "Recov'd/del : %u msgs", recovered);
    }
    if (switches & (SW_D | SW_K))
      flogEntry(LOG_STATS, 0, "Deleted     : %u msgs", nowDeleted);

    puts("Updating reply-chains in memory...");

    for (count = 1; count <= MBBOARDS; count++)
    {
      //printf("%3u%%\b\b\b\b", count >> 1);

      c = areaSubjChain[count];
      while (c != 0xffff)
      {
        ddd = linkArrayPrevReply[c];
        codeHigh = linkArraySubjectCrcHigh[c];
        codeLow  = linkArraySubjectCrcLow[c];
        while ((ddd != 0xffff) && (  (codeLow  != linkArraySubjectCrcLow[ddd])
                                  || (codeHigh != linkArraySubjectCrcHigh[ddd])
                                  )
              )
          ddd = linkArrayPrevReply[ddd];
        e = c;
        c = linkArrayPrevReply[c];
        if (ddd != 0xffff)
        {
          linkArrayPrevReply[e] = linkArrayMsgNum[ddd];
          linkArrayNextReply[ddd] = linkArrayMsgNum[e];
        }
        else
          linkArrayPrevReply[e] = 0;
      }
    }

    for (count = 0; count < DELRECNUM - 1; count++)
      keepTxt[count + 1].usedCount = keepTxt[count].usedCount
                                     + bitCountTab[keepTxt[count].used];
    //puts("100%");

    if ((switches & SW_P) && !(switches & SW_F))
    {
      unlink(expandName(dMSGHDR".bak"));
      unlink(expandName(dMSGIDX".bak"));
      unlink(expandName(dMSGTOIDX".bak"));
      unlink(expandName(dMSGTXT".bak"));
      unlink(expandName(dMSGINFO".bak"));
      if (diskFree(config.bbsPath) < (u32)(
            newHdrSize * (s32)sizeof(msgHdrRec)
            + newHdrSize * (s32)sizeof(msgToIdxRec)
            + newHdrSize * (s32)sizeof(msgIdxRec)
            + newTxtSize * (s32)sizeof(msgTxtRec))
          )
      {
        if (switches & SW_O)
          switches |= SW_F;
        else
          logEntry("Not enough disk space available to pack the message base", LOG_ALWAYS, 1);
      }
    }
    else
      switches |= SW_F;
    if ((switches & SW_F) && (switches & SW_P))
      logEntry("Overwriting existing message base files. DO NOT INTERRUPT!", LOG_ALWAYS, 0);

    puts("Writing MsgHdr."MBEXTN" and index files...");

    newMsgNum = 0;

    if (((msgIdxHandle = _sopen(expandName((switches & SW_F) ? dMSGIDX"."MBEXTN : dMSGIDX"."dEXTTMP)
                               , O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, S_IREAD | S_IWRITE)) == -1)
        || ((msgToIdxHandle = _sopen(expandName((switches & SW_F) ? dMSGTOIDX"."MBEXTN : dMSGTOIDX"."dEXTTMP)
                                    , O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, S_IREAD | S_IWRITE)) == -1))
    {
      close(msgIdxHandle);
      logEntry("Can't create purged/packed message files", LOG_ALWAYS, 1);
    }

    if (((msgHdrHandle = _sopen(expandName((switches & SW_F) ? dMSGHDR"."MBEXTN : dMSGHDR"."dEXTTMP)
                               , O_WRONLY | O_BINARY | O_CREAT, SH_DENYWR, S_IREAD | S_IWRITE)) == -1)
        || ((oldHdrHandle = open(expandName(dMSGHDR"."MBEXTN), O_RDONLY | O_BINARY)) == -1))
    {
      close(msgHdrHandle);
      logEntry("Can't create purged/packed message files", LOG_ALWAYS, 1);
    }
    oldHdrSize = (u16)(filelength(oldHdrHandle) / sizeof(msgHdrRec));

    linkArrIndex = 0;
    keepIdx = 0;
    keepBit = 1;

    while ((bufCount = read(oldHdrHandle, msgHdrBuf, HDR_BUFSIZE
                             * sizeof(msgHdrRec)) / sizeof(msgHdrRec)) > 0)
    {
      newBufCount = 0;

      for (count = 0; count < bufCount; count++)
      {
        if ((switches & SW_M)
            && (msgHdrBuf[count].Board == oldBoard))
        {
          msgHdrBuf[count].Board = newBoard;
          msgHdrBuf[count].MsgAttr &= ~(RA_ECHO_OUT | RA_NET_OUT);
        }

        // Recovery Board

        if ((switches & SW_C) && config.recBoard
            && !boardInfo[msgHdrBuf[count].Board].defined)
        {
          msgHdrBuf[count].Board = config.recBoard;
          msgHdrBuf[count].MsgAttr &= ~(RA_ECHO_OUT | RA_NET_OUT);
        }

        if ((!(switches & SW_P)) || (keepHdr[keepIdx] & keepBit))
        {
          if ((switches & SW_T))
          {
            msgTxtRec txtRec;

            lseek(msgTxtHandle, msgHdrBuf[count].StartRec * (u32)sizeof(msgTxtRec), SEEK_SET);
            for (c = 0; c < msgHdrBuf[count].NumRecs; c++)
              if (read(msgTxtHandle, &txtRec, sizeof(msgTxtRec)) == sizeof(msgTxtRec)
                  && c + 1 < msgHdrBuf[count].NumRecs
                  && txtRec.txtLength < 255)
              {
                msgHdrBuf[count].NumRecs = c + 1;
                break;
              }
          }
          if (switches & SW_N)
          {
            renumArray[min(msgHdrBuf[count].MsgNum, RENUM_BUFSIZE - 1)] = ++newMsgNum;
            msgHdrBuf[count].MsgNum = newMsgNum;
          }
          msgHdrBuf[newBufCount] = msgHdrBuf[count];

          msgHdrBuf[newBufCount].ReplyTo    = linkArrayPrevReply[linkArrIndex];
          msgHdrBuf[newBufCount].SeeAlsoNum = linkArrayNextReply[linkArrIndex];

          msgIdxBuf[newBufCount].MsgNum = msgHdrBuf[newBufCount].MsgNum;
          msgIdxBuf[newBufCount].Board  = msgHdrBuf[newBufCount].Board;

          if (keepHdr[keepIdx] & keepBit)
            msgHdrBuf[newBufCount].MsgAttr &= ~RA_DELETED;
          else
            msgHdrBuf[newBufCount].MsgAttr |= RA_DELETED;

          if (switches & SW_R)
            while ((msgHdrBuf[newBufCount].sjLength >= 3)
                   && ((!strnicmp(msgHdrBuf[newBufCount].Subj, "RE:", 3))
                       || (!strnicmp(msgHdrBuf[newBufCount].Subj, "(R)", 3))))
            {
              memcpy(msgHdrBuf[newBufCount].Subj
                   , msgHdrBuf[newBufCount].Subj + 3
                   , msgHdrBuf[newBufCount].sjLength -= 3);

              while ((msgHdrBuf[newBufCount].sjLength > 0)
                     && (*msgHdrBuf[newBufCount].Subj == ' '))
                memcpy(msgHdrBuf[newBufCount].Subj
                       , msgHdrBuf[newBufCount].Subj + 1
                       , --msgHdrBuf[newBufCount].sjLength);
            }

          if (msgHdrBuf[newBufCount].MsgAttr & RA_DELETED)
          {
            memcpy(&(msgToIdxBuf[newBufCount]), "\x0b* Deleted *", 12);
            msgIdxBuf[newBufCount].MsgNum = 0xffff;
          }
          else
            if (msgHdrBuf[newBufCount].MsgAttr & RA_RECEIVED)
              memcpy( &(msgToIdxBuf[newBufCount]), "\x0c* Received *", 13);
            else
              memcpy( &(msgToIdxBuf[newBufCount])
                    , &(msgHdrBuf[newBufCount].wtLength), 36);

          if (switches & SW_P)
          {
            ddd = msgHdrBuf[newBufCount].StartRec;
            mask = bitMaskTab[ddd & 7];
            ddd >>= 3;
            msgHdrBuf[newBufCount].StartRec = keepTxt[ddd].usedCount + bitCountTab[keepTxt[ddd].used & mask];
          }
          newBufCount++;
        }

        if ((keepBit <<= 1) == 0)
        {
          keepBit = 1;
          keepIdx++;
        }
        linkArrIndex++;
      }

      if (newBufCount != 0)
      {
        write(msgHdrHandle,   msgHdrBuf,   newBufCount * sizeof(msgHdrRec));
        write(msgIdxHandle,   msgIdxBuf,   newBufCount * sizeof(msgIdxRec));
        write(msgToIdxHandle, msgToIdxBuf, newBufCount * sizeof(msgToIdxRec));
      }
    }
    chsize(msgHdrHandle, tell(msgHdrHandle));
    newHdrSize = (u16)(filelength(msgHdrHandle) / sizeof(msgHdrRec));

    close(oldHdrHandle);
    close(msgHdrHandle);
    close(msgToIdxHandle);
    close(msgIdxHandle);
    if ((switches & SW_T))
      close(msgTxtHandle);

    free(msgIdxBuf);
    free(msgToIdxBuf);
    free(keepHdr);

    if (switches & SW_N)
    {
      lruBuf = (u16 *)msgHdrBuf;
      temp = 0;

      for (count = 0; count < RENUM_BUFSIZE; count++)
      {
        if (renumArray[count] == 0)
          renumArray[count] = temp;
        else
          temp = renumArray[count];
      }
      if ((lastReadHandle = _sopen(expandName("lastread."MBEXTN), O_RDWR | O_CREAT | O_BINARY, SH_DENYRW, S_IREAD | S_IWRITE)) == -1)
        logEntry("Can't open file LastRead."MBEXTN " for update", LOG_ALWAYS, 0);
      else
      {
        puts("Updating LastRead."MBEXTN"...");

        while ((bufCount = read(lastReadHandle, lruBuf, LRU_BUFSIZE)) > 0)
        {
          for (count = 0; count < (bufCount >> 1); count++)
            lruBuf[count] = renumArray[min(lruBuf[count], RENUM_BUFSIZE - 1)];
          lseek(lastReadHandle, -(s32)bufCount, SEEK_CUR);
          write(lastReadHandle, lruBuf, bufCount);
        }
        close(lastReadHandle);
      }

      if ((usersBBSHandle = open(expandName("users.bbs"), O_RDWR | O_BINARY)) != -1)
      {
        puts("Updating Users.BBS...");
        c = 0;

        if (  config.bbsProgram == BBS_RA20 || config.bbsProgram == BBS_RA25
           || config.bbsProgram == BBS_PROB || config.bbsProgram == BBS_ELEB
           )
        {
          // new RA format and ProBoard format
          if (filelength(usersBBSHandle) % 1016)
          {
            newLine();
            logEntry("Incorrect USERS.BBS version, check BBS program settings!", LOG_ALWAYS, 0);
          }
          else
          {
            while (((lseek(usersBBSHandle, 452 + (1016 * (s32)c++), SEEK_SET) != -1)
                    && (read(usersBBSHandle, &temp4, 4)) == 4))
            {
              temp4 = renumArray[min((u16)temp4, RENUM_BUFSIZE - 1)];
              lseek(usersBBSHandle, -4, SEEK_CUR);
              write(usersBBSHandle, &temp4, 4);
            }
          }
        }
        else
        {
          // original QuickBBS format
          if (filelength(usersBBSHandle) % 158)
          {
            newLine();
            logEntry("Incorrect USERS.BBS version, check BBS program settings!", LOG_ALWAYS, 0);
          }
          else
          {
            while (((lseek(usersBBSHandle, 130 + (158 * (s32)c++), SEEK_SET) != -1)
                    && (read(usersBBSHandle, &temp, 2)) == 2))
            {
              temp = renumArray[min(temp, RENUM_BUFSIZE - 1)];
              lseek(usersBBSHandle, -2, SEEK_CUR);
              write(usersBBSHandle, &temp, 2);
            }
          }
        }
        close(usersBBSHandle);
      }
    }

    if (switches & SW_P)
    {
      msgTxtBuf = (msgTxtRec *)msgHdrBuf;

      puts("Writing MsgTxt."MBEXTN"...");
      if (((msgTxtHandle = _sopen(expandName((switches & SW_F) ? dMSGTXT"."MBEXTN : dMSGTXT"."dEXTTMP)
                                 , O_WRONLY | O_BINARY | O_CREAT, SH_DENYWR, S_IREAD | S_IWRITE)) == -1)
          || ((oldTxtHandle = open(expandName(dMSGTXT"."MBEXTN), O_RDONLY | O_BINARY)) == -1))
      {
        close(msgTxtHandle);
        logEntry("Can't create purged/packed message files", LOG_ALWAYS, 1);
      }
      oldTxtSize = (u16)(filelength(oldTxtHandle) >> 8);

      keepIdx = 0;
      keepBit = 1;

      while ((bufCount = read(oldTxtHandle, msgTxtBuf, TXT_BUFSIZE * sizeof(msgTxtRec)) / sizeof(msgTxtRec)) > 0)
      {
        newBufCount = 0;

        for (count = 0; count < bufCount; count++)
        {
          if (keepTxt[keepIdx].used & keepBit)
          {
            msgTxtBuf[newBufCount] = msgTxtBuf[count];
            newBufCount++;
          }
          if ((keepBit <<= 1) == 0)
          {
            keepBit = 1;
            keepIdx++;
          }
        }

        if (newBufCount != 0)
          write(msgTxtHandle, msgTxtBuf, newBufCount * sizeof(msgTxtRec));
      }
      chsize(msgTxtHandle, tell(msgTxtHandle));
      newTxtSize = (u16)(filelength(msgTxtHandle) >> 8);

      close(oldTxtHandle);
      close(msgTxtHandle);

      free(msgTxtBuf);
      free(keepTxt);
    }

    // Write MSGINFO.BBS

    if (((msgInfoHandle = _sopen(expandName((switches & SW_F) ? dMSGINFO"."MBEXTN : dMSGINFO"."dEXTTMP)
                                , O_RDWR | O_CREAT | O_BINARY, SH_DENYRW, S_IREAD | S_IWRITE)) == -1)
        || (write(msgInfoHandle, &infoRec, sizeof(infoRecType)) == -1))
      logEntry("Can't open file "dMSGINFO"."MBEXTN " for output", LOG_ALWAYS, 0);
    close(msgInfoHandle);

    if (!(switches & SW_F))
    {
      if (switches & SW_B)
      {
        rename(expandName(dMSGHDR"."MBEXTN),    expandName(dMSGHDR".bak"));
        rename(expandName(dMSGIDX"."MBEXTN),    expandName(dMSGIDX".bak"));
        rename(expandName(dMSGTOIDX"."MBEXTN),  expandName(dMSGTOIDX".bak"));
        rename(expandName(dMSGTXT"."MBEXTN),    expandName(dMSGTXT".bak"));
        rename(expandName(dMSGINFO"."MBEXTN),   expandName(dMSGINFO".bak"));
      }
      else
      {
        unlink(expandName(dMSGHDR"."MBEXTN));
        unlink(expandName(dMSGIDX"."MBEXTN));
        unlink(expandName(dMSGTOIDX"."MBEXTN));
        unlink(expandName(dMSGTXT"."MBEXTN));
        unlink(expandName(dMSGINFO"."MBEXTN));
      }

      rename(expandName(dMSGHDR"."dEXTTMP),   expandName(dMSGHDR"."MBEXTN));
      rename(expandName(dMSGIDX"."dEXTTMP),   expandName(dMSGIDX"."MBEXTN));
      rename(expandName(dMSGTOIDX"."dEXTTMP), expandName(dMSGTOIDX"."MBEXTN));
      rename(expandName(dMSGTXT"."dEXTTMP),   expandName(dMSGTXT"."MBEXTN));
      rename(expandName(dMSGINFO"."dEXTTMP),  expandName(dMSGINFO"."MBEXTN));
    }

    if (switches & SW_P)
      flogEntry(LOG_STATS, 0, "Space saved ("MBNAME ") : %u bytes"
               , oldHdrSize * sizeof(msgHdrRec  )
               + oldHdrSize * sizeof(msgToIdxRec)
               + oldHdrSize * sizeof(msgIdxRec  )
               + oldTxtSize * sizeof(msgTxtRec  )
               - newHdrSize * sizeof(msgHdrRec  )
               - newHdrSize * sizeof(msgToIdxRec)
               - newHdrSize * sizeof(msgIdxRec  )
               - newTxtSize * sizeof(msgTxtRec  )
               );

    free(linkArraySubjectCrcHigh);
    free(linkArraySubjectCrcLow);
    free(linkArrayMsgNum);
    free(linkArrayNextReply);
    free(linkArrayPrevReply);

JAMonly:

    if ((!(switches & (SW_H | SW_K | SW_M)))
        && (undeleteBoard == 0 || !(switches & SW_U)))
    {
      u16 count2;
      u32 processed;
      headerType  *header;
      rawEchoType *areaBuf;

      temp = 0;
      if (openConfig(CFG_ECHOAREAS, &header, (void *)&areaBuf))
      {
        spaceSavedJAM = 0;
        processed = 0;
        for (count = 0; count < header->totalRecords; count++)
        {
          returnTimeSlice(0);
          getRec(CFG_ECHOAREAS, count);
          if (*areaBuf->msgBasePath && areaBuf->options.active
              && (!(switches & SW_Q) || areaBuf->stat.tossedTo))
          {
            if (argc >= 3 && *argv[2] != '/')
            {
              int cont = 0;
              count2 = 2;
              while (stricmp(argv[count2], areaBuf->areaName))
                if (argc <= ++count2 || *argv[count2] == '/')
                {
                  cont = 1;
                  break;
                }
              if (cont)
                continue;
              processed |= 1 << (count2 - 2);
            }
            if (!temp)
            {
              puts("Processing JAM areas...");
              temp = 1;
            }
            if (!JAMmaint(areaBuf, switches, config.sysopName, &spaceSavedJAM) && areaBuf->stat.tossedTo)
            {
              areaBuf->stat.tossedTo = 0;
              putRec(CFG_ECHOAREAS, count);
            }
          }
        }
        closeConfig(CFG_ECHOAREAS);
        count2 = 1;
        while (++count2 < argc && *argv[count2] != '/')
          if (!(processed & (1 << (count2 - 2))))
            flogEntry(LOG_ALWAYS, 0, "%s is not a JAM area or does not exist", argv[count2]);

        if (spaceSavedJAM > 0)
          flogEntry(LOG_STATS, 0, "Space saved (JAM) : %d bytes", spaceSavedJAM);
      }
    }

    logActive();
  }
  else
    if ((argc >= 2)
        && ((stricmp(argv[1], "T") == 0) || (stricmp(argv[1], "SORT") == 0)))
    {
      if ((argc >= 3) && ((argv[2][0] == '?') || (argv[2][1] == '?')))
      {
        puts("Usage:\n\n"
             "    FTools Sort [/A]\n\n"
             "Switches:\n\n"
             "    /A   Sort ALL messages\n"
             "         WARNING: It is NOT possible to sort all messages\n"
             "         and still have correct lastread pointers!");
        return 0;
      }
      switches = getSwitchFT(&argc, argv, SW_A);

      initLog("SORT", switches);

      index = 0;

      if ((switches & SW_A) == 0)
      {
        if ((msgIdxBuf = malloc(512 * sizeof(msgIdxRec))) == NULL)
          logEntry("Not enough free memory",     LOG_ALWAYS, 2);
        if ((lastReadHandle = open(expandName("lastread."MBEXTN), O_RDONLY | O_BINARY)) == -1)
          logEntry("Can't open LastRead."MBEXTN, LOG_ALWAYS, 1);
        maxRead = 0;
        while (read(lastReadHandle, &lastReadRec, 400) == 400)
          for (count = 0; count < MBBOARDS; count++)
            if (maxRead < lastReadRec[count])
              maxRead = lastReadRec[count];

        close(lastReadHandle);

        if ((msgIdxHandle = open(expandName(dMSGIDX"."MBEXTN), O_RDONLY | O_BINARY)) == -1)
          logEntry("Can't open MsgIdx."MBEXTN, LOG_ALWAYS, 1);
        index = 0;
        while ((msgIdxBufCount = (read(msgIdxHandle, msgIdxBuf, 512 * sizeof(msgIdxRec)) / 3)) != 0)
          for (count = 0; count < msgIdxBufCount; count++)
            if (msgIdxBuf[count].MsgNum <= maxRead)
              index++;

        close(msgIdxHandle);
      }

      config.mbOptions.sortNew = 1;
      config.mbOptions.updateChains = 1;
      sortBBS(index, 0);
      logActive();
    }
    else
      if ((argc >= 2)
          && ((stricmp(argv[1], "S") == 0) || (stricmp(argv[1], "STAT") == 0)))
      {
        if ((argc >= 3) && ((argv[2][0] == '?') || (argv[2][1] == '?')))
        {
          puts("Usage:\n\n"
               "    FTools Stat\n");
          return 0;
        }
        initLog("STAT", 0);

        puts("Analyzing the message base...");

        if ((msgHdrHandle = _sopen(expandName(dMSGHDR"."MBEXTN), O_RDONLY | O_BINARY, SH_DENYWR)) == -1)
          logEntry("Can't open MsgHdr."MBEXTN " for reading", LOG_ALWAYS, 1);

        totalMsgs    = 0;
        totalDeleted = 0;
        totalLocal   = 0;
        totalTxtHdr  = 0;
        highMsgNum   = 0;
        lowMsgNum    = 0;
        oldHdrSize   = (u16)(filelength(msgHdrHandle) / sizeof(msgHdrRec));

        while ((bufCount = read(msgHdrHandle, msgHdrBuf, HDR_BUFSIZE * sizeof(msgHdrRec)) / sizeof(msgHdrRec)) > 0)
        {
          totalMsgs += bufCount;
          for (count = 0; count < bufCount; count++)
          {
            lowMsgNum  = min(lowMsgNum - 1, msgHdrBuf[count].MsgNum - 1) + 1;
            highMsgNum = max(highMsgNum, msgHdrBuf[count].MsgNum);

            if (msgHdrBuf[count].MsgAttr & RA_DELETED)
              totalDeleted++;
            else
            {
              if (msgHdrBuf[count].MsgAttr & RA_LOCAL)
                totalLocal++;
              totalTxtHdr += msgHdrBuf[count].NumRecs;
            }
          }
        }
        close(msgHdrHandle);

        if (((msgTxtHandle = _sopen(expandName(dMSGTXT"."MBEXTN), O_RDONLY | O_BINARY, SH_DENYRW)) == -1)
            || ((fileSize = filelength(msgTxtHandle)) == -1)
            || (close(msgTxtHandle) == -1))
          logEntry("File status request error", LOG_ALWAYS, 1);
        totalTxtBBS = (u16)(fileSize >> 8);

        newLine();

        flogEntry(LOG_STATS, 0, "Messages are numbered from %u to %u", lowMsgNum, highMsgNum);

        newLine();

        flogEntry(LOG_STATS, 0, "Total messages : %5u", totalMsgs);
        flogEntry(LOG_STATS, 0, "* Deleted      : %5u  (%3u%%)", totalDeleted, totalDeleted * 100 / (totalMsgs ? totalMsgs : 1));
        flogEntry(LOG_STATS, 0, "* Active       : %5u  (%3u%%)", totalMsgs - totalDeleted, (totalMsgs - totalDeleted) * 100 / (totalMsgs ? totalMsgs : 1));
        flogEntry(LOG_STATS, 0, "  - Inbound    : %5u  (%3u%%)", totalMsgs - totalDeleted - totalLocal
                 , (totalMsgs - totalDeleted - totalLocal) * 100 / (totalMsgs - totalDeleted ? totalMsgs - totalDeleted : 1));
        flogEntry(LOG_STATS, 0, "  - Local      : %5u  (%3u%%)", totalLocal, totalLocal * 100 / (totalMsgs - totalDeleted ? totalMsgs - totalDeleted : 1));

        newLine();

        flogEntry(LOG_STATS, 0, "MSGTXT records : %5u", (u32)totalTxtBBS);
        flogEntry(LOG_STATS, 0, "* Used         : %5u  (%3u%%)", totalTxtHdr, totalTxtHdr * 100 / (totalTxtBBS ? totalTxtBBS : 1));
        flogEntry(LOG_STATS, 0, "* Not used     : %5u  (%3u%%)", (totalTxtBBS - totalTxtHdr), (totalTxtBBS - totalTxtHdr) * 100 / (totalTxtBBS ? totalTxtBBS : 1));

        newLine();

        flogEntry(LOG_STATS, 0, "Message base   : %9u bytes", (u32)totalTxtBBS * 256 + totalMsgs * 226 + 406);
#ifdef __CANUSE64BIT
        flogEntry(LOG_STATS, 0, "Disk space     : %9s bytes free on message base drive", fmtU64(diskFree64(config.bbsPath)));
#else
        flogEntry(LOG_STATS, 0, "Disk space     : %9u bytes free on message base drive", diskFree(config.bbsPath));
#endif

        newLine();
      }
      else
#endif
  if ((argc >= 2)
      && ((stricmp(argv[1], "N") == 0) || (stricmp(argv[1], "NOTIFY") == 0)))
    // call Notify

    return notify(argc, argv);
  else
    if (argc >= 2 && (stricmp(argv[1], "P") == 0 || stricmp(argv[1], "POST") == 0))
    {
      if (argc < 4 || (argv[2][0] == '?' || argv[2][1] == '?'))
      {
        puts("Usage:\n\n"
             "    FTools Post <file>|- <area tag> [options] [/C] [/D] [/H] [/P]\n\n"
             "Options with defaults:\n\n"
             "    -from    SysOp name as defined in FSetup\n"
             "    -to      'All'\n"
             "    -subj    <file name>\n"
             "    -dest    <node number> (netmail only)\n"
             "    -aka     Board dependant\n\n"
             "Switches:\n"
             "    /C  Crash status  (netmail only)     /R  File request       (netmail only)\n"
             "    /H  Hold status   (netmail only)     /F  File attach        (netmail only)\n"
             "    /D  Direct status (netmail only)     /E  Erase sent file    (file attach)\n"
             "    /K  Kill/sent     (netmail only)     /T  Truncate sent file (file attach)\n"
             "    /P  Private status");
        return 0;
      }
      switches = getSwitchFT(&argc, argv, SW_C | SW_H | SW_D | SW_K | SW_P | SW_R | SW_F | SW_E | SW_T);

      initLog("POST", switches);

      if ((message = malloc(INTMSG_SIZE)) == NULL)
        logEntry("Not enough memory available", LOG_ALWAYS, 2);

      memset(message, 0, INTMSG_SIZE);

      strcpy(message->toUserName  , "All");
      strcpy(message->fromUserName, config.sysopName);

      postBoard = 0;
      srcAka = 0;
      if (stricmp(argv[3], "#netdir") != 0)
        postBoard = getBoardNum(argv[3], 1, &srcAka, &areaPtr);
      isNetmail = postBoard == 0 || memicmp(argv[3], "#net", 4) == 0;

      for (count = 4; count < argc; count++)
      {
        if (stricmp(argv[count], "-from") == 0)
        {
          if (count + 1 < argc && *argv[count + 1] != '-')
                strncpy(message->fromUserName, argv[++count], 35);
          else
            *message->fromUserName = 0;
        }
        else
          if (stricmp(argv[count], "-to") == 0)
          {
            if (count + 1 < argc && *argv[count + 1] != '-')
                strncpy(message->toUserName, argv[++count], 35);
            else
              *message->toUserName = 0;
          }
          else
            if (stricmp(argv[count], "-subj") == 0)
            {
              if (count + 1 < argc && *argv[count + 1] != '-')
                strncpy(message->subject, argv[++count], 71);
              else
                *message->subject = 0;
            }
            else
              if (stricmp(argv[count], "-aka") == 0)
              {
                if (count + 1 < argc && *argv[count + 1] != '-'
                    && (temp = atoi(argv[++count])) < MAX_AKAS)
                  srcAka = temp;
              }
              else
                if (isNetmail && (stricmp(argv[count], "-dest") == 0))
                {
                  if (count + 1 < argc && *argv[count + 1] != '-')
                  {
                    temp = 0;
                    readNodeNum(argv[++count], &message->destNode, &temp);
                    if (temp < 3)
                      logEntry("Bad node number", LOG_ALWAYS, 4);
                  }
                }
                else
                  flogEntry(LOG_ALWAYS, 0, "Illegal command line option: %s", argv[count]);
      }

      if ((switches & (SW_F | SW_R)) && !*message->subject)
        logEntry("-Subj <file name> is required for file attaches and file requests", LOG_ALWAYS, 1);

      if (switches & SW_F)
      {
        if ((helpPtr = _fullpath(NULL, message->subject, 72)) == NULL)
          logEntry("Can't locate file to be attached", LOG_ALWAYS, 1);

        strncpy(message->subject, strlwr(helpPtr), 71);
        free(helpPtr);
      }

      if (strcmp(argv[2], "-"))
      {
        if ((lastReadHandle = open(argv[2], O_RDONLY | O_BINARY)) == -1)
          logEntry("Can't locate text file", LOG_ALWAYS, 1);

        read(lastReadHandle, message->text, TEXT_SIZE - 4096);
        close(lastReadHandle);
        removeLf(message->text);
        if (!*message->subject)
          strcpy(message->subject, argv[2]);
      }

      if (isNetmail && message->destNode.zone == 0)
        logEntry("Destination node not defined", LOG_ALWAYS, 4);

      message->srcNode = config.akaList[srcAka].nodeNum;

      // netmail
      if (postBoard == 0)
      {
        message->attribute = LOCAL | KILLSENT
                             | ((switches & SW_P) ? PRIVATE : 0)
                             | ((switches & SW_K) ? KILLSENT : 0)
                             | ((switches & SW_R) ? FILE_REQ : 0)
                             | ((switches & SW_F) ? FILE_ATT : 0)
                             | ((switches & SW_C) ? CRASH : ((switches & SW_H) ? HOLDPICKUP : 0));
        if (switches & (SW_D | SW_E | SW_T))
        {
          sprintf( tempStr, "\1FLAGS%s%s\r", (switches & SW_D) ? " DIR" : ""
                 , (switches & SW_E) ? " KFS" : (switches & SW_T) ? " TFS" : "");
          insertLine(message->text, tempStr);
        }

        if (writeNetMsg(message, srcAka, &message->destNode, PKT_TYPE_2PLUS, 0xFFFF) == 0)
          flogEntry(LOG_ALWAYS, 0, "Sending netmail message to node %s", nodeStr(&message->destNode));
      }
      else
        // JAM
        if (postBoard == -1)
        {
          time_t t = time(NULL);
          struct tm *tm = localtime(&t);  // localtime ok!

          message->hours   = tm->tm_hour;
          message->minutes = tm->tm_min;
          message->seconds = tm->tm_sec;
          message->day     = tm->tm_mday;
          message->month   = tm->tm_mon + 1;
          message->year    = tm->tm_year + 1900;

          message->attribute = ((switches & SW_C) ? CRASH   : 0)
                             | ((switches & SW_P) ? PRIVATE : 0) | LOCAL;
          addInfo(message, isNetmail);

          // write JAM message here
          jam_writemsg(areaPtr->msgBasePath, message, 1);
          jam_closeall();

          flogEntry(LOG_ALWAYS, 0, "Posting message in area %s (JAM base %s)", areaPtr->areaName, areaPtr->msgBasePath);
        }
        else
        {
          message->attribute = ((switches & SW_C) ? CRASH   : 0)
                             | ((switches & SW_P) ? PRIVATE : 0);
          addInfo(message, isNetmail);

          openBBSWr();
          if (writeBBS(message, postBoard, isNetmail) == 0)
            flogEntry(LOG_ALWAYS, 0, "Posting message in area %s ("MBNAME " board %u)", areaPtr->areaName, postBoard);

          closeBBS();
        }
    }
    else
      if ((argc >= 2)
          && ((stricmp(argv[1], "N") == 0) || (stricmp(argv[1], "NOTIFY") == 0)))
        // call Notify

        return notify(argc, argv);
      else
        if ((argc >= 2)
            && ((stricmp(argv[1], "E") == 0) || (stricmp(argv[1], "EXPORT") == 0)))
          // call Export

          return export (argc, argv);
        else
          if ((argc >= 2)
              && ((stricmp(argv[1], "G") == 0) || (stricmp(argv[1], "MSGM") == 0)))
          {
            if ((argc >= 3) && ((argv[2][0] == '?') || (argv[2][1] == '?')))
            {
              puts("Usage:\n\n"
                   "    FTools MsgM [-net] [-sent] [-rcvd] [-pmail] [/N]\n\n"
                   "Options:\n\n"
                   "    [-net]    Netmail directory\n"
                   "    [-sent]   Sent messages directory\n"
                   "    [-rcvd]   Received messages directory\n"
                   "    [-pmail]  Personal mail directory\n\n"
                   "Switches:\n\n"
                   "    /N   Renumber messages");
              return 0;
            }
            switches = getSwitchFT(&argc, argv, SW_N);

            initLog("MSGM", switches);

            if (switches == 0)
            {
              logEntry("Nothing to do!", LOG_ALWAYS, 0);

              return 0;
            }

            if ((msgRenumBuf = malloc(0xfff0)) == NULL)
              logEntry("Not enough free memory", LOG_ALWAYS, 2);

            for (c = 2; c < argc; c++)
            {
              helpPtr2 = NULL;
              if (stricmp(argv[c], "-net") == 0)
              {
                helpPtr2 = stpcpy(tempStr2, config.netPath);
                helpPtr3 = stpcpy(tempStr3, config.netPath);
                if (*config.semaphorePath && (config.mailer == 1))
                {
                  sprintf(tempStr, "%simrenum.now", config.semaphorePath);
                  if (((semaHandle = _sopen(tempStr, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, S_IREAD | S_IWRITE)) == -1)
                      || (lock(semaHandle, 0, 1000) == -1))
                  {
                    close(semaHandle);
                    helpPtr2 = NULL;
                    logEntry("Can't renumber netmail directory", LOG_ALWAYS, 0);
                  }
                }
                logEntry("Processing netmail directory", LOG_ALWAYS, 0);
              }
              else
                if (stricmp(argv[c], "-sent") == 0)
                {
                  helpPtr2 = stpcpy(tempStr2, config.sentPath);
                  helpPtr3 = stpcpy(tempStr3, config.sentPath);
                  logEntry("Processing sent messages directory", LOG_ALWAYS, 0);
                }
                else
                  if (stricmp(argv[c], "-rcvd") == 0)
                  {
                    helpPtr2 = stpcpy(tempStr2, config.rcvdPath);
                    helpPtr3 = stpcpy(tempStr3, config.rcvdPath);
                    logEntry("Processing received messages directory", LOG_ALWAYS, 0);
                  }
                  else
                    if (stricmp(argv[c], "-pmail") == 0)
                    {
                      helpPtr2 = stpcpy(tempStr2, config.pmailPath);
                      helpPtr3 = stpcpy(tempStr3, config.pmailPath);
                      logEntry("Processing personal mail directory", LOG_ALWAYS, 0);
                    }
                    else
                      flogEntry(LOG_ALWAYS, 0, "Unknown command line option: %s", argv[c]);

              if (helpPtr2 != NULL)
              {
                if (switches & SW_N)
                {
                  bufCount = 0;
                  *helpPtr2 = 0;
                  if ((dir = opendir(tempStr2)) != NULL)
                  {
                    while ((bufCount < 0x8ff8) && (ent = readdir(dir)) != NULL)
                    {
                      if (!match_spec("*.msg", ent->d_name))
                        continue;

                      msgNum = (u16)strtoul(ent->d_name, &helpPtr, 10);
                      if (msgNum != 0 && *helpPtr == '.')
                      {
                        low = 0;
                        high = bufCount;
                        while (low < high)
                        {
                          mid = (low + high) >> 1;
                          if (msgNum > msgRenumBuf[mid])
                            low = mid + 1;
                          else
                            high = mid;
                        }
                        memmove(&msgRenumBuf[low + 1], &msgRenumBuf[low], (bufCount - low) << 1);
                        msgRenumBuf[low] = msgNum;
                        bufCount++;
                      }
                    }
                    closedir(dir);
                  }
                  ddd = 0;
                  for (count = 0; count < bufCount; count++)
                    if (count + 1 != msgRenumBuf[count])
                    {
                      sprintf(helpPtr2, "%u.msg", msgRenumBuf[count]);
                      sprintf(helpPtr3, "%u.msg", count + 1);
                      if (rename(tempStr2, tempStr3) == 0)
                      {
                        printf("\r- Renumbering messages: %u "dARROW" %u", msgRenumBuf[count], count + 1);
                        ddd++;
                      }
                    }
                  if (ddd)
                    newLine();

                  strcpy(helpPtr2, "lastread");
                  if ((lastReadHandle = _sopen(tempStr2, O_RDWR | O_BINARY, SH_DENYRW)) != -1)
                  {
                    while (((signed)(newBufCount = read(lastReadHandle, &lastReadRec, 400))) > 0)
                    {
                      returnTimeSlice(0);
                      for (count = 0; count < (newBufCount >> 1); count++)
                      {
                        ddd = 0;
                        while ((ddd < bufCount) && (msgRenumBuf[ddd] < lastReadRec[count]))
                          ddd++;
                        lastReadRec[count] = ddd + 1;
                      }
                      lseek(lastReadHandle, -(signed)newBufCount, SEEK_CUR);
                      write(lastReadHandle, &lastReadRec, newBufCount);
                    }
                    close(lastReadHandle);
                  }
                }
                if (((config.mailer <= 2) || (config.mailer >= 4))
                    && *config.semaphorePath && (stricmp(argv[c], "-net") == 0))
                {
                  strcpy(tempStr, semaphore[config.mailer]);
                  touch(config.semaphorePath, tempStr, "");
                  if (config.mailer <= 1)
                  {
                    tempStr[1] = 'M';
                    touch(config.semaphorePath, tempStr, "");
                  }
                  if (config.mailer == 1)
                  {
                    unlock(semaHandle, 0, 1000);
                    close(semaHandle);
                  }
                }
              }
            }
          }
  else
    if ((argc >= 2)
        && ((stricmp(argv[1], "W") == 0) || (stricmp(argv[1], "AddNew") == 0)))
    {
      if (argc >= 3 && (argv[2][0] == '?' || argv[2][1] == '?'))
      {
        puts("Usage:\n\n"
             "    FTools AddNew [/A]\n\n"
             "Switches:\n\n"
             "    /A   AutoUpdate config files");
        return 0;
      }
      switches = getSwitchFT(&argc, argv, SW_A);
      initLog("ADDNEW", switches);
      addNew(switches);

      return 0;
    }
    else
      if ((argc >= 2)
          && ((stricmp(argv[1], "A") == 0) || (stricmp(argv[1], "ABOUT") == 0)))
      {
        About();
        return 0;
      }
      else
        Usage();

  return 0;
}
//----------------------------------------------------------------------------
void myexit(void)
{
  if (lastSavedUniqID != 0 && lastSavedUniqID != config.lastUniqueID)
  {
    // UniqID changed, save it
    fhandle     configHandle;
    tempStrType tempStr;

    strcpy(stpcpy(tempStr, configPath), dCFGFNAME);

    if ( (configHandle = _sopen(tempStr, O_WRONLY | O_BINARY, SH_DENYWR, S_IREAD | S_IWRITE)) == -1
       || lseek(configHandle, offsetof(configType, lastUniqueID), SEEK_SET) == -1L
       || write(configHandle, &config.lastUniqueID, sizeof(config.lastUniqueID)) < sizeof(config.lastUniqueID)
       || close(configHandle) == -1
       )
      puts("\nCan't write "dCFGFNAME);
  }

#ifdef _DEBUG0
  putStr("\nPress any key to continue... ");
  getch();
  newLine();
#endif
}
//----------------------------------------------------------------------------
