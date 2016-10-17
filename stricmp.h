#ifndef stricmpH
#define stricmpH

#if defined(__MINGW32__) || defined(_LINUX)

int _fm_stricmp (const char *s1, const char *s2);
int _fm_strnicmp(const char *s1, const char *s2, long unsigned int n);

#define stricmp(a,b)    _fm_stricmp(a,b)
#define strnicmp(a,b,c) _fm_strnicmp(a,b,c)

#endif
#endif  // stricmpH
