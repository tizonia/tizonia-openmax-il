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
 * This module implements a D-Bus-based daemon to interface with Chromecast
 * devices.
 *
 * @ingroup cast
 */

#include <string.h>
#include <string>
#include <vector>

#include <dbus-c++/dbus.h>

#include <tizcastd-dbus.hh>

// Forward declaration
namespace tiz
{
  namespace cast
  {
    class worker;
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
  int32_t connect (const std::vector< uint8_t > &uuid,
                   const std::string &name_or_ip);

  /**
   * @brief Disconnect an existing connection to a Chromecast device.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t disconnect (const std::vector< uint8_t > &uuid);

  /**
   * @brief Load a stream URL on a Chromecast device.
   *
   * @param url The stream URL.
   * @param mime_type The MIME content type of the stream.
   * @param title The stream title
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t load_url (const std::vector< uint8_t > &uuid, const std::string &url,
                    const std::string &mime_type, const std::string &title,
                    const std::string &album_art);

  /**
   * @brief Play a previously loaded stream URL.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t play (const std::vector< uint8_t > &uuid);

  /**
   * @brief Stop the stream.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t stop (const std::vector< uint8_t > &uuid);

  /**
   * @brief Pause the stream.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t pause (const std::vector< uint8_t > &uuid);

  /**
   * @brief Increase the playback volume.
   *
   * @param volume The volume level (0-100).
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t volume_set (const std::vector< uint8_t > &uuid,
                      const int32_t &volume);

  /**
   * @brief Increase the playback volume.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t volume_up (const std::vector< uint8_t > &uuid);

  /**
   * @brief Decrease the playback volume.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t volume_down (const std::vector< uint8_t > &uuid);

  /**
   * @brief Mute playback.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t mute (const std::vector< uint8_t > &uuid);

  /**
   * @brief Unmute playback.
   *
   * @return A tiz_cast_error_t error code
   */
  int32_t unmute (const std::vector< uint8_t > &uuid);

  void cast_status_forwarder (const std::string &name_or_ip,
                              const uint32_t &status, const int32_t &volume);

  void media_status_forwarder (const std::string &name_or_ip,
                               const uint32_t &status, const int32_t &volume);

  void error_status_forwarder (const std::string &name_or_ip,
                               const uint32_t &status,
                               const std::string &error_str);

private:
  tiz::cast::worker * p_worker_;
};

#endif  // TIZCASTD_HPP
