/* -*-Mode: c++; -*- */
/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
#include "tizgraphfactory.h"
#include "tizgraph.h"
#include "tizgraphconfig.h"

#include <tizomxutil.h>
#include <tizosal.h>
#include <tizmacros.h>
#include <OMX_Component.h>

#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/assign/list_of.hpp>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graphmgr"
#endif

namespace // Unnamed namespace
{

  typedef struct graph_mgr_cmd_str graph_mgr_cmd_str_t;
  struct graph_mgr_cmd_str
  {
    graph_mgr_cmd_str (tizgraphmgrcmd::cmd_type a_cmd, std::string a_str)
      : cmd (a_cmd), str (a_str)
    {}
    tizgraphmgrcmd::cmd_type cmd;
    const std::string str;
  };

  const std::vector<graph_mgr_cmd_str_t> graph_mgr_cmd_to_str_tbl
  = boost::assign::list_of
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdStart, "ETIZGraphMgrCmdStart"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdNext, "ETIZGraphMgrCmdNext"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdPrev, "ETIZGraphMgrCmdPrev"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdFwd, "ETIZGraphMgrCmdFwd"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdRwd, "ETIZGraphMgrCmdRwd"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdVolume, "ETIZGraphMgrCmdVolume"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdPause, "ETIZGraphMgrCmdPause"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdStop, "ETIZGraphMgrCmdStop"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdGraphEop, "ETIZGraphMgrCmdGraphEop"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdGraphError, "ETIZGraphMgrCmdGraphError"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdMax, "ETIZGraphMgrCmdMax"));

  /*@observer@*/ const char *
  graph_mgr_cmd_to_str (tizgraphmgrcmd::cmd_type a_cmd)
  {
    const size_t count = graph_mgr_cmd_to_str_tbl.size ();
    size_t i = 0;

    for (i = 0; i < count; ++i)
      {
        if (graph_mgr_cmd_to_str_tbl[i].cmd == a_cmd)
          {
            return graph_mgr_cmd_to_str_tbl[i].str.c_str ();
          }
      }
    return "Unknown Graph Manager command";
  }

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
tizgraphmgr::tizgraphmgr(const uri_list_t &file_list)
  :
  mgr_state_ (ETIZGraphMgrStateNull),
  thread_ (),
  mutex_ (),
  sem_ (),
  p_queue_ (NULL),
  playlist_ (file_list),
  running_graph_ptr_ ()
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
  /* Create the manager's thread */
  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom
    (tiz_thread_create (&thread_, 0, 0, g_graphmgr_thread_func, this));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));
  tiz_check_omx_err_ret_oom (tiz_sem_wait (&sem_));
  mgr_state_ = ETIZGraphMgrStateInited;
}

OMX_ERRORTYPE
tizgraphmgr::start ()
{
  assert (ETIZGraphMgrStateInited == mgr_state_);
  return send_msg (tizgraphmgrcmd::ETIZGraphMgrCmdStart);
}

OMX_ERRORTYPE
tizgraphmgr::next ()
{
  assert (ETIZGraphMgrStateStarted == mgr_state_);
  return send_msg (tizgraphmgrcmd::ETIZGraphMgrCmdNext);
}

OMX_ERRORTYPE
tizgraphmgr::prev ()
{
  assert (ETIZGraphMgrStateStarted == mgr_state_);
  return send_msg (tizgraphmgrcmd::ETIZGraphMgrCmdPrev);
}

OMX_ERRORTYPE
tizgraphmgr::fwd ()
{
  assert (ETIZGraphMgrStateStarted == mgr_state_);
  return send_msg (tizgraphmgrcmd::ETIZGraphMgrCmdFwd);
}

OMX_ERRORTYPE
tizgraphmgr::rwd ()
{
  assert (ETIZGraphMgrStateStarted == mgr_state_);
  return send_msg (tizgraphmgrcmd::ETIZGraphMgrCmdRwd);
}

OMX_ERRORTYPE
tizgraphmgr::volume ()
{
  assert (ETIZGraphMgrStateStarted == mgr_state_);
  return send_msg (tizgraphmgrcmd::ETIZGraphMgrCmdVolume);
}

OMX_ERRORTYPE
tizgraphmgr::pause ()
{
  assert (ETIZGraphMgrStateStarted == mgr_state_);
  return send_msg (tizgraphmgrcmd::ETIZGraphMgrCmdPause);
}

OMX_ERRORTYPE
tizgraphmgr::stop ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ETIZGraphMgrStateStarted == mgr_state_);
  rc = send_msg (tizgraphmgrcmd::ETIZGraphMgrCmdStop);
  return rc;
}

