/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @brief  OpenMAX IL HTTP Streaming Server graph configuration
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
                      const std::string &pass)
        : config (playlist), user_ (user), pass_ (pass)
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

    protected:
      const std::string user_;
      const std::string pass_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZSPOTIFYCONFIG_HPP
