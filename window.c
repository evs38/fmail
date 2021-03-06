//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2017 Wilfred van Velzen
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
#include <dir.h>
#include <io.h>
#include <process.h>  // spawnl()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __32BIT__
#ifdef __MINGW32__
#include "conio2.h"
#else
#include <conio.h>
#endif // __MINGW32__
#endif

#include "fmail.h"

#include "window.h"

#include "areainfo.h"
#include "areamgr.h"
#include "fs_func.h"
#include "fs_util.h"
#include "jam.h"
#include "minmax.h"
#include "mtask.h"
#include "nodeinfo.h"
#include "stpcpy.h"
#include "utils.h"

extern char boardCodeInfo[512];

#define columns 80

#ifdef __32BIT__
#define keyWaiting() kbhit()
#define keyRead()    getch()
#endif


s16 askGroup(void);
s16 askGroups(void);

extern configType config;
extern rawEchoType tempInfo;
extern nodeInfoType tempInfoN;
extern areaInfoPtrArr areaInfo;
extern u16            areaInfoCount;

extern windowLookType windowLook;

const char *months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static s16 initMagic  = 0;
       s16 color      = 1;

static windowMemType  windowStack[MAX_WINDOWS];
static u16            windowSP = 0;

static char border[4][8] = { {'?', '?', '?', '?', '?', '?', '?', '?'}
                           , {'?', '?', '?', '?', '?', '?', '?', '?'}
                           , {'?', '?', '?', '?', '?', '?', '?', '?'}
                           , {'?', '?', '?', '?', '?', '?', '?', '?'}
                           };
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
#endif

//---------------------------------------------------------------------------
#ifdef __32BIT__
//---------------------------------------------------------------------------
#if 0
void showCharNA(int x, int y, int chr)
{
  gotoxy(x + 1, y + 1);
  putch(chr);
}
#endif
//---------------------------------------------------------------------------
void showChar(int x, int y, int chr, int att, int matt)
{
  struct char_info buf;

  x++;
  y++;
  buf.letter = chr;
  buf.attr = color ? att : matt;
  puttext(x, y, x, y, &buf);
}
//---------------------------------------------------------------------------
void shadow(int x, int y)
{
  if (color)
    changeColor(x, y, DARKGRAY, DARKGRAY);
}
//---------------------------------------------------------------------------
void changeColor(int x, int y, int att, int matt)
{
  struct char_info buf;

  x++;
  y++;
  gettext(x, y, x, y, &buf);
  buf.attr = color ? att : matt;
  puttext(x, y, x, y, &buf);
}
//---------------------------------------------------------------------------
char getChar(int x, int y)
{
  struct char_info buf;

  x++;
  y++;
  gettext(x, y, x, y, &buf);

  return buf.letter;
}
//---------------------------------------------------------------------------
#if 0
u16 getColor(int x, int y, int att, int matt)
{
  struct char_info buf;

  x++;
  y++;
  gettext(x, y, x, y, &buf);

  return buf.attr;
}
#endif
//---------------------------------------------------------------------------
void locateCursor(int x, int y)
{
  gotoxy(x + 1, y + 1);
//  _setcursortype(_NORMALCURSOR);
}
#endif
//---------------------------------------------------------------------------
u16 readKbd(void)
{
  int        ch;
  time_t     ti;
  struct tm *tm;
  u16        attr;
  u8         min = 0xff;
#ifdef __32BIT__
  struct text_info txti;
#endif

  attr = calcAttr(WHITE, RED);
  do
  {
    while (!keyWaiting())
    {
      time(&ti);
      tm = localtime(&ti);

      if (tm->tm_min == min)
      {
        returnTimeSlice(1);
        continue;
      }
#ifdef __32BIT__
      // Get current cursor postion and attr
      gettextinfo(&txti);
#endif
      min = tm->tm_min;
      showChar(72, 1, (tm->tm_hour / 10) ? '0' + tm->tm_hour / 10 : ' ', attr, MONO_NORM);
      showChar(73, 1, '0' + tm->tm_hour % 10, attr, MONO_NORM);
      showChar(74, 1, ':', attr, MONO_NORM);
      showChar(75, 1, '0' + tm->tm_min / 10, attr, MONO_NORM);
      showChar(76, 1, '0' + tm->tm_min % 10, attr, MONO_NORM);
#ifdef __32BIT__
      // Restore cursor postion and attr
      gotoxy(txti.curx, txti.cury);
      textattr(txti.attribute);
#endif
    }

    ch = keyRead();
#ifdef __32BIT__
    if (ch == -1)  // Error happened in getch()
    {
      ch = 0;
      continue;
    }
    if (ch == 0
#ifdef __MINGW32__
       || ch == 0xE0
#endif // __MINGW32__
       )
      ch = keyRead() << 8;
    else
      ch &= 0xFF;
#endif
  }
  while (ch == 0);

  return ch;
}
//---------------------------------------------------------------------------
int orgCursorType;
//---------------------------------------------------------------------------
void initWindow(u16 mode)
{
  struct text_info info;
  getMultiTasker();

  if (mode == 1)
    color = 0;
  if (mode == 2)
    color = 1;

  gettextinfo(&info);
  if (info.screenheight < 25 || info.screenwidth < 80)
  {
    puts("\nYour console window isn't big enough!\n"
         "It should be at least 80x25.\n"
         "Please fix that before starting FConfig again...\n");
    exit(1);
  }
  orgCursorType = _getcursortype();
#ifdef __BORLANDC__
  textmode(color ? C80 : BW80);  // X28
#endif // __BORLANDC__
  textattr(calcAttr(WHITE, BLACK));
  clrscr();
  removeCursor();

  // Draw a border around the space used by FConfig, if the console is bigger than 80x25
  if (info.screenheight > 25)
  {
    int x
      , maxx = min(info.screenwidth, 81);

    for (x = 0; x < maxx; x++)
      showChar(x, 25, ' ', calcAttr(LIGHTGRAY, LIGHTGRAY), calcAttr(WHITE, WHITE));
  }
  if (info.screenwidth > 80)
  {
    int y
      , maxy = min(info.screenheight, 26);

    for (y = 0; y < maxy; y++)
      showChar(80, y, ' ', calcAttr(LIGHTGRAY, LIGHTGRAY), calcAttr(WHITE, WHITE));
  }
  initMagic = 0x4657;
}
//---------------------------------------------------------------------------
void deInit(u16 cursorLine)
{
#ifdef __BORLANDC__
  textmode(LASTMODE);
#endif // __BORLANDC__
  normvideo();
  clrscr();
  _setcursortype(orgCursorType >= 0 ? orgCursorType : _NORMALCURSOR);
  locateCursor(0, 0);
}
//---------------------------------------------------------------------------
void printStringFill(const char *str, char ch, s16 num, u16 x, u16 y, u16 fgc, u16 bgc, u16 mAttr)
{
  int attr = calcAttr(fgc, bgc);
  gotoxy(x + 1, y + 1);
  textattr(color ? attr : mAttr);

  if (str != NULL)
  {
    int l = strlen(str);
    if (l > num)
    {
      char *strt;
      l = num;
      strt = malloc(l + 1);
      strncpy(strt, str, l);
      strt[l] = 0;
      cputs(strt);
      free(strt);
    }
    else
      cputs(str);
    x += l;
    num -= l;
  }
  while (num-- > 0)
  {
    showChar(x, y, ch, attr, mAttr);
    x++;
  }
}
//---------------------------------------------------------------------------
static s16 checkPackNode(char *nodeString, u16 *valid, s16 wildCards, s16 numSign)
{
   u16 count;
   u16 pointFound = 0;
   u16 lastLevel  = 0;

   if ((*nodeString=='*') && (nodeString[1]==0)) return (0);

   if (*nodeString=='.')
   {
      if (*valid < 3)
        return -1;
      lastLevel = *valid = 3;
      pointFound++;
      nodeString++;
   }

   while (*nodeString)
   {
      count = 0;
      while ((isdigit(*nodeString)) ||
             (wildCards && ((*nodeString=='?') || (*nodeString=='*'))) ||
             (numSign   && *nodeString=='#'))
      {
         if (*nodeString=='*')
         {
            if ((!*valid) || (nodeString[1]!=0)) return(-1);
            return(0);
         }
         nodeString++;
         count++;
      }
      if ((count==0) || (count>5)) return(-1);
      switch (*nodeString)
      {
         case ':' : if (lastLevel) return(-1);
                    lastLevel = *valid = 1;
                    break;
         case '/' : if ((lastLevel>1) || (*valid<1)) return(-1);
                    lastLevel = *valid = 2;
                    break;
         case '.' : if ((lastLevel>2) || (*valid<2)) return(-1);
                    lastLevel = *valid = 3;
                    pointFound++;
                    break;
         case 0   : if (!pointFound)
                    {
                      if (*valid < 2)
                        return -1;
                      *valid = 3;
                    }
                    else
                    {
                      if (*valid < 3)
                        return -1;
                      *valid = 4;
                    }
                    return 0;
         default  : return -1;
      }
      nodeString++;
   }
   return -1;
}
//---------------------------------------------------------------------------
s16 groupToChar(s32 group)
{
  s16 c = 'A';

  if (group)
    while ( !(group & BIT0) )
    {
      group >>= 1;
      c++;
    }
  return c;
}
//---------------------------------------------------------------------------
u16 es__8c = 0;

