#ifndef lockH
#define lockH

#if defined(__MINGW32__) || defined(__linux__)

int lock  (int fd, long offset, long length);
int unlock(int fd, long offset, long length);

#else // defined(__MINGW32__) || defined(__linux__)
#include <sys\locking.h>
#endif
#endif // lockH
