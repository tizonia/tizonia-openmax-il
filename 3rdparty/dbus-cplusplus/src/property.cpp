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
#include <dbus-c++/property.h>

#include <dbus-c++/introspection.h>

using namespace DBus;

static const char *properties_name = "org.freedesktop.DBus.Properties";

PropertiesAdaptor::PropertiesAdaptor()
  : InterfaceAdaptor(properties_name)
{
  register_method(PropertiesAdaptor, Get, Get);
  register_method(PropertiesAdaptor, Set, Set);
}

Message PropertiesAdaptor::Get(const CallMessage &call)
{
  MessageIter ri = call.reader();

  std::string iface_name;
  std::string property_name;

  ri >> iface_name >> property_name;

  debug_log("requesting property %s on interface %s", property_name.c_str(), iface_name.c_str());

  InterfaceAdaptor *interface = (InterfaceAdaptor *) find_interface(iface_name);

  if (!interface)
    throw ErrorFailed("requested interface not found");

  Variant *value = interface->get_property(property_name);

  if (!value)
    throw ErrorFailed("requested property not found");

  on_get_property(*interface, property_name, *value);

  ReturnMessage reply(call);

  MessageIter wi = reply.writer();

  wi << *value;
  return reply;
}

Message PropertiesAdaptor::Set(const CallMessage &call)
{
  MessageIter ri = call.reader();

  std::string iface_name;
  std::string property_name;
  Variant value;

  ri >> iface_name >> property_name >> value;

  InterfaceAdaptor *interface = (InterfaceAdaptor *) find_interface(iface_name);

  if (!interface)
    throw ErrorFailed("requested interface not found");

  on_set_property(*interface, property_name, value);

  interface->set_property(property_name, value);

  ReturnMessage reply(call);

  return reply;
}

IntrospectedInterface *PropertiesAdaptor::introspect() const
{
  static IntrospectedArgument Get_args[] =
  {
    { "interface_name", "s", true },
    { "property_name", "s", true },
    { "value", "v", false },
    { 0, 0, 0 }
  };
  static IntrospectedArgument Set_args[] =
  {
    { "interface_name", "s", true },
    { "property_name", "s", true },
    { "value", "v", true },
    { 0, 0, 0 }
  };
  static IntrospectedMethod Properties_methods[] =
  {
    { "Get", Get_args },
    { "Set", Set_args },
    { 0, 0 }
  };
  static IntrospectedMethod Properties_signals[] =
  {
    { 0, 0 }
  };
  static IntrospectedProperty Properties_properties[] =
  {
    { 0, 0, 0, 0 }
  };
  static IntrospectedInterface Properties_interface =
  {
    properties_name,
    Properties_methods,
    Properties_signals,
    Properties_properties
  };
  return &Properties_interface;
}

PropertiesProxy::PropertiesProxy()
  : InterfaceProxy(properties_name)
{
}

Variant PropertiesProxy::Get(const std::string &iface, const std::string &property)
{
//todo
  Variant v;
  return v;
}

void PropertiesProxy::Set(const std::string &iface, const std::string &property, const Variant &value)
{
//todo
}

