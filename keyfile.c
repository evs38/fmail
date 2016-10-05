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


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>

#include "fmail.h"
#include "keyfile.h"
#include "config.h"
#include "output.h"
#include "crc.h"

#define N 65339L

extern int no_msg;

static int checkKeyStd(u32 *relKey1, u32 *relKey2)
{
  u16 keyResult;
  u32 key;
  u32 tempKey, crcName;
  u16 count;

  if (*relKey1 == 0 && *relKey2 == 0)
    return 0;

  /* !!! nieuwe toevoeging t.o.v. originele keysysteem !!! */
  *relKey1 ^= 0x4fd34193L;
  *relKey2 ^= *relKey1;

  keyResult = (u16)(*relKey2 >> 16) 
            ^ (u16)(*relKey2 & 0xffff);

  key = tempKey = (*relKey1 >> 16) ^ (*relKey1 & 0xffff);

  for (count = 1; count < 17; count++)
  {
    key *= tempKey;
    key %= N;
  }

  tempKey = crcName = crc32old(config.sysopName);

  tempKey ^= (tempKey >> 16);
  tempKey &= 0x0000ffff;

  tempKey ^= keyResult;
  tempKey %= N;

  if (  key != tempKey 
     || (*relKey1 == 957691693L && *relKey2 == 824577056L ) // gekraakte keys
     || (*relKey1 == 405825030L && *relKey2 == 1360920973L) // in gevonden lijst
     )
  {
    return 0;
#if 0
    setAttr (LIGHTRED, BLACK, MONO_HIGH);
    logEntry ("Registration keys are missing or invalid", LOG_ALWAYS, 100);
    exit (10);
#endif
  }
#if 0
  setAttr (LIGHTGREEN, BLACK, MONO_HIGH);
  printString ("Registered to ");
  printString (config.sysopName);
  setAttr (LIGHTGRAY, BLACK, MONO_NORM);
  newLine ();
  newLine ();
#endif
//if (crcName == 1423435765 && toupper(*config.sysopName) == 'F') // TEST "Folkert Wijnstra"
  if (crcName == 2852920193L && toupper(*config.sysopName) == 'B') // Bas Koot gaf zijn key vrij
    return 0;

  return 1;
}

keyType key;

int keyFileInit(void)
{
  int tempHandle;
  tempStrType tempStr;

  strcpy(tempStr, configPath);
  strcat(tempStr, "fmail.key");
  ++no_msg;
  if ((tempHandle = open(tempStr, O_RDONLY|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) == -1)
    return 0;

  if (read(tempHandle, &key, sizeof(keyType)) != sizeof(keyType))
  {
    close(tempHandle);
    return 0;
  }
  close(tempHandle);
  
  if ((key.crc ^ 0x4c2de439L ^ (u32)key.data[173]) != crc32len((char *)&key, sizeof(keyType) - 4))
    return 0;

  if (!checkKeyStd(&key.relKey1, &key.relKey2))
    return 0;

  return 1;
}
