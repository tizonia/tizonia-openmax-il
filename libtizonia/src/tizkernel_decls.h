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
 * @file   tizkernel_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - kernel class declarations
 *
 *
 */

#ifndef TIZKERNEL_DECLS_H
#define TIZKERNEL_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Core.h"

#include "tizkernel.h"
#include "tizservant_decls.h"
#include "tizrmproxy_c.h"

struct tizkernel
{
  /* Object */
  const struct tizservant _;
  tiz_vector_t *p_ports_;
  tiz_vector_t *p_ingress_;
  tiz_vector_t *p_egress_;
  OMX_PTR p_cport_;
  OMX_PTR p_proc_;
  OMX_BOOL eos_;
  tizrm_t rm_;
  tizrm_proxy_callbacks_t rm_cbacks_;
  OMX_PORT_PARAM_TYPE audio_init_;
  OMX_PORT_PARAM_TYPE image_init_;
  OMX_PORT_PARAM_TYPE video_init_;
  OMX_PORT_PARAM_TYPE other_init_;
  OMX_S32 cmd_completion_count_;
};

OMX_ERRORTYPE
tizkernel_super_register_port (const void *a_class, const void *ap_obj,
                               OMX_PTR ap_port, OMX_BOOL ais_config);

tiz_kernel_population_status_t
tizkernel_super_get_population_status (const void *a_class, const void *ap_obj,
                                 OMX_U32 a_pid,
                                 OMX_BOOL *ap_may_be_fully_unpopulated);

OMX_ERRORTYPE
tizkernel_super_select (const void *a_class, const void *ap_obj,
                        OMX_U32 a_nports, tiz_pd_set_t * p_set);

OMX_ERRORTYPE
tizkernel_super_claim_buffer (const void *a_class, const void *ap_obj,
                              OMX_U32 a_port_id, OMX_U32 a_pos,
                              OMX_BUFFERHEADERTYPE ** p_hdr);

OMX_ERRORTYPE
tizkernel_super_relinquish_buffer (const void *a_class, const void *ap_obj,
                                   OMX_U32 a_port_id,
                                   OMX_BUFFERHEADERTYPE * ap_hdr);

void
tizkernel_super_deregister_all_ports (const void *a_class, void *ap_obj);

struct tizkernel_class
{
  /* Class */
  const struct tizservant_class _;
  OMX_ERRORTYPE (*register_port) (const void *ap_obj, OMX_PTR ap_port,
                                  OMX_BOOL ais_config);
  void *(*get_port) (const void *ap_obj, OMX_U32 a_pid);
  OMX_ERRORTYPE (*find_managing_port) (const void *ap_obj,
                                       OMX_INDEXTYPE a_index,
                                       OMX_PTR ap_struct,
                                       OMX_PTR * app_port);
  tiz_kernel_population_status_t (*get_population_status)
  (const void *ap_obj,
   OMX_U32 a_pid,
   OMX_BOOL
   *ap_may_be_fully_unpopulated);
  OMX_ERRORTYPE (*select) (const void *p_obj, OMX_U32 a_nports,
                           tiz_pd_set_t * ap_set);
  OMX_ERRORTYPE (*claim_buffer) (const void *ap_obj, OMX_U32 a_port_id,
                                 OMX_U32 a_pos,
                                 OMX_BUFFERHEADERTYPE ** p_hdr);
  OMX_ERRORTYPE (*relinquish_buffer) (const void *ap_obj, OMX_U32 a_port_id,
                                      OMX_BUFFERHEADERTYPE * p_hdr);
  void (*deregister_all_ports) (void *ap_obj);
};

#ifdef __cplusplus
}
#endif

#endif /* TIZKERNEL_DECLS_H */
