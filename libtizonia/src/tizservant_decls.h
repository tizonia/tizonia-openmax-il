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
extern "C"
{
#endif

#include "OMX_Component.h"

#include "tizapi_decls.h"
#include "tizosal.h"

  struct tizservant
  {
    /* Object */
    const struct tizapi _;
    tiz_pqueue_t *p_pq_;
    tiz_soa_t *p_soa_;           /* Not owned */
    OMX_COMPONENTTYPE *p_hdl_;
    OMX_PTR p_appdata_;
    OMX_CALLBACKTYPE *p_cbacks_;
  };

  OMX_ERRORTYPE tizservant_super_tick (const void *class, const void *ap_obj);

  OMX_ERRORTYPE tizservant_super_enqueue (const void *class,
                                          const void *ap_obj,
                                          OMX_PTR ap_data,
                                          OMX_U32 a_priority);

  OMX_ERRORTYPE tizservant_super_remove_from_queue (const void *class,
                                                    const void *ap_obj,
                                                    tiz_pq_func_f apf_func,
                                                    OMX_S32 a_data1,
                                                    OMX_PTR ap_data2);

  OMX_ERRORTYPE tizservant_super_dispatch_msg (const void *class,
                                               const void *ap_obj,
                                               OMX_PTR ap_data);

  OMX_HANDLETYPE tizservant_super_get_hdl (const void *class,
                                           const void *ap_obj);

  OMX_BOOL tizservant_super_is_ready (const void *class, const void *ap_obj);

  OMX_ERRORTYPE tizservant_super_allocate_resources (const void *a_class,
                                                     const void *ap_obj,
                                                     OMX_U32 a_pid);

  OMX_ERRORTYPE tizservant_super_deallocate_resources (const void *a_class,
                                                       const void *ap_obj);

  OMX_ERRORTYPE tizservant_super_prepare_to_transfer (const void *a_class,
                                                      const void *ap_obj,
                                                      OMX_U32 a_pid);

  OMX_ERRORTYPE tizservant_super_transfer_and_process (const void *a_class,
                                                       const void *ap_obj,
                                                       OMX_U32 a_pid);

  OMX_ERRORTYPE tizservant_super_stop_and_return (const void *a_class,
                                                  const void *ap_obj);


  struct tizservant_class
  {
    /* Class */
    const struct tizapi_class _;
    void (*set_allocator) (void *ap_obj, tiz_soa_t *p_soa);
    void (*set_callbacks) (void *ap_obj, OMX_PTR ap_appdata,
                           OMX_CALLBACKTYPE * ap_cbacks);
    OMX_ERRORTYPE (*tick) (const void *ap_obj);
    OMX_PTR (*init_msg) (void *ap_obj, size_t msg_sz);
    OMX_ERRORTYPE (*enqueue) (const void *ap_obj, OMX_PTR ap_data,
                              OMX_U32 a_priority);
    OMX_ERRORTYPE (*remove_from_queue) (const void *ap_obj,
                                        tiz_pq_func_f apf_func,
                                        OMX_S32 a_data1, OMX_PTR ap_data2);
    OMX_ERRORTYPE (*dispatch_msg) (const void *ap_obj, OMX_PTR ap_data);
    OMX_HANDLETYPE (*get_hdl) (const void *ap_obj);
    OMX_BOOL (*is_ready) (const void *ap_obj);
    OMX_ERRORTYPE (*allocate_resources) (const void *ap_obj, OMX_U32 a_pid);
    OMX_ERRORTYPE (*deallocate_resources) (const void *ap_obj);
    OMX_ERRORTYPE (*prepare_to_transfer) (const void *ap_obj, OMX_U32 a_pid);
    OMX_ERRORTYPE (*transfer_and_process) (const void *ap_obj, OMX_U32 a_pid);
    OMX_ERRORTYPE (*stop_and_return) (const void *ap_obj);
    void (*issue_event) (const void *ap_obj, OMX_EVENTTYPE a_event,
                         OMX_U32 a_data1, OMX_U32 a_data2,
                         OMX_PTR ap_eventdata);
    void (*issue_err_event) (const void *ap_obj, OMX_ERRORTYPE a_error);
    void (*issue_cmd_event) (const void *ap_obj, OMX_COMMANDTYPE a_cmd,
                             OMX_U32 a_pid, OMX_ERRORTYPE a_error);
    void (*issue_trans_event) (const void *ap_obj, OMX_STATETYPE a_state,
                               OMX_ERRORTYPE a_error);
    void (*issue_buf_callback) (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr,
                                OMX_U32 pid, OMX_DIRTYPE dir,
                                OMX_HANDLETYPE ap_tcomp);
    OMX_ERRORTYPE (*receive_pluggable_event) (const void *ap_obj,
                                              OMX_HANDLETYPE ap_hdl,
                                              tizevent_t * ap_event);
  };

#ifdef __cplusplus
}
#endif

#endif /* TIZSERVANT_DECLS_H */
