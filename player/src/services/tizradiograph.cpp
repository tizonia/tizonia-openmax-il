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
 * @file   tizradiograph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Radiograph streaming service graph implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizgraphcmd.hpp"
#include "tizgraphops.hpp"

#include "tizradiographfsm.hpp"
#include "tizradiograph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.radiograph"
#endif

namespace graph = tiz::graph;
namespace radiofsm = tiz::graph::radiofsm;

//
// radiograph
//
graph::radiograph::radiograph (const std::string &graph_name)
  : graph::graph (graph_name),
    fsm_ (new tiz::graph::radiofsm::fsm (
        boost::msm::back::states_
        << tiz::graph::radiofsm::fsm::auto_detecting (&p_ops_)
        << tiz::graph::radiofsm::fsm::updating_graph (&p_ops_)
        << tiz::graph::radiofsm::fsm::reconfiguring_tunnel_0 (&p_ops_)
        << tiz::graph::radiofsm::fsm::reconfiguring_tunnel_1 (&p_ops_)
        << tiz::graph::radiofsm::fsm::skipping (&p_ops_),
        &p_ops_))

{
}

graph::radiograph::~radiograph ()
{
  delete (boost::any_cast< radiofsm::fsm * >(fsm_));
}

bool graph::radiograph::dispatch_cmd (const tiz::graph::cmd *p_cmd)
{
  assert (p_ops_);
  assert (p_cmd);

  if (!p_cmd->kill_thread ())
  {
    radiofsm::fsm *p_fsm = boost::any_cast< radiofsm::fsm * >(fsm_);
    assert (p_fsm);

    if (p_cmd->evt ().type () == typeid(tiz::graph::load_evt))
    {
      // Time to start the FSM
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "Starting [%s] fsm...",
               get_graph_name ().c_str ());
      p_fsm->start ();
    }

    p_cmd->inject< radiofsm::fsm >(*p_fsm, tiz::graph::radiofsm::pstate);

    // Check for internal errors produced during the processing of the last
    // event. If any, inject an "internal" error event. This is fatal and shall
    // terminate the state machine.
    if (OMX_ErrorNone != p_ops_->internal_error ())
    {
      p_fsm->process_event (tiz::graph::err_evt (
          p_ops_->internal_error (), p_ops_->internal_error_msg ()));
    }

    if (p_fsm->terminated_)
    {
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "[%s] fsm terminated...",
               get_graph_name ().c_str ());
    }
  }

  return p_cmd->kill_thread ();
}

