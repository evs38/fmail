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


#if defined(__MSDOS__) && !defined(__FMAILX__)

#define _os_install_msghandler()         _dos_install_msghandler()
#define _os_uninstall_msghandler()       _dos_uninstall_msghandler()
#define _os_execute(program, parameters) _dos_execute(program, parameters)

void _dos_install_msghandler(void);
void _dos_uninstall_msghandler(void);
s16  _dos_execute(uchar *program, uchar *parameters);

#else

#define _os_install_msghandler()
#define _os_uninstall_msghandler()
#define _os_execute(program, parameters)

#endif
