//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2015 Wilfred van Velzen
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

#ifdef __OS2__
#define INCL_DOSPROCESS
#include <os2.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>

#include "fmail.h"

#include "areainfo.h"
#include "cfgfile.h"
#include "ftools.h"
#include "ftlog.h"
#include "ftr.h"
#include "mtask.h"
#include "utils.h"


#define MAX_DISPLAY 128


typedef struct
{
  u16 index;
  u32 group;

} areaSortType;


extern configType config;
extern char configPath[128];

static wildCards;


s16 packValid (nodeNumType *node, char *packedNodes)
{
   tempStrType tempStr;
   char        *helpPtr, *helpPtr2;
   char        *stringNode;
   u16         count;
   char        nodeTempStr[32];


   wildCards = 0;
   if (packedNodes == NULL) return (0);

   strcpy (tempStr, packedNodes);
   helpPtr = strtok (tempStr, " ");

   *nodeTempStr = 0;

   while (helpPtr != NULL)
   {
      if ((*(u16*)helpPtr == '*') ||
          (strchr(helpPtr,':')!=NULL))
         strcpy (nodeTempStr,helpPtr);
      else
      {
         if (strchr(helpPtr,'/')!=NULL)
         {
            if ((helpPtr2=strchr(nodeTempStr,':'))!=NULL)
               strcpy(helpPtr2 + 1, helpPtr);
            else
               logEntry("Bad node on command line", LOG_ALWAYS, 4);
         }
         else
         {
            if (*(helpPtr)!='.')
            {
               if ((helpPtr2=strchr(nodeTempStr,'/'))!=NULL)
               {
                  strcpy (helpPtr2+1,helpPtr);
               }
               else
               {
                  logEntry("Bad node on command line",LOG_ALWAYS,4);
               }
            }
            else
            {
               if ((helpPtr2=strchr(nodeTempStr,'.'))!=NULL)
               {
                  strcpy (helpPtr2,helpPtr);
               }
               else
               {
                  strcat (nodeTempStr,helpPtr);
               }
            }
         }
      }
      stringNode = nodeStr(node);

      for (count = 0; count < (u16)strlen(nodeTempStr); count++)
      {
         if (nodeTempStr[count]=='?')
         {
            wildCards = 1;
            if (isdigit(stringNode[count]))
            {
               stringNode[count]='?';
            }
         }
         if (nodeTempStr[count]=='*')
         {
            wildCards = 1;
            if (nodeTempStr[count+1] != 0)
            {
               logEntry ("Asterisk only allowed as last character of node number", LOG_ALWAYS, 4);
            }
            nodeTempStr[count] = 0;
            stringNode[count] = 0;
            if (count && (nodeTempStr[count-1] == '.') &&
                         (stringNode[count-1] == 0))
            {
               nodeTempStr[count-1] = 0;
            }
            break;
         }
      }
      if (strcmp (stringNode, nodeTempStr) == 0)
      {
         return (1);
      }
/* geen impliciete wildcard voor points
      if ((helpPtr = strchr(stringNode, '.')) != NULL)
      {
         *helpPtr = 0;
         if (strcmp (stringNode, nodeTempStr) == 0)
         {
            return (1);
         }
      }
*/
      helpPtr = strtok (NULL, " ");
   }
   return (0);
}



