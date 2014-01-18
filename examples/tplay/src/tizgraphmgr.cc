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
 * @file   tizgraphmgr.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph manager base class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizgraphmgr.h"
#include "tizgraphmgrcmd.h"
#include "tizgraphfactory.h"
#include "tizgraph.h"
#include "tizgraphconfig.h"
#include "tizomxutil.h"

#include <tizosal.h>
#include <tizmacros.h>
#include <OMX_Component.h>

#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/assign/list_of.hpp>
#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graphmgr"
#endif

namespace // Unnamed namespace
{

  typedef struct graph_mgr_state_str graph_mgr_state_str_t;
  struct graph_mgr_state_str
  {
    graph_mgr_state_str (tizgraphmgr::mgr_state a_state, std::string a_str)
      : state (a_state), str (a_str)
    {}
    tizgraphmgr::mgr_state state;
    const std::string str;
  };

  const std::vector<graph_mgr_state_str_t> graph_mgr_state_to_str_tbl
  = boost::assign::list_of
    (graph_mgr_state_str_t (tizgraphmgr::ETIZGraphMgrStateNull, "ETIZGraphMgrStateNull"))
    (graph_mgr_state_str_t (tizgraphmgr::ETIZGraphMgrStateInited, "ETIZGraphMgrStateInited"))
    (graph_mgr_state_str_t (tizgraphmgr::ETIZGraphMgrStateStarted, "ETIZGraphMgrStateStarted"))
    (graph_mgr_state_str_t (tizgraphmgr::ETIZGraphMgrStateMax, "ETIZGraphMgrStateMax"));

  /*@observer@*/ const char *
  graph_mgr_state_to_str (tizgraphmgr::mgr_state a_state)
  {
    const size_t count = graph_mgr_state_to_str_tbl.size ();
    size_t i = 0;

    for (i = 0; i < count; ++i)
      {
        if (graph_mgr_state_to_str_tbl[i].state == a_state)
          {
            return graph_mgr_state_to_str_tbl[i].str.c_str ();
          }
      }
    return "Unknown Graph Manager state";
  }

  bool
  verify_mgr_state (tizgraphmgrcmd::cmd_type cmd, tizgraphmgr::mgr_state &mgr_state)
  {
    bool verification_ok = false;
    tizgraphmgr::mgr_state expected_state = tizgraphmgr::ETIZGraphMgrStateNull;
    tizgraphmgr::mgr_state next_state = tizgraphmgr::ETIZGraphMgrStateNull;
    switch(cmd)
      {
      case tizgraphmgrcmd::ETIZGraphMgrCmdStart:
        {
          expected_state = tizgraphmgr::ETIZGraphMgrStateInited;
          next_state = tizgraphmgr::ETIZGraphMgrStateStarted;
        }
        break;
      case tizgraphmgrcmd::ETIZGraphMgrCmdNext:
      case tizgraphmgrcmd::ETIZGraphMgrCmdPrev:
      case tizgraphmgrcmd::ETIZGraphMgrCmdFwd:
      case tizgraphmgrcmd::ETIZGraphMgrCmdRwd:
      case tizgraphmgrcmd::ETIZGraphMgrCmdVolumeUp:
      case tizgraphmgrcmd::ETIZGraphMgrCmdVolumeDown:
      case tizgraphmgrcmd::ETIZGraphMgrCmdMuteUnmute:
      case tizgraphmgrcmd::ETIZGraphMgrCmdPause:
      case tizgraphmgrcmd::ETIZGraphMgrCmdGraphEop:
        {
          expected_state = tizgraphmgr::ETIZGraphMgrStateStarted;
          next_state = expected_state;
        }
        break;
      case tizgraphmgrcmd::ETIZGraphMgrCmdStop:
        {
          expected_state = tizgraphmgr::ETIZGraphMgrStateStarted;
          next_state = tizgraphmgr::ETIZGraphMgrStateInited;
        }
        break;
      case tizgraphmgrcmd::ETIZGraphMgrCmdGraphError:
      case tizgraphmgrcmd::ETIZGraphMgrCmdMax:
      default:
        {
          assert (0);
        }
      };

    assert (expected_state == mgr_state);
    if (expected_state == mgr_state)
      {
        mgr_state = next_state;
        verification_ok = true;
      }

    return verification_ok;
  }

} // Unnamed namespace

