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
#include <io.h>
#include <dir.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fmail.h"
#include "fjlib.h"
#include "config.h"
#include "cfgfile.h"
#include "nodeinfo.h"
#include "window.h"
#include "fs_util.h"
#include "import.h"
#include "msgra.h"

#include "fdfolder.h"
#include "gestruct.h"
#include "fecfg146.h"



static rawEchoType     *echoAreaList[MAX_AREAS];


/* IMAIL status mapping */
char IMSTAT[8] = {0,1,2,0,5,3,4,5};


static void swapBits(u8 *ptr)
{
   u8 temp;

   temp = 0;
   if ( (*ptr & BIT0) )
      temp |= BIT7;
   if ( (*ptr & BIT1) )
      temp |= BIT6;
   if ( (*ptr & BIT2) )
      temp |= BIT5;
   if ( (*ptr & BIT3) )
      temp |= BIT4;
   if ( (*ptr & BIT4) )
      temp |= BIT3;
   if ( (*ptr & BIT5) )
      temp |= BIT2;
   if ( (*ptr & BIT6) )
      temp |= BIT1;
   if ( (*ptr & BIT7) )
      temp |= BIT0;
   *ptr = temp;
}


static s16 greater(nodeNumType *node1, nodeNumType *node2)
{
   return node1->zone   > node2->zone ||
	  (node1->zone == node2->zone && node1->net   > node2->net) ||
	  (node1->zone == node2->zone && node1->net  == node2->net &&
	   node1->node  > node2->node) ||
	  (node1->zone == node2->zone && node1->net  == node2->net &&
	   node1->node == node2->node && node1->point > node2->point);
}



static s16 node4d(nodeNumType *node)
{
   u16	count;

   count = 0;
   while ( (count < MAX_AKAS) &&
	   ((config.akaList[count].nodeNum.zone == 0) ||
	    (node->net != config.akaList[count].fakeNet) ||
	    (node->point != 0)) )
   {  count++;
   }
   if ( count < MAX_AKAS )
   {  node->point = node->node;
      memcpy(node, &config.akaList[count].nodeNum, 6);
      return count;
   }
   return -1;
}



static void releaseAreas(u16 echoCount)
{
   while ( echoCount-- )
      free(echoAreaList[echoCount]);
}



static int askFMArExist(u16 *echoCount)
{
   int          ch;
   u16          count;
   struct ffblk testBlk;
   headerType	*areaHeader;
   rawEchoType  *areaBuf;
   tempStrType  tempStr;

   *echoCount = 0;
   ch = 'O';
   strcpy(tempStr, configPath);
   strcat(tempStr, "FMAIL.AR");
   if ( findfirst(tempStr, &testBlk, 0) == 0 )
   {  if ( (ch = askChar("FMAIL.AR already exists. [A]dd, [O]verwrite, [C]ancel ?", "ACO")) == 'C' || !ch )
	 return 0;
      if ( ch == 'A' )
      {  if ( !openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf) )
	 {  displayMessage("Can't open file FMAIL.AR for input");
	    return 0;
	 }
	 count = areaHeader->totalRecords;
	 while ( count-- )
	 {  if ( (echoAreaList[*echoCount] = malloc(RAWECHO_SIZE)) == NULL )
	    {  displayMessage("Out of memory");
	       releaseAreas(*echoCount);
	       return 0;
	    }
	    getRec(CFG_ECHOAREAS, *echoCount);
	    memcpy(echoAreaList[(*echoCount)++], areaBuf, RAWECHO_SIZE);
	 }
	 closeConfig(CFG_ECHOAREAS);
      }
   }
   return ch;
}



static void updateAreas(u16 echoCount)
{
   u16          count;
   headerType	*areaHeader;
   rawEchoType  *areaBuf;

   if ( !openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf) )
   {  displayMessage("Can't open file FMAIL.AR for output");
      releaseAreas(echoCount);
      return;
   }
   for ( count = 0; count < echoCount; count++ )
   {  memcpy(areaBuf, echoAreaList[count], RAWECHO_SIZE);
      putRec(CFG_ECHOAREAS, count);
      free(echoAreaList[count]);
   }
   chgNumRec(CFG_ECHOAREAS, echoCount);
   closeConfig(CFG_ECHOAREAS);
}



static void releaseNodes(u16 nodeCount)
{
   while ( nodeCount-- )
      free(nodeInfo[nodeCount]);
}



static int askFMNodExist(u16 *nodeCount)
{
   int          ch;
   u16          count;
   struct ffblk testBlk;
   headerType   *nodeHeader;
   rawEchoType  *nodeBuf;
   tempStrType  tempStr;

   *nodeCount = 0;
   ch = 'O';
   strcpy(tempStr, configPath);
   strcat(tempStr, "FMAIL.NOD");
   if ( findfirst(tempStr, &testBlk, 0) == 0 )
   {  if ( (ch = askChar("FMAIL.NOD already exists. [A]dd, [O]verwrite, [C]ancel ?", "ACO")) == 'C' || !ch )
	 return 0;
      if ( ch == 'A' )
      {  if ( !openConfig(CFG_NODES, &nodeHeader, (void*)&nodeBuf) )
	 {  displayMessage("Can't open file FMAIL.NOD for input");
	    return 0;
	 }
	 count = nodeHeader->totalRecords;
	 while ( count-- )
	 {  if ( (nodeInfo[*nodeCount] = malloc(sizeof(nodeInfoType))) == NULL )
	    {  displayMessage("Out of memory");
	       releaseNodes(*nodeCount);
	       return 0;
	    }
	    getRec(CFG_NODES, *nodeCount);
	    memcpy(nodeInfo[(*nodeCount)++], nodeBuf, sizeof(nodeInfoType));
	 }
	 closeConfig(CFG_NODES);
      }
   }
   return ch;
}



static void updateNodes(u16 nodeCount)
{
   u16          count;
   headerType   *nodeHeader;
   nodeInfoType *nodeBuf;

   if ( !openConfig(CFG_NODES, &nodeHeader, (void*)&nodeBuf) )
   {  displayMessage("Can't open file FMail.Nod for output");
      releaseNodes(nodeCount);
      return;
   }
   for ( count = 0; count < nodeCount; count++ )
   {  memcpy(nodeBuf, nodeInfo[count], sizeof(nodeInfoType));
      putRec(CFG_NODES,count);
      free(nodeInfo[count]);
   }
   chgNumRec(CFG_NODES, nodeCount);
   closeConfig(CFG_NODES);
}


		  
static u16 checkDupArea(rawEchoType *echoAreaRec, u16 *echoCount, u16 *skiparname)
{
   u16 low, mid, high;
   tempStrType  tempStr;

   low = 0;
   high = *echoCount;
   while ( low < high )
   {  mid = (low + high) >> 1;
      if ( stricmp(echoAreaRec->areaName, echoAreaList[mid]->areaName) > 0 )
	 low = mid+1;
      else
	 high = mid;
   }
   if ( *echoCount > low &&
	stricmp(echoAreaRec->areaName, echoAreaList[low]->areaName) == 0 )
   {  sprintf(tempStr, "%.32s is listed more than once. [S]kip, [A]bort ?",echoAreaRec->areaName);
      free(echoAreaRec);
      if ( !*skiparname && askChar(tempStr, "AS") != 'S' )
      {  releaseAreas(*echoCount);
	 return 0;
      }
      *skiparname = 1;
   }
   else
   {  memmove(&echoAreaList[low+1], &echoAreaList[low],
              ((*echoCount)++-low) * 4); /* 4 = sizeof(ptr) */
      echoAreaList[low] = echoAreaRec;
   }
   return 1;
}



static u16 checkDupBoard(u8 *echoNumCheck, rawEchoType *echoAreaRec, u16 *echoCount, u16 *clearboard)
{
   tempStrType tempStr;

   if ( echoAreaRec->board > 0 &&
	echoAreaRec->board <= MBBOARDS &&
	echoNumCheck[echoAreaRec->board-1]++ != 0 )
   {  sprintf(tempStr, "Board %u is used more than once. [C]lear board, [A]bort ?", echoAreaRec->board);
      if ( !clearboard && askChar(tempStr, "AC") != 'C' )
      {  free(echoAreaRec);
	 releaseAreas(*echoCount);
	 return 0;
      }
      *clearboard = 1;
      echoAreaRec->board = 0;
   }
   return 1;
}



extern rawEchoType echoDefaultsRec;

s16 importAreasBBS(void)
{
   char           *areaLinePtr;
   char           tempStr[1024];
   char           echoNumCheck[MBBOARDS];
   u16            count,
		  scanCount;
   u16            echoCount,
		  nodeCount;
   u16            lastNet = 0,
		  lastNode = 0;
   u16            skiparname = 0, clearboard = 0;
   nodeNumType    nodeRec;
   rawEchoType    *echoAreaRec;
   FILE           *textFile;

   if ( !askFMArExist(&echoCount) )
      return 0;
   memset(echoNumCheck, 0, MBBOARDS);
   if ( (textFile = fopen(getSourceFileName(" Source file "), "rt")) == NULL )
   {  displayMessage("Can't find the requested file.");
      releaseAreas(echoCount);
      return 0;
   }
   working();
   fgets(tempStr, sizeof(tempStr), textFile);
   while ( fgets(tempStr, sizeof(tempStr), textFile) != NULL )
   {  if ( tempStr[0] != ';' &&
	   (areaLinePtr = strtok(tempStr, " \t\n")) != NULL )
      {  if ( echoCount == MAX_AREAS )
	 {  displayMessage("Too many areas listed in Areas.BBS");
	    releaseAreas(echoCount);
	    return 0;
	 }
	 if ( (echoAreaRec = malloc(RAWECHO_SIZE)) == NULL )
	 {  fclose(textFile);
	    displayMessage("Not enough memory available");
	    releaseAreas(echoCount);
	    return 0;
	 }
	 memcpy(echoAreaRec, &echoDefaultsRec, RAWECHO_SIZE);
         if ( *areaLinePtr == '!' )
         {  echoAreaRec->board = 0;
//#pragma message ("NIEUW ipv ^^^!!!")
//       {  echoAreaRec->board = BOARDTYPEJAM;
            strncpy(echoAreaRec->msgBasePath, areaLinePtr+1, MB_PATH_LEN-1);
         }
	 else
	 {  echoAreaRec->board = atoi(areaLinePtr);
            *echoAreaRec->msgBasePath = 0;
            if ( !checkDupBoard(echoNumCheck, echoAreaRec, &echoCount, &clearboard) )
	       return 0;
	 }
	 if ( (areaLinePtr = strtok(NULL, " \t\n")) == NULL )
	    free(echoAreaRec);
	 else
	 {  strupr(areaLinePtr);
	    strncpy(echoAreaRec->areaName, areaLinePtr, ECHONAME_LEN-1);
	    echoAreaRec->group = 1;
	    echoAreaRec->options.active = 1;
	    echoAreaRec->options.allowAreafix = 1;
	    nodeRec.zone  = config.akaList[0].nodeNum.zone;
	    nodeCount = 0;
	    while ( (areaLinePtr = strtok(NULL, " \t\n")) != NULL &&
		    areaLinePtr[0] != ';' )
	    {  nodeRec.point = 0;
	       if( strchr(areaLinePtr, ':') != NULL )
	       {  nodeRec.zone  = atoi(areaLinePtr);
		  areaLinePtr = strchr(areaLinePtr, ':') + 1;
	       }
	       if ( strchr(areaLinePtr, '/') != NULL )
		  scanCount = sscanf(areaLinePtr, "%hd/%hd.%hd", &nodeRec.net,
				     &nodeRec.node, &nodeRec.point);
	       else
	       {  if ( areaLinePtr[0] != '.' )
		  {  scanCount = sscanf(areaLinePtr, "%hd.%hd",
					&nodeRec.node, &nodeRec.point) + 1;
		     nodeRec.net = lastNet;
		  }
		  else
		  {  scanCount = sscanf(areaLinePtr, ".%hd", &nodeRec.point) + 1;
		     nodeRec.net  = lastNet;
		     nodeRec.node = lastNode;
		  }
	       }
	       if ( scanCount >= 2 )
	       {  lastNet  = nodeRec.net;
		  lastNode = nodeRec.node;
		  node4d(&nodeRec);
		  count = 0;
		  while ( count < nodeCount &&
			  greater(&nodeRec, &echoAreaRec->export[count].nodeNum) )
		  {  count++;
		  }
		  if ( memcmp(&nodeRec, &echoAreaRec->export[count].nodeNum,
			      sizeof(nodeNumType)) != 0 )
		  {  memmove(&echoAreaRec->export[count+1],
			     &echoAreaRec->export[count],
			     (nodeCount++-count)*sizeof(nodeNumXType));
		     echoAreaRec->export[count].nodeNum = nodeRec;
		  }
	       }  
	    }
	    if ( areaLinePtr != NULL )
	       strncpy(echoAreaRec->comment, areaLinePtr+1, COMMENT_LEN-1);
	    else
	       strcpy(echoAreaRec->comment, echoAreaRec->areaName);
	    if ( !checkDupArea(echoAreaRec, &echoCount, &skiparname) )
	       return 0;
	    if ( skiparname )
	       continue;
	 }
      }
   }
   fclose(textFile);
   updateAreas(echoCount);
   sprintf(tempStr, "%u areas processed", echoCount);
   displayMessage(tempStr);
   return 0;
}



s16 importFolderFD(void)
{
   fhandle        folderHandle;
   char           tempStr[256];
   char           echoNumCheck[MBBOARDS];
   u16            skiparname = 0, clearboard = 0;
   u16            echoCount;
   rawEchoType    *echoAreaRec;
   FOLDER         folderRec;

   if ( !askFMArExist(&echoCount) )
      return 0;
   memset(echoNumCheck, 0, MBBOARDS);
   if ( (folderHandle = open(getSourceFileName(" Source file "), O_BINARY|O_RDONLY)) == -1 )
   {  displayMessage("Can't find the requested file.");
      releaseAreas(echoCount);
      return 0;
   }
   working();
   while ( _read(folderHandle, &folderRec, sizeof(FOLDER)) == sizeof(FOLDER) )
   {  if ((folderRec.behave & (FD_QUICKBBS|FD_JAMAREA)) &&
	  (folderRec.behave & FD_ECHOMAIL))
      {  if ( echoCount == MAX_AREAS )
	 {  displayMessage("Too many areas listed in Areas.BBS");
	    releaseAreas(echoCount);
	    return 0;
	 }
	 echoAreaRec = malloc(RAWECHO_SIZE);
	 memcpy(echoAreaRec, &echoDefaultsRec, RAWECHO_SIZE);
         if ( folderRec.behave & FD_JAMAREA )
	 {  echoAreaRec->board = 0;
//#pragma message ("NIEUW ipv ^^^!!!")
//       {  echoAreaRec->board = BOARDTYPEJAM;
	    strncpy(echoAreaRec->msgBasePath, folderRec.path, MB_PATH_LEN-1);
         }
	 else
	 {  echoAreaRec->board = folderRec.board;
            *echoAreaRec->msgBasePath = 0;
            if ( !checkDupBoard(echoNumCheck, echoAreaRec, &echoCount, &clearboard) )
	       return 0;
	 }
	 echoAreaRec->group = 1;
	 strncpy(echoAreaRec->comment, folderRec.title, COMMENT_LEN-1);
	 strupr(folderRec.title);
	 strncpy(echoAreaRec->areaName, folderRec.title, ECHONAME_LEN-1);
	 echoAreaRec->address = folderRec.useaka & 0x0f;
	 echoAreaRec->options.allowAreafix = 1;
	 if ( (folderRec.behave & FD_EXPORT_OK) )
	    echoAreaRec->options.active = 1;
	 if ( (folderRec.behave & FD_PRIVATE) )
	    echoAreaRec->options.allowPrivate = 1;
	 if ( !checkDupArea(echoAreaRec, &echoCount, &skiparname) )
	    return 0;
	 if ( skiparname )
	    continue;
      }
   }
   close(folderHandle);
   updateAreas(echoCount);
   sprintf(tempStr, "%u areas processed", echoCount);
   displayMessage(tempStr);
   return 0;
}



s16 importAreaFileFD(void)
{
   fhandle        fileHandle;
   char           tempStr[256];
   char           echoNumCheck[MBBOARDS];
   u16            count,
		  low, mid, high;
   u16            skiparname = 0, clearboard = 0;
   u16            echoCount;
   fdAreaFileType fdAreaFileRec;
   rawEchoType    *echoAreaRec;
   nodeNumType    tempNode;

   if ( !askFMArExist(&echoCount) )
      return 0;
   memset(echoNumCheck, 0, MBBOARDS);
   if ( (fileHandle = open(getSourceFileName(" Source file "),
			   O_BINARY|O_RDWR)) == -1 )
   {  displayMessage("Can't find the requested file.");
      releaseAreas(echoCount);
      return 0;
   }
   lseek(fileHandle, 4, SEEK_SET);
   working();
   while ( _read(fileHandle, &fdAreaFileRec, sizeof(fdAreaFileType)) ==
					     sizeof(fdAreaFileType) )
   {  if ( !(fdAreaFileRec.deleted || fdAreaFileRec.tsAreaFlags.deleted) )
      {
	 if ( echoCount == MAX_AREAS )
	 {  displayMessage("Too many areas listed in AreaFile.FD");
	    releaseAreas(echoCount);
	    return 0;
	 }
	 if ( (echoAreaRec = malloc(RAWECHO_SIZE)) == NULL )
	 {  displayMessage("Out of memory");
	    releaseAreas(echoCount);
	    return 0;
	 }
	 memset(echoAreaRec, 0, RAWECHO_SIZE);
	 strncpy(echoAreaRec->areaName,
	 fdAreaFileRec.areaName, ECHONAME_LEN-1);
	 if (*fdAreaFileRec.comment)
	    strncpy(echoAreaRec->comment, fdAreaFileRec.comment, COMMENT_LEN-1);
	 else
            strncpy(echoAreaRec->comment, fdAreaFileRec.areaName, COMMENT_LEN-1);
         if (fdAreaFileRec.tsAreaFlags.passThrough)
            echoAreaRec->board = 0;
         else
            echoAreaRec->board = fdAreaFileRec.boardNum;
         echoAreaRec->address = fdAreaFileRec.mainAddress & 0x0f;
         echoAreaRec->options.security     = fdAreaFileRec.tsAreaFlags.security;
         echoAreaRec->options.tinySeenBy   = fdAreaFileRec.tsAreaFlags.tinySeenBy;
         echoAreaRec->options.allowPrivate = fdAreaFileRec.tsAreaFlags.pvt;
         echoAreaRec->options.active       = 1;
         echoAreaRec->options.allowAreafix = 1;
         echoAreaRec->group = (fdAreaFileRec.group < 'A' ||
                               fdAreaFileRec.group > 'Z') ?
                               1 : (1L << (fdAreaFileRec.group - 'A'));
         echoAreaRec->msgs = fdAreaFileRec.numMsgs;
         echoAreaRec->days = fdAreaFileRec.numDays;
         count = 0;
         while ( count < 60  &&  fdAreaFileRec.export[count].zone != 0 )
         {  tempNode = fdAreaFileRec.export[count];
            node4d(&tempNode);
            low = 0;
            high = count;
            while ( low < high )
            {  mid = (low + high) >> 1;
               if ( greater(&tempNode, &echoAreaRec->export[mid].nodeNum) )
                  low = mid+1;
               else
                  high = mid;
            }
            memmove(&echoAreaRec->export[low+1], &echoAreaRec->export[low],
                    (59-low) * sizeof(nodeNumXType));
            echoAreaRec->export[low].nodeNum = tempNode;
            count++;
         }
         if ( !checkDupArea(echoAreaRec, &echoCount, &skiparname) )
            return 0;
         if ( skiparname )
            continue;
         if ( !checkDupBoard(echoNumCheck, echoAreaRec, &echoCount, &clearboard) )
            return 0;
      }
   }
   close(fileHandle);
   updateAreas(echoCount);
   sprintf(tempStr, "%u areas processed", echoCount);
   displayMessage(tempStr);
   return 0;
}



s16 importNodeFileFD(void)
{
   fhandle        fileHandle;
   fdNodeFileType fdNodeFileRec;
   char           tempStr[256];
   u16            low, mid, high;
   u16		  nodeCount;
   nodeInfoType   *nodeInfoPtr;

   if ( !askFMNodExist(&nodeCount) )
      return 0;
   if ( (fileHandle = open(getSourceFileName(" Source file "),
			   O_BINARY|O_RDWR)) == -1 )
   {  displayMessage("Can't find the requested file.");
      releaseNodes(nodeCount);
      return 0;
   }
   lseek(fileHandle, 4, SEEK_SET);
   working();
   while ( _read(fileHandle, &fdNodeFileRec, sizeof(fdNodeFileType)) ==
					     sizeof(fdNodeFileType) )
   {  if ( !fdNodeFileRec.deleted )
      {  if ( nodeCount == MAX_NODES )
	 {  displayMessage("Too many nodes listed in NodeFile.FD");
ret0:       releaseNodes(nodeCount);
	    return 0;
	 }
	 if ( (nodeInfoPtr = malloc(sizeof(nodeInfoType))) == NULL )
	    goto ret0;
	 memset(nodeInfoPtr, 0, sizeof(nodeInfoType));
	 nodeInfoPtr->node   = fdNodeFileRec.nodeNum;
	 node4d(&nodeInfoPtr->node);
	 nodeInfoPtr->groups = fdNodeFileRec.groups;
	 nodeInfoPtr->capability = PKT_TYPE_2PLUS;
	 nodeInfoPtr->options.active = 1;
	 if ( fdNodeFileRec.tsNodeFlags.crash )
	    nodeInfoPtr->outStatus = 2;
	 if ( fdNodeFileRec.tsNodeFlags.hold )
	    nodeInfoPtr->outStatus = 1;
	 strncpy(nodeInfoPtr->password, fdNodeFileRec.password, 16);
	 low = 0;
	 high = nodeCount;
	 while ( low < high )
	 {  mid = (low + high) >> 1;
	    if ( greater(&nodeInfoPtr->node, &(nodeInfo[mid]->node)) )
	       low = mid+1;
	    else
	       high = mid;
	 }
	 memmove(&nodeInfo[low+1], &nodeInfo[low],
		 (nodeCount++-low) * 4); //sizeof(nodeInfoType));
	 nodeInfo[low] = nodeInfoPtr;
      }
   }
   close(fileHandle);
   updateNodes(nodeCount);
   sprintf(tempStr, "%u nodes processed", nodeCount);
   displayMessage(tempStr);
   return 0;
}



s16 importGechoAr(void)
{
   fhandle     fileHandle;
   char        tempStr[256];
   char        echoNumCheck[MBBOARDS];
   u16         count, 
	       low, mid, high;
   u16         echoCount;
   u16         skiparname = 0, clearboard = 0;
   u32	       index;
   size_t      recsize;
   rawEchoType *echoAreaRec;
   nodeNumType tempNode;
   AREAFILE_HDR gechoArHdrRec;
   AREAFILE_GE  gechoArRec;
   CONNECTION   gechoArExportRec[MAXCONNECTIONS];

   if ( !askFMArExist(&echoCount) )
      return 0;
   memset(echoNumCheck, 0, MBBOARDS);
   if ( (fileHandle = open(getSourceFileName(" Source file "),
			   O_BINARY|O_RDWR)) == -1 )
   {  displayMessage("Can't find the requested file.");
      releaseAreas(echoCount);
      return 0;
   }
   working();
   if ( read(fileHandle, &gechoArHdrRec, sizeof(AREAFILE_HDR)) !=
					 sizeof(AREAFILE_HDR) )
   {   close(fileHandle);
       releaseAreas(echoCount);
       return 0;
   }
   if ( gechoArHdrRec.hdrsize < sizeof(AREAFILE_HDR) ||
	gechoArHdrRec.recsize < sizeof(AREAFILE_GE)-2 )
   {  displayMessage("Wrong record size");
      close(fileHandle);
      releaseAreas(echoCount);
      return 0;
   }
   recsize = min(gechoArHdrRec.recsize, sizeof(AREAFILE_GE));
   index = 0;
   while ( lseek(fileHandle,(u32)gechoArHdrRec.hdrsize+index*((u32)gechoArHdrRec.recsize+(u32)gechoArHdrRec.maxconnections*(u32)sizeof(CONNECTION)), SEEK_SET) !=(u32)-1L &&
	   read(fileHandle, &gechoArRec, recsize) == recsize &&
	   lseek(fileHandle, (u32)gechoArHdrRec.hdrsize+(u32)gechoArHdrRec.recsize+index++*((u32)gechoArHdrRec.recsize+(u32)gechoArHdrRec.maxconnections*(u32)sizeof(CONNECTION)), SEEK_SET) != (u32)-1L &&
	   read(fileHandle, &gechoArExportRec, gechoArHdrRec.maxconnections * sizeof(CONNECTION)) ==
					       gechoArHdrRec.maxconnections * sizeof(CONNECTION) )
   {  if ( !(gechoArRec.options & REMOVED) )
      {  if ( echoCount == MAX_AREAS )
	 {  displayMessage("Too many areas listed in GEcho Area file");
	    close(fileHandle);
	    releaseAreas(echoCount);
	    return 0;
	 }
	 if ( (echoAreaRec = malloc(RAWECHO_SIZE)) == NULL )
	 {  displayMessage("Out of memory");
	    close(fileHandle);
	    releaseAreas(echoCount);
	    return 0;
	 }
	 memset(echoAreaRec, 0, RAWECHO_SIZE);
	 strncpy(echoAreaRec->areaName, gechoArRec.name, ECHONAME_LEN-1);
	 if (*gechoArRec.comment)
	    strncpy(echoAreaRec->comment, gechoArRec.comment, 50);
	 else
	    strncpy(echoAreaRec->comment, gechoArRec.name, COMMENT_LEN-1);
	 if ( gechoArRec.areaformat == FORMAT_HMB ||
	      gechoArHdrRec.recsize == sizeof(AREAFILE_GE)-2 )
	    echoAreaRec->board = gechoArRec.areanumber;
	 else if ( gechoArRec.areaformat == FORMAT_JAM )
	    strncpy(echoAreaRec->msgBasePath, gechoArRec.path, MB_PATH_LEN-1);
	 echoAreaRec->options.allowAreafix = 1;
	 echoAreaRec->options.security   = (gechoArRec.options & SECURITY)?1:0;
	 echoAreaRec->options.tinySeenBy = (gechoArRec.options & TINYSB)?1:0;
	 echoAreaRec->group = (gechoArRec.group < 'A' ||
			  gechoArRec.group > 'Z') ?
			  1 : (1L << (gechoArRec.group - 'A'));
	 echoAreaRec->msgs = gechoArRec.maxmsgs;
	 echoAreaRec->days = gechoArRec.maxdays;
	 if ((echoAreaRec->address = gechoArRec.pkt_origin) >= MAX_AKAS)
	    echoAreaRec->address = 0;
	 count = 0;
	 while ( count < 60  &&  gechoArExportRec[count].address.zone != 0 )
	 {  tempNode = *(nodeNumType*)&gechoArExportRec[count].address;
	    node4d(&tempNode);
	    low = 0;
	    high = count;
	    while ( low < high )
	    {  mid = (low + high) >> 1;
	       if (greater(&tempNode, &echoAreaRec->export[mid].nodeNum))
		  low = mid+1;
	       else
		  high = mid;
	    }
	    memmove(&echoAreaRec->export[low+1], &echoAreaRec->export[low],
		    (59-low) * sizeof(nodeNumXType));
	    echoAreaRec->export[low].nodeNum = tempNode;
	    count++;
	 }
	 if ( !checkDupArea(echoAreaRec, &echoCount, &skiparname) )
	    return 0;
	 if ( skiparname )
	    continue;
	 if ( !checkDupBoard(echoNumCheck, echoAreaRec, &echoCount, &clearboard) )
	     return 0;
      }
   }
   close(fileHandle);
   updateAreas(echoCount);
   sprintf(tempStr, "%u areas processed", echoCount);
   displayMessage(tempStr);
  
   return 0;
}



