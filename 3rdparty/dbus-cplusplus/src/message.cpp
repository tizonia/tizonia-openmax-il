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

#include <dbus/dbus.h>
#include <cstdlib>

#include "internalerror.h"
#include "message_p.h"

using namespace DBus;

/*
*/

int MessageIter::type()
{
  return dbus_message_iter_get_arg_type((DBusMessageIter *)&_iter);
}

bool MessageIter::at_end()
{
  return type() == DBUS_TYPE_INVALID;
}

bool MessageIter::has_next()
{
  return dbus_message_iter_has_next((DBusMessageIter *)&_iter);
}

MessageIter &MessageIter::operator ++()
{
  dbus_message_iter_next((DBusMessageIter *)&_iter);
  return (*this);
}

MessageIter MessageIter::operator ++(int)
{
  MessageIter copy(*this);
  ++(*this);
  return copy;
}

bool MessageIter::append_basic(int type_id, void *value)
{
  return dbus_message_iter_append_basic((DBusMessageIter *)&_iter, type_id, value);
}

void MessageIter::get_basic(int type_id, void *ptr)
{
  if (type() != type_id)
    throw ErrorInvalidArgs("type mismatch");

  dbus_message_iter_get_basic((DBusMessageIter *)_iter, ptr);
}

bool MessageIter::append_byte(unsigned char b)
{
  return append_basic(DBUS_TYPE_BYTE, &b);
}

unsigned char MessageIter::get_byte()
{
  unsigned char b;
  get_basic(DBUS_TYPE_BYTE, &b);
  return b;
}

bool MessageIter::append_bool(bool b)
{
  dbus_bool_t db = b;
  return append_basic(DBUS_TYPE_BOOLEAN, &db);
}

bool MessageIter::get_bool()
{
  dbus_bool_t db;
  get_basic(DBUS_TYPE_BOOLEAN, &db);
  return (bool)db;
}

bool MessageIter::append_int16(signed short i)
{
  return append_basic(DBUS_TYPE_INT16, &i);
}

signed short MessageIter::get_int16()
{
  signed short i;
  get_basic(DBUS_TYPE_INT16, &i);
  return i;
}

bool MessageIter::append_uint16(unsigned short u)
{
  return append_basic(DBUS_TYPE_UINT16, &u);
}

unsigned short MessageIter::get_uint16()
{
  unsigned short u;
  get_basic(DBUS_TYPE_UINT16, &u);
  return u;
}

bool MessageIter::append_int32(signed int i)
{
  return append_basic(DBUS_TYPE_INT32, &i);
}

signed int MessageIter::get_int32()
{
  signed int i;
  get_basic(DBUS_TYPE_INT32, &i);
  return i;
}

bool MessageIter::append_uint32(unsigned int u)
{
  return append_basic(DBUS_TYPE_UINT32, &u);
}

unsigned int MessageIter::get_uint32()
{
  unsigned int u;
  get_basic(DBUS_TYPE_UINT32, &u);
  return u;
}

signed long long MessageIter::get_int64()
{
  signed long long i;
  get_basic(DBUS_TYPE_INT64, &i);
  return i;
}

bool MessageIter::append_int64(signed long long i)
{
  return append_basic(DBUS_TYPE_INT64, &i);
}

unsigned long long MessageIter::get_uint64()
{
  unsigned long long u;
  get_basic(DBUS_TYPE_UINT64, &u);
  return u;
}

bool MessageIter::append_uint64(unsigned long long u)
{
  return append_basic(DBUS_TYPE_UINT64, &u);
}

double MessageIter::get_double()
{
  double d;
  get_basic(DBUS_TYPE_DOUBLE, &d);
  return d;
}

bool MessageIter::append_double(double d)
{
  return append_basic(DBUS_TYPE_DOUBLE, &d);
}

bool MessageIter::append_string(const char *chars)
{
  return append_basic(DBUS_TYPE_STRING, &chars);
}

const char *MessageIter::get_string()
{
  char *chars;
  get_basic(DBUS_TYPE_STRING, &chars);
  return chars;
}

bool MessageIter::append_path(const char *chars)
{
  return append_basic(DBUS_TYPE_OBJECT_PATH, &chars);
}

const char *MessageIter::get_path()
{
  char *chars;
  get_basic(DBUS_TYPE_OBJECT_PATH, &chars);
  return chars;
}

bool MessageIter::append_signature(const char *chars)
{
  return append_basic(DBUS_TYPE_SIGNATURE, &chars);
}

const char *MessageIter::get_signature()
{
  char *chars;
  get_basic(DBUS_TYPE_SIGNATURE, &chars);
  return chars;
}

MessageIter MessageIter::recurse()
{
  MessageIter iter(msg());
  dbus_message_iter_recurse((DBusMessageIter *)&_iter, (DBusMessageIter *) & (iter._iter));
  return iter;
}

