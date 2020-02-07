/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphconfig.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph configuration base class
 *
 *
 */

#ifndef TIZGRAPHCONFIG_HPP
#define TIZGRAPHCONFIG_HPP

#include <stdint.h>

#include <boost/shared_ptr.hpp>

#include "tizgraphtypes.hpp"

namespace tiz
{
  namespace graph
  {

    class config
    {

    public:
      explicit config (const tizplaylist_ptr_t &playlist, const uint32_t buffer_seconds)
        : playlist_ (playlist), buffer_seconds_ (buffer_seconds)
      {
      }

      virtual ~config ()
      {
      }

      tizplaylist_ptr_t get_playlist () const
      {
        return playlist_;
      }

      uint32_t get_buffer_seconds () const
      {
        return buffer_seconds_;
      }

    protected:
      tizplaylist_ptr_t playlist_;
      uint32_t buffer_seconds_;
    };

  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPHCONFIG_HPP
