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


#ifndef __DBUSXX_MESSAGE_H
#define __DBUSXX_MESSAGE_H

#include <string>
#include <map>

#include "api.h"
#include "util.h"

namespace DBus
{

class Message;
class ErrorMessage;
class SignalMessage;
class ReturnMessage;
class Error;
class Connection;

class DXXAPI MessageIter
{
public:

  MessageIter() {}

  int type();

  bool at_end();

  bool has_next();

  MessageIter &operator ++();

  MessageIter operator ++(int);

  bool append_byte(unsigned char byte);

  unsigned char get_byte();

  bool append_bool(bool b);

  bool get_bool();

  bool append_int16(signed short i);

  signed short get_int16();

  bool append_uint16(unsigned short u);

  unsigned short get_uint16();

  bool append_int32(signed int i);

  signed int get_int32();

  bool append_uint32(unsigned int u);

  unsigned int get_uint32();

  bool append_int64(signed long long i);

  signed long long get_int64();

  bool append_uint64(unsigned long long i);

  unsigned long long get_uint64();

  bool append_double(double d);

  double get_double();

  bool append_string(const char *chars);

  const char *get_string();

  bool append_path(const char *chars);

  const char *get_path();

  bool append_signature(const char *chars);

  const char *get_signature();

  char *signature() const; //returned string must be manually free()'d

  MessageIter recurse();

  bool append_array(char type, const void *ptr, size_t length);

  int array_type();

  int get_array(void *ptr);

  bool is_array();

  bool is_dict();

  MessageIter new_array(const char *sig);

  MessageIter new_variant(const char *sig);

  MessageIter new_struct();

  MessageIter new_dict_entry();

  void close_container(MessageIter &container);

  void copy_data(MessageIter &to);

  Message &msg() const
  {
    return *_msg;
  }

private:

  DXXAPILOCAL MessageIter(Message &msg) : _msg(&msg) {}

  DXXAPILOCAL bool append_basic(int type_id, void *value);

  DXXAPILOCAL void get_basic(int type_id, void *ptr);

private:

  /* I'm sorry, but don't want to include dbus.h in the public api
   */
  unsigned char _iter[sizeof(void *) * 3 + sizeof(int) * 11];

  Message *_msg;

  friend class Message;
};

class DXXAPI Message
{
public:

  struct Private;

  Message(Private *, bool incref = true);

  Message(const Message &m);

  ~Message();

  Message &operator = (const Message &m);

  Message copy();

  int type() const;

  int serial() const;

  int reply_serial() const;

  bool reply_serial(int);

  const char *sender() const;

  bool sender(const char *s);

  const char *destination() const;

  bool destination(const char *s);

  bool is_error() const;

  bool is_signal(const char *interface, const char *member) const;

  MessageIter reader() const;

  MessageIter writer();

  bool append(int first_type, ...);

  void terminate();

protected:

  Message();

protected:

  RefPtrI<Private> _pvt;

  /*	classes who need to read `_pvt` directly
  */
  friend class ErrorMessage;
  friend class ReturnMessage;
  friend class MessageIter;
  friend class Error;
  friend class Connection;
};

/*
*/

class DXXAPI ErrorMessage : public Message
{
public:

  ErrorMessage();

  ErrorMessage(const Message &, const char *name, const char *message);

  const char *name() const;

  bool name(const char *n);

  bool operator == (const ErrorMessage &) const;
};

/*
*/

class DXXAPI SignalMessage : public Message
{
public:

  SignalMessage(const char *name);

  SignalMessage(const char *path, const char *interface, const char *name);

  const char *interface() const;

  bool interface(const char *i);

  const char *member() const;

  bool member(const char *m);

  const char *path() const;

  char **path_split() const;

  bool path(const char *p);

  bool operator == (const SignalMessage &) const;
};

/*
*/

class DXXAPI CallMessage : public Message
{
public:

  CallMessage();

  CallMessage(const char *dest, const char *path, const char *iface, const char *method);

  const char *interface() const;

  bool interface(const char *i);

  const char *member() const;

  bool member(const char *m);

  const char *path() const;

  char **path_split() const;

  bool path(const char *p);

  const char *signature() const;

  bool operator == (const CallMessage &) const;
};

/*
*/

class DXXAPI ReturnMessage : public Message
{
public:

  ReturnMessage(const CallMessage &callee);

  const char *signature() const;
};

} /* namespace DBus */

#endif//__DBUSXX_MESSAGE_H