s16 notify(int argc, char *argv[])
{
   char            *helpPtr;
   s32              switches;
   u16              nodeCount
                  , count
                  , d;
   u16              bufCount;
   tempStrType      tempStr;
   u32              oldGroup;
   internalMsgType *message;
   u16              areaCount;
   areaSortType    *areaSort;
   headerType      *nodeHeader
                 , *areaHeader;
   rawEchoType     *areaBuf;
   nodeInfoType    *nodeBuf;
   tempStrType      nodeString;

   if (argc >= 3 && (argv[2][0] == '?' || argv[2][1] == '?'))
   {
      puts("Usage:\n\n"
           "    FTools Notify [<node> [<node> ...] [/A|/N]\n\n"
           "    [<node>]  Node number(s) of systems to send reports to.\n"
           "              Wildcards are allowed. If omitted, * (all nodes) is assumed.\n"
           "              If wildcards are used, only qualified nodes that are marked\n"
           "              in the Node Manager will receive a notify message.\n"
           "Switches:\n\n"
           "    /A   Send Area Status report\n"
           "    /N   Send Node Status report");

      return 0;
   }
   switches = getSwitchFT (&argc, argv, SW_A|SW_N);

   initLog("NOTIFY", switches);

   if (switches == 0)
   {
      logEntry("Nothing to do!", LOG_ALWAYS, 0);

      return 0;
   }

   if (argc < 3)
      strcpy(nodeString, "*");
   else
   {
      *nodeString = 0;
      for (count = 2; count < argc-1; count++)
      {
         strcat(nodeString, argv[count]);
         strcat(nodeString, " ");
      }
      strcat(nodeString, argv[count]);
   }

   if (((message = malloc (INTMSG_SIZE)) == NULL) ||
       ((areaSort = malloc (MAX_AREAS*sizeof(areaSortType))) == NULL))
      logEntry("Not enough memory available", LOG_ALWAYS, 2);

   memset(message, 0, INTMSG_SIZE);

   strcpy(message->fromUserName, *config.sysopName?config.sysopName:"FMail");

   if (!openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf))
      logEntry("Can't open file "dARFNAME, LOG_ALWAYS, 1);

   for (areaCount = 0; areaCount<areaHeader->totalRecords; areaCount++)
   {
      returnTimeSlice(0);
      getRec(CFG_ECHOAREAS, areaCount);
      areaBuf->areaName[ECHONAME_LEN-1] = 0;
      areaBuf->comment[COMMENT_LEN-1] = 0;
      areaBuf->originLine[ORGLINE_LEN-1] = 0;

      count = 0;
      while ((count < MAX_AREAS) && (count < areaCount) && (areaBuf->group >= areaSort[count].group))
         count++;

      if (count < MAX_AREAS)
      {
         memmove(&areaSort[count+1], &areaSort[count], sizeof(areaSortType)*(areaCount-count));
         areaSort[count].index = areaCount;
         areaSort[count].group = areaBuf->group;
      }
   }

   if (!openConfig(CFG_NODES, &nodeHeader, (void*)&nodeBuf))
      logEntry ("Can't open file "dNODFNAME, LOG_ALWAYS, 1);

   if (switches & SW_N)
   {
      for (nodeCount=0; nodeCount<nodeHeader->totalRecords; nodeCount++)
      {
         returnTimeSlice(0);
         getRec(CFG_NODES, nodeCount);
         nodeBuf->password[16] = 0;
         nodeBuf->packetPwd[8] = 0;
         nodeBuf->sysopName[35] = 0;

         if (packValid(&nodeBuf->node, nodeString) &&
             (nodeBuf->options.notify || !wildCards))
         {
            strcpy (message->toUserName, *nodeBuf->sysopName?nodeBuf->sysopName:"SysOp");
            strcpy (message->subject, "FMail node status report");

            switch (nodeBuf->archiver)
#ifdef __32BIT__
            {   case 0:  strcpy(tempStr, config.arc32.programName);
                         break;
                case 1:  strcpy(tempStr, config.zip32.programName);
                         break;
                case 2:  strcpy(tempStr, config.lzh32.programName);
                         break;
                case 3:  strcpy(tempStr, config.pak32.programName);
                         break;
                case 4:  strcpy(tempStr, config.zoo32.programName);
                         break;
                case 5:  strcpy(tempStr, config.arj32.programName);
			 break;
                case 6:  strcpy(tempStr, config.sqz32.programName);
			 break;
                case 7:  strcpy(tempStr, config.customArc32.programName);
			 break;
                case 8:  strcpy(tempStr, config.uc232.programName);
			 break;
                case 9:  strcpy(tempStr, config.rar32.programName);
			 break;
                case 10: strcpy(tempStr, config.jar32.programName);
                         break;
#else
            {   case 0:  strcpy(tempStr, config.arc.programName);
                         break;
                case 1:  strcpy(tempStr, config.zip.programName);
                         break;
                case 2:  strcpy(tempStr, config.lzh.programName);
                         break;
                case 3:  strcpy(tempStr, config.pak.programName);
                         break;
                case 4:  strcpy(tempStr, config.zoo.programName);
                         break;
		case 5:  strcpy(tempStr, config.arj.programName);
			 break;
		case 6:  strcpy(tempStr, config.sqz.programName);
			 break;
		case 7:  strcpy(tempStr, config.customArc.programName);
			 break;
		case 8:  strcpy(tempStr, config.uc2.programName);
			 break;
		case 9:  strcpy(tempStr, config.rar.programName);
                         break;
                case 10: strcpy(tempStr, config.jar.programName);
                         break;
#endif
                case 0xFF:strcpy(tempStr, "None");
                         break;
                default: strcpy(tempStr, "UNKNOWN");
			 break;
	    }
            if ( (helpPtr = strchr(tempStr, ' ')) != NULL )
            {   while ( helpPtr > tempStr && *(helpPtr-1) != '\\' )
                   --helpPtr;
                strcpy(tempStr, helpPtr);
            }
            else if ( (helpPtr = strrchr(tempStr, '\\')) != NULL )
                strcpy(tempStr, helpPtr);
            helpPtr = message->text;
	    helpPtr += sprintf (helpPtr, "Following are the options that are set for your system (%s)\r"
					 "and a list of areas that your system is connected to.\r"
					 "Please let the SysOp know, if some of these settings are not correct.\r\r"
					 "Capability           %s\r"
					 "Node status          %s\r"
					 "Allow rescan         %s\r"
					 "Remote maintenance   %s\r"
					 "Compression program  %s\r"
					 "Mail archive status  %s\r"
					 "Tiny SEEN-BYs        %s\r"
					 "Reformat date        %s\r\r"
                                         "Area name                                          AKA at this system      Mode\r"
                                         "-------------------------------------------------- ----------------------- ----\r",
					 nodeStr(&nodeBuf->node),
					 nodeBuf->capability ? "Type 2+" : "Stone Age",
					 nodeBuf->options.active ? "Active" : "Passive",
													  nodeBuf->options.allowRescan ? "Yes":"No",
					 nodeBuf->options.remMaint ? "Yes":"No",
					 tempStr,
					 nodeBuf->outStatus==0 ? "None" :
                                         nodeBuf->outStatus==1 ? "Hold" :
                                         nodeBuf->outStatus==2 ? "Crash" :
                                         nodeBuf->outStatus==3 ? "Hold/Direct" :
                                         nodeBuf->outStatus==4 ? "Crash/Direct" :
                                         nodeBuf->outStatus==5 ? "Direct":"Unknown",
					 nodeBuf->options.tinySeenBy ? "Yes" : "No",
					 nodeBuf->options.fixDate ? "Yes" : "No");

	    oldGroup = 0;
	    bufCount = MAX_DISPLAY;

	    for (areaCount = 0; areaCount < areaHeader->totalRecords; areaCount++)
	    {
               getRec(CFG_ECHOAREAS, areaSort[areaCount].index);
               if ( (areaBuf->options.active) &&
                    !areaBuf->options.local &&
                    (areaBuf->group & nodeBuf->groups) )
               {
                  count = 0;
                  while ((count < MAX_FORWARD) && nodeBuf->node.zone &&
                         memcmp(&nodeBuf->node, &areaBuf->forwards[count].nodeNum, 8))
		  {
			  count++;
		  }
                  if ( count < MAX_FORWARD &&
                       nodeBuf->node.zone &&
                       !areaBuf->forwards[count].flags.locked )
                  {
                     if (!(bufCount--))
		     {
			sprintf (helpPtr, "\r*** Listing is continued in the next message ***\r");
			writeNetMsg (message, nodeBuf->useAka-1, &nodeBuf->node, nodeBuf->capability,
					      nodeBuf->outStatus);
			strcpy (message->subject, "FMail node status report (continued)");
			helpPtr = message->text + sprintf (message->text, "*** Following list is a continuation of the previous message ***\r");
			bufCount = MAX_DISPLAY;
			oldGroup = 0;
		     }
		     if (oldGroup != areaBuf->group)
		     {
			oldGroup = areaBuf->group;
			d = 0;
			while ((areaBuf->group >>= 1) != 0)
			{
			   d++;
			}
			if (*config.groupDescr[d])
			{
			   helpPtr += sprintf (helpPtr, "\r%s\r\r",
							config.groupDescr[d]);
			}
			else
			{
			   helpPtr += sprintf (helpPtr, "\rGroup %c\r\r", 'A'+d);
			}
		     }
                     helpPtr += sprintf (helpPtr, "%-50s %-24s%s\r",
						  areaBuf->areaName,
                                                  nodeStr(&config.akaList[areaBuf->address].nodeNum),
                                                  areaBuf->forwards[count].flags.readOnly ? "R/O" :
                                                  areaBuf->forwards[count].flags.writeOnly ? "W/O":"R/W");
		  }
	       }
	    }
	    writeNetMsg (message, nodeBuf->useAka-1, &nodeBuf->node, nodeBuf->capability,
				  nodeBuf->outStatus);
	    sprintf (tempStr, "Sending node status message to node %s", nodeStr(&nodeBuf->node));
	    logEntry (tempStr, LOG_ALWAYS, 0);
	 }
      }
   }
   if (switches & SW_A)
   {
      for (nodeCount = 0; nodeCount < nodeHeader->totalRecords; nodeCount++)
      {
         returnTimeSlice(0);
	 getRec(CFG_NODES, nodeCount);
         if (packValid(&nodeBuf->node, nodeString) &&
             (nodeBuf->options.notify || !wildCards))
         {
            strcpy (message->toUserName, *nodeBuf->sysopName?nodeBuf->sysopName:"SysOp");
            strcpy (message->subject, "FMail area status report");

            helpPtr = message->text;
            helpPtr += sprintf (helpPtr, "Following is a list of echomail areas available to you at this system.\r\r"
                                         "* indicates that you (%s) are active for this area.\r"
                                         "R indicates that you are read-only for this area.\r"
                                         "W indicates that you are write-only for this area.\r",
                                         nodeStr(&nodeBuf->node));

	    oldGroup = 0;
	    bufCount = MAX_DISPLAY;

	    for (areaCount = 0; areaCount < areaHeader->totalRecords; areaCount++)
            {
	       getRec(CFG_ECHOAREAS, areaSort[areaCount].index);
               if ( (areaBuf->options.active) &&
                    !areaBuf->options.local &&
                    (areaBuf->group & nodeBuf->groups) )
               {
                  count = 0;
                  while ((count < MAX_FORWARD) &&
                         memcmp(&nodeBuf->node, &areaBuf->forwards[count].nodeNum, 8))
                  {
                     count++;
                  }
                  if ( ((areaBuf->group & nodeBuf->groups) && (count >= MAX_FORWARD || !areaBuf->forwards[count].flags.locked)) ||
                       (count < MAX_FORWARD && !areaBuf->forwards[count].flags.locked) )
                  {
                     if (!(bufCount--))
                     {
			sprintf (helpPtr, "\r*** Listing is continued in the next message ***\r");
                        writeNetMsg (message, nodeBuf->useAka-1, &nodeBuf->node, nodeBuf->capability,
                                              nodeBuf->outStatus);
                        strcpy (message->subject, "FMail area status report (continued)");
                        helpPtr = message->text + sprintf (message->text, "*** Following list is a continuation of the previous message ***\r");
                        bufCount = MAX_DISPLAY;
                        oldGroup = 0;
                     }
                     if (oldGroup != areaBuf->group)
                     {
                        oldGroup = areaBuf->group;
                        d = 0;
                        while ((areaBuf->group >>= 1) != 0)
                        {
                           d++;
                        }
                        if (*config.groupDescr[d])
                        {
                           helpPtr += sprintf (helpPtr, "\r%s\r\r",
                                                        config.groupDescr[d]);
                        }
                        else
                        {
                           helpPtr += sprintf (helpPtr, "\rGroup %c\r\r", 'A'+d);
                        }
                     }
                     helpPtr += sprintf (helpPtr, (strlen(areaBuf->areaName) < ECHONAME_LEN_OLD)?"%c %-24s  %s\r":"%c %s\r                            %s\r",
                                                  (count < MAX_FORWARD) ? (areaBuf->forwards[count].flags.readOnly ? 'R':
                                                                           areaBuf->forwards[count].flags.writeOnly ? 'W' : '*' ):
                                                  ' ',
                                                  areaBuf->areaName,
                                                  areaBuf->comment);
                  }
               }
            }
            writeNetMsg (message, nodeBuf->useAka-1, &nodeBuf->node, nodeBuf->capability,
                                  nodeBuf->outStatus);
            sprintf (tempStr, "Sending area status message to node %s", nodeStr(&nodeBuf->node));
            logEntry (tempStr, LOG_ALWAYS, 0);
         }
      }
   }
   closeConfig (CFG_ECHOAREAS);
   closeConfig (CFG_NODES);

   return 0;
}