u16 editString(char *string, u16 width, u16 x, u16 y, u16 fieldType)
{
   s16      redo;
   s16      error = 0;
   size_t   sPos;
   u16      defaultValid;
   u16      numSign;
   u16      ch;
   u16      count;
   u16      insert = 1;
   u16      update = 1;
   u16      clear  = 1; /* fieldType & CLEAR; */
   tempStrType  tempStr;
   tempStrType  testStr;
   char        *helpPtr;
   nodeNumType  nodeNum;

   es__8c = 0;

   if (fieldType & NOCLEAR)
      clear = 0;

   strcpy(tempStr, string);
   do
   {
      largeCursor();
      redo = 0;

      sPos = strlen(tempStr);
      do
      {
         if (update)
         {
            printStringFill(tempStr, '?', width - 1, x, y, windowLook.editfg, windowLook.editbg, MONO_NORM);
            update = 0;
         }
         locateCursor(x + sPos, y);

         ch = readKbd();

         if (ch == _K_CTLY_)                           /* CTRL-Y */
         {
            *tempStr = 0;
            clear = 0;
            sPos = 0;
            update++;
         } else
         if (ch == _K_HOME_)                          /* Home */
         {
            clear = 0;
            sPos = 0;
         } else
         if (ch == _K_END_)                           /* End */
         {
            clear = 0;
            sPos = strlen(tempStr);
         } else
         if (ch == _K_LEFT_)                           /* Left arrow */
         {
            clear = 0;
            if (sPos > 0)
               sPos--;
         } else
         if (ch == _K_RIGHT_)                           /* Right arrow */
         {
            clear = 0;
            if (tempStr[sPos] != 0)
               sPos++;
         } else
         if (ch == _K_CLEFT_)                           /* Ctrl Left arrow */
         {
            clear = 0;
            while (sPos && (tempStr[--sPos] == ' '))
            {}
            while (sPos && (tempStr[sPos-1] != ' '))
            {
               sPos--;
            }
         } else
         if (ch == _K_CRIGHT_)                           /* Ctrl Right arrow */
         {
            clear = 0;
            while (tempStr[sPos] && (tempStr[sPos++] != ' ')) {}
            while (tempStr[sPos] && (tempStr[sPos] == ' '))
            {
               sPos++;
            }
         } else
         if (ch == _K_INS_)                           /* Insert */
         {
            clear = 0;
            if ((insert = !insert) == 0)
               smallCursor();
            else
               largeCursor();
         }
         else
         if (ch == _K_DEL_)                           /* Delete */
         {
            clear = 0;
            if (tempStr[sPos] != 0)
            {
               strcpy (tempStr+sPos, tempStr+sPos+1);
               update++;
            }
         } else
         if (ch == _K_CEND_)                           /* Ctrl-End */
         {
            clear = 0;
            *(tempStr + sPos) = 0;
            update++;
         } else
         if (ch == _K_BSPC_)                                /* Backspace */
         {
            clear = 0;
            if (sPos > 0)
            {
               strcpy (tempStr+sPos-1, tempStr+sPos);
               sPos--;
               update++;
            }
         } else
         if (ch != 0 && ch < 256 &&
             (((fieldType & CTRLCODES) && ch >= 1 && ch <= 26 && ch != 13 && config.genOptions.ctrlcodes) ||
              (((fieldType & MASK) == FMTEXT) && (ch >= ' ')) ||
              (((fieldType & MASK) == PACK) && (ch >= ' ') && (ch <= '~')) ||
              (((fieldType & MASK) == WORD) && (ch > ' ') && (ch <= 254)) ||
              (((fieldType & MASK) == PATH ||
                (fieldType & MASK) == FILE_NAME ||
                (fieldType & MASK) == MB_NAME ||
                (fieldType & MASK) == EMAIL ||
                (fieldType & MASK) == SFILE_NAME) && ch == ' ') ||
              (((fieldType & MASK) == PATH) && (ch > ' ') && (ch <= '~') &&
                                               (ch != '<') && (ch != '>') &&
                                               (ch != '|') && (ch != '+') &&
                                               (ch != '+') && (ch != '%') &&
                                               (ch != '*') && (ch != '?')) ||
              (((fieldType & MASK) == SFILE_NAME ||
                (fieldType & MASK) == FILE_NAME) && (ch > ' ') && (ch <= '~') &&
                                                    (ch != '<') && (ch != '>') &&
                                                    (ch != '|') && (ch != '+') &&
                                                    (ch != '*') && (ch != '?')) ||
              (((fieldType & MASK) == MB_NAME) && (ch > ' ') && (ch <= '~') &&
                                                  (ch != '<') && (ch != '>') &&
                                                  (ch != '|') && (ch != '+') &&
                                                  (ch != '*') && (ch != '?')) ||
              (((fieldType & MASK) == EMAIL)      && (ch > ' ') && (ch <= '~') &&
                                                     (ch != '<') && (ch != '>') &&
                                                     (ch != '|') && (ch != '+') &&
                                                     (ch != '*') && (ch != '?') &&
                                                     (ch != '\\') && (ch != ':')) ||
              (((fieldType & MASK) == ALPHA) && (isalpha(ch))) ||
              (((fieldType & MASK) == ALPHA_AST) && ((isalpha(ch)) || (ch == '*'))) ||
              ((((fieldType & MASK) == NUM_INT) ||
                ((fieldType & MASK) == NUM_P_INT) ||
                ((fieldType & MASK) == NUM_LONG)) && (isdigit(ch))) ||
              (((fieldType & MASK) == NODE) && (isdigit(ch) || (ch == ':') || (ch == '/') || (ch == '.') || (ch == '-')))))
         {
            if (clear)
            {
               clear = 0;
               sPos = 0;
               *tempStr = 0;
            }
            if (!insert && sPos < strlen(tempStr))
            {
               if (fieldType & UPCASE)
                  tempStr[sPos++] = toupper(ch);
               else
                  tempStr[sPos++] = ch;
               update++;
            }
            else
               if (strlen(tempStr) < (size_t)width - 1)
               {
                  memmove(&tempStr[sPos+1], &tempStr[sPos], width - 1 - sPos);
//                if ((fieldType & UPCASE) && (!config.genOptions.lfn || !(fieldType & LFN)))
                  if ((fieldType & UPCASE) && !(fieldType & LFN))
                     tempStr[sPos++] = toupper(ch);
                  else
                     tempStr[sPos++] = ch;
                  update++;
               }
         }
      }
      while (ch != _K_ENTER_ && ch != _K_ESC_)
        ;

      removeCursor();

      // Remove trailing spaces
      while (((sPos = strlen(tempStr)) != 0) && (tempStr[--sPos] == ' '))
         tempStr[sPos] = 0;

      if (ch == _K_ENTER_ || ch == _K_UP_ || ch == _K_DOWN_)
      {
         error = 0;
         if ((fieldType & MASK) == PACK)
         {
            if (*tempStr)
            {
               numSign = 0;
               strcpy (testStr, tempStr);
               strupr (testStr);

               helpPtr = strtok(testStr, " ");

               defaultValid = 0;

               if ((!error) &&
                   ((helpPtr == NULL) ||
                    (strcmp (helpPtr, "VIA") == 0) ||
                    (strcmp (helpPtr, "EXCEPT") == 0)))
               {
                  redo = (askBoolean("Missing node in pack string. Edit ?", 'Y') == 'Y');
                  error = 1;
               }

               while ((!error) && (helpPtr != NULL) &&
                      (strcmp (helpPtr, "VIA") != 0) &&
                      (strcmp (helpPtr, "EXCEPT") != 0) &&
                      (*helpPtr != '/'))
               {
                  numSign |= (strchr(helpPtr, '#') != 0);
                  if (checkPackNode (helpPtr, &defaultValid, 1, 1))
                  {
                     redo = (askBoolean("Bad node in pack string. Edit ?", 'Y') == 'Y');
                     error = 1;
                  }
                  helpPtr = strtok(NULL, " ");
               }
               defaultValid = 0;
               if ((!error) && (helpPtr!=NULL) &&
                   (strcmp (helpPtr, "EXCEPT") == 0))
               {  helpPtr = strtok(NULL, " ");
                  while ((!error) &&
                         (helpPtr != NULL) &&
                         (strcmp (helpPtr, "VIA") != 0) &&
                         (*helpPtr != '/'))
                  {
                     if (checkPackNode (helpPtr, &defaultValid, 1, 1))
                     {
                        redo = (askBoolean("Bad node in pack string. Edit ?", 'Y') == 'Y');
                        error = 1;
                     }
                     helpPtr = strtok(NULL, " ");
                  }
               }

               if ((!error) && (helpPtr != NULL) &&
                               (strcmp (helpPtr, "VIA") == 0))
               {
                  defaultValid = 0;
                  if ((helpPtr = strtok(NULL, " ")) == NULL)
                  {
                     redo = (askBoolean("Missing VIA node. Edit ?", 'Y') == 'Y');
                     error = 1;
                  } else
                  if ((!error) && stricmp(helpPtr,"HOST") && checkPackNode (helpPtr, &defaultValid, 0, numSign))
                  {
                     redo = (askBoolean("Bad VIA node. Edit ?", 'Y') == 'Y');
                     error = 1;
                  } else
                  {  nodeNum.point = 0;
                     sscanf(helpPtr, "%hu:%hu/%hu.%hu",
                                      &nodeNum.zone, &nodeNum.net,
                                      &nodeNum.node, &nodeNum.point);
                     count = 0;
                     while (count < MAX_AKAS &&
                            memcmp(&nodeNum, &config.akaList[count].nodeNum, sizeof(nodeNumType)))
                     {  count++;
                     }
                     if (count < MAX_AKAS)
                     {
                        redo = (askBoolean("VIA node is local AKA. Edit ?", 'Y') == 'Y');
                        error = 1;
                     }
                  }
               }

               while (!error && (helpPtr = strtok(NULL, " ")) != NULL)
               {
                  if ((*(helpPtr++) != '/') ||
                       ((*helpPtr != 'C') &&
                        (*helpPtr != 'H') &&
                        (*helpPtr != 'I') &&
                        (*helpPtr != 'L') &&
                        (*helpPtr != 'O')) ||
                        ((*(++helpPtr) != ' ') &&
                         (*helpPtr != 0)))
                  {
                    redo = (askBoolean("Illegal switch. Edit ?", 'Y') == 'Y');
                    error = 1;
                  }
                  strtok(NULL, " ");
               }
            }
         }
         else

         if ((fieldType & MASK) == PATH || (fieldType & MASK) == FILE_NAME || (fieldType & MASK) == MB_NAME)
         {
            for (sPos = 0; sPos < strlen(tempStr); sPos++)
               if (tempStr[sPos] == '/')
                  tempStr[sPos] = '\\';

            strcpy(testStr, tempStr);
            if ((fieldType & MASK) == FILE_NAME)
            {
               if ((helpPtr = strrchr(testStr, '\\')) != NULL)
                  *helpPtr = 0;
               else
                  *testStr = 0;
            }
            else
            {
               if ((fieldType & MASK) == MB_NAME)
               {
                  helpPtr = strrchr(testStr, '\\');
//                if ( (!config.genOptions.lfn || !(fieldType & LFN)) && helpPtr != NULL && strchr(helpPtr, '.') != NULL )
                  if ( (                          !(fieldType & LFN)) && helpPtr != NULL && strchr(helpPtr, '.') != NULL )
                  {
                     error = 1;
                     redo = (askBoolean("File name may not contain an extension. Edit filename ?", 'Y') == 'Y');
                  }
                  else
                  {
                     if (helpPtr != NULL)
                       *(helpPtr + 1) = 0;
                  }
               }
               else
               {
                  if (sPos > 1)
                  {
                     if (tempStr[sPos-1] != '\\')
                     {
                        tempStr[sPos++] = '\\';
                     }
                     tempStr[sPos] = 0;
                  }
               }
               if ((*testStr) && (*(helpPtr = testStr+strlen(testStr)-1) == '\\'))
               {
                  *helpPtr = 0;
               }
            }
            if (  !error
               && *testStr
               && (strlen(testStr) > 2 || testStr[1] != ':')
               && !dirExist(testStr)
               )
            {
               if (askBoolean("Path not found. Create ?", 'N') == 'Y')
               {
                  helpPtr = tempStr;
                  while ((helpPtr = strchr(helpPtr, '\\')) != NULL)
                  {
                     *helpPtr = 0;
                     mkdir(tempStr);
                     *helpPtr++ = '\\';
                  }
                  if (!dirExist(testStr))
                  {
                     error = 1;
                     redo = askBoolean("Can't create subdirectory. Edit path ?", 'Y') == 'Y';
                  }
               }
               else
               {
                  error = 1;
                  if ((fieldType & MASK) == FILE_NAME)
                     redo = askBoolean("Edit filename ?", 'Y') == 'Y';
                  else
                     redo = askBoolean("Edit path ?", 'Y') == 'Y';
               }
            }
            if (  !redo
               && (fieldType & MASK) == FILE_NAME
               && dirExist(tempStr)
               )
            {
                error = 1;
                redo = (askBoolean("Missing filename. Edit filename ?", 'Y') == 'Y');
            }
         }
         else
         if ( (fieldType & MASK) == EMAIL )
         {
            error = 0;
            if ( *tempStr &&
                 ((helpPtr = strchr(tempStr, '@')) == NULL ||
                  helpPtr == tempStr || !*(helpPtr + 1) ||
                  strchr(helpPtr + 1, '@') != NULL) )
               ++error;
            if ( error )
               redo = (askBoolean("Illegal e-mail address. Edit ?", 'Y') == 'Y');
         }

         if ((!redo) && error)
         {
            *tempStr = 0;
            sPos = 0;
         }
         strcpy(string, tempStr);
      }
      update++;
   }
   while (redo);

   if (!error)
     strcpy(string, tempStr);

   return ch;
}
//---------------------------------------------------------------------------
void displayData(menuType *menu, u16 sx, u16 sy, s16 mark)
{
   u16      count,
            exportCount;
   u16      width = 0;
   u16      px, py;
   u16      lastZone,
            lastNet,
            lastNode;
   s16      temp;
   char     *helpPtr;
   char     tempStr[80],
            tempStr2[80];
   u16      attr, attr2 = 0;
   const char *NoYes[2] = { "No", "Yes" };

   if (sx + menu->xWidth >= 80 || menu->yWidth >= 25)
     return;

   if (sy + menu->yWidth >= 25)
     sy = 24 - menu->yWidth;

   py = sy;
   for (count = 0; count < menu->entryCount; count++)
   {
      if (menu->menuEntry[count].entryType & DISPLAY)
         attr = calcAttr(DARKGRAY, windowLook.background);
      else
      {
         attr  = calcAttr(windowLook.promptfg   , windowLook.background);
         attr2 = calcAttr(windowLook.promptkeyfg, windowLook.background);
      }

      if ((menu->menuEntry[count].entryType & MASK) != EXTRA_TEXT)
      {
         if (menu->menuEntry[count].offset)
            px = sx + menu->menuEntry[count].offset + 2;
         else
         {
            px = sx + 2;
            py++;
         }
         if (menu->menuEntry[count].prompt != NULL)
         {
            temp = (!mark) || (menu->menuEntry[count].entryType & NO_EDIT);

            helpPtr = menu->menuEntry[count].prompt;
            while (*helpPtr)
            {
               if (temp || (*helpPtr != menu->menuEntry[count].key))
                 showChar(px, py, *(helpPtr++), attr, MONO_NORM);
               else
               {
                 showChar(px, py, *(helpPtr++), attr2, MONO_HIGH);
                 temp = 1;
               }
               px++;
            }
         }
      }
   }

  py = sy;

  for (count = 0; count < menu->entryCount; count++)
  {
    u16 entryType = menu->menuEntry[count].entryType & MASK;
    if (menu->menuEntry[count].offset)
      px = sx+menu->menuEntry[count].offset + strlen(menu->menuEntry[count].prompt) + 4;
    else
    {
      px = sx+menu->pdEdge;
      py++;
    }

    switch (entryType)
    {
       case EXTRA_TEXT: py--;
                        lastZone = 0;
                        lastNet = 0;
                        lastNode = 0;
                        exportCount = 0;
                        *tempStr = 0;
                        while ((exportCount < MAX_FORWARD) && (tempInfo.forwards[exportCount].nodeNum.zone != 0))
                        {
                           strcpy(tempStr2, nodeStr (&tempInfo.forwards[exportCount].nodeNum));

                           if (lastZone != tempInfo.forwards[exportCount].nodeNum.zone)
                              helpPtr = tempStr2;
                           else
                           {
                              if (lastNet != tempInfo.forwards[exportCount].nodeNum.net)
                                 helpPtr = strchr(tempStr2, ':') + 1;
                              else
                              {
                                 if (lastNode != tempInfo.forwards[exportCount].nodeNum.node)
                                    helpPtr = strchr(tempStr2, '/') + 1;
                                 else
                                 {
                                    if (tempInfo.forwards[exportCount].nodeNum.point == 0)
                                       helpPtr = strchr(tempStr2, 0);
                                    else
                                       helpPtr = strchr(tempStr2, '.');
                                 }
                              }
                           }
                           lastZone = tempInfo.forwards[exportCount].nodeNum.zone;
                           lastNet  = tempInfo.forwards[exportCount].nodeNum.net;
                           lastNode = tempInfo.forwards[exportCount].nodeNum.node;

                           if (strlen (tempStr) + strlen (helpPtr) + 3 < ORGLINE_LEN)
                           {
                              if (strlen (tempStr) > 0)
                                 strcat (tempStr, " ");

                              strcat (tempStr, helpPtr);
                              exportCount++;
                           }
                           else
                           {
                              strcat (tempStr, " >");
                              exportCount = MAX_FORWARD;
                           }
                        }
                        width = ORGLINE_LEN-1;
                        break;
       case FMTEXT    :
       case WORD      :
       case EMAIL     :
       case ALPHA     :
       case ALPHA_AST :
       case PACK      :
       case PATH      :
       case MB_NAME   :
       case SFILE_NAME:
       case FILE_NAME : width = menu->menuEntry[count].par1;
                        strcpy(tempStr, menu->menuEntry[count].data);
                        break;
       case DATE      : width = 11;
                        if ( !*((u32*)menu->menuEntry[count].data) )
                           strcpy(tempStr, "n/a");
                        else
                        {  struct tm *tblock;

                           tblock = gmtime(((const time_t*)menu->menuEntry[count].data));
                           sprintf(tempStr, "%2u %s %u", tblock->tm_mday, months[tblock->tm_mon], tblock->tm_year + 1900);
                        }
                        break;
       case DATETIME  : width = 19;
                        if (!*((u32*)menu->menuEntry[count].data))
                          strcpy(tempStr, "n/a");
                        else
                          strcpy(tempStr, isoFmtTime(*(const time_t *)menu->menuEntry[count].data));
                        break;
       case FUNC_PAR  : if (*((funcParType*)menu->menuEntry[count].data)->f == askGroup)
                        {  sprintf (tempStr, "%c  %s",
                                    groupToChar(*(s32*)((funcParType*)menu->menuEntry[count].data)->numPtr),
                                    config.groupDescr[groupToChar(*(s32*)((funcParType*)menu->menuEntry[count].data)->numPtr)-'A']);
                           width = 32;
                        }
                        else
                        if (*((funcParType*)menu->menuEntry[count].data)->f == askGroups)
                        {  getGroupString (*(s32*)(((funcParType*)menu->menuEntry[count].data)->numPtr), tempStr);
                           width = 26;
                        }
                        else
                        if (*((funcParType*)menu->menuEntry[count].data)->f == multiAkaSelect)
                        {  helpPtr=tempStr;
                           for (temp = 0; temp < MAX_AKAS; temp++)
                           if ( *(u32*)(((funcParType*)menu->menuEntry[count].data)->numPtr) & ((u32)1<<temp) )
                              *helpPtr++ = (temp>=10)?'A'+temp-10:(temp)?'0'+temp:'M';
                           *helpPtr=0;
                           width = 12;
                        }
                        else
                        {
                           if (*((funcParType*)menu->menuEntry[count].data)->numPtr == 0)
                              sprintf(tempStr, "None");
                           else
                              sprintf(tempStr, "%-4u", *((funcParType*)menu->menuEntry[count].data)->numPtr);
                           width = 4;
                        }
                        break;
       case NUM_P_INT : if (*((s16*)menu->menuEntry[count].data) == 0)
                           *((s16*)menu->menuEntry[count].data) = 1;
       case NUM_INT   : if (*((s16*)menu->menuEntry[count].data) > menu->menuEntry[count].par2)
                           *((s16*)menu->menuEntry[count].data) =
                                   menu->menuEntry[count].par2;
                        width = menu->menuEntry[count].par1;
                        _ultoa((u16)*((s16*)menu->menuEntry[count].data), tempStr, 10);
                        tempStr[width] = 0;
                        break;
       case NUM_LONG  : width = 10;
                        _ultoa(*((s32*)menu->menuEntry[count].data), tempStr, 10);
                        break;
      case BOOL_INT    :
        width = 3;
        strcpy(tempStr, NoYes[(*(s16*)menu->menuEntry[count].data & menu->menuEntry[count].par1) ? 1 : 0]);
        break;
      case BOOL_INT_REV:
        width = 3;
        strcpy(tempStr, NoYes[(*(s16*)menu->menuEntry[count].data & menu->menuEntry[count].par1) ? 0 : 1]);
        break;
      case ENUM_INT  : { u16 teller = 0;
                         while ((teller < menu->menuEntry[count].par2) &&
                                (((toggleType*)menu->menuEntry[count].data)->retval[teller] !=
                                 *(u8*)(((toggleType*)menu->menuEntry[count].data)->data)))
                         {
                            teller++;
                         }
                         if (teller == menu->menuEntry[count].par2)
                         {
                            teller = 0;
                            *(u8*)(((toggleType*)menu->menuEntry[count].data)->data) = ((toggleType*)menu->menuEntry[count].data)->retval[0];
                         }
                         strcpy(tempStr, ((toggleType*)menu->menuEntry[count].data)->text[teller]);
                      }
                      width = menu->menuEntry[count].par1;
                      break;
      case NODE      : if (((nodeFakeType*)menu->menuEntry[count].data)->nodeNum.zone != 0)
                      {
                         strcpy(tempStr, nodeStr(menu->menuEntry[count].data));
                         if ((menu->menuEntry[count].par1 == FAKE) &&
                             (((nodeFakeType*)menu->menuEntry[count].data)->fakeNet != 0))
                         {
                            sprintf (tempStr+strlen(tempStr), "-%u",
                                     ((nodeFakeType*)menu->menuEntry[count].data)->fakeNet);
                         }
                      }
                      else
                         *tempStr = 0;
                      width = menu->menuEntry[count].par2;
                      break;
      case BIT_TOGGLE:
        temp = 1;
        *tempStr = 0;
        do
        {
          if ((unsigned char)*((char*)menu->menuEntry[count].data) & temp)
            strcat(tempStr, "x");
          else if ((char*)menu->menuEntry[count].data < (char*)&tempInfo.flagsTemplateQBBS &&
                  (unsigned char)*((char*)menu->menuEntry[count].data+4) & temp)
            strcat(tempStr, "o");
          else
            strcat(tempStr, "-");
        }
        while ((char)(temp <<= 1));
        break;
      default:
        *tempStr = 0;
        width = 0;
        break;
    }
    if (entryType != NEW_WINDOW)
      printStringFill(tempStr, ' ', width, px, py, windowLook.datafg, windowLook.background, MONO_NORM);
  }
}
//---------------------------------------------------------------------------
void fillRectangle(char ch, u16 sx, u16 sy, u16 ex, u16 ey, u16 fgc, u16 bgc, u16 mAttr)
{
  u16    count;
  u16    width;
  struct char_info line[80];

  testInit();

  if ((width = ex - sx + 1) > 80)
    return;

  u16 attr = calcAttr(fgc, bgc);
  for (count = 0; count < width; count++)
  {
    line[count].letter = ch;
    line[count].attr   = color ? attr : mAttr;
  }

  for (count = sy; count <= ey; count++)
    puttext(sx + 1, count + 1, ex + 1, count + 1, line);
}
//---------------------------------------------------------------------------
s16 displayWindow(const char *title, u16 sx, u16 sy, u16 ex, u16 ey)
{
  u16       x, y;
  u16       nx;
  u16       borderIndex;
  int       attr;
  screenCharType *helpPtr = NULL;

  testInit();

  if (windowSP >= MAX_WINDOWS)
    return 1;

  if (  !(windowLook.wAttr & NO_SAVE)
     && (helpPtr = malloc(sizeof(screenCharType) * (ex - sx + 3) * (ey - sy + 2))) == NULL
     )
  {
    displayMessage("Not enough memory available");
    return 1;
  }

  if (windowSP != 0)
  {
    attr = calcAttr(windowStack[windowSP - 1].inactvborderfg, windowStack[windowSP - 1].background);

    for (x = windowStack[windowSP - 1].sx; x < windowStack[windowSP - 1].ex - 1; x++)
    {
      if (getChar(x, windowStack[windowSP - 1].sy) >= 128)
        changeColor(x, windowStack[windowSP - 1].sy, attr, MONO_NORM);

      if (getChar(x, windowStack[windowSP - 1].ey - 1) >= 128)
        changeColor(x, windowStack[windowSP-1].ey-1, attr, MONO_NORM);
    }
    for (y = windowStack[windowSP - 1].sy; y < windowStack[windowSP - 1].ey; y++)
    {
      changeColor(windowStack[windowSP - 1].sx    , y, attr, MONO_NORM);
      changeColor(windowStack[windowSP - 1].ex - 2, y, attr, MONO_NORM);
    }
  }

  borderIndex = windowLook.wAttr & 0x03;
  if (!(windowLook.wAttr & NO_SAVE))
  {
    gettext(sx + 1, sy + 1, ex + 3, ey + 2, helpPtr);
    windowStack[windowSP].sx = sx;
    windowStack[windowSP].ex = ex + 2;
    windowStack[windowSP].sy = sy;
    windowStack[windowSP].ey = ey + 1;
    windowStack[windowSP].inactvborderfg = windowLook.inactvborderfg;
    windowStack[windowSP].background     = windowLook.background;
    windowStack[windowSP++].oldScreen    = helpPtr;
  }
  fillRectangle(' ', sx + 1, sy + 1, ex - 1, ey - 1, windowLook.editfg, windowLook.background, windowLook.mono_attr);
  attr = calcAttr(windowLook.actvborderfg, windowLook.background);
  for (x = sx + 1; x < ex; x++)
  {
    showChar (x, sy, border[borderIndex][0], attr, windowLook.mono_attr);
    showChar (x, ey, border[borderIndex][0], attr, windowLook.mono_attr);
  }
  for (y = sy + 1; y < ey; y++)
  {
    showChar (sx, y, border[borderIndex][1], attr, windowLook.mono_attr);
    showChar (ex, y, border[borderIndex][1], attr, windowLook.mono_attr);
  }
  showChar (sx, sy, border[borderIndex][2], attr, windowLook.mono_attr);
  showChar (ex, sy, border[borderIndex][3], attr, windowLook.mono_attr);
  showChar (sx, ey, border[borderIndex][4], attr, windowLook.mono_attr);
  showChar (ex, ey, border[borderIndex][5], attr, windowLook.mono_attr);
  if (title != NULL && strlen(title) > 0)
  {
    if (windowLook.wAttr & TITLE_RIGHT)
      nx = ex - strlen(title);
    else
      if (windowLook.wAttr & TITLE_LEFT)
        nx = sx + 1;
      else
        nx = sx + (ex-sx-strlen(title))/2;

    attr = calcAttr(windowLook.titlefg, windowLook.titlebg);

    while (*title)
    {
      showChar(nx, sy, *(title++), attr, windowLook.mono_attr);
      nx++;
    }
  }
  if (windowLook.wAttr & SHADOW)
  {
    for (x = ex + 1; x <= ex + 2; x++)
      for (y = sy + 1; y <= ey + 1; y++)
        shadow(x, y);
      for (x = sx + 2; x <= ex + 2; x++)
        shadow(x, ey + 1);
  }
  return 0;
}
//---------------------------------------------------------------------------
void removeWindow(void)
{
  u16             x, y;
  screenCharType *helpPtr;

  if (windowSP == 0 || (helpPtr = windowStack[--windowSP].oldScreen) == NULL)
    return;

#ifndef __32BIT__
  {
    u16 count, nx;
    nx = windowStack[windowSP].ex - windowStack[windowSP].sx + 1;
    count = 0;
    for (y =  windowStack[windowSP].sy; y <= windowStack[windowSP].ey; y++)
      memcpy(&(screen[y * columns + windowStack[windowSP].sx]), &helpPtr[nx * count++], nx << 1);
  }
#else
  puttext( windowStack[windowSP].sx + 1, windowStack[windowSP].sy + 1
         , windowStack[windowSP].ex + 1, windowStack[windowSP].ey + 1, helpPtr);
#endif
  free(helpPtr);

  if (windowSP != 0)
  {
    int attr = calcAttr(windowLook.actvborderfg, windowLook.background);

    for (x = windowStack[windowSP-1].sx; x < windowStack[windowSP-1].ex - 1; x++)
    {
      if (getChar(x, windowStack[windowSP - 1].sy) >= 128)
        changeColor(x, windowStack[windowSP - 1].sy, attr, MONO_HIGH);

      changeColor(x, windowStack[windowSP - 1].ey - 1, attr, MONO_HIGH);
    }
    for (y = windowStack[windowSP - 1].sy; y < windowStack[windowSP - 1].ey; y++)
    {
      changeColor(windowStack[windowSP - 1].sx    , y, attr, MONO_HIGH);
      changeColor(windowStack[windowSP - 1].ex - 2, y, attr, MONO_HIGH);
    }
  }
}
//---------------------------------------------------------------------------
menuType *createMenu(char *title)
{
  menuType *menu;

  if ((menu = malloc(sizeof(menuType))) == NULL)
    return NULL;

  menu->title      = title;
  menu->xWidth     = 2;
  menu->yWidth     = 2;
  menu->pdEdge     = 2;
  menu->zDataSize  = 0;
  menu->entryCount = 0;

  return menu;
}
//---------------------------------------------------------------------------
s16 addItem(menuType *menu, u16 entryType, char *prompt, u16 offset, void *data, u16 par1, u16 par2, const char *comment)
{
  u16 promptSize = 0;
  u16 dataSize   = 0;
  u16 count;

  if (menu->entryCount == MAX_ENTRIES)
    return -1;

  if (prompt == NULL)
    menu->menuEntry[menu->entryCount].key = 0;
  else
  {
    promptSize = strlen(prompt);
    count = 0;
    while (prompt[count] && !isupper(prompt[count]))
      count++;
    if (prompt[count])
      menu->menuEntry[menu->entryCount].key = prompt[count];
  }

  menu->menuEntry[menu->entryCount].entryType = entryType;
  menu->menuEntry[menu->entryCount].prompt    = prompt;
  menu->menuEntry[menu->entryCount].offset    = offset;
  menu->menuEntry[menu->entryCount].data      = data;
  menu->menuEntry[menu->entryCount].selected  = 0;
  menu->menuEntry[menu->entryCount].par1      = par1;
  menu->menuEntry[menu->entryCount].par2      = par2;
  menu->menuEntry[menu->entryCount].comment   = comment;

  switch (entryType & MASK)
  {
    case EXTRA_TEXT:
      case FMTEXT    :
      case WORD      :
      case EMAIL     :
      case ALPHA     :
      case ALPHA_AST :
      case PACK      :
      case PATH      :
      case MB_NAME   :
      case SFILE_NAME:
      case FILE_NAME :
      case NUM_INT   :
      case NUM_P_INT : dataSize = par1;
                       break;
      case DATE      : dataSize = 11;
                       break;
      case DATETIME  : dataSize = 19;
                       break;
      case NUM_LONG  : dataSize = 10;
                       break;
      case FUNC_PAR  : dataSize = par1;
                       break;
      case BOOL_INT_REV:
      case BOOL_INT  : dataSize = 3;
                       break;
      case ENUM_INT  : for (count = 0; count < par2; count++)
                         if (strlen((*(toggleType*)data).text[count]) > (size_t)dataSize)
                           dataSize = strlen((*(toggleType*)data).text[count]);
                       menu->menuEntry[menu->entryCount].par1 = dataSize;
                       break;
      case BIT_TOGGLE: dataSize = 9;
                       break;
      case NODE      : if (par1 == FAKE)
                       {
                          dataSize = 29;
                          menu->menuEntry[menu->entryCount].par2 = 30;
                       }
                       else
                       {
                          dataSize = 23;
                          menu->menuEntry[menu->entryCount].par2 = 24;
                       }
                       break;
  }

  if (offset)
    menu->xWidth = max(menu->xWidth, promptSize + dataSize + offset + (dataSize ? 6 : 4));
  else
  {
    if (dataSize)
      dataSize += 2;

    menu->zDataSize = max (menu->zDataSize, dataSize);
    menu->pdEdge    = max (menu->pdEdge, promptSize+4);
    menu->xWidth    = max (menu->xWidth, menu->pdEdge+menu->zDataSize);

    if ((entryType & MASK) != EXTRA_TEXT)
      menu->yWidth++;
  }
  menu->entryCount++;

  return 0;
}
//---------------------------------------------------------------------------
s16 displayMenu(menuType *menu, u16 sx, u16 sy)
{
  testInit();

  if (sx + menu->xWidth >= 80 || menu->yWidth >= 25)
    return 1;

  if (sy + menu->yWidth >= 25)
    sy = 24 - menu->yWidth;

  if (displayWindow(menu->title, sx, sy, sx + menu->xWidth - 1, sy + menu->yWidth - 1) != 0)
    return 1;

  displayData(menu, sx, sy, 1);

  return 0;
}
//---------------------------------------------------------------------------
static nodeNumType convertNodeStr(char *ns, u16 aka)
{
  nodeNumType tempNode;
  s16         temp;

  tempNode.point = 0;

  if (sscanf(ns, "%hu:%hu/%hu.%hu",
                 &tempNode.zone, &tempNode.net,
                 &tempNode.node, &tempNode.point) >= 3)
    return tempNode;

  if (config.akaList[aka].nodeNum.zone == 0)
  {
    if (config.akaList[aka=0].nodeNum.zone == 0)
    {
      displayMessage ("Main AKA not defined -> No default zone/net/node available");
      memset(&tempNode, 0, sizeof(nodeNumType));

      return tempNode;
    }
  }
  tempNode.zone = config.akaList[aka].nodeNum.zone;
  if (sscanf(ns, "%hu/%hu.%hu",
                 &tempNode.net, &tempNode.node, &tempNode.point) >= 2)
    return tempNode;

  tempNode.net = config.akaList[aka].nodeNum.net;
  if (  (temp = sscanf(ns, "%hu.%hu", &tempNode.node, &tempNode.point)) > 1
    || (temp == 1 && strchr(ns, ':') == 0 && strchr(ns, '/') == 0)
    )
    return tempNode;

  tempNode.node = config.akaList[aka].nodeNum.node;
  if (sscanf(ns, ".%hu", &tempNode.point) <= 0)
  {
    if (*ns != 0)
      displayMessage("Invalid node number");
    memset(&tempNode, 0, sizeof(nodeNumType));
  }
  return tempNode;
}
//---------------------------------------------------------------------------
nodeNumType getNodeNum (char *title,  u16 sx, u16 sy, u16 aka)
{
  char tempStr[25];

  *tempStr = 0;
  if (displayWindow(title, sx, sy, sx + 26, sy + 2) == 0)
  {
    editString(tempStr, 24, sx + 2, sy + 1, NODE);
    removeWindow();
  }

  return convertNodeStr(tempStr, aka);
}
//---------------------------------------------------------------------------
void processed (u16 updated, u16 total)
{
  char tempStr[64];

  sprintf(tempStr, "%u of %u selected records changed", updated, total);
  displayMessage(tempStr);
}
//---------------------------------------------------------------------------
extern funcParType multiAkaSelectRec;
extern u32         alsoSeenBy;
extern char        tempToggleRA;

