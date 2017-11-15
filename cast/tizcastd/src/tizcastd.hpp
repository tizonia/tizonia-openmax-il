/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   tizcastd.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Chromecast daemon
 *
 *
 */

#ifndef TIZCASTD_HPP
#define TIZCASTD_HPP

/**
* @defgroup tizcastd 'tizcastd' : Tizonia's Chromecast daemon process.
*
* This module implements a D-Bus-based daemon to interface with Chromecast devices.
*
* @ingroup cast
*/

#include <string.h>
#include <string>

#include <dbus-c++/dbus.h>

#include <tizchromecast_c.h>

#include <tizcastd-dbus.hh>

class tizcastd : public com::aratelia::tiz::tizcastif_adaptor,
                 public DBus::IntrospectableAdaptor,
                 public DBus::ObjectAdaptor

{

public:
  tizcastd (DBus::Connection &connection, char const *ap_dbname);
  ~tizcastd ();

  /**
   * @brief Load a stream URL on a Chromecast device.
   *
   * @param url The stream URL.
   * @param mime_type The MIME content type of the stream.
   * @param title The stream title
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t load_url (const std::string &url, const std::string &mime_type,
                    const std::string &title);

  /**
   * @brief Play a previously loaded stream URL.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t play ();

  /**
   * @brief Stop the stream.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t stop ();

  /**
   * @brief Pause the stream.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t pause ();

  /**
   * @brief Increase the playback volume.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t volume_up ();

  /**
   * @brief Decrease the playback volume.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t volume_down ();

  /**
   * @brief Mute playback.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t mute ();

  /**
   * @brief Unmute playback.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t unmute ();

private:
  tiz_chromecast_t *p_cc_;
};

#endif  // TIZCASTD_HPP
