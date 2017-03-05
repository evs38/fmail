//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2017 Wilfred van Velzen
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


#include <dir.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"

#include "export.h"

#include "areainfo.h"
#include "cfgfile.h"
#include "fdfolder.h"
#include "fs_util.h"
#include "nodeinfo.h"
#include "stpcpy.h"
#include "version.h"
#include "window.h"

const char *bar = "================================================================================";

const char *arcName[12] = {"None", "ARC", "ZIP", "LZH", "PAK", "ZOO", "ARJ", "SQZ", "Custom", "UC2", "RAR", "JAR"};
#ifndef __FMAILX__
#ifndef __32BIT__
const char *bufSizeName[5] = {"Huge", "Large", "Medium", "Small", "Tiny"};
#endif
#endif
const char *mailerName[6] = { "FrontDoor",
                              "InterMail",
                              "D'Bridge",
                              "Binkley/Portal of Power",
                              "MainDoor",
                              "Xenia" };
const char *bbsName[8] = {    "RemoteAccess < 2.00",
                              "SuperBBS",
                              "QuickBBS",
                              "TAG",
                              "RemoteAccess 2.00/2.02",
                              "RemoteAccess 2.50",
                              "ProBoard",
                              "EleBBS"
                         };

extern u16             areaInfoCount;
static areaInfoPtrArrO areaInfo;

//extern u16             nodeInfoCount;
//extern nodeInfoType    *nodeInfo;

extern char configPath[FILENAME_MAX];
extern windowLookType  windowLook;
extern configType      config;



char *bitData (char data)
{
static char bits[3][9];
static u16  index;
   char temp = 1;

   if ( ++index == 3 )
      index = 0;
   bits[index][0] = 0;
   do
   {
      if (data & temp)
         strcat (bits[index], "þ");
      else
         strcat (bits[index], "-");
   }
   while (temp <<= 1);
   return (bits[index]);
}


u16 readAreaInfo (void)
{
   u16         count;
   headerType  *areaHeader;
   rawEchoType *areaBuf;

   if (!openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf))
      areaInfoCount = 0;
   else
   {
      areaInfoCount = areaHeader->totalRecords;
      for (count = 0; count < areaInfoCount; count++)
      {
         if ((areaInfo[count] = malloc (RAWECHO_SIZE)) == NULL)
	 {
	    closeConfig (CFG_ECHOAREAS);
	    for (areaInfoCount = 0; areaInfoCount < count; areaInfoCount++)
	    {
               free (areaInfo[areaInfoCount]);
	    }
	    displayMessage ("Not enough memory read area information");
	    return (1);
	 }
	 getRec(CFG_ECHOAREAS, count);
         memcpy (areaInfo[count], areaBuf, RAWECHO_SIZE);

	 areaInfo[count]->areaName[ECHONAME_LEN-1] = 0;
	 areaInfo[count]->comment[COMMENT_LEN-1] = 0;
	 areaInfo[count]->originLine[ORGLINE_LEN-1] = 0;
      }
      closeConfig (CFG_ECHOAREAS);
   }
   return (0);
}



void freeAreaInfo (void)
{
   u16 count;

   for (count = 0; count < areaInfoCount; count++)
   {
      free (areaInfo[count]);
   }
}



s16 listGroups (void)
{
   u16      count1,
            count2;
   s32      mask;
   FILE     *textFile;
   char     *fileNamePtr;
   s16      display;
   time_t   timer;

   time (&timer);

   if (!readAreaInfo())
   {
      fileNamePtr = getDestFileName (" Destination file ");
      if (*fileNamePtr == 0)
      {
         freeAreaInfo ();
         return (0);
      }

     working ();

     if ((textFile = fopen(fileNamePtr, "wt")) == NULL)
      {
         displayMessage ("Can't open output file");
         freeAreaInfo ();
         return (0);
      }
      fprintf (textFile, "\n%s  -  Group configuration  -  %s%s\n", VersionStr(), ctime(&timer), bar);

      mask = 1;
      for (count1 = 0; count1 < 26; count1++)
      {
         display = 0;
         for (count2 = 0; count2 < areaInfoCount; count2++)
         {
            if (areaInfo[count2]->group & mask)
            {
               if (!display)
               {
                  fprintf (textFile, "\nGroup %c  %s\n\n", count1+'A', config.groupDescr[count1]);
                  display++;
               }
               fprintf (textFile, (strlen(areaInfo[count2]->areaName) < ECHONAME_LEN_OLD)?"%-24s  %s\n":"%s\n                          %s\n",
                                  areaInfo[count2]->areaName,
                                  areaInfo[count2]->comment);
            }
         }

         mask <<= 1;
      }
      fclose (textFile);
      freeAreaInfo ();
   }
   return (0);
}



