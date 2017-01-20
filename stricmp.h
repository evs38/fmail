#ifndef stricmpH
#define stricmpH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2016 - 2017  Wilfred van Velzen
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

#if defined(__MINGW32__) || defined(_LINUX)

int _fm_stricmp (const char *s1, const char *s2);
int _fm_strnicmp(const char *s1, const char *s2, long unsigned int n);

#define stricmp(a,b)    _fm_stricmp(a,b)
#define strcmpi(a,b)    _fm_stricmp(a,b)
#define strnicmp(a,b,c) _fm_strnicmp(a,b,c)

#endif
//---------------------------------------------------------------------------
#endif  // stricmpH
