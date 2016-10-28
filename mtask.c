//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2016 Wilfred van Velzen
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

#if defined(__WIN32__)
  #include <windows.h>
#elif defined(__linux__)
  #include <sched.h>
  #include <unistd.h>
#endif

#include "mtask.h"

#include "config.h"

//---------------------------------------------------------------------------
// arg == 0 : Just give up current timeslice
// arg != 0 : For busy loops, wait about 10 milliseconds
//---------------------------------------------------------------------------
void returnTimeSlice(int arg)
{
#if   defined(FTOOLS)
  if (!arg && !config.genOptions.timeSliceFT)
    return;
#elif defined(FMAIL)
  if (!arg && !config.genOptions.timeSliceFM)
    return;
#endif

#if   defined(__WIN32__)
  Sleep(arg ? 10 : 0);      // milliseconds
#elif defined(__linux__)
  if (arg)
    usleep(10000);  // microseconds
  else
    sched_yield();
#else
  #error "Specify a sleep function"
#endif
}
//---------------------------------------------------------------------------

