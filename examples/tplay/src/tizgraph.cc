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
 * @file   tizgraph.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph base class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizgraph.h"
#include "tizgraphmgr.h"
#include "tizgraphcmd.h"

#include <tizomxutil.h>
#include <tizosal.h>
#include <tizmacros.h>
#include <OMX_Component.h>

#include <assert.h>
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph"
#endif

namespace // Unnamed namespace
{

  struct transition_to
  {
  transition_to(const OMX_STATETYPE to_state, const OMX_U32 useconds = 0)
    : to_state_(to_state), delay_(useconds), error_(OMX_ErrorNone) {}
  void operator()(const OMX_HANDLETYPE &handle)
  {
    if (OMX_ErrorNone == error_)
      error_ = OMX_SendCommand (handle, OMX_CommandStateSet, to_state_,
                                NULL);
    tiz_sleep(delay_);
  }

  const OMX_STATETYPE to_state_;
  OMX_U32 delay_;
  OMX_ERRORTYPE error_;
  };

  typedef struct graph_state_str graph_state_str_t;
  struct graph_state_str
  {
    graph_state_str (OMX_STATETYPE a_state, std::string a_str)
      : state (a_state), str (a_str)
    {}
    OMX_STATETYPE state;
    const std::string str;
  };

  const std::vector<graph_state_str_t> graph_state_to_str_tbl
  = boost::assign::list_of
    (graph_state_str_t (OMX_StateLoaded, "OMX_StateLoaded"))
    (graph_state_str_t (OMX_StateIdle, "OMX_StateIdle"))
    (graph_state_str_t (OMX_StateExecuting, "OMX_StateExecuting"))
    (graph_state_str_t (OMX_StatePause, "OMX_StatePause"))
    (graph_state_str_t (OMX_StateWaitForResources, "OMX_StateWaitForResources"))
    (graph_state_str_t (OMX_StateMax, "OMX_StateMax"));

  /*@observer@*/ const char *
  graph_state_to_str (OMX_STATETYPE a_state)
  {
    const size_t count = graph_state_to_str_tbl.size ();
    size_t i = 0;

    for (i = 0; i < count; ++i)
      {
        if (graph_state_to_str_tbl[i].state == a_state)
          {
            return graph_state_to_str_tbl[i].str.c_str ();
          }
      }
    return "Unknown Graph state";
  }

}

void *
g_graph_thread_func (void *p_arg)
{
  tizgraph *p_graph = static_cast<tizgraph *>(p_arg);
  void *p_data = NULL;

  assert (NULL != p_graph);

  (void) tiz_thread_setname (&(p_graph->thread_),
                             (char *) p_graph->get_graph_name ().c_str ());

  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_graph->sem_)));

  for (;;)
    {
      tiz_check_omx_err_ret_null
        (tiz_queue_receive (p_graph->p_queue_, &p_data));

      assert (NULL != p_data);

      tizgraphcmd *p_cmd =  static_cast<tizgraphcmd *>(p_data);

      tizgraph::dispatch (p_graph, p_cmd);
      if (tizgraphcmd::ETIZGraphCmdUnload == p_cmd->get_type ())
        {
          delete p_cmd;
          break;
        }
      delete p_cmd;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Graph thread exiting...");

  return NULL;
}

OMX_ERRORTYPE
tizgraph_event_handler (OMX_HANDLETYPE hComponent,
                        OMX_PTR pAppData,
                        OMX_EVENTTYPE eEvent,
                        OMX_U32 nData1,
                        OMX_U32 nData2,
                        OMX_PTR pEventData)
{
  tizcback_handler *p_handler = NULL;

  assert (NULL != pAppData);

  p_handler = static_cast<tizcback_handler*>(pAppData);

  p_handler->receive_event (hComponent,
                           eEvent,
                           nData1,
                           nData2,
                           pEventData);
  return OMX_ErrorNone;
}


tizcback_handler::tizcback_handler(const tizgraph &graph)
:
  parent_(graph),
  mutex_(),
  cond_(),
  events_outstanding_(false),
  cbacks_(),
  received_queue_(),
  expected_list_()
{
  cbacks_.EventHandler = &tizgraph_event_handler;
  cbacks_.EmptyBufferDone = NULL;
  cbacks_.FillBufferDone = NULL;
}

void
tizcback_handler::receive_event(OMX_HANDLETYPE hComponent,
                                OMX_EVENTTYPE eEvent,
                                OMX_U32 nData1,
                                OMX_U32 nData2,
                                OMX_PTR pEventData)
{
  bool done = false;
  boost::lock_guard<boost::mutex> lock(mutex_);
  if (eEvent == OMX_EventCmdComplete)
    {
      TIZ_LOG (TIZ_PRIORITY_DEBUG, "[%s] : "
               "[%s] [%s] [%s] error [%p]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent),
               tiz_cmd_to_str ((OMX_COMMANDTYPE)nData1),
               tiz_state_to_str ((OMX_STATETYPE)nData2),
               pEventData);
    }
  else if (eEvent == OMX_EventPortSettingsChanged)
    {
      TIZ_LOG (TIZ_PRIORITY_DEBUG, "[%s] : "
               "[%s] port [%lu] index [%s] pEventData [%p]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent), nData1,
               tiz_idx_to_str ((OMX_INDEXTYPE)nData2),
               pEventData);
    }
  else if (eEvent == OMX_EventBufferFlag)
    {
      TIZ_LOG (TIZ_PRIORITY_DEBUG, "[%s] : "
               "[%s] port [%lu] flags [%lX] pEventData [%p]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent), nData1, nData2,
               pEventData);
      const_cast<tizgraph &>(parent_).eos (hComponent);
      // We are done here
      done = true;
    }
  else if (eEvent == OMX_EventError)
    {
      TIZ_LOG (TIZ_PRIORITY_DEBUG, "[%s] : "
               "[%s] error [%s] port [%lu] pEventData [%p]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent), tiz_err_to_str
               (static_cast<OMX_ERRORTYPE>(nData1)),
               nData2, pEventData);
      if (OMX_ErrorPortUnpopulated != nData1)
        {
          const_cast<tizgraph &>(parent_).error
            (static_cast<OMX_ERRORTYPE> (nData1));
//           const_cast<tizgraph &>(parent_).notify_graph_error
//             (static_cast<OMX_ERRORTYPE> (nData1),
//              const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str());
        }
      expected_list_.clear();
      events_outstanding_ = false;
    }
  else if (eEvent == OMX_EventVendorStartUnused)
    {
      TIZ_LOG (TIZ_PRIORITY_DEBUG, "[%s] : [%s]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent));
      expected_list_.clear();
      events_outstanding_ = false;
    }
  else
    {
      TIZ_LOG (TIZ_PRIORITY_DEBUG, "Received from [%s] : "
               "[%s]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent));
      // We are done here
      done = true;
    }

  if (!done)
    {
      if (OMX_EventVendorStartUnused != eEvent)
        {
          received_queue_.push_back (waitevent_info(hComponent,
                                                    eEvent,
                                                    nData1,
                                                    nData2,
                                                    pEventData));
        }

      if (all_events_received ())
        {
          assert (expected_list_.empty());
          events_outstanding_ = false;
        }
      cond_.notify_one();
    }
}

OMX_ERRORTYPE
tizcback_handler::wait_for_event_list (const waitevent_list_t &event_list)
{
  boost::unique_lock<boost::mutex> lock (mutex_);

  assert (expected_list_.empty() == true);
  expected_list_ = event_list;
  events_outstanding_ = true;

  TIZ_LOG (TIZ_PRIORITY_DEBUG, "events_outstanding_ = [%s] event_list size [%d]",
           events_outstanding_ ? "true" : "false", event_list.size());

  for (waitevent_list_t::const_iterator it = event_list.begin ();
       it != event_list.end (); ++it)
    {
      TIZ_LOG (TIZ_PRIORITY_DEBUG, "Awaiting --> [%s] event [%s] ndata1 [%s] ndata2 [%s]",
               const_cast<tizgraph &>(parent_).h2n_[(*it).component_].c_str (),
               tiz_evt_to_str ((*it).event_),
               (*it).event_ == OMX_EventCmdComplete
               ? tiz_cmd_to_str ((OMX_COMMANDTYPE)(*it).ndata1_) : "",
               (*it).event_ == OMX_EventCmdComplete
               ? tiz_state_to_str ((OMX_STATETYPE)(*it).ndata2_) : "");
    }

  if (!all_events_received ())
    {
      while (events_outstanding_)
        {
          cond_.wait (lock);
        }
    }

  events_outstanding_ = false;
  return OMX_ErrorNone;
}

bool
tizcback_handler::all_events_received ()
{
  if (!expected_list_.empty () && !received_queue_.empty ())
    {
      waitevent_list_t::iterator exp_it = expected_list_.begin ();
      while (exp_it != expected_list_.end())
        {
          waitevent_list_t::iterator queue_end = received_queue_.end ();
          waitevent_list_t::iterator queue_it = queue_end;
          if (queue_end != (queue_it = std::find(received_queue_.begin (),
                                                 received_queue_.end (),
                                                 *exp_it)))
            {
#ifdef _DEBUG
              char port_str [10];
              snprintf (port_str, 10, "%ld", exp_it->ndata2_);
              port_str[9] = '\0';
              TIZ_LOG (TIZ_PRIORITY_DEBUG, "Erasing [%s:%s(%s)] from [%s] "
                       "[%d] events remaining\n",
                       tiz_evt_to_str (exp_it->event_),
                       exp_it->event_ == OMX_EventCmdComplete
                       ? tiz_cmd_to_str ((OMX_COMMANDTYPE)exp_it->ndata1_) : "",
                       exp_it->ndata1_ == OMX_CommandStateSet
                       ? tiz_state_to_str ((OMX_STATETYPE)exp_it->ndata2_) : port_str,
                       const_cast<tizgraph &>(parent_).
                       h2n_[exp_it->component_].c_str (),
                       expected_list_.size () - 1);
#endif
              expected_list_.erase (exp_it);
              received_queue_.erase (queue_it);
              // restart the loop
              exp_it = expected_list_.begin ();
            }
          else
            {
              ++exp_it;
            }
        }
    }

  return (expected_list_.empty () ? true : false);
}


//
// tizgraph
//
tizgraph::tizgraph(const std::string & graph_name, int graph_size, tizprobe_ptr_t probe_ptr)
  :
  graph_name_ (graph_name),
  h2n_(),
  handles_(graph_size, OMX_HANDLETYPE(NULL)),
  cback_handler_(*this),
  probe_ptr_(probe_ptr),
  thread_ (),
  mutex_ (),
  sem_ (),
  p_queue_ (NULL),
  current_graph_state_ (OMX_StateLoaded),
  playlist_ (),
  file_list_ (),
  current_file_index_ (0),
  config_ (),
  p_mgr_ (NULL)
{
  assert (probe_ptr);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
  tiz_mutex_init (&mutex_);
  tiz_sem_init (&sem_, 0);
  tiz_queue_init (&p_queue_, 10);

  // TODO: Do not uncomment this. IL Core init/deinit from multiple threads do not work at the moment.
  //   tizomxutil::init ();
}

tizgraph::~tizgraph()
{
  // TODO: Do not uncomment this. IL Core init/deinit from multiple threads do not work at the moment.
//   tizomxutil::deinit();
  tiz_mutex_destroy (&mutex_);
  tiz_sem_destroy (&sem_);
  tiz_queue_destroy (p_queue_);
}

OMX_ERRORTYPE
tizgraph::init ()
{
  /* Create this graph's thread */
  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom
    (tiz_thread_create (&thread_, 0, 0, g_graph_thread_func, this));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));
  tiz_check_omx_err_ret_oom (tiz_sem_wait (&sem_));
}

