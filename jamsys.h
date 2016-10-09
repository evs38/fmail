//  JAM(mbp) - The Joaquim-Andrew-Mats Message Base Proposal
//
//  C API
//
//  Written by Joaquim Homrighausen and Mats Wallin.
//
//  ----------------------------------------------------------------------
//
//  jamsys.h (JAMmb)
//
//  Compiler and platform dependant definitions
//
//  Copyright 1993 Joaquim Homrighausen, Andrew Milner, Mats Birch, and
//  Mats Wallin. ALL RIGHTS RESERVED.
//
//  93-06-28    JoHo/MW
//  Initial coding.

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __JAMSYS_H__
#define __JAMSYS_H__

//  The following assumptions are made about compilers and platforms:
//
//  __MSDOS__       Defined if compiling for MS-DOS
//  __32BIT__         Defined if compiling for OS/2 2.x
//
//  __SMALL__       Defined if compiling under MS-DOS in small memory model
//  __MEDIUM__      Defined if compiling under MS-DOS in medium memory model
//  __COMPACT__     Defined if compiling under MS-DOS in compact memory model
//  __LARGE__       Defined if compiling under MS-DOS in large memory model
//
//  __BORLANDC__    Borland C++ 3.x

#if !defined(_BASETSD_H_) && !defined(_BASETSD_H)
typedef s32     INT32;       // 32 bits signed integer
typedef u32     UINT32;      // 32 bits unsigned integer
#endif
typedef s16     INT16;       // 16 bits signed integer
typedef u16     UINT16;      // 16 bits unsigned integer
typedef char    CHAR8;       // 8 bits signed integer
typedef u8      UCHAR8;      // 8 bits unsigned integer
typedef fhandle FHANDLE;     // File handle

#define _JAMFAR
#define _JAMPROC
#define _JAMDATA


typedef INT32  _JAMDATA *INT32ptr;
typedef UINT32 _JAMDATA *UINT32ptr;
typedef INT16  _JAMDATA *INT16ptr;
typedef UINT16 _JAMDATA *UINT16ptr;
typedef CHAR8  _JAMDATA *CHAR8ptr;
typedef UCHAR8 _JAMDATA *UCHAR8ptr;
typedef void   _JAMDATA *VOIDptr;


#endif // __JAMSYS_H__

#ifdef __cplusplus
}
#endif

// end of file "jamsys.h"
