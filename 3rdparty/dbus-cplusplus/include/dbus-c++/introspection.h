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


#ifndef __DBUSXX_INTROSPECTION_H
#define __DBUSXX_INTROSPECTION_H

#include "api.h"
#include "interface.h"

namespace DBus
{

struct DXXAPI IntrospectedArgument
{
  const char *name;
  const char *type;
  const bool  in;
};

struct DXXAPI IntrospectedMethod
{
  const char *name;
  const IntrospectedArgument *args;
};

struct DXXAPI IntrospectedProperty
{
  const char *name;
  const char *type;
  const bool  read;
  const bool  write;
};

struct DXXAPI IntrospectedInterface
{
  const char *name;
  const IntrospectedMethod *methods;
  const IntrospectedMethod *signals;
  const IntrospectedProperty *properties;
};

class DXXAPI IntrospectableAdaptor : public InterfaceAdaptor
{
public:

  IntrospectableAdaptor();

  Message Introspect(const CallMessage &);

protected:

  IntrospectedInterface *introspect() const;
};

class DXXAPI IntrospectableProxy : public InterfaceProxy
{
public:

  IntrospectableProxy();

  std::string Introspect();
};

} /* namespace DBus */

#endif//__DBUSXX_INTROSPECTION_H
