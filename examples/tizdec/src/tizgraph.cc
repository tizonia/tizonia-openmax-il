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
 * @file   tizgraph.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph base class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <assert.h>
#include <algorithm>
#include <boost/foreach.hpp>

#include "OMX_Component.h"
#include "tizgraph.hh"
#include "tizomxutil.hh"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.graph"
#endif

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

tizcback_handler::~tizcback_handler(){}

void
tizcback_handler::receive_event(OMX_HANDLETYPE hComponent,
                                OMX_EVENTTYPE eEvent,
                                OMX_U32 nData1,
                                OMX_U32 nData2,
                                OMX_PTR pEventData)
{
  boost::lock_guard<boost::mutex> lock(mutex_);
  if (eEvent == OMX_EventCmdComplete)
    {
      TIZ_LOG (TIZ_LOG_DEBUG, "[%s] : "
               "eEvent = [%s] event [%s] state [%s] error [%p]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent),
               tiz_cmd_to_str ((OMX_COMMANDTYPE)nData1),
               tiz_state_to_str ((OMX_STATETYPE)nData2),
               pEventData);
    }
  else if (eEvent == OMX_EventPortSettingsChanged)
    {
      TIZ_LOG (TIZ_LOG_DEBUG, "[%s] : "
               "eEvent = [%s] port [%lu] index [%s] pEventData [%p]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent), nData1,
               tiz_idx_to_str ((OMX_INDEXTYPE)nData2),
               pEventData);
    }
  else if (eEvent == OMX_EventBufferFlag)
    {
      TIZ_LOG (TIZ_LOG_DEBUG, "[%s] : "
               "eEvent = [%s] port [%lu] flags [%lX] pEventData [%p]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent), nData1, nData2,
               pEventData);
    }
  else if (eEvent == OMX_EventError)
    {
      TIZ_LOG (TIZ_LOG_DEBUG, "[%s] : "
               "eEvent = [%s] error [%s] port [%lu] pEventData [%p]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent), tiz_err_to_str ((OMX_ERRORTYPE)nData1),
               nData2, pEventData);
    }
  else if (eEvent == OMX_EventVendorStartUnused)
    {
      TIZ_LOG (TIZ_LOG_DEBUG, "[%s] : eEvent = [%s]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent));
      expected_list_.clear();
      events_outstanding_ = false;
    }
  else
    {
      TIZ_LOG (TIZ_LOG_DEBUG, "Received from [%s] : "
               "eEvent = [%s]\n",
               const_cast<tizgraph &>(parent_).h2n_[hComponent].c_str(),
               tiz_evt_to_str (eEvent));
    }

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

int
tizcback_handler::wait_for_event_list (const waitevent_list_t &event_list)
{
  boost::unique_lock<boost::mutex> lock(mutex_);

  assert (expected_list_.empty() == true);
  expected_list_ = event_list;
  events_outstanding_ = true;

  TIZ_LOG (TIZ_LOG_DEBUG, "events_outstanding_ = [%s] event_list size [%d]\n",
           events_outstanding_ ? "true" : "false", event_list.size());

  if (!all_events_received ())
    {
      while(events_outstanding_)
        {
          cond_.wait(lock);
        }
    }

  events_outstanding_ = false;
}

bool
tizcback_handler::all_events_received ()
{
  if (!expected_list_.empty() && !received_queue_.empty())
    {
      waitevent_list_t::iterator exp_it = expected_list_.begin();
      while (exp_it != expected_list_.end())
        {
          waitevent_list_t::iterator queue_end = received_queue_.end();
          waitevent_list_t::iterator queue_it = queue_end;
          if (queue_end != (queue_it = std::find(received_queue_.begin(),
                                                 received_queue_.end(),
                                                 *exp_it)))
            {
              expected_list_.erase (exp_it);
              received_queue_.erase(queue_it);
              TIZ_LOG (TIZ_LOG_DEBUG, "Erased [%s] "
                       "from component [%s] "
                       "event_list size [%d]\n",
                       const_cast<tizgraph &>(parent_).
                       h2n_[exp_it->component_].c_str(),
                       tiz_evt_to_str (exp_it->event_),
                       expected_list_.size());
              // restart the loop
              exp_it = expected_list_.begin();
            }
          else
            {
              ++exp_it;
            }
        }
    }

  return (expected_list_.empty() ? true : false);
}


//
// tizgraph
//
tizgraph::tizgraph(int graph_size, tizprobe_ptr_t probe_ptr)
  :
  h2n_(),
  handles_(graph_size, OMX_HANDLETYPE(NULL)),
  cback_handler_(*this),
  probe_ptr_(probe_ptr)
{
  assert (probe_ptr);
  tizomxutil::init();
}

tizgraph::~tizgraph()
{
  tizomxutil::deinit();
}

OMX_ERRORTYPE
tizgraph::verify_existence(const component_names_t &comp_list) const
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
tizgraph::verify_role(const std::string &comp,
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
tizgraph::verify_role_list(const component_names_t &comp_list,
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
tizgraph::instantiate_component(const std::string &comp_name, int graph_position)
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
tizgraph::instantiate_list(const component_names_t &comp_list)
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
tizgraph::destroy_list()
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

OMX_ERRORTYPE
tizgraph::setup_tunnels() const
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int handle_lst_size = handles_.size();

  for (int i=0; i<handle_lst_size-1 && OMX_ErrorNone == error; i++)
    {
      error = OMX_SetupTunnel(handles_[i], i==0 ? 0 : 1, handles_[i+1], 0);
    }

  return error;
}

OMX_ERRORTYPE
tizgraph::tear_down_tunnels() const
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int handle_lst_size = handles_.size();

  for (int i=0; i<handle_lst_size-1 && OMX_ErrorNone == error; i++)
    {
      error = OMX_TeardownTunnel(handles_[i], i==0 ? 0 : 1, handles_[i+1], 0);
    }

  return error;
}

OMX_ERRORTYPE
tizgraph::setup_suppliers() const
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
  int handle_lst_size = handles_.size();

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

}

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

OMX_ERRORTYPE
tizgraph::transition_all (const OMX_STATETYPE to,
                          const OMX_STATETYPE from)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  if ((to == OMX_StateIdle && from == OMX_StateLoaded)
      ||
      (to == OMX_StateExecuting && from == OMX_StateIdle))
    {
      // Non-suppliers first, hence front to back order
      error = (std::for_each(handles_.begin(), handles_.end(),
                             transition_to(to))).error_;
    }
  else
    {
      // Suppliers first, hence back to front order
      error = (for_each(handles_.rbegin(), handles_.rend(),
                        transition_to(to, 10000))).error_;
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
      cback_handler_.wait_for_event_list(event_list);
    }

  return error;
}