char *MessageIter::signature() const
{
  return dbus_message_iter_get_signature((DBusMessageIter *)&_iter);
}

bool MessageIter::append_array(char type, const void *ptr, size_t length)
{
  return dbus_message_iter_append_fixed_array((DBusMessageIter *)&_iter, type, &ptr, length);
}

int MessageIter::array_type()
{
  return dbus_message_iter_get_element_type((DBusMessageIter *)&_iter);
}

int MessageIter::get_array(void *ptr)
{
  int length;
  dbus_message_iter_get_fixed_array((DBusMessageIter *)&_iter, ptr, &length);
  return length;
}

bool MessageIter::is_array()
{
  return dbus_message_iter_get_arg_type((DBusMessageIter *)&_iter) == DBUS_TYPE_ARRAY;
}

bool MessageIter::is_dict()
{
  return is_array() && dbus_message_iter_get_element_type((DBusMessageIter *)_iter) == DBUS_TYPE_DICT_ENTRY;
}

MessageIter MessageIter::new_array(const char *sig)
{
  MessageIter arr(msg());
  dbus_message_iter_open_container(
    (DBusMessageIter *)&_iter, DBUS_TYPE_ARRAY, sig, (DBusMessageIter *) & (arr._iter)
  );
  return arr;
}

MessageIter MessageIter::new_variant(const char *sig)
{
  MessageIter var(msg());
  dbus_message_iter_open_container(
    (DBusMessageIter *)_iter, DBUS_TYPE_VARIANT, sig, (DBusMessageIter *) & (var._iter)
  );
  return var;
}

MessageIter MessageIter::new_struct()
{
  MessageIter stu(msg());
  dbus_message_iter_open_container(
    (DBusMessageIter *)_iter, DBUS_TYPE_STRUCT, NULL, (DBusMessageIter *) & (stu._iter)
  );
  return stu;
}

MessageIter MessageIter::new_dict_entry()
{
  MessageIter ent(msg());
  dbus_message_iter_open_container(
    (DBusMessageIter *)_iter, DBUS_TYPE_DICT_ENTRY, NULL, (DBusMessageIter *) & (ent._iter)
  );
  return ent;
}

void MessageIter::close_container(MessageIter &container)
{
  dbus_message_iter_close_container((DBusMessageIter *)&_iter, (DBusMessageIter *) & (container._iter));
}

static bool is_basic_type(int typecode)
{
  switch (typecode)
  {
  case 'y':
  case 'b':
  case 'n':
  case 'q':
  case 'i':
  case 'u':
  case 'x':
  case 't':
  case 'd':
  case 's':
  case 'o':
  case 'g':
    return true;
  default:
    return false;
  }
}

void MessageIter::copy_data(MessageIter &to)
{
  for (MessageIter &from = *this; !from.at_end(); ++from)
  {
    if (is_basic_type(from.type()))
    {
      debug_log("copying basic type: %c", from.type());

      unsigned char value[8];
      from.get_basic(from.type(), &value);
      to.append_basic(from.type(), &value);
    }
    else
    {
      MessageIter from_container = from.recurse();
      char *sig = from_container.signature();

      debug_log("copying compound type: %c[%s]", from.type(), sig);

      MessageIter to_container(to.msg());
      dbus_message_iter_open_container
      (
        (DBusMessageIter *) & (to._iter),
        from.type(),
        from.type() == DBUS_TYPE_VARIANT ? NULL : sig,
        (DBusMessageIter *) & (to_container._iter)
      );

      from_container.copy_data(to_container);
      to.close_container(to_container);
      free(sig);
    }
  }
}

/*
*/

Message::Message()
  : _pvt(new Private)
{
}

Message::Message(Message::Private *p, bool incref)
  : _pvt(p)
{
  if (_pvt->msg && incref) dbus_message_ref(_pvt->msg);
}

Message::Message(const Message &m)
  : _pvt(m._pvt)
{
  dbus_message_ref(_pvt->msg);
}

Message::~Message()
{
  dbus_message_unref(_pvt->msg);
}

Message &Message::operator = (const Message &m)
{
  if (&m != this)
  {
    dbus_message_unref(_pvt->msg);
    _pvt = m._pvt;
    dbus_message_ref(_pvt->msg);
  }
  return *this;
}

Message Message::copy()
{
  Private *pvt = new Private(dbus_message_copy(_pvt->msg));
  return Message(pvt);
}

bool Message::append(int first_type, ...)
{
  va_list vl;
  va_start(vl, first_type);

  bool b = dbus_message_append_args_valist(_pvt->msg, first_type, vl);

  va_end(vl);
  return b;
}

void Message::terminate()
{
  dbus_message_append_args(_pvt->msg, DBUS_TYPE_INVALID);
}

int Message::type() const
{
  return dbus_message_get_type(_pvt->msg);
}

int Message::serial() const
{
  return dbus_message_get_serial(_pvt->msg);
}

