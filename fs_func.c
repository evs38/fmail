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


#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmail.h"

#include "fs_func.h"

#include "amgr_utl.h"
#include "areainfo.h"
#include "cfgfile.h"
#include "window.h"

//---------------------------------------------------------------------------
extern areaInfoPtrArr areaInfo;
extern u16            areaInfoCount;
extern u16            areaInfoBoard;


extern configType     config;
extern char configPath[FILENAME_MAX];
extern windowLookType windowLook;

extern rawEchoType tempInfo, updInfo;

static rawEchoType *areaBuf2;

funcParType groupSelect;

//---------------------------------------------------------------------------
s16 displayGroups(void)
{
  u16 c;

  if (displayWindow(" Groups ", 15, 7, 76, 21) != 0)
    return 0;

  for (c = 0; c < 13; c++)
  {
    printChar('A' + c, 17, c + 8, windowLook.promptfg, windowLook.background, MONO_NORM);
    printString(config.groupDescr[c], 19, c + 8, windowLook.datafg, windowLook.background, MONO_NORM);
    printChar('N' + c, 47, c + 8, windowLook.promptfg, windowLook.background, MONO_NORM);
    printString(config.groupDescr[c + 13], 49, c + 8, windowLook.datafg, windowLook.background, MONO_NORM);
  }
  return 1;
}
//---------------------------------------------------------------------------
s16 askGroup(void)
{
   u16 ch;

   if (displayGroups() == 0) return 0;

   do
   {
      ch = readKbd ();
   }
   while ((ch & 0xff00) || ((ch != _K_ESC_) && (ch >= 256 || !isalpha(ch))));

	if (ch != _K_ESC_)
   {
      *(s32*)groupSelect.numPtr = ((s32)1)<<(toupper(ch)-'A');
   }

   removeWindow ();
   return (ch != _K_ESC_);
}



s32 getGroups (s32 groups)
{
   u16 c;
   s32 mask;

   if (displayWindow (" Groups ", 15, 7, 76, 21) != 0)
   {
      return (0);
   }

   do
   {
      mask = 1;
      for (c = 0; c < 13; c++)
      {
         if (groups & mask)
         {
            printChar('A'+c, 17, c+8, WHITE, windowLook.background, MONO_HIGH);
            printString(config.groupDescr[c], 19, c+8, WHITE, windowLook.background, MONO_HIGH);
         }
         else
         {
            printChar('A'+c, 17, c+8, windowLook.inactvborderfg, windowLook.background, MONO_NORM);
            printString(config.groupDescr[c], 19, c+8, DARKGRAY, windowLook.background, MONO_NORM);
         }
         if (groups & (mask<<13))
         {
            printChar('N'+c, 47, c+8, WHITE, windowLook.background, MONO_HIGH);
            printString(config.groupDescr[c+13], 49, c+8, WHITE, windowLook.background, MONO_HIGH);
         }
         else
         {
            printChar('N'+c, 47, c+8, windowLook.inactvborderfg, windowLook.background, MONO_NORM);
            printString(config.groupDescr[c+13], 49, c+8, DARKGRAY, windowLook.background, MONO_NORM);
         }
         mask <<= 1;
      }

      c = readKbd ();
      if (c < 256 && isalpha(c))
      {
         groups ^= (s32)1 << (toupper(c) - 'A');
      }
      if (c == '+' || c == _K_INS_)
      {
         groups = 0x03ffffffL;
      }
      if (c == '-' || c == _K_DEL_)
      {
         groups = 0;
      }
   }
   while ((c != _K_ESC_) && (c != _K_ENTER_));

   removeWindow ();
   return (c != _K_ESC_ ? groups : -1L);
}

extern funcParType groupsSelect;

s16 askGroups(void)
{
    s32 groups;

    groups = getGroups(*(s32*)groupsSelect.numPtr);
    if (groups == -1L)
       return 0;
    *(s32*)groupsSelect.numPtr = groups;
    return 1;
}



s16 netmailMenu(u16 v)
{
   s16 update;

   memset(&tempInfo, 0, RAWECHO_SIZE);

   if (v == 0)
      strcpy(tempInfo.areaName, "Netmail Main");
   else
      sprintf(tempInfo.areaName, "Netmail AKA %u", v);

   tempInfo.board               = config.netmailBoard[v];
   tempInfo.groupRA             = config.groupRA[v];
   tempInfo.altGroupRA[0]       = config.altGroupRA[v][0];
   tempInfo.altGroupRA[1]       = config.altGroupRA[v][1];
   tempInfo.altGroupRA[2]       = config.altGroupRA[v][2];
   tempInfo.minAgeSBBS          = config.minAgeSBBS[v];
   tempInfo.daysRcvd            = config.daysRcvdAKA[v];
   tempInfo.replyStatSBBS       = config.replyStatSBBS[v];
   tempInfo.attrSBBS            = config.attrSBBS[v];
   tempInfo.msgKindsRA          = config.msgKindsRA[v];
   tempInfo.attrRA              = config.attrRA[v];
   tempInfo.attr2RA             = config.attr2RA[v];
   tempInfo.aliasesQBBS         = config.aliasesQBBS[v];
   tempInfo.readSecRA           = config.readSecRA[v];
   *(s32*)&tempInfo.flagsRdRA   = *(s32*)&config.readFlagsRA[v];
   *(s32*)&tempInfo.flagsRdNotRA = *((s32*)(&config.readFlagsRA[v])+1);
   tempInfo.writeSecRA          = config.writeSecRA[v];
   *(s32*)&tempInfo.flagsWrRA   = *(s32*)&config.writeFlagsRA[v];
   *(s32*)&tempInfo.flagsWrNotRA = *((s32*)(&config.writeFlagsRA[v])+1);
   tempInfo.sysopSecRA          = config.sysopSecRA[v];
   *(s32*)&tempInfo.flagsSysRA  = *(s32*)&config.sysopFlagsRA[v];
   *(s32*)&tempInfo.flagsSysNotRA = *((s32*)(&config.sysopFlagsRA[v])+1);
   tempInfo.templateSecQBBS     = config.templateSecQBBS[v];
   *(s32*)&tempInfo.flagsTemplateQBBS = *(s32*)&config.templateFlagsQBBS[v];
   tempInfo.groupsQBBS          = config.groupsQBBS[v];
   tempInfo.days                = config.daysAKA[v];
   tempInfo.msgs                = config.msgsAKA[v];
   tempInfo.options             = config.optionsAKA[v];

   memcpy(tempInfo.qwkName, config.qwkName[v], 13);
   memcpy(tempInfo.comment, config.descrAKA[v], 51);

   update = editAM(EDIT_NETMAIL, 0, NULL);

   config.netmailBoard[v]          = tempInfo.board;
   config.groupRA[v]               = tempInfo.groupRA;
   config.altGroupRA[v][0]         = tempInfo.altGroupRA[0];
   config.altGroupRA[v][1]         = tempInfo.altGroupRA[1];
   config.altGroupRA[v][2]         = tempInfo.altGroupRA[2];
   config.minAgeSBBS[v]            = tempInfo.minAgeSBBS;
   config.daysRcvdAKA[v]           = tempInfo.daysRcvd;
   config.replyStatSBBS[v]         = tempInfo.replyStatSBBS;
   config.attrSBBS[v]              = tempInfo.attrSBBS;
   config.msgKindsRA[v]            = tempInfo.msgKindsRA;
   config.attrRA[v]                = tempInfo.attrRA;
   config.attr2RA[v]               = tempInfo.attr2RA;
   config.aliasesQBBS[v]           = tempInfo.aliasesQBBS;
   config.readSecRA[v]             = tempInfo.readSecRA;
   *(s32*)&config.readFlagsRA[v]  = *(s32*)&tempInfo.flagsRdRA;
   *((s32*)(&config.readFlagsRA[v])+1) = *(s32*)&tempInfo.flagsRdNotRA;
   config.writeSecRA[v]            = tempInfo.writeSecRA;
   *(s32*)&config.writeFlagsRA[v] = *(s32*)&tempInfo.flagsWrRA;
   *((s32*)(&config.writeFlagsRA[v])+1) = *(s32*)&tempInfo.flagsWrNotRA;
   config.sysopSecRA[v]            = tempInfo.sysopSecRA;
   *(s32*)&config.sysopFlagsRA[v] = *(s32*)&tempInfo.flagsSysRA;
   *((s32*)(&config.sysopFlagsRA[v])+1) = *(s32*)&tempInfo.flagsSysNotRA;
   config.templateSecQBBS[v]       = tempInfo.templateSecQBBS;
   *(s32*)&config.templateFlagsQBBS[v] = *(s32*)&tempInfo.flagsTemplateQBBS;
   config.groupsQBBS[v]            = tempInfo.groupsQBBS;
   config.daysAKA[v]               = tempInfo.days;
   config.msgsAKA[v]               = tempInfo.msgs;
   config.optionsAKA[v]            = tempInfo.options;

   memcpy(config.qwkName[v],  tempInfo.qwkName, 13);
   memcpy(config.descrAKA[v], tempInfo.comment, 51);

   return update;
}



