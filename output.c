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


#if (__BORLANDC__ < 0x0452)
#define __SegB000       0xb000
#define __SegB800       0xb800
#endif

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#if defined(STDO) || defined(__STDIO__)
#include <conio.h>
#endif
#include "fmail.h"
#include "output.h"
#include "mtask.h"

#ifndef STDO

//         CONTROLEREN !!!!!!!!!!!!!!!!!!!!!!!
#if defined (_Windows) && !defined(__DPMI16__) && !defined(__DPMI32__) && !defined(__WIN32__)
#define _setcursortype(a)
#define textattr(a)
#define movetext(a,b,c,d,e,f)
#define cputs(a) puts(a)
#endif

extern configType config;

typedef struct
{
  char ch;
  char attr;
} screenCharType;


#define showChar(chr)               \
{                                   \
  screen[y*columns+x].ch   = chr;  \
  screen[y*columns+x].attr = attr; \
}

#ifndef __STDIO__

#define removeCursor           \
{                              \
	screen[columns-1].attr = 0; \
	_DH = 0x00;                 \
	_DL = columns-1;            \
	_BH = 0;                    \
	_AH = 0x02;                 \
	geninterrupt (0x10);        \
   _CX = 0x2000;               \
   _AH = 0x01;                 \
   geninterrupt (0x10);        \
}


#define locateCursor(px,py) \
{                           \
   _DH = py;                \
   _DL = px;                \
   _BH = 0;                 \
   _AH = 0x02;              \
   geninterrupt (0x10);     \
   _CX = oldCursor;         \
   _AH = 0x01;              \
   geninterrupt (0x10);     \
}


#define whereCursor(px,py)  \
{                           \
   _BH = 0;                 \
   _AH = 0x03;              \
	geninterrupt (0x10);     \
   px = _DL;                \
   py = _DH;                \
}

#else

#define removeCursor          \
{                             \
   _setcursortype(_NOCURSOR); \
}


#define locateCursor(px,py) \
{                           \
   gotoxy(px+1, py+1);      \
   _setcursortype(_NORMALCURSOR); \
}


#define whereCursor(px,py)  \
{                           \
   px = wherex()-1;         \
   py = wherey()-1;         \
}

#endif


#ifdef __STDIO__
#define increment        \
{                        \
   if (++x == columns)   \
   {                     \
      x = 0;             \
      if (++y == rows)   \
      {                  \
	 y--;            \
	 scroll ();      \
      }                  \
      else               \
	 displayLine(y-1);\
   }                     \
}
#else
#define increment        \
{                        \
   if (++x == columns)   \
   {                     \
      x = 0;             \
      if (++y == rows)   \
      {                  \
	 y--;            \
	 scroll ();      \
      }                  \
   }                     \
}
#endif

#if defined(__STDIO__)
screenCharType *screen;
#else
screenCharType far *screen;
#endif

static s16  color    = 0;
static s16  x        = 0;
static s16  y        = 0;
static s16  rows     = 25;
static s16  columns  = 80;
static char attr     = LIGHTGRAY;

#ifndef __STDIO__
static u16 oldCursor;
union REGS regs;
#endif

#endif // STDO

#ifdef __STDIO__

#ifndef STDO
void displayLine(u16 y)
{
  u16   count;
  uchar tempStr[81];

  gotoxy(1, y+1);
	textattr(screen[80*y].attr);
	for (count = 0; count < 80; count++)
	{
		tempStr[count] = screen[80*y+count].ch;
	}
	tempStr[79] = 0;
	cputs(tempStr);
}
#endif

#ifndef STDO
void updCurrLine(void)
{
  u16   count;
  uchar tempStr[81];

  gotoxy(1, y + 1);
  textattr(screen[80 * y].attr);
  for (count = 0; count < 80; count++)
    tempStr[count] = screen[80 * y + count].ch;

  tempStr[79] = 0;
  cputs(tempStr);
}
#endif
#endif  // __STDIO__

