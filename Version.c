//----------------------------------------------------------------------------
//
// Copyright (C) 2011  Wilfred van Velzen
//
// This file is part of FMail.
//
// FMail is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// FMail is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//----------------------------------------------------------------------------

#include "Version.h"

#include "pp_date.h"

#include <stdio.h>

#ifdef __OS2__
	#define PTYPE "/OS2"
	#define P386
#elif defined __WIN32__ && !defined __DPMI32__
	#define PTYPE "-Win32"
   #define P386
#elif defined __FMAILX__
	#ifdef __DPMI32__
		#define PTYPE "X32"
		#define P386
	#elif defined __DPMI16__
		#define PTYPE "X"
		#define P386
	#else
		#define PTYPE "W"
		#define P386
	#endif
#elif defined P386
	#define PTYPE "/386"
#else
	#define PTYPE ""
#endif

#ifdef GOLDBASE
#define MBTYPE "G"
#define MBNAME "QuickBBS Gold"
#else
#define MBTYPE
#define MBNAME "Hudson"
#endif

#ifdef BETA
  #define BLSTR "-Beta"
  #define BSSTR "-B"
#else
  #define BLSTR ""
  #define BSSTR ""
#endif  

#if   defined(FSETUP)
  #define TOOL "FSetup"
#elif defined(FTOOLS)
  #define TOOL "FTools"
#else // FMAIL
  #define TOOL "FMail"
#endif  

#define VERSION_STR  TOOL PTYPE MBTYPE"-"VERNUM BLSTR
#define TEARLINE_STR "--- "TOOL PTYPE"-"VERNUM BSSTR
#define TID_STR      TOOL PTYPE"-"VERNUM BSSTR

//----------------------------------------------------------------------------
const char versionStr[] = VERSION_STR;
const char *VersionStr(void)
{
  return versionStr;
}
//----------------------------------------------------------------------------
char tearStr[82] = { 0 };
const char *TearlineStr(void)
{
#ifdef BETA
  if (*tearStr == 0)
    sprintf(tearStr, TEARLINE_STR"%04d%02d%02d\r", YEAR, MONTH + 1, DAY);
#else    
  if (*tearStr == 0)
    sprintf(tearStr, TEARLINE_STR"\r");
#endif    
  return tearStr;
}
//----------------------------------------------------------------------------
#ifdef BETA
char tidStr[82] = { 0 };
#else
const char tidStr[] = TID_STR;
#endif
const char *TIDStr(void)
{
#ifdef BETA
  if (*tidStr == 0)
    sprintf(tidStr, TID_STR"%04d%02d%02d", YEAR, MONTH + 1, DAY);
#endif    
  return tidStr;
}
//----------------------------------------------------------------------------

