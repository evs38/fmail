#include "lock.h"

#include <io.h>
#include <stdio.h>
#include <sys\locking.h>

//---------------------------------------------------------------------------
int _lock(int handle, long offset, long length, int cmd)
{
  int rc;
  long curpos = lseek(handle, 0, SEEK_CUR);
  if (curpos < 0)
    return -1;

  if (lseek(handle, offset, SEEK_SET) < 0)
    return -1;

  rc = _locking(handle, cmd, length);

  lseek(handle, curpos, SEEK_SET);

  return rc;
}
//---------------------------------------------------------------------------
int lock(int handle, long offset, long length)
{
  return _lock(handle, offset, length, LK_NBLCK);
}
//---------------------------------------------------------------------------
int unlock(int handle, long offset, long length)
{
  return _lock(handle, offset, length, LK_UNLCK);
}
//---------------------------------------------------------------------------
