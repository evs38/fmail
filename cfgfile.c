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

#ifdef __WIN32__
#include <share.h>
#endif // __WIN32__
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>    // SEEK_SET for mingw
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "fmail.h"

#include "cfgfile.h"
#include "os.h"
#include "os_string.h"
#include "utils.h"

// set to non-zero value if automatic conversion is desired
u16  allowConversion = 0;
extern configType config;

//---------------------------------------------------------------------------
typedef struct
{
  char       *fileName;
  u16         recordSize;
  u16         bufferSize;
  fhandle     handle;
  char       *recBuf;
  char       *revString;
  u16         revNumber;
  u16         dataType;
  u16         init;
  u16         status;
  headerType  header;

} configFileInfoType;
//---------------------------------------------------------------------------
#define dINITTAG(A,B) ((u16)(A) + ((u16)(B) << 8))

static configFileInfoType fileData[MAX_CFG_FILES] =
{
  // CFG_GENERAL
  {
    dCFGFNAME                                // fileName
  , 0                                        // recordSize
  , sizeof(configType)                       // bufferSize
  , -1                                       // handle
  , NULL                                     // recBuf
  , "FMail Configuration File rev. 1.1\x1a"  // revString
  , 0x0110                                   // revNumber
  , DATATYPE_CF                              // dataType
  , dINITTAG('C','F')                        // init
  , 0                                        // status
  }
  // CFG_NODES
, {
    dNODFNAME
  , 0
  , sizeof(nodeInfoType)
  , -1
  , NULL
  , "FMail Node File rev. 1.1\x1a"
  , 0x0110
  , DATATYPE_NO
  , dINITTAG('N','O')
  , 0
  }
  // CFG_ECHOAREAS
, {
    dARFNAME
  , 0
  , RAWECHO_SIZE
  , -1
  , NULL
  , "FMail Area File rev. 1.1\x1a"
  , 0x0110
  , DATATYPE_AE
  , dINITTAG('A','E')
  , 0
  }
  // CFG_AREADEF
, {
    dARDFNAME
  , 0
  , RAWECHO_SIZE
  , -1
  , NULL
  , "FMail Area File rev. 1.1\x1a"
  , 0x0110
  , DATATYPE_AD
  , dINITTAG('A','D')
  , 0
  }
};
//---------------------------------------------------------------------------
static configFileInfoType cfiArr[MAX_CFG_FILES];

