/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   tizspotifygraph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Spotify client graph
 *
 *
 */

#ifndef TIZSPOTIFYGRAPH_HPP
#define TIZSPOTIFYGRAPH_HPP

#include <tizgraph.hpp>
#include "tizspotifygraphfsm.hpp"

namespace tiz
{
  namespace graph
  {
    // Forward declarations
    class cmd;
    class ops;

    class spotify : public graph
    {

    public:
      spotify ();

    protected:
      ops *do_init ();
      bool dispatch_cmd (const tiz::graph::cmd *p_cmd);

    protected:
      spfsm::fsm fsm_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZSPOTIFYGRAPH_HPP
