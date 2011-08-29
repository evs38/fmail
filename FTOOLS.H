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


typedef struct
{
   uchar           *name;
   uchar           aka;
   uchar           defined;
   areaOptionsType options;
   u16             numDays;
   u16             numDaysRcvd;
   u16             numMsgs;  } boardInfoType;


void initLog  (char *s, s32 switches);

void logEntry (char *s, u16 entryType, u16 errorLevel);

s32 getSwitchFT (int *argc, char *argv[], s32 mask);


#define SW_A  0x00000001L
#define SW_B  0x00000002L
#define SW_C  0x00000004L
#define SW_D  0x00000008L
#define SW_E  0x00000010L
#define SW_F  0x00000020L
#define SW_G  0x00000040L
#define SW_H  0x00000080L
#define SW_I  0x00000100L
#define SW_J  0x00000200L
#define SW_K  0x00000400L
#define SW_L  0x00000800L
#define SW_M  0x00001000L
#define SW_N  0x00002000L
#define SW_O  0x00004000L
#define SW_P  0x00008000L
#define SW_Q  0x00010000L
#define SW_R  0x00020000L
#define SW_S  0x00040000L
#define SW_T  0x00080000L
#define SW_U  0x00100000L
#define SW_V  0x00200000L
#define SW_W  0x00400000L
#define SW_X  0x00800000L
#define SW_Y  0x01000000L
#define SW_Z  0x02000000L
