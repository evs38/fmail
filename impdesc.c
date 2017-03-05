//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2015 Wilfred van Velzen
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

#include <dir.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"

#include "cfgfile.h"
#include "ftlog.h"
#include "impdesc.h"
#include "minmax.h"

//---------------------------------------------------------------------------
char *findCLiStr (char *s1, char *s2);

s16 importNAInfo(char *fileName)
{
   fhandle      NAHandle;
   u16          count   = 0
              , updated = 0
              , xu;
   headerType	 *areaHeader;
   rawEchoType *areaBuf;
   char        *buf;
   u16          bufsize;
   char        *helpPtr
             , *helpPtr2;

   if ((NAHandle = open(fileName, O_RDONLY | O_BINARY)) == -1)
      logEntry("Can't find file", LOG_ALWAYS, 4);

   if ((buf = malloc(bufsize = min((u16)filelength(NAHandle)+1, 0xFFF0))) == NULL)
      logEntry("Not enough memory", LOG_ALWAYS, 2);

   if (read(NAHandle, buf, bufsize-1) != bufsize-1)
   {
      free(buf);
      logEntry("Can't read file", LOG_ALWAYS, 2);
   }
   close(NAHandle);
   buf[bufsize] = 0;

   if (!openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf))
   {
      close(NAHandle);
      free(buf);
      logEntry("Can't open "dARFNAME, LOG_ALWAYS, 2);
   }

   puts("Importing descriptions...\n");

   while (getRec(CFG_ECHOAREAS, count++))
   {
      if ((helpPtr = findCLiStr(buf, areaBuf->areaName)) == NULL)
         continue;
      while (*helpPtr != ' ')
         ++helpPtr;
      while (*helpPtr == ' ')
         ++helpPtr;
      helpPtr2 = areaBuf->comment;
      xu = 0;
      while (++xu < ECHONAME_LEN && *helpPtr && *helpPtr != '\r' && *helpPtr != '\n')
         *helpPtr2++ = *helpPtr++;

      do
      {
        *helpPtr2-- = 0;
      } while (helpPtr2 >= areaBuf->comment && *helpPtr2 == ' ');

      putRec(CFG_ECHOAREAS, count-1);
      updated++;
   }

   closeConfig(CFG_ECHOAREAS);
   free(buf);

   logEntryf(LOG_ALWAYS, 0, "%u descriptions imported", updated);

   return 0;
}
