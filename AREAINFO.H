#ifndef areainfoH
#define areainfoH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2015 Wilfred van Velzen
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

#include "fmail.h"

#if defined(__FMAILX__) || defined(__32BIT__)
#define MAX_OUTPKT      1024
#define MAX_OUTPKT_STR "1024"
#else
#define MAX_OUTPKT       256
#define MAX_OUTPKT_STR  "256"
#endif

#define ECHONAME_LEN_OLD  25
#define ECHONAME_LEN      51
#define COMMENT_LEN       51
#define ORGLINE_LEN       59


//---------------------------------------------------------------------------
typedef struct
{
  tempStrType fileName;
  s16         valid;
  void       *nextRec;

} fnRecType;


typedef u16  echoToNodeType[MAX_OUTPKT/8];
typedef u16 *echoToNodePtrType;

//---------------------------------------------------------------------------
typedef struct
{
  char           *areaName;
  char           *oldAreaName;
  u16             writeLevel;
  u16             board;
  char           *JAMdirPtr;
  uchar           address;
  u32             alsoSeenBy;
  areaOptionsType options;
  uchar           outStatus;
  char           *originLine;
  u32             areaNameCRC;
  u16             msgCount;
  u16             dupCount;
  u16             msgCountV;
  u16             dupCountV;
  u16             echoToNodeCount;

} cookedEchoType;


typedef cookedEchoType *cookedEchoPtrType;


//---------------------------------------------------------------------------
typedef struct
{
  nodeNumType   srcNode;
  nodeNumType   destNode;
  nodeNumType   destNode4d;
  s16           srcAka;       // has to be s16
  s16           requestedAka; // has to be s16
  u16           destFake;
  s32           bytesValid;
  nodeInfoType *nodePtr;
  fnRecType    *fnList;
  tempStrType   pktFileName;
  s16           pktHandle;
  u16           fromNodeSec
            	, fromNodeDup
            	, fromNodeMsg;
  u16           totalMsgs;

} nodeFileRecType;


typedef nodeFileRecType *nodeFileType[MAX_OUTPKT];


//---------------------------------------------------------------------------
typedef struct
{
  uchar          *areaName;
  uchar          *comment;
  u16             boardNumRA;
  uchar           msgBaseType;
  uchar          *msgBasePath;
  u16             board;
  areaOptionsType options;
  u16             address;
  u32             group;

} rawEchoTypeX;
//---------------------------------------------------------------------------

typedef rawEchoTypeX *areaInfoPtr;
typedef areaInfoPtr   areaInfoPtrArr[MAX_AREAS];

typedef rawEchoType  *areaInfoPtrO;
typedef areaInfoPtrO  areaInfoPtrArrO[MAX_AREAS];

#define MAX_BAD_ECHOS 50

s16  makeNFInfo    (nodeFileRecType *nfInfo, s16 srcAka, nodeNumType *destNode);
void initAreaInfo  (void);
void deInitAreaInfo(void);

//---------------------------------------------------------------------------
#define ETN_INDEX(i) (i >> 3)
#define ETN_SET(i)   (1 << (2*(i & 7)))
#define ETN_SETWO(i) (2 << (2*(i & 7)))
#define ETN_SETRO(i) (3 << (2*(i & 7)))
#define ETN_RESET(i) (~(3 << (2*(i & 7))))
#define ETN_ANYACCESS(h,i)    (((h) & ETN_SETRO(i)) != 0)
#define ETN_READACCESS(h,i)   (((h) & ETN_SET(i)) != 0)
#define ETN_WRITEACCESS(h,i)  ((((h) & ETN_SETRO(i)) == ETN_SET(i)) || (((h) & ETN_SETRO(i)) == ETN_SETWO(i)))
//---------------------------------------------------------------------------
#endif  // areainfoH
