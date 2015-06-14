/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   tizrmd.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Resource Manager Daemon
 *
 *
 */

#ifndef TIZRMD_HPP
#define TIZRMD_HPP

#include <string.h>
#include <deque>
#include <map>
#include <string>

#include <dbus-c++/dbus.h>

#include <tizrmd-dbus.hh>

#include "tizrmdb.hpp"
#include "tizrmwaiter.hpp"
#include "tizrmpreemptor.hpp"
#include "tizrmowner.hpp"

class tizrmd : public com::aratelia::tiz::tizrmif_adaptor,
               public DBus::IntrospectableAdaptor,
               public DBus::ObjectAdaptor

{

public:
  tizrmd (DBus::Connection &connection, char const *ap_dbname);
  ~tizrmd ();

  /**
   * \brief RM API for OMX_StateLoaded to OMX_StateIdle transitions where RM is
   * present
   *
   * @param rid The resource identifier
   * @param quantity The amount of resource required
   * @param cname The OpenMAX IL component's name
   * @param uuid The component's uuid
   * @param grpid The component's group
   * @param pri The component's group priority
   *
   * @return An tiz_rm_error_t error code
   */
  int32_t acquire (const uint32_t &rid, const uint32_t &quantity,
                   const std::string &cname, const std::vector< uint8_t > &uuid,
                   const uint32_t &grpid, const uint32_t &pri);

  /**
   * \brief RM API for OMX_StateIdle to OMX_StateLoaded transitions where RM is
   * present
   *
   * @param rid The resource identifier
   * @param quantity The amount of resource to be released
   * @param cname The OpenMAX IL component name
   * @param uuid The component's uuid
   * @param grpid The component's group
   * @param pri The component's group priority
   *
   * @return An tiz_rm_error_t error code
   */
  int32_t release (const uint32_t &rid, const uint32_t &quantity,
                   const std::string &cname, const std::vector< uint8_t > &uuid,
                   const uint32_t &grpid, const uint32_t &pri);

  /**
   * \brief RM API for OMX_StateWaitForResources
   *
   * @param rid The resource identifier
   * @param quantity The amount of resource required
   * @param cname The OpenMAX IL component's name
   * @param uuid The component's uuid
   * @param grpid The component's group
   * @param pri The component's group priority
   *
   * @return An tiz_rm_error_t error code
   */
  int32_t wait (const uint32_t &rid, const uint32_t &quantity,
                const std::string &cname, const std::vector< uint8_t > &uuid,
                const uint32_t &grpid, const uint32_t &pri);

  /**
   * \brief RM API for
   *
   * @param rid The resource identifier
   * @param quantity The amount of resource required
   * @param cname The OpenMAX IL component's name
   * @param uuid The component's uuid
   * @param grpid The component's group
   * @param pri The component's group priority
   *
   * @return An tiz_rm_error_t error code
   */
  int32_t cancel_wait (const uint32_t &rid, const uint32_t &quantity,
                       const std::string &cname,
                       const std::vector< uint8_t > &uuid,
                       const uint32_t &grpid, const uint32_t &pri);

  /**
   * \brief RM API for
   *
   * @param rid The resource identifier
   * @param quantity The amount of resource required
   * @param cname The OpenMAX IL component's name
   * @param uuid The component's uuid
   * @param grpid The component's group
   * @param pri The component's group priority
   *
   * @return An tiz_rm_error_t error code
   */
  int32_t preemption_conf (const uint32_t &rid, const uint32_t &quantity,
                           const std::string &cname,
                           const std::vector< uint8_t > &uuid,
                           const uint32_t &grpid, const uint32_t &pri);

  /**
   * \brief Release all currently allocated resources and cancel all allocation
   * requests
   *
   * @param cname The OpenMAX IL component's name
   * @param uuid The component's uuid
   *
   * @return A tiz_rm_error_t error code
   */
  int32_t relinquish_all (const std::string &cname,
                          const std::vector< unsigned char > &uuid);

private:
  typedef std::deque< tizrmwaiter > waitlist_t;
  typedef std::map< tizrmowner, tizrmpreemptor > preemptlist_t;

private:
  tizrmdb rmdb_;
  waitlist_t waiters_;
  preemptlist_t preemptions_;
};

#endif  // TIZRMD_HPP