OMX_ERRORTYPE
tizgraph::deinit ()
{
  void * p_result = NULL;
  return tiz_thread_join (&thread_, &p_result);
}

OMX_ERRORTYPE
tizgraph::load ()
{
  tiz_check_omx_err (init ());
  tizgraphconfig_ptr_t null_config;
  return send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdLoad, null_config));
}

OMX_ERRORTYPE
tizgraph::configure (const tizgraphconfig_ptr_t config)
{
  return send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdConfig, config));
}

OMX_ERRORTYPE
tizgraph::execute ()
{
  tizgraphconfig_ptr_t null_config;
  return send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdExecute, null_config));
}

OMX_ERRORTYPE
tizgraph::pause ()
{
  tizgraphconfig_ptr_t null_config;
  return send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdPause, null_config));
}

OMX_ERRORTYPE
tizgraph::seek ()
{
  tizgraphconfig_ptr_t null_config;
  return send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdSeek, null_config));
}

OMX_ERRORTYPE
tizgraph::skip (const int jump)
{
  tizgraphconfig_ptr_t null_config;
  return send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdSkip, null_config, NULL, jump));
}

OMX_ERRORTYPE
tizgraph::volume (const int step)
{
  tizgraphconfig_ptr_t null_config;
  return send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdVolume, null_config, NULL, step));
}