s16 importGechoNd(void)
{
   fhandle      fileHandle;
   char         tempStr[256];
   u16		nodeCount;
   u16          low, mid, high;
   u32          index;
   nodeInfoType *nodeInfoPtr;
   NODEFILE_HDR gechoNdHdrRec;
   NODEFILE_GE  gechoNdRec;

   if ( !askFMNodExist(&nodeCount) )
      return 0;
   strcpy(tempStr, configPath);
   strcat(tempStr, "FMAIL.NOD");
   if ( (fileHandle = open(getSourceFileName(" Source file "),
			   O_BINARY|O_RDWR)) == -1 )
   {  displayMessage("Can't find the requested file.");
      releaseNodes(nodeCount);
      return 0;
   }
   working();
   if ( read(fileHandle, &gechoNdHdrRec, sizeof(NODEFILE_HDR)) !=
					    sizeof(NODEFILE_HDR) )
   {  close(fileHandle);
      releaseNodes(nodeCount);
      return 0;
   }
   if ( gechoNdHdrRec.hdrsize < sizeof(NODEFILE_HDR) ||
	gechoNdHdrRec.recsize < sizeof(NODEFILE_GE) )
   {  displayMessage("Wrong record size");
      close(fileHandle);
      releaseNodes(nodeCount);
      return 0;
   }
   index = 0;
   while ( lseek(fileHandle, gechoNdHdrRec.hdrsize+index++*gechoNdHdrRec.recsize, SEEK_SET) != -1L &&
	   read(fileHandle, &gechoNdRec, sizeof(NODEFILE_GE)) == sizeof(NODEFILE_GE) )
   {  
//    if ( gechoNdRec.status == 0xFFFF )
//       continue;
      if (nodeCount == MAX_NODES)
      {  displayMessage("Too many nodes listed in GEcho nodes file");
ret0:    releaseNodes(nodeCount);
	 return 0;
      }
      if ( (nodeInfoPtr = malloc(sizeof(nodeInfoType))) == NULL )
	 goto ret0;
      memset(nodeInfoPtr, 0, sizeof(nodeInfoType));
      strncpy(nodeInfoPtr->sysopName, gechoNdRec.sysop,  35);
      strncpy(nodeInfoPtr->password,  gechoNdRec.mgrpwd, 16);
      strncpy(nodeInfoPtr->packetPwd, gechoNdRec.pktpwd,  8);
      nodeInfoPtr->node = *(nodeNumType*)&gechoNdRec.address;
      node4d(&nodeInfoPtr->node);
      if ( !(gechoNdRec.options & CHKPKTPWD ) )
	 nodeInfoPtr->options.ignorePwd = 1;
      if ( gechoNdRec.options & PACKNETMAIL )
	 nodeInfoPtr->options.packNetmail = 1;
      if ( gechoNdRec.options & FORWARDREQ )
	 nodeInfoPtr->options.forwardReq = 1;
      if ( gechoNdRec.options & REMOTEMAINT )
	 nodeInfoPtr->options.remMaint = 1;
      if ( gechoNdRec.options & ALLOWRESCAN )
	 nodeInfoPtr->options.allowRescan = 1;
      if ( !(gechoNdRec.options & NONOTIFY ) )
	 nodeInfoPtr->options.notify = 1;
      if ( gechoNdRec.mailstatus == 0x0002 ) /* crash */
      nodeInfoPtr->outStatus  = 1;
      if ( gechoNdRec.mailstatus == 0x0200 ) /* hold */
      nodeInfoPtr->outStatus  = 2;
      nodeInfoPtr->options.active = 1;
      low = 0;
      high = nodeCount;
      while ( low < high )
      {  mid = (low + high) >> 1;
	 if ( greater(&nodeInfoPtr->node, &(nodeInfo[mid]->node)) )
	    low = mid + 1;
	 else
	    high = mid;
      }
      memmove(&nodeInfo[low+1], &nodeInfo[low],
	      (nodeCount++-low) * 4); //sizeof(nodeInfoType *));
      nodeInfo[low] = nodeInfoPtr;
   }
   close(fileHandle);
   updateNodes(nodeCount);
   sprintf(tempStr, "%u nodes processed", nodeCount);
   displayMessage(tempStr);
   return 0;
}



s16 importFEAr(void)
{
   fhandle     fileHandle;
   char        tempStr[256];
   char        echoNumCheck[MBBOARDS];
   u8          nodeBits[FE_MAX_AREAS/8];
   u16         echoCount;
   u16         count,
	       xu, xu2;
   u16         skiparname = 0, clearboard = 0;
   rawEchoType *echoAreaRec;
   CONFIG      configFE;
   Area        areaFE;
   Node        nodeFE;

   if ( !askFMArExist(&echoCount) )
      return 0;
   memset(echoNumCheck, 0, MBBOARDS);
   for ( count = 0; count < echoCount; ++count )
      if ( echoAreaList[count]->board > 0 && echoAreaList[count]->board <= MBBOARDS )
	 echoNumCheck[echoAreaList[count]->board-1] = 1;
   if ( (fileHandle = open(getSourceFileName(" Source file "),
			   O_BINARY|O_RDWR)) == -1 )
   {  displayMessage("Can't find the requested file.");
      releaseAreas(echoCount);
      return 0;
   }
   working();
   if ( _read(fileHandle, &configFE, sizeof(CONFIG)) != sizeof(CONFIG) )
      goto ret0msg;
   for( xu = 0; xu < configFE.AreaCnt; xu++ )
   {  if( lseek(fileHandle, (u32)sizeof(CONFIG) + (u32)configFE.offset +
			    (u32)configFE.NodeCnt * (u32)configFE.NodeRecSize +
			    (u32)xu * (u32)configFE.AreaRecSize, SEEK_SET) == -1 )
      {  displayMessage("Read error");
	 releaseAreas(echoCount);
	 return 0;
      }
      if ( _read(fileHandle, &areaFE, sizeof(Area)) != sizeof(Area) )
	 goto ret0msg;
      if ( !*areaFE.name || *areaFE.name == ' ' )
	 continue;
      if ( areaFE.board == AREA_DELETED )
	 continue;
      if ( areaFE.flags.atype == AREA_ECHOMAIL ||
	   areaFE.flags.atype == AREA_LOCAL )
      {  if (echoCount == MAX_AREAS)
	 {  displayMessage("Too many areas listed in the source file");
	    releaseAreas(echoCount);
	    return 0;
	 }
	 if ( (echoAreaRec = malloc(RAWECHO_SIZE)) == NULL )
	 {  displayMessage("Out of memory");
	    releaseAreas(echoCount);
	    return 0;
	 }
	 memset(echoAreaRec, 0, RAWECHO_SIZE);
	 echoAreaRec->_alsoSeenBy = areaFE.conference;
	 strncpy(echoAreaRec->areaName, areaFE.name, sizeof(areaFE.name)-1);
	 if ( *areaFE.desc )
	    strncpy(echoAreaRec->comment, areaFE.desc, 50);
	 else
	    strncpy(echoAreaRec->comment, areaFE.name, COMMENT_LEN-1);
	 if ( areaFE.flags.storage == FE_QBBS )
	    echoAreaRec->board = areaFE.board;
	 if ( areaFE.flags.storage == FE_JAM )
	    strncpy(echoAreaRec->msgBasePath, areaFE.path, MB_PATH_LEN-1);
	 echoAreaRec->readSecRA = areaFE.read_sec;
	 echoAreaRec->writeSecRA = echoAreaRec->writeLevel = areaFE.write_sec;
	 echoAreaRec->address = areaFE.info.aka;
	 echoAreaRec->group = (areaFE.info.group > 26) ? 1 : (1L << areaFE.info.group);
	 if ( areaFE.flags.atype == AREA_LOCAL )
	    echoAreaRec->options.local = 1;
	 echoAreaRec->options.tinySeenBy = areaFE.advflags.tinyseen;
	 echoAreaRec->options.active = !areaFE.advflags.passive;
	 echoAreaRec->options.impSeenBy = areaFE.advflags.keepseen;
	 echoAreaRec->options.allowAreafix = !areaFE.advflags.mandatory;
	 echoAreaRec->options.sysopRead = areaFE.advflags.keepsysop;
	 echoAreaRec->alsoSeenBy = areaFE.seenbys;
	 echoAreaRec->days = areaFE.days;
	 echoAreaRec->msgs = areaFE.messages;
	 echoAreaRec->daysRcvd = areaFE.recvdays;
	 if ( !checkDupArea(echoAreaRec, &echoCount, &skiparname) )
	    return 0;
	 if ( skiparname )
	    continue;
	 if ( !checkDupBoard(echoNumCheck, echoAreaRec, &echoCount, &clearboard) )
	    return 0;
      }
   }
   for ( xu = 0; xu < configFE.NodeCnt; ++xu )
   {  if ( lseek(fileHandle, (u32)sizeof(CONFIG) + (u32)configFE.offset +
			     (u32)xu * (u32)configFE.NodeRecSize, SEEK_SET) == -1 )
	 goto ret0msg;
      if ( _read(fileHandle, &nodeFE, sizeof(Node)-1) != sizeof(Node)-1 )
	 goto ret0msg;
      if ( _read(fileHandle, &nodeBits, (configFE.MaxAreas+7)/8) != (configFE.MaxAreas+7)/8 )
ret0msg: 
      {  displayMessage("Read error");
	 releaseAreas(echoCount);
	 return 0;
      }
      for ( count = 0; count < echoCount; count++ )
      {  if ( nodeBits[echoAreaList[count]->_alsoSeenBy/8] &
	      (1 << (7 - (echoAreaList[count]->_alsoSeenBy % 8))) )
	 {  xu2 = 0;
	    while ( xu2 < config.maxForward && echoAreaList[count]->export[xu2].nodeNum.zone )
	       ++xu2;
	    if ( xu2 < config.maxForward )
	       echoAreaList[count]->export[xu2].nodeNum = *(nodeNumType *)&nodeFE.addr;
	 }
      }
   }
   close(fileHandle);
   updateAreas(echoCount);
   sprintf(tempStr, "%u areas processed", echoCount);
   displayMessage(tempStr);
   return 0;
}



