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
 * @file   tizgraphfactory.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph factory
 *
 *
 */

#ifndef TIZGRAPHFACTORY_HPP
#define TIZGRAPHFACTORY_HPP

#include <string>

#include <boost/utility.hpp>

#include <OMX_Types.h>

#include "tizgraphtypes.hpp"

namespace tiz
{
  namespace graph
  {
    class factory : boost::noncopyable
    {

    public:
      static tizgraph_ptr_t create_graph (const std::string &uri);
      static std::string coding_type (const std::string &uri);
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPHFACTORY_HPP