s16 changeGlobal(menuType *menu, void *org, void *upd)
{
   u16 count;
   u16 update = 0;

   for (count = 0; count < menu->entryCount; count++)
   {
      if (menu->menuEntry[count].selected)
      {
         switch (menu->menuEntry[count].entryType & MASK)
         {  case FMTEXT:
            case WORD:
            case EMAIL:
            case ALPHA:
            case ALPHA_AST:
            case PATH:
            case FILE_NAME:
            case MB_NAME:
            case SFILE_NAME:  if (strncmp( menu->menuEntry[count].data, (char*)upd + (menu->menuEntry[count].data - org)
                                         , menu->menuEntry[count].par1 + 1))
                              {
                                update = 1;
                                strncpy( menu->menuEntry[count].data, (char*)upd + (menu->menuEntry[count].data - org)
                                       , menu->menuEntry[count].par1);
                              }
                              break;
            case NUM_INT:
            case NUM_P_INT:   if (*(u16*)(menu->menuEntry[count].data) != *((u16*)((u8*)upd + (int)(menu->menuEntry[count].data - org))))
                              {
                                update = 1;
                                *(u16*)menu->menuEntry[count].data = *((u16*)((u8*)upd + (int)(menu->menuEntry[count].data - org)));
                              }
                              break;
            case DATE:
            case DATETIME:
            case NUM_LONG:    if (*(u32*)menu->menuEntry[count].data != *((u32*)((u8*)upd + (int)(menu->menuEntry[count].data - org))))
                              {
                                update = 1;
                                *(u32*)menu->menuEntry[count].data = *((u32*)((u8*)upd + (int)(menu->menuEntry[count].data - org)));
                              }
                              break;
            case ENUM_INT:
                              if ((char*)((toggleType*)menu->menuEntry[count].data)->data == &tempToggleRA)
                              {
                                 if (tempToggleRA)
                                 {
                                   if (!(tempInfo.attrRA & BIT3))
                                   {
                                     update = 1;
                                     tempInfo.attrRA |= BIT3;
                                   }
                                 }
                                 else
                                 {  if ( tempInfo.attrRA & BIT3 )
                                    {  update = 1;
                                       tempInfo.attrRA &= ~BIT3;
                                    }
                                 }
                                 if (tempToggleRA == 1)
                                 {  if ( !(tempInfo.attrRA & BIT5) )
                                    {  update = 1;
                                       tempInfo.attrRA |= BIT5;
                                    }
                                 }
                                 else
                                 {  if ( tempInfo.attrRA & BIT5 )
                                    {  update = 1;
                                       tempInfo.attrRA &= ~BIT5;
                                    }
                                 }
                                 break;
                              }
                              if ( *(u8*)(((toggleType*)menu->menuEntry[count].data)->data) !=
                                   *(u8*)(upd + ((((toggleType*)menu->menuEntry[count].data)->data) - org)) )
                              {
                                update = 1;
                                *(u8*)(((toggleType*)menu->menuEntry[count].data)->data) =
                                  *(u8*)(upd + ((((toggleType*)menu->menuEntry[count].data)->data) - org));
                              }
                              break;
            case NODE:
            case BIT_TOGGLE:  if (memcmp( menu->menuEntry[count].data
                                        , (u8*)upd+(int)((u8*)menu->menuEntry[count].data-(u8*)org), 8))
                              {
                                update = 1;
                                memcpy( menu->menuEntry[count].data
                                      , (u8*)upd+(int)((u8*)menu->menuEntry[count].data-(u8*)org), 8);
                              }
                              break;
            case BOOL_INT_REV:
            case BOOL_INT:    if ( ((*((u16*)menu->menuEntry[count].data)) & menu->menuEntry[count].par1) !=
                                   ((*((u16*)((u8*)upd+(int)((u8*)menu->menuEntry[count].data-(u8*)org)))) & menu->menuEntry[count].par1) )
                              {
                                update = 1;
                                *((u16*)menu->menuEntry[count].data) ^= menu->menuEntry[count].par1;
                              }
                              break;
            case NEW_WINDOW:
            case FUNCTION:
            case FUNC_VPAR:   break;
            case FUNC_PAR:    // Q&D
                              if (menu->menuEntry[count].data == &multiAkaSelectRec)
                              {
                                 if (((rawEchoType*)org)->alsoSeenBy != alsoSeenBy)
                                 {
                                    update = 1;
                                    ((rawEchoType*)org)->alsoSeenBy = alsoSeenBy;
                                 }
                              }
                              break;
            case AKA_SELECT:
            case NODE_MATCH:
            case WSELECT:
            case EXTRA_TEXT:  displayMessage("Unexpected code");
                              break;
            default:          displayMessage("Undefined code");
                              break;
         }
      }
   }
   return update;
}
//---------------------------------------------------------------------------
static u16 aboutTable[5] = {_K_ALTA_, _K_ALTB_, _K_ALTO_, _K_ALTU_, _K_ALTT_};
extern u16 am__cp;
       u16 editDefault;

