#ifndef lockH
#define lockH

#ifdef __MINGW32__

int lock  (int handle, long offset, long length);
int unlock(int handle, long offset, long length);

#endif // __MINGW32__
#endif // lockH