s16 listNodeEcho (void)
{
   u16         count1,
               count2;
   FILE        *textFile;
   char        *fileNamePtr;
   nodeNumType nodeNum;
   s16         display = 0;
   time_t      timer;

   time (&timer);

   nodeNum = getNodeNum ("Node", 37, 12, 0);
   if (nodeNum.zone == 0)
      return (0);

   if (!readAreaInfo())
   {
      fileNamePtr = getDestFileName (" Destination file ");
      if (*fileNamePtr == 0)
      {
         freeAreaInfo ();
         return (0);
      }

     working ();

     if ((textFile = fopen(fileNamePtr, "wt")) == NULL)
      {
         displayMessage ("Can't open output file");
         freeAreaInfo ();
         return (0);
      }
      fprintf(textFile, "\n%s  -  Active areas for %s  -  %s%s\n\n", VersionStr(), nodeStr(&nodeNum), ctime(&timer), bar);

      for (count1 = 0; count1 < areaInfoCount; count1++)
      {
         count2 = 0;
         while ((count2 < MAX_FORWARD) &&
                (memcmp (&areaInfo[count1]->forwards[count2].nodeNum, &nodeNum,
                         sizeof(nodeNumType)) != 0))
         {
            count2++;
         }

         if (count2 < MAX_FORWARD)
         {
            if (!display)
            {
               fprintf(textFile, "  Conference               Comment                                        Group\n"
                                 "-------------------------------------------------------------------------------\n");
               display++;
            }

            fprintf (textFile, (strlen(areaInfo[count1]->areaName) < ECHONAME_LEN_OLD)?"%c %-24s %-50s %c\n":"%c %s\n                           %-50s %c\n",
                               areaInfo[count1]->forwards[count2].flags.readOnly ? 'R':
                               areaInfo[count1]->forwards[count2].flags.writeOnly ? 'W':
                               areaInfo[count1]->forwards[count2].flags.locked ? 'L': ' ',
                               areaInfo[count1]->areaName,
                               areaInfo[count1]->comment,
                               getGroupChar(areaInfo[count1]->group));
         }
      }
      if (!display)
         fprintf (textFile, "This node is not active for any conference.\n");

      fclose (textFile);
      freeAreaInfo ();
   }
   return (0);
}



s16 listHudsonBoards(void)
{
   u16         count1,
               count2;
   FILE        *textFile;
   char        *fileNamePtr;
   s16         display;
   s16         dbl = 0;
   time_t      timer;

   time (&timer);

   if (!readAreaInfo())
   {
      fileNamePtr = getDestFileName (" Destination file ");
      if (*fileNamePtr == 0)
      {
         freeAreaInfo ();
         return (0);
      }

     working ();

     if ((textFile = fopen(fileNamePtr, "wt")) == NULL)
      {
         displayMessage ("Can't open output file");
         freeAreaInfo ();
         return (0);
      }
      fprintf(textFile, "\n%s  -  List of "MBNAME" message base boards  -  %s%s\n\n", VersionStr(), ctime(&timer), bar);

      for (count1 = 1; count1 <= MBBOARDS; count1++)
      {
         display = 0;
         for (count2 = 0; count2 < areaInfoCount; count2++)
         {
            if ((areaInfo[count2]->board != 0) &&
                (areaInfo[count2]->board == count1))
            {
               fprintf(textFile, (strlen(areaInfo[count2]->areaName) < ECHONAME_LEN_OLD) ? "%3u %-24s %s\n" : "%3u %s\n                             %s\n",
                                  count1,
                                  areaInfo[count2]->areaName,
                                  areaInfo[count2]->comment);
               if (display)
                  dbl++;
               else
                  display++;
            }
         }
         for (count2 = 0; count2 < MAX_NETAKAS; count2++)
         {
            if (config.netmailBoard[count2] == count1)
            {
               if (count2 == 0)
                  fprintf (textFile, "%3u Netmail Main             %s\n", count1, config.descrAKA[count2]);
               else
                  fprintf (textFile, "%3u Netmail AKA %-2u           %s\n", count1, count2, config.descrAKA[count2]);
               if (display)
                  dbl++;
               else
                  display++;
            }
         }
         if (config.dupBoard == count1)
         {
            fprintf (textFile, "%3u Duplicate messages\n", count1);
            if (display)
               dbl++;
            else
               display++;
         }
         if (config.badBoard == count1)
         {
            fprintf (textFile, "%3u Bad messages\n", count1);
            if (display)
               dbl++;
            else
               display++;
         }
         if (config.recBoard == count1)
         {
            fprintf (textFile, "%3u Recovery board\n", count1);
            if (display)
               dbl++;
            else
               display++;
         }
         if (!display)
         {
            fprintf (textFile, "%3u -\n", count1);
         }
      }
      if (dbl)
         fprintf (textFile, "\n--- One or more board numbers have been used more than once.\n");

      fclose (textFile);
      freeAreaInfo ();
   }
   return (0);
}