s16 importFENd(void)
{
   fhandle      fileHandle;
   char         tempStr[256];
   u16          low, mid, high, xu;
   u16		nodeCount;
   nodeInfoType *nodeInfoPtr;
   CONFIG       configFE;
   Node         nodeFE;

   if ( !askFMNodExist(&nodeCount) )
      return 0;
   if ( (fileHandle = open(getSourceFileName(" Source file "),
			   O_BINARY|O_RDWR)) == -1 )
   {  displayMessage("Can't find the requested file.");
      releaseNodes(nodeCount);
      return 0;
   }
   working();
   if ( _read(fileHandle, &configFE, sizeof(CONFIG)) != sizeof(CONFIG) )
      goto ret0msg;
   for ( xu = 0; xu < configFE.NodeCnt; xu++ )
   {  if ( lseek(fileHandle, (u32)sizeof(CONFIG) +
			     (u32)configFE.offset +
			     (u32)xu * (u32)configFE.NodeRecSize, SEEK_SET) == -1 )
ret0msg: 
      {  displayMessage("Read error");
	 goto ret0;
      }
      if ( _read (fileHandle, &nodeFE, sizeof(Node)) != sizeof(Node) )
	 goto ret0msg;
      if ( nodeCount == MAX_NODES )
      {  displayMessage("Too many nodes listed");
ret0:    releaseNodes(nodeCount);
	 return 0;
      }
      if ( (nodeInfoPtr = malloc(sizeof(nodeInfoType))) == NULL )
	 goto ret0;
      memset(nodeInfoPtr, 0, sizeof(nodeInfoType));
      nodeInfoPtr->node = *(nodeNumType *)&nodeFE.addr;
      node4d(&nodeInfoPtr->node);
      nodeInfoPtr->useAka = nodeFE.aka;
      nodeInfoPtr->options.active = !nodeFE.flags.passive;
      nodeInfoPtr->writeLevel = nodeFE.sec_level;
      nodeInfoPtr->groups     = ~nodeFE.groups;
      swapBits((u8 *)&nodeInfoPtr->groups);
      swapBits(((u8 *)&nodeInfoPtr->groups) + 1);
      swapBits(((u8 *)&nodeInfoPtr->groups) + 2);
      swapBits(((u8 *)&nodeInfoPtr->groups) + 3);
      nodeInfoPtr->groups &= 0x03FFFFFFL;
      strncpy(nodeInfoPtr->sysopName, nodeFE.name, 35);
      strncpy(nodeInfoPtr->password,  nodeFE.areafixpw, 8);
      strncpy(nodeInfoPtr->packetPwd, nodeFE.password, 8);
      low = 0;
      high = nodeCount;
      while ( low < high )
      {  mid = (low + high) >> 1;
	 if ( greater(&nodeInfoPtr->node, &(nodeInfo[mid]->node)) )
	    low = mid+1;
	 else
	    high = mid;
      }
      memmove(&nodeInfo[low+1], &nodeInfo[low],
	      (nodeCount++-low) * 4); //sizeof(nodeInfoType));
      nodeInfo[low] = nodeInfoPtr;
   }
   close(fileHandle);
   updateNodes(nodeCount);
   sprintf(tempStr, "%u nodes processed", nodeCount);
   displayMessage(tempStr);
   return 0;
}




s16 importImailAr(void)
{
   fhandle     fileHandle;
   imailArType imailArRec;
   char        tempStr[256];
   char        echoNumCheck[MBBOARDS];
   u16         echoCount;
   u16         count,
	       low, mid, high;
   u16         skiparname = 0, clearboard = 0;
   rawEchoType *echoAreaRec;
   nodeNumType tempNode;

   if ( !askFMArExist(&echoCount) )
      return 0;
   memset(echoNumCheck, 0, MBBOARDS);
   if ( (fileHandle = open(getSourceFileName(" Source file "),
			   O_BINARY|O_RDWR)) == -1 )
   {  displayMessage("Can't find the requested file.");
      releaseAreas(echoCount);
      return 0;
   }
   working();
   while ( _read(fileHandle, &imailArRec, sizeof(imailArType)) ==
					  sizeof(imailArType))
   {  if ( imailArRec.imArFlags.deleted == 0 &&
	  ((imailArRec.msgBaseType & 0x0f) == 0x03 ||
	   (imailArRec.msgBaseType & 0x0f) == 0x0f) &&
	  (imailArRec.msgBaseType & 0xf0) != 0xa0 )
      {  if ( echoCount == MAX_AREAS ) 
	 {  displayMessage("Too many areas listed in IMAIL.AR");
	    releaseAreas(echoCount);
	    return 0;
	 }
	 if ( (echoAreaRec = malloc(RAWECHO_SIZE)) == NULL )
	 {  displayMessage("Out of memory");
	    releaseAreas(echoCount);
	    return 0;
	 }
	 memset(echoAreaRec, 0, RAWECHO_SIZE);
	 strncpy(echoAreaRec->areaName, imailArRec.areaName, ECHONAME_LEN-1);
	 if(*imailArRec.comment)
	    strncpy(echoAreaRec->comment, imailArRec.comment, 50);
	 else
	    strncpy(echoAreaRec->comment, imailArRec.areaName, COMMENT_LEN-1);
	 if ((imailArRec.msgBaseType & 0x0f) == 0x03)
	    echoAreaRec->board = imailArRec.boardNum;
	 echoAreaRec->options.active = imailArRec.imArFlags.active;
	 echoAreaRec->options.allowAreafix = 1;
	 echoAreaRec->options.security   = imailArRec.imArFlags.security;
	 echoAreaRec->options.tinySeenBy = imailArRec.imArFlags.tinySeenBy;
	 if ( (imailArRec.msgBaseType & 0x0f) == 0x90 )
	    echoAreaRec->options.local = 1;
	 echoAreaRec->group = (imailArRec.group < 'A' ||
			       imailArRec.group > 'Z') ?
			       1 : (1L << (imailArRec.group - 'A'));
	 echoAreaRec->msgs = imailArRec.numMsgs;
	 echoAreaRec->days = imailArRec.numDays;
	 if ( (echoAreaRec->address = imailArRec.originAka-1) >= MAX_AKAS )
	    echoAreaRec->address = 0;
	 count = 0;
	 while ( count < 60 && imailArRec.export[count].expNode.zone != 0 )
	 {  tempNode = imailArRec.export[count].expNode;
	    node4d(&tempNode);
	    low = 0;
	    high = count;
	    while ( low < high )
	    {  mid = (low + high) >> 1;
	       if (greater (&tempNode, &echoAreaRec->export[mid].nodeNum))
		  low = mid+1;
	       else
		  high = mid;
	    }
	    memmove(&echoAreaRec->export[low+1], &echoAreaRec->export[low],
		    (59-low) * sizeof(nodeNumXType));
	    echoAreaRec->export[low].nodeNum = tempNode;
	    count++;
	 }
	 if ( !checkDupArea(echoAreaRec, &echoCount, &skiparname) )
	    return 0;
	 if ( skiparname )
	    continue;
	 if ( !checkDupBoard(echoNumCheck, echoAreaRec, &echoCount, &clearboard) )
	    return 0;
      }
   }
   close(fileHandle);
   updateAreas(echoCount);
   sprintf(tempStr, "%u areas processed", echoCount);
   displayMessage(tempStr);
   return 0;
}



