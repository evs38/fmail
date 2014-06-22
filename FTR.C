//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2014 Wilfred van Velzen
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
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dir.h> /* notify */

#include "fmail.h"
#include "areainfo.h"
#include "crc.h"
#include "msgra.h"
#include "msgmsg.h"
#include "log.h"
#include "output.h"
#include "utils.h"
#include "ftools.h"
#include "ftr.h"
#include "cfgfile.h"
#include "version.h"

extern configType config;
extern const char *months;


void addInfo (internalMsgType *message, s16 isNetmail)
{
  tempStrType tempStr;

  sprintf(tempStr, "\1PID: %s\r", PIDStr());
  insertLine(message->text, tempStr);

  sprintf(tempStr, "\1MSGID: %s %08lx\r", nodeStr(&message->srcNode), uniqueID());
  insertLine (message->text, tempStr);

  if (isNetmail)
  {
    /* TOPT kludge */

    if (message->destNode.point != 0)
    {
      sprintf (tempStr, "\1TOPT %u\r", message->destNode.point);
      insertLine (message->text, tempStr);
    }

    /* FMPT kludge */

    if (message->srcNode.point != 0)
    {
      sprintf (tempStr, "\1FMPT %u\r", message->srcNode.point);
      insertLine (message->text, tempStr);
    }

    addINTL (message);
  }
}



s16 writeNetMsg (internalMsgType *message, s16 srcAka, nodeNumType *destNode,
                 u16 capability, u16 outStatus)
{
  fhandle     msgHandle;
  u16         len;
  tempStrType tempStr;
  char        *helpPtr;
  s32         highMsgNum;
  u16         count;
  msgMsgType  msgMsg;
  s16         doneMsg;
  struct ffblk ffblkMsg;
  struct date  dateRec;
  struct time  timeRec;

  srcAka = matchAka(destNode, srcAka+1);

  message->srcNode = config.akaList[srcAka].nodeNum;
  message->destNode = *destNode;

  if ((!(capability & PKT_TYPE_2PLUS)) &&
      (isLocalPoint(destNode)))
  {
    node2d (&message->srcNode);
    node2d (&message->destNode);
  }

  addInfo (message, 1);

  if (outStatus != 0xFFFF)
  {
    message->attribute = LOCAL | PRIVATE | KILLSENT;

    switch (outStatus)
    {
      case 1 :
        message->attribute |= HOLDPICKUP;
        break;
      case 2 :
        message->attribute |= CRASH;
        break;
      case 3 :
        message->attribute |= HOLDPICKUP;
        insertLine (message->text, "\1FLAGS DIR\r");
        break;
      case 4 :
        message->attribute |= CRASH;
      case 5 :
        insertLine (message->text, "\1FLAGS DIR\r");
        break;
    }
  }
  memset (&msgMsg, 0, sizeof(msgMsgType));

  strcpy (msgMsg.fromUserName, message->fromUserName);
  strcpy (msgMsg.toUserName, message->toUserName);
  strcpy (msgMsg.subject, message->subject);

  gettime (&timeRec);
  getdate (&dateRec);

  sprintf (msgMsg.dateTime, "%02u %.3s %02u  %02u:%02u:%02u",
           dateRec.da_day,      months+(dateRec.da_mon-1)*3,
           dateRec.da_year%100, timeRec.ti_hour,
           timeRec.ti_min,      timeRec.ti_sec);

  msgMsg.wrTime.hours    = timeRec.ti_hour;
  msgMsg.wrTime.minutes  = timeRec.ti_min;
  msgMsg.wrTime.dSeconds = timeRec.ti_sec >> 1;
  msgMsg.wrTime.day      = dateRec.da_day;
  msgMsg.wrTime.month    = dateRec.da_mon;
  msgMsg.wrTime.year     = dateRec.da_year-1980;

  msgMsg.origNet   = message->srcNode.net;
  msgMsg.origNode  = message->srcNode.node;
  msgMsg.destNet   = message->destNode.net;
  msgMsg.destNode  = message->destNode.node;

  msgMsg.attribute = message->attribute;

  strcpy (tempStr, config.netPath);
  helpPtr = strchr (tempStr, 0);

  /* Determine highest message */

  highMsgNum = 0;
  strcpy(helpPtr, "lastread");
  if ((msgHandle = open(tempStr, O_RDONLY|O_BINARY|O_DENYNONE, S_IREAD|S_IWRITE)) != -1)
  {
    if (read(msgHandle, &highMsgNum, 2) != 2)
      highMsgNum = 0;
    close(msgHandle);
  }
  strcpy (helpPtr, "*.msg");
  doneMsg = findfirst (tempStr, &ffblkMsg, FA_RDONLY|FA_HIDDEN|FA_SYSTEM|
                       /*FA_LABEL|*/FA_DIREC);

  while (!doneMsg)
  {
    highMsgNum = max (highMsgNum, atol (ffblkMsg.ff_name));
    doneMsg = findnext (&ffblkMsg);
  }

  /* Try to open file */

  count = 0;
  sprintf (helpPtr, "%lu.msg", ++highMsgNum);

  while ((count < 20) &&
         ((msgHandle = open(tempStr, O_RDWR|O_CREAT|O_EXCL|
                            O_TRUNC|O_BINARY|O_DENYNONE,
                            S_IREAD|S_IWRITE)) == -1))
  {
    highMsgNum += count++ < 10 ? 1 : 10;
    sprintf (helpPtr, "%lu.msg", highMsgNum);
  }

  if (msgHandle == -1)
  {
    printString ("Can't open output file.\n");
    return (1);
  }

  len = strlen(message->text)+1;

  if ((_write (msgHandle, &msgMsg, sizeof(msgMsgType)) !=
       sizeof(msgMsgType)) ||
      (_write (msgHandle, message->text, len) != len))
  {
    close (msgHandle);
    printString ("Can't write to output file.\n");
    return (1);
  }
  close(msgHandle);

  return (0);
}



