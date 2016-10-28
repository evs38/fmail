#ifndef copyFileH
#define copyFileH

#ifdef __WIN32__
#include <windows.h>

#define copyFile(src, dst, fail)  CopyFile(src, dst, fail)
#endif

#ifdef __linux__
int copyFile(const char *src, const char *dst, int failIfExists);
#endif

#endif
