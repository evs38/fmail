#ifndef pp_dateH
#define pp_dateH
//----------------------------------------------------------------------------
//
// Copyright (C) 2011 - 2016 Wilfred van Velzen
//
//----------------------------------------------------------------------------
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
//
// Contains just pre-processor date variables derived from __DATE__
//
//----------------------------------------------------------------------------

#define NUM(C)   ((C) - '0')
#define DIGIT(C) (((C) == ' ') ? 0 : NUM(C))

#define YEAR  (((NUM(__DATE__[7]) * 10 + NUM(__DATE__[8])) * 10 + NUM(__DATE__[9])) * 10 + NUM(__DATE__[10]))
#define MONTH (__DATE__[2] == 'n' ? (__DATE__[1] == 'a' ? 0 : 5) \
: __DATE__[2] == 'b' ? 1 \
: __DATE__[2] == 'r' ? (__DATE__[0] == 'M' ? 2 : 3) \
: __DATE__[2] == 'y' ? 4 \
: __DATE__[2] == 'l' ? 6 \
: __DATE__[2] == 'g' ? 7 \
: __DATE__[2] == 'p' ? 8 \
: __DATE__[2] == 't' ? 9 \
: __DATE__[2] == 'v' ? 10 : 11)
#define DAY (DIGIT(__DATE__[4]) * 10 + NUM(__DATE__[5]))

#define YEAR_STR  (__DATE__ + 7)
#define MONTH_STR (__DATE__[2] == 'n' ? (__DATE__[1] == 'a' ? "01" : "06") \
: __DATE__[2] == 'b' ? "02" \
: __DATE__[2] == 'r' ? (__DATE__[0] == 'M' ? "03" : "04") \
: __DATE__[2] == 'y' ? "05" \
: __DATE__[2] == 'l' ? "07" \
: __DATE__[2] == 'g' ? "08" \
: __DATE__[2] == 'p' ? "09" \
: __DATE__[2] == 't' ? "10" \
: __DATE__[2] == 'v' ? "11" : "12")
#define DAY_STR (\
"00\0"\
"01\0"\
"02\0"\
"03\0"\
"04\0"\
"05\0"\
"06\0"\
"07\0"\
"08\0"\
"09\0"\
"10\0"\
"11\0"\
"12\0"\
"13\0"\
"14\0"\
"15\0"\
"16\0"\
"17\0"\
"18\0"\
"19\0"\
"20\0"\
"21\0"\
"22\0"\
"23\0"\
"24\0"\
"25\0"\
"26\0"\
"27\0"\
"28\0"\
"29\0"\
"30\0"\
"31" + (3*DAY))

//----------------------------------------------------------------------------
#endif  // pp_dateH
