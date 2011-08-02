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


#include <stdio.h>
#include <string.h>

#include "fmail.h"
#include "areainfo.h"
#include "jam.h"
#include "jamfun.h"
#include "output.h"
#include "utils.h"
#include "config.h"
#include "msgpkt.h"
#include "msgmsg.h"
#include "mtask.h"


extern cookedEchoType    *echoAreaList;

extern char jam_subfields[_JAM_MAXSUBLENTOT];

u32 jam_scan(u16 echoIndex, u32 jam_msgnum, u16 scanOne, internalMsgType *message)
{
   u32        jam_code;
   JAMHDRINFO *jam_hdrinforec;
   JAMHDR     jam_msghdrrec;
   JAMIDXREC  jam_idxrec;
// char       jam_subfields[_JAM_MAXSUBLENTOT];
// u32	      scanOne;
   tempStrType tempStr;

// scanOne = jam_msgnum;
   if ( !(jam_code = jam_open(echoAreaList[echoIndex].JAMdirPtr, &jam_hdrinforec)) )
      return 0;

   if ( !jam_msgnum )
      jam_msgnum = jam_hdrinforec->BaseMsgNum;
   else if ( jam_msgnum < jam_hdrinforec->BaseMsgNum )
   {  jam_close(jam_code);
      return 0;
   }

   if ( !jam_getidx(jam_code, &jam_idxrec, jam_msgnum+1-jam_hdrinforec->BaseMsgNum) )
   {  jam_close(jam_code);
      return 0;
   }
   do
   {  if ( jam_idxrec.HdrOffset == MAXU32 )
         goto next;
      sprintf(tempStr, "(%lu) ", jam_msgnum);
      gotoTab(0);
      printString(tempStr);

      memset(message, 0, INTMSG_SIZE);
      jam_gethdr(jam_code, jam_idxrec.HdrOffset, &jam_msghdrrec, jam_subfields, message);
      if ( ((jam_msghdrrec.Attribute & (MSG_LOCAL|MSG_TYPEECHO)) == (MSG_LOCAL|MSG_TYPEECHO)) &&
            ((jam_msghdrrec.Attribute & (MSG_DELETED|MSG_SENT)) == 0) )
      {
	 jam_gettxt(jam_code, jam_msghdrrec.TxtOffset, jam_msghdrrec.TxtLen, message->text);
	 jam_getsubfields(jam_code, jam_subfields, jam_msghdrrec.SubfieldLen, message);
         if ( (getFlags(message->text) & FL_LOK) )
            goto next;
         jam_close(jam_code);
	 return jam_msgnum;
      }
next: if ( scanOne )
      {	 jam_close(jam_code);
	 gotoTab(0);
	 return 0;
      }
      ++jam_msgnum;
   }
   while ( jam_getnextidx(jam_code, &jam_idxrec) );
   jam_close(jam_code);
   gotoTab(0);
   return 0;
}


