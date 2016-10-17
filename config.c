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
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#if defined _Windows && !defined __DPMI16__
#define _far
#define far
#endif

#ifdef __FMAILX__
unsigned long _far _pascal GlobalDosAlloc(unsigned long);
unsigned int  _far _pascal GlobalDosFree(unsigned long);
#ifdef __DPMI32__
extern unsigned _cdecl _psp;
#define MK_FP(seg,ofs) ((void *)((long)(seg)<<16) + (ofs))
#endif
#endif

#include "config.h"

#include "areainfo.h"
#include "crc.h"
#include "log.h"
//#include "msgpkt.h" // for openP
#include "mtask.h"
#include "stpcpy.h"
#include "utils.h"

extern fhandle fmailLockHandle;

// Borland C number of files
extern unsigned int _nfile;

extern time_t	startTime;

// Global available datastructures
configType config;
s16        zero = 0;
char funcStr[32] = "undefined?";

extern s16 diskError;

extern char configPath[FILENAME_MAX];
extern internalMsgType *message;

extern psRecType *seenByArray;
extern psRecType *tinySeenArray;
extern psRecType *pathArray;
extern psRecType *tinyPathArray;

fhandle fmLockHandle;

s16 breakPressed = 0;

#ifdef __STDIO__
#define keyWaiting (kbhit()?getch():0)
#else
#define keyWaiting bioskey(1)
#define keyRead    bioskey(0)
#endif

#if !defined __STDIO__ && !defined __32BIT__

//---------------------------------------------------------------------------
s16 cdecl c_break(void)
{
	printString ("\n+++ Ctrl-break +++\n");
	breakPressed = 1;

  return 1;
}
//---------------------------------------------------------------------------

#ifndef __FMAILX__
static unsigned char handle_table[TABLE_SIZE];
#endif
static unsigned char far * new_handle_tableptrR;     /* table of file DOS handles */
static unsigned char far * new_handle_tableptrP;     /* table of file DOS handles */
static unsigned char far * old_handle_tableptrR;     /* table of file DOS handles */
static unsigned char far * old_handle_tableptrP;     /* table of file DOS handles */

static unsigned char far * far * PSP_handle_ptr;     /* ptr to DOS's ptr to hand. */
static u16           far *       PSP_handle_count;   /* ptr to handle count       */

//---------------------------------------------------------------------------
void handles40(void)
{
#ifdef __FMAILX__
  u32 result;
#endif

   if (config.extraHandles)
   {
      PSP_handle_count = MK_FP(_psp, 0x32);     /* handle count at PSP:32h  */
      PSP_handle_ptr   = MK_FP(_psp, 0x34);     /* table ptr at PSP:34h     */

      old_handle_tableptrR = *PSP_handle_ptr;

#ifndef __FMAILX__
      old_handle_tableptrP = old_handle_tableptrR;
      new_handle_tableptrP = new_handle_tableptrR = handle_table;
#else
      old_handle_tableptrP = MK_FP(_psp, 0x18);
      if ((result = GlobalDosAlloc(TABLE_SIZE)) == 0)
         exit(1000);
      new_handle_tableptrP = MK_FP((u16)result, 0);
      new_handle_tableptrR = MK_FP((u16)(result >> 16), 0);
#endif
      memset (new_handle_tableptrP,
              255, TABLE_SIZE);                 /* init table               */
      memcpy (new_handle_tableptrP,
              old_handle_tableptrP,
              *PSP_handle_count);               /* relocate existing table  */
      *PSP_handle_ptr = new_handle_tableptrR;   /* set pointer to new table */
      *PSP_handle_count = _nfile =
         20 + min(235, config.extraHandles);    /* set new table size       */
   }
}
//---------------------------------------------------------------------------
void handles20(void)
{
  if (config.extraHandles)
  {
    memcpy(old_handle_tableptrP, new_handle_tableptrP, 20);        /* relocate existing table  */
    *PSP_handle_count = _nfile = 20;          /* set old table size       */
    *PSP_handle_ptr   = old_handle_tableptrR; /* set pointer to old table */
  }
}
//---------------------------------------------------------------------------
void handlesReset40 (void)
{
  if (config.extraHandles)
  {
    memcpy(new_handle_tableptrP, old_handle_tableptrP, 20);          // relocate existing table
    *PSP_handle_ptr   = new_handle_tableptrR;                        // set pointer to new table
    *PSP_handle_count = _nfile = 20 + min(235, config.extraHandles); // set new table size
  }
}

