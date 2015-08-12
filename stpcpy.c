#ifdef __MINGW32__
inline char *stpcpy(char *dst, const char *src)
{
  while (*dst = *src) ++dst, ++src;

  return dst;
}
#endif
