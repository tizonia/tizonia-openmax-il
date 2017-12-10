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
#include <dbus-c++/connection.h>

#include <dbus/dbus.h>
#include <string>

#include "internalerror.h"

#include "connection_p.h"
#include "dispatcher_p.h"
#include "server_p.h"
#include "message_p.h"
#include "pendingcall_p.h"

using namespace DBus;

Connection::Private::Private(DBusConnection *c, Server::Private *s)
  : conn(c) , dispatcher(NULL), server(s)
{
  init();
}

Connection::Private::Private(DBusBusType type)
  : dispatcher(NULL), server(NULL)
{
  InternalError e;

  conn = dbus_bus_get_private(type, e);

  if (e) throw Error(e);

  init();
}

Connection::Private::~Private()
{
  debug_log("terminating connection 0x%08x", conn);

  detach_server();

  if (dbus_connection_get_is_connected(conn))
  {
    std::vector<std::string>::iterator i = names.begin();

    while (i != names.end())
    {
      debug_log("%s: releasing bus name %s", dbus_bus_get_unique_name(conn), i->c_str());
      dbus_bus_release_name(conn, i->c_str(), NULL);
      ++i;
    }
    dbus_connection_close(conn);
  }
  dbus_connection_unref(conn);
}

void Connection::Private::init()
{
  dbus_connection_ref(conn);
  dbus_connection_ref(conn);	//todo: the library has to own another reference

  disconn_filter = new Callback<Connection::Private, bool, const Message &>(
    this, &Connection::Private::disconn_filter_function
  );

  dbus_connection_add_filter(conn, message_filter_stub, &disconn_filter, NULL); // TODO: some assert at least

  dbus_connection_set_dispatch_status_function(conn, dispatch_status_stub, this, 0);
  dbus_connection_set_exit_on_disconnect(conn, false); //why was this set to true??
}

void Connection::Private::detach_server()
{
  /*	Server::Private *tmp = server;

  	server = NULL;

  	if (tmp)
  	{
  		ConnectionList::iterator i;

  		for (i = tmp->connections.begin(); i != tmp->connections.end(); ++i)
  		{
  			if (i->_pvt.get() == this)
  			{
  				tmp->connections.erase(i);
  				break;
  			}
  		}
  	}*/
}

bool Connection::Private::do_dispatch()
{
  debug_log("dispatching on %p", conn);

  if (!dbus_connection_get_is_connected(conn))
  {
    debug_log("connection terminated");

    detach_server();

    return true;
  }

  return dbus_connection_dispatch(conn) != DBUS_DISPATCH_DATA_REMAINS;
}

void Connection::Private::dispatch_status_stub(DBusConnection *dc, DBusDispatchStatus status, void *data)
{
  Private *p = static_cast<Private *>(data);

  switch (status)
  {
  case DBUS_DISPATCH_DATA_REMAINS:
    debug_log("some dispatching to do on %p", dc);
    p->dispatcher->queue_connection(p);
    break;

  case DBUS_DISPATCH_COMPLETE:
    debug_log("all dispatching done on %p", dc);
    break;

  case DBUS_DISPATCH_NEED_MEMORY: //uh oh...
    debug_log("connection %p needs memory", dc);
    break;
  }
}

