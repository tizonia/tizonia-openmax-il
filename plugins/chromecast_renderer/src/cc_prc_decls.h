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
 * @file   cc_prc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL Chromecast renderer - base processor declarations
 * class
 *
 *
 */

#ifndef CC_PRC_DECLS_H
#define CC_PRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>

#include <tizplatform.h>

#include "tizprc_decls.h"

typedef struct cc_prc cc_prc_t;
struct cc_prc
{
  /* Object */
  const tiz_prc_t _;
  tiz_vector_t * p_hdrs_;
  tiz_vector_t * p_disabled_flags_;
  tiz_vector_t * p_port_dirs_;
  bool eos_;
};

typedef struct cc_prc_class cc_prc_class_t;
struct cc_prc_class
{
  /* Class */
  const tiz_prc_class_t _;
  OMX_BUFFERHEADERTYPE ** (*get_header_ptr) (void * ap_obj,
                                             const OMX_U32 a_pid);
  OMX_BUFFERHEADERTYPE * (*get_header) (void * ap_obj, const OMX_U32 a_pid);
  bool (*headers_available) (const void * ap_obj);
  bool (*output_headers_available) (const void * ap_obj);
  OMX_ERRORTYPE (*release_header) (void * ap_obj, const OMX_U32 a_pid);
  OMX_ERRORTYPE (*release_all_headers) (void * ap_obj);
  bool * (*get_port_disabled_ptr) (void * ap_obj, const OMX_U32 a_pid);
  bool (*is_port_disabled) (void * ap_obj, const OMX_U32 a_pid);
  bool (*is_port_enabled) (void * ap_obj, const OMX_U32 a_pid);
  bool (*is_eos) (const void * ap_obj);
  void (*update_eos_flag) (void * ap_obj, const bool flag);
  void (*update_port_disabled_flag) (void * ap_obj, const OMX_U32 a_pid,
                                     const bool flag);
};

#ifdef __cplusplus
}
#endif

#endif /* CC_PRC_DECLS_H */