OMX_ERRORTYPE
tizgraph::mute ()
{
  tizgraphconfig_ptr_t null_config;
  return send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdMute, null_config));
}

void
tizgraph::eos (OMX_HANDLETYPE handle)
{
  tizgraphconfig_ptr_t null_config;
  send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdEos, null_config, handle));
}

void
tizgraph::error (const OMX_ERRORTYPE error)
{
  tizgraphconfig_ptr_t null_config;
  send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdError, null_config, NULL, 0, error));
}

void
tizgraph::unload ()
{
  tizgraphconfig_ptr_t null_config;
  send_cmd (new tizgraphcmd (tizgraphcmd::ETIZGraphCmdUnload, null_config));
  deinit ();
}

void
tizgraph::set_manager (tizgraphmgr *ap_graph_mgr)
{
  assert (NULL != ap_graph_mgr);
  p_mgr_ = ap_graph_mgr;
}

bool
tizgraph::at_beginning_of_play () const
{
  return (current_file_index_ == 0);
}

bool
tizgraph::at_end_of_play () const
{
  return (current_file_index_ >= file_list_.size () - 1);
}


OMX_ERRORTYPE
tizgraph::verify_existence (const component_names_t &comp_list) const
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  std::vector<std::string> components;

  if (OMX_ErrorNoMore == (error = tizomxutil::list_comps(components)))
    {
      BOOST_FOREACH( std::string comp, comp_list )
        {
          bool found = false;
          BOOST_FOREACH( std::string component, components )
            {
              if (comp.compare(component) == 0)
                {
                  found = true;
                  break;
                }
            }

          if (!found)
            {
              error = OMX_ErrorComponentNotFound;
              break;
            }
        }
    }

  if (error == OMX_ErrorNoMore)
    {
      error = OMX_ErrorNone;
    }

  return error;
}

OMX_ERRORTYPE
tizgraph::verify_role (const std::string &comp,
                       const std::string &comp_role) const
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  std::vector<std::string> roles;

  if (OMX_ErrorNoMore == (error = tizomxutil::
                          roles_of_comp((OMX_STRING)comp.c_str(), roles)))
    {
      bool found = false;
      BOOST_FOREACH( std::string role, roles )
        {
          if (comp_role.compare(role) == 0)
            {
              found = true;
              break;
            }
        }
      if (!found)
        {
          error = OMX_ErrorComponentNotFound;
        }
    }

  if (error == OMX_ErrorNoMore)
    {
      error = OMX_ErrorNone;
    }

  return error;
}

OMX_ERRORTYPE
tizgraph::verify_role_list (const component_names_t &comp_list,
                            const component_roles_t &role_list) const
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  std::vector<std::string> roles;
  int role_lst_size = role_list.size();

  assert (comp_list.size () == role_lst_size);

  for (int i=0; i<role_lst_size; i++)
    {
      if (OMX_ErrorNone != (error = verify_role(comp_list[i],
                                               role_list[i])))
        {
          break;
        }
    }

  return error;
}

