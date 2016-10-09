#ifndef importH
#define importH
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

//---------------------------------------------------------------------------
typedef struct
{
  u16 seenBy     : 1;
  u16 security   : 1;
  u16 passThrough: 1;
  u16 visible    : 1;
  u16 deleted    : 1;
  u16 cpp        : 1;
  u16 tinySeenBy : 1;
  u16 pvt        : 1;
  u16            : 8;
} tsAreaFlagsType;
//---------------------------------------------------------------------------
typedef struct
{
  uchar           areaName[51];
  uchar           unused1 [51];
  tsAreaFlagsType tsAreaFlags;
  nodeNumType     export[60];
  uchar           unused2 [4];
  uchar           group;
  uchar           boardNum;
  uchar           mainAddress;
  uchar           unused3;
  u16             numMsgs;
  u16             numDays;
  u32             deleted;
  uchar           comment [40];
  uchar           unused4 [10];
} fdAreaFileType;
//---------------------------------------------------------------------------
typedef struct
{
  u16      : 1;
  u16 crash: 1;
  u16      : 7;
  u16 hold : 1;
  u16      : 6;
} tsNodeFlagsType;
//---------------------------------------------------------------------------
typedef struct
{
  nodeNumType     nodeNum;
  tsNodeFlagsType tsNodeFlags;
  uchar           password[17];
  uchar           archiver;
  u32             groups;
  u32             deleted;
  uchar           unused[23];
} fdNodeFileType;
//---------------------------------------------------------------------------
typedef struct
{                 // Used mainly by IUTIL
   u16 th_day;    // this day
   u16 la_day;    // last day
   u16 th_week;   // this week
   u16 la_week;   // last week
   u16 th_month;  // this month
   u16 la_month;  // last month
   u16 th_year;   // this year
   u16 la_year;   // last year
} imStatsType;
//---------------------------------------------------------------------------
typedef struct
{
  u8 expOnly : 1;
  u8 impOnly : 1;
  u8 paused  : 1;
  u8 reserved: 5;
} expInfoType;
//---------------------------------------------------------------------------
typedef struct
{
  nodeNumType expNode;
  expInfoType expInfo;
} eaddressType;
//---------------------------------------------------------------------------
typedef struct
{
  u16 active    : 1;
  u16 zoneGate  : 1;
  u16 tinySeenBy: 1;
  u16 security  : 1;
  u16 keepSeenBy: 1;
  u16 deleted   : 1;
  u16 autoAdded : 1;
  u16 mandatory : 1;
  u16 readOnly  : 1;
  u16 unlinked  : 1;
  u16 ulnkReq   : 1;
  u16 hidden    : 1;
  u16 toLink    : 1;
  u16 reserved  : 3;
} imArFlagsType;
//---------------------------------------------------------------------------
typedef struct
{
  uchar         areaName[51];
  uchar         comment[61];
  uchar         origin[64];
  uchar         group;
  uchar         originAka;
  uchar         useAkas[16];
  uchar         msgBaseType;
  uchar         boardNum;
  uchar         path[80];
  imArFlagsType imArFlags;
  uchar         IUtilBits;
  uchar         userBits;
  uchar         numDays;
  u16           numMsgs;
  imStatsType   t_stats;
  imStatsType   s_stats;
  imStatsType   d_stats;
  time_t        creation;
  time_t        update;
  time_t        marked;
  uchar         filler[10];
  eaddressType  export[60];
} imailArType;
//---------------------------------------------------------------------------
typedef struct
{
  u16 remMaint      : 1;
  u16 reserved1     : 1;
  u16 autoCapability: 1;
  u16 autoAdded     : 1;
  u16 newAreaAdd    : 1;
  u16 newAreaCreate : 1;
  u16 allowRescan   : 1;
  u16 notify        : 1;
  u16 reserved2     : 2;
  u16 forwardReq    : 1;
  u16 uplink        : 1;
  u16 fscAreaLink   : 1;
  u16 changePacker  : 1;
  u16 reserved3     : 2;
} imNdFlagsType;
//---------------------------------------------------------------------------
typedef struct
{
  nodeNumType   nodeNum;
  uchar         sysopName[37];
  uchar         domain;
  uchar         password[21];
  uchar         attachStatus;
  uchar         archiver;
  uchar         groups[27];
  u16           capability;
  imNdFlagsType imNdFlags;
  u16           userBits;
  uchar         newAreasDefGroup;
  uchar         packetPwd[9];
  s32           lastPwdChange;
  uchar         toProgram[11];
  uchar         msgStatus;
  uchar         filler[8];
} imailNdType;
//---------------------------------------------------------------------------
s16 importAreasBBS  (void);
s16 importFolderFD  (void);
s16 importAreaFileFD(void);
s16 importNodeFileFD(void);
s16 importGechoAr   (void);
s16 importGechoNd   (void);
s16 importFEAr      (void);
s16 importFENd      (void);
s16 importImailAr   (void);
s16 importImailNd   (void);
s16 importRAInfo    (void);
s16 importNAInfo    (void);
//---------------------------------------------------------------------------
#endif
