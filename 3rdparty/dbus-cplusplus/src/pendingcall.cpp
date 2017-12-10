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

#include <dbus-c++/pendingcall.h>

#include <dbus/dbus.h>

#include "internalerror.h"
#include "pendingcall_p.h"
#include "message_p.h"

using namespace DBus;

PendingCall::Private::Private(DBusPendingCall *dpc)
  : call(dpc), dataslot(-1)
{
  if (!dbus_pending_call_allocate_data_slot(&dataslot))
  {
    throw ErrorNoMemory("Unable to allocate data slot");
  }
}

PendingCall::Private::~Private()
{
  if (dataslot != -1)
  {
    dbus_pending_call_allocate_data_slot(&dataslot);
  }
}

void PendingCall::Private::notify_stub(DBusPendingCall *dpc, void *data)
{
  PendingCall::Private *pvt = static_cast<PendingCall::Private *>(data);

  PendingCall pc(pvt);
  pvt->slot(pc);
}

PendingCall::PendingCall(PendingCall::Private *p)
  : _pvt(p)
{
  if (!dbus_pending_call_set_notify(_pvt->call, Private::notify_stub, p, NULL))
  {
    throw ErrorNoMemory("Unable to initialize pending call");
  }
}

PendingCall::PendingCall(const PendingCall &c)
  : _pvt(c._pvt)
{
  dbus_pending_call_ref(_pvt->call);
}

PendingCall::~PendingCall()
{
  dbus_pending_call_unref(_pvt->call);
}

PendingCall &PendingCall::operator = (const PendingCall &c)
{
  if (&c != this)
  {
    dbus_pending_call_unref(_pvt->call);
    _pvt = c._pvt;
    dbus_pending_call_ref(_pvt->call);
  }
  return *this;
}

bool PendingCall::completed()
{
  return dbus_pending_call_get_completed(_pvt->call);
}

void PendingCall::cancel()
{
  dbus_pending_call_cancel(_pvt->call);
}

void PendingCall::block()
{
  dbus_pending_call_block(_pvt->call);
}

void PendingCall::data(void *p)
{
  if (!dbus_pending_call_set_data(_pvt->call, _pvt->dataslot, p, NULL))
  {
    throw ErrorNoMemory("Unable to initialize data slot");
  }
}

void *PendingCall::data()
{
  return dbus_pending_call_get_data(_pvt->call, _pvt->dataslot);
}

Slot<void, PendingCall &>& PendingCall::slot()
{
  return _pvt->slot;
}

Message PendingCall::steal_reply()
{
  DBusMessage *dmsg = dbus_pending_call_steal_reply(_pvt->call);
  if (!dmsg)
  {
    dbus_bool_t callComplete = dbus_pending_call_get_completed(_pvt->call);

    if (callComplete)
      throw ErrorNoReply("No reply available");
    else
      throw ErrorNoReply("Call not complete");
  }

  return Message(new Message::Private(dmsg));
}