void initOutput(void)
{
#ifndef STDO
#ifndef __STDIO__
   char     currMode;

   /* Get cursor format */

   _AH = 0x03;
   _BH = 0x00;
   geninterrupt (0x10);
   oldCursor = _CX;

   /* Page 0 */

   _AH = 0x05;
   _AL = 0x00;
   geninterrupt (0x10);

   /* Get video mode */

   _AH = 0x0f;
   geninterrupt (0x10);

   currMode = _AL & 0x7f;
   columns = _AH;

   screen = MK_FP (__SegB000, 0x0000);

   if (currMode < 7) /* CGA */
   {
      screen = MK_FP (__SegB800, 0x0000);
   }
   else
   {
      if (currMode > 7) /* High resolution modes */
      {
	 /* Cursor home */

         _AH = 0x02;
	 _BH = 0x00;
	 _DX = 0x0000;
	 geninterrupt (0x10);

	 /* Display '?' */

	 _AH = 0x09;
         _AL = '?';
         _BH = 0x00;
	 _BL = 0x00;
	 _CX = 0x01;
	 geninterrupt (0x10);

	 if (screen[0].ch == '?')
         {
            /* Display ' ' */

	    _AH = 0x09;
	    _AL = ' ';
	    _BH = 0x00;
	    _BL = 0x00;
	    _CX = 0x01;
	    geninterrupt (0x10);

	    if (screen[0].ch != ' ')
	       screen = MK_FP (__SegB800, 0x0000);
	 }
	 else
	 {
	    screen = MK_FP (__SegB800, 0x0000);
	 }
      }
   }

   if (FP_SEG (screen) == __SegB800)
   {
      if ((currMode != 0) && (currMode != 2))
	 color--;
   }

#ifndef __FMAILX__
   _ES = FP_SEG (screen);
   _DI = FP_OFF (screen);
   _AH = 0xfe;
   geninterrupt (0x10);
   screen = MK_FP (_ES, _DI);
#endif

   regs.h.bh = 0;
   regs.h.dl = 0;
   regs.h.al = 0x30;
   regs.h.ah = 0x11;
   int86 (0x10, &regs, &regs);
   if (regs.h.dl != 0)
      rows = regs.h.dl+1;
#else
  color = 1;
#if !defined (_Windows) || defined(__DPMI16__) || defined(__DPMI32__) || defined(__WIN32__)
  {
    struct text_info ti;
    gettextinfo(&ti);
    rows = ti.screenheight;
    // Check rows because there is a bug in Borland C++ conion,
    // if the windows console Screen Buffer Size Height >255,
    // the screenheight becomes 0 because it's an uchar!
    if (rows <= 0)
      rows = 25;
  }
#endif
  screen = malloc(80*2*rows);
#endif

   removeCursor;
   x = y = 0;
   getMultiTasker();
#endif
}

#ifndef STDO
void setAttr(u16 fgc, u16 bgc, u16 mattr)
{
  if (color)
  {
    attr = ((fgc & 0x0f) | ((bgc & 0x0f) << 4));
  }
  else
  {
    attr = mattr;
  }
}
#endif

#ifndef STDO
void scroll(void)
{
  u16 count;

  memcpy(&(screen[0]), &(screen[columns]), (rows-1)*columns*2);
  for (count = (rows-1)*columns; count < rows*columns; count++)
  {
    screen[count].ch   = ' ';
    screen[count].attr = attr;
  }
  screen[columns-1].attr = 0;

#ifdef __STDIO__
  movetext(1, 2, 80, rows, 1, 1);
  displayLine(rows-1);
#endif
  returnTimeSlice(0);
}
#endif

void cls(void)
{
#ifndef STDO
  u16 count;

  for (count = 0; count < columns; count++)
  {
    screen[count].ch   = ' ';
    screen[count].attr = attr;
  }

  for (count = 1; count < rows; count++)
  {
    memcpy (&(screen[count*columns]), &(screen[0]), 2*columns);
  }
  screen[columns-1].attr = 0;

#ifdef __STDIO__
  clrscr();
#endif
  returnTimeSlice(0);
#endif
}

u16 getTab(void)
{
#ifndef STDO
  return x;
#else
  return wherex() - 1;
#endif
}

void gotoTab(u16 tab)
{
#ifndef STDO
  x = tab;
#ifndef __STDIO__
  if (config.genOptions.checkBreak)
  {
    _AH = 0x0b;
    geninterrupt(0x21);
  }
#endif
  returnTimeSlice(0);
#else
#if 0
  if (tab == 0)
    putchar('\r');
  else
#endif
    gotoxy(++tab, wherey());
#endif
}

void gotoPos(u16 px, u16 py)
{
#ifndef STDO
#ifdef __STDIO__
  displayLine(y);
#endif
  x = px;
  y = py;
#ifndef __STDIO__
  if (config.genOptions.checkBreak)
  {
    _AH = 0x0b;
    geninterrupt (0x21);
  }
#endif
#else
  gotoxy(++px, ++py);
#endif
}

#ifndef STDO
void newLine(void)
{
#ifdef __STDIO__
  displayLine(y);
#endif
  x = 0;
  if (y++ == rows-1)
  {
    y--;
    scroll();
  }
  else
    returnTimeSlice(0);
#ifndef __STDIO__
  if (config.genOptions.checkBreak)
  {
    _AH = 0x0b;
    geninterrupt(0x21);
  }
#endif
}

void printString(const char *string)
{
  if (string != NULL)
  {
    while (*string)
    {
      if (*string == '\n')
      {
        newLine();
        string++;
      }
      else
      {
        showChar(*(string++));
        increment;
      }
    }
  }
  //returnTimeSlice(0);
}
#endif

void printLong(u32 p)
{
  char numStr[34];

  ultoa(p, numStr, 10);
  printString(numStr);
}

void printFill(void)
{
#ifndef STDO
  u16 temp;

  temp = x;
  while (x != columns-1)
  {
    showChar(' ');
    increment;
  }
  showChar(' ');
  x = temp;
#ifdef __STDIO__
  displayLine(y);
#endif
#else
  clreol();
#endif
}

void printStringFill(const char *string)
{
  printString(string);
  printFill();
}

#ifndef STDO
void printChar(char ch)
{
  if (ch == '\n')
    newLine();
  else
  {
    showChar(ch);
    increment;
  }
}
#endif

#ifndef STDO
void noCursor(void)
{
  whereCursor(x, y);
  removeCursor;
}
#endif

#ifndef STDO
void showCursor(void)
{
  locateCursor(x, y);
}
#endif

