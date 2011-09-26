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


#define WB_DOUBLE_H      1
#define WB_DOUBLE_V      2
#define TITLE_LEFT       4
#define TITLE_RIGHT      8
#define SHADOW          16
#define NO_SAVE         32
/* #define FAST_EDIT       64 */

#define TEXT             1
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
#define ENUM_INT        13
#define BIT_TOGGLE      14
#define AKA_SELECT      15
#define NODE            16
#define NODE_MATCH      17
#define SFILE_NAME      19
#define PACK            20
#define DATE            22
//#define TIME          23
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
#define MAX_ENTRIES     40 /* was 36 */


#define BIT0   0x0001
#define BIT1   0x0002
#define BIT2   0x0004
#define BIT3   0x0008
#define BIT4   0x0010
#define BIT5   0x0020
#define BIT6   0x0040
#define BIT7   0x0080
#define BIT8   0x0100
#define BIT9   0x0200
#define BIT10  0x0400
#define BIT11  0x0800
#define BIT12  0x1000
#define BIT13  0x2000
#define BIT14  0x4000
#define BIT15  0x8000


#ifndef NULL
#if defined(__TINY__) || defined(__SMALL__) || defined(__MEDIUM__)
#define NULL	0
#else
#define NULL	0L
#endif
#endif


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



typedef struct
{
   uchar ch;
   uchar attr;
}              screenCharType;



typedef struct
{
   uchar background,
        actvborderfg,
        inactvborderfg,
        promptfg,
        promptkeyfg,
        datafg,
        titlefg,
        titlebg,
        scrollbg,
        scrollfg,
        editfg,
        editbg,
        commentfg,
        commentbg,
        atttextfg,
        mono_attr,
        wAttr;      } windowLookType;



typedef struct
{
   u16      entryType;
   uchar    *prompt;
   uchar    key;
   u16      offset;
#ifndef __32BIT__
   void     far *data;
#else
   void     *data;
#endif
   u16      selected;
   u16      par1, par2;
   uchar    *comment;    } menuEntryType;



typedef struct
{
   uchar          *title;
   u16            xWidth, yWidth, pdEdge, zDataSize;
/*   windowLookType *look; */
   u16            entryCount;
   menuEntryType  menuEntry[MAX_ENTRIES]; } menuType;



typedef uchar *txtPtr;


typedef s16 (*function)(void);
typedef s16 (*functionPar)(u16 *v);
typedef s16 (*functionVPar)(u16 v);


typedef struct
{
   function f;
   u16      *numPtr;  } funcParType;



typedef struct
{
   unsigned uchar *data;
   txtPtr         text[34];
   uchar          retval[34];
} toggleType;



typedef struct
{
   u16            sx, sy;
   u16            ex, ey;
   u16            inactvborderfg, background;
   screenCharType *oldScreen; } windowMemType;



#define testInit() if (initMagic != 0x4657) initWindow(0);


#define showCharNA(px,py,chr)  \
   screen[(py)*columns+px].ch = chr    


#define showChar(px,py,chr,att,matt)  \
{                                     \
   screen[(py)*columns+px].ch = chr;    \
   if (color)                         \
      screen[(py)*columns+(px)].attr = att; \
   else                               \
      screen[(py)*columns+(px)].attr = matt; \
}


#define getChar(px,py) (screen[(py)*columns+px].ch)

#define getColor(px,py,att,matt) (screen[(py)*columns+px].attr)


#define changeColor(px,py,att,matt)  \
{                                    \
   if (color)                        \
      screen[(py)*columns+px].attr = att; \
   else                              \
      screen[(py)*columns+px].attr = matt; \
}


#define shadow(px,py)                    \
{                                        \
   if (color)                            \
      screen[(py)*columns+px].attr = DARKGRAY; \
}


#define getAttr(fgc,bgc,att)   \
{                              \
   att = ((fgc & 0x0f) |       \
	 ((bgc & 0x0f) << 4)); \
}



#ifdef __OS2__

#define removeCursor       \
{                          \
   VIOCURSORINFO vci;      \
   VioSetCurPos(1, 1, 0);  \
   vci.yStart = 0x0000;    \
   vci.cEnd = 0x0000;      \
   vci.cx = 0;             \
   vci.attr = 0xFFFF;      \
   VioSetCurType(&vci, 0); \
}

#define locateCursor(px,py) \
   VioSetCurPos(py, px, 0);

#define smallCursor()      \
{                          \
   VIOCURSORINFO vci;      \
   vci.yStart = 14;        \
   vci.cEnd = 15;          \
   vci.cx = 1;             \
   vci.attr = 0x0000;      \
   VioSetCurType(&vci, 0); \
}

#define largeCursor()      \
{                          \
   VIOCURSORINFO vci;      \
   vci.yStart = 11;        \
   vci.cEnd = 15;          \
   vci.cx = 1;             \
   vci.attr = 0x0000;      \
   VioSetCurType(&vci, 0); \
}

#else

#define removeCursor    \
{                       \
   _BH = 0x00;          \
   _DX = 0x0101;        \
   _AH = 0x02;          \
   geninterrupt (0x10); \
   _CX = 0x2000;        \
   _AH = 0x01;          \
   geninterrupt (0x10); \
}

#define locateCursor(px,py) \
{                           \
   _DH = py;                \
   _DL = px;                \
   _AH = 0x02;              \
   _BH = 0;                 \
   geninterrupt (0x10);     \
}

#define smallCursor()   \
{                       \
   if (cga)             \
   {                    \
      _CH = 6;          \
      _CL = 7;          \
   }                    \
   else                 \
   {                    \
     _CH = 11;          \
     _CL = 12;          \
   }                    \
   _AH = 0x01;          \
   geninterrupt (0x10); \
}

#define largeCursor()   \
{                       \
   if (cga)             \
   {                    \
      _CH = 4;          \
      _CL = 7;          \
   }                    \
   else                 \
   {                    \
     _CH =  9;          \
     _CL = 12;          \
   }                    \
   _AH = 0x01;          \
   geninterrupt (0x10); \
}

#endif



s16 readKbd (void);

void initWindow (u16 mode);

menuType *createMenu(char *title);

s16 displayWindow (char *title, u16 sx, u16 sy, u16 ex, u16 ey);

void removeWindow (void);

s16 addItem (menuType *menu, u16 entryType, char *prompt, u16 offset,
             void *data, u16 par1, u16 par2, char *comment);

s16 displayMenu (menuType *menu, u16 sx, u16 sy);

s16 groupToChar (s32 group);

//s32 charToGroup (s16 c);

u16 editString (char *string, u16 width, u16 x, u16 y,
                u16 fieldType);

void processed (u16 updated, u16 total);

s16 changeGlobal(menuType *menu, void *org, void *upd);

s16 runMenuDE(menuType *menu, u16 sx, u16 sy, char *dataPtr, u16 setdef, u16 esc);
#define runMenuD(a,b,c,d,e) runMenuDE(a,b,c,d,e,0)
#define runMenu(a,b,c) runMenuDE(a,b,c,NULL,0,0)

void printChar (char ch, u16 sx,  u16 sy,
                         u16 fgc, u16 bgc,
                         u16 mAttr);

void printString (char *string, u16 sx,  u16 sy,
                                u16 fgc, u16 bgc,
                                u16 mAttr);

void printStringFill (char *string, char ch, s16 num,
                            u16 x, u16 y,
                            u16 fgc, u16 bgc,
                            u16 mAttr);

void displayData (menuType *menu, u16 sx, u16 sy, s16 mark);

void fillRectangle (char ch, u16 sx, u16 sy,
                             u16 ex, u16 ey,
                             u16 fgc, u16 bgc,
                             u16 mAttr);

nodeNumType getNodeNum(char *title, u16 sx, u16 sy, u16 aka);

char *nodeStr (nodeNumType *nodeNum);

char *getSourceFileName (char *title);

char *getDestFileName (char *title);

void displayMessage (char *msg);

s16 askChar(char *prompt, u8 *keys);

s16 askBoolean (char *prompt, s16 dfault);

void working (void);

s16 displayAreas (void);

void deInit (u16 cursorLine);

void saveWindowLook(void);

void restoreWindowLook(void);

void askRemoveDir(char *path);

void askRemoveJAM(char *msgBasePath);


