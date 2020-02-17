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
 * @file   tizhttpservgraph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  HTTP Streaming Server graph
 *
 *
 */

#ifndef TIZHTTPSERVGRAPH_HPP
#define TIZHTTPSERVGRAPH_HPP

#include "tizgraph.hpp"
#include "tizhttpservgraphfsm.hpp"

namespace tiz
{
  namespace graph
  {
    // Forward declarations
    class cmd;
    class ops;

    class httpserver : public graph
    {

    public:
      httpserver ();

    protected:
      ops *do_init ();
      bool dispatch_cmd (const tiz::graph::cmd *p_cmd);

    protected:
      hsfsm::fsm fsm_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZHTTPSERVGRAPH_HPP
