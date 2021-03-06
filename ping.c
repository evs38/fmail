//---------------------------------------------------------------------------
//
//  Copyright (C) 2016 - 2017  Wilfred van Velzen
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ping.h"

#include "config.h"
#include "log.h"
#include "msgmsg.h"
#include "stpcpy.h"
#include "utils.h"

#ifdef _DEBUG
#define ADDATTR
#endif

//---------------------------------------------------------------------------
int toPing(const char *toName)
{
  const char *p = toName;

  while (isspace(*p))
    p++;

  return stricmp(p, "PING") == 0;
}
//---------------------------------------------------------------------------
#ifdef ADDATTR
const char *attributeStr(u16 attr)
{
  static char str[72];
  static const char *aStrs[]=
  {
    "Pvt"
  , "Cra"
  , "Rcv"
  , "Snt"
  , "Fil"
  , "Trs"
  , "Orp"
  , "K/s"
  , "Loc"
  , "Hld"
  , "---"
  , "Frq"
  , "Rrq"
  , "Rrc"
  , "Arq"
  , "Urq"
  };
  char *p = str;
  int i;
  u16 b = 1;

  for (i = 0; i < 16; i++)
  {
    if (b & attr)
    {
      p = stpcpy(p, aStrs[i]);
      *p++ = ' ';
    }
    b <<= 1;
  }
  if (p > str)
    p--;

  *p = 0;

  return str;
}
#endif  // ADDATTR
//---------------------------------------------------------------------------
int Ping(internalMsgType *message, int localAkaNum)
{
  u32              msgLen;
  s32              msgNum;
  tempStrType      replyStr
                 , chrsStr;
  char            *helpPtr
                , *helpPtr2;
  internalMsgType *replyMsg;
  char            *funcText = localAkaNum >= 0 ? "PING" : "TRACE";

#ifdef _DEBUG
  logEntryf(LOG_DEBUG, 0, "DEBUG Processing PING message from %s to %s", nodeStr(&message->srcNode), nodeStr(&message->destNode));
#endif

  // Maak de replyMsg aan de hand van de lengte van het ontvangen bericht.
  msgLen = (((sizeof(internalMsgType) - DEF_TEXT_SIZE) + strlen(message->text)) & 0xFFFFF000) + 0x2000;
  if (NULL == (replyMsg = (internalMsgType *)calloc(msgLen, 1)))
    logEntry("Not enough memory to create PING message", LOG_ALWAYS, 2);

  // Get the string for the REPLY: kludge
  if ((helpPtr = findCLStr(message->text, "\1MSGID: ")) != NULL)
  {
    helpPtr += 8;
    if ((helpPtr2 = strchr(helpPtr, 0x0d)) != NULL)
    {
      int l = helpPtr2 - helpPtr;
      if (l > 72)            // Set arbitrary max len
        l = 72;

      memcpy(replyStr, helpPtr, l);
      replyStr[l] = 0;
    }
    else
      *replyStr = 0;
  }
  else
    *replyStr = 0;

  // Get the string for the CHRS: kludge
  if ((helpPtr = findCLStr(message->text, "\1CHRS:")) != NULL)
  {
    if ((helpPtr2 = strchr(helpPtr + 6, 0x0d)) != NULL)
    {
      int l = helpPtr2 - helpPtr;
      if (l > 78)            // Set arbitrary max len
        l = 78;

      memcpy(chrsStr, helpPtr, l);
      chrsStr[l++] = 0x0d;
      chrsStr[l  ] = 0;
    }
    else
      *chrsStr = 0;
  }
  else
    *chrsStr = 0;

  strncpy(replyMsg->fromUserName, "FMail PING bot"     , 35);
  strncpy(replyMsg->toUserName  , message->fromUserName, 35);

  replyMsg->destNode = message->srcNode;
  replyMsg->srcNode  = localAkaNum >=0
                     ? message->destNode
                     : config.akaList[matchAka(&message->srcNode, 0)].nodeNum;

  if (strnicmp(message->subject, "Pong:", 5) == 0)
    strncpy(replyMsg->subject, message->subject, 71);
  else
    strncpy(stpcpy(replyMsg->subject, "Pong: "), message->subject, 65);

  replyMsg->attribute = LOCAL | PRIVATE;
  if (  (localAkaNum >=0 && config.pingOptions.deletePingResponses )
     || (localAkaNum < 0 && config.pingOptions.deleteTraceResponses)
     )
    replyMsg->attribute |= KILLSENT;

  setCurDateMsg(replyMsg);

  helpPtr = addINTLKludge  (replyMsg, replyMsg->text);
  helpPtr = addPointKludges(replyMsg, helpPtr);
  helpPtr = addMSGIDKludge (replyMsg, helpPtr);
  if (*replyStr != 0)
    helpPtr = addKludge(helpPtr, "REPLY:", replyStr);
  helpPtr = addTZUTCKludge (helpPtr);
  if (*chrsStr != 0)
    helpPtr = stpcpy(helpPtr, chrsStr);
  helpPtr = addPIDKludge   (helpPtr);

  // Add some text
  helpPtr += sprintf(helpPtr
                    , "\r"
                      "Hi,\r"
                      "\r"
                      "This is a %s response from %s.\r"
                      "\r"
                      "\r"
                      "Original message:\r"
                      "==============================================================================\r"
                      "From: %-36.36s  %s\r"
                      "To  : %-36.36s  %s\r"
                      "Date: %04u-%02u-%02u %02u:%02u:%02u\r"
                      "Subj: %s\r"
#ifdef ADDATTR
                      "Attr: %s\r"
#endif
                      "==============================================================================\r"
                    , funcText, nodeStr(&replyMsg->srcNode)
                    , message->fromUserName, nodeStr(&message->srcNode )
                    , message->toUserName  , nodeStr(&message->destNode)
                    , message->year, message->month, message->day, message->hours, message->minutes, message->seconds
                    , message->subject
#ifdef ADDATTR
                    , attributeStr(message->attribute)
#endif
                    );

  // Add original message text
  helpPtr2 = stpcpy(helpPtr, message->text);

  // Convert kludge lines in original
  while ((helpPtr = strchr(helpPtr, 1)) != NULL)
    *helpPtr++ = '@';

  // Check/fix if last line ends with \r char?
  if (*(helpPtr2 - 1) != '\r')
    *helpPtr2++ = '\r';

  // Add closing line
#if 0
  helpPtr2 =
#endif
  stpcpy(helpPtr2, "==============================================================================\r");

  msgNum = writeMsg(replyMsg, NETMSG, 1);

  free(replyMsg);

  logEntryf(LOG_ALWAYS, 0, "Processed PING request message from %s to %s", nodeStr(&message->srcNode), nodeStr(&message->destNode));

  return msgNum < 0;
}
//---------------------------------------------------------------------------
