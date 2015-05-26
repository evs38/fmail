//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2013 Wilfred van Velzen
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <dir.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fmail.h"

#include "areainfo.h"
#include "config.h"
#include "crc.h"
#include "filesys.h"
#include "jam.h"
#include "jamfun.h"
#include "log.h"
#include "msgpkt.h"
#include "mtask.h"
#include "output.h"
#include "utils.h"


char jam_subfields[_JAM_MAXSUBLENTOT];

#if 0
static int _mdays [13] =
				{
/* Jan */   0,
/* Feb */   31,
/* Mar */   31+28,
/* Apr */   31+28+31,
/* May */   31+28+31+30,
/* Jun */   31+28+31+30+31,
/* Jul */   31+28+31+30+31+30,
/* Aug */   31+28+31+30+31+30+31,
/* Sep */   31+28+31+30+31+30+31+31,
/* Oct */   31+28+31+30+31+30+31+31+30,
/* Nov */   31+28+31+30+31+30+31+31+30+31,
/* Dec */   31+28+31+30+31+30+31+31+30+31+30,
/* Jan */   31+28+31+30+31+30+31+31+30+31+30+31
				};

/*
**  Calculates the number of seconds that has passed from January 1, 1970
**  until the specified date and time.
**
**      Parameters:   JAMTMptr pTm        - Ptr to structure containing time
**
**         Returns:   Number of seconds since 1970 to specified date/time
*/
u32 jam_maketime(struct tm *ptm)
{
	 u32  Days;
	 int  Years;

	 /*Get number of years since 1970*/
	 Years = ptm->tm_year - 70;

	 /*Calculate number of days during these years,*/
	 /*including extra days for leap years         */
	 Days = Years * 365 + ((Years + 1) / 4);

	 /*Add the number of days during this year*/
	 Days += _mdays [ptm->tm_mon] + ptm->tm_mday - 1;
	 if((ptm->tm_year & 3) == 0 && ptm->tm_mon > 1)
		  Days++;

	 /*Convert to seconds, and add the number of seconds this day*/
	 return(((u32) Days * 86400L) + ((u32) ptm->tm_hour * 3600L) +
			  ((u32) ptm->tm_min * 60L) + (u32) ptm->tm_sec);
}

/*
**  Converts the specified number of seconds since January 1, 1970, to
**  the corresponding date and time.
**
**      Parameters:   UINT32ptr pt        - Number of seconds since Jan 1, 1970
**
**         Returns:   Ptr to struct JAMtm area containing date and time
*/
struct tm * jam_gettime(u32 t)
{
	 static struct tm      m;
	 int                   LeapDay;

	 m.tm_sec  = (int) (t % 60); t /= 60;
	 m.tm_min  = (int) (t % 60); t /= 60;
	 m.tm_hour = (int) (t % 24); t /= 24;
	 m.tm_wday = (int) ((t + 4) % 7);

	 m.tm_year = (int) (t / 365 + 1);
	 do
		  {
		  m.tm_year--;
		  m.tm_yday = (int) (t - m.tm_year * 365 - ((m.tm_year + 1) / 4));
		  }
	 while(m.tm_yday < 0);
	 m.tm_year += 70;

	 LeapDay = ((m.tm_year & 3) == 0 && m.tm_yday >= _mdays [2]);

	 for(m.tm_mon = m.tm_mday = 0; m.tm_mday == 0; m.tm_mon++)
		  if(m.tm_yday < _mdays [m.tm_mon + 1] + LeapDay)
				m.tm_mday = m.tm_yday + 1 - (_mdays [m.tm_mon] + (m.tm_mon != 1 ? LeapDay : 0));

	 m.tm_mon--;

	 m.tm_isdst = -1;

	 return(&m);
}
#endif

static fhandle jam_idxhandle = -1;
static fhandle jam_hdrhandle = -1;
static fhandle jam_txthandle = -1;
static fhandle jam_lrdhandle = -1;
static udef    jam_baseopen = 0;
static char    jam_basename[MB_PATH_LEN];
#define        JAMCODE     1


static u32 dummy;

static JAMHDRINFO jam_hdrinfo;

