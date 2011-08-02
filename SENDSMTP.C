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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dir.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

#include "fmail.h"
#include "output.h"
#include "msgmsg.h"
#include "config.h" 
#include "nodeinfo.h" 
#include "areainfo.h" 
#include "utils.h" 
#include "smtp.h"


void _RTLENTRY _EXPFUNC tzset2(void);


static int make_send_msg(u16 xu, u8 *attachName, u16 msg_tfs, u16 msg_kfs)
{
   u8           *fileName;
   int          handle;
   s32          xs;
   tempStrType  txtName;

   strcpy(message->fromUserName, config.sysopName);
   strcpy(message->toUserName, nodeInfo[xu]->sysopName);
   strcpy(message->subject, "Mail attach");
   strcpy(txtName, configPath);
   strcat(txtName, "EMAIL.TXT");
   ++no_msg;
   if ( (handle = openP(txtName, O_TEXT|O_RDONLY, S_IREAD|S_IWRITE)) != -1 )
   {  if ( (xs = read(handle, message->text, TEXT_SIZE - 1)) != - 1)
	 message->text[xs] = 0;
      else
	 message->text[0] = 0;
      close(handle);
   }
   else
      message->text[0] = 0;
   if ( !sendMessage(config.smtpServer, config.emailAddress,
		     nodeInfo[xu]->email, message, attachName) )
   {
     if ( firstFile(attachName, &fileName) )
     {  do
        {  if ( msg_tfs &&
                (handle = openP(fileName, O_WRONLY|O_CREAT|O_TRUNC|O_DENYALL, S_IREAD|S_IWRITE)) != -1 )
           {  close(handle);
           }
           else if ( msg_kfs )
           {  unlink(fileName);
           }
        }  while ( nextFile(&fileName) );
    }

      return 1;
   }
   return 0;
}


static int sendsmtp_msg(void)
{
   s16          doneMsg;
   u16          xu;
   struct ffblk ffblkMsg;
   u16          msgNum;
   char         *helpPtr, *fnsPtr;
   u16          msg_killsent, msg_tfs, msg_kfs;
   tempStrType  attachName;
   tempStrType  fileNameStrW, fileNameStr, tempStr;

   printString ("Scanning netmail directory...\n\n");

   /* Check netmail dir */

   fnsPtr = stpcpy (fileNameStr, config.netPath);

   strcpy (fileNameStrW, config.netPath);
   strcat (fileNameStrW, "*.MSG");

   doneMsg = findfirst (fileNameStrW, &ffblkMsg, FA_RDONLY|FA_HIDDEN|
						FA_SYSTEM|FA_DIREC);
   while ( (!doneMsg) && (!breakPressed) )
   {
      msgNum = (u16)strtoul(ffblkMsg.ff_name, &helpPtr, 10);

      if ( (msgNum == 0) || (*helpPtr != '.') || (ffblkMsg.ff_attrib & FA_SPEC) )
	 goto nextmsg;

      if ( readMsg(message, msgNum) )
	 goto nextmsg;

      if ( !(message->attribute & (LOCAL|IN_TRANSIT)) ||
	   !(message->attribute & FILE_ATT) ||
	   (message->attribute & (HOLDPICKUP|SENT)) )
	 goto nextmsg;

      strcpy(fnsPtr, ffblkMsg.ff_name);

      msg_killsent = (message->attribute & KILLSENT);
      msg_tfs = msg_kfs = 0;
      while ( getKludge(message->text, "\1FLAGS ", tempStr, sizeof(tempStr)) )
      {  msg_tfs = (strstr(tempStr, "TFS") != NULL);
	 msg_kfs = (strstr(tempStr, "KFS") != NULL);
      }
  
      for ( xu = 0; xu < nodeCount; ++xu )
      {  if ( memcmp(&nodeInfo[xu]->node, &message->destNode, sizeof(nodeNumType)) )
	    continue;
	 if ( !*nodeInfo[xu]->email )
	    break;
	 if ( (message->attribute & FILE_ATT) )
	    strcpy(attachName, message->subject);
	  else
	    *attachName = 0;
	 if ( make_send_msg(xu, attachName, msg_tfs, msg_kfs) )
	 {  if ( msg_killsent )
	       unlink(fileNameStr);
	    else
	    {  attribMsg(message->attribute|SENT, msgNum);
	       moveMsg(fileNameStr, config.sentPath);
	    }
	 }
	 break;
      }
nextmsg:
      doneMsg = findnext(&ffblkMsg);
   }
   return 1;
}


