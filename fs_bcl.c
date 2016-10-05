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


#include <stdio.h>
#include <string.h>
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dir.h>

#include "fmail.h"
#include "cfgfile.h"
#include "config.h"
#include "nodeinfo.h"
#include "areainfo.h"
#include "archive.h"
#include "utils.h"
#include "log.h"
#include "mtask.h"
#include "msgpkt.h"
#include "output.h"
#include "bclfun.h"

extern  time_t  startTime;


/* =============== BCL ================= */

bcl_header_type bcl_header;

bcl_type bcl;

/* ============================================= */

static fhandle bclHandle;


udef openBCL(uplinkReqType *uplinkReq)
{  tempStrType tempStr;
   nodeNumType tempNode;

   sprintf (tempStr, "%s%s", configPath, uplinkReq->fileName);
   if ( (bclHandle = open(tempStr, O_RDONLY|O_BINARY|O_DENYALL, S_IREAD|S_IWRITE)) == -1 ||
        read(bclHandle, &bcl_header, sizeof(bcl_header)) != sizeof(bcl_header) )
   {  close(bclHandle);
      return 0;
   }
   tempNode.point = 0;
   if ( memcmp(bcl_header.FingerPrint, "BCL", 4) ||
        sscanf (bcl_header.Origin, "%hu:%hu/%hu.%hu",
                                   &tempNode.zone, &tempNode.net,
                                   &tempNode.node, &tempNode.point) < 3 )
   {  close(bclHandle);
      return 0;
   }
   return 1;
}


udef readBCL(u8 **tag, u8 **descr)
{  static u8  buf[256];

   if (eof(bclHandle))
      return 0;
   if (read(bclHandle, &bcl, sizeof(bcl_type)) != sizeof(bcl_type))
      return 0;
   if (read(bclHandle, buf, bcl.EntryLength-sizeof(bcl_type)) != bcl.EntryLength - sizeof(bcl_type))
      return 0;
   *tag = buf;
   *descr = strchr(buf, 0) + 1;
   return 1;
}


udef closeBCL(void)
{  return close(bclHandle);
}