u32 jam_open(char *msgBaseName, JAMHDRINFO **jam_hdrinfo)
{  static JAMHDRINFO hdrinfo;
   char   tempstr[80];
   char   *helpptr;

   strcpy(jam_basename, msgBaseName);
   helpptr = stpcpy(tempstr, msgBaseName);
   strcpy(helpptr, EXT_HDRFILE);
   if ( (jam_hdrhandle = fsopenP(tempstr, O_RDWR|O_BINARY|O_DENYNONE|O_CREAT, S_IREAD|S_IWRITE)) == -1 )
   {
//    logEntry("jam_open 1", LOG_ALWAYS, 0);
      return 0;
  }
   strcpy(helpptr, EXT_LRDFILE);
   if ( (jam_lrdhandle = fsopenP(tempstr, O_RDWR|O_BINARY|O_DENYNONE|O_CREAT, S_IREAD|S_IWRITE)) == -1 )
   {  fsclose(jam_hdrhandle);
//    logEntry("jam_open 2", LOG_ALWAYS, 0);
      return 0;
   }
   strcpy(helpptr, EXT_TXTFILE);
   if ( (jam_txthandle = fsopenP(tempstr, O_RDWR|O_BINARY|O_DENYNONE|O_CREAT, S_IREAD|S_IWRITE)) == -1 )
   {  fsclose(jam_lrdhandle);
      fsclose(jam_hdrhandle);
//    logEntry("jam_open 3", LOG_ALWAYS, 0);
      return 0;
   }
   strcpy(helpptr, EXT_IDXFILE);
   if ( (jam_idxhandle = fsopenP(tempstr, O_RDWR|O_BINARY|O_DENYNONE|O_CREAT, S_IREAD|S_IWRITE)) == -1 )
   {  fsclose(jam_txthandle);
      fsclose(jam_lrdhandle);
      fsclose(jam_hdrhandle);
//    logEntry("jam_open 4", LOG_ALWAYS, 0);
      return 0;
   }
   *jam_hdrinfo = &hdrinfo;
   if ( read(jam_hdrhandle, &hdrinfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO) )
   {
//    Create new JAM base
      memset(&hdrinfo, 0, sizeof(JAMHDRINFO));
      memcpy(&hdrinfo.Signature, "JAM", 3);
      hdrinfo.DateCreated = 0;
      hdrinfo.PasswordCRC = -1L;
      hdrinfo.BaseMsgNum = 1L;
      return ((jam_baseopen = write(jam_hdrhandle, &hdrinfo, sizeof(JAMHDRINFO))) == sizeof(JAMHDRINFO));
   }
   return jam_baseopen = 1;
}



void jam_close(u32 jam_code)
{  dummy = jam_code;

   if ( jam_baseopen )
   {  fsclose(jam_idxhandle);
      fsclose(jam_txthandle);
      fsclose(jam_lrdhandle);
      fsclose(jam_hdrhandle);
      jam_idxhandle = -1;
      jam_txthandle = -1;
      jam_lrdhandle = -1;
      jam_hdrhandle = -1;
      jam_baseopen = 0;
   }
}


void jam_closeall(void)
{  u32 jam_code = 0;

   jam_close(jam_code);
}

