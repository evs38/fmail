//---------------------------------------------------------------------------
//
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
//---------------------------------------------------------------------------

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "fmail.h"

#include "areainfo.h"
#include "bits.h"
#include "cfgfile.h"
#include "fdfolder.h"
#include "fs_util.h"
#include "imfolder.h"
#include "minmax.h"
#include "msgra.h"
#include "os.h"
#include "stpcpy.h"
#include "utils.h"
#include "version.h"

#ifndef FSETUP
#include "log.h"

#define displayMessage(msg) logEntry(msg, LOG_ALWAYS, 0)

//---------------------------------------------------------------------------
static s16 groupToChar(s32 group)
{
  s16 c = 'A';

  if (group)
    while (!(group & 1))
    {
      group >>= 1;
      c++;
    }
  return c;
}

#ifndef GOLDBASE
char getGroupChar(s32 groupCode)
{
  char count = 0;

  while (groupCode != 0)
  {
    groupCode >>= 1;
    count++;
  }
  count += 'A' - 1;
  return (count);
}
#endif
#else   // FSETUP
#include "window.h"
#endif  // FSETUP

extern configType config;
extern char configPath[FILENAME_MAX];

#include "pshpack1.h"

typedef struct
{
  char      NameLength;
  char      Name[30];
  char      QwkNameLength;
  char      QwkName[12];
  char      Typ;        /* 0=Standard 1=Net 3=Echo */
  char      Kinds;      /* 0=Private & Public
                             1=Private 2=Public 3=Read-Only */
  char      Aliases;    /* 0=no aliases,1=yes,2=ask alias,3=name/alias */
  u16       ReadSecLvl;
  u32       ReadFlags;
  u16       WriteSecLvl;
  u32       WriteFlags;
  u16       SysopSecLvl;
  u32       SysopFlags;
  char      Group;
  char      Replystatus; /* 0=normal,1=net/normal,2=net only,3=no replies */
  char      Age;
  char      Attrib;
  char      UseAka;
} SBBSBoardRecType;


typedef struct
{
  char      NameLength;
  char      Name[40];
  char      Typ;
  char      Kinds; /* 0=Both,1=Private,2=Public,3=ROnly */
  char      Combined; /* 0/1 */
  char      Aliases; /* 0=No, 1=Maybe, 2=Yes */
  char      Aka;
  char      OriginLineLength;
  char      OriginLine[58];
  char      AllowDelete; /* 0/1 */
  u16       KeepCnt,
  KillRcvd,
  KillOld;
  u16       ReadSecLvl;
  u32       ReadFlags;
  u16       WriteSecLvl;
  u32       WriteFlags;
  u16       TemplateSecLvl;
  u32       TemplateFlags;
  u16       SysopSecLvl;
  u32       SysopFlags;
  u16       FileArea;  /* for Fmail */
  char      Group;
  char      Spare[9];
} QBBSBoardRecType;


/*---------------------------------------------------------------------------*/

typedef int8_t Byte;
typedef int8_t Boolean;
typedef u16    Word;

typedef struct      /* Fidonet Style Address (23 Bytes) */
{
  Byte Zone;          /* Zone, 1   = N. America */
  Word Net;           /* Net,  120 = SE Michigan */
  Word Node;          /* Node, 116 = CRIMP BBS */
  Word Point;         /* Point, 99% of the time = 0 */
  char Domain[16];    /* As in FIDONET */
} OldAddressType;

typedef struct      /* Fidonet Style Address (23 Bytes) */
{
  Word Zone;          /* Zone, 1   = N. America */
  Word Net;           /* Net,  120 = SE Michigan */
  Word Node;          /* Node, 116 = CRIMP BBS */
  Word Point;         /* Point, 99% of the time = 0 */
  char Domain[16];    /* As in FIDONET */
} AddressType;

#define MAXSUBOPS   10      /* Maximum number of message section SubOps */

#define AtNo        0   /* Anonymous messages not allowed */
#define AtYes       1   /* Anonymous messages allowed */
#define AtForced    2   /* Messages forced anonymous */
#define AtUnused    3   /* Reserved */

typedef Byte NoYesForcedType;   /* Message section type */

typedef struct      /* Set of Fido attr flags */
{
  u32 fPrivate : 1;                   /* Private */
  u32 fCrash : 1;                     /* Crash */
  u32 fReceived : 1;                  /* Received */
  u32 fSent : 1;                      /* Sent */
  u32 fFileAttached : 1;              /* File attach */
  u32 fInTransit : 1;                 /* In-transit */
  u32 fOrphan : 1;                    /* Orphan */
  u32 fKillSent : 1;                  /* Kill/sent */
  u32 fLocal : 1;                     /* Local */
  u32 fHoldForPickup : 1;             /* Hold */
  u32 fUnusedBit10 : 1;               /* Reserved */
  u32 fFileRequest : 1;               /* File request */
  u32 fReturnReceiptRequest : 1;      /* Return receipt request */
  u32 fIsReturnReceipt : 1;           /* Return receipt */
  u32 fAuditRequest : 1;              /* Audit request */
  u32 fFileUpdateRequest : 1;         /* File update request */
  u32 fRaDeleted : 1;
  u32 fRaNetPendingExport : 1;
  u32 fRaNetmailMessage : 1;
  u32 fRaEchoPendingExport : 1;
  u32 unused : 12;
} MsgAttrFlagSet;

#define UUMBBSTYLE      0       /* Private Mail Board */
#define LOCALSTYLE      1       /* Local Board */
#define ECHOSTYLE       2       /* Echomail                                                            Needs Origin */
#define NETMAILSTYLE    3       /* Netmail board (Only one I hope!) */
#define GROUPSTYLE      4       /* For Groupmail Support */

typedef Byte MBstyle;       /* Message section style flags */

#define UUMBTYPE        0       /* Was For Netmail Board */
#define FIDOFORMAT      1       /* Fido 1.Msg Format */
#define RAFORMAT        2       /* Remote Access Format */

typedef Byte MBtype;        /* Message section type flags */

typedef Byte DefaultYesNoType;      /* Default/yes/no types */

typedef char ArFlagType;    /* AR flags ('@'..'Z') */
typedef Byte ArFlagset[4];  /* AR flags:                    */
/* Byte 0 bits 0-7 = flags @..G */
/* Byte 1 bits 0-7 = flags H..O */
/* Byte 2 bits 0-7 = flags P..W */
/* Byte 3 bits 0-3 = flags X..Z */
/*        bits 4-7 = reserved   */

typedef struct      /* Message boards - MBOARDS.DAT */
{
  char NameLen;
  char Name[64];                  /* Name of the Board */
  MBstyle Mstyle;                 /* Local/Echo/Netmail */
  MBtype Mtype;                   /* Message Board Type */
  Byte RaBoard;                   /* Board Number if RA/QBBS type */
  char PathLen;
  char Path[64];                  /* Directory PathName */
  char OriginLineLen;
  char OriginLine[65];            /* Origin Line */
  ArFlagType AccessAR;            /* AR flag Required to Access */
  ArFlagType PostAR;              /* AR flag required to Post */
  Byte AccessSL;                  /* Security Level Required to Access */
  Byte PostSL;                    /* Security Level Required to Post */
  Word MsgCount;                  /* Count of Msgs on the Board */
  Word MaxMsgs;                   /* Max Number of Messages */
  Word uuMaxOld;                  /* Max Days for Messages */
  char PassWordLen;
  char PassWord[16];              /* PassWord Required */
  NoYesForcedType Anon;           /* Anonymous Type */
  Boolean AllowAnsi;              /* Should we allow ANSI */
  NoYesForcedType AllowHandle;    /* Should we allow handles */

  /*********************************************************/
  /* Message Board SubOpts List  - Up to 10 - User Numbers */
  /*********************************************************/

  s16  SubOps[MAXSUBOPS + 1];    /* SubOps - Item 0 = How many */

  char EchoTagLen;
  char EchoTag[32];               /* Echo Tag for Writing ECHOMAIL.BBS */
  Boolean UseOtherAddress;        /* Use something other than system */
  OldAddressType OldOtherAddress; /* The Address to use! (not used) */
  Byte MenuNumber;                /* Default read message number */
  /* (if 0, use system default)  */
  char PrePostFileLen;
  char PrePostFile[8];            /* Prepost file name */
  Byte MinMsgs;                   /* Minimum number of messages */
  char QuoteStartLen;
  char QuoteStart[70];            /* Override starting quote */
  char QuoteEndLen;
  char QuoteEnd[70];              /* Override ending quote */
  u16  QwkConf;                   /* Qwk conference number */
  Byte GroupNumber;               /* What group the board belongs */
  AddressType OtherAddress;       /* The address to use */
  NoYesForcedType RestrictPrivate;/* Private mail status */
  MsgAttrFlagSet DefaultAttr;     /* Default message flags */
  char QwkNameLen;
  char QwkName[10];               /* Qwk conference name */
  char reserved;
} MboardType;

/*---------------------------------------------------------------------------*/
/* PROBOARD 2.02 STRUCTURES */
/*
----------------------------------------------------------------------------
 MESSAGES.PB
----------------------------------------------------------------------------
*/

typedef struct
{
  u16  areaNum;              /* # of message area (1-10000)                    */
  u16  hudsonBase;           /* Number of Hudson message base                  */
  char name[81];             /* Name of message area                           */
  u8   msgType;              /* Kind of message area (Net/Echo/Local)          */
  u8   msgKind;              /* Type of message (Private only/Public only/...) */
  u8   msgBaseType;          /* Type of message base                           */
  char path[81];             /* Path to Squish or *.MSG                        */
  u8   flags;                /* Alias allowed/forced/prohibited                */
  u16  readLevel;            /* Minimum level needed to read msgs              */
  u32  readFlags;            /* flags needed to read msgs                      */
  u32  readFlagsNot;         /* flags non-grata to read msgs                   */
  u16  writeLevel;           /* Minimum level needed to write msgs             */
  u32  writeFlags;           /* flags needed to write msgs                     */
  u32  writeFlagsNot;        /* flags non-grata to read msgs                   */
  u16  sysopLevel;           /* Minimum level needed to change msgs            */
  u32  sysopFlags;           /* flags needed to change msgs                    */
  u32  sysopFlagsNot;        /* flags non-grata to read msgs                   */

  char origin[62];           /* Origin line                                    */
  u16  aka;                  /* AKA                                            */

  u16  rcvKillDays;          /* Kill received after xx days                    */
  u16  msgKillDays;          /* Kill after xx days                             */
  u16  maxMsgs;              /* Max # msgs                                     */

  char sysop[36];            /* Area Sysop                                     */
  u16  replyBoard;           /* Area number where replies should go            */

  char echoTag[61];          /* Echomail Tag Name                              */
  char qwkTag[13];           /* QWK Area Name                                  */

  u8   groups[4];            /* Groups belonging to                            */
  u8   allGroups;            /* Belongs to all groups                          */
  u8   minAge;               /* Minimum age for this area                      */

  u8   extra[112];
} proBoardType;

#include "poppack.h"

#define MSGTYPE_BOTH     0 /* Private/Public */
#define MSGTYPE_PVT      1 /* Private Only   */
#define MSGTYPE_PUBLIC   2 /* Public Only    */
#define MSGTYPE_TOALL    3 /* To All         */

#define MSGKIND_LOCAL    0 /* Local          */
#define MSGKIND_NET      1 /* NetMail        */
#define MSGKIND_ECHO     2 /* EchoMail       */
#define MSGKIND_PVTECHO  3 /* Pvt EchoMail   */

#define MSGBASE_HUDSON   0
#define MSGBASE_SQUISH   1
#define MSGBASE_SDM      2
#define MSGBASE_JAM      3


/*---------------------------------------------------------------------------*/

static u16 nodeStrIndex;

static char nodeNameStr[2][24];

