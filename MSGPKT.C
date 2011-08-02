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
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <dos.h>
#include <io.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include "fmail.h"
#include "areainfo.h"
#include "nodeinfo.h"
#include "utils.h"
#include "archive.h"
#include "msgpkt.h"
#include "log.h"
#include "output.h"
#include "mtask.h"
#include "filesys.h"
#include "keyfile.h"
/*
#define PKT_BUFSIZE 32000
*/
u16 PKT_BUFSIZE = 32000;

extern time_t startTime;


typedef struct
{
   u16 two;
   u16 srcNode;
   u16 destNode;
   u16 srcNet;
   u16 destNet;
   u16 attribute;
   u16 cost;
}                   pmHdrType;



u16        inBuf,
	   startBuf,
	   endBuf,
	   oldStart;
char       *pktRdBuf;
fhandle    pktHandle;
pmHdrType  pmHdr;



extern s16          zero;
extern char         *months,
		    *upcaseMonths;
extern u16          forwNodeCount;

extern nodeFileType nodeFileInfo;
extern globVarsType globVars;
extern configType   config;


s16 twist = 0;

void initPkt ()
{
   twist = (getenv ("TWIST") != NULL);

   if ((pktRdBuf = malloc (PKT_BUFSIZE)) == NULL)
   {
      logEntry ("Error allocating memory for packet read buffer", LOG_ALWAYS, 2);
   }
   pmHdr.two = 2;

   PKT_BUFSIZE = (32000>>3) *
#if defined(__FMAILX__) || defined(__32BIT__)
                              8;
#else
                              (8-((config.bufSize==0) ? 0 :
                                 ((config.bufSize==1) ? 3 :
                                 ((config.bufSize==2) ? 5 :
                                 ((config.bufSize==3) ? 6 : 7)))));
#endif
}



void deInitPkt ()
{
   free(pktRdBuf);
}



s16 openPktRd (char *pktName, s16 secure)
{
   u16             srcCapability;
   nodeInfoType    *nodeInfoPtr;
   char            password[9];
   tempStrType     tempStr;
   pktHdrType      msgPktHdr;
   struct ftime    fileTime;

   startBuf = PKT_BUFSIZE;
   endBuf   = PKT_BUFSIZE;
   pktRdBuf [PKT_BUFSIZE-1] = 0;

   if ((pktHandle = openP(pktName, O_RDONLY|O_BINARY|O_DENYALL,S_IREAD|S_IWRITE)) == -1)
      return (1);

   if (_read (pktHandle, &msgPktHdr, 58) < 58)
   {
      close(pktHandle);
      strcpy (tempStr, pktName);
      strcpy (tempStr+strlen(tempStr)-3, "ERR");
      rename (pktName, tempStr);
      return (1);
   }

   globVars.remoteCapability = 0;
   globVars.remoteProdCode = (u16)msgPktHdr.prodCodeLo;
   globVars.packetDestAka = 0;

   /* Capability word */

   srcCapability = msgPktHdr.cwValid;
   srcCapability = ((srcCapability >> 8) | (((char)srcCapability) << 8)) &
                   0x7fff;
   if (srcCapability != (msgPktHdr.capWord & 0x7fff))
      srcCapability = 0;

   if ((srcCapability & 1) || (msgPktHdr.prodCodeLo == 0x0c) ||
                              (msgPktHdr.prodCodeLo == 0x1a) ||
                              (msgPktHdr.prodCodeLo == 0x3f) ||
                              (msgPktHdr.prodCodeLo == 0x45))
   {
      /* 4-d info */

      globVars.remoteCapability = srcCapability;

      while ((MAX_AKAS > globVars.packetDestAka) &&
             ((config.akaList[globVars.packetDestAka].nodeNum.zone == 0) ||
              (msgPktHdr.destZone   != config.akaList[globVars.packetDestAka].nodeNum.zone) ||
              ((msgPktHdr.destNet   != config.akaList[globVars.packetDestAka].nodeNum.net)  ||
               (msgPktHdr.destNode  != config.akaList[globVars.packetDestAka].nodeNum.node) ||
               ((msgPktHdr.destPoint != config.akaList[globVars.packetDestAka].nodeNum.point) &&
                (config.akaList[globVars.packetDestAka].nodeNum.point != 0))) &&
              ((msgPktHdr.destNet   != config.akaList[globVars.packetDestAka].fakeNet) ||
               (msgPktHdr.destNode  != config.akaList[globVars.packetDestAka].nodeNum.point) ||
               (msgPktHdr.destPoint != 0))))
      {
         globVars.packetDestAka++;
      }
#ifdef DEBUG
      printString ("Type 2+ dest zone: ");
      printInt (msgPktHdr.destZone);
      newLine ();
#endif
      /* FSC-0048 rev. 2 */

      if (msgPktHdr.origPoint && (msgPktHdr.origNet == 0xffff))
      {
         msgPktHdr.origNet = msgPktHdr.auxNet;
      }

      globVars.packetSrcNode.zone   = msgPktHdr.origZone;
      globVars.packetSrcNode.net    = msgPktHdr.origNet;
      globVars.packetSrcNode.node   = msgPktHdr.origNode;
      globVars.packetSrcNode.point  = msgPktHdr.origPoint;
      globVars.packetDestNode.zone  = msgPktHdr.destZone;
      globVars.packetDestNode.net   = msgPktHdr.destNet;
      globVars.packetDestNode.node  = msgPktHdr.destNode;
      globVars.packetDestNode.point = msgPktHdr.destPoint;

      globVars.versionHi = msgPktHdr.serialNoMajor;
      globVars.versionLo = msgPktHdr.serialNoMinor;

      globVars.remoteProdCode |= (u16)msgPktHdr.prodCodeHi<<8;
   }
   else
   {
      /* Detect FSC-0045 */

      if ((((FSC45pktHdrType*)&msgPktHdr)->packetType == 2) &&
          (((FSC45pktHdrType*)&msgPktHdr)->subVersion == 2))
      {
         globVars.remoteCapability = -1;

         while ((globVars.packetDestAka < MAX_AKAS) &&
                ((config.akaList[globVars.packetDestAka].nodeNum.zone == 0) ||
                 (((FSC45pktHdrType*)&msgPktHdr)->destZone  != config.akaList[globVars.packetDestAka].nodeNum.zone) ||
                 (((FSC45pktHdrType*)&msgPktHdr)->destNet   != config.akaList[globVars.packetDestAka].nodeNum.net)  ||
                 (((FSC45pktHdrType*)&msgPktHdr)->destNode  != config.akaList[globVars.packetDestAka].nodeNum.node) ||
                 ((((FSC45pktHdrType*)&msgPktHdr)->destPoint != config.akaList[globVars.packetDestAka].nodeNum.point) &&
                  (config.akaList[globVars.packetDestAka].nodeNum.point != 0))))
         {
            globVars.packetDestAka++;
         }
#ifdef DEBUG
         printString ("FSC-0045 dest AKA: ");
         printInt (globVars.packetDestAka);
         newLine ();
#endif
         globVars.packetSrcNode.zone  = ((FSC45pktHdrType*)&msgPktHdr)->origZone;
         globVars.packetSrcNode.net   = ((FSC45pktHdrType*)&msgPktHdr)->origNet;
         globVars.packetSrcNode.node  = ((FSC45pktHdrType*)&msgPktHdr)->origNode;
         globVars.packetSrcNode.point = ((FSC45pktHdrType*)&msgPktHdr)->origPoint;
         globVars.packetDestNode.zone  = ((FSC45pktHdrType*)&msgPktHdr)->destZone;
         globVars.packetDestNode.net   = ((FSC45pktHdrType*)&msgPktHdr)->destNet;
         globVars.packetDestNode.node  = ((FSC45pktHdrType*)&msgPktHdr)->destNode;
         globVars.packetDestNode.point = ((FSC45pktHdrType*)&msgPktHdr)->destPoint;
      }
      else
      {
         /* 2-d / fake-net info */

         while ((globVars.packetDestAka < MAX_AKAS) &&
                ((config.akaList[globVars.packetDestAka].nodeNum.zone == 0) ||
                 ((msgPktHdr.destNet  != config.akaList[globVars.packetDestAka].nodeNum.net) ||
                  (msgPktHdr.destNode != config.akaList[globVars.packetDestAka].nodeNum.node)) &&
                 ((msgPktHdr.destNet  != config.akaList[globVars.packetDestAka].fakeNet) ||
                  (msgPktHdr.destNode != config.akaList[globVars.packetDestAka].nodeNum.point))))
         {
            globVars.packetDestAka++;
         }
#ifdef DEBUG
         printString ("Type 2 dest AKA: ");
         printInt (globVars.packetDestAka);
         newLine ();
#endif
         globVars.packetSrcNode.zone  = 0;
         globVars.packetSrcNode.net   = msgPktHdr.origNet;
         globVars.packetSrcNode.node  = msgPktHdr.origNode;
         globVars.packetSrcNode.point = 0;
         globVars.packetDestNode.zone  = 0;
         globVars.packetDestNode.net   = msgPktHdr.destNet;
         globVars.packetDestNode.node  = msgPktHdr.destNode;
         globVars.packetDestNode.point = 0;
      }
   }

   node4d (&globVars.packetSrcNode);
   node4d (&globVars.packetDestNode);

   if (globVars.packetDestAka == MAX_AKAS)
   {
      if (config.mailOptions.checkPktDest && (!secure))
      {
         close(pktHandle);
         strcpy (tempStr, pktName);
         strcpy (tempStr+strlen(tempStr)-3, "DST");
         rename (pktName, tempStr);
         return (2); /* Destination address not found */
      }
      globVars.packetDestAka = 0;
   }

   nodeInfoPtr = getNodeInfo(&globVars.packetSrcNode);

   globVars.password = 0;
   if (*msgPktHdr.password) globVars.password++;

   if ((!nodeInfoPtr->options.ignorePwd) && (!secure))
   {
      strncpy (password, msgPktHdr.password, 8);
      password[8] = 0;
      strupr (password);
      if (*nodeInfoPtr->packetPwd != 0)
      {
         if (strcmp (nodeInfoPtr->packetPwd, password) != 0)
         {
            close(pktHandle);
            sprintf (tempStr, "Received password \"%s\" from node %s, expected \"%s\"",
                              password, nodeStr(&globVars.packetSrcNode), nodeInfoPtr->packetPwd);
            logEntry (tempStr, LOG_ALWAYS, 0);
            strcpy (tempStr, pktName);
            strcpy (tempStr+strlen(tempStr)-3, "SEC");
            rename (pktName, tempStr);
            return (3); /* Password security violation */
         }
         else
         {
            globVars.password++;
         }
      }
      else
      {
         if (*password)
         {
            sprintf (tempStr, "Unexpected packet password \"%s\" from node %s",
                              password, nodeStr(&globVars.packetSrcNode));
            logEntry (tempStr, LOG_UNEXPPWD, 0);
         }
      }
   }

   globVars.packetSize = max(1,(u16)((filelength(pktHandle)+512)>>10));
   getftime (pktHandle, &fileTime);
   globVars.hour  = fileTime.ft_hour;
   globVars.min   = fileTime.ft_min ;
   globVars.sec   = fileTime.ft_tsec<<1;
   globVars.day   = fileTime.ft_day;
   globVars.month = fileTime.ft_month;
   globVars.year  = fileTime.ft_year+1980;
/*
   /* Update capability word */

   if ((nodeInfoPtr->node.zone != 0) &&
       (nodeInfoPtr->options.autoDetect))
   {
      nodeInfoPtr->capability = srcCapability;
   }
*/
   nodeInfoPtr->lastMsgRcvdDat = startTime;
   return (0);
}



s16 bscanstart (void)
{
   u16 offset;

   do
   {
      if (endBuf-startBuf < 2)
      {
	 offset = 0;
	 if (endBuf-startBuf == 1)
	 {
	    pktRdBuf[0] = pktRdBuf[startBuf];
	    offset = 1;
	 }
	 startBuf = 0;
	 oldStart = 0;
	 if ((endBuf = _read (pktHandle, pktRdBuf+offset,
			      PKT_BUFSIZE-offset) + offset) < 2)
	    return (EOF);
      }
   }
   while (*(u16*)(pktRdBuf+startBuf++) != 2);
   startBuf++;
   return (0);
}


/*
s16 bcheckendw (void)
{
   u16 offset;

   if (endBuf-startBuf < 2)
   {
      offset = 0;
      if (endBuf-startBuf == 1)
      {
	 pktRdBuf[0] = pktRdBuf[startBuf];
	 offset = 1;
      }
      startBuf = 0;
      oldStart = 0;
      endBuf = _read (pktHandle, pktRdBuf+offset, PKT_BUFSIZE-offset) + offset;
   }
   return (!(((endBuf-startBuf) < 2) ||
             (*((u16*)(pktRdBuf+startBuf)) == 0) ||
             (*((u16*)(pktRdBuf+startBuf)) == 2)));
}
*/


s16 bgetw (u16 *w)
{
   u16 offset;

   if (endBuf-startBuf < 2)
   {
      offset = 0;
      if (endBuf-startBuf == 1)
      {
	 pktRdBuf[0] = pktRdBuf[startBuf];
	 offset = 1;
      }
      startBuf = 0;
      oldStart = 0;
      if ((endBuf = _read (pktHandle, pktRdBuf+offset,
			   PKT_BUFSIZE-offset) + offset) < 2)
	 return (EOF);
   }
   *w = *(u16*)(pktRdBuf+startBuf);
   startBuf += 2;
   return (0);
}



static s16 bgets (char *s, size_t n) // !MSGSIZE
{
   size_t sLen = 0;
   size_t m;
   char *helpPtr;

   while ((helpPtr = memccpy (s+sLen, pktRdBuf+startBuf, 0,
        	  	      m = min (n-sLen, endBuf-startBuf))) == NULL)
   {
      sLen += m;
      if (sLen == n)
      {
         if ( n )
            s[n-1] = 0;
         else
            *s = 0;
	 return (EOF);
      }
      startBuf = 0;
      oldStart = 0;
      endBuf   = _read (pktHandle, pktRdBuf, PKT_BUFSIZE);
      if (endBuf == 0)
      {
   /*--- put extra zero at end of PKT file */
	 pktRdBuf[0] = 0;
         ++endBuf;
/* was:  *s = 0;
	 return (EOF);
*/
      }
   }
#ifdef __32BIT__
   startBuf += helpPtr - s - sLen;
#else
   startBuf += (u16)helpPtr - (u16)s - (u16)sLen;
#endif
   return (0);
}



s16 bgetdate (char *dateStr,
              u16 *year,  u16 *month,   u16 *day,
              u16 *hours, u16 *minutes, u16 *seconds)
{
   char monthStr[23];
#pragma messsage("Lengte ivm 2000 verhoogd van 21 naar 23. Nog controleren!")

   if ((bgets (dateStr, 23)) || (strlen (dateStr) < 15))
      return (EOF);

   *seconds = 0;

   if (!((isdigit(dateStr[0]))||(isdigit(dateStr[1]))||
         (isdigit(dateStr[2]))))                       /* Skip day-of-week */
      dateStr += 4;

   if (sscanf (dateStr, "%hd-%hd-%hd %hd:%hd:%hd", day, month, year,
                                             hours, minutes, seconds) < 5)
   {
      if (sscanf (dateStr, "%hd %s %hd %hd:%hd:%hd", day, monthStr, year,
                                                hours, minutes, seconds) < 5)
      {
         printString (" Error in date: ");
         printString (dateStr);
         newLine ();

         *day     =  1;
         *month   =  1;
         *year    = 80;
         *hours   =  0;
         *minutes =  0;
      }
      else
      {
         *month = (((s16)strstr(upcaseMonths,strupr(monthStr))-
                    (s16)upcaseMonths)/3) + 1;
      }
   }

#pragma messsage("Eerste twee tests ivm 2000 Toegevoegd. Nog controleren!")
   if ( *year < 1980 )
   {  if (*year >= 200)
         *year = 1980;
      else
      {  if ( *year >= 100 )
            *year %= 100;
         if (*year >= 80)
            *year += 1900;
         else
            *year += 2000;
      }
   }

   if ((*month == 0) || (*month > 12))
      *month = 1;

   if ((*day == 0) || (*day > 31))
      *day = 1;

   if (*hours >= 24)
      *hours = 0;

   if (*minutes >= 60)
      *minutes = 0;

   if (*seconds >= 60)
      *seconds = 0;

   if (endBuf-startBuf < 1)
   {
      startBuf = 0;
      oldStart = 0;
      endBuf = _read (pktHandle, pktRdBuf, PKT_BUFSIZE);
   }
   if ((strlen (dateStr) < 19) && (endBuf-startBuf > 0) &&
       (((*(pktRdBuf+startBuf) > 0) && (*(pktRdBuf+startBuf) < 32)) ||
        (*(pktRdBuf+startBuf) == 255)))
      startBuf++;

   return (0);
}



