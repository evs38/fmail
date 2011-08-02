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


#ifdef __OS2__

#define INCL_DOSPROCESS

#include <os2.h>

#include "fmstruct.h"
#include "config.h"
#include "mtask.h"

void returnTimeSlice(u16 arg)
{
#ifdef FMAIL
   if ( !arg && !config.genOptions.timeSliceFM )
      return;
#elif !defined FSETUP
   if ( !arg && !config.genOptions.timeSliceFT )
      return;
#else
   arg = arg;
#endif

   DosSleep(1);
}

#endif

#if defined __32BIT__ && !defined __OS2__

#include "dos.h"

#include "fmstruct.h"
#include "config.h"
#include "mtask.h"

void returnTimeSlice(u16 arg)
{
#ifndef FMAIL
   if ( !arg || !config.genOptions.timeSliceFT )
      return;
#else
   if ( !arg || !config.genOptions.timeSliceFM )
      return;
#endif
   sleep(1);
}
#endif

#if !defined __OS2__ && !defined __32BIT__

#include "dos.h"

#include "fmstruct.h"
#include "config.h"
#include "mtask.h"

#define  DESQVIEW  1
#define  WINDOWS   2
#define  OS2       3


static udef multitasker = 0;


void getMultiTasker(void)
{
   multitasker = 0;

   /* check DesqView */

   _AX = 0x2B01;
   _CX = 'ED';
   _DX = 'QS';
   geninterrupt(0x21);
   if ( _AL != 0xFF )
   {  multitasker = DESQVIEW;
      return;
   }

   /* check Windows */

   _AX = 0x1600;
   geninterrupt(0x2F);
   if ( _AL != 0x00 && _AL != 0x80 )
   {  multitasker = WINDOWS;
      return;
   }

   /* check OS/2 */

   _AX = 0x3001;
   geninterrupt(0x21);
   if ( _AL == 0x0A || _AL == 0x14 )
   {  multitasker = OS2;
      return;
   }
}

void returnTimeSlice(u16 arg)
{
#ifndef FMAIL
   if ( !arg && !config.genOptions.timeSliceFT )
      return;
#else
   if ( !arg && !config.genOptions.timeSliceFM )
      return;
#endif

   switch ( multitasker )
   {
      case DESQVIEW  : _AX = 0x1000;
		       geninterrupt(0x15);
                       break;
      case WINDOWS   :
      case OS2       : _AX = 0x1680;
                       geninterrupt(0x2F);
                       break;
   }
}

#if 0
u8 *multiTxt(void)
{
   switch ( multitasker )
   {
      case DESQVIEW  : return "desqVIEW";
      case WINDOWS   : return "Windows";
      case OS2       : return "OS/2";
      default        : return "";
   }
}
#endif

#endif
