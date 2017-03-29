#ifndef osH
#define osH
// ----------------------------------------------------------------------------
//  Copyright (C) 2017  Wilfred van Velzen
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
// ----------------------------------------------------------------------------
#ifdef __linux__

#define O_BINARY   0
#define O_TEXT     0
#define	SH_DENYRW  0x10	/* Deny read/write */
#define	SH_DENYWR  0x20	/* Deny write */
#define	SH_DENYRD  0x30	/* Deny read */
#define MKDIR(d)   mkdir(d, 0777)
#define dDIRSEPS   "/"
#define dDIRSEPC   '/'

int eof(int fd);
int _sopen(const char *pathname, int flags, int shflags, ... /* mode_t mode */);

#endif // __linux__
// ----------------------------------------------------------------------------
#ifdef __WIN32__

#define MKDIR(d)  mkdir(d)
#define dDIRSEPS  "\\"
#define dDIRSEPC  '\\'

#endif // __WIN32__
// ----------------------------------------------------------------------------
#if !(defined(dDIRSEPS) && defined(dDIRSEPC))
#error "Define dDIRSEP for your OS!"
#endif // dDIRSEP
// ----------------------------------------------------------------------------
#endif // osH
