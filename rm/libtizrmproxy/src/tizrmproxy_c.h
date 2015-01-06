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
 * @file   tizrmproxy_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Resource Manager client library (c wrapper)
 *
 *
 */

#ifndef TIZRMPROXY_C_H
#define TIZRMPROXY_C_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

#include <OMX_Core.h>

#include <tizrmproxytypes.h>
#include "tizrmtypes.h"

  tizrm_error_t
    tiz_rm_proxy_init (tizrm_t * ap_rm, const OMX_STRING ap_name,
                      const OMX_UUIDTYPE * ap_uuid,
                      const OMX_PRIORITYMGMTTYPE * ap_pri,
                      const tiz_rm_proxy_callbacks_t * ap_cbacks,
                      OMX_PTR ap_data);

  tizrm_error_t tiz_rm_proxy_destroy (tizrm_t * ap_rm);

  OMX_S32 tiz_rm_proxy_version (const tizrm_t * ap_rm);

    tizrm_error_t
    tiz_rm_proxy_acquire (const tizrm_t * ap_rm, OMX_U32 rid,
                         OMX_U32 quantity);

    tizrm_error_t
    tiz_rm_proxy_release (const tizrm_t * ap_rm, OMX_U32 rid,
                         OMX_U32 quantity);

    tizrm_error_t
    tiz_rm_proxy_wait (const tizrm_t * ap_rm, OMX_U32 rid, OMX_U32 quantity);

    tizrm_error_t
    tiz_rm_proxy_cancel_wait (const tizrm_t * ap_rm, OMX_U32 rid,
                             OMX_U32 quantity);

    tizrm_error_t
    tiz_rm_proxy_preemption_conf (const tizrm_t * ap_rm, OMX_U32 rid,
                                 OMX_U32 quantity);

#ifdef __cplusplus
}
#endif

#endif                          // TIZRMPROXY_C_H