//---------------------------------------------------------------------------
u16 jam_initmsghdrrec(JAMHDR *jam_msghdrrec, internalMsgType *message, u16 local)
{
  struct tm tm;
	time_t timer;

	memset(jam_msghdrrec, 0, sizeof(JAMHDR));
	memcpy(&jam_msghdrrec->Signature, "JAM", 3);
	jam_msghdrrec->Revision = 1;
	jam_msghdrrec->MsgIdCRC = -1L;
	jam_msghdrrec->ReplyCRC = -1L;
	tm.tm_year = message->year - 1900;
  tm.tm_mon  = message->month - 1;
	tm.tm_mday = message->day;
	tm.tm_hour = message->hours;
	tm.tm_min  = message->minutes;
	tm.tm_sec  = message->seconds;
	jam_msghdrrec->DateWritten = mktime(&tm);
	timer = time(NULL);
  tm = *gmtime(&timer);
	jam_msghdrrec->DateProcessed = mktime(&tm);
  jam_msghdrrec->Attribute = (local ? (MSG_LOCAL|MSG_TYPEECHO) : MSG_TYPEECHO) |
                             ((message->attribute & PRIVATE)? MSG_PRIVATE : 0);
  jam_msghdrrec->MsgNum = (filelength(jam_idxhandle) / sizeof(JAMIDXREC))+1;
	jam_msghdrrec->PasswordCRC = -1L;

	return 1;
}
//---------------------------------------------------------------------------
u16 jam_newidx(u32 jam_code, JAMIDXREC *jam_idxrec, u32 *jam_msgnum)
{
  u32 temp;
	dummy = jam_code;

  if ( ((temp = lseek(jam_idxhandle, 0, SEEK_END)) % sizeof(JAMIDXREC)) != 0 )
		return 0;
	if ( write(jam_idxhandle, jam_idxrec, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC) )
		return 0;
	*jam_msgnum = (temp / sizeof(JAMIDXREC)) + 1;
	return 1;
}



u16 jam_getidx(u32 jam_code, JAMIDXREC *jam_idxrec, u32 jam_msgnum)
{  dummy = jam_code;

   if ( !jam_msgnum )
      jam_msgnum++;
   if ( fmseek(jam_idxhandle, (jam_msgnum-1)*sizeof(JAMIDXREC), SEEK_SET, 1) != (jam_msgnum-1)*sizeof(JAMIDXREC) )
      return 0;
   if ( read(jam_idxhandle, jam_idxrec, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC) )
      return 0;
   return 1;
}



u16 jam_updidx(u32 jam_code, JAMIDXREC *jam_idxrec)
{  dummy = jam_code;

        if ( lseek(jam_idxhandle, -(s32)sizeof(JAMIDXREC), SEEK_CUR) < 0 )
		return 0;
	if ( write(jam_idxhandle, jam_idxrec, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC) )
		return 0;
	return 1;
}



u16 jam_getnextidx(u32 jam_code, JAMIDXREC *jam_idxrec)
{  dummy = jam_code;

	if ( read(jam_idxhandle, jam_idxrec, sizeof(JAMIDXREC)) != sizeof(JAMIDXREC) )
		return 0;
	return 1;
}



