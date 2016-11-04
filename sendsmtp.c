//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2016  Wilfred van Velzen
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
#include <dir.h>
#include <dirent.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"
#include "msgmsg.h"
#include "config.h"
#include "nodeinfo.h"
#include "areainfo.h"
#include "utils.h"
#include "smtp.h"
#include "spec.h"
#include "stpcpy.h"

#if 0
void _RTLENTRY _EXPFUNC tzset2(void);
#endif

//---------------------------------------------------------------------------
static int make_send_msg(u16 xu, char *attachName, u16 msg_tfs, u16 msg_kfs)
{
  char        *fileName;
  int          handle;
  s32          xs;
  tempStrType  txtName;

  strcpy(message->fromUserName, config.sysopName);
  strcpy(message->toUserName, nodeInfo[xu]->sysopName);
  strcpy(message->subject, "Mail attach");
  strcpy(stpcpy(txtName, configPath), "email.txt");
  if ( (handle = open(txtName, O_RDONLY | O_TEXT)) != -1 )
  {
    if ( (xs = read(handle, message->text, TEXT_SIZE - 1)) != - 1)
      message->text[xs] = 0;
    else
      message->text[0] = 0;
    close(handle);
  }
  else
    message->text[0] = 0;
  if (!sendMessage(config.smtpServer, config.emailAddress, nodeInfo[xu]->email, message, attachName))
  {
    if (firstFile(attachName, &fileName))
    {
      do
      {
        if (msg_tfs && (handle = _sopen(fileName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, SH_DENYRW, S_IREAD | S_IWRITE)) != -1)
          close(handle);
        else
          if (msg_kfs)
            unlink(fileName);
      }
      while (nextFile(&fileName));
    }

    return 1;
  }
  return 0;
}
//---------------------------------------------------------------------------
static int sendsmtp_msg(void)
{
  u16            xu;
  DIR           *dir;
  struct dirent *ent;
  u16            msgNum;
  char          *helpPtr
              , *fnsPtr;
  u16            msg_killsent, msg_tfs, msg_kfs;
  tempStrType    attachName
               , fileNameStr
               , tempStr;

  puts("Scanning netmail directory...\n");

  // Check netmail dir

  fnsPtr = stpcpy(fileNameStr, config.netPath);

  if ((dir = opendir(config.netPath)) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
    {
      if (!match_spec("*.msg", ent->d_name))
        continue;

      msgNum = (u16)strtoul(ent->d_name, &helpPtr, 10);

      if (msgNum == 0 || *helpPtr != '.')
        continue;

      if (readMsg(message, msgNum))
        continue;

      if (  !(message->attribute & (LOCAL | IN_TRANSIT))
         || !(message->attribute & FILE_ATT)
         ||  (message->attribute & (HOLDPICKUP | SENT))
         )
        continue;

      strcpy(fnsPtr, ent->d_name);

      msg_killsent = (message->attribute & KILLSENT);
      msg_tfs = msg_kfs = 0;
      while (getKludge(message->text, "\1FLAGS ", tempStr, sizeof(tempStr)))
      {
        msg_tfs = (strstr(tempStr, "TFS") != NULL);
        msg_kfs = (strstr(tempStr, "KFS") != NULL);
      }

      for (xu = 0; xu < nodeCount; ++xu)
      {
        if (memcmp(&nodeInfo[xu]->node, &message->destNode, sizeof(nodeNumType)))
          continue;
        if (!*nodeInfo[xu]->email)
          break;
        if ((message->attribute & FILE_ATT))
          strcpy(attachName, message->subject);
        else
          *attachName = 0;
        if (make_send_msg(xu, attachName, msg_tfs, msg_kfs))
        {
          if (msg_killsent)
            unlink(fileNameStr);
          else
          {
            attribMsg(message->attribute|SENT, msgNum);
            moveMsg(fileNameStr, config.sentPath);
          }
        }
        break;
      }
    }
    closedir(dir);
  }
  return 1;
}
//---------------------------------------------------------------------------
static int sendsmtp_bink(void)
{
  int            tempHandle;
  char          *archiveDirPtr
              , *helpPtr;
  u16            msg_tfs
               , msg_kfs = 0;
  tempStrType    archiveStr
               , archiveFile
               , tempStr;
  u16            xu;
  DIR           *dir;
  struct dirent *ent;

  for (xu = 0; xu < nodeCount; ++xu)
  {
    if (!*nodeInfo[xu]->email)
      continue;

    archiveDirPtr = stpcpy(archiveStr, config.outPath);
    if (nodeInfo[xu]->node.zone != config.akaList[0].nodeNum.zone)
    {
      archiveDirPtr += sprintf(archiveDirPtr - 1, ".%03hx", nodeInfo[xu]->node.zone);
      mkdir(archiveStr);
      strcpy(archiveDirPtr - 1, "\\");
    }
    if (nodeInfo[xu]->node.point)
    {
      archiveDirPtr += sprintf(archiveDirPtr, "%04hx%04hx.pnt", nodeInfo[xu]->node.net, nodeInfo[xu]->node.node);
      mkdir(archiveStr);
      strcpy(archiveDirPtr++, "\\");
    }
    if (nodeInfo[xu]->node.point)
      sprintf(archiveFile, "%08hx.?lo", nodeInfo[xu]->node.point);
    else
      sprintf(archiveFile, "%04hx%04hx.?lo", nodeInfo[xu]->node.net, nodeInfo[xu]->node.node);

    if ((dir = opendir(archiveStr)) != NULL)
    {
      while ((ent = readdir(dir)) != NULL)
      {
        if (!match_spec(archiveFile, ent->d_name))
          continue;

        strcpy(archiveDirPtr, ent->d_name);
        if ((tempHandle = open(archiveStr, O_RDWR | O_CREAT | O_APPEND | O_BINARY, S_IREAD | S_IWRITE)) == -1)
          continue;

        memset(tempStr, 0, sizeof(tempStr) - 1);
        helpPtr = tempStr;
        while (read(tempHandle, helpPtr, (tempStr + sizeof(tempStr) - 1 - helpPtr)) > 0 )
        {
          helpPtr = tempStr;
          while (*helpPtr && (*helpPtr == '\n' || *helpPtr == '\r'))
            ++helpPtr;

          strcpy(tempStr, helpPtr);
          if ((helpPtr = strchr(tempStr, '\n')) != NULL)
          {
            while (helpPtr > tempStr && (*(helpPtr - 1) == '\n' || *(helpPtr - 1) == '\r'))
              --helpPtr;

            *helpPtr = 0;
            if ((msg_tfs = (*tempStr == '#')) != 0 || (msg_kfs = (*tempStr == '^')) != 0)
              strcpy(tempStr, tempStr + 1);

            make_send_msg(xu, tempStr, msg_tfs, msg_kfs);
          }
          helpPtr = strchr(tempStr, 0);
        }
        close(tempHandle);
        unlink(archiveStr);
      }
    }
    closedir(dir);
  }
  return 1;
}
//---------------------------------------------------------------------------
int sendmail_smtp(void)
{
  int ret;

  if (!config.inetOptions.smtpImm)
    return 1;
#if 0
  tzset2();
#endif
  if (config.mailer == 3 || config.mailer == 5)
    ret = sendsmtp_bink();
  else
    ret = sendsmtp_msg();

  closeConnection();
#if 0
  tzset();
#endif
  return ret;
}
//---------------------------------------------------------------------------