DBusHandlerResult Connection::Private::message_filter_stub(DBusConnection *conn, DBusMessage *dmsg, void *data)
{
  MessageSlot *slot = static_cast<MessageSlot *>(data);

  Message msg = Message(new Message::Private(dmsg));

  return slot && !slot->empty() && slot->call(msg)
         ? DBUS_HANDLER_RESULT_HANDLED
         : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool Connection::Private::disconn_filter_function(const Message &msg)
{
  if (msg.is_signal(DBUS_INTERFACE_LOCAL, "Disconnected"))
  {
    debug_log("%p disconnected by local bus", conn);
    dbus_connection_close(conn);

    return true;
  }
  return false;
}

DBusDispatchStatus Connection::Private::dispatch_status()
{
  return dbus_connection_get_dispatch_status(conn);
}

bool Connection::Private::has_something_to_dispatch()
{
  return dispatch_status() == DBUS_DISPATCH_DATA_REMAINS;
}


Connection Connection::SystemBus()
{
  return Connection(new Private(DBUS_BUS_SYSTEM));
}

Connection Connection::SessionBus()
{
  return Connection(new Private(DBUS_BUS_SESSION));
}

Connection Connection::ActivationBus()
{
  return Connection(new Private(DBUS_BUS_STARTER));
}

Connection::Connection(const char *address, bool priv)
  : _timeout(-1)
{
  InternalError e;
  DBusConnection *conn = priv
                         ? dbus_connection_open_private(address, e)
                         : dbus_connection_open(address, e);

  if (e) throw Error(e);

  _pvt = new Private(conn);

  setup(default_dispatcher);

  debug_log("connected to %s", address);
}

Connection::Connection(Connection::Private *p)
  : _pvt(p), _timeout(-1)
{
  setup(default_dispatcher);
}

Connection::Connection(const Connection &c)
  : _pvt(c._pvt), _timeout(c._timeout)
{
  dbus_connection_ref(_pvt->conn);
}

Connection::~Connection()
{
  dbus_connection_unref(_pvt->conn);
}

Dispatcher *Connection::setup(Dispatcher *dispatcher)
{
  debug_log("registering stubs for connection %p", _pvt->conn);

  if (!dispatcher) dispatcher = default_dispatcher;

  if (!dispatcher) throw ErrorFailed("no default dispatcher set for new connection");

  Dispatcher *prev = _pvt->dispatcher;

  _pvt->dispatcher = dispatcher;

  dispatcher->queue_connection(_pvt.get());

  dbus_connection_set_watch_functions(
    _pvt->conn,
    Dispatcher::Private::on_add_watch,
    Dispatcher::Private::on_rem_watch,
    Dispatcher::Private::on_toggle_watch,
    dispatcher,
    0
  );

  dbus_connection_set_timeout_functions(
    _pvt->conn,
    Dispatcher::Private::on_add_timeout,
    Dispatcher::Private::on_rem_timeout,
    Dispatcher::Private::on_toggle_timeout,
    dispatcher,
    0
  );

  return prev;
}

bool Connection::operator == (const Connection &c) const
{
  return _pvt->conn == c._pvt->conn;
}

bool Connection::register_bus()
{
  InternalError e;

  bool r = dbus_bus_register(_pvt->conn, e);

  if (e) throw(e);

  return r;
}

bool Connection::connected() const
{
  return dbus_connection_get_is_connected(_pvt->conn);
}

void Connection::disconnect()
{
//	dbus_connection_disconnect(_pvt->conn); // disappeared in 0.9x
  dbus_connection_close(_pvt->conn);
}

void Connection::exit_on_disconnect(bool exit)
{
  dbus_connection_set_exit_on_disconnect(_pvt->conn, exit);
}

bool Connection::unique_name(const char *n)
{
  return dbus_bus_set_unique_name(_pvt->conn, n);
}

const char *Connection::unique_name() const
{
  return dbus_bus_get_unique_name(_pvt->conn);
}

void Connection::flush()
{
  dbus_connection_flush(_pvt->conn);
}

void Connection::add_match(const char *rule)
{
  InternalError e;

  dbus_bus_add_match(_pvt->conn, rule, e);

  debug_log("%s: added match rule %s", unique_name(), rule);

  if (e) throw Error(e);
}

void Connection::remove_match(const char	*rule,
                              bool		throw_on_error)
{
  InternalError e;

  dbus_bus_remove_match(_pvt->conn, rule, e);

  debug_log("%s: removed match rule %s", unique_name(), rule);

  if (e)
  {
    if (throw_on_error)
      throw Error(e);
    else
      debug_log("DBus::Connection::remove_match: %s (%s).",
                static_cast<DBusError *>(e)->message,
                static_cast<DBusError *>(e)->name);
  }
}

bool Connection::add_filter(MessageSlot &s)
{
  debug_log("%s: adding filter", unique_name());
  return dbus_connection_add_filter(_pvt->conn, Private::message_filter_stub, &s, NULL);
}

void Connection::remove_filter(MessageSlot &s)
{
  debug_log("%s: removing filter", unique_name());
  dbus_connection_remove_filter(_pvt->conn, Private::message_filter_stub, &s);
}

bool Connection::send(const Message &msg, unsigned int *serial)
{
  return dbus_connection_send(_pvt->conn, msg._pvt->msg, serial);
}

Message Connection::send_blocking(Message &msg, int timeout)
{
  DBusMessage *reply;
  InternalError e;

  if (this->_timeout != -1)
  {
    reply = dbus_connection_send_with_reply_and_block(_pvt->conn, msg._pvt->msg, this->_timeout, e);
  }
  else
  {
    reply = dbus_connection_send_with_reply_and_block(_pvt->conn, msg._pvt->msg, timeout, e);
  }

  if (e) throw Error(e);

  return Message(new Message::Private(reply), false);
}

PendingCall Connection::send_async(Message &msg, int timeout)
{
  DBusPendingCall *pending;

  if (!dbus_connection_send_with_reply(_pvt->conn, msg._pvt->msg, &pending, timeout))
  {
    throw ErrorNoMemory("Unable to start asynchronous call");
  }
  return PendingCall(new PendingCall::Private(pending));
}

void Connection::request_name(const char *name, int flags)
{
  InternalError e;

  debug_log("%s: registering bus name %s", unique_name(), name);

  /*
   * TODO:
   * Think about giving back the 'ret' value. Some people on the list
   * requested about this...
   */
  int ret = dbus_bus_request_name(_pvt->conn, name, flags, e);

  if (ret == -1)
  {
    if (e) throw Error(e);
  }

//	this->remove_match("destination");

  if (name)
  {
    _pvt->names.push_back(name);
    std::string match = "destination='" + _pvt->names.back() + "'";
    add_match(match.c_str());
  }
}

unsigned long Connection::sender_unix_uid(const char *sender)
{
  InternalError e;

  unsigned long ul = dbus_bus_get_unix_user(_pvt->conn, sender, e);

  if (e) throw Error(e);

  return ul;
}

bool Connection::has_name(const char *name)
{
  InternalError e;

  bool b = dbus_bus_name_has_owner(_pvt->conn, name, e);

  if (e) throw Error(e);

  return b;
}

const std::vector<std::string>& Connection::names()
{
  return _pvt->names;
}

bool Connection::start_service(const char *name, unsigned long flags)
{
  InternalError e;

  bool b = dbus_bus_start_service_by_name(_pvt->conn, name, flags, NULL, e);

  if (e) throw Error(e);

  return b;
}

void Connection::set_timeout(int timeout)
{
  _timeout = timeout;
}

int Connection::get_timeout()
{
  return _timeout;
}