u16 jam_getsubfields(u32 jam_code, char *jam_subfields, u32 jam_subfieldlen,
							internalMsgType *message)
{  u16  loID;
//	u16  hiID;
	udef index;
	u32  datlen;
	char tempstr1[_JAM_MAXSUBLEN], tempstr2[_JAM_MAXSUBLEN+32];

	dummy = jam_code;

	index = 0;
	while ( index + 8 < jam_subfieldlen )
	{  loID = *(u16*)(jam_subfields+index);
		index += 2;
//		hiID = *(u16*)(jam_subfields+index);
		index += 2;
		datlen = *(u32*)(jam_subfields+index);
		index += 4;

                if ( datlen < _JAM_MAXSUBLEN && index + datlen <= jam_subfieldlen )
		switch ( loID )
		{
			case JAMSFLD_OADDRESS :
						strncpy(tempstr1, jam_subfields+index, (udef)datlen);
						tempstr1[(udef)datlen] = 0;
						sscanf(tempstr1, "%u:%u/%u.%u", &message->srcNode.zone, &message->srcNode.net, &message->srcNode.node, &message->srcNode.point);
						break;
			case JAMSFLD_DADDRESS :
						strncpy(tempstr1, jam_subfields+index, (udef)datlen);
						tempstr1[(udef)datlen] = 0;
						sscanf(tempstr1, "%u:%u/%u.%u", &message->destNode.zone, &message->destNode.net, &message->destNode.node, &message->destNode.point);
						break;
			case JAMSFLD_SENDERNAME :
						strncpy(message->fromUserName, jam_subfields+index, min((udef)datlen, sizeof(message->fromUserName)-1));
						message->fromUserName[(udef)datlen] = 0;
						break;
			case JAMSFLD_RECVRNAME :
						strncpy(message->toUserName, jam_subfields+index, min((udef)datlen, sizeof(message->toUserName)-1));
						message->toUserName[(udef)datlen] = 0;
						break;
			case JAMSFLD_MSGID :
						strncpy(tempstr1, jam_subfields+index, (udef)datlen);
						tempstr1[(udef)datlen] = 0;
						sprintf(tempstr2, "\1MSGID: %s\r", tempstr1);
						insertLine(message->text, tempstr2);
						break;
			case JAMSFLD_REPLYID :
						strncpy(tempstr1, jam_subfields+index, (udef)datlen);
						tempstr1[(udef)datlen] = 0;
						sprintf(tempstr2, "\1REPLY: %s\r", tempstr1);
						insertLine(message->text, tempstr2);
						break;
			case JAMSFLD_SUBJECT :
                                                strncpy(message->subject, jam_subfields+index, min((udef)datlen, sizeof(message->subject)-1));
						message->subject[(udef)datlen] = 0;
						break;
			case JAMSFLD_PID :
						strncpy(tempstr1, jam_subfields+index, (udef)datlen);
						tempstr1[(udef)datlen] = 0;
						sprintf(tempstr2, "\1PID: %s\r", tempstr1);
						insertLine(message->text, tempstr2);
						break;
//#define JAMSFLD_TRACE       8
//#define JAMSFLD_ENCLFILE    9
//#define JAMSFLD_ENCLFWALIAS 10
//#define JAMSFLD_ENCLFREQ    11
//#define JAMSFLD_ENCLFILEWC  12
//#define JAMSFLD_ENCLINDFILE 13
//#define JAMSFLD_EMBINDAT    1000
			case JAMSFLD_FTSKLUDGE :
						strncpy(tempstr1, jam_subfields+index, (udef)datlen);
						tempstr1[(udef)datlen] = 0;
						sprintf(tempstr2, "\1%s\r", tempstr1);
						insertLine(message->text, tempstr2);
						break;
			case JAMSFLD_SEENBY2D :
						strncpy(tempstr1, jam_subfields+index, (udef)datlen);
						tempstr1[(udef)datlen] = 0;
						sprintf(tempstr2, "SEEN-BY: %s\r", tempstr1);
						strcat(message->text, tempstr2);
						break;
			case JAMSFLD_PATH2D:
						strncpy(tempstr1, jam_subfields+index, (udef)datlen);
						tempstr1[(udef)datlen] = 0;
						sprintf(tempstr2, "\1PATH: %s\r", tempstr1);
						strcat(message->text, tempstr2);
						break;
			case JAMSFLD_FLAGS :
						strncpy(tempstr1, jam_subfields+index, (udef)datlen);
						tempstr1[(udef)datlen] = 0;
						sprintf(tempstr2, "\1FLAGS %s\r", tempstr1);
						insertLine(message->text, tempstr2);
						break;
//#define JAMSFLD_TZUTCINFO   2004
//#define JAMSFLD_UNKNOWN     0xffff
		}
		index += (udef)datlen;
	}
	return 1;
}



u16 jam_fields[21] =
	{	JAMSFLD_OADDRESS,
		JAMSFLD_DADDRESS,
		JAMSFLD_SENDERNAME,
		JAMSFLD_RECVRNAME,
		JAMSFLD_MSGID,
		JAMSFLD_REPLYID,
		JAMSFLD_SUBJECT,
		JAMSFLD_PID,
		JAMSFLD_TRACE,
		JAMSFLD_ENCLFILE,
		JAMSFLD_ENCLFWALIAS,
		JAMSFLD_ENCLFREQ,
		JAMSFLD_ENCLFILEWC,
		JAMSFLD_ENCLINDFILE,
		JAMSFLD_EMBINDAT,
		JAMSFLD_SEENBY2D,
		JAMSFLD_PATH2D,
		JAMSFLD_FLAGS,
		JAMSFLD_TZUTCINFO,
		JAMSFLD_FTSKLUDGE,
		JAMSFLD_UNKNOWN
	};