int Message::reply_serial() const
{
  return dbus_message_get_reply_serial(_pvt->msg);
}

bool Message::reply_serial(int s)
{
  return dbus_message_set_reply_serial(_pvt->msg, s);
}

const char *Message::sender() const
{
  return dbus_message_get_sender(_pvt->msg);
}

bool Message::sender(const char *s)
{
  return dbus_message_set_sender(_pvt->msg, s);
}

const char *Message::destination() const
{
  return dbus_message_get_destination(_pvt->msg);
}

bool Message::destination(const char *s)
{
  return dbus_message_set_destination(_pvt->msg, s);
}

bool Message::is_error() const
{
  return type() == DBUS_MESSAGE_TYPE_ERROR;
}

bool Message::is_signal(const char *interface, const char *member) const
{
  return dbus_message_is_signal(_pvt->msg, interface, member);
}

MessageIter Message::writer()
{
  MessageIter iter(*this);
  dbus_message_iter_init_append(_pvt->msg, (DBusMessageIter *) & (iter._iter));
  return iter;
}

MessageIter Message::reader() const
{
  MessageIter iter(const_cast<Message &>(*this));
  dbus_message_iter_init(_pvt->msg, (DBusMessageIter *) & (iter._iter));
  return iter;
}

/*
*/

ErrorMessage::ErrorMessage()
{
  _pvt->msg = dbus_message_new(DBUS_MESSAGE_TYPE_ERROR);
}

ErrorMessage::ErrorMessage(const Message &to_reply, const char *name, const char *message)
{
  _pvt->msg = dbus_message_new_error(to_reply._pvt->msg, name, message);
}

bool ErrorMessage::operator == (const ErrorMessage &m) const
{
  return dbus_message_is_error(_pvt->msg, m.name());
}

const char *ErrorMessage::name() const
{
  return dbus_message_get_error_name(_pvt->msg);
}

bool ErrorMessage::name(const char *n)
{
  return dbus_message_set_error_name(_pvt->msg, n);
}

/*
*/

SignalMessage::SignalMessage(const char *name)
{
  _pvt->msg = dbus_message_new(DBUS_MESSAGE_TYPE_SIGNAL);
  member(name);
}

SignalMessage::SignalMessage(const char *path, const char *interface, const char *name)
{
  _pvt->msg = dbus_message_new_signal(path, interface, name);
}

bool SignalMessage::operator == (const SignalMessage &m) const
{
  return dbus_message_is_signal(_pvt->msg, m.interface(), m.member());
}

const char *SignalMessage::interface() const
{
  return dbus_message_get_interface(_pvt->msg);
}

bool SignalMessage::interface(const char *i)
{
  return dbus_message_set_interface(_pvt->msg, i);
}

const char *SignalMessage::member() const
{
  return dbus_message_get_member(_pvt->msg);
}

bool SignalMessage::member(const char *m)
{
  return dbus_message_set_member(_pvt->msg, m);
}

const char *SignalMessage::path() const
{
  return dbus_message_get_path(_pvt->msg);
}

char **SignalMessage::path_split() const
{
  char **p;
  dbus_message_get_path_decomposed(_pvt->msg, &p);	//todo: return as a std::vector ?
  return p;
}

bool SignalMessage::path(const char *p)
{
  return dbus_message_set_path(_pvt->msg, p);
}

/*
*/

CallMessage::CallMessage()
{
  _pvt->msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
}

CallMessage::CallMessage(const char *dest, const char *path, const char *iface, const char *method)
{
  _pvt->msg = dbus_message_new_method_call(dest, path, iface, method);
}

bool CallMessage::operator == (const CallMessage &m) const
{
  return dbus_message_is_method_call(_pvt->msg, m.interface(), m.member());
}

const char *CallMessage::interface() const
{
  return dbus_message_get_interface(_pvt->msg);
}

bool CallMessage::interface(const char *i)
{
  return dbus_message_set_interface(_pvt->msg, i);
}

const char *CallMessage::member() const
{
  return dbus_message_get_member(_pvt->msg);
}

bool CallMessage::member(const char *m)
{
  return dbus_message_set_member(_pvt->msg, m);
}

const char *CallMessage::path() const
{
  return dbus_message_get_path(_pvt->msg);
}

char **CallMessage::path_split() const
{
  char **p;
  dbus_message_get_path_decomposed(_pvt->msg, &p);
  return p;
}

bool CallMessage::path(const char *p)
{
  return dbus_message_set_path(_pvt->msg, p);
}

const char *CallMessage::signature() const
{
  return dbus_message_get_signature(_pvt->msg);
}

/*
*/

ReturnMessage::ReturnMessage(const CallMessage &callee)
{
  _pvt = new Private(dbus_message_new_method_return(callee._pvt->msg));
}

const char *ReturnMessage::signature() const
{
  return dbus_message_get_signature(_pvt->msg);
}

