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
{  uchar   stdtxt[28];       // "FMail Personal Key File of: "
   uchar   sysopName[36];    // terminated with CTRL-Z
   u32     relKey1;
   u32     relKey2;
   u32     betaKey;
   uchar   data[176];
   u32     crc;
} keyType;


extern keyType key;

int     keyFileInit(void);
