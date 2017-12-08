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


#ifndef __DBUSXX_UTIL_H
#define __DBUSXX_UTIL_H

#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>

#include "api.h"
#include "debug.h"

namespace DBus
{

/*
 *   Very simple reference counting
 */

class DXXAPI RefCnt
{
public:

  RefCnt()
  {
    __ref = new int;
    (*__ref) = 1;
  }

  RefCnt(const RefCnt &rc)
  {
    __ref = rc.__ref;
    ref();
  }

  virtual ~RefCnt()
  {
    unref();
  }

  RefCnt &operator = (const RefCnt &ref)
  {
    ref.ref();
    unref();
    __ref = ref.__ref;
    return *this;
  }

  bool noref() const
  {
    return (*__ref) == 0;
  }

  bool one() const
  {
    return (*__ref) == 1;
  }

private:

  DXXAPILOCAL void ref() const
  {
    ++ (*__ref);
  }
  DXXAPILOCAL void unref() const
  {
    -- (*__ref);

    if ((*__ref) < 0)
    {
      debug_log("%p: refcount dropped below zero!", __ref);
    }

    if (noref())
    {
      delete __ref;
    }
  }

private:

  int *__ref;
};

/*
 *   Reference counting pointers (emulate boost::shared_ptr)
 */

template <class T>
class RefPtrI		// RefPtr to incomplete type
{
public:

  RefPtrI(T *ptr = 0);

  ~RefPtrI();

  RefPtrI &operator = (const RefPtrI &ref)
  {
    if (this != &ref)
    {
      if (__cnt.one()) delete __ptr;

      __ptr = ref.__ptr;
      __cnt = ref.__cnt;
    }
    return *this;
  }

  T &operator *() const
  {
    return *__ptr;
  }

  T *operator ->() const
  {
    if (__cnt.noref()) return 0;

    return __ptr;
  }

  T *get() const
  {
    if (__cnt.noref()) return 0;

    return __ptr;
  }

private:

  T *__ptr;
  RefCnt __cnt;
};

template <class T>
class RefPtr
{
public:

  RefPtr(T *ptr = 0)
    : __ptr(ptr)
  {}

  ~RefPtr()
  {
    if (__cnt.one()) delete __ptr;
  }

  RefPtr &operator = (const RefPtr &ref)
  {
    if (this != &ref)
    {
      if (__cnt.one()) delete __ptr;

      __ptr = ref.__ptr;
      __cnt = ref.__cnt;
    }
    return *this;
  }

  T &operator *() const
  {
    return *__ptr;
  }

  T *operator ->() const
  {
    if (__cnt.noref()) return 0;

    return __ptr;
  }

  T *get() const
  {
    if (__cnt.noref()) return 0;

    return __ptr;
  }

private:

  T *__ptr;
  RefCnt __cnt;
};

/*
 *   Typed callback template
 */

template <class R, class P>
class Callback_Base
{
public:

  virtual R call(P param) const = 0;

  virtual ~Callback_Base()
  {}
};

template <class R, class P>
class Slot
{
public:

  Slot &operator = (Callback_Base<R, P>* s)
  {
    _cb = s;

    return *this;
  }

  R operator()(P param) const
  {
    if (!empty())
    {
      return _cb->call(param);
    }

    // TODO: think about return type in this case
    // this assert should help me to find the use case where it's needed...
    //assert (false);
  }

  R call(P param) const
  {
    if (!empty())
    {
      return _cb->call(param);
    }

    // TODO: think about return type in this case
    // this assert should help me to find the use case where it's needed...
    //assert (false);
  }

  bool empty() const
  {
    return _cb.get() == 0;
  }

private:

  RefPtr< Callback_Base<R, P> > _cb;
};

template <class C, class R, class P>
class Callback : public Callback_Base<R, P>
{
public:

  typedef R(C::*M)(P);

  Callback(C *c, M m)
    : _c(c), _m(m)
  {}

  R call(P param) const
  {
    /*if (_c)*/ return (_c->*_m)(param);
  }

private:

  C *_c;
  M _m;
};

/// create std::string from any number
template <typename T>
std::string toString(const T &thing, int w = 0, int p = 0)
{
  std::ostringstream os;
  os << std::setw(w) << std::setprecision(p) << thing;
  return os.str();
}

} /* namespace DBus */

#endif//__DBUSXX_UTIL_H
