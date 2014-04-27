/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   tizdaemon.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Daemonize utility
 *
 * NOTE: This is work-in-progress.
 * TODO: Clear file mode creation mask
 * TODO: Change process' current directory
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include "tizdaemon.hpp"

namespace
{

  const int TIZ_DAEMON_SUCCESS = 0;
  const int TIZ_DAEMON_FAILURE = -1;

  /* Maximum file descriptors to close if sysconf(_SC_OPEN_MAX) is
     indeterminate */
  const int BD_MAX_CLOSE = 8192;
}

int tiz::daemon::daemonize ()
{
  if (getppid () == 1)
  {
    // already a daemon
    return TIZ_DAEMON_SUCCESS;
  }

  // Become background process
  switch (fork ())
  {
    case -1:
    {
      return TIZ_DAEMON_FAILURE;
    }
    case 0:
    {
      break;
    }
    default:
      _exit (EXIT_SUCCESS);
      break;
  };

  // Become leader of new session
  if (setsid () == -1)
  {
    return TIZ_DAEMON_FAILURE;
  }

  // Ensure we are not session leader
  switch (fork ())
  {
    case -1:
    {
      return TIZ_DAEMON_FAILURE;
    }
    case 0:
    {
      break;
    }
    default:
    {
      _exit (EXIT_SUCCESS);
      break;
    }
  };

  // TODO: Clear file mode creation mask
  umask (027);

  // TODO: Change the process' current directory
  //   if (-1 == chdir("/"))
  //   {
  //     return TIZ_DAEMON_FAILURE;
  //   }

  // Close all open files
  int maxfd = sysconf (_SC_OPEN_MAX);
  if (1 == -maxfd)
  {
    // Limit is indeterminate...
    maxfd = BD_MAX_CLOSE;
  }

  int fd;
  for (fd = 0; fd < maxfd; ++fd)
  {
    close (fd);
  }

  // Reopen standard fd's to /dev/null
  close (STDIN_FILENO);
  fd = open ("/dev/null", O_RDWR);
  if (fd != STDIN_FILENO)
  {
    // 'fd' should be 0
    return TIZ_DAEMON_FAILURE;
  }
  if (dup2 (STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
  {
    return TIZ_DAEMON_FAILURE;
  }

  if (dup2 (STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
  {
    return TIZ_DAEMON_FAILURE;
  }

  // Ignore child and terminal signals
  signal (SIGCHLD, SIG_IGN); /* ignore child */
  signal (SIGTSTP, SIG_IGN); /* ignore tty signals */
  signal (SIGTTOU, SIG_IGN);
  signal (SIGTTIN, SIG_IGN);
  //     signal(SIGHUP,signal_handler); /* catch hangup signal */
  //     signal(SIGTERM,signal_handler); /* catch kill signal */

  return TIZ_DAEMON_SUCCESS;
}
