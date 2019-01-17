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
 * @file   tizhttpservconfig.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL HTTP Streaming Server graph configuration
 *
 *
 */

#ifndef TIZHTTPSERVCONFIG_HPP
#define TIZHTTPSERVCONFIG_HPP

#include <string>

#include "tizgraphtypes.hpp"
#include "tizgraphconfig.hpp"

namespace tiz
{
  namespace graph
  {
    class httpservconfig : public config
    {

    public:
      httpservconfig (const tizplaylist_ptr_t &playlist, const std::string &host,
                      const std::string &ip_address, const long int port,
                      const std::vector<int> &sampling_rate_list,
                      const std::vector< std::string > &bitrate_mode_list,
                      const std::string &station_name,
                      const std::string &station_genre,
                      const bool &icy_metadata_enabled)
        : config (playlist), host_ (host), addr_ (ip_address), port_ (port),
          sampling_rate_list_ (sampling_rate_list), bitrate_mode_list_ (bitrate_mode_list),
          station_name_ (station_name), station_genre_ (station_genre),
          icy_metadata_enabled_ (icy_metadata_enabled)
      {
      }

      ~httpservconfig ()
      {
      }

      std::string get_addr () const
      {
        return addr_;
      }

      std::string get_host_name () const
      {
        return host_;
      }

      long int get_port () const
      {
        return port_;
      }

      const std::vector<int> &get_sampling_rates () const
      {
        return sampling_rate_list_;
      }

      const std::vector< std::string > &get_bitrate_modes () const
      {
        return bitrate_mode_list_;
      }

      std::string get_station_name () const
      {
        return station_name_;
      }

      std::string get_station_genre () const
      {
        return station_genre_;
      }

      bool get_icy_metadata_enabled () const
      {
        return icy_metadata_enabled_;
      }

    protected:
      const std::string host_;
      const std::string addr_;
      const long int port_;
      const std::vector<int> sampling_rate_list_;
      const std::vector< std::string > bitrate_mode_list_;
      const std::string station_name_;
      const std::string station_genre_;
      const bool icy_metadata_enabled_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZHTTPSERVCONFIG_HPP
