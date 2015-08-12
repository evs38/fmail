//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2014  Wilfred van Velzen
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

#include <fcntl.h>
#include <io.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "cfgfile.h"

#include "internal.h" // not necessary for 3rd party programs

// set to non-zero value if automatic conversion is desired
u16  allowConversion = 0;
extern configType config;


typedef struct
{
   const char    *fileName;
   u16            recordSize;
   const u16      bufferSize;
   fhandle        handle;
   char          *recBuf;
   const char    *revString;
   u16            revNumber;
   const u16      dataType;
   const u16      init;
   u16            status;
   headerType     header;

} configFileInfoType;


static configFileInfoType fileData[MAX_CFG_FILES] =
{
    /* CFG_GENERAL */
    {       dCFGFNAME, 0, sizeof(configType), -1, NULL,
            "FMail Configuration File rev. 1.1\x1a",
            0x0110, DATATYPE_CF, 'CF', 0 },
    /* CFG_NODES */
    {       dNODFNAME, 0, sizeof(nodeInfoType), -1, NULL,
            "FMail Node File rev. 1.1\x1a",
            0x0110, DATATYPE_NO, 'NO', 0 },
    /* CFG_ECHOAREAS */
    {       dARFNAME, 0, RAWECHO_SIZE, -1, NULL,
            "FMail Area File rev. 1.1\x1a",
            0x0110, DATATYPE_AE, 'AE', 0 },
    /* CFG_AREADEF */
    {       dARDFNAME, 0, RAWECHO_SIZE, -1, NULL,
            "FMail Area File rev. 1.1\x1a",
            0x0110, DATATYPE_AD, 'AD', 0 }
};


static configFileInfoType cfiArr[MAX_CFG_FILES] = { 0 };

extern char configPath[128];  // Path to directory with FMail config files



s16 openConfig(u16 fileType, headerType **header, void **buf)
{
   pathType areaInfoPath, tempPath;
   fhandle  temphandle;
   char    *helpPtr;
   u16      count, count2;
   u16      orgRecSize;
   nodeNumType *nodeNumBuf;
   u16      converted = 0;
   u16      oldAreasFile;

   if (fileType >= MAX_CFG_FILES)
      return 0;

   fileData[fileType].recordSize = fileData[fileType].bufferSize;
   if ( fileType == CFG_ECHOAREAS || fileType == CFG_AREADEF )
      fileData[fileType].recordSize -=
     (MAX_FORWARDDEF - config.maxForward)*sizeof(nodeNumXType);

restart:
   strcpy(areaInfoPath, configPath);
   strcat(areaInfoPath, fileData[fileType].fileName);

   memset(&cfiArr[fileType].header, 0, sizeof(headerType));
   cfiArr[fileType].status = 0;

  if ((cfiArr[fileType].handle = open(areaInfoPath, O_BINARY|O_RDWR|O_CREAT|O_DENYALL, S_IREAD|S_IWRITE)) == -1)
    return 0;

   if (filelength(cfiArr[fileType].handle) == 0)
   {
      strcpy((char*)cfiArr[fileType].header.versionString, fileData[fileType].revString);
      cfiArr[fileType].header.revNumber    = fileData[fileType].revNumber;
      cfiArr[fileType].header.headerSize   = sizeof(headerType);
      cfiArr[fileType].header.recordSize   = fileData[fileType].recordSize;
      cfiArr[fileType].header.dataType     = fileData[fileType].dataType;
      cfiArr[fileType].header.totalRecords = 0;
      cfiArr[fileType].header.lastModified = time(&cfiArr[fileType].header.creationDate);

      write(cfiArr[fileType].handle, &cfiArr[fileType].header, sizeof(headerType));
   }
   else
   {
      read(cfiArr[fileType].handle, &cfiArr[fileType].header, sizeof(headerType));

      if (memcmp(cfiArr[fileType].header.versionString, "FMail", 5) ||
          (cfiArr[fileType].header.headerSize < sizeof(headerType)) ||
          ((!allowConversion || converted) && cfiArr[fileType].header.recordSize < fileData[fileType].recordSize) ||
      (cfiArr[fileType].header.dataType != fileData[fileType].dataType))
      {
error:   close(cfiArr[fileType].handle);
     cfiArr[fileType].handle = -1;
     *header = NULL;
     *buf = NULL;
     return 0;
      }
      if ( cfiArr[fileType].header.recordSize < fileData[fileType].recordSize ||
           ((fileType == CFG_ECHOAREAS || fileType == CFG_AREADEF) &&
           cfiArr[fileType].header.recordSize > fileData[fileType].recordSize) )
      {
         /* convert old record format */
         oldAreasFile = (fileType == CFG_ECHOAREAS || fileType == CFG_AREADEF) &&
                        cfiArr[fileType].header.recordSize < sizeof(rawEchoType)-(MAX_FORWARDDEF-MAX_FORWARDOLD)*sizeof(nodeNumXType);

         strcpy(tempPath, areaInfoPath);
         if ( (helpPtr = strrchr(tempPath, '.')) == NULL )
            goto error;
         strcpy(helpPtr+1, "$$$");
         if ( (cfiArr[fileType].recBuf = (char*)malloc(fileData[fileType].bufferSize)) == NULL )
            goto error;
         if ( (nodeNumBuf = (nodeNumType*)malloc(MAX_FORWARDOLD * sizeof(nodeNumType))) == NULL )
         {  free(cfiArr[fileType].recBuf);
            goto error;
         }
         if ((temphandle = open(tempPath, O_BINARY|O_RDWR|O_CREAT|O_DENYALL, S_IREAD|S_IWRITE)) == -1)
         {  free(cfiArr[fileType].recBuf);
            cfiArr[fileType].recBuf = NULL;
            goto error;
     }
         orgRecSize = cfiArr[fileType].header.recordSize;
         strcpy((char*)cfiArr[fileType].header.versionString, fileData[fileType].revString);
         cfiArr[fileType].header.revNumber    = fileData[fileType].revNumber;
         cfiArr[fileType].header.headerSize   = sizeof(headerType);
         cfiArr[fileType].header.recordSize   = fileData[fileType].recordSize;
         cfiArr[fileType].header.dataType     = fileData[fileType].dataType;
         time(&cfiArr[fileType].header.lastModified);
         write(temphandle, &cfiArr[fileType].header, sizeof(headerType));
         for ( count = 0; count < cfiArr[fileType].header.totalRecords; count++ )
         {  memset(cfiArr[fileType].recBuf, 0, fileData[fileType].recordSize);
            if ( read(cfiArr[fileType].handle, cfiArr[fileType].recBuf, orgRecSize) != orgRecSize )
        {  free(cfiArr[fileType].recBuf);
           cfiArr[fileType].recBuf = NULL;
           close(temphandle);
           unlink(tempPath);
           goto error;
        }
            if ( oldAreasFile )
            {  memcpy(nodeNumBuf, &((rawEchoType*)cfiArr[fileType].recBuf)->readSecRA, MAX_FORWARDOLD * sizeof(nodeNumType));
               memcpy(cfiArr[fileType].recBuf + offsetof(rawEchoType, readSecRA),
                      cfiArr[fileType].recBuf + offsetof(rawEchoType, readSecRA) + MAX_FORWARDOLD * sizeof(nodeNumType),
                      offsetof(rawEchoType, forwards) - offsetof(rawEchoType, readSecRA));
               for ( count2 = 0; count2 < MAX_FORWARDOLD; count2++ )
               {  memset(&((rawEchoType*)cfiArr[fileType].recBuf)->forwards[count2], 0, sizeof(nodeNumXType));
                  ((rawEchoType*)cfiArr[fileType].recBuf)->forwards[count2].nodeNum = nodeNumBuf[count2];
               }
               memset(&((rawEchoType*)cfiArr[fileType].recBuf)->forwards[MAX_FORWARDOLD], 0, (MAX_FORWARD-MAX_FORWARDOLD)*sizeof(nodeNumXType));
               /* BBS export flag! */
               ((rawEchoType*)cfiArr[fileType].recBuf)->options.export2BBS = 1;
            }
            if (write(temphandle, cfiArr[fileType].recBuf, cfiArr[fileType].header.recordSize) !=
                                                           cfiArr[fileType].header.recordSize)
            {  free(cfiArr[fileType].recBuf);
               cfiArr[fileType].recBuf = NULL;
               close(temphandle);
               unlink(tempPath);
               goto error;
            }
         }
         converted = 1;
         free(cfiArr[fileType].recBuf);
         free(nodeNumBuf);
         cfiArr[fileType].recBuf = NULL;
         close(temphandle);
         close(cfiArr[fileType].handle);
         unlink(areaInfoPath);
         rename(tempPath, areaInfoPath);
         cfiArr[fileType].handle = -1;
         *header = NULL;
         *buf = NULL;
         goto restart;
      }
   }
#ifndef __32BIT__
   if ( fileType == 1 && cfiArr[fileType].header.totalRecords > 340 )
      goto error;
#endif
   if ((cfiArr[fileType].recBuf = malloc(fileData[fileType].bufferSize)) == NULL)
      goto error;
   *header = &cfiArr[fileType].header;
   *buf    = cfiArr[fileType].recBuf;
   return 1;
}


s16 getRec(u16 fileType, s16 index)
{
   if (cfiArr[fileType].handle == -1) return 0;

   if (lseek(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize+
             cfiArr[fileType].header.recordSize*(s32)index, SEEK_SET) == -1)
   {  return 0;
   }
   if (read(cfiArr[fileType].handle, cfiArr[fileType].recBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
   {  return 0;
   }
   return 1;
}


s16 putRec(u16 fileType, s16 index)
{
   if (cfiArr[fileType].handle == -1) return 0;

   *(u16*)cfiArr[fileType].recBuf = fileData[fileType].init;
   if (lseek(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize+
             cfiArr[fileType].header.recordSize*(s32)index, SEEK_SET) == -1)
   {  return 0;
   }
   if (write (cfiArr[fileType].handle, cfiArr[fileType].recBuf,
          cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
   {  return 0;
   }
   cfiArr[fileType].status = 1;
   return 1;
}


s16 insRec(u16 fileType, s16 index)
{
   s16  count;
   void *tempBuf;

   if (cfiArr[fileType].handle == -1) return 0;

   *(u16*)cfiArr[fileType].recBuf = fileData[fileType].init;

   if ((tempBuf = malloc(cfiArr[fileType].header.recordSize)) == NULL)
      return 0;

   count = cfiArr[fileType].header.totalRecords;

   while (--count >= index)
   {  if (lseek(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize+
        cfiArr[fileType].header.recordSize*(s32)count, SEEK_SET) == -1)
      {  free(tempBuf);
         return 0;
      }
      if (read(cfiArr[fileType].handle, tempBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
      {  free(tempBuf);
         return 0;
      }
      if (write(cfiArr[fileType].handle, tempBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
      {  free(tempBuf);
         return 0;
      }
   }
   free(tempBuf);
   if (lseek(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize+
             cfiArr[fileType].header.recordSize*(s32)index, SEEK_SET) == -1)
     return 0;

   if (write(cfiArr[fileType].handle, cfiArr[fileType].recBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
     return 0;

   cfiArr[fileType].header.totalRecords++;
   if (lseek(cfiArr[fileType].handle, 0, SEEK_SET) == -1)
     return 0;

   time(&cfiArr[fileType].header.lastModified);
   if (write(cfiArr[fileType].handle, &cfiArr[fileType].header, cfiArr[fileType].header.headerSize) != cfiArr[fileType].header.headerSize)
     return 0;

   cfiArr[fileType].status = 1;
   return 1;
}


s16 delRec(u16 fileType, s16 index)
{
   if (cfiArr[fileType].handle == -1) return 0;
   while (++index < cfiArr[fileType].header.totalRecords)
   {  if (lseek(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize+
                cfiArr[fileType].header.recordSize*(s32)index, SEEK_SET) == -1)
      {  return 0;
      }
      if (read(cfiArr[fileType].handle, cfiArr[fileType].recBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
      {  return 0;
      }
      if (lseek(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize+
                cfiArr[fileType].header.recordSize*(s32)(index-1), SEEK_SET) == -1)
      {  return 0;
      }
      if (write(cfiArr[fileType].handle, cfiArr[fileType].recBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
      {  return 0;
      }
   }
   chsize(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize+
          cfiArr[fileType].header.recordSize*(s32)--cfiArr[fileType].header.totalRecords);

   if (lseek(cfiArr[fileType].handle, 0, SEEK_SET) == -1)
   {  return 0;
   }
   time(&cfiArr[fileType].header.lastModified);
   write(cfiArr[fileType].handle, &cfiArr[fileType].header, cfiArr[fileType].header.headerSize);

   cfiArr[fileType].status = 1;
   return 1;
}


s16 chgNumRec(u16 fileType, s16 number)
{
    cfiArr[fileType].header.totalRecords = number;
    cfiArr[fileType].status = 1;
    return 1;
}


s16 closeConfig(u16 fileType)
{
   if (cfiArr[fileType].handle == -1) return 0;

   if ((cfiArr[fileType].status == 1) &&
       (lseek(cfiArr[fileType].handle, 0, SEEK_SET) != -1))
   {
      time(&cfiArr[fileType].header.lastModified);
      write(cfiArr[fileType].handle, &cfiArr[fileType].header, cfiArr[fileType].header.headerSize);
      chsize(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize+
                      cfiArr[fileType].header.recordSize*
                                      (s32)cfiArr[fileType].header.totalRecords);
   }
   close(cfiArr[fileType].handle);
   cfiArr[fileType].handle = -1;
   free(cfiArr[fileType].recBuf);
   cfiArr[fileType].recBuf = NULL;

   return 1;
}

