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
extern "C"
{
#endif

#include "tizapi.h"
#include "tizscheduler.h"

#include "OMX_Core.h"
#include "OMX_Types.h"


  extern const void *tizsrv;
  extern const void *tizsrv_class;
  OMX_ERRORTYPE tiz_srv_init (void);

  OMX_ERRORTYPE tiz_srv_set_allocator (void *ap_obj, tiz_soa_t * p_soa);

  void tiz_srv_set_callbacks (void *ap_obj, OMX_PTR ap_appdata,
                                 OMX_CALLBACKTYPE * ap_cbacks);

  OMX_ERRORTYPE tiz_srv_tick (const void *ap_obj);

  OMX_PTR tiz_srv_init_msg (void *ap_obj, size_t msg_sz);

  OMX_ERRORTYPE tiz_srv_enqueue (const void *ap_obj, OMX_PTR ap_data,
                                    OMX_U32 a_priority);

  OMX_ERRORTYPE tiz_srv_remove_from_queue (const void *ap_obj,
                                              tiz_pq_func_f apf_func,
                                              OMX_S32 a_data1,
                                              OMX_PTR ap_data2);

  OMX_ERRORTYPE tiz_srv_dispatch_msg (const void *ap_obj, OMX_PTR ap_data);

  OMX_BOOL tiz_srv_is_ready (const void *ap_obj);

  OMX_ERRORTYPE tiz_srv_allocate_resources (const void *ap_obj,
                                               OMX_U32 a_pid);

  OMX_ERRORTYPE tiz_srv_deallocate_resources (const void *ap_obj);

  OMX_ERRORTYPE tiz_srv_prepare_to_transfer (const void *ap_obj,
                                                OMX_U32 a_pid);

  OMX_ERRORTYPE tiz_srv_transfer_and_process (const void *ap_obj,
                                                 OMX_U32 a_pid);

  OMX_ERRORTYPE tiz_srv_stop_and_return (const void *ap_obj);

  /*
   * Event/callback handling
   */

  void tiz_srv_issue_event (const void *ap_obj, OMX_EVENTTYPE a_event,
                               OMX_U32 a_data1, OMX_U32 a_data2,
                               /*@null@*/ OMX_PTR ap_eventdata);

  void tiz_srv_issue_err_event (const void *ap_obj, OMX_ERRORTYPE a_error);

  void tiz_srv_issue_cmd_event (const void *ap_obj, OMX_COMMANDTYPE a_cmd,
                                   OMX_U32 a_pid, OMX_ERRORTYPE a_error);

  void tiz_srv_issue_trans_event (const void *ap_obj,
                                     OMX_STATETYPE a_state,
                                     OMX_ERRORTYPE a_error);

  void tiz_srv_issue_buf_callback (const void *ap_obj,
                                      OMX_BUFFERHEADERTYPE * p_hdr,
                                      OMX_U32 pid,
                                      OMX_DIRTYPE dir,
                                      OMX_HANDLETYPE ap_tcomp);

  /* Pluggable events interface */
  OMX_ERRORTYPE tiz_srv_receive_pluggable_event (const void *ap_obj,
                                                    OMX_HANDLETYPE ap_hdl,
                                                    tiz_event_pluggable_t * ap_event);

#ifdef __cplusplus
}
#endif

#endif                          /* TIZSERVANT_H */
