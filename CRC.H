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


u32 crc32      (uchar *string);
u32 crc32old   (uchar *string);
u32 crc32len   (uchar *string, s16 len);
u32 crc32lennc (uchar *string, s16 len); /* for JAM */
u32 crc32alpha (uchar *string);
u32 crc32t     (uchar *string, uchar terminator);
u32 crc32jam   (uchar *string);