static char *nodeStrP(nodeNumType *nodeNum)
{
  char *tempPtr;

  tempPtr = nodeNameStr[nodeStrIndex = !nodeStrIndex];
  if (nodeNum->zone != 0)
    tempPtr += sprintf(tempPtr, "%u:", nodeNum->zone);

  sprintf(tempPtr, "%u/%u.%u", nodeNum->net, nodeNum->node, nodeNum->point);

  return nodeNameStr[nodeStrIndex];
}

#ifndef GOLDBASE
static u32 bitSwap(const u8 *b)
{
  u32 x = ((u32)b[3] << 24) | ((u32)b[0] << 16) | ((u32)b[0] << 8) | (u32)b[0];
  x = ((x >>  1) & 0x55555555u) | ((x & 0x55555555u) <<  1);
  x = ((x >>  2) & 0x33333333u) | ((x & 0x33333333u) <<  2);
  x = ((x >>  4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) <<  4);
  x = ((x >>  8) & 0x00ff00ffu) | ((x & 0x00ff00ffu) <<  8);
  x = ((x >> 16) & 0xffffu    ) | ((x & 0xffffu    ) << 16);
  return x;
}
#endif

#define checkMax(v,max) ((v)>(max)?0:(v))

typedef u16   aiType[MAX_AREAS];
typedef char *anType[MAX_AREAS];

