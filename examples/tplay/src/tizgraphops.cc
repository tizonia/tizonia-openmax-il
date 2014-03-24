/* -*-Mode: c++; -*- */
/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphops.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph fsm operations base class - implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <boost/make_shared.hpp>
#include <boost/mem_fn.hpp>

#include <tizosal.h>
#include <tizmacros.h>

#include "tizgraphfactory.h"
#include "tizgraph.h"
#include "tizgraphconfig.h"
#include "tizgraphutil.h"
#include "tizgraphcback.h"
#include "tizgraphops.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.ops"
#endif

namespace graph = tiz::graph;

//
// ops
//
graph::ops::ops (graph *p_graph, const omx_comp_name_lst_t &comp_lst,
                 const omx_comp_role_lst_t &role_lst)
  : p_graph_ (p_graph),
    probe_ptr_ (),
    comp_lst_ (comp_lst),
    role_lst_ (role_lst),
    handles_ (comp_lst.size (), OMX_HANDLETYPE (NULL)),
    h2n_ (),
    config_ (),
    expected_transitions_lst_ (),
    expected_port_transitions_lst_ (),
    playlist_ (),
    jump_ (SKIP_DEFAULT_VALUE),
    error_code_ (OMX_ErrorNone),
    error_msg_ ()
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
  assert (NULL != p_graph_);
  assert (comp_lst.size () == role_lst_.size ());
}

graph::ops::~ops ()
{
}

void graph::ops::do_load ()
{
  assert (!comp_lst_.empty ());
  assert (!role_lst_.empty ());
  assert (NULL != p_graph_);

  G_OPS_BAIL_IF_ERROR (util::verify_comp_list (comp_lst_),
                       "Unable to verify the component list.");
  G_OPS_BAIL_IF_ERROR (util::verify_role_list (comp_lst_, role_lst_),
                       "Unable to verify the role list.");

  tiz::graph::cbackhandler &cbacks = p_graph_->cback_handler_;
  G_OPS_BAIL_IF_ERROR (
      util::instantiate_comp_list (comp_lst_, handles_, h2n_, &(cbacks),
                                   cbacks.get_omx_cbacks ()),
      "Unable to instantiate the component list.");
}

void graph::ops::do_setup ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (util::setup_suppliers (handles_),
                         "Unable to setup suppliers.");
    G_OPS_BAIL_IF_ERROR (util::setup_tunnels (handles_),
                         "Unable to setup the tunnels.");
  }
}

void graph::ops::do_ack_loaded ()
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, " p_graph_ [%p]", p_graph_);
  if (last_op_succeeded () && NULL != p_graph_)
  {
    p_graph_->graph_loaded ();
  }
}

void graph::ops::do_store_config (const tizgraphconfig_ptr_t &config)
{
  config_ = config;
  playlist_ = config_->get_playlist ();
}

void graph::ops::do_disable_ports ()
{
  // This is a no-op in the base class.
}

void graph::ops::do_probe ()
{
  // This is a no-op in the base class.
}

bool graph::ops::is_port_settings_evt_required () const
{
  // To be overriden in child classes when needed.
  return false;
}

bool graph::ops::is_disabled_evt_required () const
{
  // To be overriden in child classes when needed.
  return false;
}

void graph::ops::do_configure ()
{
  // This is a no-op in the base class.
}

void graph::ops::do_omx_loaded2idle ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (
        util::transition_all (handles_, OMX_StateIdle, OMX_StateLoaded),
        "Unable to transition from Loaded->Idle");
    record_expected_transitions (OMX_StateIdle);
  }
}

void graph::ops::do_omx_idle2exe ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (
        util::transition_all (handles_, OMX_StateExecuting, OMX_StateIdle),
        "Unable to transition from Idle->Exe");
    record_expected_transitions (OMX_StateExecuting);
  }
}

void graph::ops::do_ack_execd ()
{
  if (last_op_succeeded () && NULL != p_graph_)
  {
    p_graph_->graph_execd ();
  }
}

