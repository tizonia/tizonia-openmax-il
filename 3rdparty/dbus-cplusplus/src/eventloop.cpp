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

#include <dbus-c++/eventloop.h>
#include <dbus-c++/debug.h>

#include <sys/poll.h>
#include <sys/time.h>

#include <dbus/dbus.h>

using namespace DBus;
using namespace std;

static double millis(timeval tv)
{
  return (tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0);
}

DefaultTimeout::DefaultTimeout(int interval, bool repeat, DefaultMainLoop *ed)
  : _enabled(true), _interval(interval), _repeat(repeat), _expiration(0), _data(0), _disp(ed)
{
  timeval now;
  gettimeofday(&now, NULL);

  _expiration = millis(now) + interval;

  _disp->_mutex_t.lock();
  _disp->_timeouts.push_back(this);
  _disp->_mutex_t.unlock();
}

DefaultTimeout::~DefaultTimeout()
{
  _disp->_mutex_t.lock();
  _disp->_timeouts.remove(this);
  _disp->_mutex_t.unlock();
}

DefaultWatch::DefaultWatch(int fd, int flags, DefaultMainLoop *ed)
  : _enabled(true), _fd(fd), _flags(flags), _state(0), _data(0), _disp(ed)
{
  _disp->_mutex_w.lock();
  _disp->_watches.push_back(this);
  _disp->_mutex_w.unlock();
}

DefaultWatch::~DefaultWatch()
{
  _disp->_mutex_w.lock();
  _disp->_watches.remove(this);
  _disp->_mutex_w.unlock();
}

DefaultMutex::DefaultMutex()
{
  pthread_mutex_init(&_mutex, NULL);
}

DefaultMutex::DefaultMutex(bool recursive)
{
  if (recursive)
  {
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_mutex, &attr);
  }
  else
  {
    pthread_mutex_init(&_mutex, NULL);
  }
}

DefaultMutex::~DefaultMutex()
{
  pthread_mutex_destroy(&_mutex);
}

void DefaultMutex::lock()
{
  pthread_mutex_lock(&_mutex);
}

void DefaultMutex::unlock()
{
  pthread_mutex_unlock(&_mutex);
}

DefaultMainLoop::DefaultMainLoop() :
  _mutex_w(true)
{
}

DefaultMainLoop::~DefaultMainLoop()
{
  _mutex_w.lock();

  DefaultWatches::iterator wi = _watches.begin();
  while (wi != _watches.end())
  {
    DefaultWatches::iterator wmp = wi;
    ++wmp;
    _mutex_w.unlock();
    delete(*wi);
    _mutex_w.lock();
    wi = wmp;
  }
  _mutex_w.unlock();

  _mutex_t.lock();

  DefaultTimeouts::iterator ti = _timeouts.begin();
  while (ti != _timeouts.end())
  {
    DefaultTimeouts::iterator tmp = ti;
    ++tmp;
    _mutex_t.unlock();
    delete(*ti);
    _mutex_t.lock();
    ti = tmp;
  }
  _mutex_t.unlock();
}

void DefaultMainLoop::dispatch()
{
  _mutex_w.lock();

  int nfd = _watches.size();

  if (_fdunlock)
  {
    nfd = nfd + 2;
  }

  pollfd fds[nfd];

  DefaultWatches::iterator wi = _watches.begin();

  for (nfd = 0; wi != _watches.end(); ++wi)
  {
    if ((*wi)->enabled())
    {
      fds[nfd].fd = (*wi)->descriptor();
      fds[nfd].events = (*wi)->flags();
      fds[nfd].revents = 0;

      ++nfd;
    }
  }

  if (_fdunlock)
  {
    fds[nfd].fd = _fdunlock[0];
    fds[nfd].events = POLLIN | POLLOUT | POLLPRI ;
    fds[nfd].revents = 0;

    nfd++;
    fds[nfd].fd = _fdunlock[1];
    fds[nfd].events = POLLIN | POLLOUT | POLLPRI ;
    fds[nfd].revents = 0;
  }

  _mutex_w.unlock();

  int wait_min = 10000;

  DefaultTimeouts::iterator ti;

  _mutex_t.lock();

  for (ti = _timeouts.begin(); ti != _timeouts.end(); ++ti)
  {
    if ((*ti)->enabled() && (*ti)->interval() < wait_min)
      wait_min = (*ti)->interval();
  }

  _mutex_t.unlock();

  poll(fds, nfd, wait_min);

  timeval now;
  gettimeofday(&now, NULL);

  double now_millis = millis(now);

  _mutex_t.lock();

  ti = _timeouts.begin();

  while (ti != _timeouts.end())
  {
    DefaultTimeouts::iterator tmp = ti;
    ++tmp;

    if ((*ti)->enabled() && now_millis >= (*ti)->_expiration)
    {
      (*ti)->expired(*(*ti));

      if ((*ti)->_repeat)
      {
        (*ti)->_expiration = now_millis + (*ti)->_interval;
      }

    }

    ti = tmp;
  }

  _mutex_t.unlock();

  _mutex_w.lock();

  for (int j = 0; j < nfd; ++j)
  {
    DefaultWatches::iterator wi;

    for (wi = _watches.begin(); wi != _watches.end();)
    {
      DefaultWatches::iterator tmp = wi;
      ++tmp;

      if ((*wi)->enabled() && (*wi)->_fd == fds[j].fd)
      {
        if (fds[j].revents)
        {
          (*wi)->_state = fds[j].revents;

          (*wi)->ready(*(*wi));

          fds[j].revents = 0;
        }
      }

      wi = tmp;
    }
  }
  _mutex_w.unlock();
}