//---------------------------------------------------------------------------
s16 openConfig(u16 fileType, headerType **header, void **buf)
{
   pathType areaInfoPath
          , tempPath;
   fhandle  temphandle;
   char    *helpPtr;
   u16      count
          , count2;
   u16      orgRecSize;
   nodeNumType *nodeNumBuf;
   u16      converted = 0;
   u16      oldAreasFile;

   if (fileType >= MAX_CFG_FILES)
      return 0;

   fileData[fileType].recordSize = fileData[fileType].bufferSize;
   if (fileType == CFG_ECHOAREAS || fileType == CFG_AREADEF)
      fileData[fileType].recordSize -= (MAX_FORWARDDEF - config.maxForward) * sizeof(nodeNumXType);

restart:
   strcpy(stpcpy(areaInfoPath, configPath), fileData[fileType].fileName);

   memset(&cfiArr[fileType].header, 0, sizeof(headerType));
   cfiArr[fileType].status = 0;

  if ((cfiArr[fileType].handle = _sopen(areaInfoPath, O_RDWR | O_BINARY | O_CREAT, SH_DENYRW, S_IREAD | S_IWRITE)) == -1)
  {
#ifdef _DEBUG
    printf("DEBUG open failed: %s\n", areaInfoPath);
#endif // _DEBUG
    return 0;
  }

   if (fileLength(cfiArr[fileType].handle) == 0)
   {
      strcpy((char*)cfiArr[fileType].header.versionString, fileData[fileType].revString);
      cfiArr[fileType].header.revNumber    = fileData[fileType].revNumber;
      cfiArr[fileType].header.headerSize   = sizeof(headerType);
      cfiArr[fileType].header.recordSize   = fileData[fileType].recordSize;
      cfiArr[fileType].header.dataType     = fileData[fileType].dataType;
      cfiArr[fileType].header.totalRecords = 0;
      cfiArr[fileType].header.lastModified =
      cfiArr[fileType].header.creationDate = time(NULL);

      write(cfiArr[fileType].handle, &cfiArr[fileType].header, sizeof(headerType));
   }
   else
   {
      if (sizeof(headerType) != read(cfiArr[fileType].handle, &cfiArr[fileType].header, sizeof(headerType)))
      {
#ifdef _DEBUG
         printf("DEBUG openConfig error read\n");
#endif // _DEBUG
error:   close(cfiArr[fileType].handle);
         cfiArr[fileType].handle = -1;
         *header = NULL;
         *buf = NULL;
         return 0;
      }

      if (memcmp(cfiArr[fileType].header.versionString, "FMail", 5) ||
          (cfiArr[fileType].header.headerSize < sizeof(headerType)) ||
          ((!allowConversion || converted) && cfiArr[fileType].header.recordSize < fileData[fileType].recordSize) ||
          (cfiArr[fileType].header.dataType != fileData[fileType].dataType))
        goto error;

      if ( cfiArr[fileType].header.recordSize < fileData[fileType].recordSize ||
           ((fileType == CFG_ECHOAREAS || fileType == CFG_AREADEF) &&
           cfiArr[fileType].header.recordSize > fileData[fileType].recordSize) )
      {
         // convert old record format
         oldAreasFile = (fileType == CFG_ECHOAREAS || fileType == CFG_AREADEF) &&
                        cfiArr[fileType].header.recordSize < sizeof(rawEchoType)-(MAX_FORWARDDEF-MAX_FORWARDOLD)*sizeof(nodeNumXType);

         strcpy(tempPath, areaInfoPath);
         if ( (helpPtr = strrchr(tempPath, '.')) == NULL )
            goto error;
         strcpy(helpPtr + 1, dEXTTMP);
         if ( (cfiArr[fileType].recBuf = (char*)malloc(fileData[fileType].bufferSize)) == NULL )
            goto error;
         if ( (nodeNumBuf = (nodeNumType*)malloc(MAX_FORWARDOLD * sizeof(nodeNumType))) == NULL )
         {
            free(cfiArr[fileType].recBuf);
            goto error;
         }
         if ((temphandle = _sopen(tempPath, O_RDWR | O_BINARY | O_CREAT, SH_DENYRW, S_IREAD | S_IWRITE)) == -1)
         {
            free(cfiArr[fileType].recBuf);
            cfiArr[fileType].recBuf = NULL;
            goto error;
         }
         orgRecSize = cfiArr[fileType].header.recordSize;
         strcpy((char*)cfiArr[fileType].header.versionString, fileData[fileType].revString);
         cfiArr[fileType].header.revNumber    = fileData[fileType].revNumber;
         cfiArr[fileType].header.headerSize   = sizeof(headerType);
         cfiArr[fileType].header.recordSize   = fileData[fileType].recordSize;
         cfiArr[fileType].header.dataType     = fileData[fileType].dataType;
         cfiArr[fileType].header.lastModified = time(NULL);
         write(temphandle, &cfiArr[fileType].header, sizeof(headerType));
         for ( count = 0; count < cfiArr[fileType].header.totalRecords; count++ )
         {
            memset(cfiArr[fileType].recBuf, 0, fileData[fileType].recordSize);
            if (read(cfiArr[fileType].handle, cfiArr[fileType].recBuf, orgRecSize) != orgRecSize)
            {
              free(cfiArr[fileType].recBuf);
              cfiArr[fileType].recBuf = NULL;
              close(temphandle);
              unlink(tempPath);
              goto error;
            }
            if (oldAreasFile)
            {
               memcpy(nodeNumBuf, &((rawEchoType*)cfiArr[fileType].recBuf)->readSecRA, MAX_FORWARDOLD * sizeof(nodeNumType));
               memcpy(cfiArr[fileType].recBuf + offsetof(rawEchoType, readSecRA),
                      cfiArr[fileType].recBuf + offsetof(rawEchoType, readSecRA) + MAX_FORWARDOLD * sizeof(nodeNumType),
                      offsetof(rawEchoType, forwards) - offsetof(rawEchoType, readSecRA));
               for ( count2 = 0; count2 < MAX_FORWARDOLD; count2++ )
               {
                  memset(&((rawEchoType*)cfiArr[fileType].recBuf)->forwards[count2], 0, sizeof(nodeNumXType));
                  ((rawEchoType*)cfiArr[fileType].recBuf)->forwards[count2].nodeNum = nodeNumBuf[count2];
               }
               memset(&((rawEchoType*)cfiArr[fileType].recBuf)->forwards[MAX_FORWARDOLD], 0, (MAX_FORWARD-MAX_FORWARDOLD)*sizeof(nodeNumXType));
               /* BBS export flag! */
               ((rawEchoType*)cfiArr[fileType].recBuf)->options.export2BBS = 1;
            }
            if (write(temphandle, cfiArr[fileType].recBuf, cfiArr[fileType].header.recordSize) !=
                                                           cfiArr[fileType].header.recordSize)
            {
               free(cfiArr[fileType].recBuf);
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
  if ((cfiArr[fileType].recBuf = (char*)malloc(fileData[fileType].bufferSize)) == NULL)
    goto error;
  *header = &cfiArr[fileType].header;
  *buf    = cfiArr[fileType].recBuf;

  return 1;
}
//---------------------------------------------------------------------------
s16 getRec(u16 fileType, s16 index)
{
  if (cfiArr[fileType].handle == -1)
    return 0;

  if (lseek( cfiArr[fileType].handle, cfiArr[fileType].header.headerSize
           + cfiArr[fileType].header.recordSize * (s32)index, SEEK_SET) == -1
     )
    return 0;

  if (read(cfiArr[fileType].handle, cfiArr[fileType].recBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
    return 0;

  return 1;
}
//---------------------------------------------------------------------------
s16 putRec(u16 fileType, s16 index)
{
  if (cfiArr[fileType].handle == -1)
    return 0;

  *(u16*)cfiArr[fileType].recBuf = fileData[fileType].init;
  if (lseek( cfiArr[fileType].handle, cfiArr[fileType].header.headerSize
           + cfiArr[fileType].header.recordSize * (s32)index, SEEK_SET) == -1
     )
    return 0;

  if (write( cfiArr[fileType].handle, cfiArr[fileType].recBuf
           , cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize
     )
    return 0;

  cfiArr[fileType].status = 1;

  return 1;
}
//---------------------------------------------------------------------------
s16 insRec(u16 fileType, s16 index)
{
  s16  count;
  void *tempBuf;

  if (cfiArr[fileType].handle == -1)
    return 0;

  *(u16*)cfiArr[fileType].recBuf = fileData[fileType].init;

  if ((tempBuf = malloc(cfiArr[fileType].header.recordSize)) == NULL)
    return 0;

  count = cfiArr[fileType].header.totalRecords;

  while (--count >= index)
  {
    if ( lseek(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize
       + cfiArr[fileType].header.recordSize*(s32)count, SEEK_SET) == -1
       )
    {
      free(tempBuf);
      return 0;
    }
    if (read(cfiArr[fileType].handle, tempBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
    {
      free(tempBuf);
      return 0;
    }
    if (write(cfiArr[fileType].handle, tempBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
    {
      free(tempBuf);
      return 0;
    }
  }
  free(tempBuf);
  if (lseek( cfiArr[fileType].handle, cfiArr[fileType].header.headerSize
           + cfiArr[fileType].header.recordSize*(s32)index, SEEK_SET) == -1
     )
    return 0;

  if (write(cfiArr[fileType].handle, cfiArr[fileType].recBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
    return 0;

  cfiArr[fileType].header.totalRecords++;
  if (lseek(cfiArr[fileType].handle, 0, SEEK_SET) == -1)
    return 0;

  cfiArr[fileType].header.lastModified = time(NULL);
  if (write(cfiArr[fileType].handle, &cfiArr[fileType].header, cfiArr[fileType].header.headerSize) != cfiArr[fileType].header.headerSize)
    return 0;

  cfiArr[fileType].status = 1;

  return 1;
}
//---------------------------------------------------------------------------
s16 delRec(u16 fileType, s16 index)
{
  if (cfiArr[fileType].handle == -1)
    return 0;

  while (++index < cfiArr[fileType].header.totalRecords)
  {
    if (lseek(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize+
              cfiArr[fileType].header.recordSize*(s32)index, SEEK_SET) == -1)
      return 0;

    if (read(cfiArr[fileType].handle, cfiArr[fileType].recBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
      return 0;

    if (lseek(cfiArr[fileType].handle, cfiArr[fileType].header.headerSize+
              cfiArr[fileType].header.recordSize*(s32)(index-1), SEEK_SET) == -1)
      return 0;

    if (write(cfiArr[fileType].handle, cfiArr[fileType].recBuf, cfiArr[fileType].header.recordSize) != cfiArr[fileType].header.recordSize)
      return 0;

  }
  ftruncate( cfiArr[fileType].handle, cfiArr[fileType].header.headerSize
        + cfiArr[fileType].header.recordSize*(s32)--cfiArr[fileType].header.totalRecords);

  if (lseek(cfiArr[fileType].handle, 0, SEEK_SET) == -1)
    return 0;

  cfiArr[fileType].header.lastModified = time(NULL);
  write(cfiArr[fileType].handle, &cfiArr[fileType].header, cfiArr[fileType].header.headerSize);

  cfiArr[fileType].status = 1;

  return 1;
}
//---------------------------------------------------------------------------
s16 chgNumRec(u16 fileType, s16 number)
{
  cfiArr[fileType].header.totalRecords = number;
  cfiArr[fileType].status = 1;

  return 1;
}
//---------------------------------------------------------------------------
s16 closeConfig(u16 fileType)
{
  if (cfiArr[fileType].handle == -1)
    return 0;

  if (  cfiArr[fileType].status == 1
     && lseek(cfiArr[fileType].handle, 0, SEEK_SET) != -1
     )
  {
    cfiArr[fileType].header.lastModified = time(NULL);
    write(cfiArr[fileType].handle, &cfiArr[fileType].header, cfiArr[fileType].header.headerSize);
    ftruncate( cfiArr[fileType].handle, cfiArr[fileType].header.headerSize
          + cfiArr[fileType].header.recordSize * (s32)cfiArr[fileType].header.totalRecords);
  }
  close(cfiArr[fileType].handle);
  cfiArr[fileType].handle = -1;
  free(cfiArr[fileType].recBuf);
  cfiArr[fileType].recBuf = NULL;

  return 1;
}
//---------------------------------------------------------------------------
