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

#ifdef __WIN32__
#pragma message "Compiling in 32 bit mode"
#endif

#ifdef _Windows
#pragma message "Compiling for Windows"
#endif

#ifdef __MSDOS__
#pragma message "__MSDOS__ is defined"
#endif

#include <conio.h>
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>     // SH_DENY* flags
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"

#include "areamgr.h"
#include "cfgfile.h"
#include "export.h"
#include "fs_func.h"
#include "import.h"
#include "mtask.h"
#include "nodeinfo.h"
#include "nodemgr.h"
#include "packmgr.h"
#include "update.h"
#include "version.h"
#include "window.h"

extern  u16 allowConversion;

#ifdef __32BIT__
#define WaitForKey() kbhit()
#define GetKey()     getch()
#endif

extern uplinkNodeStrType uplinkNodeStr;
extern s16 color;

u16 defaultEnumRA;
s16 boardEdit = 0;

badEchoType  badEchos[MAX_BAD_ECHOS];
u16          badEchoCount = 0;

funcParType boardSelect[MAX_AKAS + 3];

configType     config;
char           configPath[FILENAME_MAX];
windowLookType windowLook;

u16             nodeInfoCount;
nodeInfoType   *nodeInfo[MAX_NODES];
rawEchoType     rawEchoRec;

rawEchoType     echoDefaultsRec; /* used in IMPORT.C */

#ifndef __32BIT__
u16 xOS2 = 0;
#endif

#ifdef __BORLANDC__
#ifdef __WIN32__
#ifndef __DPMI32__
struct  COUNTRY
{
  short   co_date;
  char    co_curr[5];
  char    co_thsep[2];
  char    co_desep[2];
  char    co_dtsep[2];
  char    co_tmsep[2];
  char    co_currstyle;
  char    co_digits;
  char    co_time;
  long    co_case;
  char    co_dasep[2];
  char    co_fill[10];
};
#endif
#endif
  struct COUNTRY countryInfo;
#else
  struct country countryInfo;
#endif