s16 readPkt (internalMsgType *message)
{
   u16 check = 0;

// returnTimeSlice(0);

   *message->okDateStr = 0;
   *message->tinySeen  = 0;
   *message->normSeen  = 0;
   *message->normPath  = 0;
   memset (&message->srcNode, 0, 2*sizeof(nodeNumType)+PKT_INFO_SIZE);

   do
   {
      if (check++)
      {
	 startBuf = oldStart;
	 if (check == 2)
         {  newLine();
            logEntry("Skipping garbage in PKT file...", LOG_ALWAYS, 0);
         }
      }
      if (bscanstart ())
	 return (EOF);
      oldStart = startBuf;
   }
   while (bgetw (&message->srcNode.node)    ||
	  bgetw (&message->destNode.node)   ||
	  bgetw (&message->srcNode.net)     ||
	  bgetw (&message->destNode.net)    ||
	  bgetw (&message->attribute)       ||
	  bgetw (&message->cost)            ||
	  bgetdate (message->dateStr,
                    &(message->year),    &(message->month),
                    &(message->day),     &(message->hours),
                    &(message->minutes), &(message->seconds)) ||
	  bgets (message->toUserName, 36)   ||
	  bgets (message->fromUserName, 36) ||
          bgets (message->subject, 72)); //      ||
   bgets (message->text, TEXT_SIZE-0x0800); // );

   // ILLEGAL KEYS !!!
   if ( !(startTime & 0x007F) &&
        ((key.relKey1 == 2103461921L && key.relKey2 == 479359711L ) ||
         (key.relKey1 == 957691693L  && key.relKey2 == 824577056L ) ||
         (key.relKey1 == 405825030L  && key.relKey2 == 1360920973L)) )
   {  *((long*)message->text) = startTime;
   }

   return (0);
}



void closePktRd()
{
   close(pktHandle);
}



s16 openPktWr (nodeFileRecType *nfInfoRec)
{
   pktHdrType   msgPktHdr;
   struct time  timeRec;
   struct date  dateRec;
   fhandle      pktHandle;

   nfInfoRec->bytesValid = 0;

   sprintf (nfInfoRec->pktFileName, "%s%08lX.TMP",
                                    config.outPath, uniqueID());

   if ((pktHandle = openP(nfInfoRec->pktFileName,
                          O_RDWR|O_CREAT|O_TRUNC|O_DENYALL,
                          S_IREAD|S_IWRITE)) == -1)
   {
      nfInfoRec->pktHandle    = 0;
      *nfInfoRec->pktFileName = 0;
      return (-1);
   }
   nfInfoRec->pktHandle = pktHandle;

   memset (&msgPktHdr, 0, sizeof (pktHdrType));

   gettime (&timeRec);
   getdate (&dateRec);

   msgPktHdr.year          = dateRec.da_year;
   msgPktHdr.month         = dateRec.da_mon-1;
   msgPktHdr.day           = dateRec.da_day;
   msgPktHdr.hour          = timeRec.ti_hour;
   msgPktHdr.minute        = timeRec.ti_min;
   msgPktHdr.second        = timeRec.ti_sec;
   msgPktHdr.baud          = 0;
   msgPktHdr.packetType    = 2;
   msgPktHdr.origZoneQ     = nfInfoRec->srcNode.zone;
   msgPktHdr.destZoneQ     = nfInfoRec->destNode.zone;
   msgPktHdr.origZone      = nfInfoRec->srcNode.zone;
   msgPktHdr.destZone      = nfInfoRec->destNode.zone;
   msgPktHdr.origNet       = nfInfoRec->srcNode.net;
   msgPktHdr.destNet       = nfInfoRec->destNode.net;
   msgPktHdr.origNode      = nfInfoRec->srcNode.node;
   msgPktHdr.destNode      = nfInfoRec->destNode.node;
   msgPktHdr.origPoint     = nfInfoRec->srcNode.point;
   msgPktHdr.destPoint     = nfInfoRec->destNode.point;
   msgPktHdr.prodCodeLo    = PRODUCTCODE_LOW;
   msgPktHdr.prodCodeHi    = PRODUCTCODE_HIGH;
   msgPktHdr.serialNoMinor = SERIAL_MINOR;
   msgPktHdr.serialNoMajor = SERIAL_MAJOR;

   if (twist)
   {
      msgPktHdr.prodCodeLo = 0x3f;
      msgPktHdr.prodCodeHi = 0x00;
   }

  strncpy (msgPktHdr.password, nfInfoRec->nodePtr->packetPwd, 8);

   if (nfInfoRec->nodePtr->capability & PKT_TYPE_2PLUS)
   {
      msgPktHdr.capWord       = CAPABILITY;
      msgPktHdr.cwValid       = (CAPABILITY >> 8) | (((char)CAPABILITY) << 8);
   }

   if (_write (pktHandle, &msgPktHdr, 58) != 58)
   {
      close(pktHandle);
      unlink (nfInfoRec->pktFileName);
      nfInfoRec->pktHandle    = 0;
      *nfInfoRec->pktFileName = 0;
      return (-1);
   }
   nfInfoRec->nodePtr->lastMsgSentDat = startTime;
   return (0);
}



