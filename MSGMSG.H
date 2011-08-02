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


#define NETMSG   -1
#define PERMSG   -2
#define DUPMSG   -3
#define BADMSG   -4
#define SECHOMSG -5



typedef struct
{
   unsigned day      : 5;
   unsigned month    : 4;
   unsigned year     : 7;
   unsigned dSeconds : 5;
   unsigned minutes  : 6;
   unsigned hours    : 5;   }  packedTime;



typedef struct
{
   uchar       fromUserName [36];
   uchar       toUserName [36];
   uchar       subject [72];
   uchar       dateTime [20];
   u16         timesRead;
   u16         destNode;
   u16         origNode;
   u16         cost;
   u16         origNet;
   u16         destNet;
   packedTime  wrTime;
   packedTime  recTime;
   u16         replyTo;
   u16         attribute;
   u16         nextReply;   } msgMsgType;



#define MAX_ATTACH 512

typedef struct
{
   uchar       fileName[13];
   nodeNumType origNode;
   nodeNumType destNode;     } fAttInfoType;


void initMsg (s16 noAreaFix);

u16 getFlags (uchar *text);

s16 attribMsg (u16 attribute, s32 msgNum);

void moveMsg (char *msgName, char *destDir);

s16 readMsg (internalMsgType *message, s32 msgNum);

s32  writeMsg (internalMsgType *message, s16 msgType, s16 valid);

s32  writeMsgLocal (internalMsgType *message, s16 msgType, s16 valid);

void validateMsg (void);

s16 fileAttach (uchar *fileName, nodeNumType *srcNode, nodeNumType *destNode,
                nodeInfoType *nodeInfo);