static int sendsmtp_bink(void)
{
   int          tempHandle;
   u8           *archivePtr, *helpPtr;
   u16          msg_tfs, msg_kfs;
   tempStrType  archiveStr, tempStr;
   u16          xu;
   struct ffblk ffblkArc;

   for ( xu = 0; xu < nodeCount; ++xu )
   {  if ( !*nodeInfo[xu]->email )
	 continue;
      archivePtr = stpcpy (archiveStr, config.outPath);
      if ( nodeInfo[xu]->node.zone != config.akaList[0].nodeNum.zone )
      {
	 archivePtr += sprintf(archivePtr-1, ".%03hX", nodeInfo[xu]->node.zone);
	 mkdir(archiveStr);
	 strcpy (archivePtr-1, "\\");
      }
      if ( nodeInfo[xu]->node.point )
      {
	 archivePtr += sprintf (archivePtr, "%04hX%04hX.PNT", nodeInfo[xu]->node.net, nodeInfo[xu]->node.node);
	 mkdir (archiveStr);
	 strcpy (archivePtr++, "\\");
      }
      if ( nodeInfo[xu]->node.point )
	 archivePtr += sprintf (archivePtr, "%08hX", nodeInfo[xu]->node.point);
      else
	 archivePtr += sprintf (archivePtr, "%04hX%04hX", nodeInfo[xu]->node.net, nodeInfo[xu]->node.node);
      strcpy(archivePtr, ".?LO");
      if ( findfirst(archiveStr, &ffblkArc, 0) )
	 continue;
      do
      {  strcpy(archivePtr, strchr(ffblkArc.ff_name, '.'));
	 if ((tempHandle = openP(archiveStr,O_RDWR|O_CREAT|O_APPEND|O_BINARY|O_DENYNONE,S_IREAD|S_IWRITE)) == -1)
	    continue;
	 memset(tempStr, 0, sizeof(tempStr)-1);
	 helpPtr = tempStr;
	 while ( read(tempHandle, helpPtr, (tempStr+sizeof(tempStr)-1-helpPtr)) > 0 )
	 {  helpPtr = tempStr;
	    while ( *helpPtr && (*helpPtr == '\n' || *helpPtr == '\r') )
	      ++helpPtr;
	    strcpy(tempStr, helpPtr);
	    if ( (helpPtr = strchr(tempStr, '\n')) == NULL )
	       goto next;
	    while ( helpPtr > tempStr && (*(helpPtr-1) == '\n' || *(helpPtr-1) == '\r') )
	       --helpPtr;
	    *helpPtr = 0;
	    if ( (msg_tfs = (*tempStr == '#')) != 0 || (msg_kfs = (*tempStr == '^')) != 0 )
	       strcpy(tempStr, tempStr+1);
	    make_send_msg(xu, tempStr, msg_tfs, msg_kfs);
next:	    helpPtr = strchr(tempStr, 0);
	 }
	 close(tempHandle);
	 unlink(archiveStr);
      }
      while ( !findnext(&ffblkArc) );
   }
   return 1;
}


int sendmail_smtp(void)
{
   int ret;

   if ( !config.inetOptions.smtpImm )
      return 1;
   tzset2();
   if ( config.mailer == 3 || config.mailer == 5 )
      ret = sendsmtp_bink();
   else
      ret = sendsmtp_msg();
   closeConnection();
   tzset();
   return ret;
 }

