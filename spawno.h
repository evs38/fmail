/********************************************************************/
/*   SPAWNO v3.0   EMS/XMS/disk swapping replacement for spawn...() */
/*   (c) Copyright 1990 Ralf Brown  All Rights Reserved 	    */
/*								    */
/*   May be freely copied provided that this copyright notice is    */
/*   not altered or removed.					    */
/********************************************************************/

#ifndef __SPAWNO
#define __SPAWNO

#include <stdarg.h>

#ifdef M_I86	      /* MSC 5.x */
#  define _Cdecl cdecl
#endif /* M_I86 */

int _Cdecl spawnvo(const char *overlay_path, const char *name, va_list args) ;
int _Cdecl spawnvpo(const char *overlay_path, const char *name, va_list args) ;
int _Cdecl spawnveo(const char *overlay_path, const char *name, va_list args,const char **env) ;
int _Cdecl spawnvpeo(const char *overlay_path, const char *name, va_list args,const char **env) ;
int _Cdecl spawnlo(const char *overlay_path, const char *name, ...) ;
int _Cdecl spawnlpo(const char *overlay_path, const char *name, ...) ;
int _Cdecl spawnleo(const char *overlay_path, const char *name, ...) ;
int _Cdecl spawnlpeo(const char *overlay_path, const char *name, ...) ;

/* this function is normally called only by the spawn...o() functions */
int pascal __spawnv(const char *overlay_path,const char *name,va_list args,int env) ;

  /* The following variable determines whether SPAWNO is allowed to use XMS */
  /* memory if available.  Set to 0 to disable XMS, 1 (default) to enable   */
extern char _Cdecl __spawn_xms ;

  /* The next variable determines whether SPAWNO is allowed to use EMS	    */
  /* memory if available and XMS is either unavailable or disabled.  Set to */
  /* 0 to disable EMS, 1 (default) to enable.				    */
extern char _Cdecl __spawn_ems ;

  /* The last variable specifies the number of paragraphs to keep resident  */
  /* while swapped out (default 0 means minimum possible)		    */
extern unsigned int _Cdecl __spawn_resident ;

/********************************************************************/
/* You may define REPLACE_SPAWN before including this header file   */
/* in order to allow existing code to call the new functions.  You  */
/* should also define OVERLAY_PATH to a string literal or char*     */
/* expression indicating where to store the swap file.		    */

#ifdef REPLACE_SPAWN

#ifdef P_WAIT
#  undef P_WAIT
#endif
#ifdef P_OVERLAY
#  undef P_OVERLAY
#endif

#ifndef OVERLAY_PATH
/* use the root directory of the current drive if no path defined */
#  define OVERLAY_PATH "/"
#endif

#define spawnl	  spawnlo
#define spawnlp   spawnlpo
#define spawnle   spawnleo
#define spawnlpe  spawnlpeo
#define spawnv	  spawnvo
#define spawnvp   spawnvpo
#define spawnve   spawnveo
#define spawnvpe  spawnvpeo
#define P_WAIT    OVERLAY_PATH
#define P_OVERLAY OVERLAY_PATH
/* Note: as redefined here, P_OVERLAY has different semantics than the normal */
/*	 library function, which never returns on success */

#endif /* REPLACE_SPAWN */

#endif /* __SPAWNO */
