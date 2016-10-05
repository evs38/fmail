//---------------------------------------------------------------------------
//
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
//---------------------------------------------------------------------------

#include <ctype.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "fmail.h"

#include "areainfo.h"
#include "crc.h"
#include "dups.h"
#include "log.h"
#include "msgpkt.h" // for openP
#include "time.h"
#include "utils.h"

//---------------------------------------------------------------------------
extern char       configPath[FILENAME_MAX];
extern configType config;

#define DUP_LEVEL  1

static s16 dupOpened  = 0;
static u32 *dupBuffer = NULL;
static u32 nextDupOld[256];

typedef struct
{
  char headerText[128];
  u32  version;
  u32  level;
  u32  kRecs;
  u32  totalSize;
  u8  _reserved[112];
  u32  nextDup[256];
} dupHdrStruct;

static dupHdrStruct dupHdr;

#ifdef __MINGW32__
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#endif

//---------------------------------------------------------------------------
#ifndef __OS2__
u32 *_memint(u32 *p, u32 c, u32 size)
{
  u32 count = 0;

  while (count < size && p[count] != c)
    count++;

  if (count == size)
    return NULL;

  return p + count;
}
#define memint _memint
#else
void * _RTLENTRY _EXPFUNC memint(const void * __s, int __c, size_t __n);
#endif