s16 readNodeNum (char *nodeString, nodeNumType *nodeNum, u16 *valid)
{
  u16 v, order = 0;

  nodeNum->point = 0;

  if (*nodeString == '.')
  {
    if (*valid >= 3)
    {
      if (isdigit (*++nodeString))
      {
        nodeNum->point = atoi (nodeString);
        while (isdigit(*++nodeString));
        if (*nodeString == 0)
        {
          *valid = 4;
          return (0);
        }
      }
      else
      {
        if ((*nodeString == '*') && (*(nodeString+1) == 0))
        {
          *valid = 3;
          return (0);
        }
      }
    }
    return (-1);
  }

  do
  {
    if (*nodeString == '*')
    {
      if ((*(nodeString+1) == 0) && (*valid < 3))
      {
        return (0);
      }
      return (-1);
    }
    else
    {
      if (isdigit (*nodeString))
      {
        v = atoi (nodeString);
        while (isdigit(*++nodeString));

        switch (*nodeString)
        {
          case ':' :
            if (order++ > 0)
            {
              return (-1);
            }
            nodeNum->zone = v;
            *valid = 1;
            nodeString++;
            break;
          case '/' :
            if ((v == 0) || (order > 1) || (*valid == 0))
            {
              return (-1);
            }
            nodeNum->net = v;
            order = 2;
            *valid = 2;
            nodeString++;
            break;
          case  0  :
            if (*valid == 3)
            {
              nodeNum->point = v;
              *valid = 4;
              break;
            }
          case '.' :
            if (((*valid < 2) || (order != 0)) && (order != 2))
            {
              return (-1);
            }
            nodeNum->node = v;
            *valid = 3;
            if (*nodeString == 0)
            {
              return (0);
            }
            nodeString++;
            break;
          default  :
            return (-1);
        }
      }
      else
      {
        return (-1);
      }
    }
  }
  while (*valid < 4);

  if (*nodeString == 0)
  {
    return (0);
  }
  return (-1);
}



extern char configPath[128];

#define HDR_BUFSIZE 32

