/*
**  folder.h (FrontDoor)
**
**  Copyright 1989, 1990 Joaquim H. Homrighausen. All rights reserved
**
**  Definitions for each record in FOLDER.SYS
**
**  Last revision:  90-02-22
 **  Update FJW  :  94-08-09 indented
**
**  -------------------------------------------------------------------------
**  This information is not necessarily final and is subject to change at any
**  given time without further notice
**  -------------------------------------------------------------------------
*/

/*
**  Constant long bit values
*/
#define FD_RESTRICT    0x00000001L
#define FD_ECHO_INFO   0x00000002L
#define FD_EXPORT_OK   0x00000004L
#define FD_USE_XLAT    0x00000008L
#define FD_PRIVATE     0x00000010L
#define FD_READONLY    0x00000020L
#define FD_JAMAREA     0x01000000L
#define FD_NETMAIL     0x08000000L
#define FD_QUICKBBS    0x10000000L
#define FD_DELETED     0x20000000L         /* Never written to disk */
#define FD_LOCAL       0x40000000L
#define FD_ECHOMAIL    0x80000000L


/*
**  User access mask
*/
#define USER_1      0x00000001L
#define USER_2      0x00000002L
#define USER_3      0x00000004L
#define USER_4      0x00000008L
#define USER_5      0x00000010L
#define USER_6      0x00000020L
#define USER_7      0x00000040L
#define USER_8      0x00000080L
#define USER_9      0x00000100L
#define USER_10     0x00000200L


#include <pshpack1.h>
/*
**  Folder structure
**
**  The "path" and "title" fields below are NUL terminated.
*/
typedef struct
{
  char    path [65],      /* Path if "board==0", otherwise emtpy */
          title[41];      /* Title to appear on screen */
  u8      origin;         /* Default origin line, 0-19 */
  s32     behave;         /* Behavior, see above */
  s32     pwdcrc;         /* CRC32 of password or -1L if unprotected */
  s32     userok;         /* Users with initial access */
  u8      useaka;         /* AKA to use, 0==primary */
  u16     board;          /* QuickBBS/RemoteAccess board number */

} FOLDER, *FOLDERPTR;

#include <poppack.h>
/* end of file "folder.h" */
