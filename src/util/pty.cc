/*
    Mosh: the mobile shell
    Copyright 2012 Keith Winstein

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "pty.h"

#ifndef HAVE_OPENPTY
/* from dropbear 0.53.1 sshpty.c, original license: "can be used freely for any purpose." */
int my_openpty (int *amaster, int *aslave, char *name, const struct termios *termp,
         const struct winsize *winp)
{
  int master, slave;
  char *name_slave;

  master = open("/dev/ptmx", O_RDWR);
  if (master == -1) {
//    TRACE(("Fail to open master"))
    return -1;
  }

  if (grantpt(master))
    goto fail;

  if (unlockpt(master))
    goto fail;

  name_slave = ptsname(master);
//  TRACE(("openpty: slave name %s", name_slave))
  slave = open(name_slave, O_RDWR | O_NOCTTY);
  if (slave == -1)
    {
      goto fail;
    }

  if(termp)
    tcsetattr(slave, TCSAFLUSH, termp);
  if (winp)
    ioctl (slave, TIOCSWINSZ, winp);

  *amaster = master;
  *aslave = slave;
  if (name != NULL)
    strcpy(name, name_slave);

  return 0;

 fail:
  close (master);
  return -1;
}
#endif

#ifndef HAVE_LOGIN_TTY
/* from glibc 2.14, login/login_tty.c, under the BSD license */
int login_tty(int fd)
{
        (void) setsid();
#ifdef TIOCSCTTY
        if (ioctl(fd, TIOCSCTTY, (char *)NULL) == -1)
                return (-1);
#else
        {
          /* This might work.  */
          char *fdname = ttyname (fd);
          int newfd;
          if (fdname)
            {
              if (fd != 0)
                (void) close (0);
              if (fd != 1)
                (void) close (1);
              if (fd != 2)
                (void) close (2);
              newfd = open (fdname, O_RDWR);
              (void) close (newfd);
            }
        }
#endif
        while (dup2(fd, 0) == -1 && errno == EBUSY)
          ;
        while (dup2(fd, 1) == -1 && errno == EBUSY)
          ;
        while (dup2(fd, 2) == -1 && errno == EBUSY)
          ;
        if (fd > 2)
                (void) close(fd);
        return (0);
}
#endif

int
my_forkpty (int *amaster, char *name, const struct termios *termp, const struct winsize *winp) {
#ifdef HAVE_FORKPTY
  return forkpty(amaster, name, termp, winp);
#else
/* from glibc 2.14, login/forkpty.c, under the LGPL 2.1 (or later) license */
  int master, slave, pid;

  if (openpty (&master, &slave, name, termp, winp) == -1)
    return -1;

  switch (pid = fork ())
    {
    case -1:
      close (master);
      close (slave);
      return -1;
    case 0:
      /* Child.  */
      close (master);
      if (login_tty (slave))
        _exit (1);

      return 0;
    default:
      /* Parent.  */
      *amaster = master;
      close (slave);

      return pid;
    }
#endif
}