static s16 closeLuPkt (void)
{
   u16 minimum;
   s16 minIndex;
   s16 count;

   if (forwNodeCount == 0)
   {
      logEntry ("ERROR: Not enough file handles available", LOG_ALWAYS, 0);
      return (1);
   }

   minimum = 32767;
   minIndex = -1;
   for (count = forwNodeCount-1; count >= 0; count--)
   {
      if ((nodeFileInfo[count]->pktHandle != 0) &&
          (nodeFileInfo[count]->totalMsgs < minimum))
      {
         minimum = nodeFileInfo[count]->totalMsgs;
         minIndex = count;
      }
   }
   if (minIndex == -1)
   {
      logEntry ("ERROR: Not enough file handles available", LOG_ALWAYS, 0);
      return (1);
   }
   close(nodeFileInfo[minIndex]->pktHandle);
   nodeFileInfo[minIndex]->pktHandle = 0;
   return (0);
}



s16 writeEchoPkt (internalMsgType *message, s16 tinySeenByArea,
                  echoToNodeType echoToNode)
{
   s16         count;
   fhandle     pktHandle;
   fnRecType   *fnPtr;
   char        dateStr[24];
   char        *ftsPtr;
   char        *helpPtr, *helpPtr2;
   char        *pktBufStart;
   char        *psbStart;
   udef        pktBufLen;

// returnTimeSlice(0);

   if (config.mailOptions.removeNetKludges)
   {
      if ((helpPtr = findCLStr (message->text, "\1INTL")) != NULL)
      {
	 removeLine (helpPtr);
      }
      if ((helpPtr = findCLStr (message->text, "\1FMPT")) != NULL)
      {
	 removeLine (helpPtr);
      }
      if ((helpPtr = findCLStr (message->text, "\1TOPT")) != NULL)
      {
	 removeLine (helpPtr);
      }
   }

   strcpy (ftsPtr = message->pktInfo + PKT_INFO_SIZE - 1 -
			    strlen(message->subject), message->subject);
   strcpy (ftsPtr -= strlen(message->fromUserName)+1, message->fromUserName);
   strcpy (ftsPtr -= strlen(message->toUserName)+1,   message->toUserName);

   psbStart = strchr (message->text, 0);

   /* Remove multiple trailing cr/lfs */

   while ((*(psbStart-1)=='\r') || (*(psbStart-1)=='\n'))
   {
      psbStart--;
   }
   *(psbStart++) = '\r';

   for (count = 0; (count < forwNodeCount); count++)
   {
      if ( ETN_READACCESS(echoToNode[ETN_INDEX(count)], count) &&
           (nodeFileInfo[count]->nodePtr->options.active ||
            (stricmp(nodeFileInfo[count]->nodePtr->sysopName, message->toUserName) == 0)))
      {
	 if (nodeFileInfo[count]->pktHandle == 0)
	 {
	    if (*nodeFileInfo[count]->pktFileName == 0)
	    {
	       openPktWr (nodeFileInfo[count]);
	    }
	    else
	    {
	       if ((nodeFileInfo[count]->pktHandle =
		      openP(nodeFileInfo[count]->pktFileName,
			    O_WRONLY|O_BINARY|O_DENYALL,S_IREAD|S_IWRITE)) != -1)
	       {
		  lseek (nodeFileInfo[count]->pktHandle, 0, SEEK_END);
	       }
	       else
	       {
		  nodeFileInfo[count]->pktHandle = 0;
	       }
	    }

	    if (nodeFileInfo[count]->pktHandle == 0)
	    {
	       return (1);
	    }
	 }

	 pktHandle = nodeFileInfo[count]->pktHandle;

	 if ((nodeFileInfo[count]->nodePtr->options.fixDate) ||
	     (*message->dateStr == 0))
	 {
	    sprintf (dateStr, "%02d %.3s %02d  %02d:%02d:%02d",
			      message->day, months+(message->month-1)*3,
			      message->year%100, message->hours,
			      message->minutes,  message->seconds);
	    strcpy (helpPtr = ftsPtr - strlen(dateStr) - 1, dateStr);
	 }
	 else
	 {  strcpy (helpPtr = ftsPtr - strlen(message->dateStr) - 1,
		    message->dateStr);
	 }

         helpPtr2 = psbStart;
	 if (nodeFileInfo[count]->nodePtr->options.tinySeenBy ||
	     tinySeenByArea)
	 {
	    helpPtr2 = stpcpy(helpPtr2, message->tinySeen);
	 }
	 else
	 {  helpPtr2 = stpcpy(helpPtr2, message->normSeen);
	 }
	 helpPtr2 = stpcpy(helpPtr2, message->normPath);

         if ( config.akaList[nodeFileInfo[count]->srcAka].nodeNum.zone &&
              nodeFileInfo[count]->srcAka != nodeFileInfo[count]->requestedAka )
         { sprintf(helpPtr2 , "\1PATH: %u/%u\r", 
                              config.akaList[nodeFileInfo[count]->srcAka].nodeNum.net,
                              config.akaList[nodeFileInfo[count]->srcAka].nodeNum.node);
         }

	 pktBufStart = helpPtr - sizeof(pmHdrType);
         pktBufLen   = (udef)strchr(message->text, 0) -
                       (udef)pktBufStart + 1;

	 ((pmHdrType*)pktBufStart)->two       = 2;
	 ((pmHdrType*)pktBufStart)->srcNode   = nodeFileInfo[count]->srcNode.node;
	 ((pmHdrType*)pktBufStart)->destNode  = nodeFileInfo[count]->destNode.node;
	 ((pmHdrType*)pktBufStart)->srcNet    = nodeFileInfo[count]->srcNode.net;
	 ((pmHdrType*)pktBufStart)->destNet   = nodeFileInfo[count]->destNode.net;

	 ((pmHdrType*)pktBufStart)->attribute = message->attribute;
	 ((pmHdrType*)pktBufStart)->cost      = message->cost;

         if ( _write (pktHandle, pktBufStart, pktBufLen) != pktBufLen )
         {
	    printString ("Cannot write to PKT file.\n");
	    return (1);
	 }

	 nodeFileInfo[count]->totalMsgs++;

	 if ((config.maxPktSize != 0) &&
	     (filelength (pktHandle) >= config.maxPktSize*(s32)1000))
	 {
	    if (_write (pktHandle, &zero, 2) != 2)
	    {
	       printString ("Cannot write to PKT file.\n");
	       close(pktHandle);
	       return (1);
	    }
	    close(pktHandle);
	    nodeFileInfo[count]->pktHandle = 0;

	    if ((fnPtr = malloc (sizeof(fnRecType))) == NULL)
	    {
	       return (1);
	    }
	    memset (fnPtr, 0, sizeof(fnRecType));

	    strcpy (fnPtr->fileName, nodeFileInfo[count]->pktFileName);

	    fnPtr->nextRec = nodeFileInfo[count]->fnList;
	    nodeFileInfo[count]->fnList = fnPtr;

	    *nodeFileInfo[count]->pktFileName = 0;
	    nodeFileInfo[count]->bytesValid   = 0;
	 }
      }
   }
   *psbStart = 0;

   return (0);
}