int main(int argc, char *argv[])
{
  s16       ch;
  menuType *mainMenu     = NULL;
  menuType *arc32Menu    = NULL;
  menuType *deArc32Menu  = NULL;
  menuType *sysMiscMenu  = NULL;
  menuType *systemMenu   = NULL;
  menuType *logMenu      = NULL;
  menuType *miscMenu     = NULL;
  menuType *userMenu     = NULL;
  menuType *pmailMenu    = NULL;
  menuType *pathMenu     = NULL;
  menuType *mailMenu     = NULL;
  menuType *mbMenu       = NULL;
  menuType *internetMenu = NULL;
  menuType *mgrMenu      = NULL;
  menuType *pingMenu     = NULL;
  menuType *netMenu      = NULL;
  menuType *address1Menu = NULL;
  menuType *address2Menu = NULL;
  menuType *uplMenu      = NULL;
  menuType *groupMenu    = NULL;
  menuType *genMenu      = NULL;
  menuType *warningMenu  = NULL;
  menuType *autoExpMenu  = NULL;
  menuType *anImpMenu    = NULL;
  menuType *impExpMenu   = NULL;
  toggleType bbsToggle;
  toggleType logStyleToggle;
  toggleType tearToggle;
  toggleType arcToggle;
  toggleType colorToggle;
  fhandle     configHandle;
  s16         update = 0;
  u16         mode = 0;
  tempStrType configFileName;
  char        versionStr[80];
  fhandle     tempHandle
            , fmailLocHandle = -1;
  tempStrType tempStr
            , tempStr2;
  char       *helpPtr;
  u16         count;
  toggleType  mailerToggle;

  headerType  *adefHeader;
  rawEchoType *adefBuf;
  time_t      time1, time2, time2a;

#ifdef __BORLANDC__
  putenv("TZ=LOC0");
  tzset();
#endif // __BORLANDC__

  allowConversion = 1;

#ifndef __WIN32__
  country(0, &countryInfo);
#else
  countryInfo.co_tmsep[0] = ':';
  countryInfo.co_time = 1;  // 24 hour format
#endif

  for (count = 0; count < MAX_AKAS; count++)
  {
    boardSelect[count].numPtr = &config.netmailBoard[count];
    boardSelect[count].f      = (function)netDisplayAreas;
  }
  boardSelect[MAX_AKAS].numPtr   = &config.badBoard;
  boardSelect[MAX_AKAS].f        = (function)badduprecDisplayAreas;
  boardSelect[MAX_AKAS+1].numPtr = &config.dupBoard;
  boardSelect[MAX_AKAS+1].f      = (function)badduprecDisplayAreas;
  boardSelect[MAX_AKAS+2].numPtr = &config.recBoard;
  boardSelect[MAX_AKAS+2].f      = (function)badduprecDisplayAreas;

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

  memset(&config, 0, sizeof(configType));
  strcpy(stpcpy(configFileName, configPath), dCFGFNAME);

  if (((configHandle = open(configFileName, O_BINARY | O_RDONLY)) == -1) ||
      ((count = read(configHandle, &config, sizeof (configType))) == 0) ||
      (close(configHandle) == -1))
  {
    memset(&config, 0, sizeof(configType));

    time(&config.creationDate);
    config.creationDate ^= 0xe534a17bL;

    config.versionMajor = CONFIG_MAJOR;
    config.versionMinor = CONFIG_MINOR;

    config.colorSet = 1;

    config.mailOptions.dupDetection = 1;
    config.mailOptions.checkPktDest = 1;
    config.mbOptions.mbSharing      = 1;

    config.logInfo = LOG_INBOUND |LOG_OUTBOUND|LOG_PKTINFO|LOG_XPKTINFO|
                     LOG_UNEXPPWD|LOG_SENTRCVD|LOG_STATS;
  }
  if (count == 0)
  {
    strcpy (config.arc32.programName, "PKArc.Com -a");
    strcpy (config.zip32.programName, "PKZip.Exe -ex");
    strcpy (config.lzh32.programName, "LHA.Exe a /m /o");
    strcpy (config.pak32.programName, "Pak.Exe a /ST /P /WN");
    strcpy (config.zoo32.programName, "Zoo.Exe aP:");
    strcpy (config.arj32.programName, "ARJ.Exe a -e -u -y");
    strcpy (config.sqz32.programName, "SQZ.Exe a /p3");
    strcpy (config.uc232.programName, "UC.Exe a");
    strcpy (config.rar32.programName, "RAR.Exe a -y -s -m5 -ep -std -cfg-");
    strcpy (config.jar32.programName, "JAR.Exe a -y");
    config.arc32.memRequired = 0;
    config.lzh32.memRequired = 0;
    config.pak32.memRequired = 0;
    config.zoo32.memRequired = 0;
    config.arj32.memRequired = 0;
    config.sqz32.memRequired = 0;
    config.uc232.memRequired = 0;
    config.rar32.memRequired = 0;
    config.jar32.memRequired = 0;

    strcpy (config.unArc32.programName, "PKXArc.Com -r");
    strcpy (config.unZip32.programName, "PKUnzip.Exe -o");
    strcpy (config.unLzh32.programName, "LHA.Exe e /cm");
    strcpy (config.unPak32.programName, "PAK.Exe E /WA");
    strcpy (config.unZoo32.programName, "Zoo.Exe eO");
    strcpy (config.unArj32.programName, "ARJ.Exe e -c -y");
    strcpy (config.unSqz32.programName, "SQZ.Exe e /o1");
    strcpy (config.unUc232.programName, "UC.Exe e");
    strcpy (config.unRar32.programName, "RAR.Exe e -y -std -cfg-");
    strcpy (config.unJar32.programName, "JAR.Exe e -y");
    config.unArc32.memRequired = 0;
    config.unLzh32.memRequired = 0;
    config.unPak32.memRequired = 0;
    config.unZoo32.memRequired = 0;
    config.unArj32.memRequired = 0;
    config.unSqz32.memRequired = 0;
    config.unUc232.memRequired = 0;
    config.unRar32.memRequired = 0;
    config.unJar32.memRequired = 0;
  }
  else if (count <= 8192)
  {
    strcpy (config.arc32.programName, config.arc.programName);
    strcpy (config.zip32.programName, config.zip.programName);
    strcpy (config.lzh32.programName, config.lzh.programName);
    strcpy (config.pak32.programName, config.pak.programName);
    strcpy (config.zoo32.programName, config.zoo.programName);
    strcpy (config.arj32.programName, config.arj.programName);
    strcpy (config.sqz32.programName, config.sqz.programName);
    strcpy (config.uc232.programName, config.uc2.programName);
    strcpy (config.rar32.programName, config.rar.programName);
    strcpy (config.jar32.programName, config.jar.programName);

    strcpy (config.unArc32.programName, config.unArc.programName);
    strcpy (config.unZip32.programName, config.unZip.programName);
    strcpy (config.unLzh32.programName, config.unLzh.programName);
    strcpy (config.unPak32.programName, config.unPak.programName);
    strcpy (config.unZoo32.programName, config.unZoo.programName);
    strcpy (config.unArj32.programName, config.unArj.programName);
    strcpy (config.unSqz32.programName, config.unSqz.programName);
    strcpy (config.unUc232.programName, config.unUc2.programName);
    strcpy (config.unRar32.programName, config.unRar.programName);
    strcpy (config.unJar32.programName, config.unJar.programName);

    memcpy(config.optionsAKA,        config._optionsAKA,        2*MAX_NA_OLD);
    memcpy(config.groupsQBBS,        config._groupsQBBS,          MAX_NA_OLD);
    memcpy(config.templateSecQBBS,   config._templateSecQBBS,   2*MAX_NA_OLD);
    memcpy(config.templateFlagsQBBS, config._templateFlagsQBBS, 4*MAX_NA_OLD);
    memcpy(config.attr2RA,           config._attr2RA,             MAX_NA_OLD);
    memcpy(config.aliasesQBBS,       config._aliasesQBBS,         MAX_NA_OLD);
    memcpy(config.groupRA,           config._groupRA,             MAX_NA_OLD);
    memcpy(config.altGroupRA,        config._altGroupRA,      2*3*MAX_NA_OLD);
    memcpy(config.qwkName,           config._qwkName,          13*MAX_NA_OLD);
    memcpy(config.minAgeSBBS,        config._minAgeSBBS,        2*MAX_NA_OLD);
    memcpy(config.daysRcvdAKA,       config._daysRcvdAKA,       2*MAX_NA_OLD);
    memcpy(config.replyStatSBBS,     config._replyStatSBBS,       MAX_NA_OLD);
    memcpy(config.attrSBBS,          config._attrSBBS,          2*MAX_NA_OLD);
    memcpy(config.msgKindsRA,        config._msgKindsRA,          MAX_NA_OLD);
    memcpy(config.attrRA,            config._attrRA,              MAX_NA_OLD);
    memcpy(config.readSecRA,         config._readSecRA,         2*MAX_NA_OLD);
    memcpy(config.readFlagsRA,       config._readFlagsRA,       4*MAX_NA_OLD);
    memcpy(config.writeSecRA,        config._writeSecRA,        2*MAX_NA_OLD);
    memcpy(config.writeFlagsRA,      config._writeFlagsRA,      4*MAX_NA_OLD);
    memcpy(config.sysopSecRA,        config._sysopSecRA,        2*MAX_NA_OLD);
    memcpy(config.sysopFlagsRA,      config._sysopFlagsRA,      4*MAX_NA_OLD);
    memcpy(config.daysAKA,           config._daysAKA,           2*MAX_NA_OLD);
    memcpy(config.msgsAKA,           config._msgsAKA,           2*MAX_NA_OLD);
    memcpy(config.descrAKA,          config._descrAKA,         51*MAX_NA_OLD);
    memcpy(config.netmailBoard,      config._netmailBoard,      2*MAX_NA_OLD);
    memcpy(config.akaList,           config._akaList,   sizeof(_akaListType));

    memset(config._optionsAKA,        0,    2*MAX_NA_OLD);
    memset(config._groupsQBBS,        0,      MAX_NA_OLD);
    memset(config._templateSecQBBS,   0,    2*MAX_NA_OLD);
    memset(config._templateFlagsQBBS, 0,    4*MAX_NA_OLD);
    memset(config._attr2RA,           0,      MAX_NA_OLD);
    memset(config._aliasesQBBS,       0,      MAX_NA_OLD);
    memset(config._groupRA,           0,      MAX_NA_OLD);
    memset(config._altGroupRA,        0,  2*3*MAX_NA_OLD);
    memset(config._qwkName,           0,   13*MAX_NA_OLD);
    memset(config._minAgeSBBS,        0,    2*MAX_NA_OLD);
    memset(config._daysRcvdAKA,       0,    2*MAX_NA_OLD);
    memset(config._replyStatSBBS,     0,      MAX_NA_OLD);
    memset(config._attrSBBS,          0,    2*MAX_NA_OLD);
    memset(config._msgKindsRA,        0,      MAX_NA_OLD);
    memset(config._attrRA,            0,      MAX_NA_OLD);
    memset(config._readSecRA,         0,    2*MAX_NA_OLD);
    memset(config._readFlagsRA,       0,    4*MAX_NA_OLD);
    memset(config._writeSecRA,        0,    2*MAX_NA_OLD);
    memset(config._writeFlagsRA,      0,    4*MAX_NA_OLD);
    memset(config._sysopSecRA,        0,    2*MAX_NA_OLD);
    memset(config._sysopFlagsRA,      0,    4*MAX_NA_OLD);
    memset(config._daysAKA,           0,    2*MAX_NA_OLD);
    memset(config._msgsAKA,           0,    2*MAX_NA_OLD);
    memset(config._descrAKA,          0,   51*MAX_NA_OLD);
    memset(config._netmailBoard,      0,    2*MAX_NA_OLD);
    memset(config._akaList,           0,      sizeof(_akaListType));

    memset(config.autoFMail102Path,   0,      sizeof(pathType));

    update = 1;
  }

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

  if (config.tearType == 2)
    config.tearType = 3;
  if (config.tearType == 4)
    config.tearType = 5;
  if (config.recBoard > MBBOARDS)
    config.recBoard = 0;

  if (config.bbsProgram == BBS_RA1X && config.genOptions._RA2)
  {
    config.bbsProgram = BBS_RA20;
    config.genOptions._RA2 = 0;
  }

  memset(&echoDefaultsRec, 0, RAWECHO_SIZE);
  echoDefaultsRec.options.security = 1;
  if (openConfig(CFG_AREADEF, &adefHeader, (void*)&adefBuf))
  {
    if (getRec(CFG_AREADEF, 0))
      memcpy (&echoDefaultsRec, adefBuf, RAWECHO_SIZE);

    closeConfig(CFG_AREADEF);
  }
  else
  {
    strcpy(stpcpy(tempStr, configPath), dARDFNAME);
    unlink (tempStr);
  }

  if (config.genOptions.monochrome)
    mode = 1;

  if (argc > 1)
  {
    if (stricmp(argv[1], "/M") == 0)
      mode = 1;
    if (stricmp(argv[1], "/C") == 0)
      mode = 2;
  }
  arcToggle.data = (char*)&config.defaultArc;
  arcToggle.text  [ 0] = "None";
  arcToggle.retval[ 0] = 0xFF;
  arcToggle.text  [ 1] = "ARC";
  arcToggle.retval[ 1] = 0;
  arcToggle.text  [ 2] = "ZIP";
  arcToggle.retval[ 2] = 1;
  arcToggle.text  [ 3] = "LZH";
  arcToggle.retval[ 3] = 2;
  arcToggle.text  [ 4] = "PAK";
  arcToggle.retval[ 4] = 3;
  arcToggle.text  [ 5] = "ZOO";
  arcToggle.retval[ 5] = 4;
  arcToggle.text  [ 6] = "ARJ";
  arcToggle.retval[ 6] = 5;
  arcToggle.text  [ 7] = "SQZ";
  arcToggle.retval[ 7] = 6;
  arcToggle.text  [ 8] = "UC2";
  arcToggle.retval[ 8] = 8;
  arcToggle.text  [ 9] = "RAR";
  arcToggle.retval[ 9] = 9;
  arcToggle.text  [10] = "JAR";
  arcToggle.retval[10] = 10;

  mailerToggle.data    = (char*)&config.mailer;
  mailerToggle.text[0] = "FrontDoor";
  mailerToggle.retval[0] = dMT_FrontDoor;
  mailerToggle.text[1] = "InterMail";
  mailerToggle.retval[1] = dMT_InterMail;
  mailerToggle.text[2] = "D'Bridge";
  mailerToggle.retval[2] = dMT_DBridge;
  mailerToggle.text[3] = "Binkley/Portal of Power";
  mailerToggle.retval[3] = dMT_Binkley;
  mailerToggle.text[4] = "MainDoor";
  mailerToggle.retval[4] = dMT_MainDoor;
  mailerToggle.text[5] = "Xenia";
  mailerToggle.retval[5] = dMT_Xenia;

  logStyleToggle.data    = (char*)&config.logStyle;
  logStyleToggle.text[0] = "FrontDoor";
  logStyleToggle.retval[0] = 0;
  logStyleToggle.text[1] = "QuickBBS";
  logStyleToggle.retval[1] = 1;
  logStyleToggle.text[2] = "D'Bridge";
  logStyleToggle.retval[2] = 2;
  logStyleToggle.text[3] = "Binkley";
  logStyleToggle.retval[3] = 3;
  logStyleToggle.text[4] = "FMail (W32)";
  logStyleToggle.retval[4] = 4;

  colorToggle.data    = (char*)&config.colorSet;
  colorToggle.text[0] = "Summer";
  colorToggle.retval[0] = 0;
  colorToggle.text[1] = "Winter";
  colorToggle.retval[1] = 1;
  colorToggle.text[2] = "Marine";
  colorToggle.retval[2] = 2;

  bbsToggle.data    = (char*)&config.bbsProgram;
  bbsToggle.text[0] = "RemoteAccess < 2.00";
  bbsToggle.retval[0] = BBS_RA1X;
  bbsToggle.text[1] = "RemoteAccess 2.00/2.02";
  bbsToggle.retval[1] = BBS_RA20;
  bbsToggle.text[2] = "RemoteAccess >= 2.50";
  bbsToggle.retval[2] = BBS_RA25;
  bbsToggle.text[3] = "SuperBBS";
  bbsToggle.retval[3] = BBS_SBBS;
  bbsToggle.text[4] = "QuickBBS";
  bbsToggle.retval[4] = BBS_QBBS;
  bbsToggle.text[5] = "TAG";
  bbsToggle.retval[5] = BBS_TAG;
  bbsToggle.text[6] = "ProBoard";
  bbsToggle.retval[6] = BBS_PROB;
  bbsToggle.text[7] = "EleBBS";
  bbsToggle.retval[7] = BBS_ELEB;

  tearToggle.data = &config.tearType;
  tearToggle.text[0] = "Default";
  tearToggle.retval[0] = 0;
  tearToggle.text[1] = "Custom + TID";
  tearToggle.retval[1] = 1;
  tearToggle.text[2] = "Empty + TID";
  tearToggle.retval[2] = 3;
  tearToggle.text[3] = "Absent + TID";
  tearToggle.retval[3] = 5;
  tearToggle.text[4] = "Default + TID";
  tearToggle.retval[4] = 6;

  initWindow (mode);
  color = 1;
  windowLook.background   = RED;
  windowLook.actvborderfg = YELLOW;
  windowLook.editfg       = RED;
  windowLook.mono_attr    = MONO_NORM;
  windowLook.wAttr        = NO_SAVE; // WB_DOUBLE_H | NO_SAVE;

  if (displayWindow(NULL, 0, 0, 79, 3) != 0)
  {
    displayMessage("Not enough memory available");
    deInit(5);
    return 1;
  }
  sprintf(versionStr, "%s - Configuration utility", VersionStr());

  printString(versionStr, 3, 1, YELLOW, RED, MONO_HIGH);
  sprintf(tempStr, "Copyright (C) 1991-%s by FMail Developers - All rights reserved", __DATE__ + 7);
  printString(tempStr, 3, 2, YELLOW, RED, MONO_HIGH);

  fillRectangle('Ü', 0, 4, 79, 4, BLUE, BLACK, 0);
  fillRectangle('ß', 0, 23, 79, 23, BLUE, BLACK, 0);
  fillRectangle(' ', 0, 24, 79, 24, YELLOW, BLACK, MONO_NORM);

  fillRectangle(' ', 0, 5, 79, 22, BLACK, BLUE, MONO_NORM);

  switch (config.colorSet)
  {
    case 0 : /* Summer colors */

      windowLook.background     = GREEN;
      windowLook.actvborderfg   = YELLOW;
      windowLook.inactvborderfg = LIGHTGREEN;
      windowLook.promptfg       = WHITE;
      windowLook.promptkeyfg    = YELLOW;
      windowLook.datafg         = LIGHTCYAN;
      windowLook.titlefg        = YELLOW;
      windowLook.titlebg        = LIGHTGRAY;
      windowLook.scrollbg       = LIGHTGRAY;
      windowLook.scrollfg       = BLUE;
      windowLook.editfg         = YELLOW;
      windowLook.editbg         = BLUE;
      windowLook.commentfg      = YELLOW;
      windowLook.commentbg      = BLACK;
      windowLook.atttextfg      = WHITE;
      windowLook.mono_attr      = MONO_HIGH;
      windowLook.wAttr          = TITLE_RIGHT | SHADOW; // WB_DOUBLE_H |
      break;

    case 1 : /* Winter colors */

      windowLook.background     = LIGHTGRAY;
      windowLook.actvborderfg   = WHITE;
      windowLook.inactvborderfg = DARKGRAY;
      windowLook.promptfg       = BLACK;
      windowLook.promptkeyfg    = RED;
      windowLook.datafg         = WHITE;
      windowLook.titlefg        = YELLOW;
      windowLook.titlebg        = LIGHTGRAY;
      windowLook.scrollbg       = BLUE;
      windowLook.scrollfg       = LIGHTCYAN;
      windowLook.editfg         = YELLOW;
      windowLook.editbg         = BLUE;
      windowLook.commentfg      = YELLOW;
      windowLook.commentbg      = BLACK;
      windowLook.atttextfg      = BLACK;
      windowLook.mono_attr      = MONO_HIGH;
      windowLook.wAttr          = TITLE_RIGHT | SHADOW; // WB_DOUBLE_H |
      break;

    case 2 : /* Marine colors */

      windowLook.background     = CYAN;
      windowLook.actvborderfg   = LIGHTCYAN;
      windowLook.inactvborderfg = DARKGRAY;
      windowLook.promptfg       = BLUE;
      windowLook.promptkeyfg    = RED;
      windowLook.datafg         = WHITE;
      windowLook.titlefg        = YELLOW;
      windowLook.titlebg        = LIGHTGRAY;
      windowLook.scrollbg       = BLUE;
      windowLook.scrollfg       = WHITE;
      windowLook.editfg         = YELLOW;
      windowLook.editbg         = BLUE;
      windowLook.commentfg      = YELLOW;
      windowLook.commentbg      = BLACK;
      windowLook.atttextfg      = BLACK;
      windowLook.mono_attr      = MONO_HIGH;
      windowLook.wAttr          = TITLE_RIGHT | SHADOW; // WB_DOUBLE_H |
      break;
  }

  if (_osmajor < 3)
  {
    displayMessage(FSNAME" requires at least DOS 3.0");
    fillRectangle(' ', 0, 4, 79, 24, LIGHTGRAY, BLACK, MONO_NORM);
    deInit(5);
    return 0;
  }

  strcpy(tempStr2, strcpy(tempStr, config.bbsPath));
  strcat(tempStr, "fmail.loc");
  if ((helpPtr = strrchr(tempStr2, '\\')) != NULL)
    *helpPtr = 0;

  if  (  !access(tempStr2, 0)
      && (fmailLocHandle = _sopen(tempStr, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYWR, S_IREAD | S_IWRITE)) == -1
      && errno != ENOFILE
      )
  {
    displayWindow(NULL, 7, 10, 73, 14);
    printString("Waiting for another copy of FMail, FTools or "FSNAME" to finish", 10, 12, WHITE, LIGHTGRAY, MONO_HIGH);

    time(&time1);
    time2 = time1;

    ch = 0;
    while (  (fmailLocHandle = _sopen(tempStr, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, S_IREAD | S_IWRITE)) == -1
          && (!config.activTimeOut || time2 - time1 < config.activTimeOut)
          && (ch = GetKey() & 0xff) != 27
          )
    {
      if (ch != 0 && ch != -1)
        WaitForKey();
      else
      {
        time2a = time2 + 1;
        while (time(&time2) <= time2a)
          returnTimeSlice(1);
      }
    }
    if (ch != 0 && ch != -1)
      WaitForKey();

    if (fmailLocHandle == -1)
    {
      removeWindow();
      if (ch != 27)
        printStringFill("Another copy did not finish in time. Exiting...", ' ', 76, 3, 2, WHITE, RED, MONO_HIGH);
      deInit(5);
      return 0;
    }
    removeWindow();
  }

  if (  config.versionMajor != CONFIG_MAJOR
     || config.versionMinor < 92
     || config.versionMinor > CONFIG_MINOR
     )
  {
    displayMessage(dCFGFNAME" was not created by "FSNAME" version 0.90/0.92/0.94/0.96");
    fillRectangle(' ', 0, 4, 79, 24, LIGHTGRAY, BLACK, MONO_NORM);
    deInit(5);
    if (fmailLocHandle != -1)
      close(fmailLocHandle);
    return 0;
  }

  if (config.versionMinor != CONFIG_MINOR)
  {
    update++;
    config.versionMinor = CONFIG_MINOR;
  }

  defaultEnumRA = (echoDefaultsRec.attrRA & BIT3)
                ? ((echoDefaultsRec.attrRA & BIT5) ? 1 : 2) : 0;

  strcpy(stpcpy(tempStr, configPath), dBDEFNAME);
  if ((tempHandle = open(tempStr, O_BINARY | O_RDONLY)) != -1)
  {
    badEchoCount = read(tempHandle, badEchos, MAX_BAD_ECHOS * sizeof(badEchoType)) / sizeof(badEchoType);
    close(tempHandle);
    if (badEchoCount)
    {
      printStringFill("Install new areas names found by FMail when it was tossing"
                     , ' ', 80, 0, 24, YELLOW, BLACK, MONO_NORM);
      if ((ch = askBoolean("New areas found. Install in AreaManager ?", 'Y')) == 'Y')
        areaMgr ();

      if (ch == 'N')
        unlink(tempStr);

      badEchoCount = 0;
    }
  }

  if ((warningMenu = createMenu(" WARNING ")) == NULL)
  {
    displayMessage("Not enough memory available");
    deInit(5);
    if (fmailLocHandle != -1)
      close(fmailLocHandle);
    return 1;
  }
  addItem(warningMenu, DISPLAY, FSNAME"'s AutoExport feature overwrites existing", 0, NULL, 0, 0, NULL);
  addItem(warningMenu, DISPLAY, "area configuration files! Any information stored", 0, NULL, 0, 0, NULL);
  addItem(warningMenu, DISPLAY, "in those files not present in "FSNAME"'s Area Manager", 0, NULL, 0, 0, NULL);
  addItem(warningMenu, DISPLAY, "will be lost.                Press ESC to continue.", 0, NULL, 0, 0, NULL);

  if ((autoExpMenu = createMenu(" AutoExport ")) == NULL)
  {
    displayMessage("Not enough memory available");
    deInit(5);
    if (fmailLocHandle != -1)
      close(fmailLocHandle);
    return 1;
  }
  addItem(autoExpMenu, NEW_WINDOW, "* WARNING *", 0, warningMenu, 6, 3,
           "Read this message before using AutoExport !");
  addItem(autoExpMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(autoExpMenu, PATH, "FMail 1.02 cfg", 0, config.autoFMail102Path, sizeof(pathType)-1, 0,
           "CHANGE SOMETHING IN AREAMGR AND NODEMGR TO CREATE AREA AND NODE FILES !!!");
  addItem(autoExpMenu, PATH, "Areas.BBS path", 0, config.autoAreasBBSPath, sizeof(pathType)-1, 0,
           "Directory to create Areas.BBS in");
  addItem(autoExpMenu, BOOL_INT, "ÈÍ> Include PT", 0, &config.genOptions, BIT8, 0,
           "Include pass through areas");
  addItem(autoExpMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(autoExpMenu, PATH, "Folder.FD/IM path", 0, config.autoFolderFdPath, sizeof(pathType)-1, 0,
           "Directory to create Folder.FD or IMFolder.CFG in");
  addItem(autoExpMenu, BOOL_INT, "ÈÍ> Use comment", 0, &config.genOptions, BIT7, 0,
           "Use comment instead of area tag for Folder.FD or IMFolder.CFG");
  addItem(autoExpMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(autoExpMenu, PATH, "Areas.GLD path", 0, config.autoGoldEdAreasPath, sizeof(pathType)-1, 0,
           "Directory to create GoldEd areas file in");
  addItem(autoExpMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(autoExpMenu, PATH, "BBS area file path", 0, config.autoRAPath, sizeof(pathType)-1, 0,
           "Directory to create MESSAGES.RA / BOARDS.BBS / MESSAGES.PB in");
  addItem(autoExpMenu, BOOL_INT, "ÌÍ> Include B/D/R", 0, &config.genOptions, BIT11, 0,
           "Include the Bad msg board, the Duplicate msg board and the Recovery board");
  addItem(autoExpMenu, BOOL_INT, "ÈÍ> Use comment", 0, &config.genOptions, BIT9, 0,
           "Use comment instead of area tag for MESSAGES.RA / BOARDS.BBS");

  if ((anImpMenu = createMenu(" Import config ")) == NULL)
    goto nomem;

  addItem(anImpMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(anImpMenu, FUNCTION, "Import Areas.BBS", 0, importAreasBBS, 0, 0,
           "Convert Areas.BBS into "dARFNAME);
  addItem(anImpMenu, FUNCTION, "Import RA info", 0, importRAInfo, 0, 0,
           "Import RA area (security) info into an existing FMail areas file");
  addItem(anImpMenu, FUNCTION, "Import Folder.FD", 0, importFolderFD, 0, 0,
           "Convert FrontDoor's Folder.FD into "dARFNAME);
  addItem(anImpMenu, FUNCTION, "Import Backbone.NA", 0, importNAInfo, 0, 0,
           "Import descriptions from Backbone.NA into "dARFNAME);
  addItem(anImpMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(anImpMenu, FUNCTION, "Import AreaFile.FD", 0, importAreaFileFD, 0, 0,
           "Convert TosScan's AreaFile.FD into "dARFNAME);
  addItem(anImpMenu, FUNCTION, "Import NodeFile.FD", 0, importNodeFileFD, 0, 0,
           "Convert TosScan's NodeFile.FD into "dNODFNAME);
  addItem(anImpMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(anImpMenu, FUNCTION, "Import AREAFILE.GE", 0, importGechoAr, 0, 0,
           "Convert GECHO's AREAFILE.GE into "dARFNAME" (GECHO 1.00 and up)");
  addItem(anImpMenu, FUNCTION, "Import NODEFILE.GE", 0, importGechoNd, 0, 0,
           "Convert GECHO's NODEFILE.GE into "dNODFNAME" (GECHO 1.00 and up)");
  addItem(anImpMenu, FUNCTION, "Import FE areas", 0, importFEAr, 0, 0,
           "Convert FastEcho's areas into "dARFNAME" (FastEcho 1.46 and up)");
  addItem(anImpMenu, FUNCTION, "Import FE nodes", 0, importFENd, 0, 0,
           "Convert FastEcho's nodes into "dNODFNAME" (FastEcho 1.46 and up)");
  addItem(anImpMenu, FUNCTION, "Import IMAIL.Ar", 0, importImailAr, 0, 0,
           "Convert IMAIL's IMAIL.AR into "dARFNAME" (IMAIL 1.40 and up)");
  addItem(anImpMenu, FUNCTION, "Import IMAIL.Nd", 0, importImailNd, 0, 0,
           "Convert IMAIL's IMAIL.Nd into "dNODFNAME" (IMAIL 1.40 and up)");

  if ((impExpMenu = createMenu(" Import/export ")) == NULL)
    goto nomem;

  addItem(impExpMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(impExpMenu, FUNCTION, "List area config", 0, listAreaConfig, 0, 0,
           "List area configuration");
  addItem(impExpMenu, FUNCTION, "List node config", 0, listNodeConfig, 0, 0,
           "List node configuration");
  addItem(impExpMenu, FUNCTION, "List pack config", 0, listPackConfig, 0, 0,
           "List Pack Manager configuration");
  addItem(impExpMenu, FUNCTION, "List groups", 0, listGroups, 0, 0,
           "List all existing groups and the areas belonging to them");
  addItem(impExpMenu, FUNCTION, "List member", 0, listNodeEcho, 0, 0,
           "List all conferences received by one system");
  addItem(impExpMenu, FUNCTION, "List "MBNAME" boards", 0, listHudsonBoards, 0, 0,
           "List "MBNAME" message base boards");
  addItem(impExpMenu, FUNCTION, "List JAM boards", 0, listJAMBoards, 0, 0,
           "List JAM message base boards");
  addItem(impExpMenu, FUNCTION, "List PT areas", 0, listPtAreas, 0, 0,
           "List all passthrough areas");
  addItem(impExpMenu, FUNCTION, "List general info", 0, listGeneralConfig, 0, 0,
           "List general configuration");
  addItem(impExpMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(impExpMenu, NEW_WINDOW, "Import config", 0, anImpMenu, 2, 0,
           "Import FD/IMAIL/Areas.BBS node and area configuration files");
  addItem(impExpMenu, NEW_WINDOW, "AutoExport", 0, autoExpMenu, 0, 0,
           "Automatically create GoldEd/FM/RA/Other BBS's area files and Areas.BBS");

  if ((arc32Menu = createMenu(" Compression programs ")) == NULL)
    goto nomem;

  addItem(arc32Menu, ENUM_INT, "Def", 0, &arcToggle, 0, 11,
           "Compression program to be used for nodes not listed in the Node Manager");
  addItem(arc32Menu, TEXT, "ARC", 0, config.arc32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(arc32Menu, TEXT, "ZIP", 0, config.zip32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(arc32Menu, TEXT, "LZH", 0, config.lzh32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(arc32Menu, TEXT, "PAK", 0, config.pak32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(arc32Menu, TEXT, "ZOO", 0, config.zoo32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(arc32Menu, TEXT, "ARJ", 0, config.arj32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(arc32Menu, TEXT, "SQZ", 0, config.sqz32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(arc32Menu, TEXT, "UC2", 0, config.uc232.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(arc32Menu, TEXT, "RAR", 0, config.rar32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(arc32Menu, TEXT, "JAR", 0, config.jar32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(arc32Menu, TEXT, "-?-", 0, config.customArc32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches> of extra compression utility");
  addItem(arc32Menu, TEXT, "Pre", 0, config.preArc32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches> of program called before compression");
  addItem(arc32Menu, TEXT, "Post", 0, config.postArc32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches> of program called after compression");

  if ((deArc32Menu = createMenu(" Decompression programs ")) == NULL)
    goto nomem;

  addItem(deArc32Menu, TEXT, "ARC", 0, config.unArc32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(deArc32Menu, TEXT, "ZIP", 0, config.unZip32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(deArc32Menu, TEXT, "LZH", 0, config.unLzh32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(deArc32Menu, TEXT, "PAK", 0, config.unPak32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(deArc32Menu, TEXT, "ZOO", 0, config.unZoo32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(deArc32Menu, TEXT, "ARJ", 0, config.unArj32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(deArc32Menu, TEXT, "SQZ", 0, config.unSqz32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(deArc32Menu, TEXT, "UC2", 0, config.unUc232.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(deArc32Menu, TEXT, "RAR", 0, config.unRar32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(deArc32Menu, TEXT, "JAR", 0, config.unJar32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches>");
  addItem(deArc32Menu, TEXT, "GUS", 0, config.GUS32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches> of General Unpack Shell");
  addItem(deArc32Menu, TEXT, "Pre", 0, config.preUnarc32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches> of program called before decompression");
  addItem(deArc32Menu, TEXT, "Post", 0, config.postUnarc32.programName, sizeof(archiverInfo)-3, 0,
           "<program name>.<extension> <switches> of program called after decompression");

  if ((pathMenu = createMenu(" Directories ")) == NULL)
    goto nomem;

  addItem(pathMenu, PATH, "Message base"    , 0, config.bbsPath      , sizeof(pathType) - 1, 0, "Where the RemoteAccess/SuperBBS/QuickBBS/TAG message base files are located");
  addItem(pathMenu, PATH, "Netmail"         , 0, config.netPath      , sizeof(pathType) - 1, 0, "Where the netmail messages are stored");
  addItem(pathMenu, PATH, "Incoming mail"   , 0, config.inPath       , sizeof(pathType) - 1, 0, "Where the incoming mailbundles are stored");
  addItem(pathMenu, PATH, "Outgoing mail"   , 0, config.outPath      , sizeof(pathType) - 1, 0, "Where the outgoing mailbundles are stored");
  addItem(pathMenu, PATH, "Outgoing backup" , 0, config.outBakPath   , sizeof(pathType) - 1, 0, "Where backups of the outgoing mail files are stored");
  addItem(pathMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(pathMenu, PATH, "Local PKTs"      , 0, config.securePath   , sizeof(pathType) - 1, 0, "Used for local programs that generate PKT files with bad address info (optional)");
  addItem(pathMenu, PATH, "Sent netmail"    , 0, config.sentPath     , sizeof(pathType) - 1, 0, "Where sent netmail messages are moved to (optional)");
  addItem(pathMenu, PATH, "Received netmail", 0, config.rcvdPath     , sizeof(pathType) - 1, 0, "Where received netmail messages are moved to (optional)");
  addItem(pathMenu, PATH, "Sent echomail"   , 0, config.sentEchoPath , sizeof(pathType) - 1, 0, "Where sent echomail messages are copied to (optional)");
  addItem(pathMenu, PATH, "Semaphore"       , 0, config.semaphorePath, sizeof(pathType) - 1, 0, "Where the rescan semaphore files of your mailer should be created (optional)");

  if ((logMenu = createMenu(" Log files ")) == NULL)
    goto nomem;

  addItem(logMenu, ENUM_INT, "Log style", 0, &logStyleToggle, 0, 5,
           "Which log file format should be used");
  addItem(logMenu, FILE_NAME, "Log file name", 0, config.logName, sizeof(pathType) - 1, 0,
           "Name of the FMail log file");
  addItem(logMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(logMenu, BOOL_INT, "Inbound activities", 0, &config.logInfo, BIT0, 0,
           "Log inbound activities such as decompressing files");
  addItem(logMenu, BOOL_INT, "Unexpected pwd       ", 32, &config.logInfo, BIT4, 0,
           "Log unexpected packet passwords");
  addItem(logMenu, BOOL_INT, "Outbound activities", 0, &config.logInfo, BIT1, 0,
           "Log outbound activities such created and updated mail files");
  addItem(logMenu, BOOL_INT, "Moving Sent/Rcvd mail", 32, &config.logInfo, BIT5, 0,
           "Log moving of Sent and Received netmail");
  addItem(logMenu, BOOL_INT, "Pack routing", 0, &config.logInfo, BIT7, 0,
           "Log netmail packing information");
  addItem(logMenu, BOOL_INT, "Outbound echomail    ", 32, &config.logInfo, BIT9, 0,
           "Log outbound echomail activity");
  addItem(logMenu, BOOL_INT, "Netmail import", 0, &config.logInfo, BIT10, 0,
           "Log netmail import information");
  addItem(logMenu, BOOL_INT, "Netmail export       ", 32, &config.logInfo, BIT11, 0,
           "Log netmail export information");
  addItem(logMenu, BOOL_INT, "FMail statistics", 0, &config.logInfo, BIT6, 0,
           "Log FMail's statistics");
  addItem(logMenu, BOOL_INT, "Message base info    ", 32, &config.logInfo, BIT8, 0,
           "Log message base information");
  addItem(logMenu, BOOL_INT, "Inbound packet info", 0, &config.logInfo, BIT2, 0,
           "Log inbound mail packet filenames and origin/destination nodes");
  addItem(logMenu, BOOL_INT, "Program execution    ", 32, &config.logInfo, BIT13, 0,
           "Log information about programs being started (e.g. compression programs)");
  addItem(logMenu, BOOL_INT, "ÈÍ> Extended info", 0, &config.logInfo, BIT3, 0,
           "Log extended packet info, such as program name, type, size, date and time");
  addItem(logMenu, BOOL_INT, "File open errors     ", 32, &config.logInfo, LOG_OPENERR, 0,
           "Log info about files that could not be opened by FMail (NOT ALWAYS REAL ERRORS!)");
  addItem(logMenu, BOOL_INT, "DEBUG: log all", 0, &config.logInfo, BIT15, 0,
           "Log all information without losing your custom settings");
  addItem(logMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(logMenu, FILE_NAME, "AreaMgr log", 0, config.areaMgrLogName, sizeof(pathType)-1, 0,
           "Name of the AreaMgr log file");
  addItem(logMenu, FILE_NAME, "Toss summary", 0, config.summaryLogName, sizeof(pathType)-1, 0,
           "Name of the Toss Summary log file");

  if ((internetMenu = createMenu(" Internet settings ")) == NULL)
    goto nomem;

  addItem(internetMenu, BOOL_INT, "Send smtp", 0, &config.inetOptions, BIT0, 0,
           "Send mail directly from FMail if possible");
  addItem(internetMenu, WORD, "Email address", 0, config.emailAddress, 56, 0,
           "Email address that will be used as the sender address");
  addItem(internetMenu, WORD, "SMTP server", 0, config.smtpServer, 56, 0,
           "SMTP server that FMail can use to send Internet mail");

  if ((mailMenu = createMenu(" Mail ")) == NULL)
    goto nomem;

  addItem(mailMenu, BOOL_INT, "Never use ARCmail", 0, &config.mailOptions, BIT3, 0,
           "Never use the ARCmail 0.60 naming convention");
  addItem(mailMenu, BOOL_INT, "Dupe detection        ", 28, &config.mailOptions, BIT8, 0,
           "Detect and remove duplicate messages");
  addItem(mailMenu, BOOL_INT, "ÈÍ> Out-of-zone", 0, &config.mailOptions, BIT10, 0,
           "Use ARCmail 0.60 naming convention for out-of-zone mail");
  addItem(mailMenu, BOOL_INT, "ÌÍ> Ignore MSGID      ", 28, &config.mailOptions, BIT9, 0,
           "Do not use MSGIDs for dupe detection");
  addItem(mailMenu, BOOL_INT, "Ext. bundle names", 0, &config.mailOptions, BIT11, 0,
           "Use 0-Z in stead of only 0-9 for mail bundle name extensions");
#ifdef __32BIT__
  addItem(mailMenu, NUM_INT, "ÈÍ> Dup recs (x1024)  ", 28, &config.kDupRecs, 4, 9999,
           "Number of messages x 1024 of which dup checking information is stored");
#else
#ifdef __FMAILX__
  if (xOS2)
    addItem(mailMenu, NUM_INT, "ÈÍ> Dup recs (x1024)  ", 28, &config.kDupRecs, 4, 9999,
             "Number of messages x 1024 of which dup checking information is stored");
  else
    addItem(mailMenu, DISPLAY, "ÈÍ> Use EMS           ", 28, &config.genOptions, BIT0, 0,
             "Use 64 kb of EMS memory to store message signatures in");
#else
  addItem(mailMenu, BOOL_INT, "ÈÍ> Use EMS           ", 28, &config.genOptions, BIT0, 0,
           "Use 64 kb of EMS memory to store message signatures in");
#endif
#endif
  addItem(mailMenu, BOOL_INT, "Check PKT dest"        ,  0, &config.mailOptions      , BIT2,  0, "Only toss PKT files that are addressed to one of your AKAs"                     );
  addItem(mailMenu, NUM_INT , "Max message size (kb) ", 28, &config.maxMsgSize       , 4,  9999, "Maximum size of a message in kilobytes (45-9999) / 32-bit versions only!"       );
  addItem(mailMenu, DISPLAY , NULL                    ,  0, NULL                     , 0,     0, NULL                                                                             );
  addItem(mailMenu, NUM_INT , "Old message    (days) ", 28, &config.oldMsgDays       , 4,  9999, "Msgs older then the specified number of days are tossed to the bad area (0=Off)");
  addItem(mailMenu, NUM_INT , "Max PKT size (kb)"     ,  0, &config.maxPktSize       , 4,  9999, "Maximum size of a PKT file in kilobytes (1-9999, 0 = no maximum)"               );
  addItem(mailMenu, NUM_INT , "Max net msgs          ", 28, &config.allowedNumNetmail, 4,  9999, "Max allowed number of net msgs in one packet when tossing (1-9999, 0 = no max)" );
  addItem(mailMenu, NUM_INT , "Max bundle size (kb)"  ,  0, &config.maxBundleSize    , 4,  9999, "Maximum size of a mail bundle in kilobytes (1-9999, 0 = no maximum)"            );
  addItem(mailMenu, BOOL_INT, "Keep exported netmail ", 28, &config.mailOptions      , BIT14, 0, "Keep exported .MSG netmail after they have been sent"                           );
  addItem(mailMenu, BOOL_INT, "Kill empty netmail"    ,  0, &config.mailOptions      , BIT15, 0, "Kill empty netmail messages"                                                    );
  addItem(mailMenu, BOOL_INT, "Remove netmail kludges", 28, &config.mailOptions      , BIT0,  0, "Remove INTL, FMPT and TOPT kludges from forwarded echomail messages"            );
  addItem(mailMenu, BOOL_INT, "Kill bad ARCmail msg"  ,  0, &config.mailOptions      , BIT7,  0, "Kill ARCmail messages of which the attached file is missing"                    );
  addItem(mailMenu, BOOL_INT, "Set Pvt on import     ", 28, &config.mailOptions      , BIT13, 0, "Set private status on all imported netmail messages"                            );
  addItem(mailMenu, BOOL_INT, "No point in PATH"      ,  0, &config.mailOptions      , BIT1,  0, "Do not add the node number of the boss to the PATH kludge on point systems"     );
  addItem(mailMenu, BOOL_INT, "Daily mailbundles     ", 28, &config.mailOptions      , BIT5,  0, "Create a new mail bundle every day"                                             );

  if ((mgrMenu = createMenu(" Mgr options ")) == NULL)
    goto nomem;

  addItem(mgrMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(mgrMenu, BOOL_INT, "Keep requests", 0, &config.mgrOptions, BIT0, 0,
           "Don't kill AreaMgr requests after they have been processed");
// addItem(mgrMenu, DISPLAY, "AutoPassive (days)", 26, NULL, 0, 0, NULL);
  addItem(mgrMenu, BOOL_INT, "Keep receipts", 0, &config.mgrOptions, BIT1, 0,
           "Don't kill AreaMgr receipts after they have been sent");
  addItem(mgrMenu, BOOL_INT, "Uplink Areas List", 0, &config.mgrOptions, BIT2, 0,
           "Also send list of areas available from Uplinks (BCL only) when %LIST cmd is used");
  addItem(mgrMenu, NUM_INT, "Def. rescan size", 0, &config.defMaxRescan, 4, 9999,
           "Default number of messages rescanned with rescan request (1-9999, 0 = no max)");
// addItem(mgrMenu, NUM_INT, " Nodes      ", 26, &config.adiscDaysNode, 4, 9999,
//          "Max number of days between polls before auto-passive (1-9999, 0 = no max)");
  addItem(mgrMenu, BOOL_INT, "Allow %PASSWORD", 0, &config.mgrOptions, BIT12, 0,
           "Allow nodes to use %PASSWORD to change their AreaMgr password");
// addItem(mgrMenu, NUM_INT, " Points     ", 26, &config.adiscDaysPoint, 4, 9999,
//          "Max number of days between polls before auto-passive (1-9999, 0 = no max)");
  addItem(mgrMenu, BOOL_INT, "Allow %PKTPWD", 0, &config.mgrOptions, BIT13, 0,
           "Allow nodes to use %PKTPWD to change their packet password");
// addItem(mgrMenu, DISPLAY, "AutoPassive (size)", 26, NULL, 0, 0, NULL);
  addItem(mgrMenu, BOOL_INT, "Allow %ACTIVE", 0, &config.mgrOptions, BIT10, 0,
           "Allow nodes to use %ACTIVE and %PASSIVE");
// addItem(mgrMenu, NUM_INT, " Nodes (kb) ", 26, &config.adiscSizeNode, 4, 9999,
//	    "Max total size of bundles before auto-passive (1-9999, 0 = no max)");
  addItem(mgrMenu, BOOL_INT, "Allow %COMPRESSION", 0, &config.mgrOptions, BIT15, 0,
           "Allow nodes to use %COMPRESSION to change the compression type");
// addItem(mgrMenu, NUM_INT, " Points (kb)", 26, &config.adiscSizePoint, 4, 9999,
//          "Max total size of bundles before auto-passive (1-9999, 0 = no max)");
  addItem(mgrMenu, BOOL_INT, "Allow %NOTIFY", 0, &config.mgrOptions, BIT14, 0,
           "Allow nodes to use %NOTIFY ON|OFF");
  addItem(mgrMenu, BOOL_INT, "Allow %AUTOBCL", 0, &config.mgrOptions, BIT11, 0,
           "Allow nodes to use %AUTOBCL");
  addItem(mgrMenu, BOOL_INT, "Allow %+ALL", 0, &config.mgrOptions, BIT9, 0,
           "Allow nodes to use %+ALL");
  addItem(mgrMenu, BOOL_INT, "Auto-disconnect", 0, &config.mgrOptions, BIT4, 0,
           "Automatically disconnect passthru areas with only one link");
/* addItem(mgrMenu, BOOL_INT, "ÈÍ> Remove record", 0, &config.mgrOptions, BIT5, 0,
  	    "Immediatly remove an auto-disconnected area from the Area Manager");
*/

  if ((pingMenu = createMenu(" Ping/trace options ")) == NULL)
    goto nomem;

  addItem(pingMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(pingMenu, BOOL_INT_REV, "Enable PING"           , 0, &config.pingOptions, BIT0, 0, "Enable the PING function");
  addItem(pingMenu, BOOL_INT_REV, "Enable TRACE"          , 0, &config.pingOptions, BIT1, 0, "Enable the TRACE function");
  addItem(pingMenu, BOOL_INT_REV, "Keep PING requests"    , 0, &config.pingOptions, BIT2, 0, "Keep a copy of PING request messages after processing");
//addItem(pingMenu, BOOL_INT_REV, "Keep TRACE requests"   , 0, &config.pingOptions, BIT3, 0, "Keep a copy of TRACE request messages after processing");
  addItem(pingMenu, BOOL_INT_REV, "Keep PING responses"   , 0, &config.pingOptions, BIT4, 0, "Keep a copy of PING response messages after they have been sent");
  addItem(pingMenu, BOOL_INT_REV, "Keep TRACE responses"  , 0, &config.pingOptions, BIT5, 0, "Keep a copy of TRACE response messages after they have been sent");

  if ((netMenu = createMenu(" Netmail boards ")) == NULL)
    goto nomem;

// addItem(netMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(netMenu, FUNC_VPAR, "Main", 0, netmailMenu, 0, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA  8", 9, netmailMenu, 8, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 16", 18, netmailMenu, 16, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 24", 27, netmailMenu, 24, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA  1", 0, netmailMenu, 1, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA  9", 9, netmailMenu, 9, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 17", 18, netmailMenu, 17, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 25", 27, netmailMenu, 25, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA  2", 0, netmailMenu, 2, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 10", 9, netmailMenu, 10, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 18", 18, netmailMenu, 18, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 26", 27, netmailMenu, 26, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA  3", 0, netmailMenu, 3, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 11", 9, netmailMenu, 11, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 19", 18, netmailMenu, 19, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 27", 27, netmailMenu, 27, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA  4", 0, netmailMenu, 4, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 12", 9, netmailMenu, 12, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 20", 18, netmailMenu, 20, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 28", 27, netmailMenu, 28, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA  5", 0, netmailMenu, 5, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 13", 9, netmailMenu, 13, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 21", 18, netmailMenu, 21, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 29", 27, netmailMenu, 29, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA  6", 0, netmailMenu, 6, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 14", 9, netmailMenu, 14, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 22", 18, netmailMenu, 22, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 30", 27, netmailMenu, 30, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA  7", 0, netmailMenu, 7, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 15", 9, netmailMenu, 15, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 23", 18, netmailMenu, 23, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");
  addItem(netMenu, FUNC_VPAR, "AKA 31", 27, netmailMenu, 31, 0,
           "Netmail board in the RemoteAccess/SuperBBS/QuickBBS/TAG message base");

  if ((mbMenu = createMenu(" Message base ")) == NULL)
    goto nomem;

  addItem(mbMenu, NEW_WINDOW, "Netmail boards"        ,  0, netMenu                 ,     2,    -2, "Netmail boards in the message base");
  addItem(mbMenu, BOOL_INT  , "Check dupe net boards ", 32, &config.mbOptions       , BIT13,     0, "Check if a netmail board is already used for another AKA when entering a board #");
  addItem(mbMenu, FUNC_PAR  , "Bad message board"     ,  0, &boardSelect[MAX_AKAS]  ,     4,     0, "Where the bad messages are stored");
  addItem(mbMenu, BOOL_INT  , "Imp. netmail to SysOp ", 32, &config.mbOptions       , BIT15,     0, "Import netmail messages to the SysOp into the message base");
  addItem(mbMenu, FUNC_PAR  , "Dupl. message board"   ,  0, &boardSelect[MAX_AKAS+1],     4,     0, "Where the duplicate messages are stored");
  addItem(mbMenu, BOOL_INT  , "Remove 'Re:'          ", 32, &config.mbOptions       , BIT6 ,     0, "Remove 'Re:' from subject when importing messages into the message base");
  addItem(mbMenu, FUNC_PAR  , "Recovery board"        ,  0, &boardSelect[MAX_AKAS+2],     4,     0, "Where messages in undefined boards are moved to");
  addItem(mbMenu, BOOL_INT  , "Remove soft cr        ", 32, &config.mbOptions       , BIT7 ,     0, "Remove soft cr's when exporting messages");
  addItem(mbMenu, NUM_INT   , "AutoRenumber"          ,  0, &config.autoRenumber    ,     5, 32767, "Highest msg number before FTools Maint will automatically renumber the msg base");
  addItem(mbMenu, BOOL_INT  , "Remove lf             ", 32, &config.mbOptions       , BIT5 ,     0, "Remove linefeeds when exporting messages");
  addItem(mbMenu, DISPLAY   , NULL                    ,  0, NULL                    ,     0,     0, NULL);
  addItem(mbMenu, BOOL_INT  , "Update text after Scan", 32, &config.mbOptions       , BIT9 ,     0, "Update the text of a scanned message in the message base with PATH and SEEN-BY");
  addItem(mbMenu, DISPLAY   , NULL                    ,  0, NULL                    ,     0,     0, NULL);
  addItem(mbMenu, BOOL_INT  , "Update reply chains"   ,  0, &config.mbOptions       , BIT2 ,     0, "Update reply chains after messages have been tossed into the message base");
  addItem(mbMenu, BOOL_INT  , "Message base sharing  ", 32, &config.mbOptions       , BIT10,     0, "Enable the message base locking system");
  addItem(mbMenu, BOOL_INT  , "Sort new messages"     ,  0, &config.mbOptions       , BIT0 ,     0, "Sort the messages that have been tossed into the message base");
  addItem(mbMenu, BOOL_INT  , "Scan always           ", 32, &config.mbOptions       , BIT8 ,     0, "Do not use ECHOMAIL.BBS or NETMAIL.BBS");
  addItem(mbMenu, BOOL_INT  , "ÈÍ> Use subject"       ,  0, &config.mbOptions       , BIT1 ,     0, "Apart from time/date also use the subject to sort");
  addItem(mbMenu, BOOL_INT  , "Quick toss            ", 32, &config.mbOptions       , BIT12,     0, "Do not update message base directory entries after tossing a packet");
  addItem(mbMenu, DISPLAY   , NULL                    ,  0, NULL                    ,     0,     0, NULL);
  addItem(mbMenu, FILE_NAME , "Tossed Areas List"     ,  0, config.tossedAreasList , sizeof(pathType)-3, 0, "List of echomail areas in which mail has been tossed");

  if ((groupMenu = createMenu(" Group names ")) == NULL)
    goto nomem;

  addItem(groupMenu, TEXT, "A", 0, config.groupDescr[0], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "N", 32, config.groupDescr[13], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "B", 0, config.groupDescr[1], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "O", 32, config.groupDescr[14], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "C", 0, config.groupDescr[2], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "P", 32, config.groupDescr[15], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "D", 0, config.groupDescr[3], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "Q", 32, config.groupDescr[16], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "E", 0, config.groupDescr[4], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "R", 32, config.groupDescr[17], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "F", 0, config.groupDescr[5], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "S", 32, config.groupDescr[18], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "G", 0, config.groupDescr[6], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "T", 32, config.groupDescr[19], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "H", 0, config.groupDescr[7], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "U", 32, config.groupDescr[20], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "I", 0, config.groupDescr[8], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "V", 32, config.groupDescr[21], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "J", 0, config.groupDescr[9], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "W", 32, config.groupDescr[22], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "K", 0, config.groupDescr[10], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "X", 32, config.groupDescr[23], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "L", 0, config.groupDescr[11], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "Y", 32, config.groupDescr[24], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "M", 0, config.groupDescr[12], 26, 0,
           "Description of a group of echomail conferences");
  addItem(groupMenu, TEXT, "Z", 32, config.groupDescr[25], 26, 0,
           "Description of a group of echomail conferences");

  if ((address1Menu = createMenu(" Addresses 1 ")) == NULL)
    goto nomem;

  addItem(address1Menu, NODE, "Main", 0, &config.akaList[0], FAKE, 0,
           "Main Address  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA  1", 0, &config.akaList[1], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA  2", 0, &config.akaList[2], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA  3", 0, &config.akaList[3], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA  4", 0, &config.akaList[4], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA  5", 0, &config.akaList[5], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA  6", 0, &config.akaList[6], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA  7", 0, &config.akaList[7], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA  8", 0, &config.akaList[8], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA  9", 0, &config.akaList[9], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA 10", 0, &config.akaList[10], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA 11", 0, &config.akaList[11], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA 12", 0, &config.akaList[12], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA 13", 0, &config.akaList[13], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA 14", 0, &config.akaList[14], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address1Menu, NODE, "AKA 15", 0, &config.akaList[15], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");

  if ((address2Menu = createMenu(" Addresses 2 ")) == NULL)
    goto nomem;

  addItem(address2Menu, NODE, "AKA 16", 0, &config.akaList[16], FAKE, 0,
           "Main Address  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 17", 0, &config.akaList[17], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 18", 0, &config.akaList[18], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 19", 0, &config.akaList[19], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 20", 0, &config.akaList[20], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 21", 0, &config.akaList[21], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 22", 0, &config.akaList[22], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 23", 0, &config.akaList[23], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 24", 0, &config.akaList[24], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 25", 0, &config.akaList[25], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 26", 0, &config.akaList[26], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 27", 0, &config.akaList[27], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 28", 0, &config.akaList[28], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 29", 0, &config.akaList[29], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 30", 0, &config.akaList[30], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");
  addItem(address2Menu, NODE, "AKA 31", 0, &config.akaList[31], FAKE, 0,
           "AKA  zone:net/node[.point][-fakenet]");

  if ((genMenu = createMenu(" General ")) == NULL)
    goto nomem;

  addItem(genMenu, TEXT, "SysOp name", 0, &config.sysopName, sizeof(config.sysopName)-1, 0, "Name of the SysOp");
  addItem(genMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(genMenu, ENUM_INT, "Mailer", 0, &mailerToggle, 0, 6, "Mailer used");
  addItem(genMenu, BOOL_INT, "ÈÍ> Busy flags", 0, &config.mailOptions, BIT4, 0
          , "Create busy flags when mail is being compressed");
  addItem(genMenu,
#ifdef GOLDBASE
           ENUM_INT|NO_EDIT,
#else
           ENUM_INT,
#endif
           "BBS program", 0, &bbsToggle, 0, 8,
           "BBS program program used (used by AutoExport)");
  /* addItem(genMenu, BOOL_INT, "ÈÍ> RA 2", 0, &config.genOptions, BIT15, 0,
              "If you are using RemoteAccess, are you using version 2.00 or higher?");
  */
  addItem(genMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(genMenu, ENUM_INT, "Tearline      ", 0, &tearToggle, 0, 5, "Tearline type");
  addItem(genMenu, TEXT  , "ÈÍ> Custom    ", 0, &config.tearLine, 24, 0, "Custom tearline");
  addItem(genMenu, BOOL_INT, "ReTear        ", 0, &config.mbOptions, BIT3, 0, "Replace an existing tear line");

  if ((userMenu = createMenu(" Users ")) == NULL)
    goto nomem;

  addItem(userMenu, TEXT, " 1 ", 0, config.users[0].userName, 35, 0, "Name of user");
  addItem(userMenu, TEXT, " 2 ", 0, config.users[1].userName, 35, 0, "Name of user");
  addItem(userMenu, TEXT, " 3 ", 0, config.users[2].userName, 35, 0, "Name of user");
  addItem(userMenu, TEXT, " 4 ", 0, config.users[3].userName, 35, 0, "Name of user");
  addItem(userMenu, TEXT, " 5 ", 0, config.users[4].userName, 35, 0, "Name of user");
  addItem(userMenu, TEXT, " 6 ", 0, config.users[5].userName, 35, 0, "Name of user");
  addItem(userMenu, TEXT, " 7 ", 0, config.users[6].userName, 35, 0, "Name of user");
  addItem(userMenu, TEXT, " 8 ", 0, config.users[7].userName, 35, 0, "Name of user");

  if ((pmailMenu = createMenu(" Personal mail ")) == NULL)
    goto nomem;

  addItem(pmailMenu, PATH, "Pers. mail path", 0, config.pmailPath, sizeof(pathType)-1, 0,
           "Where copies of echo messages directed to the SysOp will be stored (optional)");
  addItem(pmailMenu, TEXT|UPCASE, "ÌÍ> Topic 1", 0, config.topic1, 15, 0,
           "Scan subject line of messages for a topic");
  addItem(pmailMenu, TEXT|UPCASE, "ÈÍ> Topic 2", 0, config.topic2, 15, 0,
           "Scan subject line of messages for a topic");
  addItem(pmailMenu, BOOL_INT, "Include netmail", 0, &config.mailOptions, BIT12, 0,
           "Also scan netmail messages for personal mail");
  addItem(pmailMenu, BOOL_INT, "include Sent mail", 0, &config.mbOptions, BIT14, 0,
           "Include messages sent by the SysOp");
  addItem(pmailMenu, BOOL_INT, "New mail warning", 0, &config.mailOptions, BIT6, 0,
           "Let FMail send you a netmail message when new personal messages have arrived");
  addItem(pmailMenu, NEW_WINDOW, "Users", 0, userMenu, 2, 2,
           "Scan echo/netmail messages also for the names of these users");

  if ((sysMiscMenu = createMenu(" Miscellaneous ")) == NULL)
    goto nomem;

  addItem(sysMiscMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(sysMiscMenu, NUM_INT, "Max exports", 0, &config.maxForward, 3, 256,
           "The max number of export addresses that can be used in the Area Manager (>= 64)");
  addItem(sysMiscMenu, BOOL_INT, "FMail MT aware", 0, &config.genOptions, BIT13, 0,
           "Let FMail return time slices in Multi Tasking environments (SLOWS FMAIL DOWN!)");
  addItem(sysMiscMenu, BOOL_INT, "FTools MT aware", 0, &config.genOptions, BIT14, 0,
           "Let FTools return time slices in Multi Tasking environments (SLOWS FTOOLS DOWN!)");
  addItem(sysMiscMenu, NUM_INT, "Active time out", 0, &config.activTimeOut, 3, 999,
           "How long FMail should wait for another copy to finish, 0 = keep waiting");
  addItem(sysMiscMenu, BOOL_INT, "Control codes", 0, &config.genOptions, BIT12, 0,
           "Allow control codes (ASCII 1-26) in area descriptions (use at your own risk)");
  addItem(sysMiscMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(sysMiscMenu, BOOL_INT, "Monochrome", 0, &config.genOptions, BIT6, 0,
           "Do not use color even if a color card is detected (same as /M switch)");
  addItem(sysMiscMenu, ENUM_INT, "Color set", 0, &colorToggle, 0, 3,
           "Color set used by "FSNAME" (has no effect in monochrome mode)");

  if ((systemMenu = createMenu(" System info ")) == NULL)
    goto nomem;

  addItem(systemMenu, DISPLAY   , NULL           , 0, NULL        ,  0,  0, NULL);
  addItem(systemMenu, NEW_WINDOW, "Miscellaneous", 0, sysMiscMenu ,  2, -2, "File handles, colors, buffer sizes");
  addItem(systemMenu, NEW_WINDOW, "Directories"  , 0, pathMenu    ,  2,  0, "Various path and file names");
  addItem(systemMenu, NEW_WINDOW, "Log files"    , 0, logMenu     ,  0, -3, "Log file settings");
  addItem(systemMenu, NEW_WINDOW, "Internet"     , 0, internetMenu, -4,  4, "Internet mail settings");
  addItem(systemMenu, DISPLAY   , NULL           , 0, NULL        ,  0,  0, NULL);
  addItem(systemMenu, NEW_WINDOW, "Compression"  , 0, arc32Menu   ,  2, -2, "Programs and switches used to compress mailbundles");
  addItem(systemMenu, NEW_WINDOW, "Decompression", 0, deArc32Menu ,  2, -1, "Programs and switches used to decompress mailbundles");

  memset (uplinkNodeStr, 0, sizeof(uplinkNodeStrType));
  for ( count = 0; count < MAX_UPLREQ; count++ )
  {
    sprintf (uplinkNodeStr[count], "%2u  %s", count+1,
             config.uplinkReq[count].node.zone?nodeStr(&config.uplinkReq[count].node):"                       ");
  }
  if ((uplMenu = createMenu(" Uplink Manager ")) == NULL)
    goto nomem;

  for ( count = 0; count < MAX_UPLREQ/2; count++ )
  {
    addItem(uplMenu, FUNC_VPAR, uplinkNodeStr[count], 0, uplinkMenu, count, 0,
             " - Uplink system -");
    addItem(uplMenu, FUNC_VPAR, uplinkNodeStr[count+(MAX_UPLREQ/2)], 30, uplinkMenu, count+(MAX_UPLREQ/2), 0,
             " - Uplink system -");
  }
  if ((miscMenu = createMenu(" Miscellaneous ")) == NULL)
    goto nomem;

  addItem(miscMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(miscMenu, NEW_WINDOW, "General options", 0, genMenu, 2, -1,
           "General options");
  addItem(miscMenu, NEW_WINDOW, "Message base", 0, mbMenu, 2, 0,
           "Message base related options");
  addItem(miscMenu, NEW_WINDOW, "Mail options", 0, mailMenu, 2, 2,
           "Mailflow related options");
  addItem(miscMenu, NEW_WINDOW, "Mgr options", 0, mgrMenu, 2, -1,
           "Mgr related options");
  addItem(miscMenu, NEW_WINDOW, "Ping/trace options", 0, pingMenu, 2, -1,
           "Ping/trace related options");
  addItem(miscMenu, NEW_WINDOW, "Personal mail", 0, pmailMenu, 2, 2,
           "Personal mail settings");
  addItem(miscMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(miscMenu, NEW_WINDOW, "Addresses 1", 0, address1Menu, 2, -4,
           "Main address and AKA's 1-15");
  addItem(miscMenu, NEW_WINDOW, "Addresses 2", 0, address2Menu, 2, -4,
           "AKA's 16-31");
  addItem(miscMenu, NEW_WINDOW, "Group names", 0, groupMenu, 2, -1,
           "Description of groups of echomail conferences");
  addItem(miscMenu, FUNCTION, "AreaMgr defaults", 0, defaultBBS, 0, 0,
           "Default settings used for new areas: origin line, message base maintenance");

  if ((mainMenu = createMenu(" Main ")) == NULL)
  {
nomem:
    free(autoExpMenu);
    displayMessage ("Not enough memory available");
    deInit(5);
    if (fmailLocHandle != -1)
      close(fmailLocHandle);
    return 1;
  }
  addItem(mainMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
  addItem(mainMenu, NEW_WINDOW, "Miscellaneous", 0, miscMenu, 2, 2,
           "Miscellaneous options");
  addItem(mainMenu, NEW_WINDOW, "System info", 0, systemMenu, 2, 2,
           "System related options");
  addItem(mainMenu, NEW_WINDOW, "Import/export", 0, impExpMenu, 2, -1,
           "Import or export configuration information");
  /*   addItem(mainMenu, NEW_WINDOW, "AKA matching", 0, akaMatchMenu, 2, -2,
  		      "AKA matching");*/
  addItem(mainMenu, NEW_WINDOW, "Uplink manager", 0, uplMenu, 2, -2,
           "Uplink manager configuration");
  addItem(mainMenu, FUNCTION, "Pack manager", 0, packMgr, 0, 0,
           "Netmail routing configuration");
  addItem(mainMenu, FUNCTION, "Node manager", 0, nodeMgr, 0, 0,
           "Connected systems configuration");
  addItem(mainMenu, FUNCTION, "Area manager", 0, areaMgr, 0, 0,
           "Area (conference) configuration");
  addItem(mainMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);

  do
  {
    update |= runMenu(mainMenu, 3, 6);
    if (update)
      ch = askBoolean("Save changes ?", 'Y');
    else
      ch = 'N';
  }
  while (ch != 'Y' && ch != 'N');

  working();

  if (defaultEnumRA)
    echoDefaultsRec.attrRA |= BIT3;
  else
    echoDefaultsRec.attrRA &= ~BIT3;
  if (defaultEnumRA == 1)
    echoDefaultsRec.attrRA |= BIT5;
  else
    echoDefaultsRec.attrRA &= ~BIT5;

  if (ch == 'Y')
  {
    if (((configHandle = open(configFileName, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) == -1) ||
        (write(configHandle, &config, sizeof (configType)) != sizeof (configType)) ||
        (close(configHandle) == -1))
      displayMessage ("Can't write to "dCFGFNAME);

    if (*config.autoFMail102Path)
    {
      if (!stricmp(config.autoFMail102Path, configPath))
        displayMessage("AutoExport for FMail 1.02 format should be set to another directory");
      else if ((configHandle = open(strcat(strcpy(tempStr, config.autoFMail102Path), dCFGFNAME), O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) != -1 )
      {
        memcpy(config._optionsAKA,        config.optionsAKA,        2*MAX_NA_OLD);
        memcpy(config._groupsQBBS,        config.groupsQBBS,          MAX_NA_OLD);
        memcpy(config._templateSecQBBS,   config.templateSecQBBS,   2*MAX_NA_OLD);
        memcpy(config._templateFlagsQBBS, config.templateFlagsQBBS, 4*MAX_NA_OLD);
        memcpy(config._attr2RA,           config.attr2RA,             MAX_NA_OLD);
        memcpy(config._aliasesQBBS,       config.aliasesQBBS,         MAX_NA_OLD);
        memcpy(config._groupRA,           config.groupRA,             MAX_NA_OLD);
        memcpy(config._altGroupRA,        config.altGroupRA,      2*3*MAX_NA_OLD);
        memcpy(config._qwkName,           config.qwkName,          13*MAX_NA_OLD);
        memcpy(config._minAgeSBBS,        config.minAgeSBBS,        2*MAX_NA_OLD);
        memcpy(config._daysRcvdAKA,       config.daysRcvdAKA,       2*MAX_NA_OLD);
        memcpy(config._replyStatSBBS,     config.replyStatSBBS,       MAX_NA_OLD);
        memcpy(config._attrSBBS,          config.attrSBBS,          2*MAX_NA_OLD);
        memcpy(config._msgKindsRA,        config.msgKindsRA,          MAX_NA_OLD);
        memcpy(config._attrRA,            config.attrRA,              MAX_NA_OLD);
        memcpy(config._readSecRA,         config.readSecRA,         2*MAX_NA_OLD);
//          memcpy(config._readFlagsRA,       config.readFlagsRA,       4*MAX_NA_OLD);
        memcpy(config._writeSecRA,        config.writeSecRA,        2*MAX_NA_OLD);
//          memcpy(config._writeFlagsRA,      config.writeFlagsRA,      4*MAX_NA_OLD);
        memcpy(config._sysopSecRA,        config.sysopSecRA,        2*MAX_NA_OLD);
//          memcpy(config._sysopFlagsRA,      config.sysopFlagsRA,      4*MAX_NA_OLD);
        memcpy(config._daysAKA,           config.daysAKA,           2*MAX_NA_OLD);
        memcpy(config._msgsAKA,           config.msgsAKA,           2*MAX_NA_OLD);
        memcpy(config._descrAKA,          config.descrAKA,         51*MAX_NA_OLD);
        memcpy(config._netmailBoard,      config.netmailBoard,      2*MAX_NA_OLD);
        memcpy(config._akaList,           config.akaList,   sizeof(_akaListType));
        for ( count = 0; count < MAX_UPLREQ; count++ )
        {
          if ( config.uplinkReq[count].originAka > 15 )
            config.uplinkReq[count].originAka = 0;
        }

        write(configHandle, &config, 8192);
        close(configHandle);
      }
    }
    if (!openConfig(CFG_AREADEF, &adefHeader, (void*)&adefBuf))
    {
      displayMessage ("Can't write to "dARDFNAME);
    }
    else
    {
      memcpy (adefBuf, &echoDefaultsRec, RAWECHO_SIZE);
      putRec(CFG_AREADEF, 0);
      chgNumRec(CFG_AREADEF, 1);
      closeConfig(CFG_AREADEF);
    }
  }
  autoUpdate();

  deInit(5);

  if (mainMenu    ) free(mainMenu    );
  if (arc32Menu   ) free(arc32Menu   );
  if (deArc32Menu ) free(deArc32Menu );
  if (sysMiscMenu ) free(sysMiscMenu );
  if (systemMenu  ) free(systemMenu  );
  if (logMenu     ) free(logMenu     );
  if (miscMenu    ) free(miscMenu    );
  if (userMenu    ) free(userMenu    );
  if (pmailMenu   ) free(pmailMenu   );
  if (pathMenu    ) free(pathMenu    );
  if (mailMenu    ) free(mailMenu    );
  if (mbMenu      ) free(mbMenu      );
  if (internetMenu) free(internetMenu);
  if (mgrMenu     ) free(mgrMenu     );
  if (pingMenu    ) free(pingMenu    );
  if (netMenu     ) free(netMenu     );
  if (address1Menu) free(address1Menu);
  if (address2Menu) free(address2Menu);
  if (uplMenu     ) free(uplMenu     );
  if (groupMenu   ) free(groupMenu   );
  if (genMenu     ) free(genMenu     );
  if (warningMenu ) free(warningMenu );
  if (autoExpMenu ) free(autoExpMenu );
  if (anImpMenu   ) free(anImpMenu   );
  if (impExpMenu  ) free(impExpMenu  );

  if (fmailLocHandle != -1)
    close(fmailLocHandle);

  return 0;
}
//---------------------------------------------------------------------------

