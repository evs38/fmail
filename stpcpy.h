#ifndef stpcpyH
#define stpcpyH

#ifdef __MINGW32__

static inline char *stpcpy(char *dst, const char *src)
{
  while ((*dst = *src))
    ++dst, ++src;

  return dst;
}

#endif

#pragma warn -8060
#pragma warn -8027

__inline static char *stpncpy(char *dst, const char *src, int n)
{
  while (n-- > 0  && (*dst = *src))
    ++dst, ++src;

  return dst;
}

#endif
