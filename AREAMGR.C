//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2015  Wilfred van Velzen
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
#include <dir.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"

#include "areamgr.h"

#include "amgr_utl.h"
#include "areainfo.h"
#include "cfgfile.h"
#include "fs_func.h"
#include "fs_util.h"
#include "utils.h"
#include "window.h"

typedef struct      /* OLD !!! */
{
   uchar           zero; /* Should always be zero */
   uchar           msgBasePath[MB_PATH_LEN_OLD];
   uchar           groupsQBBS;
   uchar           flagsTemplateQBBS[4];
   uchar           comment[COMMENT_LEN];
   u32             group;
   u16             board;
   u16             address;
   u16             alsoSeenBy;
   u16             groupRA;
   u16             altGroupRA[3];
   areaOptionsType options;
   u16             outStatus;
   u16             days;
   u16             msgs;
   u16             daysRcvd;
   u16             templateSecQBBS;
   u16             readSecRA;
   uchar           flagsRdRA[4];
   u16             writeSecRA;
   uchar           flagsWrRA[4];
   u16             sysopSecRA;
   uchar           flagsSysRA[4];
   uchar           attrRA;
   uchar           msgKindsRA;
   u16             attrSBBS;
   uchar           replyStatSBBS;
   areaNameType    areaName;
   uchar           qwkName[13];
   u16             minAgeSBBS;
   uchar           attr2RA;
   uchar           aliasesQBBS;
   uchar           originLine[ORGLINE_LEN];
   nodeNumType     export[MAX_FORWARDOLD];

} rawEchoTypeOld;


typedef struct
{
   u16             signature; /* contains "AE" for echo areas in FMAIL.AR and */
                              /* "AD" for default settings in FMAIL.ARD       */
   u16             writeLevel;
   areaNameType    areaName;
   uchar           comment[COMMENT_LEN];
   areaOptionsType options;
   u16             boardNumRA;
   uchar           msgBaseType;
   uchar           msgBasePath[MB_PATH_LEN];
   u16             board;
   uchar           originLine[ORGLINE_LEN];
   u16             address;
   u32             group;
   u16             alsoSeenBy;
   u16             msgs;
   u16             days;
   u16             daysRcvd;

   nodeNumType     forwards[MAX_FORWARDOLD];

   u16             readSecRA;
   uchar           flagsRdNotRA[4];
   uchar           flagsRdRA[4];
   u16             writeSecRA;
   uchar           flagsWrNotRA[4];
   uchar           flagsWrRA[4];
   u16             sysopSecRA;
   uchar           flagsSysRA[4];
   uchar           flagsSysNotRA[4];
   u16             templateSecQBBS;
   uchar           flagsTemplateQBBS[4];
   uchar           reserved2;
   u16             netReplyBoardRA;
   uchar           boardTypeRA;
   uchar           attrRA;
   uchar           attr2RA;
   u16             groupRA;
   u16             altGroupRA[3];
   uchar           msgKindsRA;
   uchar           qwkName[13];
   u16             minAgeSBBS;
   u16             attrSBBS;
   uchar           replyStatSBBS;
   uchar           groupsQBBS;
   uchar           aliasesQBBS;  } rawEchoType102;

typedef struct
{
   u16             signature; /* contains "AE" for echo areas in FMAIL.AR and */
                              /* "AD" for default settings in FMAIL.ARD       */
   u16             writeLevel;
   areaNameType    areaName;
   uchar           comment[COMMENT_LEN];
   areaOptionsType options;
   u16             boardNumRA;
   uchar           msgBaseType;
   uchar           msgBasePath[MB_PATH_LEN];
   u16             board;
   uchar           originLine[ORGLINE_LEN];
   u16             address;
   u32             group;
   u16             _alsoSeenBy; /* obsolete: see the 32-bit alsoSeenBy below */
   u16             msgs;
   u16             days;
   u16             daysRcvd;

   nodeNumType     forwards[MAX_FORWARDOLD];

   u16             readSecRA;
   uchar           flagsRdRA[4];
   uchar           flagsRdNotRA[4];
   u16             writeSecRA;
   uchar           flagsWrRA[4];
   uchar           flagsWrNotRA[4];
   u16             sysopSecRA;
   uchar           flagsSysRA[4];
   uchar           flagsSysNotRA[4];
   u16             templateSecQBBS;
   uchar           flagsTemplateQBBS[4];
   uchar           _internalUse;
   u16             netReplyBoardRA;
   uchar           boardTypeRA;
   uchar           attrRA;
   uchar           attr2RA;
   u16             groupRA;
   u16             altGroupRA[3];
   uchar           msgKindsRA;
   uchar           qwkName[13];
   u16             minAgeSBBS;
   u16             attrSBBS;
   uchar           replyStatSBBS;
   uchar           groupsQBBS;
   uchar           aliasesQBBS;
   u32             lastMsgTossDat;
   u32             lastMsgScanDat;
   u32             alsoSeenBy;
   areaStatType    stat;
   uchar           reserved[180];   } rawEchoType120;



extern badEchoType  badEchos[MAX_BAD_ECHOS];
extern u16          badEchoCount;

extern rawEchoType  echoDefaultsRec;

extern s16 boardEdit;

areaInfoPtrArr areaInfo;
u16            areaInfoCount;
u16            areaInfoBoard;

u32            alsoSeenBy;
s16            groupChar;
u16            am__cp;

extern windowLookType windowLook;
extern configType     config;
extern char configPath[128];
/*extern s32           pow2[32];*/
u16                   *boardPtr;

char boardCodeInfo[512];

funcParType multiAkaSelectRec;
rawEchoType tempInfo, updInfo;




/*
static s32 getGroups (void)
{
   char tempStr[28];
   char ch;

   saveWindowLook();
   strcpy (tempStr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
   if (displayWindow (NULL, 36, 13, 72, 15) != 0) return (0);
   printString ("Groups", 38, 14, windowLook.promptfg,
                                  windowLook.background,
                                  MONO_NORM);
   ch = editString (tempStr, 27, 45, 14, ALPHA_AST|UPCASE);
   removeWindow();
   restoreWindowLook();

   if (ch == _K_ESC_)
      return (-1);

   return (getGroupCode (tempStr));
}
*/


