/*
 *
 *  D-Bus++ - C++ bindings for D-Bus
 *
 *  Copyright (C) 2005-2007  Paolo Durante <shackan@gmail.com>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Project */
#include <dbus-c++/pipe.h>
#include <dbus-c++/util.h>
#include <dbus-c++/error.h>

/* STD */
#include <unistd.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <cassert>

using namespace DBus;
using namespace std;

Pipe::Pipe(void(*handler)(const void *data, void *buffer, unsigned int nbyte), const void *data) :
  _handler(handler),
  _fd_write(0),
  _fd_read(0),
  _data(data)
{
  int fd[2];

  if (pipe(fd) == 0)
  {
    _fd_read = fd[0];
    _fd_write = fd[1];
    fcntl(_fd_read, F_SETFL, O_NONBLOCK);
  }
  else
  {
    throw Error("PipeError:errno", toString(errno).c_str());
  }
}

void Pipe::write(const void *buffer, unsigned int nbytes)
{
  // TODO: ignoring return of read/write generates warning; maybe relevant for eventloop work...
  // first write the size into the pipe...
  ::write(_fd_write, static_cast <const void *>(&nbytes), sizeof(nbytes));

  // ...then write the real data
  ::write(_fd_write, buffer, nbytes);
}

ssize_t Pipe::read(void *buffer, unsigned int &nbytes)
{
  // TODO: ignoring return of read/write generates warning; maybe relevant for eventloop work...
  // first read the size from the pipe...
  ::read(_fd_read, &nbytes, sizeof(nbytes));

  //ssize_t size = 0;
  return ::read(_fd_read, buffer, nbytes);
}

void Pipe::signal()
{
  // TODO: ignoring return of read/write generates warning; maybe relevant for eventloop work...
  char nullc[1] = {'\0'};
  ::write(_fd_write, nullc, 1);
}
