#ifndef cfgfileH
#define cfgfileH
//---------------------------------------------------------------------------
//
//  Copyright (C) 2007         Folkert J. Wijnstra
//  Copyright (C) 2007 - 2014  Wilfred van Velzen
//
//  Config file interface header file for FMail
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

#include "fmstruct.h"

#define CFG_GENERAL   0
#define CFG_NODES     1
#define CFG_ECHOAREAS 2
#define CFG_AREADEF   3
#define MAX_CFG_FILES 4

s16 openConfig (u16 fileType, headerType **header, void **buf);
s16 getRec     (u16 fileType, s16 index);
s16 putRec     (u16 fileType, s16 index);
s16 insRec     (u16 fileType, s16 index);
s16 delRec     (u16 fileType, s16 index);
s16 chgNumRec  (u16 fileType, s16 number);
s16 closeConfig(u16 fileType);

//---------------------------------------------------------------------------

#endif  // cfgfileH

