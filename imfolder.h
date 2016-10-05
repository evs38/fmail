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


/*
**  folder.h (InterMail)
2**
**  Copyright 1989-1994 InterZone Software, inc. All rights reserved
**
**  Definitions for each record in IMFOLDER.CFG
**
**  Last revision:  01-24-94
**
**  -------------------------------------------------------------------------
**  This information is not necessarily final and is subject to change at any
**  given time without further notice
**  -------------------------------------------------------------------------
*/

/*
**  Constant long bit values
*/
#define IM_RESTRICT    0x00000001L
#define IM_ECHO_INFO   0x00000002L
#define IM_EXPORT_OK   0x00000004L
#define IM_USE_XLAT    0x00000008L
#define IM_PRIVATE     0x00000010L
#define IM_READONLY    0x00000020L
#define IM_F_NETMAIL   0x08000000L
#define IM_BOARDTYPE   0x10000000L
#define IM_DELETED     0x20000000L         /* Never written to disk */
#define IM_LOCAL       0x40000000L
#define IM_ECHOMAIL    0x80000000L


/*
**  User access mask
*/
#define USER_1      0x00000001L
#define USER_2      0x00000002L
#define USER_3      0x00000004L
#define USER_4      0x00000008L
#define USER_5      0x00000010L
#define USER_6      0x00000020L
#define USER_7      0x00000040L
#define USER_8      0x00000080L
#define USER_9      0x00000100L
#define USER_10     0x00000200L

#define NOTBOARDTYPE   0xEFFFFFFFL


#define F_MSG       0
#define F_HUDSON    1
#define F_WC35      2
#define F_PCB15     3
#define F_JAM       4
#define F_PASSTHRU  5

/*
**  Folder structure
**
**  The "path" and "title" fields below are NUL terminated.
*/

typedef struct
    {
    uchar    path[65];      /* Path if "board==0", otherwise empty (65) */
    uchar    ftype;         /* Folder type                              */
    uchar    areatag[39];   /* Echomail area tag                        */
    uchar    origin;        /* Default origin line, 0-19                */
    uchar    title[41];     /* Title to appear on screen                */
    uchar    useaka;        /* AKA to use, 0==primary                   */
    u16      board;         /* QuickBBS/RemoteAccess/WC conf number     */
    u16      upzone;        /* Uplink zone                              */
    u16      upnet;         /* Uplink net                               */
    u16      upnode;        /* Uplink node                              */
    u16      uppoint;       /* Uplink point                             */
    u32      behave;        /* Behavior, see above                      */
    u32      hiwater;       /* Highwater mark for echomail              */
    u32      pwdcrc;        /* CRC32 of password or -1L if unprotected  */
    u32      userok;        /* Users with initial access                */
    u32      accflags;      /* access flags, for network environment    */
    u32      timestamp;     /* Time stamp for detecting msg base updates*/
    uchar    reserved[4];   /* for future expansion                     */
    }
    IMFOLDER;

//  The following struct was used in IM 2.00-2.25, file name FOLDER.CFG.
//
//typedef struct
//    {
//    char    path[65];      /* Path if "board==0", otherwise empty (65) */
//    char    title[41];     /* Title to appear on screen                */
//    byte    origin;        /* Default origin line, 0-19                */
//    long    behave;        /* Behavior, see above                      */
//    long    pwdcrc;        /* CRC32 of password or -1L if unprotected  */
//    long    userok;        /* Users with initial access                */
//    byte    useaka;        /* AKA to use, 0==primary                   */
//    word    board;         /* QuickBBS/RemoteAccess/WC board number    */
//    }
//    OLDFOLDER, far *OLDFOLDERPTR;

/* end of file "folder.h" */
