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

#ifndef DBUSXX_PIPE_H
#define DBUSXX_PIPE_H

/* Project */
#include "api.h"

/* STD */
#include <cstdlib>
#include <sys/types.h>

namespace Tiz { namespace DBus
{

class DXXAPI Pipe
{
public:
  /*!
   * Write some data into the communication pipe.
   *
   * @param buffer The raw data to write.
   * @param nbytes The number of bytes to write from the buffer.
   */
  void write(const void *buffer, unsigned int nbytes);

  ssize_t read(void *buffer, unsigned int &nbytes);

  /*!
   * Simply write one single byte into the pipe. This is a shortcut
   * if there's really no data to transport, but to activate the handler.
   */
  void signal();

private:
  void(*_handler)(const void *data, void *buffer, unsigned int nbyte);
  int _fd_write;
  int _fd_read;
  const void *_data;

  // allow construction only in BusDispatcher
  Pipe(void(*handler)(const void *data, void *buffer, unsigned int nbyte), const void *data);
  ~Pipe() {};

  friend class BusDispatcher;
};

} /* namespace DBus */ } /* namespace Tiz */

#endif // DBUSXX_PIPE_H
