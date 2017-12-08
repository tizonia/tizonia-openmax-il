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


#ifndef __DBUSXX_CONNECTION_H
#define __DBUSXX_CONNECTION_H

#include <list>

#include "api.h"
#include "types.h"
#include "util.h"
#include "message.h"
#include "pendingcall.h"

namespace DBus
{

class Connection;

typedef Slot<bool, const Message &> MessageSlot;

typedef std::list<Connection>	ConnectionList;

class ObjectAdaptor;
class Dispatcher;

class DXXAPI Connection
{
public:

  static Connection SystemBus();

  static Connection SessionBus();

  static Connection ActivationBus();

  struct Private;

  typedef std::list<Private *> PrivatePList;

  Connection(Private *);

  Connection(const char *address, bool priv = true);

  Connection(const Connection &c);

  virtual ~Connection();

  Dispatcher *setup(Dispatcher *);

  bool operator == (const Connection &) const;

  /*!
   * \brief Adds a match rule to match messages going through the message bus.
   *
   * The "rule" argument is the string form of a match rule.
   *
   * If you pass NULL for the error, this function will not block; the match
   * thus won't be added until you flush the connection, and if there's an error
   * adding the match (only possible error is lack of resources in the bus), you
   * won't find out about it.
   *
   * Normal API conventions would have the function return a boolean value
   * indicating whether the error was set, but that would require blocking always
   * to determine the return value.
   *
   * The AddMatch method is fully documented in the D-Bus specification. For
   * quick reference, the format of the match rules is discussed here, but the
   * specification is the canonical version of this information.
   *
   * Rules are specified as a string of comma separated key/value pairs. An
   * example is "type='signal',sender='org.freedesktop.DBus',
   * interface='org.freedesktop.DBus',member='Foo', path='/bar/foo',destination=':452345.34'"
   *
   * Possible keys you can match on are type, sender, interface, member, path,
   * destination and numbered keys to match message args (keys are 'arg0', 'arg1', etc.).
   * Omitting a key from the rule indicates a wildcard match. For instance omitting
   * the member from a match rule but adding a sender would let all messages from
   * that sender through regardless of the member.
   *
   * Matches are inclusive not exclusive so as long as one rule matches the
   * message will get through. It is important to note this because every time a
   * essage is received the application will be paged into memory to process it.
   * This can cause performance problems such as draining batteries on embedded platforms.
   *
   * If you match message args ('arg0', 'arg1', and so forth) only string arguments
   * will match. That is, arg0='5' means match the string "5" not the integer 5.
   *
   * Currently there is no way to match against non-string arguments.
   *
   * Matching on interface is tricky because method call messages only optionally
   * specify the interface. If a message omits the interface, then it will NOT
   * match if the rule specifies an interface name. This means match rules on
   * method calls should not usually give an interface.
   *
   * However, signal messages are required to include the interface so when
   * matching signals usually you should specify the interface in the match rule.
   *
   * For security reasons, you can match arguments only up to DBUS_MAXIMUM_MATCH_RULE_ARG_NUMBER.
   *
   * Match rules have a maximum length of DBUS_MAXIMUM_MATCH_RULE_LENGTH bytes.
   *
   * Both of these maximums are much higher than you're likely to need, they only
   * exist because the D-Bus bus daemon has fixed limits on all resource usage.
   *
   * \param rule Textual form of match rule.
   * \throw Error
   */
  void add_match(const char *rule);

  /*!
   * \brief Removes a previously-added match rule "by value" (the most
   *        recently-added identical rule gets removed).
   *
   * The "rule" argument is the string form of a match rule.
   *
   * The bus compares match rules semantically, not textually, so whitespace and
   * ordering don't have to be identical to the rule you passed to add_match().
   *
   * \param rule Textual form of match rule.
   * \throw Error
   */
  void remove_match(const char *rule, bool		throw_on_error);

  /*!
   * \brief Adds a message filter.
   *
   * Filters are handlers that are run on all incoming messages, prior to the
   * objects registered with ObjectAdaptor::register_obj(). Filters are
   * run in the order that they were added. The same handler can be added as a
   * filter more than once, in which case it will be run more than once. Filters
   * added during a filter callback won't be run on the message being processed.
   *
   * \param s The MessageSlot to add.
   */
  bool add_filter(MessageSlot &s);

  /*!
   * \brief Removes a previously-added message filter.
   *
   * It is a programming error to call this function for a handler that has not
   * been added as a filter. If the given handler was added more than once, only
   * one instance of it will be removed (the most recently-added instance).
   *
   * \param s The MessageSlot to remove.
   */
  void remove_filter(MessageSlot &s);

  /*!
   * \brief Sets the unique name of the connection, as assigned by the message bus.
   *
   * Can only be used if you registered with the bus manually (i.e. if you did
   * not call register_bus()). Can only be called once per connection. After
   * the unique name is set, you can get it with unique_name(void).
   *
   * The only reason to use this function is to re-implement the equivalent of
   * register_bus() yourself. One (probably unusual) reason to do that might
   * be to do the bus registration call asynchronously instead of synchronously.
   *
   * \note Just use dbus_bus_get() or dbus_bus_get_private(), or worst case
   *       register_bus(), instead of messing with this function. There's
   *       really no point creating pain for yourself by doing things manually.
   *       (Not sure if this is yet wrapped.)
   *
   * It's hard to use this function safely on shared connections (created by
   * Connection()) in a multithreaded application, because only one
   * registration attempt can be sent to the bus. If two threads are both
   * sending the registration message, there is no mechanism in libdbus itself
   * to avoid sending it twice.
   *
   * Thus, you need a way to coordinate which thread sends the registration
   * attempt; which also means you know which thread will call
   * unique_name(const char*). If you don't know about all threads in the app
   * (for example, if some libraries you're using might start libdbus-using
   * threads), then you need to avoid using this function on shared connections.
   *
   * \param n The unique name.
   */
  bool unique_name(const char *n);

  /*!
   * \brief Gets the unique name of the connection as assigned by the message bus.
   *
   * Only possible after the connection has been registered with the message bus.
   * All connections returned by dbus_bus_get() or dbus_bus_get_private() have
   * been successfully registered. (Not sure if this is yet wrapped.)
   *
   * The name remains valid until the connection is freed, and should not be
   * freed by the caller.
   *
   * Other than dbus_bus_get(), there are two ways to set the unique name; one
   * is register_bus(), the other is unique_name(const char*). You are
   * responsible for calling unique_name(const char*) if you register by hand
   * instead of using register_bus().
   */
  const char *unique_name() const;

  /*!
   * \brief Registers a connection with the bus.
   *
   * This must be the first thing an application does when connecting to the
   * message bus. If registration succeeds, the unique name will be set, and
   * can be obtained using unique_name(void).
   *
   * This function will block until registration is complete.
   *
   * If the connection has already registered with the bus (determined by
   * checking whether unique_name(void) returns a non-NULL value),
   * then this function does nothing.
   *
   * If you use dbus_bus_get() or dbus_bus_get_private() this function will be
   * called for you.  (Not sure if this is yet wrapped.)
   *
   * \note Just use dbus_bus_get() or dbus_bus_get_private() instead of
   * register_bus() and save yourself some pain. Using register_bus()
   * manually is only useful if you have your own custom message bus not found
   * in DBusBusType.
   *
   * If you open a bus connection by the contructor of Connection() you will have to register_bus()
   * yourself, or make the appropriate registration method calls yourself. If
   * you send the method calls yourself, call unique_name(const char*) with
   * the unique bus name you get from the bus.
   *
   * For shared connections (created with dbus_connection_open()) in a
   * multithreaded application, you can't really make the registration calls
   * yourself, because you don't know whether some other thread is also
   * registering, and the bus will kick you off if you send two registration
   * messages. (TODO: how is this done in the wrapper?)
   *
   * If you use register_bus() however, there is a lock that keeps both
   * apps from registering at the same time.
   *
   * The rule in a multithreaded app, then, is that register_bus() must be
   * used to register, or you need to have your own locks that all threads in
   * the app will respect.
   *
   * In a single-threaded application you can register by hand instead of using
   * register_bus(), as long as you check unique_name(void) to
   * see if a unique name has already been stored by another thread before you
   * send the registration messages.
   */
  bool register_bus();

  /*!
   * \brief Gets whether the connection is currently open.
   *
   * A connection may become disconnected when the remote application closes its
   * end, or exits; a connection may also be disconnected with disconnect().
   *
   * There are not separate states for "closed" and "disconnected," the two
   * terms are synonymous.
   *
   * \return true If the connection is still alive.
   */
  bool connected() const;

  /*!
   * \brief Closes a private connection, so no further data can be sent or received.
   *
   * This disconnects the transport (such as a socket) underlying the connection.
   *
   * Attempts to send messages after closing a connection are safe, but will
   * result in error replies generated locally in libdbus.
   *
   * This function does not affect the connection's reference count. It's safe
   * to close a connection more than once; all calls after the first do nothing.
   * It's impossible to "reopen" a connection, a new connection must be created.
   * This function may result in a call to the DBusDispatchStatusFunction set
   * with Private::init(), as the disconnect
   * message it generates needs to be dispatched.
   *
   * If a connection is dropped by the remote application, it will close itself.
   *
   * You must close a connection prior to releasing the last reference to the
   * connection.
   *
   * You may not close a shared connection. Connections created with
   * dbus_connection_open() or dbus_bus_get() are shared. These connections are
   * owned by libdbus, and applications should only unref them, never close them.
   * Applications can know it is safe to unref these connections because libdbus
   * will be holding a reference as long as the connection is open. Thus, either
   * the connection is closed and it is OK to drop the last reference, or the
   * connection is open and the app knows it does not have the last reference.
   *
   * Connections created with dbus_connection_open_private() or
   * dbus_bus_get_private() are not kept track of or referenced by libdbus.
   * The creator of these connections is responsible for calling
   * dbus_connection_close() prior to releasing the last reference, if the
   * connection is not already disconnected.
   *
   * \todo dbus_connection_disconnect() was removed in dbus 0.9x. Maybe this
   *       function should be renamed to close().
   */
  void disconnect();

  /*!
   * \brief Set whether _exit() should be called when the connection receives a
   *        disconnect signal.
   *
   * The call to _exit() comes after any handlers for the disconnect signal run;
   * handlers can cancel the exit by calling this function.
   *
   * By default, exit_on_disconnect is false; but for message bus connections
   * returned from dbus_bus_get() it will be toggled on by default.
   *
   * \param exit true If _exit() should be called after a disconnect signal.
   */
  void exit_on_disconnect(bool exit);

  /*!
   * \brief Blocks until the outgoing message queue is empty.
   */
  void flush();

  /*!
   * \brief Adds a message to the outgoing message queue.
   *
   * Does not block to write the message to the network; that happens
   * asynchronously. To force the message to be written, call
   * dbus_connection_flush(). Because this only queues the message, the only
   * reason it can fail is lack of memory. Even if the connection is disconnected,
   * no error will be returned.
   *
   * If the function fails due to lack of memory, it returns FALSE. The function
   * will never fail for other reasons; even if the connection is disconnected,
   * you can queue an outgoing message, though obviously it won't be sent.
   *
   * The message serial is used by the remote application to send a reply; see
   * Message::serial() or the D-Bus specification.
   *
   * \param msg The Message to write.
   * \param serial Return location for message serial, or NULL if you don't care.
   * \return true On success.
   */
  bool send(const Message &msg, unsigned int *serial = NULL);

