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


#ifndef __DBUSXX_PENDING_CALL_H
#define __DBUSXX_PENDING_CALL_H

#include "api.h"
#include "util.h"
#include "message.h"

namespace DBus
{

class Connection;

class DXXAPI PendingCall
{
public:

  struct Private;

  PendingCall(Private *);

  PendingCall(const PendingCall &);

  virtual ~PendingCall();

  PendingCall &operator = (const PendingCall &);

  /*!
   * \brief Checks whether the pending call has received a reply yet, or not.
   *
   * \return true If a reply has been received.
   */
  bool completed();

  /*!
   * \brief Cancels the pending call, such that any reply or error received will
   *        just be ignored.
   *
   * Drops the dbus library's internal reference to the DBusPendingCall so will
   * free the call if nobody else is holding a reference. However you usually
   * get a reference from Connection::send_async() so probably your app
   * owns a ref also.
   *
   * Note that canceling a pending call will not simulate a timed-out call; if a
   * call times out, then a timeout error reply is received. If you cancel the
   * call, no reply is received unless the the reply was already received before
   * you canceled.
   */
  void cancel();

  /*!
   * \brief Block until the pending call is completed.
   *
   * The blocking is as with Connection::send_blocking(); it
   * does not enter the main loop or process other messages, it simply waits for
   * the reply in question.
   *
   * If the pending call is already completed, this function returns immediately.
   */
  void block();

  /*!
   * \brief Stores a pointer on a PendingCall, along with an optional function to
   *        be used for freeing the data when the data is set again, or when the
   *        pending call is finalized.
   *
   * The slot is allocated automatic.
   *
   * \param data The data to store.
   * \throw ErrorNoMemory
   */
  void data(void *data);

  /*!
   * \brief Retrieves data previously set with dbus_pending_call_set_data().
   *
   * The slot must still be allocated (must not have been freed).
   *
   * \return The data, or NULL if not found.
   */
  void *data();

  /*!
   * \return The data slot.
   */
  Slot<void, PendingCall &>& slot();

  /*!
   * \brief Gets the reply
   *
   * Ownership of the reply message passes to the caller. This function can only
   * be called once per pending call, since the reply message is tranferred to
   * the caller.
   *
   * \return The reply Message.
   * \throw ErrorNoReply
   */
  Message steal_reply();

private:

  RefPtrI<Private> _pvt;

  friend struct Private;
  friend class Connection;
};

} /* namespace DBus */

#endif//__DBUSXX_PENDING_CALL_H
