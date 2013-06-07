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
#include <dos.h>
#include <io.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fmail.h"
#include "fs_func.h"
#include "window.h"
#include "cfgfile.h"
#include "nodemgr.h"
#include "fs_util.h"
#include "nodeinfo.h"

#define dARROW "ออ"


typedef struct
{
   u16             signature; /* contains "ND" */
   u16             writeLevel;
   nodeNumType     node;
   nodeNumType     viaNode;
   u16             capability;
   nodeOptionsType options;
   u16             archiver;
   u32             groups;
   u16             outStatus;
   uchar           sysopName[36];
   uchar           password[18];
   uchar           packetPwd[9];
   uchar           useAka;        } nodeInfoType102;



#define MAX_NL_WINSIZE 13


extern windowLookType windowLook;

extern char configPath[128];
extern s32          pow2[32];
//extern u16          nodeInfoCount;
//extern nodeInfoType *nodeInfo;

funcParType groupsSelect;

extern configType  config;



s16 nodesList(s16 currentElem)
{
   u16      elemCount = 0;
   u16      windowBase = 0;
   u16      count;
   s16      ch;
   tempStrType tempStr;

   saveWindowLook();
   windowLook.background = CYAN;
   windowLook.actvborderfg = BLUE;

   windowBase = max(0, currentElem-MAX_NL_WINSIZE/2);
   if ((nodeInfoCount >= MAX_NL_WINSIZE) &&
       (windowBase+MAX_NL_WINSIZE > nodeInfoCount))
   {
      windowBase = nodeInfoCount-MAX_NL_WINSIZE;
   }

   elemCount = nodeInfoCount;

   if (displayWindow (" Nodes ", 10, 7, 73, 8+MAX_NL_WINSIZE) != 0)
   {
      restoreWindowLook();
      return (0);
   }

   do
   {
      for (count = 0; count < MAX_NL_WINSIZE; count++)
      {
         if (windowBase+count < elemCount)
         {
            if (nodeInfo[windowBase+count]->node.point)
               sprintf (tempStr, " %35s  %u:%u/%u.%u",
                                 nodeInfo[windowBase+count]->sysopName,
                                 nodeInfo[windowBase+count]->node.zone,
                                 nodeInfo[windowBase+count]->node.net,
				 nodeInfo[windowBase+count]->node.node,
                                 nodeInfo[windowBase+count]->node.point);
            else
               sprintf (tempStr, " %35s  %u:%u/%u",
                                 nodeInfo[windowBase+count]->sysopName,
                                 nodeInfo[windowBase+count]->node.zone,
                                 nodeInfo[windowBase+count]->node.net,
                                 nodeInfo[windowBase+count]->node.node);
         }
         else
            *tempStr = 0;

	 if ((elemCount != 0) && (windowBase+count == currentElem))
         {
            printStringFill (tempStr, ' ', 62, 11, 8+count,
                             YELLOW, BLACK, MONO_INV);
         }
         else
         {
            printStringFill (tempStr, ' ', 62, 11, 8+count,
			     BLUE, CYAN, MONO_NORM);
         }
      }
      if (elemCount == 0)
      {
         printString ("Empty", 38, 10, CYAN, BLUE, MONO_NORM);
      }

      ch=readKbd();
      switch (ch)
      {
         case _K_PGUP_ :
                       if (windowBase >= MAX_NL_WINSIZE-1)       /* PgUp */
		       {
                          windowBase -= MAX_NL_WINSIZE-1;
                          currentElem -= MAX_NL_WINSIZE-1;
                       }
                       else
                       {
                          windowBase = currentElem = 0;
                       }
		       break;
         case _K_PGDN_ :
                       if (windowBase+2*(MAX_NL_WINSIZE-1) <= elemCount) /* PgDn */
                       {
                          windowBase += MAX_NL_WINSIZE-1;
                          currentElem += MAX_NL_WINSIZE-1;
                       }
                       else
                       {
                          if (elemCount > MAX_NL_WINSIZE)
                          {
                             windowBase = elemCount - MAX_NL_WINSIZE;
                          }
			  currentElem = elemCount-1;
                       }
                       break;
         case _K_CPGUP_ :
         case _K_HOME_:
                       windowBase = currentElem = 0;        /* Home */
                       break;
         case _K_CPGDN_ :
         case _K_END_: if (elemCount > MAX_NL_WINSIZE)      /* End */
                       {
                          windowBase = elemCount - MAX_NL_WINSIZE;
		       }
                       currentElem = elemCount-1;
                       break;
         case _K_UP_ : if (currentElem > 0)                 /* Cursor Up */
                       {
                          if (--currentElem < windowBase)
                             windowBase--;
                       }
                       break;
         case _K_DOWN_ :
                       if (currentElem+1 < elemCount)       /* Cursor Down */
                       {
                          if (++currentElem >= windowBase + MAX_NL_WINSIZE)
			     windowBase++;
                       }
                       break;
      }
   }
   while ((ch != _K_ESC_) && (ch != _K_ENTER_));

   restoreWindowLook();
   removeWindow ();

   if (ch == _K_ENTER_)
   {
      return (currentElem);
   }
   return (-1);
}


typedef struct
{  u8  *desc;
   u16 flags;
   u16 index;
} ldType;


#define MAX_LD_WINSIZE 12
ldType listDataArray[MAX_AREAS];
ldType listDataArray2[MAX_AREAS];

static s16 listData(u16 *totalElem, ldType listDataArray[MAX_AREAS], u16 *totalElem2, ldType listDataArray2[MAX_AREAS])
{
   u16            count;
   u16            windowBase = 0;
   u16            currentElem = 0;
   s16            ch = 0;
   tempStrType    tempStr;
   s16            xu, xu2;

   if ( listDataArray2 != NULL )
   {  saveWindowLook();
      windowLook.background = CYAN;
      windowLook.actvborderfg = BLUE;
   }

   if (displayWindow (listDataArray2 != NULL ? " Active areas ": " Non-active areas " , 18, 8, 73, 9+MAX_LD_WINSIZE) != 0)
   {
      if ( listDataArray2 != NULL )
         restoreWindowLook();
      return (0);
   }

   do
   {
      for ( count = 0; count < MAX_LD_WINSIZE; count++ )
      {
         if ( windowBase+count < *totalElem )
            strcpy(tempStr, listDataArray[windowBase+count].desc);
         else
            *tempStr = 0;

         if ( (*totalElem != 0) && (windowBase+count == currentElem) )
            printStringFill ( tempStr, ' ', 54, 19, 9+count,
                              YELLOW, BLACK, MONO_INV );
         else
            printStringFill ( tempStr, ' ', 54, 19, 9+count,
                              BLUE, CYAN, MONO_NORM );
      }
//      if ( *totalElem == 0 )
//         printString("No active areas", 42, 13, CYAN, BLUE, MONO_NORM);

      ch=readKbd();
      switch(ch)
      {
         case _K_INS_:     if ( listDataArray2 != NULL )
                           {  xu2 = listData(totalElem2, listDataArray2, NULL, NULL);
                              if ( !*totalElem2 || xu2 < 0 )
                                 break;
                              xu = 0;
                              while ( xu < *totalElem )
                              {  if ( strcmp(listDataArray[xu].desc, listDataArray2[xu2].desc) >= 0 )
                                    break;
                                 xu++;
                              }
                              if ( xu < MAX_AREAS - 1 )
                                 memmove(&listDataArray[xu + 1], &listDataArray[xu], (MAX_AREAS - xu - 1) * sizeof(ldType));
                              listDataArray[xu] = listDataArray2[xu2];
                              memcpy(&listDataArray2[xu2], &listDataArray2[xu2 + 1], (MAX_AREAS - xu2 - 1) * sizeof(ldType));
                              ++*totalElem;
                              --*totalElem2;
                           }
                           break;
         case _K_DEL_:     if ( listDataArray2 != NULL )
                           {  xu2 = 0;
                              while ( xu2 < *totalElem2 )
                              {  if ( strcmp(listDataArray2[xu2].desc, listDataArray[currentElem].desc) >= 0 )
                                    break;
                                 xu2++;
                              }
                              if ( xu2 < MAX_AREAS - 1 )
                                 memmove(&listDataArray2[xu2 + 1], &listDataArray2[xu2], (MAX_AREAS - xu2 - 1) * sizeof(ldType));
                              listDataArray2[xu2] = listDataArray[currentElem];
                              memcpy(&listDataArray[currentElem], &listDataArray[currentElem + 1], (MAX_AREAS - currentElem - 1) * sizeof(ldType));
                              --*totalElem;
                              ++*totalElem2;
                           }
                           break;
         case _K_PGUP_ :   if ( windowBase >= MAX_LD_WINSIZE-1 )       /* PgUp */
                           {  windowBase -= MAX_LD_WINSIZE-1;
                              currentElem -= MAX_LD_WINSIZE-1;
                           }
                           else
                              windowBase = currentElem = 0;
                           break;
         case _K_PGDN_ :   if ( windowBase+2*(MAX_LD_WINSIZE-1) <= *totalElem ) /* PgDn */
                           {  windowBase += MAX_LD_WINSIZE-1;
                              currentElem += MAX_LD_WINSIZE-1;
                           }
                           else
                           {  if ( *totalElem > MAX_LD_WINSIZE )
                                windowBase = *totalElem - MAX_LD_WINSIZE;
                              currentElem = *totalElem-1;
                           }
                           break;
         case _K_CPGUP_:
         case _K_HOME_ :   windowBase = currentElem = 0;        /* Home */
                           break;
         case _K_CPGDN_ :
         case _K_END_   :  if ( *totalElem > MAX_LD_WINSIZE )      /* End */
                              windowBase = *totalElem - MAX_LD_WINSIZE;
                           currentElem = *totalElem-1;
                           break;
         case _K_UP_ :     if (currentElem > 0)                 /* Cursor Up */
                           {  if ( --currentElem < windowBase )
                                 windowBase--;
                           }
                           break;
         case _K_DOWN_ :   if ( currentElem+1 < *totalElem )       /* Cursor Down */
                              if ( ++currentElem >= windowBase + MAX_LD_WINSIZE )
                                 windowBase++;
                           break;
         default:          if ( (ch > 32) && (ch < 256) )
                           {  ch = toupper(ch);
                              count = currentElem;
                              do
                              {  if ( ++count >= *totalElem )
                                    count = 0;
                              }
                              while ( (count != currentElem) &&
                                      (*listDataArray[count].desc != ch) );
                              if ( count != currentElem )
                              {  currentElem = count;
                                 if ( currentElem < windowBase )
                                    windowBase = currentElem;
                                 if ( currentElem >= windowBase+MAX_LD_WINSIZE )
                                    windowBase = currentElem-MAX_LD_WINSIZE+1;
                              }
                           }
                           break;
      }
   }
   while ( (ch != _K_ESC_) && (ch != _K_ENTER_) );

   if ( listDataArray2 != NULL )
      restoreWindowLook();

   removeWindow();

   if ( ch == _K_ENTER_ )
      return currentElem;

   return -1;
}

nodeInfoType tempInfoN, updInfoN;

s16 nodeNGOnly (s16 editType, s16 code)
{
   if ((editType == EDIT_NODE) || (editType == DISPLAY_NODE_DATA) ||
                                  (editType == DISPLAY_NODE_WINDOW))
      return (code);
   else
      return (DISPLAY);
}