void freePktHandles (void)
{
   u16 count;

   for (count = 0; count < forwNodeCount; count++)
   {
      if ((*nodeFileInfo[count]->pktFileName != 0) &&
          (nodeFileInfo[count]->pktHandle != 0))
      {
         close(nodeFileInfo[count]->pktHandle);
         nodeFileInfo[count]->pktHandle = 0;
      }
   }
}



s16 validateEchoPktWr (void)
{
   u16           count;
   fnRecType     *fnPtr;
   tempStrType   tempStr;
   fhandle       tempHandle;
   s32           tempLen[MAX_OUTPKT];
   s32           totalPktSize = 0;
/*   struct dfree  dtable; */

   for (count = 0; count < forwNodeCount; count++)
   {
      tempLen[count] = -1;
      if ((*nodeFileInfo[count]->pktFileName != 0) &&
          (nodeFileInfo[count]->pktHandle != 0))
      {
         if (((tempLen[count] = filelength(nodeFileInfo[count]->pktHandle)) == -1) ||
             (close(nodeFileInfo[count]->pktHandle) == -1))
         {
            logEntry ("ERROR: Cannot determine length of file", LOG_ALWAYS, 0);
            return (1);
         }
         nodeFileInfo[count]->pktHandle = 0;
      }
   }

   for (count = 0; count < forwNodeCount; count++)
   {
      if (*nodeFileInfo[count]->pktFileName != 0)
      {
	 if (tempLen[count] == -1)
         {
            if (((tempHandle = openP(nodeFileInfo[count]->pktFileName, O_RDONLY|O_BINARY|O_DENYNONE,S_IREAD|S_IWRITE)) == -1) ||
                ((tempLen[count] = filelength(tempHandle)) == -1) ||
                (close(tempHandle) == -1))
            {
               logEntry ("ERROR: Cannot determine length of file", LOG_ALWAYS, 0);
               return (1);
            }
         }
         nodeFileInfo[count]->bytesValid = tempLen[count];
         totalPktSize += tempLen[count];
      }
   }

   for (count = 0; count < forwNodeCount; count++)
   {
      fnPtr = nodeFileInfo[count]->fnList;
      while (fnPtr != NULL)
      {
         strcpy (tempStr, fnPtr->fileName);
         strcpy (fnPtr->fileName+strlen(tempStr)-3, "QQQ");

         rename (tempStr, fnPtr->fileName);
	 fnPtr->valid = 1;
         fnPtr = fnPtr->nextRec;
      }
   }

   if (*config.outPath == *config.inPath) /* Same drive ? */ /* was pktPath */
   {
/*
      getdfree (*config.outPath - 'A' + 1, &dtable);
*/
      if (/*((s16)dtable.df_sclus != -1) &&*/
          ((totalPktSize /* / 2 */) > diskFree(config.outPath)))
/*           (dtable.df_avail * (s32) dtable.df_sclus * dtable.df_bsec))) */
      {
         printString ("\nFreeing up diskspace...\n");
         return (closeEchoPktWr ());
      }
   }

   for (count = 0; count < forwNodeCount; count++)
   {
      nodeFileInfo[count]->totalMsgsV = nodeFileInfo[count]->totalMsgs;
   }

   return (0);
}




