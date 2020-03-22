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
 * @file   tiziheartconfig.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Iheart client graph configuration
 *
 *
 */

#ifndef TIZIHEARTCONFIG_HPP
#define TIZIHEARTCONFIG_HPP

#include <string>

#include <OMX_TizoniaExt.h>

#include "tizgraphtypes.hpp"
#include "tizgraphconfig.hpp"

namespace tiz
{
  namespace graph
  {
    class iheartconfig : public config
    {

    public:
      iheartconfig (const tizplaylist_ptr_t &playlist, uint32_t buffer_seconds,
                    const std::string &api_key,
                    const OMX_TIZONIA_AUDIO_IHEARTPLAYLISTTYPE &playlist_type)
        : config (playlist, buffer_seconds),
          api_key_ (api_key),
          playlist_type_ (playlist_type)
      {
      }

      ~iheartconfig ()
      {
      }

      std::string get_api_key () const
      {
        return api_key_;
      }

      OMX_TIZONIA_AUDIO_IHEARTPLAYLISTTYPE get_playlist_type () const
      {
        return playlist_type_;
      }

    protected:
      const std::string api_key_;
      const OMX_TIZONIA_AUDIO_IHEARTPLAYLISTTYPE playlist_type_;

    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZIHEARTCONFIG_HPP