s16 importImailNd(void)
{
   fhandle      fileHandle;
   imailNdType  imailNdRec;
   char         tempStr[256];
   u16          low, mid, high;
   u16		nodeCount;
   nodeInfoType *nodeInfoPtr;

   if ( !askFMNodExist(&nodeCount) )
      return 0;
   if ((fileHandle = open(getSourceFileName(" Source file "),
			  O_BINARY|O_RDWR)) == -1)
   {  displayMessage("Can't find the requested file.");
      releaseNodes(nodeCount);
      return 0;
   }
   working();
   while( _read(fileHandle, &imailNdRec, sizeof(imailNdType)) ==
					 sizeof(imailNdType) )
   {  if (nodeCount == MAX_NODES)
      {  displayMessage("Too many nodes listed in IMAIL.Nd");
ret0:    releaseNodes(nodeCount);
	 return 0;
      }
      if ( (nodeInfoPtr = malloc(sizeof(nodeInfoType))) == NULL )
	 goto ret0;
      memset(nodeInfoPtr, 0, sizeof(nodeInfoType));
      nodeInfoPtr->node       = imailNdRec.nodeNum;
      node4d(&nodeInfoPtr->node);
      nodeInfoPtr->groups     = getGroupCode(imailNdRec.groups);
      nodeInfoPtr->outStatus  = IMSTAT[imailNdRec.attachStatus];
      nodeInfoPtr->capability = imailNdRec.capability;
      nodeInfoPtr->options.active      = 1;
      nodeInfoPtr->options.remMaint    = imailNdRec.imNdFlags.remMaint;
      nodeInfoPtr->options.allowRescan = imailNdRec.imNdFlags.allowRescan;
      nodeInfoPtr->options.notify      = imailNdRec.imNdFlags.notify;
      nodeInfoPtr->options.forwardReq  = imailNdRec.imNdFlags.forwardReq;
      strncpy(nodeInfoPtr->sysopName, imailNdRec.sysopName, 35);
      strncpy(nodeInfoPtr->password,  imailNdRec.password,  16);
      strncpy(nodeInfoPtr->packetPwd, imailNdRec.packetPwd,  8);
      low = 0;
      high = nodeCount;
      while ( low < high )
      {  mid = (low + high) >> 1;
	 if (greater(&nodeInfoPtr->node, &(nodeInfo[mid]->node)))
	    low = mid+1;
	 else
	    high = mid;
      }
      memmove(&nodeInfo[low+1], &nodeInfo[low],
	      (nodeCount++-low) * 4); //sizeof(nodeInfoType));
      nodeInfo[low] = nodeInfoPtr;
   }
   close(fileHandle);
   updateNodes(nodeCount);
   sprintf(tempStr, "%u nodes processed", nodeCount);
   displayMessage(tempStr);
   return 0;
}



