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
 * @file   tizyoutubeconfig.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Youtube client graph configuration
 *
 *
 */

#ifndef TIZYOUTUBECONFIG_HPP
#define TIZYOUTUBECONFIG_HPP

#include <string>

#include <OMX_TizoniaExt.h>

#include "tizgraphconfig.hpp"
#include "tizgraphtypes.hpp"

namespace tiz
{
  namespace graph
  {
    class youtubeconfig : public config
    {

    public:
      youtubeconfig (const std::string &api_key,
                     const tizplaylist_ptr_t &playlist, uint32_t buffer_seconds,
                     const OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE &playlist_type)
        : config (playlist, buffer_seconds),
          api_key_ (api_key),
          playlist_type_ (playlist_type)
      {
      }

      ~youtubeconfig ()
      {
      }

      std::string get_api_key () const
      {
        return api_key_;
      }

      OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE get_playlist_type () const
      {
        return playlist_type_;
      }

    protected:
      const std::string api_key_;
      const OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE playlist_type_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZYOUTUBECONFIG_HPP
