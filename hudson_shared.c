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
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fmail.h"

#include "msgra.h"

#include "config.h"
#include "hudson_shared.h"
#include "lock.h"
#include "log.h"
#include "msgradef.h"
#include "os.h"
#include "utils.h"

//---------------------------------------------------------------------------
char *expandNameHudson(const char *fileName, int orgName)
{
  static tempStrType expandStr;

  strcpy(stpcpy(stpcpy(expandStr, fixPath(config.bbsPath)), fileName), (mbSharingInternal && !orgName) ? "."MBEXTB : "."MBEXTN);

  return expandStr;
}
//---------------------------------------------------------------------------
int testMBUnlockNow(void)
{
  static time_t mtime;
  tempStrType tempStr;
  struct stat st;
  int unlock = 0;

  if (mbSharingInternal)
  {
    strcpy(stpcpy(tempStr, fixPath(config.bbsPath)), "mbunlock.now");

    if (stat(tempStr, &st) != 0)  // already fixPath'd
      mtime = 0;
    else
    {
      unlock = mtime != st.st_mtime;
      mtime = st.st_mtime;
    }
  }
  return unlock;
}
//---------------------------------------------------------------------------
void setMBUnlockNow(void)
{
  tempStrType tempStr;

  if (mbSharingInternal)
  {
    strcpy(stpcpy(tempStr, fixPath(config.bbsPath)), "mbunlock.now");
    close(open(tempStr, O_RDWR | O_CREAT | O_BINARY, dDEFOMODE));
    testMBUnlockNow();
  }
}
//---------------------------------------------------------------------------
int lockMB(void)
{
  tempStrType tempStr;
  time_t      time1
            , time2;

  strcpy(stpcpy(tempStr, fixPath(config.bbsPath)), dMSGINFO"."MBEXTN);

  if ((lockHandle = open(tempStr, O_RDWR | O_CREAT | O_BINARY, dDEFOMODE)) == -1)  // already fixPath'd
  {
    logEntryf(LOG_ALWAYS, 0, "Can't open file %s for output", tempStr);

    return 1;
  }

#ifdef FMAIL
  if (mbSharingInternal)
#endif
  {
    testMBUnlockNow();
    if (  lock(lockHandle, sizeof(infoRecType) + 1, 1) == -1
       && errno == EACCES
       )
    {
      puts("Retrying to lock the message base\n");
      setMBUnlockNow();

      time(&time1);

      do
      {
        time(&time2);
      }
      while (  lock(lockHandle, sizeof(infoRecType) + 1, 1) == -1
            && errno == EACCES
            && (time2 - time1 < 15)
            );

      if (errno == EACCES)
      {
        logEntry("Can't lock the message base for update", LOG_ALWAYS, 0);
        close(lockHandle);

        return 1;
      }
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
void unlockMB(void)
{
#ifdef FMAIL
  if (mbSharingInternal)
#endif
    unlock(lockHandle, sizeof(infoRecType) + 1, 1);

  close(lockHandle);
}
//---------------------------------------------------------------------------