s16 runMenuDE(menuType *menu, u16 sx, u16 sy, u16 *dataPtr, u16 setdef, u16 esc)
{
   u16          count
              , count2
              , offset
              , par2;
   u16          px
              , py
              , editX;
   u16          attr
              , attr2;
   s16          more;
   tempStrType  tempStr
              , tempStr2;
   u16          ch;
   u16          update = 0;
   char        *helpPtr;
   u16          aboutIndex;
   u16          maxEntryIndex;

#ifdef _DEBUG0
  sprintf(tempStr, " %lu ", coreleft());
  printString(tempStr, 70, 2, YELLOW, RED, MONO_HIGH);
#endif

  if (sx + menu->xWidth >= 80 || menu->yWidth >= 25)
    return 1;
  if (sy + menu->yWidth >= 25)
    sy = 24 - menu->yWidth;

  if (displayMenu(menu, sx, sy) != 0)
    return 0;

  if (setdef)
  {
    printString("Change global", 4, sy, windowLook.titlefg, windowLook.titlebg, windowLook.mono_attr);
    attr = calcAttr(windowLook.promptfg, windowLook.background);
    py = sy + 1;
    for (count = 0; count < menu->entryCount; count++)
    {
      if (menu->menuEntry[count].offset != 0)
        py--;
      showChar( sx + 1 + menu->menuEntry[count].offset, py
              , menu->menuEntry[count].selected ? '*' : ' ', attr, windowLook.mono_attr);
      py++;
    }
  }

  count = dataPtr != NULL ? *dataPtr : 0;

  while ((menu->menuEntry[count].entryType & NO_EDIT) && count < menu->entryCount)
    count++;

  if (count == menu->entryCount)
  {
    do
    {
      ch = readKbd();
    }
    while (ch != _K_ESC_);

    removeWindow();

    return update;
  }

  do
  {
    py = sy + 1;
    for (count2 = 0; count2 < count; count2++)
      if (menu->menuEntry[count2].offset == 0)
        py++;

    if (menu->menuEntry[count].offset)
      py--;
/*--- display asterisks */
    attr = calcAttr(windowLook.promptfg, windowLook.background);
    if (menu->menuEntry[count].selected )
      showChar(sx + 1 + menu->menuEntry[count].offset, py, '*', attr, windowLook.mono_attr);
    else
      showChar(sx + 1 + menu->menuEntry[count].offset, py, ' ', attr, windowLook.mono_attr);

    attr = calcAttr(windowLook.scrollfg, windowLook.scrollbg);
    if (menu->menuEntry[count].offset)
    {
         editX = sx + menu->menuEntry[count].offset +
                      strlen(menu->menuEntry[count].prompt) + 4;

         for ( px = sx + 1u + menu->menuEntry[count].offset
             ; (size_t)px < sx + 3u + menu->menuEntry[count].offset + strlen(menu->menuEntry[count].prompt); px++)
            changeColor(px, py, attr, MONO_INV);
    }
    else
    {
         editX = sx + menu->pdEdge;
         for (px = sx + 1; px < editX - 1; px++)
            changeColor(px, py, attr, MONO_INV);
    }
      printStringFill(menu->menuEntry[count].comment, ' ', 79, 0, 24, windowLook.commentfg, windowLook.commentbg, MONO_NORM);
      aboutIndex = 0;
      do
      {
        ch = readKbd();
      } while (ch == aboutTable[aboutIndex] && ++aboutIndex < 5);

      if (aboutIndex == 5)
        displayMessage("This program was compiled on "__DATE__);

      if ( setdef && (menu->menuEntry[count].entryType & MASK) != FUNCTION )
      {
        update = 1;
        if ( ch == _K_INS_ )
          menu->menuEntry[count].selected = 1;
        else
          if ( ch == _K_DEL_ )
            menu->menuEntry[count].selected = 0;
      }
      if (  ch == _K_ENTER_
         || ((menu->menuEntry[count].entryType & MASK) == ENUM_INT   && ch == 8               )
         || ((menu->menuEntry[count].entryType & MASK) == BIT_TOGGLE && ch >= '1' && ch <= '8')
         )
      {
         if (setdef && (menu->menuEntry[count].entryType & MASK) != FUNCTION)
           menu->menuEntry[count].selected = 1;

         switch (menu->menuEntry[count].entryType & MASK)
         {
            case FMTEXT     :
            case WORD       :
            case EMAIL      :
            case ALPHA      :
            case ALPHA_AST  :
            case PACK       :
            case PATH       :
            case MB_NAME    :
            case SFILE_NAME :
            case FILE_NAME  : helpPtr = menu->menuEntry[count].data;
                              strcpy(tempStr, helpPtr);
                              for (;;)
                              {
                                 editString(helpPtr, menu->menuEntry[count].par1 + 1, editX, py, menu->menuEntry[count].entryType);

                                 if ( (menu->menuEntry[count].entryType & MASK) == MB_NAME
                                    && helpPtr == tempInfo.msgBasePath
                                    && *tempInfo.msgBasePath
                                    )
                                 {
                                    for (count2 = 0; count2 < areaInfoCount; ++count2)
                                       if (!stricmp(tempInfo.msgBasePath, areaInfo[count2]->msgBasePath))
                                          break;
                                    if (count2 < areaInfoCount && stricmp(tempInfo.areaName, areaInfo[count2]->areaName))
                                    {
                                       if (askBoolean("Message base name already used. Edit ?", 'Y') == 'Y')
                                          continue;
                                       *tempInfo.msgBasePath = 0;
                                    }
                                 }

                                 if (  (menu->menuEntry[count].entryType & MASK) != SFILE_NAME
                                    || !*helpPtr
                                    )
                                    break;

                                 if (NULL == strpbrk(helpPtr, ":\\/"))  // Check for path chars
                                 {
                                    strcpy(stpcpy(tempStr2, configPath), helpPtr);
                                    if (isFile(tempStr2))
                                        break;
                                 }
                                 else
                                   if (isFile(helpPtr))
                                      break;

                                 if (askBoolean("File not found. Continue editing ?", 'Y') != 'Y')
                                 {
                                    strcpy(helpPtr, tempStr);
                                    break;
                                 }
                              }

                              // PATCH
                              if (menu->menuEntry[count].data == tempInfo.msgBasePath)
                              {
                                 if (*tempInfo.msgBasePath)
                                 {
                                    if (editDefault)
                                       tempInfo.board = 2;
                                    else
                                    {  // clear extern boardCodeInfo
                                       if (tempInfo.board && tempInfo.board-- <= MBBOARDS)
                                           boardCodeInfo[tempInfo.board>>3] &= ~(1<<(tempInfo.board&7));

                                       tempInfo.board = 0;
                                    }
                                 }
                                 if (!editDefault && !am__cp && !es__8c && *tempStr && strcmp(tempInfo.msgBasePath, tempStr))
                                 {
                                    askRemoveJAM(tempStr);
                                    if (!*tempInfo.msgBasePath)
                                       tempInfo.boardNumRA = 0;
                                 }
                              }

                              if (menu->menuEntry[count].data == tempInfoN.pktOutPath)
                              {
#if 0
                                 if (*tempInfoN.pktOutPath == 0 && tempInfoN.archiver == 0xFF)
                                    tempInfoN.archiver = config.defaultArc;
                                 else
#endif
                                    if (*tempInfoN.pktOutPath)
                                    {
#if 0
                                       tempInfoN.archiver = 0xFF;
#endif
                                       *tempInfoN.email = 0;
                                    }
                              }
                              if (menu->menuEntry[count].data == tempInfoN.email)
                              {
                                 if (*tempInfoN.email)
                                 {
                                    *tempInfoN.pktOutPath = 0;
                                    if (tempInfoN.archiver == 0xFF)
                                       tempInfoN.archiver = config.defaultArc;
                                 }
                              }
                              update = 1;
                              break;
            case NUM_P_INT  :
            case NUM_INT    : _ultoa((u16)*((s16*)menu->menuEntry[count].data), tempStr, 10);
                   /*ch =*/   editString (tempStr,
                                          menu->menuEntry[count].par1+1,
                                          editX, py,
                                          menu->menuEntry[count].entryType);
                              *(s16*)menu->menuEntry[count].data = atoi (tempStr);
                              // kludge
                              if ( (u16*)menu->menuEntry[count].data == &config.maxForward && config.maxForward < 64 )
                                 config.maxForward = 64;
                              if ( (u16*)menu->menuEntry[count].data == &config.maxMsgSize && config.maxMsgSize < 45 )
                                 config.maxMsgSize = 45;
                              update = 1;
                              break;
            case NUM_LONG   : _ultoa(*((s32*)menu->menuEntry[count].data), tempStr, 10);
                              editString(tempStr, 11, editX, py, menu->menuEntry[count].entryType);
                              *(s32*)menu->menuEntry[count].data = atol (tempStr);
                              update = 1;
                              break;
            case BOOL_INT_REV:
            case BOOL_INT   : *(s16*)menu->menuEntry[count].data ^= menu->menuEntry[count].par1;
                              // kludge
                              if (  (nodeOptionsType*)menu->menuEntry[count].data == &tempInfoN.options
                                 && (menu->menuEntry[count].par1 == BIT4)
                                 && ((nodeOptionsType*)menu->menuEntry[count].data)->active
                                 )
                              {
                                tempInfoN.referenceLNBDat = 0;
                              }
                              update = 1;
                              break;
            case ENUM_INT:
            {
               u16       teller;
               menuType *tempMenu;
               u16       tempvar;

               if ((tempMenu = createMenu ("")) == NULL)
                  break;
               tempvar = par2 = menu->menuEntry[count].par2;
               if (tempvar > 16)
                  tempvar = (tempvar + 1) / 2;
               for (teller = 0; teller < tempvar; teller++)
               {
                  addItem(tempMenu, WSELECT,
                          (*(toggleType*)menu->menuEntry[count].data).text[teller],
                          0, NULL, 0, 0, menu->menuEntry[count].comment);
                  if ( teller + tempvar < par2 )
                     addItem(tempMenu, WSELECT,(*(toggleType*)menu->menuEntry[count].data).text[teller + tempvar]
                            , 24, NULL, 0, 0, menu->menuEntry[count].comment);
               }
               tempvar = 0;
               while ( tempvar < par2 &&
                       (((toggleType*)menu->menuEntry[count].data)->retval[tempvar] !=
                         *(u8*)(((toggleType*)menu->menuEntry[count].data)->data)))
               {
                  tempvar++;
               }
               if (tempvar == par2)
               {
                  tempvar = 0;
                  *(u8*)(((toggleType*)menu->menuEntry[count].data)->data) = ((toggleType*)menu->menuEntry[count].data)->retval[0];
               }
               if (par2 > 16)
                  tempvar = (tempvar*2)-((tempvar>=(par2+1)/2)?par2-1+(par2&1):0);
               if (runMenuD(tempMenu, px, py - 1, &tempvar, 0))
               {
                  if (par2 > 16)
                     tempvar = ((tempvar&1)?(par2+1)/2:0)+tempvar/2;
                  *(u8*)(((toggleType*)menu->menuEntry[count].data)->data) = ((toggleType*)menu->menuEntry[count].data)->retval[tempvar];
                  update = 1;
               }
               free(tempMenu);
               break;
            }
            case WSELECT    : removeWindow();
                              if (dataPtr != NULL)
                                 *dataPtr = count;
                              update = 1;
                              return update;
            case NODE       : *tempStr = 0;
                              if (((nodeFakeType*)menu->menuEntry[count].data)->nodeNum.zone != 0)
                              {
                                 strcpy(tempStr, nodeStr(menu->menuEntry[count].data));
                                 if ((menu->menuEntry[count].par1 == FAKE) &&
                                     (((nodeFakeType*)menu->menuEntry[count].data)->fakeNet != 0))
                                 {
                                    sprintf(tempStr+strlen(tempStr), "-%u",
                                            ((nodeFakeType*)menu->menuEntry[count].data)->fakeNet);
                                 }
                              }
                     /*ch =*/ editString (tempStr, menu->menuEntry[count].par2,
                                          editX, py, menu->menuEntry[count].entryType);
                              if ((helpPtr = strchr(tempStr, '-')) != NULL)
                              {
                                 if (menu->menuEntry[count].par1 == FAKE)
                                    (*(nodeFakeType*)menu->menuEntry[count].data).fakeNet = atoi (helpPtr+1);
                                 *helpPtr = 0;
                              }
                              else
                              {
                                 if (menu->menuEntry[count].par1 == FAKE)
                                    (*(nodeFakeType*)menu->menuEntry[count].data).fakeNet = 0;
                              }
                              *(nodeNumType*)menu->menuEntry[count].data =
                                  convertNodeStr (tempStr, 0);

                              update = 1;
                              break;
            case BIT_TOGGLE : if (ch != _K_ENTER_)
                              {
                                 if ( config.bbsProgram != BBS_QBBS &&
                                      (char*)menu->menuEntry[count].data < (char*)&tempInfo.flagsTemplateQBBS )
                                 {
                                    if ( *(char*)menu->menuEntry[count].data & (1 << (ch - '1')) )
                                    {  *(char*)menu->menuEntry[count].data &= ~(1 << (ch - '1'));
                                       *((char*)menu->menuEntry[count].data+4) |= (1 << (ch - '1'));
                                    }
                                    else if ( *((char*)menu->menuEntry[count].data+4) & (1 << (ch - '1')) )
                                    {  *(char*)menu->menuEntry[count].data &= ~(1 << (ch - '1'));
                                       *((char*)menu->menuEntry[count].data+4) &= ~(1 << (ch - '1'));
                                    }
                                    else
                                       *(char*)menu->menuEntry[count].data |= 1 << (ch - '1');
                                 }
                                 else
                                    *(char*)menu->menuEntry[count].data ^= 1 << (ch - '1');
                                 update = 1;
                              }
                              break;
            case FUNCTION   : update |= ((function)menu->menuEntry[count].data) ();
                              break;
            case FUNC_PAR   : update |= ((functionPar)((funcParType*)menu->menuEntry[count].data)->f) (((funcParType*)menu->menuEntry[count].data)->numPtr);
                              break;
            case FUNC_VPAR  : update |= ((functionVPar)menu->menuEntry[count].data) (menu->menuEntry[count].par1);
                              break;
            case NEW_WINDOW : update |= runMenuD(menu->menuEntry[count].data,
                                                 sx+menu->menuEntry[count].par1,
                                                 sy+menu->menuEntry[count].par2, NULL, setdef);
                              break;
         }
         displayData(menu, sx, sy, 1);
#ifdef _DEBUG0
         sprintf (tempStr, " %lu ", coreleft());
         printString (tempStr, 70, 2, YELLOW, RED, MONO_HIGH);
#endif
      }
      if ((ch == _K_HOME_)  || (ch == _K_UP_) ||
          (ch == _K_LEFT_)  || (ch == _K_RIGHT_) ||
          (ch == _K_END_)   || (ch == _K_DOWN_) ||
          (ch == _K_CPGUP_) || (ch == _K_CPGDN_) ||
          (ch < 256 && isgraph(ch)) || (ch == _K_ESC_))
      {
         attr  = calcAttr(windowLook.promptfg   , windowLook.background);
         attr2 = calcAttr(windowLook.promptkeyfg, windowLook.background);
         more = 0;
         if (menu->menuEntry[count].offset)
         {
            for ( px = sx + 1u + menu->menuEntry[count].offset
                ; (size_t)px < sx + 3u + menu->menuEntry[count].offset + strlen(menu->menuEntry[count].prompt); px++)
            if (more || (getChar(px, py) != menu->menuEntry[count].key))
              changeColor(px, py, attr, MONO_NORM);
            else
            {
              changeColor(px, py, attr2, MONO_HIGH);
              more = 1;
            }
         }
         else
         {
            for (px = sx+1; px < sx+menu->pdEdge-1; px++)
            if (more || (getChar(px, py) != menu->menuEntry[count].key))
              changeColor (px, py, attr, MONO_NORM);
            else
            {
              changeColor (px, py, attr2, MONO_HIGH);
              more = 1;
            }
         }
         maxEntryIndex = menu->entryCount-1;
         if (ch < 256 && isgraph(ch))
         {
            ch = toupper(ch);
            count2 = count;
            do
            {
               do /* Right */
               {
                  if (count < maxEntryIndex)
                     count++;
                  else
                     count = 0;
               }
               while (menu->menuEntry[count].entryType & NO_EDIT);
            }
            while (count2 != count && menu->menuEntry[count].key != ch);
         } else
         switch (ch)
         {
            case _K_LEFT_ :
                          do /* Left */
                          {
                             if (count > 0)
                             {
                                count--;
                             }
                             else
                             {
                                count = maxEntryIndex;
                             }
                          }
                          while (menu->menuEntry[count].entryType & NO_EDIT);
                          break;
            case _K_RIGHT_ :
                          do /* Right */
                          {
                             if (count < maxEntryIndex)
                                count++;
                             else
                                count = 0;
                          }
                          while (menu->menuEntry[count].entryType & NO_EDIT);
                          break;
            case _K_UP_ : count2 = count; /* Up */
                          while (/* (menu->menuEntry[count].entryType & NO_EDIT) || */
                                 (menu->menuEntry[count].offset))
                          {
                             if (count > 0)
                                count--;
                             else
                                count = maxEntryIndex;
                          }
                          do
                          {  if (count > 0)
                                count--;
                             else
                                count = maxEntryIndex;
                          }
                          while ((menu->menuEntry[count].entryType & NO_EDIT) ||
                                 (menu->menuEntry[count].offset > menu->menuEntry[count2].offset));
                          break;
            case _K_DOWN_ :
                          offset = menu->menuEntry[count].offset;
                          count2 = count;
                          count = 0xffff;
                          do
                          {
                             if (count2 < maxEntryIndex)
                                count2++;
                             else
                                count2 = 0;
                          }
                          while ((menu->menuEntry[count2].offset));
                          if (!(menu->menuEntry[count2].entryType & NO_EDIT))
                          {
                             count = count2;
                          }
                          if (count2 < maxEntryIndex)
                             count2++;
                          else
                             count2 = 0;

                          while ((count == 0xffff) ||
                                 (menu->menuEntry[count2].offset != 0))
                          {
                             if ((!(menu->menuEntry[count2].entryType & NO_EDIT)) &&
                                 (menu->menuEntry[count2].offset <= offset))
                             {
                                count = count2;
                             }

                             if (count2 < maxEntryIndex)
                                count2++;
                             else
                                count2 = 0;
                          }
                          break;
            case _K_CPGUP_:
            case _K_HOME_ :
                          count = 0; /* First */
                          while (menu->menuEntry[count].entryType & NO_EDIT)
                          {
                             count++;
                          }
                          break;
            case _K_CPGDN_:
            case _K_END_ :
                          count = maxEntryIndex; /* Last */
                          while (menu->menuEntry[count].entryType & NO_EDIT)
                          {
                             count--;
                          }
                          break;
      }
    }
  }
  while (ch != _K_ESC_ && ch != _K_F7_ && ch != _K_F10_);

  removeWindow();

  if (ch != _K_ESC_ && dataPtr != NULL)
    *dataPtr = count;

  return update;
}
//---------------------------------------------------------------------------
void printChar(char ch, u16 sx, u16 sy, u16 fgc, u16 bgc, u16 mAttr)
{
  showChar(sx, sy, ch, calcAttr(fgc, bgc), mAttr);
}
//---------------------------------------------------------------------------
void printString(const char *string, u16 sx, u16 sy, u16 fgc, u16 bgc, u16 mAttr)
{
  if (string != NULL)
  {
#ifndef __32BIT__
    u16 attr = calcAttr(fgc, bgc);

    while (*string)
    {
      showChar(sx, sy, *(string++), attr, mAttr);
      sx++;
    }
#else
    gotoxy(sx + 1, sy + 1);
    textattr(color ? calcAttr(fgc, bgc) : mAttr);
    cputs(string);
#endif
  }
}
//---------------------------------------------------------------------------
void displayMessage(const char *msg)
{
  u16 sx = (76 - strlen(msg)) / 2;

  fillRectangle(' ', 0, 24, 78, 24, BLACK, BLACK, MONO_NORM);
  if (displayWindow(NULL, sx, 9, sx + strlen(msg) + 3, 13) == 0)
  {
    printString(msg, sx + 2, 11, windowLook.promptfg, windowLook.background, MONO_NORM);
    readKbd();
    removeWindow();
  }
}
//---------------------------------------------------------------------------
char *getStr(const char *title, const char *init)
{
  static char str[65];
  strncpy(str, init, 64);
  str[64] = 0;
  if (displayWindow(title, 6, 12, 72, 14) == 0)
  {
    if (_K_ESC_ == editString(str, 64, 8, 13, NOCLEAR | FMTEXT))
      *str = 0;
    removeWindow();
  }
  return str;
}
//---------------------------------------------------------------------------
char fileNameStr[65];

