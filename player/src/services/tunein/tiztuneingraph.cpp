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
 * @file   tiztuneingraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tunein streaming service graph implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizgraphcmd.hpp"
#include "tizgraphops.hpp"

#include "tiztuneingraphfsm.hpp"
#include "tiztuneingraphops.hpp"
#include "tiztuneingraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.tunein"
#endif

namespace graph = tiz::graph;
namespace tuneinfsm = tiz::graph::tuneinfsm;

//
// tunein
//
graph::tunein::tunein ()
  : graph::graph ("tuneingraph"),
    fsm_ (new tiz::graph::tuneinfsm::fsm (
        boost::msm::back::states_
        << tiz::graph::tuneinfsm::fsm::auto_detecting (&p_ops_)
        << tiz::graph::tuneinfsm::fsm::updating_graph (&p_ops_)
        << tiz::graph::tuneinfsm::fsm::reconfiguring_tunnel_0 (&p_ops_)
        << tiz::graph::tuneinfsm::fsm::reconfiguring_tunnel_1 (&p_ops_)
        << tiz::graph::tuneinfsm::fsm::skipping (&p_ops_),
        &p_ops_))

{
}

graph::tunein::~tunein ()
{
  delete (boost::any_cast< tuneinfsm::fsm * >(fsm_));
}

graph::ops *graph::tunein::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.audio_source.http");

  omx_comp_role_lst_t role_list;
  role_list.push_back ("audio_source.http.tunein");

  return new tuneinops (this, comp_list, role_list);
}

bool graph::tunein::dispatch_cmd (const tiz::graph::cmd *p_cmd)
{
  assert (p_ops_);
  assert (p_cmd);

  if (!p_cmd->kill_thread ())
  {
    tuneinfsm::fsm *p_fsm = boost::any_cast< tuneinfsm::fsm * >(fsm_);
    assert (p_fsm);

    if (p_cmd->evt ().type () == typeid(tiz::graph::load_evt))
    {
      // Time to start the FSM
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "Starting [%s] fsm...",
               get_graph_name ().c_str ());
      p_fsm->start ();
    }

    p_cmd->inject< tuneinfsm::fsm >(*p_fsm, tiz::graph::tuneinfsm::pstate);

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

