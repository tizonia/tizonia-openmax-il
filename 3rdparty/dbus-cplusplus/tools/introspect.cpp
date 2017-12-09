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

#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "introspect.h"

DBus::BusDispatcher dispatcher;
static bool systembus;
static char *path;
static char *service;

void niam(int sig)
{
  DBus::Connection conn = systembus ? DBus::Connection::SystemBus() : DBus::Connection::SessionBus();

  IntrospectedObject io(conn, path, service);

  std::cout << io.Introspect();

  dispatcher.leave();
}

int main(int argc, char **argv)
{
  signal(SIGTERM, niam);
  signal(SIGINT, niam);
  signal(SIGALRM, niam);

  if (argc == 1)
  {
    std::cerr << std::endl << "Usage: " << argv[0] << " [--system] <object_path> [<destination>]" << std::endl << std::endl;
  }
  else
  {
    if (strcmp(argv[1], "--system"))
    {
      systembus = false;
      path = argv[1];
      service = argc > 2 ? argv[2] : 0;
    }
    else
    {
      systembus = true;
      path = argv[2];
      service = argc > 3 ? argv[3] : 0;
    }

    DBus::default_dispatcher = &dispatcher;

    alarm(1);

    dispatcher.enter();
  }

  return 0;
}
