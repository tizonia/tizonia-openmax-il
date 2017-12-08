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

#include <dbus-c++/dispatcher.h>

#include <dbus/dbus.h>

#include "dispatcher_p.h"
#include "server_p.h"
#include "connection_p.h"

DBus::Dispatcher *DBus::default_dispatcher = NULL;

using namespace DBus;

Timeout::Timeout(Timeout::Internal *i)
  : _int(i)
{
  dbus_timeout_set_data((DBusTimeout *)i, this, NULL);
}

int Timeout::interval() const
{
  return dbus_timeout_get_interval((DBusTimeout *)_int);
}

bool Timeout::enabled() const
{
  return dbus_timeout_get_enabled((DBusTimeout *)_int);
}

bool Timeout::handle()
{
  return dbus_timeout_handle((DBusTimeout *)_int);
}

/*
*/

Watch::Watch(Watch::Internal *i)
  : _int(i)
{
  dbus_watch_set_data((DBusWatch *)i, this, NULL);
}

int Watch::descriptor() const
{
#if HAVE_WIN32
  return dbus_watch_get_socket((DBusWatch *)_int);
#else
  // check dbus version and use dbus_watch_get_unix_fd() only in dbus >= 1.1.1
#if (DBUS_VERSION_MAJOR == 1 && DBUS_VERSION_MINOR == 1 && DBUS_VERSION_MICRO >= 1) || \
		(DBUS_VERSION_MAJOR == 1 && DBUS_VERSION_MAJOR > 1) || \
		(DBUS_VERSION_MAJOR > 1)
  return dbus_watch_get_unix_fd((DBusWatch *)_int);
#else
  return dbus_watch_get_fd((DBusWatch *)_int);
#endif
#endif
}

int Watch::flags() const
{
  return dbus_watch_get_flags((DBusWatch *)_int);
}

bool Watch::enabled() const
{
  return dbus_watch_get_enabled((DBusWatch *)_int);
}

bool Watch::handle(int flags)
{
  return dbus_watch_handle((DBusWatch *)_int, flags);
}

/*
*/

dbus_bool_t Dispatcher::Private::on_add_watch(DBusWatch *watch, void *data)
{
  Dispatcher *d = static_cast<Dispatcher *>(data);

  Watch::Internal *w = reinterpret_cast<Watch::Internal *>(watch);

  d->add_watch(w);

  return true;
}

void Dispatcher::Private::on_rem_watch(DBusWatch *watch, void *data)
{
  Dispatcher *d = static_cast<Dispatcher *>(data);

  Watch *w = static_cast<Watch *>(dbus_watch_get_data(watch));

  d->rem_watch(w);
}

void Dispatcher::Private::on_toggle_watch(DBusWatch *watch, void *data)
{
  Watch *w = static_cast<Watch *>(dbus_watch_get_data(watch));

  w->toggle();
}

dbus_bool_t Dispatcher::Private::on_add_timeout(DBusTimeout *timeout, void *data)
{
  Dispatcher *d = static_cast<Dispatcher *>(data);

  Timeout::Internal *t = reinterpret_cast<Timeout::Internal *>(timeout);

  d->add_timeout(t);

  return true;
}

void Dispatcher::Private::on_rem_timeout(DBusTimeout *timeout, void *data)
{
  Dispatcher *d = static_cast<Dispatcher *>(data);

  Timeout *t = static_cast<Timeout *>(dbus_timeout_get_data(timeout));

  d->rem_timeout(t);
}

void Dispatcher::Private::on_toggle_timeout(DBusTimeout *timeout, void *data)
{
  Timeout *t = static_cast<Timeout *>(dbus_timeout_get_data(timeout));

  t->toggle();
}

void Dispatcher::queue_connection(Connection::Private *cp)
{
  _mutex_p.lock();
  _pending_queue.push_back(cp);
  _mutex_p.unlock();
}


bool Dispatcher::has_something_to_dispatch()
{
  _mutex_p.lock();
  bool has_something = false;
  for (Connection::PrivatePList::iterator it = _pending_queue.begin();
       it != _pending_queue.end() && !has_something;
       ++it)
  {
    has_something = (*it)->has_something_to_dispatch();
  }

  _mutex_p.unlock();
  return has_something;
}


