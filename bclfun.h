#ifndef bclfunH
#define bclfunH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2015  Wilfred van Velzen
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

#include <pshpack1.h>

#define BCLH_ISLIST   0x00000001L  // File is complete list
#define BCLH_ISUPDATE 0x00000002L  // File contains update/diff information

typedef struct
{
  char FingerPrint[4];   // BCL<NUL>
  char ConfMgrName[31];  // Name of "ConfMgr"
  char Origin[51];       // Originating network addr
  u32  CreationTime;     // UNIX-timestamp when created
  u32  flags;            // Options, see below
  u8   Reserved[256];    // Reserved data
} bcl_header_type;

#define FLG1_READONLY 0x00000001L  // Read only, software must not allow users to enter mail.
#define FLG1_PRIVATE  0x00000002L  // Private attribute of messages is honored.
#define FLG1_FILECONF 0x00000004L  // File conference.
#define FLG1_MAILCONF 0x00000008L  // Mail conference.
#define FLG1_REMOVE   0x00000010L  // Remove specified conference from list (otherwise add/upd).

typedef struct
{
  u16 EntryLength;      // Length of entry data
  u32 flags1
    , flags2;           // Conference flags
} bcl_type;

extern bcl_header_type bcl_header;
extern bcl_type bcl;

#include <poppack.h>

//---------------------------------------------------------------------------
udef openBCL (uplinkReqType *uplinkReq);
udef readBCL (char **tag, char **descr);
udef closeBCL(void);

udef process_bcl(char *fileName);
void send_bcl(nodeNumType *srcNode, nodeNumType *destNode, nodeInfoType *nodeInfoPtr);

void ChkAutoBCL(void);
udef ScanNewBCL(void);

//---------------------------------------------------------------------------
#endif  // bclfunH
