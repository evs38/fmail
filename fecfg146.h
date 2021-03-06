#ifndef fecfcg146H
#define fecfcg146H

/********************************************************/
/* 'C' Structures of FastEcho 1.46                      */
/* Copyright (c) 1997 by Tobias Burchhardt              */
/* Last update: 01 Apr 1997                             */
/********************************************************/

/********************************************************/
/* FASTECHO.CFG = <CONFIG>                              */
/*                + <optional extensions>               */
/*                + <CONFIG.NodeCnt * Node>             */
/*                + <CONFIG.AreaCnt * Area>             */
/********************************************************/

#include "gestruct.h"

#define REVISION        6       /* current revision     */

/* Note: there is a major change in this revision - the
         Node records have no longer a fixed length !   */

#define FE_MAX_AREAS       4096    /* max # of areas       	*/
#define FE_MAX_NODES       1024    /* max # of nodes       	*/
#define FE_MAX_GROUPS      32      /* max # of groups      	*/
#define FE_MAX_AKAS        32      /* max # of akas        	*/
#define FE_MAX_ROUTE       15      /* max # of 'vias'      	*/
#define FE_MAX_ORIGINS     20      /* max # of origins     	*/
#define FE_MAX_GATES	10	/* max # of Internet gates 	*/

/*
  Note: The FE_MAX_AREAS and FE_MAX_NODES are only the absolute maximums
        as the handling is flexible. To get the maximums which are
        used for the config file you read, you have to examine the
        CONFIG.MaxAreas and CONFIG.MaxNodes variables !

  Note: The FE_MAX_AREAS and FE_MAX_NODES maximums are subject to change
        with any new version, therefore - if possible - make hand-
        ling as flexible  possible  and  use  CONFIG.MaxAreas  and
        .MaxNodes whereever possible. But be aware that you  might
        (under normal DOS and depending on the way you handle  it)
        hit the 64kB segment limit pretty quickly!

        Same goes for the # of AKAs and Groups -  use  the  values
        found in CONFIG.AkaCnt and CONFIG.GroupCnt!

  Note: Define INC_FE_TYPES, INC_FE_BAMPROCS  and  INC_FE_DATETYPE
        to include the typedefs if necessary.
*/

/********************************************************/
/* CONFIG.flags                                         */
/********************************************************/
/*
#define RETEAR                  0x00000001l
#define AUTOCREATE              0x00000002l
#define KILLEMPTY               0x00000004l
#define KILLDUPES               0x00000008l
#define CLEANTEARLINE           0x00001000l
#define IMPORT_INCLUDEUSERSBBS  0x00002000l
#define KILLSTRAYATTACHES       0x00004000l
#define PURGE_PROCESSDATE       0x00008000l
#define MAILER_RESCAN           0x00010000l
#define EXCLUDE_USERS           0x00020000l
#define EXCLUDE_SYSOPS          0x00040000l
#define CHECK_DESTINATION       0x00080000l
#define UPDATE_BBS_CONFIG       0x00100000l
#define KILL_GRUNGED_DATE       0x00200000l
#define NOT_BUFFER_EMS          0x00400000l
#define KEEP_NETMAILS           0x00800000l
#define NOT_UPDATE_MAILER       0x01000000l
#define NOT_CHECK_SEMAPHORES    0x02000000l
#define CREATE_SEMAPHORES       0x04000000l
#define CHECK_COMPLETE          0x08000000l
#define RESPOND_TO_RRQ          0x10000000l
#define TEMP_OUTB_HARDDISK      0x20000000l
#define FORWARD_PACKETS         0x40000000l
#define UNPACK_UNPROTECTED      0x80000000l
*/
/********************************************************/
/* CONFIG.mailer                                        */
/********************************************************/
#define FrontDoor               0x0001
#define InterMail               0x0002
#define DBridge                 0x0004
#define Binkley                 0x0010
#define PortalOfPower           0x0020
#define McMail                  0x0040

/********************************************************/
/* CONFIG.BBSSoftware                                   */
/********************************************************/
enum BBSSoft
{
  NoBBSSoft = 0
, RemoteAccess111
, QuickBBS
, SuperBBS
, ProBoard122 /* Unused */
, TagBBS
, RemoteAccess200
, ProBoard130 /* Unused */
, ProBoard200
, ProBoard212
, Maximus202
, Maximus300
};

