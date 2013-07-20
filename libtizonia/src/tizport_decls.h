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
 * @file   tizport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - port class declarations
 *
 *
 */

#ifndef TIZPORT_DECLS_H
#define TIZPORT_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Component.h"
#include "tizapi_decls.h"
#include "tizutils.h"
#include "tizosal.h"

  typedef struct tiz_port tiz_port_t;
  struct tiz_port
  {
    /* Object */
    const tiz_api_t _;
    tiz_vector_t *p_indexes_;
    tiz_vector_t *p_hdrs_info_;
    tiz_vector_t *p_hdrs_;
    tiz_vector_t *p_marks_;
    OMX_U32 pid_;
    OMX_U32 tpid_;
    OMX_S32 claimed_count_;
    OMX_HANDLETYPE thdl_;
    tiz_port_options_t opts_;
    tiz_pd_set_t flags_;
    OMX_PARAM_PORTDEFINITIONTYPE portdef_;
    OMX_BOOL contiguity_pref_;
    OMX_PARAM_BUFFERSUPPLIERTYPE bufsupplier_;
    OMX_BOOL announce_bufs_;
    OMX_CONFIG_TUNNELEDPORTSTATUSTYPE peer_port_status_;
  };

  OMX_ERRORTYPE tiz_port_super_register_index (const void *a_class,
                                               const void *ap_obj,
                                               OMX_INDEXTYPE a_index);

  OMX_ERRORTYPE tiz_port_super_find_index (const void *a_class,
                                           const void *ap_obj,
                                           OMX_INDEXTYPE a_index);

  OMX_ERRORTYPE tiz_port_super_populate (const void *a_class,
                                        const void *ap_obj,
                                        OMX_HANDLETYPE ap_hdl);

  OMX_ERRORTYPE tiz_port_super_depopulate (const void *a_class,
                                           const void *ap_obj);

  OMX_BOOL tiz_port_super_check_tunnel_compat (const void *a_class,
                                               const void *ap_obj,
                                               OMX_PARAM_PORTDEFINITIONTYPE *
                                               ap_this_def,
                                               OMX_PARAM_PORTDEFINITIONTYPE *
                                               ap_other_def);

  OMX_ERRORTYPE tiz_port_super_apply_slaving_behaviour (void *a_class,
                                                        void *ap_obj,
                                                        void *ap_mos_port,
                                                        const OMX_INDEXTYPE
                                                        a_index,
                                                        const OMX_PTR
                                                        ap_struct,
                                                        tiz_vector_t *
                                                        ap_changed_idxs);

  typedef struct tiz_port_class tiz_port_class_t;
  struct tiz_port_class
  {
    /* Class */
    const tiz_api_class_t _;
    OMX_ERRORTYPE (*register_index) (const void *ap_obj,
                                     OMX_INDEXTYPE a_index);
    OMX_ERRORTYPE (*find_index) (const void *ap_obj, OMX_INDEXTYPE a_index);
    OMX_U32 (*index) (const void *ap_obj);
    void (*set_index) (void *ap_obj, OMX_U32 a_pid);
    OMX_ERRORTYPE (*set_portdef_format) (void *ap_obj,
                                         const OMX_PARAM_PORTDEFINITIONTYPE
                                         * ap_pdef);
    OMX_S32 (*buffer_count) (const void *ap_obj);
    OMX_DIRTYPE (*dir) (const void *ap_obj);
    OMX_PORTDOMAINTYPE (*domain) (const void *ap_obj);
    OMX_HANDLETYPE (*get_tunnel_comp) (const void *ap_obj);
    tiz_vector_t *(*get_hdrs_list) (void *ap_obj);
    OMX_BOOL (*check_flags) (const void *ap_obj, OMX_U32 a_nflags,
                             va_list * app);
    void (*set_flags) (const void *ap_obj, OMX_U32 a_nflags, va_list * app);
    void (*clear_flags) (const void *ap_obj, OMX_U32 a_nflags, va_list * app);
    OMX_BOOL (*check_tunneled_port_status) (const void *ap_obj,
                                            OMX_U32 a_post_status);
    OMX_ERRORTYPE (*populate) (const void *ap_obj, OMX_HANDLETYPE ap_hdl);
    OMX_ERRORTYPE (*depopulate) (const void *ap_obj);
    OMX_BOOL (*check_tunnel_compat) (const void *ap_obj,
                                     OMX_PARAM_PORTDEFINITIONTYPE *
                                     ap_this_def,
                                     OMX_PARAM_PORTDEFINITIONTYPE *
                                     ap_other_def);
    OMX_S32 (*update_claimed_count) (void *ap_obj, OMX_S32 a_offset);
    OMX_ERRORTYPE (*store_mark) (void *ap_obj,
                                 const OMX_MARKTYPE * ap_mark_info,
                                 OMX_BOOL a_owned);
    OMX_ERRORTYPE (*mark_buffer) (void *ap_obj,
                                  OMX_BUFFERHEADERTYPE * ap_hdr);
    void (*set_alloc_hooks) (void *ap_obj,
                             const tiz_alloc_hooks_t * ap_new_hooks,
                             tiz_alloc_hooks_t * ap_old_hooks);
    OMX_ERRORTYPE (*populate_header) (const void *ap_obj,
                                      OMX_HANDLETYPE ap_hdl,
                                      OMX_BUFFERHEADERTYPE * ap_hdr);
    void (*depopulate_header) (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                               OMX_BUFFERHEADERTYPE * ap_hdr);
    OMX_BOOL (*is_master_or_slave) (const void *ap_obj,
                                    OMX_U32 * ap_mos_pid);
    OMX_ERRORTYPE (*apply_slaving_behaviour) (void *ap_obj,
                                              void *ap_mos_port,
                                              const OMX_INDEXTYPE a_index,
                                              const OMX_PTR ap_struct,
                                              tiz_vector_t *
                                              ap_changed_idxs);
    void (*update_tunneled_status) (void *ap_obj, OMX_HANDLETYPE ap_hdl,
                                    OMX_U32 a_port_status);
    void (*reset_tunneled_port_status_flag) (void *ap_obj,
                                             OMX_HANDLETYPE ap_hdl,
                                             OMX_U32 a_port_status_flag);
  };

#ifdef __cplusplus
}
#endif

#endif                          /* TIZPORT_DECLS_H */
