/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   tizmprisif.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  The MPRIS interface DBUS adaptor
 *
 *
 */

#ifndef TIZMPRISIF_HPP
#define TIZMPRISIF_HPP

#include <dbus-c++/dbus.h>

#include <mpris_dbus.hpp>

#include "tizmprisprops.hpp"
#include "tizmpriscbacks.hpp"

namespace tiz
{
  namespace control
  {

    /**
     *  @class mprisif
     *  @brief The MPRIS interface DBUS adaptor.
     *
     *  The MPRIS manager instantiates this class to process the MPRIS protocol
     *  messages received on the DBUS channel.
     */
    class mprisif : public org::mpris::MediaPlayer2_adaptor,
                    public org::mpris::MediaPlayer2::Player_adaptor,
                    public Tiz::DBus::IntrospectableAdaptor,
                    public Tiz::DBus::PropertiesAdaptor,
                    public Tiz::DBus::ObjectAdaptor
    {
    public:
      static const char * TIZONIA_MPRIS_OBJECT_PATH;

    public:
      mprisif (Tiz::DBus::Connection &connection,
               mpris_mediaplayer2_props_t props,
               mpris_mediaplayer2_player_props_t player_props,
               mpris_callbacks_t cbacks);

      void on_set_property
      (Tiz::DBus::InterfaceAdaptor &interface, const std::string &property, const Tiz::DBus::Variant &value);

      void UpdateProps (const mpris_mediaplayer2_props_t &props);
      void UpdatePlayerProps (const mpris_mediaplayer2_player_props_t &props);

      /* Methods exported by the MediaPlayer2_adaptor */
      void Raise();
      void Quit();

      /* Methods exported by the Player_adaptor */
      void Next();
      void Previous();
      void Pause();
      void PlayPause();
      void Stop();
      void Play();
      void Seek(const int64_t& Offset);
      void SetPosition(const ::Tiz::DBus::Path& TrackId, const int64_t& Position);
      void OpenUri(const std::string& Uri);

    private:
      mpris_mediaplayer2_props_t props_;
      mpris_mediaplayer2_player_props_t player_props_;
      mpris_callbacks_t cbacks_;
    };
  }  // namespace control
}  // namespace tiz

#endif  // TIZMPRISIF_HPP