void graph::ops::do_omx_exe2pause ()
{
  assert (!handles_.empty ());
  if (last_op_succeeded ())
  {
    const int renderer_handle_index = handles_.size () - 1;
    ;
    G_OPS_BAIL_IF_ERROR (
        util::transition_one (handles_, renderer_handle_index, OMX_StatePause),
        "Unable to transition renderer from Exe->Pause");
    clear_expected_transitions ();
    add_expected_transition (handles_[renderer_handle_index], OMX_StatePause);
  }
}

void graph::ops::do_omx_pause2exe ()
{
  assert (!handles_.empty ());
  if (last_op_succeeded ())
  {
    const int renderer_handle_index = handles_.size () - 1;
    G_OPS_BAIL_IF_ERROR (util::transition_one (handles_, renderer_handle_index,
                                               OMX_StateExecuting),
                         "Unable to transition renderer from Pause->Exe");
    clear_expected_transitions ();
    add_expected_transition (handles_[renderer_handle_index],
                             OMX_StateExecuting);
  }
}

void graph::ops::do_omx_exe2idle ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (
        util::transition_all (handles_, OMX_StateIdle, OMX_StateExecuting),
        "Unable to transition from Exe->Idle");
    record_expected_transitions (OMX_StateIdle);
  }
}

void graph::ops::do_omx_idle2loaded ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (
        util::transition_all (handles_, OMX_StateLoaded, OMX_StateIdle),
        "Unable to transition from Idle->Loaded");
    record_expected_transitions (OMX_StateLoaded);
  }
}

void graph::ops::do_seek ()
{
  // TODO
}

void graph::ops::do_skip ()
{
  if (last_op_succeeded () && 0 != jump_ && !is_end_of_play ())
  {
    playlist_->skip (jump_);
    // Reset the jump value, to its default value
    jump_ = SKIP_DEFAULT_VALUE;
  }
}

void graph::ops::do_store_skip (const int jump)
{
  jump_ = jump;
}

/**
 * Default implementation of do_volume () operation. It applies a volume
 * increment or decrement on port #0 of the last element of the graph.
 *
 * @param step The number of "units" by which the volume will be increased (if
 * positive) or decreased (negative).
 */
void graph::ops::do_volume (const int step)
{
  if (last_op_succeeded ())
  {
    OMX_U32 input_port = 0;
    assert (!handles_.empty ());
    G_OPS_BAIL_IF_ERROR (
        util::apply_volume (handles_[handles_.size () - 1], input_port, step),
        "Unable to apply volume");
  }
}

/**
 * Default implementation of do_mute () operation. It applies mute/unmute on
 * port #0 of the last element of the graph.
 *
 */
void graph::ops::do_mute ()
{
  if (last_op_succeeded ())
  {
    OMX_U32 input_port = 0;
    assert (!handles_.empty ());
    G_OPS_BAIL_IF_ERROR (
        util::apply_mute (handles_[handles_.size () - 1], input_port),
        "Unable to apply mute");
  }
}

void graph::ops::do_error ()
{
  if (p_graph_)
  {
    p_graph_->graph_error (error_code_, error_msg_);
  }
}

void graph::ops::do_end_of_play ()
{
  if (p_graph_)
  {
    p_graph_->graph_end_of_play ();
  }
}

void graph::ops::do_tear_down_tunnels ()
{
  G_OPS_BAIL_IF_ERROR (util::tear_down_tunnels (handles_),
                       "Unable to tear down tunnels.");
}

void graph::ops::do_destroy_graph ()
{
  util::destroy_list (handles_);
  handles_ = omx_comp_handle_lst_t (comp_lst_.size (), OMX_HANDLETYPE (NULL));
  h2n_.clear ();
}

void graph::ops::do_ack_unloaded ()
{
  if (p_graph_)
  {
    p_graph_->graph_unloaded ();
  }
}

void graph::ops::do_reset_internal_error ()
{
  error_code_ = OMX_ErrorNone;
  error_msg_.clear ();
}

