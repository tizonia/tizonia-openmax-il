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
 * @file   tizproc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - processor class declarations
 *
 *
 */

#ifndef TIZPROC_DECLS_H
#define TIZPROC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tizproc.h"
#include "tizservant_decls.h"

#include "OMX_Core.h"

  typedef struct tiz_prc tiz_prc_t;
  struct tiz_prc
  {
    /* Object */
    const tiz_srv_t _;
  };

  OMX_ERRORTYPE
  tiz_prc_super_buffers_ready (const void *class, const void *p_obj);

  typedef struct tiz_prc_class tiz_prc_class_t;
  struct tiz_prc_class
  {
    /* Class */
    const tiz_srv_class_t _;
    OMX_ERRORTYPE (*buffers_ready) (const void *p_obj);
    OMX_ERRORTYPE (*pause) (const void *p_obj);
    OMX_ERRORTYPE (*resume) (const void *p_obj);
    OMX_ERRORTYPE (*port_flush) (const void *p_obj, OMX_U32 a_pid);
    OMX_ERRORTYPE (*port_disable) (const void *p_obj, OMX_U32 a_pid);
    OMX_ERRORTYPE (*port_enable) (const void *p_obj, OMX_U32 a_pid);
    OMX_ERRORTYPE (*config_change) (const void *p_obj, OMX_U32 a_pid,
                                    OMX_INDEXTYPE a_config_idx);
    OMX_ERRORTYPE (*io_ready) (void *ap_obj,
                               tiz_event_io_t * ap_ev_io, int a_fd,
                               int a_events);
    OMX_ERRORTYPE (*timer_ready) (void *ap_obj,
                                  tiz_event_timer_t * ap_ev_timer,
                                  void *ap_arg);
    OMX_ERRORTYPE (*stat_ready) (void *ap_obj,
                                 tiz_event_stat_t * ap_ev_stat,
                                 int a_events);
  };

#ifdef __cplusplus
}
#endif

#endif                          /* TIZPROC_DECLS_H */