  /*!
   * \brief Sends a message and blocks a certain time period while waiting for a reply.
   *
   * This function does not reenter the main loop, i.e. messages other than the
   * reply are queued up but not processed. This function is used to invoke
   * method calls on a remote object.
   *
   * If a normal reply is received, it is returned, and removed from the
   * incoming message queue. If it is not received, NULL is returned and the
   * error is set to DBUS_ERROR_NO_REPLY. If an error reply is received, it is
   * converted to a DBusError and returned as an error, then the reply message
   * is deleted and NULL is returned. If something else goes wrong, result is
   * set to whatever is appropriate, such as DBUS_ERROR_NO_MEMORY or DBUS_ERROR_DISCONNECTED.
   *
   * \warning While this function blocks the calling thread will not be
   *          processing the incoming message queue. This means you can end up
   *          deadlocked if the application you're talking to needs you to reply
   *          to a method. To solve this, either avoid the situation, block in a
   *          separate thread from the main connection-dispatching thread, or
   *          use PendingCall to avoid blocking.
   *
   * \param msg The Message to write.
   * \param timeout Timeout in milliseconds (omit for default).
   * \throw Error
   */
  Message send_blocking(Message &msg, int timeout = -1);

  /*!
   * \brief Queues a message to send, as with send(), but also
   *        returns a DBusPendingCall used to receive a reply to the message.
   *
   * If no reply is received in the given timeout_milliseconds, this function
   * expires the pending reply and generates a synthetic error reply (generated
   * in-process, not by the remote application) indicating that a timeout occurred.
   *
   * A PendingCall will see a reply message before any filters or registered
   * object path handlers. See Connection::Private::do_dispatch() in dbus documentation
   * for details on when handlers are run. (here: Connection::Private::do_dispatch())
   *
   * A PendingCall will always see exactly one reply message, unless it's
   * cancelled with PendingCall::cancel().
   *
   * If -1 is passed for the timeout, a sane default timeout is used. -1 is
   * typically the best value for the timeout for this reason, unless you want
   * a very short or very long timeout. There is no way to avoid a timeout
   * entirely, other than passing INT_MAX for the timeout to mean "very long
   * timeout." libdbus clamps an INT_MAX timeout down to a few hours timeout though.
   *
   * \param msg The Message to write.
   * \param timeout Timeout in milliseconds (omit for default).
   * \throw ErrorNoMemory
   */
  PendingCall send_async(Message &msg, int timeout = -1);

  void request_name(const char *name, int flags = 0);

  unsigned long sender_unix_uid(const char *sender);

  /*!
   * \brief Asks the bus whether a certain name has an owner.
   *
   * Using this can easily result in a race condition, since an owner can appear
   * or disappear after you call this.
   *
   * If you want to request a name, just request it; if you want to avoid
   * replacing a current owner, don't specify DBUS_NAME_FLAG_REPLACE_EXISTING
   * and you will get an error if there's already an owner.
   *
   * \param name The name.
   * \throw Error
   */
  bool has_name(const char *name);

  /*!
   * \brief Starts a service that will request ownership of the given name.
   *
   * The returned result will be one of be one of DBUS_START_REPLY_SUCCESS or
   * DBUS_START_REPLY_ALREADY_RUNNING if successful. Pass NULL if you don't
   * care about the result.
   *
   * The flags parameter is for future expansion, currently you should specify 0.
   *
   * It's often easier to avoid explicitly starting services, and just send a
   * method call to the service's bus name instead. Method calls start a service
   * to handle them by default unless you call dbus_message_set_auto_start() to
   * disable this behavior.
   *
   * \todo dbus_message_set_auto_start() not yet wrapped!
   */
  bool start_service(const char *name, unsigned long flags);

  const std::vector<std::string>& names();

  void set_timeout(int timeout);

  int get_timeout();

private:

  DXXAPILOCAL void init();

private:

  RefPtrI<Private> _pvt;
  int _timeout;

  friend class ObjectAdaptor; // needed in order to register object paths for a connection
};

} /* namespace DBus */

#endif//__DBUSXX_CONNECTION_H
