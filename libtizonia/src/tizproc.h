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
 * @file   tizproc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - processor class
 *
 *
 */

#ifndef TIZPROC_H
#define TIZPROC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tizservant.h"

/* factory_new(tizproc, ...) */
  extern const void *tizproc;
  extern const void *tizproc_class;

  OMX_ERRORTYPE tizproc_buffers_ready (const void *p_obj);

  OMX_ERRORTYPE tizproc_port_flush (const void *ap_obj, OMX_U32 a_pid);

  OMX_ERRORTYPE tizproc_port_disable (const void *ap_obj, OMX_U32 a_pid);

  OMX_ERRORTYPE tizproc_port_enable (const void *ap_obj, OMX_U32 a_pid);

  void init_tizproc (void);

#ifdef __cplusplus
}
#endif

#endif                          /* TIZPROC_H */
