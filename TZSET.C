/*------------------------------------------------------------------------
 * filename - tzset.c
 *
 * function(s)
 *        tzset     - UNIX time compatibility
 *        _isDst    - determines whether daylight savings is in effect
 *        _totalsec - converts a date to seconds since 1970
 *-----------------------------------------------------------------------*/

/*
 *      C/C++ Run Time Library - Version 1.5
 *
 *      Copyright (c) 1987, 1994 by Borland International
 *      All Rights Reserved.
 *
 */

#if defined(__WIN32__)
#include <ntbc.h>
#endif

#include <time.h>
#include <_time.h>
#include <_ctype.h>
#define ISALPHA(c) iscalpha(c)
#include <stdlib.h>
#include <string.h>

#define YES 1
#define NO  0

#define Normal    0
#define Daylight  1
#define TZstrlen        3        /* Len of tz string(- null terminator) */
#define DefaultTimeZone 0L
#define DefaultDaylight NO
#define DefaultTZname   "GMT"    /* Default normal time zone name */
#define DefaultDSTname  "GMT"    /* Default daylight savings zone name */

/*---------------------------------------------------------------------*

Name            tzset

Usage           void tzset(void);

Prototype in    time.h

Description     sets local timezone info base on the "TZ" environment string

Return value    None

*---------------------------------------------------------------------*/

void _RTLENTRY _EXPFUNC tzset(void)
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

--=====================_37360781==_
