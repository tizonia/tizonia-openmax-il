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
 * @file   tizrmproxytypes.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - Resource Manager client API types
 * 
 * 
 */

#ifndef TIZRMPROXYTYPES_H
#define TIZRMPROXYTYPES_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

#include <OMX_Types.h>

  typedef void *tizrm_t;

  /* Callback function to signal a waiting client when a resource has become
   * available */
  typedef void (*tizrm_proxy_wait_complete_f) (OMX_U32 rid, OMX_PTR ap_data);

  /* Callback function to signal a client when a previously acquired resource
   * is being preempted */
  typedef void (*tizrm_proxy_preemption_req_f) (OMX_U32 rid, OMX_PTR ap_data);

  /* Callback function to signal a client when a previously requested resource
   * has been preempted */
  typedef void (*tizrm_proxy_preemption_complete_f) (OMX_U32 rid,
                                                     OMX_PTR ap_data);

  typedef struct tizrm_proxy_callbacks_t
  {
    tizrm_proxy_wait_complete_f pf_waitend;
    tizrm_proxy_preemption_req_f pf_preempt;
    tizrm_proxy_preemption_complete_f pf_preempt_end;
  } tizrm_proxy_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif                          // TIZRMPROXYTYPES_H
