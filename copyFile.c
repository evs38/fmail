#ifdef __linux__
#include "copyFile.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//---------------------------------------------------------------------------
// Returns 0 on fail to be compatibel with Win CopyFile()
//
int copyFile(const char *src, const char *dst, int failIfExists)
{
  int in_fd = -1
    , ou_fd = -1
    , t_errno;

  if ((in_fd = open(src, O_RDONLY)) >= 0)
  {
    // Advise the OS, so it can do optimal read ahead
    if ((errno = posix_fadvise(in_fd, 0, 0, POSIX_FADV_SEQUENTIAL)) == 0)
    {
      struct stat statBuf;

      if (fstat(in_fd, &statBuf) == 0)
      {
        int oflag = O_CREAT | O_WRONLY | O_TRUNC;

        if (failIfExists)
          oflag |= O_EXCL;

        // Open the output file for writing, with the same permissions as the source file.
        if ((ou_fd = open(dst, oflag, statBuf.st_mode)) >= 0)
        {
          // Pre allocate storage space to prevent fragmentation
          if ((errno = posix_fallocate(ou_fd, 0, statBuf.st_size)) == 0)
          {
            // sendfile() copies  data between one file descriptor and another.
            // Because this copying is done within the kernel, sendfile() is more
            // efficient than the combination of read(2) and write(2), which would
            // require transferring data to and from user space.

            ssize_t r = 0;
            off_t offset = 0;
            size_t count = statBuf.st_size;
            while (count > 0)
              if ((r = sendfile(ou_fd, in_fd, &offset, count)) > 0)
              {
                count -= r;
                offset += r;
              }
              else
                break;

            // Applications may wish to fall back to read(2)/write(2)
            // in the case where sendfile() fails with EINVAL or ENOSYS
            if (r < 0 && (errno == EINVAL || errno == ENOSYS))
            {
              char buf[0x2000];
              while ((r = read(in_fd, buf, sizeof(buf))) > 0)
              {
                // Allow for partial writes - see Advanced Unix Programming (2nd Ed.),
                // Marc Rochkind, Addison-Wesley, 2004, page 94
                offset = 0;
                count  = r;
                while (count > 0)
                  if ((r = write(ou_fd, buf + offset, count)) > 0)
                  {
                    count -= r;
                    offset += r;
                  }
                  else
                    break;
                if (r < 0)
                  break;
              }
            }
          }
        }
      }
    }
  }

  t_errno = errno;
  if (ou_fd >= 0) close(ou_fd);
  if (in_fd >= 0) close(in_fd);
  errno = t_errno;

  return errno == 0;
}
#endif