s16 importRAInfo(void)
{
   fhandle        RAHandle;
   u16            count = 0,
                  echoCount = 0,
                  raCount, setBoardCode;
   s16            notFound;
   tempStrType    tempStr;
   messageRaType  messageRaRec;
   messageRa2Type messageRa2Rec;
   headerType	  *areaHeader;
   rawEchoType    *areaBuf;

   if ( config.bbsProgram != BBS_RA1X && config.bbsProgram != BBS_RA20 &&
        config.bbsProgram != BBS_RA25 && config.bbsProgram != BBS_ELEB )
      displayMessage("This function only supports RemoteAccess");
   if ( (RAHandle = open(getSourceFileName(" MESSAGES.RA file "), O_RDONLY|O_BINARY|O_DENYNONE)) == -1 )
   {  displayMessage("Can't find the requested file.");
      return 0;
   }
   if ( !openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf) )
   {  close(RAHandle);
      displayMessage("Can't open file FMAIL.AR");
      return 0;
   }
   if ( config.bbsProgram == BBS_RA1X )
   {  working();
      while ( getRec(CFG_ECHOAREAS, count) )
      {  if ( areaBuf->board >= 1  &&  areaBuf->board <= MBBOARDS )
	 {  lseek(RAHandle, (areaBuf->board-1)*(s32)sizeof(messageRaType), SEEK_SET);
	    read(RAHandle, &messageRaRec, sizeof(messageRaType));
	    if ( *areaBuf->comment == 0 )
	    {  memcpy(areaBuf->comment, messageRaRec.name, 40);
	       areaBuf->comment[messageRaRec.nameLength] = 0;
	    }
            if ( *areaBuf->originLine == 0 )
	    {  memcpy(areaBuf->originLine, messageRaRec.origin, ORGLINE_LEN-1);
	       areaBuf->originLine[min(messageRaRec.originLength, ORGLINE_LEN-1)] = 0;
	    }
	    areaBuf->msgKindsRA    = messageRaRec.msgKinds;
	    areaBuf->days          = messageRaRec.daysKill;
	    areaBuf->daysRcvd      = messageRaRec.rcvdKill;
	    areaBuf->msgs          = messageRaRec.countKill;
	    areaBuf->attrRA        = messageRaRec.attribute;
	    areaBuf->readSecRA     = messageRaRec.readSecurity;
	    areaBuf->writeSecRA    = messageRaRec.writeSecurity;
	    areaBuf->sysopSecRA    = messageRaRec.sysopSecurity;
	    *((s32*)&areaBuf->flagsRdRA)  = *((s32*)&messageRaRec.readFlags);
	    *((s32*)&areaBuf->flagsWrRA)  = *((s32*)&messageRaRec.writeFlags);
	    *((s32*)&areaBuf->flagsSysRA) = *((s32*)&messageRaRec.sysopFlags);
	    putRec(CFG_ECHOAREAS, count);
	    echoCount++;
	 }
	 count++;
      }
   }
   else if ( config.bbsProgram == BBS_RA20 || config.bbsProgram == BBS_RA25 ||
                                              config.bbsProgram == BBS_ELEB )
   {  setBoardCode = (askBoolean("Also set Board Code?", 'Y') == 'Y');
      working();
      while ( getRec(CFG_ECHOAREAS, count) )
      {  notFound = 1;
	 lseek(RAHandle, 0, SEEK_SET);
         raCount = 0;
	 while ( notFound && sizeof(messageRa2Type) ==
		 read(RAHandle, &messageRa2Rec, sizeof(messageRa2Type)) )
         {  ++raCount;
            if ( (areaBuf->board >= 1 && areaBuf->board <= MBBOARDS &&
                  ((config.bbsProgram == BBS_RA20 &&
		    areaBuf->board == tell(RAHandle)/sizeof(messageRa2Rec)) ||
                   ((config.bbsProgram == BBS_RA25 || config.bbsProgram == BBS_ELEB) &&
		    areaBuf->board == messageRa2Rec.areanum))) ||
                 (*areaBuf->msgBasePath &&
//#pragma message ("NIEUW ipv ^^^!!!")
// NIEUW:        (areaBuf->board == BOARDTYPEJAM &&
		  (memicmp(areaBuf->msgBasePath, messageRa2Rec.JAMbase,
						 strlen(areaBuf->msgBasePath)) == 0)) )
	    {  notFound = 0;
	       if ( *areaBuf->comment == 0 )
	       {  memcpy(areaBuf->comment, messageRa2Rec.name, 40);
		  areaBuf->comment[messageRa2Rec.nameLength] = 0;
	       }
	       if ( *areaBuf->originLine == 0 )
	       {  memcpy(areaBuf->originLine, messageRa2Rec.origin, ORGLINE_LEN-1);
		  areaBuf->originLine[min(messageRa2Rec.originLength, ORGLINE_LEN-1)] = 0;
	       }
	       areaBuf->netReplyBoardRA = messageRa2Rec.netReply;
	       areaBuf->msgKindsRA    = messageRa2Rec.msgKinds;
	       areaBuf->days          = messageRa2Rec.daysKill;
	       areaBuf->daysRcvd      = messageRa2Rec.rcvdKill;
	       areaBuf->msgs          = messageRa2Rec.countKill;
	       areaBuf->attrRA        = messageRa2Rec.attribute;
	       areaBuf->minAgeSBBS    = messageRa2Rec.age;
	       areaBuf->groupRA       = messageRa2Rec.group;
	       areaBuf->altGroupRA[0] = messageRa2Rec.altGroup[0];
	       areaBuf->altGroupRA[1] = messageRa2Rec.altGroup[1];
	       areaBuf->altGroupRA[2] = messageRa2Rec.altGroup[2];
	       areaBuf->attr2RA       = messageRa2Rec.attribute2;
	       areaBuf->readSecRA     = messageRa2Rec.readSecurity;
	       areaBuf->writeSecRA    = messageRa2Rec.writeSecurity;
	       areaBuf->sysopSecRA    = messageRa2Rec.sysopSecurity;
	       *((s32*)&areaBuf->flagsRdRA)  = *((s32*)&messageRa2Rec.readFlags);
	       *((s32*)&areaBuf->flagsRdNotRA) = *((s32*)&messageRa2Rec.readNotFlags);
	       *((s32*)&areaBuf->flagsWrRA)  = *((s32*)&messageRa2Rec.writeFlags);
	       *((s32*)&areaBuf->flagsWrNotRA) = *((s32*)&messageRa2Rec.writeNotFlags);
	       *((s32*)&areaBuf->flagsSysRA) = *((s32*)&messageRa2Rec.sysopFlags);
	       *((s32*)&areaBuf->flagsSysNotRA) = *((s32*)&messageRa2Rec.sysopNotFlags);
               if ( setBoardCode )
               {  if ( *areaBuf->msgBasePath )
//#pragma message ("NIEUW ipv ^^^!!!")
//             {  if ( areaBuf->board == BOARDTYPEJAM )
                     areaBuf->boardNumRA = raCount;
                  else
                     areaBuf->boardNumRA = 0;
               }
               putRec(CFG_ECHOAREAS, count);
	       echoCount++;
	    }
	 }
	 count++;
      }
   }
   closeConfig(CFG_ECHOAREAS);
   close(RAHandle);
   sprintf(tempStr, "%u areas processed", echoCount);
   displayMessage(tempStr);
   return 0;
}



char *findCLiStr(char *s1, char *s2)
{
   char *helpPtr;

   if ( strnicmp(s1, s2, strlen(s2)) == 0 )
      return s1;
   helpPtr = s1;
   while ( (helpPtr = stristr(helpPtr+1, s2)) != NULL )
   {  if ( *(helpPtr-1) == '\r' || *(helpPtr-1) == '\n' )
	 return helpPtr;
   }
   return NULL;
}



s16 importNAInfo(void)
{
   fhandle        NAHandle;
   char           *buf;
   char           *helpPtr, *helpPtr2;
   u16            bufsize;
   u16            count = 0, updated = 0, xu;
   tempStrType    tempStr;
   headerType	  *areaHeader;
   rawEchoType    *areaBuf;

   if ( (NAHandle = open(getSourceFileName(" BACKBONE.NA file "), O_RDONLY|O_BINARY|O_DENYNONE)) == -1 )
   {  displayMessage("Can't find the requested file.");
      return 0;
   }
   if ( (buf = malloc(bufsize = min((u16)filelength(NAHandle)+1, 0xFFF0))) == NULL )
   {  displayMessage("Not enough memory");
      return 0;
   }
   if ( read(NAHandle, buf, bufsize-1) != bufsize-1 )
   {  free(buf);
      return 0;
   }
   close(NAHandle);
   buf[bufsize] = 0;
   if ( !openConfig(CFG_ECHOAREAS, &areaHeader, (void*)&areaBuf) )
   {  close(NAHandle);
      free(buf);
      displayMessage("Can't open file FMAIL.AR");
      return 0;
   }
   working();
   while ( getRec(CFG_ECHOAREAS, count++) )
   {  if ( (helpPtr = findCLiStr(buf, areaBuf->areaName)) == NULL )
	 continue;
      while ( *helpPtr != ' ' )
	 ++helpPtr;
      while ( *helpPtr == ' ' )
	 ++helpPtr;
      helpPtr2 = areaBuf->comment;
      xu = 0;
      while ( ++xu < ECHONAME_LEN && *helpPtr && *helpPtr != '\r' && *helpPtr != '\n' )
	 *helpPtr2++ = *helpPtr++;
      do
	 *helpPtr2-- = 0;
      while ( helpPtr2 >= areaBuf->comment && *helpPtr2 == ' ' );
      putRec(CFG_ECHOAREAS, count-1);
      updated++;
   }
   closeConfig(CFG_ECHOAREAS);
   free(buf);
   sprintf(tempStr, "%u descriptions imported", updated);
   displayMessage(tempStr);
   return 0;
}