#endif // __32BIT__
//---------------------------------------------------------------------------
s16 getNetmailBoard (nodeNumType *akaNode)
{
  s16 count = MAX_NETAKAS;

  while (  count >= 0
        && (memcmp(&config.akaList[count].nodeNum, akaNode, sizeof(nodeNumType)) != 0)
        )
    count--;

  if (count < 0 || config.netmailBoard[count] == 0)
    return -1;

  return config.netmailBoard[count];
}
//---------------------------------------------------------------------------
s16 isNetmailBoard (u16 board)
{
  s16 count = MAX_NETAKAS;

  while (count >= 0 && config.netmailBoard[count] != board)
    count--;

  return count != -1;
}
//---------------------------------------------------------------------------
void initFMail(const char *_funcStr, s32 switches)
{
  s16         ch;
  fhandle     configHandle;
  time_t      time1
            , time2
            , time2a;
  tempStrType tempStr
            , tempStr2;
  char       *helpPtr;
  int         readlen = -1;

  strcpy(funcStr, _funcStr);
  strcpy(stpcpy(tempStr, configPath), dCFGFNAME);

  if ( (configHandle = open(tempStr, O_RDONLY | O_BINARY)) == -1
     || (readlen = read(configHandle, &config, sizeof(configType))) < (int)sizeof(configType)
     || close(configHandle) == -1
     )
  {
    printf("Can't read: %s\n", tempStr);
    exit(4);
  }
#if !defined __STDIO__ && !defined __32BIT__
  if (config.genOptions.checkBreak)
    ctrlbrk (c_break);              // Set control-break handler
#endif
  if (config.versionMajor != CONFIG_MAJOR || config.versionMinor != CONFIG_MINOR)
  {
    printf("ERROR: "dCFGFNAME" is not created by FSetup %d.%d or later.\n", CONFIG_MAJOR, CONFIG_MINOR);
    if ((config.versionMajor == 0) && (config.versionMinor == 36))
      puts("Running FSetup will update the config files.");

    exit(4);
  }

  strcpy(tempStr2, strcpy(tempStr, config.bbsPath));
  strcat(tempStr, "FMAIL.LOC");
  if ((helpPtr = strrchr(tempStr2, '\\')) != NULL)
    *helpPtr = 0;

  if (  !access(tempStr2, 0)
     && (fmailLockHandle = _sopen(tempStr, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, S_IREAD | S_IWRITE)) == -1
     && errno != ENOFILE
     )
  {
    puts("Waiting for another copy of FMail, FTools or FSetup to finish...");

    time(&time1);
    time2 = time1;

    while (  (fmailLockHandle = _sopen(tempStr, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, SH_DENYRW, S_IREAD | S_IWRITE)) == -1
          && (!config.activTimeOut || time2 - time1 < config.activTimeOut)
          && (ch = (keyWaiting & 0xff)) != 27
          )
    {
      if (ch == 0 || ch == -1)
      {
        time2a = time2+1;
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
    if (fmailLockHandle == -1)
    {
      puts("\nAnother copy of FMail, FTools or FSetup did not finish in time...\n\nExiting...");
      exit(4);
    }
  }

  if (config.akaList[0].nodeNum.zone == 0)
  {
    puts("Main nodenumber not defined in FSetup!");
    exit(4);
  }

  if ((*config.netPath == 0) ||
    (*config.bbsPath == 0) ||
    (*config.inPath  == 0) ||
    (*config.outPath == 0))
  {
    puts("Not all required subdirectories are defined in FSetup!");
    exit(4);
  }

  if (  (existDir(config.netPath, "netmail"     ) == 0)
     || (existDir(config.bbsPath, "message base") == 0)
     || (existDir(config.inPath,  "inbound"     ) == 0)
     || (existDir(config.outPath, "outbound"    ) == 0)
#if !defined(__FMAILX__) && !defined(__32BIT__)
     || (*config.swapPath            && existDir(config.swapPath           , "swap"         ) == 0)
#endif
     || (*config.pmailPath           && existDir(config.pmailPath          , "personal mail") == 0)
     || (*config.sentEchoPath        && existDir(config.sentEchoPath       , "sent echomail") == 0)
     || (*config.autoGoldEdAreasPath && existDir(config.autoGoldEdAreasPath, "AREAS.GLD"    ) == 0)
     )
  {
    puts("Please enter the required subdirectories first!");
    exit(4);
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

  initLog(funcStr, switches);

  if (  NULL == (message       = (internalMsgType *)malloc(INTMSG_SIZE                          ))
     || NULL == (seenByArray   = (psRecType       *)malloc(sizeof(psRecType) * MAX_MSGSEENBY    ))
     || NULL == (tinySeenArray = (psRecType       *)malloc(sizeof(psRecType) * MAX_MSGTINYSEENBY))
     || NULL == (pathArray     = (psRecType       *)malloc(sizeof(psRecType) * MAX_MSGPATH      ))
     || NULL == (tinyPathArray = (psRecType       *)malloc(sizeof(psRecType) * MAX_MSGTINYPATH  ))
     )
    logEntry("Not enough memory to initialize FMail", LOG_ALWAYS, 2);

#if !defined(__FMAILX__) && !defined(__32BIT__)
  if (!(*config.swapPath))
    strcpy(config.swapPath, configPath);
#endif
  strupr(config.topic1);
  strupr(config.topic2);
#ifndef __STDIO__
  handles40(); // Only effective if necessary
#endif

  puts("\n");
}
//---------------------------------------------------------------------------
void deinitFMail(void)
{
  fhandle     configHandle;
  tempStrType tempStr;

  strcpy(stpcpy(tempStr, configPath), dCFGFNAME);

  if (  (configHandle = open(tempStr, O_WRONLY | O_BINARY)) == -1
     || lseek(configHandle, offsetof(configType, uplinkReq), SEEK_SET) == -1L
     || write(configHandle, &config.uplinkReq, sizeof(uplinkReqType)*MAX_UPLREQ) < (int)sizeof(uplinkReqType)*MAX_UPLREQ
     || lseek(configHandle, offsetof(configType, lastUniqueID), SEEK_SET) == -1L
     || write(configHandle, &config.lastUniqueID, sizeof(config.lastUniqueID)) < (int)sizeof(config.lastUniqueID)
     || close(configHandle) == -1
     )
  {
    if (configHandle != -1)
      close(configHandle);
    logEntry("Can't write "dCFGFNAME, LOG_ALWAYS, 0);
  }
  freeNull(message      );
  freeNull(seenByArray  );
  freeNull(tinySeenArray);
  freeNull(pathArray    );
  freeNull(tinyPathArray);
}
//---------------------------------------------------------------------------