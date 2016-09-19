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
#include <time.h>

#include "fmail.h"

#include "amgr_utl.h"

#include "areainfo.h"
#include "areamgr.h"
#include "fs_util.h"
#include "window.h"

//---------------------------------------------------------------------------
#define MAX_NS_WINSIZE 9
#define MAX_AL_WINSIZE 15

extern configType     config;

extern windowLookType windowLook;
extern rawEchoType    tempInfo;

extern areaInfoPtrArr areaInfo;
extern u16            areaInfoCount;

//---------------------------------------------------------------------------
s16 nodeScroll(void)
{
  u16         update = 0;
  u16         elemCount = 0;
  u16         count;
  u16         windowBase = 0;
  u16         currentElem = 0;
  u16         ch;
  char        tempStr[35];
  nodeNumType tempNode;

/*
0         1         2         3         4         5         6         7         8
012345678901234567890123456789012345678901234567890123456789012345678901234567890
Page Up   Page Down   Ins Insert   Del Delete   Home First   End Last    ^   V  |
Page Up  Page Down  Insert  Delete  Home First  End Last  R/o  W/o  Lock  ^V    |
*/
  printString("Page Up   " ,  0, 24, YELLOW      , BLACK, MONO_NORM);
  printString("Page Down  ",  9, 24, YELLOW      , BLACK, MONO_NORM);
  printString("Ins"        , 20, 24, YELLOW      , BLACK, MONO_NORM);
  printString("ert  "      , 23, 24, LIGHTMAGENTA, BLACK, MONO_HIGH);
  printString("Del "       , 28, 24, YELLOW      , BLACK, MONO_NORM);
  printString("ete  "      , 31, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
  printString("Home "      , 36, 24, YELLOW      , BLACK, MONO_NORM);
  printString("First  "    , 41, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
  printString("End "       , 48, 24, YELLOW      , BLACK, MONO_NORM);
  printString("Last  "     , 52, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
  printString("R"          , 58, 24, YELLOW      , BLACK, MONO_NORM);
  printString("/o  "       , 59, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
  printString("W"          , 63, 24, YELLOW      , BLACK, MONO_NORM);
  printString("/o  "       , 64, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
  printString("L"          , 68, 24, YELLOW      , BLACK, MONO_NORM);
  printString("ock  "      , 69, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
  printString("     "      , 74, 24, YELLOW      , BLACK, MONO_NORM);

  while (elemCount < MAX_FORWARD && tempInfo.forwards[elemCount].nodeNum.zone != 0)
    elemCount++;

  if (displayWindow(" Nodes ", 40, 8, 71, 9 + MAX_NS_WINSIZE) != 0)
    return 0;

  do
  {
    for (count = 0; count < MAX_NS_WINSIZE; count++)
    {
      if (windowBase + count < elemCount)
      {
        *tempStr = ' ';
        strcpy(tempStr + 1, nodeStr(&tempInfo.forwards[windowBase + count].nodeNum));
      }
      else
        *tempStr = 0;

      if (elemCount != 0 && windowBase+count == currentElem)
      {
        printStringFill(tempStr, ' ', 25, 41, 9+count,
                        windowLook.scrollfg,
                        windowLook.scrollbg, MONO_INV);
        printStringFill((tempInfo.forwards[windowBase+count].flags.locked) ? "lock":
                        (tempInfo.forwards[windowBase+count].flags.readOnly) ? "r/o":
                        (tempInfo.forwards[windowBase+count].flags.writeOnly) ? "w/o":"",
                        ' ', 5, 66, 9+count,
                        (tempInfo.forwards[windowBase+count].flags.locked) ? RED :
                        (tempInfo.forwards[windowBase+count].flags.readOnly) ? YELLOW : LIGHTGREEN,
                        windowLook.scrollbg, MONO_NORM_BLINK);
      }
      else
      {
        printStringFill (tempStr, ' ', 25, 41, 9+count,
                         windowLook.datafg,
                         windowLook.background, MONO_NORM);
        printStringFill((tempInfo.forwards[windowBase+count].flags.locked) ? "lock":
                        (tempInfo.forwards[windowBase+count].flags.readOnly) ? "r/o":
                        (tempInfo.forwards[windowBase+count].flags.writeOnly) ? "w/o":"",
                        ' ', 5, 66, 9+count,
                        (tempInfo.forwards[windowBase+count].flags.locked) ? RED :
                        (tempInfo.forwards[windowBase+count].flags.readOnly) ? YELLOW : LIGHTGREEN,
                        windowLook.background, MONO_NORM_BLINK);
      }
    }
    if (elemCount == 0)
       printString("Empty", 52, 13, windowLook.datafg, windowLook.background, MONO_NORM);

    ch = readKbd();
    switch (ch)
    {
         case 'l':
         case 'L':     tempInfo.forwards[currentElem].flags.readOnly = 0;
                       tempInfo.forwards[currentElem].flags.writeOnly = 0;
                       tempInfo.forwards[currentElem].flags.locked ^= 1;
                       update = 1;
                       break;
         case 'r':
         case 'R':     tempInfo.forwards[currentElem].flags.readOnly ^= 1;
                       tempInfo.forwards[currentElem].flags.writeOnly = 0;
                       tempInfo.forwards[currentElem].flags.locked = 0;
                       update = 1;
                       break;
         case 'w':
         case 'W':     tempInfo.forwards[currentElem].flags.readOnly = 0;
                       tempInfo.forwards[currentElem].flags.writeOnly ^= 1;
                       tempInfo.forwards[currentElem].flags.locked = 0;
                       update = 1;
                       break;
         case _K_PGUP_ :
                       if (windowBase >= MAX_NS_WINSIZE-1)       /* PgUp */
                       {
                          windowBase -= MAX_NS_WINSIZE-1;
                          currentElem -= MAX_NS_WINSIZE-1;
                       }
                       else
                       {
                          windowBase = currentElem = 0;
                       }
                       break;
         case _K_PGDN_ :
                       if (windowBase+2*(MAX_NS_WINSIZE-1) <= elemCount) /* PgDn */
                       {
                          windowBase += MAX_NS_WINSIZE-1;
                          currentElem += MAX_NS_WINSIZE-1;
                       }
                       else
                       {
                          if (elemCount > MAX_NS_WINSIZE)
                             windowBase = elemCount - MAX_NS_WINSIZE;

                          currentElem = elemCount - 1;
                       }
                       break;
         case _K_CPGUP_ :
         case _K_HOME_ :
                       windowBase = currentElem = 0;        /* Home */
                       break;
         case _K_CPGDN_ :
         case _K_END_: if (elemCount > MAX_NS_WINSIZE)      /* End */
                          windowBase = elemCount - MAX_NS_WINSIZE;

                       currentElem = elemCount - 1;
                       break;
         case _K_UP_ : if (currentElem > 0)                 /* Cursor Up */
                       {
                          if (--currentElem < windowBase)
                             windowBase--;
                       }
                       break;
         case _K_DOWN_ :
                       if (currentElem+1 < elemCount)       /* Cursor Down */
                       {
                          if (++currentElem >= windowBase + MAX_NS_WINSIZE)
                             windowBase++;
                       }
                       break;
         case _K_INS_ :
                       if (elemCount < MAX_FORWARD)            /* Insert */
                       {
                          tempNode = getNodeNum ("Insert node", 37, 12, tempInfo.address);
                          if (tempNode.zone != 0)
                          {
                             currentElem = 0;
                             while ((currentElem < elemCount) &&
                                    ((tempNode.zone   > tempInfo.forwards[currentElem].nodeNum.zone) ||
                                     ((tempNode.zone == tempInfo.forwards[currentElem].nodeNum.zone) &&
                                      (tempNode.net   > tempInfo.forwards[currentElem].nodeNum.net)) ||
                                     ((tempNode.zone == tempInfo.forwards[currentElem].nodeNum.zone) &&
                                      (tempNode.net  == tempInfo.forwards[currentElem].nodeNum.net) &&
                                      (tempNode.node  > tempInfo.forwards[currentElem].nodeNum.node)) ||
                                     ((tempNode.zone == tempInfo.forwards[currentElem].nodeNum.zone) &&
                                      (tempNode.net  == tempInfo.forwards[currentElem].nodeNum.net) &&
                                      (tempNode.node == tempInfo.forwards[currentElem].nodeNum.node) &&
                                      (tempNode.point > tempInfo.forwards[currentElem].nodeNum.point))))
                             {
                                currentElem++;
                             }
                             if (memcmp (&tempNode, &tempInfo.forwards[currentElem].nodeNum,
                                         sizeof(nodeNumType)) == 0)
                                displayMessage ("Nodenumber is already in list");
                             else
                             {
                                update = 1;
                                memmove (&tempInfo.forwards[currentElem+1],
                                         &tempInfo.forwards[currentElem],
                                         (elemCount++-currentElem)*sizeof(nodeNumXType));
                                memset(&tempInfo.forwards[currentElem], 0, sizeof(nodeNumXType));
                                tempInfo.forwards[currentElem].nodeNum = tempNode;
                                if (elemCount <= MAX_NS_WINSIZE)
                                   windowBase = 0;
                                else
                                   if (currentElem + MAX_NS_WINSIZE > elemCount)
                                      windowBase = elemCount - MAX_NS_WINSIZE;
                                   else
                                      windowBase = currentElem;
                             }
                          }
                       }
                       break;
         case _K_DEL_ :
                       if ((elemCount > 0) &&
                           (askBoolean ("Delete this node ?", 'Y') == 'Y'))
                       {
                          update = 1;
                          memcpy (&tempInfo.forwards[currentElem], &tempInfo.forwards[currentElem+1],
                                  (elemCount-currentElem-1)*sizeof(nodeNumXType));
                          memset (&tempInfo.forwards[--elemCount], 0, sizeof(nodeNumXType));
                          if (currentElem == elemCount)
                             currentElem--;
                          if ((elemCount-currentElem < MAX_NS_WINSIZE) &&
                              (windowBase > 0))
                             windowBase--;
                       }
                       break;
    }
  }
  while (ch != _K_ESC_);

  removeWindow();

  return update;
}
//---------------------------------------------------------------------------
s16 areasList(s16 currentElem, char groupSelectMask)
{
   static          s16 useComment = -1;
   u16             elemCount = 0;
   u16             count;
   u16             windowBase = 0;
   u16             ch = 0;
   tempStrType     tempStr;
   areaInfoPtrArr *areaInfoTwo;
   areaInfoPtr     areaInfoHelpPtr;
   u16             low, mid, high;
   void           *testPtr;

   if ((areaInfoTwo = malloc(sizeof(areaInfoPtrArr))) == NULL)
      return 0;

   if (useComment == -1) useComment = config.genOptions.commentFFD;

   saveWindowLook();
   windowLook.background = CYAN;
   windowLook.actvborderfg = BLUE;

   if (displayWindow (" Areas ", 16, 7, 71, 8+MAX_AL_WINSIZE) != 0)
   {
      restoreWindowLook();
      return (0);
   }
   restoreWindowLook();

   do
   {
      if (( ch == 0) || (ch == _K_LEFT_) || (ch == _K_RIGHT_))
      {
         memcpy (*areaInfoTwo, areaInfo, sizeof(areaInfoPtrArr));

         if (!ch)
            testPtr = (*areaInfoTwo)[currentElem];

         for (count = 1; count < areaInfoCount; count++)
         {
            areaInfoHelpPtr = (*areaInfoTwo)[count];
            low = 0;
            high = count;
            while (low < high)
            {
               mid = (low + high) >> 1;
               if (stricmp ((useComment &&
			    *areaInfoHelpPtr->comment) ?
                            areaInfoHelpPtr->comment : areaInfoHelpPtr->areaName,
			    (useComment &&
			    *(*areaInfoTwo)[mid]->comment) ?
                            (*areaInfoTwo)[mid]->comment : (*areaInfoTwo)[mid]->areaName) > 0)
                  low = mid+1;
               else
                  high = mid;
            }

            memmove (&(*areaInfoTwo)[low+1], &(*areaInfoTwo)[low],
                     (count-low) * 4); /* 4 = sizeof(ptr) */
            (*areaInfoTwo)[low] = areaInfoHelpPtr;
         }

         if (groupSelectMask)
         {
            elemCount = 0;
            for (count = 0; count < areaInfoCount; count++)
            {
               if ((*areaInfoTwo)[count]->group & (1L << (groupSelectMask-1)))
               {
                  (*areaInfoTwo)[elemCount++] = (*areaInfoTwo)[count];
               }
            }
         }
         else
            elemCount = areaInfoCount;

         currentElem = 0;
         while ((currentElem < elemCount) &&
                (testPtr != (*areaInfoTwo)[currentElem]))
         {
            currentElem++;
         }

         windowBase = max(0, currentElem-MAX_AL_WINSIZE/2);
         if ((elemCount >= MAX_AL_WINSIZE) &&
             (windowBase+MAX_AL_WINSIZE > elemCount))
         {
            windowBase = elemCount-MAX_AL_WINSIZE;
         }
      }
      for (count = 0; count < MAX_AL_WINSIZE; count++)
      {
         if (windowBase + count < elemCount)
         {
            if (useComment && *(*areaInfoTwo)[windowBase+count]->comment)
               sprintf( tempStr, " %-50.50s %c"
                      , (*areaInfoTwo)[windowBase + count]->comment
                      , getGroupChar((*areaInfoTwo)[windowBase + count]->group)
                      );
            else
               sprintf( tempStr, " %-50.50s %c"
                      , (*areaInfoTwo)[windowBase + count]->areaName
                      , getGroupChar((*areaInfoTwo)[windowBase + count]->group)
                      );
         }
         else
           *tempStr = 0;

         if ((elemCount != 0) && (windowBase+count == currentElem))
           printStringFill(tempStr, ' ', 54, 17, 8+count, YELLOW, BLACK, MONO_INV);
         else
           printStringFill (tempStr, ' ', 54, 17, 8+count, BLUE, CYAN, MONO_NORM);
      }
      if (elemCount == 0)
        printString ("Empty", 46, 12, CYAN, BLUE, MONO_NORM);

      ch = readKbd();
      switch (ch)
      {
         case _K_PGUP_ :
                       if (windowBase >= MAX_AL_WINSIZE-1)       /* PgUp */
                       {
                          windowBase -= MAX_AL_WINSIZE-1;
                          currentElem -= MAX_AL_WINSIZE-1;
                       }
                       else
                       {
                          windowBase = currentElem = 0;
                       }
                       break;
         case _K_PGDN_ :
                       if (windowBase+2*(MAX_AL_WINSIZE-1) <= elemCount) /* PgDn */
                       {
                          windowBase += MAX_AL_WINSIZE-1;
                          currentElem += MAX_AL_WINSIZE-1;
                       }
                       else
                       {
                          if (elemCount > MAX_AL_WINSIZE)
                          {
                             windowBase = elemCount - MAX_AL_WINSIZE;
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
                       if (elemCount > MAX_AL_WINSIZE)      /* End */
                       {
                          windowBase = elemCount - MAX_AL_WINSIZE;
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
                       if (currentElem+1 < elemCount)       /* Cursor Down */
                       {
                          if (++currentElem >= windowBase + MAX_AL_WINSIZE)
                             windowBase++;
                       }
                       break;
         case _K_LEFT_ :
         case _K_RIGHT_ :
                       useComment = !useComment;
                       break;
         default:      if ((ch > 32) && (ch < 256))
                       {
                          ch = toupper(ch);
                          count = currentElem;
                          do
                          {
        									  if (++count >= elemCount)
        										  count = 0;
        								  }
        								  while ((count != currentElem) &&
        											(((useComment &&
        												*(*areaInfoTwo)[count]->comment) ?
                                 *(*areaInfoTwo)[count]->comment :
        												*(*areaInfoTwo)[count]->areaName) != ch));
								  if (count != currentElem)
								  {
									  currentElem = count;
									  if (currentElem < windowBase)
										  windowBase = currentElem;

									  if (currentElem >= windowBase + MAX_AL_WINSIZE)
										  windowBase = currentElem - MAX_AL_WINSIZE + 1;

								  }
							  }
							  break;
    }
    testPtr = (*areaInfoTwo)[currentElem];
  }
  while (ch != _K_ESC_ && ch != _K_ENTER_);

  removeWindow();

  if (ch == _K_ENTER_)
  {
    count = 0;
    while (count < areaInfoCount && (*areaInfoTwo)[currentElem] != areaInfo[count])
      count++;

    if (count < areaInfoCount)
      return count;
  }
  return -1;
}
//---------------------------------------------------------------------------
