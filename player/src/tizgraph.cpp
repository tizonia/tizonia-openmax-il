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
 * @file   tizgraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph base class implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizomxutil.hpp>
#include <tizplatform.h>
#include <tizmacros.h>
#include <OMX_Core.h>
#include <OMX_Component.h>

#include "tizgraphmgr.hpp"
#include "tizgraphcmd.hpp"
#include "tizgraphfsm.hpp"
#include "tizgraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph"
#endif

#define TIZ_GRAPH_QUEUE_MAX_ITEMS 30

namespace graph = tiz::graph;

void *graph::thread_func (void *p_arg)
{
  graph *p_graph = static_cast< graph * >(p_arg);
  void *p_data = NULL;
  bool done = false;

  assert (p_graph);

  (void)tiz_thread_setname (&(p_graph->thread_),
                            (char *)p_graph->get_graph_name ().c_str ());
  tiz_check_omx_ret_null (tiz_sem_post (&(p_graph->sem_)));

  while (!done)
  {
    tiz_check_omx_ret_null (tiz_queue_receive (p_graph->p_queue_, &p_data));

    assert (p_data);

    cmd *p_cmd = static_cast< cmd * >(p_data);
    done = p_graph->dispatch_cmd (p_cmd);

    delete p_cmd;
  }

  tiz_check_omx_ret_null (tiz_sem_post (&(p_graph->sem_)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] thread exiting...",
           p_graph->get_graph_name ().c_str ());

  return NULL;
}

//
// tizgraph
//
graph::graph::graph (const std::string &graph_name)
  : graph_name_ (graph_name),
    cback_handler_ (this),
    p_mgr_ (NULL),
    p_ops_ (NULL),
    thread_ (0),
    mutex_ (),
    sem_ (),
    p_queue_ (NULL)
{
}

graph::graph::~graph ()
{
}

OMX_ERRORTYPE
graph::graph::init ()
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");

  // Init command queue infrastructure
  tiz_check_omx_ret_oom (init_cmd_queue ());

  // Init this graph's operations using the do_init template method
  tiz_check_null_ret_oom ((p_ops_ = do_init ()) != NULL);

  // Create the graph's thread
  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (
      tiz_thread_create (&thread_, 0, 0, thread_func, this));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  // Let's wait until the graph's thread is ready to receive requests
  tiz_check_omx_ret_oom (tiz_sem_wait (&sem_));

  return OMX_ErrorNone;
}

void graph::graph::deinit ()
{
  void *p_result = NULL;

  // Kill the graph thread
  const bool kill_thread = true;
  const int anyvalue = 0;
  static_cast< void >(post_cmd (new tiz::graph::cmd (anyvalue, kill_thread)));

  TIZ_LOG (TIZ_PRIORITY_NOTICE, "[%s] Waiting for thread exit...",
           get_graph_name ().c_str ());
  static_cast< void >(tiz_sem_wait (&sem_));

  // Wait for thread to join
  static_cast< void >(tiz_thread_join (&thread_, &p_result));
  deinit_cmd_queue ();

  delete p_ops_;
  p_ops_ = NULL;
}

OMX_ERRORTYPE
graph::graph::load ()
{
  return post_cmd (new tiz::graph::cmd (tiz::graph::load_evt ()));
}

OMX_ERRORTYPE
graph::graph::execute (const tizgraphconfig_ptr_t config)
{
  return post_cmd (new tiz::graph::cmd (tiz::graph::execute_evt (config)));
}

OMX_ERRORTYPE
graph::graph::pause ()
{
  return post_cmd (new tiz::graph::cmd (tiz::graph::pause_evt ()));
}

OMX_ERRORTYPE
graph::graph::seek ()
{
  return post_cmd (new tiz::graph::cmd (tiz::graph::seek_evt ()));
}

OMX_ERRORTYPE
graph::graph::skip (const int jump)
{
  return post_cmd (new tiz::graph::cmd (tiz::graph::skip_evt (jump)));
}

OMX_ERRORTYPE
graph::graph::volume_step (const int step)
{
  return post_cmd (new tiz::graph::cmd (tiz::graph::volume_step_evt (step)));
}

OMX_ERRORTYPE
graph::graph::volume (const double vol)
{
  return post_cmd (new tiz::graph::cmd (tiz::graph::volume_evt (vol)));
}

OMX_ERRORTYPE
graph::graph::mute ()
{
  return post_cmd (new tiz::graph::cmd (tiz::graph::mute_evt ()));
}

OMX_ERRORTYPE
graph::graph::stop ()
{
  return post_cmd (new tiz::graph::cmd (tiz::graph::stop_evt ()));
}

void graph::graph::unload ()
{
  post_cmd (new tiz::graph::cmd (tiz::graph::unload_evt ()));
}