u32 jam_update(u16 echoIndex, u32 jam_msgnum, internalMsgType *message)
{
   u32        jam_code;
   JAMHDRINFO *jam_hdrinforec;
   JAMHDR     jam_msghdrrec;
   JAMIDXREC  jam_idxrec;
// char       jam_subfields[_JAM_MAXSUBLENTOT];
   u32        txtlen;

   if ( !(jam_code = jam_open(echoAreaList[echoIndex].JAMdirPtr, &jam_hdrinforec)) )
      return 0;

   if ( jam_msgnum < jam_hdrinforec->BaseMsgNum )
   {  jam_close(jam_code);
      return 0;
   }

   if ( !jam_getlock(jam_code) )
   {  jam_close(jam_code);
      return 0;
   }

   if ( jam_getidx(jam_code, &jam_idxrec, jam_msgnum+1-jam_hdrinforec->BaseMsgNum) &&
        jam_idxrec.HdrOffset != MAXU32 &&
        jam_gethdr(jam_code, jam_idxrec.HdrOffset, &jam_msghdrrec, jam_subfields, NULL) )
   {  if ( config.mbOptions.scanUpdate )
      {
         jam_msghdrrec.Attribute |= MSG_DELETED;
         txtlen = jam_msghdrrec.TxtLen;
         jam_msghdrrec.TxtLen = 0;
         jam_puthdr(jam_code, jam_idxrec.HdrOffset, &jam_msghdrrec);
         jam_msghdrrec.TxtLen = txtlen;
         jam_makesubfields(jam_code, jam_subfields, &jam_msghdrrec.SubfieldLen, &jam_msghdrrec, message);
         jam_puttext(&jam_msghdrrec, message->text);
         jam_msghdrrec.Attribute &= ~MSG_DELETED;
         jam_msghdrrec.Attribute |= MSG_SENT;
         jam_newhdr(jam_code, &jam_idxrec.HdrOffset, &jam_msghdrrec, jam_subfields);
         jam_updidx(jam_code, &jam_idxrec);
      }
      else
      {  jam_msghdrrec.Attribute |= MSG_SENT;
         jam_puthdr(jam_code, jam_idxrec.HdrOffset, &jam_msghdrrec);
      }
   }
   jam_freelock(jam_code);
   jam_close(jam_code);
   return 1;
}


u32 jam_rescan(u16 echoIndex, u16 maxRescan, nodeInfoType *nodeInfo, internalMsgType *message)
{
   u32        jam_code;
   JAMHDRINFO *jam_hdrinforec;
   JAMHDR     jam_msghdrrec;
   JAMIDXREC  jam_idxrec;
   char       tempstr[80];
// char       jam_subfields[_JAM_MAXSUBLENTOT];
   u32        msgCount = 0;
   u32        count;
   nodeFileRecType nfInfo;
   u32        found;

   if ( !(jam_code = jam_open(echoAreaList[echoIndex].JAMdirPtr, &jam_hdrinforec)) )
      return 0;

   printString ("Scanning for messages in area ");
   printString (echoAreaList[echoIndex].areaName);
   printString ("...\n");
   sprintf (tempstr, "AREA:%s\r\1RESCANNED %s\r", echoAreaList[echoIndex].areaName, nodeStr(&config.akaList[echoAreaList[echoIndex].address].nodeNum));
   makeNFInfo (&nfInfo, echoAreaList[echoIndex].address, &nodeInfo->node);
   count = jam_hdrinforec->ActiveMsgs;
   found = jam_getidx(jam_code, &jam_idxrec, 0);
   while ( found )
   {  if ( jam_idxrec.HdrOffset == MAXU32 )
         goto next;
      returnTimeSlice(0);
      if ( count-- <= maxRescan )
      {
         memset(message, 0, INTMSG_SIZE);
         jam_gethdr(jam_code, jam_idxrec.HdrOffset, &jam_msghdrrec, jam_subfields, message);
	 if ( ((jam_msghdrrec.Attribute & MSG_TYPEECHO) == MSG_TYPEECHO) &&
	      ((jam_msghdrrec.Attribute & MSG_DELETED) == 0) )
	 {
	    jam_gettxt(jam_code, jam_msghdrrec.TxtOffset, jam_msghdrrec.TxtLen, message->text);
	    jam_getsubfields(jam_code, jam_subfields, jam_msghdrrec.SubfieldLen, message);
	    insertLine(message->text, tempstr);
	    message->srcNode  = config.akaList[echoAreaList[echoIndex].address].nodeNum;
	    message->destNode = nodeInfo->node;
	    writeNetPktValid (message, &nfInfo);
	    msgCount++;
	 }
      }
next: found = jam_getnextidx(jam_code, &jam_idxrec);
   }
   closeNetPktWr(&nfInfo);
   jam_close(jam_code);
   return msgCount;
}