OMX_ERRORTYPE
graph::ops::internal_error () const
{
  return error_code_;
}

std::string graph::ops::internal_error_msg () const
{
  return error_msg_;
}

bool graph::ops::is_last_component (const OMX_HANDLETYPE handle) const
{
  bool rc = false;
  if (!handles_.empty ())
  {
    rc = (handles_[handles_.size () - 1] == handle);
  }
  return rc;
}

bool graph::ops::is_trans_complete (const OMX_HANDLETYPE handle,
                                    const OMX_STATETYPE to_state)
{
  bool rc = false;

  assert (std::find (handles_.begin (), handles_.end (), handle)
          != handles_.end ());
  assert (to_state <= OMX_StateWaitForResources);
  assert (!expected_transitions_lst_.empty ());

  TIZ_LOG (TIZ_PRIORITY_TRACE, "handle [%p] to_state [%s]...", handle,
           tiz_state_to_str (to_state));

  if (!handles_.empty () && !expected_transitions_lst_.empty ())
  {
    omx_event_info_lst_t::iterator it = std::find (
        expected_transitions_lst_.begin (), expected_transitions_lst_.end (),
        omx_event_info (handle, to_state, OMX_ErrorNone));
    assert (expected_transitions_lst_.end () != it);

    if (expected_transitions_lst_.end () != it)
    {
      expected_transitions_lst_.erase (it);
      assert (util::verify_transition_one (handle, to_state));
      if (expected_transitions_lst_.empty ())
      {
        rc = true;
      }
    }
  }
  return rc;
}

bool graph::ops::is_port_disabling_complete (const OMX_HANDLETYPE handle,
                                             const OMX_U32 port_id)
{
  return is_port_transition_complete (handle, port_id, OMX_CommandPortDisable);
}

bool graph::ops::is_port_enabling_complete (const OMX_HANDLETYPE handle,
                                            const OMX_U32 port_id)
{
  return is_port_transition_complete (handle, port_id, OMX_CommandPortEnable);
}

bool graph::ops::last_op_succeeded () const
{
#ifdef _DEBUG
  if (error_code_ != OMX_ErrorNone)
  {
    TIZ_LOG (TIZ_PRIORITY_TRACE, "error_code_ [%d] [%s]...",
             tiz_err_to_str (error_code_), error_msg_);
  }
#endif
  return (error_code_ == OMX_ErrorNone);
}

bool graph::ops::is_end_of_play () const
{
  bool rc = true;

  assert (playlist_);

  if (playlist_->loop_playback ()
      || !(playlist_->before_begin () || playlist_->past_end ()))
  {
    rc = false;
  }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "is_end_of_play [%s]...", rc ? "YES" : "NO");
  return rc;
}

bool graph::ops::is_probing_result_ok () const
{
  bool rc = true;
  const OMX_ERRORTYPE int_error = internal_error ();
  if (OMX_ErrorNone != int_error)
  {
    rc = false;
  }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] : is_probing_result_ok [%s]...",
           tiz_err_to_str (int_error), rc ? "YES" : "NO");
  return rc;
}

std::string graph::ops::handle2name (const OMX_HANDLETYPE handle) const
{
  const omx_hdl2name_map_t::const_iterator it = h2n_.find (handle);
  if (it != h2n_.end ())
  {
    return (*it).second;
  }
  return "Unknown handle";
}

void graph::ops::record_error (const OMX_ERRORTYPE err_code,
                               const std::string &err_msg)
{
  error_msg_.assign (err_msg);
  error_code_ = err_code;
}

void graph::ops::clear_expected_transitions ()
{
  expected_transitions_lst_.clear ();
}

void graph::ops::record_expected_transitions (const OMX_STATETYPE to_state)
{
  const size_t nhandles = handles_.size ();
  clear_expected_transitions ();
  for (int i = 0; i < nhandles; ++i)
  {
    expected_transitions_lst_.push_back (
        omx_event_info (handles_[i], to_state, OMX_ErrorNone));
  }
}

