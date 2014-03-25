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
 * @file   tizhttpservconfig.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL HTTP Streaming Server graph configuration
 *
 *
 */

#ifndef TIZHTTPSERVCONFIG_H
#define TIZHTTPSERVCONFIG_H

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
                      const std::string &ip_address, const long int port)
        : config (playlist), host_ (host), addr_ (ip_address), port_ (port)
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

    protected:
      std::string host_;
      std::string addr_;
      long int port_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZHTTPSERVCONFIG_H
