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
 * @file   tizdeezerconfig.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Deezer client graph configuration
 *
 *
 */

#ifndef TIZDEEZERCONFIG_HPP
#define TIZDEEZERCONFIG_HPP

#include <string>

#include <OMX_TizoniaExt.h>

#include "tizgraphtypes.hpp"
#include "tizgraphconfig.hpp"

namespace tiz
{
  namespace graph
  {
    class deezerconfig : public config
    {

    public:
      deezerconfig (const tizplaylist_ptr_t &playlist, const std::string &user,
                    const OMX_TIZONIA_AUDIO_DEEZERPLAYLISTTYPE playlist_type)
        : config (playlist),
          user_ (user),
          playlist_type_ (playlist_type)
      {
      }

      ~deezerconfig ()
      {
      }

      std::string get_user_name () const
      {
        return user_;
      }

      OMX_TIZONIA_AUDIO_DEEZERPLAYLISTTYPE get_playlist_type () const
      {
        return playlist_type_;
      }

    protected:
      const std::string user_;
      const OMX_TIZONIA_AUDIO_DEEZERPLAYLISTTYPE playlist_type_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZDEEZERCONFIG_HPP
