#ifndef configH
#define configH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2014 Wilfred van Velzen
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

#include "fmail.h"

//---------------------------------------------------------------------------
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

extern configType config;

//---------------------------------------------------------------------------
void handles20      (void);
void handlesReset40 (void);

s16  getNetmailBoard(nodeNumType *akaNode);
s16  isNetmailBoard (u16 board);
void initFMail      (const char *s, s32 switches);
void deinitFMail    (void);
void readAreas      (void);

//---------------------------------------------------------------------------
#endif  // configH
