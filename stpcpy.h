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
#endif
