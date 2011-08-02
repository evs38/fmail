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


#define CONNECTION_CHECK        0
#define USER_CHECK		1
#define PASSWORD_CHECK          2
#define QUIT_CHECK		3
#define DELETE_CHECK            4
#define RSET_CHECK		5 
#define STAT_CHECK		6 
#define NOOP_CHECK		7
#define LIST_CHECK		8
#define RETR_CHECK		9

u8 *popGetText(int peek);
int popConnect(char *host, char *user, char *password);
int popDelete(int msgNumber);
int popDisconnect(void);
int popNoop(void);
int popReset(void);
u8 *popRetrieve(int msgNumber);
int popStat(u16 *a, u16 *b);
u8 *popList(u16 *a);


