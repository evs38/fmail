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


#include <dos.h>
#include "fmail.h"


/* Abort, Retry, Fail handler */

static s16 _dos_msghandler(void)
{
	_hardresume(_HARDERR_FAIL);
	return 0;
}

void _dos_install_msghandler(void)
{
	 harderr(_dos_msghandler);
}

void _dos_uninstall_msghandler(void)
{
}