extern rawEchoType echoDefaultsRec;

s16 defaultBBS (void)
{
   s16 update;

   memcpy(&tempInfo, &echoDefaultsRec, RAWECHO_SIZE);

   update = editAM(EDIT_ECHO_DEFAULT, 0, NULL);

   memcpy(&echoDefaultsRec, &tempInfo, RAWECHO_SIZE);

   return update;
}



uplinkNodeStrType uplinkNodeStr;

s16 uplinkMenu (u16 v)
{
  menuType *uplMenu;
  s16       update;
  u16       count;
  char      title[20];
  char      addressText[MAX_AKAS+1][34];

   toggleType fileTypeToggle;
   toggleType addressToggle;

   fileTypeToggle.data      = &config.uplinkReq[v].fileType;
   fileTypeToggle.text  [0] = "Random";
   fileTypeToggle.retval[0] = 0;
   fileTypeToggle.text  [1] = "<AREANAME> <DESCRIPTION>";
   fileTypeToggle.retval[1] = 1;
   fileTypeToggle.text  [2] = "BCL";
   fileTypeToggle.retval[2] = 2;

   addressToggle.data = &config.uplinkReq[v].originAka;
   for (count = 0; count < MAX_AKAS; count++)
   {
      if (config.akaList[count].nodeNum.zone != 0)
         strcpy(addressText[count], nodeStr((nodeNumType*)(&config.akaList[count])));
      else
         if (count == 0)
            strcpy(addressText[0], "Main (not defined)");
         else
            sprintf(addressText[count], "AKA %2u (not defined)", count);
      addressToggle.text[count] = addressText[count];
      addressToggle.retval[count] = count;
   }

   groupsSelect.numPtr = (u16*)&config.uplinkReq[v].groups;
   groupsSelect.f      = askGroups;

   sprintf (title, " Uplink %u ", v+1);
   if ((uplMenu = createMenu (title)) == NULL)
   {
      return(0);
   }
   addItem (uplMenu, NODE, "Uplink system", 0, &config.uplinkReq[v].node, 0, 0,
		     "Node number of the uplink");
   addItem (uplMenu, WORD, "AreaFix program", 0, config.uplinkReq[v].program, 8, 0,
		     "Name of the program to which to send the request");
   addItem (uplMenu, WORD, "AreaFix password", 0, config.uplinkReq[v].password, 16, 0,
		     "Password to put on the subject line");
   addItem (uplMenu, BOOL_INT, "Add '+' prefix", 0, &config.uplinkReq[v].options, BIT0, 0,
		     "If the AreaFix program requires a '+' prefix to connect an area");
   addItem (uplMenu, FUNC_PAR, "Authorized groups", 0, &groupsSelect, 26, 0,
                     "The groups a requesting node must have access to");
   addItem (uplMenu, BOOL_INT, "Unconditional", 0, &config.uplinkReq[v].options, BIT4, 0,
                     "Forward all requests to this uplink if the node has access to a group");
   addItem (uplMenu, ENUM_INT, "Areas file type", 0, &fileTypeToggle, 0, 3,
                     "Type of areas file");
   addItem (uplMenu, SFILE_NAME|UPCASE, "Areas file name", 0, &config.uplinkReq[v].fileName, 12, 0,
                     "File with list of areas available from this uplink");
   addItem (uplMenu, ENUM_INT, "Origin address", 0, &addressToggle, 0, MAX_AKAS,
		     "The AKA that should be used as origin address for the uplink request messages");

   update = runMenu (uplMenu, 7, 8);

   free (uplMenu);

   count = sprintf(uplinkNodeStr[v], "%2u  %s", v + 1, config.uplinkReq[v].node.zone ? nodeStr(&config.uplinkReq[v].node) : "                       ");
   memset (uplinkNodeStr[v]+count, ' ', 27-count);

   uplinkNodeStr[v][27] = 0;

   return update;
}


extern char displayAreasArray[MBBOARDS];
extern s16  displayAreasSelect;

extern char boardCodeInfo[512];

s16 netDisplayAreas (u16 *v)
{
   u16         count;
   s16         temp;
   headerType  *areaHeader;
   rawEchoType *areaBuf;

   memset (displayAreasArray, 0, MBBOARDS);

   if (*v > MBBOARDS) *v = 0;

   if (openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf))
   {
      working ();
      for (count=0; count<areaHeader->totalRecords; count++)
      {
	 getRec(CFG_ECHOAREAS, count);
         if ( areaBuf->board && areaBuf->board <= MBBOARDS )
            displayAreasArray[areaBuf->board-1]++;
         if ( areaBuf->boardNumRA && areaBuf->boardNumRA <= MBBOARDS )
            displayAreasArray[areaBuf->boardNumRA-1]++;
      }
      closeConfig(CFG_ECHOAREAS);
   }

   if ( config.mbOptions.checkNetBoard )
      for (count = 0; count < MAX_NETAKAS; count++)
      {
         if ( config.netmailBoard[count] && config.netmailBoard[count] <= MBBOARDS )
            displayAreasArray[config.netmailBoard[count]-1]++;
         if (*v)
            boardCodeInfo[(*v-1)>>3] &= ~(1<<((*v-1)&7));
      }
   if ( config.badBoard && config.badBoard <= MBBOARDS )
      displayAreasArray[config.badBoard-1]++;
   if ( config.dupBoard && config.dupBoard <= MBBOARDS )
      displayAreasArray[config.dupBoard-1]++;
   if ( config.recBoard && config.recBoard <= MBBOARDS )
      displayAreasArray[config.recBoard-1]++;

   displayAreasSelect = *v;
   temp = displayAreas ();
   *v = displayAreasSelect;
   return (temp);
}