void graph::ops::add_expected_transition (
    const OMX_HANDLETYPE handle, const OMX_STATETYPE to_state,
    const OMX_ERRORTYPE error /* = OMX_ErrorNone */)
{
  expected_transitions_lst_.push_back (
      omx_event_info (handle, to_state, error));
}

void graph::ops::clear_expected_port_transitions ()
{
  expected_port_transitions_lst_.clear ();
}

void graph::ops::add_expected_port_transition (
    const OMX_HANDLETYPE handle, const OMX_U32 port_id,
    const OMX_COMMANDTYPE disable_or_enable,
    const OMX_ERRORTYPE error /* = OMX_ErrorNone */)
{
  expected_port_transitions_lst_.push_back (
      omx_event_info (handle, port_id, disable_or_enable, error));
}

bool graph::ops::is_port_transition_complete (
    const OMX_HANDLETYPE handle, const OMX_U32 port_id,
    const OMX_COMMANDTYPE disable_or_enable)
{
  bool rc = false;

  assert (std::find (handles_.begin (), handles_.end (), handle)
          != handles_.end ());
  assert (!expected_port_transitions_lst_.empty ());

  if (!handles_.empty () && !expected_port_transitions_lst_.empty ())
  {
    omx_event_info_lst_t::iterator it = std::find (
        expected_port_transitions_lst_.begin (),
        expected_port_transitions_lst_.end (),
        omx_event_info (handle, port_id, disable_or_enable, OMX_ErrorNone));
    assert (expected_port_transitions_lst_.end () != it);

    if (expected_port_transitions_lst_.end () != it)
    {
      expected_port_transitions_lst_.erase (it);
      // TODO: Assert that the port is disabled/enabled
      // assert (util::verify_port_transition (handle, port_id,
      // disable_or_enable));
      if (expected_port_transitions_lst_.empty ())
      {
        rc = true;
      }
    }
  }
  return rc;
}

OMX_ERRORTYPE
graph::ops::probe_stream (const OMX_PORTDOMAINTYPE omx_domain,
                          const int omx_coding, const std::string &graph_id,
                          const std::string &graph_action,
                          stream_info_dump_func_t stream_info_dump_f,
                          const bool quiet  // = false
                          )
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  assert (playlist_);

  const std::string &uri = playlist_->get_current_uri ();
  assert (!uri.empty ());

  // Probe a new uri
  probe_ptr_.reset ();
  bool quiet_probing = true;
  probe_ptr_ = boost::make_shared< tiz::probe >(uri, quiet_probing);

  if (probe_ptr_)
  {
    bool omx_coding_found = false;
    switch (omx_domain)
    {
      case OMX_PortDomainAudio:
      {
        const int coding = probe_ptr_->get_audio_coding_type ();
        omx_coding_found = coding == omx_coding;
      }
      break;
      case OMX_PortDomainVideo:
      {
        const int coding = probe_ptr_->get_video_coding_type ();
        omx_coding_found = coding == omx_coding;
      }
      break;
      case OMX_PortDomainImage:
      case OMX_PortDomainOther:
      default:
      {
        assert (0);
      }
      break;
    };

    if (probe_ptr_->get_omx_domain () != omx_domain || !omx_coding_found)
    {
      // The current uri is not what we expected. So skip it and erase it from
      // the playlist so that we don't attempt the playback again.
      tiz::graph::util::dump_graph_info ("Unknown format", "skip", uri);
      playlist_->erase_uri (playlist_->current_index ());
      playlist_->set_index (playlist_->current_index () - 1);
      rc = OMX_ErrorContentURIError;
    }
    else
    {
      if (!quiet)
      {
        tiz::graph::util::dump_graph_info (graph_id.c_str (),
                                           graph_action.c_str (), uri);
        probe_ptr_->dump_stream_metadata ();
        boost::bind (boost::mem_fn (stream_info_dump_f), probe_ptr_)();
      }

      // Everything went well..
      rc = OMX_ErrorNone;
    }
  }

  return rc;
}
