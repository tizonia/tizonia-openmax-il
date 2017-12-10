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

#include <dbus-c++/message.h>
#include <dbus-c++/error.h>

#include <dbus/dbus.h>

#include "message_p.h"
#include "internalerror.h"

using namespace DBus;

/*
*/

Error::Error()
  : _int(new InternalError)
{}

Error::Error(InternalError &i)
  : _int(new InternalError(i))
{}

Error::Error(const char *name, const char *message)
  : _int(new InternalError)
{
  set(name, message);
}

Error::Error(Message &m)
  : _int(new InternalError)
{
  dbus_set_error_from_message(&(_int->error), m._pvt->msg);
}

Error::~Error() throw()
{
}

const char *Error::name() const
{
  return _int->error.name;
}

const char *Error::message() const
{
  return _int->error.message;
}

bool Error::is_set() const
{
  return *(_int);
}

void Error::set(const char *name, const char *message)
{
  dbus_set_error_const(&(_int->error), name, message);
}

const char *Error::what() const throw()
{
  return _int->error.message;
}