s16 export (int argc, char *argv[])
{
  s32             switches;
  tempStrType     tempStr;
  u16             dummy;
  rawEchoType     *areaPtr;
  msgHdrRec       *msgHdrBuf;
  msgTxtRec       msgTxtBuf;
  char            *text;
  char            *helpPtr,
  *helpPtr2;
  u16             textCount;
  fhandle         msgHdrHandle;
  fhandle         msgTxtHandle;
  fhandle         outHandle;
  u16             hdrBufCount;
  u16             totalHdrBuf;
  s16             board;

  if ( argc < 4 || (argv[2][0] == '?' || argv[2][1] == '?') )
  {
    printString ("Usage:\n\n"
                 "    FTools Export <area name> <file name> [/D] [/F] [/T] [/S] [/X] [/P] [/K]\n\n"
                 "    <area tag>   Name of the area to export\n"
                 "    <file name>  Name of file to export messages to\n\n"
                 "Switches:\n\n"
                 "    /D           Omit 'Date' line\n"
                 "    /F           Omit 'From' line\n"
                 "    /T           Omit 'To' line\n"
                 "    /S           Omit 'Subject' line\n"
                 "    /X           Omit message text\n"
                 "    /P           Exclude private messages\n"
                 "    /K           Show kludges\n");
    showCursor ();
    return(0);
  }

  switches = getSwitchFT (&argc, argv, SW_D|SW_F|SW_K|SW_P|SW_S|SW_T|SW_X);

  initLog ("EXPORT", switches);

  if ((board = getBoardNum(argv[2], argc-2, &dummy, &areaPtr)) == -1)
  {
    logEntry("This function does not yet support the JAM base", LOG_ALWAYS, 1000);
  }

  if ((text = malloc (32767)) == NULL)
  {
    logEntry ("Not enough memory available", LOG_ALWAYS, 2);
  }

  if ((msgHdrBuf = malloc (HDR_BUFSIZE*sizeof(msgHdrRec))) == NULL)
  {
    logEntry ("Not enough memory to allocate message base file buffers", LOG_ALWAYS, 2);
  }

  strcpy (tempStr, config.bbsPath);
  strcat (tempStr, "msghdr."MBEXTN);

  if ((msgHdrHandle = open(tempStr, O_RDONLY|O_BINARY|O_DENYNONE)) == -1)
  {
    logEntry ("Can't open message base files for input", LOG_ALWAYS, 1);
  }

  strcpy (tempStr, config.bbsPath);
  strcat (tempStr, "msgtxt."MBEXTN);

  if ((msgTxtHandle = open(tempStr, O_RDONLY|O_BINARY|O_DENYNONE)) == -1)
  {
    logEntry ("Can't open message base files for input", LOG_ALWAYS, 1);
  }

  if ((outHandle = open(argv[3], O_WRONLY|O_CREAT|O_TRUNC|O_TEXT,
                        S_IREAD|S_IWRITE)) == -1)
  {
    logEntry ("Can't open output file", LOG_ALWAYS, 1);
  }

  sprintf (tempStr, "Exporting messages in board %u to file %s", board, strupr(argv[3]));
  logEntry (tempStr, LOG_ALWAYS, 0);

  totalHdrBuf = 0;
  hdrBufCount = 0;

  while ((hdrBufCount < totalHdrBuf) || !eof(msgHdrHandle))
  {
    if (hdrBufCount == totalHdrBuf)
    {
      totalHdrBuf = read (msgHdrHandle, msgHdrBuf,
                          HDR_BUFSIZE*sizeof(msgHdrRec))/sizeof(msgHdrRec);
      hdrBufCount = 0;
    }
    if (hdrBufCount < totalHdrBuf)
    {
      if (((msgHdrBuf[hdrBufCount].MsgAttr & RA_DELETED) == 0) &&
          ((!(switches & SW_P)) || ((msgHdrBuf[hdrBufCount].MsgAttr & RA_PRIVATE) == 0)) &&
          (msgHdrBuf[hdrBufCount].Board == board))
      {
        write (outHandle, "===============================================================================\n",80);
        if (!(switches & SW_D))
        {
          write (outHandle, "Date : ", 7);
          write (outHandle, msgHdrBuf[hdrBufCount].PostDate,
                 msgHdrBuf[hdrBufCount].pdLength);
          write (outHandle, " ", 1);
          write (outHandle, msgHdrBuf[hdrBufCount].PostTime,
                 msgHdrBuf[hdrBufCount].ptLength);
          write (outHandle, "\n", 1);
        }
        if (!(switches & SW_F))
        {
          write (outHandle, "From : ", 7);
          write (outHandle, msgHdrBuf[hdrBufCount].WhoFrom,
                 msgHdrBuf[hdrBufCount].wfLength);
          write (outHandle, "\n", 1);
        }
        if (!(switches & SW_T))
        {
          write (outHandle, "To   : ", 7);
          write (outHandle, msgHdrBuf[hdrBufCount].WhoTo,
                 msgHdrBuf[hdrBufCount].wtLength);
          write (outHandle, "\n", 1);
        }
        if (!(switches & SW_S))
        {
          write (outHandle, "Subj : ", 7);
          write (outHandle, msgHdrBuf[hdrBufCount].Subj,
                 msgHdrBuf[hdrBufCount].sjLength);
          write (outHandle, "\n", 1);
        }

        if (!(switches & SW_X))
        {
          write (outHandle, "-------------------------------------------------------------------------------\n",80);

          memset (text, 0, 32767);
          textCount = 0;
          lseek (msgTxtHandle, msgHdrBuf[hdrBufCount].StartRec*(s32)sizeof(msgTxtRec), SEEK_SET);

          while ((msgHdrBuf[hdrBufCount].NumRecs--) && (textCount < 32000))
          {
            read (msgTxtHandle, &msgTxtBuf, sizeof(msgTxtRec));
            memcpy (text+textCount, &msgTxtBuf.txtStr, msgTxtBuf.txtLength);
            textCount += msgTxtBuf.txtLength;
          }

          helpPtr = text;
          while ((helpPtr = strchr(helpPtr,0x8d)) != NULL)
          {
            *helpPtr = '\r';
          }

          helpPtr = text;
          while ((helpPtr = strchr(helpPtr,'\r')) != NULL)
          {
            if (*(helpPtr+1) == '\n')
              strcpy (helpPtr, helpPtr+1);
            else
              *helpPtr = '\n';
          }

          helpPtr = text;
          while ((helpPtr = strchr(helpPtr,1)) != NULL)
          {
            if ((helpPtr == text) || (*(helpPtr-1) == '\n'))
            {
              if (switches & SW_K)
              {
                *helpPtr = '@';
              }
              else
              {
                if ((helpPtr2 = strchr (helpPtr, '\n')) == NULL)
                  *helpPtr = 0;
                else
                  strcpy (helpPtr, helpPtr2+1);
              }
            }
            else
              helpPtr++;
          }
          write (outHandle, text, strlen(text));
          write (outHandle, "\n\n", 2);
        }
      }
    }
    hdrBufCount++;
  }

  close (msgHdrHandle);
  close (msgTxtHandle);
  close (outHandle);

  showCursor ();
  return (0);
}