s16 listJAMBoards(void)
{
   u16         count;
   FILE        *textFile;
   char        *fileNamePtr;
   time_t      timer;

   time (&timer);

   if (!readAreaInfo())
   {
      fileNamePtr = getDestFileName (" Destination file ");
      if (*fileNamePtr == 0)
      {
         freeAreaInfo ();
         return (0);
      }

     working ();

     if ((textFile = fopen(fileNamePtr, "wt")) == NULL)
      {
         displayMessage ("Can't open output file");
         freeAreaInfo ();
         return (0);
      }
      fprintf (textFile, "\n%s  -  List of JAM message areas  -  %s%s\n\n", VersionStr(), ctime(&timer), bar);

      for (count = 0; count < areaInfoCount; count++)
      {
	 if ( *areaInfo[count]->msgBasePath )
	    fprintf (textFile, "%-24s %s\n                         %s\n",
			  areaInfo[count]->areaName,
                          areaInfo[count]->comment,
			  areaInfo[count]->msgBasePath);
      }
      fclose (textFile);
      freeAreaInfo ();
   }
   return (0);
}



s16 listPtAreas (void)
{
   u16         count;
   FILE        *textFile;
   char        *fileNamePtr;
   time_t      timer;

   time (&timer);

   if (!readAreaInfo())
   {
      fileNamePtr = getDestFileName (" Destination file ");
      if (*fileNamePtr == 0)
      {
         freeAreaInfo ();
         return (0);
      }

      working ();

      if ((textFile = fopen(fileNamePtr, "wt")) == NULL)
      {
         displayMessage("Can't open output file");
         freeAreaInfo();
         return 0;
      }
      fprintf (textFile, "\n%s  -  List of passthrough areas  -  %s%s\n\n", VersionStr(), ctime(&timer), bar);

      for (count = 0; count < areaInfoCount; count++)
      {
         if ((areaInfo[count]->options.active) &&
             (!areaInfo[count]->options.local) &&
             (areaInfo[count]->board == 0) &&
             !*areaInfo[count]->msgBasePath)
         {
            fprintf (textFile, (strlen(areaInfo[count]->areaName) < ECHONAME_LEN_OLD)?"%-24s   %s\n":"%s\n                           %s\n",
                               areaInfo[count]->areaName,
                               areaInfo[count]->comment);
         }
      }

      fclose (textFile);
      freeAreaInfo ();
   }
   return (0);
}



s16 listPackConfig (void)
{
   u16         count;
   FILE        *textFile;
   fhandle     packHandle;
   char        *fileNamePtr;
   tempStrType tempStr;
   time_t      timer;

   time (&timer);

   fileNamePtr = getDestFileName (" Destination file ");
   if (*fileNamePtr == 0)
   {
      return (0);
   }

   working ();

   if ((textFile = fopen(fileNamePtr, "wt")) == NULL)
   {
      displayMessage ("Can't open output file");
      return (0);
   }
   strcpy(stpcpy(tempStr, configPath), dPCKFNAME);
   if ((packHandle = open(tempStr, O_RDONLY | O_BINARY)) == -1)
   {
      displayMessage ("Can't open "dPCKFNAME);
      return (0);
   }
   fprintf(textFile, "\n%s  -  Pack Manager  -  %s%s\n\n", VersionStr(), ctime(&timer), bar);
   count = 0;
   while (read(packHandle, tempStr, PACK_STR_SIZE) == PACK_STR_SIZE && *tempStr)
       fprintf (textFile, "%3u.   %s\n", ++count, tempStr);

   fclose(textFile);
   close(packHandle);
   return 0;
}