void autoUpdate(void)
{
  u16          count,
  count2;
  tempStrType  tempStr;
  char         *helpPtr;
  FILE         *textFile;
  time_t       timer;
  u16          lastZone,
  lastNet,
  lastNode;
  FOLDER       folderFdRec;
  IMFOLDER     folderImRec;
  QBBSBoardRecType QBBSBoardRec;
  fhandle          folderHandle;
  char           *areaInfoNamePtr;
  u16            areaInfoCount;
  u16            low, mid, high;
  headerType     *areaHeader;
  rawEchoType    *areaBuf;
  aiType         *areaInfoIndex;
  anType         *areaNames;

  u16              maxBoardNumRA;
#ifndef GOLDBASE
  fhandle          indexHandle = -1;
  messageRaType    messageRaRec;
  messageRa2Type   messageRa2Rec;
  SBBSBoardRecType SBBSBoardRec;
  char             SBBSakaUsed[MBBOARDS];
  MboardType       TAGBoardRec;
  s32              group;
  const u16        nul = 0;
  u16              temp;
#endif

  if ((areaInfoIndex = malloc(sizeof(aiType))) == NULL)
    return;

  if (!openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf))
  {
    displayMessage("Could not open Area File for AutoExport");
    free(areaInfoIndex);
    return;
  }
  areaInfoCount = areaHeader->totalRecords;

  if (*config.autoAreasBBSPath != 0)
  {
    time(&timer);
    strcpy(stpcpy(tempStr, fixPath(config.autoAreasBBSPath)), "areas.bbs");

    if ((textFile = fopen(tempStr, "wb")) == NULL)  // already fixPath'd
      displayMessage("Can't open Areas.BBS for output");
    else
    {
      if (config.akaList[0].nodeNum.zone != 0)
        fprintf(textFile, "%s ! ", nodeStr(&config.akaList[0].nodeNum));

      fprintf(textFile, "%s\r\n", config.sysopName);
      fprintf(textFile, "; Created by %s - %s\r\n", VersionStr(), isoFmtTime(timer));

      for (count = 0; count < areaInfoCount; count++)
      {
        getRec(CFG_ECHOAREAS, count);
        if (  areaBuf->options.active
           && (areaBuf->board || *areaBuf->msgBasePath || config.genOptions.PTAreasBBS)
           )
        {
          if (*areaBuf->msgBasePath)
            fprintf(textFile, "!%-29s %-19s ", areaBuf->msgBasePath, areaBuf->areaName);
          else if (areaBuf->board)
            fprintf(textFile, "%03u %-19s ", areaBuf->board, areaBuf->areaName);
          else
            fprintf(textFile, "P   %-19s ", areaBuf->areaName);

          if (!areaBuf->options.local)
          {
            lastZone = 0;
            lastNet = 0;
            lastNode = 0;
            count2 = 0;
            while ((count2 < MAX_FORWARD) && (areaBuf->forwards[count2].nodeNum.zone != 0))
            {
              strcpy(tempStr, nodeStr(&areaBuf->forwards[count2].nodeNum));
              helpPtr = tempStr;

              if (lastZone == areaBuf->forwards[count2].nodeNum.zone)
              {
                if (lastNet != areaBuf->forwards[count2].nodeNum.net)
                  helpPtr = strchr(tempStr, ':') + 1;
                else
                {
                  if (lastNode != areaBuf->forwards[count2].nodeNum.node)
                    helpPtr = strchr(tempStr, '/') + 1;
                  else
                  {
                    if (areaBuf->forwards[count2].nodeNum.point == 0)
                      helpPtr = strchr(tempStr, 0);
                    else
                      helpPtr = strchr(tempStr, '.');
                  }
                }
              }
              lastZone = areaBuf->forwards[count2].nodeNum.zone;
              lastNet  = areaBuf->forwards[count2].nodeNum.net;
              lastNode = areaBuf->forwards[count2].nodeNum.node;

              fprintf (textFile, " %s", helpPtr);

              count2++;
            }
          }
          fprintf(textFile, "\r\n");
        }
      }
      fclose(textFile);
    }
  }
  if (*config.autoGoldEdAreasPath != 0)
  {
    strcpy(stpcpy(tempStr, fixPath(config.autoGoldEdAreasPath)), "areas.gld");

    if ((textFile = fopen(tempStr, "wb")) == NULL)  // already fixPath'd
      displayMessage("Can't open Areas.GLD for output");
    else
    {
      if (*config.netPath)
        fprintf(textFile, "AREADEF NETMAIL              \"Netmail area\"          0 Net FIDO %s .    (Loc)\r\n", fixPath(config.netPath));

      if (*config.sentPath && !strcmp(config.sentPath, config.rcvdPath))
        fprintf(textFile, "AREADEF SENT_RCVD            \"Sent/Received netmail\" 0 Net FIDO %s . (R/O Loc)\r\n", fixPath(config.sentPath));
      else
      {
        if (*config.sentPath)
          fprintf(textFile, "AREADEF SENT_NETMAIL         \"Sent netmail\"          0 Net FIDO %s . (R/O Loc)\r\n", fixPath(config.sentPath));

        if (*config.rcvdPath)
          fprintf(textFile, "AREADEF RCVD_NETMAIL         \"Received netmail\"      0 Net FIDO %s . (R/O Loc)\r\n", fixPath(config.rcvdPath));
      }

      if (*config.pmailPath)
        fprintf(textFile, "AREADEF PERSONAL_MAIL        \"Personal mail\"         0 Local FIDO %s . (R/O Loc)\r\n", fixPath(config.pmailPath));

      if (*config.sentEchoPath)
        fprintf(textFile, "AREADEF SENT_ECHOMAIL        \"Sent echomail\"         0 Local FIDO %s . (R/O Loc)\r\n", fixPath(config.sentEchoPath));

      if (config.dupBoard)
        fprintf(textFile, "AREADEF DUP_MSGS             \"Duplicate messages\"    0 Local Hudson %u . (R/O Loc)\r\n", config.dupBoard);

      if (config.badBoard)
        fprintf(textFile, "AREADEF BAD_MSGS             \"Bad messages\"          0 Local Hudson %u . (R/O Loc)\r\n", config.badBoard);

      if (config.recBoard)
        fprintf(textFile, "AREADEF RECOVERY_BOARD       \"Recovery board\"        0 Local Hudson %u . (R/O Loc)\r\n", config.recBoard);

      for (count = 0; count < MAX_NETAKAS; count++)
      {
        if (config.netmailBoard[count])
        {
          count2 = 0;
          while (count2 < count && config.netmailBoard[count] != config.netmailBoard[count2])
            count2++;

          if (count == count2)
          {
            if (count == 0)
            {
              const char *t = *config.descrAKA[0] ? config.descrAKA[0] : "Netmail Main";
              fprintf( textFile, "AREADEF NETMAIL_MAIN         \"%s\"%*c Net Hudson %u %s (Loc)\r\n"
                     , t, (int)(44 - strlen(t)), '0', config.netmailBoard[count], nodeStrP(&config.akaList[count].nodeNum));
            }
            else
            {
              const char *t = *config.descrAKA[count] ? config.descrAKA[count] : "Netmail AKA (use comment)";
              fprintf( textFile, "AREADEF NETMAIL_AKA_%-8u \"""%s\"%*c Net Hudson %u %s (Loc)\r\n"
                     , count, t, (int)(44 - strlen(t)), '0', config.netmailBoard[count], nodeStrP(&config.akaList[count].nodeNum));
            }
          }
        }
      }

      for (count = 0; count < areaInfoCount; count++)
      {
        getRec(CFG_ECHOAREAS, count);
        if (areaBuf->options.active && areaBuf->board &&
            !*areaBuf->msgBasePath)
        {
          fprintf(textFile, "AREADEF %-20s \"%s\"%*c %s Hudson %u %s (%sLoc%s)\r\n",
                  areaBuf->areaName,
                  areaBuf->comment,
                  (int)(44 - strlen(areaBuf->comment)),
                  groupToChar(areaBuf->group),
                  areaBuf->options.local?"Local":"Echo",
                  areaBuf->board,
                  nodeStrP(&config.akaList[areaBuf->address].nodeNum),
                  (config.bbsProgram != BBS_PROB && areaBuf->msgKindsRA == 3) ? "R/O ":"",
                  ""
                 );
        }
        if (areaBuf->options.active && *areaBuf->msgBasePath)
        {
          fprintf(textFile, "AREADEF %-20s \"%s\"%*c %s JAM %s %s (%sLoc%s)\r\n",
                  areaBuf->areaName,
                  areaBuf->comment,
                  (int)(44 - strlen(areaBuf->comment)),
                  groupToChar(areaBuf->group),
                  areaBuf->options.local ? "Local" : "Echo",
                  fixPath(areaBuf->msgBasePath),
                  nodeStrP(&config.akaList[areaBuf->address].nodeNum),
                  (config.bbsProgram != BBS_PROB && areaBuf->msgKindsRA == 3) ? "R/O ":"",
                  ""
                 );
        }
      }
      fclose(textFile);
    }
  }

  if ( (*config.autoFolderFdPath ||
        (*config.autoRAPath && config.bbsProgram == BBS_TAG)) &&
       (areaNames = malloc(sizeof(anType))) != NULL )
  {
    /* Sort areas for FrontDoor and TAG */

    for (count = 0; count < areaInfoCount; count++)
      (*areaInfoIndex)[count] = count;
    if ( config.genOptions.commentFFD )
    {
      for (count = 0; count < areaInfoCount; count++)
      {
        if ( !getRec(CFG_ECHOAREAS, count) ||
             ((*areaNames)[count] = malloc(strlen(*areaBuf->comment ? areaBuf->comment : areaBuf->areaName) + 1)) == NULL )
          goto error;
        strcpy((*areaNames)[count], *areaBuf->comment ? areaBuf->comment : areaBuf->areaName);
      }
      for (count = 1; count < areaInfoCount; count++)
      {
        areaInfoNamePtr = (*areaNames)[count];
        low = 0;
        high = count;
        while (low < high)
        {
          mid = (low + high) >> 1;
          if (stricmp(areaInfoNamePtr, (*areaNames)[mid]) > 0)
            low = mid + 1;
          else
            high = mid;
        }
        memmove (&(*areaNames)[low+1], &(*areaNames)[low], (count-low) * 4); /* 4 = sizeof(ptr) */
        (*areaNames)[low] = areaInfoNamePtr;
        memmove (&(*areaInfoIndex)[low+1], &(*areaInfoIndex)[low], (count-low) * 2); /* 2 = sizeof(u16) */
        (*areaInfoIndex)[low] = count;
      }
      for (count = 1; count < areaInfoCount; count++)
        free((*areaNames)[count]);
      free(areaNames);
    }
  }
  if (*config.autoFolderFdPath != 0)
  {
    strcpy(stpcpy(tempStr, fixPath(config.autoFolderFdPath)), config.mailer == dMT_InterMail ? "imfolder.cfg" : "folder.fd");

    if ((folderHandle = open(tempStr, O_WRONLY | O_CREAT| O_TRUNC | O_BINARY, dDEFOMODE)) == -1)  // already fixPath'd
    {
      if (config.mailer != dMT_InterMail)
        displayMessage("Can't open FOLDER.FD for output");
      else
        displayMessage("Can't open IMFOLDER.CFG for output");
    }
    else
    {
      if (config.mailer != dMT_InterMail)
      {
        if (*config.sentPath && !strcmp(config.sentPath, config.rcvdPath))
        {
          memset(&folderFdRec, 0, sizeof(FOLDER));
          strcpy(folderFdRec.title, "Sent/Received netmail");
          strcpy(folderFdRec.path , fixPath(config.sentPath));
          folderFdRec.behave = FD_LOCAL | FD_NETMAIL | FD_READONLY | FD_EXPORT_OK;
          folderFdRec.userok = 0x03ff;
          folderFdRec.pwdcrc = -1L;
          write(folderHandle, &folderFdRec, sizeof(FOLDER));
        }
        else
        {
          if (*config.sentPath)
          {
            memset(&folderFdRec, 0, sizeof(FOLDER));
            strcpy(folderFdRec.title, "Sent netmail");
            strcpy(folderFdRec.path , fixPath(config.sentPath));
            folderFdRec.behave = FD_LOCAL | FD_NETMAIL | FD_READONLY | FD_EXPORT_OK;
            folderFdRec.userok = 0x03ff;
            folderFdRec.pwdcrc = -1L;
            write(folderHandle, &folderFdRec, sizeof(FOLDER));
          }

          if (*config.rcvdPath)
          {
            memset(&folderFdRec, 0, sizeof(FOLDER));
            strcpy(folderFdRec.title, "Received netmail");
            strcpy(folderFdRec.path , fixPath(config.rcvdPath));
            folderFdRec.behave = FD_LOCAL | FD_NETMAIL | FD_READONLY | FD_EXPORT_OK;
            folderFdRec.userok = 0x03ff;
            folderFdRec.pwdcrc = -1L;
            write(folderHandle, &folderFdRec, sizeof(FOLDER));
          }
        }

        if (*config.pmailPath)
        {
          memset(&folderFdRec, 0, sizeof(FOLDER));
          strcpy(folderFdRec.title, "Personal mail");
          strcpy(folderFdRec.path , fixPath(config.pmailPath));
          folderFdRec.behave = FD_LOCAL | FD_READONLY | FD_EXPORT_OK;
          folderFdRec.userok = 0x03ff;
          folderFdRec.pwdcrc = -1L;
          write(folderHandle, &folderFdRec, sizeof(FOLDER));
        }

        if (*config.sentEchoPath)
        {
          memset(&folderFdRec, 0, sizeof(FOLDER));
          strcpy(folderFdRec.title, "Sent echomail");
          strcpy(folderFdRec.path , fixPath(config.sentEchoPath));
          folderFdRec.behave = FD_LOCAL | FD_READONLY | FD_EXPORT_OK;
          folderFdRec.userok = 0x03ff;
          folderFdRec.pwdcrc = -1L;
          write(folderHandle, &folderFdRec, sizeof(FOLDER));
        }

        if (config.dupBoard)
        {
          memset(&folderFdRec, 0, sizeof(FOLDER));
          strcpy(folderFdRec.title, "Duplicate messages");
          folderFdRec.board = config.dupBoard;
          folderFdRec.behave = FD_QUICKBBS | FD_LOCAL | FD_READONLY | FD_EXPORT_OK;
          folderFdRec.userok = 0x03ff;
          folderFdRec.pwdcrc = -1L;
          write(folderHandle, &folderFdRec, sizeof(FOLDER));
        }

        if (config.badBoard)
        {
          memset(&folderFdRec, 0, sizeof(FOLDER));
          strcpy(folderFdRec.title, "Bad messages");
          folderFdRec.board = config.badBoard;
          folderFdRec.behave = FD_QUICKBBS | FD_LOCAL | FD_READONLY | FD_EXPORT_OK;
          folderFdRec.userok = 0x03ff;
          folderFdRec.pwdcrc = -1L;
          write(folderHandle, &folderFdRec, sizeof(FOLDER));
        }

        if (config.recBoard)
        {
          memset(&folderFdRec, 0, sizeof(FOLDER));
          strcpy(folderFdRec.title, "Recovery board");
          folderFdRec.board = config.recBoard;
          folderFdRec.behave = FD_QUICKBBS | FD_LOCAL | FD_READONLY | FD_EXPORT_OK;
          folderFdRec.userok = 0x03ff;
          folderFdRec.pwdcrc = -1L;
          write(folderHandle, &folderFdRec, sizeof(FOLDER));
        }

        for (count = 0; count < MAX_NETAKAS; count++)
        {
          if (config.netmailBoard[count])
          {
            count2 = 0;
            while (count2 < count && config.netmailBoard[count] != config.netmailBoard[count2])
              count2++;

            if (count == count2)
            {
              memset(&folderFdRec, 0, sizeof(FOLDER));

              if (*config.descrAKA[count])
                strncpy(folderFdRec.title, config.descrAKA[count], 40);
              else
              {
                if (count == 0)
                  sprintf(folderFdRec.title, "NETMAIL MAIN");
                else
                  sprintf(folderFdRec.title, "NETMAIL AKA %u", count);
              }

              folderFdRec.useaka = count;
              folderFdRec.board  = config.netmailBoard[count];
              folderFdRec.behave = FD_PRIVATE | FD_NETMAIL | FD_QUICKBBS | FD_EXPORT_OK
                                 | (config.attrRA[count] & BIT0 ? FD_ECHO_INFO : 0);
              folderFdRec.userok = 0x03ff;
              folderFdRec.origin = min(count, 19);
              folderFdRec.pwdcrc = -1L;

              write(folderHandle, &folderFdRec, sizeof(FOLDER));
            }
          }
        }

        for (count = 0; count < areaInfoCount; count++)
        {
          getRec(CFG_ECHOAREAS, (*areaInfoIndex)[count]);
          if (areaBuf->options.active && (areaBuf->board || *areaBuf->msgBasePath))
          {
            memset(&folderFdRec, 0, sizeof(FOLDER));

            if (config.genOptions.commentFFD && *areaBuf->comment)
              strncpy(folderFdRec.title, areaBuf->comment, 40);
            else
              strncpy(folderFdRec.title, areaBuf->areaName, 40);

            if (*areaBuf->msgBasePath)
              strcpy(folderFdRec.path, fixPath(areaBuf->msgBasePath));
            else
              folderFdRec.board  = areaBuf->board;
            folderFdRec.behave = (*areaBuf->msgBasePath ? FD_JAMAREA : FD_QUICKBBS) |
                                 ((areaBuf->msgKindsRA==1) ? FD_PRIVATE:0) |
                                 (areaBuf->options.local ? FD_LOCAL:FD_ECHOMAIL|FD_EXPORT_OK) |
                                 (areaBuf->attrRA & BIT0 ? FD_ECHO_INFO:0);
            folderFdRec.userok = 0x03ff;
            folderFdRec.origin = checkMax(areaBuf->address, 19);
            folderFdRec.pwdcrc = -1L;
            folderFdRec.useaka = checkMax(areaBuf->address, 10);
            write(folderHandle, &folderFdRec, sizeof(FOLDER));
          }
        }
      }
      else
      {
        if (*config.sentPath && !strcmp(config.sentPath, config.rcvdPath))
        {
          memset(&folderImRec, 0, sizeof(IMFOLDER));
          strcpy(folderImRec.title, "Sent/Received netmail");
          strcpy(folderImRec.areatag, "Sent/Received netmail");
          strcpy(folderImRec.path, fixPath(config.sentPath));
          folderImRec.ftype  = F_MSG;
          folderImRec.behave = IM_LOCAL | IM_F_NETMAIL | IM_READONLY | IM_EXPORT_OK;
          folderImRec.userok = 0x03ff;
          folderImRec.pwdcrc = -1L;
          write(folderHandle, &folderImRec, sizeof(IMFOLDER));
        }
        else
        {
          if (*config.sentPath)
          {
            memset(&folderImRec, 0, sizeof(IMFOLDER));
            strcpy(folderImRec.title, "Sent netmail");
            strcpy(folderImRec.areatag, "Sent netmail");
            strcpy(folderImRec.path, fixPath(config.sentPath));
            folderImRec.ftype  = F_MSG;
            folderImRec.behave = IM_LOCAL | IM_F_NETMAIL | IM_READONLY | IM_EXPORT_OK;
            folderImRec.userok = 0x03ff;
            folderImRec.pwdcrc = -1L;
            write(folderHandle, &folderImRec, sizeof(IMFOLDER));
          }

          if (*config.rcvdPath)
          {
            memset(&folderImRec, 0, sizeof(IMFOLDER));
            strcpy(folderImRec.title, "Received netmail");
            strcpy(folderImRec.areatag, "Received netmail");
            strcpy(folderImRec.path, fixPath(config.rcvdPath));
            folderImRec.ftype  = F_MSG;
            folderImRec.behave = IM_LOCAL | IM_F_NETMAIL | IM_READONLY | IM_EXPORT_OK;
            folderImRec.userok = 0x03ff;
            folderImRec.pwdcrc = -1L;
            write(folderHandle, &folderImRec, sizeof(IMFOLDER));
          }
        }

        if (*config.pmailPath)
        {
          memset(&folderImRec, 0, sizeof(IMFOLDER));
          strcpy(folderImRec.title, "Personal mail");
          strcpy(folderImRec.areatag, "Personal mail");
          strcpy(folderImRec.path, fixPath(config.pmailPath));
          folderImRec.ftype  = F_MSG;
          folderImRec.behave = IM_LOCAL | IM_READONLY | IM_EXPORT_OK;
          folderImRec.userok = 0x03ff;
          folderImRec.pwdcrc = -1L;
          write(folderHandle, &folderImRec, sizeof(IMFOLDER));
        }

        if (*config.sentEchoPath)
        {
          memset(&folderImRec, 0, sizeof(IMFOLDER));
          strcpy(folderImRec.title, "Sent echomail");
          strcpy(folderImRec.areatag, "Sent echomail");
          strcpy(folderImRec.path, fixPath(config.sentEchoPath));
          folderImRec.ftype  = F_MSG;
          folderImRec.behave = IM_LOCAL | IM_READONLY | IM_EXPORT_OK;
          folderImRec.userok = 0x03ff;
          folderImRec.pwdcrc = -1L;
          write(folderHandle, &folderImRec, sizeof(IMFOLDER));
        }

        if (config.dupBoard)
        {
          memset(&folderImRec, 0, sizeof(IMFOLDER));
          strcpy(folderImRec.title, "Duplicate messages");
          strcpy(folderImRec.areatag, "Duplicate messages");
          folderImRec.board = config.dupBoard;
          folderImRec.ftype  = F_HUDSON;
          folderImRec.behave = IM_BOARDTYPE | IM_LOCAL | IM_BOARDTYPE | IM_READONLY | IM_EXPORT_OK;
          folderImRec.userok = 0x03ff;
          folderImRec.pwdcrc = -1L;
          write(folderHandle, &folderImRec, sizeof(IMFOLDER));
        }

        if (config.badBoard)
        {
          memset(&folderImRec, 0, sizeof(IMFOLDER));
          strcpy(folderImRec.title, "Bad messages");
          strcpy(folderImRec.areatag, "Bad messages");
          folderImRec.board = config.badBoard;
          folderImRec.ftype  = F_HUDSON;
          folderImRec.behave = IM_BOARDTYPE | IM_LOCAL | IM_BOARDTYPE | IM_READONLY | IM_EXPORT_OK;
          folderImRec.userok = 0x03ff;
          folderImRec.pwdcrc = -1L;
          write(folderHandle, &folderImRec, sizeof(IMFOLDER));
        }

        if (config.recBoard)
        {
          memset(&folderImRec, 0, sizeof(IMFOLDER));
          strcpy(folderImRec.title, "Recovery board");
          strcpy(folderImRec.areatag, "Recovery board");
          folderImRec.board = config.recBoard;
          folderImRec.ftype  = F_HUDSON;
          folderImRec.behave = IM_BOARDTYPE | IM_LOCAL | IM_BOARDTYPE | IM_READONLY | IM_EXPORT_OK;
          folderImRec.userok = 0x03ff;
          folderImRec.pwdcrc = -1L;
          write(folderHandle, &folderImRec, sizeof(IMFOLDER));
        }

        for (count = 0; count < MAX_NETAKAS; count++)
        {
          if (config.netmailBoard[count])
          {
            count2 = 0;
            while (count2 < count && config.netmailBoard[count] != config.netmailBoard[count2])
              count2++;

            if (count == count2)
            {
              memset(&folderImRec, 0, sizeof(IMFOLDER));

              if (*config.descrAKA[count])
                strncpy(folderImRec.title, config.descrAKA[count], 40);
              else
              {
                if (count == 0)
                  sprintf(folderImRec.title, "NETMAIL MAIN");
                else
                  sprintf(folderImRec.title, "NETMAIL AKA %u", count);
              }
              if (count == 0)
                sprintf(folderImRec.areatag, "NETMAIL MAIN");
              else
                sprintf(folderImRec.areatag, "NETMAIL AKA %u", count);

              folderImRec.useaka = count;
              folderImRec.board  = config.netmailBoard[count];
              folderImRec.ftype  = F_HUDSON;
              folderImRec.behave = IM_BOARDTYPE | IM_PRIVATE | IM_BOARDTYPE | IM_F_NETMAIL | IM_EXPORT_OK |
                                   (config.attrRA[count] & BIT0 ? IM_ECHO_INFO:0);
              folderImRec.userok = 0x03ff;
              folderImRec.origin = min(count, 19);
              folderImRec.pwdcrc = -1L;

              write(folderHandle, &folderImRec, sizeof(IMFOLDER));
            }
          }
        }

        for (count = 0; count < areaInfoCount; count++)
        {
          getRec(CFG_ECHOAREAS, (*areaInfoIndex)[count]);
          if (areaBuf->options.active &&
              (areaBuf->board || *areaBuf->msgBasePath))
          {
            memset(&folderImRec, 0, sizeof(IMFOLDER));

            if (config.genOptions.commentFFD && *areaBuf->comment)
              strncpy(folderImRec.title, areaBuf->comment, 40);
            else
              strncpy (folderImRec.title, areaBuf->areaName, 40);
            strncpy(folderImRec.areatag, areaBuf->areaName, 38);

            folderImRec.ftype  = !*areaBuf->msgBasePath ?
                                 F_HUDSON : F_JAM;
            if (*areaBuf->msgBasePath)
              strcpy(folderImRec.path, fixPath(areaBuf->msgBasePath));
            else
              folderImRec.board  = areaBuf->board;
            folderImRec.behave = (*areaBuf->msgBasePath?0:IM_BOARDTYPE) |
                                 ((areaBuf->msgKindsRA==1) ? IM_PRIVATE:0) |
                                 (areaBuf->options.local ? IM_LOCAL:IM_ECHOMAIL|IM_EXPORT_OK) |
                                 (areaBuf->attrRA & BIT0 ? IM_ECHO_INFO:0);
            folderImRec.userok = 0x03ff;
            folderImRec.origin = checkMax(areaBuf->address, 19);
            folderImRec.pwdcrc = -1L;
            folderImRec.useaka = checkMax(areaBuf->address, 20);
            write(folderHandle, &folderImRec, sizeof(IMFOLDER));
          }
        }
      }
      close(folderHandle);
    }
  }

  if (*config.autoRAPath != 0)
  {
#ifndef GOLDBASE
    if (config.bbsProgram == BBS_TAG) /* TAG */
    {
      strcpy(stpcpy(tempStr, fixPath(config.autoRAPath)), "mboards.dat");

      if ((folderHandle = open(tempStr, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, dDEFOMODE)) == -1)  // already fixPath'd
        displayMessage("Can't open MBOARDS.DAT for output");
      else
      {
        if (*config.pmailPath)
        {
          memset(&TAGBoardRec, 0, sizeof(MboardType));
          strcpy(TAGBoardRec.Name, "Personal mail");
          TAGBoardRec.NameLen = strlen(TAGBoardRec.Name);
          strcpy(TAGBoardRec.Path, fixPath(config.pmailPath));
          TAGBoardRec.PathLen = strlen(TAGBoardRec.Path);
          TAGBoardRec.Mstyle  = LOCALSTYLE;
          TAGBoardRec.Mtype   = FIDOFORMAT;
          TAGBoardRec.AccessAR = '@';
          write(folderHandle, &TAGBoardRec, sizeof(MboardType));
        }
        if (*config.sentPath && !strcmp(config.sentPath, config.rcvdPath))
        {
          memset(&TAGBoardRec, 0, sizeof(MboardType));
          strcpy(TAGBoardRec.Name, "Sent/Received netmail");
          TAGBoardRec.NameLen = strlen(TAGBoardRec.Name);
          strcpy(TAGBoardRec.Path, fixPath(config.sentPath));
          TAGBoardRec.PathLen = strlen(TAGBoardRec.Path);
          TAGBoardRec.Mstyle  = LOCALSTYLE;
          TAGBoardRec.Mtype   = FIDOFORMAT;
          TAGBoardRec.AccessAR = '@';
          write(folderHandle, &TAGBoardRec, sizeof(MboardType));
        }
        else
        {
          if (*config.sentPath)
          {
            memset(&TAGBoardRec, 0, sizeof(MboardType));
            strcpy(TAGBoardRec.Name, "Sent netmail");
            TAGBoardRec.NameLen = strlen(TAGBoardRec.Name);
            strcpy(TAGBoardRec.Path, fixPath(config.sentPath));
            TAGBoardRec.PathLen = strlen(TAGBoardRec.Path);
            TAGBoardRec.Mstyle  = LOCALSTYLE;
            TAGBoardRec.Mtype   = FIDOFORMAT;
            TAGBoardRec.AccessAR = '@';
            TAGBoardRec.AccessSL = 0;   // was 32000, maar dit is een byte...?
            TAGBoardRec.PostSL   = 0;   // was 32000, maar dit is een byte...?
            /*                TAGBoardRec.MsgCount = 0; */
            TAGBoardRec.MaxMsgs  = 0;
            TAGBoardRec.uuMaxOld = 0;
            /* password */
            /* anonymous */
            /* allowansi */
            /* allowhandles */
            write(folderHandle, &TAGBoardRec, sizeof(MboardType));
          }

          if (*config.rcvdPath)
          {
            memset(&TAGBoardRec, 0, sizeof(MboardType));
            strcpy(TAGBoardRec.Name, "Received netmail");
            TAGBoardRec.NameLen = strlen(TAGBoardRec.Name);
            strcpy(TAGBoardRec.Path, fixPath(config.rcvdPath));
            TAGBoardRec.PathLen = strlen(TAGBoardRec.Path);
            TAGBoardRec.Mstyle  = LOCALSTYLE;
            TAGBoardRec.Mtype   = FIDOFORMAT;
            TAGBoardRec.AccessAR = '@';
            write(folderHandle, &TAGBoardRec, sizeof(MboardType));
          }
        }

        if (*config.sentEchoPath)
        {
          memset(&TAGBoardRec, 0, sizeof(MboardType));
          strcpy(TAGBoardRec.Name, "Sent echomail");
          TAGBoardRec.NameLen = strlen(TAGBoardRec.Name);
          strcpy(TAGBoardRec.Path, fixPath(config.sentEchoPath));
          TAGBoardRec.PathLen = strlen(TAGBoardRec.Path);
          TAGBoardRec.Mstyle  = LOCALSTYLE;
          TAGBoardRec.Mtype   = FIDOFORMAT;
          TAGBoardRec.AccessAR = '@';
          write(folderHandle, &TAGBoardRec, sizeof(MboardType));
        }

        if (config.dupBoard)
        {
          memset(&TAGBoardRec, 0, sizeof(MboardType));
          strcpy(TAGBoardRec.Name, "Duplicate messages");
          TAGBoardRec.NameLen = strlen(TAGBoardRec.Name);
          strcpy(TAGBoardRec.Path, fixPath(config.sentPath));
          TAGBoardRec.PathLen = strlen(TAGBoardRec.Path);
          TAGBoardRec.Mstyle  = LOCALSTYLE;
          TAGBoardRec.Mtype   = RAFORMAT;
          TAGBoardRec.AccessAR = '@';
          TAGBoardRec.RaBoard = config.dupBoard;
          write(folderHandle, &TAGBoardRec, sizeof(MboardType));
        }

        if (config.badBoard)
        {
          memset(&TAGBoardRec, 0, sizeof(MboardType));
          strcpy(TAGBoardRec.Name, "Bad messages");
          TAGBoardRec.NameLen = strlen(TAGBoardRec.Name);
          strcpy(TAGBoardRec.Path, fixPath(config.sentPath));
          TAGBoardRec.PathLen = strlen(TAGBoardRec.Path);
          TAGBoardRec.Mstyle  = LOCALSTYLE;
          TAGBoardRec.Mtype   = RAFORMAT;
          TAGBoardRec.AccessAR = '@';
          TAGBoardRec.RaBoard = config.badBoard;
          write(folderHandle, &TAGBoardRec, sizeof(MboardType));
        }

        if (config.recBoard)
        {
          memset(&TAGBoardRec, 0, sizeof(MboardType));
          strcpy(TAGBoardRec.Name, "Recovered messages");
          TAGBoardRec.NameLen = strlen(TAGBoardRec.Name);
          strcpy(TAGBoardRec.Path, fixPath(config.sentPath));
          TAGBoardRec.PathLen = strlen(TAGBoardRec.Path);
          TAGBoardRec.Mstyle  = LOCALSTYLE;
          TAGBoardRec.Mtype   = RAFORMAT;
          TAGBoardRec.AccessAR = '@';
          TAGBoardRec.RaBoard = config.recBoard;
          write(folderHandle, &TAGBoardRec, sizeof(MboardType));
        }

        for (count = 0; count < MAX_NETAKAS; count++)
        {
          if (config.netmailBoard[count])
          {
            count2 = 0;
            while ((count2 < count) &&
                   (config.netmailBoard[count] != config.netmailBoard[count2]))
            {
              count2++;
            }
            if (count == count2)
            {
              memset (&TAGBoardRec, 0, sizeof(MboardType));

              if (*config.descrAKA[count])
              {
                strcpy(TAGBoardRec.Name, config.descrAKA[count]);
              }
              else
              {
                if (count == 0)
                  strcpy(TAGBoardRec.Name, "NETMAIL MAIN");
                else
                  sprintf(TAGBoardRec.Name, "NETMAIL AKA %u", count);
              }
              TAGBoardRec.NameLen = strlen(TAGBoardRec.Name);
              strcpy(TAGBoardRec.Path, fixPath(config.bbsPath));
              TAGBoardRec.PathLen  = strlen(TAGBoardRec.Path);
              TAGBoardRec.Mstyle   = NETMAILSTYLE;
              TAGBoardRec.Mtype    = RAFORMAT;
              TAGBoardRec.AccessAR = '@';
              TAGBoardRec.RaBoard  = config.netmailBoard[count];

              write (folderHandle, &TAGBoardRec, sizeof(MboardType));
            }
          }
        }

        for (count = 0; count < areaInfoCount; count++)
        {
          getRec(CFG_ECHOAREAS, (*areaInfoIndex)[count]);
          if (areaBuf->options.active && areaBuf->board &&
              !*areaBuf->msgBasePath &&
              (areaBuf->options.export2BBS) )
          {
            memset(&TAGBoardRec, 0, sizeof(MboardType));

            if (config.genOptions.commentFFD && *areaBuf->comment)
              strcpy(TAGBoardRec.Name, areaBuf->comment);
            else
              strcpy(TAGBoardRec.Name, areaBuf->areaName);
            TAGBoardRec.NameLen  = strlen(TAGBoardRec.Name);
            strncpy(TAGBoardRec.EchoTag, areaBuf->areaName, 31);
            TAGBoardRec.EchoTagLen = strlen(TAGBoardRec.EchoTag);
            strcpy(TAGBoardRec.Path, fixPath(config.bbsPath));
            TAGBoardRec.PathLen = strlen(TAGBoardRec.Path);
            strcpy(TAGBoardRec.OriginLine, areaBuf->originLine);
            TAGBoardRec.OriginLineLen = strlen (TAGBoardRec.OriginLine);
            strncpy(TAGBoardRec.QwkName, areaBuf->qwkName, 10);
            TAGBoardRec.QwkNameLen = min(strlen(areaBuf->qwkName),10);
            TAGBoardRec.Mstyle   = ECHOSTYLE;
            TAGBoardRec.Mtype    = RAFORMAT;
            TAGBoardRec.AccessAR = 'A';
            group = areaBuf->group;
            while ((group>>=1) != 0)
              TAGBoardRec.AccessAR++;
            TAGBoardRec.PostAR   = 'A';
            TAGBoardRec.RaBoard  = areaBuf->board;
            TAGBoardRec.AccessSL = areaBuf->readSecRA;
            TAGBoardRec.PostSL   = areaBuf->writeSecRA;
            TAGBoardRec.MaxMsgs  = areaBuf->msgs;
            TAGBoardRec.uuMaxOld = areaBuf->days;
            /* password */
            /* anonymous */
            /* allowansi */
            TAGBoardRec.RestrictPrivate = areaBuf->msgKindsRA;
            TAGBoardRec.AllowHandle = areaBuf->attrRA & BIT5 ? 1 :
                                      areaBuf->attrRA & BIT3 ? 2 : 0;
            if (areaBuf->address)
            {
              TAGBoardRec.UseOtherAddress = 1;
              TAGBoardRec.OtherAddress.Zone  = TAGBoardRec.OldOtherAddress.Zone  = config.akaList[areaBuf->address].nodeNum.zone;
              TAGBoardRec.OtherAddress.Net   = TAGBoardRec.OldOtherAddress.Net   = config.akaList[areaBuf->address].nodeNum.net;
              TAGBoardRec.OtherAddress.Node  = TAGBoardRec.OldOtherAddress.Node  = config.akaList[areaBuf->address].nodeNum.node;
              TAGBoardRec.OtherAddress.Point = TAGBoardRec.OldOtherAddress.Point = config.akaList[areaBuf->address].nodeNum.point;
            }
            TAGBoardRec.QwkConf = areaBuf->board;
            write (folderHandle, &TAGBoardRec, sizeof(MboardType));
          }
        }
        close (folderHandle);
      }
    }
#endif
    maxBoardNumRA = MBBOARDS;
    for (count = 0; count < areaInfoCount; count++)
    {
      getRec(CFG_ECHOAREAS, count);
      (*areaInfoIndex)[count] = ( (areaBuf->board == 0 &&
                                   *areaBuf->msgBasePath == 0) ||
                                  (areaBuf->board > 0 &&
                                   areaBuf->board <= MBBOARDS) ) ?
                                areaBuf->board : areaBuf->boardNumRA;
      maxBoardNumRA = max(maxBoardNumRA, (*areaInfoIndex)[count]);
    }
#ifndef GOLDBASE
    if ( config.bbsProgram == BBS_RA1X || config.bbsProgram == BBS_RA20 ||
         config.bbsProgram == BBS_RA25 || config.bbsProgram == BBS_ELEB )
    {
      strcpy(stpcpy(tempStr, fixPath(config.autoRAPath)), "messages.ra");

      if ((folderHandle = open(tempStr, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, dDEFOMODE)) == -1)  // already fixPath'd
        displayMessage("Can't open MESSAGES.RA for output");
      else
      {
        if (config.bbsProgram == BBS_RA1X)
        {
          for (count2 = 1; count2 <= MBBOARDS; count2++)
          {
            memset(&messageRaRec, 0, sizeof(messageRaType));

            count = 0;
            while (count < MAX_NETAKAS && config.netmailBoard[count] != count2)
              count++;

            if (count < MAX_NETAKAS)
            {
              if (*config.descrAKA[count])
              {
                strncpy(messageRaRec.name, config.descrAKA[count], 40);
              }
              else
              {
                if (count)
                  sprintf(messageRaRec.name, "Netmail AKA %2u", count);
                else
                  strcpy(messageRaRec.name, "Netmail Main");
              }
              messageRaRec.nameLength = strlen(messageRaRec.name);

              messageRaRec.typ = 1;
              messageRaRec.msgKinds      = config.msgKindsRA [count];
              messageRaRec.daysKill      = config.daysAKA    [count];
              messageRaRec.rcvdKill      = config.daysRcvdAKA[count];
              messageRaRec.countKill     = config.msgsAKA    [count];
              messageRaRec.attribute     = config.attrRA     [count] & ~BIT7;
              messageRaRec.readSecurity  = config.readSecRA  [count];
              messageRaRec.writeSecurity = config.writeSecRA [count];
              messageRaRec.sysopSecurity = config.sysopSecRA [count];
              memcpy(&messageRaRec.readFlags , &config.readFlagsRA [count], 4);
              memcpy(&messageRaRec.writeFlags, &config.writeFlagsRA[count], 4);
              memcpy(&messageRaRec.sysopFlags, &config.sysopFlagsRA[count], 4);

              messageRaRec.akaAddress = count;
            }
            else if (config.dupBoard == count2)
            {
              if (config.genOptions.incBDRRA)
              {
                strcpy(messageRaRec.name, "Duplicate messages");
                messageRaRec.nameLength = strlen(messageRaRec.name);
                messageRaRec.msgKinds = 3;
                messageRaRec.readSecurity  = 32000;
                messageRaRec.writeSecurity = 32000;
                messageRaRec.sysopSecurity = 32000;
              }
            }
            else if (config.badBoard == count2)
            {
              if (config.genOptions.incBDRRA)
              {
                strcpy(messageRaRec.name, "Bad messages");
                messageRaRec.nameLength = strlen(messageRaRec.name);
                messageRaRec.msgKinds = 3;
                messageRaRec.readSecurity  = 32000;
                messageRaRec.writeSecurity = 32000;
                messageRaRec.sysopSecurity = 32000;
              }
            }
            else if (config.recBoard == count2)
            {
              if (config.genOptions.incBDRRA)
              {
                strcpy(messageRaRec.name, "Recovery board");
                messageRaRec.nameLength = strlen(messageRaRec.name);
                messageRaRec.msgKinds = 3;
                messageRaRec.readSecurity  = 32000;
                messageRaRec.writeSecurity = 32000;
                messageRaRec.sysopSecurity = 32000;
              }
            }
            else
            {
              count = 0;
              while (count < areaInfoCount)
              {
                if ((*areaInfoIndex)[count] == count2)
                {
                  getRec(CFG_ECHOAREAS, count);
                  if (!*areaBuf->msgBasePath)
                    break;
                }
                count++;
              }
              if ( (count < areaInfoCount) &&
                   (areaBuf->options.active) &&
                   (areaBuf->options.export2BBS) )
              {
                if (config.genOptions.commentFRA && *areaBuf->comment)
                  strncpy(messageRaRec.name, areaBuf->comment, 40);
                else
                  strncpy(messageRaRec.name, areaBuf->areaName, 40);

                messageRaRec.nameLength = strlen(messageRaRec.name);

                messageRaRec.typ = areaBuf->options.local?0:2;
                messageRaRec.msgKinds = areaBuf->msgKindsRA;
                strcpy(messageRaRec.origin, areaBuf->originLine);
                messageRaRec.originLength = strlen(messageRaRec.origin);

                messageRaRec.akaAddress    = checkMax(areaBuf->address, 9);
                messageRaRec.daysKill      = areaBuf->days;
                messageRaRec.rcvdKill      = areaBuf->daysRcvd;
                messageRaRec.countKill     = areaBuf->msgs;
                messageRaRec.attribute     = areaBuf->attrRA & ~BIT7;
                messageRaRec.readSecurity  = areaBuf->readSecRA;
                messageRaRec.writeSecurity = areaBuf->writeSecRA;
                messageRaRec.sysopSecurity = areaBuf->sysopSecRA;
                memcpy(&messageRaRec.readFlags , &areaBuf->flagsRdRA , 4);
                memcpy(&messageRaRec.writeFlags, &areaBuf->flagsWrRA , 4);
                memcpy(&messageRaRec.sysopFlags, &areaBuf->flagsSysRA, 4);
              }
            }
            write(folderHandle, &messageRaRec, sizeof(messageRaType));
          }
        }
        else
        {  /* RA 2 */
          strcpy(stpcpy(tempStr, fixPath(config.autoRAPath)), "messages.rdx");

          if ((config.bbsProgram == BBS_RA25 || config.bbsProgram == BBS_ELEB) &&
              ((indexHandle = open(tempStr, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, dDEFOMODE)) == -1))  // already fixPath'd
            displayMessage("Can't open MESSAGES.RDX for output");
          else
          {
            for (count2 = 1; count2 <= maxBoardNumRA; count2++)
            {
              memset(&messageRa2Rec, 0, sizeof(messageRa2Type));

              count = 0;
              while (count < MAX_NETAKAS && config.netmailBoard[count] != count2)
              {
                count++;
              }
              if (count < MAX_NETAKAS)
              {
                if (*config.descrAKA[count])
                {
                  strncpy(messageRa2Rec.name, config.descrAKA[count], 40);
                }
                else
                {
                  if (count)
                    sprintf(messageRa2Rec.name, "Netmail AKA %2u", count);
                  else
                    strcpy(messageRa2Rec.name, "Netmail Main");
                }
                messageRa2Rec.nameLength = strlen(messageRa2Rec.name);

                messageRa2Rec.typ = 1;
                messageRa2Rec.msgKinds  = config.msgKindsRA[count];
                messageRa2Rec.daysKill  = config.daysAKA[count];
                messageRa2Rec.rcvdKill  = config.daysRcvdAKA[count];
                messageRa2Rec.countKill = config.msgsAKA[count];
                messageRa2Rec.attribute = config.attrRA[count] & ~BIT7;
                messageRa2Rec.age       = config.minAgeSBBS[count];
                messageRa2Rec.group     = config.groupRA[count];
                messageRa2Rec.altGroup[0] = config.altGroupRA[count][0];
                messageRa2Rec.altGroup[1] = config.altGroupRA[count][1];
                messageRa2Rec.altGroup[2] = config.altGroupRA[count][2];
                messageRa2Rec.attribute2    = config.attr2RA[count];
                messageRa2Rec.readSecurity  = config.readSecRA[count];
                messageRa2Rec.writeSecurity = config.writeSecRA[count];
                messageRa2Rec.sysopSecurity = config.sysopSecRA[count];
                memcpy(&messageRa2Rec.readFlags    , &config.readFlagsRA [count]    , 4);
                memcpy(&messageRa2Rec.readNotFlags , &config.readFlagsRA [count] + 4, 4);
                memcpy(&messageRa2Rec.writeFlags   , &config.writeFlagsRA[count]    , 4);
                memcpy(&messageRa2Rec.writeNotFlags, &config.writeFlagsRA[count] + 4, 4);
                memcpy(&messageRa2Rec.sysopFlags   , &config.sysopFlagsRA[count]    , 4);
                memcpy(&messageRa2Rec.sysopNotFlags, &config.sysopFlagsRA[count] + 4, 4);

                messageRa2Rec.akaAddress = count;
              }
              else if (config.dupBoard == count2)
              {
                if (config.genOptions.incBDRRA)
                {
                  strcpy(messageRa2Rec.name, "Duplicate messages");
                  messageRa2Rec.nameLength = strlen(messageRa2Rec.name);
                  messageRa2Rec.msgKinds = 3;
                  messageRa2Rec.readSecurity  = 32000;
                  messageRa2Rec.writeSecurity = 32000;
                  messageRa2Rec.sysopSecurity = 32000;
                }
              }
              else if (config.badBoard == count2)
              {
                if (config.genOptions.incBDRRA)
                {
                  strcpy(messageRa2Rec.name, "Bad messages");
                  messageRa2Rec.nameLength = strlen(messageRa2Rec.name);
                  messageRa2Rec.msgKinds = 3;
                  messageRa2Rec.readSecurity  = 32000;
                  messageRa2Rec.writeSecurity = 32000;
                  messageRa2Rec.sysopSecurity = 32000;
                }
              }
              else if (config.recBoard == count2)
              {
                if (config.genOptions.incBDRRA)
                {
                  strcpy(messageRa2Rec.name, "Recovery board");
                  messageRa2Rec.nameLength = strlen(messageRa2Rec.name);
                  messageRa2Rec.msgKinds = 3;
                  messageRa2Rec.readSecurity  = 32000;
                  messageRa2Rec.writeSecurity = 32000;
                  messageRa2Rec.sysopSecurity = 32000;
                }
              }
              else
              {
                count = 0;
                while ( count < areaInfoCount )
                {
                  if ( (*areaInfoIndex)[count] == count2 )
                  {
                    getRec(CFG_ECHOAREAS, count);
                    break;
                  }
                  count++;
                }
                if ( (count < areaInfoCount) &&
                     (areaBuf->options.active) &&
                     (areaBuf->options.export2BBS) )
                {
                  if (config.genOptions.commentFRA && *areaBuf->comment)
                  {
                    strncpy(messageRa2Rec.name, areaBuf->comment, 40);
                  }
                  else
                  {
                    strncpy(messageRa2Rec.name, areaBuf->areaName, 40);
                  }
                  messageRa2Rec.nameLength = strlen(messageRa2Rec.name);

                  messageRa2Rec.typ = areaBuf->boardTypeRA ?
                                      areaBuf->boardTypeRA+2 :
                                      (areaBuf->options.local?0:2);
                  messageRa2Rec.netReply = areaBuf->netReplyBoardRA;
                  messageRa2Rec.msgKinds = areaBuf->msgKindsRA;
                  strcpy(messageRa2Rec.origin, areaBuf->originLine);
                  messageRa2Rec.originLength = strlen(messageRa2Rec.origin);

                  if (config.bbsProgram == BBS_ELEB)
                    messageRa2Rec.akaAddress = areaBuf->address;
                  else
                    messageRa2Rec.akaAddress = checkMax(areaBuf->address, 9);
                  messageRa2Rec.daysKill      = areaBuf->days;
                  messageRa2Rec.rcvdKill      = areaBuf->daysRcvd;
                  messageRa2Rec.countKill     = areaBuf->msgs;
                  messageRa2Rec.attribute     = areaBuf->attrRA & ~BIT7;
                  if (*areaBuf->msgBasePath)
                  {
                    strcpy(messageRa2Rec.JAMbase, areaBuf->msgBasePath);
                    messageRa2Rec.JAMbaseLength = strlen(messageRa2Rec.JAMbase);
                    messageRa2Rec.attribute |= BIT7;
                  }
                  messageRa2Rec.age           = areaBuf->minAgeSBBS;
                  messageRa2Rec.group         = areaBuf->groupRA;
                  messageRa2Rec.altGroup[0]   = areaBuf->altGroupRA[0];
                  messageRa2Rec.altGroup[1]   = areaBuf->altGroupRA[1];
                  messageRa2Rec.altGroup[2]   = areaBuf->altGroupRA[2];
                  messageRa2Rec.attribute2    = areaBuf->attr2RA;
                  messageRa2Rec.readSecurity  = areaBuf->readSecRA;
                  messageRa2Rec.writeSecurity = areaBuf->writeSecRA;
                  messageRa2Rec.sysopSecurity = areaBuf->sysopSecRA;
                  memcpy(&messageRa2Rec.readFlags    , &areaBuf->flagsRdRA    , 4);
                  memcpy(&messageRa2Rec.readNotFlags , &areaBuf->flagsRdNotRA , 4);
                  memcpy(&messageRa2Rec.writeFlags   , &areaBuf->flagsWrRA    , 4);
                  memcpy(&messageRa2Rec.writeNotFlags, &areaBuf->flagsWrNotRA , 4);
                  memcpy(&messageRa2Rec.sysopFlags   , &areaBuf->flagsSysRA   , 4);
                  memcpy(&messageRa2Rec.sysopNotFlags, &areaBuf->flagsSysNotRA, 4);
                }
              }
              if (config.bbsProgram == BBS_RA25 || config.bbsProgram == BBS_ELEB)
                messageRa2Rec.areanum = count2;

              write(folderHandle, &messageRa2Rec, sizeof(messageRa2Type));
              if (config.bbsProgram == BBS_RA25 || config.bbsProgram == BBS_ELEB)
              {
                if (*messageRa2Rec.name)
                {
                  temp = (u16)(lseek(folderHandle, 0, SEEK_CUR) / sizeof(messageRa2Type));
                  write(indexHandle, &temp, 2);
                }
                else
                  write(indexHandle, &nul, 2);
              }
            }
            for (count = 0; count < areaInfoCount; count++)
            {
              getRec(CFG_ECHOAREAS, count);
              if (! (areaBuf->options.active &&
                     areaBuf->boardNumRA == 0 &&
                     *areaBuf->msgBasePath &&
                     areaBuf->options.export2BBS) )
                continue;
              memset(&messageRa2Rec, 0, sizeof(messageRa2Type));
              if (config.genOptions.commentFRA && *areaBuf->comment)
                strncpy(messageRa2Rec.name, areaBuf->comment, 40);
              else
                strncpy(messageRa2Rec.name, areaBuf->areaName, 40);

              messageRa2Rec.nameLength = strlen(messageRa2Rec.name);

              strcpy(messageRa2Rec.JAMbase, fixPath(areaBuf->msgBasePath));
              messageRa2Rec.JAMbaseLength = strlen(messageRa2Rec.JAMbase);

              messageRa2Rec.typ = areaBuf->boardTypeRA ?
                                  areaBuf->boardTypeRA+2 :
                                  (areaBuf->options.local?0:2);
              messageRa2Rec.netReply = areaBuf->netReplyBoardRA;
              messageRa2Rec.msgKinds = areaBuf->msgKindsRA;
              strcpy(messageRa2Rec.origin, areaBuf->originLine);
              messageRa2Rec.originLength = strlen(messageRa2Rec.origin);
              if (config.bbsProgram == BBS_ELEB)
                messageRa2Rec.akaAddress = areaBuf->address;
              else
                messageRa2Rec.akaAddress = checkMax(areaBuf->address, 9);

              messageRa2Rec.daysKill      = areaBuf->days;
              messageRa2Rec.rcvdKill      = areaBuf->daysRcvd;
              messageRa2Rec.countKill     = areaBuf->msgs;
              messageRa2Rec.attribute     = areaBuf->attrRA | BIT7;
              messageRa2Rec.age           = areaBuf->minAgeSBBS;
              messageRa2Rec.group         = areaBuf->groupRA;
              messageRa2Rec.altGroup[0]   = areaBuf->altGroupRA[0];
              messageRa2Rec.altGroup[1]   = areaBuf->altGroupRA[1];
              messageRa2Rec.altGroup[2]   = areaBuf->altGroupRA[2];
              messageRa2Rec.attribute2    = areaBuf->attr2RA;
              messageRa2Rec.readSecurity  = areaBuf->readSecRA;
              messageRa2Rec.writeSecurity = areaBuf->writeSecRA;
              messageRa2Rec.sysopSecurity = areaBuf->sysopSecRA;
              memcpy(&messageRa2Rec.readFlags    , &areaBuf->flagsRdRA    , 4);
              memcpy(&messageRa2Rec.readNotFlags , &areaBuf->flagsRdNotRA , 4);
              memcpy(&messageRa2Rec.writeFlags   , &areaBuf->flagsWrRA    , 4);
              memcpy(&messageRa2Rec.writeNotFlags, &areaBuf->flagsWrNotRA , 4);
              memcpy(&messageRa2Rec.sysopFlags   , &areaBuf->flagsSysRA   , 4);
              memcpy(&messageRa2Rec.sysopNotFlags, &areaBuf->flagsSysNotRA, 4);
              if (config.bbsProgram == BBS_RA25 || config.bbsProgram == BBS_ELEB)
                messageRa2Rec.areanum = count2++;

              write(folderHandle, &messageRa2Rec, sizeof(messageRa2Type));
              if (config.bbsProgram == BBS_RA25 || config.bbsProgram == BBS_ELEB)
              {
                temp = (u16)(lseek(folderHandle, 0, SEEK_CUR) / sizeof(messageRa2Type));
                write(indexHandle, &temp, 2);
              }
            }
          }
          if (config.bbsProgram == BBS_RA25 || config.bbsProgram == BBS_ELEB)
            close(indexHandle);
        }
        close(folderHandle);
      }
    }

    if (config.bbsProgram == BBS_SBBS)
    {
      memset(SBBSakaUsed, 0, MBBOARDS);

      strcpy(stpcpy(tempStr, fixPath(config.autoRAPath)), "boards.bbs");

      if ((folderHandle = open(tempStr, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, dDEFOMODE)) == -1)  // already fixPath'd
        displayMessage("Can't open BOARDS.BBS for output");
      else
      {
        for (count2 = 1; count2 <= MBBOARDS; count2++)
        {
          memset(&SBBSBoardRec, 0, sizeof(SBBSBoardRecType));

          count = 0;
          while (count < MAX_NETAKAS && config.netmailBoard[count] != count2)
            count++;

          if (count < MAX_NETAKAS)
          {
            if (*config.descrAKA[count])
              strncpy(SBBSBoardRec.Name, config.descrAKA[count], 30);
            else
            {
              if (count)
                sprintf(SBBSBoardRec.Name, "Netmail AKA %2u", count);
              else
                strcpy(SBBSBoardRec.Name, "Netmail Main");
            }
            SBBSBoardRec.NameLength = strlen(SBBSBoardRec.Name);

            memcpy(SBBSBoardRec.QwkName, config.qwkName[count], 12);
            SBBSBoardRec.QwkNameLength = strlen(config.qwkName[count]);

            SBBSBoardRec.Typ         = 1;
            SBBSBoardRec.Kinds       = config.msgKindsRA[count];
            SBBSBoardRec.Aliases     = config.attrRA[count] & BIT5 ? 1 :
                                       config.attrRA[count] & BIT3 ? 3 : 0;
            SBBSBoardRec.Attrib      = config.attrSBBS[count] |
                                       (config.attrRA[count] & BIT1 ? BIT0 : 0) |
                                       (config.attrRA[count] & BIT6 ? BIT2 : 0);
            SBBSBoardRec.Replystatus = config.replyStatSBBS[count];
            SBBSBoardRec.Age         = config.minAgeSBBS[count];
            SBBSBoardRec.UseAka      = count;
            SBBSBoardRec.ReadSecLvl  = config.readSecRA[count];
            SBBSBoardRec.WriteSecLvl = config.writeSecRA[count];
            SBBSBoardRec.SysopSecLvl = config.sysopSecRA[count];
            memcpy(&SBBSBoardRec.ReadFlags , &config.readFlagsRA [count], 4);
            memcpy(&SBBSBoardRec.WriteFlags, &config.writeFlagsRA[count], 4);
            memcpy(&SBBSBoardRec.SysopFlags, &config.sysopFlagsRA[count], 4);

            SBBSakaUsed[count2-1] = count;
          }
          else if (config.dupBoard == count2)
          {
            if (config.genOptions.incBDRRA)
            {
              strcpy(SBBSBoardRec.Name, "Duplicate msgs");
              SBBSBoardRec.NameLength = strlen (SBBSBoardRec.Name);
              strcpy(SBBSBoardRec.QwkName, "Duplicates");
              SBBSBoardRec.QwkNameLength = strlen (SBBSBoardRec.QwkName);
              SBBSBoardRec.Kinds = 3;
              SBBSBoardRec.Replystatus = 3;
              SBBSBoardRec.ReadSecLvl  = 32000;
              SBBSBoardRec.WriteSecLvl = 32000;
              SBBSBoardRec.SysopSecLvl = 32000;
            }
          }
          else if (config.badBoard == count2)
          {
            if (config.genOptions.incBDRRA)
            {
              strcpy(SBBSBoardRec.Name, "Bad messages");
              SBBSBoardRec.NameLength = strlen (SBBSBoardRec.Name);
              strcpy(SBBSBoardRec.QwkName, "Bad messages");
              SBBSBoardRec.QwkNameLength = strlen (SBBSBoardRec.QwkName);
              SBBSBoardRec.Kinds       = 3;
              SBBSBoardRec.Replystatus = 3;
              SBBSBoardRec.ReadSecLvl  = 32000;
              SBBSBoardRec.WriteSecLvl = 32000;
              SBBSBoardRec.SysopSecLvl = 32000;
            }
          }
          else if (config.recBoard == count2)
          {
            if (config.genOptions.incBDRRA)
            {
              strcpy(SBBSBoardRec.Name, "Recovery board");
              SBBSBoardRec.NameLength = strlen (SBBSBoardRec.Name);
              strcpy(SBBSBoardRec.QwkName, "Recovery");
              SBBSBoardRec.QwkNameLength = strlen (SBBSBoardRec.QwkName);
              SBBSBoardRec.Kinds = 3;
              SBBSBoardRec.Replystatus = 3;
              SBBSBoardRec.ReadSecLvl  = 32000;
              SBBSBoardRec.WriteSecLvl = 32000;
              SBBSBoardRec.SysopSecLvl = 32000;
            }
          }
          else
          {
            count = 0;
            while (count < areaInfoCount)
            {
              if ((*areaInfoIndex)[count] == count2)
              {
                getRec(CFG_ECHOAREAS, count);
                if (!*areaBuf->msgBasePath)
                  break;
              }
              count++;
            }
            if ((count < areaInfoCount) &&
                (!*areaBuf->msgBasePath) &&
                (areaBuf->options.active) &&
                (areaBuf->options.export2BBS) )
            {
              if (config.genOptions.commentFRA && *areaBuf->comment)
                strncpy(SBBSBoardRec.Name, areaBuf->comment, 30);
              else
                strncpy(SBBSBoardRec.Name, areaBuf->areaName, 30);

              SBBSBoardRec.NameLength = strlen(SBBSBoardRec.Name);

              strncpy(SBBSBoardRec.QwkName, areaBuf->qwkName, 12);
              SBBSBoardRec.QwkNameLength = strlen(areaBuf->qwkName);

              SBBSBoardRec.Typ     = areaBuf->options.local?0:3;
              SBBSBoardRec.Kinds   = areaBuf->msgKindsRA;

              SBBSBoardRec.Aliases = areaBuf->attrRA & BIT5 ? 1 :
                                     areaBuf->attrRA & BIT3 ? 3 : 0;
              SBBSBoardRec.Attrib  = areaBuf->attrSBBS |
                                     (areaBuf->attrRA & BIT1 ? BIT0 : 0) |
                                     (areaBuf->attrRA & BIT6 ? BIT2 : 0);
              SBBSBoardRec.Replystatus = areaBuf->replyStatSBBS;
              SBBSBoardRec.Group   = getGroupChar(areaBuf->group);
              SBBSBoardRec.UseAka  = checkMax(areaBuf->address, 9);
              SBBSBoardRec.Age     = areaBuf->minAgeSBBS;

              SBBSBoardRec.ReadSecLvl = areaBuf->readSecRA;
              SBBSBoardRec.WriteSecLvl = areaBuf->writeSecRA;
              SBBSBoardRec.SysopSecLvl = areaBuf->sysopSecRA;
              memcpy(&SBBSBoardRec.ReadFlags , &areaBuf->flagsRdRA , 4);
              memcpy(&SBBSBoardRec.WriteFlags, &areaBuf->flagsWrRA , 4);
              memcpy(&SBBSBoardRec.SysopFlags, &areaBuf->flagsSysRA, 4);

              SBBSakaUsed[count2-1] = checkMax(areaBuf->address, 9);
            }
          }
          write (folderHandle, &SBBSBoardRec, sizeof(SBBSBoardRecType));
        }
        close (folderHandle);

        strcpy(stpcpy(tempStr, fixPath(config.autoRAPath)), "config.bbs");

        if (((folderHandle = open(tempStr, O_WRONLY | O_BINARY)) == -1) ||  // already fixPath'd
            (lseek(folderHandle, 0x442, SEEK_SET) == -1)             ||
            (write(folderHandle, SBBSakaUsed, MBBOARDS) != MBBOARDS)           ||
            (close(folderHandle) == -1))
          displayMessage("Could not update CONFIG.BBS");
      }
    }
#endif
    if (config.bbsProgram == BBS_QBBS)
    {
      strcpy(stpcpy(tempStr, fixPath(config.autoRAPath)), "msgcfg.dat");

      if ((folderHandle = open(tempStr, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, dDEFOMODE)) == -1)  // already fixPath'd
        displayMessage("Can't open MSGCFG.DAT for output");

      else
      {
        for (count2 = 1; count2 <= MBBOARDS; count2++)
        {
          memset(&QBBSBoardRec, 0, sizeof(QBBSBoardRecType));

          count = 0;
          while (count < MAX_NETAKAS && config.netmailBoard[count] != count2)
            count++;

          if (count < MAX_NETAKAS)
          {
            if (*config.descrAKA[count])
              strncpy(QBBSBoardRec.Name, config.descrAKA[count], 40);
            else
            {
              if (count)
                sprintf(QBBSBoardRec.Name, "Netmail AKA %2u", count);
              else
                strcpy(QBBSBoardRec.Name, "Netmail Main");
            }
            QBBSBoardRec.NameLength = strlen(QBBSBoardRec.Name);

            QBBSBoardRec.Typ   = 1;
            QBBSBoardRec.Kinds = config.msgKindsRA[count];
            QBBSBoardRec.Combined = config.attrRA[count] & BIT1 ? 1 : 0;
            QBBSBoardRec.Aliases  = config.attrRA[count] & BIT5 ? 2 :
                                    config.attrRA[count] & BIT3 ? 1 : 0;
            QBBSBoardRec.Group    = config.groupsQBBS[count];
            QBBSBoardRec.Aka      = count;
            QBBSBoardRec.AllowDelete = (config.attrRA[count] & BIT6) ? 1:0;
            QBBSBoardRec.KeepCnt  = config.msgsAKA[count];
            QBBSBoardRec.KillOld  = config.daysAKA[count];
            QBBSBoardRec.KillRcvd = config.daysRcvdAKA[count];
            QBBSBoardRec.ReadSecLvl     = config.readSecRA[count];
            QBBSBoardRec.WriteSecLvl    = config.writeSecRA[count];
            QBBSBoardRec.TemplateSecLvl = config.templateSecQBBS[count];
            QBBSBoardRec.SysopSecLvl    = config.sysopSecRA[count];
            memcpy(&QBBSBoardRec.ReadFlags    , &config.readFlagsRA      [count], 4);
            memcpy(&QBBSBoardRec.WriteFlags   , &config.writeFlagsRA     [count], 4);
            memcpy(&QBBSBoardRec.TemplateFlags, &config.templateFlagsQBBS[count], 4);
            memcpy(&QBBSBoardRec.SysopFlags   , &config.sysopFlagsRA     [count], 4);
          }
          else if (config.dupBoard == count2)
          {
            if (config.genOptions.incBDRRA)
            {
              strcpy(QBBSBoardRec.Name, "Duplicate msgs");
              QBBSBoardRec.NameLength = strlen (QBBSBoardRec.Name);
              QBBSBoardRec.Kinds = 3;
              QBBSBoardRec.ReadSecLvl     = 32000;
              QBBSBoardRec.WriteSecLvl    = 32000;
              QBBSBoardRec.TemplateSecLvl = 32000;
              QBBSBoardRec.SysopSecLvl    = 32000;
            }
          }
          else if (config.badBoard == count2)
          {
            if (config.genOptions.incBDRRA)
            {
              strcpy(QBBSBoardRec.Name, "Bad messages");
              QBBSBoardRec.NameLength = strlen (QBBSBoardRec.Name);
              QBBSBoardRec.Kinds = 3;
              QBBSBoardRec.ReadSecLvl     = 32000;
              QBBSBoardRec.WriteSecLvl    = 32000;
              QBBSBoardRec.TemplateSecLvl = 32000;
              QBBSBoardRec.SysopSecLvl    = 32000;
            }
          }
          else if (config.recBoard == count2)
          {
            if (config.genOptions.incBDRRA)
            {
              strcpy(QBBSBoardRec.Name, "Recovery board");
              QBBSBoardRec.NameLength = strlen(QBBSBoardRec.Name);
              QBBSBoardRec.Kinds = 3;
              QBBSBoardRec.ReadSecLvl     = 32000;
              QBBSBoardRec.WriteSecLvl    = 32000;
              QBBSBoardRec.TemplateSecLvl = 32000;
              QBBSBoardRec.SysopSecLvl    = 32000;
            }
          }
          else
          {
            count = 0;
            while ( count < areaInfoCount )
            {
              if ( (*areaInfoIndex)[count] == count2 )
              {
                getRec(CFG_ECHOAREAS, count);
                if (!*areaBuf->msgBasePath)
                  break;
              }
              count++;
            }
            if ((count < areaInfoCount) &&
                (!*areaBuf->msgBasePath) &&
                (areaBuf->options.active) &&
                (areaBuf->options.export2BBS) )
            {
              if (config.genOptions.commentFRA && *areaBuf->comment)
                strncpy(QBBSBoardRec.Name, areaBuf->comment, 40);
              else
                strncpy(QBBSBoardRec.Name, areaBuf->areaName, 40);

              QBBSBoardRec.NameLength = strlen(QBBSBoardRec.Name);

              QBBSBoardRec.Typ      = areaBuf->options.local?0:3;
              QBBSBoardRec.Kinds    = areaBuf->msgKindsRA;
              QBBSBoardRec.Combined = areaBuf->attrRA & BIT1 ? 1 : 0;
              QBBSBoardRec.Aliases  = areaBuf->attrRA & BIT5 ? 2 :
                                      areaBuf->attrRA & BIT3 ? 1 : 0;
              QBBSBoardRec.Group    = areaBuf->groupsQBBS;
              QBBSBoardRec.Aka      = checkMax(areaBuf->address, 10);

              strncpy(QBBSBoardRec.OriginLine, areaBuf->originLine, 58);
              QBBSBoardRec.OriginLineLength = strlen(QBBSBoardRec.OriginLine);

              QBBSBoardRec.AllowDelete = (areaBuf->attrRA & BIT6) ? 1:0;
              QBBSBoardRec.KeepCnt     = areaBuf->msgs;
              QBBSBoardRec.KillOld     = areaBuf->days;
              QBBSBoardRec.KillRcvd    = areaBuf->daysRcvd;

              QBBSBoardRec.ReadSecLvl     = areaBuf->readSecRA;
              QBBSBoardRec.WriteSecLvl    = areaBuf->writeSecRA;
              QBBSBoardRec.TemplateSecLvl = areaBuf->templateSecQBBS;
              QBBSBoardRec.SysopSecLvl    = areaBuf->sysopSecRA;
              memcpy(&QBBSBoardRec.ReadFlags    , &areaBuf->flagsRdRA        , 4);
              memcpy(&QBBSBoardRec.WriteFlags   , &areaBuf->flagsWrRA        , 4);
              memcpy(&QBBSBoardRec.TemplateFlags, &areaBuf->flagsTemplateQBBS, 4);
              memcpy(&QBBSBoardRec.SysopFlags   , &areaBuf->flagsSysRA       , 4);
            }
          }
          write(folderHandle, &QBBSBoardRec, sizeof(QBBSBoardRecType));
        }
        close(folderHandle);
      }
    }
#ifndef GOLDBASE
    if (config.bbsProgram == BBS_PROB ) /* ProBoard */
    {
      proBoardType proBoardRec;

      strcpy(stpcpy(tempStr, fixPath(config.autoRAPath)), "messages.pb");

      if ((folderHandle = open(tempStr, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, dDEFOMODE)) == -1)  // already fixPath'd
        displayMessage ("Can't open MESSAGES.PB for output");
      else
      {
        for (count2 = 1; count2 <= maxBoardNumRA; count2++)
        {
          memset(&proBoardRec, 0, sizeof(proBoardType));

          proBoardRec.areaNum = count2;
          proBoardRec.hudsonBase = count2;

          count = 0;
          while (count < MAX_NETAKAS && config.netmailBoard[count] != count2)
            count++;

          if (count < MAX_NETAKAS)
          {
            if (*config.descrAKA[count])
              strncpy(proBoardRec.name, config.descrAKA[count], 40);
            else
            {
              if (count)
                sprintf(proBoardRec.name, "Netmail AKA %2u", count);
              else
                strcpy(proBoardRec.name, "Netmail Main");
            }
            proBoardRec.msgType       = (config.msgKindsRA[count] <= 3) ? config.msgKindsRA[count] : 0;
            proBoardRec.msgKind       = MSGKIND_NET;;
            proBoardRec.msgBaseType   = MSGBASE_HUDSON;
            proBoardRec.groups[0]     = config.groupRA[count];
            proBoardRec.groups[1]     = config.altGroupRA[count][0];
            proBoardRec.groups[2]     = config.altGroupRA[count][1];
            proBoardRec.groups[3]     = config.altGroupRA[count][2];
            proBoardRec.allGroups     = config.attr2RA[count] & BIT0;
            proBoardRec.flags         = (config.attrRA[count] & BIT3) ? ((config.attrRA[count] & BIT5) ? 3 : 1) : 0;
            proBoardRec.readLevel     = config.readSecRA[count];
            proBoardRec.readFlags     = bitSwap(&config.readFlagsRA [count][0]);
            proBoardRec.readFlagsNot  = bitSwap(&config.readFlagsRA [count][4]);
            proBoardRec.writeLevel    = config.writeSecRA[count];
            proBoardRec.writeFlags    = bitSwap(&config.writeFlagsRA[count][0]);
            proBoardRec.writeFlagsNot = bitSwap(&config.writeFlagsRA[count][4]);
            proBoardRec.sysopLevel    = config.sysopSecRA[count];
            proBoardRec.sysopFlags    = bitSwap(&config.sysopFlagsRA[count][0]);
            proBoardRec.sysopFlagsNot = bitSwap(&config.sysopFlagsRA[count][4]);

            proBoardRec.aka = count;
            proBoardRec.rcvKillDays = config.daysRcvdAKA[count];
            proBoardRec.msgKillDays = config.daysAKA[count];
            proBoardRec.maxMsgs     = config.msgsAKA[count];

            /* board names here.... */
            proBoardRec.minAge = config.minAgeSBBS[count];
          }
          else if (config.dupBoard == count2)
          {
            if (config.genOptions.incBDRRA)
            {
              strcpy(proBoardRec.name, "Duplicate messages");
              proBoardRec.msgKind = MSGKIND_LOCAL;
              proBoardRec.readLevel  = 32000;
              proBoardRec.writeLevel = 32000;
              proBoardRec.sysopLevel = 32000;
            }
          }
          else if (config.badBoard == count2)
          {
            if (config.genOptions.incBDRRA)
            {
              strcpy(proBoardRec.name, "Bad messages");
              proBoardRec.msgKind = MSGKIND_LOCAL;
              proBoardRec.readLevel  = 32000;
              proBoardRec.writeLevel = 32000;
              proBoardRec.sysopLevel = 32000;
            }
          }
          else if (config.recBoard == count2)
          {
            if (config.genOptions.incBDRRA)
            {
              strcpy(proBoardRec.name, "Recovery board");
              proBoardRec.msgKind = MSGKIND_LOCAL;
              proBoardRec.readLevel  = 32000;
              proBoardRec.writeLevel = 32000;
              proBoardRec.sysopLevel = 32000;
            }
          }
          else
          {
            count = 0;
            while (count < areaInfoCount)
            {
              if ((*areaInfoIndex)[count] == count2)
              {
                getRec(CFG_ECHOAREAS, count);
                break;
              }
              count++;
            }
            if ((count < areaInfoCount) &&
                (areaBuf->options.active) &&
                (areaBuf->options.export2BBS) )
            {
              if (config.genOptions.commentFRA && *areaBuf->comment)
                strcpy(proBoardRec.name, areaBuf->comment);
              else
                strcpy(proBoardRec.name, areaBuf->areaName);

              strcpy(proBoardRec.echoTag, areaBuf->areaName);
              strcpy(proBoardRec.qwkTag, areaBuf->qwkName);
              proBoardRec.msgType = (areaBuf->msgKindsRA <= 3) ? areaBuf->msgKindsRA : 0;
              proBoardRec.msgKind = areaBuf->options.local ? MSGKIND_LOCAL:
                                    areaBuf->options.allowPrivate ? MSGKIND_PVTECHO:
                                    MSGKIND_ECHO;
              proBoardRec.groups[0] = areaBuf->groupRA;
              proBoardRec.groups[1] = areaBuf->altGroupRA[0];
              proBoardRec.groups[2] = areaBuf->altGroupRA[1];
              proBoardRec.groups[3] = areaBuf->altGroupRA[2];
              proBoardRec.allGroups = areaBuf->attr2RA & BIT0;
              proBoardRec.flags     = areaBuf->attrRA & BIT3 ?
                                      ((areaBuf->attrRA & BIT5) ? 3 : 1) : 0;
              proBoardRec.replyBoard = areaBuf->netReplyBoardRA;
              strcpy(proBoardRec.origin, areaBuf->originLine);

              proBoardRec.aka    = areaBuf->address; //checkMax(areaBuf->address, 10);
              proBoardRec.rcvKillDays = areaBuf->daysRcvd;
              proBoardRec.msgKillDays = areaBuf->days;
              proBoardRec.maxMsgs     = areaBuf->msgs;
              if (*areaBuf->msgBasePath)
              {
                strcpy(proBoardRec.path, fixPath(areaBuf->msgBasePath));
                proBoardRec.msgBaseType = MSGBASE_JAM;
              }
              else
              {
                proBoardRec.hudsonBase = 0;
                proBoardRec.msgBaseType = MSGBASE_HUDSON;
              }
              proBoardRec.minAge        = areaBuf->minAgeSBBS;
              proBoardRec.readLevel     = areaBuf->readSecRA;
              proBoardRec.writeLevel    = areaBuf->writeSecRA;
              proBoardRec.sysopLevel    = areaBuf->sysopSecRA;
              proBoardRec.readFlags     = bitSwap(&areaBuf->flagsRdRA    [0]);
              proBoardRec.readFlagsNot  = bitSwap(&areaBuf->flagsRdNotRA [0]);
              proBoardRec.writeFlags    = bitSwap(&areaBuf->flagsWrRA    [0]);
              proBoardRec.writeFlagsNot = bitSwap(&areaBuf->flagsWrNotRA [0]);
              proBoardRec.sysopFlags    = bitSwap(&areaBuf->flagsSysRA   [0]);
              proBoardRec.sysopFlagsNot = bitSwap(&areaBuf->flagsSysNotRA[0]);
            }
          }
          write(folderHandle, &proBoardRec, sizeof(proBoardType));
        }
        for (count = 0; count < areaInfoCount; count++)
        {
          getRec(CFG_ECHOAREAS, count);
          if (areaBuf->options.active && areaBuf->boardNumRA == 0 && *areaBuf->msgBasePath && areaBuf->options.export2BBS)
          {
            memset(&proBoardRec, 0, sizeof(proBoardType));

            if (config.genOptions.commentFRA && *areaBuf->comment)
              strcpy(proBoardRec.name, areaBuf->comment);
            else
              strcpy(proBoardRec.name, areaBuf->areaName);

            strcpy(proBoardRec.echoTag, areaBuf->areaName);
            strcpy(proBoardRec.qwkTag , areaBuf->qwkName);
            strcpy(proBoardRec.path   , fixPath(areaBuf->msgBasePath));
            strcpy(proBoardRec.origin , areaBuf->originLine);
            proBoardRec.areaNum       = count2++;
            proBoardRec.msgBaseType   = MSGBASE_JAM;
            proBoardRec.msgType       = (areaBuf->msgKindsRA <= 3) ? areaBuf->msgKindsRA : 0;
            proBoardRec.msgKind       = areaBuf->options.local ? MSGKIND_LOCAL
                                      : areaBuf->options.allowPrivate ? MSGKIND_PVTECHO : MSGKIND_ECHO;
            proBoardRec.groups[0]     = areaBuf->groupRA;
            proBoardRec.groups[1]     = areaBuf->altGroupRA[0];
            proBoardRec.groups[2]     = areaBuf->altGroupRA[1];
            proBoardRec.groups[3]     = areaBuf->altGroupRA[2];
            proBoardRec.allGroups     = areaBuf->attr2RA & BIT0;
            proBoardRec.flags         = areaBuf->attrRA  & BIT3 ? ((areaBuf->attrRA & BIT5) ? 3 : 1) : 0;
            proBoardRec.replyBoard    = areaBuf->netReplyBoardRA;
            proBoardRec.aka           = areaBuf->address; //checkMax(areaBuf->address, 10);
            proBoardRec.rcvKillDays   = areaBuf->daysRcvd;
            proBoardRec.msgKillDays   = areaBuf->days;
            proBoardRec.maxMsgs       = areaBuf->msgs;
            proBoardRec.minAge        = areaBuf->minAgeSBBS;
            proBoardRec.readLevel     = areaBuf->readSecRA;
            proBoardRec.writeLevel    = areaBuf->writeSecRA;
            proBoardRec.sysopLevel    = areaBuf->sysopSecRA;
            proBoardRec.readFlags     = bitSwap(&areaBuf->flagsRdRA    [0]);
            proBoardRec.readFlagsNot  = bitSwap(&areaBuf->flagsRdNotRA [0]);
            proBoardRec.writeFlags    = bitSwap(&areaBuf->flagsWrRA    [0]);
            proBoardRec.writeFlagsNot = bitSwap(&areaBuf->flagsWrNotRA [0]);
            proBoardRec.sysopFlags    = bitSwap(&areaBuf->flagsSysRA   [0]);
            proBoardRec.sysopFlagsNot = bitSwap(&areaBuf->flagsSysNotRA[0]);
            write(folderHandle, &proBoardRec, sizeof(proBoardType));
          }
        }
        close(folderHandle);
      }
    }
#endif
  }
error:
  closeConfig(CFG_ECHOAREAS);
  free(areaInfoIndex);
}