OMX_ERRORTYPE
tizgraphmgr::deinit ()
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Waiting until done wit stop [%s]...",
           graph_mgr_state_to_str (mgr_state_));
  tiz_check_omx_err_ret_oom (tiz_sem_wait (&sem_));
  assert (ETIZGraphMgrStateInited == mgr_state_);
  void * p_result = NULL;
  return tiz_thread_join (&thread_, &p_result);
}

OMX_ERRORTYPE
tizgraphmgr::send_msg (const tizgraphmgrcmd::cmd_type type)
{
  assert (type < tizgraphmgrcmd::ETIZGraphMgrCmdMax);
  assert (NULL != p_queue_);

  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom
    (tiz_queue_send (p_queue_, new tizgraphmgrcmd (type)));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));
  return OMX_ErrorNone;
}

void
tizgraphmgr::dispatch (tizgraphmgr *p_graph_mgr, const tizgraphmgrcmd *p_cmd)
{
  assert (NULL != p_graph_mgr);
  assert (NULL != p_cmd);

  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Dispatching [%s] in [%s]...",
           graph_mgr_cmd_to_str (p_cmd->get_type ()),
           graph_mgr_state_to_str (p_graph_mgr->mgr_state_));

  if (!p_graph_mgr->verify_mgr_state (p_cmd->get_type ()))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Command [%s] not supported "
               "in mgr state [%s]. Discarding...",
               graph_mgr_cmd_to_str (p_cmd->get_type ()),
               graph_mgr_state_to_str (p_graph_mgr->mgr_state_));
      return;
    }

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
    case tizgraphmgrcmd::ETIZGraphMgrCmdVolume:
      {
        p_graph_mgr->do_vol ();
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
        p_graph_mgr->do_graph_error ();
        break;
      }
    default:
      assert (0);
    };

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

bool
tizgraphmgr::verify_mgr_state (tizgraphmgrcmd::cmd_type cmd)
{
  bool verification_ok = false;
  mgr_state expected_state = ETIZGraphMgrStateNull;
  mgr_state next_state = ETIZGraphMgrStateNull;
  switch(cmd)
    {
    case tizgraphmgrcmd::ETIZGraphMgrCmdStart:
      {
        expected_state = ETIZGraphMgrStateInited;
        next_state = ETIZGraphMgrStateStarted;
      }
      break;
    case tizgraphmgrcmd::ETIZGraphMgrCmdNext:
    case tizgraphmgrcmd::ETIZGraphMgrCmdPrev:
    case tizgraphmgrcmd::ETIZGraphMgrCmdFwd:
    case tizgraphmgrcmd::ETIZGraphMgrCmdRwd:
    case tizgraphmgrcmd::ETIZGraphMgrCmdVolume:
    case tizgraphmgrcmd::ETIZGraphMgrCmdPause:
    case tizgraphmgrcmd::ETIZGraphMgrCmdGraphEop:
      {
        expected_state = ETIZGraphMgrStateStarted;
        next_state = expected_state;
      }
      break;
    case tizgraphmgrcmd::ETIZGraphMgrCmdStop:
      {
        expected_state = ETIZGraphMgrStateStarted;
        next_state = ETIZGraphMgrStateInited;
      }
      break;
    case tizgraphmgrcmd::ETIZGraphMgrCmdGraphError:
    case tizgraphmgrcmd::ETIZGraphMgrCmdMax:
    default:
      {
        assert (0);
      }
    };

  assert (expected_state == mgr_state_);
  if (expected_state == mgr_state_)
    {
      mgr_state_ = next_state;
      verification_ok = true;
    }

  return verification_ok;
}

OMX_ERRORTYPE
tizgraphmgr::graph_end_of_play ()
{
  assert (ETIZGraphMgrStateStarted == mgr_state_);
  return send_msg (tizgraphmgrcmd::ETIZGraphMgrCmdGraphEop);
}

OMX_ERRORTYPE
tizgraphmgr::graph_error ()
{
  return OMX_ErrorNone;
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

  if (!g_ptr)
    {
      // At this point we have removed all unsupported media, so we should
      // always have a graph object.
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Could not create a graph.");
      ret = OMX_ErrorInsufficientResources;
      goto end;
    }

  if (OMX_ErrorNone != (ret = g_ptr->load ()))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : While loading the graph.",
               tiz_err_to_str (ret));
      goto end;
    }

  if (OMX_ErrorNone != (ret = g_ptr->configure (config)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Could not configure a graph.");
      goto end;
    }

  if (OMX_ErrorNone != (ret = g_ptr->execute ()))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "Found error %s while executing the graph.",
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
      return running_graph_ptr_->skip (-2);
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
tizgraphmgr::do_vol ()
{
  // TODO:
  return OMX_ErrorNone;
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

OMX_ERRORTYPE
tizgraphmgr::do_graph_error ()
{
  // TODO:
  return OMX_ErrorNone;
}
