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


void addInfo (internalMsgType *message, s16 isNetmail);

s16 writeNetMsg (internalMsgType *message, s16 srcAka, nodeNumType *destNode,
                 u16 capability, u16 outStatus);

s16 readNodeNum (uchar *nodeString, nodeNumType *nodeNum, u16 *valid);

s16 export (int argc, char *argv[]);

/* os2 */

s16 getBoardNum (uchar *text, s16 valid, u16 *aka, rawEchoType **areaPtr);