s16 badduprecDisplayAreas (u16 *v)
{
   s16         temp;
   u16         count;
   headerType  *areaHeader;
   rawEchoType *areaBuf;

   memset (displayAreasArray, 0, MBBOARDS);

   if (*v > MBBOARDS) *v = 0;

   if (openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf))
   {
      working ();
      for (count=0; count<areaHeader->totalRecords; count++)
      {
         getRec(CFG_ECHOAREAS, count);
         if ( areaBuf->board && areaBuf->board <= MBBOARDS )
	    displayAreasArray[areaBuf->board-1]++;
         if ( areaBuf->boardNumRA && areaBuf->boardNumRA <= MBBOARDS )
	    displayAreasArray[areaBuf->boardNumRA-1]++;
      }
      closeConfig(CFG_ECHOAREAS);
   }

   for (count = 0; count < MAX_NETAKAS; count++)
   {
      if ( config.netmailBoard[count] && config.netmailBoard[count] <= MBBOARDS )
         displayAreasArray[config.netmailBoard[count]-1]++;
   }
   if ( config.badBoard && config.badBoard <= MBBOARDS && (*v != config.badBoard) )
      displayAreasArray[config.badBoard-1]++;

   if ( config.dupBoard && config.dupBoard <= MBBOARDS && (*v != config.dupBoard) )
      displayAreasArray[config.dupBoard-1]++;

   if ( config.recBoard && config.recBoard <= MBBOARDS && (*v != config.recBoard) )
      displayAreasArray[config.recBoard-1]++;

   displayAreasSelect = *v;
   temp = displayAreas ();
   *v = displayAreasSelect;
   return (temp);
}



s16 echoOnly(s16 editType, s16 code)
{
   if ((editType == EDIT_ECHO)   || (editType == DISPLAY_ECHO_DATA) ||
       (editType == EDIT_ECHO_GLOBAL) || (editType == DISPLAY_ECHO_WINDOW))
      return code;
   else
      return DISPLAY;
}


s16 echoNGOnly (s16 editType, s16 code)
{
   if ((editType == EDIT_ECHO) || (editType == DISPLAY_ECHO_DATA) ||
                                  (editType == DISPLAY_ECHO_WINDOW))
      return (code);
   else
      return (DISPLAY);
}
//---------------------------------------------------------------------------
s16 echoDefOnly(s16 editType, s16 code)
{
  if (  editType == EDIT_ECHO
     || editType == DISPLAY_ECHO_DATA
     || editType == EDIT_ECHO_GLOBAL
     || editType == EDIT_ECHO_DEFAULT
     || editType == DISPLAY_ECHO_WINDOW
     )
    return code;
  else
    return DISPLAY;
}
//---------------------------------------------------------------------------
s16 nonGlobOnly (s16 editType, s16 code)
{
   if ( editType != EDIT_ECHO_GLOBAL )
      return (code);
   else
      return (DISPLAY);
}


s16 echoDefNGOnly (s16 editType, s16 code)
{
   if ((editType == EDIT_ECHO) || (editType == DISPLAY_ECHO_DATA) ||
       (editType == EDIT_ECHO_DEFAULT) || (editType == DISPLAY_ECHO_WINDOW))
      return (code);
   else
      return (DISPLAY);
}



funcParType mgrBoardSelect;
funcParType boardCodeSelect;
funcParType boardCodeSelectGold;

s16 echoDisplayAreas (u16 *v)
{
   u16   count;
   s16   temp;

   memset (displayAreasArray, 0, MBBOARDS);

   if (*v > MBBOARDS) *v = 0;

   for (count = 0; count < MBBOARDS; count++)
   {
      if ( (count != areaInfoBoard-1) &&
           (boardCodeInfo[count>>3] & (1<<(count&7))) )
      {
         displayAreasArray[count]++;
      }
   }
   for (count = 0; count < MAX_NETAKAS; count++)
   {
      if ( config.netmailBoard[count] && config.netmailBoard[count] <= MBBOARDS )
         displayAreasArray[config.netmailBoard[count]-1]++;
   }
   if ( config.badBoard && config.badBoard <= MBBOARDS )
      displayAreasArray[config.badBoard-1]++;
   if ( config.dupBoard && config.dupBoard <= MBBOARDS )
      displayAreasArray[config.dupBoard-1]++;
   if ( config.recBoard && config.recBoard <= MBBOARDS )
      displayAreasArray[config.recBoard-1]++;

   if (*v)
      boardCodeInfo[(*v-1)>>3] &= ~(1<<((*v-1)&7));

   displayAreasSelect = *mgrBoardSelect.numPtr;
   temp = displayAreas ();
   *v = displayAreasSelect;

   if (*v)
      boardCodeInfo[(*v-1)>>3] |= (1<<((*v-1)&7));

   if ((displayAreasSelect != 0) ||
       ((displayAreasSelect == 0) &&
        (tempInfo.boardNumRA <= MBBOARDS)) ||
       ((displayAreasSelect == 0) &&
        (*tempInfo.msgBasePath == 0)))
   {
      tempInfo.boardNumRA = 0;
   }
   return (temp);
}


s16 selectBoardCode (u16 *v)
{
   s16 index = *v;
   s16 ixc;
   s16 count;
   u16 ch;
   char tempStr[8];

   if (tempInfo.board)
   {
      displayMessage("Board codes can only be used for non-"MBNAME" areas");
      *v = 0;
      return 0;
   }

   if (!*tempInfo.msgBasePath)
   {
      displayMessage("Board codes cannot be used for pass-through areas");
      *v = 0;
      return 0;
   }

   if (index)
      boardCodeInfo[(index-1)>>3] &= ~(1<<((index-1)&7));

   displayWindow("Board", 37, 11, 44, 19);
   do
   {
      if (index)
         sprintf(tempStr,"%4u", index);
      else
         strcpy(tempStr, "None");
      printString(tempStr, 39, 15, windowLook.datafg, windowLook.background, MONO_INV);
      ixc = index;
      for(count = 0; count <= 2; count++)
      {
         do
         {
            ixc++;
         }
         while ((ixc <= 4096) && (boardCodeInfo[(ixc-1)>>3] & (1<<((ixc-1)&7))));
         if (ixc > 0 && ixc <= 4096)
            sprintf(tempStr, "%4u", ixc);
         else
            sprintf(tempStr, "    ");
         printString(tempStr, 39, 16+count, windowLook.promptfg, windowLook.background, MONO_NORM);
      }
      ixc = index;
      for(count = 0; count <= 2; count++)
      {
         do
         {
            ixc--;
         }
         while ((ixc > 0) && (boardCodeInfo[(ixc-1)>>3] & (1<<((ixc-1)&7))));
         if (ixc > 0 && ixc <= 4096)
            sprintf(tempStr, "%4u", ixc);
         else
            if (ixc < 0)
               strcpy(tempStr, "    ");
            else
               strcpy(tempStr, "None");
         printString(tempStr, 39, 14-count, windowLook.promptfg, windowLook.background, MONO_NORM);
      }
      ch = readKbd();

      count = 0;
      if (ch == _K_PGUP_)
      {
         count = 6;
         ch = _K_UP_;
      }
      if (ch == _K_PGDN_)
      {
         count = 6;
         ch = _K_DOWN_;
      }
      do
      {
         if (ch == _K_END_ || ch == _K_CPGDN_)
         {
            index = 4097;
            ch = _K_UP_;
         }
         if (ch == _K_HOME_ || ch == _K_CPGUP_)
            index = 0;
         if (ch == _K_UP_ && (index > 0))
            do
            {
               index--;
            }
            while ((index > 0) && (boardCodeInfo[(index-1)>>3] & (1<<((index-1)&7))));
         if (ch == _K_DOWN_ && index < 4096)
            do
            {
               index++;
            }
            while ((index <= 4096) && (boardCodeInfo[(index-1)>>3] & (1<<((index-1)&7))));
      }
      while (count--);

      if (ch >= '0' && ch <= '9')
      {
          tempStr[0] = ch;
          tempStr[1] = 0;
          displayWindow("", 35, 12, 54, 14);
          printString("Board code", 37, 13, windowLook.promptfg, windowLook.background, MONO_NORM);
          if (editString(tempStr, 5, 49, 13, NUM_INT|NOCLEAR) != _K_ESC_)
          {
             ixc = index = atoi(tempStr);
             while ((index <= 4096) && (boardCodeInfo[(index-1)>>3] & (1<<((index-1)&7))))
                index++;
             if (index > 4096) index = 0;
             if (index == ixc) ch = _K_ENTER_;
          }
          removeWindow();
      }
   }
   while (ch != _K_ESC_ && ch != _K_ENTER_);
   if (ch == _K_ENTER_)
      *v = index;
   removeWindow();
   if (*v)
      boardCodeInfo[(*v-1)>>3] |= (1<<((*v-1)&7));
   return (ch == _K_ENTER_);
}


