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
 * @file   tizdirblegraph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Dirble client graph
 *
 *
 */

#ifndef TIZDIRBLEGRAPH_HPP
#define TIZDIRBLEGRAPH_HPP

#include "tizservicegraph.hpp"
#include "tizgraphops.hpp"

namespace tiz
{
  namespace graph
  {
    // Forward declarations
    class ops;

    class dirble : public servicegraph
    {

    public:
      dirble ();

    protected:
      ops *do_init ();
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZDIRBLEGRAPH_HPP
