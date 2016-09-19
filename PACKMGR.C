//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2016  Wilfred van Velzen
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

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "fmail.h"

#include "window.h"


#define MAX_PS_WINSIZE 15

extern configType config;
extern windowLookType windowLook;
extern char configPath[128];


void displayPack (packType pack, u16 elemCount,
                  u16 windowBase, u16 currentElem)
{
   u16 count;
   tempStrType tempStr;

   for (count = 0; count < MAX_PS_WINSIZE; count++)
   {
      if (windowBase+count < elemCount)
      {
         strcpy (tempStr, pack[windowBase+count]);
         memmove (tempStr+1, tempStr, strlen(tempStr)+1);
         *tempStr = ' ';
      }
      else
         *tempStr = 0;

      if ((elemCount != 0) && (windowBase+count == currentElem))
      {
         printStringFill (tempStr, ' ', PACK_STR_SIZE+1, 6, 6+count,
                          windowLook.scrollfg,
                          windowLook.scrollbg, MONO_INV);
      }
      else
      {
         printStringFill (tempStr, ' ', PACK_STR_SIZE+1, 6, 6+count,
                          windowLook.datafg,
                          windowLook.background, MONO_NORM);
      }
   }
  if (elemCount == 0)
    printString("**** Empty ****", 32, 13, windowLook.datafg, windowLook.background, MONO_NORM);
}



s16 packMgr (void)
{
   u16 update = 0;
   u16 elemCount = 0;
   u16 windowBase = 0;
   u16 currentElem = 0;
   u16      ch;
   fhandle  packHandle;
   tempStrType tempStr;
   packType *pack;

   if ((pack = malloc(sizeof(packType))) == NULL)
      return 0;

   memset(pack, 0, sizeof(packType));

   strcpy(stpcpy(tempStr, configPath), dPCKFNAME);

   if ((packHandle = open(tempStr, O_RDONLY|O_BINARY)) != -1)
   {
      if ((_read(packHandle, pack, sizeof(packType)) < (sizeof(packType)>>1)) ||
          (close(packHandle) == -1))
         displayMessage("Can't read "dPCKFNAME);
   }

   printString("Page Up  ", 0, 24, YELLOW, BLACK, MONO_HIGH);
   printString("Page Down  ", 9, 24, YELLOW, BLACK, MONO_HIGH);
   printString("Ins ", 20, 24, YELLOW, BLACK, MONO_HIGH);
   printString("Insert  ", 24, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
   printString("Del ", 32, 24, YELLOW, BLACK, MONO_HIGH);
   printString("Delete  ", 36, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
   printString("Enter ", 44, 24, YELLOW, BLACK, MONO_HIGH);
   printString("Edit  ", 50, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
   printString("Home ", 56, 24, YELLOW, BLACK, MONO_HIGH);
   printString("First  ", 61, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
   printString("End ", 68, 24, YELLOW, BLACK, MONO_HIGH);
   printString("Last ", 72, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
   printString("  ", 77, 24, YELLOW, BLACK, MONO_HIGH);

   while (elemCount < MAX_PACK && *(*pack)[elemCount] != 0)
      elemCount++;

   if (displayWindow(" Pack Manager ", 5, 5, 71, 6 + MAX_PS_WINSIZE) != 0)
   {
      free(pack);
      return 0;
   }

   do
   {
      do
      {
        displayPack (*pack, elemCount, windowBase, currentElem);

        ch = readKbd();
        switch (ch)
        {
            case _K_PGUP_ :
                          if (windowBase >= MAX_PS_WINSIZE-1)       /* PgUp */
			  {
			     windowBase -= MAX_PS_WINSIZE-1;
			     currentElem -= MAX_PS_WINSIZE-1;
			  }
			  else
			  {
			     windowBase = currentElem = 0;
			  }
			  break;
            case _K_PGDN_ :
                          if (windowBase+2*(MAX_PS_WINSIZE-1) <= elemCount) /* PgDn */
			  {
			     windowBase += MAX_PS_WINSIZE-1;
			     currentElem += MAX_PS_WINSIZE-1;
			  }
			  else
			  {
			     if (elemCount >= MAX_PS_WINSIZE)
			     {
				windowBase = elemCount - MAX_PS_WINSIZE + 1;
			     }
			     currentElem = elemCount-1;
			  }
			  break;
            case _K_CPGUP_ :
            case _K_HOME_ :
                          windowBase = currentElem = 0;        /* Home */
			  break;
            case _K_CPGDN_ :
            case _K_END_ :
                          if (elemCount > MAX_PS_WINSIZE)      /* End */
			  {
			     windowBase = elemCount - MAX_PS_WINSIZE;
			  }
			  currentElem = elemCount-1;
			  break;
            case _K_UP_ : if (currentElem > 0)                 /* Cursor Up */
			  {
			     if (--currentElem < windowBase)
				windowBase--;
			  }
			  break;
            case _K_DOWN_ :
                          if (currentElem < elemCount)       /* Cursor Down */
			  {
			     if (++currentElem >= windowBase + MAX_PS_WINSIZE)
				windowBase++;
			  }
			  break;
            case _K_ENTER_ :
                          if (currentElem < elemCount)           /* Edit */
			  {
                             displayPack (*pack, elemCount, windowBase, -1);
                             editString ((*pack)[currentElem],
                                         PACK_STR_SIZE, 7, 6+currentElem-windowBase,
                                         PACK);
                             update++;
                             if (*(*pack)[currentElem] == 0)
                             {
                                memmove ((*pack)[currentElem],
                                         (*pack)[currentElem+1],
                                         PACK_STR_SIZE*(MAX_PACK-currentElem-1));
                                memset((*pack)[MAX_PACK-1], 0, PACK_STR_SIZE);
                                elemCount--;
                             }
                          }
                          break;
            case _K_INS_ :
                          if (elemCount < MAX_PACK)            /* Insert */
                          {
                             memmove ((*pack)[currentElem+1],
                                      (*pack)[currentElem],
                                      PACK_STR_SIZE*(MAX_PACK-currentElem-1));
                             memset((*pack)[currentElem], 0, PACK_STR_SIZE);
                             elemCount++;
                             displayPack (*pack, elemCount, windowBase, -1);

                             *tempStr = 0;
                             editString (tempStr, PACK_STR_SIZE,
                                                  7, 6+currentElem-windowBase,
                                                  PACK);
                             if (*tempStr)
                             {
                                strcpy ((*pack)[currentElem], tempStr);
                                update++;
                             }
                             else
                             {
                                memmove ((*pack)[currentElem],
                                         (*pack)[currentElem+1],
                                         PACK_STR_SIZE*(MAX_PACK-currentElem-1));
                                memset((*pack)[MAX_PACK-1], 0, PACK_STR_SIZE);
                                elemCount--;
                             }
                          }
                          break;
            case _K_DEL_ :
                          if ((elemCount > 0) &&               /* Delete */
                              (currentElem < elemCount) &&
                              (askBoolean ("Delete this entry ?", 'Y') == 'Y'))
                          {
                             memcpy ((*pack)[currentElem], (*pack)[currentElem+1],
                                     (MAX_PACK-1-currentElem)*PACK_STR_SIZE);
                             if (currentElem == elemCount)
                                currentElem--;
                             memset ((*pack)[MAX_PACK-1], 0, PACK_STR_SIZE);
                             elemCount--;
                             if ((elemCount-currentElem < MAX_PS_WINSIZE) &&
                                 (windowBase > 0))
                                windowBase--;
                             update++;
                          }
                          break;
         }
      }
      while ((ch != _K_ESC_) && (ch != _K_F7_) && (ch != _K_F10_));
      if ( ch == _K_F7_ || ch == _K_F10_ )
         ch = 'Y';
      else
         if (update)
            ch = askBoolean ("Save changes in pack manager ?", 'Y');
         else
            ch = 0;
   }
   while (ch == _K_ESC_);

   if (ch == 'Y')
   {
      strcpy(stpcpy(tempStr, configPath), dPCKFNAME);

      if (((packHandle = open(tempStr, O_WRONLY|O_CREAT|O_BINARY|O_DENYALL,
                                       S_IREAD|S_IWRITE)) == -1) ||
          (_write(packHandle, pack, sizeof(packType)) != sizeof(packType)) ||
          (close(packHandle) == -1))
         displayMessage("Can't write to "dPCKFNAME);
   }
   removeWindow();
   free(pack);
   return (0);
}
//---------------------------------------------------------------------------
