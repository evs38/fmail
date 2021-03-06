#ifndef fmailH
#define fmailH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2017 Wilfred van Velzen
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

#include "fmstruct.h"
#include "stricmp.h"

#include <stdio.h>       // FILENAME_MAX
#include <sys/types.h>   // stat.off_t

#define CONFIG_MAJOR   0
#define CONFIG_MINOR  94 // do not change

#ifdef GOLDBASE
   #define MBNAME   "QuickBBS Gold"
   #define MBEXTN   "dat"
   #define MBEXTB   "fmg"
   #define MBBOARDS 500
#else
   #define MBNAME   "Hudson"
   #define MBEXTN   "bbs"
   #define MBEXTB   "fml"
   #define MBBOARDS 200
#endif

#ifdef FSETUP
#ifdef __32BIT__
#define FSNAME "FConfig"
#else
#define FSNAME "FSetup"
#endif
#endif

#define PRODUCTCODE_HIGH  0x00
#define PRODUCTCODE_LOW   0x81  // ftscprod

#define DEF_TEXT_SIZE 0xC000
#ifdef __32BIT__
// 1 megabyte
#define TEXT_SIZE     (config.maxMsgSize * 1024L)
#else
#define TEXT_SIZE     0xC000
#endif
#define INTMSG_SIZE   (sizeof(internalMsgType) - DEF_TEXT_SIZE + TEXT_SIZE)

#define PKT_INFO_SIZE      192

#define MAX_MSGTINYPATH      2  // 1 should be enough
#define MAX_MSGPATH        200
#define MAX_MSGTINYSEENBY   80
#define MAX_MSGSEENBY     1000

#define TINY_SEEN_SIZE 1024
#define NORM_SEEN_SIZE 8192
#define TINY_PATH_SIZE 128
#define NORM_PATH_SIZE 1024

#define dEXTTMP        "$$$"
#define dEXTBAK        "bak"
#define dECHOMAIL_JAM  "echomail.jam"
#define dFMAIL_LOC     "fmail.loc"
#define dFMAIL32_DUP   "fmail32.dup"
#define dLASTREAD      "lastread"

//---------------------------------------------------------------------------
extern long         gmtOffset;
extern time_t       startTime;
extern struct tm    timeBlock;
extern char         funcStr[32];
extern char         configPath[FILENAME_MAX];
extern configType   config;

//---------------------------------------------------------------------------
typedef struct
{
  char        fromUserName[36];
  char        toUserName  [36];
  char        subject     [72];
  char        dateStr     [22];
  char        okDateStr   [20];
  u16         year
            , month
            , day
            , hours
            , minutes
            , seconds;
  u16         attribute;
  u16         cost;
  nodeNumType srcNode;
  nodeNumType destNode;
  char        tinySeen[TINY_SEEN_SIZE];
  char        normSeen[NORM_SEEN_SIZE];
  char        tinyPath[TINY_PATH_SIZE];
  char        normPath[NORM_PATH_SIZE];
  char        pktInfo [PKT_INFO_SIZE ];
  char        text    [DEF_TEXT_SIZE ];

} internalMsgType;

typedef struct
{
  u16 net;
  u16 node;

} psRecType;

//typedef psRecType psType[];

#define dTEMPSTRLEN  (FILENAME_MAX + 64)  // Groter dan FILENAME_MAX (wordt ook voor paden gebruikt)
typedef char tempStrType[dTEMPSTRLEN];

typedef struct
{
  nodeNumType  packetSrcNode;     /* Generated by openPktRd (MSGPKT.C) */
                                  /* Used for security check(FMAIL.C)  */
  nodeNumType  packetDestNode;    /* Used for 'Processing..'(FMAIL.C)  */
  u16          packetDestAka;     /* Generated by openPktRd (MSGPKT.C) */
                                  /* Used by      addVia    (FMAIL.C)  */
  u32          packetSize;        /* Used for 'Processing..'(FMAIL.C)  */
  u16          hour, min, sec,    /* Used for 'Processing..'(FMAIL.C)  */
               day, month, year;
  u16          versionHi,versionLo;
  u16          password;
  u16          remoteCapability;  /* Generated by openPktRd (MSGPKT.C) */
  u16          remoteProdCode;    /* Generated by openPktRd (MSGPKT.C) */
#ifdef GOLDBASE
  u32          origTotalMsgBBS;   /* Generated by initBBS   (MSGRA.C)  */
                                  /* Used by      sortBBS   (UTILRA.C) */
  u32          baseTotalTxtRecs;  /* Generated by initBBS   (MSGRA.C)  */
                                  /* Used by      writeBBS  (MSGRA.C) */
#else
  u16          origTotalMsgBBS;   /* Generated by initBBS   (MSGRA.C)  */
                                  /* Used by      sortBBS   (UTILRA.C) */
  u16          baseTotalTxtRecs;  /* Generated by initBBS   (MSGRA.C)  */
                                  /* Used by      writeBBS  (MSGRA.C) */
#endif
  u32          switches;          /* Generated by initFMail (CONFIG.C) */

  u16          createSemaphore;
  u16          movedBad,
  mbCount,
  mbCountV,
  jamCountV,
  echoCount,
  echoCountV,
  nmbCountV,
  netCount,
  netCountV,
  dupCount,
  dupCountV,
  badCount,
  badCountV,
  perCount,
  perCountV,
  perNetCount,
  perNetCountV,
  fromNoExpMsg,
  fromNoExpDup,
  fromNoExpSec;
} globVarsType;

extern globVarsType globVars;

