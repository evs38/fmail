#ifndef hudson_sharedH
#define hudson_sharedH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2017  Wilfred van Velzen
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

char *expandNameHudson(const char *fileName, int orgName);
#if 1
#define expandNameH(FN)  expandNameHudson(FN, 1)
#else
char *expandNameH(char *fileName);
#endif
int  testMBUnlockNow(void);
void setMBUnlockNow (void);
int  lockMB         (void);
void unlockMB       (void);
//---------------------------------------------------------------------------
#endif