char *getSourceFileName(char *title)
{
   *fileNameStr = 0;
   if (displayWindow(title, 6, 12, 72, 14) == 0)
   {
      editString(fileNameStr, 64, 8, 13, FILE_NAME | UPCASE);
      removeWindow();
      if (strcmp(fileNameStr, "CON") == 0)
      {
         displayMessage("Can't read from or write to the console");
         *fileNameStr = 0;
      }
      if (strcmp(fileNameStr, "CLOCK$") == 0)
      {
         displayMessage("Can't read from oR write to the clock device");
         *fileNameStr = 0;
      }
   }
   return fileNameStr;
}
//---------------------------------------------------------------------------
char *getDestFileName(char *title)
{
  char drive[_MAX_DRIVE];
  char dir  [_MAX_DIR  ];
  char file [_MAX_FNAME];
  char ext  [_MAX_EXT  ];

  getSourceFileName(title);

  if (*fileNameStr)
  {
    _splitpath(fileNameStr, drive, dir, file, ext);
    if (  strcmp(file, "FMAIL") == 0
       && (  strcmp(ext, ".AR" ) == 0 || strcmp(ext, ".NOD") == 0
          || strcmp(ext, ".PCK") == 0 || strcmp(ext, ".CFG") == 0
          || strcmp(ext, ".DUP") == 0 || strcmp(ext, ".LOG") == 0
          )
       )
    {
      displayMessage("Can't write to FMail system files");
      *fileNameStr = 0;
    }
  }
  if (  *fileNameStr
     && access(fileNameStr, 0) == 0
     && askBoolean("File already exists. Overwrite ?", 'N') != 'Y'
     )
  {
    *fileNameStr = 0;
  }
  return fileNameStr;
}
//---------------------------------------------------------------------------
int askChar(char *prompt, char *keys)
{
   char *helpPtr;
   int   ch;
   u16   sx = (76 - strlen(prompt)) / 2;

   if (displayWindow(NULL, sx, 9, sx + strlen(prompt) + 3, 13) != 0)
     return 0;

   printString(prompt, sx + 2, 11, windowLook.atttextfg, windowLook.background, MONO_HIGH);
   do
   {
     ch = toupper(readKbd());
   }
   while (ch != _K_ESC_ && (helpPtr = strchr(keys, ch)) == NULL);
   removeWindow();
   if (ch == _K_ESC_)
      return 0;

   return *helpPtr;
}



