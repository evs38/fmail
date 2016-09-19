#ifndef msgpktH
#define msgpktH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2016  Wilfred van Velzen
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

typedef struct
{
   u16  origNode
      , destNode
      , year
      , month
      , day
      , hour
      , minute
      , second
      , baud
      , packetType
      , origNet
      , destNet;
  uchar prodCodeLo
      , serialNoMajor;
  uchar password[8];
  u16   origZoneQ
      , destZoneQ
      , auxNet
      , cwValid;
  uchar prodCodeHi
      , serialNoMinor;
  u16   capWord
      , origZone
      , destZone
      , origPoint
      , destPoint;
  uchar prodData[4];

} pktHdrType;
//---------------------------------------------------------------------------
typedef struct
{
  u16   origNode
      , destNode
      , origPoint
      , destPoint;
  uchar reserved[8];
  u16   subVersion
      , packetType
      , origNet
      , destNet;
  uchar prodCode
      , revisionLevel;
  uchar password [8];
  u16   origZone
      , destZone;
  uchar origDomain[8]
      , destDomain[8];
  uchar prodData[4];

} FSC45pktHdrType;
//---------------------------------------------------------------------------
//   0       2     Originating node number
//   2       2     Destination node number
//   4       2   * Originating point number
//   6       2   * Destination point number
//   8       8     Reserved, must be zero
//  16       2     Packet sub-version (2)
//  18       2     Packet version (2)
//  20       2     Originating network
//  22       2     Destination network
//  24       1     Product code
//  25       1     Product revision level
//  26       8     Password
//  34       2   * Originating zone
//  36       2   * Destination zone
//  38       8   * Originating domain
//  46       8   * Destination domain
//  54       4     Product specific data
//  58     ---     Start of first packed message
//               * Field only guaranteed accurate in a type 2.2 header
//---------------------------------------------------------------------------

void  initPkt          (void);
void  deInitPkt        (void);

s16   openPktRd        (char *fileName, s16 secure);
s16   readPkt          (internalMsgType *message);
void  closePktRd       (void);

s16   closeLuPkt       (void);
fhandle openP          (const char *pathname, int access, u16 mode);
fhandle fsopenP        (const char *pathname, int access, u16 mode);

char *setSeenByPath    ( internalMsgType *msg, char *txtEnd
                       , areaOptionsType areaOptions, nodeOptionsType nodeOptions);
s16   writeEchoPkt     (internalMsgType *message, areaOptionsType areaOptions, echoToNodeType echoToNode);
void  freePktHandles   (void);
s16   validateEchoPktWr(void);
s16   closeEchoPktWr   (void);
s16   writeNetPktValid (internalMsgType *message, nodeFileRecType *nfInfoRec);
s16   closeNetPktWr    (nodeFileRecType *nfInfoRec);

//---------------------------------------------------------------------------
#endif  // msgpktH