//---------------------------------------------------------------------------
void openDup(void)
{
   tempStrType tempPath;
   fhandle     dupHandle;
   u32         group;

   if ( !config.mailOptions.dupDetection )
      return;
   if ( config.kDupRecs < 16 )
      config.kDupRecs = 16;

   strcpy(tempPath, configPath);
   strcat(tempPath, "FMAIL32.DUP");

   if (  (dupHandle = openP(tempPath, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1
      || read(dupHandle, &dupHdr, sizeof(dupHdrStruct)) != sizeof(dupHdrStruct)
      || filelength(dupHandle) != (long)dupHdr.totalSize
      )
   {
      if (dupHandle != -1)
      {
        close(dupHandle);
        dupHandle = -1;
      }
      memset(&dupHdr, 0, sizeof(dupHdrStruct));
      strcpy(dupHdr.headerText, "FMail Dup File rev. 1.0\x1A");
      dupHdr.version = 0x00000100;
      dupHdr.level   = 0x00000100;
      dupHdr.kRecs   = config.kDupRecs;
   }
   if ((dupBuffer = (u32 *)malloc(config.kDupRecs * DUP_LEVEL * 4096)) == NULL)
   {
      config.mailOptions.dupDetection = 0;
      logEntry("Can't allocate dupbuffer space", LOG_ALWAYS, 2);
   }
   memset(dupBuffer, 0, config.kDupRecs * DUP_LEVEL * 4096);
   if (dupHandle != -1)
   {
      if (dupHdr.kRecs == (u32)config.kDupRecs)
         read(dupHandle, dupBuffer, dupHdr.kRecs * DUP_LEVEL * 4096);
      else
      {
         for (group = 0; group < 256; group++)
         {
            lseek(dupHandle, sizeof(dupHdrStruct) + dupHdr.kRecs * 16 * group, SEEK_SET);
            read(dupHandle, &dupBuffer[config.kDupRecs * 4 * group], 16 * min((u32)config.kDupRecs, dupHdr.kRecs));
            if (dupHdr.nextDup[group] >= (u32)config.kDupRecs * 4)
               dupHdr.nextDup[group] = 0;
         }
         dupHdr.kRecs = config.kDupRecs;
      }
      close(dupHandle);
   }
   dupHdr.totalSize = sizeof(dupHdrStruct)+DUP_LEVEL*dupHdr.kRecs*4096;
   memcpy(nextDupOld, dupHdr.nextDup, sizeof(nextDupOld));
   dupOpened++;
}
//---------------------------------------------------------------------------
s16 checkDup(internalMsgType *message, u32 areaNameCRC)
{
  u32   nodeCode = 0;
  u32   dupCode  = 0;
  u32   group;
  char *helpPtr;
  u16   dummy;

  if (!config.mailOptions.dupDetection || config.kDupRecs == 0)
    return 0;

  if (!config.mailOptions.ignoreMSGID && (helpPtr = findCLStr(message->text, "\x1MSGID: ")) != NULL)
  {
    helpPtr += 8;
    if (sscanf (helpPtr, "%hu:%hu/%hu", &dummy, &dummy, &dummy) < 3)
    {
      // non-Fido MSGIDs
      dupCode  = crc32t(helpPtr, '\n');
      nodeCode = crc32t(helpPtr, ' ');
    }
    else
    {
      nodeCode = crc32t(helpPtr, ' ');
      dupCode = 0;

      if ((helpPtr = strchr(helpPtr, ' ')) != NULL)
      {
        while (isxdigit(*(++helpPtr)))
          dupCode = (dupCode << 4) + (*helpPtr <= '9' ? *helpPtr - '0' : toupper(*helpPtr) - 'A' + 10);

        dupCode ^= ((s32)message->fromUserName[0] << 24)
                 ^ ((s32)message->toUserName[0]   << 16)
                 ^ ((s32)message->subject[0]      <<  8);

        if (*helpPtr > 0x20)
          dupCode ^= crc32t(helpPtr, '\n');
      }
    }
  }

  if (  nodeCode == 0
     && (helpPtr = findCLStr(message->text, " * Origin: ")) != NULL
     && (helpPtr = srchar(helpPtr+1, '\r', '(')) != NULL
     )
    nodeCode = crc32t(helpPtr,')');

  if (dupCode == 0)
    dupCode = crc32(message->fromUserName)
            ^ crc32(message->toUserName  )
            ^ crc32(message->subject     )
            ^ ((s32)message->year  << 20 )
            ^ ((s32)message->month << 16 )
            ^ ((s32)message->day   << 11 )
            ^ ((s32)message->hours <<  6 )
            ^ ((s32)message->minutes     );

  group = dupCode & 0xFF;
  dupCode ^= nodeCode ^ areaNameCRC;

  if (memint(&dupBuffer[dupHdr.kRecs * 4 * group], dupCode, dupHdr.kRecs * 4) != NULL)
    return 1;

  // dupeCode not found add it to dupe database
  dupBuffer[dupHdr.kRecs * 4 * group + dupHdr.nextDup[group]++] = dupCode;
  if (dupHdr.nextDup[group] >= dupHdr.kRecs * 4)
    dupHdr.nextDup[group] = 0;

  return 0;
}
//---------------------------------------------------------------------------
void validateDups(void)
{
  memcpy(nextDupOld, dupHdr.nextDup, 1024);
}
//---------------------------------------------------------------------------
void closeDup(void)
{
  u32         group
            , currentDup
            , validDup;
  tempStrType tempPath;
  fhandle     dupHandle;

  if ( !config.mailOptions.dupDetection || config.kDupRecs == 0 || !dupOpened )
    return;

  dupOpened = 0;

  for (group = 0; group < 256; group++)
  {
    if ((validDup = nextDupOld[group]) != (currentDup = dupHdr.nextDup[group]))
    {
      if (validDup < currentDup)
        memset(&dupBuffer[dupHdr.kRecs * 4 * group + validDup], 0, (currentDup - validDup) * 4);
      else
      {
        memset(&dupBuffer[dupHdr.kRecs * 4 * group], 0, currentDup * 4);
        memset(&dupBuffer[dupHdr.kRecs * 4 * group + validDup], 0, (dupHdr.kRecs * 4-validDup) * 4);
      }
    }
  }
  memcpy(dupHdr.nextDup, nextDupOld, sizeof(dupHdr.nextDup));

  strcpy(tempPath, configPath);
  strcat(tempPath, "FMAIL32.DUP");

  if (  (dupHandle = openP(tempPath, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1
     || write(dupHandle, &dupHdr, sizeof(dupHdrStruct)) != sizeof(dupHdrStruct)
     || write(dupHandle, dupBuffer, dupHdr.kRecs * DUP_LEVEL * 4096) != (int)(DUP_LEVEL * dupHdr.kRecs * 4096)
     || close(dupHandle) == -1
     )
    logEntry("Can't write to dup file", LOG_ALWAYS, 0);

  freeNull(dupBuffer);
}
//---------------------------------------------------------------------------
