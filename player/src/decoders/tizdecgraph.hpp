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
 * @file   tizgraph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL decoder graph
 *
 *
 */

#ifndef TIZDECGRAPH_HPP
#define TIZDECGRAPH_HPP

#include <boost/any.hpp>

#include "tizgraph.hpp"
#include "tizgraphops.hpp"

namespace tiz
{
  namespace graph
  {
    // Forward declarations
    class cmd;

    class decoder : public graph
    {

    public:
      explicit decoder (const std::string &graph_name);
      ~decoder ();

    protected:
      bool dispatch_cmd (const tiz::graph::cmd *p_cmd);

    protected:
      boost::any fsm_;
    };

    class decops : public ops
    {
    public:
      decops (graph *p_graph, const omx_comp_name_lst_t &comp_lst,
              const omx_comp_role_lst_t &role_lst);

    public:
      void do_disable_comp_ports (const int comp_id, const int port_id);
      bool is_disabled_evt_required () const;
    };

  }  // namespace graph
}  // namespace tiz

#endif  // TIZDECGRAPH_HPP
