#include "lock.h"

#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <sys\locking.h>

//---------------------------------------------------------------------------
int _lock(int handle, long offset, long length, int cmd)
{
  int t_errno = 0;
  long curpos = lseek(handle, 0, SEEK_CUR);

  if (curpos < 0)
    return -1;

  if (lseek(handle, offset, SEEK_SET) < 0)
    return -1;

  if (_locking(handle, cmd, length) != 0)
    t_errno = errno;

  if (lseek(handle, curpos, SEEK_SET) < 0 && t_errno == 0)
    t_errno = errno;

  return (errno = t_errno) != 0;
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