s16 getSetReset (void)
{
   u16 ch;

   if (displayWindow (NULL, 49, 12, 70, 14) != 0) return (0);
   printString ("(S)et or (R)eset ?", 51, 13,
                windowLook.promptfg, windowLook.background, MONO_NORM);

   do
   {
      ch=readKbd();
      ch = toupper (ch);
   }
   while ((ch != 'R') && (ch != 'S') && (ch != _K_ESC_));

   removeWindow ();
   return (ch);
}



s16 multiAkaSelect (void)
{
   u16      update = 0;
   u16      count;
   u16      currentElem = 0;
   s16      ch;
   char     tempStr[48];
   u16      offset = 0;

   if (displayWindow (" Other AKAs ", 33, 4, 71, 21) != 0) return (0);

   do
   {
      for (count = 0; count < MAX_AKAS; count++)
      {
         if (count == 0)
            strcpy (tempStr, " Main   : ");
         else
            sprintf (tempStr, " AKA %2u : ", count);

         if (config.akaList[count].nodeNum.zone != 0)
            strcat (tempStr, nodeStr((nodeNumType*)(&config.akaList[count])));

         if ( count >= offset && count < offset+16 )
         {  if (count == currentElem)
               printStringFill (tempStr, ' ', 34, 34, 5+count-offset,
                                windowLook.scrollfg, windowLook.scrollbg,
                                MONO_INV);
            else
               printStringFill (tempStr, ' ', 34, 34, 5+count-offset,
                                windowLook.promptfg, windowLook.background,
                                MONO_NORM);
            if (alsoSeenBy & (1L<<count))
               printChar ('þ', 69, 5+count-offset, windowLook.datafg, windowLook.background, MONO_HIGH);
            else
               printChar (' ', 69, 5+count-offset, windowLook.datafg, windowLook.background, MONO_NORM);
         }
      }
      ch=readKbd();
      switch (ch)
      {
         case _K_ENTER_ :
                       update = 1;                          /* Enter */
                       alsoSeenBy ^= (1L<<currentElem);
                       break;
         case _K_CPGUP_ :
         case _K_HOME_ :
                       currentElem = 0;                     /* Home */
                       offset = 0;
                       break;
         case _K_CPGDN_ :
         case _K_END_: currentElem = MAX_AKAS-1;           /* End */
                       offset = MAX_AKAS-16;
                       break;
         case _K_UP_ : if (currentElem > 0)                 /* Cursor Up */
                       {  if ( currentElem == offset )
                             offset--;
                          currentElem--;
                       }
                       break;
         case _K_DOWN_ :
                       if (currentElem+1 < MAX_AKAS)       /* Cursor Down */
                       {  if ( currentElem == offset + 15 )
                             offset++;
                          currentElem++;
                       }
                       break;
      }
   }
   while (ch != _K_ESC_);

   removeWindow ();
   return (update);
}



static void fillAreaInfo(u16 index, rawEchoType *tempInfo)
{
   memset(areaInfo[index], 0, sizeof(rawEchoTypeX));
   if ( (areaInfo[index]->areaName = malloc(strlen(tempInfo->areaName)+1)) != NULL )
      strcpy(areaInfo[index]->areaName, tempInfo->areaName);
   if ( (areaInfo[index]->comment = malloc(strlen(tempInfo->comment)+1)) != NULL )
      strcpy(areaInfo[index]->comment, tempInfo->comment);
   if ( (areaInfo[index]->msgBasePath = malloc(strlen(tempInfo->msgBasePath)+1)) != NULL )
      strcpy(areaInfo[index]->msgBasePath, tempInfo->msgBasePath);
   areaInfo[index]->boardNumRA = tempInfo->boardNumRA;
   areaInfo[index]->msgBaseType = tempInfo->msgBaseType;
   areaInfo[index]->board = tempInfo->board;
   areaInfo[index]->options = tempInfo->options;
   areaInfo[index]->address = tempInfo->address;
   areaInfo[index]->group = tempInfo->group;
}


static void freeAreaInfo(u16 index)
{
   free(areaInfo[index]->areaName);
   free(areaInfo[index]->comment);
   free(areaInfo[index]->msgBasePath);
}