void graph::graph::omx_evt (const omx_event_info &evt_info)
{
  if (p_ops_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "[%s] : [%s] -> %s",
             get_graph_name ().c_str (),
             p_ops_->handle2name (evt_info.component_).c_str (),
             evt_info.to_string ().c_str ());
    TIZ_PRINTF_DBG_RED ("[%s] : [%s] -> %s\n",
                        get_graph_name ().c_str (),
                        p_ops_->handle2name (evt_info.component_).c_str (),
                        evt_info.to_string ().c_str ());

    if (evt_info.event_ == OMX_EventCmdComplete
        && static_cast< OMX_COMMANDTYPE > (evt_info.ndata1_)
               == OMX_CommandStateSet)
    {
      OMX_ERRORTYPE error
          = static_cast< OMX_ERRORTYPE >(*((int *)&((evt_info.pEventData_))));
      post_cmd (new tiz::graph::cmd (tiz::graph::omx_trans_evt (
          evt_info.component_, static_cast< OMX_STATETYPE >(evt_info.ndata2_),
          error)));
    }
    else if (evt_info.event_ == OMX_EventCmdComplete
             && static_cast< OMX_COMMANDTYPE > (evt_info.ndata1_)
                    == OMX_CommandPortDisable)
    {
      OMX_ERRORTYPE error
          = static_cast< OMX_ERRORTYPE >(*((int *)&((evt_info.pEventData_))));
      post_cmd (new tiz::graph::cmd (tiz::graph::omx_port_disabled_evt (
          evt_info.component_, evt_info.ndata2_, error)));
    }
    else if (evt_info.event_ == OMX_EventCmdComplete
             && static_cast< OMX_COMMANDTYPE > (evt_info.ndata1_)
                    == OMX_CommandPortEnable)
    {
      OMX_ERRORTYPE error
          = static_cast< OMX_ERRORTYPE > (*((int *)&((evt_info.pEventData_))));
      post_cmd (new tiz::graph::cmd (tiz::graph::omx_port_enabled_evt (
          evt_info.component_, evt_info.ndata2_, error)));
    }
    else if (evt_info.event_ == OMX_EventError)
    {
      post_cmd (new tiz::graph::cmd (tiz::graph::omx_err_evt (
          evt_info.component_, static_cast< OMX_ERRORTYPE >(evt_info.ndata1_),
          evt_info.ndata2_)));
    }
    else if (evt_info.event_ == OMX_EventPortSettingsChanged)
    {
      post_cmd (new tiz::graph::cmd (tiz::graph::omx_port_settings_evt (
          evt_info.component_, evt_info.ndata1_,
          static_cast< OMX_INDEXTYPE >(evt_info.ndata2_))));
    }
    else if (evt_info.event_ == OMX_EventIndexSettingChanged)
    {
      post_cmd (new tiz::graph::cmd (tiz::graph::omx_index_setting_evt (
          evt_info.component_, evt_info.ndata1_,
          static_cast< OMX_INDEXTYPE >(evt_info.ndata2_))));
    }
    else if (evt_info.event_ == OMX_EventPortFormatDetected)
    {
      post_cmd (new tiz::graph::cmd (
          tiz::graph::omx_format_detected_evt (evt_info.component_)));
    }
    else if (evt_info.event_ == OMX_EventBufferFlag)
    {
      post_cmd (new tiz::graph::cmd (tiz::graph::omx_eos_evt (
          evt_info.component_, evt_info.ndata1_, evt_info.ndata2_)));
    }
    else
    {
      post_cmd (new tiz::graph::cmd (tiz::graph::omx_evt (
          evt_info.component_, evt_info.event_, evt_info.ndata1_,
          evt_info.ndata2_, evt_info.pEventData_)));
    }
  }
}

void graph::graph::set_manager (tiz::graphmgr::mgr *ap_mgr)
{
  assert (ap_mgr);
  p_mgr_ = ap_mgr;
}

void graph::graph::graph_loaded ()
{
  if (p_mgr_)
  {
    p_mgr_->graph_loaded ();
  }
}

void graph::graph::graph_execd ()
{
  if (p_mgr_)
  {
    p_mgr_->graph_execd ();
  }
}

void graph::graph::graph_stopped ()
{
  if (p_mgr_)
  {
    p_mgr_->graph_stopped ();
  }
}

void graph::graph::graph_paused ()
{
  if (p_mgr_)
  {
    p_mgr_->graph_paused ();
  }
}

void graph::graph::graph_unpaused ()
{
  if (p_mgr_)
  {
    p_mgr_->graph_unpaused ();
  }
}

void graph::graph::graph_metadata (const track_metadata_map_t &metadata)
{
  if (p_mgr_)
  {
    p_mgr_->graph_metadata (metadata);
  }
}

void graph::graph::graph_volume (const int volume)
{
  if (p_mgr_)
  {
    p_mgr_->graph_volume (volume);
  }
}

void graph::graph::graph_unloaded ()
{
  if (p_mgr_)
  {
    p_mgr_->graph_unloaded ();
  }
}

void graph::graph::graph_end_of_play ()
{
  if (p_mgr_)
  {
    p_mgr_->graph_end_of_play ();
  }
}

void graph::graph::graph_error (const OMX_ERRORTYPE error,
                                const std::string &msg)
{
  if (p_mgr_)
  {
    p_mgr_->graph_error (error, msg);
  }
}

std::string graph::graph::get_graph_name () const
{
  return graph_name_;
}

OMX_ERRORTYPE
graph::graph::init_cmd_queue ()
{
  tiz_check_omx_ret_oom (tiz_mutex_init (&mutex_));
  tiz_check_omx_ret_oom (tiz_sem_init (&sem_, 0));
  tiz_check_omx_ret_oom (tiz_queue_init (&p_queue_, TIZ_GRAPH_QUEUE_MAX_ITEMS));
  return OMX_ErrorNone;
}

void graph::graph::deinit_cmd_queue ()
{
  tiz_mutex_destroy (&mutex_);
  tiz_sem_destroy (&sem_);
  tiz_queue_destroy (p_queue_);
  p_queue_ = NULL;
}

OMX_ERRORTYPE
graph::graph::post_cmd (tiz::graph::cmd *p_cmd)
{
  assert (p_cmd);
  if (p_ops_ && p_queue_)
    {
      tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
      (void) tiz_queue_send (p_queue_, p_cmd);
      tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));
    }
  return OMX_ErrorNone;
}
