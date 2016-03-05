/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizhttpservgraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL HTTP Streaming Server graph implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_TizoniaExt.h>
#include <tizplatform.h>

#include "tizgraphutil.hpp"
#include "tizgraphcmd.hpp"
#include "tizprobe.hpp"
#include "tizhttpservconfig.hpp"
#include "tizhttpservgraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.httpserver"
#endif

namespace graph = tiz::graph;

//
// httpserver
//
graph::httpserver::httpserver ()
  : graph::graph ("httpservgraph"),
    fsm_ (boost::msm::back::states_
          << tiz::graph::hsfsm::fsm::configuring (&p_ops_)
          << tiz::graph::hsfsm::fsm::skipping (&p_ops_),
          &p_ops_)
{
}

graph::ops *graph::httpserver::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.audio_metadata_eraser.mp3");
  comp_list.push_back ("OMX.Aratelia.audio_renderer.http");

  omx_comp_role_lst_t role_list;
  role_list.push_back ("audio_metadata_eraser.mp3");
  role_list.push_back ("audio_renderer.http");

  return new httpservops (this, comp_list, role_list);
}

bool graph::httpserver::dispatch_cmd (const tiz::graph::cmd *p_cmd)
{
  assert (p_cmd);

  if (!p_cmd->kill_thread ())
  {
    if (p_cmd->evt ().type () == typeid(tiz::graph::load_evt))
    {
      // Time to start the FSM
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "Starting [%s] fsm...",
               get_graph_name ().c_str ());
      fsm_.start ();
    }

    p_cmd->inject< hsfsm::fsm >(fsm_, tiz::graph::hsfsm::pstate);

    // Check for internal errors produced during the processing of the last
    // event. If any, inject an "internal" error event. This is fatal and shall
    // terminate the state machine.
    if (OMX_ErrorNone != p_ops_->internal_error ())
    {
      fsm_.process_event (tiz::graph::err_evt (p_ops_->internal_error (),
                                               p_ops_->internal_error_msg ()));
    }

    if (fsm_.terminated_)
    {
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "[%s] fsm terminated...",
               get_graph_name ().c_str ());
    }
  }

  return p_cmd->kill_thread ();
}
