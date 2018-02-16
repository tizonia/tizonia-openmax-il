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
 * @file   tizplexconfig.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Plex client graph configuration
 *
 *
 */

#ifndef TIZPLEXCONFIG_HPP
#define TIZPLEXCONFIG_HPP

#include <string>

#include <OMX_TizoniaExt.h>

#include "tizgraphtypes.hpp"
#include "tizgraphconfig.hpp"

namespace tiz
{
  namespace graph
  {
    class plexconfig : public config
    {

    public:
      plexconfig (const tizplaylist_ptr_t &playlist, const std::string &oauth_token,
                    const OMX_TIZONIA_AUDIO_SOUNDCLOUDPLAYLISTTYPE &playlist_type)
        : config (playlist),
          oauth_token_ (oauth_token),
          playlist_type_ (playlist_type)
      {
      }

      ~plexconfig ()
      {
      }

      std::string get_oauth_token () const
      {
        return oauth_token_;
      }

      OMX_TIZONIA_AUDIO_SOUNDCLOUDPLAYLISTTYPE get_playlist_type () const
      {
        return playlist_type_;
      }

    protected:
      const std::string oauth_token_;
      const OMX_TIZONIA_AUDIO_SOUNDCLOUDPLAYLISTTYPE playlist_type_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZPLEXCONFIG_HPP
