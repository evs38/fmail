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
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include "fmail.h"
#include "areainfo.h"
#include "dups.h"
#include "utils.h"
#include "crc.h"
#include "time.h"
#include "log.h"
#include "output.h"
#include "msgpkt.h" /* for openP */

#define MAX_DUPS 16384 /* Maximum i.v.m. EMS ! */

void* cdecl memint (const void *s, int c, size_t n);

extern char configPath[128];
extern configType   config;
extern time_t       startTime;


static u16      validDup   = 0;
static u16      currentDup = 0;
static s16      dupOpened  = 0;

static u16      *dupBuffer1,
                *dupBuffer2;

s16             ems = 0;
#ifndef __32BIT__
#ifndef __FMAILX__
static int      EMSHandle;
#endif
#endif


void openDup (void)
{
   tempStrType tempPath;
   fhandle     dupHandle;
#ifndef __32BIT__
#ifndef __FMAILX__
   u16         count;
#endif
#endif
   u16         dfSize, dupLimit;

   if (!config.mailOptions.dupDetection)
      return;
#ifndef __32BIT__
#ifndef __FMAILX__
   if (config.genOptions.useEMS)
   {
      if ((EMSHandle = _open("EMMXXXX0", O_RDONLY)) != -1)
      {
   	   ems = ioctl (EMSHandle, 7);
              close (EMSHandle);
	      if (ems)
	      {
	        _AH = 0x41;
	        geninterrupt (0x67); /* Get pageframe */
	        if (_AH == 0)
	        {
    	        dupBuffer1 = MK_FP (_BX, 0);
	           dupBuffer2 = MK_FP (_BX, 0x8000);
	           _AH = 0x43;
	           _BX = 4;    /* 4 pages */
	           geninterrupt (0x67);        /* Open handle */
	           if (_AH == 0)
	           {
		           EMSHandle = _DX;

		           _AH = 0x47;
		           geninterrupt (0x67);

		           if (_AH)
		              ems = 0;
		           else
		              for (count = 0; (count < 4) & (ems != 0); count++)
		              {
			              _DX = EMSHandle;
			              _BX = count;
			              _AL = _BX;
			              _AH = 0x44;
			              geninterrupt (0x67);
			              if (_AH)
			                 ems = 0;
		              }

		           if (ems == 0)
		           {
		              _DX = EMSHandle;
		              _AH = 0x45;
		              geninterrupt (0x67);

#ifdef DEBUG
		              printString ("DEBUG (dups.c): Error allocating EMS memory\n");
#endif

		           }
	           }
		   else
		           ems = 0;
	        }
	        else
	           ems = 0;
	      }
      }
   }

#ifdef DEBUG
   if (ems)
      printString ("DEBUG (dups.c): EMS driver detected.\n");
   else
      printString ("DEBUG (dups.c): EMS driver not detected.\n");
#endif
#endif /* __FMAILX__ */
#endif /* __32BIT__ */
   if (!ems)
   {
      if (((dupBuffer1 = malloc (MAX_DUPS<<1)) == NULL) ||
	  ((dupBuffer2 = malloc (MAX_DUPS<<1)) == NULL))
      {
	 config.mailOptions.dupDetection = 0;
	 logEntry ("Can't allocate dupbuffer space", LOG_ALWAYS, 2);
      }
   }

   memset (dupBuffer1, 0, MAX_DUPS<<1);
   memset (dupBuffer2, 0, MAX_DUPS<<1);

   strcpy (tempPath, configPath);
   strcat (tempPath, "FMAIL.DUP");

   if ((dupHandle = openP(tempPath, O_RDONLY|O_DENYNONE|O_BINARY,
				    S_IREAD|S_IWRITE)) != -1)
   {
      dupLimit = min(MAX_DUPS, dfSize=(u16)(filelength(dupHandle)>>2));

      _read (dupHandle, &dupBuffer1[MAX_DUPS-dupLimit], dupLimit<<1);
      lseek (dupHandle, dfSize<<1, SEEK_SET);
      _read (dupHandle, &dupBuffer2[MAX_DUPS-dupLimit], dupLimit<<1);

      close (dupHandle);
   }
   dupOpened++;
}



s16 checkDup (internalMsgType *message, u32 areaNameCRC)
{
   u32           nodeCode = 0;
   u32           dupCode = 0;
   u16           dupCodeHi,
		 dupCodeLo;
   char          *helpPtr;
   u16           *dupPtr;
   u16           index;
   u16           dummy;

   if (!config.mailOptions.dupDetection)
      return (0);

   if ( (!config.mailOptions.ignoreMSGID) &&
	((helpPtr = findCLStr(message->text, "\x1MSGID: ")) != NULL) )
   {
      helpPtr += 8;
      if (sscanf (helpPtr, "%hu:%hu/%hu", &dummy, &dummy, &dummy) < 3)
//    if ( !isdigit(*(helpPtr += 8)) )
      {
      /* non-Fido MSGIDs */
         dupCode = crc32t(helpPtr, '\n');
         nodeCode = crc32t(helpPtr, ' ');
      }
      else
      {  nodeCode = crc32t(helpPtr, ' ');
         if ((helpPtr = strchr(helpPtr, ' ')) != NULL)
         {
            while (isxdigit(*(++helpPtr)))
            {  dupCode = (dupCode<<4) + (*helpPtr <= '9' ?
                          *helpPtr - '0' : toupper(*helpPtr) - 'A' + 10);
            }
            dupCode ^= ((s32)message->fromUserName[0] << 24) ^
                       ((s32)message->toUserName[0]   << 16) ^
                       ((s32)message->subject[0]      <<  8);

            if (*helpPtr > 0x20)
               dupCode ^= crc32t(helpPtr, '\n');
         }
      }
   }

   if ((nodeCode == 0) &&
       ((helpPtr = findCLStr(message->text, " * Origin: ")) != NULL) &&
       ((helpPtr = srchar(helpPtr+1, '\r', '(')) != NULL))
   {
      nodeCode = crc32t(helpPtr,')');
   }

   if (dupCode == 0)
      dupCode = crc32(message->fromUserName) ^
                crc32(message->toUserName)   ^
                crc32(message->subject)      ^
		((s32)message->year  << 20)  ^
		((s32)message->month << 16)  ^
		((s32)message->day   << 11)  ^
		((s32)message->hours <<  6)  ^
		((s32)message->minutes);

   dupCode ^= nodeCode ^ areaNameCRC;

   dupCodeHi = (u16)(dupCode>>16);
   dupCodeLo = (u16)dupCode;

   dupPtr = dupBuffer2;
   index = 0;
   while ((index < MAX_DUPS) &&
          (dupPtr = memint(dupPtr, dupCodeLo, MAX_DUPS-index)) != NULL)
   {
      index = ((u16)dupPtr-(u16)dupBuffer2)>>1;
      if (dupBuffer1[index] == dupCodeHi)
      {
	 return (1);
      }
      dupPtr++;
      index++;
   }

   if (currentDup >= MAX_DUPS)
   {
      currentDup = 0;
   }

   dupBuffer1[currentDup]   = dupCodeHi;
   dupBuffer2[currentDup++] = dupCodeLo;

   return (0);
}



void validateDups (void)
{
   validDup = currentDup;
}



void closeDup (void)
{
   tempStrType tempPath;
   fhandle     dupHandle;

   if ((!config.mailOptions.dupDetection) || (!dupOpened))
      return;

   dupOpened = 0;

   strcpy (tempPath, configPath);
   strcat (tempPath, "FMAIL.DUP");

   if ((dupHandle = openP(tempPath, O_WRONLY|O_CREAT|O_TRUNC|O_DENYNONE|O_BINARY,
                                    S_IREAD|S_IWRITE)) == -1)
   {
      printString ("Can't write to dupfile.\n");
   }
   else
   {  if ( validDup != currentDup )
      {  if ( validDup < currentDup )
         {  memset(&dupBuffer1[validDup], 0, (currentDup-validDup)<<1);
            memset(&dupBuffer2[validDup], 0, (currentDup-validDup)<<1);
         }
         else
         {  memset(dupBuffer1, 0, currentDup<<1);
            memset(dupBuffer2, 0, currentDup<<1);
            memset(&dupBuffer1[validDup], 0, (MAX_DUPS-validDup)<<1);
            memset(&dupBuffer2[validDup], 0, (MAX_DUPS-validDup)<<1);
         }
      }
      write(dupHandle, &dupBuffer1[validDup], (MAX_DUPS-validDup)<<1);
      write(dupHandle, dupBuffer1, validDup<<1);
      write(dupHandle, &dupBuffer2[validDup], (MAX_DUPS-validDup)<<1);
      write(dupHandle, dupBuffer2, validDup<<1);

      close (dupHandle);
   }
#ifndef __32BIT__
#ifndef __FMAILX__
   /* Return EMS-pages */

  if (ems)
   {
      _DX = EMSHandle;
      _AH = 0x48;
      geninterrupt (0x67);

      _DX = EMSHandle;
      _AH = 0x45;
      geninterrupt (0x67);

#ifdef DEBUG
      printString ("DEBUG (dups.c): EMS memory released\n");
#endif

   }
   else
#endif /* __FMAILX__ */
#endif /* __32BIT__ */
   {
      free (dupBuffer1);
      free (dupBuffer2);
   }
}