#ifdef GOLDBASE
s16 selectBoardCodeGold(u16 *v)
{
   s16 index = *v;
   s16 ixc;
   s16 count;
   u16      ch;
   char tempStr[8];

   if (index)
      boardCodeInfo[(index-1)>>3] &= ~(1<<((index-1)&7));

   displayWindow("Board", 17, 6, 24, 14);

   do
   {
      if (index)
         sprintf(tempStr,"%4u", index);
      else
         strcpy(tempStr, "None");
      printString(tempStr, 19, 10, windowLook.datafg, windowLook.background, MONO_INV);
      ixc = index;
      for(count = 0; count <= 2; count++)
      {
         do
         {
            ixc++;
         }
         while ((ixc <= MBBOARDS) && (boardCodeInfo[(ixc-1)>>3] & (1<<((ixc-1)&7))));
         if (ixc > 0 && ixc <= MBBOARDS)
            sprintf(tempStr, "%4u", ixc);
         else
            sprintf(tempStr, "    ");
         printString(tempStr, 19, 11+count, windowLook.promptfg, windowLook.background, MONO_NORM);
      }
      ixc = index;
      for(count = 0; count <= 2; count++)
      {
         do
         {
            ixc--;
         }
         while ((ixc > 0) && (boardCodeInfo[(ixc-1)>>3] & (1<<((ixc-1)&7))));
         if (ixc > 0 && ixc <= MBBOARDS)
            sprintf(tempStr, "%4u", ixc);
         else
            if (ixc < 0)
               strcpy(tempStr, "    ");
            else
               strcpy(tempStr, "None");
         printString(tempStr, 19, 9-count, windowLook.promptfg, windowLook.background, MONO_NORM);
      }
      ch = readKbd();

      count = 0;
      if (ch == _K_PGUP_)
      {
         count = 6;
         ch = _K_UP_;
      }
      if (ch == _K_PGDN_)
      {
         count = 6;
         ch = _K_DOWN_;
      }
      do
      {
         if (ch == _K_END_ || ch == _K_CPGDN_)
         {
            index = MBBOARDS+1;
            ch = _K_UP_;
         }
         if (ch == _K_HOME_ || ch == _K_CPGUP_)
            index = 0;
         if (ch == _K_UP_ && (index > 0))
            do
            {
               index--;
            }
            while ((index > 0) && (boardCodeInfo[(index-1)>>3] & (1<<((index-1)&7))));
         if (ch == _K_DOWN_ && index < MBBOARDS)
            do
            {
               index++;
            }
            while ((index <= MBBOARDS) && (boardCodeInfo[(index-1)>>3] & (1<<((index-1)&7))));
      }
      while (count--);

      if (ch >= '0' && ch <= '9')
      {
          tempStr[0] = ch;
          tempStr[1] = 0;
          displayWindow("", 15, 7, 34, 9);
          printString("Board code", 17, 8, windowLook.promptfg, windowLook.background, MONO_NORM);
          if (editString(tempStr, 5, 29, 8, NUM_INT|NOCLEAR) != _K_ESC_)
          {
             ixc = index = atoi(tempStr);
             while ((index <= MBBOARDS) && (boardCodeInfo[(index-1)>>3] & (1<<((index-1)&7))))
                index++;
             if (index > MBBOARDS)
                index = 0;
             if (index == ixc)
                ch = _K_ENTER_;
          }
          removeWindow();
      }
   }
   while (ch != _K_ESC_ && ch != _K_ENTER_);
   if (ch == _K_ENTER_)
   {  *v = index;
      *tempInfo.msgBasePath = 0;
   }
   removeWindow();
   if (*v)
      boardCodeInfo[(*v-1)>>3] |= (1<<((*v-1)&7));
   return (ch == _K_ENTER_);
}
#endif




static s16 nodeIn(nodeNumType *nodeNum)
{
   u16 c, c2;

   c = 0;
   while ((c < MAX_FORWARD) &&
          (areaBuf2->forwards[c].nodeNum.zone != 0) &&
          ((nodeNum->zone   > areaBuf2->forwards[c].nodeNum.zone) ||
           ((nodeNum->zone == areaBuf2->forwards[c].nodeNum.zone) &&
            (nodeNum->net   > areaBuf2->forwards[c].nodeNum.net)) ||
           ((nodeNum->zone == areaBuf2->forwards[c].nodeNum.zone) &&
            (nodeNum->net  == areaBuf2->forwards[c].nodeNum.net) &&
            (nodeNum->node  > areaBuf2->forwards[c].nodeNum.node)) ||
           ((nodeNum->zone == areaBuf2->forwards[c].nodeNum.zone) &&
            (nodeNum->net  == areaBuf2->forwards[c].nodeNum.net) &&
            (nodeNum->node == areaBuf2->forwards[c].nodeNum.node) &&
            (nodeNum->point > areaBuf2->forwards[c].nodeNum.point))))
   {
      c++;
   }
   c2 = 0;
   while ( c2 < MAX_FORWARD && areaBuf2->forwards[c2].nodeNum.zone )
      c2++;
   if ( c < MAX_FORWARD && c2 < MAX_FORWARD &&
       (memcmp (&areaBuf2->forwards[c].nodeNum, nodeNum, sizeof(nodeNumType)) != 0))
   {
      memmove (&areaBuf2->forwards[c+1],
               &areaBuf2->forwards[c],
               (MAX_FORWARD-1-c) * sizeof(nodeNumXType));
      areaBuf2->forwards[c].nodeNum = *nodeNum;
		return (1);
   }
   return (0);
}



static s16 nodeOut(nodeNumType *nodeNum)
{
   u16 c;

   c = 0;
   while ((c < MAX_FORWARD) &&
          (areaBuf2->forwards[c].nodeNum.zone != 0) &&
          (memcmp (&areaBuf2->forwards[c].nodeNum, nodeNum,
						 sizeof(nodeNumType)) != 0))
   {
      c++;
   }
   if ((c < MAX_FORWARD) &&
       (memcmp (&areaBuf2->forwards[c].nodeNum, nodeNum,
                sizeof(nodeNumType)) == 0))
   {
      memcpy (&areaBuf2->forwards[c],
              &areaBuf2->forwards[c+1],
              (MAX_FORWARD-1-c) * sizeof(nodeNumXType));
      memset (&areaBuf2->forwards[MAX_FORWARD-1], 0,
              sizeof(nodeNumXType));
      return (1);
   }
   return (0);
}