/********************************************************/
/* CONFIG.CC.what                                       */
/********************************************************/
#define CC_FROM                 1
#define CC_TO                   2
#define CC_SUBJECT              3
#define CC_KLUDGE               4

/********************************************************/
/* CONFIG.QuietLevel                                    */
/********************************************************/
#define QUIET_PACK              0x0001
#define QUIET_UNPACK            0x0002
#define QUIET_EXTERN            0x0004

/********************************************************/
/* CONFIG.Swapping                                      */
/********************************************************/
#define SWAP_TO_XMS             0x0001
#define SWAP_TO_EMS             0x0002
#define SWAP_TO_DISK            0x0004

/********************************************************/
/* CONFIG.Buffers                                       */
/********************************************************/
#define BUF_LARGE               0x0000
#define BUF_MEDIUM              0x0001
#define BUF_SMALL               0x0002

/********************************************************/
/* CONFIG.arcext.inb/outb                               */
/********************************************************/
enum ARCmailExt
{
  ARCDigits = 0
, ARCHex
, ARCAlpha
};

/********************************************************/
/* CONFIG.AreaFixFlags                                  */
/********************************************************/
/*
#define ALLOWRESCAN             0x0001
#define KEEPREQUEST             0x0002
#define KEEPRECEIPT             0x0004
#define ALLOWREMOTE             0x0008
#define DETAILEDLIST            0x0010
#define ALLOWPASSWORD           0x0020
#define ALLOWPKTPWD             0x0040
#define ALLOWCOMPRESS           0x0080
#define SCANBEFORE              0x0100
#define ADDRECEIPTLIST          0x0200
#define NOTIFYPASSWORDS         0x0400
*/
/********************************************************/
/* Area.board (1-200 = Hudson)                          */
/********************************************************/
#define NO_BOARD        0x4000u /* JAM/Sq/Passthru etc. */
#define AREA_DELETED    0x8000u /* usually never written*/

/********************************************************/
/* Area.flags.storage                                   */
/********************************************************/
#define FE_QBBS                    0
#define FE_FIDO                    1
#define FE_SQUISH                  2
#define FE_JAM                     3
#define FE_PASSTHRU                7

/********************************************************/
/* Area.flags.atype                                     */
/********************************************************/
#define AREA_ECHOMAIL           0
#define AREA_NETMAIL            1
#define AREA_LOCAL              2
#define AREA_BADMAILBOARD       3
#define AREA_DUPEBOARD          4

/********************************************************/
/* GateDef.flags                                        */
/********************************************************/
#define GATE_KEEPMAILS	0x0001

/********************************************************/
/* Types and other definitions                          */
/********************************************************/
#ifdef INC_FE_TYPES
typedef unsigned char byte;
typedef unsigned short word;     /* normal int = 16 bit */
typedef unsigned long dword;
#endif

enum ARCers
{
  ARC_Unknown = -1
, ARC_SeaArc
, ARC_PkArc
, ARC_Pak
, ARC_ArcPlus
, ARC_Zoo
, ARC_PkZip
, ARC_Lha
, ARC_Arj
, ARC_Sqz
, ARC_RAR
, ARC_UC2
}; /* for Unpackers */

enum NetmailStatus
{
  NetNormal = 0
, NetHold
, NetCrash
/*, NetImm */
};

enum AreaFixType
{
  NoAreaFix = 0
, NormalAreaFix
, FSC57AreaFix
};

enum AreaFixSendTo
{
  AreaFix = 0
, AreaMgr
, AreaLink
, EchoMgr
};

/********************************************************/
/* Structures                                           */
/********************************************************/

#include <pshpack1.h>

typedef struct
{
  word zone
     , net
     , node
     , point;
} Address;

#define _MAXPATH 56

