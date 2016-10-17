#include "stricmp.h"

#include <ctype.h>

//---------------------------------------------------------------------------
int _fm_stricmp(const char *s1, const char *s2)
{
  char c1, c2;

  while ((c1 = (char)toupper(*s1)) == (c2 = (char)toupper(*s2)) && c1 != '\0')
    s1++, s2++;

  return c1 - c2;
}
//---------------------------------------------------------------------------
int _fm_strnicmp(const char *s1, const char *s2, long unsigned int n)
{
  while (n > 0 && *s1 != '\0' && toupper(*s1) == toupper(*s2))
    n--, s1++, s2++;

  if (0 == n)
    return 0;

  return toupper(*s1) - toupper(*s2);
}
//---------------------------------------------------------------------------