int askBoolean(char *prompt, int dfault)
{
   int ch;
   u16 sx = (76 - strlen(prompt)) / 2;

   if (displayWindow(NULL, sx, 9, sx + strlen(prompt) + 3, 14) == 0)
   {
      printString(prompt, sx + 2, 11, windowLook.atttextfg, windowLook.background, MONO_HIGH);
      if (dfault == 'Y')
         printString("[Y/n]", 37, 12, windowLook.atttextfg, windowLook.background, MONO_HIGH);
      else
         printString("[y/N]", 37, 12, windowLook.atttextfg, windowLook.background, MONO_HIGH);
      do
      {
         ch = toupper(readKbd());
      }
      while (ch != _K_ENTER_ && ch != _K_ESC_ && ch != 'Y' && ch != 'N');

      removeWindow();
      if (ch != _K_ENTER_)
         return ch;
   }
   return dfault;
}
//---------------------------------------------------------------------------
void working(void)
{
  printStringFill("Working...", ' ', 79, 0, 24, LIGHTRED, BLACK | COLOR_BLINK, MONO_NORM_BLINK);
}
//---------------------------------------------------------------------------
char displayAreasArray[MBBOARDS];
s16  displayAreasSelect;
extern s16 boardEdit;

s16 displayAreas(void)
{
   s16      count;
   u16      x = 0,
            y = 0;
   u16      ch;
   char     boardStr[5];

   boardEdit = 1;

   if (displayAreasSelect > 0 && displayAreasSelect <= MBBOARDS)
      displayAreasArray[displayAreasSelect - 1] = 0;

   if (displayWindow(" Available boardnumbers ", 4, 7, 76, 21) != 0)
   {
      return 0;
   }

   for (count = 0; count < MBBOARDS; count++)
   {
      sprintf(boardStr, "%3u", count + 1);
      if (displayAreasArray[count])
      {
         printString(boardStr, 7+(4*x++), 9+y, DARKGRAY, windowLook.background, MONO_NORM);
      }
      else
      {
         printString(boardStr, 7+(4*x++), 9+y, WHITE, windowLook.background, MONO_HIGH);
      }
      if (x == 17)
      {
         x = 0;
         y++;
      }
   }
   printString("None", 59, 20, WHITE, windowLook.background, MONO_HIGH);

   printStringFill("Select board number (None = Don't import messages into the message base)", ' '
                  , 79, 0, 24, windowLook.commentfg, windowLook.commentbg, MONO_NORM);

   if ((count = displayAreasSelect-1) == -1)
     count = MBBOARDS;

   if (count < 0 || count > MBBOARDS)
     count = 0;

   if (count < MBBOARDS && displayAreasArray[count])
   {
     count = 0;
     while (displayAreasArray[count] && count < MBBOARDS)
       count++;
   }

   do
   {
      if (count == MBBOARDS)
        strcpy(boardStr, "None");
      else
        sprintf(boardStr, "%3u", count + 1);

      printString(boardStr, 7 + (4 * (count % 17)), 9 + (count / 17), windowLook.scrollfg, windowLook.scrollbg, MONO_HIGH_BLINK);

      ch = readKbd();

      printString(boardStr, 7+(4*(count%17)), 9+(count/17), WHITE, windowLook.background, MONO_HIGH);

      if (ch >= '0' && ch <= '9')
      {
          boardStr[0] = ch;
          boardStr[1] = 0;
          displayWindow("", 35, 12, 55, 14);
          printString("Board number", 37, 13, windowLook.promptfg, windowLook.background, MONO_NORM);
          if (editString(boardStr, 4, 51, 13, NUM_INT|NOCLEAR) != _K_ESC_)
          {
             count = atoi(boardStr);
             if (count >= 1 && count <= MBBOARDS)
                count--;
             else
                count = MBBOARDS;
             if (count == MBBOARDS || !displayAreasArray[count])
                ch = _K_ENTER_;
             else
             {
                while ((count < MBBOARDS) && displayAreasArray[count])
                   count++;
             }
          }
          removeWindow();
      }
      else
        switch (ch)
        {
         case _K_LEFT_ :
                       do
                       {
                          if (count-- == 0)
                             count = MBBOARDS;
                       }
                       while ((count != MBBOARDS) && displayAreasArray[count]);
                       break;
         case _K_RIGHT_ :
                       do
                       {
                          if (++count == MBBOARDS + 1)
                             count = 0;
                       }
                       while ((count != MBBOARDS) && displayAreasArray[count]);
                       break;
         case _K_UP_ : do
                       {
                         count -= 17;
                         if (count < 0)
                         {
                            if (count < -2)
                               count += MBBOARDS + 3;
                            else
                               count += MBBOARDS - 14;
                       } }
                       while ((count != MBBOARDS) && displayAreasArray[count]);
                       break;
         case _K_DOWN_ :
                       do
                       {
                          count += 17;
                          if (count > MBBOARDS)
                          {
                             if (count >= MBBOARDS + 3)
                                count -= MBBOARDS + 3;
                             else
                                count -= MBBOARDS - 14;
                          }
                       }
                       while ((count != MBBOARDS) && displayAreasArray[count]);
                       break;
         case _K_CPGUP_ :
         case _K_HOME_ :
                       count = 0;
                       while ((count < MBBOARDS ) && displayAreasArray[count])
                         count++;

                       break;
         case _K_CPGDN_ :
         case _K_END_ :
                       count = MBBOARDS;
                       break;
      }
   }
   while ((ch != _K_ENTER_) && (ch != _K_ESC_));

   removeWindow();

   if (ch == _K_ENTER_)
   {
      if (count == MBBOARDS)
         displayAreasSelect = 0;
      else
      {
         displayAreasSelect = count+1;
         *tempInfo.msgBasePath = 0;
      }
      return (1);
   }
   return (0);
}
//---------------------------------------------------------------------------
static windowLookType orgWindowLook;

