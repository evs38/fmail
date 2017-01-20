#ifndef fmstructH
#define fmstructH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2017  Wilfred van Velzen
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
//
//  File structures for FMail
//
//  All information in this document is subject to change at any time
//  without prior notice!
//
//  Strings are NUL terminated arrays of char type.
//  Path names always end on a \ character (followed by NUL).
//
//---------------------------------------------------------------------------

#if (__BORLANDC__ >= 0x0500 || __GNUC__ >= 3) && defined(__32BIT__)
#define __CANUSE64BIT
#endif

#include <stdint.h>
#include <time.h>

// **** Modify the type definitions below if necessary for your compiler ****

#define fhandle int
#define u8      uint8_t
#define uchar   unsigned char
#define u16     uint16_t
#define s16     int16_t
#define u32     uint32_t
#define s32     int32_t
#define udef    unsigned int
//#define sdef  signed int
#ifdef __CANUSE64BIT
#define u64     uint64_t
#endif

//#define MAXU16 0xFFFF
//#define MAXU32 0xFFFFFFFFL


// ********** General structures **********

#include <pshpack1.h>

typedef struct
{
  char programName[46];
  u16  reserved;        // Was: memRequired;
} archiverInfo;

typedef char pathType[48];

typedef struct
{
  u16   zone;
  u16   net;
  u16   node;
  u16   point;
} nodeNumType;

typedef struct
{
  u16 readOnly   : 1;  // Bit  0
  u16 writeOnly  : 1;  // Bit  1
  u16 locked     : 1;  // Bit  2
  u16 reserved   : 13; // Bits 3-15
} nodeFlagsType;

typedef struct
{
  nodeNumType   nodeNum;
  nodeFlagsType flags;
} nodeNumXType;

typedef struct
{
  nodeNumType nodeNum;
  u16         fakeNet;
} nodeFakeType;


// ********** File header structure **********

#define DATATYPE_CF    0x0102 /* not used yet			  */
#define DATATYPE_NO    0x0202 /* node file                        */
#define DATATYPE_AD    0x0401 /* area file for echo mail defaults */
#define DATATYPE_AE    0x0402 /* area file for echo mail          */

typedef struct
{
  char   versionString[32]; /* Always starts with 'FMail' */
  u16    revNumber;         /* Is now 0x0100 */
  u16    dataType;          /* See #defines above */
  u16    headerSize;
  time_t creationDate;
  time_t lastModified;
  u16    totalRecords;
  u16    recordSize;
} headerType;


// The structure below is used by the Areas File and (only partly)
// by the Config File

typedef struct
{
  u16 active      : 1; // Bit  0
  u16 tinySeenBy  : 1; // Bit  1
  u16 security    : 1; // Bit  2
  u16 noRescan    : 1; // Bit  3
  u16 allowPrivate: 1; // Bit  4
  u16 impSeenBy   : 1; // Bit  5
  u16 checkSeenBy : 1; // Bit  6
  u16 tinyPath    : 1; // Bit  7
  u16 local       : 1; // Bit  8
  u16 disconnected: 1; // Bit  9
  u16 _reserved   : 1; // Bit 10
  u16 allowAreafix: 1; // Bit 11
  u16 export2BBS  : 1; // Bit 12
  u16             : 1; // Bit 13
  u16 arrivalDate : 1; // Bit 14
  u16 sysopRead   : 1; // Bit 15
} areaOptionsType;


// ********** FMAIL.CFG **********

#define dCFGFNAME     "fmail.cfg"

#define MAX_AKAS      32
#define MAX_AKAS_F    64
#define MAX_AKAS_OLD  16
#define MAX_NA_OLD    11
#define MAX_NETAKAS   32
#define MAX_NETAKAS_F 64
#define MAX_USERS     16
#define MAX_UPLREQ    32
#define MAX_MATCH     16           // not used yet

#define BBS_RA1X      0
#define BBS_SBBS      1
#define BBS_QBBS      2
#define BBS_TAG       3
#define BBS_RA20      4
#define BBS_RA25      5
#define BBS_PROB      6
#define BBS_ELEB      7