static s16 getNetAka(char *areaTag)
{
  u16      count;
  char     nameBuf[10];

  for (count = 0; count <= MAX_NETAKAS; count++)
  {
    if (count)
      sprintf(nameBuf, "#net%u", count);
    else
      strcpy(nameBuf, "#netm");
    if (stricmp(nameBuf, areaTag) == 0)
      return count;
  }
  return -1;
}


s16 getBoardNum (char *areaTag, s16 valid, u16 *aka, rawEchoType **areaPtr)
{
  static      rawEchoType areaRec;
  u16         count;
  s16         netaka;
  u16         board;
  headerType  *header;
  rawEchoType *areaBuf;

  if (valid < 1)
    goto errorhalt;

  board = 0;
  if ((netaka = getNetAka(areaTag)) != -1)
  {
    *aka = netaka;
    *areaPtr = memset(&areaRec, 0, RAWECHO_SIZE);
    return config.netmailBoard[netaka];
  }
  if (stricmp(areaTag, "#bad") == 0)
    board = config.badBoard;
  if (stricmp(areaTag, "#dup") == 0)
    board = config.dupBoard;
  if (stricmp(areaTag, "#rec") == 0)
    board = config.recBoard;
  if (board)
  {
    *aka = 0;
    *areaPtr = memset(&areaRec, 0, RAWECHO_SIZE);
    return board;
  }
  if (!openConfig (CFG_ECHOAREAS, &header, (void*)&areaBuf))
    logEntry ("Can't open areas file", LOG_ALWAYS, 4);
  for (count = 0; count < header->totalRecords; count++)
  {
    getRec(CFG_ECHOAREAS, count);
    if (stricmp(areaBuf->areaName, areaTag) == 0)
    {
      *aka = areaBuf->address;
      *areaPtr = memcpy(&areaRec, areaBuf, RAWECHO_SIZE);
      board = areaBuf->board;
      closeConfig (CFG_ECHOAREAS);
      if ( !board && !*(*areaPtr)->msgBasePath )
        logEntry("Pass-through area is not allowed", LOG_ALWAYS, 4);
      if ( *(*areaPtr)->msgBasePath )
        board = -1;
      return board;
    }
  }
  closeConfig (CFG_ECHOAREAS);
errorhalt:
  logEntry ("Bad or missing area tag", LOG_ALWAYS, 4);
  return 0;
}