u16 jam_makesubfields(u32 jam_code, char *jam_subfields, u32 *jam_subfieldLen,
		      JAMHDR *jam_msghdrrec, internalMsgType *message)
{  u16   loID;
//	u16   hiID;
	udef  count;
	udef	again;
	char  *helpPtr1, *helpPtr2;
	udef  stlen;
	char  tempStr[_JAM_MAXSUBLEN];

	dummy = jam_code;

	helpPtr2 = jam_subfields;
	*jam_subfieldLen = 0;
	for ( count = 0; count < sizeof(jam_fields)/sizeof(u16); count++)
	{  loID = jam_fields[count];
//		hiID = 0;
		do
		{ 	again = 0;
			helpPtr1 = NULL;
			switch ( loID )
			{
/* NIET BIJ ECHOMAIL !!!!!!!!!
				case JAMSFLD_OADDRESS :
							helpPtr1 = nodeStr(&message->srcNode);
							break;
				case JAMSFLD_DADDRESS :
							helpPtr1 = nodeStr(&message->destNode);
							break;
*/
				case JAMSFLD_SENDERNAME :
							helpPtr1 = message->fromUserName;
							break;
				case JAMSFLD_RECVRNAME :
							helpPtr1 = message->toUserName;
							break;
				case JAMSFLD_MSGID :
                                                        if ( getKludge(message->text, "\1MSGID: ", tempStr, _JAM_MAXSUBLEN) != 0 )
							{  helpPtr1 = tempStr;
							   jam_msghdrrec->MsgIdCRC = crc32jam(helpPtr1);
							}
							break;
				case JAMSFLD_REPLYID :
                                                        if ( getKludge(message->text, "\1REPLY: ", tempStr, _JAM_MAXSUBLEN) != 0 )
							{  helpPtr1 = tempStr;
							   jam_msghdrrec->ReplyCRC = crc32jam(helpPtr1);
							}
							break;
				case JAMSFLD_SUBJECT :
							helpPtr1 = message->subject;
							break;
				case JAMSFLD_PID :
                                                        if ( getKludge(message->text, "\1PID: ", tempStr, _JAM_MAXSUBLEN) != 0 )
								helpPtr1 = tempStr;
							break;
//#de	fine JAMSFLD_TRACE       8
//#define JAMSFLD_ENCLFILE    9
//#define JAMSFLD_ENCLFWALIAS 10
//#define JAMSFLD_ENCLFREQ    11
//#define JAMSFLD_ENCLFILEWC  12
//#define JAMSFLD_ENCLINDFILE 13
//#define JAMSFLD_EMBINDAT    1000
				case JAMSFLD_SEENBY2D :
                                                        if ( (again = getKludge(message->text, "SEEN-BY: ", tempStr, _JAM_MAXSUBLEN)) != 0 )
                                                                helpPtr1 = tempStr;
//                                                      else if ( (again = getKludge(message->text, "\1SEEN-BY: ", tempStr, _JAM_MAXSUBLEN)) != 0 )
//                                                              helpPtr1 = tempStr;
                                                        break;
				case JAMSFLD_PATH2D:
                                                        if ( (again = getKludge(message->text, "\1PATH: ", tempStr, _JAM_MAXSUBLEN)) != 0 )
								helpPtr1 = tempStr;
							break;
				case JAMSFLD_FLAGS :
                                                        if ( (again = getKludge(message->text, "\1FLAGS ", tempStr, _JAM_MAXSUBLEN)) != 0 )
								helpPtr1 = tempStr;
							break;
//#define JAMSFLD_TZUTCINFO   2004
				case JAMSFLD_FTSKLUDGE :
                                                        if ( (again = getKludge(message->text, "\1", tempStr, _JAM_MAXSUBLEN)) != 0 )
								helpPtr1 = tempStr;
							break;
//#define JAMSFLD_UNKNOWN     0xffff
			}
			if ( helpPtr1 != NULL && *jam_subfieldLen+(stlen = strlen(helpPtr1))+8 < _JAM_MAXSUBLENTOT )
			{  *(((u16*)helpPtr2)++) = loID;
			   *(((u16*)helpPtr2)++) = 0;
			   *(((u32*)helpPtr2)++) = stlen;
			   helpPtr2 = stpcpy(helpPtr2, helpPtr1);
			   *jam_subfieldLen += 8 + stlen;
			}
		}
		while ( again );
	}
	return 1;
}