s16 listAreaConfig (void)
{
   u16    count1
        , count2
        , exportCount
        , rows1
        , rows2;
   u32    bitField;
   FILE  *textFile;
   char  *fileNamePtr;
   time_t timer;

   time(&timer);

   if (!readAreaInfo())
   {
      fileNamePtr = getDestFileName(" Destination file ");
      if (*fileNamePtr == 0)
      {
         freeAreaInfo ();
         return (0);
      }

     working();

     if ((textFile = fopen(fileNamePtr, "wt")) == NULL)
      {
         displayMessage("Can't open output file");
         freeAreaInfo();

         return 0;
      }
      fprintf(textFile, "\n%s  -  Area configuration  -  %s%s\n\n", VersionStr(), ctime(&timer), bar);

      fprintf(textFile, "Switches:  A - Active                       D - Use arrival date when purging\n"
                        "           L - Local                        N - Do not delete messages that\n"
                        "           S - Secure                           have not been read by the SysOp\n"
                        "           P - Allow private messages\n"
                        "           R - Rescan allowed\n"
                        "           U - Use SEEN-BYs\n"
                        "           T - Tiny SEEN-BYs\n"
                        "           I - Import SEEN-BYs\n"
                        "           Y - Tiny PATH\n"
             );


      for (count1 = 0; count1 < areaInfoCount; count1++)
      {
         fprintf(textFile, "\n-------------------------------------------------------------------------------\n\n"
                            "Area        : %-50s        Group %c\n\n",
                            areaInfo[count1]->areaName,
                            getGroupChar (areaInfo[count1]->group));

         if (*areaInfo[count1]->comment != 0)
            fprintf (textFile, "Comment     : %s\n",
                               areaInfo[count1]->comment);

         fprintf (textFile, "Board       : ");
         if (areaInfo[count1]->board == 0)
            fprintf (textFile, "None\n");
         else
            fprintf (textFile, "%u\n", areaInfo[count1]->board);

         fprintf (textFile, "JAM path    : ");
         if (*areaInfo[count1]->msgBasePath == 0)
            fprintf (textFile, "None\n");
         else
            fprintf (textFile, "%s\n", areaInfo[count1]->msgBasePath);

         fprintf (textFile, "Board code  : ");
         if (areaInfo[count1]->boardNumRA == 0)
            fprintf (textFile, "None\n");
         else
            fprintf (textFile, "%u\n", areaInfo[count1]->boardNumRA);

         fprintf (textFile, "Switches    : %c%c%c%c%c%c%c%c%c %c%c\n",
                  areaInfo[count1]->options.active       ? 'A':'-',
                  areaInfo[count1]->options.local        ? 'L':'-',
                  areaInfo[count1]->options.security     ? 'S':'-',
                  areaInfo[count1]->options.allowPrivate ? 'P':'-',
                  areaInfo[count1]->options.noRescan     ? '-':'R',  // Reversed option!
                  areaInfo[count1]->options.checkSeenBy  ? 'U':'-',
                  areaInfo[count1]->options.tinySeenBy   ? 'T':'-',
                  areaInfo[count1]->options.impSeenBy    ? 'I':'-',
                  areaInfo[count1]->options.tinyPath     ? 'Y':'-',
                  areaInfo[count1]->options.arrivalDate  ? 'D':'-',
                  areaInfo[count1]->options.sysopRead    ? 'N':'-');

         if (areaInfo[count1]->msgs != 0)
	    fprintf (textFile, "# Messages  : %u\n",
                               areaInfo[count1]->msgs);

         if (areaInfo[count1]->days != 0)
	    fprintf (textFile, "# Days old  : %u\n",
                               areaInfo[count1]->days);

	 if (areaInfo[count1]->daysRcvd != 0)
	    fprintf (textFile, "# Days rcvd : %u\n",
			       areaInfo[count1]->daysRcvd);

         if (areaInfo[count1]->readSecRA     ||
             areaInfo[count1]->writeSecRA    ||
             areaInfo[count1]->sysopSecRA    ||
             areaInfo[count1]->flagsRdRA[0]  ||
             areaInfo[count1]->flagsWrRA[0]  ||
             areaInfo[count1]->flagsSysRA[0] ||
             areaInfo[count1]->flagsRdRA[1]  ||
             areaInfo[count1]->flagsWrRA[1]  ||
             areaInfo[count1]->flagsSysRA[1] ||
             areaInfo[count1]->flagsRdRA[2]  ||
             areaInfo[count1]->flagsWrRA[2]  ||
             areaInfo[count1]->flagsSysRA[2] ||
             areaInfo[count1]->flagsRdRA[3]  ||
             areaInfo[count1]->flagsWrRA[3]  ||
             areaInfo[count1]->flagsSysRA[3] ||
             areaInfo[count1]->flagsSysRA[3])
         {
            fprintf (textFile, "RA          :         Read %-5u   Write %-5u  SysOp %-5u\n",
                               areaInfo[count1]->readSecRA,
                               areaInfo[count1]->writeSecRA,
                               areaInfo[count1]->sysopSecRA);
            for (count2 = 0; count2 < 4; count2++)
            {
               fprintf (textFile, "              Flag %c  %s     %s     %s\n", 'A'+count2,
                                  bitData(areaInfo[count1]->flagsRdRA[count2]),
                                  bitData(areaInfo[count1]->flagsWrRA[count2]),
                                  bitData(areaInfo[count1]->flagsSysRA[count2]));
            }
         }
         fprintf (textFile, "Origin      : ");
         if (config.akaList[areaInfo[count1]->address].nodeNum.zone == 0)
            fprintf (textFile, "<not defined>");
         else
            fprintf (textFile, nodeStr(&(config.akaList[areaInfo[count1]->address].nodeNum)));
         if (areaInfo[count1]->address == 0)
            fprintf (textFile, " (main)\n");
         else
            fprintf (textFile, " (AKA %u)\n",
                               areaInfo[count1]->address);

         if (*areaInfo[count1]->originLine)
         {
            fprintf (textFile, "Origin line : %s\n", areaInfo[count1]->originLine);
         }

	 if ((bitField = areaInfo[count1]->alsoSeenBy) != 0)
	 {
	    fprintf (textFile, "\nSeen by\n\n");
	    for (count2 = 0; count2 < MAX_AKAS; count2++)
	    {
	       if (bitField & 1L)
	       {
		  if (count2 == 0)
		     fprintf (textFile, "   Main   : ");
		  else
		     fprintf (textFile, "   AKA %2u : ", count2);
		  if (config.akaList[count2].nodeNum.zone == 0)
		     fprintf (textFile, "<not defined>\n");
		  else
		     fprintf (textFile, "%s\n",
					nodeStr(&(config.akaList[count2].nodeNum)));
	       }
	       bitField >>= 1;
	    }
	 }

         exportCount = 0;
         while (areaInfo[count1]->forwards[exportCount].nodeNum.zone != 0)
         {
            exportCount++;
         }
         if (exportCount != 0)
         {
            fputs("\nExport\n", textFile);
            rows1 = (exportCount+2)/3;
            rows2 = (exportCount+1)/3;
            for (count2 = 0; count2 < rows1; count2++)
            {
               fprintf (textFile, "\n %c %-24s",
                                  areaInfo[count1]->forwards[count2].flags.readOnly ? 'R':
                                  areaInfo[count1]->forwards[count2].flags.writeOnly ? 'W':
                                  areaInfo[count1]->forwards[count2].flags.locked ? 'L': ' ',
                                  nodeStr(&areaInfo[count1]->forwards[count2].nodeNum));

               if (count2 < rows2)
                  fprintf (textFile, " %c %-24s",
                                     areaInfo[count1]->forwards[count2+rows1].flags.readOnly ? 'R':
                                     areaInfo[count1]->forwards[count2+rows1].flags.writeOnly ? 'W':
                                     areaInfo[count1]->forwards[count2+rows1].flags.locked ? 'L': ' ',
                                     nodeStr(&areaInfo[count1]->forwards[count2+rows1].nodeNum));
               if (count2 < exportCount/3)
                  fprintf (textFile, " %c %-24s",
                                     areaInfo[count1]->forwards[count2+rows1+rows2].flags.readOnly ? 'R':
                                     areaInfo[count1]->forwards[count2+rows1+rows2].flags.writeOnly ? 'W':
                                     areaInfo[count1]->forwards[count2+rows1+rows2].flags.locked ? 'L': ' ',
                                     nodeStr(&areaInfo[count1]->forwards[count2+rows1+rows2].nodeNum));
            }
            fputc('\n', textFile);
         }
      }
      if (areaInfoCount == 0)
         fputs("\nThere are no areas defined.\n", textFile);
      else
         fputs("\n-------------------------------------------------------------------------------\n", textFile);

      fclose(textFile);
      freeAreaInfo();
   }
   return 0;
}
//---------------------------------------------------------------------------
s16 listNodeConfig (void)
{
   FILE        *textFile;
   char        *fileNamePtr;
   u16         count;
   char        groupString[27];
   time_t      timer;
   headerType  *nodeHeader;
   nodeInfoType *nodeBuf;

   time (&timer);

   fileNamePtr = getDestFileName (" Destination file ");
   if (*fileNamePtr == 0)
      return (0);

   working ();

   if ((textFile = fopen(fileNamePtr, "wt")) == NULL)
   {
      displayMessage ("Can't open output file");
      return (0);
   }
   openConfig(CFG_NODES, &nodeHeader, (void*)&nodeBuf);
   nodeInfoCount = nodeHeader->totalRecords;

   fprintf(textFile, "\n%s  -  Node configuration  -  %s%s\n\n", VersionStr(), ctime(&timer), bar);

   fprintf(textFile, "Switches:  A - Active\n"
                     "           P - Pack netmail\n"
                     "           R - Route point\n"
                     "           F - Forward requests\n"
                     "           N - Notify\n"
                     "           M - Remote maintenance\n"
                     "           S - Allow rescan\n"
                     "           D - Reformat date\n"
                     "           T - Tiny SEEN-BY\n"
          );

   for (count = 0; count < nodeInfoCount; count++)
   {
      getRec(CFG_NODES, count);

      fprintf(textFile, "\n-------------------------------------------------------------------------------\n"
                        "\nSystem           : %s\n", nodeStr(&(nodeBuf->node)));
      if (nodeBuf->viaNode.zone)
         fprintf (textFile, "Via system       : %s\n", nodeStr(&(nodeBuf->viaNode)));

      if (*nodeBuf->sysopName)
         fprintf (textFile, "SysOp            : %s\n\n", nodeBuf->sysopName);

      fprintf (textFile, "Groups           : %s\n", getGroupString(nodeBuf->groups, groupString));
      fprintf (textFile, "Write level      : %u\n", nodeBuf->writeLevel);

      fprintf (textFile, "Capability       : ");
      switch (nodeBuf->capability)
      {
         case 0  : fputs("Stone Age", textFile);
                   break;
         case 1  : fputs("Type 2+", textFile);
                   break;
         default : fputs("** unknown **", textFile);
                   break;
      }
      fputs("\nArchiver         : ", textFile);
      switch (nodeBuf->archiver)
      {
         case 0  : fputs("ARC", textFile);
                   break;
         case 1  : fputs("ZIP", textFile);
                   break;
         case 2  : fputs("LZH", textFile);
                   break;
         case 3  : fputs("PAK", textFile);
                   break;
         case 4  : fputs("ZOO", textFile);
                   break;
         case 5  : fputs("ARJ", textFile);
                   break;
         case 6  : fputs("SQZ", textFile);
                   break;
         case 8  : fputs("UC2", textFile);
                   break;
         case 9  : fputs("RAR", textFile);
                   break;
         case 10 : fputs("JAR", textFile);
                   break;
         case 0xFF:fputs("None", textFile);
                   break;
         default : fputs("** unknown **", textFile);
                   break;
      }
      fputs("\nFile Att status  : ", textFile);
      switch (nodeBuf->outStatus)
      {
         case 0  : fputs("None", textFile);
                   break;
         case 1  : fputs("Hold", textFile);
                   break;
         case 2  : fputs("Crash", textFile);
                   break;
         case 3  : fputs("Hold/Direct", textFile);
                   break;
         case 4  : fputs("Crash/Direct", textFile);
                   break;
         case 5  : fputs("Direct", textFile);
                   break;
         default : fputs("** unknown **", textFile);
                   break;
      }

      fprintf(textFile, "\nSwitches         : %c%c%c%c%c%c%c%c%c\n"
                      , nodeBuf->options.active      ? 'A':'-'
                      , nodeBuf->options.packNetmail ? 'P':'-'
                      , nodeBuf->options.routeToPoint? 'R':'-'
                      , nodeBuf->options.forwardReq  ? 'F':'-'
                      , nodeBuf->options.notify      ? 'N':'-'
                      , nodeBuf->options.remMaint    ? 'M':'-'
                      , nodeBuf->options.allowRescan ? 'S':'-'
                      , nodeBuf->options.fixDate     ? 'D':'-'
                      , nodeBuf->options.tinySeenBy  ? 'T':'-'
             );

      fprintf(textFile, "AreaMgr password : %s\n", nodeBuf->password);
      fprintf(textFile, "Packet password  : %s\n", nodeBuf->packetPwd);
      fprintf(textFile, "ÈÍ> Ignore pwd   : %s\n", nodeBuf->options.ignorePwd ? "Yes":"No");
   }
   if (nodeInfoCount == 0)
      fputs("\nThere are no nodes defined.\n", textFile);
   else
      fputs("\n-------------------------------------------------------------------------------\n", textFile);

   fclose(textFile);
   closeConfig(CFG_NODES);

   return 0;
}
//---------------------------------------------------------------------------
s16 listGeneralConfig (void)
{
   FILE    *textFile;
   char    *fileNamePtr;
   u16      count
          , count2;
   time_t   timer;
   char     numStr[8];
   int      archiver = config.defaultArc;

   time(&timer);

   fileNamePtr = getDestFileName(" Destination file ");
   if (*fileNamePtr == 0)
      return 0;

   working();

   if ((textFile = fopen(fileNamePtr, "wt")) == NULL)
   {
      displayMessage ("Can't open output file");
      return 0;
   }
   fprintf (textFile, "\n%s  -  General configuration  -  %s%s\n\n", VersionStr(), ctime(&timer), bar);

   fprintf (textFile, "SysOp Name         : %s\n\n", config.sysopName);
   fprintf (textFile, "Mailer             : %s\n", mailerName[config.mailer]);
   fprintf (textFile, "BBS program        : %s\n\n", bbsName[config.bbsProgram]);
   fprintf (textFile, "Pers. mail path    : %s\n", *config.pmailPath ? config.pmailPath : "Not defined");
   fprintf (textFile, "+-> Topic 1        : %s\n", config.topic1);
   fprintf (textFile, "+-> Topic 2        : %s\n", config.topic2);
   fprintf (textFile, "Include netmail    : %s\n\n", config.mailOptions.persNetmail ? "Yes" : "No");
   fprintf (textFile, "Incl.sent by SysOp : %s\n\n", config.mbOptions.persSent ? "Yes" : "No");

   fprintf (textFile, "Log file name      : %s\n", *config.logName ? config.logName : "Not defined");
   fprintf (textFile, "Area Manager log   : %s\n", *config.areaMgrLogName ? config.areaMgrLogName : "Not defined");
   fprintf (textFile, "Toss summary       : %s\n\n", *config.summaryLogName?config.summaryLogName : "Not defined");

   fprintf (textFile, "Dup detection      : %s\n", config.mailOptions.dupDetection ? "Yes":"No");
   fprintf (textFile, "Maximum PKT size   : ");
      if (config.maxPktSize == 0)
         fprintf (textFile, "Not defined\n");
      else
         fprintf (textFile, "%u\n", config.maxPktSize);
   fprintf (textFile, "Maximum bundle size: ");
      if (config.maxBundleSize == 0)
         fprintf (textFile, "Not defined\n");
      else
         fprintf (textFile, "%u\n", config.maxBundleSize);
   fprintf (textFile, "Kill empty netmail : %s\n", config.mailOptions.killEmptyNetmail ? "Yes":"No");
   fprintf (textFile, "Kill bad ARCmail   : %s\n", config.mailOptions.killBadFAtt ? "Yes":"No");
   fprintf (textFile, "ARCmail compatible : %s\n", config.mailOptions.ARCmail060 ? "Yes":"No");
   fprintf (textFile, "Ext. bundle names  : %s\n", config.mailOptions.extNames ? "Yes":"No");
   fprintf (textFile, "Check PKT dest.    : %s\n", config.mailOptions.checkPktDest ? "Yes":"No");
   fprintf (textFile, "Max net messages   : ");
      if (config.allowedNumNetmail == 0)
         fprintf (textFile, "Not defined\n");
      else
         fprintf (textFile, "%u\n", config.allowedNumNetmail);
   fprintf (textFile, "Keep exported netm.: %s\n", config.mailOptions.keepExpNetmail ? "Yes":"No");
   fprintf (textFile, "Remove net kludges : %s\n", config.mailOptions.removeNetKludges ? "Yes":"No");
   fprintf (textFile, "Set Pvt on import  : %s\n\n", config.mailOptions.privateImport ? "Yes":"No");

   fprintf (textFile, "Keep requests      : %s\n", config.mgrOptions.keepRequest   ? "Yes":"No");
   fprintf (textFile, "Keep receipts      : %s\n", config.mgrOptions.keepReceipt   ? "Yes":"No");
   fprintf (textFile, "Allow PASSWORD     : %s\n", config.mgrOptions.allowPassword ? "Yes":"No");
   fprintf (textFile, "Allow PKTPWD       : %s\n", config.mgrOptions.allowPktPwd   ? "Yes":"No");
   fprintf (textFile, "Allow ACTIVE       : %s\n", config.mgrOptions.allowActive   ? "Yes":"No");
   fprintf (textFile, "Allow COMPRESSION  : %s\n", config.mgrOptions.allowCompr    ? "Yes":"No");
   fprintf (textFile, "Allow NOTIFY       : %s\n", config.mgrOptions.allowNotify   ? "Yes":"No");
   fprintf (textFile, "Allow +ALL         : %s\n", config.mgrOptions.allowAddAll   ? "Yes":"No");
   fprintf (textFile, "Auto-disconnect    : %s\n\n", config.mgrOptions.autoDiscArea  ? "Yes":"No");

   fprintf (textFile, "Sort new messages  : %s\n", config.mbOptions.sortNew ? "Yes":"No");
   fprintf (textFile, "+-> Use subject    : %s\n", config.mbOptions.sortSubject ? "Yes":"No");
   fprintf (textFile, "Update reply chains: %s\n", config.mbOptions.updateChains? "Yes":"No");
   fprintf (textFile, "Quick toss         : %s\n", config.mbOptions.quickToss ? "Yes":"No");
   fprintf (textFile, "Scan Always        : %s\n", config.mbOptions.scanAlways ? "Yes":"No");
// fprintf (textFile, "Msg base sharing   : %s\n", config.mbOptions.mbSharing ? "Yes":"No");
   fputc('\n', textFile);

   fprintf (textFile, "Import netmail\nto SysOp           : %s\n", config.mbOptions.sysopImport ? "Yes":"No");
   fprintf (textFile, "Remove Re:         : %s\n"  , config.mbOptions.removeRe ? "Yes":"No");
   fprintf (textFile, "Strip soft cr      : %s\n"  , config.mbOptions.removeSr ? "Yes":"No");
   fprintf (textFile, "Strip lf           : %s\n\n", config.mbOptions.removeLf ? "Yes":"No");

   fprintf (textFile, "Monochrome         : %s\n\n", config.genOptions.monochrome ? "Yes":"No");

   count2 = 0;
   for (count = 0; count < MAX_NETAKAS; count++)
   {
      if (config.netmailBoard[count])
      {
         if (count2++ == 0)
         {
           fprintf(textFile, "Netmail boards\n");
         }
         if (count == 0)
            fprintf(textFile, "\n  Main              : ");
         else
            fprintf(textFile, "\n  AKA %-2u            : ", count);
         fprintf(textFile, "%u\n", config.netmailBoard[count]);

         fprintf(textFile, "    Comment         : %-51s\n", config.descrAKA[count]);
         fprintf(textFile, "    # Messages      : %u\n", config.msgsAKA[count]);
         fprintf(textFile, "    # Days old      : %u\n", config.daysAKA[count]);
         fprintf(textFile, "    # Days rcvd     : %u\n", config.daysRcvdAKA[count]);
      }
   }

   fprintf(textFile, "\nBad messages       : %s\n", config.badBoard ? itoa(config.badBoard, numStr, 10) : "Not defined");
   fprintf(textFile, "Duplicate messages : %s\n", config.dupBoard ? itoa(config.dupBoard, numStr, 10) : "Not defined");
   fprintf(textFile, "Recovery board     : %s\n\n", config.recBoard ? itoa(config.recBoard, numStr, 10) : "Not defined");

   fprintf(textFile, "Message base       : %s\n", config.bbsPath);
   fprintf(textFile, "Netmail messages   : %s\n", config.netPath);
   fprintf(textFile, "Incoming mail      : %s\n", config.inPath);
   fprintf(textFile, "Outgoing mail      : %s\n", config.outPath);
   fprintf(textFile, "Outgoing backup    : %s\n\n", config.outBakPath);

   fprintf(textFile, "Local PKTs         : %s\n", *config.securePath ? config.securePath : "Not defined");
   fprintf(textFile, "Semaphore          : %s\n", *config.semaphorePath ? config.semaphorePath : "Not defined");
   fprintf(textFile, "Sent netmail       : %s\n", *config.sentPath ? config.sentPath : "Not defined");
   fprintf(textFile, "Rcvd netmail       : %s\n", *config.rcvdPath ? config.rcvdPath : "Not defined");

   if (archiver > 10)
     archiver = 0;
   else
     archiver++;

   fprintf(textFile, "\n"
          "Compression programs\n"
          "\n"
		      "           Default : %s\n"
		      "           ARC     : %s\n"
		      "           ZIP     : %s\n"
		      "           LZH     : %s\n"
		      "           PAK     : %s\n"
		      "           ZOO     : %s\n"
		      "           ARJ     : %s\n"
		      "           SQZ     : %s\n"
          "           UC2     : %s\n"
          "           RAR     : %s\n"
          "           JAR     : %s\n"
          "           -?-     : %s\n"
          , arcName[archiver]
          , config.arc32.programName
          , config.zip32.programName
          , config.lzh32.programName
          , config.pak32.programName
          , config.zoo32.programName
          , config.arj32.programName
          , config.sqz32.programName
          , config.uc232.programName
          , config.rar32.programName
          , config.jar32.programName
          , config.customArc32.programName
          );

   fprintf(textFile, "\n"
          "Decompression programs\n"
          "\n"
		      "           ARC     : %s\n"
		      "           ZIP     : %s\n"
		      "           LZH     : %s\n"
		      "           PAK     : %s\n"
		      "           ZOO     : %s\n"
		      "           ARJ     : %s\n"
		      "           SQZ     : %s\n"
          "           UC2     : %s\n"
          "           RAR     : %s\n"
          "           JAR     : %s\n"
          "           GUS     : %s\n"
          , config.unArc32.programName
          , config.unZip32.programName
          , config.unLzh32.programName
          , config.unPak32.programName
          , config.unZoo32.programName
          , config.unArj32.programName
          , config.unSqz32.programName
          , config.unUc232.programName
          , config.unRar32.programName
          , config.unJar32.programName
          , config.GUS32  .programName
          );

   fclose(textFile);
   return 0;
}
//---------------------------------------------------------------------------
