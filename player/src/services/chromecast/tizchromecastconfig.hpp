/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tizchromecastconfig.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Chromecast rendering graph configuration
 *
 *
 */

#ifndef TIZCHROMECASTCONFIG_HPP
#define TIZCHROMECASTCONFIG_HPP

#include <string>

#include <OMX_TizoniaExt.h>

#include "tizgraphconfig.hpp"
#include "tizgraphtypes.hpp"

namespace tiz
{
  namespace graph
  {
    class chromecastconfig : public config
    {
    public:
      enum service_config_type_t
        {
         ConfigHttpStreaming,
         ConfigGoogleMusic,
         ConfigSoundCloud,
//          ConfigTunein,
         ConfigYouTube,
         ConfigPlex,
         ConfigUnknown
        };

    public:
      chromecastconfig (const std::string &cc_name_or_ip,
                        const tizgraphconfig_ptr_t &service_config,
                        const service_config_type_t service_config_type)
        : config (service_config->get_playlist (), 0),
          name_or_ip_ (cc_name_or_ip),
          service_config_ (service_config),
          service_config_type_(service_config_type)
      {
      }

      ~chromecastconfig ()
      {
      }

      std::string get_name_or_ip () const
      {
        return name_or_ip_;
      }

      tizgraphconfig_ptr_t get_service_config () const
      {
        return service_config_;
      }

      service_config_type_t get_service_config_type () const
      {
        return service_config_type_;
      }

    protected:
      const std::string name_or_ip_;
      tizgraphconfig_ptr_t service_config_;
      service_config_type_t service_config_type_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZCHROMECASTCONFIG_HPP
