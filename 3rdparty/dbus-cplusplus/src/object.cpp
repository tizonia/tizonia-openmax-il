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
#include <dbus-c++/object.h>
#include "internalerror.h"

#include <cstring>
#include <map>
#include <dbus/dbus.h>

#include "message_p.h"
#include "server_p.h"
#include "connection_p.h"

using namespace DBus;

Object::Object(Connection &conn, const Path &path, const char *service)
  : _conn(conn), _path(path), _service(service ? service : ""), _default_timeout(-1)
{
}

Object::~Object()
{
}

void Object::set_timeout(int new_timeout)
{
  debug_log("%s: %d millies", __PRETTY_FUNCTION__, new_timeout);
  if (new_timeout < 0 && new_timeout != -1)
    throw ErrorInvalidArgs("Bad timeout, cannot set it");
  _default_timeout = new_timeout;
}

struct ObjectAdaptor::Private
{
  static void unregister_function_stub(DBusConnection *, void *);
  static DBusHandlerResult message_function_stub(DBusConnection *, DBusMessage *, void *);
};

static DBusObjectPathVTable _vtable =
{
  ObjectAdaptor::Private::unregister_function_stub,
  ObjectAdaptor::Private::message_function_stub,
  NULL, NULL, NULL, NULL
};

void ObjectAdaptor::Private::unregister_function_stub(DBusConnection *conn, void *data)
{
  //TODO: what do we have to do here ?
}

