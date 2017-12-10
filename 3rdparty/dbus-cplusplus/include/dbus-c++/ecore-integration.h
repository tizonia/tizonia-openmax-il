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


#ifndef __DBUSXX_ECORE_INTEGRATION_H
#define __DBUSXX_ECORE_INTEGRATION_H

#include <Ecore.h>

#include "api.h"
#include "dispatcher.h"
#include "Ecore.h"

namespace DBus
{

namespace Ecore
{

class BusDispatcher;

class DXXAPI BusTimeout : public Timeout
{
private:

  BusTimeout(Timeout::Internal *);

  ~BusTimeout();

  void toggle();

  static Eina_Bool timeout_handler(void *);

  void _enable();

  void _disable();

private:
  Ecore_Timer *_etimer;

  friend class BusDispatcher;
};

class DXXAPI BusWatch : public Watch
{
private:

  BusWatch(Watch::Internal *);

  ~BusWatch();

  void toggle();

  static Eina_Bool watch_check(void *data, Ecore_Fd_Handler *fdh);
  static Eina_Bool watch_prepare(void *data, Ecore_Fd_Handler *fdh);
  static Eina_Bool watch_dispatch(void *data, Ecore_Fd_Handler *fdh);

  void _enable();

  void _disable();

  void data(Ecore::BusDispatcher *bd);

private:
  Ecore_Fd_Handler *fd_handler;
  Ecore::BusDispatcher *_bd;

  friend class BusDispatcher;
};

class DXXAPI BusDispatcher : public Dispatcher
{
public:
  BusDispatcher();

  void enter() {}

  void leave() {}

  Timeout *add_timeout(Timeout::Internal *);

  void rem_timeout(Timeout *);

  Watch *add_watch(Watch::Internal *);

  void rem_watch(Watch *);

  static Eina_Bool dispatch(void *data, Ecore_Fd_Handler *fdh);
  static Eina_Bool check(void *data, Ecore_Fd_Handler *fdh);

private:
};

} /* namespace Ecore */

} /* namespace DBus */

#endif//__DBUSXX_ECORE_INTEGRATION_H
