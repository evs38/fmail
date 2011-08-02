/*
 *  Copyright (C) 2007 Folkert J. Wijnstra
 *
 *
 *  This file is part of FMail.
 *
 *  FMail is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  FMail is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <io.h>
#include <dir.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#ifdef __STDIO__
#include <conio.h>
#else
#include <bios.h>
#endif
#include <errno.h>
#include <ctype.h>
#include <stddef.h>

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

#include "fmail.h"
#include "config.h"
#include "areainfo.h"
#include "log.h"
#include "output.h"
#include "utils.h"
#include "mtask.h"
#include "crc.h"
#include "msgpkt.h" /* for openP */
#include "keyfile.h"

extern   fhandle fmailLockHandle;

/* Borland C number of files */

extern   unsigned int _nfile;

extern	time_t	startTime;

/* Global available datastructures */

configType config;
s16        zero = 0;
s16        registered = 0;

extern s16 diskError;

extern char 	       configPath[128];
extern internalMsgType *message;

extern psType *seenByArray;
extern psType *tinySeenArray;
extern psType *pathArray;

fhandle fmLockHandle;



s16 breakPressed = 0;


#ifdef __STDIO__
#define keyWaiting (kbhit()?getch():0)
#else
#define keyWaiting bioskey(1)
#define keyRead    bioskey(0)
#endif


#if !defined __STDIO__ && !defined __32BIT__

s16 cdecl c_break (void)
{
	printString ("\n+++ Ctrl-break +++\n");
	breakPressed = 1;
        return 1;
}


#ifndef __FMAILX__
static unsigned char handle_table[TABLE_SIZE];
#endif
static unsigned char far * new_handle_tableptrR;     /* table of file DOS handles */
static unsigned char far * new_handle_tableptrP;     /* table of file DOS handles */
static unsigned char far * old_handle_tableptrR;     /* table of file DOS handles */
static unsigned char far * old_handle_tableptrP;     /* table of file DOS handles */

static unsigned char far * far * PSP_handle_ptr;     /* ptr to DOS's ptr to hand. */
static u16           far *       PSP_handle_count;   /* ptr to handle count       */


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



void handles20(void)
{
   if (config.extraHandles)
   {
      memcpy (old_handle_tableptrP,
              new_handle_tableptrP, 20);        /* relocate existing table  */
      *PSP_handle_count = _nfile = 20;          /* set old table size       */
      *PSP_handle_ptr   = old_handle_tableptrR; /* set pointer to old table */
   }
}



void handlesReset40 (void)
{
   if (config.extraHandles)
   {
      memcpy (new_handle_tableptrP,
				  old_handle_tableptrP, 20);        /* relocate existing table  */
      *PSP_handle_ptr   = new_handle_tableptrR; /* set pointer to new table */
      *PSP_handle_count = _nfile =
         20 + min(235, config.extraHandles);    /* set new table size       */
   }
}

#endif /* __32BIT__ */


#define N 65339L

#ifdef BETA

static u16 checkKeyBeta(void)
{
   u16      keyResult;
   u32      key;
   u32      tempKey;
   u16      count;

//#pragma message ("Controleer bij aanpassen key systeem!")
   if ( config.key && !((keyResult = (u16)(config.key & 0xffffL)) & 0x4000) )
   {
      diskError = DERR_HACKER;
   }

   key = tempKey = (config.key >> 16);

   for (count = 1; count < 17; count++)
	{
      key *= tempKey;
      key %= N;
   }

   tempKey = crc32old (config.sysopName);

   tempKey ^= crc32old (nodeStr(&config.akaList[0].nodeNum));

   tempKey ^= (tempKey >> 16);
   tempKey &= 0x0000ffff;

   tempKey ^= keyResult;
   tempKey %= N;

   if ((!(keyResult & 0x4000)) || (key != tempKey))
   {
#ifdef BETA
      setAttr (LIGHTRED, BLACK, MONO_HIGH);
      logEntry ("Beta registration key is missing or invalid", LOG_ALWAYS, 100);
#endif
      return 0;
   }
   return ++registered;
}

#endif


s16 getNetmailBoard (nodeNumType *akaNode)
{
   s16 count = MAX_NETAKAS;

   while ((count >= 0) &&
	  (memcmp (&config.akaList[count].nodeNum, akaNode,
		   sizeof(nodeNumType)) != 0))
   {
      count--;
   }
   if ((count < 0) || (config.netmailBoard[count] == 0))
   {
      return -1;
   }
   return config.netmailBoard[count];
}



s16 isNetmailBoard (u16 board)
{
   s16 count = MAX_NETAKAS;

   while ((count >= 0) &&
          (config.netmailBoard[count] != board))
   {
      count--;
   }
   return count != -1;
}



