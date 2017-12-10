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
#include <dbus-c++/interface.h>

#include "internalerror.h"

using namespace DBus;

Interface::Interface(const std::string &name)
  : _name(name)
{}

Interface::~Interface()
{}

InterfaceAdaptor *AdaptorBase::find_interface(const std::string &name)
{
  InterfaceAdaptorTable::const_iterator ii = _interfaces.find(name);

  return ii != _interfaces.end() ? ii->second : NULL;
}

InterfaceAdaptor::InterfaceAdaptor(const std::string &name)
  : Interface(name)
{
  debug_log("adding interface %s", name.c_str());

  _interfaces[name] = this;
}

Message InterfaceAdaptor::dispatch_method(const CallMessage &msg)
{
  const char *name = msg.member();

  MethodTable::iterator mi = _methods.find(name);
  if (mi != _methods.end())
  {
    return mi->second.call(msg);
  }
  else
  {
    return ErrorMessage(msg, DBUS_ERROR_UNKNOWN_METHOD, name);
  }
}

void InterfaceAdaptor::emit_signal(const SignalMessage &sig)
{
  SignalMessage &sig2 = const_cast<SignalMessage &>(sig);

  if (sig2.interface() == NULL)
    sig2.interface(name().c_str());

  _emit_signal(sig2);
}

Variant *InterfaceAdaptor::get_property(const std::string &name)
{
  PropertyTable::iterator pti = _properties.find(name);

  if (pti != _properties.end())
  {
    if (!pti->second.read)
      throw ErrorAccessDenied("property is not readable");

    return &(pti->second.value);
  }
  return NULL;
}

void InterfaceAdaptor::set_property(const std::string &name, Variant &value)
{
  PropertyTable::iterator pti = _properties.find(name);

  if (pti != _properties.end())
  {
    if (!pti->second.write)
      throw ErrorAccessDenied("property is not writeable");

    Signature sig = value.signature();

    if (pti->second.sig != sig)
      throw ErrorInvalidSignature("property expects a different type");

    pti->second.value = value;
    return;
  }
  throw ErrorFailed("requested property not found");
}

InterfaceProxy *ProxyBase::find_interface(const std::string &name)
{
  InterfaceProxyTable::const_iterator ii = _interfaces.find(name);

  return ii != _interfaces.end() ? ii->second : NULL;
}

InterfaceProxy::InterfaceProxy(const std::string &name)
  : Interface(name)
{
  debug_log("adding interface %s", name.c_str());

  _interfaces[name] = this;
}

bool InterfaceProxy::dispatch_signal(const SignalMessage &msg)
{
  const char *name = msg.member();

  SignalTable::iterator si = _signals.find(name);
  if (si != _signals.end())
  {
    si->second.call(msg);
    // Here we always return false because there might be
    // another InterfaceProxy listening for the same signal.
    // This way we instruct libdbus-1 to go on dispatching
    // the signal.
    return false;
  }
  else
  {
    return false;
  }
}

Message InterfaceProxy::invoke_method(const CallMessage &call)
{
  CallMessage &call2 = const_cast<CallMessage &>(call);

  if (call.interface() == NULL)
    call2.interface(name().c_str());

  return _invoke_method(call2);
}

bool InterfaceProxy::invoke_method_noreply(const CallMessage &call)
{
  CallMessage &call2 = const_cast<CallMessage &>(call);

  if (call.interface() == NULL)
    call2.interface(name().c_str());

  return _invoke_method_noreply(call2);
}
