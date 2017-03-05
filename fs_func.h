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


#define EDIT_NETMAIL         1
#define EDIT_GLOB_BBS        2
#define EDIT_ECHO            4
#define EDIT_ECHO_DEFAULT    8
#define EDIT_ECHO_GLOBAL    16
#define DISPLAY_ECHO_WINDOW 32
#define DISPLAY_ECHO_DATA   64

typedef char uplinkNodeStrType[MAX_UPLREQ][28];

s16 displayGroups(void);
s16 askGroups    (void);
s32 getGroups    (s32 groups);
s16 netmailMenu  (u16 v);
s16 defaultBBS   (void);
s16 uplinkMenu   (u16 v);

s16 netDisplayAreas(u16 *v);
s16 badduprecDisplayAreas(u16 *v);
s16 editAM(s16 editType, u16 setdef, rawEchoType *areaBuf);
