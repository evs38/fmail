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


int fileSys(const char *path);
int fsopen(const char *path, int access, unsigned mode, u16 lfn);
#define fsclose(handle)  close(handle)
#if !defined(FMAIL) && !defined(FTOOLS)
int fsmkdir(const char *path, u16 lfn);
int fsfindfirst(const char *pathname, struct ffblk *ffblk, int attrib, u16 lfn);
int fsunlink(const char *pathname, u16 lfn);
int fsrename(const char *oldname, const char *newname, u16 lfn);
#endif
