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

#include <dbus-c++/introspection.h>
#include <dbus-c++/object.h>
#include <dbus-c++/message.h>

#include <dbus/dbus.h>

#include <sstream>

using namespace DBus;

static const char *introspectable_name = "org.freedesktop.DBus.Introspectable";

IntrospectableAdaptor::IntrospectableAdaptor()
  : InterfaceAdaptor(introspectable_name)
{
  register_method(IntrospectableAdaptor, Introspect, Introspect);
}

Message IntrospectableAdaptor::Introspect(const CallMessage &call)
{
  debug_log("requested introspection data");

  std::ostringstream xml;

  xml << DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE;

  const std::string path = object()->path();

  xml << "<node name=\"" << path << "\">";

  InterfaceAdaptorTable::const_iterator iti;

  for (iti = _interfaces.begin(); iti != _interfaces.end(); ++iti)
  {
    debug_log("introspecting interface %s", iti->first.c_str());

    IntrospectedInterface *const intro = iti->second->introspect();
    if (intro)
    {
      xml << "\n\t<interface name=\"" << intro->name << "\">";

      for (const IntrospectedProperty *p = intro->properties; p->name; ++p)
      {
        std::string access;

        if (p->read)  access += "read";
        if (p->write) access += "write";

        xml << "\n\t\t<property name=\"" << p->name << "\""
            << " type=\"" << p->type << "\""
            << " access=\"" << access << "\"/>";
      }

      for (const IntrospectedMethod *m = intro->methods; m->args; ++m)
      {
        xml << "\n\t\t<method name=\"" << m->name << "\">";

        for (const IntrospectedArgument *a = m->args; a->type; ++a)
        {
          xml << "\n\t\t\t<arg direction=\"" << (a->in ? "in" : "out") << "\""
              << " type=\"" << a->type << "\"";

          if (a->name) xml << " name=\"" << a->name << "\"";

          xml << "/>";
        }

        xml << "\n\t\t</method>";
      }

      for (const IntrospectedMethod *m = intro->signals; m->args; ++m)
      {
        xml << "\n\t\t<signal name=\"" << m->name << "\">";

        for (const IntrospectedArgument *a = m->args; a->type; ++a)
        {
          xml << "<arg type=\"" << a->type << "\"";

          if (a->name) xml << " name=\"" << a->name << "\"";

          xml << "/>";
        }
        xml << "\n\t\t</signal>";
      }

      xml << "\n\t</interface>";
    }
  }

  const ObjectPathList nodes = ObjectAdaptor::child_nodes_from_prefix(path + '/');
  ObjectPathList::const_iterator oni;

  for (oni = nodes.begin(); oni != nodes.end(); ++oni)
  {
    xml << "\n\t<node name=\"" << (*oni) << "\"/>";
  }

  /* broken
  const ObjectAdaptorPList children = ObjectAdaptor::from_path_prefix(path + '/');

  ObjectAdaptorPList::const_iterator oci;

  for (oci = children.begin(); oci != children.end(); ++oci)
  {
  	std::string name = (*oci)->path().substr(path.length()+1);
  	name.substr(name.find('/'));

  	xml << "<node name=\"" << name << "\"/>";
  }
  */

  xml << "\n</node>";

  ReturnMessage reply(call);
  MessageIter wi = reply.writer();
  wi.append_string(xml.str().c_str());
  return reply;
}

IntrospectedInterface *IntrospectableAdaptor::introspect() const
{
  static IntrospectedArgument Introspect_args[] =
  {
    { "data", "s", false },
    { 0, 0, 0 }
  };
  static IntrospectedMethod Introspectable_methods[] =
  {
    { "Introspect", Introspect_args },
    { 0, 0 }
  };
  static IntrospectedMethod Introspectable_signals[] =
  {
    { 0, 0 }
  };
  static IntrospectedProperty Introspectable_properties[] =
  {
    { 0, 0, 0, 0 }
  };
  static IntrospectedInterface Introspectable_interface =
  {
    introspectable_name,
    Introspectable_methods,
    Introspectable_signals,
    Introspectable_properties
  };
  return &Introspectable_interface;
}

IntrospectableProxy::IntrospectableProxy()
  : InterfaceProxy(introspectable_name)
{}

std::string IntrospectableProxy::Introspect()
{
  DBus::CallMessage call;

  call.member("Introspect");

  DBus::Message ret = invoke_method(call);

  DBus::MessageIter ri = ret.reader();
  const char *str = ri.get_string();

  return str;
}
