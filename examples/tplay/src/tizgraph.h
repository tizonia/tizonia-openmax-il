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
 * @file   tizgraph.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph base class
 *
 *
 */

#ifndef TIZGRAPH_H
#define TIZGRAPH_H

#include "tizgraphtypes.h"
#include "tizprobe.h"
#include "tizplaylist.h"

#include <tizosal.h>

#include <OMX_Core.h>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

// Forward declarations
void * g_graph_thread_func (void *p_arg);
class  tizgraphcmd;

struct waitevent_info;
typedef std::list<waitevent_info> waitevent_list_t;

OMX_ERRORTYPE
tizgraph_event_handler (OMX_HANDLETYPE hComponent,
                        OMX_PTR pAppData,
                        OMX_EVENTTYPE eEvent,
                        OMX_U32 nData1,
                        OMX_U32 nData2,
                        OMX_PTR pEventData);

struct waitevent_info
{
  waitevent_info(OMX_HANDLETYPE component,
                 OMX_EVENTTYPE event,
                 OMX_U32 ndata1,
                 OMX_U32 ndata2,
                 OMX_PTR pEventData)
    :
    component_ (component),
    event_ (event),
    ndata1_ (ndata1),
    ndata2_ (ndata2),
    pEventData_ (pEventData)
  {}

  bool operator==(const waitevent_info& b)
  {
    if (component_ == b.component_
        && event_  == b.event_
        && ndata1_ == b.ndata1_
        && ndata2_ == b.ndata2_)
      // TODO: Ignore pEventData for now. This is to make events like this pass the comparison:
      // e.g.: [tizgraph.cc:receive_event:238] --- [OMX.Aratelia.file_reader.binary] : [OMX_EventCmdComplete] [OMX_CommandStateSet] [OMX_StateLoaded] error [0x80001017]
      //       && pEventData_ == b.pEventData_)
      {
        return true;
      }
    return false;
  }

  OMX_HANDLETYPE component_;
  OMX_EVENTTYPE event_;
  OMX_U32 ndata1_;
  OMX_U32 ndata2_;
  OMX_PTR pEventData_;
};

class tizcback_handler
{

public:

  explicit tizcback_handler (const tizgraph &graph);
  ~tizcback_handler () {};

  OMX_CALLBACKTYPE *get_omx_cbacks ()
  {
    return &cbacks_;
  }
  
  OMX_ERRORTYPE wait_for_event_list (const waitevent_list_t &event_list);

  void receive_event (OMX_HANDLETYPE component,
                      OMX_EVENTTYPE event,
                      OMX_U32 ndata1,
                      OMX_U32 ndata2,
                      OMX_PTR pEventData);

protected:

  bool all_events_received ();

protected:

  const tizgraph &parent_;
  boost::mutex mutex_;
  boost::condition_variable cond_;
  bool events_outstanding_;
  OMX_CALLBACKTYPE cbacks_;
  waitevent_list_t received_queue_;
  waitevent_list_t expected_list_;
};

class tizgraph
{

  friend class tizcback_handler;
  friend       void* ::g_graph_thread_func (void *);

public:

  tizgraph(int graph_size, tizprobe_ptr_t probe);
  virtual ~tizgraph();

  OMX_ERRORTYPE load ();
  OMX_ERRORTYPE configure (const tizgraphconfig_ptr_t config);
  OMX_ERRORTYPE execute ();
  OMX_ERRORTYPE pause ();
  OMX_ERRORTYPE seek ();
  OMX_ERRORTYPE skip (const int jump);
  OMX_ERRORTYPE volume (const int step);
  OMX_ERRORTYPE mute ();
  void unload();

  void set_manager (tizgraphmgr_t *ap_graph_mgr);
  bool at_beginning_of_play () const;
  bool at_end_of_play () const;
  
protected:

  virtual OMX_ERRORTYPE do_load ()                                        = 0;
  virtual OMX_ERRORTYPE do_configure (const tizgraphconfig_ptr_t &config) = 0;
  virtual OMX_ERRORTYPE do_execute ()                                     = 0;
  virtual OMX_ERRORTYPE do_pause ()                                       = 0;
  virtual OMX_ERRORTYPE do_seek ()                                        = 0;
  virtual OMX_ERRORTYPE do_skip (const int jump)                          = 0;
  virtual OMX_ERRORTYPE do_volume (const int step)                        = 0;
  virtual OMX_ERRORTYPE do_mute ()                                        = 0;
  virtual void do_error (const OMX_ERRORTYPE error)                       = 0;
  virtual void do_eos (const OMX_HANDLETYPE handle)                       = 0;
  virtual void do_unload ()                                               = 0;

  virtual OMX_ERRORTYPE init ();
  virtual OMX_ERRORTYPE deinit ();

protected:
  
  void eos (OMX_HANDLETYPE handle);
  void error (const OMX_ERRORTYPE error);

  OMX_ERRORTYPE verify_existence (const component_names_t &comp_list) const;
  OMX_ERRORTYPE verify_role (const std::string &comp,
                            const std::string &role) const;
  OMX_ERRORTYPE verify_role_list (const component_names_t &comp_list,
                                  const component_roles_t &role_list) const;

  OMX_ERRORTYPE instantiate_component (const std::string &comp,
                                      int graph_position);
  OMX_ERRORTYPE instantiate_list (const component_names_t &comp_list);
  void destroy_list();

  virtual OMX_ERRORTYPE probe_uri (const int uri_index, const bool quiet = false) = 0;
  virtual void dump_graph_info (const char *ap_coding_type_str,
                                const char *ap_graph_type_str,
                                const std::string &uri) const;
  virtual void dump_pcm_info (const OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype) const;
  virtual void dump_mp3_info (const OMX_AUDIO_PARAM_MP3TYPE &mp3type) const;
  void dump_mp3_and_pcm_info (const OMX_AUDIO_PARAM_MP3TYPE &mp3type,
                              const OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype) const;
  virtual void dump_stream_info (const std::string &title, const std::string &artist,
                                 const std::string &file_path) const;

  OMX_ERRORTYPE setup_tunnels () const;
  OMX_ERRORTYPE tear_down_tunnels () const;

  OMX_ERRORTYPE setup_suppliers () const;

  OMX_ERRORTYPE transition_all (const OMX_STATETYPE to,
                                const OMX_STATETYPE from);
  OMX_ERRORTYPE transition_one (const int handle_id,
                                const OMX_STATETYPE to);
  OMX_ERRORTYPE apply_volume (const OMX_HANDLETYPE handle, const OMX_U32 pid,
                              const int step);
  OMX_ERRORTYPE apply_mute (const OMX_HANDLETYPE handle, const OMX_U32 pid);
  OMX_ERRORTYPE modify_tunnel (const int tunnel_id, const OMX_COMMANDTYPE cmd);
  OMX_ERRORTYPE disable_tunnel (const int tunnel_id);
  OMX_ERRORTYPE enable_tunnel (const int tunnel_id);

  OMX_ERRORTYPE send_cmd (tizgraphcmd *p_cmd);

  OMX_ERRORTYPE notify_graph_end_of_play ();
  void notify_graph_error (const OMX_ERRORTYPE error, const std::string &msg);

  static void dispatch (tizgraph *p_graph, const tizgraphcmd *p_cmd);

protected:

  handle_to_name_t      h2n_;
  component_handles_t   handles_;
  tizcback_handler      cback_handler_;
  std::string           uri_;
  tizprobe_ptr_t        probe_ptr_;
  tiz_thread_t          thread_;
  tiz_mutex_t           mutex_;
  tiz_sem_t             sem_;
  tiz_queue_t          *p_queue_;
  OMX_STATETYPE         current_graph_state_;
  tizplaylist_t         playlist_;
  uri_list_t            file_list_;
  int                   current_file_index_;
  tizgraphconfig_ptr_t  config_;
  tizgraphmgr_t     *     p_mgr_;
};

#endif // TIZGRAPH_H
