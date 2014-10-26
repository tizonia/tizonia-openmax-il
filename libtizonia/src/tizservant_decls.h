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
 * @file   tizservant_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Servant class
 *
 *
 */

#ifndef TIZSERVANT_DECLS_H
#define TIZSERVANT_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tizscheduler.h"
#include "tizapi_decls.h"

typedef struct tiz_srv tiz_srv_t;
struct tiz_srv
{
  /* Object */
  const tiz_api_t _;
  tiz_pqueue_t *p_pq_;
  tiz_soa_t *p_soa_; /* Not owned */
  tiz_map_t *p_watchers_;
  uint32_t watcher_id_;
  OMX_PTR p_appdata_;
  OMX_CALLBACKTYPE *p_cbacks_;
};

OMX_ERRORTYPE tiz_srv_super_tick (const void *class, const void *ap_obj);

OMX_ERRORTYPE tiz_srv_super_enqueue (const void *class, const void *ap_obj,
                                     OMX_PTR ap_data, OMX_U32 a_priority);

void tiz_srv_super_remove_from_queue (const void *class, const void *ap_obj,
                                      tiz_pq_func_f apf_func, OMX_S32 a_data1,
                                      OMX_PTR ap_data2);

OMX_ERRORTYPE tiz_srv_super_dispatch_msg (const void *class, const void *ap_obj,
                                          OMX_PTR ap_data);

bool tiz_srv_super_is_ready (const void *class, const void *ap_obj);

OMX_ERRORTYPE tiz_srv_super_allocate_resources (const void *a_class,
                                                const void *ap_obj,
                                                OMX_U32 a_pid);

OMX_ERRORTYPE tiz_srv_super_deallocate_resources (const void *a_class,
                                                  const void *ap_obj);

OMX_ERRORTYPE tiz_srv_super_prepare_to_transfer (const void *a_class,
                                                 const void *ap_obj,
                                                 OMX_U32 a_pid);

OMX_ERRORTYPE tiz_srv_super_transfer_and_process (const void *a_class,
                                                  const void *ap_obj,
                                                  OMX_U32 a_pid);

OMX_ERRORTYPE tiz_srv_super_stop_and_return (const void *a_class,
                                             const void *ap_obj);

typedef struct tiz_srv_class tiz_srv_class_t;
struct tiz_srv_class
{
  /* Class */
  const tiz_api_class_t _;
  OMX_ERRORTYPE (*set_allocator)(void *ap_obj, tiz_soa_t *p_soa);
  void (*set_callbacks)(void *ap_obj, OMX_PTR ap_appdata,
                        OMX_CALLBACKTYPE *ap_cbacks);
  OMX_ERRORTYPE (*tick)(const void *ap_obj);
  OMX_PTR (*init_msg)(void *ap_obj, size_t msg_sz);
  OMX_ERRORTYPE (*enqueue)(const void *ap_obj, OMX_PTR ap_data,
                           OMX_U32 a_priority);
  void (*remove_from_queue)(const void *ap_obj, tiz_pq_func_f apf_func,
                            OMX_S32 a_data1, OMX_PTR ap_data2);
  OMX_ERRORTYPE (*dispatch_msg)(const void *ap_obj, OMX_PTR ap_data);
  bool (*is_ready)(const void *ap_obj);
  OMX_ERRORTYPE (*allocate_resources)(const void *ap_obj, OMX_U32 a_pid);
  OMX_ERRORTYPE (*deallocate_resources)(const void *ap_obj);
  OMX_ERRORTYPE (*prepare_to_transfer)(const void *ap_obj, OMX_U32 a_pid);
  OMX_ERRORTYPE (*transfer_and_process)(const void *ap_obj, OMX_U32 a_pid);
  OMX_ERRORTYPE (*stop_and_return)(const void *ap_obj);
  void (*issue_event)(const void *ap_obj, OMX_EVENTTYPE a_event,
                      OMX_U32 a_data1, OMX_U32 a_data2,
                      /*@null@*/ OMX_PTR ap_eventdata);
  void (*issue_err_event)(const void *ap_obj, OMX_ERRORTYPE a_error);
  void (*issue_cmd_event)(const void *ap_obj, OMX_COMMANDTYPE a_cmd,
                          OMX_U32 a_pid, OMX_ERRORTYPE a_error);
  void (*issue_trans_event)(const void *ap_obj, OMX_STATETYPE a_state,
                            OMX_ERRORTYPE a_error);
  void (*issue_buf_callback)(const void *ap_obj, OMX_BUFFERHEADERTYPE *p_hdr,
                             OMX_U32 pid, OMX_DIRTYPE dir,
                             OMX_HANDLETYPE ap_tcomp);
  OMX_ERRORTYPE (*receive_pluggable_event)(void *ap_obj,
                                           tiz_event_pluggable_t *ap_event);

  void *(*soa_calloc)(void *ap_obj, size_t a_size);
  void (*soa_free)(void *ap_obj, void *ap_addr);

  OMX_ERRORTYPE (*io_watcher_init)(void *ap_obj, tiz_event_io_t **app_ev_io,
                                   int a_fd, tiz_event_io_event_t a_event,
                                   bool only_once);
  OMX_ERRORTYPE (*io_watcher_start)(void *ap_obj, tiz_event_io_t *ap_ev_io);
  OMX_ERRORTYPE (*io_watcher_stop)(void *ap_obj, tiz_event_io_t *ap_ev_io);
  void (*io_watcher_destroy)(void *ap_obj, tiz_event_io_t *ap_ev_io);

  OMX_ERRORTYPE (*timer_watcher_init)(void *ap_obj,
                                      tiz_event_timer_t **app_ev_timer);
  OMX_ERRORTYPE (*timer_watcher_start)(void *ap_obj,
                                       tiz_event_timer_t *ap_ev_timer,
                                       const double a_after,
                                       const double a_repeat);
  OMX_ERRORTYPE (*timer_watcher_restart)(void *ap_obj,
                                         tiz_event_timer_t *ap_ev_timer);
  OMX_ERRORTYPE (*timer_watcher_stop)(void *ap_obj,
                                      tiz_event_timer_t *ap_ev_timer);
  void (*timer_watcher_destroy)(void *ap_obj, tiz_event_timer_t *ap_ev_timer);

  OMX_ERRORTYPE (*event_io)(void *ap_obj, tiz_event_io_t *ap_ev_io,
                            const uint32_t a_id, int a_fd, int a_events);
  OMX_ERRORTYPE (*event_timer)(void *ap_obj, tiz_event_timer_t *ap_ev_timer,
                               const uint32_t a_id);
  OMX_ERRORTYPE (*event_stat)(void *ap_obj, tiz_event_stat_t *ap_ev_stat,
                              const uint32_t a_id, int a_events);
  OMX_ERRORTYPE (*io_ready)(void *ap_obj, tiz_event_io_t *ap_ev_io, int a_fd,
                            int a_events);
  OMX_ERRORTYPE (*timer_ready)(void *ap_obj, tiz_event_timer_t *ap_ev_timer);
  OMX_ERRORTYPE (*stat_ready)(void *ap_obj, tiz_event_stat_t *ap_ev_stat,
                              int a_events);
};

#ifdef __cplusplus
}
#endif

#endif /* TIZSERVANT_DECLS_H */
