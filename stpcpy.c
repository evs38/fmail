#ifdef __MINGW32__
inline char *stpcpy(char *dst, const char *src)
{
  while (*dst = *src)
    ++dst, ++src;

  return dst;
}
#endif

inline char *stpncpy(char *dst, const char *src, int n)
{
  while (n-- > 0  && (*dst = *src))
    ++dst, ++src;

  return dst;
}
