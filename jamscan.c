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

#include <stdio.h>
#include <string.h>

#include "fmail.h"

#include "jam.h"
#include "jamfun.h"

#include "areainfo.h"
#include "config.h"
#include "log.h"
#include "msgmsg.h"
#include "msgpkt.h"
#include "mtask.h"
#include "utils.h"


extern cookedEchoType *echoAreaList;
extern char jam_subfields[_JAM_MAXSUBLENTOT];
extern echoToNodePtrType echoToNode[MAX_AREAS];

//---------------------------------------------------------------------------
u32 jam_scan(u16 echoIndex, u32 jam_msgnum, u16 scanOne, internalMsgType *message)
{
  u32         jam_code;
  JAMHDRINFO *jam_hdrinforec;
  JAMHDR      jam_msghdrrec;
  JAMIDXREC   jam_idxrec;

  if (!(jam_code = jam_open(echoAreaList[echoIndex].JAMdirPtr, &jam_hdrinforec)))
    return 0;

  if (!jam_msgnum)
    jam_msgnum = jam_hdrinforec->BaseMsgNum;
  else
    if (jam_msgnum < jam_hdrinforec->BaseMsgNum)
    {
      jam_close(jam_code);

      return 0;
    }

  if (!jam_getidx(jam_code, &jam_idxrec, jam_msgnum + 1 - jam_hdrinforec->BaseMsgNum))
  {
    jam_close(jam_code);

    return 0;
  }
  do
  {
    if (jam_idxrec.HdrOffset != UINT32_MAX)
    {
      memset(message, 0, INTMSG_SIZE);
      jam_gethdr(jam_code, jam_idxrec.HdrOffset, &jam_msghdrrec, jam_subfields, message);
      if (  ((jam_msghdrrec.Attribute & (MSG_LOCAL | MSG_TYPEECHO)) == (MSG_LOCAL | MSG_TYPEECHO))
         && ((jam_msghdrrec.Attribute & (MSG_DELETED | MSG_SENT)) == 0)
         )
      {
        jam_gettxt(jam_code, jam_msghdrrec.TxtOffset, jam_msghdrrec.TxtLen, message->text);
        jam_getsubfields(jam_code, jam_subfields, jam_msghdrrec.SubfieldLen, message);
        if (!(getFlags(message->text) & FL_LOK))
        {
          jam_close(jam_code);
          removeLfSr(message->text);

          return jam_msgnum;
        }
      }
    }
    if (scanOne)
    {
      jam_close(jam_code);
      putchar('\r');

      return 0;
    }
    ++jam_msgnum;
  }
  while (jam_getnextidx(jam_code, &jam_idxrec));

  jam_close(jam_code);

  return 0;
}
//---------------------------------------------------------------------------
u32 jam_update(u16 echoIndex, u32 jam_msgnum, internalMsgType *message)
{
   u32         jam_code;
   JAMHDRINFO *jam_hdrinforec;
   JAMHDR      jam_msghdrrec;
   JAMIDXREC   jam_idxrec;
// char        jam_subfields[_JAM_MAXSUBLENTOT];
   u32         txtlen;

   if (!(jam_code = jam_open(echoAreaList[echoIndex].JAMdirPtr, &jam_hdrinforec)))
      return 0;

   if (jam_msgnum < jam_hdrinforec->BaseMsgNum)
   {
      jam_close(jam_code);
      return 0;
   }

   if (!jam_getlock(jam_code))
   {
      jam_close(jam_code);
      return 0;
   }

   if (  jam_getidx(jam_code, &jam_idxrec, jam_msgnum+1-jam_hdrinforec->BaseMsgNum)
      && jam_idxrec.HdrOffset != UINT32_MAX
      && jam_gethdr(jam_code, jam_idxrec.HdrOffset, &jam_msghdrrec, jam_subfields, NULL)
      )
   {
      if (config.mbOptions.scanUpdate)
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
      {
         jam_msghdrrec.Attribute |= MSG_SENT;
         jam_puthdr(jam_code, jam_idxrec.HdrOffset, &jam_msghdrrec);
      }
   }
   jam_freelock(jam_code);
   jam_close(jam_code);

   return 1;
}
//---------------------------------------------------------------------------
u32 jam_rescan(u16 echoIndex, u32 maxRescan, nodeInfoType *nodeInfo, internalMsgType *message)
{
  u32             jam_code;
  JAMHDRINFO     *jam_hdrinforec;
  JAMHDR          jam_msghdrrec;
  JAMIDXREC       jam_idxrec;
  tempStrType     tempStr
                , tempStr2;
  u32             msgCount = 0;
  u32             count;
  nodeFileRecType nfInfo;
  u32             found;

#ifdef _DEBUG0
  logEntry("DEBUG Rescan jam", LOG_DEBUG, 0);
#endif

  if (!(jam_code = jam_open(echoAreaList[echoIndex].JAMdirPtr, &jam_hdrinforec)))
    return 0;

  logEntryf(LOG_ALWAYS, 0, "Scanning for messages in JAM area: %s", echoAreaList[echoIndex].areaName);

  sprintf(tempStr2, "AREA:%s\r\1RESCANNED", echoAreaList[echoIndex].areaName);
  setViaStr(tempStr, tempStr2, echoAreaList[echoIndex].address);
  makeNFInfo(&nfInfo, echoAreaList[echoIndex].address, &nodeInfo->node);
  count = jam_hdrinforec->ActiveMsgs;
  found = jam_getidx(jam_code, &jam_idxrec, 0);

  while (found)
  {
    if (jam_idxrec.HdrOffset != UINT32_MAX)
    {
      returnTimeSlice(0);
      if (count-- <= maxRescan)
      {
        memset(message, 0, INTMSG_SIZE);
        jam_gethdr(jam_code, jam_idxrec.HdrOffset, &jam_msghdrrec, jam_subfields, message);
        if (  (jam_msghdrrec.Attribute & MSG_TYPEECHO) == MSG_TYPEECHO
           && (jam_msghdrrec.Attribute & MSG_DELETED ) == 0
           )
        {
          jam_gettxt(jam_code, jam_msghdrrec.TxtOffset, jam_msghdrrec.TxtLen, message->text);
          removeLfSr(message->text);
          jam_getsubfields(jam_code, jam_subfields, jam_msghdrrec.SubfieldLen, message);
          insertLine(message->text, tempStr);
          addPathSeenBy(message, echoToNode[echoIndex], echoIndex, &nodeInfo->node);
          setSeenByPath(message, NULL, echoAreaList[echoIndex].options, nodeInfo->options);
          message->srcNode  = *getAkaNodeNum(echoAreaList[echoIndex].address, 1);
          message->destNode = nodeInfo->node;
          writeNetPktValid(message, &nfInfo);
          msgCount++;
        }
      }
    }
    found = jam_getnextidx(jam_code, &jam_idxrec);
  }
  closeNetPktWr(&nfInfo);
  jam_close(jam_code);

#ifdef _DEBUG0
  logEntry("DEBUG Rescan jam end", LOG_DEBUG, 0);
#endif

  return msgCount;
}
//---------------------------------------------------------------------------
