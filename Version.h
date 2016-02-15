#ifndef versionH
#define versionH
//----------------------------------------------------------------------------
//
// Copyright (C) 2011 - 2014  Wilfred van Velzen
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

#define VERNUM       "1.69"
#define SERIAL_MAJOR  1
#define SERIAL_MINOR  69

extern const char *TOOLSTR;

const char *VersionStr   (void);
const char *Version      (void);
const char *TearlineStr  (void);
const char *TIDStr       (void);
#ifdef __WIN32__
const char *VersionString(void);
#endif

#define PIDStr() TIDStr()

//----------------------------------------------------------------------------
#endif  // versionH