u16 jam_gethdr(u32 jam_code, u32 jam_hdroffset, JAMHDR *jam_hdrrec, char *jam_subfields, internalMsgType *message)
{
  dummy = jam_code;

  if ((u32)fmseek(jam_hdrhandle, jam_hdroffset, SEEK_SET, 2) != jam_hdroffset)
    return 0;
  if (read(jam_hdrhandle, jam_hdrrec, sizeof(JAMHDR)) != sizeof(JAMHDR))
    return 0;
  if (jam_hdrrec->SubfieldLen >= _JAM_MAXSUBLENTOT) // SubfieldLen not defined yet?!?
  {
    logEntry("Subfields too big", LOG_ALWAYS, 0);
    return 0;
  }
  if ( read(jam_hdrhandle, jam_subfields, (udef)jam_hdrrec->SubfieldLen) != jam_hdrrec->SubfieldLen )
    return 0;
  if ( message != NULL )
  {
    struct tm *tm = gmtime((const time_t *)&jam_hdrrec->DateWritten);
    message->year    = tm->tm_year + 1900;
    message->month   = tm->tm_mon + 1;
    message->day     = tm->tm_mday;
    message->hours   = tm->tm_hour;
    message->minutes = tm->tm_min;
    message->seconds = tm->tm_sec;
    message->attribute = ((jam_hdrrec->Attribute & MSG_PRIVATE) ? PRIVATE : 0);
  }
  return 1;
}



u16 jam_puthdr(u32 jam_code, u32 jam_hdroffset, JAMHDR *jam_hdrrec)
{
  dummy = jam_code;

  if ( (u32)fmseek(jam_hdrhandle, jam_hdroffset, SEEK_SET, 3) != jam_hdroffset )
    return 0;
  if ( write(jam_hdrhandle, jam_hdrrec, sizeof(JAMHDR)) != sizeof(JAMHDR) )
    return 0;
  return 1;
}



u16 jam_newhdr(u32 jam_code, u32 *jam_hdrOffset, JAMHDR *jam_hdrrec, char *jam_subfields)
{  dummy = jam_code;

   if ( (signed long)(*jam_hdrOffset = lseek(jam_hdrhandle, 0, SEEK_END)) < 0 )
      return 0;
   if ( jam_hdrrec->SubfieldLen >= _JAM_MAXSUBLENTOT )
   {  logEntry("Subfields too big", LOG_ALWAYS, 0);
      jam_hdrrec->SubfieldLen = 0;
   }
   if ( write(jam_hdrhandle, jam_hdrrec, sizeof(JAMHDR)) != sizeof(JAMHDR) )
      return 0;
   if ( write(jam_hdrhandle, jam_subfields, (udef)jam_hdrrec->SubfieldLen) != (udef)jam_hdrrec->SubfieldLen)
      return 0;
   return 1;
}

u16 jam_gettxt(u32 jam_code, u32 jam_txtoffset, u32 jam_txtlen, char *txt)
{
  dummy = jam_code;

  if ( jam_txtlen >= TEXT_SIZE )
    return 0;
  if ( (u32)fmseek(jam_txthandle, jam_txtoffset, SEEK_SET, 4) != jam_txtoffset )
    return 0;
// !!!!!!!!!!
  if (read(jam_txthandle, txt, (udef)jam_txtlen) != jam_txtlen)
    return 0;
  return 1;
}

u16 jam_puttext(JAMHDR *jam_hdrrec, char *txt)
{
   if ( (signed long)(jam_hdrrec->TxtOffset = lseek(jam_txthandle, 0, SEEK_END)) < 0 )
      return 0;
   jam_hdrrec->TxtLen = strlen(txt);
   if ( write(jam_txthandle, txt, (size_t)jam_hdrrec->TxtLen) != (size_t)jam_hdrrec->TxtLen )
      return 0;
   return 1;
}

static s16 useLocks = -1;

