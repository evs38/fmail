/***************************************************************************\
* msgbase.h - message base header file *
*****************************************************************************
* *
* mb_lib (msgbase library) v1.00 *
* (hudson message base access / manipulation routines) *
* *
* this c version (c) f.w. van wensveen 1993. all rights reserved. *
* original pascal source code (c) richard faasen and in fase productions, *
* the netherlands. *
* *
* first revision: 17/08/1992 *
* *
\***************************************************************************/


/* first, a c++ fixup ******************************************************/

#ifndef __mb_lib_h
#define __mb_lib_h

#ifdef __cplusplus
extern "c" {
#endif


/* includes and defines ****************************************************/

#include <dir.h>

#define hcr 0x0d /* hard carriage return */
#define scr 0x8d /* soft carriage return */
#define lf 0x0a /* line feed */
#define kludge 0x01 /* kludge denominator */
#define bell 0x07 /* bell character / ring a ding */

#define nokludges 0x00 /* txt_dump shouldn't dump kludges */
#define kludges 0x01 /* txt_dump should dump kludges */

/* critical errors */
#define not_err 0x00 /* no error */
#define mem_err 0x01 /* memory error - out of memory */
#define frd_err 0x02 /* file read error */
#define fwr_err 0x03 /* file write error */
#define fcr_err 0x04 /* file create error */
#define mno_err 0x05 /* message base not open error */
#define irn_err 0x06 /* invalid record no., out of bound */
#define mbc_err 0x07 /* msg base corrupt */

#define ma_deleted 0x01 /* message attributes */
#define ma_unsent 0x02
#define ma_netmail 0x04
#define ma_private 0x08
#define ma_received 0x10
#define ma_unmoved 0x20
#define ma_local 0x40

#define na_kill 0x01 /* hudson netmail attributes */
#define na_sent 0x02
#define na_file 0x04
#define na_crash 0x08
#define na_receipt 0x10
#define na_audit 0x20
#define na_return 0x40

#define nm_private 0x0001 /* opus (*.msg) netmail attributes */
#define nm_crash 0x0002
#define nm_received 0x0004
#define nm_sent 0x0008
#define nm_file 0x0010
#define nm_transit 0x0020
#define nm_orphan 0x0040
#define nm_kill 0x0080
#define nm_local 0x0100
#define nm_hold 0x0200
#define nm_unused 0x4000
#define nm_request 0x0800
#define nm_receipt 0x1000
#define nm_isreceipt 0x2000
#define nm_audit 0x4000
#define nm_updatereq 0x8000


/**************************** c a u t i o n ********************************/
/* the strings in the hudson message base files are all pascal convention */
/* strings - a length byte followed by the actual characters. */
/* i have attempted to maintain this format by defining the strings as a */
/* structure containing a length byte and a character array, but this */
/* lead to too much trouble from the programmers point of view. so the */
/* strings in the following structure are just defined as arrays of chars. */
/* they may only be read from and written to disk using the read and write */
/* functions in this library, which convert the string formats from pascal */
/* to c format and vice versa. they may not be read and written directly!! */
/* writing structures to the message base by any other means than the */
/* functions provided therefore will cause corruption of the message base. */
/***************************************************************************/

typedef struct { /* msginfo.bbs structure definition */
unsigned int low_msg; /* lowest msg # in message base */
unsigned int high_msg; /* highest msg # in message base */
unsigned int total_msgs; /* total # of messages in base */
unsigned int total_on_board [200]; /* number of messages / board */
} msginfo_record;

typedef struct { /* msgidx.bbs structure definiton */
int msg_num; /* message # */
unsigned char board; /* board # where msg is stored */
} msgidx_record;

typedef char msgtoidx_record [36]; /* msgtoidx.bbs structure def. */

typedef struct { /* msghdr.bbs structure definition */
int msgnum; /* message number */
unsigned int prev_reply; /* msg # of previous reply, 0 if no */
unsigned int next_reply; /* msg # of next reply, 0 if none */
unsigned int times_read; /* # of times msg was read, unused */
unsigned int start_block; /* record # of msg in msgtxt.bbs */
unsigned int num_blocks; /* # of records in msgtxt.bbs */
unsigned int dest_net; /* destination net */
unsigned int dest_node; /* destination node */
unsigned int orig_net; /* origin net */
unsigned int orig_node; /* origin node */
unsigned char dest_zone; /* destination zone */
unsigned char orig_zone; /* origin zone */
unsigned int cost; /* cost (netmail) */
unsigned char msg_attr; /* msg attributes. bits as follows: */
/* 0 : deleted */
/* 1 : unsent */
/* 2 : netmail */
/* 3 : private */
/* 4 : received */
/* 5 : unmoved outgoing echo */
/* 6 : local */
/* 7 : reserved */
unsigned char net_attr; /* netmail attributes. bits follow: */
/* 0 : kill/sent */
/* 1 : sent */
/* 2 : file attach */
/* 3 : crash */
/* 4 : receipt request */
/* 5 : audit request */
/* 6 : is a return receipt */
/* 7 : reserved */
unsigned char board; /* message board # */
char post_time [6]; /* time message was posted */
char post_date [9]; /* date message was posted */
char who_to [36]; /* recipient to whom msg is sent */
char who_from [36]; /* sender who posted message */
char subject [73]; /* subject line of message */
} msghdr_record;

typedef struct { /* msgtxt.bbs structure definition */
unsigned char str_len; /* this string is stored in memory */
char str_txt [255]; /* in pascal format to reduce */
} msgtxt_record; /* overhead, so take care! */


/* the strings in the *.msg file header (opus style) aren't pascal type */
/* strings but have the 'normal' asciiz format. */

typedef struct { /* opus-style (*.msg) msg format */
char who_from [36]; /* sender who posted message */
char who_to [36]; /* recipient to whom msg is sent */
char subject [72]; /* subject line of message */
char datetime [20]; /* date/time msg was last edited */
unsigned int times_read; /* # of times message was read */
unsigned int dest_node; /* destination node */
unsigned int orig_node; /* origin node */
unsigned int cost; /* cost to send netmail msg */
unsigned int orig_net; /* origin net */
unsigned int dest_net; /* destination net */
unsigned int dest_zone; /* destination zone (these fields) */
unsigned int orig_zone; /* origin zone (were padded ) */
unsigned int dest_point; /* destination point (with 8 0's ) */
unsigned int orig_point; /* origin point (in fsc-0001 ) */
unsigned int reply_to; /* msg # to which this one replies */
unsigned int attribute; /* msg attributes. bits as follows: */
/* 0 : private */
/* 1 : crash */
/* 2 : received */
/* 3 : sent */
/* 4 : file attached */
/* 5 : in transit */
/* 6 : orphan */
/* 7 : kill when sent */
/* 8 : locak */
/* 9 : hold for pickup */
/* 10 : unused */
/* 11 : file request */
/* 12 : return receipt request */
/* 13 : is a return receipt */
/* 14 : audit request */
/* 15 : file update request */
unsigned int next_reply; /* next msg in reply chain */
} net_record;

/* message text is stored as follows: a linked list of records contains */
/* pointers to a msgtxt record. this record contains 255 bytes of message */
/* text. this message text block list (mtbl) can be changed at will. */
/* because the programmer won't work with the 'raw' message text in ram */
/* directly, the string format of the message text is *not* converted. the */
/* text blocks contain the strings in pascal format. */

typedef struct __mtbl__ { /* element of msg text block list */
msgtxt_record * txt; /* pointer to text block */
struct __mtbl__ * next; /* pointer to next struct in list */
} mtbl;

typedef mtbl * m_text; /* text handle - ptr to mtbl start */


/* public data *************************************************************/

extern msginfo_record msginfo; /* global msginfo record */
extern int _cdecl errortype; /* indicate type of error occurred */
extern char _cdecl errorstring []; /* printable string with error msg. */


/* macro's and prototypes **************************************************/

/* manipulating message text in memory */
m_text txt_new (char *); /* create new text */
int txt_add (m_text, char *); /* add a line of text */
void txt_dispose (m_text); /* dispose of text */
int txt_dump (m_text, int (*) (char *), unsigned char,
unsigned char); /* dump message text */

/* message base file access */
int msg_open (char *); /* open message base */
void msg_close (void); /* close message base */

/* message base file locking */
int msg_lock (char *); /* lock message base */
void msg_unlock (void); /* unlock message base */

/* reading message base */
int msg_read_info (void); /* read msginfo.bbs */
int msg_read_hdr (long, msghdr_record *); /* read msghdr.bbs */
int msg_read_idx (long, msgidx_record *); /* read msgidx.bbs */
int msg_read_toidx (long, msgtoidx_record *); /* the same with */
m_text msg_read_text (msghdr_record *); /* read text 4 spec'd msg */

/* writing message base */
int msg_write_info (void); /* write msginfo.bbs */
int msg_write_hdr (long, msghdr_record *); /* write msghdr.bbs */
int msg_write_idx (long, msgidx_record *); /* write msgidx.bbs */
int msg_write_toidx (long, msgtoidx_record *); /* msgtoidx.bbs */
int msg_write_text (msghdr_record *, m_text); /* new/change txt */
int msg_write_new (msghdr_record *, m_text); /* write new msg */
int msg_kill (long); /* kill message */

/* message manipulation support routines */
long msg_msgnr2recnr (unsigned int); /* convert msg # to rec # */
unsigned int msg_recnr2msgnr (long); /* convert rec # to msg # */
void msg_hdr_clear (msghdr_record *); /* clear header record */
int msg_fixup4d (msghdr_record *, m_text); /* hdr -> @intl */

/* message base search routines */
long msg_firstinboard (unsigned char); /* get # of 1st msg in brd */
long msg_lastinboard (unsigned char); /* get # of last msg in b.*/
long msg_nextinboard (long); /* get # of next msg in b.*/
long msg_previnboard (long); /* get # of prev msg in b.*/
long msg_firstto (msgtoidx_record *); /* get # of 1st msg to.. */
long msg_lastto (msgtoidx_record *); /* get # of last msg to.. */
long msg_nextto (long); /* get # of next msg to.. */
long msg_prevto (long); /* get # of prev msg to.. */

/* netmail search and manipulation support routines */
int net_first (char *); /* get # of first net msg */
int net_last (char *); /* get # of last net msg */
int net_next (char *, int); /* get # of next net msg */
int net_prev (char *, int); /* get # of prev net msg */
void net_hdr_clear (net_record *); /* clear a netmail header */
int net_fixup4d (net_record *, m_text); /* intl/fmpt/topt */
int net_getlastread (char *); /* get *.msg lastread ptr */
int net_setlastread (char *, int); /* set *.msg lastread ptr */

/* reading and writing netmail */
int net_read_hdr (char *, int, net_record *); /* read net. hdr */
m_text net_read_text (char *, int); /* read netmail text */
int net_write (char *, int, net_record *, m_text); /* write net. */
int net_kill (char *, int); /* kill *.msg netmail msg */


/* wrap up the c++ fixup ***************************************************/

#ifdef __cplusplus
}
#endif
#endif /* __mb_lib_h */


/* eof */