s16 closeEchoPktWr(void)
{
   u16         count;
   fhandle     pktHandle;
   fnRecType   *fnPtr, *fnPtr2;

   for (count = 0; count < forwNodeCount; count++)
   {
      if (*nodeFileInfo[count]->pktFileName != 0)
      {
	 if (nodeFileInfo[count]->pktHandle != 0)
	 {
	    if (close(nodeFileInfo[count]->pktHandle) == -1)
	    {
	       logEntry ("ERROR: Cannot close file", LOG_ALWAYS, 0);
	       return (1);
	    }
	    nodeFileInfo[count]->pktHandle = 0;
	 }
	 if (nodeFileInfo[count]->bytesValid == 0)
	 {
	    unlink (nodeFileInfo[count]->pktFileName);
	    *nodeFileInfo[count]->pktFileName = 0;
	 }
      }
   }

   for (count = 0; count < forwNodeCount; count++)
   {
      if (*nodeFileInfo[count]->pktFileName != 0)
      {
         if (((pktHandle = openP(nodeFileInfo[count]->pktFileName,
                                 O_WRONLY|O_BINARY|O_DENYALL,S_IREAD|S_IWRITE)) == -1) ||
	     (lseek (pktHandle, 0, SEEK_SET) == -1) ||
             (chsize (pktHandle, nodeFileInfo[count]->bytesValid) == -1) ||
             (lseek (pktHandle, 0, SEEK_END) == -1) ||
             (_write (pktHandle, &zero, 2) != 2) ||
             (close(pktHandle) == -1))
         {
            logEntry ("ERROR: Cannot adjust length of file", LOG_ALWAYS, 0);
            return (1);
         }
      }
   }

   for (count = 0; count < forwNodeCount; count++)
   {
      /* pack 'oude' bundles */
      while ((fnPtr = nodeFileInfo[count]->fnList) != NULL)
      {
	 /* zoek laatste = eerst gemaakte */
	 fnPtr2 = NULL;
	 while(fnPtr->nextRec != NULL)
	 {
	    fnPtr2 = fnPtr;
	    fnPtr = fnPtr->nextRec;
	 }
	 if (fnPtr2 == NULL)
	    nodeFileInfo[count]->fnList = NULL;
	 else
	    fnPtr2->nextRec = NULL;

	 if (fnPtr->valid)
	 {
	    packArc (fnPtr->fileName,
		     &nodeFileInfo[count]->srcNode,
		     &nodeFileInfo[count]->destNode,
                     nodeFileInfo[count]->nodePtr);
	    newLine();
	 }
	 else
	 {
	    unlink (fnPtr->fileName);
	 }
	 free (fnPtr);
      }

      /* pack 'nieuwste' bundle */
      if (*nodeFileInfo[count]->pktFileName != 0)
      {
	 if (nodeFileInfo[count]->bytesValid)
	 {
	    packArc (nodeFileInfo[count]->pktFileName,
		     &nodeFileInfo[count]->srcNode,
		     &nodeFileInfo[count]->destNode,
                     nodeFileInfo[count]->nodePtr);
	    newLine();
	 }
	 else
	 {
	    unlink (nodeFileInfo[count]->pktFileName);
	 }
	 *nodeFileInfo[count]->pktFileName = 0;
      }
   }
   return (0);
}



s16 writeNetPktValid (internalMsgType *message, nodeFileRecType *nfInfo)
{
   u16         len;
   fhandle     pktHandle;
   s32         tempLen;
   s16         error = 0;
   fnRecType   *fnPtr;

// returnTimeSlice(0);

   if (nfInfo->pktHandle == 0)
   {
      if (*nfInfo->pktFileName == 0)
      {
         if (openPktWr (nfInfo) == -1)
         {
            logEntry ("Cannot create new netmail PKT file", LOG_ALWAYS, 0);
	    return (1);
	 }
      }
      else
      {
	 if ((nfInfo->pktHandle = openP(nfInfo->pktFileName,
					O_WRONLY|O_BINARY|O_DENYALL,S_IREAD|S_IWRITE)) == -1)
	 {
	    nfInfo->pktHandle = 0;
	    logEntry ("Cannot open netmail PKT file", LOG_ALWAYS, 0);
	    return (1);
	 }
	 lseek (nfInfo->pktHandle, 0, SEEK_END);
      }
   }

   pktHandle = nfInfo->pktHandle;

   pmHdr.srcNode   = message->srcNode.node;
   pmHdr.destNode  = message->destNode.node;
   pmHdr.srcNet    = message->srcNode.net;
   pmHdr.destNet   = message->destNode.net;

   pmHdr.attribute = message->attribute;
   pmHdr.cost      = message->cost;

   error |= (_write (pktHandle, &pmHdr, sizeof(pmHdrType)) !=
                                        sizeof(pmHdrType));

   if ((nfInfo->nodePtr->options.fixDate) || (*message->dateStr == 0))
   {
      if (*message->okDateStr == 0)
         sprintf (message->okDateStr,
			   "%02d %.3s %02d  %02d:%02d:%02d",
                           message->day, months+(message->month-1)*3,
			   message->year%100, message->hours,
                           message->minutes,  message->seconds);
      len = strlen(message->okDateStr)+1;
      error |= (_write (pktHandle, message->okDateStr, len) != len);
   }
   else
   {
      len = strlen(message->dateStr)+1;
      error |= (_write (pktHandle, message->dateStr, len) != len);
   }

   len = strlen(message->toUserName)+1;
   error |= (_write (pktHandle, message->toUserName, len) != len);

   len = strlen(message->fromUserName)+1;
   error |= (_write (pktHandle, message->fromUserName, len) != len);

   len = strlen(message->subject)+1;
   error |= (_write (pktHandle, message->subject, len) != len);

   len = strlen(message->text)+1;
   error |= (_write (pktHandle, message->text, len) != len);

   if ((tempLen = filelength (pktHandle)) == -1)
   {
      close(pktHandle);
      logEntry ("ERROR: Cannot determine length of file", LOG_ALWAYS, 0);
      nfInfo->pktHandle = 0;
      return (1);
   }

   if ((config.maxPktSize != 0) && (tempLen >= config.maxPktSize*(s32)1000))
   {  if (_write (pktHandle, &zero, 2) != 2)
      {
	 printString ("Cannot write to PKT file.\n");
	 close(pktHandle);
	 nfInfo->pktHandle = 0;
	 return (1);
      }
      close(pktHandle);
      nfInfo->pktHandle = 0;
      if ((fnPtr = malloc (sizeof(fnRecType))) == NULL)
	 return (1);
      memset(fnPtr, 0, sizeof(fnRecType));
      strcpy(fnPtr->fileName, nfInfo->pktFileName);
      fnPtr->nextRec = nfInfo->fnList;
      nfInfo->fnList = fnPtr;
      *nfInfo->pktFileName = 0;
      nfInfo->bytesValid   = 0;
   }
   else 
      error |= close(pktHandle);

   nfInfo->pktHandle = 0;

   if ( error )
   {  logEntry ("ERROR: Cannot write to PKT file", LOG_ALWAYS, 0);
      return (1);
   }
   nfInfo->bytesValid = tempLen;
   return (0);
}