u16 jam_getlock(u32 jam_code)
{
   s16 stat;

   dummy = jam_code;
   if (useLocks)
   {
      stat=lock(jam_hdrhandle, 0L, 1L);
      if (useLocks == -1)
		{
	 useLocks = 1;
	 if (stat == -1 && errno == EINVAL)
	 {
            if (!config.mbOptions.mbSharing)
	       useLocks = 0;
	    else
	    {
	       newLine();
	       logEntry("SHARE is required when Message Base Sharing is enabled", LOG_ALWAYS, 0);
	       return 0;
	    }
	 }
      }
   }
   return 1;
}



u16 jam_freelock(u32 jam_code)
{  dummy = jam_code;

//	Update MOD counter
        if ( lseek(jam_hdrhandle, 0, SEEK_SET) != 0 )
		return 0;
	if ( read(jam_hdrhandle, &jam_hdrinfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO) )
		return 0;
	++jam_hdrinfo.ModCounter;
        if ( lseek(jam_hdrhandle, 0, SEEK_SET) != 0 )
		return 0;
	if ( write(jam_hdrhandle, &jam_hdrinfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO) )
		return 0;
	return !useLocks || !unlock(jam_hdrhandle, 0, 1);
}


u32 jam_incmsgcount(u32 jam_code)
{  dummy = jam_code;

        if ( lseek(jam_hdrhandle, 0, SEEK_SET) != 0 )
		return 0;
	if ( read(jam_hdrhandle, &jam_hdrinfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO) )
		return 0;
	++jam_hdrinfo.ActiveMsgs;
        if ( lseek(jam_hdrhandle, 0, SEEK_SET) != 0 )
		return 0;
	if ( write(jam_hdrhandle, &jam_hdrinfo, sizeof(JAMHDRINFO)) != sizeof(JAMHDRINFO) )
		return 0;
	return 1;
}


// !!! jam_writemsg moet gevolgd worden door jam_close() of jam_closeall() !!!

u32 jam_writemsg(char *msgbasename, internalMsgType *message, u16 local)
{
	u32        jam_code;
	JAMHDRINFO *jam_hdrinforec;
	JAMHDR     jam_msghdrrec;
	JAMIDXREC  jam_idxrec;
//      char       jam_subfields[_JAM_MAXSUBLENTOT];
	u32	   msgNum;
	char	   tempStr[80];
	fhandle	   handle;
	int	   len;

        returnTimeSlice(0);

        if ( jam_baseopen && strcmp(msgbasename, jam_basename) )
        {
           jam_close(JAMCODE);
        }
        if ( !jam_baseopen && !(jam_code = jam_open(msgbasename, &jam_hdrinforec)) )
        {  //logEntry("jam_writemsg 1", LOG_ALWAYS, 0);
           return 0;
        }
        if ( !jam_getlock(jam_code) )
	{  jam_close(jam_code);
//         logEntry("jam_writemsg 2", LOG_ALWAYS, 0);
	   return 0;
	}
        jam_initmsghdrrec(&jam_msghdrrec, message, local);
	jam_makesubfields(jam_code, jam_subfields, &jam_msghdrrec.SubfieldLen, &jam_msghdrrec, message);
	jam_puttext(&jam_msghdrrec, message->text);
	jam_newhdr(jam_code, &jam_idxrec.HdrOffset, &jam_msghdrrec, jam_subfields);
	jam_idxrec.UserCRC = crc32jam(message->toUserName);
	jam_newidx(jam_code, &jam_idxrec, &msgNum);
	jam_incmsgcount(jam_code);

  /*--- create/update ECHOMAIL.JAM */
	if ( local )
	{  strcpy (tempStr, config.bbsPath);
	   strcat (tempStr, "ECHOMAIL.JAM");
           if ( (handle = fsopenP(tempStr, O_WRONLY|O_CREAT|O_APPEND|O_BINARY, S_IREAD|S_IWRITE)) != -1 )
           {  len = sprintf(tempStr, "%s %lu\r\n", msgbasename, msgNum);
	      write(handle, tempStr, len);
              fsclose(handle);
	   }
	}

	jam_freelock(jam_code);
//      jam_close(jam_code);

	return 1;
}