void saveWindowLook(void)
{
  memcpy(&orgWindowLook, &windowLook, sizeof(windowLookType));
}
//---------------------------------------------------------------------------
void restoreWindowLook(void)
{
  memcpy(&windowLook, &orgWindowLook, sizeof(windowLookType));
}
//---------------------------------------------------------------------------
void askRemoveDir(char *path)
{
  char *helpPtr;
  u16   again = 0;

  do
  {
    if (dirIsEmpty(path) && (again || askBoolean("Delete empty path ?", 'Y') == 'Y'))
    {
      rmdir(path);
      if ((helpPtr = strrchr(path, '\\')) != NULL)
      {
        *helpPtr = 0;
        again = 1;
      }
      else
        again = 0;
    }
    else
      again = 0;
  }
  while (again);
}
//---------------------------------------------------------------------------
void askRemoveJAM(char *msgBasePath)
{
  tempStrType tempStr;
  char       *hp1
           , *hp2;

  hp1 = stpcpy(tempStr, msgBasePath);
  strcpy(hp1, ".j*");
  if ((hp2 = strrchr(tempStr, '\\')) != NULL)
    *hp2 = 0;

  if (  *tempStr && (hp2 ? existPattern(tempStr, hp2 + 1) : existPattern(".", tempStr))
     && askBoolean("Delete JAM files of this area ?", 'Y') == 'Y'
     )
  {
    if (hp2)
      *hp2 = '\\';
    strcpy(hp1, EXT_IDXFILE);
    unlink(tempStr);
    strcpy(hp1, EXT_HDRFILE);
    unlink(tempStr);
    strcpy(hp1, EXT_TXTFILE);
    unlink(tempStr);
    strcpy(hp1, EXT_LRDFILE);
    unlink(tempStr);
    if (hp2)
    {
      *hp2 = 0;
      askRemoveDir(tempStr);
    }
  }
}
//---------------------------------------------------------------------------