static s16 insNode(void)
{
   s16         updated = 0;
   s16         total = 0;
   u16    count;
   s32         groupCode;
   nodeNumType tempNode;

   tempNode = getNodeNum (" Insert Node ", 30, 15, 0);
   if ((tempNode.zone != 0) &&
       ((groupCode = getGroups(0)) != -1))
   {
      for (count = 0; count < areaInfoCount; count++)
      {  getRec(CFG_ECHOAREAS, count);
         if (areaBuf2->group & groupCode)
         {  updated += nodeIn(&tempNode);
            putRec(CFG_ECHOAREAS, count);
            total++;
         }
      }
      processed (updated, total);
   }
   return (updated);
}



static s16 delNode(void)
{
   s16         updated = 0;
   s16         total = 0;
   u16         count;
   s32         groupCode;
   nodeNumType tempNode;

   tempNode = getNodeNum (" Delete Node ", 30, 15, 0);
   if ((tempNode.zone != 0) &&
       ((groupCode = getGroups(0)) != -1))
   {
      for (count = 0; count < areaInfoCount; count++)
      {  getRec(CFG_ECHOAREAS, count);
         if (areaBuf2->group & groupCode)
         {  updated += nodeOut (&tempNode);
            putRec(CFG_ECHOAREAS, count);
            total++;
         }
      }
      processed (updated, total);
   }
   return (updated);
}



static s16 replaceNode(void)
{
   s16         updated = 0;
   s16         total = 0;
   u16         count;
   s32         groupCode;
   nodeNumType oldNode,
               newNode;

   oldNode = getNodeNum (" Old Node ", 30, 15, 0);
   if (oldNode.zone != 0)
   {  newNode = getNodeNum (" New Node ", 30, 15, 0);
      if ((newNode.zone != 0) &&
          ((groupCode = getGroups(0)) != -1))
      {
         for (count = 0; count < areaInfoCount; count++)
         {  getRec(CFG_ECHOAREAS, count);
            if (areaBuf2->group & groupCode)
            {  if (nodeOut(&oldNode))
               {  nodeIn(&newNode);
                  putRec(CFG_ECHOAREAS, count);
                  updated++;
               }
               total++;
            }
         }
         processed (updated, total);
      }
   }
   return (updated);
}

static u16 globNodeChange;

