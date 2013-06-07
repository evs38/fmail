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

#ifndef __JAMFUN_H__
#define __JAMFUN_H__

#include "jam.h"


#define _JAM_MAXSUBLEN    1024
#define _JAM_MAXSUBLENTOT 8192


u32  jam_open(char *, JAMHDRINFO **jam_hdrinfo);
void jam_close(u32);
void jam_closeall(void);

u16	jam_newidx(u32, JAMIDXREC *, u32 *);
u16	jam_getidx(u32, JAMIDXREC *, u32);
u16	jam_getnextidx(u32, JAMIDXREC *);
u16	jam_updidx(u32, JAMIDXREC *);

u16	jam_getsubfields(u32, char *, u32, internalMsgType *);
u16	jam_makesubfields( u32 jam_code, char *jam_subfields, u32 *jam_subfieldLen
                     , JAMHDR *jam_msghdrrec, internalMsgType *message);

u16	jam_gethdr(u32, u32, JAMHDR *, char *, internalMsgType *);
u16 jam_puthdr(u32, u32, JAMHDR *);
u16 jam_newhdr(u32, u32 *, JAMHDR *, char *);

u16	jam_gettxt(u32, u32, u32, char *);
u16	jam_puttext(JAMHDR *, char *);

u16	jam_getlock(u32);
u16	jam_freelock(u32);

u32	jam_writemsg(char *, internalMsgType *, u16);

u32 jam_scan(u16, u32, u16, internalMsgType *);
u32	jam_update(u16, u32, internalMsgType *);
u32 jam_rescan (u16, u16, nodeInfoType *, internalMsgType *);

#endif