OMX_ERRORTYPE
tizgraph::instantiate_component (const std::string &comp_name, int graph_position)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = NULL;
  assert (graph_position < handles_.size());

  if (OMX_ErrorNone == (error =
                        OMX_GetHandle (&p_hdl,
                                       (OMX_STRING)comp_name.c_str(),
                                       &cback_handler_,
                                       cback_handler_.get_omx_cbacks())))
    {
      handles_[graph_position] = p_hdl;
      h2n_[p_hdl] = comp_name;
    }

  return error;
}

OMX_ERRORTYPE
tizgraph::instantiate_list (const component_names_t &comp_list)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int position = 0;
  bool graph_instantiated = true;

  assert (comp_list.size() == handles_.size());

  BOOST_FOREACH( std::string comp, comp_list )
    {
      if (OMX_ErrorNone != (error = instantiate_component(comp, position++)))
        {
          graph_instantiated = false;
          break;
        }
    }

  if (false == graph_instantiated)
    {
      destroy_list ();
    }

  return error;
}

void
tizgraph::destroy_list ()
{
  int handle_lst_size = handles_.size();

  for (int i=0; i<handle_lst_size; i++)
    {
      if (handles_[i] != NULL)
        {
          OMX_FreeHandle (handles_[i]);
          handles_[i] = NULL;
        }
    }
}

void
tizgraph::dump_graph_info (const char *ap_coding_type_str,
                           const char *ap_graph_type_str,
                           const std::string &uri) const
{
#define KNRM "\x1B[0m"
#define KGRN "\x1B[32m"
  fprintf (stdout, "%s[%s] [%s] : '%s'.%s\n",
           KGRN,
           ap_coding_type_str,
           ap_graph_type_str,
           uri.c_str (),
           KNRM);
}

OMX_ERRORTYPE
tizgraph::setup_tunnels () const
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int handle_lst_size = handles_.size();

  for (int i=0; i<handle_lst_size-1 && OMX_ErrorNone == error; i++)
    {
      error = OMX_SetupTunnel(handles_[i], i==0 ? 0 : 1, handles_[i+1], 0);
    }

  return error;
}

OMX_ERRORTYPE
tizgraph::tear_down_tunnels () const
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int handle_lst_size = handles_.size();

  for (int i=0; i<handle_lst_size-1 && OMX_ErrorNone == error; i++)
    {
      error = OMX_TeardownTunnel(handles_[i], i==0 ? 0 : 1, handles_[i+1], 0);
    }

  return error;
}

OMX_ERRORTYPE
tizgraph::setup_suppliers () const
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
  const int handle_lst_size = handles_.size();

  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.eBufferSupplier = OMX_BufferSupplyInput;

  for (int i=0; i<handle_lst_size-1 && OMX_ErrorNone == error; i++)
    {
      supplier.nPortIndex = i==0 ? 0 : 1;
      error = OMX_SetParameter(handles_[i], OMX_IndexParamCompBufferSupplier,
                               &supplier);
      if (OMX_ErrorNone == error)
        {
          supplier.nPortIndex = 0;
          error = OMX_SetParameter(handles_[i+1],
                                   OMX_IndexParamCompBufferSupplier,
                                   &supplier);
        }
    }
  return error;
}

OMX_ERRORTYPE
tizgraph::transition_all (const OMX_STATETYPE to, const OMX_STATETYPE from)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  if (to == current_graph_state_)
    {
      // Nothing to do
      return OMX_ErrorNone;
    }

  if ((to == OMX_StateIdle && from == OMX_StateLoaded)
      ||
      (to == OMX_StateExecuting && from == OMX_StateIdle))
    {
      // Suppliers first, hence back to front order
      error = (std::for_each(handles_.rbegin(), handles_.rend(),
                             transition_to(to))).error_;

      // NOTE: Leave this commented code here - for testing purposes
      // Non-suppliers first, hence front to back order
      //       error = (std::for_each(handles_.begin(), handles_.end(),
      //                              transition_to(to))).error_;

    }
  else
    {
      // Non-suppliers first, hence front to back order
      error = (std::for_each(handles_.begin(), handles_.end(),
                             transition_to(to))).error_;

      // NOTE: Leave this commented code here - for testing purposes
      // Suppliers first, hence back to front order
      //       error = (std::for_each(handles_.rbegin(), handles_.rend(),
      //                              transition_to(to))).error_;

    }

  if (OMX_ErrorNone == error)
    {
      waitevent_list_t event_list;
      BOOST_FOREACH( OMX_HANDLETYPE handle, handles_ )
        {
          event_list.push_back(waitevent_info(handle,
                                              OMX_EventCmdComplete,
                                              OMX_CommandStateSet,
                                              to,
                                              (OMX_PTR) OMX_ErrorNone));
        }
      tiz_check_omx_err (cback_handler_.wait_for_event_list(event_list));
    }

  TIZ_LOG (TIZ_PRIORITY_DEBUG, "to [%s] from [%s] error [%s]",
           tiz_state_to_str (to), tiz_state_to_str (from), tiz_err_to_str (error));

  current_graph_state_ = to;

  return error;
}