void initFMail (char *s, s32 switches)
{
   s16     ch;
   fhandle configHandle;
   time_t  time1, time2, time2a;
	tempStrType tempStr, tempStr2;
   char	  *helpPtr;

   strcpy (tempStr, configPath);
   strcat (tempStr, "FMail.CFG");

   if (((configHandle = openP(tempStr, O_RDONLY|O_BINARY|O_DENYNONE,S_IREAD|S_IWRITE)) == -1) ||
       (_read (configHandle, &config, sizeof(configType)) < sizeof(configType))  ||
       (close(configHandle) == -1))
	{
      printString ("Can't read FMail.CFG\n");
      showCursor ();
      exit (4);
   }
#if !defined __STDIO__ && !defined __32BIT__
   if (config.genOptions.checkBreak)
   {
      ctrlbrk (c_break); /* Set control-break handler */
   }
#endif
   if ((config.versionMajor != CONFIG_MAJOR) ||
       (config.versionMinor != CONFIG_MINOR))
   {
      printString ("ERROR: FMail.CFG is not created by FSetup ");
      printInt (CONFIG_MAJOR);
      printChar ('.');
      printInt (CONFIG_MINOR);
      printString (" or later.\n");
      if ((config.versionMajor == 0) && (config.versionMinor == 36))
      {
         printString ("Running FSetup will update the config files.\n");
      }
      showCursor ();
      exit (4);
   }

   strcpy(tempStr2, strcpy(tempStr, config.bbsPath));
   strcat (tempStr, "FMAIL.LOC");
   if ((helpPtr = strrchr(tempStr2, '\\')) != NULL)
      *helpPtr = 0;

#undef open(a,b,c)

   ++no_msg;
   if  ((!access(tempStr2, 0)) &&
        ((fmailLockHandle = open(tempStr,
                               O_WRONLY|O_DENYWRITE|O_BINARY|O_CREAT|O_TRUNC,
			       S_IREAD|S_IWRITE)) == -1) &&
	(errno != ENOFILE))
   {
      printString ("Waiting for another copy of FMail, FTools or FSetup to finish...\n");

      time (&time1);
      time2 = time1;

      ch = 0;
      while (((fmailLockHandle = open(tempStr,
				      O_WRONLY|O_DENYALL|O_BINARY|O_CREAT|O_TRUNC,
				      S_IREAD|S_IWRITE)) == -1) &&
		  (!config.activTimeOut || time2-time1 < config.activTimeOut) && ((ch = (keyWaiting & 0xff)) != 27))
      {
	 if (ch == 0 || ch == -1)
	 {  time2a = time2+1;
	    while ( time(&time2) <= time2a )
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
         printString ("\nAnother copy of FMail, FTools or FSetup did not finish in time...\n\nExiting...\n");
	 showCursor ();
	 exit (4);
      }
   }
   no_msg = 0;

#define open(a,b,c)  openP(a,b,c)

   if (config.akaList[0].nodeNum.zone == 0)
   {
      printString("Main nodenumber not defined in FSetup!\n");
      showCursor();
      exit(4);
   }

   if ((*config.netPath == 0) ||
      (*config.bbsPath == 0) ||
      (*config.inPath  == 0) ||
      (*config.outPath == 0))
   {
      printString("Not all required subdirectories are defined in FSetup!\n");
      showCursor();
      exit(4);
   }

   if ( (existDir(config.netPath, "netmail"     ) == 0) ||
        (existDir(config.bbsPath, "message base") == 0) ||
        (existDir(config.inPath,  "inbound"     ) == 0) ||
        (existDir(config.outPath, "outbound"    ) == 0) ||
#ifndef __32BIT__
#ifndef __FMAILX__
        ((*config.swapPath) && (existDir (config.swapPath, "swap") == 0))  ||
#endif
#endif
        ((*config.pmailPath)&& (existDir (config.pmailPath, "personal mail") == 0)) ||
        ((*config.sentEchoPath)&& (existDir (config.sentEchoPath, "sent echomail") == 0)) ||
/*      ((*config.autoAreasBBSPath) && (existDir (config.autoAreasBBSPath, "AREAS.BBS") == 0)   ) || */
        ((*config.autoGoldEdAreasPath) && (existDir (config.autoGoldEdAreasPath, "AREAS.GLD") == 0)))
   {
      printString("Please enter the required subdirectories first!\n");
      showCursor();
      exit(4);
   }

   if ( config.maxForward < 64 )
      config.maxForward = 64;
   if ( config.maxMsgSize < 45 )
      config.maxMsgSize = 45;
   if ( config.recBoard > MBBOARDS )
      config.recBoard = 0;
   if ( config.badBoard > MBBOARDS )
      config.badBoard = 0;
   if ( config.dupBoard > MBBOARDS )
      config.dupBoard = 0;

#ifdef GOLDBASE
   config.bbsProgram = BBS_QBBS;
#endif

   initLog (s, switches);

   if (((message       = malloc(INTMSG_SIZE)) == NULL)      ||
       ((seenByArray   = malloc(sizeof(psRecType) * MAX_MSGSEENBY)) == NULL) ||
       ((tinySeenArray = malloc(sizeof(psRecType) * MAX_MSGTINY)) == NULL)   ||
       ((pathArray     = malloc(sizeof(psRecType) * MAX_MSGPATH)) == NULL) )
   {
      logEntry ("Not enough memory to initialize FMail", LOG_ALWAYS, 2);
   }

#ifndef __32BIT__
#ifndef __FMAILX__
   if (!(*config.swapPath))
   {
      strcpy (config.swapPath, configPath);
   }
#endif
#endif
   strupr (config.topic1);
   strupr (config.topic2);
#ifndef __STDIO__
   handles40(); /* Only effective if necessary */
#endif

   newLine();

#ifndef BETA
   registered = keyFileInit();
#else
   registered = checkKeyBeta();
   !!!
#endif

   if ( registered )
   {  setAttr (LIGHTGREEN, BLACK, MONO_HIGH);
      printString ("Registered to ");
      printString (config.sysopName);
      setAttr (LIGHTGRAY, BLACK, MONO_NORM);
   }
   else
   {  if ( config.key )
      {  setAttr (LIGHTRED, BLACK, MONO_HIGH);
         logEntry ("Registration keys are invalid", LOG_ALWAYS, 100);
      }
      setAttr (LIGHTGREEN, BLACK, MONO_HIGH);
      printString ("Unregistered version");
      setAttr (LIGHTGRAY, BLACK, MONO_NORM);
   }
   newLine ();
   newLine ();
}


void deinitFMail(void)
{
   fhandle     configHandle;
   tempStrType tempStr;
   u16         counter;

   strcpy (tempStr, configPath);
   strcat (tempStr, "FMail.CFG");


   if ( !(startTime & 0x001F) &&
        ((key.relKey1 == 2103461921L && key.relKey2 == 479359711L)  ||
         (key.relKey1 == 2146266905L && key.relKey2 == 579690730L)  ||
         (key.relKey1 == 1909407035L && key.relKey2 == 740316073L)  ||
         (key.relKey1 == 659038778L  && key.relKey2 == 412971433L)  ||
         (key.relKey1 == 1039097288L && key.relKey2 == 1077836572L) ||
         (key.relKey1 == 1381959498L && key.relKey2 == 298979948L)  ||
         (key.relKey1 == 1056834494L && key.relKey2 == 972832725L)  ||
         (key.relKey1 == 1248593969L && key.relKey2 == 727542166L)  ||
         (key.relKey1 == 1022716332L && key.relKey2 == 2120709631L) ||
         (key.relKey1 == 208164969L  && key.relKey2 == 1209153882L) ||
         (key.relKey1 == 69886344L   && key.relKey2 == 1407278542L) ||
         (key.relKey1 == 1909407035L && key.relKey2 == 740316073L)) )
   {
      if ( (configHandle = openP(tempStr, O_WRONLY|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) == -1 ||
           lseek(configHandle, 0, SEEK_SET) == -1L )
      {
         close(configHandle);
         logEntry("Can't write FMail.CFG", LOG_ALWAYS, 0);
         showCursor();
      }
      for ( counter = 0; counter < 400; counter++ )
            memset((u8 *)&config + counter * 32, 'q', 3);
      write(configHandle, &config, sizeof(configType));
      close(configHandle);
   }
   else
   {  if ( (configHandle = openP(tempStr, O_WRONLY|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) == -1 ||
           lseek(configHandle, offsetof(configType, uplinkReq), SEEK_SET) == -1L ||
           write(configHandle, &config.uplinkReq, sizeof(uplinkReqType)*MAX_UPLREQ) < sizeof(uplinkReqType)*MAX_UPLREQ ||
           lseek(configHandle, offsetof(configType, lastUniqueID), SEEK_SET) == -1L ||
           write(configHandle, &config.lastUniqueID, sizeof(config.lastUniqueID)) < sizeof(config.lastUniqueID) ||
           close(configHandle) == -1 )
      {
         close(configHandle);
         logEntry("Can't write FMail.CFG", LOG_ALWAYS, 0);
         showCursor();
      }
   }
}
