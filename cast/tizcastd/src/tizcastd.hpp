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
 * @brief  Tizonia cast daemon
 *
 *
 */

#ifndef TIZCASTD_HPP
#define TIZCASTD_HPP

/**
* @defgroup tizcastd 'tizcastd' : Tizonia's cast daemon process.
*
* This module implements a D-Bus-based daemon to interface with Chromecast devices.
*
* @ingroup cast
*/

#include <string.h>
#include <string>

#include <dbus-c++/dbus.h>

#include <tizcastd-dbus.hh>

// Forward declaration
namespace tiz
{
  namespace castmgr
  {
    class mgr;
  }
}

class tizcastd : public com::aratelia::tiz::tizcastif_adaptor,
                 public Tiz::DBus::IntrospectableAdaptor,
                 public Tiz::DBus::ObjectAdaptor

{

public:
  tizcastd (Tiz::DBus::Connection &connection);
  ~tizcastd ();

  /**
   * @brief Connect to a Chromecast device.
   *
   * @param name_or_ip The name or ip address of the Chromecast device.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t connect (const std::string &name_or_ip);

  /**
   * @brief Disconnect an existing connection to a Chromecast device.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t disconnect ();

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
  tiz::castmgr::mgr * p_cast_mgr_;
  std::string cc_name_or_ip_;
};

#endif  // TIZCASTD_HPP
