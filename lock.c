#include "lock.h"

#include <unistd.h>

#if defined(__MINGW32__)
#include <errno.h>
#include <stdio.h>
#include <sys\locking.h>

//---------------------------------------------------------------------------
int _lock(int fd, long offset, long length, int cmd)
{
  int t_errno = 0;
  long curpos = lseek(fd, 0, SEEK_CUR);

  if (curpos < 0)
    return -1;

  if (lseek(fd, offset, SEEK_SET) < 0)
    return -1;

  if (_locking(fd, cmd, length) != 0)
    t_errno = errno;

  if (lseek(fd, curpos, SEEK_SET) < 0 && t_errno == 0)
    t_errno = errno;

  return (errno = t_errno) != 0;
}
//---------------------------------------------------------------------------
int lock(int fd, long offset, long length)
{
  return _lock(fd, offset, length, LK_NBLCK);
}
//---------------------------------------------------------------------------
int unlock(int fd, long offset, long length)
{
  return _lock(fd, offset, length, LK_UNLCK);
}
#endif // __MINGW32__
//---------------------------------------------------------------------------
#ifdef __linux__
#include <fcntl.h>

int lock(int fd, long offset, long length)
{
  struct flock fl;
  int rv;

  if ((rv = fcntl(fd, F_GETFL)) == -1)
    return rv;

  fl.l_type   = (rv & O_ACCMODE) ? F_WRLCK : F_RDLCK;
  fl.l_start  = offset;
  fl.l_len    = length;
  fl.l_whence = SEEK_SET;

  return fcntl(fd, F_SETLK, &fl);
}
//---------------------------------------------------------------------------
int unlock(int fd, long offset, long length)
{
  struct flock fl;

  fl.l_type   = F_UNLCK;
  fl.l_start  = offset;
  fl.l_len    = length;
  fl.l_whence = SEEK_SET;

  return fcntl(fd, F_SETLK, &fl);
}
#endif // __linux__
//---------------------------------------------------------------------------
