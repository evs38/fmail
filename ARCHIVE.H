#ifndef archiveH
#define archiveH
// ----------------------------------------------------------------------------
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2014  Wilfred van Velzen
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

#include "fmail.h"

void unpackArc(char *fullFileName, struct _finddata_t *fdArc);
s16  packArc  (char *qqqName, nodeNumType *srcNode, nodeNumType *destNode, nodeInfoType *nodeInfo);
void retryArc (void);

// ----------------------------------------------------------------------------
#endif  // archiveH