// AttributeWord   bit       meaning
//                 ---       --------------------
//                   0  +    Private
//                   1  + s  Crash
//                   2       Recd
//                   3       Sent
//                   4  +    FileAttached
//                   5       InTransit
//                   6       Orphan
//                   7       KillSent
//                   8       Local
//                   9    s  HoldForPickup
//                  10  +    unused
//                  11    s  FileRequest
//                  12  + s  ReturnReceiptRequest
//                  13  + s  IsReturnReceipt
//                  14  + s  AuditRequest
//                  15    s  FileUpdateReq
//
//                        s - need not be recognized, but it's ok
//                        + - not zeroed before packeting
//
// Bits numbers ascend with arithmetic significance of bit position.

#define PRIVATE       0x0001  // PVT
#define CRASH         0x0002  // CRA
#define RECEIVED      0x0004  // RCV
#define SENT          0x0008  // SNT
#define FILE_ATT      0x0010  // FIL
#define IN_TRANSIT    0x0020  // TRS
#define ORPHAN        0x0040  // ORP
#define KILLSENT      0x0080  // K/S
#define LOCAL         0x0100  // LOC
#define HOLDPICKUP    0x0200  // HLD
#define UNUSED        0x0400  // ---
#define FILE_REQ      0x0800  // FRQ
#define RET_REC_REQ   0x1000  // RRQ
#define IS_RET_REC    0x2000  // RRC
#define AUDIT_REQ     0x4000  // ARQ
#define FILE_UPD_REQ  0x8000  // URQ


#define _K_ENTER_ 0x000d
#define _K_ESC_   0x001b
#define _K_BSPC_  0x0008

#define _K_F1_    0x3b00
#define _K_F2_    0x3c00
#define _K_F3_    0x3d00
#define _K_F4_    0x3e00
#define _K_F5_    0x3f00
#define _K_F6_    0x4000
#define _K_F7_    0x4100
#define _K_F8_    0x4200
#define _K_F9_    0x4300
#define _K_F10_   0x4400

#define _K_HOME_  0x4700
#define _K_END_   0x4f00
#define _K_PGUP_  0x4900
#define _K_PGDN_  0x5100
#define _K_LEFT_  0x4b00
#define _K_RIGHT_ 0x4d00
#define _K_UP_    0x4800
#define _K_DOWN_  0x5000
#define _K_INS_   0x5200
#define _K_DEL_   0x5300

#define _K_CPGUP_ 0x8400
#define _K_CPGDN_ 0x7600
#define _K_CUP    0x4800
#define _K_CDOWN_ 0x5000
#define _K_CLEFT_ 0x7300
#define _K_CRIGHT_ 0x7400
#define _K_CEND_  0x7500

#define _K_CTLF_  0x0006
#define _K_CTLY_  0x0019

#define _K_ALTA_  0x1E00
#define _K_ALTB_  0x3000
#define _K_ALTC_  0x2E00
#define _K_ALTD_  0x2000
#define _K_ALTE_  0x1200
#define _K_ALTF_  0x2100
#define _K_ALTG_  0x2200
#define _K_ALTH_  0x2300
#define _K_ALTI_  0x1700
#define _K_ALTJ_  0x2400
#define _K_ALTK_  0x2500
#define _K_ALTL_  0x2600
#define _K_ALTM_  0x3200
#define _K_ALTN_  0x3100
#define _K_ALTO_  0x1800
#define _K_ALTP_  0x1900
#define _K_ALTQ_  0x1000
#define _K_ALTR_  0x1300
#define _K_ALTS_  0x1F00
#define _K_ALTT_  0x1400
#define _K_ALTU_  0x1600
#define _K_ALTV_  0x2F00
#define _K_ALTW_  0x1100
#define _K_ALTX_  0x2D00
#define _K_ALTY_  0x1500
#define _K_ALTZ_  0x2C00

extern internalMsgType *message;

u16 getAreaCode(char *msgText);

#define DERR_HACKER     -1
#define DERR_VAL        2
#define DERR_WRPKTE     3
#define DERR_CLPKTE     4
#define DERR_PRSCN      5
#define DERR_PACK       6
#define DERR_WRHECHO    16
#define DERR_WRHBAD     17
#define DERR_WRHDUP     18
#define DERR_WRJECHO    32

#define nodegreater(n1, n2)      \
(  ((n1.zone  >  n2.zone) ||     \
   ((n1.zone  == n2.zone) &&     \
    (n1.net   >  n2.net)) ||     \
   ((n1.zone  == n2.zone) &&     \
    (n1.net   == n2.net) &&      \
    (n1.node  >  n2.node)) ||    \
   ((n1.zone  == n2.zone) &&     \
    (n1.net   == n2.net) &&      \
    (n1.node  == n2.node) &&     \
    (n1.point >  n2.point)))     \
)

// Use macro for Multi-character character constants, to prevent compiler warnings
#define MC(c1, c2) (((c1) << 8) | (c2))

#ifndef FSETUP
#define dARROW "->"

#define newLine()  putchar('\n')
#define putStr(S)  fputs(S, stdout)
#endif

u16 foundToUserName(const char *toUserName);

// Mailer types:
#define dMT_FrontDoor  0
#define dMT_InterMail  1
#define dMT_DBridge    2
#define dMT_Binkley    3
#define dMT_MainDoor   4
#define dMT_Xenia      5

//---------------------------------------------------------------------------
struct bt
{
  char       fname[FILENAME_MAX];
  time_t     mtime;
  long       size;
  struct bt *nextb;
};
//---------------------------------------------------------------------------
#endif  // fmailH