s16 areaMgr (void)
{
   u8		*tempPtr; //, *tempPtr2;
   u16          index = 0;
   u16          pos, idx;
   areaInfoPtr  areaHelpPtr;
   u16          ch;
   s16          count, count2;
   tempStrType  areaInfoPath;
   fhandle      areaInfoHandle;
   u16          update = 0;
   char         usedArea[MBBOARDS];
   tempStrType  tempStr;
   s16          newEcho;
   headerType   *areaHeader;
   rawEchoType  *areaBuf;
   rawEchoTypeX *tempBufPtr;
   rawEchoTypeOld     areaInfoOld;
   char		      *helpPtr;
   static char  groupSelectMask = 0;
   static char  groupSearch = 0;
   areaNameType findStr;
   s16          findPos = 0;
   s16          findOk;
   nodeNumXType tempNodeX;

   multiAkaSelectRec.numPtr =(u16*)&alsoSeenBy;
   multiAkaSelectRec.f      = multiAkaSelect;
#if 0
   if ((gSwMenu = createMenu (" Switches ")) == NULL)
   {
      return(0);
   }
   addItem (gSwMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
   addItem (gSwMenu, FUNC_VPAR, "Active", 0, globSwitch, BIT0, 0,
   	    "Set to No to deactivate areas");
   addItem (gSwMenu, FUNC_VPAR, "Local", 0, globSwitch, BIT8, 0,
	    "Set to Yes for non-echomail areas");
   addItem (gSwMenu, FUNC_VPAR, "AreaFix", 0, globSwitch, BIT11, 0,
	    "Allow systems to connect and disconnect this area themselves");
   addItem (gSwMenu, FUNC_VPAR, "Security", 0, globSwitch, BIT2, 0,
	    "Check origin of incoming mailbundles");
   addItem (gSwMenu, FUNC_VPAR, "Private", 0, globSwitch, BIT4, 0,
	    "Allow private (privileged) messages");
   addItem (gSwMenu, FUNC_VPAR, "Use SeenBy", 0, globSwitch, BIT6, 0,
	    "Use SEEN-BYs for duplicate prevention (normally NOT necessary)");
   addItem (gSwMenu, FUNC_VPAR, "Tiny SeenBy", 0, globSwitch, BIT1, 0,
	    "Remove all SEEN-BYs except of your own downlinks (should normally NOT be used)");
   addItem (gSwMenu, FUNC_VPAR, "Imp. SeenBy", 0, globSwitch, BIT5, 0,
	    "Import SEEN-BYs into the message base");

   if ((gMnMenu = createMenu (" Maintenance ")) == NULL)
   {
      free (gSwMenu);
      return(0);
   }
   addItem (gMnMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
   addItem (gMnMenu, FUNCTION, "# Messages", 0, globNumMsgs, 0, 0,
	    	     "Maximum number of messages in an area (1-9999, 0 = no maximum)");
   addItem (gMnMenu, FUNCTION, "# Days old", 0, globNumDays, 0, 0,
		     "Delete messages older than a number of days (1-999, 0 = no maximum)");
   addItem (gMnMenu, FUNCTION, "# Days rcvd", 0, globNumDaysRcvd, 0, 0,
		     "Delete received messages older than a number of days (1-999, 0 = no maximum)");
   addItem (gMnMenu, FUNC_VPAR, "Arrival date", 0, globSwitch, BIT14, 0,
		     "Use the date that a message arrived on your system when deleting messages");
   addItem (gMnMenu, FUNC_VPAR, "Keep SysOp", 0, globSwitch, BIT15, 0,
		     "Do not remove messages that have not been read by the SysOp (user #1)");
#endif
#if 0
   if ((aglMenu = createMenu (" Global ")) == NULL)
   {
//    free (gNdMenu);
      free (gMnMenu);
      free (gSwMenu);
      return(0);
   }
   addItem (aglMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
   addItem (aglMenu, NEW_WINDOW, "Switches", 0, gSwMenu, 2, -1,
   		     "Set or reset switches (Private/privileged, Security, tiny SEEN-BYs etc.)");
   addItem (aglMenu, NEW_WINDOW, "Maintenance", 0, gMnMenu, 2, 2,
		     "Modify message base maintenance settings used by FTools");
   addItem (aglMenu, FUNCTION, "Write level", 0, globWriteLevel, 0, 0,
		     "Level should be > than Write Level of a node in order to be read-only");
   addItem (aglMenu, FUNCTION, "BBS info", 0, globRA, 0, 0,
		     "BBS message board information, used by AutoExport");
   addItem (aglMenu, FUNCTION, "Origin AKA", 0, globAddress, 0, 0,
		     "Address used for origin line and as packet origin address");
   addItem (aglMenu, FUNCTION, "Other AKAs", 0, globOtherAkas, 0, 0,
		     "AKAs added to SEEN-BY lines");
   addItem (aglMenu, FUNCTION, "Orig. line", 0, globOrgLine, 0, 0,
		     "Origin line used for messages without one");
   addItem (aglMenu, NEW_WINDOW, "Export", 0, gNdMenu, 2, 2,
		     "Add, remove or replace nodes in the connected systems lists");
   addItem (aglMenu, FUNCTION, "Group RA", 0, globGroupRA, 0, 0,
		     "RA group to which an area belongs");
   addItem (aglMenu, FUNCTION, "Min.age", 0, globAge, 0, 0,
		     "Minimum age to read an area");
#endif

   memset(boardCodeInfo, 0, sizeof(boardCodeInfo));
   if ( config.recBoard )
      boardCodeInfo[(config.recBoard-1)>>3] |= (1<<((config.recBoard-1)&7));
   if ( config.badBoard )
      boardCodeInfo[(config.badBoard-1)>>3] |= (1<<((config.badBoard-1)&7));
   if ( config.dupBoard )
      boardCodeInfo[(config.dupBoard-1)>>3] |= (1<<((config.dupBoard-1)&7));

   working ();
   if ( !openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf) )
   {
      helpPtr = stpcpy (areaInfoPath, configPath);
      strcpy(helpPtr, dARDFNAME);
      unlink(areaInfoPath);
      strcpy(helpPtr, dARFNAME);
      helpPtr = stpcpy(tempStr, areaInfoPath);
      strcpy(helpPtr, "FMAIL.$$$");
      rename(tempStr, areaInfoPath);

      if ((areaInfoHandle = open(areaInfoPath, O_BINARY|O_RDWR)) == -1)
         areaInfoCount = 0;
      else
      {
         if ( !openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf) )
         {  displayMessage("Can't convert old Areas file");
	    close (areaInfoHandle);
	    return (0);
         }
         lseek (areaInfoHandle, 0, SEEK_SET);
	 areaInfoCount = 0;
	 while ((areaInfoCount < MAX_AREAS) &&
	 	(read (areaInfoHandle, &areaInfoOld, sizeof(rawEchoTypeOld)) == sizeof (rawEchoTypeOld)))
	 {
            memset (&tempInfo, 0, RAWECHO_SIZE);

	    memcpy (tempInfo.areaName,   areaInfoOld.areaName,   sizeof(areaNameType)-1);
	    memcpy (tempInfo.comment,    areaInfoOld.comment,    COMMENT_LEN-1);
	    memcpy (tempInfo.msgBasePath,areaInfoOld.msgBasePath,MB_PATH_LEN_OLD-1);
	    memcpy (tempInfo.originLine, areaInfoOld.originLine, ORGLINE_LEN-1);
            for ( count = 0; count < MAX_FORWARDOLD; count++ )
               tempInfo.forwards[count].nodeNum = areaInfoOld.export[count];
	    tempInfo.signature  = 'AE';
	    tempInfo.group      = areaInfoOld.group;
	    tempInfo.board      = areaInfoOld.board;
	    tempInfo.address    = areaInfoOld.address;
	    tempInfo.alsoSeenBy = areaInfoOld.alsoSeenBy;
	    tempInfo.options    = areaInfoOld.options;
	    tempInfo.msgs       = areaInfoOld.msgs;
	    tempInfo.days       = areaInfoOld.days;
	    tempInfo.daysRcvd   = areaInfoOld.daysRcvd;
	    tempInfo.msgKindsRA = areaInfoOld.msgKindsRA;
	    tempInfo.groupRA    = areaInfoOld.groupRA;
	    memcpy (tempInfo.altGroupRA, areaInfoOld.altGroupRA, 3);
	    tempInfo.attrRA     = areaInfoOld.attrRA;
	    tempInfo.attr2RA    = areaInfoOld.attr2RA;
	    tempInfo.readSecRA  = areaInfoOld.readSecRA;
	    *(s32*)&tempInfo.flagsRdRA  = *(s32*)&areaInfoOld.flagsRdRA;
	    tempInfo.writeSecRA          = areaInfoOld.writeSecRA;
	    *(s32*)&tempInfo.flagsWrRA  = *(s32*)&areaInfoOld.flagsWrRA;
	    tempInfo.sysopSecRA          = areaInfoOld.sysopSecRA;
	    *(s32*)&tempInfo.flagsSysRA = *(s32*)&areaInfoOld.flagsSysRA;
	    tempInfo.templateSecQBBS     = areaInfoOld.templateSecQBBS;
	    *(s32*)&tempInfo.flagsTemplateQBBS = *(s32*)&areaInfoOld.flagsTemplateQBBS;
	    strncpy (tempInfo.qwkName, areaInfoOld.qwkName, 12);
	    tempInfo.minAgeSBBS      = areaInfoOld.minAgeSBBS;
	    tempInfo.attrSBBS        = areaInfoOld.attrSBBS;
	    tempInfo.replyStatSBBS   = areaInfoOld.replyStatSBBS;
	    tempInfo.aliasesQBBS     = areaInfoOld.aliasesQBBS;
	    tempInfo.groupsQBBS    = areaInfoOld.groupsQBBS;
            memcpy(areaBuf, &tempInfo, RAWECHO_SIZE);
            putRec(CFG_ECHOAREAS, areaInfoCount++);
         }
	 close (areaInfoHandle);
	 update++;
      }
   }
   *areaInfoPath = 0;
   areaInfoCount = min(MAX_AREAS, areaHeader->totalRecords);
#if 0
   for (count = 0; count < areaInfoCount; count++)
   {
      getRec(CFG_ECHOAREAS, count);
      if ( areaBuf->board > MBBOARDS )
         areaBuf->board = 0;
      if (areaBuf->board)
         boardCodeInfo[(areaBuf->board-1)>>3] |= (1<<((areaBuf->board-1)&7));
   }
#endif
   for (count = 0; count < areaInfoCount; count++)
   {
      getRec(CFG_ECHOAREAS, count);
#if 0
      if ( areaBuf->board > MBBOARDS )
	 areaBuf->board = 0;

      if (areaBuf->board || (!areaBuf->boardNumRA) ||
	  (boardCodeInfo[(areaBuf->boardNumRA-1)>>3] & (1<<((areaBuf->boardNumRA-1)&7))))
	 areaBuf->boardNumRA = 0;
      else
	 boardCodeInfo[(areaBuf->boardNumRA-1)>>3] |= (1<<((areaBuf->boardNumRA-1)&7));
#endif
      // EXPORT ADDRESS SORT ORDER CHECK!!!
      ch = 0;
      do
      {  findOk = 0;
	 count2 = 0;
	 while ( count2 < config.maxForward - 1 && areaBuf->forwards[count2+1].nodeNum.zone )
         {  if ( nodegreater(areaBuf->forwards[count2].nodeNum, areaBuf->forwards[count2+1].nodeNum) )
	    {  findOk = 1;
	       tempNodeX = areaBuf->forwards[count2];
	       areaBuf->forwards[count2] = areaBuf->forwards[count2+1];
	       areaBuf->forwards[count2+1] = tempNodeX;
               ch = 1;
	    }
	    ++count2;
	 }
      }
      while ( findOk );
      if ( ch )
         putRec(CFG_ECHOAREAS, count);

      areaBuf->comment[COMMENT_LEN-1] = 0;
      areaBuf->msgBasePath[MB_PATH_LEN-1] = 0;
      areaBuf->originLine[ORGLINE_LEN-1] = 0;
      areaBuf->address = min(areaBuf->address, (u16)MAX_AKAS);
      areaBuf->group  &= 0x03FFFFFFL;

      /* 32 AKAs alsoSeenBy */
      if ( !areaBuf->alsoSeenBy && areaBuf->_alsoSeenBy )
      {  areaBuf->alsoSeenBy = areaBuf->_alsoSeenBy;
	 areaBuf->_alsoSeenBy = 0;
      }

      /* fixes problems reported by Darr Hoag with Global functions Amgr */
      count2 = 0;
      while ( count2 < MAX_FORWARD )
      {  if ( !areaBuf->forwards[count2].nodeNum.zone )
	 {   memset(&areaBuf->forwards[count2], 0, (MAX_FORWARD-count2)*sizeof(nodeNumXType));
	     break;
	 }
	 ++count2;
      }

      if ( areaBuf->_internalUse != 'Y' )
      {  u32 temp;
	 temp = *(u32*)&areaBuf->flagsRdRA;
	 *(u32*)&areaBuf->flagsRdRA = *(u32*)&areaBuf->flagsRdNotRA;
	 *(u32*)&areaBuf->flagsRdNotRA = temp;
	 temp = *(u32*)&areaBuf->flagsWrRA;
	 *(u32*)&areaBuf->flagsWrRA = *(u32*)&areaBuf->flagsWrNotRA;
	 *(u32*)&areaBuf->flagsWrNotRA = temp;
	 temp = *(u32*)&areaBuf->flagsSysRA;
	 *(u32*)&areaBuf->flagsSysRA = *(u32*)&areaBuf->flagsSysNotRA;
	 *(u32*)&areaBuf->flagsSysNotRA = temp;
	 areaBuf->_internalUse = 'Y';
      }

      if ((tempBufPtr = malloc (sizeof(rawEchoTypeX))) == NULL)
      {
	 for (ch = 0; ch < count; ch++)
	 {  freeAreaInfo(ch);
	    free(areaInfo[ch]);
	 }
	 displayMessage ("Not enough memory available");
	 closeConfig (CFG_ECHOAREAS);
	 return (0);
      }
      if (areaBuf->group == 0) areaBuf->group = 1;

      areaInfo[count] = tempBufPtr;
      fillAreaInfo(count, areaBuf);
#if 0
      /* sorting */
      if (count && stricmp(areaBuf->areaName, areaInfo[count-1]->areaName) < 0 )
      {  pos = count-1;
	 while ( pos && stricmp(areaBuf->areaName, areaInfo[pos-1]->areaName) < 0 )
	    pos--;
	 memmove(&(areaInfo[pos+1]), (&areaInfo[pos]), (count-pos)*sizeof(rawEchoType *));
	 memcpy(areaInfo[pos] = tempBufPtr, areaBuf, RAWECHO_SIZE);
	 update = 1;
      }
      else
	 memcpy (areaInfo[count] = tempBufPtr, areaBuf, RAWECHO_SIZE);
#endif
      areaInfo[count] = tempBufPtr;

   }
// closeConfig (CFG_ECHOAREAS);

   for (count = 0; count < areaInfoCount; count++)
   {
      if ( areaInfo[count]->board > MBBOARDS )
         areaInfo[count]->board = 0;
      if (areaInfo[count]->board)
         boardCodeInfo[(areaInfo[count]->board-1)>>3] |= (1<<((areaInfo[count]->board-1)&7));
   }
   for (count = 0; count < areaInfoCount; count++)
   {
      if (areaInfo[count]->board || (!areaInfo[count]->boardNumRA) ||
          (boardCodeInfo[(areaInfo[count]->boardNumRA-1)>>3] & (1<<((areaInfo[count]->boardNumRA-1)&7))))
         areaInfo[count]->boardNumRA = 0;
      else
         boardCodeInfo[(areaInfo[count]->boardNumRA-1)>>3] |= (1<<((areaInfo[count]->boardNumRA-1)&7));
   }

   memset (&tempInfo, 0, RAWECHO_SIZE);

   if ( editAM (DISPLAY_ECHO_WINDOW, 0, NULL) )
      return (1);

   if (badEchoCount)
   {
      strcpy(stpcpy(tempStr, configPath), dBDEFNAME);
      unlink(tempStr);
   }

// do
   {
      do
      {
         if (areaInfoCount == 0)
	 {
            memset (&tempInfo, 0, RAWECHO_SIZE);
            removeWindow();
            if ( editAM (DISPLAY_ECHO_WINDOW, 0, NULL) )
	       return (1);

	    printString ("**** Empty ****", 28, 7,
	    		 windowLook.datafg, windowLook.background, MONO_HIGH);

	    printString ("F1 ", 0, 24, BROWN, BLACK, MONO_NORM);
	    printString ("Edit  ", 3, 24, MAGENTA, BLACK, MONO_NORM);
	    printString ("F3 ", 9, 24, BROWN, BLACK, MONO_NORM);
	    printString ("Global  ", 12, 24, MAGENTA, BLACK, MONO_NORM);
	    printString ("F4 ", 20, 24, BROWN, BLACK, MONO_NORM);
	    printString ("Groups  ", 23, 24, MAGENTA, BLACK, MONO_NORM);
	    printString ("F5 ", 31, 24, BROWN, BLACK, MONO_NORM);
	    printString ("Browse  ", 34, 24, MAGENTA, BLACK, MONO_NORM);
	    printString ("F6 ", 42, 24, BROWN, BLACK, MONO_HIGH);
	    printString ("Copy  ", 45, 24, MAGENTA, BLACK, MONO_NORM);
	    printString ("Ins ", 51, 24, YELLOW, BLACK, MONO_HIGH);
	    printString ("Insert  ", 55, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
	    printString ("Del ", 63, 24, BROWN, BLACK, MONO_NORM);
	    printString ("Delete    ", 67, 24, MAGENTA, BLACK, MONO_NORM);
	    printString ("\x1b \x1a", 77, 24, BROWN, BLACK, MONO_NORM);
	 }
	 else
	 {
	    if (groupSelectMask)
	    {
	       if (!((*areaInfo[index]).group & (1L<<(groupSelectMask-1))))
	       {
	          count = -1;
		  if (groupSearch)
		  {
		     if (groupSearch == 2) /* Backward */
		     {
		        count = index;
			while ((count >= 0) &&
			       !((*areaInfo[count]).group & (1L<<(groupSelectMask-1))))
			   count--;
			groupSearch--;
		     }
		     if (count < 0 && groupSearch == 1)
		     {
		        count = index;
			while ((count < areaInfoCount) &&
			       !((*areaInfo[count]).group & (1L<<(groupSelectMask-1))))
			   count++;
			if (count == areaInfoCount)
			   count = -1;
		     }
		     if (count == -1) /* Backward */
		     {
		        count = index;
			while ((count >= 0) &&
			       !((*areaInfo[count]).group & (1L<<(groupSelectMask-1))))
			   count--;
			groupSearch--;
		     }
		  }
		  if (count >= 0)
		     index = count;
		  else
		  {
		     index = 0;
		     while ((index < areaInfoCount) &&
		            !((*areaInfo[index]).group & (1L<<(groupSelectMask-1))))
		        index++;
		     if (index == areaInfoCount)
		     {
		        sprintf(tempStr, "No areas found in group %c.", '@'+groupSelectMask);
			displayMessage(tempStr);
			groupSelectMask = 0;
			index = 0;
		     }
		  }
	       }
	    }
	    groupSearch = 0;
	    sprintf (tempStr, " %*u/%u ", areaInfoCount < 10 ? 1 :
                                          areaInfoCount < 100 ? 2 : 3,
                                          index+1, areaInfoCount);
	    printString (tempStr, /*areaMenu->xWidth*/75-strlen(tempStr)+1, /*areaMenu->yWidth*/17+4, YELLOW, LIGHTGRAY, MONO_HIGH);

//          tempInfo = *areaInfo[index];
            getRec(CFG_ECHOAREAS, index);
            memcpy(&tempInfo, areaBuf, RAWECHO_SIZE);

	    areaInfoBoard = tempInfo.board;
//          count = 25;
//          while ((count > 0) &&
//                 ((tempInfo.group & (1L<<count)) == 0))
//          {
//             count--;
//          }
            editAM(DISPLAY_ECHO_DATA, 0, NULL);

            printString ("F1 ", 0, 24, YELLOW, BLACK, MONO_HIGH);
            printString ("Edit  ", 3, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
            printString ("F3 ", 9, 24, YELLOW, BLACK, MONO_HIGH);
            printString ("Global  ", 12, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
	    printString ("F4 ", 20, 24, YELLOW, BLACK, MONO_HIGH);
	    printString ("Groups  ", 23, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
	    printString ("F5 ", 31, 24, YELLOW, BLACK, MONO_HIGH);
	    printString ("Browse  ", 34, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
	    if (areaInfoCount >= MAX_AREAS)
	    {
	       printString ("F6 ", 42, 24, BROWN, BLACK, MONO_HIGH);
	       printString ("Copy  ", 45, 24, MAGENTA, BLACK, MONO_NORM);
	       printString ("Ins ", 51, 24, BROWN, BLACK, MONO_NORM);
	       printString ("Insert  ", 55, 24, MAGENTA, BLACK, MONO_NORM);
	    }
	    else
	    {
	       printString ("F6 ", 42, 24, YELLOW, BLACK, MONO_HIGH);
	       printString ("Copy  ", 45, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
	       printString ("Ins ", 51, 24, YELLOW, BLACK, MONO_HIGH);
	       printString ("Insert  ", 55, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
	    }
	    printString ("Del ", 63, 24, YELLOW, BLACK, MONO_HIGH);
	    printString ("Delete    ", 67, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
	    printString ("\x1b \x1a", 77, 24, YELLOW, BLACK, MONO_HIGH);
	 }
	 if (groupSelectMask)
	 {
	    sprintf(tempStr, " Group %c ", '@'+groupSelectMask);
	    printString(tempStr, 3, 21,
	    		windowLook.titlefg, windowLook.background, MONO_HIGH);
	    printString("อออ", 12, 21, windowLook.actvborderfg, windowLook.background, MONO_HIGH);
	 }
	 else
	    printString(" All groups ", 3, 21,
	          	windowLook.titlefg, windowLook.background, MONO_HIGH);

	 if (badEchoCount)
	    ch = _K_INS_;
	 else
	    ch=readKbd();

	 newEcho = badEchoCount;

	 if ((areaInfoCount != 0) || (ch == _K_INS_))
	 {
	    am__cp = 0;

	    findOk = 0;
	    switch (ch)
	    {
	       case _K_CTLF_ :
	       case _K_ALTF_ :
               case _K_F5_ :
               case _K_F9_ :  if ((count = areasList(index, groupSelectMask)) != -1) /* F5 */
                                 index = count;
                              break;
	       case _K_ENTER_ :
	       case _K_F1_ :  if (areaInfoCount == 0)
                                 break;  /* F1 */
	 	              if (tempInfo.options.disconnected)
                              {
                                 tempInfo.options.disconnected = 0;
                                 *tempInfo.comment = 0;
                              }
                              pos = index;
                              do
			      {
                                 update |= (ch = editAM(EDIT_ECHO, 0, NULL));
				 if (ch)
                                 {
                                    editAM(DISPLAY_ECHO_DATA, 0, NULL);
                                    if (strlen(tempInfo.areaName) == 0)
                                    {
                                       ch = askBoolean ("Area name field is empty. Discard changes ?", 'N');
	 		            }
	 			    else
	 		            {
	 			       index = 0;
                                       while ( (index < areaInfoCount) &&
                                               (strcmp (tempInfo.areaName,
                                                areaInfo[index]->areaName) > 0) )
	 			       {
	 			          index++;
				       }
	 			       if ((index < areaInfoCount) &&
                                           !strcmp(tempInfo.areaName,
						   areaInfo[index]->areaName) &&
                                            index != pos )
	 			       {
	 			          ch = askBoolean ("Area name is already in use. Discard changes ?", 'N');
	 			       }
	 			    }
	 			 }
	 	                 else ch = 'Y';
	 		      }
	 	              while ((ch == 'N') || (ch == _K_ESC_));

                              if (ch == 'Y')
			      {
	 		         index = pos;
	 		      }
	 		      else
                              {  freeAreaInfo(pos);
                                 fillAreaInfo(pos, &tempInfo);
//??                             memcpy(areaBuf, &tempInfo, RAWECHO_SIZE);
                                 if ( pos != index )
                                 {
				    areaHelpPtr = areaInfo[pos];
				    memcpy (&areaInfo[pos],
					    &areaInfo[pos+1],
					    (areaInfoCount-pos-1) *
					    sizeof(areaInfoPtr));
				    delRec(CFG_ECHOAREAS, pos);

				    if (pos < index)
				       index--;

				    memmove (&areaInfo[index+1],
					     &areaInfo[index],
					     (areaInfoCount-index) *
					     sizeof(areaInfoPtr));
				    areaInfo[index] = areaHelpPtr;
				    memcpy(areaBuf, &tempInfo, RAWECHO_SIZE);
                                    insRec(CFG_ECHOAREAS, index);
				 }
				 else
                                 {
                                    memcpy(areaBuf, &tempInfo, RAWECHO_SIZE);
                                    putRec(CFG_ECHOAREAS, index);
                                 }
                              }
	                      break;
               case _K_F4_ :  displayGroups(); /* F4 */
			      do
                              {
			       	 ch=toupper(readKbd());
                              }
                              while ((ch < 'A' || ch > 'Z') && ch != _K_ENTER_ && ch != _K_ESC_);
                              removeWindow();

			      if (ch >= 'A' && ch <= 'Z')
                                 groupSelectMask = ch - '@';
                              else
                                 if (ch == _K_ENTER_)
                                    groupSelectMask = 0;
                              ch = 0;
                              break;
               case _K_F3_ :
                              memset(&tempInfo, 0, sizeof(tempInfo));
                              update |= editAM(EDIT_ECHO_GLOBAL, 1, areaBuf);
                              getRec(CFG_ECHOAREAS, index);
                              memcpy(&tempInfo, areaBuf, RAWECHO_SIZE);
                              break;
               case _K_CPGUP_ :
               case _K_HOME_ :
                              index = 0;                         /* Home */
                              break;
	       case _K_LEFT_ :
			      if (index > 0)                     /* Left Arrow */
                                 index--;
                              groupSearch = 2;
                              break;
               case _K_RIGHT_ :
                              if (index < areaInfoCount-1)      /* Right Arrow */
                                 index++;
			      groupSearch = 1;
			      break;
	       case _K_CPGDN_ :
	       case _K_END_ :
			      index = areaInfoCount - 1;        /* End */
			      groupSearch = 2;
			      break;
	        case _K_F6_ :
                case _K_INS_: if (areaInfoCount >= MAX_AREAS)
                                 break;                          /* Insert */

                              am__cp = 1;
                              if (ch == _K_F6_)
			      {  if (areaInfoCount == 0)
			            break;
                                 tempInfo.board = echoDefaultsRec.board;
				 tempInfo.boardNumRA = 0;
			      }
			      else
                              {
                                 memcpy (&tempInfo, &echoDefaultsRec, RAWECHO_SIZE);

				 if (groupSelectMask)
				    tempInfo.group = (1L<<(groupSelectMask-1));
				 else
				    if (!tempInfo.group)
                                       tempInfo.group = 1;

				 tempInfo.options.active = 1;

				  if (badEchoCount)
          {
            strncpy(tempInfo.areaName, badEchos[--badEchoCount].badEchoName, ECHONAME_LEN-1);
				    strncpy(tempInfo.comment , badEchos[  badEchoCount].badEchoName, ECHONAME_LEN-1);
				 // strupr(tempInfo.areaName);
            tempInfo.forwards[0].nodeNum = badEchos[badEchoCount].srcNode;
				    tempInfo.address = badEchos[badEchoCount].destAka;

            if (tempInfo.board == 2) // JAM
            {
              MakeJamAreaPath(&tempInfo);
              for ( ; ; )
              {
                idx = 0;
                while ( idx < areaInfoCount )
                {
                  if ( *areaInfo[idx]->msgBasePath && !stricmp(areaInfo[idx]->msgBasePath, tempInfo.msgBasePath) )
                    break;
                  ++idx;
                }
                if ( idx == areaInfoCount )
                  break;
                tempPtr = strchr(tempInfo.msgBasePath, 0) - 1;
                for ( ; ; )
                {  if ( !isdigit(*tempPtr) )
                      *tempPtr = '0';
                   else
                   {  if ( *tempPtr != '9' )
                         ++*tempPtr;
                      else
                      {  *tempPtr = '0';
                         --tempPtr;
                         if ( tempPtr >= tempInfo.msgBasePath && *tempPtr != '\\' )
                            continue;
                      }
                   }
                   break;
                }
              }
            }
            update = 1;
				 }
      }
                              tempInfo.boardNumRA = 0;

                              areaInfoBoard = 0;

			      if (tempInfo.board == 1) /* Hudson */
			      {
                                 memset (usedArea, 0, MBBOARDS);
				 for (count = 0; count < areaInfoCount; count++)
				 {
                                    if (areaInfo[count]->board - 1 < MBBOARDS)
				        usedArea[areaInfo[count]->board-1] = 1;
                                    if (areaInfo[count]->boardNumRA - 1 < MBBOARDS)
				       usedArea[areaInfo[count]->boardNumRA-1] = 1;
				 }

                                 count = 0;
                                 do
				 {  count++;
                                    count2 = 0;
                                    while ( count2 < MAX_NETAKAS )
                                    {  if ( count == config.netmailBoard[count2] )
                                          break;
                                       count2++;
                                    }
                                 }
                                 while ( (count <= MBBOARDS) &&
                                         (count2 < MAX_NETAKAS || (usedArea[count-1] == 1) ||
				       	 (count == config.badBoard) ||
                                         (count == config.dupBoard) ||
                                         (count == config.recBoard)) );
//					 (count == config.netmailBoard[0]) || (count == config.netmailBoard[1]) ||
//					 (count == config.netmailBoard[2]) || (count == config.netmailBoard[3]) ||
//					 (count == config.netmailBoard[4]) || (count == config.netmailBoard[5]) ||
//                                       (count == config.netmailBoard[6]) || (count == config.netmailBoard[7]) ||
//					 (count == config.netmailBoard[8]) || (count == config.netmailBoard[9]) ||
//                                       (count == config.netmailBoard[10])))

                                 tempInfo.board = (u16)((count <= MBBOARDS) ? count:0);
                                 *tempInfo.msgBasePath = 0;
			      }
                              else
                                 if (tempInfo.board == 2) /* JAM */
                                    tempInfo.board = 0;
			         else /* pass-through */
                                    *tempInfo.msgBasePath = 0;

                              if ( newEcho )
                              {
                                 editAM(DISPLAY_ECHO_DATA, 0, NULL);
                                 if ( (ch = askBoolean("Add this area?", 'Y')) != 'Y' )
                                    break;
                              }
			      pos = index;
			      do
                              {
                                 update |= (ch = editAM(EDIT_ECHO, 0, NULL));
                                 if (ch || newEcho)
                                 {
                                    editAM(DISPLAY_ECHO_DATA, 0, NULL);
                                    if (strlen(tempInfo.areaName) == 0)
			            {
				       ch = askBoolean ("Area name field is empty. Discard current record ?", 'N');
				    }
				    else if ( (count = strlen(tempInfo.msgBasePath))!= 0 &&
					      tempInfo.msgBasePath[count-1] == '\\' )
                                    {
                                       ch = askBoolean ("Invalid JAM base name. Discard current record ?", 'N');
                                    }
                                    else
                                    {
                                       index = 0;
				       while ((index < areaInfoCount) &&
				              (strcmp (tempInfo.areaName,
                                                      areaInfo[index]->areaName) > 0))
				       {
                                          index++;
                                       }
                                       if ((index < areaInfoCount) &&
                                           (strcmp (tempInfo.areaName,
                                                    areaInfo[index]->areaName) == 0))
                                       {
                                          ch = askBoolean ("Area name is already in use. Discard current record ?", 'N');
				       }
				    }
			      	 }
                                 else ch = 'Y';
			      }
                              while ((ch == 'N') || (ch == _K_ESC_));

			      if (ch == 'Y')
                              {
			         index = pos;
                              }
			      else
                              {
                                 if (index < areaInfoCount++)
                                 {
                                    memmove (&areaInfo[index+1],
                                             &areaInfo[index],
                                             (areaInfoCount-index) *
				             sizeof(areaInfoPtr));
				 }

                                 if ((areaInfo[index] =
                                      malloc (sizeof(rawEchoTypeX))) == NULL)
                                    return (0);
				 memcpy(areaBuf, &tempInfo, RAWECHO_SIZE);

                                 fillAreaInfo(index, areaBuf);
                                 insRec(CFG_ECHOAREAS, index);
                              }
                              break;
	        case _K_DEL_: if ((areaInfoCount == 0) ||         /* Delete */
                                  (askBoolean ("Delete this entry ?", 'Y') != 'Y')) break;
                              if (tempInfo.msgBasePath)
                                 askRemoveJAM(tempInfo.msgBasePath);
                              delRec(CFG_ECHOAREAS, index);
                              update++;
                              freeAreaInfo(index);
                              free(areaInfo[index]);
                              memmove (&areaInfo[index], &areaInfo[index+1],
                                      (--areaInfoCount-index) *
                                      sizeof(areaInfoPtr));
                              if (index == areaInfoCount)
				 index--;
                              groupSearch = 1;
                              break;
                default:      if (ch >= 256 || !isgraph(ch))
                                 break;
                              findOk++;
                              if (findPos < sizeof(areaNameType)-1)
                              {
                                 findStr[findPos++] = toupper(ch);
                                 findStr[findPos] = 0;
                              }
                              index = 0;
/*                            if (groupSelectMask)
                                 while ((index < areaInfoCount) &&
                                        (!((*areaInfo[index]).group & (1L<<(groupSelectMask-1))) ||
                                         (strncmp(areaInfo[index]->areaName, findStr, findPos) < 0)))
                                 {
				    index++;
				 }
                              else
*/
                              while ((index < areaInfoCount) &&
                                     (strncmp(areaInfo[index]->areaName, findStr, findPos) < 0))
                              {
                                 index++;
                              }
                              groupSearch = 1;
/*
                              if ((findPos == 1) &&
                                  ((index == areaInfoCount) ||
                                   (*areaInfo[index]->areaName > *findStr)))
                              {
                                 index = 0;
                                 findPos = 0;
                              }
*/
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!       vv == gaat fout, waarom??? */
                              if (index >= areaInfoCount)
                                 index = areaInfoCount-1;
                              break;
            }
            if (!findOk)
            {
               findOk = findPos = 0;
            }
         }
      }
      while ((ch != _K_ESC_) && (ch != _K_F7_) && (ch != _K_F10_));
//    if ( ch == _K_F7_ || ch == _K_F10_ )
//       ch = 'Y';
//    else
//       if (update)
//          ch = askBoolean ("Save changes in area setup ?", 'Y');
//       else
//          ch = 0;
   }
// while (ch == _K_ESC_);

   closeConfig(CFG_ECHOAREAS);

   removeWindow ();

//   if (ch == 'Y')
   if ( update )
   {
      working ();

      /* remove old style config file */
      if (*areaInfoPath) unlink (areaInfoPath);

      if (openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf))
      {
	 headerType areaHeader2;
	 fhandle    fml102handle;
         nodeNumType *nodeNumBuf;

         if ( (nodeNumBuf = malloc(MAX_FORWARDOLD * sizeof(nodeNumType))) == NULL )
            goto error;
         if ( *config.autoFMail102Path )
         {  if ( !stricmp(config.autoFMail102Path, configPath) )
               displayMessage("AutoExport for FMail 1.02 format should be set to another directory");
	    else
	    if ( (fml102handle = open(strcpy(stpcpy(tempStr, config.autoFMail102Path), dARFNAME), O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE)) != -1 )
	    {
	       areaHeader2 = *areaHeader;
	       strcpy(strchr(areaHeader2.versionString, '\x1a'), "/conv\x1a");
	       areaHeader2.revNumber = 0x0100;
	       areaHeader2.recordSize = sizeof(rawEchoType102);
	       write(fml102handle, &areaHeader2, sizeof(headerType));
	       for (count = 0; count < areaInfoCount; count++)
	       {
		  getRec(CFG_ECHOAREAS, count);

		  if ( areaBuf->address > 15 )
		     areaBuf->address = 0;
		  areaBuf->_internalUse = 0;
		  {  u32 temp;
		     temp = *(u32*)&areaBuf->flagsRdRA;
		     *(u32*)&areaBuf->flagsRdRA = *(u32*)&areaBuf->flagsRdNotRA;
		     *(u32*)&areaBuf->flagsRdNotRA = temp;
		     temp = *(u32*)&areaBuf->flagsWrRA;
		     *(u32*)&areaBuf->flagsWrRA = *(u32*)&areaBuf->flagsWrNotRA;
		     *(u32*)&areaBuf->flagsWrNotRA = temp;
//		     temp = *(u32*)&areaBuf->flagsSysRA;
//		     *(u32*)&areaBuf->flagsSysRA = *(u32*)&areaBuf->flagsSysNotRA;
//		     *(u32*)&areaBuf->flagsSysNotRA = temp;
		  }
		  areaBuf->_alsoSeenBy = (u16)areaBuf->alsoSeenBy;
		  for ( count2 = 0; count2 < MAX_FORWARDOLD; count2++ )
		  nodeNumBuf[count2] = areaBuf->forwards[count2].nodeNum;
		  memmove((uchar*)areaBuf + offsetof(rawEchoType, readSecRA) + MAX_FORWARDOLD * sizeof(nodeNumType),
			 (uchar*)areaBuf + offsetof(rawEchoType, readSecRA),
			 offsetof(rawEchoType, forwards) - offsetof(rawEchoType, readSecRA));
		  memcpy(&areaBuf->readSecRA, nodeNumBuf, MAX_FORWARDOLD * sizeof(nodeNumType));
		  write(fml102handle, areaBuf, sizeof(rawEchoType102));
	       }
	       close(fml102handle);
	    }
	 }
	 closeConfig(CFG_ECHOAREAS);
	 free(nodeNumBuf);
      }
error:
      boardEdit = 1;
   }

   for (count = 0; count < areaInfoCount; count++)
   {  freeAreaInfo(count);
      free (areaInfo[count]);
   }

// free (gSwMenu);
// free (gMnMenu);
// free (gNdMenu);
// free (aglMenu);

   return update;
}

