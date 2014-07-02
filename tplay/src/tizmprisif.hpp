/* -*-Mode: c++; -*- */
/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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

#include <mpris_dbus.hh>

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
                    public DBus::IntrospectableAdaptor,
                    public DBus::ObjectAdaptor
    {

    public:
      mprisif (DBus::Connection &connection,
               mpris_mediaplayer2_props_ptr_t mp2_props_ptr,
               mpris_mediaplayer2_player_props_ptr_t mp2_player_props_ptr);
      ~mprisif ();

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
      void SetPosition(const ::DBus::Path& TrackId, const int64_t& Position);
      void OpenUri(const std::string& Uri);

    private:
      mpris_mediaplayer2_props_ptr_t props_ptr_;
      mpris_mediaplayer2_player_props_ptr_t player_props_ptr_;

    };
  }  // namespace control
}  // namespace tiz

#endif  // TIZMPRISIF_HPP