typedef struct CONFIGURATION
{
 word revision;
 dword flags;
 word NodeCnt,AreaCnt,unused1;
 char NetMPath[_MAXPATH],
      MsgBase[_MAXPATH],
      InBound[_MAXPATH],
      OutBound[_MAXPATH],
      Unpacker[_MAXPATH],               // DOS default decompression program
      LogFile[_MAXPATH],
      unused2[336],
      Unpacker2[_MAXPATH],              // OS/2 default decompression program
      UnprotInBound[_MAXPATH],
      StatFile[_MAXPATH],
      SwapPath[_MAXPATH],
      SemaphorePath[_MAXPATH],
      BBSConfigPath[_MAXPATH],
      QueuePath[_MAXPATH],
      RulesPrefix[32],
      RetearTo[40],
      LocalInBound[_MAXPATH],
      ExtAfter[_MAXPATH-4],
      ExtBefore[_MAXPATH-4];
 byte unused3[480];
 struct
 {
  byte what;
  char object[31];
  word conference;
 } CC[10];
 byte security,loglevel;
 word def_days,def_messages;
 byte unused4[462];
 word autorenum;
 word def_recvdays;
 byte openQQQs,Swapping;
 word compressafter;
 word afixmaxmsglen;
 word compressfree;
 char TempPath[_MAXPATH];
 byte graphics,BBSSoftware;
 char AreaFixHelp[_MAXPATH];
 byte unused5[504];
 word AreaFixFlags;
 byte QuietLevel,Buffers;
 byte FWACnt,GDCnt;             /* # of ForwardAreaFix records,
                                   # of Group Default records */
 struct
 {
  word flags;
  word days[2];
  word msgs[2];
 } rescan_def;
 dword duperecords;
 struct
 {
  byte inb;
  byte outb;
 } arcext;
 word AFixRcptLen;
 byte AkaCnt,resv;              /* # of Aka records stored */
 word maxPKT;
 byte sharing,sorting;
 struct
 {
  char name[36];
  dword resv;
 } sysops[11];
 char AreaFixLog[_MAXPATH];
 char TempInBound[_MAXPATH];
 word maxPKTmsgs;
 word RouteCnt;                 /* # of PackRoute records */
 byte maxPACKratio;
 byte SemaphoreTimer;
 byte PackerCnt,UnpackerCnt;    /* # of Packers and Unpackers records */
 byte GroupCnt,OriginCnt;       /* # of GroupNames and Origin records */
 word mailer;
 word maxarcsize,maxarcdays;
 word minInbPKTsize;
 char reserved[804];
 word AreaRecSize,GrpDefRecSize;      /* Size  of  Area  and  GroupDefaults
                                         records stored in this file        */
 word MaxAreas,MaxNodes;              /* Current max values for this config */
 word NodeRecSize;                    /* Size of each stored Node record    */
 dword offset;                        /* This is the offset from the current
                                         file-pointer to the 1st Node       */
} CONFIG;

/* To directly access the 'Nodes' and/or 'Areas' while bypassing the */
/* Extensions, perform an absolute (from beginning of file) seek to  */
/*                   sizeof(CONFIG) + CONFIG.offset                  */
/* If you want to access the 'Areas', you have to add the following  */
/* value to the above formula:  CONFIG.NodeCnt * CONFIG.NodeRecSize  */

typedef struct
{
 Address addr;                  /* Main address                          */
 Address arcdest;               /* ARCmail fileattach address            */
 byte aka,autopassive,newgroup,resv1;
 struct
 {
  u32 passive        : 1;
  u32 dddd           : 1;    /* Type 2+/4D                            */
  u32 arcmail060     : 1;
  u32 tosscan        : 1;
  u32 umlautnet      : 1;
  u32 exportbyname   : 1;
  u32 allowareacreate: 1;
  u32 disablerescan  : 1;
  u32 arc_status     : 2;    /* NetmailStatus for ARCmail attaches    */
  u32 arc_direct     : 1;    /* Direct flag for ARCmail attaches      */
  u32 noattach       : 1;    /* don't create a ARCmail file attach    */
  u32 mgr_status     : 2;    /* NetMailStatus for AreaFix receipts    */
  u32 mgr_direct     : 1;    /* Direct flag for ...                   */
  u32 not_help       : 1;
  u32 not_notify     : 1;
  u32 packer         : 4;    /* # of Packer used, 0xf = send .PKT     */
  u32 packpriority   : 2;    /* system has priority packing ARCmail   */
  u32 resv           : 1;
 } flags;                       /* 24 bits total !                       */
 struct
 {
  u16 type       : 2;    /* Type of AreaFix: None (human),
                                   Normal or Advanced (FSC-57)           */
  u16 noforward  : 1;    /* Don't forward AFix requests           */
  u16 allowremote: 1;
  u16 allowdelete: 1;    /* flags for different FSC-57 requests   */
  u16 allowrename: 1;    /* all 3 reserved for future use         */
  u16 binarylist : 1;
  u16 addplus    : 1;    /* add '+' when requesting new area      */
  u16 addtear    : 1;    /* add tearline to the end of requests   */
  u16 sendto     : 3;    /* name of this systems's AreaFix robot  */
  u16 resv       : 4;
 } afixflags;
 word resv2;
 char password[9];              /* .PKT password                         */
 char areafixpw[9];             /* AreaFix password                      */
 word sec_level;
 dword groups;                  /* Bit-field, Byte 0/Bit 7 = 'A' etc.    */
                                /* FALSE means group is active           */
 dword resv3;
 word resv4;
 word maxarcsize;
 char name[36];                 /* Name of sysop                         */
 byte areas[1];                 /* Bit-field with CONFIG.MaxAreas / 8
                                   bits, Byte 0/Bit 7 is conference #0   */
} Node;                         /* Total size of each record is stored
                                   in CONFIG.NodeRecSize                 */

typedef struct
{
 char name[52];
 word board;                    /* 1-200 Hudson, others reserved/special */
 word conference;               /* 0 ... CONFIG.MaxAreas-1               */
 word read_sec,write_sec;
 struct
 {
  u16 aka  : 8;              /* 0 ... CONFIG.AkaCnt                   */
  u16 group: 8;              /* 0 ... CONFIG.GroupCnt                 */
 } info;
 struct
 {
  u16 storage: 4;
  u16 atype  : 4;
  u16 origin : 5;              /* # of origin line                      */
  u16 resv   : 3;
 } flags;
 struct
 {
  u16 autoadded : 1;
  u16 tinyseen  : 1;
  u16 cpd       : 1;
  u16 passive   : 1;
  u16 keepseen  : 1;
  u16 mandatory : 1;
  u16 keepsysop : 1;
  u16 killread  : 1;
  u16 disablepsv: 1;
  u16 keepmails : 1;
  u16 hide      : 1;
  u16 nomanual  : 1;
  u16 umlaut    : 1;
  u16 hideseen  : 1;
  u16 resv      : 2;
 } advflags;
 word resv1;
 dword seenbys;                 /* LSB = Aka0, MSB = Aka31           */
 dword resv2;
 short days;
 short messages;
 short recvdays;
 char path[_MAXPATH];
 char desc[52];
} Area;

/********************************************************/
/* Optional Extensions                                  */
/********************************************************/
/* These are the variable length extensions between     */
/* CONFIG and the first Node record. Each extension has */
/* a header which contains the info about the type and  */
/* the length of the extension. You can read the fields */
/* using the following algorithm:                       */
/*                                                      */
/* offset := 0;                                         */
/* while (offset<CONFIG.offset) do                      */
/*  read_header;                                        */
/*  if(header.type==EH_abc) then                        */
/*   read_and_process_data;                             */
/*    else                                              */
/*  if(header.type==EH_xyz) then                        */
/*   read_and_process_data;                             */
/*    else                                              */
/*   [...]                                              */
/*    else  // unknown or unwanted extension found      */
/*  seek_forward(header.offset); // Seek to next header */
/*  offset = offset + header.offset + sizeof(header);   */
/* end;                                                 */
/********************************************************/

typedef struct
{
 word type;             /* EH_...                           */
 dword offset;          /* length of field excluding header */
} ExtensionHeader;


#define EH_AREAFIX      0x0001 /* CONFIG.FWACnt * <ForwardAreaFix> */

enum AreaFixAreaListFormat { Areas_BBS = 0, Area_List };

typedef struct
{
 word nodenr;
 struct
 {
  u16 newgroup: 8;
  u16 active  : 1;
  u16 valid   : 1;
  u16 uncond  : 1;
  u16 format  : 3;
  u16 resv    : 2;
 } flags;
 char file[_MAXPATH];
 word sec_level;
 word resv1;
 dword groups;
 char resv2[4];
} ForwardAreaFix;

#define EH_GROUPS       0x000C /* CONFIG.GroupCnt * <GroupNames> */

typedef struct
{
 char name[36];
} GroupNames;

#define EH_GRPDEFAULTS  0x0006  /* CONFIG.GDCnt * <GroupDefaults>
                                   Size of each full GroupDefault
                                   record is CONFIG.GrpDefResSize */
typedef struct
{
 byte group;
 byte resv[15];
 Area area;
 byte nodes[1];         /* variable, c.MaxNodes / 8 bytes */
} GroupDefaults;

#define EH_AKAS         0x0007  /* CONFIG.AkaCnt * <SysAddress> */

typedef struct
{
 Address main;
 char domain[28];
 word pointnet;
 dword flags;           /* unused       */
} SysAddress;

#define EH_ORIGINS      0x0008  /* CONFIG.OriginCnt * <OriginLines> */

typedef struct
{
 char line[62];
} OriginLines;

#define EH_PACKROUTE    0x0009  /* CONFIG.RouteCnt * <PackRoute> */

typedef struct
{
 Address dest;
 Address routes[FE_MAX_ROUTE];
} PackRoute;

#define EH_PACKERS      0x000A  /* CONFIG.Packers * <Packers> (DOS)  */
#define EH_PACKERS2     0x100A  /* CONFIG.Packers * <Packers> (OS/2) */

typedef struct
{
 char tag[6];
 char command[_MAXPATH];
 char list[4];
 byte ratio;
 byte resv[7];
} Packers;

#define EH_UNPACKERS    0x000B  /* CONFIG.Unpackers * <Unpackers> (DOS)  */
#define EH_UNPACKERS2   0x100B  /* CONFIG.Unpackers * <Unpackers> (OS/2) */

/* Calling convention:                                      */
/* 0 = change path to inbound directory, 1 = <path> *.PKT,  */
/* 2 = *.PKT <path>, 3 = *.PKT #<path>, 4 = *.PKT -d <path> */

typedef struct
{
 char command[_MAXPATH];
 byte callingconvention;
 byte resv[7];
} Unpackers;

#define EH_RA111_MSG    0x0100  /* Original records of BBS systems */
#define EH_QBBS_MSG     0x0101
#define EH_SBBS_MSG     0x0102
#define EH_TAG_MSG      0x0104
#define EH_RA200_MSG    0x0105
#define EH_PB200_MSG    0x0106
#define EH_PB211_MSG    0x0107  /* See BBS package's documentation */
#define EH_MAX202_MSG   0x0108  /* for details                     */


/********************************************************/
/* Routines to access Node.areas, Node.groups           */
/********************************************************/

#ifdef INC_FE_BAMPROCS

word AddBam(byte *bam,word nr)
{
byte c=(1<<(7-(nr&7))),d;

 d=bam[nr/8]&c;
 bam[nr/8]|=c;
 return(d);
}

void FreeBam(byte *bam,word nr)
{
 bam[nr/8]&=~(1<<(7-(nr&7)));
}

word GetBam(byte *bam,word nr)
{
 if(bam[nr/8]&(1<<(7-(nr&7)))) return(TRUE);
 return(FALSE);
}

#define IsActive(nr,area)      GetBam(Node[nr].areas,area)
#define SetActive(nr,area)     AddBam(Node[nr].areas,area)
#define SetDeActive(nr,area)   FreeBam(Node[nr].areas,area)

#endif

/********************************************************/
/* FASTECHO.DAT = <STATHEADER>                          */
/*                + <STATHEADER.NodeCnt * StatNode>     */
/*                + <STATHEADER.AreaCnt * StatArea>     */
/********************************************************/

#define STAT_REVISION       3   /* current revision     */

//#ifdef INC_FE_DATETYPE
struct date                     /* Used in FASTECHO.DAT */
{
 word year;
 byte day;
 byte month;
};
//#endif

typedef struct
{
 char signature[10];            /* contains 'FASTECHO\0^Z'      */
 word revision;
 struct date lastupdate;        /* last time file was updated   */
 word NodeCnt,AreaCnt;
 dword startnode,startarea;     /* unix timestamp of last reset */
 word NodeSize,AreaSize;        /* size of StatNode and StatArea records */
 char resv[32];
} STATHEADER;

typedef struct
{
 Address adr;
 dword import,export;
 struct date lastimport,lastexport;
 dword dupes;
 dword importbytes,exprotbytes;
} StatNode;

typedef struct
{
 word conference;               /* conference # of area */
 dword tagcrc;                  /* CRC32 of area tag    */
 dword import,export;
 struct date lastimport,lastexport;
 dword dupes;
} StatArea;

#include <poppack.h>

//---------------------------------------------------------------------------
#endif  // fecfcg146H
