/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizkernel.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - kernel class
 *
 *
 */

#ifndef TIZKERNEL_H
#define TIZKERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 *@defgroup tizkernel 'tizkernel' : libtizonia's 'kernel' servant
 *
 * The 'kernel' servant is responsible for all the OpenMAX IL port management
 * functionality.
 *
 *@ingroup libtizonia
 */

#include <OMX_Core.h>
#include <OMX_Component.h>

#include "tizutils.h"

void *tiz_krn_class_init (void *ap_tos, void *ap_hdl);
void *tiz_krn_init (void *ap_tos, void *ap_hdl);

typedef enum tiz_krn_population_status tiz_krn_population_status_t;
enum tiz_krn_population_status
{
  ETIZKrnFullyPopulated,
  ETIZKrnUnpopulated,
  ETIZKrnFullyUnpopulated,
};

typedef enum tiz_krn_restriction tiz_krn_restriction_t;
enum tiz_krn_restriction
{
  ETIZKrnMayInitiateAllocPhase,
  ETIZKrnMayExchangeBuffers,
  ETIZKrnMayInitiateExeToIdle,
  ETIZKrnMayMax,
};

OMX_ERRORTYPE tiz_krn_register_port (const void *ap_obj, OMX_PTR ap_port,
                                     const bool ais_config);
void tiz_krn_deregister_all_ports (void *ap_obj);
void *tiz_krn_get_port (const void *ap_obj, const OMX_U32 a_pid);
OMX_ERRORTYPE tiz_krn_find_managing_port (const void *ap_obj,
                                          const OMX_INDEXTYPE a_index,
                                          const OMX_PTR ap_struct,
                                          OMX_PTR *app_port);

/** \deprecated
 */
OMX_ERRORTYPE tiz_krn_select (const void *ap_obj, const OMX_U32 a_nports,
                              /*@out@*/ tiz_pd_set_t *ap_set);
OMX_ERRORTYPE tiz_krn_claim_buffer (const void *ap_obj, const OMX_U32 a_pid,
                                    const OMX_U32 a_pos,
                                    OMX_BUFFERHEADERTYPE **p_hdr);
OMX_ERRORTYPE tiz_krn_release_buffer (const void *ap_obj, const OMX_U32 a_pid,
                                      OMX_BUFFERHEADERTYPE *ap_hdr);
void tiz_krn_reset_tunneled_ports_status (void *ap_obj,
                                          const OMX_U32 a_port_status_flag);
tiz_krn_population_status_t tiz_krn_get_population_status (
    const void *ap_obj, const OMX_U32 a_pid,
    OMX_BOOL *ap_may_be_fully_unpopulated);
bool tiz_krn_get_restriction_status (const void *ap_obj,
                                     const tiz_krn_restriction_t a_restriction);
void tiz_krn_clear_metadata (void *ap_obj);
OMX_ERRORTYPE tiz_krn_store_metadata (void *ap_obj, const OMX_CONFIG_METADATAITEMTYPE *ap_meta_item);
OMX_ERRORTYPE  tiz_krn_SetParameter_internal (const void *ap_obj,
                                              OMX_HANDLETYPE ap_hdl,
                                              OMX_INDEXTYPE a_index, OMX_PTR ap_struct);
OMX_ERRORTYPE tiz_krn_SetConfig_internal (const void *ap_obj,
                                          OMX_HANDLETYPE ap_hdl,
                                          OMX_INDEXTYPE a_index, OMX_PTR ap_struct);

#define TIZ_KRN_MAY_INIT_ALLOC_PHASE(_p) \
  tiz_krn_get_restriction_status (_p, ETIZKrnMayInitiateAllocPhase)

#define TIZ_KRN_MAY_EXCHANGE_BUFFERS(_p) \
  tiz_krn_get_restriction_status (_p, ETIZKrnMayExchangeBuffers)

#define TIZ_KRN_MAY_INIT_EXE_TO_IDLE(_p) \
  tiz_krn_get_restriction_status (_p, ETIZKrnMayInitiateExeToIdle)

#ifdef __cplusplus
}
#endif

#endif /* TIZKERNEL_H */
