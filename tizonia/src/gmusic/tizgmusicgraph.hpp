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
 * @file   tizgmusicgraph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Google Music client graph
 *
 *
 */

#ifndef TIZGMUSICGRAPH_HPP
#define TIZGMUSICGRAPH_HPP

#include "tizgraph.hpp"
#include "tizgmusicgraphfsm.hpp"

namespace tiz
{
  namespace graph
  {
    // Forward declarations
    class cmd;
    class ops;

    class gmusic : public graph
    {

    public:
      gmusic ();

    protected:
      ops *do_init ();
      bool dispatch_cmd (const tiz::graph::cmd *p_cmd);

    protected:
      gmfsm::fsm fsm_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZGMUSICGRAPH_HPP