#define LOG_NEVER     0x0000
#define LOG_INBOUND   0x0001
#define LOG_OUTBOUND  0x0002
#define LOG_PKTINFO   0x0004
#define LOG_XPKTINFO  0x0008
#define LOG_UNEXPPWD  0x0010
#define LOG_SENTRCVD  0x0020
#define LOG_STATS     0x0040
#define LOG_PACK      0x0080
#define LOG_MSGBASE   0x0100
#define LOG_ECHOEXP   0x0200
#define LOG_NETIMP    0x0400
#define LOG_NETEXP    0x0800
#define LOG_OPENERR   0x1000
#define LOG_EXEC      0x2000
#define LOG_NOSCRN    0x4000
#define LOG_ALWAYS    0x8000
#define LOG_DEBUG     0x8000

typedef nodeFakeType _akaListType[MAX_AKAS_OLD];
typedef nodeFakeType  akaListType[MAX_AKAS_F  ];

typedef struct
{
  u16              : 1;  // BIT 0  (was useEMS)
  u16              : 1;  // BIT 1  (was checkBreak)
  u16              : 1;  // BIT 2  (was swap)
  u16              : 1;  // BIT 3  (was swapEMS)
  u16              : 1;  // BIT 4  (was swapXMS)
  u16              : 1;  // BIT 5  (was lfn)
  u16 monochrome   : 1;  // BIT 6
  u16 commentFFD   : 1;  // BIT 7
  u16 PTAreasBBS   : 1;  // BIT 8
  u16 commentFRA   : 1;  // BIT 9
  u16              : 1;  // BIT 10
  u16 incBDRRA     : 1;  // BIT 11
  u16 ctrlcodes    : 1;  // BIT 12
  u16 timeSliceFM  : 1;  // BIT 13
  u16 timeSliceFT  : 1;  // BIT 14
  u16 _RA2         : 1;  // BIT 15
} genOptionsType;

typedef struct
{
  u16 removeNetKludges : 1;  // Bit 0
  u16 addPointToPath   : 1;  // Bit 1
  u16 checkPktDest     : 1;  // Bit 2
  u16 neverARC060      : 1;  // Bit 3
  u16 createSema       : 1;  // Bit 4
  u16 dailyMail        : 1;  // Bit 5
  u16 warnNewMail      : 1;  // bit 6
  u16 killBadFAtt      : 1;  // Bit 7
  u16 dupDetection     : 1;  // Bit 8
  u16 ignoreMSGID      : 1;  // Bit 9
  u16 ARCmail060       : 1;  // Bit 10
  u16 extNames         : 1;  // Bit 11
  u16 persNetmail      : 1;  // Bit 12
  u16 privateImport    : 1;  // Bit 13
  u16 keepExpNetmail   : 1;  // Bit 14
  u16 killEmptyNetmail : 1;  // Bit 15
} mailOptionsType;

typedef struct
{
  u16 sortNew      : 1;  // bit  0
  u16 sortSubject  : 1;  // bit  1
  u16 updateChains : 1;  // bit  2
  u16 reTear       : 1;  // bit  3
  u16              : 1;  // bit  4
  u16 removeLf     : 1;  // bit  5
  u16 removeRe     : 1;  // bit  6
  u16 removeSr     : 1;  // bit  7
  u16 scanAlways   : 1;  // bit  8
  u16 scanUpdate   : 1;  // bit  9
  u16              : 1;  // bit 10  // Was: mbSharing
  u16              : 1;  // bit 11
  u16 quickToss    : 1;  // bit 12
  u16 checkNetBoard: 1;  // bit 13
  u16 persSent     : 1;  // bit 14
  u16 sysopImport  : 1;  // bit 15
} mbOptionsType;

typedef struct
{
  u16 keepRequest  : 1;  // Bit  0
  u16 keepReceipt  : 1;  // Bit  1
  u16 sendUplArList: 1;  // Bit  2
  u16              : 1;  // Bit  3
  u16 autoDiscArea : 1;  // Bit  4
  u16 autoDiscDel  : 1;  // Bit  5 has temp. no effect, rec is always deleted
  u16              : 3;  // Bit 6-8
  u16 allowAddAll  : 1;  // Bit  9
  u16 allowActive  : 1;  // Bit 10
  u16 allowBCL     : 1;  // Bit 11
  u16 allowPassword: 1;  // Bit 12
  u16 allowPktPwd  : 1;  // Bit 13
  u16 allowNotify  : 1;  // Bit 14
  u16 allowCompr   : 1;  // Bit 15
} mgrOptionsType;

typedef struct
{
  u8 disablePing         : 1;  // Bit  0
  u8 disableTrace        : 1;  // Bit  1
  u8 deletePingRequests  : 1;  // Bit  2
  u8                     : 1;  // Bit  3
  u8 deletePingResponses : 1;  // Bit  4
  u8 deleteTraceResponses: 1;  // Bit  5
  u8                     : 1;  // Bit  6
  u8                     : 1;  // Bit  7
} pingOptionsType;

#ifdef __BORLANDC__
# if sizeof(pingOptionsType) != sizeof(u8)
#  error "*** Error: sizeof(pingOptionsType) != sizeof(u8)"
# endif
#endif

typedef struct
{
  u16 smtpImm : 1;   // BIT 0
  u16         : 15;  // BIT 1-15
} inetOptionsType;

typedef struct
{
  u16 addPlusPrefix :  1; // BIT 0
  u16               :  3;
  u16 unconditional :  1; // BIT 4
  u16               : 11;
} uplOptType;

typedef struct
{
  char  userName[36];
  uchar reserved[28];
} userType;

typedef struct
{
  nodeNumType node;
  char        program[9];
  char        password[17];
  char        fileName[13];
  u8          fileType;
  u32         groups;
  u8          originAka;
  uplOptType  options;
  u8          bclReport;
  u8          reserved[8];
} uplinkReqType;

typedef struct
{
  u16   valid;
  u16   zone;
  u16   net;
  u16   node;
} akaMatchNodeType;

typedef struct
{
  akaMatchNodeType amNode;
  u16              aka;
} akaMatchType;

