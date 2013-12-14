/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphfactory.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph factory
 *
 *
 */

#ifndef TIZGRAPHFACTORY_H
#define TIZGRAPHFACTORY_H

#include "tizgraph.h"

#include <OMX_Types.h>

#include <string>

class tizgraphfactory
{

public:

  static tizgraph_ptr_t create_graph(const std::string &uri);
  static std::string coding_type(const std::string &uri);

protected:

   tizgraphfactory(int graph_size);

};

#endif // TIZGRAPHFACTORY_H
