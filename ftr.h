#ifndef exportH
#define exportH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2016 Wilfred van Velzen
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

void addInfo    (internalMsgType *message, s16 isNetmail);
s16  writeNetMsg(internalMsgType *message, s16 srcAka, nodeNumType *destNode, u16 capability, u16 outStatus);
s16  readNodeNum(char *nodeString, nodeNumType *nodeNum, u16 *valid);
void Export     (int argc, char *argv[]);
s16  getBoardNum(char *text, s16 valid, u16 *aka, rawEchoType **areaPtr);

//---------------------------------------------------------------------------
#endif  // exportH