//---------------------------------------------------------------------------
// ATTENTION: FMAIL.CFG does NOT use the new config file type yet (no header) !!!
//
typedef struct
{
  u8              versionMajor;
  u8              versionMinor;
  time_t          creationDate;
  u32             key_NotUsed;
  u32             reservedKey_NotUsed;
  u32             relKey1_NotUsed;
  u32             relKey2_NotUsed;
  u8              reserved1[14];
  u32             lastUniqueID;
  inetOptionsType inetOptions;
  u16             maxForward;
  mgrOptionsType  mgrOptions;
  _akaListType    _akaList;
  u16             _netmailBoard[MAX_NA_OLD];
  u16             _reservedNet[16 - MAX_NA_OLD];
  genOptionsType  genOptions;
  mbOptionsType   mbOptions;
  mailOptionsType mailOptions;
  u16             maxPktSize;
  u16             kDupRecs;
  u16             mailer;
  u16             bbsProgram;
  u16             maxBundleSize;
  u16             reserved8;         // Was: extraHandles;  // 0-235
  u16             autoRenumber;
  u16             reserved9;         // Was: bufSize;
  u16             reserved10;        // Was: ftBufSize;
  u16             allowedNumNetmail;
  u16             logInfo;
  u16             logStyle;
  u16             activTimeOut;
  u16             maxMsgSize;
  u16             defMaxRescan;
  u16             oldMsgDays;
  pingOptionsType pingOptions;
  u8              reserved2[59];
  u16             colorSet;
  char            sysopName[36];
  u16             defaultArc;
  u16             _adiscDaysNode;
  u16             _adiscDaysPoint;
  u16             _adiscSizeNode;
  u16             _adiscSizePoint;
  u8              reserved3[16];
  uchar           tearType;
  char            tearLine[25];
  pathType        summaryLogName;
  u16             recBoard;
  u16             badBoard;
  u16             dupBoard;
  char            topic1[16];
  char            topic2[16];
  pathType        bbsPath;
  pathType        netPath;
  pathType        sentPath;
  pathType        rcvdPath;
  pathType        inPath;
  pathType        outPath;
  pathType        securePath;
  pathType        logName;
  pathType        reserve8;             // (was swapPath)
  pathType        semaphorePath;
  pathType        pmailPath;
  pathType        areaMgrLogName;
  pathType        autoRAPath;
  pathType        autoFolderFdPath;
  pathType        autoAreasBBSPath;
  pathType        autoGoldEdAreasPath;
  u8              reserved_ai_100[sizeof(archiverInfo)];  // Was: archiverInfo    unArc;
  u8              reserved_ai_101[sizeof(archiverInfo)];  // Was: archiverInfo    unZip;
  u8              reserved_ai_102[sizeof(archiverInfo)];  // Was: archiverInfo    unLzh;
  u8              reserved_ai_103[sizeof(archiverInfo)];  // Was: archiverInfo    unPak;
  u8              reserved_ai_104[sizeof(archiverInfo)];  // Was: archiverInfo    unZoo;
  u8              reserved_ai_105[sizeof(archiverInfo)];  // Was: archiverInfo    unArj;
  u8              reserved_ai_106[sizeof(archiverInfo)];  // Was: archiverInfo    unSqz;
  u8              reserved_ai_107[sizeof(archiverInfo)];  // Was: archiverInfo    GUS;
  u8              reserved_ai_108[sizeof(archiverInfo)];  // Was: archiverInfo    arc;
  u8              reserved_ai_109[sizeof(archiverInfo)];  // Was: archiverInfo    zip;
  u8              reserved_ai_110[sizeof(archiverInfo)];  // Was: archiverInfo    lzh;
  u8              reserved_ai_111[sizeof(archiverInfo)];  // Was: archiverInfo    pak;
  u8              reserved_ai_112[sizeof(archiverInfo)];  // Was: archiverInfo    zoo;
  u8              reserved_ai_113[sizeof(archiverInfo)];  // Was: archiverInfo    arj;
  u8              reserved_ai_114[sizeof(archiverInfo)];  // Was: archiverInfo    sqz;
  u8              reserved_ai_115[sizeof(archiverInfo)];  // Was: archiverInfo    customArc;
  pathType        autoFMail102Path;
  u8              reserved4[35];
  areaOptionsType _optionsAKA[MAX_NA_OLD];
  uchar           _groupsQBBS[MAX_NA_OLD];
  u16             _templateSecQBBS[MAX_NA_OLD];
  uchar           _templateFlagsQBBS[MAX_NA_OLD][4];
  uchar           _attr2RA[MAX_NA_OLD];
  uchar           _aliasesQBBS[MAX_NA_OLD];
  u16             _groupRA[MAX_NA_OLD];
  u16             _altGroupRA[MAX_NA_OLD][3];
  uchar           _qwkName[MAX_NA_OLD][13];
  u16             _minAgeSBBS[MAX_NA_OLD];
  u16             _daysRcvdAKA[MAX_NA_OLD];
  uchar           _replyStatSBBS[MAX_NA_OLD];
  u16             _attrSBBS[MAX_NA_OLD];
  char            groupDescr[26][27];
  u8              reserved5[9];
  uchar           _msgKindsRA[MAX_NA_OLD];
  uchar           _attrRA[MAX_NA_OLD];
  u16             _readSecRA[MAX_NA_OLD];
  uchar           _readFlagsRA[MAX_NA_OLD][4];
  u16             _writeSecRA[MAX_NA_OLD];
  uchar           _writeFlagsRA[MAX_NA_OLD][4];
  u16             _sysopSecRA[MAX_NA_OLD];
  uchar           _sysopFlagsRA[MAX_NA_OLD][4];
  u16             _daysAKA[MAX_NA_OLD];
  u16             _msgsAKA[MAX_NA_OLD];
  uchar           _descrAKA[MAX_NA_OLD][51];
  userType        users[MAX_USERS];
  akaMatchType    akaMatch[MAX_MATCH];     // not used yet
  u8              reserved6[496];
  pathType        inBakPath;
  pathType        outBakPath;
  char            emailAddress[80];        // max 56 chars used
  char            pop3Server[80];          // max 56 chars used
  char            smtpServer[80];          // max 56 chars used
  pathType        tossedAreasList;
  pathType        sentEchoPath;
  u8              reserved_ai_200[sizeof(archiverInfo) * 1];  // Was: archiverInfo    preUnarc;
  u8              reserved_ai_201[sizeof(archiverInfo) * 1];  // Was: archiverInfo    postUnarc;
  u8              reserved_ai_202[sizeof(archiverInfo) * 1];  // Was: archiverInfo    preArc;
  u8              reserved_ai_203[sizeof(archiverInfo) * 1];  // Was: archiverInfo    postArc;
  u8              reserved_ai_204[sizeof(archiverInfo) * 1];  // Was: archiverInfo    unUc2;
  u8              reserved_ai_205[sizeof(archiverInfo) * 1];  // Was: archiverInfo    unRar;
  u8              reserved_ai_206[sizeof(archiverInfo) * 1];  // Was: archiverInfo    unJar;
  u8              reserved_ai_207[sizeof(archiverInfo) * 5];  // Was: archiverInfo    resUnpack[5];
  u8              reserved_ai_208[sizeof(archiverInfo) * 1];  // Was: archiverInfo    uc2;
  u8              reserved_ai_209[sizeof(archiverInfo) * 1];  // Was: archiverInfo    rar;
  u8              reserved_ai_210[sizeof(archiverInfo) * 1];  // Was: archiverInfo    jar;
  u8              reserved_ai_211[sizeof(archiverInfo) * 5];  // Was: archiverInfo    resPack[5];
  uplinkReqType   uplinkReq[MAX_UPLREQ + 32];
  archiverInfo    unArc32;
  archiverInfo    unZip32;
  archiverInfo    unLzh32;
  archiverInfo    unPak32;
  archiverInfo    unZoo32;
  archiverInfo    unArj32;
  archiverInfo    unSqz32;
  archiverInfo    unUc232;
  archiverInfo    unRar32;
  archiverInfo    GUS32;
  archiverInfo    unJar32;
  archiverInfo    resUnpack32[5];
  archiverInfo    preUnarc32;
  archiverInfo    postUnarc32;
  archiverInfo    arc32;
  archiverInfo    zip32;
  archiverInfo    lzh32;
  archiverInfo    pak32;
  archiverInfo    zoo32;
  archiverInfo    arj32;
  archiverInfo    sqz32;
  archiverInfo    uc232;
  archiverInfo    rar32;
  archiverInfo    customArc32;
  archiverInfo    jar32;
  archiverInfo    resPack32[5];
  archiverInfo    preArc32;
  archiverInfo    postArc32;
  char            descrAKA[MAX_NETAKAS][51];
  char            qwkName[MAX_NETAKAS][13];
  areaOptionsType optionsAKA[MAX_NETAKAS];
  uchar           msgKindsRA[MAX_NETAKAS];
  u16             daysAKA[MAX_NETAKAS];
  u16             msgsAKA[MAX_NETAKAS];
  uchar           groupsQBBS[MAX_NETAKAS];
  uchar           attrRA[MAX_NETAKAS];
  uchar           attr2RA[MAX_NETAKAS];
  u16             attrSBBS[MAX_NETAKAS];
  uchar           aliasesQBBS[MAX_NETAKAS];
  u16             groupRA[MAX_NETAKAS];
  u16             altGroupRA[MAX_NETAKAS][3];
  u16             minAgeSBBS[MAX_NETAKAS];
  u16             daysRcvdAKA[MAX_NETAKAS];
  uchar           replyStatSBBS[MAX_NETAKAS];
  u16             readSecRA[MAX_NETAKAS];
  uchar           readFlagsRA[MAX_NETAKAS][8];
  u16             writeSecRA[MAX_NETAKAS];
  uchar           writeFlagsRA[MAX_NETAKAS][8];
  u16             sysopSecRA[MAX_NETAKAS];
  uchar           sysopFlagsRA[MAX_NETAKAS][8];
  u16             templateSecQBBS[MAX_NETAKAS];
  uchar           templateFlagsQBBS[MAX_NETAKAS][8];
  u8              reserved7[512];
  u16             netmailBoard[MAX_NETAKAS_F];
  akaListType     akaList;
} configType ;


// ********** FMAIL.AR **********

#define dARFNAME   "fmail.ar"
#define dARDFNAME  "fmail.ard"

#if defined(__FMAILX__) || defined(__32BIT__)
#define MAX_AREAS   8000
#else
#define MAX_AREAS    512
#endif

#define MAX_FORWARDOLD  64
#define MAX_FORWARDDEF 256    // NOTE: this is a dummy. 'config.maxForward'
                              // contains the real size. 256 is the maximum.
#define MAX_FORWARD    config.maxForward
#define RAWECHO_SIZE   sizeof(rawEchoType)

#define MB_PATH_LEN_OLD   19
#define MB_PATH_LEN       61
#define ECHONAME_LEN_090  25
#define ECHONAME_LEN      51
#define COMMENT_LEN       51
#define ORGLINE_LEN       59

typedef char areaNameType[ECHONAME_LEN];

// NOTE: See above for the file header structure !!!

typedef struct
{
  u16 tossedTo     : 1;   // BIT 0
  u16              : 15;  // BIT 1-15
} areaStatType;

typedef struct
{
  u16             signature;   // contains "AE" for echo areas in FMAIL.AR and
                               // "AD" for default settings in FMAIL.ARD
  u16             writeLevel;
  areaNameType    areaName;
  char            comment[COMMENT_LEN];
  areaOptionsType options;
  u16             boardNumRA;
  u8              msgBaseType;
  char            msgBasePath[MB_PATH_LEN];
  u16             board;
  char            originLine[ORGLINE_LEN];
  u16             address;
  u32             group;
  u16             _alsoSeenBy;  // obsolete: see the 32-bit alsoSeenBy below
  u16             msgs;
  u16             days;
  u16             daysRcvd;
  u16             readSecRA;
  u8              flagsRdRA[4];
  u8              flagsRdNotRA[4];
  u16             writeSecRA;
  u8              flagsWrRA[4];
  u8              flagsWrNotRA[4];
  u16             sysopSecRA;
  u8              flagsSysRA[4];
  u8              flagsSysNotRA[4];
  u16             templateSecQBBS;
  u8              flagsTemplateQBBS[4];
  u8              _internalUse;
  u16             netReplyBoardRA;
  u8              boardTypeRA;
  u8              attrRA;
  u8              attr2RA;
  u16             groupRA;
  u16             altGroupRA[3];
  u8              msgKindsRA;
  char            qwkName[13];
  u16             minAgeSBBS;
  u16             attrSBBS;
  u8              replyStatSBBS;
  u8              groupsQBBS;
  u8              aliasesQBBS;
  u32             lastMsgTossDat;
  u32             lastMsgScanDat;
  u32             alsoSeenBy;
  areaStatType    stat;
  u8              reserved[180];

  nodeNumXType    forwards[MAX_FORWARDDEF];
} rawEchoType;



// ********** FMAIL.NOD **********

#define dNODFNAME  "fmail.nod"

#define MAX_NODES        1024
#define PKTOUT_PATH_LEN  53
#define PKT_TYPE_2PLUS   1
#define CAPABILITY       PKT_TYPE_2PLUS

typedef struct
{
  u16 fixDate     : 1;  // Bit 0
  u16 tinySeenBy  : 1;  // Bit 1
  u16             : 1;  // Bit 2
  u16 ignorePwd   : 1;  // Bit 3
  u16 active      : 1;  // Bit 4
  u16 nosysopmail : 1;  // Bit 5
  u16 routeToPoint: 1;  // Bit 6
  u16 packNetmail : 1;  // Bit 7
  u16             : 1;  // Bit 8
  u16             : 3;  // Bit 9 - 11
  u16 forwardReq  : 1;  // Bit 12
  u16 remMaint    : 1;  // Bit 13
  u16 allowRescan : 1;  // Bit 14
  u16 notify      : 1;  // Bit 15
} nodeOptionsType;

typedef struct  // OLD !!!
{
  nodeNumType     node;
  uchar           reserved1[2];
  u16             capability;
  u16             archiver;
  nodeOptionsType options;
  u32             groups;
  u16             outStatus;
  uchar           reserved2[32];
  uchar           password[18];
  uchar           packetPwd[10];
  uchar           reserved[2];
  nodeNumType     viaNode;
  uchar           sysopName[36];
} nodeInfoTypeOld;

// NOTE: see above for the file header structure !!!

typedef struct
{
  u16             signature;  // contains "ND"
  u16             writeLevel;
  nodeNumType     node;
  nodeNumType     viaNode;
  u16             capability;
  nodeOptionsType options;
  u8              archiver;
  u8              reserved1;
  u32             groups;
  u16             outStatus;
  char            sysopName[36];
  char            password[18];
  char            packetPwd[9];
  uchar           useAka;
  u32             lastMsgRcvdDat;
  u32             lastMsgSentDat;
  u32             lastNewBundleDat;
  char            pktOutPath[PKTOUT_PATH_LEN];
  u16             passiveDays;
  u16             passiveSize;
  u32             lastSentBCL;
  u16             autoBCL;
  u32             referenceLNBDat;
  uchar           reserved2[17];
  char            email[PKTOUT_PATH_LEN];
  uchar           reserved3[11];
} nodeInfoType;


// ********** FMAIL.PCK **********

#define dPCKFNAME  "fmail.pck"

#define PACK_STR_SIZE 64
#define MAX_PACK      128

typedef char packEntryType[PACK_STR_SIZE];
typedef packEntryType packType[MAX_PACK];


// ********** FMAIL.BDE **********

#define dBDEFNAME  "fmail.bde"

#define MAX_BAD_ECHOS 50

typedef struct
{
  areaNameType badEchoName;
  nodeNumType  srcNode;
  s16          destAka;
} badEchoType;

#include <poppack.h>

//---------------------------------------------------------------------------
#endif  // fmstructH
