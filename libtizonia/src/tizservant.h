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

#include "OMX_Core.h"
#include "OMX_Types.h"

#include "tizosalpqueue.h"
#include "tizapi.h"
#include "tizscheduler.h"

  /* factory_new(tizservant, ...); */
  extern const void *tizservant;
  extern const void *tizservant_class;

  void tizservant_set_allocator (void *ap_obj, tiz_soa_t *p_soa);

  void tizservant_set_callbacks (void *ap_obj, OMX_PTR ap_appdata,
                                 OMX_CALLBACKTYPE * ap_cbacks);

  OMX_ERRORTYPE tizservant_tick (const void *ap_obj);

  OMX_PTR tizservant_init_msg (void *ap_obj, size_t msg_sz);

  OMX_ERRORTYPE tizservant_enqueue (const void *ap_obj, OMX_PTR ap_data,
                                    OMX_U32 a_priority);

  OMX_ERRORTYPE tizservant_remove_from_queue (const void *ap_obj,
                                              tiz_pq_func_f apf_func,
                                              OMX_S32 a_data1, OMX_PTR ap_data2);

  OMX_ERRORTYPE tizservant_dispatch_msg (const void *ap_obj, OMX_PTR ap_data);

  OMX_HANDLETYPE tizservant_get_hdl (const void *ap_obj);

  OMX_BOOL tizservant_is_ready (const void *ap_obj);

  OMX_ERRORTYPE tizservant_allocate_resources (const void *ap_obj,
                                               OMX_U32 a_pid);

  OMX_ERRORTYPE tizservant_deallocate_resources (const void *ap_obj);

  OMX_ERRORTYPE tizservant_prepare_to_transfer (const void *ap_obj,
                                                OMX_U32 a_pid);

  OMX_ERRORTYPE tizservant_transfer_and_process (const void *ap_obj,
                                                 OMX_U32 a_pid);

  OMX_ERRORTYPE tizservant_stop_and_return (const void *ap_obj);

  /*
   * Event/callback handling
   */

  void tizservant_issue_event (const void *ap_obj, OMX_EVENTTYPE a_event,
                               OMX_U32 a_data1, OMX_U32 a_data2,
                               OMX_PTR ap_eventdata);

  void tizservant_issue_err_event (const void *ap_obj, OMX_ERRORTYPE a_error);

  void tizservant_issue_cmd_event (const void *ap_obj, OMX_COMMANDTYPE a_cmd,
                                   OMX_U32 a_pid, OMX_ERRORTYPE a_error);

  void tizservant_issue_trans_event (const void *ap_obj, OMX_STATETYPE a_state,
                                     OMX_ERRORTYPE a_error);

  void tizservant_issue_buf_callback (const void *ap_obj,
                                      OMX_BUFFERHEADERTYPE * p_hdr,
                                      OMX_U32 pid,
                                      OMX_DIRTYPE dir,
                                      OMX_HANDLETYPE ap_tcomp);

  /* Pluggable events interface */
  OMX_ERRORTYPE tizservant_receive_pluggable_event (const void *ap_obj,
                                                    OMX_HANDLETYPE ap_hdl,
                                                    tizevent_t * ap_event);

  void init_tizservant (void);

#ifdef __cplusplus
}
#endif

#endif /* TIZSERVANT_H */