OMX_ERRORTYPE
tizgraph::transition_one (const int handle_id, const OMX_STATETYPE to)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  assert (handle_id < handles_.size());

  struct transition_to transition_component (to);

  transition_component (handles_[handle_id]);
  error = transition_component.error_;

  if (OMX_ErrorNone == error)
    {
      waitevent_list_t event_list;
      event_list.push_back(waitevent_info(handles_[handle_id],
                                          OMX_EventCmdComplete,
                                          OMX_CommandStateSet,
                                          to,
                                          (OMX_PTR) OMX_ErrorNone));
      tiz_check_omx_err (cback_handler_.wait_for_event_list(event_list));
    }

  return error;
}

OMX_ERRORTYPE
tizgraph::apply_volume (const OMX_HANDLETYPE handle, const OMX_U32 pid, const int step)
{
  #define VOL_STEP 5
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (OMX_StateExecuting == current_graph_state_)
    {
      bool new_volume = false;
      OMX_AUDIO_CONFIG_VOLUMETYPE volume;
      TIZ_INIT_OMX_PORT_STRUCT (volume, pid);
      tiz_check_omx_err (OMX_GetConfig (handle, OMX_IndexConfigAudioVolume, &volume));
      if (volume.sVolume.nValue <= (volume.sVolume.nMax - VOL_STEP) && step > 0)
        {
          volume.sVolume.nValue += VOL_STEP;
          new_volume = true;
        }
      else if (volume.sVolume.nValue >= (volume.sVolume.nMin + VOL_STEP) && step < 0)
        {
          volume.sVolume.nValue -= VOL_STEP;
          new_volume = true;
        }
      if (new_volume)
        {
          tiz_check_omx_err (OMX_SetConfig (handle, OMX_IndexConfigAudioVolume, &volume));
        }
    }
  return rc;
}

OMX_ERRORTYPE
tizgraph::apply_mute (const OMX_HANDLETYPE handle, const OMX_U32 pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (OMX_StateExecuting == current_graph_state_)
    {
      OMX_AUDIO_CONFIG_MUTETYPE mute;
      TIZ_INIT_OMX_PORT_STRUCT (mute, pid);
      tiz_check_omx_err (OMX_GetConfig (handle, OMX_IndexConfigAudioMute, &mute));
      mute.bMute = (mute.bMute == OMX_FALSE ? OMX_TRUE
                    : (mute.bMute == OMX_TRUE ? OMX_FALSE : OMX_TRUE));
      tiz_check_omx_err (OMX_SetConfig (handle, OMX_IndexConfigAudioMute, &mute));
    }
  return rc;
}