void *
g_graphmgr_thread_func (void *p_arg)
{
  tizgraphmgr *p_graphmgr = static_cast<tizgraphmgr *>(p_arg);
  void *p_data = NULL;

  assert (NULL != p_graphmgr);

  (void) tiz_thread_setname (&(p_graphmgr->thread_), (char *) "tizgraphmgr");

  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_graphmgr->sem_)));

  for (;;)
    {
      tiz_check_omx_err_ret_null
        (tiz_queue_receive (p_graphmgr->p_queue_, &p_data));

      assert (NULL != p_data);

      tizgraphmgrcmd *p_cmd =  static_cast<tizgraphmgrcmd *>(p_data);

      tizgraphmgr::dispatch (p_graphmgr, p_cmd);
      if (tizgraphmgrcmd::ETIZGraphMgrCmdStop == p_cmd->get_type ())
        {
          delete p_cmd;
          break;
        }
      delete p_cmd;
    }

  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_graphmgr->sem_)));

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Graph manager thread exiting...");

  return NULL;
}

//
// tizgraphmgr
//
tizgraphmgr::tizgraphmgr(const uri_list_t &file_list,
                         const error_callback_t &error_cback)
  :
  mgr_state_ (ETIZGraphMgrStateNull),
  thread_ (),
  mutex_ (),
  sem_ (),
  p_queue_ (NULL),
  playlist_ (file_list),
  running_graph_ptr_ (),
  error_cback_ (error_cback)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
  tiz_mutex_init (&mutex_);
  tiz_sem_init (&sem_, 0);
  tiz_queue_init (&p_queue_, 10);
  tizomxutil::init ();
}

tizgraphmgr::~tizgraphmgr()
{
  tizomxutil::deinit();
  tiz_mutex_destroy (&mutex_);
  tiz_sem_destroy (&sem_);
  tiz_queue_destroy (p_queue_);
}

OMX_ERRORTYPE
tizgraphmgr::init ()
{
  assert (ETIZGraphMgrStateNull == mgr_state_);
  if (ETIZGraphMgrStateNull == mgr_state_)
    {
      /* Create the manager's thread */
      tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
      tiz_check_omx_err_ret_oom
        (tiz_thread_create (&thread_, 0, 0, g_graphmgr_thread_func, this));
      tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));
      tiz_check_omx_err_ret_oom (tiz_sem_wait (&sem_));
      mgr_state_ = ETIZGraphMgrStateInited;
    }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizgraphmgr::start ()
{
  assert (ETIZGraphMgrStateInited == mgr_state_);
  return send_cmd (new tizgraphmgrcmd (tizgraphmgrcmd::ETIZGraphMgrCmdStart));
}

OMX_ERRORTYPE
tizgraphmgr::next ()
{
  assert (ETIZGraphMgrStateInited <= mgr_state_);
  return send_cmd (new tizgraphmgrcmd (tizgraphmgrcmd::ETIZGraphMgrCmdNext));
}

OMX_ERRORTYPE
tizgraphmgr::prev ()
{
  assert (ETIZGraphMgrStateInited <= mgr_state_);
  return send_cmd (new tizgraphmgrcmd (tizgraphmgrcmd::ETIZGraphMgrCmdPrev));
}

OMX_ERRORTYPE
tizgraphmgr::fwd ()
{
  assert (ETIZGraphMgrStateInited <= mgr_state_);
  return send_cmd (new tizgraphmgrcmd (tizgraphmgrcmd::ETIZGraphMgrCmdFwd));
}

OMX_ERRORTYPE
tizgraphmgr::rwd ()
{
  assert (ETIZGraphMgrStateInited <= mgr_state_);
  return send_cmd (new tizgraphmgrcmd (tizgraphmgrcmd::ETIZGraphMgrCmdRwd));
}

OMX_ERRORTYPE
tizgraphmgr::volume (const int step)
{
  assert (ETIZGraphMgrStateInited <= mgr_state_);
  if (step == 0)
    {
      return OMX_ErrorNone;
    }
  return send_cmd (new tizgraphmgrcmd (step > 0 ? tizgraphmgrcmd::ETIZGraphMgrCmdVolumeUp
                                       : tizgraphmgrcmd::ETIZGraphMgrCmdVolumeDown));
}

OMX_ERRORTYPE
tizgraphmgr::mute ()
{
  assert (ETIZGraphMgrStateInited <= mgr_state_);
  return send_cmd (new tizgraphmgrcmd (tizgraphmgrcmd::ETIZGraphMgrCmdMuteUnmute));
}

OMX_ERRORTYPE
tizgraphmgr::pause ()
{
  assert (ETIZGraphMgrStateInited <= mgr_state_);
  return send_cmd (new tizgraphmgrcmd (tizgraphmgrcmd::ETIZGraphMgrCmdPause));
}

OMX_ERRORTYPE
tizgraphmgr::stop ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ETIZGraphMgrStateInited <= mgr_state_);
  rc = send_cmd (new tizgraphmgrcmd (tizgraphmgrcmd::ETIZGraphMgrCmdStop));
  return rc;
}

OMX_ERRORTYPE
tizgraphmgr::deinit ()
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Waiting until done with stop [%s]...",
           graph_mgr_state_to_str (mgr_state_));
  tiz_check_omx_err_ret_oom (tiz_sem_wait (&sem_));
  assert (ETIZGraphMgrStateInited == mgr_state_);
  void * p_result = NULL;
  return tiz_thread_join (&thread_, &p_result);
}

OMX_ERRORTYPE
tizgraphmgr::send_cmd (tizgraphmgrcmd *p_cmd)
{
  assert (NULL != p_cmd);
  assert (p_cmd->get_type () < tizgraphmgrcmd::ETIZGraphMgrCmdMax);
  assert (NULL != p_queue_);
  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom
    (tiz_queue_send (p_queue_, p_cmd));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));
  return OMX_ErrorNone;
}

