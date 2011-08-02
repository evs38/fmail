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


#define CS_SECURITY 0xc6cad720L

/* Msg Attributes:

      Bit 0: Deleted
      Bit 1: Unmoved Outgoing Net Message
      Bit 2: Is a Net Mail Message
      Bit 3: Private
      Bit 4: Received
      Bit 5: Unmoved Outgoing Echo Message
      Bit 6: Local Bit
      Bit 7: [ Reserved ]

   Net Attributes:

      Bit 0: Kill Message after it's been sent
      Bit 1: Sent OK
      Bit 2: File(s) Attached
      Bit 3: Crash Priority
      Bit 4: Request Receipt
      Bit 5: Audit Request
      Bit 6: Is a Return Receipt
      Bit 7: [ Reserved ]

*/

#define RA_DELETED      1
#define RA_NET_OUT      2
#define RA_IS_NETMAIL   4
#define RA_PRIVATE      8
#define RA_RECEIVED    16
#define RA_ECHO_OUT    32
#define RA_LOCAL       64
#define QBBS_GROUPMSG 128 /* QBBS-only Groupmail? */

#define RA_KILLSENT     1
#define RA_SENT         2
#define RA_FILE_ATT     4
#define RA_CRASH        8
#define QBBS_REQ_REC   16
#define QBBS_AUDIT_REQ 32
#define QBBS_RET_REC   64
#define QBBS_FREQ     128


typedef struct
{  uchar wtLength;
   uchar WhoTo[35];   } msgToIdxRec;

typedef struct
{
   uchar txtLength;
   uchar txtStr[255]; } msgTxtRec;

#ifdef GOLDBASE

typedef struct
{  u32           MsgNum,
		 ReplyTo,
                 SeeAlsoNum;
   u16           TRead;
   u32           StartRec;
   u16           NumRecs,
		 DestNet,
		 DestNode,
		 OrigNet,
		 OrigNode;
   uchar         DestZone,
		 OrigZone;
   u16           Cost;
   u16           MsgAttr,
		 NetAttr,
		 Board;
   uchar         ptLength;
   schar         PostTime[5];
   uchar         pdLength;
   schar         PostDate[8];
   uchar         wtLength;
   schar         WhoTo[35];
   uchar         wfLength;
   schar         WhoFrom[35];
   uchar         sjLength;
   schar         Subj[56]; /* 72 - 16 */
   u32           subjCrc;
   u32           wrTime;
   s32           recTime;
   u32           checkSum;    } msgHdrRec;

typedef struct
{  u32   MsgNum;
   u16   Board;   } msgIdxRec;

typedef struct
  { u32  LowMsg,
         HighMsg,
         TotalActive;
    u16  ActiveMsgs[200]; } infoRecType;

#else

typedef struct
{  u16           MsgNum,
                 ReplyTo,
                 SeeAlsoNum,
                 TRead,
                 StartRec,
                 NumRecs,
                 DestNet,
                 DestNode,
                 OrigNet,
                 OrigNode;
   uchar         DestZone,
                 OrigZone;
   u16           Cost;
   uchar         MsgAttr,
                 NetAttr,
                 Board;
   uchar         ptLength;
   schar         PostTime[5];
   uchar         pdLength;
   schar         PostDate[8];
   uchar         wtLength;
   schar         WhoTo[35];
   uchar         wfLength;
   schar         WhoFrom[35];
   uchar         sjLength;
   schar         Subj[56]; /* 72 - 16 */
   u32           subjCrc;
   u32           wrTime;
   s32           recTime;
   u32           checkSum;    } msgHdrRec;

typedef struct
{  u16   MsgNum;
   uchar Board;   } msgIdxRec;

typedef struct
  { u16  LowMsg,
         HighMsg,
         TotalActive,
         ActiveMsgs[200]; } infoRecType;

#endif

void openBBSWr (void);

s16 writeBBS (internalMsgType *message, u16 boardNum, s16 isNetmail);

void closeBBS (void);

