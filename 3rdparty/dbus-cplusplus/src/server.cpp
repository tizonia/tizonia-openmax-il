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

#include <dbus-c++/debug.h>
#include <dbus-c++/server.h>

#include "internalerror.h"
#include "server_p.h"
#include "connection_p.h"
#include "dispatcher_p.h"

using namespace DBus;

Server::Private::Private(DBusServer *s)
  : server(s)
{
}

Server::Private::~Private()
{
}

void Server::Private::on_new_conn_cb(DBusServer *server, DBusConnection *conn, void *data)
{
  Server *s = static_cast<Server *>(data);

  Connection nc(new Connection::Private(conn, s->_pvt.get()));

  s->_pvt->connections.push_back(nc);

  s->on_new_connection(nc);

  debug_log("incoming connection 0x%08x", conn);
}

Server::Server(const char *address)
{
  InternalError e;
  DBusServer *server = dbus_server_listen(address, e);

  if (e) throw Error(e);

  debug_log("server 0x%08x listening on %s", server, address);

  _pvt = new Private(server);

  dbus_server_set_new_connection_function(_pvt->server, Private::on_new_conn_cb, this, NULL);

  setup(default_dispatcher);
}
/*
Server::Server(const Server &s)
: _pvt(s._pvt)
{
	dbus_server_ref(_pvt->server);
}
*/
Server::~Server()
{
  dbus_server_unref(_pvt->server);
}

Dispatcher *Server::setup(Dispatcher *dispatcher)
{
  debug_log("registering stubs for server %p", _pvt->server);

  Dispatcher *prev = _pvt->dispatcher;

  dbus_server_set_watch_functions(
    _pvt->server,
    Dispatcher::Private::on_add_watch,
    Dispatcher::Private::on_rem_watch,
    Dispatcher::Private::on_toggle_watch,
    dispatcher,
    0
  );

  dbus_server_set_timeout_functions(
    _pvt->server,
    Dispatcher::Private::on_add_timeout,
    Dispatcher::Private::on_rem_timeout,
    Dispatcher::Private::on_toggle_timeout,
    dispatcher,
    0
  );

  _pvt->dispatcher = dispatcher;

  return prev;
}

bool Server::operator == (const Server &s) const
{
  return _pvt->server == s._pvt->server;
}

bool Server::listening() const
{
  return dbus_server_get_is_connected(_pvt->server);
}
void Server::disconnect()
{
  dbus_server_disconnect(_pvt->server);
}