OMX_ERRORTYPE
tizgraph::modify_tunnel (const int tunnel_id, const OMX_COMMANDTYPE cmd)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int handle_lst_size = handles_.size();
  assert (tunnel_id < handle_lst_size - 1);

  component_handles_t tunnel_handles;
  tunnel_handles.push_back (handles_[tunnel_id]);
  tunnel_handles.push_back (handles_[tunnel_id + 1]);

  std::vector < int > port_ids;
  port_ids.push_back (tunnel_id == 0 ? 0 : 1);
  port_ids.push_back (tunnel_id+1==handle_lst_size - 1 ? 0 : 1);

  for (int i=0; i<2 && error == OMX_ErrorNone; i++)
    {
      TIZ_LOG (TIZ_PRIORITY_DEBUG, "tunnel_id [%d] cmd [%s] comp [%s] port [%d]",
               tunnel_id, tiz_cmd_to_str (cmd), h2n_[tunnel_handles[i]].c_str(),
               port_ids[i]);
      error = OMX_SendCommand (tunnel_handles[i], cmd, port_ids[i], NULL);
    }

  if (error == OMX_ErrorNone)
    {
      waitevent_list_t event_list;
      for (int i=0; i<2; i++)
        {
          event_list.push_back(waitevent_info(tunnel_handles[i],
                                              OMX_EventCmdComplete,
                                              cmd,
                                              port_ids[i],
                                              (OMX_PTR) OMX_ErrorNone));
        }
      tiz_check_omx_err (cback_handler_.wait_for_event_list(event_list));
    }

  return error;
}

OMX_ERRORTYPE
tizgraph::disable_tunnel (const int tunnel_id)
{
  return modify_tunnel (tunnel_id, OMX_CommandPortDisable);
}

OMX_ERRORTYPE
tizgraph::enable_tunnel (const int tunnel_id)
{
  return modify_tunnel (tunnel_id, OMX_CommandPortEnable);
}

OMX_ERRORTYPE
tizgraph::send_cmd (tizgraphcmd * p_cmd)
{
  assert (NULL != p_cmd);
  assert (p_cmd->get_type () < tizgraphcmd::ETIZGraphCmdMax);
  assert (NULL != p_queue_);
  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom
    (tiz_queue_send (p_queue_, p_cmd));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));
  return OMX_ErrorNone;
}

void
tizgraph::notify_graph_error (const OMX_ERRORTYPE error, const std::string &msg)
{
  if (NULL != p_mgr_)
    {
      p_mgr_->graph_error (error, msg);
    }
}

OMX_ERRORTYPE
tizgraph::notify_graph_end_of_play ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (NULL != p_mgr_)
    {
      rc = p_mgr_->graph_end_of_play ();
    }
  return rc;
}

void
tizgraph::dispatch (tizgraph *p_graph, const tizgraphcmd *p_cmd)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != p_graph);
  assert (NULL != p_cmd);

  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Dispatching [%s] in [%s]...",
           p_cmd->c_str (),
           graph_state_to_str (p_graph->current_graph_state_));

  switch(p_cmd->get_type ())
    {
    case tizgraphcmd::ETIZGraphCmdLoad:
      {
        rc = p_graph->do_load ();
        break;
      }
    case tizgraphcmd::ETIZGraphCmdConfig:
      {
        rc = p_graph->do_configure (p_cmd->get_config ());
        break;
      }
    case tizgraphcmd::ETIZGraphCmdExecute:
      {
        rc = p_graph->do_execute ();
        break;
      }
    case tizgraphcmd::ETIZGraphCmdPause:
      {
        rc = p_graph->do_pause ();
        break;
      }
    case tizgraphcmd::ETIZGraphCmdSeek:
      {
        rc = p_graph->do_seek ();
        break;
      }
    case tizgraphcmd::ETIZGraphCmdSkip:
      {
        rc = p_graph->do_skip (p_cmd->get_jump ());
        break;
      }
    case tizgraphcmd::ETIZGraphCmdVolume:
      {
        rc = p_graph->do_volume (p_cmd->get_jump ());
        break;
      }
    case tizgraphcmd::ETIZGraphCmdMute:
      {
        rc = p_graph->do_mute ();
        break;
      }
    case tizgraphcmd::ETIZGraphCmdUnload:
      {
        p_graph->do_unload ();
        break;
      }
    case tizgraphcmd::ETIZGraphCmdEos:
      {
        p_graph->do_eos (p_cmd->get_handle ());
        break;
      }
    case tizgraphcmd::ETIZGraphCmdError:
      {
        p_graph->do_error (p_cmd->get_error ());
        break;
      }
    default:
      assert (0);
    };

  if (OMX_ErrorNone != rc)
    {
      p_graph->notify_graph_error (rc, std::string ());
    }
}
