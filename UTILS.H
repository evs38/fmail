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


#define ECHO_SEENBY 0
#define ECHO_PATH   1

#define FL_DIRECT       1
#define FL_IMMEDIATE    2
#define FL_FILE_REQ     4
#define FL_LOK        128

#define SW_A  0x00000001L
#define SW_B  0x00000002L
#define SW_C  0x00000004L
#define SW_D  0x00000008L
#define SW_E  0x00000010L
#define SW_F  0x00000020L
#define SW_G  0x00000040L
#define SW_H  0x00000080L
#define SW_I  0x00000100L
#define SW_J  0x00000200L
#define SW_K  0x00000400L
#define SW_L  0x00000800L
#define SW_M  0x00001000L
#define SW_N  0x00002000L
#define SW_O  0x00004000L
#define SW_P  0x00008000L
#define SW_Q  0x00010000L
#define SW_R  0x00020000L
#define SW_S  0x00040000L
#define SW_T  0x00080000L
#define SW_U  0x00100000L
#define SW_V  0x00200000L
#define SW_W  0x00400000L
#define SW_X  0x00800000L
#define SW_Y  0x01000000L
#define SW_Z  0x02000000L

int   moveFile        (char *oldName, char *newName);
s16   existDir        (uchar *dir, uchar *descr);
u32   diskFree        (uchar *path);
s16   matchAka        (nodeNumType *node, uchar useAka);
void  touch           (uchar *path, uchar *filename, uchar *t);
s16   emptyText       (uchar *text);
void  delete          (uchar *path, uchar *wildCard);
s32   getSwitch       (int *argc, uchar *argv[], s32 mask);
u32   uniqueID        (void);
void  waitID          (void);
uchar *removeRe       (uchar *string);
void  removeLfSr      (uchar *msgText);
uchar *srchar         (uchar *string, s16 t, s16 c);
uchar *nodeStr        (nodeNumType *nodeNum);
uchar *nodeStrZ       (nodeNumType *nodeNum);
uchar *findCLStr      (uchar *s1, uchar *s2);
uchar *findCLiStr     (uchar *s1, uchar *s2);
void  removeLine      (uchar *s);
void  addINTL         (internalMsgType *message);
s16   getLocalAka     (nodeNumType *node);
s16   isLocalPoint    (nodeNumType *node);
s16   node2d          (nodeNumType *node);
void  make2d          (internalMsgType *message);
s16   node4d          (nodeNumType *node);
void  point4d         (internalMsgType *message);
void  make4d          (internalMsgType *message);
s16   getLocalAkaNum  (nodeNumType *node);
uchar *insertLine     (uchar *pos, uchar *line);
uchar *insertLineN    (uchar *pos, uchar *line, u16 num);
void  addPathSeenBy   (uchar *msgText, uchar *seenBy, uchar *tinySeen, uchar *path,
                       echoToNodeType echoToNode, u16 areaIndex);
void  addVia          (uchar *msgText, u16 aka);
s16   scanDate        (uchar *datePtr, u16 *year, u16 *month, u16 *day,
                       u16 *hours, u16 *minutes);
s32   checkDate       (u16 year,  u16 month,  u16 day,
                       u16 hours, u16 minutes, u16 seconds);
uchar *makeName       (uchar *path, uchar *name);
long  fmseek          (int handle, long offset, int fromwhere, int code);
uchar *makeFullPath   (uchar *deflt, uchar *override, uchar *name);
u16   getKludge       (char *txt, char *kludge, char *subfield, u16 bufsize);
void  getKludgeNode   (char *txt, char *kludge, nodeNumType *nodeNum);
u16   firstFile       (u8 *string, u8 **filename);
u16   nextFile        (u8 **filename);