void Dispatcher::dispatch_pending()
{
  while (1)
  {
    _mutex_p.lock();
    if (_pending_queue.empty())
    {
      _mutex_p.unlock();
      break;
    }

    Connection::PrivatePList pending_queue_copy(_pending_queue);
    _mutex_p.unlock();

    size_t copy_elem_num(pending_queue_copy.size());

    dispatch_pending(pending_queue_copy);

    //only push_back on list is mandatory!
    _mutex_p.lock();

    Connection::PrivatePList::iterator i, j;
    i = _pending_queue.begin();
    size_t counter = 0;
    while (counter < copy_elem_num && i != _pending_queue.end())
    {
      j = i;
      ++j;
      _pending_queue.erase(i);
      i = j;
      ++counter;
    }

    _mutex_p.unlock();
  }
}

void Dispatcher::dispatch_pending(Connection::PrivatePList &pending_queue)
{
  // SEEME: dbus-glib is dispatching only one message at a time to not starve the loop/other things...

  _mutex_p_copy.lock();
  while (pending_queue.size() > 0)
  {
    Connection::PrivatePList::iterator i, j;

    i = pending_queue.begin();

    while (i != pending_queue.end())
    {
      j = i;

      ++j;

      if ((*i)->do_dispatch())
        pending_queue.erase(i);
      else
        debug_log("dispatch_pending_private: do_dispatch error");

      i = j;
    }
  }
  _mutex_p_copy.unlock();
}

void DBus::_init_threading()
{
#ifdef DBUS_HAS_THREADS_INIT_DEFAULT
  dbus_threads_init_default();
#else
  debug_log("Thread support is not enabled! Your D-Bus version is too old!");
#endif//DBUS_HAS_THREADS_INIT_DEFAULT
}

void DBus::_init_threading(
  MutexNewFn m1,
  MutexFreeFn m2,
  MutexLockFn m3,
  MutexUnlockFn m4,
  CondVarNewFn c1,
  CondVarFreeFn c2,
  CondVarWaitFn c3,
  CondVarWaitTimeoutFn c4,
  CondVarWakeOneFn c5,
  CondVarWakeAllFn c6
)
{
#ifndef DBUS_HAS_RECURSIVE_MUTEX
  DBusThreadFunctions functions =
  {
    DBUS_THREAD_FUNCTIONS_MUTEX_NEW_MASK |
    DBUS_THREAD_FUNCTIONS_MUTEX_FREE_MASK |
    DBUS_THREAD_FUNCTIONS_MUTEX_LOCK_MASK |
    DBUS_THREAD_FUNCTIONS_MUTEX_UNLOCK_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_NEW_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_FREE_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_WAIT_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_WAIT_TIMEOUT_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_WAKE_ONE_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_WAKE_ALL_MASK,
    (DBusMutexNewFunction) m1,
    (DBusMutexFreeFunction) m2,
    (DBusMutexLockFunction) m3,
    (DBusMutexUnlockFunction) m4,
    (DBusCondVarNewFunction) c1,
    (DBusCondVarFreeFunction) c2,
    (DBusCondVarWaitFunction) c3,
    (DBusCondVarWaitTimeoutFunction) c4,
    (DBusCondVarWakeOneFunction) c5,
    (DBusCondVarWakeAllFunction) c6
  };
#else
  DBusThreadFunctions functions =
  {
    DBUS_THREAD_FUNCTIONS_RECURSIVE_MUTEX_NEW_MASK |
    DBUS_THREAD_FUNCTIONS_RECURSIVE_MUTEX_FREE_MASK |
    DBUS_THREAD_FUNCTIONS_RECURSIVE_MUTEX_LOCK_MASK |
    DBUS_THREAD_FUNCTIONS_RECURSIVE_MUTEX_UNLOCK_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_NEW_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_FREE_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_WAIT_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_WAIT_TIMEOUT_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_WAKE_ONE_MASK |
    DBUS_THREAD_FUNCTIONS_CONDVAR_WAKE_ALL_MASK,
    0, 0, 0, 0,
    (DBusCondVarNewFunction) c1,
    (DBusCondVarFreeFunction) c2,
    (DBusCondVarWaitFunction) c3,
    (DBusCondVarWaitTimeoutFunction) c4,
    (DBusCondVarWakeOneFunction) c5,
    (DBusCondVarWakeAllFunction) c6,
    (DBusRecursiveMutexNewFunction) m1,
    (DBusRecursiveMutexFreeFunction) m2,
    (DBusRecursiveMutexLockFunction) m3,
    (DBusRecursiveMutexUnlockFunction) m4
  };
#endif//DBUS_HAS_RECURSIVE_MUTEX
  dbus_threads_init(&functions);
}
