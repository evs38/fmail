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


typedef struct
{
   unsigned seenBy      : 1;
   unsigned security    : 1;
   unsigned passThrough : 1;
   unsigned visible     : 1;
   unsigned deleted     : 1;
   unsigned cpp         : 1;
   unsigned tinySeenBy  : 1;
   unsigned pvt         : 1;
   unsigned             : 8;    } tsAreaFlagsType;


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
   uchar           unused4 [10];    } fdAreaFileType;


typedef struct
{
   unsigned       : 1;
   unsigned crash : 1;
   unsigned       : 7;
   unsigned hold  : 1;
   unsigned       : 6;    } tsNodeFlagsType;


typedef struct
{
   nodeNumType     nodeNum;
   tsNodeFlagsType tsNodeFlags;
   uchar           password[17];
   uchar           archiver;
   u32             groups;
   u32             deleted;
   uchar           unused[23];  } fdNodeFileType;

/*
typedef struct
{
   unsigned active      : 1;
   unsigned passThrough : 1;
   unsigned tinySeenBy  : 1;
   unsigned security    : 1;
   unsigned keepSeenBy  : 1;
   unsigned             : 3;    } imArFlagsType;

typedef struct
{
   uchar         areaName[41];
   uchar         comment[61];
   uchar         boardNum;
   uchar         group;
   uchar         origin[64];
   uchar         originAka;
   u16           seenby;
   uchar         unused1[14];
   imArFlagsType imArFlags;
   uchar         unused2[12];
   uchar         numDays;
   u16           numMsgs;
   uchar         unused3[5];
   nodeNumType   export[60];   } imailArType;
*/

typedef struct
{                   /* Used mainly by IUTIL */
   u16     th_day;                      /* this day   */
   u16     la_day;                      /* last day   */
   u16     th_week;                     /* this week  */
   u16     la_week;                     /* last week  */
   u16     th_month;                    /* this month */
   u16     la_month;                    /* last month */
   u16     th_year;                     /* this year  */
   u16     la_year;                     /* last year  */
} imStatsType;

typedef struct
{
   expOnly  : 1;
   impOnly  : 1;
   paused   : 1;
   reserved : 5;  } expInfoType;

typedef struct
{
   nodeNumType     expNode;
   expInfoType     expInfo;     } eaddressType;

typedef struct
{
   unsigned active      : 1;
   unsigned zoneGate    : 1;
   unsigned tinySeenBy  : 1;
   unsigned security    : 1;
   unsigned keepSeenBy  : 1;
   unsigned deleted     : 1;
   unsigned autoAdded   : 1;
   unsigned mandatory   : 1;
   unsigned readOnly    : 1;
   unsigned unlinked    : 1;
   unsigned ulnkReq     : 1;
   unsigned hidden      : 1;
   unsigned toLink      : 1;
   unsigned reserved    : 3;   } imArFlagsType;

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
   eaddressType  export[60];   } imailArType;

/*
typedef struct
{
   unsigned remMaint    :  1;
   unsigned direct      :  1;
   unsigned             : 14;   } imNdFlagsType;


typedef struct
{
   nodeNumType   nodeNum;
   uchar         password[21];
   uchar         outStatus;
   uchar         archiver;
   uchar         groups[27];
   u16           capability;
   imNdFlagsType imNdFlags;
/* uchar         unused[8]; IMAIL 1.10 */
   uchar         unused[35];    } imailNdType;
*/

typedef struct
{
   unsigned remMaint       : 1;
   unsigned reserved1      : 1;
   unsigned autoCapability : 1;
   unsigned autoAdded      : 1;
   unsigned newAreaAdd     : 1;
   unsigned newAreaCreate  : 1;
   unsigned allowRescan    : 1;
   unsigned notify         : 1;
   unsigned reserved2      : 2;
   unsigned forwardReq     : 1;
   unsigned uplink         : 1;
   unsigned fscAreaLink    : 1;
   unsigned changePacker   : 1;
   unsigned reserved3      : 2;   } imNdFlagsType;


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
   uchar         filler[8];   }  imailNdType;



s16 importAreasBBS (void);
s16 importFolderFD (void);
s16 importAreaFileFD (void);
s16 importNodeFileFD (void);
s16 importGechoAr (void);
s16 importGechoNd (void);
s16 importFEAr (void);
s16 importFENd (void);
s16 importImailAr (void);
s16 importImailNd (void);
s16 importRAInfo (void);
s16 importNAInfo (void);