void
tizgraphmgr::dispatch (tizgraphmgr *p_graph_mgr, const tizgraphmgrcmd *p_cmd)
{
  assert (NULL != p_graph_mgr);
  assert (NULL != p_cmd);

  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Dispatching [%s] in [%s]...",
           p_cmd->c_str (),
           graph_mgr_state_to_str (p_graph_mgr->mgr_state_));

  if (!verify_mgr_state (p_cmd->get_type (), p_graph_mgr->mgr_state_))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Command [%s] not supported "
               "in mgr state [%s]. Discarding...",
               p_cmd->c_str (),
               graph_mgr_state_to_str (p_graph_mgr->mgr_state_));
    }
  else
    {
      switch(p_cmd->get_type ())
        {
        case tizgraphmgrcmd::ETIZGraphMgrCmdStart:
          {
            p_graph_mgr->do_start ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdNext:
          {
            p_graph_mgr->do_next ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdPrev:
          {
            p_graph_mgr->do_prev ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdFwd:
          {
            p_graph_mgr->do_fwd ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdRwd:
          {
            p_graph_mgr->do_rwd ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdVolumeUp:
          {
            p_graph_mgr->do_vol_up ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdVolumeDown:
          {
            p_graph_mgr->do_vol_down ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdMuteUnmute:
          {
            p_graph_mgr->do_mute ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdPause:
          {
            p_graph_mgr->do_pause ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdStop:
          {
            p_graph_mgr->do_stop ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdGraphEop:
          {
            p_graph_mgr->do_graph_end_of_play ();
            break;
          }
        case tizgraphmgrcmd::ETIZGraphMgrCmdGraphError:
          {
            break;
          }
        default:
          assert (0);
        };
    }
}

tizgraph_ptr_t
tizgraphmgr::get_graph (const std::string & uri)
{
  tizgraph_ptr_t g_ptr;
  std::string encoding (tizgraphfactory::coding_type (uri));
  tizgraph_ptr_map_t::const_iterator it
    = graph_registry_.find (encoding);
  if (it == graph_registry_.end ())
    {
      g_ptr = tizgraphfactory::create_graph (uri);
      if (g_ptr)
        {
          // TODO: Check rc
          graph_registry_.insert
            (std::make_pair<std::string, tizgraph_ptr_t>(encoding, g_ptr));
          g_ptr->set_manager (this);
        }
    }
  else
    {
      g_ptr = it->second;
    }
  return g_ptr;
}

OMX_ERRORTYPE
tizgraphmgr::graph_end_of_play ()
{
  assert (ETIZGraphMgrStateStarted == mgr_state_);
  return send_cmd (new tizgraphmgrcmd (tizgraphmgrcmd::ETIZGraphMgrCmdGraphEop));
}

void
tizgraphmgr::graph_error (OMX_ERRORTYPE error, std::string msg)
{
  error_cback_ (error, msg);
}

OMX_ERRORTYPE
tizgraphmgr::do_start ()
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;

  tizplaylist_t sub_playlist = playlist_.get_next_sub_playlist ();
  const uri_list_t & sub_urilist = sub_playlist.get_uri_list ();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "sub_urilist size %d",
           sub_urilist.size ());

  tizgraph_ptr_t g_ptr (get_graph (sub_urilist[0]));
  const bool continuous_play = false;
  tizgraphconfig_ptr_t config
    = boost::make_shared < tizgraphconfig > (sub_urilist, continuous_play);
  bool graph_loaded = false;
  std::string error;

  if (!g_ptr)
    {
      // At this point we have removed all unsupported media, so we should
      // always have a graph object.
      error.assign ("Could not create a graph. Format not supported.");
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s]", error.c_str ());
      ret = OMX_ErrorInsufficientResources;
      goto end;
    }

  if (OMX_ErrorNone != (ret = g_ptr->load ()))
    {
      error.assign ("Error while loading the graph.");
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : While loading the graph.",
               tiz_err_to_str (ret));
      goto end;
    }

  if (OMX_ErrorNone != (ret = g_ptr->configure (config)))
    {
      error.assign ("Error while configuring the graph.");
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : While configuring the graph.",
               tiz_err_to_str (ret));
      goto end;
    }

  if (OMX_ErrorNone != (ret = g_ptr->execute ()))
    {
      error.assign ("Error while executing the graph.");
      TIZ_LOG (TIZ_PRIORITY_ERROR,"[%s] : While executing the graph.",
               tiz_err_to_str (ret));
      goto end;
    }

 end:

  if (OMX_ErrorNone != ret)
    {
      if (graph_loaded)
        {
          g_ptr->unload ();
        }
      // This is bad enough, we are finishing here
      graph_error (ret, error);
    }
  else
    {
      running_graph_ptr_ = g_ptr;
    }

  return ret;
}

OMX_ERRORTYPE
tizgraphmgr::do_next ()
{
  if (!running_graph_ptr_->at_end_of_play ())
    {
      return running_graph_ptr_->skip (1);
    }
  else
    {
      return do_graph_end_of_play ();
    }
}

OMX_ERRORTYPE
tizgraphmgr::do_prev ()
{
  if (!running_graph_ptr_->at_beginning_of_play ())
    {
      return running_graph_ptr_->skip (-1);
    }
  else
    {
      return do_graph_end_of_play ();
    }
}

OMX_ERRORTYPE
tizgraphmgr::do_fwd ()
{
  // TODO:
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizgraphmgr::do_rwd ()
{
  // TODO:
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizgraphmgr::do_vol_up ()
{
  return running_graph_ptr_->volume (1);
}

OMX_ERRORTYPE
tizgraphmgr::do_vol_down ()
{
  return running_graph_ptr_->volume (-1);
}

OMX_ERRORTYPE
tizgraphmgr::do_mute ()
{
  return running_graph_ptr_->mute ();
}

OMX_ERRORTYPE
tizgraphmgr::do_pause ()
{
  return running_graph_ptr_->pause ();
}

OMX_ERRORTYPE
tizgraphmgr::do_stop ()
{
  running_graph_ptr_->unload ();
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizgraphmgr::do_graph_end_of_play ()
{
  running_graph_ptr_->unload ();
  return do_start ();
}
