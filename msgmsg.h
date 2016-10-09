#ifndef msgmsgH
#define msgmsgH
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

#define NETMSG   -1
#define PERMSG   -2
#define DUPMSG   -3
#define BADMSG   -4
#define SECHOMSG -5

#include <pshpack1.h>
//---------------------------------------------------------------------------
typedef struct
{
  u32 day     : 5;
  u32 month   : 4;
  u32 year    : 7;
  u32 dSeconds: 5;
  u32 minutes : 6;
  u32 hours   : 5;

} packedTime;
//---------------------------------------------------------------------------
typedef struct
{
  char       fromUserName[36];
  char       toUserName  [36];
  char       subject     [72];
  char       dateTime    [20];
  u16        timesRead;
  u16        destNode;
  u16        origNode;
  u16        cost;
  u16        origNet;
  u16        destNet;
  packedTime wrTime;
  packedTime recTime;
  u16        replyTo;
  u16        attribute;
  u16        nextReply;

} msgMsgType;
//---------------------------------------------------------------------------
#define MAX_ATTACH 512

typedef struct
{
  char        fileName[13];
  nodeNumType origNode;
  nodeNumType destNode;

} fAttInfoType;

#include <poppack.h>
//---------------------------------------------------------------------------
void initMsg      (s16 noAreaFix);
u16  getFlags     (char *text);
s16  attribMsg    (u16 attribute, s32 msgNum);
void moveMsg      (char *msgName, char *destDir);
s16  readMsg      (internalMsgType *message, s32 msgNum);
s32  writeMsg     (internalMsgType *message, s16 msgType, s16 valid);
s32  writeMsgLocal(internalMsgType *message, s16 msgType, s16 valid);
void validateMsg  (void);
s16  fileAttach   (char *fileName, nodeNumType *srcNode, nodeNumType *destNode, nodeInfoType *nodeInfo);
//---------------------------------------------------------------------------

#endif  // msgmsgH
