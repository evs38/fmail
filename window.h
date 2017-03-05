#ifndef __fmail_windows_h
#define __fmail_windows_h
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

#include "bits.h"

//---------------------------------------------------------------------------

#define WB_DOUBLE_H      1
#define WB_DOUBLE_V      2
#define TITLE_LEFT       4
#define TITLE_RIGHT      8
#define SHADOW          16
#define NO_SAVE         32
#define FMTEXT           1
#define WORD             2
#define ALPHA            3
#define ALPHA_AST        4
#define PATH             6
#define FILE_NAME        7
#define NUM_INT          8
#define NUM_P_INT        9
#define NUM_LONG        10
#define MB_NAME         11
#define BOOL_INT        12
#define BOOL_INT_REV    13
#define ENUM_INT        14
#define BIT_TOGGLE      15
#define AKA_SELECT      16
#define NODE            17
#define NODE_MATCH      18
#define SFILE_NAME      19
#define PACK            20
#define DATE            22
#define DATETIME        23
#define FUNCTION        24
#define FUNC_PAR        25
#define FUNC_VPAR       26
#define NEW_WINDOW      32
#define WSELECT         33
#define EMAIL           48
#define EXTRA_TEXT     256

#define CTRLCODES   0x0800
#define LFN         0x1000
#define NOCLEAR     0x2000
#define UPCASE      0x4000
#define NO_EDIT     0x8000
#define DISPLAY     0x8000

#define MASK        0x07ff

#define FAKE             1

#define MAX_WINDOWS    100
#define MAX_ENTRIES     40   // was 36

#define COLOR_BLINK     0x08

#define MONO_NORM       0x07
#define MONO_HIGH       0x0f
#define MONO_INV        0x70
#define MONO_HIGH_BG    0x7f
#define MONO_NORM_BLINK 0x87
#define MONO_HIGH_BLINK 0x8f

#if !defined(__COLORS) && !defined(_CONIO2_H_)
#define __COLORS

enum COLORS
{
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
#endif  // __COLORS

#define BLINK	  128	          /* blink bit */

#ifdef __BORLANDC__
typedef struct char_info
{
  char letter;
  u8   attr;
} screenCharType;
#endif // __BORLANDC__
#ifdef __MINGW32__
typedef struct char_info screenCharType;
#endif // __MINGW32__

typedef struct
{
  uchar background
      , actvborderfg
      , inactvborderfg
      , promptfg
      , promptkeyfg
      , datafg
      , titlefg
      , titlebg
      , scrollbg
      , scrollfg
      , editfg
      , editbg
      , commentfg
      , commentbg
      , atttextfg
      , mono_attr
      , wAttr;
} windowLookType;

typedef struct
{
  u16      entryType;
  char    *prompt;
  u8       key;
  u16      offset;
  void    *data;
  u16      selected;
  u16      par1
         , par2;
  const char *comment;
} menuEntryType;

typedef struct
{
  char          *title;
  u16            xWidth, yWidth, pdEdge, zDataSize;
  u16            entryCount;
  menuEntryType  menuEntry[MAX_ENTRIES];
} menuType;

typedef char *txtPtr;
typedef s16(*function)(void);
typedef s16(*functionPar)(u16 *v);
typedef s16(*functionVPar)(u16 v);

typedef struct
{
  function f;
  u16      *numPtr;
} funcParType;

typedef struct
{
  void   *data;
  txtPtr  text  [34];
  u8      retval[34];
} toggleType;

typedef struct
{
  u16             sx, sy;
  u16             ex, ey;
  u16             inactvborderfg, background;
  screenCharType *oldScreen;
} windowMemType;


#define testInit() if (initMagic != 0x4657) initWindow(0);

void  showChar   (int x, int y, int chr, int att, int matt);
void  shadow     (int x, int y);
void  changeColor(int x, int y, int att, int matt);
char  getChar    (int x, int y);

#define calcAttr(fgc, bgc)  (((fgc) & 0x0F) | (((bgc) & 0x0F) << 4))

void locateCursor(int x, int y);

#define removeCursor() _setcursortype(_NOCURSOR)
#define smallCursor()  _setcursortype(_NORMALCURSOR)
#define largeCursor()  _setcursortype(_SOLIDCURSOR)

//---------------------------------------------------------------------------
u16       readKbd      (void);
void      initWindow   (u16 mode);
menuType *createMenu   (char *title);
s16       displayWindow(const char *title, u16 sx, u16 sy, u16 ex, u16 ey);
void      removeWindow (void);
s16       addItem      (menuType *menu, u16 entryType, char *prompt, u16 offset, void *data, u16 par1, u16 par2, const char *comment);
s16       displayMenu  (menuType *menu, u16 sx, u16 sy);
s16       groupToChar  (s32 group);
u16       editString   (char *string, u16 width, u16 x, u16 y, u16 fieldType);
void      processed    (u16 updated, u16 total);
s16       changeGlobal (menuType *menu, void *org, void *upd);
s16       runMenuDE    (menuType *menu, u16 sx, u16 sy, u16 *dataPtr, u16 setdef, u16 esc);
#define runMenuD(a,b,c,d,e) runMenuDE(a,b,c,d,e,0)
#define runMenu(a,b,c) runMenuDE(a,b,c,NULL,0,0)

void  printChar        (char ch, u16 sx, u16 sy, u16 fgc, u16 bgc, u16 mAttr);
void  printString      (const char *str, u16 sx, u16 sy, u16 fgc, u16 bgc, u16 mAttr);
void  printStringFill  (const char *str, char ch, s16 num, u16 x, u16 y, u16 fgc, u16 bgc, u16 mAttr);
void  displayData      (menuType *menu, u16 sx, u16 sy, s16 mark);
void  fillRectangle    (char ch, u16 sx, u16 sy, u16 ex, u16 ey, u16 fgc, u16 bgc, u16 mAttr);
nodeNumType getNodeNum (char *title, u16 sx, u16 sy, u16 aka);
const char *nodeStr    (const nodeNumType *nodeNum);
char *getStr           (const char *title, const char *init);
char *getSourceFileName(char *title);
char *getDestFileName  (char *title);
void  displayMessage   (const char *msg);
int   askChar          (char *prompt, char *keys);
int   askBoolean       (char *prompt, int dfault);
void  working          (void);
s16   displayAreas     (void);
void  deInit           (u16 cursorLine);
void  saveWindowLook   (void);
void  restoreWindowLook(void);
void  askRemoveDir     (char *path);
void  askRemoveJAM     (char *msgBasePath);

//---------------------------------------------------------------------------
#endif // __fmail_windows_h
