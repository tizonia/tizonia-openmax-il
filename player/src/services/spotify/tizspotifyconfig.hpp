/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   tizspotifyconfig.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Spotify client graph configuration
 *
 *
 */

#ifndef TIZSPOTIFYCONFIG_HPP
#define TIZSPOTIFYCONFIG_HPP

#include <string>

#include "tizgraphtypes.hpp"
#include "tizgraphconfig.hpp"

namespace tiz
{
  namespace graph
  {
    class spotifyconfig : public config
    {

    public:
      spotifyconfig (const tizplaylist_ptr_t &playlist, const std::string &user,
                     const std::string &pass,
                     const std::string &proxy_server,
                     const std::string &proxy_user,
                     const std::string &proxy_password,
                     const OMX_TIZONIA_AUDIO_SPOTIFYPLAYLISTTYPE playlist_type,
                     const std::string &owner,
                     const bool recover_lost_token,
                     const bool allow_explicit_tracks,
                     const uint32_t preferred_bitrate)
      : config (playlist, 0),
        user_ (user),
        pass_ (pass),
        proxy_server_ (proxy_server),
        proxy_user_ (proxy_user),
        proxy_password_ (proxy_password),
        playlist_type_ (playlist_type),
        owner_ (owner),
        recover_lost_token_ (recover_lost_token),
        allow_explicit_tracks_ (allow_explicit_tracks),
        preferred_bitrate_ (preferred_bitrate)
      {
      }

      ~spotifyconfig ()
      {
      }

      std::string get_user_name () const
      {
        return user_;
      }

      std::string get_user_pass () const
      {
        return pass_;
      }

      std::string get_proxy_server () const
      {
        return proxy_server_;
      }

      std::string get_proxy_user () const
      {
        return proxy_user_;
      }

      std::string get_proxy_password () const
      {
        return proxy_password_;
      }

      OMX_TIZONIA_AUDIO_SPOTIFYPLAYLISTTYPE get_playlist_type () const
      {
        return playlist_type_;
      }

      std::string get_playlist_owner () const
      {
        return owner_;
      }

      bool get_recover_lost_token () const
      {
        return recover_lost_token_;
      }

      bool get_allow_explicit_tracks () const
      {
        return allow_explicit_tracks_;
      }

      uint32_t get_preferred_bitrate () const
      {
        return preferred_bitrate_;
      }

    protected:
      const std::string user_;
      const std::string pass_;
      const std::string proxy_server_;
      const std::string proxy_user_;
      const std::string proxy_password_;
      const OMX_TIZONIA_AUDIO_SPOTIFYPLAYLISTTYPE playlist_type_;
      const std::string owner_;
      const bool recover_lost_token_;
      const bool allow_explicit_tracks_;
      const uint32_t preferred_bitrate_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZSPOTIFYCONFIG_HPP
