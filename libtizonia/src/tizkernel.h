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
extern "C"
{
#endif

#include "OMX_Core.h"

#include "tizscheduler.h"
#include "tizservant.h"
#include "tizutils.h"

  /* factory_new(tizkernel, ...) */
  extern const void *tizkernel, *tizkernel_class;
  typedef enum tiz_kernel_population_status tiz_kernel_population_status_t;
  enum tiz_kernel_population_status
  {
    ETIZKernelFullyPopulated,
    ETIZKernelUnpopulated,
    ETIZKernelFullyUnpopulated
  };

    OMX_ERRORTYPE
    tizkernel_register_port (const void *ap_obj, OMX_PTR ap_port,
                             OMX_BOOL ais_config);

  void *tizkernel_get_port (const void *ap_obj, OMX_U32 a_pid);

  OMX_ERRORTYPE tizkernel_find_managing_port (const void *ap_obj,
                                              OMX_INDEXTYPE a_index,
                                              OMX_PTR ap_struct,
                                              OMX_PTR * app_port);

  tiz_kernel_population_status_t tizkernel_get_population_status
    (const void *ap_obj,
     OMX_U32 a_pid, OMX_BOOL * ap_may_be_fully_unpopulated);

  OMX_ERRORTYPE tizkernel_select (const void *ap_obj,
                                  OMX_U32 a_nports, tiz_pd_set_t * ap_set);

  OMX_ERRORTYPE tizkernel_claim_buffer (const void *ap_obj,
                                        OMX_U32 a_pid, OMX_U32 a_pos,
                                        OMX_BUFFERHEADERTYPE ** p_hdr);

  OMX_ERRORTYPE tizkernel_relinquish_buffer (const void *ap_obj,
                                             OMX_U32 a_pid,
                                             OMX_BUFFERHEADERTYPE * ap_hdr);

  void tizkernel_deregister_all_ports (void *ap_obj);

  void init_tizkernel (void);

#ifdef __cplusplus
}
#endif

#endif                          /* TIZKERNEL_H */
