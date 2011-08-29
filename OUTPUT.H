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

#define COLOR_BLINK     0x08

#define MONO_NORM       0x07
#define MONO_HIGH       0x0f
#define MONO_INV        0x70
#define MONO_HIGH_BG    0x7f
#define MONO_NORM_BLINK 0x87
#define MONO_HIGH_BLINK 0x8f


#if !defined(__COLORS)

   #define __COLORS

   enum COLORS {
	          BLACK, 	  /* dark colors */
	          BLUE,
	          GREEN,
	          CYAN,
	          RED,
	          MAGENTA,
	          BROWN,
	          LIGHTGRAY,
	          DARKGRAY,       /* light colors */
	          LIGHTBLUE,
	          LIGHTGREEN,
	          LIGHTCYAN,
	          LIGHTRED,
	          LIGHTMAGENTA,
	          YELLOW,
	          WHITE
               };
#endif

#define BLINK	  128	          /* blink bit */


void initOutput      (void);
void setAttr         (u16 fgc, u16 bgc, u16 mattr);
void cls             (void);
u16  getTab          (void);
void gotoTab         (u16 tab);
void gotoPos         (u16 px, u16 py);
#ifdef STDO
#define newLine()      putchar('\n')
#define printString(S) printf("%s", (S))
#define printChar(C)   putchar(C)
#else
void newLine         (void);
void printString     (char *string);
void printChar       (char ch);
#endif

//void printInt      (s16 p);
#define printInt(a)  printLong(a)
void printLong       (u32 p);
void printFill       (void);
void printStringFill (char *string);
void noCursor        (void);
void showCursor      (void);

#ifdef __32BIT__
void updCurrLine  (void);
#define updateCurrLine() updCurrLine()
#else
#define updateCurrLine();
#endif


