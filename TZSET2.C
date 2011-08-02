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


#if defined(__WIN32__)
#pragma pack(4)
#define _X86_
#define i386
#define WIN32
#define _M_IX86 300
#define NONAMELESSUNION
#define NO_ANONYMOUS_STRUCT
#define _UNION_NAME(X) X
#define _WINERROR_
#define i386
#define WIN32
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#pragma pack()
#endif

#include <time.h>
#include <ctype.h>
#define ISALPHA(c) isalpha(c)
#include <stdlib.h>
#include <string.h>

extern char  _Days[12];
extern int   _YDays[13];

#define Normal    		0
#define Daylight  		1
#define TZstrlen        3
#define DefaultTimeZone 0L
#define DefaultDaylight 0
#define DefaultTZname   "GMT"
#define DefaultDSTname  "GMT"



void tzset2(void)
{
#pragma startup tzset 30

	 register int  i;
	 char *env;

#define  issign(c)   (((c) == '-') || ((c) == '+'))

	 if ( ((env = getenv("TZ")) == 0)                                       ||
			 (strlen(env) < (TZstrlen+1))                                     ||
			 ((!ISALPHA(env[0])) || (!ISALPHA(env[1])) || (!ISALPHA(env[2]))) ||
			 (!(issign(env[ TZstrlen ]) || isdigit(env[ TZstrlen ])))         ||
			 ((!isdigit(env[ TZstrlen ])) && (!isdigit(env[ TZstrlen+1 ]))) )
	 {
#if defined(__WIN32__)
		  TIME_ZONE_INFORMATION tzi;

		  _daylight = (GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_DAYLIGHT);
		  _timezone = (tzi.Bias + tzi.StandardBias) * 60;
		  strcpy(_tzname[Normal], "");
		  strcpy(_tzname[Daylight], "");
#else
		  _daylight = DefaultDaylight;
		  _timezone = DefaultTimeZone * 60L * 60L;
		  strcpy(_tzname[Normal], DefaultTZname);
		  strcpy(_tzname[Daylight], DefaultDSTname);
#endif
	 }
	 else
	 {
		  memset(_tzname[Daylight], 0, TZstrlen+1);
		  strncpy(_tzname[Normal], env, TZstrlen);
		  _tzname[Normal][TZstrlen] = '\0';
		  _timezone = atol(&env[TZstrlen]) * 3600L;

		  for ( _daylight=0, i=TZstrlen; env[i]; i++ )
		  {
				if ( ISALPHA(env[i]) )
				{
					 if ( (strlen(&env[i])<TZstrlen) ||
												(!ISALPHA(env[i+1])) ||
												(!ISALPHA(env[i+2])) )
						  break;
					 strncpy(_tzname[Daylight], &env[i], TZstrlen);
					 _tzname[Daylight][TZstrlen] = '\0';
					 _daylight = 1;
					 break;
				}
		  }
	 }
}

