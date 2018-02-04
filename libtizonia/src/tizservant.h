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
 * @file   tizservant.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Servant class
 *
 *
 */

#ifndef TIZSERVANT_H
#define TIZSERVANT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>

#include <tizplatform.h>

#include "tizscheduler.h"
#include "tizapi.h"

void *
tiz_srv_class_init (void * ap_tos, void * ap_hdl);
void *
tiz_srv_init (void * ap_tos, void * ap_hdl);

OMX_ERRORTYPE
tiz_srv_set_allocator (void * ap_obj, tiz_soa_t * p_soa);

void
tiz_srv_set_callbacks (void * ap_obj, OMX_PTR ap_appdata,
                       OMX_CALLBACKTYPE * ap_cbacks);

OMX_ERRORTYPE
tiz_srv_tick (const void * ap_obj);

OMX_PTR
tiz_srv_init_msg (void * ap_obj, size_t msg_sz);

OMX_ERRORTYPE
tiz_srv_enqueue (const void * ap_obj, OMX_PTR ap_data, OMX_U32 a_priority);

void
tiz_srv_remove_from_queue (const void * ap_obj,
                           /*@null@*/ tiz_pq_func_f apf_func, OMX_S32 a_data1,
                           OMX_PTR ap_data2);

OMX_ERRORTYPE
tiz_srv_dispatch_msg (const void * ap_obj, OMX_PTR ap_data);

bool
tiz_srv_is_ready (const void * ap_obj);

OMX_ERRORTYPE
tiz_srv_allocate_resources (const void * ap_obj, OMX_U32 a_pid);

OMX_ERRORTYPE
tiz_srv_deallocate_resources (const void * ap_obj);

OMX_ERRORTYPE
tiz_srv_prepare_to_transfer (const void * ap_obj, OMX_U32 a_pid);

OMX_ERRORTYPE
tiz_srv_transfer_and_process (const void * ap_obj, OMX_U32 a_pid);

OMX_ERRORTYPE
tiz_srv_stop_and_return (const void * ap_obj);

/*
 * Event/callback handling
 */

void
tiz_srv_issue_event (const void * ap_obj, OMX_EVENTTYPE a_event,
                     OMX_U32 a_data1, OMX_U32 a_data2,
                     /*@null@*/ OMX_PTR ap_eventdata);

void
tiz_srv_issue_err_event (const void * ap_obj, OMX_ERRORTYPE a_error);

void
tiz_srv_issue_err_event_with_data (const void * ap_obj, OMX_ERRORTYPE a_error,
                                   OMX_STRING ap_msg);

void
tiz_srv_issue_cmd_event (const void * ap_obj, OMX_COMMANDTYPE a_cmd,
                         OMX_U32 a_pid, OMX_ERRORTYPE a_error);

void
tiz_srv_issue_trans_event (const void * ap_obj, OMX_STATETYPE a_state,
                           OMX_ERRORTYPE a_error);

void
tiz_srv_issue_buf_callback (const void * ap_obj, OMX_BUFFERHEADERTYPE * p_hdr,
                            OMX_U32 pid, OMX_DIRTYPE dir,
                            OMX_HANDLETYPE ap_tcomp);

/* Pluggable events interface */
OMX_ERRORTYPE
tiz_srv_receive_pluggable_event (void * ap_obj,
                                 tiz_event_pluggable_t * ap_event);

/* Small object allocator access */
void *
tiz_srv_soa_calloc (void * ap_obj, size_t a_size);
void
tiz_srv_soa_free (void * ap_obj, void * ap_addr);

/* io event helpers */
OMX_ERRORTYPE
tiz_srv_io_watcher_init (void * ap_obj, tiz_event_io_t ** app_ev_io, int a_fd,
                         tiz_event_io_event_t a_event, bool only_once);
OMX_ERRORTYPE
tiz_srv_io_watcher_start (void * ap_obj, tiz_event_io_t * ap_ev_io);
OMX_ERRORTYPE
tiz_srv_io_watcher_stop (void * ap_obj, tiz_event_io_t * ap_ev_io);
void
tiz_srv_io_watcher_destroy (void * ap_obj, tiz_event_io_t * ap_ev_io);

/* timer event helpers */
OMX_ERRORTYPE
tiz_srv_timer_watcher_init (void * ap_obj, tiz_event_timer_t ** app_ev_timer);
OMX_ERRORTYPE
tiz_srv_timer_watcher_start (void * ap_obj, tiz_event_timer_t * ap_ev_timer,
                             const double a_after, const double a_repeat);
OMX_ERRORTYPE
tiz_srv_timer_watcher_restart (void * ap_obj, tiz_event_timer_t * ap_ev_timer);
OMX_ERRORTYPE
tiz_srv_timer_watcher_stop (void * ap_obj, tiz_event_timer_t * ap_ev_timer);
void
tiz_srv_timer_watcher_destroy (void * ap_obj, tiz_event_timer_t * ap_ev_timer);

/* TODO: stat event helpers */

/* Event filters */
OMX_ERRORTYPE
tiz_srv_event_io (void * ap_obj, tiz_event_io_t * ap_ev_io, const uint32_t a_id,
                  int a_fd, int a_events);
OMX_ERRORTYPE
tiz_srv_event_timer (void * ap_obj, tiz_event_timer_t * ap_ev_timer,
                     const uint32_t a_id);
OMX_ERRORTYPE
tiz_srv_event_stat (void * ap_obj, tiz_event_stat_t * ap_ev_stat,
                    const uint32_t a_id, int a_events);

/* Event handlers */
OMX_ERRORTYPE
tiz_srv_io_ready (void * ap_obj, tiz_event_io_t * ap_ev_io, int a_fd,
                  int a_events);
OMX_ERRORTYPE
tiz_srv_timer_ready (void * ap_obj, tiz_event_timer_t * ap_ev_timer);
OMX_ERRORTYPE
tiz_srv_stat_ready (void * ap_obj, tiz_event_stat_t * ap_ev_stat, int a_events);

#ifdef __cplusplus
}
#endif

#endif /* TIZSERVANT_H */