DBusHandlerResult ObjectAdaptor::Private::message_function_stub(DBusConnection *, DBusMessage *dmsg, void *data)
{
  ObjectAdaptor *o = static_cast<ObjectAdaptor *>(data);

  if (o)
  {
    Message msg(new Message::Private(dmsg));

    debug_log("in object %s", o->path().c_str());
    debug_log(" got message #%d from %s to %s",
              msg.serial(),
              msg.sender(),
              msg.destination()
             );

    return o->handle_message(msg)
           ? DBUS_HANDLER_RESULT_HANDLED
           : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  else
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}

typedef std::map<Path, ObjectAdaptor *> ObjectAdaptorTable;
static ObjectAdaptorTable _adaptor_table;

ObjectAdaptor *ObjectAdaptor::from_path(const Path &path)
{
  ObjectAdaptorTable::iterator ati = _adaptor_table.find(path);

  if (ati != _adaptor_table.end())
    return ati->second;

  return NULL;
}

ObjectAdaptorPList ObjectAdaptor::from_path_prefix(const std::string &prefix)
{
  ObjectAdaptorPList ali;

  ObjectAdaptorTable::iterator ati = _adaptor_table.begin();

  size_t plen = prefix.length();

  while (ati != _adaptor_table.end())
  {
    if (!strncmp(ati->second->path().c_str(), prefix.c_str(), plen))
      ali.push_back(ati->second);

    ++ati;
  }

  return ali;
}

ObjectPathList ObjectAdaptor::child_nodes_from_prefix(const std::string &prefix)
{
  ObjectPathList ali;

  ObjectAdaptorTable::iterator ati = _adaptor_table.begin();

  size_t plen = prefix.length();

  while (ati != _adaptor_table.end())
  {
    if (!strncmp(ati->second->path().c_str(), prefix.c_str(), plen))
    {
      std::string p = ati->second->path().substr(plen);
      p = p.substr(0, p.find('/'));
      ali.push_back(p);
    }
    ++ati;
  }

  ali.sort();
  ali.unique();

  return ali;
}

ObjectAdaptor::ObjectAdaptor(Connection &conn, const Path &path)
  : Object(conn, path, conn.unique_name())
{
  register_obj();
}

ObjectAdaptor::~ObjectAdaptor()
{
  unregister_obj(false);
}

void ObjectAdaptor::register_obj()
{
  debug_log("registering local object %s", path().c_str());

  if (!dbus_connection_register_object_path(conn()._pvt->conn, path().c_str(), &_vtable, this))
  {
    throw ErrorNoMemory("unable to register object path");
  }

  _adaptor_table[path()] = this;
}

void ObjectAdaptor::unregister_obj(bool)
{
  _adaptor_table.erase(path());

  debug_log("unregistering local object %s", path().c_str());

  dbus_connection_unregister_object_path(conn()._pvt->conn, path().c_str());
}

void ObjectAdaptor::_emit_signal(SignalMessage &sig)
{
  sig.path(path().c_str());

  conn().send(sig);
}

struct ReturnLaterError
{
  const Tag *tag;
};

bool ObjectAdaptor::handle_message(const Message &msg)
{
  switch (msg.type())
  {
  case DBUS_MESSAGE_TYPE_METHOD_CALL:
  {
    const CallMessage &cmsg = reinterpret_cast<const CallMessage &>(msg);
    const char *member      = cmsg.member();
    const char *interface   = cmsg.interface();
    InterfaceAdaptor *ii    = NULL;

    debug_log(" invoking method %s.%s", interface, member);

    if (interface)
      ii = find_interface(interface);
    else
      ii = NULL;

    if (ii)
    {
      try
      {
        Message ret = ii->dispatch_method(cmsg);
        conn().send(ret);
      }
      catch (Error &e)
      {
        ErrorMessage em(cmsg, e.name(), e.message());
        conn().send(em);
      }
      catch (ReturnLaterError &rle)
      {
        _continuations[rle.tag] = new Continuation(conn(), cmsg, rle.tag);
      }
      return true;
    }
    else
    {
      return false;
    }
  }
  default:
  {
    return false;
  }
  }
}

void ObjectAdaptor::return_later(const Tag *tag)
{
  ReturnLaterError rle = { tag };
  throw rle;
}

void ObjectAdaptor::return_now(Continuation *ret)
{
  ret->_conn.send(ret->_return);

  ContinuationMap::iterator di = _continuations.find(ret->_tag);

  delete di->second;

  _continuations.erase(di);
}

void ObjectAdaptor::return_error(Continuation *ret, const Error error)
{
  ret->_conn.send(ErrorMessage(ret->_call, error.name(), error.message()));

  ContinuationMap::iterator di = _continuations.find(ret->_tag);

  delete di->second;

  _continuations.erase(di);
}

ObjectAdaptor::Continuation *ObjectAdaptor::find_continuation(const Tag *tag)
{
  ContinuationMap::iterator di = _continuations.find(tag);

  return di != _continuations.end() ? di->second : NULL;
}

ObjectAdaptor::Continuation::Continuation(Connection &conn, const CallMessage &call, const Tag *tag)
  : _conn(conn), _call(call), _return(_call), _tag(tag)
{
  _writer = _return.writer(); //todo: verify
}

/*
*/

ObjectProxy::ObjectProxy(Connection &conn, const Path &path, const char *service)
  : Object(conn, path, service)
{
  register_obj();
}

ObjectProxy::~ObjectProxy()
{
  unregister_obj(false);
}

void ObjectProxy::register_obj()
{
  debug_log("registering remote object %s", path().c_str());

  _filtered = new Callback<ObjectProxy, bool, const Message &>(this, &ObjectProxy::handle_message);

  conn().add_filter(_filtered);

  InterfaceProxyTable::const_iterator ii = _interfaces.begin();
  while (ii != _interfaces.end())
  {
    std::string im = "type='signal',interface='" + ii->first + "',path='" + path() + "'";
    conn().add_match(im.c_str());
    ++ii;
  }
}

void ObjectProxy::unregister_obj(bool throw_on_error)
{
  debug_log("unregistering remote object %s", path().c_str());

  InterfaceProxyTable::const_iterator ii = _interfaces.begin();
  while (ii != _interfaces.end())
  {
    std::string im = "type='signal',interface='" + ii->first + "',path='" + path() + "'";
    conn().remove_match(im.c_str(), throw_on_error);
    ++ii;
  }
  conn().remove_filter(_filtered);
}

Message ObjectProxy::_invoke_method(CallMessage &call)
{
  if (call.path() == NULL)
    call.path(path().c_str());

  if (call.destination() == NULL)
    call.destination(service().c_str());

  return conn().send_blocking(call, get_timeout());
}

bool ObjectProxy::_invoke_method_noreply(CallMessage &call)
{
  if (call.path() == NULL)
    call.path(path().c_str());

  if (call.destination() == NULL)
    call.destination(service().c_str());

  return conn().send(call);
}

bool ObjectProxy::handle_message(const Message &msg)
{
  switch (msg.type())
  {
  case DBUS_MESSAGE_TYPE_SIGNAL:
  {
    const SignalMessage &smsg = reinterpret_cast<const SignalMessage &>(msg);
    const char *interface	= smsg.interface();
    const char *member	= smsg.member();
    const char *objpath	= smsg.path();

    if (objpath != path()) return false;

    debug_log("filtered signal %s(in %s) from %s to object %s",
              member, interface, msg.sender(), objpath);

    InterfaceProxy *ii = find_interface(interface);
    if (ii)
    {
      return ii->dispatch_signal(smsg);
    }
    else
    {
      return false;
    }
  }
  default:
  {
    return false;
  }
  }
}