u16 editNM(s16 editType, u16 setdef)
{
   menuType     *nodeMenu;
   u16          count;
   toggleType   pktTypeToggle;
   toggleType   archiveToggle;
   toggleType   addressToggle;
   toggleType   statusToggle;
   s16          update = 0;
   s16          total = 0;
   char         addressText[MAX_AKAS+1][34];
   char         tempStr[24];
   char         ch;
   char         *helpPtr1, *helpPtr2;

   groupsSelect.numPtr = (u16*)&tempInfoN.groups;
   groupsSelect.f      = askGroups;

   pktTypeToggle.data    = (char*)&tempInfoN.capability;
   pktTypeToggle.text[0] = "Stone Age";
   pktTypeToggle.retval[0] = 0;
   pktTypeToggle.text[1] = "Type 2+";
   pktTypeToggle.retval[1] = 1;

   archiveToggle.data    = (char*)&tempInfoN.archiver;
   archiveToggle.text[0] = "ARC";
   archiveToggle.retval[0] = 0;
   archiveToggle.text[1] = "ZIP";
   archiveToggle.retval[1] = 1;
   archiveToggle.text[2] = "LZH";
   archiveToggle.retval[2] = 2;
   archiveToggle.text[3] = "PAK";
   archiveToggle.retval[3] = 3;
   archiveToggle.text[4] = "ZOO";
   archiveToggle.retval[4] = 4;
   archiveToggle.text[5] = "ARJ";
   archiveToggle.retval[5] = 5;
   archiveToggle.text[6] = "SQZ";
   archiveToggle.retval[6] = 6;
   archiveToggle.text[7] = "UC2";
   archiveToggle.retval[7] = 8;
   archiveToggle.text[8] = "RAR";
   archiveToggle.retval[8] = 9;
   archiveToggle.text[9] = "JAR";
   archiveToggle.retval[9] = 10;
   archiveToggle.text[10] = "Custom";
   archiveToggle.retval[10] = 7;
   archiveToggle.text[11] = "None";
   archiveToggle.retval[11] = 0xFF;

   statusToggle.data    = (char*)&tempInfoN.outStatus;
   statusToggle.text[0] = "None";
   statusToggle.retval[0] = 0;
   statusToggle.text[1] = "Hold";
   statusToggle.retval[1] = 1;
   statusToggle.text[2] = "Crash";
   statusToggle.retval[2] = 2;
   statusToggle.text[3] = "Hold/Direct";
   statusToggle.retval[3] = 3;
   statusToggle.text[4] = "Crash/Direct";
   statusToggle.retval[4] = 4;
   statusToggle.text[5] = "Direct";
   statusToggle.retval[5] = 5;

   addressToggle.data = (char*)&tempInfoN.useAka;
   addressToggle.text[0] = "Auto";
   addressToggle.retval[0] = 0;
   for (count = 0; count < MAX_AKAS; count++)
   {
      if (config.akaList[count].nodeNum.zone != 0)
         strcpy(addressText[count], nodeStr((nodeNumType*)(&config.akaList[count])));
      else
         if (count == 0)
            strcpy(addressText[0], "Main (not defined)");
	 else
            sprintf(addressText[count], "AKA %2u (not defined)", count);
      addressToggle.text[count+1] = addressText[count];
      addressToggle.retval[count+1] = count+1;
   }

   if ((nodeMenu = createMenu (" Node Manager ")) == NULL) return(0);
   addItem (nodeMenu, TEXT, "SysOp Name", 0, tempInfoN.sysopName, sizeof(tempInfoN.sysopName)-1, 0,
                      "Name of the SysOp");
   addItem (nodeMenu, DATE|DISPLAY, "Ref", 52, &tempInfoN.referenceLNBDat, 0, 0,
                      "");
   addItem (nodeMenu, nodeNGOnly(editType, NODE), "System", 0, &tempInfoN.node, 0, 0,
                      "Node number");
   addItem (nodeMenu, DATE|DISPLAY, "LastRcvd", 47, &tempInfoN.lastMsgRcvdDat, 0, 0,
                      "Last time a message was received from this node");
   addItem (nodeMenu, NODE, "Via system", 0, &tempInfoN.viaNode, 0, 0,
                      "Address where to route mail for this node to");
   addItem (nodeMenu, DATE|DISPLAY, "LastSent", 47, &tempInfoN.lastMsgSentDat, 0, 0,
                      "Last time a message was sent to this node");
   addItem (nodeMenu, ENUM_INT, "Use AKA", 0, &addressToggle, 0, MAX_AKAS+1,
                      "Which AKA should be used in PKT files for this node (NORMALLY USE AUTO)");
   addItem (nodeMenu, DATE|DISPLAY, "New bundle", 45, &tempInfoN.lastNewBundleDat, 0, 0,
                      "Last time a new bundle was created for this node");
   addItem (nodeMenu, FUNC_PAR, "Groups", 0, &groupsSelect, 26, 0,
                      "Groups of message areas available to a node");
   addItem (nodeMenu, DATE|DISPLAY, "Last BCL", 47, &tempInfoN.lastSentBCL, 0, 0,
                      "Last time a BCL file was sent to this node");

   addItem (nodeMenu, NUM_INT, "Write level", 0, &tempInfoN.writeLevel, 5, 32767,
                      "Level should be >= than Write Level of an area in order to be allowed to write");
   addItem (nodeMenu, ENUM_INT, "Capability   ", 22, &pktTypeToggle, 0, 2,
                      "Capability of the echomail processor used at the remote node");
   addItem (nodeMenu, ENUM_INT, "Compression", 0, &archiveToggle, 0, 12,
                      "Compression method used to create mailbundles");
   addItem (nodeMenu, ENUM_INT, "Attach status", 22, &statusToggle, 0, 6,
                      "Status of file attach messages");
   addItem (nodeMenu, PATH|UPCASE, "PKT path", 0, tempInfoN.pktOutPath, PKTOUT_PATH_LEN-1, 0,
                      "Directory where uncompressed PKT files should be stored");
   addItem (nodeMenu, EMAIL, "E-mail", 0, tempInfoN.email, PKTOUT_PATH_LEN-1, 0,
                      "E-mail address where mail bundles should be sent to");
   addItem (nodeMenu, BOOL_INT, "Active", 0, &tempInfoN.options, BIT4, 0,
                      "Not Active means that a node will only receive echomail directed to SysOp Name");
   addItem (nodeMenu, BOOL_INT, "Forw.requests", 22, &tempInfoN.options, BIT12, 0,
                      "Automatically send requests by this node for new areas to your uplinks");
   addItem (nodeMenu, BOOL_INT, "Rem.maint       ", 45, &tempInfoN.options, BIT13, 0,
                      "If this node is allowed to perform remote maintenance");
   addItem (nodeMenu, BOOL_INT, "Notify", 0, &tempInfoN.options, BIT15, 0,
                      "Send list of active areas to this node with the FTools Notify command");
   addItem (nodeMenu, BOOL_INT, "Allow rescan ", 22, &tempInfoN.options, BIT14, 0,
                      "If this node is allowed to use %RESCAN");
   addItem (nodeMenu, BOOL_INT, "Tiny SEEN-BYs   ", 45, &tempInfoN.options, BIT1, 0,
                      "Remove all SEEN-BYs except of your own downlinks (should normally NOT be used)");
   addItem (nodeMenu, BOOL_INT, "Pack netm.", 0, &tempInfoN.options, BIT7, 0,
		       "Pack netmail direct for this node");
   addItem (nodeMenu, BOOL_INT, "Route point  ", 22, &tempInfoN.options, BIT6, 0,
                      "Reroute netmail for this point directed to a local AKA to the point number");
   addItem (nodeMenu, BOOL_INT, "Reformat date   ", 45, &tempInfoN.options, BIT0, 0,
                      "Correct the format of dates in forwarded messages (should normally NOT be used)");
   addItem (nodeMenu, DISPLAY, NULL, 0, NULL, 0, 0, NULL);
   addItem (nodeMenu, NUM_INT, "Auto BCL days   ", 45, &tempInfoN.autoBCL, 3, 999,
                      "Interval in days between automatic BCL files");
   addItem (nodeMenu, WORD, "AreaMgr pwd", 0, &tempInfoN.password, sizeof(tempInfoN.password)-2, 0,
		      "Password for AreaMgr requests");
   addItem (nodeMenu, NUM_INT, "AutoPassive days", 45, &tempInfoN.passiveDays, 4, 9999,
                      "Max number of days between polls before auto-passive (1-9999, 0 = no max)");
   addItem (nodeMenu, WORD|UPCASE, "Packet pwd", 0, &tempInfoN.packetPwd, 8, 0,
                      "Password put in outgoing mail packets and required in incoming mail packets");
   addItem (nodeMenu, BOOL_INT, dARROW" Ignore   ", 22, &tempInfoN.options, BIT3, 0,
                      "Put password in outgoing packets but do not check incoming packets for it");
   addItem (nodeMenu, NUM_INT, "AutoPassive size", 45, &tempInfoN.passiveSize, 4, 9999,
		       "Max size of mail bundle before auto-passive (1-9999, 0 = no max)");

   switch (editType)
   {
      case DISPLAY_NODE_WINDOW : update = displayMenu (nodeMenu, 2, 5);
               		         break;
      case DISPLAY_NODE_DATA   : displayData (nodeMenu, 2, 5, 0);
      				 update = 0;
				 break;
      case EDIT_NODE           : update = runMenuD(nodeMenu, 2, 5, NULL, setdef);
      		           	 break;
      case EDIT_NODE_GLOBAL    : runMenuD(nodeMenu, 2, 5, NULL, setdef);
                                 update = 0;
                                 break;
//    case EDIT_DEFAULT        : update = runMenuD(nodeMenu, 2, 5, NULL, setdef);
//      			 break;
   }
   if ( setdef )
   {  total = 0;
      for ( count = 0; count < nodeMenu->entryCount; count++ )
      {  total += (nodeMenu->menuEntry[count].selected != 0);
      }
      if ( total )
      {  *tempStr = 0;
         do
         {
            update = 0;
            printStringFill ("Allowed wildcards are * (at the end only) and ?", ' ', 80, 0, 24,
                             windowLook.commentfg, windowLook.commentbg,
                             MONO_NORM);
            if (displayWindow (" Node number mask ", 10, 12, 12+23, 14) != 0) return (0);
            ch = editString(tempStr, 23, 12, 13, TEXT);
            removeWindow ();
            if ( ch == _K_ESC_ )
               break;
            helpPtr1 = tempStr;
            while ( *helpPtr1 )
            {  if ( (!isdigit(*helpPtr1) && *helpPtr1 != '?' && *helpPtr1 != '*' &&
                    *helpPtr1 != ':' && *helpPtr1 != '/' && *helpPtr1 != '.' ) ||
                    (*helpPtr1 == '*' && *(helpPtr1+1) != 0) )
               {  displayMessage("Illegal node number mask");
                  ++update;
                  break;
               }
               ++helpPtr1;
            }
            ++helpPtr1;
         }
         while ( update );

         update = total = 0;

         if ( ch != _K_ESC_ )
         {  updInfoN = tempInfoN;
            for ( count = 0; count < nodeInfoCount; count++ )
            {  helpPtr1 = tempStr;
               helpPtr2 = nodeStr(&(nodeInfo[count]->node));
               while ( *helpPtr1 == *helpPtr2 || *helpPtr1 == '?' || *helpPtr1 == '*' )
               {  if ( *helpPtr1 == '*' || (*helpPtr1 == 0 && *helpPtr2 == 0) )
                  {  ++total;
                     tempInfoN = *nodeInfo[count];
                     update += changeGlobal(nodeMenu, &tempInfoN, &updInfoN);
                     *nodeInfo[count] = tempInfoN;
                     break;
                  }
                  ++helpPtr1;
                  ++helpPtr2;
               }
            }
            processed(update, total);
         }
      }
   }

   free (nodeMenu);
   return update;
}





s16 nodeMgr (void)
{
   u16          index = 0;
   nodeNumType  tempNode;
   u16          ch;
   u16          count, count2;
   u16          pos;
   tempStrType  nodeInfoPath;
   fhandle      nodeInfoHandle;
   s16          update = 0;
   void         *helpPtr;
   tempStrType  tempStr;
   headerType   *nodeHeader;
   nodeInfoType *nodeBuf;
   headerType   *areaHeader;
   rawEchoType  *areaBuf;
   nodeInfoTypeOld nodeInfoOld;
   u16          totalAreas, selAreas, selAreas2;

   if (openConfig(CFG_NODES, &nodeHeader, (void*)&nodeBuf))
   {
      *nodeInfoPath = 0;
      nodeInfoCount = min((u16)MAX_NODES, nodeHeader->totalRecords);

      for (count = 0; count < nodeInfoCount; count++)
      {
         if ( (nodeInfo[count] = malloc(sizeof(nodeInfoType))) == NULL )
         {  displayMessage("Not enough memory available");
            nodeInfoCount = 0;
            break;
         }
	 getRec(CFG_NODES, count);
	 memcpy (nodeInfo[count], nodeBuf, sizeof(nodeInfoType));
	 nodeInfo[count]->password[16] = 0;
	 nodeInfo[count]->packetPwd[8] = 0;
	 nodeInfo[count]->sysopName[35] = 0;
      }
      closeConfig (CFG_NODES);
   }
   else
   {
      strcpy (nodeInfoPath, configPath);
      strcat (nodeInfoPath, "FMAIL.NOD");

      if ((nodeInfoHandle = open(nodeInfoPath, O_BINARY|O_RDWR)) == -1)
      {
	 nodeInfoCount = 0;
      }
      else
      {
//         memset (nodeInfo, 0, sizeof(nodeInfoType)*nodeInfoCount);
	 lseek (nodeInfoHandle, 0, SEEK_SET);
	 nodeInfoCount = 0;
         while ((nodeInfoCount <= MAX_NODES) &&
		(_read (nodeInfoHandle, &nodeInfoOld, sizeof(nodeInfoTypeOld)) == sizeof (nodeInfoTypeOld)))
	 {
            if ( nodeInfoCount == MAX_NODES )
            {  displayMessage("Too many nodes in file");
               nodeInfoCount = 0;
               break;
            }
            if ( (nodeInfo[nodeInfoCount] = malloc(sizeof(nodeInfoType))) == NULL )
            {  displayMessage("Not enough memory available");
               nodeInfoCount = 0;
               break;
            }
            memset(nodeInfo[nodeInfoCount], 0, sizeof(nodeInfoType));
            nodeInfo[nodeInfoCount]->signature  = 'NO';
	    nodeInfo[nodeInfoCount]->node       = nodeInfoOld.node;
	    nodeInfo[nodeInfoCount]->viaNode    = nodeInfoOld.viaNode;
	    nodeInfo[nodeInfoCount]->capability = nodeInfoOld.capability;
	    nodeInfo[nodeInfoCount]->options    = nodeInfoOld.options;
	    nodeInfo[nodeInfoCount]->archiver   = nodeInfoOld.archiver;
	    nodeInfo[nodeInfoCount]->groups     = nodeInfoOld.groups;
	    nodeInfo[nodeInfoCount]->outStatus  = nodeInfoOld.outStatus;
	    memcpy (nodeInfo[nodeInfoCount]->sysopName,   nodeInfoOld.sysopName,35);
	    memcpy (nodeInfo[nodeInfoCount]->password,    nodeInfoOld.password, 17);
	    memcpy (nodeInfo[nodeInfoCount++]->packetPwd, nodeInfoOld.packetPwd, 9);
	 }

	 close(nodeInfoHandle);
         if ( nodeInfoCount )
            update++;
      }
   }

   memset (&tempInfoN, 0, sizeof(nodeInfoType));

   if (editNM(DISPLAY_NODE_WINDOW, 0)) return (1);
// if (displayMenu (nodeMenu, 5, 5)) return (1);

   do
   {
      do
      {
	 if (nodeInfoCount == 0)
	 {
	    memset (&tempInfoN, 0, sizeof (nodeInfoType));

	    removeWindow();
            if (editNM(DISPLAY_NODE_WINDOW, 0)) return (1);
//	    if (displayMenu (nodeMenu, 5, 5)) return (1);
            printString ("**** Empty ****", 17, 6, windowLook.datafg,
			  windowLook.background, MONO_HIGH);

	    printString ("F1 ", 0, 24, BROWN, BLACK, MONO_NORM);
	    printString ("Edit  ", 3, 24, MAGENTA, BLACK, MONO_NORM);
	    printString ("F2 ", 9, 24, BROWN, BLACK, MONO_NORM);
	    printString ("Search  ", 12, 24, MAGENTA, BLACK, MONO_NORM);
	    printString ("F3 ", 20, 24, BROWN, BLACK, MONO_NORM);
	    printString ("Global  ", 23, 24, MAGENTA, BLACK, MONO_NORM);
	    printString ("F5 ", 31, 24, BROWN, BLACK, MONO_NORM);
	    printString ("Browse  ", 34, 24, MAGENTA, BLACK, MONO_NORM);
	    printString ("F6 ", 42, 24, BROWN, BLACK, MONO_HIGH);
	    printString ("Copy  ", 45, 24, MAGENTA, BLACK, MONO_NORM);
            printString ("F8 ", 51, 24, YELLOW, BLACK, MONO_NORM);
            printString ("Areas     ", 54, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
            printString ("Ins ", 64, 24, YELLOW, BLACK, MONO_HIGH);
            printString ("Del      ", 68, 24, BROWN, BLACK, MONO_NORM);
	    printString ("\x1b \x1a", 77, 24, BROWN, BLACK, MONO_NORM);
	 }
	 else
	 {
	    sprintf (tempStr, " %*u/%u ", nodeInfoCount < 10 ? 1 :
					  nodeInfoCount < 100 ? 2 : 3,
					  index+1, nodeInfoCount);
            printString (tempStr, 73-strlen(tempStr), // 66,
                                  21,
				  YELLOW, LIGHTGRAY, MONO_HIGH);

	    tempInfoN = *nodeInfo[index];
            if (editNM(DISPLAY_NODE_DATA, 0)) return (1);
//	    displayData (nodeMenu, 5, 5, 0);

	    printString ("F1 ", 0, 24, YELLOW, BLACK, MONO_HIGH);
	    printString ("Edit  ", 3, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
	    printString ("F2 ", 9, 24, YELLOW, BLACK, MONO_HIGH);
	    printString ("Search  ", 12, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
	    printString ("F3 ", 20, 24, YELLOW, BLACK, MONO_HIGH);
            printString ("Global  ", 23, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
	    printString ("F5 ", 31, 24, YELLOW, BLACK, MONO_HIGH);
	    printString ("Browse  ", 34, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
            if (nodeInfoCount >= MAX_NODES)
            {  printString ("F6 ", 42, 24, BROWN, BLACK, MONO_HIGH);
	       printString ("Copy  ", 45, 24, MAGENTA, BLACK, MONO_NORM);
	    }
	    else
            {  printString ("F6 ", 42, 24, YELLOW, BLACK, MONO_HIGH);
	       printString ("Copy  ", 45, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
            }
            printString ("F8 ", 51, 24, YELLOW, BLACK, MONO_HIGH);
            printString ("Areas     ", 54, 24, LIGHTMAGENTA, BLACK, MONO_NORM);
            if (nodeInfoCount >= MAX_NODES)
               printString ("Ins ", 64, 24, BROWN, BLACK, MONO_NORM);
	    else
               printString ("Ins ", 64, 24, YELLOW, BLACK, MONO_HIGH);
            printString ("Del      ", 68, 24, YELLOW, BLACK, MONO_HIGH);
	    printString ("\x1b \x1a", 77, 24, YELLOW, BLACK, MONO_HIGH);
	 }

	 ch=readKbd();
	 if ((nodeInfoCount != 0) || (ch == _K_INS_))
	 {
	    switch (ch)
	    {
	       case _K_CTLF_ :
	       case _K_ALTF_ :
               case _K_F5_ :
               case _K_F9_ : if ( (count = nodesList(index)) != 0xffff )
				index = count;
			     break;
	       case _K_ENTER_ :
	       case _K_F1_ : pos = index;                           /* F1 */
			     do
			     {
//				update |= (ch = runMenu (nodeMenu, 5, 5));
				update |= (ch = editNM (EDIT_NODE, 0));

				if (ch)
				{
				   if ( !*tempInfoN.pktOutPath && tempInfoN.archiver == 0xFF )
				   {  ch = askBoolean ("Either PKT path or Archiver has to be specified. Discard changes ?", 'N');
				   }
				   else if (tempInfoN.node.zone == 0)
				   {  ch = askBoolean ("Node number is not defined. Discard changes ?", 'N');
				   }
				   else
				   {
				      index = 0;
				      while ((index < nodeInfoCount) &&
                                             ((memcmp (&tempInfoN.node,
                                                       &(nodeInfo[index]->node),
                                                       sizeof(nodeNumType)) != 0) ||
                                              (pos == index)))
                                      {
                                         index++;
                                      }
                                      if (index < nodeInfoCount)
                                      {
                                         ch = askBoolean ("Node is already in list. Discard changes ?", 'N');
                                      }
                                      else
                                      {
                                         index = 0;
                                         while ((index < nodeInfoCount) &&
                                                ((tempInfoN.node.zone > nodeInfo[index]->node.zone) ||
                                                 ((tempInfoN.node.zone == nodeInfo[index]->node.zone) &&
                                                  (tempInfoN.node.net > nodeInfo[index]->node.net)) ||
                                                 ((tempInfoN.node.zone == nodeInfo[index]->node.zone) &&
                                                  (tempInfoN.node.net == nodeInfo[index]->node.net) &&
                                                  (tempInfoN.node.node > nodeInfo[index]->node.node)) ||
                                                 ((tempInfoN.node.zone == nodeInfo[index]->node.zone) &&
                                                  (tempInfoN.node.net == nodeInfo[index]->node.net) &&
                                                  (tempInfoN.node.node == nodeInfo[index]->node.node) &&
                                                  (tempInfoN.node.point > nodeInfo[index]->node.point))))
                                         {
                                            index++;
                                         }
                                         if ((index == nodeInfoCount) ||
                                             (memcmp (&tempInfoN.node, &(nodeInfo[index]->node),
                                                      sizeof(nodeNumType)) != 0))
                                         {
                                            helpPtr = nodeInfo[pos];
                                            memcpy (&nodeInfo[pos],
                                                    &nodeInfo[pos+1],
                                                    ((--nodeInfoCount)-pos)*sizeof(nodeInfoType*));
                                            if (pos < index)
                                               index--;
                                            memmove (&nodeInfo[index+1], &nodeInfo[index],
                                                     ((nodeInfoCount++)-index) *
                                                      sizeof(nodeInfoType*));
                                            nodeInfo[index] = helpPtr;
                                         }
                                         *nodeInfo[index] = tempInfoN;
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

                             break;
               case _K_F2_ : tempNode = getNodeNum (" Search node ", 37, 12, 0);
                             if (tempNode.zone != 0)
                             {
                                count = 0;
                                while ((count < nodeInfoCount) &&
                                       (memcmp (&tempNode, &(nodeInfo[count]->node),
                                                sizeof (nodeNumType)) != 0))
                                {
                                   count++;
                                }
                                if (count < nodeInfoCount)
                                {
                                   index = count;
                                }
                                else
                                {
                                   displayMessage ("Node was not found");
                                }
                             }
                             break;
               case _K_F3_ :
                             memset(&tempInfoN, 0, sizeof(tempInfoN));
                             update |= editNM(EDIT_NODE_GLOBAL, 1);
                             tempInfoN = *nodeInfo[index];
//                           update |= runMenu (aglMenu, 47, 7); /* F3 */
                             break;
               case _K_F8_ : if ( openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf) )
                             {  totalAreas = min(MAX_AREAS, areaHeader->totalRecords);
                                selAreas = 0;
                                selAreas2 = 0;
                                for ( count = 0; count < totalAreas; count++ )
                                {  if ( getRec(CFG_ECHOAREAS, count) )
                                   {
                                      count2 = 0;
                                      while ( count2 < MAX_FORWARD )
                                      {  if ( !memcmp(&tempInfoN.node, &areaBuf->export[count2++].nodeNum, sizeof(nodeNumType)) )
                                            break;
                                      }
                                      if ( count2 < MAX_FORWARD )
                                      {
                                         if ( (listDataArray[selAreas].desc = malloc(56)) == NULL )
                                            goto nomem;
                                         sprintf(listDataArray[selAreas].desc, " %-50.50s %c",
                                                 areaBuf->areaName,
                                                 getGroupChar(areaBuf->group));
                                         listDataArray[selAreas].flags = 0;
                                         listDataArray[selAreas++].index = count;
                                      }
                                      else
                                      {
                                         if ( !(areaBuf->group & tempInfoN.groups) )
                                            continue;
                                         if ( (listDataArray2[selAreas2].desc = malloc(56)) == NULL )
                                         {
nomem:                                      while ( selAreas )
                                               free(listDataArray[--selAreas].desc);
                                            while ( selAreas2 )
                                               free(listDataArray2[--selAreas2].desc);
                                            break;
                                         }
                                         sprintf(listDataArray2[selAreas2].desc, " %-50.50s %c",
                                                 areaBuf->areaName,
                                                 getGroupChar(areaBuf->group));
                                         listDataArray2[selAreas2].flags = BIT15;
                                         listDataArray2[selAreas2++].index = count;
                                      }
                                   }
                                }
                                listData(&selAreas, listDataArray, &selAreas2, listDataArray2);
                                while ( selAreas )
                                {  if ( (listDataArray[--selAreas].flags & BIT15) &&
                                        getRec(CFG_ECHOAREAS, listDataArray[selAreas].index) )
                                   {  count2 = 0;
                                      while ( count2 < MAX_FORWARD )
                                      {  if ( !memcmp(&tempInfoN.node, &areaBuf->export[count2].nodeNum, sizeof(nodeNumType)) )
                                            break;
                                         if ( areaBuf->export[count2].nodeNum.zone == 0 ||
                                              nodegreater(areaBuf->export[count2].nodeNum, tempInfoN.node) )
                                         {  if ( count2 < MAX_FORWARD - 1 )
                                               memmove(&areaBuf->export[count2 + 1].nodeNum, &areaBuf->export[count2].nodeNum, (MAX_FORWARD - count2 - 1) * sizeof(nodeNumXType));
                                            areaBuf->export[count2].nodeNum = tempInfoN.node;
//                                          areaBuf->export[count2].flags = (listDataArray[--selAreas].flags & ~BIT15);
                                            break;
                                         }
                                         ++count2;
                                      }
                                      putRec(CFG_ECHOAREAS, listDataArray[selAreas].index);
                                   }
                                   free(listDataArray[selAreas].desc);
                                }
                                while ( selAreas2 )
                                {  if ( !(listDataArray2[--selAreas2].flags & BIT15) &&
                                        getRec(CFG_ECHOAREAS, listDataArray2[selAreas2].index) )
                                   {  count2 = 0;
                                      while ( count2 < MAX_FORWARD )
                                      {  if ( !memcmp(&tempInfoN.node, &areaBuf->export[count2].nodeNum, sizeof(nodeNumType)) )
                                         {  memmove(&areaBuf->export[count2].nodeNum, &areaBuf->export[count2 + 1].nodeNum, (MAX_FORWARD - count2 - 1) * sizeof(nodeNumXType));
                                            break;
                                         }
                                         ++count2;
                                      }
                                      putRec(CFG_ECHOAREAS, listDataArray2[selAreas2].index);
                                   }
                                   free(listDataArray2[selAreas2].desc);
                                }
                                closeConfig (CFG_ECHOAREAS);
                             }
                             break;
               case _K_CPGUP_:
               case _K_HOME_ :
                             index = 0;                               /* Home */
                             break;
               case _K_LEFT_ :
                             if (index > 0)                     /* Left Arrow */
                                index--;
                             break;
               case _K_RIGHT_ :
                             if (index < nodeInfoCount-1)      /* Right Arrow */
                                index++;
                             break;
               case _K_CPGDN_ :
               case _K_END_ :
                             index = nodeInfoCount - 1;                /* End */
                             break;
               case _K_F6_ : if (nodeInfoCount == 0 || nodeInfoCount >= MAX_NODES) /* F6 */
                                break;
                             tempInfoN.lastMsgRcvdDat = 0;
                             tempInfoN.lastMsgSentDat = 0;
                             tempInfoN.lastNewBundleDat = 0;
                             tempInfoN.referenceLNBDat = 0;
                             tempInfoN.lastSentBCL = 0;
                             goto addrecord;
               case _K_INS_ :
                             if (nodeInfoCount == MAX_NODES) break; /* Insert */
                             memset (&tempInfoN, 0, sizeof(nodeInfoType));
                             tempInfoN.options.active = 1;
                             tempInfoN.capability = PKT_TYPE_2PLUS;
                             tempInfoN.archiver = config.defaultArc;
addrecord:                   tempInfoN.signature = 'ND';
                             pos = index;
                             do
                             {
//                              update |= (ch = runMenu (nodeMenu, 5, 5));
                                update |= (ch = editNM(EDIT_NODE, 0));

                                if (ch)
                                {
                                   if ( !*tempInfoN.pktOutPath && tempInfoN.archiver == 0xFF )
                                   {  ch = askBoolean ("Either PKT path or Archiver has to be specified. Discard changes ?", 'N');
                                   }
                                   else if (tempInfoN.node.zone == 0)
                                   {  ch = askBoolean ("Node number is not defined. Discard current record ?", 'N');
                                   }
                                   else
                                   {
                                      index = 0;
                                      while ((index < nodeInfoCount) &&
                                             ((tempInfoN.node.zone > nodeInfo[index]->node.zone) ||
                                              ((tempInfoN.node.zone == nodeInfo[index]->node.zone) &&
                                               (tempInfoN.node.net > nodeInfo[index]->node.net)) ||
                                              ((tempInfoN.node.zone == nodeInfo[index]->node.zone) &&
                                               (tempInfoN.node.net == nodeInfo[index]->node.net) &&
                                               (tempInfoN.node.node > nodeInfo[index]->node.node)) ||
                                              ((tempInfoN.node.zone == nodeInfo[index]->node.zone) &&
                                               (tempInfoN.node.net == nodeInfo[index]->node.net) &&
                                               (tempInfoN.node.node == nodeInfo[index]->node.node) &&
                                               (tempInfoN.node.point > nodeInfo[index]->node.point))))
                                      {
                                         index++;
                                      }
                                      if ((index < nodeInfoCount) &&
                                          (memcmp (&tempInfoN.node, &(nodeInfo[index]->node),
                                                   sizeof(nodeNumType)) == 0))
                                      {
                                         ch = askBoolean ("Node is already in list. Discard current record ?", 'N');
                                      }
                                      else
                                      {
                                         if ( (helpPtr = malloc(sizeof(nodeInfoType))) != NULL )
                                         {  memmove (&nodeInfo[index+1], &nodeInfo[index],
                                                     ((nodeInfoCount++)-index) *
                                                      sizeof(nodeInfoType*));
                                            nodeInfo[index] = helpPtr;
                                            *nodeInfo[index] = tempInfoN;
                                         }
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

                             break;
               case _K_DEL_ :
                             if ((nodeInfoCount > 0) &&             /* Delete */
                                 (askBoolean ("Delete this entry (also in export lists in the AreaMgr) ?", 'Y') == 'Y'))
                             {
                                update = 1;
                                if (openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf))
                                {
                                   for (count = 0; count < areaHeader->totalRecords; count++)
                                   {
                                      getRec(CFG_ECHOAREAS, count);
                                      count2 = 0;
                                      while (count2 < MAX_FORWARD)
                                      {
                                         if (memcmp (&(areaBuf->export[count2].nodeNum),
                                                     &(nodeInfo[index]->node),
                                                     sizeof(nodeNumType)) == 0)
                                         {  memcpy (&(areaBuf->export[count2]),
                                                    &(areaBuf->export[count2+1]),
                                                    (MAX_FORWARD-1 - count2 ) * sizeof(nodeNumXType));
                                            memset (&(areaBuf->export[MAX_FORWARD-1]), 0,
                                                    sizeof(nodeNumXType));
                                            putRec(CFG_ECHOAREAS, count);
                                            break;
                                         }
                                         count2++;
                                      }
                                   }
                                   closeConfig (CFG_ECHOAREAS);
                                }
                                free(nodeInfo[index]);
                                memcpy (&nodeInfo[index],
                                        &nodeInfo[index+1],
                                        (--nodeInfoCount-index)*sizeof(nodeInfoType*));
                                if ((index == nodeInfoCount) && (index != 0))
                                   index--;
                             }
                             break;
            }
         }
      }
      while ((ch != _K_ESC_) && (ch != _K_F7_) && (ch != _K_F10_));
      if ( ch == _K_F7_ || ch == _K_F10_ )
         ch = 'Y';
      else
         if (update)
            ch = askBoolean ("Save changes in node setup ?", 'Y');
         else
            ch = 0;
   }
   while (ch == _K_ESC_);

   removeWindow ();

   if (ch == 'Y')
   {
      /* remove old style config file */
      if (*nodeInfoPath) unlink (nodeInfoPath);

      if (openConfig(CFG_NODES, &nodeHeader, (void*)&nodeBuf))
      {
         headerType nodeHeader2;
	 u16        fml102 = 0;
	 fhandle    fml102handle;

         if ( *config.autoFMail102Path )
         {  if ( !stricmp(config.autoFMail102Path, configPath) )
               displayMessage("AutoExport for FMail 1.02 format should be set to another directory");
            else
            if ( (fml102handle = open(strcat(strcpy(tempStr, config.autoFMail102Path), "FMAIL.NOD"), O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE)) != -1 )
            {  fml102 = 1;
               nodeHeader2 = *nodeHeader;
               strcpy(strchr(nodeHeader2.versionString, '\x1a'), "/conv\x1a");
               nodeHeader2.revNumber = 0x0100;
               nodeHeader2.recordSize = sizeof(nodeInfoType102);
               write(fml102handle, &nodeHeader2, sizeof(headerType));
            }
         }
         chgNumRec(CFG_NODES, nodeInfoCount);
         for (count = 0; count < nodeInfoCount; count++)
	 {
            memcpy (nodeBuf, nodeInfo[count], sizeof(nodeInfoType));
	    putRec(CFG_NODES, count);
            if ( fml102 )
            {   if ( nodeBuf->useAka > 15 )
                   nodeBuf->useAka = 0;
                write(fml102handle, nodeBuf, sizeof(nodeInfoType102));
            }
         }
	 closeConfig(CFG_NODES);
         if ( fml102 )
            close(fml102handle);
       }
   }
   for (count = 0; count < nodeInfoCount; count++)
      free(nodeInfo[count]);

   return (0);
}

