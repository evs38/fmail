#ifndef msgraH
#define msgraH
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

#define CS_SECURITY 0xc6cad720L

#include <pshpack1.h>

typedef struct
{
   uchar    nameLength;
   uchar    name[40];
   uchar    typ;
   uchar    msgKinds;
   uchar    attribute;   /* Bit 0 : Enable EchoInfo
                                1 : Combined access
                                2 : File attaches
                                3 : Allow aliases
                                5 : Force handle
                                6 : Deletes (FJ) */
   uchar    daysKill;
   uchar    rcvdKill;
   u16      countKill;
   u16      readSecurity;
   uchar    readFlags[4];
   u16      writeSecurity;
   uchar    writeFlags[4];
   u16      sysopSecurity;
   uchar    sysopFlags[4];
   uchar    originLength;
   uchar    origin[60];
   uchar    akaAddress;
} messageRaType;


typedef struct
{
   u16      areanum;
   uchar    unused[2];
   uchar    nameLength;
   uchar    name[40];
   uchar    typ;
   uchar    msgKinds;
   uchar    attribute;   /* Bit 0 : Enable EchoInfo
                                1 : Combined access
                                2 : File attaches
                                3 : Allow aliases
                                4 : Use SoftCRs as characters RA2
                                5 : Force handle
                                6 : Allow deletes
                                7 : Is a JAM area */
   uchar    daysKill;
   uchar    rcvdKill;
   u16      countKill;
   u16      readSecurity;
   uchar    readFlags[4];
   uchar    readNotFlags[4];
   u16      writeSecurity;
   uchar    writeFlags[4];
   uchar    writeNotFlags[4];
   u16      sysopSecurity;
   uchar    sysopFlags[4];
   uchar    sysopNotFlags[4];
   uchar    originLength;
   uchar    origin[60];
   uchar    akaAddress;
   uchar    age;
   uchar    JAMbaseLength;
   uchar    JAMbase[60];
   u16      group;
   u16      altGroup[3];
   uchar    attribute2;  /* Bit 0 : Include in all groups */
   u16      netReply; /* FJ */
   uchar     freeSpace[7];
} messageRa2Type;

/* RA 2
  MESSAGErecord  = record
                     AreaNum        : Word;
                     Unused         : Array[1..2] of Byte;
                     Name           : String[40];
                     Typ            : MsgType;
                     MsgKinds       : MsgKindsType;
                     Attribute      : Byte;

                      { Bit 0 : Enable EchoInfo
                            1 : Combined access
                            2 : File attaches
                            3 : Allow aliases
                            4 : Use SoftCRs as characters
                            5 : Force handle
                            6 : Allow deletes
                            7 : Is a JAM area }

                     DaysKill,    { Kill older than 'x' days }
                     RecvKill       : Byte; { Kill recv msgs, recv for more than 'x' days }
                     CountKill      : Word;

                     ReadSecurity   : Word;
                     ReadFlags,
                     ReadNotFlags   : FlagType;

                     WriteSecurity  : Word;
                     WriteFlags,
                     WriteNotFlags  : FlagType;

                     SysopSecurity  : Word;
                     SysopFlags,
                     SysopNotFlags  : FlagType;

                     OriginLine     : String[60];
                     AkaAddress     : Byte;

                     Age            : Byte;

                     JAMbase        : String[60];
                     Group          : Word;
                     AltGroup       : Array[1..3] of Word;

                     Attribute2     : Byte;

                      { Bit 0 : Include in all groups }

                     FreeSpace      : Array[1..9] of Byte;
                   end;
*/

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
{
  u8   wtLength;
  char WhoTo[35];
} msgToIdxRec;

typedef struct
{
  u8   txtLength;
  char txtStr[255];
} msgTxtRec;

#ifdef GOLDBASE

typedef struct
{
   u32           MsgNum,
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
   u32           checkSum;
} msgHdrRec;

typedef struct
{
  u32 MsgNum;
  u16 Board;
} msgIdxRec;

typedef struct
{
  u32 LowMsg
    , HighMsg
    , TotalActive;
  u16  ActiveMsgs[MBBOARDS];
} infoRecType;

#else

typedef struct
{
   u16           MsgNum,
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
   u8            MsgAttr,
                 NetAttr,
                 Board;
   u8            ptLength;
   char          PostTime[5];
   u8            pdLength;
   char          PostDate[8];
   u8            wtLength;
   char          WhoTo[35];
   u8            wfLength;
   char          WhoFrom[35];
   u8            sjLength;
   char          Subj[56]; /* 72 - 16 */
   u32           subjCrc;
   u32           wrTime;
   s32           recTime;
   u32           checkSum;
} msgHdrRec;

typedef struct
{
  u16   MsgNum;
  uchar Board;
} msgIdxRec;

typedef struct
{
  u16 LowMsg
    , HighMsg
    , TotalActive
    , ActiveMsgs[MBBOARDS];
} infoRecType;

#endif

#include <poppack.h>
//---------------------------------------------------------------------------
void  closeBBSWr      (u16);
char *expandNameHudson(const char *fileName, int orgName);
void  initBBS         (void);
int   lockMB          (void);
void  moveBadBBS      (void);
s16   multiUpdate     (void);
void  openBBSWr       (u16 orgName);
s16   rescan          (nodeInfoType *nodeInfo, const char *areaName, u16 maxRescan, fhandle msgHandle1, fhandle msgHandle2);
#ifndef GOLDBASE
s16   scanBBS         (u16 index, internalMsgType *message, u16 rescan);
#else
s16   scanBBS         (u32 index, internalMsgType *message, u16 rescan);
#endif
int   testMBUnlockNow (void);
void  unlockMB        (void);
s16   updateCurrHdrBBS(internalMsgType *message);
s16   validate1BBS    (void);
void  validate2BBS    (u16);
s16   writeBBS        (internalMsgType *message, u16 boardNum, u16 impSeenBy);

//---------------------------------------------------------------------------
#endif  // msgraH