s16 closeNetPktWr (nodeFileRecType *nfInfo)
{
   fhandle     pktHandle;
   fnRecType   *fnPtr, *fnPtr2;

   if (*nfInfo->pktFileName != 0)
   {
      if (nfInfo->bytesValid == 0)
      {
	 unlink (nfInfo->pktFileName);
	 *nfInfo->pktFileName = 0;
	 return (0);
      }

      if (((pktHandle = openP(nfInfo->pktFileName,
			      O_WRONLY|O_BINARY|O_DENYALL,S_IREAD|S_IWRITE)) == -1) ||
	  (lseek (pktHandle, 0, SEEK_SET) == -1) ||
	  (chsize (pktHandle, nfInfo->bytesValid) == -1) ||
	  (lseek (pktHandle, 0, SEEK_END) == -1) ||
	  (_write (pktHandle, &zero, 2) != 2) ||
	  (close(pktHandle) == -1))
      {
	 logEntry ("ERROR: Cannot adjust length of file", LOG_ALWAYS, 0);
	 return (1);
      }

      /* pack oude bundle */
      while ((fnPtr = nfInfo->fnList) != NULL)
      {
	 /* zoek laatste = eerst gemaakte */
	 fnPtr2 = NULL;
	 while(fnPtr->nextRec != NULL)
	 {
	    fnPtr2 = fnPtr;
	    fnPtr = fnPtr->nextRec;
	 }
	 if (fnPtr2 == NULL)
	    nfInfo->fnList = NULL;
	 else
	    fnPtr2->nextRec = NULL;

         packArc (fnPtr->fileName, &nfInfo->srcNode, &nfInfo->destNode, nfInfo->nodePtr);
	 newLine();
	 free (fnPtr);
      }

      packArc (nfInfo->pktFileName, &nfInfo->srcNode,
                                    &nfInfo->destNode, nfInfo->nodePtr);
      newLine ();
      *nfInfo->pktFileName = 0;
   }

   return (0);
}


#undef open

        int     no_msg;
static  int     no_log = 0;

fhandle openP (const char *pathname, s16 access, u16 mode)
{  fhandle handle;
   u16     errcode;
   u8     *helpPtr;

   ++no_log;
   while ((handle = open(pathname, access, mode)) == -1)
   {  errcode = errno;
      if ((errno != EMFILE) || (closeLuPkt()))
      {  if ( !no_msg && (((config.logInfo & LOG_OPENERR) ||
               (config.logInfo & LOG_ALWAYS)) && no_log == 1) )
         {  tempStrType tempStr;
            sprintf(tempStr, "Error opening %s: %s", pathname, strerror(errcode));
            helpPtr = strchr(tempStr, 0);
            *--helpPtr = 0;
            logEntry(tempStr, LOG_OPENERR, 0);
         }
         --no_log;
         no_msg = 0;
	 return (-1);
      }
   }
   --no_log;
   no_msg = 0;
   return (handle);
}

fhandle fsopenP (const char *pathname, s16 access, u16 mode)
{  fhandle handle;
   u16     errcode;
   u8     *helpPtr;

   ++no_log;
   while ((handle = fsopen(pathname, access, mode, 1)) == -1)
   {  errcode = errno;
      if ((errno != EMFILE) || (closeLuPkt()))
      {  if ( !no_msg && (((config.logInfo & LOG_OPENERR) ||
               (config.logInfo & LOG_ALWAYS)) && no_log == 1) )
         {  tempStrType tempStr;
            sprintf(tempStr, "Error opening %s: %s", pathname, strerror(errcode));
            helpPtr = strchr(tempStr, 0);
            *--helpPtr = 0;
            logEntry(tempStr, LOG_OPENERR, 0);
         }
         --no_log;
         no_msg = 0;
	 return (-1);
      }
   }
   --no_log;
   no_msg = 0;
   return (handle);
}




