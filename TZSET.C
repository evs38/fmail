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


#include <time.h>
#include <ctype.h>
#include <string.h>

#define Normal				0
#define Daylight			1
#define TZstrlen			3
#define DefaultTimeZone 0L
#define DefaultDaylight 0
#define DefaultTZname   "GMT"
#define DefaultDSTname  "GMT"


void tzset(void)
{
#ifndef __OS2__
	_daylight = DefaultDaylight;
	_timezone = DefaultTimeZone * 60L * 60L;
   strcpy(_tzname[Normal], DefaultTZname);
   strcpy(_tzname[Daylight], DefaultDSTname);
#else
   daylight = DefaultDaylight;
   timezone = DefaultTimeZone * 60L * 60L;
   strcpy(tzname[Normal], DefaultTZname);
   strcpy(tzname[Daylight], DefaultDSTname);
#endif
}