s16 gndFun(void)
{
	menuType    *gNdMenu;

	if ((gNdMenu = createMenu (" Export ")) == NULL)
		return(0);
	addItem (gNdMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
	addItem (gNdMenu, FUNCTION, "Insert node", 0, insNode, 0, 0,
				"Insert a node in connected systems list");
	addItem (gNdMenu, FUNCTION, "Delete node", 0, delNode, 0, 0,
				"Remove a node from connected systems list");
	addItem (gNdMenu, FUNCTION, "Replace node", 0, replaceNode, 0, 0,
				"Replace a node from connected systems list");
	return (globNodeChange |= runMenu(gNdMenu, 16, 14));
}

extern menuType    *raMenu;
extern menuType    *areaMenu;

extern funcParType akaFunc;
extern funcParType multiAkaSelectRec;
extern u32         alsoSeenBy;
extern s16         groupChar;
extern u16         defaultEnumRA;

extern u16         editDefault;

char        tempToggleRA;

s16 editAM (s16 editType, u16 setdef, rawEchoType *areaBuf)
{
   menuType    *raMenu;
   menuType    *areaMenu;
   toggleType  boardTypeToggle;
   toggleType  RAToggle;
   toggleType  RAToggle2;
   toggleType  RAToggle3;
   toggleType  RAToggle4;
   toggleType  QBBSToggle;
   toggleType  addressToggle;
   s16         temp;
   char        addressText[MAX_AKAS + 1][34];
   u16         count, total;
   static s32  groupCode;

   areaBuf2 = areaBuf;

   globNodeChange = 0;
   editDefault = (editType == EDIT_ECHO_DEFAULT);

   groupSelect.numPtr = (u16*)&tempInfo.group;
   groupSelect.f      = askGroup;

   mgrBoardSelect.numPtr = &tempInfo.board;
   if ( (editType == EDIT_ECHO) || (editType == DISPLAY_ECHO_WINDOW) ||
        (editType == DISPLAY_ECHO_DATA) )
      mgrBoardSelect.f = (function)echoDisplayAreas;
   else
      mgrBoardSelect.f = (function)netDisplayAreas;

   boardCodeSelect.numPtr = &tempInfo.boardNumRA;
   boardCodeSelect.f      = (function)selectBoardCode;

#ifdef GOLDBASE
   boardCodeSelectGold.numPtr = &tempInfo.board;
   boardCodeSelectGold.f      = (function)selectBoardCodeGold;
#endif

   boardTypeToggle.data      = (u8*)&tempInfo.board;
   boardTypeToggle.text  [0] = "Pass through";
   boardTypeToggle.retval[0] = 0;
   boardTypeToggle.text  [1] = MBNAME;
   boardTypeToggle.retval[1] = 1;
   boardTypeToggle.text  [2] = "JAM";
   boardTypeToggle.retval[2] = 2;

   if ( editType == EDIT_ECHO_DEFAULT )
      RAToggle.data = (u8*)&defaultEnumRA;
   else
   {
      RAToggle.data = (u8*)&tempToggleRA;
      tempToggleRA = (tempInfo.attrRA & BIT3) ?
                     (tempInfo.attrRA & BIT5) ? 1 : 2 : 0;
   }
   RAToggle.text[0] = "Real names only";
   RAToggle.retval[0] = 0;
   RAToggle.text[1] = "Handles only";
   RAToggle.retval[1] = 1;
   RAToggle.text[2] = "Pick an alias";
   RAToggle.retval[2] = 2;

   RAToggle2.data    = &tempInfo.msgKindsRA;
   RAToggle2.text[0] = "Private/public";
   RAToggle2.retval[0] = 0;
   RAToggle2.text[1] = "Private";
   RAToggle2.retval[1] = 1;
   RAToggle2.text[2] = "Public";
   RAToggle2.retval[2] = 2;
   if ( config.bbsProgram == BBS_PROB )
   {
      RAToggle2.text[3] = "To-all";
      RAToggle2.retval[3] = 3;
   }
   else
   {
      RAToggle2.text[3] = "Read-only";
      RAToggle2.retval[3] = 3;
      RAToggle2.text[4] = "No-reply";
      RAToggle2.retval[4] = 4;
   }

   RAToggle3.data    = &tempInfo.replyStatSBBS;
   RAToggle3.text[0] = "Normal";
   RAToggle3.retval[0] = 0;
   RAToggle3.text[1] = "Netmail/normal";
   RAToggle3.retval[1] = 1;
   RAToggle3.text[2] = "Netmail";
   RAToggle3.retval[2] = 2;
   RAToggle3.text[3] = "No replies";
   RAToggle3.retval[3] = 3;

   RAToggle4.data    = &tempInfo.boardTypeRA;
   RAToggle4.text[0] = "Echomail";
   RAToggle4.retval[0] = 0;
   RAToggle4.text[1] = "Internet";
   RAToggle4.retval[1] = 1;
   RAToggle4.text[2] = "Newsgroup";
   RAToggle4.retval[2] = 2;

   QBBSToggle.data    = &tempInfo.aliasesQBBS;
   QBBSToggle.text[0] = "No";
   QBBSToggle.retval[0] = 0;
   QBBSToggle.text[1] = "Ask";
   QBBSToggle.retval[1] = 1;
   QBBSToggle.text[2] = "Yes";
   QBBSToggle.retval[2] = 2;

   addressToggle.data = (u8*)&tempInfo.address;
   for (count = 0; count < MAX_AKAS; count++)
   {
      if (config.akaList[count].nodeNum.zone != 0)
         strcpy(addressText[count], nodeStr((nodeNumType*)(&config.akaList[count])));
      else
         if (count == 0)
            strcpy(addressText[0], "Main (not defined)");
         else
            sprintf(addressText[count], "AKA %2u (not defined)", count);
      addressToggle.text[count] = addressText[count];
      addressToggle.retval[count] = count;
   }

   alsoSeenBy = tempInfo.alsoSeenBy;

   if ((raMenu = createMenu ((config.bbsProgram==BBS_RA1X ||
                              config.bbsProgram==BBS_RA20 ||
                              config.bbsProgram==BBS_RA25)?" RemoteAccess Info ":
                              config.bbsProgram==BBS_SBBS ?" SuperBBS info ":
                              config.bbsProgram==BBS_QBBS ?" QuickBBS info ":
                              config.bbsProgram==BBS_TAG  ?" TAG info ":
                              config.bbsProgram==BBS_PROB ?" ProBoard info ":" EleBBS info ")) == NULL)
   {
      return(0);
   }
   addItem (raMenu, BOOL_INT, "BBS Export", 0, &tempInfo.options, BIT12, 0,
                    "Export this area to the BBS areas file");
   if ( config.bbsProgram == BBS_SBBS || config.bbsProgram == BBS_TAG || config.bbsProgram == BBS_PROB )
   {
      if (editType&(EDIT_ECHO_DEFAULT))
      {  *tempInfo.qwkName = 0;
      }
      addItem (raMenu, (editType & (EDIT_ECHO_DEFAULT|EDIT_ECHO_GLOBAL)) ? (DISPLAY|WORD):(WORD|UPCASE), "QWK name   ", 40, tempInfo.qwkName, 12, 0,
                       "Short QWK name, empty = area not included in the QWK system");
   }
   addItem (raMenu, ENUM_INT, "Status", 0, &RAToggle2, 0, (config.bbsProgram == BBS_PROB) ? 4 : 5,
                    "Private/public, private only, public only or read-only messages");
   addItem (raMenu, echoNGOnly(editType,FUNC_PAR), config.bbsProgram == BBS_QBBS?"Board cd":"Board code ", (config.bbsProgram == BBS_QBBS)?47:40, &boardCodeSelect, 0, 0,
                    "Board number used for MESSAGES.RA (= "MBNAME" board number if defined)");
   addItem (raMenu, ENUM_INT, "Users", 0, &RAToggle, 0, 3,
                    "User names - Real names only, handles only or ask for an alias");

   if ( config.bbsProgram == BBS_RA1X || config.bbsProgram >= BBS_RA20 )
   {
      addItem (raMenu, NUM_INT, "Minimum age", 40, &tempInfo.minAgeSBBS, 3, 250,
                       "Minimum age required for this area");
   }
   if ( config.bbsProgram == BBS_RA1X || config.bbsProgram == BBS_RA20 ||
        config.bbsProgram == BBS_RA25 || config.bbsProgram == BBS_ELEB )
   {
      addItem (raMenu, BOOL_INT, "EchoInfo", 0, &tempInfo.attrRA, BIT0, 0,
                       "Append an origin line to outbound messages");
      addItem (raMenu, NUM_INT, "Group RA   ", 40, &tempInfo.groupRA, 5, -1,
                       "RA group to which this area belongs");
      addItem (raMenu, BOOL_INT, "Combined", 0, &tempInfo.attrRA, BIT1, 0,
                       "Permit combined access to a message area");
      addItem (raMenu, NUM_INT, "Alt Group 1", 40, &tempInfo.altGroupRA[0], 5, -1,
                       "Secondary group to which this area belongs");
      addItem (raMenu, BOOL_INT, "Attaches", 0, &tempInfo.attrRA, BIT2, 0,
                       "Permit users to attach files to messages");
      addItem (raMenu, NUM_INT, "Alt Group 2", 40, &tempInfo.altGroupRA[1], 5, -1,
                       "Secondary group to which this area belongs");
      addItem (raMenu, BOOL_INT, "SoftCRs", 0, &tempInfo.attrRA, BIT4, 0,
                       "Treat SoftCRs as normal characters in an area?");
      addItem (raMenu, NUM_INT, "Alt Group 3", 40, &tempInfo.altGroupRA[2], 5, -1,
                       "Secondary group to which this area belongs");
      addItem (raMenu, BOOL_INT, "Deletes", 0, &tempInfo.attrRA, BIT6, 0,
                       "Permit users to delete messages in an area");
      addItem (raMenu, BOOL_INT, "All Groups ", 40, &tempInfo.attr2RA, BIT0, 0,
                       "\"Yes\" means this area appears in all groups");
      addItem (raMenu, echoDefOnly(editType,ENUM_INT), "Type", 0, &RAToggle4, 0, 3,
                       "Echomail, Internet or Newsgroup");
      addItem (raMenu, echoDefOnly(editType,NUM_INT), "Net reply  ", 40, &tempInfo.netReplyBoardRA, 5, -1,
                       "Area in which to post netmail replies (0 = first available)");
   }

   if ( config.bbsProgram == BBS_SBBS )
   {
      addItem (raMenu, ENUM_INT, "Rep stat", 0, &RAToggle3, 0, 4,
                       "How replies are handled in this area");
      addItem (raMenu, BOOL_INT, "Combined", 0, &tempInfo.attrRA, BIT1, 0,
                       "Permit combined access to a message area");
      addItem (raMenu, NUM_INT, "Minimum age ", 40, &tempInfo.minAgeSBBS, 3, 250,
                       "Minimum age required for this area");
      addItem (raMenu, BOOL_INT, "Deletes", 0, &tempInfo.attrRA, BIT6, 0,
                       "Permit users to delete messages in an area");
      addItem (raMenu, BOOL_INT, "Default comb", 40, &tempInfo.attrSBBS, BIT1, 0,
                       "Is this area toggle ON by default in the combined board");
      addItem (raMenu, BOOL_INT, "Template", 0, &tempInfo.attrSBBS, BIT4, 0,
                       "Use this area in the template system");
      addItem (raMenu, BOOL_INT, "8bit -> 7bit", 40, &tempInfo.attrSBBS, BIT5, 0,
                       "Convert 8-bit characters to 7-bit characters in this area");
      addItem (raMenu, BOOL_INT, "Taglines", 0, &tempInfo.attrSBBS, BIT3, 0,
                       "Allow taglines in this area");
      addItem (raMenu, BOOL_INT, "Mail check  ", 40, &tempInfo.attrSBBS, BIT6, 0,
                       "Always use this area in the mail check");
   }

   if ( config.bbsProgram == BBS_QBBS )
   {
      addItem (raMenu, NUM_INT, "Template", 47, &tempInfo.templateSecQBBS, 5, 32000,
                       "BBS Template security level");
      addItem (raMenu, ENUM_INT, "Aliases", 0, &QBBSToggle, 0, 3,
                       "Whether or not aliases should be used for messages in this area");
      addItem (raMenu, BIT_TOGGLE, "A flag  ", 47, &tempInfo.flagsTemplateQBBS[0], 0, 0,
                       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BOOL_INT, "Combined", 0, &tempInfo.attrRA, BIT1, 0,
                       "Permit combined access to a message area");
      addItem (raMenu, BIT_TOGGLE, "B flag  ", 47, &tempInfo.flagsTemplateQBBS[1], 0, 0,
                       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BOOL_INT, "Deletes", 0, &tempInfo.attrRA, BIT6, 0,
                       "Permit users to delete messages in an area");
      addItem (raMenu, BIT_TOGGLE, "C flag  ", 47, &tempInfo.flagsTemplateQBBS[2], 0, 0,
                       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "Groups", 0, &tempInfo.groupsQBBS, 0, 0,
                       "QuickBBS template grouping (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "D flag  ", 47, &tempInfo.flagsTemplateQBBS[3], 0, 0,
                       "BBS security flags (Enter 1-8 to toggle)");
   }

   if ( config.bbsProgram == BBS_PROB )
   {
      addItem (raMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
      addItem (raMenu, NUM_INT, "Group #1", 0, &tempInfo.groupRA, 3, 255,
                       "RA group to which this area belongs");
      addItem (raMenu, NUM_INT, "Group #3   ", 40, &tempInfo.altGroupRA[1], 3, 255,
                       "Secondary group to which this area belongs");
      addItem (raMenu, NUM_INT, "Group #2", 0, &tempInfo.altGroupRA[0], 3, 255,
                       "Secondary group to which this area belongs");
      addItem (raMenu, NUM_INT, "Group #4   ", 40, &tempInfo.altGroupRA[2], 3, 255,
                       "Secondary group to which this area belongs");
      addItem (raMenu, BOOL_INT, "All Groups", 0, &tempInfo.attr2RA, BIT0, 0,
                       "\"Yes\" means this area appears in all groups");
   }

   addItem (raMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
   addItem (raMenu, NUM_INT, "Read", 0, &tempInfo.readSecRA, 5, (config.bbsProgram==BBS_RA1X||config.bbsProgram==BBS_RA20||config.bbsProgram==BBS_RA25||config.bbsProgram==BBS_ELEB)?-1:config.bbsProgram==BBS_SBBS?32500:32000,
                    "BBS Read security level");
   addItem (raMenu, NUM_INT, "Write   ", 23, &tempInfo.writeSecRA, 5, (config.bbsProgram==BBS_RA1X||config.bbsProgram>=BBS_RA20)?-1:config.bbsProgram==BBS_SBBS?32500:32000,
                    "BBS Write security level");
   if ( config.bbsProgram <= BBS_QBBS || config.bbsProgram >= BBS_RA20 )
   {
      addItem (raMenu, NUM_INT, "SysOp   ", 47, &tempInfo.sysopSecRA, 5, (config.bbsProgram==BBS_RA1X||config.bbsProgram==BBS_RA20||config.bbsProgram==BBS_RA25||config.bbsProgram==BBS_ELEB)?-1:config.bbsProgram==BBS_SBBS?32500:32000,
		       "BBS SysOp security level");
      addItem (raMenu, BIT_TOGGLE, "A flag", 0, &tempInfo.flagsRdRA[0], 0, 0,
                       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "A flag  ", 23, &tempInfo.flagsWrRA[0], 0, 0,
                       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "A flag  ", 47, &tempInfo.flagsSysRA[0], 0, 0,
		       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "B flag", 0, &tempInfo.flagsRdRA[1], 0, 0,
                       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "B flag  ", 23, &tempInfo.flagsWrRA[1], 0, 0,
                       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "B flag  ", 47, &tempInfo.flagsSysRA[1], 0, 0,
		       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "C flag", 0, &tempInfo.flagsRdRA[2], 0, 0,
		       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "C flag  ", 23, &tempInfo.flagsWrRA[2], 0, 0,
		       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "C flag  ", 47, &tempInfo.flagsSysRA[2], 0, 0,
                       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "D flag", 0, &tempInfo.flagsRdRA[3], 0, 0,
		       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "D flag  ", 23, &tempInfo.flagsWrRA[3], 0, 0,
		       "BBS security flags (Enter 1-8 to toggle)");
      addItem (raMenu, BIT_TOGGLE, "D flag  ", 47, &tempInfo.flagsSysRA[3], 0, 0,
		       "BBS security flags (Enter 1-8 to toggle)");
   }

   if ((areaMenu = createMenu (
          (editType == EDIT_ECHO_DEFAULT) ? " AreaMgr defaults ":
   	      (editType == EDIT_NETMAIL     ) ? " Netmail areas "   :
                                            " Echo mail areas ")) == NULL)
   {
     free(raMenu);
     return 0;
   }

   addItem (areaMenu, (editType&(EDIT_NETMAIL|EDIT_ECHO_DEFAULT|EDIT_ECHO_GLOBAL))?(DISPLAY|WORD):(WORD|UPCASE), "Area name  ", 0, tempInfo.areaName, ECHONAME_LEN-1, 0,
                      "Name of the area (echo tag)");
   addItem (areaMenu, (editType&(EDIT_ECHO_DEFAULT))?(DISPLAY|TEXT):TEXT|CTRLCODES, "Comment", 0, tempInfo.comment, COMMENT_LEN-1, 0,
                      "Comment describing the area");
   addItem (areaMenu, (editType&(EDIT_NETMAIL|EDIT_ECHO_GLOBAL))?(DISPLAY|TEXT):((editType&EDIT_ECHO_DEFAULT)?(PATH|UPCASE|LFN):(MB_NAME|UPCASE|LFN)), "JAM MB name", 0, tempInfo.msgBasePath, (editType&EDIT_ECHO_DEFAULT)?MB_PATH_LEN-11:MB_PATH_LEN-3, 0,
                      "Path and file name of the JAM message base");
   if (editType == EDIT_ECHO_DEFAULT)
   {  addItem (areaMenu, ENUM_INT, "Board type", 0, &boardTypeToggle, 0, 3,
                         "Message base type to be used for a new area");
   }
   else
   {
#ifndef GOLDBASE
      addItem (areaMenu, nonGlobOnly(editType,FUNC_PAR), "Board", 0, &mgrBoardSelect, 0, 0,
                         MBNAME" message base board in which messages will be imported");
#else
      addItem (areaMenu, nonGlobOnly(editType,FUNC_PAR), "Board", 0, &boardCodeSelectGold, 0, 0,
                         MBNAME" message base board in which messages will be imported");
#endif
   }
   addItem(areaMenu, echoOnly(editType,BOOL_INT), "Local      ", 37, &tempInfo.options, BIT8, 0,
                     "Set to Yes for a non-echomail area");
   addItem(areaMenu, echoOnly(editType,BOOL_INT), "Active      ", 56, &tempInfo.options, BIT0, 0,
                     "Set to No to deactivate this area");
   addItem(areaMenu, echoDefOnly(editType, NUM_INT), "Write level", 0, &tempInfo.writeLevel, 5, 32767,
                     "Level should be > than Write Level of a node in order to be read-only");
   addItem(areaMenu, echoDefOnly(editType, BOOL_INT), "AreaFix    ", 37, &tempInfo.options, BIT11, 0,
                     "Allow systems to connect and disconnect this area themselves");
   addItem(areaMenu, NEW_WINDOW, "BBS info    ", 56, raMenu, 0, 2,
                     "BBS message board information, used by AutoExport");
   addItem(areaMenu, echoDefNGOnly(editType, FUNC_PAR), "Group", 0, &groupSelect, 1, 0,
                     "Group of areas to which this area belongs (A-Z)");
   addItem(areaMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
   addItem(areaMenu, echoDefOnly(editType, BOOL_INT), "Use SeenBy", 0, &tempInfo.options, BIT6, 0,
                     "Use SEEN-BYs for duplicate prevention (Should normally be on)");
   addItem(areaMenu, echoDefOnly(editType, BOOL_INT), "Security    ", 18, &tempInfo.options, BIT2, 0,
                     "Check origin of incoming mailbundles");
   addItem(areaMenu, NUM_INT, "# Messages ", 37, &tempInfo.msgs, 5, 65535,
                     "Maximum number of messages in an area (1-65535, 0 = no maximum)");
   addItem(areaMenu, BOOL_INT, "Arrival date", 56, &tempInfo.options, BIT14, 0,
                     "Use the date that a message arrived on your system when deleting messages");
   addItem(areaMenu, echoDefOnly(editType, BOOL_INT), "Tiny SeenBy", 0, &tempInfo.options, BIT1, 0,
                     "Remove all SEEN-BYs except of your own links (should normally NOT be used)");
   addItem(areaMenu, echoDefOnly(editType, BOOL_INT), "Private     ", 18, &tempInfo.options, BIT4, 0,
                     "Allow private (privileged) messages");
   addItem(areaMenu, NUM_INT, "# Days old ", 37, &tempInfo.days, 3, 999,
                     "Delete messages older than a number of days (1-999, 0 = no maximum)");
   addItem(areaMenu, BOOL_INT, "Keep SysOp  ", 56, &tempInfo.options, BIT15, 0,
                     "Do not remove messages that have not been read by the SysOp (user #1)");
   addItem(areaMenu, echoDefOnly(editType, BOOL_INT), "Tiny Path", 0, &tempInfo.options, BIT7, 0,
                     "Remove all PATHs except of your own system (should normally NOT be used)");
   addItem(areaMenu, echoDefOnly(editType, BOOL_INT_REV), "Allow rescan", 18, &tempInfo.options, BIT3, 0,
                     "Allow rescans for this area");
   addItem(areaMenu, NUM_INT, "# Days rcvd", 37, &tempInfo.daysRcvd, 3, 999,
                     "Delete received messages older than a number of days (1-999, 0 = no maximum)");
   addItem(areaMenu, echoDefOnly(editType, BOOL_INT), "Imp. SB/Pth", 0, &tempInfo.options, BIT5, 0,
                     "Import SEEN-BYs and PATH lines into the message base");
   addItem(areaMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
   addItem(areaMenu, echoOnly(editType, ENUM_INT), "Origin AKA", 0, &addressToggle, 0, MAX_AKAS,
                     "Address used for origin line and as packet origin address");
   if ( !(editType & EDIT_ECHO_DEFAULT) )
     addItem (areaMenu, DATE|DISPLAY, "LastScanned", 47, &tempInfo.lastMsgScanDat, 0, 0,
                      "Last time a message was scanned from this area");
   addItem (areaMenu, echoOnly(editType,FUNC_PAR), "Other AKAs", 0, &multiAkaSelectRec, 0, 4,
                      "AKAs added to SEEN-BY lines");
   if ( !(editType & EDIT_ECHO_DEFAULT) )
      addItem (areaMenu, DATE|DISPLAY, "LastTossed ", 47, &tempInfo.lastMsgTossDat, 0, 0,
                      "Last time a message was tossed into this area");
   addItem (areaMenu, echoDefOnly(editType,TEXT), "Origin line", 0, tempInfo.originLine, ORGLINE_LEN-1, 0,
                      "Origin line that will be used for messages from your system");
   if ( !(editType & EDIT_ECHO_GLOBAL) )
      addItem (areaMenu, echoOnly(editType,FUNCTION), "Export", 0, nodeScroll, 2, 2,
                      "Nodes to which the echo area is exported");
   else
      addItem (areaMenu, FUNCTION, "Export", 0, gndFun, 2, 2,
                      "Nodes to which the echo area is exported");

   if (  editType == EDIT_ECHO
      || editType == DISPLAY_ECHO_WINDOW
      || editType == DISPLAY_ECHO_DATA
      )
      addItem(areaMenu, echoOnly(editType, EXTRA_TEXT | NO_EDIT), NULL, 0, NULL, ORGLINE_LEN - 1, 0, "");

   switch (editType)
   {
      case DISPLAY_ECHO_WINDOW :
        temp = displayMenu(areaMenu, 1, 5);
        break;
      case DISPLAY_ECHO_DATA   :
        displayData(areaMenu, 1, 5, 0);
        temp = 0;
        break;
      case EDIT_GLOB_BBS       :
        temp = runMenuD   (raMenu  , 2, 7, NULL, setdef);
        break;
      case EDIT_ECHO           :
        temp = runMenuD   (areaMenu, 1, 5, NULL, setdef);
        break;
      case EDIT_NETMAIL        :
        temp = runMenuD   (areaMenu, 1, 5, NULL, setdef);
        break;
      case EDIT_ECHO_GLOBAL    :
        runMenuD   (areaMenu, 1, 5, NULL, setdef);
        temp = 0;
        break;
      case EDIT_ECHO_DEFAULT   :
        temp = runMenuD   (areaMenu, 1, 5, NULL, setdef);
        break;
   }
   if (editType == EDIT_ECHO_DEFAULT)
   {
      if (defaultEnumRA)
         tempInfo.attrRA |= BIT3;
      else
         tempInfo.attrRA &= ~BIT3;
      if (defaultEnumRA == 1)
         tempInfo.attrRA |= BIT5;
      else
         tempInfo.attrRA &= ~BIT5;
   }
   else
   {  if (tempToggleRA)
        tempInfo.attrRA |= BIT3;
      else
        tempInfo.attrRA &= ~BIT3;
      if (tempToggleRA == 1)
        tempInfo.attrRA |= BIT5;
      else
        tempInfo.attrRA &= ~BIT5;
   }
   tempInfo.alsoSeenBy = alsoSeenBy;

   if (setdef)
   {
      total = 0;
      for ( count = 0; count < areaMenu->entryCount; count++ )
        total += (areaMenu->menuEntry[count].selected != 0);

      for ( count = 0; count < raMenu->entryCount; count++ )
        total += (raMenu->menuEntry[count].selected != 0);

      if ( total )
      {
         temp = total = 0;
         if ((groupCode = getGroups(0)) != -1)
         {
            updInfo = tempInfo;
            for ( count = 0; count < areaInfoCount; count++ )
            {
               if (areaInfo[count]->group & groupCode)
               {
                  ++total;
                  getRec(CFG_ECHOAREAS, count);
                  memcpy(&tempInfo, areaBuf, RAWECHO_SIZE);
                  temp += changeGlobal(areaMenu, &tempInfo, &updInfo)
                        | changeGlobal(raMenu  , &tempInfo, &updInfo);
                  memcpy(areaBuf, &tempInfo, RAWECHO_SIZE);
                  putRec(CFG_ECHOAREAS, count);
               }
            }
            processed(temp, total);
         }
      }
   }

   free(raMenu);
   free(areaMenu);

   return (temp || globNodeChange);
}
//---------------------------------------------------------------------------
